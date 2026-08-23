# Routine-level alias/closure audit

This file records the final source-guided audit of exported `::` labels in the surviving September 1994 `MAZE/MAZESCRN.S` and `MAZE/PLAYER.S`. Data labels are not counted as routines.

## MAZESCRN.S

Direct C routine surfaces: `RestoreMazeList`, `SetMazeList`, `MazeList`, `InitDblBufs`, `DoPause`, `pause_off`, `LoseSounds`, `RestoreSounds`, `SwapScreens`, `PreFrame`, `PostFrame`, `InitPXFades`, `InitPredAvail`, `InitSwing`, `Init1stOvers`, `InitWeps`, `fill_weps`, `ExpandOvers`, `InitScrOverlays`, `hug_init`, `StdHUD`, `enable_swing`, `init_fades`.

`MazeList`, `LoseSounds`, `RestoreSounds`, `pause_off`, and `ExpandOvers` retain the ordinary-68000 orchestration under their historical names while Object Processor/DSP/resource-transfer details are delegated to `AvpRuntimeOps`.

Data, not routines: `trigger`, `bnaptr`, `fade_rate`, `fade_lim`, `fade_level`, `faded`, `in_fade`, `ammo_info`, `cur_wepptr`, `cur_wepno`, `cur_weps`, `new_wepno`, `fire_damage`, `fire_distance`, `fire_width`, `num_weps`, `swing_x`, `swap_screens`, `frames`, `resets`, `hud_pic`, `scbase1pos`, `scbase2pos`, `scbasexpos`, `blippos`, `mkillpos`, `allscpos`.

## PLAYER.S

Direct C routine surfaces: `StdController`, `init_random`, `ResetClct`, `Collectables`, `CollectIt`, `TidyMove`, `do_predo`, `Pain`, and `TestSpark`.

Intentional readable-C aliases/merged helpers:

- `random` -> `avp_random`.
- `give_energy` -> `avp_give_energy`.
- `give_wep` -> `avp_give_weapon`.
- `give_ammo` -> `avp_give_ammo`.
- `shoot_loop` -> the AMP-selection loop inside `TestSpark`; it is an internal entry point in assembly, not a separate gameplay operation in C.
- `LMM` -> the deceleration/acceleration/velocity-clamp portion of `MovePlayer`; the assembly label is an internal continuation.
- `CompCollect` -> body/object specialization within `Collectables`.
- `HumanPain` -> the human damage-sound branch folded into `Pain`'s player-type sound selection.
- `test_fade` exists only under the source `TEST_FADE`/debug conditional; the retail `NO_DEBUG` build excludes it.

Function-pointer/data exports, not routines: `x_read`, `init_move`, `reset_move`, `do_move`, `new_pos`, `old_pos`, `ang_vel`, `ang_acc`, `max_avel`, `acc_shift`, `dec_shift`, `x_vel`, `y_vel`, `player_vel`, `player_dead`, `game_over`, `key_lock`, `destruct_flag`, `launch_flag`, `player_energy`, `max_energy`, `max_speed`, `pain_cols`, `score`, `cheat`, `pain`, `old_energy`, `last_moan`, `fade_c`, `xtra_c`, `invisflag`, `wep_fire`, `one_fire`, `invis_act`, `invis_stat`, `medpak_act`, plus authored tables `nrg_maxes`, `collect_info`, `level_bodies`, `cur_bodies`, and `limits`.
