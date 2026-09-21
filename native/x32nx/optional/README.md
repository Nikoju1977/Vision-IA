# Bindings VFX TFLite + Alembic

Cette cible construit le module Python natif `vision_ia` lorsque
`VISIONIA_BUILD_VFX_BINDINGS=ON`.

## VisionEngine

- charge un vrai modèle TensorFlow Lite ;
- inspecte la forme d'entrée du modèle ;
- prétraite l'image en RGB ;
- utilise un détecteur frontal OpenCV si le cascade Haar système est disponible ;
- sinon utilise un crop carré centré ;
- exécute réellement TFLite ;
- cherche le tenseur de landmarks 468×3 ;
- remappe les coordonnées dans l'image source.

Le modèle MediaPipe historique est prévu pour une ROI visage 192×192 et produit
468 landmarks ; le graphe officiel normalise l'entrée sur [0,1]. Le smoke test
CI télécharge le modèle officiel `face_landmark.tflite` depuis
`storage.googleapis.com/mediapipe-assets/`.

## AlembicExporter

L'exporteur écrit un objet Alembic `OPoints` animé nommé `face_points`.
Chaque frame contient les 468 positions et des identifiants stables.
La cadence est configurable, 30 i/s par défaut.

## Build

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DVISIONIA_BUILD_VFX_BINDINGS=ON

cmake --build build --parallel

PYTHONPATH=build python3 python/test_vfx_bindings.py models/face_landmark.tflite
```

La cible VFX dépend de TensorFlow Lite, Alembic C++ et Pybind11. Le noyau
X32-NX principal reste indépendant lorsque l'option est désactivée.
