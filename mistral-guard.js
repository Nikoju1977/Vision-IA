/* Vision-IA · garde Mistral v1.3
   Charge après index.html. Ne modifie que le transport Mistral.
   Objectifs : éviter les rafales, sérialiser les départs et appliquer
   un backoff exponentiel avec jitter sur 429/5xx. */
(function(){
  if(window.__visionIaMistralGuard) return;
  if(!window.PROVIDERS || !window.PROVIDERS.mistral ||
     typeof window.tourDeRole !== 'function' ||
     typeof window.xhrJson !== 'function' ||
     typeof window.xhrFlux !== 'function') return;

  window.__visionIaMistralGuard = '1.3.0';

  var p = window.PROVIDERS.mistral;
  p.cadence = Math.max(Number(p.cadence) || 0, 1800);

  var baseTour = window.tourDeRole;
  var baseJson = window.xhrJson;
  var baseFlux = window.xhrFlux;
  var attendre = typeof window.patienter === 'function'
    ? window.patienter
    : function(ms){ return new Promise(function(ok){ setTimeout(ok, ms); }); };

  var etat = {
    dernierDepart: 0,
    bloqueJusqua: 0,
    echecsConsecutifs: 0,
    file: Promise.resolve()
  };

  function estMistral(url){
    return typeof url === 'string' && url.indexOf('api.mistral.ai') > -1;
  }

  function delaiSecours(){
    var n = Math.max(1, etat.echecsConsecutifs);
    return Math.min(30, Math.pow(2, n)) + Math.random();
  }

  function succes(){
    etat.echecsConsecutifs = 0;
  }

  function echec(err){
    if(!err || !(err.code === 429 || err.code >= 500)) return err;

    etat.echecsConsecutifs++;
    var secondes = Number(err.attendre) || 0;
    if(!(secondes > 0)) secondes = delaiSecours();
    secondes = Math.min(90, Math.max(1, secondes));

    err.attendre = secondes;
    etat.bloqueJusqua = Math.max(etat.bloqueJusqua, Date.now() + secondes * 1000);
    return err;
  }

  /* Les départs Mistral passent tous par la même file. La file ne bloque pas
     Groq/Cerebras et n'attend pas la fin de la réponse : elle espace seulement
     le démarrage des requêtes, ce qui évite les rafales accidentelles. */
  window.tourDeRole = function(provider){
    if(!provider || provider !== p) return baseTour(provider);

    var passage = etat.file.catch(function(){}).then(function(){
      var maintenant = Date.now();
      var intervalle = Math.max(1800, Number(provider.cadence) || 0);
      var cible = Math.max(etat.bloqueJusqua, etat.dernierDepart + intervalle);
      var ms = Math.max(0, cible - maintenant);

      return attendre(ms).then(function(){
        etat.dernierDepart = Date.now();
        if(window.dernierAppel) window.dernierAppel[provider.url] = etat.dernierDepart;
      });
    });

    etat.file = passage;
    return passage;
  };

  /* On conserve les XHR existants : on ajoute uniquement l'état adaptatif
     Mistral. Si Retry-After est déjà fourni par l'API, il reste prioritaire. */
  window.xhrJson = function(url){
    var args = arguments;
    var promesse = baseJson.apply(this, args);
    if(!estMistral(url)) return promesse;
    return promesse.then(function(valeur){
      succes();
      return valeur;
    }, function(err){
      throw echec(err);
    });
  };

  window.xhrFlux = function(url){
    var args = arguments;
    var promesse = baseFlux.apply(this, args);
    if(!estMistral(url)) return promesse;
    return promesse.then(function(valeur){
      succes();
      return valeur;
    }, function(err){
      throw echec(err);
    });
  };

  /* Visible depuis la console de diagnostic sans exposer de clé. */
  window.__visionIaMistralEtat = etat;
})();
