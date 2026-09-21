#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np


def check_geometric_tolerance(
    current_points,
    last_points,
    max_delta=0.005,
):
    if last_points is None:
        return True

    current = np.asarray(current_points, dtype=np.float32)
    previous = np.asarray(last_points, dtype=np.float32)

    if current.shape != previous.shape:
        return False

    deviation = float(np.max(np.abs(current - previous)))
    return deviation < max_delta


def batch_process_vfx(
    video_folder,
    output_alembic,
    model_path="models/face_landmark.tflite",
    fps=30.0,
):
    try:
        import vision_ia
    except ImportError as exc:
        raise RuntimeError(
            "Le module natif vision_ia n'est pas installé. "
            "Construis avec -DVISIONIA_BUILD_VFX_BINDINGS=ON."
        ) from exc

    folder = Path(video_folder)

    if not folder.is_dir():
        raise FileNotFoundError(
            f"Dossier image introuvable : {folder}"
        )

    frames = sorted(
        path
        for path in folder.iterdir()
        if path.is_file()
        and path.suffix.lower() in {
            ".png", ".jpg", ".jpeg", ".webp", ".bmp"
        }
    )

    if not frames:
        raise RuntimeError("Aucune frame image à traiter.")

    engine = vision_ia.Engine(model_path)
    exporter = vision_ia.AlembicExporter(output_alembic, fps)

    last_valid_points = None
    point_ids = list(range(468))

    try:
        for frame_path in frames:
            raw_landmarks = engine.process_frame(str(frame_path))

            if len(raw_landmarks) != 468:
                raise RuntimeError(
                    f"Le moteur a retourné {len(raw_landmarks)} landmarks au lieu de 468."
                )

            if check_geometric_tolerance(
                raw_landmarks,
                last_valid_points,
            ):
                exporter.write_frame(
                    raw_landmarks,
                    point_ids,
                )
                last_valid_points = raw_landmarks

            elif last_valid_points is not None:
                exporter.write_frame(
                    last_valid_points,
                    point_ids,
                )
    finally:
        exporter.close()


def main():
    parser = argparse.ArgumentParser(
        description="Vision-IA VFX batch exporter"
    )

    parser.add_argument("input", help="Dossier contenant les frames source")
    parser.add_argument("output", help="Fichier Alembic .abc")
    parser.add_argument(
        "--model",
        default="models/face_landmark.tflite",
    )
    parser.add_argument(
        "--fps",
        type=float,
        default=30.0,
    )

    args = parser.parse_args()

    batch_process_vfx(
        args.input,
        args.output,
        args.model,
        args.fps,
    )


if __name__ == "__main__":
    main()
