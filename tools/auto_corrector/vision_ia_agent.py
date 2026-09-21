#!/usr/bin/env python3
from __future__ import annotations

import json
from dataclasses import dataclass
from hashlib import sha256
from typing import Callable, Optional

from auto_corrector import ExecutionResult, run_generated_code, validate_python


Provider = Callable[[str, str], str]


@dataclass(frozen=True)
class AgentIteration:
    index: int
    analysis: str
    code_hash: str
    execution: ExecutionResult


@dataclass(frozen=True)
class AgentOutcome:
    success: bool
    code: Optional[str]
    iterations: tuple[AgentIteration, ...]
    message: str


class VisionIAAgent:
    SYSTEM_PROMPT = """
Tu es un agent logiciel expert en Python.
Ton objectif est de générer ou corriger du code Python.

CONTRAINTE DE SORTIE ABSOLUE :
réponds uniquement avec un objet JSON valide, sans Markdown ni texte autour,
et avec exactement ces deux clés :
{
  "analyse": "explication brève de la logique ou de l'erreur identifiée",
  "code": "code Python pur"
}

Le champ analyse doit être une chaîne.
Le champ code doit être une chaîne contenant un programme Python complet.
""".strip()

    def __init__(
        self,
        provider: Provider,
        *,
        max_iterations: int = 5,
        timeout_sec: float = 10.0,
        memory_mb: int = 256,
    ):
        if not callable(provider):
            raise TypeError("provider doit être une fonction (system_prompt, prompt) -> str")
        if max_iterations < 1:
            raise ValueError("max_iterations doit être >= 1")
        if timeout_sec <= 0:
            raise ValueError("timeout_sec doit être > 0")
        if memory_mb < 32:
            raise ValueError("memory_mb doit être >= 32")

        self.provider = provider
        self.max_iterations = max_iterations
        self.timeout_sec = timeout_sec
        self.memory_mb = memory_mb

    @staticmethod
    def _parse_strict_json(raw: str) -> tuple[str, str]:
        if not isinstance(raw, str):
            raise ValueError("La réponse IA n'est pas une chaîne JSON.")

        try:
            data = json.loads(raw)
        except json.JSONDecodeError as exc:
            raise ValueError(f"JSON invalide: {exc}") from exc

        if not isinstance(data, dict):
            raise ValueError("La réponse JSON doit être un objet.")

        if set(data.keys()) != {"analyse", "code"}:
            raise ValueError("Le JSON doit contenir exactement les clés 'analyse' et 'code'.")

        analysis = data["analyse"]
        code = data["code"]

        if not isinstance(analysis, str) or not isinstance(code, str):
            raise ValueError("'analyse' et 'code' doivent être des chaînes.")

        if not code.strip():
            raise ValueError("Le champ 'code' est vide.")

        return analysis.strip(), code.strip()

    def _call_provider(self, prompt: str) -> str:
        return self.provider(self.SYSTEM_PROMPT, prompt)

    def solve(self, objective: str) -> AgentOutcome:
        if not objective or not objective.strip():
            raise ValueError("L'objectif ne peut pas être vide.")

        prompt = f"Objectif : {objective.strip()}"
        iterations: list[AgentIteration] = []
        seen_hashes: set[str] = set()

        for attempt in range(1, self.max_iterations + 1):
            try:
                raw = self._call_provider(prompt)
                analysis, code = self._parse_strict_json(raw)
            except Exception as exc:
                prompt = (
                    "ERREUR DE PROTOCOLE : la réponse précédente n'était pas conforme. "
                    f"Détail : {exc}. "
                    "Réponds uniquement avec l'objet JSON strict demandé."
                )
                continue

            digest = sha256(code.encode("utf-8")).hexdigest()

            if digest in seen_hashes:
                repeated = ExecutionResult(
                    returncode=4,
                    stdout="",
                    stderr="Même code défaillant renvoyé : arrêt anti-boucle.",
                )
                iterations.append(
                    AgentIteration(attempt, analysis, digest, repeated)
                )
                return AgentOutcome(
                    False,
                    None,
                    tuple(iterations),
                    "Arrêt anti-boucle : code identique reproduit.",
                )

            seen_hashes.add(digest)

            syntax_error = validate_python(code)
            if syntax_error:
                static_failure = ExecutionResult(
                    returncode=2,
                    stdout="",
                    stderr=syntax_error,
                )
                iterations.append(
                    AgentIteration(attempt, analysis, digest, static_failure)
                )
                prompt = (
                    "Le code précédent est syntaxiquement invalide.\n"
                    f"Erreur AST : {syntax_error}\n"
                    "Corrige le programme et renvoie uniquement le JSON strict."
                )
                continue

            execution = run_generated_code(
                code,
                timeout_seconds=self.timeout_sec,
                memory_mb=self.memory_mb,
            )

            iterations.append(
                AgentIteration(attempt, analysis, digest, execution)
            )

            if execution.returncode == 0:
                return AgentOutcome(
                    True,
                    code,
                    tuple(iterations),
                    f"Objectif résolu en {attempt} itération(s).",
                )

            if execution.timed_out:
                failure = "TIMEOUT : le programme a dépassé la durée autorisée."
            else:
                failure = (
                    f"Code retour : {execution.returncode}\n"
                    f"stderr :\n{execution.stderr}"
                )

            prompt = (
                f"Objectif initial : {objective.strip()}\n\n"
                "Le code précédent a échoué pendant l'exécution.\n"
                f"{failure}\n\n"
                "Corrige uniquement ce qui est nécessaire, conserve le comportement correct "
                "et renvoie uniquement le JSON strict demandé."
            )

        return AgentOutcome(
            False,
            None,
            tuple(iterations),
            f"Échec après {self.max_iterations} itérations.",
        )


VisionIA_Agent = VisionIAAgent
