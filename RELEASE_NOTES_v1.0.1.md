# v1.0.1 — verification hardening

This maintenance release follows a clean-room re-extraction and double-check of v1.0.0.

- Fixed hosted optimizing-compiler diagnostics for the Jaguar low-memory vector writes at addresses 0 and 4 by routing those writes through an explicit Jaguar hardware helper. The address/value behavior is unchanged.
- Strengthened `tools/validate.sh` to perform a strict optimized Release build.
- Strengthened the standalone regression-test compile to use `-O2` with `-Wall -Wextra -Werror`.
- Re-ran the complete regression suite, whole-archive link, public payload audit, and checksum verification from a fresh extraction.

No game semantics were intentionally changed by this maintenance release.

- Added explicit readable C representations for `MAIN/ROM.S` bootstrap orchestration and the 440-byte retail `util.o` semantic surface after cross-checking the historical `ROM_OBJS` list.
- Added ROM-util and ROM-bootstrap regression tests.
