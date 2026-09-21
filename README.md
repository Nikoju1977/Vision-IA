# Vision-IA

Agent autonome qui tient le rôle de réalisateur : il prend le projet au pitch et le mène jusqu'aux directives de production.

**Application single-file HTML.** Aucune dépendance, aucun build, aucun serveur. Les clés API restent sur l'appareil.

## Les six phases

1. **Plateau** — dialogue avec l'agent. Il questionne, tranche, demande la validation du producteur avant chaque étape.
2. **Note d'intention** — 450 à 650 mots, à la première personne : sujet, parti pris, image et son, direction d'acteurs.
3. **Découpage** — tableau de plans (échelle, mouvement, focale, angle, durée, intention), chaque case corrigible au clavier.
4. **Image** — palette de 5 à 6 teintes avec nuancier, matière de l'image, lumière, optiques, références.
5. **Production** — casting, repérages, équipe, musique, calibrés sur les moyens annoncés, plus trois risques et leur parade.
6. **Dossier** — export Markdown, sauvegarde JSON, impression.

## Moteurs

Chaîne de secours automatique : Mistral → Groq (GPT-OSS 20B) → Cerebras. Une clé suffit ; avec plusieurs, l'app bascule seule si un fournisseur tombe.

Clés à récupérer sur `console.groq.com`, `cloud.cerebras.ai`, `console.mistral.ai`. Elles sont stockées via `safeStorage` (localStorage avec repli mémoire) et ne quittent l'appareil que vers le fournisseur choisi.

## Conventions Studio Niko Design

- XHR uniquement pour les appels API — compatibilité origine nulle sur Android
- `sanitizeKey()` sur toute saisie de clé
- `safeStorage` : localStorage + repli en mémoire
- 100dvh, `safe-area-inset`, `viewport-fit=cover`, saisies à 16 px minimum (pas de zoom iOS)
- Validation avant livraison : `node --check`, zéro ID dupliqué, bijection `getElementById`, zéro `fetch()`
- Typographies : Bebas Neue (titrage) et Courier Prime (corps, convention scénario)

## Installation

Ouvrir `index.html`. Sur GitHub Pages ou Vercel, servir la racine du dépôt. PWA installable (manifest + service worker, coquille en cache pour l'ouverture hors ligne).

---

Studio Niko Design

## Classeur du film

Vision-IA travaille sur pièces. Tout ce qui est versé au classeur entre dans son contexte et nourrit la note, le découpage, l'image et les directives.

- **Documents** — PDF (pdf.js, jusqu'à 400 pages, lignes reconstruites depuis l'ordonnée des blocs pour préserver les en-têtes de séquence), DOCX (mammoth), TXT, MD, Fountain, FDX, CSV. Glisser-déposer accepté. Jusqu'à 900 k signes conservés par pièce, soit un roman entier.
- **Repérages** — une photo de lieu est lue par Pixtral : architecture et matières, lumière disponible et son orientation, axes de caméra possibles, coût de tournage, usage dans ce film. Seule une vignette de 160 px est conservée, pas l'image d'origine.

### Scénarios

Les en-têtes de séquence sont repérés à l'import (`INT.`, `EXT.`, `INT./EXT.`, `SÉQUENCE 12`, numérotés ou non). Le classeur annonce le nombre de séquences, et la vue Découpage propose un sélecteur : choisir une séquence injecte son **texte intégral** dans le prompt de découpage — pas un résumé. Le réalisateur découpe donc les dialogues et les actions réellement écrits.

### Livres et textes longs

Les chapitres sont repérés de la même façon. Au-delà de 5 k signes, un bouton **Dépouiller en entier** lance une lecture en map-reduce : le texte est découpé en tranches de 9 k signes, chacune fait l'objet d'une fiche de dépouillement, puis toutes les fiches sont recousues en une note de lecture de réalisateur (histoire et arc, personnages et direction de jeu, lieux et lumière, séquences porteuses, difficultés de tournage). C'est cette note qui entre ensuite dans le contexte — le film travaille alors sur l'œuvre entière, pas sur son premier chapitre.

Compter une requête par tranche : un scénario de long métrage fait environ 25 tranches, un roman de 400 pages environ 80. Le garde-fou est fixé à 90.

### Mode Mistral gratuit

Quand **Mistral est la seule clé configurée**, Vision-IA n'utilise pas l'API Batch : les documents longs sont dépouillés via `/v1/chat/completions`, une tranche après l'autre. Le mode Free démarre à **4 500 signes par tranche**. Si Mistral renvoie un `429`, Vision-IA ne rejoue plus plusieurs fois la même grosse requête : il divise automatiquement la taille de tranche par deux (jusqu'à 1 200 signes), reconstruit le reste du travail et mémorise la taille qui fonctionne pour une reprise ultérieure. Chaque fiche réussie est sauvegardée immédiatement et l'offset n'avance jamais sur une requête refusée. Un `402` reçu lors d'une tentative Batch déclenche également le repli vers Mistral temps réel.

### Stockage

Le localStorage plafonne vers 5 Mo : il ne garde que les métadonnées du film. Les corps de texte vont en IndexedDB, avec repli en mémoire si la base est refusée (Safari en navigation privée) — dans ce cas les pièces disparaissent à la fermeture de l'onglet, le reste du film survit.

