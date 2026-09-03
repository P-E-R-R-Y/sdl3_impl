# sdl3_impl — journal

Marqueurs : 🟢 ajout · 🔴 rupture · 🔵 correctif · ⚪ interne ou doc · 🟡 propose
dans le plan, code non ecrit.

## v0.1.0

- 🟢 remplit `graphic2` + `audio`
- 🟢 point d'entree unique `getModules()`
- 🟢 cibles SHARED et STATIC (la STATIC recompile `sources/sdl3.cpp` pour
  un consommateur qui n'en a pas besoin — constate, pas corrige)

### Faille mesuree, pas corrigee

- 🔴 `createTexture`/`createSoundBuffer` etc. rendent un objet mort
  (`isReady()==false`) au lieu de `nullptr` quand le chargement echoue.
  meme comportement tolerant que sdl2

## Propose, pas ecrit

- 🟡 `claims()` : rien — meme raison que sdl2
- 🟡 cible STATIC → INTERFACE, zero object code
- 🟡 des exemples : n'en a toujours aucun
