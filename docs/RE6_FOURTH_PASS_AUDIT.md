# RE #6 fourth-pass semantic audit

Date: 2026-08-23

## Result

**FAIL — prior final readable-C closure claim withdrawn.**

The archive previously labelled `RE6_ThirdPass_Corrected` passes its checksum, build, test, link, and public-payload gates. A fresh comparison against the surviving September 1994 ordinary-68000 source nevertheless found source-visible gameplay/control semantics that are not faithfully represented in the portable C tree.

This is not a byte-identical-source claim issue and not merely missing original comments/macros. The defects below affect executable CPU-side behavior/state.

## Why the earlier audit missed them

The earlier strengthened audit concentrated heavily on exported `::` labels and a finite TODO list. That is insufficient for this source tree. Substantive active code is frequently reached through single-colon local labels, branch targets, or fallthrough from exported entries. A C function can therefore exist under the historical exported name while representing only the entry fragment of the source routine.

Build and link gates cannot detect logic that was never translated into the C call graph. Regression tests likewise cannot prove a path that has no implementation.

## Confirmed blockers

### AMP/AMP.S — initialization and level construction

Historical `initialise` clears AMP modes/maps and disc state, then copies 16 bytes from `init_invent` into `inventory_table`. The current C implementation does not represent that inventory initialization.

Historical `build_level` initializes the AMP free list and then **continues** into player-type classification, sets the current level's visited bit, enters `level_loop`, chooses the authored creature list (retail uses the common-list path under the final conditional), and populates initial AMP records through `next_creature`. The current C `build_level` returns immediately after free-list initialization.

### AMP/AMP.S — level restore/save lifecycle

Historical `restore_level` performs substantially more than `initialise + build_level`:

- resets projectile/generator state;
- initializes the current AMP block;
- calls `place_grid`;
- branches on `levels_visit`;
- first visit: `build_level`, `append_objs`, cocoon restoration (`xcocoons`), random population (`rand_set`), grid cleanup, and level-specific Queen/Predator population for levels 14/15;
- revisit: `rebuild_level`, then grid cleanup.

The current C surface does not preserve that control flow. Related active local routines including `rebuild_level`, `ram_save`, `xcocoons`, and `rand_set` therefore require explicit source-guided closure or a proven equivalent representation.

### MAZE/HUD.S — cocoon persistence

Historical Alien cocoon state uses `ccn_xsave` and `ccn_ysave`, decodes three cocoons from two packed save-game longs in `InitCocoons`/`extract_cocoon`, and records the used cocoon destination coordinates so AMP restoration can materialize a cocoon on a never-before-visited destination level.

The current C tree has no `ccn_xsave`/`ccn_ysave`, leaves `extract_cocoon` empty, and does not perform the packed restoration/carry-over behavior.

### AMP/FONT.S — text engine

The historical font module is more than a pixel-blit backend. CPU-side logic includes font-brush scanning into proportional character metrics, encoded string coordinates/content, control bytes (including teletype mode and coordinate changes), wrapping/layout, controller-driven exit/typewriter behavior, and flashing cursor timing.

The current C implementation treats input as an ordinary NUL-terminated C string, advances by a fixed width, and draws a literal underscore cursor. Hardware pixel transfer may remain backend-owned; these parsing/state/timing semantics may not.

### MAZE/COMPUTER.S — terminal state machine

Historical `InitComp` initializes the font. Historical `c_readpad` clears `repeat_pad` and returns without reading when repeat is set; otherwise `xc_readpad` reads the controller and bitwise-inverts both current/edge masks for the computer UI's active-high interpretation. The current C implements different repeat behavior and does not invert the masks.

Historical `Computer` also indexes per-level 10-byte computer descriptors by `comp_offset`, loads page/base state, and dispatches through a handler vector. The current C reduces this to display setup plus a generic frontend event. The many terminal/menu handlers reached internally are part of ordinary-68000 control/game logic and cannot be excluded merely because they are not exported `::` labels.

### MAIN/MAIN.S — game/front-end orchestration

Historical `main_start` initializes EEPROM/controller/files/video/audio and enters the persistent title -> game -> post-game/front-end loop. `NewTitle` performs title/menu/load-save/character-select/high-score/intro sequencing, and the source contains the `PlayAvP` gameplay orchestration and species-specific post-game flows.

The current compact `main_game.c` does not represent that complete CPU-side sequencing. Hardware setup calls may be backend boundaries, but menu/game/post-game state flow is not.

### MAZE/MAZE.S — ResetMaze

Historical `ResetMaze` calls `hug_init` before map/GPU reset, clears `alien_bite`, and starts ambient audio. The current C omits `hug_init`.

## Corrected audit method

The next closure pass should build a source-block/control-flow manifest per active retail module, not just an exported-symbol manifest. For each active source region, record one of:

- direct C function/block mapping;
- intentional merged C helper mapping, with source-equivalence note;
- authored data/resource region;
- Jaguar hardware/GPU/DSP/backend operation with an explicit host boundary;
- retail-disabled conditional/debug code.

Reachable single-colon labels and exported-entry fallthroughs must be included. The preservation reconstruction remains the behavioral/instruction-level oracle when the surviving source and portable abstraction disagree.

## Mechanical gates

At the time of this audit, the prior package still passes the existing strict GCC and Clang validation, tests, whole-archive linking, checksum verification, and public/restricted-payload audit. These gates remain mandatory for a future final release, but the fourth pass demonstrates they are necessary rather than sufficient.
