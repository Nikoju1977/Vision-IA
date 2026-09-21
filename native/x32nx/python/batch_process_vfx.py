#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
from pathlib import Path

import numpy as np


def check_geometric_tolerance(
    current_points,
    last_points,
    max_delta=0.005,
):
    if last_points is None:
        return True

    current = np.asarray(
        current_points,
        dtype=np.float32,
    )

    previous = np.asarray(
        last_points,
        dtype=np.float32,
    )

    if current.shape != previous.shape:
        return False

    deviation = float(
        np.max(
            np.abs(
                current - previous
            )
        )
    )

    return deviation < max_delta


def batch_process_vfx(
    video_folder,
    output_alembic,
    model_path="models/face_landmark.tflite",
):
    try:
        import vision_ia
    except ImportError as exc:
        raise RuntimeError(
            "Le module natif optionnel vision_ia n'est pas installé. "
            "Construis d'abord les bindings TFLite/Alembic."
        ) from exc

    folder = Path(video_folder)

    if not folder.is_dir():
        raise FileNotFoundError(
            f"Dossier image introuvable : {folder}"
        )

    engine = vision_ia.Engine(
        model_path
    )

    exporter = vision_ia.AlembicExporter(
        output_alembic
    )

    face_indices = list(
        range(468 * 3)
    )

    last_valid_points = None

    frames = sorted(
        path
        for path in folder.iterdir()
        if path.is_file()
    )

    if not frames:
        raise RuntimeError(
            "Aucune frame à traiter."
        )

    for frame_path in frames:
        raw_landmarks = engine.process_frame(
            str(frame_path)
        )

        if check_geometric_tolerance(
            raw_landmarks,
            last_valid_points,
        ):
            exporter.write_frame(
                raw_landmarks,
                face_indices,
            )

            last_valid_points = raw_landmarks

        elif last_valid_points is not None:
            exporter.write_frame(
                last_valid_points,
                face_indices,
            )


def main():
    parser = argparse.ArgumentParser(
        description="Vision-IA VFX batch exporter"
    )

    parser.add_argument(
        "input",
        help="Dossier contenant les frames source",
    )

    parser.add_argument(
        "output",
        help="Fichier Alembic .abc",
    )

    parser.add_argument(
        "--model",
        default="models/face_landmark.tflite",
    )

    args = parser.parse_args()

    batch_process_vfx(
        args.input,
        args.output,
        args.model,
    )


if __name__ == "__main__":
    main()
