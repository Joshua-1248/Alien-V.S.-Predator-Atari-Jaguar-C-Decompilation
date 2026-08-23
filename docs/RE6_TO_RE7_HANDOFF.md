# Alien vs Predator (Atari Jaguar) — RE #6 -> next-session C/H handoff

**Date:** 2026-08-23  
**Project:** readable high-level C/H reconstruction of the ordinary Motorola 68000 portion of *Alien vs Predator* (Atari Jaguar)  
**Current tree:** `/mnt/data/avp_re6_finish` before packaging  
**Truthful status:** **ADVANCED WORKING TREE — NOT FINAL / NOT YET SAFE TO CALL 100% READABLE-C**

---

## 1. Immediate instruction to the next session

Resume from the new advanced source ZIP supplied with this handoff. **Do not restart from M2, the fourth-pass checkpoint, or either of the earlier incorrectly labeled final ZIPs.**

The separate preservation/oracle effort remains 100% ordinary-68000 semantic/source representation (84,142 / 84,142 bytes). The current portable C/H tree is far beyond M2 and passes strict GCC+Clang gates, but the final source-block/control-flow proof is still open, primarily around MJP/front-end reconstruction and any remaining CPU semantics hidden behind backend/event seams.

The finish standard is no longer module closure or exported-routine closure. It is:

> Every reachable ordinary-68000 CPU-side basic block/local-label/fallthrough/state-machine decision maps to readable C, or is explicitly proved to be a hardware/resource/presentation-only boundary that discards no gameplay/control semantics.

Do not declare final completion until that is satisfied and the clean packaged ZIP is validated again.

---

## 2. Why prior “final” claims were withdrawn

RE #5 and early RE #6 progressively raised the audit standard. Several times the tree built and tested cleanly but a deeper source comparison found real missing semantics:

- `TestSpark` existed only as a declaration before being restored.
- `lockxxx`/Queen continuation had been oversimplified.
- `restore_level`, `build_level`, HUD cocoon persistence, `FONT.S`, `COMPUTER.S`, and `MAIN.S` proved that function-name existence did not imply full source equivalence.
- 68000 expressions such as `maze_width+2` had been misread as arithmetic “width + 2”; in the original assembly they address the low word of a 32-bit variable.
- callbacks sometimes masked missing CPU gameplay logic (`AreaDamage`, `SafePos`, chase/lift checks).

Therefore **green builds/tests are necessary but not sufficient**.

Superseded historical packages from this conversation include:

- `AVP_Jaguar_C_Decompilation_RE6_Final_DoublyAudited_2026-08-23.zip`
- `AVP_Jaguar_C_Decompilation_RE6_ThirdPass_Corrected_2026-08-23.zip`
- `AVP_Jaguar_C_Decompilation_RE6_FourthPass_Audit_Checkpoint_NOT_FINAL_2026-08-23.zip`
- `AVP_Jaguar_C_Decompilation_RE6_Working_Fixes_M2_2026-08-23.zip`

M2 is still useful as a stable historical checkpoint/diff base, but it is **not** the latest tree.

---

## 3. Current advanced tree: major changes beyond M2

The end-of-session tree has 45 changed/new source/header/test files relative to M2. Key work completed during the later RE #6 continuation includes the following.

### Collision / hitscan / movement

- Restored a source-shaped `COLLIDE.S` maze scanner / `FireDistance` path.
- Connected `PLAYER.S::TestSpark` to translated CPU `FireDistance` by default.
- Corrected remaining `maze_width+2` / `maze_height+2` address-displacement mistakes in collision/maze indexing.
- Fixed the collision regression test itself: it had modeled a fake border and therefore encoded the old incorrect row stride.
- AMP projectile and chase movement now call translated CPU `SafePos` by default when no host override is supplied.
- AMP explosions now call translated CPU `AreaDamage` by default instead of silently doing no damage without a runtime callback.

### AMP level lifecycle / creatures / endgame

- Expanded first-visit vs revisit level lifecycle.
- Restored RAM-save / cocoon carry-over semantics and random population flow.
- Restored level-14 Queen and level-15 fixed Predator setup paths.
- Restored Alien end-Queen local-label flow including generator check, fight delay, event skip, escape timer, input lock, and game-over transition.
- Reworked Queen chase/retreat and cocoon lift-cell checks so ordinary gameplay logic is not backend-dependent.
- Historical Queen bit-change (`BCHG`) old-bit semantics were already corrected earlier in RE #6.

