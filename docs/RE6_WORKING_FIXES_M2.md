# RE #6 — Working semantic fixes M2

Date: 2026-08-23

This document records the verified corrections included in the GitHub working snapshot created after the fourth-pass audit. It is deliberately not a final-completion statement.

## 1. 68000 low-word displacement correction

The historical source declares `maze_width` and `maze_height` as longwords. Instructions such as `move.w maze_width+2,d0` and `mulu.w maze_width+2,d7` use `+2` as an **address displacement** to the low 16-bit word of the longword variable. The previous C incorrectly translated this as mathematical `width + 2` in several places. The affected player, door, and level-grid indexing code in this snapshot now uses the actual width/height value.

## 2. Access initialization

`DOORS.S::InitAccess` gives Human/Marine `INIT_ACS` (0 in the shipping `NO_DEBUG` build) and gives Alien/Predator 10. The previous C set every player type to zero.

## 3. ResetMaze facehugger reset

`MAZE.S::ResetMaze` begins with `hug_init`. The C now performs this call before `ResetMap` / `ResetMGPU`, then clears `alien_bite` and starts ambient audio as before.

## 4. Cocoon save/restore semantics

The snapshot restores `HUD.S` cocoon persistence details that were previously absent or simplified:

- `ccn_xsave` and `ccn_ysave` globals;
- exact bit-manipulation structure for extracting three cocoon records from the two packed save longs;
- coordinate centering at low word `$8000`;
- empty/start/restored-frame handling;
- negative-time forced redraw without incrementing the frame;
- `UseCocoon` destination carry-over state;
- source-equivalent record shifting and first-slot frame reset.

The AMP first-visit consumer of `ccn_xsave` / `ccn_ysave` is still part of the open AMP lifecycle audit and is not claimed complete in this snapshot.

## 5. Validation

The working tree passes both:

```text
./tools/validate.sh
CC=clang ./tools/validate.sh
```

This covers strict optimized builds, the current regression suite, whole-archive links, unfinished-marker scan used by the validator, and the public/restricted-payload audit.

These gates validate the working snapshot mechanically; they are not a substitute for the ongoing source-block semantic audit.
