# Active ordinary-68000 retail build graph audit

Derived from the surviving `MAIN/MAKEFILE`. This list is intentionally a module-name audit, not redistributed historical source.

## RAM-resident RDB program

`main.o`, `message.o`, `blitter.o`, `jaguar.o`, `joypad.o`, `objects.o`, `joyed.o`, `music.o`, `files.o`, `sprites.o`, `maze.o`, `mazescrn.o`, `govers.o`, `levels.o`, `player.o`, `spctrl.o`, `sp.o`, `doors.o`, `collide.o`, `hud.o`, `hud_msg.o`, `computer.o`, `amp.o`, `font.o`, `remjp.o`, `clearjag.o`, `fame.o`, `mjpfont.o`, `intro.o`, `listin.o`, `listsel.o`, `listtit.o`, `select.o`, `title.o`, `win.o`, `avpcart.o`, `eeprim.o`.

Readable-C mapping is documented in `TRANSLATION_MATRIX.md`. The MJP-era objects are grouped into `mjp.c`; restricted Spacetec controller source is replaced by a clean interface; `eeprim.o` is a hardware backend boundary.

## ROM bootstrap

`rom.o`, `alloc.o`, `inflate.o`, `util.o`.

All four now have explicit readable-C representations under `src/unzip/`.

## Not ordinary-68000 semantic modules

Linker marker objects (`startseg.o`, `endsegs.o`), Jaguar GPU images/programs, DSP/FullSynth products, and raw binary/resource payloads are not counted in the ordinary-68000 C denominator.
