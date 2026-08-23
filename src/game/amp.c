/* Readable C reconstruction of the ordinary 68000 AMP allocator/lifecycle
 * core from AMP/AMP.S.  Creature-specific mode handlers are translated in
 * subsequent sections of this module family.
 *
 * The original AMP record is 48 bytes on m68k.  This host-readable struct uses
 * a real function pointer for amp_mode, so host sizeof(AvpAmp) is intentionally
 * not asserted to the Jaguar binary layout.
 */
#include "amp.h"
#include "avp_runtime.h"
#include "maze.h"
#include "player.h"
#include "levels.h"
#include "hud.h"
#include "collision.h"
#include "mazescrn.h"
#include "weapons.h"
#include <stddef.h>
#include <string.h>

AvpAmp level1amps[AVP_NUM_AMPS];
AvpAmp *amp_data=level1amps,*amps_at=level1amps;
u16 collmap[AVP_AMP_GRID_W*AVP_AMP_GRID_H];
u8 objmap[AVP_AMP_OBJ_ROW_BYTES*AVP_AMP_OBJ_H];
u16 levels_visit,discflag;
u8 inventory_table[16];
u32 ampcount;

static u16 free_list[AVP_NUM_AMPS];
static unsigned free_count;
static void (*save_cb)(AvpAmp *,unsigned);
static void (*restore_cb)(AvpAmp *,unsigned);
static AvpAmpSavedPlacement saved_levels[AVP_MAX_LEVEL][AVP_NUM_AMPS+1];
static const AvpAmpRandomEntry *random_lists[3][AVP_MAX_LEVEL];

void avp_amp_set_save_callback(void (*fn)(AvpAmp *,unsigned)){save_cb=fn;}
void avp_amp_set_restore_callback(void (*fn)(AvpAmp *,unsigned)){restore_cb=fn;}

void initialise(void)
{
    unsigned i;
    /* Historical code only clears amp_mode; other fields are left alone until
     * an active slot is initialized from a template or amp_req. */
    for(i=0;i<AVP_NUM_AMPS;++i) level1amps[i].mode=NULL;
    amp_data=amps_at=level1amps;
    memset(collmap,0,sizeof(collmap));
    memset(objmap,0,sizeof(objmap));
    discflag=0;
    {
        static const u8 init_invent[16]={0,3,0,0,0,6,0,0,0,6,0,0,0,6,0,0};
        /* AMP.S copies the raw 16-byte init_invent block byte-for-byte. */
        memcpy(inventory_table,init_invent,sizeof(inventory_table));
    }
}

void wipe_flgs(void){levels_visit=0;}

void InitAMPs(void)
{
    initialise();
    /* Source deliberately does not call build_level here. */
    wipe_flgs();
}

static void reset_free_list(void)
{
    for(unsigned i=0;i<AVP_NUM_AMPS;++i)free_list[i]=(u16)(AVP_NUM_AMPS-1u-i);
    free_count=AVP_NUM_AMPS;
}

void build_level(void)
{
    reset_free_list();
    /* Retail !CEDITOR always selects common_list inside level_loop; the player
     * classification still exists in the source immediately before it. */
    level_loop();
}

AvpAmp *amp_req(void)
{
    unsigned index;
    AvpAmp *a;
    if(free_count==0) return &level1amps[AVP_NUM_AMPS-1]; /* retail clamps on underflow */
    --free_count;
    index=free_list[free_count];
    if(index>=AVP_NUM_AMPS) index=AVP_NUM_AMPS-1;
    a=&amp_data[index];
    a->flags=0;
    a->yoffset=0;
    a->astype=-1;
    return a;
}

void amp_release(unsigned current_amp_number_1based)
{
    u16 index;
    if(current_amp_number_1based==0) return;
    index=(u16)(current_amp_number_1based-1u);
    if(free_count>=AVP_NUM_AMPS) return;
    free_list[free_count++]=index;
}

const AvpAmpSavedPlacement *avp_amp_saved_level(unsigned level_1based)
{
    return (level_1based>=1u&&level_1based<=AVP_MAX_LEVEL)?saved_levels[level_1based-1u]:NULL;
}

void ram_save(void)
{
    AvpAmpSavedPlacement *out;
    unsigned n=0;
    if(cur_level<1||cur_level>AVP_MAX_LEVEL)return;
    out=saved_levels[cur_level-1];
    for(unsigned i=0;i<AVP_NUM_AMPS;i++){
        AvpAmp *a=&amp_data[i];
        if((!a->mode&&!a->host_static)||a->astype<0)continue;
        if(n>=AVP_NUM_AMPS)break;
        out[n].x=(s16)((u32)a->xpos>>8);
        out[n].y=(s16)((u32)a->ypos>>8);
        out[n].astype=a->astype;
        out[n].flags=a->flags;
        ++n;
    }
    out[n].x=-1;out[n].y=0;out[n].astype=0;out[n].flags=0;
}

void save_level(void)
{
    /* The two projectile loop IDs are DSP/backend state in the portable tree;
     * ram_save is the CPU/game-state part of the historical routine. */
    ram_save();
    if(save_cb)save_cb(amp_data,AVP_NUM_AMPS);
}

static void clear_runtime_grids(void)
{
    memset(collmap,0,sizeof(collmap));
    memset(objmap,0,sizeof(objmap));
}

void restore_level(void)
{
    int visited;
    if(cur_level<1||cur_level>AVP_MAX_LEVEL)return;
    ngens=0;discflag=0;amp_data=amps_at=level1amps;
    initialise();
    place_grid();
    visited=(levels_visit&(u16)(1u<<((unsigned)cur_level-1u)))!=0;
    if(!visited){
        build_level();
        append_objs();
        ampcount=0;
        xcocoons();
        rand_set();
        clear_runtime_grids();
        if(cur_level==14&&player_type!=PT_ALIEN)make_queen();
        if(cur_level==15&&player_type==PT_HUMAN)end_preds();
        if(cur_level==15&&player_type==PT_ALIEN)make_end_queen();
    }else{
        rebuild_level();
        clear_runtime_grids();
    }
    if(restore_cb)restore_cb(amp_data,AVP_NUM_AMPS);
}

/* Projectile construction and invisible-player search are direct translations
 * of AMP.S player_weapon/lostplayer. */
extern u32 x_pos,y_pos;
extern u16 invisflag;
extern u16 avp_random(void);
extern void pre_discmove(AvpAmp *);
extern void pre_flame_on(AvpAmp *);

static u32 beam_counter;
static void amp_chase_target(AvpAmp *,s32,s32,int);
static void make_dead_egg(AvpAmp *);
static void (*chase_cb)(AvpAmp *,s32,s32);
void avp_amp_set_chase_callback(void (*fn)(AvpAmp *,s32,s32)){chase_cb=fn;}

static s32 scale_projectile_component(s32 heading,u16 velocity)
{
    /* 68000: velocity<<8; MULS.W with heading's low word; <<2; SWAP;
     * EXT.L.  Keep the exact signed-16 multiply/truncation sequence. */
    s16 h=(s16)heading;
    s16 mul=(s16)(velocity<<8);
    s32 p=(s32)h*(s32)mul;
    u32 q=(u32)p<<2;
    s16 w=(s16)(q>>16);
    return (s32)w;
}

AvpAmp *player_weapon(s32 heading_x,s32 heading_y,u16 kind,u16 reset_counter,
                      s32 sin_angle,s32 cos_angle)
{
    static const struct {
        u16 creature; s16 yoffset; AvpAmpModeFn mode; u16 velocity;
    } projectile[3]={
        {AC_SMART,20,pre_discmove,30},
        {AC_FIRE,9,pre_flame_on,40},
        {AC_BOLT,-38,pre_flame_on,24}
    };
    const unsigned k=(kind<3)?kind:0;
    AvpAmp *a=amp_req();
    s32 px=(s32)x_pos,py=(s32)y_pos;
    s32 vx,vy;
    if(kind==2){px-=sin_angle>>2;py-=cos_angle>>2;}
    a->xpos=px;a->ypos=py;
    a->animseq=AS_STAND;a->animframe=0;
    a->creature=projectile[k].creature;
    a->yoffset=projectile[k].yoffset;
    a->mode=projectile[k].mode;
    a->yvel=(s16)reset_counter;     /* amp_launchtime alias */
    a->xvel=(s16)-1;                /* amp_launchamp: player launch */
    a->flags=(u16)((1u<<AMP_PLAYER)|(1u<<AMP_PHIT)|invisflag);
    vx=scale_projectile_component(heading_x,projectile[k].velocity);
    vy=scale_projectile_component(heading_y,projectile[k].velocity);
    a->xvector=vx;a->yvector=vy;
    a->xpos+=vx;a->ypos+=vy;
    a->timer=1;
    return a;
}

static u16 fseq_dir(const AvpAmp *a){return (u16)((u32)a->xvector>>16);}
static s16 fseq_count(const AvpAmp *a){return (s16)(u16)a->xvector;}
static void set_fseq(AvpAmp *a,u16 dir,s16 count)
{a->xvector=(s32)(((u32)dir<<16)|(u16)count);}

void lostplayer(AvpAmp *a)
{
    u16 dir;
    s16 count;
    s32 tx,ty;
    if(!a)return;
    if(invisflag==0 || (a->flags&(1u<<AMP_PHIT))){
        a->xvector=0;                 /* clr.l amp_fseq */
        a->mode=chase_player;
        return;                       /* source only restores chase_x/y here */
    }
    dir=fseq_dir(a);count=(s16)(fseq_count(a)-1);
    if(count<0){count=(s16)(avp_random()&255u);dir=(u16)(avp_random()&3u);}
    set_fseq(a,dir,count);
    switch(dir){
    default:case 0:tx=(s32)0x7fff0000u;ty=0;break;
    case 1:tx=(s32)0x7fff0000u;ty=(s32)0x7fff0000u;break;
    case 2:tx=0;ty=0;break;
    case 3:tx=0;ty=(s32)0x7fff0000u;break;
    }
    /* Historical .here calls chase_player with chase_x/chase_y temporarily
     * redirected.  The source-shaped chase helper accepts that target while
     * retaining all combat/recoil checks. */
    amp_chase_target(a,tx,ty,1);
}

/* -------------------------------------------------------------------------
 * Active projectile/effect/generator modes translated from AMP/AMP.S.      */
u16 ngens;
u16 avp_reset_counter;
static u16 cur_amp_no;
static u16 entime;
static s16 explo1;
extern s16 use_cocoon;
static void spark(AvpAmp *a);

void avp_amp_set_reset_counter(u16 counter){avp_reset_counter=counter;}

