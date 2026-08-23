/* Readable C translation of the first-person/HUD orchestration in
 * MAZE/MAZESCRN.S.  Picture decompression and Object Processor writes are
 * platform services; the original command-stream state machine lives here. */
#include "mazescrn.h"
#include "maze.h"
#include "player.h"
#include "weapons.h"
#include "collectables.h"
#include "hud.h"
#include "hud_message.h"
#include "avp_runtime.h"
#include <stddef.h>
#include <string.h>

s16 fade_rate,fade_lim,fade_level;
s8 faded,in_fade;
s16 fire_damage;
u32 fire_distance;
u16 fire_width;
s16 num_weps,active_wep,wep_action;
s32 wep_x,wep_y,wep_xvel,wep_yvel;
s16 swing_x,swing_y,swing_max;
u8 swing_on,wait_state,wep_desel;
s16 hugkill,hug_recov,hug_throw,unstick_dir;
s16 invis_bright,medpak_bright,inwep_bright,outwep_bright;
s16 inwep_no,outwep_no,remove_wepno;
static u8 mpak_maxed,mpak_time,mpak_state;
static u32 medpak_ID;

static void *screen_a,*screen_b; static size_t screen_bytes;
static int screen_flip;
static s16 wep_low;
static u16 a1st_time=1;
static s16 chase_vel,swing_angle,gunratemul;
static const AvpWeaponDef *cur_def;
static const AvpWepCmd *wep_animptr,*wait_ptr;
static u8 cur_action;
static s16 over_id=-1;
static u8 over_visible,use1_visible,use2_visible;
static AvpAmp *(*shot_cb)(s16,u32,u16);

void avp_mazescrn_set_screen_buffers(void *a,void *b,size_t n){screen_a=a;screen_b=b;screen_bytes=n;build_screen=a;}
void avp_mazescrn_set_shot_callback(AvpAmp *(*fn)(s16,u32,u16)){shot_cb=fn;}

/* Logical overlays.  Actual pixel data is user-ROM-derived and is intentionally
 * not embedded in this code-only repository. */
enum { OV_NONE=-1,OV_GSHOT1,OV_GSHOT2,OV_GSHOT3,OV_GSHOT4,OV_PULSE,OV_FLAMER,OV_SMART,
       OV_TAIL1,OV_TAIL3,OV_BITE1,OV_BITE2,OV_BITE3,OV_SWIPE1,OV_SWIPE2,OV_SWIPE3,
       OV_STICK1,OV_STICK2,OV_STICK3,OV_STICK4,OV_DISK1,OV_DISK2,OV_DISK3,
       OV_PUNCH1,OV_PUNCH2 };
enum { SFX_SHOTGUN=1,SFX_COCK,SFX_PULSE,SFX_FLAME,SFX_SMART,SFX_TAIL,SFX_SWIPE,
       SFX_MELEE,SFX_ROCKET,SFX_DISK,SFX_WRBLADE,SFX_EMPTY };

#define C0(o) {o,0,0}
#define C1(o,a) {o,a,0}
#define C2(o,a,b) {o,a,b}
#define EF C0(AVP_WC_END_FRAME)
#define EA C0(AVP_WC_END_ANIM)

static const AvpWepCmd std_in[]={C2(AVP_WC_OFFSET_POS,0,100),C2(AVP_WC_SET_VEL,0,-10),C1(AVP_WC_SET_TIME,9),EF,EA};
static const AvpWepCmd std_out[]={C2(AVP_WC_SET_VEL,0,10),C1(AVP_WC_SET_TIME,9),EF,EA};
static const AvpWepCmd no_action[]={EA};
static const AvpWepCmd no_frame0[]={C0(AVP_WC_HIDE_OVER),EA};

