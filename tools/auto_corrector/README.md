# Vision-IA Auto-Corrector

Boucle agentique de génération, exécution contrôlée et correction de scripts Python.

## Deux couches

### Auto-correcteur générique

`auto_corrector.py` accepte un fournisseur simple `prompt -> texte` et sait extraire du code Markdown.

### VisionIAAgent JSON strict

`vision_ia_agent.py` impose un protocole beaucoup plus strict :

```json
{
  "analyse": "raisonnement bref",
  "code": "programme Python pur"
}
```

Le fournisseur est injecté sous forme :

```python
def provider(system_prompt: str, prompt: str) -> str:
    ...
```

Exemple :

```python
from vision_ia_agent import VisionIAAgent

agent = VisionIAAgent(
    provider,
    max_iterations=5,
    timeout_sec=10,
    memory_mb=256,
)

result = agent.solve("Afficher les 50 premiers nombres de Fibonacci")

if result.success:
    print(result.code)
```

## Garde-fous

- JSON strict : exactement `analyse` + `code`
- validation AST avant exécution
- Python isolé avec `-I -B`
- répertoire temporaire par tentative
- timeout
- limites CPU/mémoire/fichiers/descripteurs sur POSIX
- sortie stdout/stderr bornée
- empreinte SHA-256 de chaque proposition
- arrêt anti-boucle si le même code défaillant revient
- historique structuré des itérations

Cette couche réduit les risques d'accident mais n'est pas une sandbox de sécurité contre du code hostile. Pour du code non fiable provenant d'un tiers, utiliser un conteneur ou une sandbox OS dédiée.

## Validation

La CI exécute :

```bash
python -m py_compile tools/auto_corrector/auto_corrector.py
cd tools/auto_corrector
python -m unittest -v
```
