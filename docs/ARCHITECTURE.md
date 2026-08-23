# Architecture and port boundary

The readable C tree separates the original Jaguar program into three domains:

1. **ordinary Motorola 68000 game/control code** — represented here in readable C;
2. **Jaguar GPU/DSP programs** — separate processor domains retained by the preservation project / supplied by a Jaguar backend;
3. **retail data/resources** — supplied from the user's ROM and never committed here.

`AvpRuntimeOps` is the portability seam. Rendering, sound playback, resource transfer, vblank, access-table queries and specialized hardware services can be replaced by native host implementations without rewriting authoritative game rules.

Player state, movement integration, weapons, door state, AMP state, level progression, HUD state, save semantics and front-end/control orchestration remain on the C side. Collision queries that historically walked fixed Jaguar maze/AMP storage can be supplied by the host while preserving the same decision ordering.

For a PC port, the original gameplay/simulation cadence should remain authoritative; smooth high-refresh rendering belongs above this layer rather than silently changing simulation timing.