static unsigned amp_number(const AvpAmp *a)
{
    ptrdiff_t n=a-amp_data;
    return (n>=0 && n<AVP_NUM_AMPS)?(unsigned)n+1u:0u;
}
static void release_amp(AvpAmp *a)
{
    unsigned n=amp_number(a);
    a->mode=NULL; a->host_static=0; a->aux_ptr=NULL;
    if(n) amp_release(n);
}
static u32 abs32diff(s32 a,s32 b){s64 d=(s64)a-(s64)b;return (u32)(d<0?-d:d);}
static int within_player(const AvpAmp *a,u16 pixels)
{
    u32 lim=(u32)pixels<<9;
    return abs32diff(a->xpos,(s32)x_pos)<=lim && abs32diff(a->ypos,(s32)y_pos)<=lim;
}
static int safe_projectile(const AvpAmp *a,u16 width)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    return o->safe_pos ? (o->safe_pos(o->user,a->xpos,a->ypos,70,width)!=AVP_COLL_SAFE)
                       : (SafePos(70,a->xpos,a->ypos,width)!=AVP_COLL_SAFE);
}
static AvpAmp *collmap_amp(const AvpAmp *p)
{
    /* move.w amp_xpos/amp_ypos reads the first word of each big-endian
     * 32-bit fixed-point coordinate: the integer cell, not the fraction. */
    unsigned gx=(unsigned)((u32)p->xpos>>16);
    unsigned gy=(unsigned)((u32)p->ypos>>16);
    if(gx>=AVP_AMP_GRID_W || gy>=AVP_AMP_GRID_H) return NULL;
    u16 no=collmap[gy*AVP_AMP_GRID_W+gx];
    if(no==0 || no>AVP_NUM_AMPS || no==(u16)p->xvel) return NULL;
    return &amp_data[no-1u];
}
static int projectile_pixels_hit(const AvpAmp *p,const AvpAmp *victim)
{
    u32 lim;
    switch(p->creature){
    case AC_BOLT:  lim=(24u+1u)<<9; break;
    case AC_SMART: lim=(30u+1u)<<9; break;
    default:       lim=(40u+1u)<<9; break;
    }
    return abs32diff(p->xpos,victim->xpos)<=lim && abs32diff(p->ypos,victim->ypos)<=lim;
}
AvpAmp *weapon_hit(AvpAmp *p)
{
    AvpAmp *v=collmap_amp(p);
    if(!v || !projectile_pixels_hit(p,v)) return NULL;
    s16 e=v->energy;
    if(p->creature==AC_BOLT) e=(s16)(e-100);
    else if(p->creature==AC_SMART) e=(s16)(e-50);
    else if(p->creature==AC_FIRE) e=(s16)(e-10); /* deliberately less than player damage */
    v->energy=e;
    u16 transfer=(u16)(p->flags&((1u<<AMP_PHIT)|(1u<<AMP_INVHIT)));
    if(e<=0) v->flags&=(u16)~(1u<<AMP_PHIT);
    v->flags|=transfer;
    v->flags&=(u16)~(1u<<AMP_CLOSEHIT);
    return v;
}

static void disc_kill2(AvpAmp *a){release_amp(a);discflag=0;}
static void disc_kill1(AvpAmp *a){a->mode=disc_kill2;}
void pre_discmove(AvpAmp *a)
{
    a->mode=discmove;
    if((u16)a->yvel!=avp_reset_counter) discmove(a);
}
void discmove(AvpAmp *a)
{
    a->creature=AC_SMART;
    if(++a->animframe==3) a->animframe=0;
    if(safe_projectile(a,40)){const AvpRuntimeOps*o=avp_runtime_ops();if(o->play_sfx)o->play_sfx(o->user,20);disc_kill1(a);return;}
    if(!(a->flags&(1u<<AMP_PLAYER)) && within_player(a,40)){
        player_energy=(s16)(player_energy-50);discflag=0;
        const AvpRuntimeOps*o=avp_runtime_ops();if(o->play_sfx)o->play_sfx(o->user,21);
        release_amp(a);return;
    }
    if(weapon_hit(a)){const AvpRuntimeOps*o=avp_runtime_ops();if(o->play_sfx)o->play_sfx(o->user,22);disc_kill1(a);return;}
    a->xpos+=a->xvector;a->ypos+=a->yvector;
}

static void flame_kill2(AvpAmp *a){release_amp(a);discflag=0;}
static void flame_kill1(AvpAmp *a){a->mode=flame_kill2;}
AvpAmp *explosion_ex(s32 x,s32 y,s16 yoff,s16 damage,u32 range)
{
    AvpAmp *e=amp_req();
    e->xpos=x;e->ypos=y;e->yoffset=yoff;e->energy=damage;e->timer=(s32)range;
    e->creature=AC_FIRE;e->animseq=AS_STAND;e->animframe=0;e->mode=exp_handle;
    e->xvel=5;e->yvel=7;e->host_static=0;e->aux_ptr=NULL;
    if(!explo1){const AvpRuntimeOps*o=avp_runtime_ops();if(o->play_sfx)o->play_sfx(o->user,23);explo1=-1;}
    return e;
}
void explosion_at(s32 x,s32 y){(void)explosion_ex(x,y,0,200,0x10000u);}
AvpAmp *explosion(s32 x,s32 y,s16 yoff,s16 damage,u32 range){return explosion_ex(x,y,yoff,damage,range);}
void exp_handle(AvpAmp *a)
{
    u16 f=(u16)(a->animframe+1u);a->animframe=f;
    if(f==(u16)a->xvel){
        const AvpRuntimeOps*o=avp_runtime_ops();
        u16 k=(u16)(a->flags&((1u<<AMP_PHIT)|(1u<<AMP_INVHIT)));
        if(o->area_damage)o->area_damage(o->user,a->xpos,a->ypos,(u32)a->timer,a->energy,k);
        else AreaDamage(a->xpos,a->ypos,(s32)(u32)a->timer,a->energy,k);
    }
    if(f>=(u16)a->yvel) release_amp(a);
}
void pre_flame_on(AvpAmp *a)
{
    a->mode=flame_on;
    if((u16)a->yvel!=avp_reset_counter) flame_on(a);
}
void flame_on(AvpAmp *a)
{
    discflag=1;
    if(a->timer>0){--a->timer;if(a->timer!=0){a->animframe=0;a->animseq=AS_HIDDEN;return;}a->animseq=AS_STAND;}
    a->flags^=(1u<<AMP_ALTFRAME);
    if(a->flags&(1u<<AMP_ALTFRAME)){if(a->animframe<6)++a->animframe;}
    if(safe_projectile(a,40)){
        if(a->creature==AC_BOLT){AvpAmp *e=explosion_ex(a->xpos,a->ypos,-38,200,0x10000u);e->flags=(u16)(a->flags&((1u<<AMP_PHIT)|(1u<<AMP_INVHIT)));flame_kill2(a);}
        else flame_kill1(a);
        return;
    }
    if(!(a->flags&(1u<<AMP_PLAYER)) && within_player(a,40)){
        player_energy=(s16)(player_energy-(a->creature==AC_BOLT?100:25));release_amp(a);discflag=0;return;
    }
    if(weapon_hit(a)){flame_kill1(a);return;}
    a->xpos+=a->xvector;a->ypos+=a->yvector;
}

static void make_spark_common(s32 x,s32 y,const AvpAmp *victim,int legacy_blood)
{
    AvpAmp *a=amp_req();u16 kind=AC_SPARK1,frames=4;s16 yoff=0;
    a->xpos=x;a->ypos=y;a->animseq=AS_STAND;a->animframe=(u16)-1;
    if(victim){
        switch(victim->creature){
        case AC_ALIEN:case AC_AQUEEN:kind=AC_ABLOOD;frames=2;break;
        case AC_PREDATOR:kind=AC_PBLOOD;frames=2;break;
        case AC_HUMAN:kind=AC_MBLOOD;frames=2;break;
        case AC_EGG:yoff=30;break;
        case AC_DRUM:yoff=20;break;
        case AC_AMMO2:yoff=10;break;
        case AC_FACEHUG:yoff=40;break;
        default:break;
        }
    }else if(legacy_blood){kind=AC_ABLOOD;frames=2;}
    a->creature=kind;a->energy=(s16)frames;
    a->yoffset=(s16)(yoff-(s16)(avp_random()&3u));a->mode=spark;a->host_static=0;
}
void make_spark_at(s32 x,s32 y,int blood){make_spark_common(x,y,NULL,blood);}
void make_spark_hit_at(s32 x,s32 y,const AvpAmp *victim){make_spark_common(x,y,victim,0);}
void make_spark(s32 x,s32 y,const AvpAmp *victim){make_spark_common(x,y,victim,0);}
static void spark(AvpAmp *a){++a->animframe;if(--a->energy==0)release_amp(a);}

/* Generator / queen-shield complex.  Pointer-valued amp_timer in the Jaguar
 * source is represented by aux_ptr on 64-bit hosts. */
