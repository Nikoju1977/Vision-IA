# Vision-IA

Vision-IA est un réalisateur agentique de cinéma, pensé pour accompagner un projet du pitch au dossier de production sans serveur applicatif ni chaîne de build.

L'application tient dans `index.html` et fonctionne comme une PWA. Elle lit des documents de travail, construit le contexte du film, produit une note d'intention, un découpage technique, un conducteur, un parti pris d'image, des directives de production et un dossier exportable.

## Phases de travail

1. **Plateau** — dialogue avec Vision-IA et classeur du film.
2. **Note d'intention** — sujet, mise en scène, image/son et direction d'acteurs.
3. **Découpage technique** — plans, échelles, mouvements, focales, angles, durées et intentions.
4. **Conducteur** — séquences, I/E, moments, durée estimée et regroupement par décor.
5. **Parti pris d'image** — palette, matière, lumière, optiques et références.
6. **Production** — casting, repérages, équipe, matériel, musique, son et risques.
7. **Dossier** — Markdown, sauvegarde JSON et impression propre.

## Sources et cohérence

Les documents du classeur sont la source factuelle du projet. Vision-IA sépare désormais :

- les informations réellement présentes dans les documents ;
- les déductions ;
- les propositions de mise en scène ;
- les éléments à vérifier.

Lorsqu'une séquence de scénario est choisie pour le découpage, son texte devient la **source unique de la séquence**. Le découpage est lié à cette source par un identifiant persistant ; « Ajouter des plans » refuse de mélanger deux séquences différentes.

Pour les projets historiques et documentaires, les prompts interdisent d'introduire comme faits des personnages, lieux, dates, objets, événements ou causalités absents des sources.

## Classeur du film

Formats lus :

- PDF texte, jusqu'à 400 pages ;
- DOCX ;
- TXT, Markdown, Fountain, FDX, RTF et CSV ;
- images de repérage.

Les corps de texte sont conservés dans IndexedDB ; les métadonnées du projet utilisent `safeStorage`. Jusqu'à 900 000 caractères sont conservés par pièce.

La détection distingue scénario, livre et texte. Les séquences de scénario alimentent le Conducteur et le sélecteur de Découpage ; les chapitres d'un livre ne sont plus proposés comme séquences de tournage.

## Dépouillement des documents longs

Le dépouillement est reprenable. Chaque fiche réussie est sauvegardée immédiatement avec un offset de progression.

Routage actuel :

- si **Groq** est configuré et qu'il reste plus de 60 000 caractères, le document long est envoyé directement vers Groq par tranches d'environ 9 000 caractères ;
- sinon, avec Mistral, le mode économique travaille par tranches d'environ 3 000 caractères et peut basculer sur Groq ;
- sans Mistral, Groq/Cerebras assurent le secours ;
- un `429` ou un `5xx` déclenche refroidissement, reprise bornée et bascule vers un moteur disponible ;
- le `Retry-After` du fournisseur est respecté.

La synthèse finale utilise un prompt de **script analyste**, pas le prompt créatif du réalisateur, afin d'éviter qu'une proposition artistique devienne un fait du scénario.

## Moteurs

Chaîne générale :

- **Mistral** — `mistral-small-latest` ;
- **Groq** — `openai/gpt-oss-20b` ;
- **Cerebras** — `llama-3.3-70b`.

Pour l'analyse visuelle des repérages, Vision-IA utilise **Ministral 3** via `ministral-3b-latest`.

En dépouillement économique, si Mistral est temporairement limité, Vision-IA pénalise le fournisseur et bascule immédiatement vers Groq lorsqu'il est disponible. Un fournisseur encore en refroidissement passe derrière les moteurs prêts.

Les quotas réels dépendent du compte et du fournisseur ; l'application ne suppose pas de seuil universel.

## Sécurité des clés

Les clés API passent toutes par `sanitizeKey()`.

Elles ne sont pas conservées en clair :

- coffre AES-256-GCM ;
- clé dérivée par PBKDF2-SHA256, 310 000 tours ;
- sel aléatoire de 16 octets ;
- IV aléatoire de 12 octets renouvelé à chaque écriture ;
- phrase de passe jamais persistée ;
- sans phrase de passe, les clés restent uniquement en RAM et disparaissent au rechargement.

L'analyse d'image lit elle aussi la clé Mistral dans ce coffre, et non dans le stockage en clair.

## Conducteur et plan de travail

Le Conducteur est calculé localement à partir des en-têtes de séquence, sans appel IA :

- nombre de séquences ;
- durée estimée ;
- décors distincts ;
- intérieurs/extérieurs ;
- séquences de nuit ;
- regroupement par décor.

Le Conducteur et le plan de travail par décor sont également intégrés au dossier Markdown.

## Storyboard

Le storyboard est construit depuis les plans du découpage avec Pollinations. Le prompt reprend l'échelle, l'angle, la focale, l'action, le genre, le décor et, lorsqu'elle existe, la palette du film. Un nouveau découpage de séquence efface les vues de l'ancienne source pour éviter la contamination.

## Export et impression

Le dossier conserve :

- titre et métadonnées ;
- note d'intention ;
- découpage et source verrouillée ;
- conducteur ;
- plan de travail par décor ;
- parti pris d'image ;
- directives de production ;
- repérages et notes de lecture.

À l'impression, seule la vue Dossier est imprimée. Les toasts, barres d'état, écrans de travail et autres phases ne sont plus capturés dans le PDF.

## Compatibilité et architecture

- application single-file : logique intégrée à `index.html` ;
- appels modèles en `XMLHttpRequest`, notamment pour la compatibilité Android/origine nulle ;
- pdf.js et Mammoth chargés depuis CDN et préchargés par le service worker ;
- `index.html` en réseau d'abord pour éviter de rester bloqué sur une ancienne version ;
- service worker versionné ;
- interface mobile avec `100dvh`, safe areas et champs à 16 px minimum.

## Qualité

La CI GitHub vérifie notamment :

- syntaxe JavaScript avec `node --check` ;
- garde-fous de cadence et reprise ;
- présence du routage Groq pour documents longs ;
- absence de l'ancien modèle Pixtral déprécié ;
- absence de lecture de clé Vision en clair ;
- discipline de source et verrouillage du découpage ;
- nettoyage de l'impression ;
- non-régression des anciens délais fixes.

## Installation

Ouvrir `index.html` directement ou utiliser GitHub Pages :

https://nikoju1977.github.io/Vision-IA/

---

Studio Niko Design
