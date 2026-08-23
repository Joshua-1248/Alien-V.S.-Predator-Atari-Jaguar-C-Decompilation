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
#include "eeprom.h"
#include "collectables.h"
#include "joypad.h"
#include <string.h>

#define DESTRUCT_TIME 120

s16 counttime,counting;
u32 ticktime;
s8 flash_dir;

u8 show_coords,map_on;
s16 use_cocoon,num_cocoons;
u32 ccn_xsave,ccn_ysave;
AvpCocoonState cocoon_data[AVP_MAX_COCOONS];
static s16 old_meter,scope_frame;
static u32 tracker_nearest;
static s32 pred_meter_sample_a,pred_meter_sample_b;
s16 nrg_x,nrg_y,nrg_barwidth,nrg_width,nrg_height,nrg_nframes,nrg_frame;
u16 nrg_rescale,nrg_size;
s8 hud_bright;
static s16 nrg_ftime,nrg_flash;
static u32 click_track;
AvpTrackerAudioState tracker_audio_state;
s16 pred_meter_left,pred_meter_right;
static u8 old_map;
AvpMapInfo map_info;
#define COCOON_START (-1)
#define COCOON_EMPTY AVP_COCOON_EMPTY
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


static u32 hud_be32(const u8 *p)
{
    return ((u32)p[0]<<24)|((u32)p[1]<<16)|((u32)p[2]<<8)|(u32)p[3];
}

static u32 ror32(u32 v,unsigned n)
{
    n&=31u;
    return n ? (v>>n)|(v<<(32u-n)) : v;
}

void extract_cocoon(u32 packed,AvpCocoonState *out)
{
    u32 v;
    u16 low,rot;
    s16 f;
    if(!out)return;

    /* HUD.S extract_cocoon operates on d2.  The apparently odd masks and
     * MOVE.W #$8000 preserve the cell coordinate in the high word and centre
     * the cocoon within that cell in the low word. */
    v=(packed>>10)&0x003f0000u;
    out->x=(v&0xffff0000u)|0x00008000u;
    v=(packed>>4)&0x003f0000u;
    out->y=(v&0xffff0000u)|0x00008000u;
    out->level=(s16)((packed>>16)&0x0fu);

    low=(u16)packed;
    rot=(u16)((u16)(low<<5)|(u16)(low>>11)); /* ROL.W #5 */
    f=(s16)(rot&0x1fu);
    if(f==(s16)(32+COCOON_EMPTY)){
        out->frame=COCOON_EMPTY;
        return;
    }

    ++num_cocoons;
    if(f==(s16)(32+COCOON_START)){
        out->frame=COCOON_START;
        out->time=1;
    }else{
        out->frame=f;
        out->time=-1; /* force redraw without advancing the restored frame */
    }
}