static const AvpWepCmd shot_f0[]={C1(AVP_WC_SET_OVER,OV_GSHOT1),C0(AVP_WC_HIDE_USE),EA};
static const AvpWepCmd shot_act[]={C0(AVP_WC_FIRE),C1(AVP_WC_SOUND,SFX_SHOTGUN),C0(AVP_WC_SHOW_USE1),C2(AVP_WC_OFFSET_POS,0,24),EF,
 C0(AVP_WC_HIDE_USE1),C2(AVP_WC_SET_VEL,0,-6),C1(AVP_WC_SET_TIME,2),EF,
 C1(AVP_WC_SET_OVER,OV_GSHOT2),EF,C1(AVP_WC_SOUND_ALT,SFX_COCK),C1(AVP_WC_SET_OVER,OV_GSHOT3),EF,
 C0(AVP_WC_ZERO_VEL),C1(AVP_WC_SET_OVER,OV_GSHOT4),C1(AVP_WC_SET_TIME,2),EF,C1(AVP_WC_SET_OVER,OV_GSHOT3),EF,C1(AVP_WC_SET_OVER,OV_GSHOT2),EF,EA};
static const AvpWepCmd pulse_f0[]={C1(AVP_WC_SET_OVER,OV_PULSE),C0(AVP_WC_HIDE_USE),EA};
static const AvpWepCmd pulse_act[]={C1(AVP_WC_SOUND,SFX_PULSE),C0(AVP_WC_SHOW_USE),C2(AVP_WC_OFFSET_POS,0,20),C0(AVP_WC_WAIT_FIRE),C0(AVP_WC_FIRE),EF,
 C0(AVP_WC_KILL_SOUND),C0(AVP_WC_HIDE_USE),C2(AVP_WC_OFFSET_POS,0,-10),EF,EA};
static const AvpWepCmd flame_f0[]={C1(AVP_WC_SET_OVER,OV_FLAMER),EA};
static const AvpWepCmd flame_act[]={C2(AVP_WC_OFFSET_POS,0,10),C1(AVP_WC_SOUND,SFX_FLAME),C0(AVP_WC_WAIT_FIRE),C1(AVP_WC_PROJECTILE,1),C1(AVP_WC_SET_TIME,2),EF,
 C0(AVP_WC_KILL_SOUND),C2(AVP_WC_OFFSET_POS,0,-5),EF,C0(AVP_WC_ZERO_POS),EF,EA};
static const AvpWepCmd smart_f0[]={C1(AVP_WC_SET_OVER,OV_SMART),C0(AVP_WC_HIDE_USE),EA};
static const AvpWepCmd smart_act[]={C1(AVP_WC_SOUND,SFX_SMART),C0(AVP_WC_SHOW_USE),C2(AVP_WC_OFFSET_POS,0,20),C0(AVP_WC_WAIT_FIRE),C0(AVP_WC_FIRE),EF,
 C0(AVP_WC_KILL_SOUND),C2(AVP_WC_OFFSET_POS,0,-10),EF,EA};

static const AvpWepCmd tail_in[]={C2(AVP_WC_OFFSET_POS,60,-60),C2(AVP_WC_SET_VEL,-30,30),C1(AVP_WC_SET_TIME,1),EF,EA};
static const AvpWepCmd tail_out[]={C2(AVP_WC_SET_VEL,30,-30),C1(AVP_WC_SET_TIME,1),EF,EA};
static const AvpWepCmd tail_f0[]={C1(AVP_WC_SET_OVER,OV_TAIL1),C2(AVP_WC_SET_POS,20,-30),EA};
static const AvpWepCmd tail_act[]={C1(AVP_WC_SOUND,SFX_TAIL),C2(AVP_WC_SET_POS,-20,-5),EF,C1(AVP_WC_SET_OVER,OV_TAIL3),C0(AVP_WC_FIRE),C2(AVP_WC_SET_POS,0,20),C1(AVP_WC_SET_TIME,2),EF,
 C1(AVP_WC_SET_OVER,OV_TAIL1),C2(AVP_WC_SET_POS,-20,-5),EF,C2(AVP_WC_SET_POS,20,-30),C1(AVP_WC_SET_TIME,2),EF,EA};
