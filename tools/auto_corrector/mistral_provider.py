#!/usr/bin/env python3
from __future__ import annotations

from typing import Any, Optional


class MistralProvider:
    """Adaptateur Mistral pour VisionIAAgent.

    - Lit MISTRAL_API_KEY via l'environnement lorsque le client n'est pas injecté.
    - Utilise le mode JSON de Mistral.
    - Le schéma exact analyse/code est ensuite vérifié par VisionIAAgent.
    - Un client peut être injecté pour les tests afin d'éviter tout appel réseau en CI.
    """

    def __init__(
        self,
        model: str = "mistral-large-latest",
        *,
        client: Optional[Any] = None,
        temperature: float = 0.2,
    ):
        if not model or not model.strip():
            raise ValueError("Le modèle Mistral ne peut pas être vide.")

        if not 0.0 <= temperature <= 1.5:
            raise ValueError("temperature doit être comprise entre 0.0 et 1.5.")

        self.model = model.strip()
        self.temperature = float(temperature)

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

    def __call__(self, system_prompt: str, prompt: str) -> str:
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


def create_mistral_agent(
    *,
    model: str = "mistral-large-latest",
    max_iterations: int = 5,
    timeout_sec: float = 10.0,
    memory_mb: int = 256,
    temperature: float = 0.2,
    client: Optional[Any] = None,
):
    from vision_ia_agent import VisionIAAgent

    provider = MistralProvider(
        model=model,
        client=client,
        temperature=temperature,
    )

    return VisionIAAgent(
        provider,
        max_iterations=max_iterations,
        timeout_sec=timeout_sec,
        memory_mb=memory_mb,
    )
