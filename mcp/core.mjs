/** Fonctions locales et déterministes, sans appels réseau et sans stockage. */
const HEADING = /^(?:(?:\d+[A-Z]?\s+)?(?:INT\.?\s*\/\s*EXT\.?|EXT\.?\s*\/\s*INT\.?|INT(?:ÉRIEUR|ERIEUR)?\.?|EXT(?:ÉRIEUR|ERIEUR)?\.?|I\s*\/\s*E|E\s*\/\s*I)(?:\s|\.|\/|-|$))/i;
const MOMENT = /\b(JOUR|NUIT|MATIN|SOIR|AUBE|CRÉPUSCULE|CREPUSCULE)\b/i;

export function extraireScenes(scenario, limite = 80) {
  if (typeof scenario !== 'string' || !scenario.trim()) throw new TypeError('Un scénario non vide est requis.');
  if (!Number.isInteger(limite) || limite < 1 || limite > 200) throw new RangeError('Limite invalide (1 à 200).');
  const lignes = scenario.replace(/\r\n?/g, '\n').split('\n');
  const scenes = [];
  let total = 0;
  for (let i = 0; i < lignes.length; i++) {
    const titre = lignes[i].trim();
    if (!HEADING.test(titre)) continue;
    total++;
    if (scenes.length >= limite) continue;
    const interieur = /^\d*[A-Z]?\s*(INT|I\s*\/\s*E)/i.test(titre);
    const exterieur = /^\d*[A-Z]?\s*(EXT|E\s*\/\s*I)/i.test(titre);
    const mixte = /INT\.?\s*\/\s*EXT|EXT\.?\s*\/\s*INT|I\s*\/\s*E|E\s*\/\s*I/i.test(titre);
    scenes.push({ numero: total, ligne: i + 1, entete: titre, lieu: mixte ? 'mixte' : interieur ? 'intérieur' : exterieur ? 'extérieur' : 'indéterminé', moment: (titre.match(MOMENT)?.[1] ?? 'non précisé').toLowerCase() });
  }
  return { nombre_total: total, scenes, tronque: total > scenes.length, avertissement: 'Repérage des en-têtes seulement, sans analyse sémantique ni estimation de durée.' };
}

export function promptStoryboard({ action, echelle = 'plan moyen', angle = 'à hauteur du regard', mouvement = 'caméra fixe', lumiere = 'lumière naturelle', palette = 'couleurs réalistes', format = '16:9' }) {
  if (typeof action !== 'string' || !action.trim()) throw new TypeError('L’action de la séquence est obligatoire.');
  return `Storyboard cinématographique, format ${format}. Action : ${action.trim()}. Échelle : ${echelle}. Angle : ${angle}. Mouvement : ${mouvement}. Lumière : ${lumiere}. Palette et texture : ${palette}. Image de prévisualisation de mise en scène, composition lisible, sans texte incrusté. À interpréter comme une PROPOSITION artistique et non comme un fait du scénario.`;
}

export function modeleDossier(titre, pitch = '') {
  if (typeof titre !== 'string' || !titre.trim()) throw new TypeError('Le titre est obligatoire.');
  return `# Dossier de film — ${titre.trim()}\n\n## Pitch\n${pitch.trim() || 'À compléter — source non fournie'}\n\n## Sources du projet\nÀ documenter (SOURCE / DÉDUCTION / PROPOSITION / À VÉRIFIER).\n\n## Note d’intention\nÀ compléter.\n\n## Découpage technique\n| Séquence | Plan | Échelle | Mouvement | Focale | Angle | Durée | Intention |\n|---|---|---|---|---|---|---|---|\n\n## Conducteur / plan de travail\nÀ compléter à partir du scénario.\n\n## Parti pris d’image et de son\nÀ compléter.\n\n## Directives de production\nCasting, équipe, décors, matériel, sécurité, musique, budget : à préciser.\n\n## Décisions validées et points à vérifier\nAucune décision validée à ce stade.\n`;
}

export const CAPACITES = Object.freeze({
  application: 'Vision-IA — Réalisateur agentique',
  depot: 'https://github.com/Nikoju1977/Vision-IA',
  pwa: 'https://nikoju1977.github.io/Vision-IA/',
  etapes: ['plateau', 'note d’intention', 'découpage technique', 'conducteur', 'parti pris d’image', 'directives de production', 'dossier'],
  outils_mcp: ['vision_ia_capabilities', 'vision_ia_extract_scenes', 'vision_ia_storyboard_prompt', 'vision_ia_dossier_template'],
  limitations: ['Le serveur MCP est local et sans appels IA.', 'Ne lit pas les fichiers ni IndexedDB de la PWA.', 'Ne contrôle pas le site publié.', 'Ne remplace pas l’expertise d’un réalisateur.']
});