static const AvpWepCmd bite_in[]={C2(AVP_WC_OFFSET_POS,0,90),C2(AVP_WC_SET_VEL,0,-30),C1(AVP_WC_SET_TIME,2),EF,EA};
static const AvpWepCmd bite_out[]={C2(AVP_WC_SET_VEL,0,40),C1(AVP_WC_SET_TIME,2),EF,EA};
static const AvpWepCmd bite_f0[]={C0(AVP_WC_DISABLE_SWING),C1(AVP_WC_SET_OVER,OV_BITE1),EA};
static const AvpWepCmd bite_act[]={C0(AVP_WC_SHOW_OVER),EF,C1(AVP_WC_SOUND,SFX_SWIPE),C1(AVP_WC_SET_OVER,OV_BITE2),C1(AVP_WC_SET_TIME,1),EF,
 C0(AVP_WC_FIRE),C1(AVP_WC_SET_OVER,OV_BITE3),C1(AVP_WC_SET_TIME,2),EF,C1(AVP_WC_SET_OVER,OV_BITE1),C1(AVP_WC_SET_TIME,1),EF,EA};
static const AvpWepCmd swipe_in[]={C2(AVP_WC_OFFSET_POS,0,90),C2(AVP_WC_SET_VEL,0,-30),C1(AVP_WC_SET_TIME,2),EF,EA};
static const AvpWepCmd swipe_out[]={C2(AVP_WC_SET_VEL,0,30),C1(AVP_WC_SET_TIME,2),EF,EA};
static const AvpWepCmd swipe_f0[]={C1(AVP_WC_SET_OVER,OV_SWIPE1),C2(AVP_WC_OFFSET_POS,24,72),EA};
static const AvpWepCmd swipe_act[]={C2(AVP_WC_OFFSET_POS,-24,-72),C1(AVP_WC_SET_TIME,1),EF,C1(AVP_WC_SOUND,SFX_SWIPE),C1(AVP_WC_SET_OVER,OV_SWIPE2),C1(AVP_WC_SET_TIME,1),EF,
 C0(AVP_WC_FIRE),C1(AVP_WC_SET_OVER,OV_SWIPE3),C1(AVP_WC_SET_TIME,3),EF,C1(AVP_WC_SET_OVER,OV_SWIPE2),C2(AVP_WC_OFFSET_POS,6,18),C1(AVP_WC_SET_TIME,1),EF,
 C1(AVP_WC_SET_OVER,OV_SWIPE1),C2(AVP_WC_OFFSET_POS,12,36),C1(AVP_WC_SET_TIME,1),EF,C2(AVP_WC_OFFSET_POS,6,18),C1(AVP_WC_SET_TIME,2),EF,EA};

static const AvpWepCmd stick_f0[]={C1(AVP_WC_SET_OVER,OV_STICK1),EA};
static const AvpWepCmd stick_in[]={C2(AVP_WC_OFFSET_POS,60,60),C2(AVP_WC_SET_VEL,-10,-10),C1(AVP_WC_SET_TIME,5),EF,EA};
static const AvpWepCmd stick_out[]={C2(AVP_WC_SET_VEL,10,10),C1(AVP_WC_SET_TIME,5),EF,EA};
static const AvpWepCmd stick_act[]={C1(AVP_WC_SET_OVER,OV_STICK2),C1(AVP_WC_SET_TIME,1),EF,C1(AVP_WC_SET_OVER,OV_STICK3),C1(AVP_WC_SET_TIME,1),EF,
 C0(AVP_WC_FIRE),C1(AVP_WC_SOUND,SFX_MELEE),C1(AVP_WC_SET_OVER,OV_STICK4),C1(AVP_WC_SET_TIME,5),EF,C1(AVP_WC_SET_OVER,OV_STICK3),C1(AVP_WC_SET_TIME,1),EF,
 C1(AVP_WC_SET_OVER,OV_STICK2),C1(AVP_WC_SET_TIME,1),EF,C1(AVP_WC_SET_OVER,OV_STICK1),C1(AVP_WC_SET_TIME,4),EF,EA};