### Doors / lifts / airlocks / ducts

`DOORS.S` received a much deeper source-order rewrite. Important fixes include:

- lift travel / panel behavior;
- duct exits;
- airlock interlocks;
- access/special panel handling;
- closing-door collision reversal;
- forced lift-door reset;
- double-door direction semantics;
- correct return behavior for lift-panel paths;
- fix for the assembly-label trap where local `move_doors` inside `DoorKeys` only clears `lift_key`/returns and is **not** the exported `MoveDoors::` routine;
- fix for `0xFF` unbound sentinel comparisons accidentally classifying ordinary panels as special panels.

### Player / save/load

- `savegame` uses a host-safe pointer-sized representation; Jaguar on-cart save bytes remain exactly 20 bytes.
- `InitLevels` / `SetStart` now consume the real historical `savegame` pointer, with binders only as test/resource fallback.
- Saved player state is wired through reset/init: species, level, start, score, energy, access, motion tracker, Human ammo, Predator medpak, available weapons, and Alien cocoon state.
- Restored `PLAYER.S::HumanPain` and exact species-specific random pain-sound selection paths.
- Human AMP death/recoil now routes through `HumanPain` as the assembly does.
- Corrected the minimum-speed clamp ordering: the retail speed approximation `3*max(abs(x),abs(y)) + min(abs(x),abs(y))` is evaluated after current-frame acceleration, not before.
- Restored view/GPU snapshot semantics in `NextFrame`, including the Alien bite viewpoint shift before maze-GPU launch.

### HUD / cocoon

Earlier M2 fixes remain in this tree:

- `ccn_xsave` / `ccn_ysave`;
- packed cocoon save decode;
- no-frame-advance redraw when restored timer is negative;
- exact `UseCocoon` shift/reset behavior;
- destination coordinate carry-over.

### Font

`FONT.S` is no longer a fixed-width placeholder. Current C includes:

- proportional glyph metric scanning from the historical 2bpp brushes;
- retail encoded string/control stream handling (`$FE`, `$FD`, `$FF`, embedded coordinates/wrap behavior);
- `repeat_pad` abort semantics;
- 10/10 cursor flash timing;
- drawing/pixels remain resource/backend owned.

### Computer / terminals

- Corrected `c_readpad`/repeat semantics, including inverted computer-interface pad state.
- Live final-terminal dispatch was audited: 132 retail terminals use four active handlers (generic, armoury, medical, pod); legacy `security` code exists but is not a final terminal-table target.
- Restored medical second-level log/species menu behavior.
- Corrected armoury page-source behavior.
- Corrected self-destruct path that returned too early.
- `InitAccess` correction remains: Human/Marine starts access 0; Alien and Predator access 10.

### Main loop / endings

- Restored `MAIN.S` title -> load/select -> `PlayAvP` -> ending -> Hall-of-Fame orchestration.
- `ResetMaze` calls `hug_init`.
- `LoadLevel` and `PlayAvP` are explicit C control paths.
- Retail end-game explosion start delay is 22 updates (old C value 60 was wrong).
- MJP modes 7 and 8 restored: Simulation Terminated / Base Explodes.
- Title-menu Hall of Fame clears `score` first, preventing stale score insertion.

### MJP/front-end

This is the current primary closure area.

The exact retail MJP block has **46 named entry boundaries**:

1. `MTEST`
2. `mjppad`
3. `Load_GPU`
4. `Make_Bet`
5. `EncodeGa`
6. `Clear`
7. `Clear2`
8. `Do_Fame`
9. `Pack_Fam`
10. `Unpack_F`
11. `Init_Hig`
12. `New_Text`
13. `Show_Tex`
14. `Fame_Tex`
15. `Do_Intro`
16. `Do_Lose`
17. `Do_Escap`
18. `One_Tick`
19. `Make_New`
20. `Make_Sca`
21. `New_Make`
22. `Lister4`
23. `Change_Y`
24. `Hide_Obj`
25. `Unhide_O`
26. `No_Scale`
27. `Change_D`
28. `Change_X`
29. `Change_S`
30. `Sel_Upda`
31. `Lister3`
32. `Make_TLi`
33. `Update2`
34. `Update`
35. `Lister`
36. `Do_Selec`
37. `End_Sele`
38. `Pause`
39. `Make_Rel`
40. `Do_Title`
41. `Waitbl`
42. `Waitnow`
43. `Quick_St`
44. `Clear_Bu`
45. `Wait_Up`
46. `Do_Win`

