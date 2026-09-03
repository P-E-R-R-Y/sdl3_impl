# sdl3_impl — changelog

Markers: 🟢 added · 🔴 breaking · 🔵 fix · ⚪ internal or docs · 🟡 proposed
in the plan, no code written yet.

## v0.1.0

- 🟢 fills `graphic2` + `audio`
- 🟢 single entry point `getModules()`
- 🟢 SHARED and STATIC targets (STATIC recompiles `sources/sdl3.cpp` for
  a consumer that doesn't need it — known, not fixed)

### Measured failure, not fixed

- 🔴 `createTexture`/`createSoundBuffer` etc. return a dead object
  (`isReady()==false`) instead of `nullptr` when loading fails.
  same forgiving behaviour as sdl2

## Proposed, not written

- 🟡 `claims()`: nothing — same reason as sdl2
- 🟡 STATIC target → INTERFACE, zero object code
- 🟡 examples: still has none
