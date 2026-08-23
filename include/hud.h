#ifndef AVP_HUD_H
#define AVP_HUD_H
#include "avp_types.h"

typedef enum AvpCountdownCue {
    AVP_COUNT_CUE_NONE=0,
    AVP_COUNT_CUE_2MIN,
    AVP_COUNT_CUE_1MIN,
    AVP_COUNT_CUE_10,
    AVP_COUNT_CUE_9,
    AVP_COUNT_CUE_8,
    AVP_COUNT_CUE_7,
    AVP_COUNT_CUE_6,
    AVP_COUNT_CUE_5,
    AVP_COUNT_CUE_4,
    AVP_COUNT_CUE_3,
    AVP_COUNT_CUE_2,
    AVP_COUNT_CUE_1
} AvpCountdownCue;

typedef void (*AvpCountdownCueFn)(AvpCountdownCue cue);
typedef void (*AvpAlarmFn)(int start);

void InitHUD(void);
void ResetHUD(void);
void InitCountdown(void);
void Countdown(void);
void avp_hud_set_clock(u32 frame_counter,u32 frames_per_second);
void avp_hud_set_countdown_callbacks(AvpCountdownCueFn cue,AvpAlarmFn alarm);

extern s16 counttime,counting;
extern u32 ticktime;
extern s8 flash_dir;
#define AVP_MAX_COCOONS 3
typedef struct AvpCocoonState { s16 frame,time,level; u32 x,y; } AvpCocoonState;
void InitCocoons(void);
void MakeCocoon(u32 x,u32 y);
int CheckCocoon(void);
void UseCocoon(void);
void UpdateCocoons(void);
void RedrawCocoons(void);
void ResetMap(void);
extern u8 show_coords,map_on;
extern s16 use_cocoon,num_cocoons;
extern AvpCocoonState cocoon_data[AVP_MAX_COCOONS];


#endif
