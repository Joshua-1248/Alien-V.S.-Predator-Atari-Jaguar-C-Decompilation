# Provenance

## Primary sources

1. Canonical World retail ROM behavior and the byte-exact reconstruction/oracle repository.
2. The surviving near-final September 1994 Jaguar assembly source tree (`MAIN`, `MAZE`, `AMP`, etc.) used as historical reference where available.
3. Reconstructed missing modules (`OBJECTS`, `MJP`, Jaguar support, FILES, MESSAGES, AVPSOUND, UNZIP) from the preservation project.
4. Prototype builds used only as comparative evidence where retail semantics are ambiguous.

## C conversion rule

A C function should express the established runtime semantics, not mimic register allocation. Address/bit-width/order-sensitive behavior is documented in comments when it matters. The exact preservation repository remains the binary oracle.

## Distribution rule

No retail assets, ROMs, exact retail binary function slices, or proprietary third-party historical source files are included.
