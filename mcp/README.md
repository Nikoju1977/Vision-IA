# Vision-IA — MCP local (expérimental)

Ce serveur expose quatre outils au protocole **Model Context Protocol (MCP)** avec le SDK officiel v2. Il prépare des éléments cinématographiques depuis un assistant compatible MCP. Il ne pilote pas la PWA Vision-IA, ne lit ni son IndexedDB ni ses secrets et n’envoie aucune donnée à un service externe.

## Installation

Prérequis : Node.js 20+ et accès au registre npm pour installer le SDK.

```bash
cd mcp
npm install
npm test
npm start
```

La commande `npm start` laisse le serveur attendre un client MCP sur l’entrée/sortie standard. **Ce n’est pas un site web.** Le canal stdout est réservé au protocole JSON-RPC.

## Configurer un client MCP compatible stdio

Configurer le client avec un chemin absolu :

```json
{
  "mcpServers": {
    "vision-ia": {
      "command": "node",
      "args": ["/CHEMIN/ABSOLU/Vision-IA/mcp/server.mjs"]
    }
  }
}
```

Les options exactes varient selon le client. Installer les dépendances dans `mcp/` avant la première utilisation.

## Outils disponibles

| Outil | Résultat | Appelle une IA ? |
|---|---|---|
| `vision_ia_capabilities` | Capacités et limites du projet | Non |
| `vision_ia_extract_scenes` | En-têtes de scènes INT/EXT avec numéros de ligne | Non |
| `vision_ia_storyboard_prompt` | Prompt de cadrage pour un futur générateur d’images | Non |
| `vision_ia_dossier_template` | Structure Markdown de dossier de film | Non |

Les données passent par la mémoire du processus Node. Ces outils n’écrivent rien sur disque.

## Vérification

- Fonctions déterministes : `npm test` (ne requiert pas le SDK).
- Syntaxe : `npm run check` (ne requiert pas le SDK).
- Intégration : `npx @modelcontextprotocol/inspector node server.mjs` après `npm install`, puis essayer `tools/list` et chaque outil.

**Ne pas confondre** : `SKILL.md` est une notice pour agents (voir `.agents/skills/vision-ia/SKILL.md`) ; le MCP est un serveur exécutable. GitHub Pages peut héberger la PWA statique mais pas un processus Node MCP. Une connexion à la session du navigateur ou aux API IA exigerait une passerelle sécurisée supplémentaire.