The preservation V3 readable-source status had only 14 of these previously lifted with instruction-level readable proof. During RE #6, the current C tree substantially expanded that coverage using exact binary disassembly and local-target maps.

Confirmed recovered/fixed MJP behavior in the current tree includes:

- exact `mjppad` inversion;
- `Pause` skip mask/direction (`0x22002000`);
- `Make_Bet` using `High_Line`;
- `EncodeGa` 20-byte save-slot stride and literal `mapx` lookup;
- `New_Make` modifies/scales the existing `Ob_Addr`; it does not allocate a new object;
- exact mode/resource dispatcher for MTEST modes 0–8;
- Hall-of-Fame packed format: five `{packed species/name long, score long}` records at EEPROM/cartcopy offset 60;
- Hall-of-Fame alphabet and default entries recovered: MIKE, ANDY, PURPLE, JAMES, KEONI with retail default scores;
- title menu and save-slot choice return contract, including negative save-slot values;
- character-selection species return contract Human=0, Alien=4, Predator=8;
- `Show_Tex` encoded MJP text/typewriter behavior substantially lifted;
- intro/escape/lose/win sequence timing/control substantially lifted;
- `Do_Win` continuation confirmed through `$020190` rather than the older truncated boundary;
- Hall-of-Fame title mode vs score-insertion mode separated;
- Hall-of-Fame title stale-score clear added in main loop.

**Still needs final MJP proof:** several object/list/update helpers (`Lister4`, `Lister3`, `Lister`, `Update2`, `Update`, `Sel_Upda`, `Make_TLi`, and presentation transitions) still terminate in typed `frontend_event` seams. The next session must compare those against exact retail disassembly/local targets and prove they contain only Jaguar Object Processor/list/presentation mechanics. If any CPU selection/timing/state decision is still hidden there, lift it into C.

Exact MJP local-target addresses are preserved in `avp_mjpref/manifests/internal_local_targets.json`; do not audit only the 46 global names.

---

## 4. Current validation status at handoff

Validation run from `/mnt/data/avp_re6_finish` immediately before packaging:

### GCC

- strict Release CMake build: PASS
- all C-decomp regression tests: PASS
- whole-archive `libavp_c.a`: PASS
- whole-archive ROM util/bootstrap: PASS
- active-source unfinished marker scan: PASS
- public/restricted-payload audit: PASS

### Clang

- strict Release CMake build: PASS
- regression tests: PASS
- whole-archive `libavp_c.a`: PASS
- public/restricted-payload audit: PASS

The final ZIP must still be created, extracted into a clean directory, hash-verified and rerun through GCC+Clang before any final-release claim.

---

## 5. Exact next work order

### A. Finish MJP proof first

Use the supplied MJP analysis/reference bundle. Work from the exact `.dis` files and manifests, not names or intuition.

1. Compare every remaining event-only helper body against exact disassembly.
2. Walk every address in `internal_local_targets.json` and account for local continuations.
3. Reverify controller-local state and branch direction in `Do_Title`, `Do_Selec`, `Do_Fame`, `Do_Intro`, `Do_Escap`, `Do_Lose`, `Do_Win`.
4. Keep asset/file identity, pixel copying, Object Processor phrase packing, blitter/GPU/DSP work and VBL hardware synchronization as typed backend/resource seams **only where the 68000 contains no additional gameplay/menu decision logic**.
5. Add focused tests for any newly lifted decision branch.

### B. Run final whole-source basic-block sweep

Recompare all surviving ordinary-68000 shipping source modules and reconstructed-source modules. The key lesson from RE #6 is that local labels/fallthrough code can carry major logic without a `::` export.

For each reachable code block, classify it as:

- explicit readable C;
- documented internal continuation/merged helper with source-equivalence proof;
- GPU/DSP/OP/hardware/resource-only boundary;
- dead/debug/non-retail code.

Anything else is a blocker.

### C. Callback/event leak scan

Search for `callback`, `frontend_event`, `backend`, `fallback`, `approx`, `conservative`, `compat`, etc. Do not assume these are wrong; prove each one. CPU gameplay/control behavior must default to translated C, as already fixed for `SafePos`, `AreaDamage`, collision/hitscan and several AMP paths.

### D. Final release gate

