# Status

## Readable-C milestone

**Ordinary Motorola 68000 source representation: complete for the active retail build graph.**

The source tree provides C/H representations for every active ordinary-68000 subsystem in the final build graph, including the ROM bootstrap allocator/inflate path. Specialized Jaguar GPU/DSP programs and raw binary/media resources are separate domains and are not falsely counted as C conversion work.

## Validation gates

The release candidate is required to pass all of these:

- C99 strict compile (`-Wall -Wextra -Werror`) for `avp_c` and reconstructed inflate;
- complete static-library build;
- whole-archive link of `libavp_c.a` with zero unresolved symbols and zero duplicate definitions;
- focused semantic regression suite, including collision and DEFLATE;
- source audit for unfinished `TODO`/placeholder markers in shipping C;
- publication audit for ROM/media/restricted-source payloads.

Run `./tools/validate.sh` to reproduce the local gates.

## Claim boundary

“Complete readable C” means the known ordinary-68000 semantics are represented in C and all active subsystem surfaces are present. It does not mean:

- literal recovery of the original developers' missing source text;
- byte-identical compilation from this readable tree;
- conversion of Jaguar GPU or DSP machine programs into C;
- inclusion of proprietary data tables/artwork/audio merely to make a host build self-contained.

The byte-exact repository remains the binary oracle and is the correct project for retail-ROM reproducibility claims.