static const AvpWepCmd laser_act[]={C1(AVP_WC_SOUND,SFX_ROCKET),C1(AVP_WC_PROJECTILE,2),C1(AVP_WC_SET_TIME,10),EF,EA};
static const AvpWepCmd disk_f0[]={C1(AVP_WC_SET_OVER,OV_DISK1),C0(AVP_WC_HIDE_OVER),EA};
static const AvpWepCmd disk_act[]={C0(AVP_WC_SHOW_OVER),C1(AVP_WC_SET_TIME,1),EF,C1(AVP_WC_SET_OVER,OV_DISK2),C1(AVP_WC_SET_TIME,1),EF,
 C1(AVP_WC_SOUND,SFX_DISK),C1(AVP_WC_PROJECTILE,0),C1(AVP_WC_SET_OVER,OV_DISK3),C1(AVP_WC_SET_TIME,1),EF,C2(AVP_WC_OFFSET_POS,-20,20),C1(AVP_WC_SET_TIME,1),EF,
 C0(AVP_WC_HIDE_OVER),C1(AVP_WC_SET_TIME,10),EF,EA};
static const AvpWepCmd punch_f0[]={C1(AVP_WC_SET_OVER,OV_PUNCH1),C0(AVP_WC_HIDE_OVER),EA};
static const AvpWepCmd punch_act[]={C0(AVP_WC_SHOW_OVER),C1(AVP_WC_SET_TIME,1),EF,C1(AVP_WC_SOUND,SFX_WRBLADE),C0(AVP_WC_FIRE),C1(AVP_WC_SET_OVER,OV_PUNCH2),C1(AVP_WC_SET_TIME,3),EF,
 C1(AVP_WC_SET_OVER,OV_PUNCH1),C2(AVP_WC_OFFSET_POS,6,20),C1(AVP_WC_SET_TIME,1),EF,C0(AVP_WC_HIDE_OVER),C1(AVP_WC_SET_TIME,5),EF,EA};
static const AvpWepCmd invis_action[]={C0(AVP_WC_TOGGLE_INVIS),EF,EA};
/* Source medpak action is also toggle_invis. Preserve it rather than “fixing” it. */
static const AvpWepCmd medpak_action[]={C0(AVP_WC_TOGGLE_INVIS),EF,EA};

#define DEF(n,dam,dist,w,ia,ma,f0,mi,mo,a0) {n,dam,dist,w,ia,ma,f0,mi,mo,{a0,NULL},1}
static const AvpWeaponDef human_defs[]={
 DEF("shotgun",10,0x880,40,0,44,shot_f0,std_in,std_out,shot_act),
 DEF("pulse rifle",5,0x880,20,0,264,pulse_f0,std_in,std_out,pulse_act),
 DEF("flamethrower",0,0,0,0,88,flame_f0,std_in,std_out,flame_act),
 DEF("smartgun",60,0x880,20,0,88,smart_f0,std_in,std_out,smart_act)};
static const AvpWeaponDef alien_defs[]={
 DEF("tail",20,0x90,20,200,200,tail_f0,tail_in,tail_out,tail_act),
 DEF("swipe",30,0x90,20,200,200,swipe_f0,swipe_in,swipe_out,swipe_act),
 DEF("bite",100,0x90,20,200,200,bite_f0,bite_in,bite_out,bite_act)};
static const AvpWeaponDef pred_defs[]={
 DEF("combi-stick",100,0x90,20,200,200,stick_f0,stick_in,stick_out,stick_act),
 DEF("shoulder cannon",0,0,0,200,200,no_frame0,no_action,no_action,laser_act),
 DEF("smart disc",0,0,0,200,200,disk_f0,no_action,no_action,disk_act),
 DEF("wrist blade",30,0x90,20,200,200,punch_f0,no_action,no_action,punch_act),
 DEF("invisibility",0,0,0,0,0xffff,no_frame0,no_action,no_action,invis_action),
 DEF("medpak",0,0,0,0,1000,no_frame0,no_action,no_action,medpak_action)};