Only after semantic closure:

1. update status/translation/alias docs;
2. GCC `-O2 -Wall -Wextra -Werror` / strict Release build;
3. Clang strict Release build;
4. full regression suite;
5. whole-archive game and ROM link;
6. active source marker scan;
7. prohibited/restricted payload audit;
8. remove build products;
9. regenerate internal `SHA256SUMS.txt`;
10. create final ZIP;
11. extract final ZIP from scratch;
12. verify internal hashes;
13. rerun GCC validation from extracted ZIP;
14. rerun Clang validation from extracted ZIP;
15. only then change NOT FINAL wording and create the real final tag/package.

---

## 6. Canonical references / provenance

### Historical private source

`Source Code.zip` is the near-final September 1994 source tree (`Version 0.99s`, Tue Sep 6 23:44:42 1994). Use it privately for source comparison. **Do not copy it into the public C repository.** In particular, restricted Spacetec material under CONTROL must never be redistributed publicly.

### Preservation oracle

Use `AVP_Jaguar_RE_M92_Final_100pct_Closure.zip` from the prior core reference bundle as the ultimate ordinary-68000 instruction/behavior oracle when surviving source and current C disagree.

Canonical retail world ROM oracle from the earlier handoff:

- size: 4,194,304 bytes
- SHA-256: `b31ca5c2415881ce50d0c076d327297547214a6240f0058b0f225a74f7ce440b`
- cartridge base: `$800000`
- bootstrap: ROM offset `0x2000` / `$802000`
- inflated main runtime text: RAM `$008000`

Do not put the ROM into any public bundle.

---

## 7. Public repository rules

Keep the public repo code-only:

- no `.jag`/ROM;
- no extracted retail textures/sprites/audio/levels;
- no private executable/oracle slices;
- no restricted Spacetec source;
- no proprietary Atari/Rebellion historical binaries;
- retain original developer/publisher provenance;
- state AI-assisted reverse engineering/reconstruction;
- do not apply a blanket permissive/open-source license to original-derived game code;
- independently authored tooling/docs may be separately licensed if clearly identified.

The future PC port should require the user's own retail ROM and derive assets locally.

---

## 8. Tool/environment note

At the end of RE #6, `xxd` was unavailable. Two install attempts failed: the first timed out and the second exited with failure. The user explicitly instructed us to **use alternatives instead of getting hung up on installs**.

Use `od`, Python binary reads, existing `.dis` outputs, or `m68kmini.py`. For any future required install failure: tell the user at the failure point and avoid looping indefinitely; for `xxd` specifically, do not retry unless circumstances materially change.

---

## 9. User fidelity requirements to preserve

- Strict 1:1 retail behavior is the baseline.
- Do not “beautify,” modernize, smooth, reinterpret or silently fix historical quirks.
- GPU/DSP are separate processor domains, not missing 68000 C.
- Original simulation baseline is 10 Hz; future smooth presentation must be decoupled from simulation.
- Preserve exact timing, AI, movement, collision, weapon cadence, doors, random logic and saves.
- Preserve licensing/credits/provenance and public/private separation.

---

## 10. Suggested first message in the next chat

> Alien VS Predator (Atari Jaguar) — RE continuation. Read `AVP_JAGUAR_C_RE6_TO_RE7_HANDOFF_2026-08-23.md` first and use `AVP_Jaguar_C_Decompilation_RE6_Advanced_WIP_2026-08-23.zip` as the working tree. This is newer than M2 and contains 45 changed/new source/test/header files beyond it. GCC and Clang strict gates pass, but do NOT call the portable C/H repo 100% final yet. Finish exact MJP source-block/local-target equivalence using the supplied MJP disassembly/manifests, then run the whole ordinary-68000 local-label/fallthrough/callback-seam sweep and the full clean-ZIP release gate. Use `Source Code.zip` and M92 privately as oracles; never put restricted source, retail media or private binary slices in the public repo. `xxd` install already failed twice; use `od`/Python/existing disassembly instead.

---

## 11. Final warning to the next assistant

Do **not** regress to any of these completion tests:

- “every module has a `.c` file”;
- “every exported symbol has a C name”;
- “the library links”;
- “all tests pass”;
- “the preservation oracle is already 100%.”

RE #6 proved repeatedly that all of those can be true while meaningful CPU source blocks are still simplified. The required finish criterion is source-block/control-flow equivalence plus the final clean-package validation.