void InitCocoons(void)
{
    num_cocoons=0;
    for(unsigned i=0;i<AVP_MAX_COCOONS;i++){
        cocoon_data[i].frame=COCOON_EMPTY;
        cocoon_data[i].time=0;
        cocoon_data[i].level=0;
        cocoon_data[i].x=cocoon_data[i].y=0;
    }

    if(player_type!=PT_ALIEN)return;
    ccn_xsave=0;
    ccn_ysave=0;

    if(savegame){
        const u8 *p=(const u8 *)(uintptr_t)savegame+8;
        u32 d0=hud_be32(p);
        u32 d2=d0;
        extract_cocoon(d2,&cocoon_data[0]);

        d0&=0x000007ffu;
        d2=ror32(d0,11);
        d0=hud_be32(p+4);
        d2|=d0>>11;
        extract_cocoon(d2,&cocoon_data[1]);

        d2=d0<<10;
        extract_cocoon(d2,&cocoon_data[2]);
    }
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
    /* Retail only performs the compare here; the caller owns use_cocoon. */
    return cocoon_data[AVP_MAX_COCOONS-1].frame==COCOON_READY;
}
static void draw_cocoon_index(unsigned i)
{
    AvpCocoonState *c;
    if(i>=AVP_MAX_COCOONS)return;
    c=&cocoon_data[i];
    {const AvpRuntimeOps *o=avp_runtime_ops();
     if(o->object_event)o->object_event(o->user,0x206u,i,(u32)(u16)c->frame,(u32)(u16)c->time);}
}
void UseCocoon(void)
{
    AvpCocoonState *c=&cocoon_data[AVP_MAX_COCOONS-1];
    u32 dx,dy;
    if(c->frame!=COCOON_READY)return;

    dx=(c->x&0xffff0000u)|0x00008000u;
    dy=(c->y&0xffff0000u)|0x00008000u;
    new_x=(s32)dx;new_y=(s32)dy;new_level=c->level;
    ccn_xsave=dx;ccn_ysave=dy;
    player_energy=max_energy;

    /* HUD.S copies the first two complete cocoon records toward the oldest
     * slot and then resets only cocoon_frame in the first slot. */
    cocoon_data[2]=cocoon_data[1];
    cocoon_data[1]=cocoon_data[0];
    cocoon_data[0].frame=COCOON_EMPTY;
    if(num_cocoons>0)--num_cocoons;
    RedrawCocoons();
    {const AvpRuntimeOps *o=avp_runtime_ops();if(o->play_sfx)o->play_sfx(o->user,70);}
}
void UpdateCocoons(void)
{
    for(unsigned i=0;i<AVP_MAX_COCOONS;i++){
        AvpCocoonState *c=&cocoon_data[i];
        if(c->frame==COCOON_EMPTY)continue;
        if(c->time<0){
            /* Restored ready/in-progress cocoon: force redraw of the current
             * frame, but do not advance it. */
            c->time=COCOON_TIME;
            draw_cocoon_index(i);
            continue;
        }
        if(c->frame==COCOON_READY)continue;
        --c->time;
        if(c->time!=0)continue;
        ++c->frame;
        c->time=COCOON_TIME;
        draw_cocoon_index(i);
    }
}
void RedrawCocoons(void)
{
    for(unsigned i=0;i<AVP_MAX_COCOONS;i++)draw_cocoon_index(i);
}

