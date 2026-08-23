> **SUPERSEDED BY FOURTH-PASS AUDIT:** The exported-routine TODO list below was completed, but that did not prove active source-block closure. See `docs/RE6_FOURTH_PASS_AUDIT.md`.

# Routine closure status

The pre-final routine-level closure list is complete as of RE #6.

AMP/AMP.S historical-name C lifts completed: `level_loop`, `next_creature`, `append_objs`, `gofight`, `QFRAME`, `stun_mode`, `stun_death`, `cocoon`, `do_score`, `amp_setgrid`, `eggwait`, `lockxxx`.

Required internal continuations reached by those routines (egg opening/hugger delay, dead-egg conversion, Queen lock/recoil/death paths) are represented rather than replaced with empty wrappers. Authored placement/template payloads remain data/resource-side and are bound through the public AMP data interfaces.

MAZESCRN/PLAYER exported-label closure and aliases are documented in `docs/ROUTINE_ALIAS_AUDIT.md`.

No remaining routine-level C lift is known from the strengthened audit. Final release status still depends on all validation/package gates passing from a clean extracted archive.