const AvpWeaponDef *avp_weapon_def(s16 pt,s16 no){
    if(no<=0) return NULL;
    unsigned i=(unsigned)(no-1);
    if(pt==PT_HUMAN)return i<4?&human_defs[i]:NULL;
    if(pt==PT_ALIEN)return i<3?&alien_defs[i]:NULL;
    if(pt==PT_PREDATOR)return i<6?&pred_defs[i]:NULL;
    return NULL;
}

void InitDblBufs(void){screen_flip=0;build_screen=screen_a;}
void RestoreMazeList(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->restore_maze_list)o->restore_maze_list(o->user);}
void SetMazeList(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->set_maze_list)o->set_maze_list(o->user);}
void DoPause(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->pause)o->pause(o->user);}
void MazeList(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->gpu_draw_screen)o->gpu_draw_screen(o->user);}
void LoseSounds(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->kill_sounds)o->kill_sounds(o->user);}
void RestoreSounds(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->ambient)o->ambient(o->user);}
void pause_off(void){RestoreSounds();const AvpRuntimeOps*o=avp_runtime_ops();if(o->restore_maze_list)o->restore_maze_list(o->user);}
void ExpandOvers(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->file_event)o->file_event(o->user,0x4f564552u,(u32)(u16)player_type,0,0);}
void SwapScreens(void){screen_flip^=1;build_screen=screen_flip?screen_b:screen_a;const AvpRuntimeOps*o=avp_runtime_ops();if(o->swap_screens)o->swap_screens(o->user);}
void PreFrame(void){UpdtScrOverlays();const AvpRuntimeOps*o=avp_runtime_ops();if(o->pre_frame)o->pre_frame(o->user);}
void PostFrame(void){
    set_avail(); fade_selects(); if(player_type==PT_PREDATOR)PXFades();
    const AvpRuntimeOps*o=avp_runtime_ops();if(o->post_frame)o->post_frame(o->user);
}

#define MAX_FADE 6
#define FADE_STEP 1
#define MED_TRANS 10
#define WEP_MEDPAK 6
#define MPAK_MARK 6
#define MPAK_SPACE 3

void InitPXFades(void){
    invis_bright=0; medpak_bright=0; mpak_maxed=mpak_time=mpak_state=0; medpak_ID=0;
}

/* Source set_avail/fade_selects game-side state.  Pixel shading and cross-hatch
 * are presentation-only Blitter operations and remain a renderer responsibility. */
void set_avail(void){
    u8 now=cur_weps, changed=(u8)(now^old_weps);
    if(!changed)return;
    u8 falling=(u8)(changed & (u8)~now);
    for(s16 w=1;w<=num_weps;w++){
        u8 bit=(u8)(1u<<w);
        if(falling&bit){
            if(cur_wepno==w && (new_wepno==0 || new_wepno==w))new_wepno=-1;
            if(inwep_no==w)remove_wepno=w;
        }
    }
    old_weps=now;
}

void fade_selects(void){
    if(outwep_bright==0 && inwep_bright>=MAX_FADE){
        s16 n=new_wepno;
        if(n<=0)n=cur_wepno;
        if(n!=inwep_no){
            if(inwep_no){outwep_no=inwep_no;outwep_bright=inwep_bright;inwep_bright=MAX_FADE;}
            inwep_no=n;
            if(inwep_no)inwep_bright=0;
        }
    }
    if(inwep_bright<MAX_FADE)++inwep_bright;
    if(outwep_bright){
        --outwep_bright;
        if(outwep_bright==0){s16 old=outwep_no;outwep_no=0;if(old==remove_wepno)remove_wepno=0;}
    }
}

