import test from 'node:test';
import assert from 'node:assert/strict';
import { CAPACITES, extraireScenes, modeleDossier, promptStoryboard } from '../core.mjs';

test('Extrait les scènes et le numéro de ligne', () => {
  const source = 'Titre\n\nINT. SALON - JOUR\nUn regard.\n\nEXT. RUE - NUIT\nUn mouvement.';
  const r = extraireScenes(source);
  assert.equal(r.nombre_total, 2);
  assert.equal(r.scenes[0].ligne, 3);
  assert.equal(r.scenes[0].lieu, 'intérieur');
  assert.equal(r.scenes[1].moment, 'nuit');
});

test('Distingue les scènes mixtes et les limites', () => {
  const r = extraireScenes('INT/EXT. VOITURE - JOUR\nEXT. ROUTE - SOIR', 1);
  assert.equal(r.scenes[0].lieu, 'mixte');
  assert.equal(r.nombre_total, 2);
  assert.equal(r.tronque, true);
  assert.throws(() => extraireScenes('', 20));
  assert.throws(() => extraireScenes('INT. TEST', 201));
});

test('N’invente rien dans le dossier', () => {
  const md = modeleDossier('Mon film');
  assert.match(md, /À compléter/);
  assert.match(md, /# Dossier de film — Mon film/);
});

test('Produit un prompt de storyboard et un catalogue de capacités', () => {
  const p = promptStoryboard({ action: 'Une porte s’ouvre', format: '2.39:1' });
  assert.match(p, /2.39:1/);
  assert.match(p, /Une porte s’ouvre/);
  assert.equal(CAPACITES.etapes.length, 7);
});
