#!/usr/bin/env python3
from __future__ import annotations

import argparse
import pathlib
import subprocess
import sys


HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parent


def run(command: list[str]) -> None:
    subprocess.run(command, check=True)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Vision-IA X32-NX Core Console"
    )

    subparsers = parser.add_subparsers(
        dest="command",
        required=True,
    )

    subparsers.add_parser(
        "audit",
        help="Tests unitaires et AMDEC",
    )

    studio = subparsers.add_parser(
        "studio",
        help="Nœud UDP temps réel",
    )

    studio.add_argument(
        "--port",
        type=int,
        default=8001,
    )

    export = subparsers.add_parser(
        "export",
        help="Export métrologique Alembic via bindings optionnels",
    )

    export.add_argument(
        "--input",
        required=True,
    )

    export.add_argument(
        "--output",
        required=True,
    )

    args = parser.parse_args()

    binary_dir = ROOT / "build"

    if args.command == "audit":
        run([
            str(binary_dir / "run_fec_tests"),
        ])

        run([
            str(binary_dir / "run_component_tests"),
        ])

    elif args.command == "studio":
        run([
            str(binary_dir / "vision_ia_node"),
            "--port",
            str(args.port),
        ])

    elif args.command == "export":
        run([
            sys.executable,
            str(HERE / "batch_process_vfx.py"),
            args.input,
            args.output,
        ])


if __name__ == "__main__":
    main()
