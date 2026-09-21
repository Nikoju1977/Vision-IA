#!/usr/bin/env python3
from __future__ import annotations

import random
import time
from typing import Any, Callable, Optional


_RETRYABLE_STATUS = {
    408,
    409,
    425,
    429,
    500,
    502,
    503,
    504,
}

_RETRYABLE_MARKERS = (
    "rate limit",
    "rate_limit",
    "too many requests",
    "overloaded",
    "capacity",
    "temporarily unavailable",
    "service unavailable",
    "timeout",
    "timed out",
    "gateway timeout",
)


class MistralProvider:
    """Adaptateur Mistral robuste pour VisionIAAgent.

    Protections anti-saturation :
    - mode JSON natif Mistral ;
    - limite explicite de taille du prompt ;
    - cadence minimale entre deux appels ;
    - retry exponentiel sur erreurs transitoires/429/5xx ;
    - jitter pour éviter de réessayer en rafale ;
    - client et horloge injectables pour les tests hors réseau.
    """

    def __init__(
        self,
        model: str = "mistral-large-latest",
        *,
        client: Optional[Any] = None,
        temperature: float = 0.2,
        max_retries: int = 5,
        base_retry_delay: float = 1.0,
        max_retry_delay: float = 20.0,
        min_interval: float = 0.35,
        max_input_chars: int = 80_000,
        sleep_fn: Callable[[float], None] = time.sleep,
        monotonic_fn: Callable[[], float] = time.monotonic,
        random_fn: Callable[[], float] = random.random,
    ):
        if not model or not model.strip():
            raise ValueError("Le modèle Mistral ne peut pas être vide.")

        if not 0.0 <= temperature <= 1.5:
            raise ValueError("temperature doit être comprise entre 0.0 et 1.5.")

        if max_retries < 0:
            raise ValueError("max_retries doit être >= 0.")

        if base_retry_delay < 0 or max_retry_delay < 0:
            raise ValueError("Les délais de retry doivent être >= 0.")

        if max_retry_delay < base_retry_delay:
            raise ValueError("max_retry_delay doit être >= base_retry_delay.")

        if min_interval < 0:
            raise ValueError("min_interval doit être >= 0.")

        if max_input_chars < 2_000:
            raise ValueError("max_input_chars doit être >= 2000.")

        self.model = model.strip()
        self.temperature = float(temperature)
        self.max_retries = int(max_retries)
        self.base_retry_delay = float(base_retry_delay)
        self.max_retry_delay = float(max_retry_delay)
        self.min_interval = float(min_interval)
        self.max_input_chars = int(max_input_chars)
        self._sleep = sleep_fn
        self._monotonic = monotonic_fn
        self._random = random_fn
        self._last_call_at: Optional[float] = None

        if client is None:
            try:
                import os
                from mistralai.client import Mistral
            except ImportError as exc:
                raise RuntimeError(
                    "Le SDK Mistral n'est pas installé. Exécute : pip install -U mistralai"
                ) from exc

            api_key = os.environ.get("MISTRAL_API_KEY")
            if not api_key:
                raise RuntimeError(
                    "MISTRAL_API_KEY est absente de l'environnement."
                )

            client = Mistral(api_key=api_key)

        self.client = client

    @staticmethod
    def _extract_status_code(exc: Exception) -> Optional[int]:
        for candidate in (
            getattr(exc, "status_code", None),
            getattr(getattr(exc, "response", None), "status_code", None),
        ):
            if isinstance(candidate, int):
                return candidate
        return None

    @classmethod
    def _is_retryable(cls, exc: Exception) -> bool:
        status = cls._extract_status_code(exc)
        if status in _RETRYABLE_STATUS:
            return True

        message = str(exc).lower()
        return any(marker in message for marker in _RETRYABLE_MARKERS)

    def _wait_for_rate_slot(self) -> None:
        if self._last_call_at is None or self.min_interval <= 0:
            return

        elapsed = self._monotonic() - self._last_call_at
        remaining = self.min_interval - elapsed

        if remaining > 0:
            self._sleep(remaining)

    def _mark_call(self) -> None:
        self._last_call_at = self._monotonic()

    def _retry_delay(self, attempt: int) -> float:
        exponential = self.base_retry_delay * (2 ** attempt)
        capped = min(self.max_retry_delay, exponential)

        # Jitter 0.85x -> 1.15x pour éviter les rafales synchronisées.
        jitter = 0.85 + (0.30 * self._random())
        return capped * jitter

    def _compact_prompt(self, system_prompt: str, prompt: str) -> tuple[str, str]:
        total = len(system_prompt) + len(prompt)
        if total <= self.max_input_chars:
            return system_prompt, prompt

        available = self.max_input_chars - len(system_prompt)
        if available < 1_000:
            raise ValueError(
                "Le system prompt est trop volumineux pour la limite Mistral configurée."
            )

        marker = (
            "\n\n[... CONTEXTE CENTRAL COMPACTÉ AUTOMATIQUEMENT "
            "POUR ÉVITER LA SATURATION MISTRAL ...]\n\n"
        )

        usable = max(1, available - len(marker))
        head_size = int(usable * 0.70)
        tail_size = usable - head_size

        compacted = (
            prompt[:head_size]
            + marker
            + prompt[-tail_size:]
        )

        return system_prompt, compacted

    def __call__(self, system_prompt: str, prompt: str) -> str:
        system_prompt, prompt = self._compact_prompt(system_prompt, prompt)

        last_error: Optional[Exception] = None

        for attempt in range(self.max_retries + 1):
            self._wait_for_rate_slot()

            try:
                response = self.client.chat.complete(
                    model=self.model,
                    messages=[
                        {
                            "role": "system",
                            "content": system_prompt,
                        },
                        {
                            "role": "user",
                            "content": prompt,
                        },
                    ],
                    response_format={
                        "type": "json_object",
                    },
                    temperature=self.temperature,
                )

                self._mark_call()

                try:
                    output = response.choices[0].message.content
                except (AttributeError, IndexError, TypeError) as exc:
                    raise RuntimeError(
                        "Réponse Mistral inattendue : aucun contenu exploitable."
                    ) from exc

                if not isinstance(output, str) or not output.strip():
                    raise RuntimeError(
                        "Mistral n'a renvoyé aucun objet JSON exploitable."
                    )

                return output

            except Exception as exc:
                self._mark_call()
                last_error = exc

                if attempt >= self.max_retries or not self._is_retryable(exc):
                    raise

                self._sleep(self._retry_delay(attempt))

        raise RuntimeError(
            f"Mistral indisponible après retries: {last_error}"
        )


def create_mistral_agent(
    *,
    model: str = "mistral-large-latest",
    max_iterations: int = 5,
    timeout_sec: float = 10.0,
    memory_mb: int = 256,
    temperature: float = 0.2,
    max_retries: int = 5,
    min_interval: float = 0.35,
    max_input_chars: int = 80_000,
    client: Optional[Any] = None,
):
    from vision_ia_agent import VisionIAAgent

    provider = MistralProvider(
        model=model,
        client=client,
        temperature=temperature,
        max_retries=max_retries,
        min_interval=min_interval,
        max_input_chars=max_input_chars,
    )

    return VisionIAAgent(
        provider,
        max_iterations=max_iterations,
        timeout_sec=timeout_sec,
        memory_mb=memory_mb,
    )
