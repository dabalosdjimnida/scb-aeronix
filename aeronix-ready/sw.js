const CACHE_NAME = 'scb-aeronix-v5';
const APP_SHELL = [
  './index.html',
  './manifest.json',
  './scb-aeronix-final-logo.png',
  './scb-aeronix-icon-192.png',
  './scb-aeronix-icon-512.png'
];

self.addEventListener('install', event => {
  event.waitUntil(
    caches.open(CACHE_NAME)
      .then(cache => cache.addAll(APP_SHELL))
      .then(() => self.skipWaiting())
  );
});

self.addEventListener('activate', event => {
  event.waitUntil(
    caches.keys()
      .then(cacheNames => Promise.all(
        cacheNames
          .filter(cacheName => cacheName.startsWith('scb-aeronix-') && cacheName !== CACHE_NAME)
          .map(cacheName => caches.delete(cacheName))
      ))
      .then(() => self.clients.claim())
  );
});

self.addEventListener('fetch', event => {
  const request = event.request;
  const url = new URL(request.url);

  if (request.method !== 'GET' || url.origin !== self.location.origin) {
    return;
  }

  if (request.mode === 'navigate') {
    event.respondWith(
      fetch(request).catch(() => caches.match('./index.html'))
    );
    return;
  }

  if (url.pathname.endsWith('/scb-aeronix-final-logo.png') ||
      url.pathname.endsWith('/scb-aeronix-icon-192.png') ||
      url.pathname.endsWith('/scb-aeronix-icon-512.png') ||
      url.pathname.endsWith('/manifest.json')) {
    event.respondWith(
      caches.match(request).then(cachedResponse => cachedResponse || fetch(request))
    );
  }
});
