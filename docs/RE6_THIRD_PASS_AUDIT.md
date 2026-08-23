# RE #6 third-pass audit correction

A fresh source-to-C audit was performed after the package previously labelled
"Final Doubly Audited".  That package is superseded by this tree.

The third pass found one substantive routine-closure defect in the readable C
translation of `AMP/AMP.S::lockxxx`: the first RE #6 lift represented the entry
point but simplified its internal Alien Queen continuation too aggressively.
The omitted/incorrectly collapsed behavior included the trigger-driven retreat,
qfight delay/selection and frame/damage stream, recoil restart, death animation,
non-Predator card-10 resurrection path, Predator 20-tick end-game path, and the
68000 `BCHG` old-bit branch semantics used by death/regeneration animation.

This tree restores those continuations in `src/game/amp.c` and adds focused
regression coverage for Queen fight timing/damage and Predator Queen death/end
flow.  HUD exported labels were also rechecked: `show_mt` is data, while
`UpdtHUD`, `DecPrint`, `DecCommon`, `HexPrint`, `ResetMap`, and `ShowMap` are
present C functions and were not missing routines.

Validation after the correction: strict optimized GCC build, strict optimized
Clang build, regression suite, whole-archive links, unfinished-marker scan, and
public-payload audit all pass.  The high-level C claim boundary remains semantic
routine-level closure; the separate preservation reconstruction remains the
byte-exact binary oracle.
