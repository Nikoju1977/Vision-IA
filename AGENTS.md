# AGENTS.md — Vision-IA

Instructions pour tout agent de code (Codex, Claude Code, Cursor, Copilot, Gemini)
intervenant sur ce dépôt. Conventions Studio Niko Design.

## Le projet

Vision-IA est un agent autonome qui tient le rôle d'un réalisateur de cinéma
professionnel, du pitch jusqu'à la post-production. Six phases : plateau
conversationnel, note d'intention, découpage technique, parti pris d'image et
palette, directives de production, dossier d'export (Markdown / JSON /
impression). Un classeur accepte PDF, DOCX, TXT, MD et Fountain ; les repérages
photo sont analysés par Pixtral.

## Architecture

Application **single-file HTML**. Tout tient dans `index.html` : structure,
style, logique. Les seuls autres fichiers sont `sw.js` (service worker),
`manifest.json` et les deux icônes.

**Ne jamais extraire de logique dans un fichier `.js` séparé.** Un correctif
livré comme module externe doit être fondu dans `index.html` avant d'être
poussé. Le single-file n'est pas une préférence esthétique : c'est ce qui rend
l'application déployable partout, lisible d'un seul tenant et dépourvue de
chaîne de build.

## Règles non négociables

- **XHR uniquement.** Aucun appel `fetch()` vers une API. `XMLHttpRequest` est
  requis pour la compatibilité origine nulle sur Android. Le seul `fetch` admis
  est celui du service worker.
- **`sanitizeKey()`** sur toute saisie de clé d'API, sans exception.
- **`safeStorage`** pour tout accès au stockage : jamais `localStorage` en direct.
- **Coffre à clés chiffré.** Les clés d'API ne touchent jamais le stockage en
  clair. AES-256-GCM, clé dérivée par PBKDF2-SHA256 (310 000 tours), sel de
  16 octets et IV de 12 octets tirés par `crypto.getRandomValues`, IV neuf à
  chaque écriture. La phrase de passe n'est jamais conservée ; seule la
  `CryptoKey` dérivée, non exportable, vit en mémoire. Sans phrase de passe, les
  clés restent en RAM et meurent au rechargement — c'est le comportement voulu,
  ne pas « améliorer » en réintroduisant une persistance en clair.
- **Chaîne de secours** Mistral → Groq → Cerebras. Un 429 n'est pas une panne :
  on patiente et on réessaie le même fournisseur, on ne le disqualifie pas.

## Cadence et quotas

La temporisation a **un seul propriétaire** : `tourDeRole()`. Ne jamais ajouter
de `patienter()` dans une boucle de reprise — l'attente serait comptée deux fois.

- Les départs sont sérialisés par **Web Locks**, verrou par fournisseur, tenu
  pendant l'attente. Deux onglets ouverts ne peuvent pas doubler la cadence.
- L'échéance de refroidissement transite par `safeStorage` : un 429 pris dans un
  onglet freine aussi les autres.
- `penaliser()` arme le refroidissement (Retry-After prioritaire, sinon backoff
  exponentiel avec jitter, plafonné à 90 s). `apaiser()` remet le compteur
  d'échecs à zéro après un succès.
- Mistral limite les requêtes/seconde **et** les jetons/minute, indépendamment.
  Le plafond de jetons est réglable par l'utilisateur (0 = désarmé), car Mistral
  ne publie plus les seuils du palier gratuit ; ils se relèvent dans Mistral AI
  Studio → Limits. Le décompte exige `stream_options: { include_usage: true }`
  sur les appels en flux, sans quoi aucun `usage` ne revient.

## Interface

- Polices : **Bebas Neue** pour les titres, **Courier Prime** pour le corps.
  Jamais Inter, Arial, Roboto ou Helvetica.
- Mobile : `100dvh`, `safe-area-inset`, `viewport-fit=cover`, taille de police
  des champs à 16 px minimum pour empêcher le zoom iOS.
- Vocabulaire de cinéma dans le code comme dans l'interface : `clap`, `voile`,
  `tirage`, `plateau`, `tally`, `tungstene`. Identifiants et commentaires en
  français.

## Service worker

`index.html` en réseau d'abord — une mise en ligne ne doit pas rester
prisonnière du cache. Le reste en cache d'abord. Les appels aux API de modèles
sont exclus de l'interception. Les lecteurs PDF/DOCX servis par CDN sont
préchargés en `no-cors` pour que le classeur survive hors ligne.

**Incrémenter `VERSION`** à chaque livraison, sinon les clients gardent
l'ancienne coquille.

Ne pas réintroduire de réécriture du HTML dans le service worker : `rep.text()`
sur le document supprime le streaming de 90 ko.

## Avant de livrer

1. `node --check` sur chaque bloc de script extrait, et sur `sw.js`.
2. Zéro identifiant DOM dupliqué.
3. Bijection complète entre les `$('...')` du script et les `id` du HTML.
4. Zéro `fetch(` dans `index.html`.
5. Zéro écriture de clé d'API en clair dans le stockage.
6. Vérifier le résultat **depuis `main`**, pas depuis la copie locale.

## Déploiement

GitHub Pages sur la branche `main`, racine. Pousser par l'API Contents
(`GET` du `sha`, puis `PUT`). Les jetons classiques à portée `repo` fonctionnent ;
les jetons à portée fine échouent souvent. Révoquer le jeton immédiatement
après usage.