static void dead_gen2(AvpAmp *a);
static void amp_cleargrid_c(AvpAmp *a);
static void amp_shield_start_mode(AvpAmp *a);
static void dead_gen(AvpAmp *a)
{
    const AvpRuntimeOps*o=avp_runtime_ops();if(o->play_sfx)o->play_sfx(o->user,23);
    a->mode=dead_gen2;a->animseq=AS_DEATH;a->animframe=0;
}
static void dead_gen2(AvpAmp *a)
{
    if(++a->animframe==6){
        a->mode=NULL; a->host_static=0;
        amp_cleargrid_c(a);
        player_energy=(s16)(player_energy-50);
    }
}
AvpAmp *make_shield(AvpAmp *owner,s32 dx,s32 dy)
{
    AvpAmp *s=amp_req();s->xpos=owner->xpos+dx;s->ypos=owner->ypos+dy;s->xvel=-2;
    s->mode=amp_shield_start_mode;s->creature=AC_AQSHIELD;s->animseq=AS_STAND;s->animframe=(u16)(avp_random()&7u);return s;
}
void shield2(AvpAmp *a)
{
    const AvpRuntimeOps *o=avp_runtime_ops();unsigned snd=0;
    if(within_player(a,70)){player_energy=(s16)(player_energy-10);snd=24;}
    else if(within_player(a,100)){player_energy=(s16)(player_energy-5);snd=25;}
    if(snd){s16 c=(s16)(a->xvel+1);if(c>=7){c=0;if(o->play_sfx)o->play_sfx(o->user,snd);}a->xvel=c;}
    a->animseq=AS_STAND;
    if(!(a->flags&(1u<<AMP_SSHIELD)) && ngens!=4){u16 r=avp_random();if(ngens==3)r&=7;else if(ngens==2)r&=3;else if(ngens==1)r&=1;if(r==0)a->animseq=AS_DEATH;}
    ++a->animframe;
    if(!ngens){a->mode=NULL;a->host_static=0;amp_cleargrid_c(a);}
}
void side_shield(AvpAmp *a,s32 dx,s32 dy)
{
    if(a->flags&(1u<<AMP_DEADGEN)){dead_gen(a);return;}
    AvpAmp *s=make_shield(a,dx,dy);s->flags|=(1u<<AMP_SSHIELD);a->aux_ptr=s;++ngens;a->mode=genmode;amp_setgrid(a);
}
void genmode(AvpAmp *a)
{
    ++a->animframe;
    if(use_cocoon && ngens==1)return;
    if(a->energy>0)return;
    a->flags|=(1u<<AMP_DEADGEN);
    if(a->aux_ptr){AvpAmp*s=(AvpAmp*)a->aux_ptr;s->host_static=1;s->mode=NULL;s->animseq=AS_DEATH;}
    if(ngens) --ngens;
    dead_gen(a);
}
static void amp_generate_running(AvpAmp *a)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    if(!ngens){
        if(o->kill_ambient)o->kill_ambient(o->user);
        a->mode=NULL;a->host_static=0;
        return;
    }
    if(o->play_sfx_params){
        u32 dx=abs32diff(a->xpos,(s32)x_pos),dy=abs32diff(a->ypos,(s32)y_pos);
        u32 big=dx>dy?dx:dy,small=dx>dy?dy:dx,d=big+(small>>2);
        s32 missing=(s32)(4-ngens);
        s32 vol=(s32)0x1a00-(s32)(d>>7)-(missing<<8);
        s32 pitch=(s32)0xe0-(s32)(d>>16)-(missing<<3);
        if(vol<0)vol=0;
        if(pitch<0x80)pitch=0x80;
        o->play_sfx_params(o->user,26,vol,pitch);
    }
}
void generate(AvpAmp *a)
{
    /* AMP.S starts the force-field alarm and deliberately returns for one
     * frame before .xgen2 begins attenuation/pitch maintenance. */
    const AvpRuntimeOps *o=avp_runtime_ops();
    if(o->play_sfx_params)o->play_sfx_params(o->user,26,(s32)0x80000000u,0);
    else if(o->play_sfx)o->play_sfx(o->user,26);
    a->mode=amp_generate_running;
}
static void amp_side_n(AvpAmp *a){if(player_type!=PT_ALIEN){a->mode=NULL;a->host_static=0;return;}side_shield(a,0,(0x1a800-(0xf0<<8))/2);}
static void amp_side_s(AvpAmp *a){if(player_type!=PT_ALIEN){a->mode=NULL;a->host_static=0;return;}side_shield(a,0,-((0x1a800-(0xf0<<8))/2));}
static void amp_side_e(AvpAmp *a){if(player_type!=PT_ALIEN){a->mode=NULL;a->host_static=0;return;}side_shield(a,-((0x1a800-(0xf0<<8))/2),0);}
static void amp_side_w(AvpAmp *a){if(player_type!=PT_ALIEN){a->mode=NULL;a->host_static=0;return;}side_shield(a,(0x1a800-(0xf0<<8))/2,0);}
void cgenmode(AvpAmp *a)
{
    if(player_type!=PT_ALIEN){a->mode=NULL;a->host_static=0;return;}
    const s32 D=0x1a800, R=0xf0;
    const s32 dxy[4][2]={{0,-D},{0,D},{D,0},{-D,0}};
    AvpAmpModeFn sides[4]={amp_side_n,amp_side_s,amp_side_e,amp_side_w};
    for(unsigned i=0;i<4;i++){AvpAmp*g=amp_req();g->xpos=a->xpos+dxy[i][0];g->ypos=a->ypos+dxy[i][1];g->creature=AC_GEN;g->animseq=AS_STAND;g->energy=200;g->flags=(1u<<AMP_KILLABLE);g->astype=(s16)(44u+i);g->mode=sides[i];}
    for(unsigned i=0;i<12;i++){u16 ang=(u16)((0x10000u/12u)*i);s32 dx=(cos_d0(ang)*(s32)R)>>7; s32 dy=(sin_d0(ang)*(s32)R)>>7;AvpAmp*s=make_shield(a,dx,dy);s->astype=49;}
    a->animseq=AS_HIDDEN;a->astype=43;a->mode=generate;
}

void UpdateAMPs(void)
{
    entime=(u16)((entime+1u)&7u);explo1=0;
    for(unsigned i=0;i<AVP_NUM_AMPS;i++){
        AvpAmp *a=&amp_data[i];cur_amp_no=(u16)(i+1u);
        if(a->host_static || !a->mode) continue;
        if(entime==0)a->oldenergy=a->energy;
        AvpAmpModeFn fn=a->mode;fn(a);
    }
    (void)cur_amp_no;
}

/* -------------------------------------------------------------------------
 * Source-block closure for AMP.S mobile-actor state machine.                 */
static void amp_cleargrid_c(AvpAmp *a);
static int amp_in_range(const AvpAmp *a,u16 pixels);
static void set_static(AvpAmp *a);

static void amp_sfx(unsigned id)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    if(o->play_sfx)o->play_sfx(o->user,id);
}

static int amp_los_clear(const AvpAmp *a)
{
    AvpXY from={a->xpos,a->ypos},to={(s32)x_pos,(s32)y_pos};
    return LineOfSight(&from,&to)==0;
}

static void amp_recover(AvpAmp *a);
static void amp_acid(AvpAmp *a);
static void amp_pred_dead(AvpAmp *a);
static void amp_alien_dead(AvpAmp *a);
static void amp_handalien(AvpAmp *a);
static void amp_alien_clfight(AvpAmp *a);
static void amp_crawlfight(AvpAmp *a);
static void amp_handpred(AvpAmp *a);
static void amp_pred_clfight(AvpAmp *a);
static void amp_closehug(AvpAmp *a);
static void amp_invisible(AvpAmp *a);
static void amp_invisible_frames(AvpAmp *a);
static void amp_pred_hold(AvpAmp *a);
static void amp_appear(AvpAmp *a);
static void amp_appear_frames(AvpAmp *a);
static void amp_roar(AvpAmp *a);
static void amp_disc_prepare(AvpAmp *a);
static void amp_laser(AvpAmp *a);
static void amp_launchfire(AvpAmp *a);
static void amp_marine_recoil(AvpAmp *a);
static void amp_crouch_fire(AvpAmp *a);
static void amp_fallhug(AvpAmp *a);
static void amp_used_egg(AvpAmp *a);
static void amp_fake_north(AvpAmp *a);
static void amp_fake_south(AvpAmp *a);
static void amp_fake_east(AvpAmp *a);
static void amp_fake_west(AvpAmp *a);

static const s16 death_pred[]={0,1,2,3,4,-1};
static const s16 death_human[]={0,1,2,-1};
static const s16 death_hug[]={0,1,2,3,2,3,2,3,2,3,2,3,2,3,2,3,2,3,2,3,2,3,2,3,-1};
static const s16 death_alien[]={0,1,2,3,-1};
static const s16 death_crawl[]={2,3,-1};
static const s16 roar_frames[]={0,1,2,3,2,4,2,1,1,0,0,0,-1};
static const s16 invis_frames[]={0,1,1,1,1,1,1,1,1,-2,1,1,1,2,3,4,5,6,-1};
static const s16 appear_frames[]={6,5,4,3,2,0,-1};
static const s16 laser_frames[]={0,0,1,1,1,2,2,2,-2,1,1,-1};
static const s16 crouch_frames[]={0,1,2,2,3,2,1,0,-1};

typedef struct AvpFightFrame { s16 frame,damage; u8 event; } AvpFightFrame;
static const AvpFightFrame fight_bite[]={ {0,0,0},{1,0,0},{2,0,0},{2,100,1},{1,0,0},{0,0,0},{-1,0,0} };
static const AvpFightFrame fight_tail[]={ {0,0,0},{1,0,0},{2,0,0},{3,0,0},{3,20,1},{2,0,0},{1,0,0},{0,0,0},{-1,0,0} };
static const AvpFightFrame fight_pred_combi[]={ {0,0,0},{0,0,0},{0,0,0},{1,0,0},{2,0,0},{3,0,0},{3,0,0},{3,0,0},{4,100,1},{1,0,0},{-1,0,0} };
static const AvpFightFrame fight_pred_knife[]={ {0,0,0},{1,0,0},{2,0,1},{3,0,0},{4,0,0},{5,50,1},{-1,0,0} };
static const AvpFightFrame fight_swipe[]={ {1,0,0},{2,30,1},{3,0,0},{3,0,0},{2,0,0},{1,0,0},{-1,0,0} };
static const AvpFightFrame fight_crawl[]={ {0,0,0},{1,0,0},{2,0,0},{3,0,0},{3,100,1},{2,0,0},{1,0,0},{0,0,0},{-1,0,0} };
static const AvpFightFrame *const fight_playlists[]={fight_bite,fight_tail,fight_pred_combi,fight_pred_knife,fight_swipe};
static const s16 alien_all[]={0,1,2,0,1,1,2,1,0,2,1,0,0,1,0,2,1,0,2,2,1,0,-1};
static const s16 alien_bite_only[]={0,-1};
static const s16 alien_swipe_only[]={1,-1};
static const s16 alien_tail_only[]={2,-1};
static const s16 *const alien_sequences[]={alien_all,alien_bite_only,alien_swipe_only,alien_tail_only};

static void amp_begin_death(AvpAmp *a)
{
    const s16 *seq=NULL;
    a->flags|=(u16)(1u<<AMP_DEAD);
    amp_cleargrid_c(a);
    switch(a->creature){
    case AC_PREDATOR: seq=death_pred; a->astype=54; amp_sfx(40); break; /* DEF_DPRED */
    case AC_HUMAN: seq=death_human; a->astype=53; HumanPain(); break; /* PLAYER.S::HumanPain */
    case AC_ACRAWL: a->creature=AC_ALIEN; seq=death_crawl; a->astype=52; amp_sfx(42); break;
    case AC_ALIEN: seq=death_alien; a->astype=52; amp_sfx(42); break;
    case AC_FACEHUG: seq=death_hug; a->yoffset=0; a->astype=55; break;
    default:
        a->flags&=(u16)~(1u<<AMP_DEAD);
        amp_setgrid(a);
        return;
    }
    if(a->creature==AC_ALIEN && amp_in_range(a,96))player_energy=(s16)(player_energy-50);
    a->flags&=(u16)~((1u<<AMP_KILLABLE)|(1u<<AMP_MOTION));
    a->aux_ptr=(void*)seq;
    a->timer=0;
    a->animseq=AS_DEATH;
    if(a->creature==AC_ALIEN)a->mode=amp_alien_dead;
    else { a->flags|=(u16)(1u<<AMP_ALTFRAME); a->mode=amp_pred_dead; }
    a->mode(a);
}

