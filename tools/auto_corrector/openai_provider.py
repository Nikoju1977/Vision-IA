#!/usr/bin/env python3
from __future__ import annotations

from typing import Any, Optional


VISION_IA_RESPONSE_SCHEMA = {
    "type": "object",
    "properties": {
        "analyse": {
            "type": "string",
            "description": "Diagnostic bref de la solution ou de l'erreur identifiée, sans chaîne de raisonnement détaillée.",
        },
        "code": {
            "type": "string",
            "description": "Programme Python complet, sans balises Markdown.",
        },
    },
    "required": ["analyse", "code"],
    "additionalProperties": False,
}


class OpenAIProvider:
    """Adaptateur OpenAI pour VisionIAAgent.

    La clé est lue par le SDK depuis OPENAI_API_KEY.
    Le client peut être injecté pour les tests afin qu'aucun appel réseau
    ne soit nécessaire en CI.
    """

    def __init__(
        self,
        model: str = "gpt-5.6",
        *,
        client: Optional[Any] = None,
    ):
        if not model or not model.strip():
            raise ValueError("Le modèle OpenAI ne peut pas être vide.")

        self.model = model.strip()

        if client is None:
            try:
                from openai import OpenAI
            except ImportError as exc:
                raise RuntimeError(
                    "Le SDK OpenAI n'est pas installé. Exécute : pip install -U openai"
                ) from exc

            client = OpenAI()

        self.client = client

    def __call__(self, system_prompt: str, prompt: str) -> str:
        response = self.client.responses.create(
            model=self.model,
            instructions=system_prompt,
            input=prompt,
            text={
                "format": {
                    "type": "json_schema",
                    "name": "vision_ia_agent_result",
                    "description": "Réponse structurée de l'agent de correction Vision-IA.",
                    "strict": True,
                    "schema": VISION_IA_RESPONSE_SCHEMA,
                }
            },
        )

        output = getattr(response, "output_text", None)

        if not isinstance(output, str) or not output.strip():
            raise RuntimeError("L'API OpenAI n'a renvoyé aucun texte structuré.")

        return output


def create_openai_agent(
    *,
    model: str = "gpt-5.6",
    max_iterations: int = 5,
    timeout_sec: float = 10.0,
    memory_mb: int = 256,
    client: Optional[Any] = None,
):
    from vision_ia_agent import VisionIAAgent

    provider = OpenAIProvider(model=model, client=client)

    return VisionIAAgent(
        provider,
        max_iterations=max_iterations,
        timeout_sec=timeout_sec,
        memory_mb=memory_mb,
    )
