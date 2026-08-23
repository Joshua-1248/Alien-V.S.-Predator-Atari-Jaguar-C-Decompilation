/* Readable C orchestration of the ordinary 68000 control flow in MAIN/MAIN.S.
 * Fixed-address DEHUFF storage and Jaguar GPU/DSP shutdown remain platform
 * concerns; game/front-end sequencing is kept here. */
#include "main_game.h"
#include "player.h"
#include "maze.h"
#include "mjp.h"
#include "files.h"
#include "music.h"
#include "objects.h"
#include "jaguar.h"
#include "avp_runtime.h"

u8 in_title,in_select;
static u8 stopped;

void ClearStack(void){ /* host C owns its call stack; historical BSS stack clearing is not applicable. */ }
void SetupGpu(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->gpu_load_base)o->gpu_load_base(o->user);}
void SetFixGpu(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->gpu_fix_overlay)o->gpu_fix_overlay(o->user,0);}
void SetGpuBin(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->gpu_load_overlay)o->gpu_load_overlay(o->user,0);}
void stop(void){stopped=1;KillAll();}
void re_start(void){KillAll();stopped=0;}
void reset(void){KillAll();main_start();}
void start(void){main_start();}
void Version(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->frontend_event)o->frontend_event(o->user,0x56455253u,0,0);}
void Title(void){in_title=0xffu;const AvpRuntimeOps*o=avp_runtime_ops();if(o->frontend_event)o->frontend_event(o->user,0x5449544cu,0,0);}
void NewTitle(void){
    in_title=0xffu;MTEST(AVP_MJP_TITLE);
    const AvpRuntimeOps*o=avp_runtime_ops();
    if(o->frontend_event)o->frontend_event(o->user,0x4d454e55u,0,0);
    InitFile();
}
void main_start(void){
    stopped=0;
    /* NO_DEBUG retail source initially selects Alien before the title/front-end
     * chooses/loads the actual character. */
    player_type=PT_ALIEN;
    ClearStack();
    InitJaguar();InitVideo();InitSynt();InitFile();
    NewTitle();
}
