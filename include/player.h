#ifndef AVP_PLAYER_H
#define AVP_PLAYER_H
#include "avp_types.h"

enum AvpPlayerType { PT_HUMAN=0, PT_ALIEN=4, PT_PREDATOR=8 };

typedef void (*AvpVoidFn)(void);

extern volatile u16 seed0, seed1;
extern s32 x_vel, y_vel;
extern s16 ang_vel, ang_acc, max_avel, acc_shift, dec_shift;
extern u32 max_speed;
extern u16 player_vel;
extern s16 player_type, player_energy, max_energy, old_energy, pain;
extern s16 player_dead, game_over, key_lock, destruct_flag, launch_flag;
extern s32 score;
extern s16 cheat;
extern u32 pain_cols;
extern u32 new_pos, old_pos;
extern u8 wep_fire, one_fire, plreset_count;
extern u16 fade_c, xtra_c, invisflag;
extern u32 bg_fx;
extern s16 bg_repeat, bg_delay, bg_count, last_bg;

extern AvpVoidFn init_move, reset_move, do_move, x_read;

void StdController(void);
void InitPlayer(void);
void ResetPlayer(void);
void init_random(void);
u16 avp_random(void);
void BGSounds(void);
void UpdatePlayer(void);
void ResetMove(void);
void InitMove(void);
void MovePlayer(void);
void TidyMove(void);
void PlayerWeapons(void);
void MapKeys(void);
void ChangeLevel(void);
void ExtraKeys(void);
void TestKeys(void);
void Pain(void);
struct AvpAmp; struct AvpAmp *TestSpark(void);
extern u8 map_on,allow_lcs,allow_god;
#endif
