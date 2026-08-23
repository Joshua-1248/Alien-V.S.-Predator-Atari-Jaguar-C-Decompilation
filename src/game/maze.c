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
#include "computer.h"
#include "doors.h"
#include "objects.h"
#include <stddef.h>

void *build_screen;
s32 sin_ang,cos_ang;
s32 gpu_sin_ang,gpu_cos_ang;
u32 gpu_xpos,gpu_ypos,gpu_angle;
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
extern void LeaveLevel(void),LoadLevel(void);

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

static void display_wait_tick(void)
{
    VSync();
    /* On Jaguar MazeList is called by the display interrupt while the 68000
     * sleeps in STOP.  Invoke its CPU-visible state portion explicitly here so
     * the portable build has the same fade progression without an interrupt. */
    MazeList();
}

void PlayAvP(void)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    InitGame();
    LoadLevel();
    PostFirst();

load_complete:
    if(levfx_ID){if(o->file_event)o->file_event(o->user,0x554e4c50u,levfx_ID,0,0);levfx_ID=0;}
    if(!destruct_flag && o->file_event)o->file_event(o->user,0x4b414c4du,0,0,0); /* KillAlarm */
    use_cocoon=0;
    ResetMaze();
    if(in_fade){
        in_fade=0;
        while(!faded)display_wait_tick();
        fade_level=0x80;fade_lim=0;fade_rate=-2;
    }

    for(;;){
        if(end_count){
            if(end_count==22 && destruct_flag && !launch_flag)ExplodeFade();
            if((end_count==12 || end_count==11) && destruct_flag && !launch_flag){if(o->play_sfx)o->play_sfx(o->user,0); /* bsxp1 */}
            if(end_count==10 && (!destruct_flag || launch_flag))FadeDown();
            if(--end_count==0){
                if(use_cocoon){UseCocoon();}
                else break;
            }
        }
        NextFrame();
        if(comp_panel){
            if(o->kill_sounds)o->kill_sounds(o->user);
            if(o->play_sfx)o->play_sfx(o->user,0); /* compenga */
            FadeDown();
            while(!faded)display_wait_tick();
            Computer();
            if(o->play_sfx)o->play_sfx(o->user,0); /* compdis */
            if(launch_flag)break;
            fade_level=0x80;fade_lim=0;fade_rate=-2;
        }
        if(new_level){
            int newduct=(new_level>=AVP_FIRST_DUCT&&new_level<=AVP_LAST_DUCT);
            int oldduct=(cur_level>=AVP_FIRST_DUCT&&cur_level<=AVP_LAST_DUCT);
            if(newduct||oldduct){if(o->play_sfx)o->play_sfx(o->user,0);FadeDown();}
            else if(!levfx_ID&&!use_cocoon && o->play_sfx)o->play_sfx(o->user,0); /* ddoors */
            if(cur_level<=AVP_LAST_DUCT && new_level<=AVP_LAST_DUCT && o->set_message)
                o->set_message(o->user,(unsigned)(new_level>=AVP_FIRST_DUCT?4:3));
            LeaveLevel();
            LoadLevel();
            goto load_complete;
        }
    }
    if(o->file_event)o->file_event(o->user,0x414c524du,0,0,0); /* clear alarm_ID */
    if(o->kill_sounds)o->kill_sounds(o->user);
    if(o->kill_ambient)o->kill_ambient(o->user);
}

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
    s32 view_sin,view_cos;

    /* MAZE.S snapshots the current player state for the GPU before the CPU is
     * allowed to advance the next simulation frame.  Keep that separation in
     * hosted builds as well; a renderer may read these gpu_* values directly. */
    gpu_xpos=x_pos;gpu_ypos=y_pos;gpu_angle=centre_angle;
    sin_ang=sin_d0(ang); cos_ang=cos_d0(ang);
    gpu_sin_ang=sin_ang;gpu_cos_ang=cos_ang;

    /* Alien bite is a view-only push.  The source arithmetic-shifts the view
     * cos/sin by alien_bite and adjusts only the GPU snapshot, never x_pos/y_pos. */
    view_sin=sin_ang;view_cos=cos_ang;
    if(alien_bite){gpu_xpos=(u32)((s32)gpu_xpos+(view_cos>>alien_bite));gpu_ypos=(u32)((s32)gpu_ypos-(view_sin>>alien_bite));}

    const AvpRuntimeOps *o=avp_runtime_ops();
    if(o->gpu_draw_screen)o->gpu_draw_screen(o->user);
    UpdatePlayer();
    UpdateAMPs();
    if(player_energy<0)player_energy=0;
    Process_EndGame();
    PreFrame(); SwapScreens(); PostFrame(); DoPause();
}
