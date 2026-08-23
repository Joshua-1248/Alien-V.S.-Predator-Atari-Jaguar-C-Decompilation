# RE #7 source-block closure method

This pass uses a fail-closed matrix for the active ordinary Motorola 68000 retail program.

A reachable source block is closed only when it is recorded as one of:

- `C_EXACT`: the ordinary-68000 state/branch/arithmetic behavior is explicit C.
- `C_MERGED_PROVED`: the source block is intentionally merged into a C helper and source-equivalence is documented.
- `HARDWARE_RESOURCE_ONLY`: the CPU performs no omitted gameplay/control decision; the remaining operation is Jaguar GPU/DSP/Object-Processor/blitter/MMIO/resource presentation work.
- `DEAD_OR_RETAIL_DISABLED`: the block is excluded by the retail build configuration or unreachable shipping/debug code.
- `DATA`: authored/resource/data region, not executable ordinary-68000 logic.

`tools/re7_closure_matrix.py` consumes the surviving historical source from an **external private path** and emits only label/line/status metadata. It never copies historical source into the public tree. Its `symbol_hint` is diagnostic only and can never auto-certify equivalence.

## Safety rules

1. Exact retail 68000 instructions/source control flow outrank the previous C implementation.
2. A passing build/test does not close a block.
3. An event/callback is not a valid closure unless the source block is proved hardware/resource-only.
4. Local labels, fallthrough paths and conditional-build regions are audited, not only `::` exports.
5. Retail-disabled/debug blocks are proved from build conditionals before classification.
6. Every code change must exist in the working tree, receive a ledger entry, and survive GCC + Clang validation before being reported as closed.
7. Private/restricted historical source and binary oracle material never enters the public package.

## Work order

Risk-ranked batching is used to make the pass faster without changing the proof standard:

1. event/callback-only live CPU routines;
2. gameplay/control state machines;
3. front-end/HUD CPU state machines;
4. source-local continuations/fallthroughs;
5. hardware/resource boundaries;
6. dead/debug/data classification;
7. final zero-red matrix, clean ZIP extraction, GCC/Clang/test/link/public-payload validation.
