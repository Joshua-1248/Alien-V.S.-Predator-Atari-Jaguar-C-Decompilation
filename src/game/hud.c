/* Source-guided C translation of the destruction-countdown core from HUD.S.
 * Rendering of the rest of the HUD remains separate; this routine preserves
 * the retail 120-second timer, two-second initial delay, flash oscillator,
 * voice cue thresholds and game-over transition.
 */
#include "hud.h"
#include "hud_message.h"
#include "player.h"
#include "hud_score.h"
#include "avp_runtime.h"
#include "levels.h"
#include <string.h>

#define DESTRUCT_TIME 120

s16 counttime,counting;
u32 ticktime;
s8 flash_dir;

u8 show_coords,map_on;
s16 use_cocoon,num_cocoons;
AvpCocoonState cocoon_data[AVP_MAX_COCOONS];
static s16 old_meter,scope_frame;
s16 nrg_x,nrg_y,nrg_barwidth,nrg_width,nrg_height,nrg_nframes,nrg_frame;
u16 nrg_rescale,nrg_size;
s8 hud_bright;
static s16 nrg_ftime,nrg_flash;
static u32 click_track;
static u8 old_map;
#define COCOON_START (-1)
#define COCOON_EMPTY (-2)
#define COCOON_READY 17
#define COCOON_TIME 70

extern u16 xtra_c;
extern s16 game_over;
extern u32 x_pos,y_pos,centre_angle;

static u32 frames_now,one_sec=60;
static AvpCountdownCueFn cue_fn;
static AvpAlarmFn alarm_fn;

void avp_hud_set_clock(u32 frame_counter,u32 frames_per_second)
{
    frames_now=frame_counter;
    one_sec=frames_per_second ? frames_per_second : 1u;
}
void avp_hud_set_countdown_callbacks(AvpCountdownCueFn cue,AvpAlarmFn alarm)
{
    cue_fn=cue; alarm_fn=alarm;
}


void InitCocoons(void)
{
    num_cocoons=0;use_cocoon=0;
    for(unsigned i=0;i<AVP_MAX_COCOONS;i++){cocoon_data[i].frame=COCOON_EMPTY;cocoon_data[i].time=0;cocoon_data[i].level=0;cocoon_data[i].x=cocoon_data[i].y=0;}
    /* Packed save-game cocoon extraction stays at the save/resource boundary;
     * when a host supplies restored states it can write this public array before
     * ResetHUD.  The original format is documented in docs/savegame. */
}
void MakeCocoon(u32 x,u32 y)
{
    if(num_cocoons>=AVP_MAX_COCOONS)return;
    /* New cocoons are inserted from the oldest end, exactly matching the
     * descending-address allocation in HUD.S. */
    unsigned i=(unsigned)(AVP_MAX_COCOONS-1-num_cocoons++);
    cocoon_data[i].frame=COCOON_START;cocoon_data[i].time=1;cocoon_data[i].level=cur_level;cocoon_data[i].x=x;cocoon_data[i].y=y;
}
int CheckCocoon(void)
{
    int ready=(cocoon_data[AVP_MAX_COCOONS-1].frame==COCOON_READY);
    use_cocoon=(s16)(ready?-1:0);return ready;
}
void UseCocoon(void)
{
    AvpCocoonState *c=&cocoon_data[AVP_MAX_COCOONS-1];
    if(c->frame!=COCOON_READY)return;
    new_x=(s32)((c->x&0xffff0000u)|0x8000u);new_y=(s32)((c->y&0xffff0000u)|0x8000u);new_level=c->level;player_energy=max_energy;
    cocoon_data[2]=cocoon_data[1];cocoon_data[1]=cocoon_data[0];cocoon_data[0].frame=COCOON_EMPTY;cocoon_data[0].time=0;
    if(num_cocoons>0)--num_cocoons;
    use_cocoon=0;
    RedrawCocoons();
    {const AvpRuntimeOps *o=avp_runtime_ops();if(o->play_sfx)o->play_sfx(o->user,70);}
}
void UpdateCocoons(void)
{
    for(unsigned i=0;i<AVP_MAX_COCOONS;i++){
        AvpCocoonState *c=&cocoon_data[i];if(c->frame==COCOON_EMPTY||c->frame==COCOON_READY)continue;
        if(c->time<0||--c->time==0){++c->frame;c->time=COCOON_TIME;}
    }
}
void RedrawCocoons(void){ /* renderer owns the 32x32 HUD images */ }

