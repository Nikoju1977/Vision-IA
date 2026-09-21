#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json

from long_document_processor import MistralLongDocumentProcessor
from mistral_provider import MistralProvider


def main():
    parser = argparse.ArgumentParser(
        description="Vision-IA · dépouillement Mistral anti-saturation"
    )

    parser.add_argument("document")
    parser.add_argument(
        "--objective",
        default="Dépouiller intégralement le document",
    )
    parser.add_argument(
        "--model",
        default="mistral-large-latest",
    )
    parser.add_argument(
        "--chunk-chars",
        type=int,
        default=12000,
    )
    parser.add_argument(
        "--overlap-chars",
        type=int,
        default=600,
    )
    parser.add_argument(
        "--batch-size",
        type=int,
        default=4,
    )
    parser.add_argument(
        "--min-interval",
        type=float,
        default=0.5,
    )
    parser.add_argument(
        "--max-retries",
        type=int,
        default=6,
    )

    args = parser.parse_args()

    provider = MistralProvider(
        model=args.model,
        max_retries=args.max_retries,
        min_interval=args.min_interval,
        max_input_chars=80000,
    )

    processor = MistralLongDocumentProcessor(
        provider,
        chunk_chars=args.chunk_chars,
        overlap_chars=args.overlap_chars,
        reduction_batch_size=args.batch_size,
    )

    def progress(current, total):
        print(
            f"[Vision-IA] fragment {current}/{total}",
            flush=True,
        )

    result = processor.analyze_file(
        args.document,
        objective=args.objective,
        progress=progress,
    )

    output = {
        "resume": result.digest.resume,
        "faits": list(result.digest.faits),
        "anomalies": list(result.digest.anomalies),
        "references": list(result.digest.references),
        "chunks_processed": result.chunks_processed,
        "reduction_rounds": result.reduction_rounds,
        "provider_calls": result.provider_calls,
    }

    print(
        json.dumps(
            output,
            ensure_ascii=False,
            indent=2,
        )
    )


if __name__ == "__main__":
    main()
