/* Readable C translation of active 68000 logic from MAZE/MAZE.S.
 * GPU rasterization itself remains a Jaguar/host-backend responsibility. */
#include "maze.h"
#include "avp_runtime.h"
#include "player.h"
#include "amp.h"
#include "levels.h"
#include "hud.h"
#include "eeprom.h"
#include "mazescrn.h"
#include <stddef.h>

void *build_screen;
s32 sin_ang,cos_ang;
void *gmps_at,*clist_at;
u16 maze_width,maze_height;
s16 sprite_rescale=AVP_NORMAL_SCALE,true_width=AVP_NORMAL_WIDTH,centre_offset;
u32 x_pos,y_pos,centre_angle;
s16 alien_bite,end_count;
u32 levfx_ID;

/* MAZESCRN-owned fade state; kept extern to preserve original ownership. */
extern s16 fade_rate,fade_lim,fade_level;
extern s8 faded,in_fade;
extern s16 use_cocoon;
extern void InitDblBufs(void),InitScrOverlays(void),SetMazeList(void),ScreenOff(void);
extern void PreFrame(void),PostFrame(void),SwapScreens(void),DoPause(void);
extern void ResetMap(void),InitComp(void),InitSprites(void),ResetMGPU(void);

static const u16 avp_sintab[513]={
#include "sintab.inc"
};

s32 sin_d0(u16 a)
{
    u16 original=a;
    u16 folded=(u16)(a & 0x7fffu);
    if(folded & 0x4000u) folded=(u16)(0x8000u-folded);
    /* Source: asr #5, asl #1 to byte offset => angle / 32 as word index. */
    unsigned idx=(unsigned)(folded>>5);
    if(idx>512u) idx=512u;
    s32 v=(s32)(avp_sintab[idx]>>1);
    if(original & 0x8000u) v=-v;
    return v;
}
s32 cos_d0(u16 a){return sin_d0((u16)(a+0x4000u));}

void FadeUp(void){faded=-1;in_fade=-1;}
void FadeDown(void){fade_level=0;fade_lim=0x80;faded=0;in_fade=-1;fade_rate=2;}
void ExplodeFade(void){fade_level=0;fade_lim=-0x100;faded=0;in_fade=-1;fade_rate=-1;}

void PostFirst(void)
{
    InitScrOverlays();
    ScreenOff();
    SetMazeList();
    FadeUp();
    savegame=0;
    end_count=0;
}

void ResetMaze(void)
{
    hug_init();
    ResetMap();
    ResetMGPU();
    alien_bite=0;
    const AvpRuntimeOps *o=avp_runtime_ops();
    if(o->ambient)o->ambient(o->user);
}

void InitGame(void)
{
    /* Saved player type occupies the low two bits of the final packed save word. */
    if(savegame){
        const u8 *p=(const u8 *)(uintptr_t)savegame;
        u32 d=((u32)p[16]<<24)|((u32)p[17]<<16)|((u32)p[18]<<8)|p[19];
        player_type=(s16)((d&3u)<<2);
    }
    InitDblBufs();
    InitLevels();
    InitSprites();
    MazeDebug();
    const AvpRuntimeOps *o=avp_runtime_ops();
    if(o->gpu_init_overlays)o->gpu_init_overlays(o->user);
    InitPlayer();
    InitAMPs();
    InitComp();
}

void MazeDebug(void){ /* active retail branch has no required side effects */ }

void Process_EndGame(void)
{
    if(end_count) return;
    if(!key_lock){
        if(player_energy==0 && !cheat){
            player_dead=-1; key_lock=-1; wep_fire=0; end_count=10;
            const AvpRuntimeOps *o=avp_runtime_ops();
            if(o->set_message)o->set_message(o->user,1); /* msg_dead */
            if(o->play_sfx)o->play_sfx(o->user,(unsigned)(player_type==PT_HUMAN?1:player_type==PT_ALIEN?2:3));
            use_cocoon=0;
            if(player_type==PT_ALIEN){
                CheckCocoon();
                if(use_cocoon){
                    player_dead=0; key_lock=0; end_count=40;
                    if(o->set_message)o->set_message(o->user,2); /* msg_usecocoon */
                }
            }
            return;
        }
    }
    if(game_over){
        key_lock=-1; end_count=10;
        if(destruct_flag && !launch_flag) end_count=AVP_EXP_START;
    }
}

void ResetScale(void)
{
    s16 scale=AVP_NORMAL_SCALE,width=AVP_NORMAL_WIDTH;
    if(cur_level>=AVP_FIRST_DUCT && cur_level<=AVP_LAST_DUCT){scale=AVP_DUCT_SCALE;width=AVP_DUCT_WIDTH;}
    sprite_rescale=scale; true_width=width;
    centre_offset=(s16)(((64u*128u)/(u16)scale)-64u);
}

void NextFrame(void)
{
    u16 ang=(u16)(centre_angle>>16);
    sin_ang=sin_d0(ang); cos_ang=cos_d0(ang);
    const AvpRuntimeOps *o=avp_runtime_ops();
    if(o->gpu_draw_screen)o->gpu_draw_screen(o->user);
    UpdatePlayer();
    UpdateAMPs();
    if(player_energy<0)player_energy=0;
    Process_EndGame();
    PreFrame(); SwapScreens(); PostFrame(); DoPause();
}
