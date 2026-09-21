#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import zipfile
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Callable, Iterable, Optional
from xml.etree import ElementTree


Provider = Callable[[str, str], str]


SYSTEM_PROMPT = """
Tu analyses de très longs documents pour Vision-IA.

Tu ne dois jamais inventer une information absente du fragment fourni.
Tu dois conserver les noms, nombres, dates et termes techniques utiles.

Réponds uniquement avec un objet JSON valide ayant exactement ces clés :
{
  "resume": "synthèse concise et factuelle",
  "faits": ["faits importants"],
  "anomalies": ["contradictions, ambiguïtés ou problèmes détectés"],
  "references": ["repères de pages, sections, chapitres ou citations courtes"]
}

Contraintes :
- 12 éléments maximum par liste ;
- aucun Markdown ;
- aucune information supposée ;
- indique clairement les incertitudes.
""".strip()


@dataclass(frozen=True)
class DocumentDigest:
    resume: str
    faits: tuple[str, ...]
    anomalies: tuple[str, ...]
    references: tuple[str, ...]


@dataclass(frozen=True)
class LongDocumentResult:
    digest: DocumentDigest
    chunks_processed: int
    reduction_rounds: int
    provider_calls: int


def _unique(items: Iterable[str], limit: int = 12) -> tuple[str, ...]:
    output: list[str] = []
    seen: set[str] = set()

    for item in items:
        normalized = re.sub(r"\s+", " ", str(item)).strip()
        if not normalized:
            continue

        key = normalized.casefold()
        if key in seen:
            continue

        seen.add(key)
        output.append(normalized)

        if len(output) >= limit:
            break

    return tuple(output)


def _parse_digest(raw: str) -> DocumentDigest:
    try:
        data = json.loads(raw)
    except json.JSONDecodeError as exc:
        raise ValueError(f"Réponse document JSON invalide: {exc}") from exc

    expected = {"resume", "faits", "anomalies", "references"}

    if not isinstance(data, dict) or set(data.keys()) != expected:
        raise ValueError(
            "La réponse document doit contenir exactement "
            "resume, faits, anomalies et references."
        )

    resume = data["resume"]

    if not isinstance(resume, str) or not resume.strip():
        raise ValueError("Le champ resume doit être une chaîne non vide.")

    lists: dict[str, tuple[str, ...]] = {}

    for key in ("faits", "anomalies", "references"):
        value = data[key]

        if not isinstance(value, list) or not all(
            isinstance(item, str) for item in value
        ):
            raise ValueError(f"Le champ {key} doit être une liste de chaînes.")

        lists[key] = _unique(value)

    return DocumentDigest(
        resume=re.sub(r"\s+", " ", resume).strip(),
        faits=lists["faits"],
        anomalies=lists["anomalies"],
        references=lists["references"],
    )


def split_long_text(
    text: str,
    *,
    chunk_chars: int = 12_000,
    overlap_chars: int = 600,
) -> list[str]:
    if chunk_chars < 2_000:
        raise ValueError("chunk_chars doit être >= 2000.")

    if overlap_chars < 0 or overlap_chars >= chunk_chars:
        raise ValueError(
            "overlap_chars doit être >= 0 et strictement inférieur à chunk_chars."
        )

    normalized = text.replace("\r\n", "\n").replace("\r", "\n").strip()

    if not normalized:
        return []

    if len(normalized) <= chunk_chars:
        return [normalized]

    chunks: list[str] = []
    start = 0
    length = len(normalized)

    while start < length:
        hard_end = min(length, start + chunk_chars)
        end = hard_end

        if hard_end < length:
            search_start = start + int(chunk_chars * 0.60)
            candidates = [
                normalized.rfind("\n\n", search_start, hard_end),
                normalized.rfind("\n", search_start, hard_end),
                normalized.rfind(". ", search_start, hard_end),
            ]
            best = max(candidates)

            if best > start:
                end = best + (2 if normalized[best:best + 2] == ". " else 0)

        chunk = normalized[start:end].strip()

        if chunk:
            chunks.append(chunk)

        if end >= length:
            break

        next_start = max(0, end - overlap_chars)

        if next_start <= start:
            next_start = end

        start = next_start

    return chunks


def _docx_text(path: Path) -> str:
    with zipfile.ZipFile(path) as archive:
        xml = archive.read("word/document.xml")

    root = ElementTree.fromstring(xml)
    namespace = "{http://schemas.openxmlformats.org/wordprocessingml/2006/main}"

    paragraphs: list[str] = []

    for paragraph in root.iter(namespace + "p"):
        parts = [
            node.text or ""
            for node in paragraph.iter(namespace + "t")
        ]
        value = "".join(parts).strip()

        if value:
            paragraphs.append(value)

    return "\n\n".join(paragraphs)


