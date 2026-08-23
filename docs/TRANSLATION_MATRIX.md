# Ordinary 68000 → readable C translation matrix

This matrix tracks the **active shipping program**, not every archived/SAFE/OLD/test copy in the surviving source tree. The byte-exact preservation repository remains the acceptance oracle.

| Shipping domain / historical module | Readable C target | Status / boundary |
|---|---|---|
| `MAIN/MAIN.S` startup/control | `src/game/main_game.c` | represented; fixed-address hardware work is backend-owned |
| `MAIN/ROM.S`, allocator/inflate bootstrap | `src/unzip/*` | represented as separate historical-ABI libraries |
| `JAGUAR/jaguar.o` | `src/platform/jaguar/jaguar.c` | represented; MMIO remains Jaguar-specific |
| `JAGUAR/blitter.o` | `src/platform/jaguar/blitter.c` | represented |
| `JAGUAR/joypad.o` | `src/platform/jaguar/joypad.c` | represented |
| `JOYED` | `src/game/joyed.c` | represented |
| `FILES/files.o` | `src/game/files.c` | represented; payload bytes come from user ROM |
| `SPRITES/sprites.o` | `src/game/sprites.c` | represented; sprite payloads are external data |
| `MESSAGES/message.o` | `src/game/message.c`, `hud_message.c` | represented |
| `AVPSOUND/music.o` + CPU sound control | `src/game/music.c`, runtime callbacks | represented; DSP/FullSynth is separate processor domain |
| `OBJECTS` / Object Processor CPU support | `src/game/objects.c` | semantic reconstruction represented; Jaguar phrase execution is backend-owned |
| `MAZE/MAZE.S` | `src/game/maze.c` | represented |
| `MAZE/MAZESCRN.S` | `src/game/mazescrn.c`, `weapons.c` | represented, including typed first-person command streams |
| `MAZE/GOVERS.S` | `src/game/gpu_overlays.c` | 68000 orchestration represented; GPU programs remain GPU-domain |
| `MAZE/LEVELS.S`, `LEVINFO/LEVDOOR` semantics | `src/game/levels.c`, `doors.c` | represented; authored tables remain resource data |
| `MAZE/PLAYER.S` | `src/game/player.c`, `collectables.c` | represented: init/update, movement, controls, weapons, map/debug retail paths, pain, pickups |
| `MAZE/DOORS.S` | `src/game/doors.c` | represented: persistence, access seam, opening/motion/double-door state; collision backend is explicit |
| `MAZE/COLLIDE.S` | `src/game/collision.c` + runtime collision seam | CPU math/helpers represented; world-query backend is explicit where host geometry replaces fixed Jaguar data |
| `MAZE/HUD.S` | `src/game/hud.c`, `hud_score.c` | represented: HUD state, countdown, map/cocoon state, score/access state; drawing is backend-owned |
| `MAZE/HUD_MSG.S` | `src/game/hud_message.c` | represented |
| `MAZE/COMPUTER.S` | `src/game/computer.c` | control/input/display orchestration represented; terminal page corpus is ROM data |
| `MAZE/AVPCART.S` / EEPROM | `src/game/eeprom.c` | represented |
| `AMP/AMP.S` | `src/game/amp.c` | represented: allocation/lifecycle, creature/projectile/generator/shield/queen state and runtime hooks |
| `AMP/FONT.S` | `src/game/font.c` | represented; glyph pixels are resource data |
| `MJP` front-end block | `src/game/mjp.c` | semantic reconstruction of named retail routine surface; graphics/object payloads are external |
| `CONTROL/SP.S` restricted third-party source | **not redistributed** | intentionally replaced by clean controller-facing interfaces; restricted historical source excluded |

## Excluded from the 68000 denominator

- Jaguar GPU RISC programs (`MAZE*.GAS`, decompression/conversion GPU routines, etc.).
- Jaguar DSP / FullSynth programs and tables.
- Archived `SAFE/`, `OLD/`, editor/test copies that are not separate retail shipping modules.
- Retail artwork/audio/level/resource payloads.
- Linker-only segment marker/data wrapper files with no independent ordinary-68000 gameplay semantics.

## Completion meaning

“Represented” means the active ordinary-68000 behavior has a readable C surface or an explicit, documented hardware/resource backend boundary. It does **not** mean literal recovery of the original authors' exact C/assembly source text or byte-identical recompilation from this high-level tree.