void PXFades(void){
    const AvpRuntimeOps *o=avp_runtime_ops();
    if(invis_stat==0){
        if(invis_act){if(invis_bright<MAX_FADE && ++invis_bright>=MAX_FADE){invis_bright=MAX_FADE;invis_stat=1;}}
        else if(invis_bright>0)--invis_bright;
    }
    s16 charge=medpak;
    if(!(cur_weps&(1u<<WEP_MEDPAK)) && charge)cur_weps=(u8)(cur_weps|(1u<<WEP_MEDPAK));
    if(medpak_act){
        if(!medpak_ID){if(o->play_sfx)o->play_sfx(o->user,61u);medpak_ID=1;}
        if(charge){
            s16 transfer=MED_TRANS;if(charge<transfer)transfer=charge;charge=(s16)(charge-transfer);medpak=charge;
            s32 energy=(s32)player_energy+(s32)transfer*6;int maxed=0;if(energy>=max_energy){energy=max_energy;maxed=1;}player_energy=(s16)energy;
            if(medpak_bright<MAX_FADE)++medpak_bright;
            else if(maxed){
                if(!mpak_maxed){mpak_maxed=0xff;mpak_time=0;mpak_state=0;}
                if((s8)--mpak_time<0){mpak_state=(u8)~mpak_state;if(mpak_state){mpak_time=MPAK_MARK-1;medpak_bright=MAX_FADE;}else{mpak_time=MPAK_SPACE-1;medpak_bright=0;}}
            }else if(mpak_maxed){mpak_maxed=0;if(!mpak_state)medpak_bright=MAX_FADE;}
        }else medpak_act=0;
    }
    if(!medpak_act){
        mpak_maxed=0;
        if(medpak_bright>0)--medpak_bright;
        if(medpak_bright==0){medpak_ID=0;if(charge==0)cur_weps=(u8)(cur_weps&~(1u<<WEP_MEDPAK));}
    }
}

void InitScrOverlays(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->init_screen_overlays)o->init_screen_overlays(o->user);Init1stOvers();StdHUD();}
void hug_init(void){hug_recov=0;hug_throw=0;unstick_dir=0;}
void StdHUD(void){InitHUD();}

void enable_swing(void){swing_on=0xffu;} void disable_swing(void){swing_on=0;}
void InitSwing(void){static const s16 rate[3]={0x6000,0x4000,0x6000};unsigned i=(unsigned)player_type>>2;if(i>2)i=0;swing_max=0x2000;gunratemul=rate[i];swing_angle=0x4000;chase_vel=0;SwingPos();}
void SwingPos(void){
    s16 target=(s16)player_vel;
    if(chase_vel<target){s32 n=chase_vel+0x1000;chase_vel=(s16)(n>target?target:n);} else if(chase_vel>target){s32 n=chase_vel-0x1000;chase_vel=(s16)(n<target?target:n);}
    u32 rate=(u32)(u16)gunratemul*(u16)chase_vel; swing_angle=(s16)(swing_angle+(s16)(rate>>16));
    s32 cx=cos_d0((u16)swing_angle),cy=cos_d0((u16)((u16)swing_angle<<1)); s16 amp=chase_vel;if(amp>swing_max)amp=swing_max;
    s32 xx=(s32)(s16)cx*amp, yy=(s32)(s16)cy*amp; swing_x=(s16)((xx>>16)>>8); swing_y=(s16)(-(((yy>>16)>>9)&~1));
}

