# Status — RE #6 fourth-pass semantic audit checkpoint (NOT FINAL)

## Verdict

The ordinary Motorola 68000 program remains 100% semantically/source represented in the separate preservation reconstruction (84,142 / 84,142 ordinary-68000 bytes, 0 source-level unresolved bytes), but this portable readable C/H tree **does not yet have 100% source-guided high-level closure**.

A fourth audit, performed from the previously labelled `RE6_ThirdPass_Corrected` archive and compared again with the surviving September 1994 assembly, found substantive CPU/game-state logic that was collapsed into callbacks, abbreviated compatibility surfaces, or omitted local-label continuations. Therefore the prior "final" / "100% readable C/H closure" wording is withdrawn.

## Confirmed semantic blockers

The fourth pass has confirmed at least these blockers:

- `AMP/AMP.S`: `initialise` omits the historical 16-byte `init_invent` -> `inventory_table` initialization.
- `AMP/AMP.S`: `build_level` currently stops after free-list setup, but the source continues through player-type selection, visited-level marking, `level_loop`, and initial creature population.
- `AMP/AMP.S`: `restore_level` omits first-visit versus revisit control flow, `place_grid`, `append_objs`, cocoon restoration, random population, `rebuild_level`, and special level 14/15 Queen/Predator setup.
- `AMP/AMP.S`: active local routines/continuations such as `rebuild_level`, `ram_save`, `xcocoons`, and `rand_set` are not yet represented explicitly enough for source-equivalent C closure.
- `MAZE/HUD.S`: cocoon save restoration is incomplete: `ccn_xsave`/`ccn_ysave`, packed save extraction, and the `UseCocoon` carry-over into an unvisited destination level are missing.
- `AMP/FONT.S`: encoded-string parsing, proportional font metrics, control bytes, wrapping, controller/typewriter behavior, and cursor timing are not preserved by the current minimal host text wrapper.
- `MAZE/COMPUTER.S`: terminal input semantics and dispatcher/game-state logic are substantially abbreviated. In particular, source `c_readpad` repeat behavior and active-low -> active-high inversion do not match the current C implementation.
- `MAIN/MAIN.S`: the persistent title/game/post-game sequencing and `PlayAvP` orchestration are not represented by the current compact `main_game.c` surface.
- `MAZE/MAZE.S`: `ResetMaze` omits the source call to `hug_init`.

See `docs/RE6_FOURTH_PASS_AUDIT.md` for the audit detail and corrected closure criterion.

## Mechanical validation

The fourth-pass source tree still passes the existing mechanical gates (strict optimized GCC/Clang builds, regression tests, whole-archive links, unfinished-marker scan used by the existing validator, and public/restricted-payload audit). Those results remain useful, but they are **not evidence of semantic completeness** because omitted source logic can be unreachable from the current C call graph and therefore invisible to link/tests.

## Correct completion criterion

Do not declare the portable C/H tree complete from exported routine names alone. The next audit must cover active source control-flow blocks, including reachable local labels and fallthrough regions. Every active retail CPU-side block must map to either:

1. explicit readable C semantics; or
2. a documented hardware/resource/backend boundary with proof that no game-state/control semantics were discarded.

Jaguar GPU/DSP programs remain separate processor domains. The historical source tree, retail ROM/assets, and restricted third-party material remain excluded from public packages.
