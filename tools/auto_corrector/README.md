# Vision-IA Auto-Corrector

Boucle agentique de génération, exécution contrôlée et correction de scripts Python.

## Moteur IA : Mistral

Le fournisseur principal est `MistralProvider`, basé sur le SDK officiel `mistralai`.

Configuration :

```bash
pip install -r tools/auto_corrector/requirements.txt
export MISTRAL_API_KEY="..."
```

## Anti-saturation

Le provider Mistral protège désormais les appels contre les saturations courantes :

- cadence minimale configurable entre requêtes ;
- retries exponentiels sur 429, 5xx et timeouts transitoires ;
- jitter pour éviter les rafales de retry ;
- limite de taille de prompt ;
- compaction automatique d'un prompt trop volumineux ;
- traitement séquentiel des documents longs.

Pour les gros documents, utiliser `long_document_processor.py`. Il ne transmet jamais le document complet en un seul appel : il le découpe, analyse les fragments un par un puis fusionne les résultats par niveaux.

Formats pris en charge :
- TXT / Markdown / JSON / CSV ;
- DOCX via la bibliothèque standard ;
- PDF via `pypdf`.

Exemple :

```bash
python tools/auto_corrector/analyze_long_document.py mon_document.pdf \
  --objective "Dépouillement complet et détection des incohérences"
```

Réglages par défaut du mode long document :
- fragments de 12 000 caractères ;
- chevauchement de 600 caractères ;
- fusion par lots de 4 ;
- une seule requête Mistral à la fois ;
- 6 retries maximum dans le CLI ;
- délai minimum de 0,5 s entre appels dans le CLI.

## Agent de code

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
```

## Architecture

- `auto_corrector.py` : sandbox locale bornée et boucle d'auto-correction générique ;
- `vision_ia_agent.py` : protocole agentique JSON strict, AST, historique et anti-boucle ;
- `mistral_provider.py` : Mistral avec retry, throttling et compaction ;
- `long_document_processor.py` : découpage + synthèse hiérarchique ;
- `analyze_long_document.py` : CLI pour gros fichiers ;
- `test_auto_corrector.py` et `test_long_document.py` : tests hors réseau.

## Garde-fous

- sortie JSON contrôlée ;
- validation AST avant exécution de code ;
- Python isolé avec `-I -B` ;
- timeout ;
- limites CPU/mémoire/fichiers sur POSIX ;
- anti-boucle ;
- aucune clé API stockée dans le dépôt ;
- les documents longs ne sont pas injectés intégralement dans une seule requête.

Cette couche réduit fortement les erreurs de saturation et de contexte, mais elle ne peut pas garantir qu'un service distant ne renverra jamais de 429 ou d'indisponibilité. Dans ce cas, les retries et la reprise par fragments limitent l'impact.
