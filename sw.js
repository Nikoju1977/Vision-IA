/* Vision-IA · service worker — Studio Niko Design
   index.html en réseau d'abord : une mise en ligne ne reste pas
   prisonnière du cache. Le reste en cache d'abord. */
var VERSION = 'vision-ia-v9';
var COQUILLE = ['./', './index.html', './manifest.json', './icon-192.png', './icon-512.png'];

/* Lecteurs PDF/DOCX servis par CDN : hors cache, le classeur tombait
   dès que le réseau manquait. On les prend en opaque (no-cors) ;
   un <script src> s'en accommode, contrairement à un fetch() lu. */
var TIERS = [
  'https://cdnjs.cloudflare.com/ajax/libs/pdf.js/3.4.120/pdf.min.js',
  'https://cdnjs.cloudflare.com/ajax/libs/pdf.js/3.4.120/pdf.worker.min.js',
  'https://cdnjs.cloudflare.com/ajax/libs/mammoth/1.6.0/mammoth.browser.min.js'
];

function precharger(c){
  return Promise.all(TIERS.map(function(u){
    return c.add(new Request(u, { mode:'no-cors' })).catch(function(){});
  }));
}

self.addEventListener('install', function(e){
  self.skipWaiting();
  e.waitUntil(caches.open(VERSION).then(function(c){
    return c.addAll(COQUILLE).catch(function(){}).then(function(){ return precharger(c); });
  }));
});

self.addEventListener('activate', function(e){
  e.waitUntil(caches.keys().then(function(ks){
    return Promise.all(ks.map(function(k){ return k === VERSION ? null : caches.delete(k); }));
  }).then(function(){ return self.clients.claim(); }));
});

function estDocument(req){
  return req.mode === 'navigate' ||
         (req.headers.get('accept') || '').indexOf('text/html') > -1;
}

self.addEventListener('fetch', function(e){
  var u = e.request.url;
  if(e.request.method !== 'GET') return;
  if(u.indexOf('api.groq.com') > -1 || u.indexOf('api.cerebras.ai') > -1 ||
     u.indexOf('api.mistral.ai') > -1 || u.indexOf('pollinations.ai') > -1) return;

  if(estDocument(e.request)){
    e.respondWith(
      fetch(e.request).then(function(rep){
        var copie = rep.clone();
        caches.open(VERSION).then(function(c){ c.put('./index.html', copie); });
        return rep;
      }).catch(function(){
        return caches.match('./index.html').then(function(r){ return r || caches.match('./'); });
      })
    );
    return;
  }

  e.respondWith(
    caches.match(e.request).then(function(r){
      return r || fetch(e.request).then(function(rep){
        if(rep && (rep.type === 'opaque' || (rep.status === 200 && rep.type === 'basic'))){
          var copie = rep.clone();
          caches.open(VERSION).then(function(c){ c.put(e.request, copie); });
        }
        return rep;
      }).catch(function(){ return caches.match('./index.html'); });
    })
  );
});
