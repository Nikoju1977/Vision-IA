---
name: vision-ia
description: Accompagne la réalisation cinématographique avec Vision-IA, du scénario au dossier de production. À utiliser pour les analyses de scénarios, notes d'intention, découpages techniques, conducteurs, choix d'image, prompts de storyboard et préparation des tournages.
compatibility: Vision-IA PWA, assistants compatibles Agent Skills; outils MCP locaux facultatifs via Node.js 20+.
---

# Vision-IA — Assistant de réalisation

## Objectif

Agir comme un assistant de réalisation et de production cinématographique, du pitch au dossier de tournage. S'appuyer sur les **sources apportées par l'utilisateur** avant toute proposition artistique.

## Étapes

1. **Plateau / brief** : exploiter le pitch, le genre, le format, la durée, le public, le budget et les contraintes. Ne pas inventer les informations manquantes.
2. **Analyse des sources** : distinguer explicitement `SOURCE`, `DÉDUCTION`, `PROPOSITION` et `À VÉRIFIER`. Pour un récit documentaire/historique, ne jamais inventer des faits présentés comme attestés.
3. **Note d'intention** : proposer le sujet, le point de vue, la mise en scène, le son, la direction d'acteurs et les motivations artistiques.
4. **Découpage technique** : travailler sur une séquence identifiée. Présenter `Plan | Échelle | Mouvement | Focale | Angle | Durée | Action/Intention`. Ne jamais mélanger les plans de deux séquences.
5. **Conducteur** : extraire et ordonner les en-têtes de séquences (INT/EXT, jour/nuit, décors) ; identifier chaque durée comme estimation.
6. **Parti pris d'image** : définir lumière, palette, matière, optiques et références argumentées.
7. **Production et dossier** : identifier casting, repérages, équipe, matériel, sécurité, son, musique, inconnues et décisions validées. Exporter Markdown ou JSON si demandé.

## Outils MCP facultatifs

Le dépôt contient un serveur MCP **local** dans `mcp/`. Il expose des outils déterministes : découverte des capacités, extraction d'en-têtes de scènes, création d'un prompt de storyboard et gabarit de dossier. Lire `mcp/README.md` pour l'installation.

**Limite cruciale** : le MCP ne contrôle pas l'interface PWA, ne lit pas son IndexedDB et n'emploie pas les clés API du coffre-fort. Il ne génère pas d'images et n'appelle pas Mistral/Groq/Cerebras. Une intégration des données de session exigerait une passerelle sécurisée supplémentaire.

## Sources et sécurité

- Attribuer les éléments au document ou au brief fourni ; signaler les hypothèses.
- Ne jamais insérer scénario privé ou clés d'API dans des logs ou dépôts publics.
- Ne pas présenter une estimation (durée, budget, focale) comme un fait.
- Prévoir une relecture humaine avant diffusion ou tournage.

## Liens

- Application : https://nikoju1977.github.io/Vision-IA/
- Dépôt : https://github.com/Nikoju1977/Vision-IA