def _pdf_text(path: Path) -> str:
    try:
        from pypdf import PdfReader
    except ImportError as exc:
        raise RuntimeError(
            "Lecture PDF indisponible. Installe pypdf : pip install -U pypdf"
        ) from exc

    reader = PdfReader(str(path))
    pages: list[str] = []

    for index, page in enumerate(reader.pages, start=1):
        text = page.extract_text() or ""
        pages.append(
            f"--- PAGE {index} ---\n{text.strip()}"
        )

    return "\n\n".join(pages)


def load_document_text(path: str | Path) -> str:
    file_path = Path(path)

    if not file_path.is_file():
        raise FileNotFoundError(file_path)

    suffix = file_path.suffix.lower()

    if suffix in {".txt", ".md", ".py", ".json", ".csv"}:
        return file_path.read_text(
            encoding="utf-8",
            errors="replace",
        )

    if suffix == ".docx":
        return _docx_text(file_path)

    if suffix == ".pdf":
        return _pdf_text(file_path)

    raise ValueError(
        f"Format non pris en charge pour l'analyse longue : {suffix or '(sans extension)'}"
    )


class MistralLongDocumentProcessor:
    def __init__(
        self,
        provider: Provider,
        *,
        chunk_chars: int = 12_000,
        overlap_chars: int = 600,
        reduction_batch_size: int = 4,
    ):
        if not callable(provider):
            raise TypeError("provider doit être callable.")

        if reduction_batch_size < 2:
            raise ValueError("reduction_batch_size doit être >= 2.")

        self.provider = provider
        self.chunk_chars = chunk_chars
        self.overlap_chars = overlap_chars
        self.reduction_batch_size = reduction_batch_size

    def _call_digest(self, prompt: str) -> DocumentDigest:
        raw = self.provider(SYSTEM_PROMPT, prompt)
        return _parse_digest(raw)

    def _analyze_chunk(
        self,
        chunk: str,
        index: int,
        total: int,
        objective: str,
    ) -> DocumentDigest:
        prompt = (
            f"Objectif global : {objective}\n"
            f"Fragment {index}/{total}.\n"
            "Analyse uniquement ce fragment. "
            "Ne prétends pas connaître les fragments absents.\n\n"
            f"{chunk}"
        )
        return self._call_digest(prompt)

    def _reduce_group(
        self,
        digests: list[DocumentDigest],
        objective: str,
        round_index: int,
        group_index: int,
    ) -> DocumentDigest:
        compact = [
            asdict(digest)
            for digest in digests
        ]

        prompt = (
            f"Objectif global : {objective}\n"
            f"Fusion hiérarchique, niveau {round_index}, lot {group_index}.\n"
            "Fusionne les analyses ci-dessous sans ajouter de faits externes. "
            "Déduplique les faits et conserve les contradictions signalées.\n\n"
            + json.dumps(
                compact,
                ensure_ascii=False,
                separators=(",", ":"),
            )
        )

        return self._call_digest(prompt)

    def analyze_text(
        self,
        text: str,
        *,
        objective: str = "Dépouiller intégralement le document",
        progress: Optional[Callable[[int, int], None]] = None,
    ) -> LongDocumentResult:
        chunks = split_long_text(
            text,
            chunk_chars=self.chunk_chars,
            overlap_chars=self.overlap_chars,
        )

        if not chunks:
            raise ValueError("Le document est vide.")

        digests: list[DocumentDigest] = []
        provider_calls = 0

        for index, chunk in enumerate(chunks, start=1):
            digest = self._analyze_chunk(
                chunk,
                index,
                len(chunks),
                objective,
            )
            provider_calls += 1
            digests.append(digest)

            if progress:
                progress(index, len(chunks))

        reduction_rounds = 0

        while len(digests) > 1:
            reduction_rounds += 1
            reduced: list[DocumentDigest] = []

            for start in range(
                0,
                len(digests),
                self.reduction_batch_size,
            ):
                group = digests[
                    start:start + self.reduction_batch_size
                ]

                if len(group) == 1:
                    reduced.append(group[0])
                    continue

                reduced.append(
                    self._reduce_group(
                        group,
                        objective,
                        reduction_rounds,
                        (start // self.reduction_batch_size) + 1,
                    )
                )
                provider_calls += 1

            digests = reduced

        return LongDocumentResult(
            digest=digests[0],
            chunks_processed=len(chunks),
            reduction_rounds=reduction_rounds,
            provider_calls=provider_calls,
        )

    def analyze_file(
        self,
        path: str | Path,
        *,
        objective: str = "Dépouiller intégralement le document",
        progress: Optional[Callable[[int, int], None]] = None,
    ) -> LongDocumentResult:
        return self.analyze_text(
            load_document_text(path),
            objective=objective,
            progress=progress,
        )
