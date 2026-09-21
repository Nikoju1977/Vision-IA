# Vision-IA X32-NX native core

Ce sous-système natif est isolé de l'application web Vision-IA afin de préserver
l'architecture single-file du site principal.

## Noyau compilé et testé

- Protocol Buffers `StreamPacket`
- Reed-Solomon RS(255,223)
- correction de 0 à 16 symboles inconnus
- rejet AMDEC d'un cas reproductible à 17 symboles
- filtre de Kalman spatial 3D
- RVQ audio dynamique
- multiplexeur vidéo/audio à budget total de 32 kbit/s
- ChaCha20-Poly1305 avec AAD
- contrôle SSIM corrigé
- préfiltre anti-flood avant FEC
- nœud UDP de diagnostic Protobuf
- Docker + CTest

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Docker

```bash
docker build -t vision-ia-x32nx .
docker run --rm -p 8001:8001/udp vision-ia-x32nx
```

Le nœud de démonstration écoute UDP/8001 et inspecte les `StreamPacket`
Protobuf. Le pipeline sécurisé ChaCha/FEC est fourni comme bibliothèque ; son
transport complet inter-paquets reste une couche distincte.

Les bindings TFLite/Alembic sont conservés dans le dossier `optional/` et ne
sont pas prétendus validés tant que les SDK natifs et le modèle exact ne sont
pas fournis.
