#ifndef AVP_MAZESCRN_H
#define AVP_MAZESCRN_H
#include "avp_types.h"
#include "amp.h"

#define AVP_MAX_WEPS 6
#define AVP_APAMM_MAX 16384

typedef enum AvpWepCmdOp {
    AVP_WC_END_ANIM=0,
    AVP_WC_END_FRAME,
    AVP_WC_ZERO_POS,
    AVP_WC_SET_POS,
    AVP_WC_OFFSET_POS,
    AVP_WC_ZERO_VEL,
    AVP_WC_SET_VEL,
    AVP_WC_SET_FVEL,
    AVP_WC_SET_TIME,
    AVP_WC_WAIT_FIRE,
    AVP_WC_FIRE,
    AVP_WC_PROJECTILE,
    AVP_WC_TOGGLE_INVIS,
    AVP_WC_SET_OVER,
    AVP_WC_SHOW_OVER,
    AVP_WC_HIDE_OVER,
    AVP_WC_SHOW_USE1,
    AVP_WC_HIDE_USE1,
    AVP_WC_SHOW_USE,
    AVP_WC_HIDE_USE,
    AVP_WC_SOUND,
    AVP_WC_SOUND_ALT,
    AVP_WC_KILL_SOUND,
    AVP_WC_ENABLE_SWING,
    AVP_WC_DISABLE_SWING,
    AVP_WC_SET_BITE,
    AVP_WC_CLEAR_BITE
} AvpWepCmdOp;

typedef struct AvpWepCmd { u8 op; s16 a,b; } AvpWepCmd;

typedef struct AvpWeaponDef {
    const char *name;
    s16 damage;
    u16 distance;
    u16 width;
    u16 init_or_dec;
    u16 max_or_inc;
    const AvpWepCmd *frame0;
    const AvpWepCmd *move_in;
    const AvpWepCmd *move_out;
    const AvpWepCmd *actions[2];
    u8 action_count;
} AvpWeaponDef;

void InitDblBufs(void);
void RestoreMazeList(void);
void SetMazeList(void);
void ScreenOff(void);
void DoPause(void);
void SwapScreens(void);
void PreFrame(void);
void PostFrame(void);
void InitPXFades(void);
void InitSwing(void);
void SwingPos(void);
void Init1stOvers(void);
void InitWeps(void);
void fill_weps(void);
void InitScrOverlays(void);
void hug_init(void);
void StdHUD(void);
void UpdtScrOverlays(void);
void update_wep(void);
void process_shot(void);
void recharge(void);
void init_fades(void);
void frame0(void);
void new_weapon(void);
void fire_wep(void);
void toggle_invis(void);
void enable_swing(void);
void disable_swing(void);

const AvpWeaponDef *avp_weapon_def(s16 player_type,s16 one_based_weapon);
void avp_mazescrn_set_screen_buffers(void *a,void *b,size_t bytes);
void avp_mazescrn_set_shot_callback(AvpAmp *(*fn)(s16 damage,u32 distance,u16 width));

extern s16 fade_rate,fade_lim,fade_level;
extern s8 faded,in_fade;
extern s16 fire_damage;
extern u32 fire_distance;
extern u16 fire_width;
extern s16 num_weps,active_wep,wep_action;
extern s32 wep_x,wep_y,wep_xvel,wep_yvel;
extern s16 swing_x,swing_y,swing_max;
extern u8 swing_on,wait_state,wep_desel;
extern s16 hugkill,hug_recov,hug_throw,unstick_dir;

#endif