static void reset_frame0(void){active_wep=0;wep_x=wep_y=wep_xvel=wep_yvel=0;use1_visible=use2_visible=0;over_visible=1;enable_swing();}
static void execute_cmd(const AvpWepCmd **pp,const AvpWepCmd *c){
    const AvpRuntimeOps *o=avp_runtime_ops();
    switch(c->op){
    case AVP_WC_ZERO_POS:wep_x=wep_y=0;break;
    case AVP_WC_SET_POS:wep_x=(s32)c->a<<16;wep_y=(s32)c->b<<16;break;
    case AVP_WC_OFFSET_POS:wep_x+=(s32)c->a<<16;wep_y+=(s32)c->b<<16;break;
    case AVP_WC_ZERO_VEL:wep_xvel=wep_yvel=0;break;
    case AVP_WC_SET_VEL:wep_xvel=(s32)c->a<<16;wep_yvel=(s32)c->b<<16;break;
    case AVP_WC_SET_FVEL:wep_xvel=c->a;wep_yvel=c->b;break;
    case AVP_WC_SET_TIME:a1st_time=(u16)c->a;break;
    case AVP_WC_WAIT_FIRE:wait_state=0xffu;wait_ptr=*pp;break;
    case AVP_WC_FIRE:fire_wep();break;
    case AVP_WC_PROJECTILE:{s32 sy=-sin_ang;player_weapon(cos_ang,sy,(u16)c->a,(u16)0,sin_ang,cos_ang);fire_damage=-1;break;}
    case AVP_WC_TOGGLE_INVIS:toggle_invis();break;
    case AVP_WC_SET_OVER:over_id=c->a;over_visible=1;break;
    case AVP_WC_SHOW_OVER:over_visible=1;break;case AVP_WC_HIDE_OVER:over_visible=0;break;
    case AVP_WC_SHOW_USE1:use1_visible=1;break;case AVP_WC_HIDE_USE1:use1_visible=0;break;
    case AVP_WC_SHOW_USE:use1_visible=use2_visible=1;break;case AVP_WC_HIDE_USE:use1_visible=use2_visible=0;break;
    case AVP_WC_SOUND:case AVP_WC_SOUND_ALT:if(o->play_sfx)o->play_sfx(o->user,(unsigned)c->a);break;
    case AVP_WC_KILL_SOUND:break;
    case AVP_WC_ENABLE_SWING:enable_swing();break;case AVP_WC_DISABLE_SWING:disable_swing();break;
    case AVP_WC_SET_BITE:alien_bite=c->a;break;case AVP_WC_CLEAR_BITE:alien_bite=0;break;
    default:break;
    }
}
static int play_frame(const AvpWepCmd **pp){
    const AvpWepCmd *p=*pp;
    for(;;){const AvpWepCmd *c=p++;if(c->op==AVP_WC_END_FRAME){*pp=p;return 1;}if(c->op==AVP_WC_END_ANIM){*pp=p;return 0;}execute_cmd(&p,c);}
}
void frame0(void){reset_frame0();if(cur_def&&cur_def->frame0){const AvpWepCmd*p=cur_def->frame0;while(p->op!=AVP_WC_END_ANIM){execute_cmd(&p,p);p++;}}}
void new_weapon(void){cur_def=avp_weapon_def(player_type,cur_wepno);if(!cur_def){cur_wepno=0;over_visible=use1_visible=use2_visible=0;wep_animptr=NULL;return;}frame0();wep_animptr=cur_def->move_in;wep_action=0;wep_desel=0;wait_state=0;fire_damage=0;}
void fire_wep(void){if(!cur_def)return;fire_damage=cur_def->damage;fire_distance=(u32)cur_def->distance<<8;fire_width=cur_def->width;}
void toggle_invis(void){invisflag^=(1u<<AMP_INVHIT);}

void InitWeps(void){
    unsigned n=player_type==PT_HUMAN?4:player_type==PT_ALIEN?3:6;num_weps=(s16)n;
    memset(ammo_info,0,sizeof(ammo_info));
    for(unsigned i=0;i<n&&i<6;i++){const AvpWeaponDef*d=avp_weapon_def(player_type,(s16)(i+1));AvpAmmoInfo*a=&ammo_info[i];if(player_type==PT_HUMAN || i>=4){a->cur=d->init_or_dec;a->max=d->max_or_inc;a->dec=0;a->inc=0;}else{a->cur=AVP_APAMM_MAX;a->max=AVP_APAMM_MAX;a->dec=d->init_or_dec;a->inc=d->max_or_inc;}a->old=0xffffu;a->rescale=a->max?(u16)(0xffffffffu/a->max):0;}
}
void fill_weps(void){for(int i=0;i<num_weps&&i<6;i++)ammo_info[i].cur=ammo_info[i].max;}
void init_fades(void){outwep_bright=0;inwep_bright=MAX_FADE;inwep_no=0;outwep_no=0;remove_wepno=0;}
void Init1stOvers(void){InitSwing();InitWeps();if(player_type==PT_PREDATOR){InitPredAvail();InitPXFades();}init_fades();fire_damage=0;new_wepno=player_type==PT_PREDATOR?4:1;cur_wepno=0;cur_def=NULL;wep_low=0;cur_weps=player_type==PT_HUMAN?0:player_type==PT_ALIEN?0x0e:0x30;old_weps=0xff;hugkill=0;hug_recov=0;hug_throw=0;}

