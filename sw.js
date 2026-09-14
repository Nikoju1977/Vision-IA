/* Vision-IA · service worker — Studio Niko Design
   index.html en réseau d'abord : une mise en ligne ne reste pas
   prisonnière du cache. Le reste en cache d'abord. */
var VERSION = 'vision-ia-v5';
var COQUILLE = ['./', './index.html', './manifest.json', './icon-192.png', './icon-512.png', './mistral-guard.js'];

self.addEventListener('install', function(e){
  self.skipWaiting();
  e.waitUntil(caches.open(VERSION).then(function(c){ return c.addAll(COQUILLE).catch(function(){}); }));
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

function injecterGardeMistral(rep){
  if(!rep || !rep.ok) return Promise.resolve(rep);
  return rep.text().then(function(html){
    if(html.indexOf('mistral-guard.js') < 0){
      html = html.replace('</body>', '<script src="./mistral-guard.js"></script>\n</body>');
    }
    var h = new Headers(rep.headers);
    h.delete('content-length');
    h.delete('content-encoding');
    return new Response(html, {
      status: rep.status,
      statusText: rep.statusText,
      headers: h
    });
  });
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
        return injecterGardeMistral(rep);
      }).catch(function(){
        return caches.match('./index.html').then(function(r){
          return injecterGardeMistral(r || null);
        }).then(function(r){ return r || caches.match('./'); });
      })
    );
    return;
  }

  e.respondWith(
    caches.match(e.request).then(function(r){
      return r || fetch(e.request).then(function(rep){
        if(rep && rep.status === 200 && rep.type === 'basic'){
          var copie = rep.clone();
          caches.open(VERSION).then(function(c){ c.put(e.request, copie); });
        }
        return rep;
      }).catch(function(){ return caches.match('./index.html'); });
    })
  );
});
