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
#define AVP_COCOON_EMPTY (-2)
typedef struct AvpCocoonState { s16 frame,time,level; u32 x,y; } AvpCocoonState;
void InitCocoons(void);
void MakeCocoon(u32 x,u32 y);
int CheckCocoon(void);
void UseCocoon(void);
void UpdateCocoons(void);
void RedrawCocoons(void);
void ResetMap(void);
typedef struct AvpMapInfo { s16 x,y,w,h; } AvpMapInfo;
extern AvpMapInfo map_info;
extern u8 show_coords,map_on;
extern s16 use_cocoon,num_cocoons;
extern u32 ccn_xsave,ccn_ysave;
extern AvpCocoonState cocoon_data[AVP_MAX_COCOONS];

/* Explicit C surfaces for the active HUD.S routines.  Pixel/Blitter/GPU work
 * is routed through AvpRuntimeOps; game-side state remains visible here. */
typedef struct AvpTrackerAudioState { u32 volume,pitch,env_rate,mod_depth; } AvpTrackerAudioState;
extern AvpTrackerAudioState tracker_audio_state;
extern s16 pred_meter_left,pred_meter_right;
void avp_hud_set_tracker_distance(u32 nearest);
void avp_hud_set_pred_meter_samples(s32 a,s32 b);
void TracTest(void); void HUD_human(void); void TC(void); void HP2(void); void UpdtHUD(void);
void InitNrg(void); void UpdtNrg(void); void extract_cocoon(u32 packed,AvpCocoonState *out); void DrawCocoon(void);
void ShowPos(void); void xDecPrint(void); void DecPrint(void); void DecCommon(void); void HexPrint(void);
void InitHUDPal(void); void SetHUDPal(void); void ZeroHUDBright(void); void SetHUDBright(void); void HUDBright(void);
void InitMap(void); void UpdtMap(void); void ShowMap(void); void wmasks(void); void dmasks(void);
extern s16 nrg_x,nrg_y,nrg_barwidth,nrg_width,nrg_height,nrg_nframes,nrg_frame;
extern u16 nrg_rescale,nrg_size; extern s8 hud_bright;

#endif
