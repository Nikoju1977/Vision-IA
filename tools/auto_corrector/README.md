# Vision-IA Auto-Corrector

Boucle agentique de génération, exécution contrôlée et correction de scripts Python.

Fonctions principales :
- validation AST avant exécution ;
- Python isolé avec `-I -B` ;
- répertoire temporaire par tentative ;
- timeout ;
- limites CPU/mémoire/fichiers sur POSIX ;
- sortie bornée ;
- arrêt anti-boucle si le même code défaillant revient.

Le fournisseur IA est injecté sous forme de fonction `prompt -> texte`.

Cette couche réduit les risques d'accident mais n'est pas une sandbox de sécurité contre du code hostile. Pour du code non fiable provenant d'un tiers, utiliser un conteneur ou une sandbox OS.
