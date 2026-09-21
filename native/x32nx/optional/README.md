# Bindings VFX optionnels

Le fichier `vision_ia_bindings.cpp` conserve l'interface Pybind11 proposée
pour deux adaptateurs :

- `VisionEngine` : extraction de landmarks via le modèle TFLite exact ;
- `VisionAlembicExporter` : export Alembic.

Ils ne sont volontairement pas reliés au build par défaut. Il manque encore
les deux implémentations natives, le SDK TensorFlow Lite sélectionné, le modèle
`face_landmark.tflite` exact et la configuration Alembic de la plateforme.

Le noyau X32-NX testé par CI ne dépend donc pas de stubs prétendant réaliser
une inférence ou un export Alembic qu'ils ne font pas réellement.