static void amp_finish_death(AvpAmp *a)
{
    do_score(a); amp_cleargrid_c(a);
    a->flags&=(u16)~((1u<<AMP_KILLABLE)|(1u<<AMP_MOTION));
    if(a->creature==AC_ALIEN){a->mode=amp_acid;return;}
    set_static(a);
}

static void amp_alien_dead(AvpAmp *a)
{
    const s16 *p=(const s16*)a->aux_ptr; s16 f;
    if(!p){amp_finish_death(a);return;}
    f=p[(unsigned)a->timer];
    if(f<0){amp_finish_death(a);return;}
    ++a->timer;
    if(f==60){a->animframe=0;a->animseq=AS_HIDDEN;return;}
    a->animframe=(u16)f;
}

static void amp_pred_dead(AvpAmp *a)
{
    /* 68000 BCHG/BNE advances on the old bit == 1 half-cycles. */
    int old=(a->flags&(1u<<AMP_ALTFRAME))!=0;
    a->flags^=(u16)(1u<<AMP_ALTFRAME);
    if(old)amp_alien_dead(a);
}

static void amp_acid(AvpAmp *a)
{
    if(amp_in_range(a,30)){
        if(!(a->flags&(1u<<AMP_ALTFRAME)))amp_sfx(43);
        a->flags|=(u16)(1u<<AMP_ALTFRAME);
        player_energy=(s16)(player_energy-3);
    }else a->flags&=(u16)~(1u<<AMP_ALTFRAME);
}

static void amp_recoil(AvpAmp *a)
{
    if(a->creature==AC_HUMAN)HumanPain();
    else if(a->creature==AC_PREDATOR)amp_sfx(45);
    else amp_sfx(46);
    a->animseq=AS_KNOCKBACK;a->animframe=0;a->timer=2;a->mode=amp_recover;
}
static void amp_recover(AvpAmp *a)
{
    if(--a->timer!=0)return;
    a->mode=chase_player;a->animframe=0;a->animseq=AS_RUN;
}

static int amp_grid_nonzero(int x,int y)
{
    if(x<0||y<0||x>=(int)AVP_AMP_GRID_W||y>=(int)AVP_AMP_GRID_H)return 1;
    return collmap[(unsigned)y*AVP_AMP_GRID_W+(unsigned)x]!=0;
}

static int amp_neighbor_occupied(const AvpAmp *a,int dx,int dy)
{
    int x=(int)((u32)a->xpos>>16),y=(int)((u32)a->ypos>>16);
    u16 fx=(u16)a->xpos,fy=(u16)a->ypos;
    enum { SAFE_PIXELS=32<<9 };
    int near_x=0,near_y=0;
    if(a->creature==AC_AQUEEN)return 0;
    if(dx<0)near_x=fx<SAFE_PIXELS;
    else if(dx>0)near_x=fx>(u16)(0xffffu-SAFE_PIXELS);
    if(dy<0)near_y=fy<SAFE_PIXELS;
    else if(dy>0)near_y=fy>(u16)(0xffffu-SAFE_PIXELS);

    /* AMP.S diagonal helpers independently test each side if that fractional
     * edge is close, and only test the diagonal when both edges are close. */
    if(dx && near_x && amp_grid_nonzero(x+dx,y))return 1;
    if(dy && near_y && amp_grid_nonzero(x,y+dy))return 1;
    if(dx && dy && near_x && near_y && amp_grid_nonzero(x+dx,y+dy))return 1;
    return 0;
}

static void amp_dims(const AvpAmp *a,u8 *height,u16 *width)
{
    switch(a->creature){
    case AC_HUMAN:*height=68;*width=46;break;
    case AC_PREDATOR:*height=60;*width=46;break;
    case AC_ALIEN:*height=80;*width=46;break;
    case AC_ACRAWL:*height=80;*width=51;break;
    case AC_FACEHUG:*height=20;*width=46;break;
    default:*height=80;*width=46;break;
    }
}

static int amp_safe_move(s32 x,s32 y,u8 h,u16 w)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    return o->safe_pos ? o->safe_pos(o->user,x,y,(s16)h,w)==AVP_COLL_SAFE
                       : SafePos(h,x,y,w)==AVP_COLL_SAFE;
}

static s32 target_center(s32 p){return (s32)(((u32)p&0xffff0000u)|0x8000u);}

static void amp_random_move(AvpAmp *a,int tried_ew);
static void amp_move_toward(AvpAmp *a,s32 tx,s32 ty)
{
    enum { SFX_CHATC=57, SFX_ANYTIME1=58, SFX_CMON2=59 };
    u8 h;u16 w; s32 ox=a->xpos,oy=a->ypos,nx=ox,ny=oy;
    int dx=0,dy=0,tried_ew=0,moved=0;
    if(chase_cb)chase_cb(a,tx,ty); /* observer sees source chase_x/y */

    /* AMP.S::go performs Predator chatter before it recentres chase_x/y.
     * Outside five squares the chat latch is cleared; inside four squares a
     * Predator that has not spoken yet chooses chatc/anytime1/cmon2 and sets
     * AMP_CHATFLAG.  The sound IDs are portable backend tokens; the state
     * transition and random branch are the ordinary-68000 semantics. */
    if(a->creature==AC_PREDATOR){
        if(!amp_in_range(a,5u*128u)){
            a->flags&=(u16)~(1u<<AMP_CHATFLAG);
        }else if(amp_in_range(a,4u*128u) && !(a->flags&(1u<<AMP_CHATFLAG))){
            u32 r=avp_random()&3u;
            amp_sfx(r==2u?SFX_ANYTIME1:(r==3u?SFX_CMON2:SFX_CHATC));
            a->flags|=(u16)(1u<<AMP_CHATFLAG);
        }
    }

    tx=target_center(tx);ty=target_center(ty);
    if(a->creature==AC_HUMAN && amp_in_range(a,80)){
        a->animframe=0;a->animseq=AS_STAND;a->flags|=(u16)(1u<<AMP_PHIT);return;
    }
    amp_cleargrid_c(a);amp_dims(a,&h,&w);
    if(a->creature!=AC_PREDATOR || a->animseq!=AS_HIDDEN){
        if(a->animseq!=AS_RUN){a->animframe=0;a->animseq=AS_RUN;}
    }
    if(tx>ox){dx=1;tried_ew=1;}else if(tx<ox){dx=-1;tried_ew=1;}
    if(ty>oy)dy=1;else if(ty<oy)dy=-1;

    if(dx){
        if(!amp_neighbor_occupied(a,dx,0)){
            /* Source tries diagonal first when both axes differ, then falls back to X. */
            if(dy && !amp_neighbor_occupied(a,dx,dy)){
                nx=ox+(s32)dx*(s32)a->xvel;ny=oy+(s32)dy*(s32)a->yvel;
                if(amp_safe_move(nx,ny,h,w)){
                    if(dx>0&&nx>tx)nx=tx;
                    if(dx<0&&nx<tx)nx=tx;
                    if(dy>0&&ny>ty)ny=ty;
                    if(dy<0&&ny<ty)ny=ty;
                    a->xpos=nx;a->ypos=ny;a->ldir=(s16)(dx>0?(dy>0?7:1):(dy>0?5:3));moved=1;
                }
            }
            if(!moved){
                nx=ox+(s32)dx*(s32)a->xvel;ny=oy;
                if(amp_safe_move(nx,ny,h,w)){
                    if(dx>0&&nx>tx)nx=tx;
                    if(dx<0&&nx<tx)nx=tx;
                    a->xpos=nx;a->ldir=(s16)(dx>0?0:4);moved=1;
                }
            }
        }
        /* Failed X/diagonal: source falls through .chk_y and tries Y. */
        if(!moved && dy && !amp_neighbor_occupied(a,0,dy)){
            nx=ox;ny=oy+(s32)dy*(s32)a->yvel;
            if(amp_safe_move(nx,ny,h,w)){
                if(dy>0&&ny>ty)ny=ty;
                    if(dy<0&&ny<ty)ny=ty;
                a->ypos=ny;a->ldir=(s16)(dy>0?6:2);moved=1;
            }
        }
    }else if(dy && !amp_neighbor_occupied(a,0,dy)){
        ny=oy+(s32)dy*(s32)a->yvel;
        if(amp_safe_move(ox,ny,h,w)){
            if(dy>0&&ny>ty)ny=ty;
                    if(dy<0&&ny<ty)ny=ty;
            a->ypos=ny;a->ldir=(s16)(dy>0?6:2);moved=1;
        }
    }
    amp_setgrid(a);
    if(!moved){
        if(a->mode!=lostplayer && a->mode!=amp_fallhug && a->creature!=AC_AQUEEN && a->yvector==0)
            amp_random_move(a,tried_ew);
        if(a->animseq!=AS_HIDDEN){a->animframe=2;a->animseq=AS_RUN;}
    }else if(a->animseq!=AS_HIDDEN)++a->animframe;
    /* ror.w #3 turns 0..7 direction IDs into renderer angle octants. */
    a->angle=(u16)(((u16)a->ldir>>3)|((u16)a->ldir<<13));
}

static void amp_fake_common(AvpAmp *a,int dx,int dy,AvpAmpModeFn self)
{
    s32 tx=(s32)x_pos,ty=(s32)y_pos;
    if(--a->yvector==0){a->mode=chase_player;}
    else {
        a->mode=self;
        if(dx<0)tx-=0x10000;else if(dx>0)tx+=0x10000;
        if(dy<0)ty-=0x10000;else if(dy>0)ty+=0x10000;
    }
    amp_chase_target(a,tx,ty,1);
}
static void amp_fake_north(AvpAmp *a){amp_fake_common(a,0,-1,amp_fake_north);}
static void amp_fake_south(AvpAmp *a){amp_fake_common(a,0,1,amp_fake_south);}
static void amp_fake_east(AvpAmp *a){amp_fake_common(a,1,0,amp_fake_east);}
static void amp_fake_west(AvpAmp *a){amp_fake_common(a,-1,0,amp_fake_west);}
static void amp_random_move(AvpAmp *a,int tried_ew)
{
    a->yvector=12;
    if(tried_ew){if(avp_random()&1u){a->mode=amp_fake_south;amp_fake_south(a);}else{a->mode=amp_fake_north;amp_fake_north(a);}}
    else {if(avp_random()&1u){a->mode=amp_fake_west;amp_fake_west(a);}else{a->mode=amp_fake_east;amp_fake_east(a);}}
}

