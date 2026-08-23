# Status — RE #7 ordinary-68000 readable-C closure

**Date:** 2026-08-23

## Verdict

The readable portable C/H reconstruction has reached the RE #7 finish criterion for the **ordinary Motorola 68000 CPU domain**: every reachable executable label in the surviving retail source inventory is now classified as either explicit/merged C, a proved hardware/resource-only boundary, or dead/retail-disabled code. The generated closure matrix reports **0 unresolved executable labels**.

The separate preservation/oracle effort remains the byte-exact authority and already covers 84,142 / 84,142 ordinary-68000 bytes. Jaguar GPU/DSP programs remain separate processor domains and are intentionally outside this 68000-to-C denominator.

This does **not** claim recovery of every original comment, symbol name, macro expansion, compiler decision, or proprietary resource byte. It is a semantic readable-C reconstruction of the shipping ordinary-68000 behavior suitable as the baseline gameplay/control core for a faithful native port.

## RE #7 closure work

RE #7 completed the source-block/local-label audit that earlier passes had not finished. Major closure areas included:

- MJP/front-end local targets, authored character-select trajectory/state, Object Processor list phrase manipulation, title/select/end/fame controllers and InitMJP state;
- pause controller, maze-list save/restore, pause-audio CPU bookkeeping and weapon/overlay state;
- LEVELS panel/overlay CPU state and arithmetic, plus fixes to place-grid, saved-level and level-entry semantics;
- DOORS, MAIN, PLAYER, MAZE, COLLIDE, HUD, COMPUTER and small-module source-span closure;
- AMP lifecycle, placement/random population, projectile/chase/collision, creature state machines, Queen/endgame, scoring, cocoon/egg/generator/shield/spark and related ordinary-CPU behavior;
- callback/event seam audit so gameplay/control defaults to translated C and Jaguar-only resource/GPU/DSP/blitter work stays behind typed backend boundaries.

The fail-closed matrix and ledger are in `research/re7_closure/`.

## Validation requirement

The release package is valid only if both GCC and Clang strict validation pass from a clean extraction, internal SHA-256 verification passes, whole-archive links pass, all C regression tests pass, and the public/restricted-payload audit passes.

Run:

```sh
./tools/validate.sh
CC=clang ./tools/validate.sh
```

## Public/private boundary

The public repository contains no retail ROM, extracted retail media, private oracle slices, restricted historical Spacetec source, or proprietary historical tool binaries. Those materials remain private/reference-only where used for verification.

## RE #7 post-final independent re-audit

An independent clean-package audit found that the original-source closure matrix did not itself cover MJP because the historical MJP source is lost. A separate disassembly audit found and corrected remaining Hall-of-Fame CPU semantics (`Do_Fame` / `Fame_Tex`, score decimal formatting, branch ordering, and authored no-score text selection). The corrected package supersedes the first RE #7 final candidate. See `docs/RE7_MJP_FINAL_AUDIT.md`.
