#!/usr/bin/env python3
from __future__ import annotations

import ast
import os
import re
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from hashlib import sha256
from typing import Callable, Optional

CodeGenerator = Callable[[str], str]

@dataclass(frozen=True)
class ExecutionResult:
    returncode: int
    stdout: str
    stderr: str
    timed_out: bool = False

@dataclass(frozen=True)
class Attempt:
    index: int
    code_hash: str
    result: ExecutionResult

@dataclass(frozen=True)
class CorrectionResult:
    success: bool
    code: Optional[str]
    attempts: tuple[Attempt, ...]
    message: str

FENCE_RE = re.compile(r"\\x60\\x60\\x60(?:python|py)?\\s*\\n(.*?)\\n\\x60\\x60\\x60", re.IGNORECASE | re.DOTALL)

def extract_code(response_text: str) -> str:
    if not isinstance(response_text, str):
        raise TypeError("La réponse IA doit être une chaîne.")
    match = FENCE_RE.search(response_text)
    return (match.group(1) if match else response_text).strip()

def validate_python(code: str) -> Optional[str]:
    try:
        ast.parse(code, filename="<vision-ia-generated>")
        return None
    except SyntaxError as exc:
        line = exc.text.strip() if exc.text else ""
        return f"SyntaxError ligne {exc.lineno}, colonne {exc.offset}: {exc.msg}. {line}"

def _limited_preexec(cpu_seconds: int, memory_mb: int):
    def apply_limits():
        try:
            import resource
            resource.setrlimit(resource.RLIMIT_CPU, (cpu_seconds, cpu_seconds))
            memory_bytes = memory_mb * 1024 * 1024
            resource.setrlimit(resource.RLIMIT_AS, (memory_bytes, memory_bytes))
            resource.setrlimit(resource.RLIMIT_FSIZE, (8 * 1024 * 1024, 8 * 1024 * 1024))
            resource.setrlimit(resource.RLIMIT_NOFILE, (64, 64))
        except Exception:
            pass
    return apply_limits

def run_generated_code(
    code: str,
    *,
    timeout_seconds: float = 10.0,
    memory_mb: int = 256,
    max_output_chars: int = 20000,
) -> ExecutionResult:
    syntax_error = validate_python(code)
    if syntax_error:
        return ExecutionResult(2, "", syntax_error)

    with tempfile.TemporaryDirectory(prefix="vision-ia-agent-") as tmp:
        script_path = os.path.join(tmp, "generated.py")
        with open(script_path, "w", encoding="utf-8", newline="\\n") as handle:
            handle.write(code)
            handle.write("\\n")

        env = {
            "PATH": os.environ.get("PATH", ""),
            "LANG": os.environ.get("LANG", "C.UTF-8"),
            "LC_ALL": os.environ.get("LC_ALL", "C.UTF-8"),
            "PYTHONIOENCODING": "utf-8",
            "PYTHONDONTWRITEBYTECODE": "1",
        }

        kwargs = {
            "args": [sys.executable, "-I", "-B", script_path],
            "cwd": tmp,
            "env": env,
            "capture_output": True,
            "text": True,
            "timeout": timeout_seconds,
        }

        if os.name == "posix":
            kwargs["preexec_fn"] = _limited_preexec(max(1, int(timeout_seconds)), memory_mb)

        try:
            process = subprocess.run(**kwargs)
            return ExecutionResult(
                process.returncode,
                process.stdout[-max_output_chars:],
                process.stderr[-max_output_chars:],
            )
        except subprocess.TimeoutExpired as exc:
            stdout = exc.stdout or ""
            stderr = exc.stderr or ""
            if isinstance(stdout, bytes):
                stdout = stdout.decode("utf-8", errors="replace")
            if isinstance(stderr, bytes):
                stderr = stderr.decode("utf-8", errors="replace")
            return ExecutionResult(
                124,
                stdout[-max_output_chars:],
                (stderr + "\\nTimeout d'exécution.").strip()[-max_output_chars:],
                True,
            )

def _correction_prompt(objective: str, code: str, result: ExecutionResult) -> str:
    failure = (
        "Le programme a dépassé la durée maximale autorisée."
        if result.timed_out
        else f"Code retour: {result.returncode}\\nErreur:\\n{result.stderr}"
    )
    return (
        f"Objectif:\\n{objective}\\n\\n"
        f"Code précédent:\\n{code}\\n\\n"
        f"Résultat d'exécution:\\n{failure}\\n\\n"
        "Corrige uniquement les causes du problème. "
        "Renvoie uniquement le code Python corrigé dans un bloc markdown."
    )

def auto_correct(
    objective: str,
    generator: CodeGenerator,
    *,
    max_iterations: int = 5,
    timeout_seconds: float = 10.0,
    memory_mb: int = 256,
) -> CorrectionResult:
    if not objective.strip():
        raise ValueError("L'objectif ne peut pas être vide.")
    if max_iterations < 1:
        raise ValueError("max_iterations doit être >= 1.")

    prompt = (
        "Écris un script Python pour l'objectif suivant. "
        "Renvoie uniquement le code dans un bloc markdown.\\n\\n"
        f"Objectif: {objective}"
    )

    history: list[Attempt] = []
    seen_hashes: set[str] = set()

    for iteration in range(1, max_iterations + 1):
        response = generator(prompt)
        if response is None:
            result = ExecutionResult(3, "", "Le fournisseur IA n'a renvoyé aucune réponse.")
            history.append(Attempt(iteration, "", result))
            return CorrectionResult(False, None, tuple(history), result.stderr)

        code = extract_code(response)
        digest = sha256(code.encode("utf-8")).hexdigest()

        if digest in seen_hashes:
            result = ExecutionResult(4, "", "Même code défaillant renvoyé : arrêt anti-boucle.")
            history.append(Attempt(iteration, digest, result))
            return CorrectionResult(False, None, tuple(history), result.stderr)

        seen_hashes.add(digest)
        result = run_generated_code(
            code,
            timeout_seconds=timeout_seconds,
            memory_mb=memory_mb,
        )
        history.append(Attempt(iteration, digest, result))

        if result.returncode == 0:
            return CorrectionResult(True, code, tuple(history), f"Succès après {iteration} tentative(s).")

        prompt = _correction_prompt(objective, code, result)

    return CorrectionResult(False, None, tuple(history), f"Échec après {max_iterations} tentatives.")

def boucle_auto_correction(
    objectif: str,
    appeler_api_ia: CodeGenerator,
    max_iterations: int = 5,
) -> Optional[str]:
    result = auto_correct(
        objectif,
        appeler_api_ia,
        max_iterations=max_iterations,
    )
    for attempt in result.attempts:
        print(f"Tentative {attempt.index}/{max_iterations}")
        if attempt.result.stdout:
            print("stdout:\\n", attempt.result.stdout)
        if attempt.result.stderr:
            print("stderr:\\n", attempt.result.stderr)
    print(result.message)
    return result.code