static s32 amp_projectile_component(s32 v,u16 speed)
{
    s32 p=(s32)(s16)v*(s32)(s16)(speed<<8);u32 q=(u32)p<<3;return (s32)(s16)(q>>16);
}
static AvpAmp *amp_spawn_npc_projectile(AvpAmp *owner,u16 kind,u16 speed,s16 yoff,s16 delay,int eighth_step)
{
    AvpXY from={owner->xpos,owner->ypos},to={(s32)x_pos,(s32)y_pos};s32 vx,vy;AvpAmp *p=amp_req();
    Vector(&from,&to,&vx,&vy);vx=amp_projectile_component(vx,speed);vy=amp_projectile_component(vy,speed);
    p->xpos=owner->xpos;p->ypos=owner->ypos;p->creature=kind;p->animseq=AS_STAND;p->animframe=0;p->yoffset=yoff;
    p->xvel=(s16)amp_number(owner);p->yvel=(s16)avp_reset_counter;p->mode=(kind==AC_SMART)?pre_discmove:pre_flame_on;p->timer=delay;
    p->xvector=vx;p->yvector=vy;p->flags=0;p->host_static=0;
    p->xpos+=eighth_step?(vx>>3):vx;p->ypos+=eighth_step?(vy>>3):vy;discflag=1;return p;
}

static void amp_disc_prepare(AvpAmp *a)
{
    a->mode=amp_disc_prepare;a->animseq=AS_FIGHT4;
    if(a->animframe==0){a->animframe=1;return;}
    ++a->animframe;
    if(a->animframe==3){amp_sfx(47);(void)amp_spawn_npc_projectile(a,AC_SMART,30,0,0,1);return;}
    if(a->animframe==5){a->mode=chase_player;a->animseq=AS_RUN;a->animframe=0;}
}

static void amp_laser(AvpAmp *a)
{
    const s16 *p=laser_frames;unsigned i=(unsigned)a->timer; s16 f=p[i];
    a->mode=amp_laser;a->animseq=AS_FIGHT3;
    while(f==-2){amp_sfx(48);(void)amp_spawn_npc_projectile(a,AC_BOLT,24,-38,1,1);f=p[++i];}
    if(f<0){a->animframe=0;a->animseq=AS_STAND;a->mode=chase_player;a->timer=0;return;}
    a->animframe=(u16)f;a->timer=(s32)(i+1u);
}

static void amp_marine_recoil(AvpAmp *a)
{
    if(--a->animframe!=0)return;
    a->animseq=AS_STAND;a->mode=chase_player;a->animframe=0;
}
static void amp_launchfire(AvpAmp *a)
{
    s32 vx,vy;AvpXY from={a->xpos,a->ypos},to={(s32)x_pos,(s32)y_pos};
    if(a->energy<a->oldenergy){amp_recoil(a);return;}
    a->mode=amp_launchfire;++a->animframe;if(a->animframe!=2)return;
    Vector(&from,&to,&vx,&vy);vx=amp_projectile_component(vx,40);vy=amp_projectile_component(vy,40);
    amp_sfx(49);
    for(s16 delay=1;delay<=4;delay++){
        AvpAmp *p=amp_req();p->xpos=a->xpos+vx;p->ypos=a->ypos+vy;p->creature=AC_FIRE;p->animseq=AS_STAND;p->animframe=0;p->yoffset=0;
        p->xvel=(s16)amp_number(a);p->yvel=(s16)avp_reset_counter;p->mode=pre_flame_on;p->timer=delay;p->xvector=vx;p->yvector=vy;p->flags=0;p->host_static=0;discflag=1;
    }
    a->mode=amp_marine_recoil;
}
static void amp_crouch_fire(AvpAmp *a)
{
    if(a->energy<a->oldenergy){amp_recoil(a);return;}
    if(crouch_frames[(unsigned)a->timer]<0){a->mode=chase_player;a->animframe=0;a->animseq=AS_STAND;player_energy=(s16)(player_energy-100);a->timer=0;return;}
    a->animframe=(u16)crouch_frames[(unsigned)a->timer++];
}

static void amp_roar(AvpAmp *a)
{
    s16 f=roar_frames[(unsigned)a->timer];a->mode=amp_roar;
    if(f<0){a->animframe=0;a->mode=chase_player;a->animseq=AS_STAND;a->timer=0;return;}
    a->animframe=(u16)f;++a->timer;
}

static void amp_invisible(AvpAmp *a)
{
    a->animseq=AS_FIGHT1;a->animframe=0;a->timer=0;a->mode=amp_invisible_frames;amp_invisible_frames(a);
}
static void amp_invisible_frames(AvpAmp *a)
{
    s16 f=invis_frames[(unsigned)a->timer++];
    while(f==-2){amp_sfx(50);f=invis_frames[(unsigned)a->timer++];}
    if(f<0){a->animseq=AS_HIDDEN;a->yvector=(s32)((avp_random()&63u)+20u);amp_cleargrid_c(a);a->mode=amp_pred_hold;return;}
    a->animframe=(u16)f;
}
static void amp_pred_hold(AvpAmp *a)
{
    if(--a->yvector==0){a->mode=chase_player;return;}
    amp_chase_target(a,0,0,1);
}
static void amp_appear(AvpAmp *a)
{
    amp_sfx(51);a->timer=0;a->mode=amp_appear_frames;amp_appear_frames(a);
}
static void amp_appear_frames(AvpAmp *a)
{
    s16 f; amp_setgrid(a);a->animseq=AS_FIGHT1;f=appear_frames[(unsigned)a->timer++];
    if(f<0){a->mode=chase_player;a->animframe=0;a->animseq=AS_RUN;a->timer=0;return;}a->animframe=(u16)f;
}

static void amp_apply_fight_frame(AvpAmp *a,const AvpFightFrame *seq,u16 range,AvpAmpModeFn back)
{
    const AvpFightFrame *f=&seq[(unsigned)a->ldir];
    if(f->frame<0){a->animseq=AS_STAND;a->animframe=0;a->mode=back;a->timer=0;a->ldir=0;return;}
    a->animframe=(u16)f->frame;if(f->event && amp_in_range(a,range)){player_energy=(s16)(player_energy-f->damage);amp_sfx(52);}++a->ldir;
}
static void amp_handalien(AvpAmp *a)
{
    const s16 *sel;unsigned family,idx,act;
    a->animseq=AS_STAND;a->animframe=0;a->mode=amp_handalien;
    if(++a->timer!=5){if(a->energy<=0){a->energy=0;amp_begin_death(a);return;}if(a->energy<a->oldenergy){amp_recoil(a);return;}if(!amp_in_range(a,65)){a->mode=chase_player;a->animseq=AS_RUN;}return;}
    a->timer=0;family=(unsigned)(u32)a->xvector;if(family>3)family=0;sel=alien_sequences[family];idx=(unsigned)a->angle;while(sel[idx]<0){idx=0;a->angle=0;}act=(unsigned)sel[idx];a->angle=(u16)(idx+1u);
    if(act==0){a->animseq=AS_FIGHT1;a->aux_ptr=(void*)fight_bite;}
    else if(act==1){a->animseq=AS_FIGHT2;a->aux_ptr=(void*)fight_swipe;}
    else {a->animseq=AS_FIGHT3;a->aux_ptr=(void*)fight_tail;}
    a->ldir=0;a->mode=amp_alien_clfight;
}
static void amp_alien_clfight(AvpAmp *a)
{
    if(a->energy<a->oldenergy){amp_recoil(a);return;}if(a->energy<=0){a->energy=0;amp_begin_death(a);return;}
    amp_apply_fight_frame(a,(const AvpFightFrame*)a->aux_ptr,65,amp_handalien);
}
static void amp_crawlfight(AvpAmp *a)
{
    a->mode=amp_crawlfight;if(a->energy<=0){a->energy=0;amp_begin_death(a);return;}
    amp_apply_fight_frame(a,fight_crawl,70,chase_player);if(a->mode==chase_player)a->timer=10;
}
static void amp_handpred(AvpAmp *a)
{
    unsigned act;
    a->animseq=AS_STAND;a->animframe=0;a->mode=amp_handpred;
    if(++a->timer!=5){if(a->energy<=0){a->energy=0;amp_begin_death(a);return;}if(a->energy<a->oldenergy){amp_recoil(a);return;}if(!amp_in_range(a,77)){a->mode=chase_player;a->animseq=AS_RUN;a->animframe=0;}return;}
    a->timer=0;act=avp_random()&1u;a->aux_ptr=(void*)fight_playlists[act?3:2];a->animseq=act?AS_FIGHT6:AS_FIGHT5;a->ldir=0;a->mode=amp_pred_clfight;
}
static void amp_pred_clfight(AvpAmp *a)
{
    if(a->energy<=0){a->energy=0;amp_begin_death(a);return;}amp_apply_fight_frame(a,(const AvpFightFrame*)a->aux_ptr,57,amp_handpred);if(a->mode==amp_handpred)a->angle=0;
}
static void amp_closehug(AvpAmp *a)
{
    a->mode=amp_closehug;++a->animframe;if(a->animframe<=3)return;hugkill=1;amp_cleargrid_c(a);release_amp(a);
}

static void amp_stunned_start(AvpAmp *a)
{
    if(a->energy<=0){a->animseq=AS_RUN;a->animframe=0;a->mode=chase_player;a->flags&=(u16)~(1u<<AMP_STUN);return;}
    a->timer=(s32)(50-a->energy);a->animseq=AS_KNOCKBACK;a->animframe=0;a->mode=stun_mode;
}

static void amp_fallhug(AvpAmp *a)
{
    a->yoffset=(s16)(a->yoffset+10);if(a->yoffset==0){a->mode=chase_player;return;}chase_player(a);
}
static void amp_used_egg(AvpAmp *a)
{
    amp_setgrid(a);if(a->energy<=0){a->energy=0;make_dead_egg(a);}
}

static int amp_ranged_predator(AvpAmp *a)
{
    u16 r;
    if(a->animseq==AS_HIDDEN||discflag)return 0;
    r=avp_random();
    if((r&31u)==2u && amp_los_clear(a)){
        a->mode=amp_roar;a->animseq=AS_FIGHT2;a->timer=0;amp_sfx(53);amp_roar(a);return 1;
    }
    if(!amp_in_range(a,576))return 0;
    if((avp_random()&7u)!=2u||!amp_los_clear(a))return 0;
    r=(u16)(avp_random()&3u);
    if(r==0){a->timer=0;amp_laser(a);}
    else if(r==1)amp_invisible(a);
    else {amp_sfx(47);a->mode=amp_disc_prepare;a->animseq=AS_FIGHT4;a->animframe=0;}
    return 1;
}

