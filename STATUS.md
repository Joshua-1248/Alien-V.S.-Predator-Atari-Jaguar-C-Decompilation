# Status — RE #6 end-of-session advanced C/H working tree (NOT FINAL)

**Date:** 2026-08-23

## Truthful verdict

The separate byte-exact preservation/reconstruction effort remains the acceptance oracle and has 100% ordinary Motorola 68000 semantic/source representation (84,142 / 84,142 bytes). The portable readable C/H tree in this directory has advanced substantially beyond the earlier M2 working snapshot, but **must not yet be tagged as the final 100% readable-C release**.

The remaining risk is no longer broad module coverage. It is final source-block/control-flow equivalence, particularly the historically source-missing MJP front-end block and any residual CPU gameplay/control decisions that could still be hidden behind a host/backend event seam.

## Major corrections/lifts in the current advanced tree

This tree includes the M2 corrections plus later RE #6 work, including:

- source-shaped `COLLIDE.S` wall scanning / `FireDistance` path and direct `TestSpark` integration;
- corrected `maze_width+2` / `maze_height+2` 68000 address-displacement interpretation in remaining collision/maze paths;
- direct CPU fallbacks for translated `SafePos` and `AreaDamage` so gameplay does not silently depend on host callbacks;
- expanded AMP level first-visit/revisit lifecycle, RAM-save/cocoon carry-over, random population, level-14 Queen and level-15 Predator setup, and Alien end-Queen continuations;
- direct Queen/chase/lift-cell gameplay fallbacks where earlier code depended on callbacks;
- source-order door/lift/airlock/duct behavior, including fixes for the local `move_doors` label trap, double-door direction, lift-panel return ordering, and 0xFF sentinel handling;
- full saved-game pointer integration across level/start/player state and first-person inventory/ammo/weapon restore;
- 64-bit-safe host representation of the historical 32-bit save pointer;
- restored `HumanPain` and species-specific pain sound selection; Human AMP death/recoil routes through it;
- corrected player minimum-speed clamp ordering to compute the 68000 speed approximation after current-frame acceleration;
- restored `NextFrame` GPU-view snapshot semantics and Alien bite view shift;
- expanded proportional/encoded `FONT.S` text behavior;
- expanded live `COMPUTER.S` terminal input/menu/progression behavior;
- restored top-level `MAIN.S` title -> load/select -> `PlayAvP` -> ending -> Hall-of-Fame control flow;
- corrected retail end-game explosion start delay to 22 updates;
- restored MJP modes 7/8 (Simulation Terminated / Base Explodes);
- recovered MJP retail resource dispatch, title/select/intro/escape/win/lose controllers, Hall-of-Fame packed EEPROM format/default entries, and multiple exact helper semantics;
- fixed title-menu Hall-of-Fame stale-score clearing;
- recovered `New_Make`, `Pause`, `Make_Bet`, `EncodeGa`, `Do_Win` continuation, and related MJP behavior from exact retail disassembly.

## Current mechanical validation

At the RE #6 handoff point:

- GCC strict Release build: PASS
- GCC regression suite: PASS
- GCC whole-archive game/ROM link: PASS
- active-source unfinished-marker scan: PASS
- public/restricted-payload audit: PASS
- Clang strict Release build: PASS
- Clang regression suite: PASS
- Clang whole-archive game link: PASS
- Clang public/restricted-payload audit: PASS

These are necessary gates, not proof of semantic completion.

## Remaining work before a final 100% readable-C claim

1. Finish/verify **MJP source-block equivalence** against the exact 46 retail entry boundaries and local-target/call-graph manifests. The current C has substantial controller lifts, but several object/list/update helpers still terminate in typed frontend events and need proof that only Object Processor/presentation mechanics remain behind those seams.
2. Recompare the current `Do_Title`, `Do_Selec`, `Do_Fame`, `Do_Intro`, `Do_Escap`, `Do_Lose`, `Do_Win`, `Show_Tex`, and helper continuations against the exact retail disassembly, including local-label targets.
3. Complete the final **active source-block/local-label/fallthrough sweep** across all surviving shipping 68000 source and reconstructed-source modules. Do not use exported-routine-name existence as the criterion.
4. Scan every remaining callback/event seam and prove it is hardware/resource/presentation-only; if it owns CPU gameplay/control semantics, move that logic into C.
5. Update `ROUTINE_ALIAS_AUDIT.md` / translation documentation with the final source-block proof.
6. Only after semantic closure: regenerate status/release docs, run GCC+Clang+tests+whole-archive+payload audit, make the final clean ZIP, verify its internal hashes, extract it from scratch, and rerun the full validation from that extracted copy.

## Tool note from end of RE #6

`xxd` is not installed in the environment. Two install attempts failed (first timed out, second exited unsuccessfully). Do **not** waste time retrying it unless absolutely necessary. Use already available alternatives such as `od`, Python byte reads, or the existing `m68kmini.py`/disassembly outputs.

## Public/private boundary

The public C repository must not contain the retail ROM, extracted retail media, private binary oracle slices, the private historical `Source Code.zip`, restricted Spacetec source, or proprietary historical tool binaries. Historical/private material in the next-session core bundle is reference-only.
