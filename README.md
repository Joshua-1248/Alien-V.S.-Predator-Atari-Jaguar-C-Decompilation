# Alien vs Predator (Atari Jaguar) — C Decompilation

Readable high-level C reconstruction of the ordinary Motorola 68000 portion of **Alien vs Predator** for the Atari Jaguar.

This repository is intentionally separate from the byte-exact preservation repository:

- **Exact preservation/oracle:** [Alien V.S. Predator Atari Jaguar Decompilation](https://github.com/Joshua-1248/Alien-V.S.-Predator-Atari-Jaguar-Decompilation)
- **Historical toolchain reconstruction:** [Atari Jaguar 1994 Toolchain Reconstruction](https://github.com/Joshua-1248/Atari_Jaguar_1994_Toolchain_Reconstruction)

## Status

The active ordinary-68000 shipping program now has a readable C/H representation across startup, Jaguar support, files/resources, Object Processor orchestration, audio control, maze/gameplay, player movement, collision interfaces, doors, HUD, weapons, AMP/creature state, progression/computer logic, EEPROM/save support, and the reconstructed MJP/front-end block.

This is **not** a claim that every original 1994 source line, comment, macro, variable name or compiler decision has been recovered. Where historical source does not survive (notably reconstructed MJP/OBJECTS/support modules), the C is a readable semantic reconstruction verified against the preservation project. Jaguar GPU/DSP programs are separate processor domains and intentionally remain outside the 68000-to-C denominator.

The tree currently passes:

```text
strict C99 build (-Wall -Wextra -Werror)        PASS
whole-archive game-library link                  PASS
C-decomp regression suite                        PASS
public-tree ROM/media payload audit              PASS
```

Run all gates with:

```sh
./tools/validate.sh
```

## Goal

Express the retail game's ordinary 68000 behavior in understandable `.c` / `.h` while preserving the semantics established by the exact reconstruction. Readability is the priority here; byte-identical compiler output is **not** required from this repository.

The resulting architecture deliberately exposes Jaguar-specific rendering, GPU/DSP, sound playback and ROM-resource work through explicit runtime/backend seams. That makes this repository suitable as the gameplay/control foundation of a native PC port without pretending that Jaguar hardware programs are ordinary 68000 C.

## Asset policy

**No retail game assets are included.** Graphics, sprites, audio, music samples, levels, fonts and other proprietary game data must come from a user's own dumped `.jag` through the preservation/tooling workflow.

The repository also excludes retail binary slices used as reverse-engineering oracles and excludes the restricted historical Spacetec IMC controller source. The controller-facing behavior needed by the game is represented through independently reconstructed interfaces instead.

## Build

```sh
cmake -S . -B build -DAVP_STRICT=ON
cmake --build build
```

`avp_c` is the readable game/68000 library. The historical ROM-bootstrap allocator/inflate lineage is built separately because it has a deliberately old ABI and is not part of the normal RAM-resident game library.

## Portability boundary

`AvpRuntimeOps` is the main host/Jaguar service seam. A native host supplies rendering, sound, ROM-resource, vblank and specialized hardware services while authoritative game state remains in the translated C.

See:

- `docs/ARCHITECTURE.md`
- `docs/TRANSLATION_MATRIX.md`
- `docs/VALIDATION.md`
- `PROVENANCE.md`

## Provenance / licensing

This is a reverse-engineered/source-reconstruction project. Original game code and behavior remain subject to their respective rights holders. New project scaffolding and independently written glue are distinguished from reconstructed/translated material. There is no blanket relicensing of Atari/Rebellion-derived material.

See `LICENSE.md`, `PROVENANCE.md`, `CREDITS.md`, and `THIRD_PARTY_NOTICES.md`.