static int amp_ranged_human(AvpAmp *a)
{
    if(!(a->flags&(1u<<AMP_PHIT)) && invisflag)return 0;
    if(discflag)return 0;
    if(!amp_in_range(a,576)){a->flags&=(u16)~(1u<<AMP_PHIT);return 0;}
    if((beam_counter&7u)!=2u)return 0;
    if(!amp_los_clear(a)){a->flags&=(u16)~(1u<<AMP_PHIT);return 0;}
    a->animframe=0;
    if(beam_counter&1u){a->animseq=AS_FIGHT2;amp_launchfire(a);}
    else{a->animseq=AS_FIGHT1;a->timer=0;a->mode=amp_crouch_fire;amp_sfx(54);amp_crouch_fire(a);}
    return 1;
}

static void amp_chase_target(AvpAmp *a,s32 tx,s32 ty,int allow_combat)
{
    u16 close=60;
    if(!a)return;
    a->flags|=(u16)((1u<<AMP_KILLABLE)|(1u<<AMP_MOTION));
    if(a->creature==AC_AQUEEN){amp_move_toward(a,tx,ty);return;}
    if(a->creature==AC_HUMAN && (a->flags&(1u<<AMP_STUN))){amp_stunned_start(a);return;}
    if(a->energy<=0){a->energy=0;amp_begin_death(a);return;}
    if(!amp_in_range(a,1152))return;
    if(a->creature==AC_HUMAN){a->timer=(s32)(((u16)a->timer+1u)&1u);if((u16)a->timer){if(a->energy<a->oldenergy)amp_recoil(a);return;}}
    if((a->creature==AC_PREDATOR||a->creature==AC_HUMAN||a->creature==AC_ACRAWL||a->creature==AC_ALIEN) && a->energy<a->oldenergy){amp_recoil(a);return;}
    if(allow_combat && a->creature==AC_HUMAN && !(a->flags&(1u<<AMP_PHIT)) && invisflag){a->mode=lostplayer;a->xvector=0;lostplayer(a);return;}
    if(a->creature==AC_ACRAWL)close=70;else if(a->creature==AC_PREDATOR)close=57;
    if(allow_combat && amp_in_range(a,close) && amp_los_clear(a)){
        if(a->creature==AC_ACRAWL){if((u16)a->timer){--a->timer;if((u16)a->timer)return;}a->ldir=0;a->animseq=AS_FIGHT1;amp_sfx(55);amp_crawlfight(a);return;}
        if(a->creature==AC_ALIEN){a->timer=4;a->angle=0;amp_sfx(55);amp_handalien(a);return;}
        if(a->creature==AC_PREDATOR){if(a->animseq==AS_HIDDEN){a->timer=0;amp_appear(a);return;}a->timer=0;a->angle=0;amp_handpred(a);return;}
        if(a->creature==AC_FACEHUG){a->animseq=AS_FIGHT1;a->animframe=0;amp_sfx(56);amp_closehug(a);return;}
    }
    if(allow_combat){
        if(a->creature==AC_PREDATOR && amp_ranged_predator(a))return;
        if(a->creature==AC_HUMAN && amp_ranged_human(a))return;
    }
    amp_move_toward(a,tx,ty);
}

void chase_player(AvpAmp *a)
{
    amp_chase_target(a,(s32)x_pos,(s32)y_pos,1);
}

/* -------------------------------------------------------------------------
 * Routine-level source lifts retained under the historical AMP.S names.
 * These are deliberately explicit rather than folded into surrounding modes
 * so the readable-C tree can be audited one historical routine at a time.  */

void QFRAME(AvpAmp *a,u16 frame)
{
    if(!a)return;
    a->animframe=frame;
}

void do_score(AvpAmp *a)
{
    static const s32 score_list[]={
        10000,50000,5000,300,900,6000,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        100,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,200000,50000,0
    };
    s32 add;
    u16 flags,creature;

    if(!a)return;
    flags=a->flags;
    if(!(flags&(1u<<AMP_PHIT)))return; /* no killy, no scorey */

    creature=a->creature;
    if(creature >= (u16)(sizeof(score_list)/sizeof(score_list[0])))return;
    add=score_list[creature];

    if(player_type==PT_ALIEN){
        if(creature==AC_HUMAN)add=(s32)((u32)add<<2);
    }else if(player_type==PT_PREDATOR){
        if(flags&(1u<<AMP_INVHIT)){
            add=-add;
            if(!(flags&(1u<<AMP_CLOSEHIT)))add=(s32)((u32)add<<1);
        }else if(!(flags&(1u<<AMP_CLOSEHIT))){
            /* 68000 ASR.L #1: arithmetic divide by two with sign preserved. */
            add >>= 1;
        }
    }

    score += add;
    if(score<0)score=0;
}

void amp_setgrid(AvpAmp *a)
{
    unsigned x,y,n;
    if(!a)return;

    /* AMP.S indexes collmap using the integer words of amp_xpos/amp_ypos.
     * In the host-readable representation those are the upper 16 bits of the
     * 16.16 positions.  Each cell is a 16-bit AMP number, zero meaning free. */
    x=(unsigned)((u32)a->xpos>>16);
    y=(unsigned)((u32)a->ypos>>16);
    if(x>=AVP_AMP_GRID_W || y>=AVP_AMP_GRID_H)return;

    n=amp_number(a);
    if(n==0 || n>0xffffu)return;
    collmap[y*AVP_AMP_GRID_W+x]=(u16)n;
}


static void write_obj_grid(AvpAmp *a);

/* Historical primary-template mode entries.  The secondary numeric template
 * payload stays resource-bindable, while every CPU-side mode is executable C. */
static void amp_collectable_mode(AvpAmp *a){write_obj_grid(a);set_static(a);}
static void amp_furniture_mode(AvpAmp *a){amp_setgrid(a);set_static(a);}
static void amp_start_hug_mode(AvpAmp *a)
{
    if(player_type==PT_ALIEN){a->mode=NULL;a->host_static=0;return;}
    a->mode=chase_player;
}
static void amp_drum_live_mode(AvpAmp *a)
{
    AvpAmp *e;
    if(a->energy>0)return;
    e=explosion_ex(a->xpos,a->ypos,0,200,0x10000u);
    e->flags=(u16)(a->flags&((1u<<AMP_CLOSEHIT)|(1u<<AMP_PHIT)|(1u<<AMP_INVHIT)));
    e->creature=AC_DRUM;e->xvel=4;e->yvel=6;
    do_score(a);amp_cleargrid_c(a);release_amp(a);
}
static void amp_drum_start_mode(AvpAmp *a){amp_setgrid(a);a->mode=amp_drum_live_mode;amp_drum_live_mode(a);}
static void amp_smart_activate_mode(AvpAmp *a)
{
    if(cur_weps&(1u<<4)){a->timer=0;a->mode=amp_appear;amp_appear(a);}
}
static void amp_queen_start_mode(AvpAmp *a)
{
    if(((u32)x_pos>>16)==0x16u && ((u32)y_pos>>16)==0x0du && (u16)y_pos>0xc000u)lockxxx(a);
}
static void amp_shield_start_mode(AvpAmp *a){amp_setgrid(a);a->mode=shield2;shield2(a);}
static void end_queen_init_mode(AvpAmp *a);
static void queen_regen_mode(AvpAmp *a);

static int avp_amp_builtin_static_for_def(unsigned d)
{
    switch(d){case 17:case 53:case 54:case 55:case 56:case 57:return 1;default:return 0;}
}

AvpAmpModeFn avp_amp_builtin_mode_for_def(unsigned d)
{
    switch(d){
    case 0:case 1:case 2:case 5:case 29:case 30:case 31:case 32:case 33:case 34:
    case 35:case 36:case 37:return chase_player;
    case 3:return eggwait;
    case 4:return amp_start_hug_mode;
    case 11:case 12:case 14:case 15:case 16:case 19:case 21:case 23:case 24:
    case 25:case 38:case 39:case 40:return amp_collectable_mode;
    case 13:case 26:case 27:case 28:case 41:case 42:return amp_furniture_mode;
    case 17:case 53:case 54:case 55:case 56:case 57:return NULL; /* static=-1 is host_static */
    case 18:return cocoon;
    case 20:case 22:return amp_drum_start_mode;
    case 43:return generate;
    case 44:return amp_side_n;case 45:return amp_side_s;case 46:return amp_side_e;case 47:return amp_side_w;
    case 48:return cgenmode;
    case 49:return amp_shield_start_mode;
    case 50:return amp_queen_start_mode;
    case 51:return amp_smart_activate_mode;
    case 52:return amp_acid;
    case 58:return end_queen_init_mode;
    case 59:return queen_regen_mode;
    default:return NULL;
    }
}

/* -------------------------------------------------------------------------
 * Final AMP.S routine-closure tranche. Authored placement/template payloads
 * are bound by the resource layer; this file owns the 68000 control semantics. */
#include "collision.h"
#include "levels.h"
#include "hud.h"
#include "collision.h"
#include "mazescrn.h"
#include "weapons.h"
#include "doors.h"
#include "player.h"

static const AvpAmpPlacement *level_lists[4][AVP_MAX_LEVEL];
static const AvpAmpTemplate *bound_templates;
static unsigned bound_template_count;
static void (*fight_cb)(AvpAmp *,int);
static void (*death_cb)(AvpAmp *);
static int (*lift_test_cb)(const AvpAmp *);

void avp_amp_bind_level_list(unsigned k,unsigned level,const AvpAmpPlacement *list){if(k<4&&level>=1&&level<=AVP_MAX_LEVEL)level_lists[k][level-1]=list;}
void avp_amp_bind_random_list(unsigned k,unsigned level,const AvpAmpRandomEntry *list){if(k<3&&level>=1&&level<=AVP_MAX_LEVEL)random_lists[k][level-1]=list;}
void avp_amp_bind_templates(const AvpAmpTemplate *t,unsigned n){bound_templates=t;bound_template_count=n;}
void avp_amp_set_beam_counter(u32 v){beam_counter=v;}
void avp_amp_set_fight_callback(void (*fn)(AvpAmp *,int)){fight_cb=fn;}
void avp_amp_set_death_callback(void (*fn)(AvpAmp *)){death_cb=fn;}
void avp_amp_set_lift_test_callback(int (*fn)(const AvpAmp *)){lift_test_cb=fn;}

