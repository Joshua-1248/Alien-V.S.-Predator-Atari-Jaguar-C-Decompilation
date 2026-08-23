> **HISTORICAL / SUPERSEDED:** Completion wording in this historical checkpoint is withdrawn by the fourth-pass semantic audit.

# v1.0.0 — ordinary 68000 readable-C baseline

This release closes the first complete readable-C representation pass over the active ordinary Motorola 68000 shipping program.

Highlights:

- source-guided player movement, weapon input, pain, pickups and level/map control;
- typed `MAZESCRN.S` first-person weapon command-stream interpreter;
- AMP lifecycle, projectile, generator/shield and Queen/state logic;
- doors, persistence and active opening/motion state;
- HUD message/countdown/score/map/cocoon state;
- levels, terminals/computer, EEPROM/save support;
- reconstructed FILES, SPRITES, MESSAGE, MUSIC, OBJECTS and MJP CPU-side interfaces;
- explicit GPU/DSP/resource/backend boundaries suitable for a native host;
- historical ROM unzip allocator/inflate isolated from the portable game library;
- strict whole-tree build and whole-archive link gates;
- code-only/publication audit with no retail ROM/media payloads.

Important scope boundary: this release is a readable semantic reconstruction, not a claim to reproduce every original source line or compiler output. GPU and DSP programs are intentionally not translated as 68000 C.
