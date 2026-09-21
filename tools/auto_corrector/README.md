# Vision-IA Auto-Corrector

Boucle agentique de génération, exécution contrôlée et correction de scripts Python.

## Moteur IA : Mistral

Le fournisseur principal est désormais `MistralProvider`, basé sur le SDK officiel `mistralai`.

Configuration :

```bash
pip install -U mistralai
export MISTRAL_API_KEY="..."
```

Exemple :

```python
from mistral_provider import create_mistral_agent

agent = create_mistral_agent(
    model="mistral-large-latest",
    max_iterations=5,
    timeout_sec=10,
    memory_mb=256,
    temperature=0.2,
)

result = agent.solve("Afficher les 50 premiers nombres de Fibonacci")

if result.success:
    print(result.code)
else:
    print(result.message)
```

Le SDK Mistral force le mode JSON avec :

```python
response_format={"type": "json_object"}
```

Puis `VisionIAAgent` applique une validation stricte supplémentaire : la réponse doit contenir exactement les champs `analyse` et `code`.

## Architecture

- `auto_corrector.py` : sandbox locale bornée et boucle d'auto-correction générique ;
- `vision_ia_agent.py` : protocole agentique JSON strict, AST, historique et anti-boucle ;
- `mistral_provider.py` : intégration Mistral ;
- `test_auto_corrector.py` : tests unitaires hors réseau.

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
- aucune clé API stockée dans le dépôt

Cette couche réduit les risques d'accident mais n'est pas une sandbox de sécurité contre du code hostile. Pour du code non fiable provenant d'un tiers, utiliser un conteneur ou une sandbox OS dédiée.

## Validation

La CI vérifie la syntaxe de tous les modules et exécute les tests sans appel réseau.