Un PDF scanné n'a pas de texte à extraire : l'app le détecte et renvoie vers « Analyser un repérage ». La lecture d'image demande la clé Mistral, seul moteur de la chaîne à voir les images. Pas de lecture EPUB pour l'instant : convertir en PDF ou en TXT.
\n\nAvec une clé Groq configurée, le dépouillement gratuit utilise aussi **Groq GPT-OSS 20B** (`openai/gpt-oss-20b`) en secours après les modèles Mistral. Le modèle Groq historique `llama-3.3-70b-versatile` a été retiré du Free/Developer tier le 16 août 2026.\n
## Conducteur

Dès qu'un scénario est versé, la phase Conducteur se remplit sans une seule requête IA, à partir des seuls en-têtes de séquence : minutage estimé (1 500 signes ≈ 1 page ≈ 1 minute), nombre de décors distincts, répartition intérieurs/extérieurs, séquences de nuit, et surtout le **regroupement par décor** — l'ordre dans lequel tourner pour ne pas revenir deux fois au même endroit.

## Storyboard

Depuis le découpage, chaque plan devient une vue générée par Pollinations (gratuit, sans clé, CORS ouvert). Le prompt est construit sur l'échelle, l'angle, la focale et l'action du plan, plus la palette et le genre du film. 24 vues maximum par tirage.

## Reprise et robustesse

- **Reprendre un film** relit une sauvegarde JSON, corpus intégral compris — l'export n'est plus un cul-de-sac.
- Le **dépouillement est reprenable** : chaque fiche est sauvegardée dès qu'elle arrive. Une coupure à la 62ᵉ tranche sur 80 ne perd rien, le bouton propose de reprendre là où ça s'est arrêté.
- Cadence de 2,2 s entre les tranches pour rester sous les 30 requêtes/minute de Groq, et réessai avec attente croissante (12, 24, 36, 48 s) sur erreur de quota.
- Note d'intention, directives et dialogue s'**écrivent en direct** (SSE lu par `XMLHttpRequest.onprogress`, conforme à la règle XHR).
- Service worker **versionné**, `index.html` servi réseau d'abord : une mise en ligne n'est plus prisonnière du cache.

## Durcissement (v1.1)

- **Verrou de génération corrigé.** Le clap s'effaçait au premier mot en remettant `occupe` à faux : les boutons redevenaient actifs pendant l'écriture, et un second envoi écrasait le premier. Le clap et le verrou sont désormais deux choses distinctes — une barre « Vision-IA écrit… » remplace le clap, les boutons restent bloqués.
- **Bouton Couper.** Toute génération est interruptible (`XMLHttpRequest.abort()`, touche Échap aussi). Le texte déjà écrit est conservé, pas jeté. Une interruption ne déclenche pas la bascule vers le fournisseur suivant.
- **Chaîne de fournisseurs unifiée.** Les modes streaming et non-streaming parcouraient la chaîne dans deux fonctions jumelles : ce qui diverge finit par diverger en bug. Une seule fonction `chaine()` désormais.
- **Rendu incrémental du dialogue.** Le fil entier était reconstruit à chaque token (`innerHTML` + `scrollIntoView`) — injouable sur mobile. Seul le dernier tour est mis à jour.
- **Parseur SSE testé** sur des trames coupées en plein milieu de JSON : recollage vérifié sous node.
- **Storyboard** : état de chargement visible et repli propre quand une vue ne revient pas.
- **pdf.js** : garde-fou de 45 s si le worker du CDN est injoignable, au lieu d'une attente infinie.
- **Clavier** : flèches gauche/droite entre les phases, Échap ferme les réglages puis coupe une génération, focus rendu au bouton d'origine.

## Quotas et 429 (v1.2)

L'offre gratuite de Mistral plafonne à une requête par seconde, Groq à trente par minute. Trois mécanismes, du plus discret au plus visible :

1. **Cadence anticipée** — chaque fournisseur a son intervalle minimal (Mistral 1,4 s, Cerebras 1,2 s, Groq 2,1 s) et les appels sont espacés avant d'être émis. Le 429 est évité plutôt que rattrapé. C'est ce qui rend le dépouillement d'un roman praticable : plus de temporisation artificielle dans la boucle, la cadence est tenue au niveau du transport.
2. **Reprise sur le même moteur** — un 429 n'est pas une panne mais une file d'attente. L'en-tête `Retry-After` est lu et respecté ; à défaut, attente de 3, 9 puis 20 s. Un 5xx suit la même règle.
3. **Bascule intelligente** — s'il reste une clé libre derrière, une seule reprise courte (2 s) puis on passe au moteur suivant : inutile d'attendre 30 s quand Groq est disponible. C'est seulement sur le dernier moteur de la chaîne qu'on patiente pour de bon.

Les messages sont explicites : « Mistral sature, reprise dans 9 s. » puis « Bascule sur Groq. » Les erreurs 401 et 403 sont nommées « Clé refusée » et ne déclenchent aucune attente — une clé invalide ne se répare pas en patientant.

L'analyse de repérage, qui attaque Pixtral hors chaîne et sans repli possible, suit la même politique de reprise.
