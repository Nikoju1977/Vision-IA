import { McpServer } from '@modelcontextprotocol/server';
import { serveStdio } from '@modelcontextprotocol/server/stdio';
import * as z from 'zod/v4';
import { CAPACITES, extraireScenes, modeleDossier, promptStoryboard } from './core.mjs';

const message = value => ({ content: [{ type: 'text', text: typeof value === 'string' ? value : JSON.stringify(value, null, 2) }] });

function createServer() {
  const server = new McpServer({ name: 'vision-ia', version: '0.1.0' });

  server.registerTool('vision_ia_capabilities', {
    description: 'Présenter les sept phases de Vision-IA et les limites réelles de ce connecteur local.',
    inputSchema: z.object({})
  }, async () => message(CAPACITES));

  server.registerTool('vision_ia_extract_scenes', {
    description: 'Extraire localement les en-têtes INT/EXT d’un scénario (sans analyse IA).',
    inputSchema: z.object({
      scenario: z.string().min(1).max(200_000).describe('Scénario texte/Fountain fourni explicitement par l’utilisateur.'),
      max_scenes: z.number().int().min(1).max(200).default(80)
    })
  }, async ({ scenario, max_scenes }) => message(extraireScenes(scenario, max_scenes)));

  server.registerTool('vision_ia_storyboard_prompt', {
    description: 'Composer un prompt de storyboard ; ne génère pas d’image.',
    inputSchema: z.object({
      action: z.string().min(1).max(4000),
      echelle: z.string().max(200).default('plan moyen'),
      angle: z.string().max(200).default('à hauteur du regard'),
      mouvement: z.string().max(200).default('caméra fixe'),
      lumiere: z.string().max(200).default('lumière naturelle'),
      palette: z.string().max(200).default('couleurs réalistes'),
      format: z.enum(['16:9', '2.39:1', '4:3', '9:16']).default('16:9')
    })
  }, async args => message(promptStoryboard(args)));

  server.registerTool('vision_ia_dossier_template', {
    description: 'Créer un gabarit Markdown de dossier de film ; n’invente pas les informations absentes.',
    inputSchema: z.object({
      titre: z.string().min(1).max(200),
      pitch: z.string().max(8000).default('')
    })
  }, async ({ titre, pitch }) => message(modeleDossier(titre, pitch)));

  return server;
}

void serveStdio(createServer);
console.error('Vision-IA MCP (stdio) prêt : outils locaux, aucune clé API requise.');
