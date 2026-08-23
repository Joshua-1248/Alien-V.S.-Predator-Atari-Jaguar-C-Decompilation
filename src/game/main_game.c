/* Source-shaped ordinary-68000 control flow from retail MAIN/MAIN.S.
 *
 * Jaguar stack/MMIO layout checks and GPU/DSP stop/start operations are host
 * services.  The shipping title -> select/load -> PlayAvP -> ending -> Hall of
 * Fame sequencing is kept explicitly in C. */
#include "main_game.h"
#include "player.h"
#include "maze.h"
#include "mjp.h"
#include "files.h"
#include "music.h"
#include "objects.h"
#include "jaguar.h"
#include "eeprom.h"
#include "joypad.h"
#include "avp_runtime.h"
#include <stdint.h>

u8 in_title,in_select,allow_reset;
static u8 stopped;

static void fevent(unsigned e,s32 a,s32 b){const AvpRuntimeOps*o=avp_runtime_ops();if(o->frontend_event)o->frontend_event(o->user,e,a,b);}

void ClearStack(void)
{
    /* MAIN.S clears the unused fixed Jaguar BSS stack region.  A hosted C ABI
     * owns its call stack; there is no corresponding game-state side effect. */
}
void SetupGpu(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->gpu_load_base)o->gpu_load_base(o->user);}
void SetFixGpu(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->gpu_fix_overlay)o->gpu_fix_overlay(o->user,0);}
void SetGpuBin(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->gpu_load_overlay)o->gpu_load_overlay(o->user,0);}
void stop(void){stopped=1;KillAll();}
static void back_in_runtime(void);
void re_start(void){KillAll();stopped=0;back_in_runtime();}
void reset(void){KillAll();main_start();}
void start(void){main_start();}

void Version(void)
{
    fevent(0x56455253u,0,0);
    /* Text/object rendering and the wait-for-pad presentation are front-end
     * resources; Version has no persistent game-state mutation. */
}

void Title(void)
{
    in_title=0xffu;
    fevent(0x5449544cu,0,0);
}

void NewTitle(void)
{
    in_title=0xffu;
restart_title:
    InitMJP();
    SetScreen(320,240);
    allow_reset=0;

    /* Holding OPTION on entry opens Hall of Fame and can request EEPROM clear. */
    readpad();
    if((joy_cur&(1u<<OPTION))==0u){
        player_type=-1;Choice=0;grunke=0;
        MTEST(AVP_MJP_FAME);
        /* MJP Hall-of-Fame returns its special EEPROM-clear request through
         * player_type in the retail MAIN.S contract, not through Choice. */
        {
            s16 fame_result=player_type;
            player_type=PT_ALIEN;
            if(fame_result==-99)Trash_EE();
        }
    }
    allow_reset=0xffu;

    /* Retail starts title music then waits 141 VBLs before entering the MJP
     * main menu.  Asset identity is backend/resource-owned. */
    fevent(0x544d5553u,1,0);
    for(unsigned i=0;i<141u;i++)VSync();

main_menu:
    /* 50 Hz video uses the authored 320x280 menu framing; 60 Hz remains at
     * 320x240.  InitVideo's one_second_ticks is the portable video-standard
     * state corresponding to the CONFIG bit tested by MAIN.S. */
    if(one_second_ticks==50u)SetScreen(320,280);
    Choice=0;
    MTEST(AVP_MJP_TITLE);
    SetScreen(320,240);
    if(Choice<0){
        s32 slot=-Choice-1;
        if(slot<0||slot>2)slot=0;

        /* MAIN.S performs the title fade/music shutdown before resolving the
         * save slot pointer.  Keep that CPU-side ordering explicit. */
        WaitFXFa();
        StopMusi();
        fevent(0x5545564fu,0x7fff,0); /* UEBERVOLUME restore */

        u8 *p=cartcopy+(size_t)slot*AVP_SAVE_SIZE;
        u32 valid=((u32)p[0]<<24)|((u32)p[1]<<16)|((u32)p[2]<<8)|p[3];
        if(!valid)goto restart_title;
        savegame=(uintptr_t)p;
        VSync();
        fevent(0x44495350u,0,0); /* clear OLP/display_fn */
        InitFile();
        return;
    }
    if(Choice==0){
        Choice=0;MTEST(AVP_MJP_SELECT);
        s32 p=Choice&0x0c;
        if(p==0x0c)p=0;
        player_type=(s16)p;
        in_select=0xffu;
        VSync();
        InitFile();
        return;
    }
    if(Choice==2){
        /* MAIN.S clears score before the title-menu Hall of Fame so a stale
         * gameplay score cannot be considered for insertion. */
        score=0;
        grunke=0;Choice=0;MTEST(AVP_MJP_FAME);
        goto main_menu;
    }
    if(Choice==4){MTEST(AVP_MJP_INTRO);goto main_menu;}
    goto main_menu;
}

static void post_game_frontend(void)
{
    /* MAIN.S ProtectVoices(7), then exact InitMJP video/front-end setup. */
    ProtectV(7);
    InitMJP();
    SetScreen(320,240);

    if(player_dead){
        MTEST(AVP_MJP_SIM_TERMINATED);
        grunke=2;
    }else{
        /* The retail source starts pred_music for the ending block regardless
         * of species before dispatching the species-specific sequence. */
        fevent(0x454d5553u,1,0);
        if(player_type==PT_HUMAN){
            if(launch_flag)MTEST(AVP_MJP_ESCAPE);
            else MTEST(AVP_MJP_BASE_EXPLODES);
        }
        if(player_type==PT_ALIEN)MTEST(AVP_MJP_ALIEN_WIN);
        if(player_type==PT_PREDATOR)MTEST(AVP_MJP_PRED_WIN);
        grunke=1;
    }
    MTEST(AVP_MJP_FAME);
    WaitFXFa();
    StopMusi();
    fevent(0x5545564fu,0x7fff,0); /* UEBERVOLUME restore */
}

static void back_in_runtime(void)
{
    /* MAIN.S::back_in is the common restart body.  re_start branches here
     * specifically so it does not execute main_start's species reset. */
    InitJaguar();
    ClearStack();
    Init_EE();
    StdController();
    initpad(); /* MAIN.S::InitController portable counterpart */
    InitFile();
    InitVideo();
    InitSynt();
    fevent(0x434c5554u,0,0); /* CLUT[0]=0 */

    while(!stopped){
        in_title=0;in_select=0;savegame=0;
        NewTitle();
        PlayAvP();
        post_game_frontend();
    }
}

void main_start(void)
{
    stopped=0;
    /* Retail NO_DEBUG initial species. */
    player_type=PT_ALIEN;
    back_in_runtime();
}
