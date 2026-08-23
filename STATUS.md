# Status — RE #6 working semantic-fix snapshot (NOT FINAL)

## Verdict

The separate preservation reconstruction still provides the 100% ordinary-68000 semantic/source oracle (84,142 / 84,142 bytes). This portable readable C/H tree is **still under active source-block/control-flow audit** and must not yet be tagged as the final 100% readable-C release.

This working snapshot contains verified corrections discovered after the fourth-pass audit and is suitable for updating the GitHub working branch.

## Corrections applied in this snapshot

- Corrected the 68000 `maze_width+2` / `maze_height+2` address-displacement misunderstanding in the translated player/door/level indexing paths. In the assembly these expressions address the low word of 32-bit variables; they do **not** mean mathematical width/height plus two.
- Corrected `InitAccess`: retail Human/Marine starts at access level 0 while Alien and Predator start at access level 10.
- Corrected `ResetMaze` to call `hug_init` before map/GPU reset, matching `MAZE.S`.
- Restored HUD cocoon persistence state `ccn_xsave` / `ccn_ysave`.
- Restored the three-cocoon packed save-game decode from the two save longs used by `HUD.S`.
- Corrected restored-cocoon redraw semantics: a negative cocoon timer forces redraw of the current frame without advancing it.
- Corrected `UseCocoon` shifting/reset behavior: the two younger cocoon records shift toward the oldest slot and only the first slot's frame word is reset to `COCOON_EMPTY`, as in the assembly.
- `UseCocoon` now records the destination position in `ccn_xsave` / `ccn_ysave` for the AMP first-visit carry-over path.

## Still open before a 100% proper claim

The source-block audit is continuing. Confirmed areas that still require completion or exact re-verification include:

- AMP level lifecycle and first-visit/revisit population/save-restore paths (`build_level`, `restore_level`, `rebuild_level`, `ram_save`, `xcocoons`, `rand_set`, and related local continuations).
- The AMP consumer of `ccn_xsave` / `ccn_ysave` for first visits to a destination level.
- Source-exact `COLLIDE.S` wall scanning / `FireDistance` / line-of-sight behavior.
- Full `FONT.S` encoded/proportional text engine semantics.
- Full `COMPUTER.S` terminal input/dispatch/state-machine semantics.
- Full `MAIN.S` title -> gameplay -> post-game orchestration.
- Remaining local-label/fallthrough/state-table closure across the rest of the active shipping 68000 source and reconstructed-source modules.

## Mechanical validation of this snapshot

Both GCC and Clang strict Release validation pass after the fixes above, including regression tests, whole-archive linking, unfinished-marker scan used by the validator, and the public/restricted-payload audit. These are necessary gates, but are **not** being used as proof of semantic completion.

## Completion criterion

A final release requires active source-block/control-flow closure: every reachable ordinary-68000 CPU-side block must map to explicit readable C semantics or a documented hardware/resource/backend boundary that demonstrably discards no gameplay/control semantics. GPU/DSP programs remain separate processor domains.