void ResetMap(void)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    if(o->reset_map)o->reset_map(o->user);
}
void InitHUD(void)
{
    /* Retail order/state from HUD.S.  Brush copies are resource work and stay
     * in the renderer; all CPU-visible state is initialized here. */
    show_coords=0;
    InitHUDMsg();
    InitNrg();
    InitScores();
    InitMap();
    old_meter=0;
    show_mt=0; /* START_MT=0 in the shipping build */
    InitCocoons();
    invisflag=0;
    counting=0;
}
void ResetHUD(void)
{
    /* ResetHUD does not reset the automap.  It resets tracker state and draws
     * the current-level digits into the position brush. */
    click_track=0;
    scope_frame=7;
    { const AvpRuntimeOps *o=avp_runtime_ops(); if(o->object_event) o->object_event(o->user,0x204u,(u32)(u16)cur_level,2u,13u); }
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
    AvpCountdownCue pending=AVP_COUNT_CUE_NONE;
    if (counttime==0) return;
    {
        u8 v=(u8)((u8)(xtra_c>>8)+(u8)flash_dir);
        if (v>0xa0u || v<=0x60u) flash_dir=(s8)-flash_dir;
        xtra_c=(u16)((xtra_c&0x00ffu)|((u16)v<<8));
    }

    /* HUD.S keeps the selected SFX address in d0 across catch-up ticks and
     * starts it once after the loop.  If several thresholds are crossed in one
     * update, the last (lowest-time) cue wins. */
    while (counttime!=0 && (s32)(frames_now-ticktime)>=0) {
        AvpCountdownCue cue;
        ticktime+=one_sec;
        --counttime;
        cue=cue_for_time(counttime);
        if (cue!=AVP_COUNT_CUE_NONE) pending=cue;
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
    if(pending!=AVP_COUNT_CUE_NONE && cue_fn) cue_fn(pending);
}


/* Remaining active HUD.S entry points.  Final retail uses GPU_AUTOMAP=1, so
 * map drawing itself is a GPU overlay; these routines preserve CPU-side state
 * and expose the specialized rendering step through the runtime boundary. */
enum { HUD_EV_TRACKER=0x200,HUD_EV_SCOPE,HUD_EV_ENERGY,HUD_EV_MAP,HUD_EV_POS,HUD_EV_PALETTE,HUD_EV_COCOON };
static void hud_event(unsigned e,u32 a,u32 b,u32 c){const AvpRuntimeOps*o=avp_runtime_ops();if(o->object_event)o->object_event(o->user,e,a,b,c);}
void avp_hud_set_tracker_distance(u32 nearest){tracker_nearest=nearest;}
void avp_hud_set_pred_meter_samples(s32 a,s32 b){pred_meter_sample_a=a;pred_meter_sample_b=b;}

void TracTest(void){hud_event(HUD_EV_TRACKER,(u32)player_type,(u32)(u16)scope_frame,0);}
void TC(void)
{
    enum { MT_MAX=15<<16, CTRACK=4<<16 };
    u32 target;
    unsigned trigger=0;
    if(scope_frame!=0)return; /* SCOPE_CLICK=0 */
    target=(u32)MT_MAX-tracker_nearest;
    if(click_track<target) click_track=target;
    else if(click_track>target){
        u32 next=click_track-(u32)CTRACK;
        click_track=(next>=target)?next:target;
    }
    if(click_track){
        u32 d=click_track;
        tracker_audio_state.volume=(u32)((((u64)d<<8)>>16)+0x200u);
        tracker_audio_state.pitch=(d>>2)+0x000a7000u;
        tracker_audio_state.env_rate=(u32)((d>>16)+0x2fu);
        tracker_audio_state.mod_depth=(u32)(((((u32)MT_MAX-d)<<4)>>16)+0x51u);
        trigger|=1u; /* trac1 */
    }
    trigger|=2u;     /* trac2 is always emitted at click phase */
    hud_event(HUD_EV_SCOPE,click_track,(u32)(u16)scope_frame,trigger);
}
void HP2(void)
{
    enum { METER_W=43 };
    u32 sum=(u32)pred_meter_sample_a+(u32)pred_meter_sample_b;
    s16 target=(s16)((sum<<6)>>16);
    s16 delta=(s16)(target-old_meter);
    s16 smooth=(s16)(old_meter+(s16)(delta>>1));
    const s16 lim=(s16)(METER_W/2);
    if(smooth>lim)smooth=lim; else if(smooth<-lim)smooth=-lim;
    if(target>lim)target=lim; else if(target<-lim)target=-lim;
    pred_meter_left=smooth;
    pred_meter_right=target;
    old_meter=target;
    hud_event(HUD_EV_SCOPE,(u32)(u16)smooth,(u32)(u16)target,0x100u);
    PREDScore();
}
void HUD_human(void)
{
    UpdtNrg();
    if(show_mt){
        s16 next=(s16)(scope_frame+1);
        if(next>7)next=0; /* SCOPE_ANIM=7 */
        scope_frame=next;
        /* GPU motion tracker writes the nearest-distance result consumed by TC. */
        hud_event(HUD_EV_TRACKER,(u32)(u16)((scope_frame>7)?7:scope_frame),click_track,0);
        TC();
    }
    HUMScores();
    if(destruct_flag){
        if(!counting){counting=-1;InitCountdown();}
        Countdown();
    }
}
void InitNrg(void){
    static const s16 cfg[3][7]={{250,8,44,48,13,1,0},{246,11,44,48,13,14,0},{250,10,44,48,14,9,0}};
    unsigned i=(unsigned)player_type>>2;if(i>2)i=0;
    nrg_x=cfg[i][0];nrg_y=cfg[i][1];nrg_barwidth=cfg[i][2];nrg_width=cfg[i][3];nrg_height=cfg[i][4];nrg_nframes=cfg[i][5];
    nrg_rescale=max_energy?(u16)((((u32)(u16)nrg_barwidth<<12)/(u16)max_energy)-1u):0;
    nrg_size=(u16)((u16)nrg_width*(u16)nrg_height);nrg_frame=0;nrg_ftime=4;nrg_flash=0;
}
void UpdtNrg(void)
{
    s16 frame=nrg_frame;
    s16 energy=player_energy, maxe=max_energy;
    u16 pixels=0;
    if(nrg_nframes>0){s16 next=(s16)(frame+1);if(next>=nrg_nframes)next=0;nrg_frame=next;}
    if(--nrg_ftime==0){s16 t=(s16)(2+nrg_flash);nrg_ftime=t;nrg_flash^=2;}
    if(energy>0 && maxe>0){
        s16 threshold=(s16)((maxe>>2)+(maxe>>3));
        if(!(energy<=threshold && nrg_flash)){
            if(energy>maxe)energy=maxe;
            pixels=(u16)((((u32)(u16)energy*(u32)nrg_rescale)<<4)>>16);
            ++pixels;
        }
    }
    /* Source frame is the pre-increment frame; destination width is CPU-computed. */
    hud_event(HUD_EV_ENERGY,(u32)(u16)pixels,(u32)(u16)frame,(u32)nrg_rescale);
}
void DrawCocoon(void){RedrawCocoons();}
void ShowPos(void){hud_event(HUD_EV_POS,(u32)(x_pos>>16),(u32)(y_pos>>16),(u32)(u16)centre_angle);}
void xDecPrint(void){hud_event(HUD_EV_POS,0,0,0);} void DecPrint(void){xDecPrint();} void DecCommon(void){xDecPrint();} void HexPrint(void){xDecPrint();}
void ZeroHUDBright(void)
{
    /* Retail clears hud_usepal but deliberately leaves hud_bright untouched. */
    hud_event(HUD_EV_PALETTE,(u32)(u8)hud_bright,2u,0);
}
void SetHUDBright(void)
{
    /* Per-entry CRY palette RMW is presentation/resource work; hud_bright is
     * the CPU-owned input and the backend applies the retail saturating RMW. */
    hud_event(HUD_EV_PALETTE,(u32)(u8)hud_bright,0,0);
}
void SetHUDPal(void){hud_event(HUD_EV_PALETTE,(u32)(u8)hud_bright,1,0);}
void InitHUDPal(void){hud_bright=64;SetHUDBright();SetHUDPal();}
void HUDBright(void)
{
    s8 old=hud_bright;
    s8 v=old;
    if((joy_cur&(1u<<JOY_RIGHT))==0u){
        int n=(int)v+8;
        if(n>127)n=127;
        v=(s8)n;
    }
    if((joy_cur&(1u<<JOY_LEFT))==0u){
        int n=(int)v-8;
        if(n<-96)n=-96;
        v=(s8)n;
    }
    if(v==old)return;
    hud_bright=v;
    if(v==-96)ZeroHUDBright();
    else SetHUDBright();
}
void InitMap(void)
{
    static const AvpMapInfo shapes[3]={{114,48,120,128},{102,52,120,120},{102,36,128,118}};
    unsigned i=(unsigned)player_type>>2;
    if(i>2u)i=0u;
    old_map=0;
    map_on=0; /* retail START_MAP=0 */
    map_info=shapes[i];
}
void UpdtMap(void){/* GPU_AUTOMAP=1 in the shipping build: no CPU map raster update. */}
void ShowMap(void){u8 falling=(u8)(old_map & (u8)~map_on);old_map=map_on;hud_event(HUD_EV_MAP,map_on,falling,(u32)(u16)centre_angle);}
void wmasks(void){/* Data-symbol compatibility entry: wall masks are GPU/backend-owned in GPU_AUTOMAP retail. */}
void dmasks(void){/* Data-symbol compatibility entry: door masks are GPU/backend-owned in GPU_AUTOMAP retail. */}
void UpdtHUD(void)
{
    if(score<0)score=0;
    if(player_type==PT_HUMAN) HUD_human();
    else if(player_type==PT_ALIEN){UpdtNrg();UpdateCocoons();ALScore();}
    else {UpdtNrg();HP2();}
    ShowHUDMsg();
    ShowMap();
    if(show_coords)ShowPos();
}