static void start_action(void){if(!cur_def||cur_def->action_count==0)return;unsigned a=cur_action++;if(cur_action>=cur_def->action_count)cur_action=0;wep_animptr=cur_def->actions[a];active_wep^=-1;}
void update_wep(void){
    if(hugkill){if(hug_recov){hugkill=0;hug_recov=0;over_visible=0;wep_fire=0;wait_state=0;if(cur_def)wep_animptr=cur_def->move_in;return;}if(hug_throw==0){hug_throw=9;over_visible=1;use1_visible=use2_visible=0;}player_energy=(s16)(player_energy-10);return;}
    if(new_wepno>=0 && ((cur_weps&(1u<<new_wepno))==0))new_wepno=0;
    if(!wep_desel&&new_wepno==cur_wepno)new_wepno=0;
    if(!cur_def){if(new_wepno){cur_wepno=new_wepno;new_wepno=0;new_weapon();}return;}
    if(new_wepno)wait_state=0;
    if(!wep_animptr){
        if(new_wepno){wep_desel=0xffu;wep_animptr=cur_def->move_out;}
        else if(wep_fire){AvpAmmoInfo*a=&ammo_info[cur_wepno-1];if(a->cur>wep_low)start_action();}
    }
    if(wep_animptr){
        if(a1st_time>1){--a1st_time;}else{
            a1st_time=1;
            if(wait_state){AvpAmmoInfo*a=&ammo_info[cur_wepno-1];if(a->cur&&wep_fire)wep_animptr=wait_ptr;else wait_state=0;}
            if(!wait_state){
                if(!play_frame(&wep_animptr)){
                    if(wep_desel){wep_desel=0;cur_wepno=new_wepno;new_wepno=0;new_weapon();}
                    else{wep_animptr=NULL;frame0();}
                }
            }else play_frame(&wep_animptr);
        }
    }
    SwingPos();wep_x+=wep_xvel;wep_y+=wep_yvel;
    (void)over_id;(void)over_visible;(void)use1_visible;(void)use2_visible;
}

void process_shot(void){
    if(fire_damage==0||!cur_def||cur_wepno<=0) return;
    AvpAmmoInfo *a=&ammo_info[cur_wepno-1];
    u16 charge=a->cur;
    if(player_type==PT_HUMAN&&charge){charge--;if(charge==0&&cheat)charge=a->max;a->cur=charge;}
    if(fire_damage>0){s16 dam=fire_damage;if(player_type!=PT_HUMAN){u32 d=(u32)charge*(u16)dam;d>>=14;dam=(s16)((d+(u16)dam)>>1);fire_damage=dam;}AvpAmp*hit=shot_cb?shot_cb(dam,fire_distance,fire_width):TestSpark();if(hit&&player_type==PT_ALIEN&&cur_wepno!=2){if(cur_wepno==1)hit->flags^=(1u<<AMP_STUN);else hit->flags&=(u16)~(1u<<AMP_STUN);}}
    fire_damage=0;
}
void recharge(void){if(player_type==PT_HUMAN)return;for(int i=0;i<num_weps&&i<6;i++){AvpAmmoInfo*a=&ammo_info[i];s32 v;if(i==cur_wepno-1&&active_wep)v=(s32)a->cur-a->dec;else v=(s32)a->cur+a->inc;if(v<0)v=0;if(v>a->max)v=a->max;a->cur=(u16)v;}}
void UpdtScrOverlays(void){if(player_type==PT_PREDATOR)PredAvail();update_wep();process_shot();recharge();}