void ResetMap(void)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    if(o->reset_map)o->reset_map(o->user);
}
void InitHUD(void)
{
    show_coords=0;InitHUDMsg();InitScores();old_meter=0;scope_frame=7;click_track=0;InitMap();InitNrg();InitCocoons();invisflag=0;counting=0;
}
void ResetHUD(void)
{
    click_track=0;scope_frame=7;ResetMap();
}

void InitCountdown(void)
{
    counttime=DESTRUCT_TIME+1;
    ticktime=frames_now+one_sec+one_sec;
    xtra_c=0xf180u;
    flash_dir=20;
    if (alarm_fn) alarm_fn(1);
}

static AvpCountdownCue cue_for_time(s16 t)
{
    switch(t) {
    case 120:return AVP_COUNT_CUE_2MIN;
    case 60:return AVP_COUNT_CUE_1MIN;
    case 10:return AVP_COUNT_CUE_10;
    case 9:return AVP_COUNT_CUE_9; case 8:return AVP_COUNT_CUE_8;
    case 7:return AVP_COUNT_CUE_7; case 6:return AVP_COUNT_CUE_6;
    case 5:return AVP_COUNT_CUE_5; case 4:return AVP_COUNT_CUE_4;
    case 3:return AVP_COUNT_CUE_3; case 2:return AVP_COUNT_CUE_2;
    case 1:return AVP_COUNT_CUE_1;
    default:return AVP_COUNT_CUE_NONE;
    }
}

static void update_count_text(void)
{
    unsigned t=(counttime<0)?0u:(unsigned)counttime;
    unsigned mins=t/60u, rem=t%60u;
    avp_countdown_text[5]=(char)('0'+mins);
    avp_countdown_text[7]=(char)('0'+rem/10u);
    avp_countdown_text[8]=(char)('0'+rem%10u);
}

void Countdown(void)
{
    if (counttime==0) return;
    {
        int v=(int)((xtra_c>>8)&0xffu)+(int)flash_dir;
        if (v>0xa0 || v<=0x60) flash_dir=(s8)-flash_dir;
        xtra_c=(u16)((xtra_c&0x00ffu)|((u16)(u8)v<<8));
    }

    while (counttime!=0 && (s32)(frames_now-ticktime)>=0) {
        AvpCountdownCue cue;
        ticktime+=one_sec;
        --counttime;
        cue=cue_for_time(counttime);
        if (cue!=AVP_COUNT_CUE_NONE && cue_fn) cue_fn(cue);
        if (counttime==0) {
            game_over=-1;
            xtra_c=0x8880u;
            if (alarm_fn) alarm_fn(0);
        }
        if (msg_status==AVP_HUDMSG_STOP || avp_hudmsg_current()==avp_msg_countdown) {
            update_count_text();
            avp_hudmsg_queue(avp_msg_countdown);
        }
    }
}


/* Remaining active HUD.S entry points.  Final retail uses GPU_AUTOMAP=1, so
 * map drawing itself is a GPU overlay; these routines preserve CPU-side state
 * and expose the specialized rendering step through the runtime boundary. */
