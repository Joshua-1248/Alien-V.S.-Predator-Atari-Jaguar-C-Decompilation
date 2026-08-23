#ifndef AVP_MAZE_H
#define AVP_MAZE_H
#include "avp_types.h"

#define AVP_SCREEN_HEIGHT 228
#define AVP_SCREEN_WIDTH 320
#define AVP_WALL_SIZE 128
#define AVP_NORMAL_SCALE 85
#define AVP_NORMAL_WIDTH 0
#define AVP_DUCT_SCALE 70
#define AVP_DUCT_WIDTH 1
#define AVP_FIRST_DUCT 6
#define AVP_LAST_DUCT 13
#define AVP_EXP_START 60

extern void *build_screen;
extern s32 sin_ang,cos_ang;
extern void *gmps_at,*clist_at;
extern u16 maze_width,maze_height;
extern s16 sprite_rescale,true_width,centre_offset;
extern u32 x_pos,y_pos,centre_angle;
extern s16 alien_bite,end_count;
extern u32 levfx_ID;

void PostFirst(void);
void FadeUp(void);
void FadeDown(void);
void ExplodeFade(void);
void ResetMaze(void);
void InitGame(void);
void MazeDebug(void);
void NextFrame(void);
void Process_EndGame(void);
s32 sin_d0(u16 angle);
s32 cos_d0(u16 angle);
void ResetScale(void);

#endif