static void amp_cleargrid_c(AvpAmp *a){unsigned x=(u32)a->xpos>>16,y=(u32)a->ypos>>16;if(x<64&&y<64)collmap[y*64+x]=0;}
static int amp_in_range(const AvpAmp *a,u16 pixels){u32 lim=(u32)pixels<<9;return abs32diff(a->xpos,(s32)x_pos)<=lim&&abs32diff(a->ypos,(s32)y_pos)<=lim;}
static void set_static(AvpAmp *a){a->mode=NULL;a->host_static=1;}
static void write_obj_grid(AvpAmp *a){unsigned x=(u32)a->xpos>>16,y=(u32)a->ypos>>16,n=amp_number(a);size_t o=y*AVP_AMP_OBJ_ROW_BYTES+x*2u;if(x<64&&y<64&&n&&o+1<sizeof(objmap)){objmap[o]=(u8)(n>>8);objmap[o+1]=(u8)n;}}
static void make_dead_egg(AvpAmp *a){amp_cleargrid_c(a);a->flags&=(u16)~(1u<<AMP_KILLABLE);a->animframe=5;a->astype=56;set_static(a);do_score(a);}

static void apply_template(AvpAmp *a,const AvpAmpPlacement *p){
    const AvpAmpTemplate *t;if(!bound_templates||p->def>=bound_template_count){set_static(a);return;}t=&bound_templates[p->def];
    a->xpos=(s32)((u32)(u16)p->x<<8);a->ypos=(s32)((u32)(u16)p->y<<8);a->mode=t->mode?t->mode:avp_amp_builtin_mode_for_def(p->def);a->creature=t->creature;
    a->animseq=t->animseq;a->animframe=t->animframe;a->angle=t->angle;a->timer=t->timer;a->xvel=t->xvel;a->yvel=t->yvel;a->ldir=t->ldir;
    a->xvector=t->xvector;a->yvector=t->yvector;a->energy=t->energy;a->oldenergy=t->oldenergy;a->flags=(u16)(t->flags|t->flags_or);a->yoffset=t->yoffset;a->astype=(s16)p->def;a->host_static=(u8)(t->host_static||avp_amp_builtin_static_for_def(p->def));a->aux_ptr=NULL;
    {unsigned x=(u32)a->xpos>>16,y=(u32)a->ypos>>16;if(x<64&&y<64)collmap[y*64+x]=1;}
    if(a->creature==AC_HUMAN||a->creature==AC_ALIEN){a->xpos=(s32)(((u32)a->xpos&0xffff0000u)|0x8000u);a->ypos=(s32)(((u32)a->ypos&0xffff0000u)|0x8000u);}
}
static AvpAmp *make_template_at(u16 def,s32 xpos,s32 ypos,int occupy)
{
    AvpAmp *a=amp_req();
    AvpAmpPlacement p={(s16)((u32)xpos>>8),(s16)((u32)ypos>>8),def};
    apply_template(a,&p);
    a->xpos=xpos;a->ypos=ypos;
    if(occupy){unsigned x=(u32)xpos>>16,y=(u32)ypos>>16;if(x<64u&&y<64u)collmap[y*64u+x]=1;}
    ++ampcount;
    return a;
}

void next_creature(const AvpAmpPlacement **cursor)
{
    const AvpAmpPlacement *p;
    if(!cursor||!(p=*cursor))return;
    while(p->x!=-1){AvpAmp*a=amp_req();apply_template(a,p);++p;}
    *cursor=p;
}

void level_loop(void)
{
    const AvpAmpPlacement *p;
    if(cur_level<1||cur_level>AVP_MAX_LEVEL)return;
    levels_visit|=(u16)(1u<<((unsigned)cur_level-1u));
    p=level_lists[AVP_AMP_LIST_COMMON][cur_level-1];
    next_creature(&p);
}

void append_objs(void)
{
    unsigned a,b;const AvpAmpPlacement*p;
    if(cur_level<1||cur_level>AVP_MAX_LEVEL)return;
    if(player_type==PT_ALIEN){a=AVP_AMP_LIST_HUMAN;b=AVP_AMP_LIST_PREDATOR;}
    else if(player_type==PT_PREDATOR){a=AVP_AMP_LIST_ALIEN;b=AVP_AMP_LIST_HUMAN;}
    else{a=AVP_AMP_LIST_PREDATOR;b=AVP_AMP_LIST_ALIEN;}
    p=level_lists[a][cur_level-1];next_creature(&p);
    p=level_lists[b][cur_level-1];next_creature(&p);
}

void rebuild_level(void)
{
    const AvpAmpSavedPlacement *p;
    reset_free_list();
    if(cur_level<1||cur_level>AVP_MAX_LEVEL)return;
    p=saved_levels[cur_level-1];
    while(p->x!=-1){
        AvpAmp *a=amp_req();
        if(p->astype>=0&&(unsigned)p->astype<bound_template_count){
            AvpAmpPlacement q={p->x,p->y,(u16)p->astype};
            apply_template(a,&q);
            /* Rebuild restores the saved flagword exactly, not template ORs. */
            a->flags=p->flags;
            if(a->flags&(1u<<AMP_COLLECT))set_static(a);
        }else set_static(a);
        ++p;
    }
}

void xcocoons(void)
{
    enum { DEF_COCOON=18 };
    if(player_type!=PT_ALIEN)return;
    for(unsigned i=0;i<AVP_MAX_COCOONS;i++){
        const AvpCocoonState *c=&cocoon_data[i];
        if(c->frame==AVP_COCOON_EMPTY||c->level!=cur_level)continue;
        (void)make_template_at(DEF_COCOON,(s32)c->x,(s32)c->y,1);
    }
    if(ccn_xsave)(void)make_template_at(DEF_COCOON,(s32)ccn_xsave,(s32)ccn_ysave,1);
}

void rand_set(void)
{
    unsigned pk;
    const AvpAmpRandomEntry *p;
    if(cur_level<1||cur_level>AVP_MAX_LEVEL)return;
    pk=(player_type==PT_ALIEN)?1u:(player_type==PT_PREDATOR)?2u:0u;
    p=random_lists[pk][cur_level-1];
    if(!p)return;
    while(p->def>=0){
        for(s16 n=0;n<p->count;n++){
            unsigned x,y,tries=0;
            do{x=avp_random()&63u;y=avp_random()&63u;if(++tries>65536u)return;}while(collmap[y*64u+x]!=0);
            (void)make_template_at((u16)p->def,(s32)((x<<16)|0x8000u),(s32)((y<<16)|0x8000u),1);
        }
        ++p;
    }
}

static s16 queen_respawns,queen_end_timer;

/* Special encounter constructors are local-label control flow in AMP.S and
 * therefore part of the ordinary-68000 closure even though they are not :: exports. */
static void end_queen_init_mode(AvpAmp *a);
static void end_queen_mode(AvpAmp *a);
static void end_queen_fight_mode(AvpAmp *a);
static void alien_gameover_mode(AvpAmp *a);
static void alien_free_queen_mode(AvpAmp *a);

void make_queen(void)
{
    enum { DEF_QUEEN=50 };
    AvpAmp *a;
    queen_respawns=0;
    a=make_template_at(DEF_QUEEN,0x180000,0x0e6400,0);
    a->timer=1;
}

void make_end_queen(void)
{
    enum { DEF_QUEEN=50 };
    AvpAmp *a=make_template_at(DEF_QUEEN,0x266b00,0x0fdf00,0);
    a->timer=5;a->mode=end_queen_init_mode;
}

void end_preds(void)
{
    enum { DEF_SMARTPRED=51 };
    static const s32 pos[4][2]={{0x2a7700,0x117700},{0x287700,0x0f7700},{0x2b7700,0x0e7700},{0x0e7700,0x157700}};
    for(unsigned i=0;i<4;i++)(void)make_template_at(DEF_SMARTPRED,pos[i][0],pos[i][1],0);
}

void gofight(AvpAmp *a)
{
    if(!a)return;
    if(discflag){amp_move_toward(a,(s32)x_pos,(s32)y_pos);return;}
    if(!amp_in_range(a,576)){
        a->flags&=(u16)~(1u<<AMP_PHIT);
        amp_move_toward(a,(s32)x_pos,(s32)y_pos);return;
    }
    if((beam_counter&7u)!=2u){amp_move_toward(a,(s32)x_pos,(s32)y_pos);return;}
    if(!amp_los_clear(a)){
        a->flags&=(u16)~(1u<<AMP_PHIT);
        amp_move_toward(a,(s32)x_pos,(s32)y_pos);return;
    }
    a->animframe=0;
    if(beam_counter&1u){
        a->animseq=AS_FIGHT2;
        if(fight_cb)fight_cb(a,AVP_AMP_FIGHT_FLAME);
        amp_launchfire(a);
    }else{
        a->animseq=AS_FIGHT1;a->timer=0;a->mode=amp_crouch_fire;
        if(fight_cb)fight_cb(a,AVP_AMP_FIGHT_CROUCH);
        amp_sfx(54);amp_crouch_fire(a);
    }
}

void stun_mode(AvpAmp *a)
{
    if(!a)return;
    if(a->timer==0){
        a->animseq=AS_RUN;a->animframe=0;a->mode=chase_player;
        a->flags&=(u16)~(1u<<AMP_STUN);return;
    }
    --a->timer;
    if(!(a->flags&(1u<<AMP_STUN))){a->energy=0;amp_begin_death(a);return;}
    if(a->energy>0)return;
    {
        int in_lift=avp_doors_cell_is_liftside(a->xpos,a->ypos);
        if(lift_test_cb)in_lift=lift_test_cb(a); /* test/port override */
        if(use_cocoon||in_lift){a->energy=0;amp_begin_death(a);return;}
    }
    amp_cleargrid_c(a);
    a->mode=stun_death;a->timer=50;a->animseq=AS_DEATH;a->animframe=0;
}

void stun_death(AvpAmp *a)
{
    if(!a)return;
    if(a->animframe!=2){++a->animframe;return;}
    if(--a->timer==0){set_static(a);return;}
    if(!amp_in_range(a,20))return;
    if(num_cocoons==AVP_MAX_COCOONS){
        set_static(a);a->flags=(1u<<AMP_DEAD);return;
    }
    set_static(a);a->creature=AC_COCOON;a->animseq=AS_STAND;a->animframe=0;
    a->astype=18;a->flags=0;MakeCocoon((u32)a->xpos,(u32)a->ypos);amp_sfx(27);
}

void cocoon(AvpAmp *a){if(!a)return;if((u16)(a->xpos>>16)!=(u16)(x_pos>>16)||(u16)(a->ypos>>16)!=(u16)(y_pos>>16)){write_obj_grid(a);set_static(a);return;}a->xpos=(s32)(((u32)a->xpos&0xffff0000u)|0x8000u);a->ypos=(s32)(((u32)a->ypos&0xffff0000u)|0x8000u);a->creature=AC_EGG;make_dead_egg(a);}