enum { HUD_EV_TRACKER=0x200,HUD_EV_SCOPE,HUD_EV_ENERGY,HUD_EV_MAP,HUD_EV_POS,HUD_EV_PALETTE,HUD_EV_COCOON };
static void hud_event(unsigned e,u32 a,u32 b,u32 c){const AvpRuntimeOps*o=avp_runtime_ops();if(o->object_event)o->object_event(o->user,e,a,b,c);}
void TracTest(void){hud_event(HUD_EV_TRACKER,(u32)player_type,(u32)scope_frame,0);}
void TC(void){hud_event(HUD_EV_SCOPE,(u32)scope_frame,0,0);}
void HP2(void){UpdtNrg();}
void HUD_human(void){TracTest();HUMScores();UpdtNrg();}
void InitNrg(void){
    static const s16 cfg[3][7]={{250,8,44,48,13,1,0},{246,11,44,48,13,14,0},{250,10,44,48,14,9,0}};
    unsigned i=(unsigned)player_type>>2;if(i>2)i=0;
    nrg_x=cfg[i][0];nrg_y=cfg[i][1];nrg_barwidth=cfg[i][2];nrg_width=cfg[i][3];nrg_height=cfg[i][4];nrg_nframes=cfg[i][5];
    nrg_rescale=max_energy?(u16)((((u32)(u16)nrg_barwidth<<12)/(u16)max_energy)-1u):0;
    nrg_size=(u16)((u16)nrg_width*(u16)nrg_height);nrg_frame=0;nrg_ftime=4;nrg_flash=0;
}
void UpdtNrg(void){if(nrg_nframes>0){++nrg_frame;if(nrg_frame>=nrg_nframes)nrg_frame=0;}if(--nrg_ftime<0){nrg_ftime=4;nrg_flash^=-1;}hud_event(HUD_EV_ENERGY,(u32)(u16)player_energy,(u32)(u16)nrg_frame,(u32)nrg_rescale);}
void extract_cocoon(void){/* Save-format extraction belongs at the user-ROM/save boundary; InitCocoons owns decoded state. */}
void DrawCocoon(void){hud_event(HUD_EV_COCOON,(u32)num_cocoons,0,0);}
void ShowPos(void){hud_event(HUD_EV_POS,(u32)(x_pos>>16),(u32)(y_pos>>16),(u32)(u16)centre_angle);}
void xDecPrint(void){hud_event(HUD_EV_POS,0,0,0);} void DecPrint(void){xDecPrint();} void DecCommon(void){xDecPrint();} void HexPrint(void){xDecPrint();}
void ZeroHUDBright(void){hud_bright=-128;hud_event(HUD_EV_PALETTE,(u32)(u8)hud_bright,0,0);}
void SetHUDBright(void){hud_event(HUD_EV_PALETTE,(u32)(u8)hud_bright,0,0);}
void SetHUDPal(void){hud_event(HUD_EV_PALETTE,(u32)(u8)hud_bright,1,0);}
void InitHUDPal(void){hud_bright=64;SetHUDBright();SetHUDPal();}
void HUDBright(void){SetHUDBright();}
void InitMap(void){old_map=0;map_on=0;}
void UpdtMap(void){/* GPU_AUTOMAP=1 in the shipping build: no CPU map raster update. */}
void ShowMap(void){u8 falling=(u8)(old_map & (u8)~map_on);old_map=map_on;hud_event(HUD_EV_MAP,map_on,falling,(u32)(u16)centre_angle);}
void wmasks(void){/* Data-symbol compatibility entry: wall masks are GPU/backend-owned in GPU_AUTOMAP retail. */}
void dmasks(void){/* Data-symbol compatibility entry: door masks are GPU/backend-owned in GPU_AUTOMAP retail. */}
void UpdtHUD(void){if(score<0)score=0;if(player_type==PT_HUMAN)HUD_human();else if(player_type==PT_ALIEN){ALScore();UpdtNrg();}else{PREDScore();UpdtNrg();}ShowHUDMsg();ShowMap();if(show_coords)ShowPos();}
