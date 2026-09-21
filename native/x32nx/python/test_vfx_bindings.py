#!/usr/bin/env python3
from __future__ import annotations

import math
import os
import sys
import tempfile
from pathlib import Path

import cv2
import numpy as np

import vision_ia


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: test_vfx_bindings.py MODEL.tflite")

    model_path = sys.argv[1]

    with tempfile.TemporaryDirectory(prefix="vision-ia-vfx-") as tmp:
        tmp_path = Path(tmp)

        image = np.zeros((256, 256, 3), dtype=np.uint8)
        yy, xx = np.mgrid[0:256, 0:256]
        image[..., 0] = xx.astype(np.uint8)
        image[..., 1] = yy.astype(np.uint8)
        image[..., 2] = ((xx + yy) // 2).astype(np.uint8)

        image_path = tmp_path / "synthetic.png"
        if not cv2.imwrite(str(image_path), image):
            raise RuntimeError("Impossible d'écrire l'image de smoke test.")

        engine = vision_ia.Engine(model_path)
        points = engine.process_frame(str(image_path))

        assert len(points) == 468, len(points)
        assert engine.input_width > 0
        assert engine.input_height > 0

        for point in points:
            assert len(point) == 3
            assert all(math.isfinite(float(value)) for value in point)

        alembic_path = tmp_path / "landmarks.abc"
        exporter = vision_ia.AlembicExporter(str(alembic_path), 30.0)

        ids = list(range(468))
        exporter.write_frame(points, ids)
        exporter.write_frame(points, ids)

        assert exporter.frames_written == 2
        exporter.close()

        assert alembic_path.exists()
        assert alembic_path.stat().st_size > 128

        print(
            "VFX bindings PASS:",
            f"{len(points)} landmarks",
            f"{alembic_path.stat().st_size} bytes Alembic",
        )


if __name__ == "__main__":
    main()