static void wait_hug_mode(AvpAmp *a);
static void egg_open2_mode(AvpAmp *a){if(a->energy<=0){a->energy=0;make_dead_egg(a);return;}if(++a->animframe==4){a->mode=wait_hug_mode;a->timer=0;}}
static void egg_open_mode(AvpAmp *a){const AvpRuntimeOps*o=avp_runtime_ops();if(o->play_sfx)o->play_sfx(o->user,28);a->mode=egg_open2_mode;egg_open2_mode(a);}
static void fire_hug_mode(AvpAmp *a){AvpAmp*h;if(!bound_templates||4u>=bound_template_count){set_static(a);return;}amp_sfx(56);h=amp_req();*h=(AvpAmp){0};h->xpos=a->xpos;h->ypos=a->ypos;{AvpAmpPlacement p={(s16)((u32)a->xpos>>8),(s16)((u32)a->ypos>>8),4};apply_template(h,&p);h->xpos=a->xpos;h->ypos=a->ypos;}h->yoffset=-30;h->mode=amp_fallhug;a->mode=amp_used_egg;}
static void wait_hug_mode(AvpAmp *a){if(!(a->flags&(1u<<AMP_EGGOPEN))){if(a->energy<=0){a->energy=0;make_dead_egg(a);return;}if((u16)a->timer!=20u){a->timer=(s32)((u16)a->timer+1u);return;}}a->mode=fire_hug_mode;}
void eggwait(AvpAmp *a){AvpXY from,to;if(!a||player_type==PT_ALIEN)return;a->flags&=(u16)~(1u<<AMP_EGGOPEN);if(a->energy<a->ldir){a->ldir=a->energy;a->flags|=(1u<<AMP_EGGOPEN);}else{amp_setgrid(a);if(a->energy<=0){a->energy=0;make_dead_egg(a);return;}if(!amp_in_range(a,128))return;}from.x=a->xpos;from.y=a->ypos;to.x=(s32)x_pos;to.y=(s32)y_pos;if(LineOfSight(&from,&to))return;a->mode=egg_open_mode;}

/* Queen state machine from AMP.S lockxxx/.lockx/qrunb/qfight through the
 * retail death/resurrection/end-game continuations.  The original qfight
 * tables interleave frame words, a $fe damage opcode, a damage word and an
 * SFX pointer.  The normalized host tables below retain the gameplay-visible
 * frame/damage ordering while sound dispatch remains behind fight_cb. */
enum { QEV_END=-1, QEV_DAMAGE=-2 };
static const s16 qseq_swipe[]={2,0,1,2,QEV_DAMAGE,50,3,QEV_END};
static const s16 qseq_bite[] ={0,1,2,QEV_DAMAGE,50,2,1,QEV_END};
static const s16 qseq_tail[] ={0,1,2,QEV_DAMAGE,50,2,1,0,QEV_END};
static const s16 *const qseqs[4]={qseq_swipe,qseq_bite,qseq_tail,qseq_swipe};
static const u16 qseq_anim[4]={AS_FIGHT2,AS_FIGHT1,AS_FIGHT3,AS_FIGHT2};


/* AMP.S endque/endqueen/eqfight/agameover/.freeq.  The retail end-Queen
 * tables use the same qfightab streams as the prison Queen.  $00fe is an
 * event record occupying 8 bytes (opcode, damage, long SFX pointer); this
 * gameplay-side translation skips the whole record and leaves audio mapping
 * to the runtime backend, just as the other source-derived fight streams do. */
static void end_queen_init_mode(AvpAmp *a)
{
    if(!a)return;
    a->mode=end_queen_mode;
    a->astype=58; /* DEF_EQUEEN */
    a->timer=5;
}
static void alien_gameover_mode(AvpAmp *a)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    if(!a)return;
    if(o->play_sfx)o->play_sfx(o->user,0); /* ashriek: backend remaps */
    queen_end_timer=45;
    a->mode=alien_free_queen_mode;
    key_lock=(s16)-1;
    a->animseq=AS_RUN;
    a->animframe=0;
}
static void alien_free_queen_mode(AvpAmp *a)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    if(!a)return;
    if(!amp_in_range(a,60))amp_move_toward(a,(s32)x_pos,(s32)y_pos);
    if(--queen_end_timer!=0)return;
    game_over=1;
    if(o->play_sfx)o->play_sfx(o->user,0); /* ashriek */
    set_static(a);
}
static void end_queen_mode(AvpAmp *a)
{
    unsigned i;
    if(!a)return;
    if(ngens==0){alien_gameover_mode(a);return;}
    --a->timer;
    if(a->timer>=0)return;
    i=(unsigned)(avp_random()&3u);
    a->animframe=(u16)qseqs[i][0];
    a->aux_ptr=(void *)(qseqs[i]+1);
    a->animseq=qseq_anim[i];
    a->mode=end_queen_fight_mode;
}
static void end_queen_fight_mode(AvpAmp *a)
{
    const s16 *q;
    if(!a)return;
    a->mode=end_queen_fight_mode; /* source rewrites this every update */
    q=(const s16 *)a->aux_ptr;
    if(!q){a->mode=end_queen_mode;a->timer=10;return;}
    for(;;){
        s16 t=*q++;
        if(t==QEV_DAMAGE){
            /* Retail eqfight sees $00FE before its BMI end-marker test and
             * skips the complete damage/SFX event without damaging the player,
             * then immediately consumes the following frame in this tick. */
            (void)*q++;
            continue;
        }
        if(t<0){
            a->aux_ptr=(void *)q;
            a->mode=end_queen_mode;
            a->animframe=0;
            a->animseq=AS_STAND;
            a->timer=10;
            return;
        }
        a->animframe=(u16)t;
        a->aux_ptr=(void *)q;
        return;
    }
}


static int queen_trigger(int retreat)
{
    u16 xi=(u16)((u32)x_pos>>16), yi=(u16)((u32)y_pos>>16);
    u16 yf=(u16)y_pos;
    if(xi!=0x16u || yi!=0x0du)return 0;
    return retreat ? (yf<=0xc000u) : (yf>0xc000u);
}
static void queen_dead_begin(AvpAmp *a);
static void queen_lock_mode(AvpAmp *a);
static void queen_reverse_mode(AvpAmp *a);
static void queen_fight_mode(AvpAmp *a);
static void queen_regen_mode(AvpAmp *a);
static void queen_wait_card_mode(AvpAmp *a);
static void queen_end_mode(AvpAmp *a);

static void queen_delay_mode(AvpAmp *a){if(--a->timer==0)lockxxx(a);}

static void queen_play_death_mode(AvpAmp *a)
{
    a->flags^=(u16)(1u<<AMP_ALTFRAME);
    /* 68000 BCHG sets Z from the old bit: BEQ when old bit was clear,
     * which is the state where the toggled bit is now set. */
    if(a->flags&(1u<<AMP_ALTFRAME))return;
    if(++a->animframe!=3)return;
    a->flags&=(u16)~(1u<<AMP_MOTION);
    a->astype=57; /* DEF_DQUEEN */
    if(player_type==PT_PREDATOR){
        key_lock=1;queen_end_timer=20;a->mode=queen_end_mode;return;
    }
    ++queen_respawns;
    if(queen_respawns==1){a->mode=queen_wait_card_mode;return;}
    set_static(a);
}
static void queen_dead_begin(AvpAmp *a)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    if(o->play_sfx)o->play_sfx(o->user,0); /* ahit: backend may remap */
    a->animseq=AS_DEATH;a->animframe=0;a->mode=queen_play_death_mode;
}
static void queen_end_mode(AvpAmp *a)
{
    if(--queen_end_timer!=0)return;
    set_static(a);game_over=1;
}
static void queen_wait_card_mode(AvpAmp *a)
{
    if(queen_respawns!=1){set_static(a);return;}
    if(acs_level!=10)return;
    a->astype=59; /* DEF_RQUEEN */
    a->mode=queen_regen_mode;
    queen_regen_mode(a);
}
static void queen_regen_mode(AvpAmp *a)
{
    a->flags|=(u16)(1u<<AMP_MOTION);
    amp_setgrid(a);
    a->flags^=(u16)(1u<<AMP_ALTFRAME);
    if(a->flags&(1u<<AMP_ALTFRAME))return;
    --a->animframe;
    if((s16)a->animframe>=0)return;
    lockxxx(a);a->energy=1000;a->oldenergy=1000;
}
static void queen_fight_mode(AvpAmp *a)
{
    const s16 *q=(const s16 *)a->aux_ptr;
    if(!q){lockxxx(a);return;}
    amp_setgrid(a);                 /* queenfight begins with amp_setgrid */
    a->mode=queen_fight_mode;       /* source rewrites mode every update */
    for(;;){
        s16 t=*q++;a->aux_ptr=(void *)q;
        if(t==QEV_END){lockxxx(a);return;}
        if(t==QEV_DAMAGE){
            s16 dmg=*q++;a->aux_ptr=(void *)q;
            if(amp_in_range(a,90)){
                s16 scaled=(s16)(dmg*2);
                if(player_type==PT_PREDATOR)scaled=(s16)(dmg*3);
                player_energy=(s16)(player_energy-scaled);
            }
            if(fight_cb)fight_cb(a,AVP_AMP_FIGHT_CROUCH);
            /* qdam/add8 branches straight back to qf2: the event itself does
             * not consume a frame/update. */
            continue;
        }
        a->animframe=(u16)t;
        return;
    }
}
static void queen_choose_fight(AvpAmp *a)
{
    unsigned i;
    if(--a->timer>=0)return;
    i=(unsigned)(avp_random()&3u);
    a->animframe=(u16)qseqs[i][0];
    a->aux_ptr=(void *)(qseqs[i]+1);
    a->animseq=qseq_anim[i];
    a->mode=queen_fight_mode;
    if(fight_cb)fight_cb(a,AVP_AMP_FIGHT_CROUCH);
}
static void queen_reverse_mode(AvpAmp *a)
{
    if(a->energy<0)a->energy=0;
    if(a->energy==0){queen_dead_begin(a);return;}
    amp_move_toward(a,0x180000,0x0e6400);
    if(queen_trigger(0))lockxxx(a);
}
static void queen_lock_mode(AvpAmp *a)
{
    if(a->energy<0)a->energy=0;
    if(a->energy==0){queen_dead_begin(a);return;}
    if(a->energy<a->oldenergy){
        a->animseq=AS_KNOCKBACK;a->animframe=0;a->timer=2;a->mode=queen_delay_mode;return;
    }
    if(queen_trigger(1)){a->mode=queen_reverse_mode;return;}
    if(amp_in_range(a,90)){queen_choose_fight(a);return;}
    amp_move_toward(a,(s32)x_pos,(s32)y_pos);
}
void lockxxx(AvpAmp *a){if(!a)return;a->mode=queen_lock_mode;a->animframe=0;a->animseq=AS_STAND;a->timer=3;}
