/* Readable C reconstruction of the ordinary 68000 AMP allocator/lifecycle
 * core from AMP/AMP.S.  Creature-specific mode handlers are translated in
 * subsequent sections of this module family.
 *
 * The original AMP record is 48 bytes on m68k.  This host-readable struct uses
 * a real function pointer for amp_mode, so host sizeof(AvpAmp) is intentionally
 * not asserted to the Jaguar binary layout.
 */
#include "amp.h"
#include <stddef.h>
#include <string.h>

AvpAmp level1amps[AVP_NUM_AMPS];
AvpAmp *amp_data=level1amps,*amps_at=level1amps;
u16 collmap[AVP_AMP_GRID_W*AVP_AMP_GRID_H];
u8 objmap[AVP_AMP_OBJ_ROW_BYTES*AVP_AMP_OBJ_H];
u16 levels_visit,discflag;

static u16 free_list[AVP_NUM_AMPS];
static unsigned free_count;
static void (*save_cb)(AvpAmp *,unsigned);
static void (*restore_cb)(AvpAmp *,unsigned);

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
}

void wipe_flgs(void){levels_visit=0;}

void InitAMPs(void)
{
    initialise();
    /* Source deliberately does not call build_level here. */
    wipe_flgs();
}

void build_level(void)
{
    unsigned i;
    /* Assembly writes 299..0 from low to high memory and leaves the pointer at
     * the end, so requests pop 0,1,2,... .  The equivalent stack below is
     * represented directly by free_count. */
    for(i=0;i<AVP_NUM_AMPS;++i) free_list[i]=(u16)(AVP_NUM_AMPS-1u-i);
    free_count=AVP_NUM_AMPS;
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

void save_level(void)
{
    if(save_cb) save_cb(amp_data,AVP_NUM_AMPS);
}

void restore_level(void)
{
    discflag=0;
    amp_data=amps_at=level1amps;
    initialise();
    build_level();
    if(restore_cb) restore_cb(amp_data,AVP_NUM_AMPS);
}

/* Projectile construction and invisible-player search are direct translations
 * of AMP.S player_weapon/lostplayer. */
extern u32 x_pos,y_pos;
extern u16 invisflag;
extern u16 avp_random(void);
extern void pre_discmove(AvpAmp *);
extern void pre_flame_on(AvpAmp *);

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
        a->xvector=0;
        /* Mode is set to chase_player by the eventual mode-function module;
         * chase callback executes its source-visible target setup now. */
        if(chase_cb)chase_cb(a,(s32)x_pos,(s32)y_pos);
        return;
    }
    dir=fseq_dir(a);count=(s16)(fseq_count(a)-1);
    if(count<0){count=(s16)(avp_random()&255u);dir=(u16)(avp_random()&3u);}
    set_fseq(a,dir,count);
    switch(dir){
    default:case 0:tx=0x7fff0000;ty=0;break;
    case 1:tx=0x7fff0000;ty=0x7fff0000;break;
    case 2:tx=0;ty=0;break;
    case 3:tx=0;ty=0x7fff0000;break;
    }
    if(chase_cb)chase_cb(a,tx,ty);
}

/* -------------------------------------------------------------------------
 * Active projectile/effect/generator modes translated from AMP/AMP.S.      */
#include "avp_runtime.h"
#include "maze.h"
#include "player.h"

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
    return o->safe_pos ? o->safe_pos(o->user,a->xpos,a->ypos,70,width) : 0;
}
static AvpAmp *collmap_amp(const AvpAmp *p)
{
    /* AMP.S uses the low word of each 16.16-ish world coordinate as a grid
     * index.  The retail collision map is 64 rows × 64 big-endian words. */
    unsigned gx=(unsigned)(u16)p->xpos;
    unsigned gy=(unsigned)(u16)p->ypos;
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
void exp_handle(AvpAmp *a)
{
    u16 f=(u16)(a->animframe+1u);a->animframe=f;
    if(f==(u16)a->xvel){const AvpRuntimeOps*o=avp_runtime_ops();if(o->area_damage)o->area_damage(o->user,a->xpos,a->ypos,(u32)a->timer,a->energy,(u16)(a->flags&((1u<<AMP_PHIT)|(1u<<AMP_INVHIT))));}
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

void make_spark_at(s32 x,s32 y,int blood)
{
    AvpAmp *a=amp_req();
    a->xpos=x;a->ypos=y;a->animseq=AS_STAND;a->animframe=(u16)-1;
    a->creature=blood?AC_ABLOOD:AC_SPARK1;a->energy=blood?2:4;
    a->yoffset=(s16)-(avp_random()&3u);a->mode=spark;a->host_static=0;
}
static void spark(AvpAmp *a){++a->animframe;if(--a->energy==0)release_amp(a);}

/* Generator / queen-shield complex.  Pointer-valued amp_timer in the Jaguar
 * source is represented by aux_ptr on 64-bit hosts. */
static void dead_gen2(AvpAmp *a);
static void dead_gen(AvpAmp *a)
{
    const AvpRuntimeOps*o=avp_runtime_ops();if(o->play_sfx)o->play_sfx(o->user,23);
    a->mode=dead_gen2;a->animseq=AS_DEATH;a->animframe=0;
}
static void dead_gen2(AvpAmp *a)
{
    if(++a->animframe==6){player_energy=(s16)(player_energy-50);release_amp(a);}
}
AvpAmp *make_shield(AvpAmp *owner,s32 dx,s32 dy)
{
    AvpAmp *s=amp_req();s->xpos=owner->xpos+dx;s->ypos=owner->ypos+dy;s->xvel=-2;
    s->mode=shield2;s->creature=AC_AQSHIELD;s->animseq=AS_STAND;s->animframe=(u16)(avp_random()&7u);return s;
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
    if(!ngens) release_amp(a);
}
void side_shield(AvpAmp *a,s32 dx,s32 dy)
{
    if(a->flags&(1u<<AMP_DEADGEN)){dead_gen(a);return;}
    AvpAmp *s=make_shield(a,dx,dy);s->flags|=(1u<<AMP_SSHIELD);a->aux_ptr=s;++ngens;a->mode=genmode;
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
void generate(AvpAmp *a)
{
    /* Source starts the force-field alarm for one frame, then continually
     * adjusts its attenuation/pitch from player distance and live generator
     * count.  Audio backend owns exact DSP table writes. */
    const AvpRuntimeOps *o=avp_runtime_ops();
    if(!ngens){if(o->kill_ambient)o->kill_ambient(o->user);release_amp(a);return;}
    if(o->play_sfx_params){u32 dx=abs32diff(a->xpos,(s32)x_pos),dy=abs32diff(a->ypos,(s32)y_pos);u32 big=dx>dy?dx:dy,small=dx>dy?dy:dx;u32 d=big+(small>>2);s32 vol=(s32)0x1a00-(s32)(d>>7)-(s32)((4-ngens)<<8);if(vol<0)vol=0;o->play_sfx_params(o->user,26,vol,(s32)(0xe0-(d>>16)-(4-ngens)*8));}
}
void cgenmode(AvpAmp *a)
{
    if(player_type!=PT_ALIEN){release_amp(a);return;}
    const s32 D=0x1a800, R=0xf0, OFF=(0x1a800-(0xf0<<8))/2;
    const s32 dxy[4][2]={{0,-D},{0,D},{D,0},{-D,0}};
    for(unsigned i=0;i<4;i++){AvpAmp*g=amp_req();g->xpos=a->xpos+dxy[i][0];g->ypos=a->ypos+dxy[i][1];g->creature=AC_GEN;g->animseq=AS_STAND;g->energy=200;g->flags=(1u<<AMP_KILLABLE);g->astype=(s16)i;side_shield(g,i==2?-OFF:i==3?OFF:0,i==0?OFF:i==1?-OFF:0);}
    for(unsigned i=0;i<12;i++){u16 ang=(u16)((0x10000u/12u)*i);s32 dx=(cos_d0(ang)*(s32)R)>>7; s32 dy=(sin_d0(ang)*(s32)R)>>7;AvpAmp*s=make_shield(a,dx,dy);s->astype=12;}
    a->animseq=AS_HIDDEN;a->astype=13;a->mode=generate;
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

void chase_player(AvpAmp *a)
{
    if(!a)return;
    a->flags|=(u16)((1u<<AMP_KILLABLE)|(1u<<AMP_MOTION));
    if(invisflag && !(a->flags&(1u<<AMP_PHIT))){a->mode=lostplayer;lostplayer(a);return;}
    if(chase_cb)chase_cb(a,(s32)x_pos,(s32)y_pos);
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

/* -------------------------------------------------------------------------
 * Final AMP.S routine-closure tranche. Authored placement/template payloads
 * are bound by the resource layer; this file owns the 68000 control semantics. */
#include "collision.h"
#include "levels.h"
#include "hud.h"

static const AvpAmpPlacement *level_lists[4][AVP_MAX_LEVEL];
static const AvpAmpTemplate *bound_templates;
static unsigned bound_template_count;
static u32 beam_counter;
static void (*fight_cb)(AvpAmp *,int);
static void (*death_cb)(AvpAmp *);
static int (*lift_test_cb)(const AvpAmp *);

void avp_amp_bind_level_list(unsigned k,unsigned level,const AvpAmpPlacement *list){if(k<4&&level>=1&&level<=AVP_MAX_LEVEL)level_lists[k][level-1]=list;}
void avp_amp_bind_templates(const AvpAmpTemplate *t,unsigned n){bound_templates=t;bound_template_count=n;}
void avp_amp_set_beam_counter(u32 v){beam_counter=v;}
void avp_amp_set_fight_callback(void (*fn)(AvpAmp *,int)){fight_cb=fn;}
void avp_amp_set_death_callback(void (*fn)(AvpAmp *)){death_cb=fn;}
void avp_amp_set_lift_test_callback(int (*fn)(const AvpAmp *)){lift_test_cb=fn;}

static void amp_cleargrid_c(AvpAmp *a){unsigned x=(u32)a->xpos>>16,y=(u32)a->ypos>>16;if(x<64&&y<64)collmap[y*64+x]=0;}
static int amp_in_range(const AvpAmp *a,u16 pixels){u32 lim=(u32)pixels<<9;return abs32diff(a->xpos,(s32)x_pos)<=lim&&abs32diff(a->ypos,(s32)y_pos)<=lim;}
static void set_static(AvpAmp *a){a->mode=NULL;a->host_static=1;}
static void dead_dispatch(AvpAmp *a){a->energy=0;if(death_cb)death_cb(a);else set_static(a);}
static void write_obj_grid(AvpAmp *a){unsigned x=(u32)a->xpos>>16,y=(u32)a->ypos>>16,n=amp_number(a);size_t o=y*AVP_AMP_OBJ_ROW_BYTES+x*2u;if(x<64&&y<64&&n&&o+1<sizeof(objmap)){objmap[o]=(u8)(n>>8);objmap[o+1]=(u8)n;}}
static void make_dead_egg(AvpAmp *a){amp_cleargrid_c(a);a->flags&=(u16)~(1u<<AMP_KILLABLE);a->animframe=5;a->astype=56;set_static(a);do_score(a);}

static void apply_template(AvpAmp *a,const AvpAmpPlacement *p){
    const AvpAmpTemplate *t;if(!bound_templates||p->def>=bound_template_count){set_static(a);return;}t=&bound_templates[p->def];
    a->xpos=(s32)((u32)(u16)p->x<<8);a->ypos=(s32)((u32)(u16)p->y<<8);a->mode=t->mode;a->creature=t->creature;
    a->animseq=t->animseq;a->animframe=t->animframe;a->angle=t->angle;a->timer=t->timer;a->xvel=t->xvel;a->yvel=t->yvel;a->ldir=t->ldir;
    a->xvector=t->xvector;a->yvector=t->yvector;a->energy=t->energy;a->oldenergy=t->oldenergy;a->flags=(u16)(t->flags|t->flags_or);a->yoffset=t->yoffset;a->astype=(s16)p->def;a->host_static=0;a->aux_ptr=NULL;
    {unsigned x=(u32)a->xpos>>16,y=(u32)a->ypos>>16;if(x<64&&y<64)collmap[y*64+x]=1;}
    if(a->creature==AC_HUMAN||a->creature==AC_ALIEN){a->xpos=(s32)(((u32)a->xpos&0xffff0000u)|0x8000u);a->ypos=(s32)(((u32)a->ypos&0xffff0000u)|0x8000u);}
}
void next_creature(const AvpAmpPlacement **cursor){const AvpAmpPlacement *p;if(!cursor||!(p=*cursor))return;while(p->x!=-1){AvpAmp*a=amp_req();apply_template(a,p);++p;}*cursor=p;}
void level_loop(void){const AvpAmpPlacement *p;if(cur_level<1||cur_level>AVP_MAX_LEVEL)return;levels_visit|=(u16)(1u<<((unsigned)cur_level-1u));p=level_lists[AVP_AMP_LIST_COMMON][cur_level-1];next_creature(&p);}
void append_objs(void){unsigned a,b;const AvpAmpPlacement*p;if(cur_level<1||cur_level>AVP_MAX_LEVEL)return;if(player_type==PT_ALIEN){a=AVP_AMP_LIST_HUMAN;b=AVP_AMP_LIST_PREDATOR;}else if(player_type==PT_PREDATOR){a=AVP_AMP_LIST_ALIEN;b=AVP_AMP_LIST_HUMAN;}else{a=AVP_AMP_LIST_PREDATOR;b=AVP_AMP_LIST_ALIEN;}p=level_lists[a][cur_level-1];next_creature(&p);p=level_lists[b][cur_level-1];next_creature(&p);}

void gofight(AvpAmp *a){AvpXY from,to;if(!a||discflag)return;if(!amp_in_range(a,(128u*9u)/2u)){a->flags&=(u16)~(1u<<AMP_PHIT);return;}if((beam_counter&7u)!=2u)return;from.x=a->xpos;from.y=a->ypos;to.x=(s32)x_pos;to.y=(s32)y_pos;if(LineOfSight(&from,&to)){a->flags&=(u16)~(1u<<AMP_PHIT);return;}a->animframe=0;if(beam_counter&1u){a->animseq=AS_FIGHT2;if(fight_cb)fight_cb(a,AVP_AMP_FIGHT_FLAME);}else{a->animseq=AS_FIGHT1;if(fight_cb)fight_cb(a,AVP_AMP_FIGHT_CROUCH);}}

void stun_mode(AvpAmp *a){if(!a)return;if(a->timer==0){a->animseq=AS_RUN;a->animframe=0;a->mode=chase_player;a->flags&=(u16)~(1u<<AMP_STUN);return;}--a->timer;if(!(a->flags&(1u<<AMP_STUN))){dead_dispatch(a);return;}if(a->energy>0)return;if(use_cocoon||(lift_test_cb&&lift_test_cb(a))){dead_dispatch(a);return;}amp_cleargrid_c(a);a->mode=stun_death;a->timer=50;a->animseq=AS_DEATH;a->animframe=0;}
void stun_death(AvpAmp *a){const AvpRuntimeOps*o=avp_runtime_ops();if(!a)return;if(a->animframe!=2){++a->animframe;return;}if(--a->timer==0){set_static(a);return;}if(!amp_in_range(a,20))return;if(num_cocoons>=AVP_MAX_COCOONS){set_static(a);a->flags=(1u<<AMP_DEAD);return;}set_static(a);a->creature=AC_COCOON;a->animseq=AS_STAND;a->animframe=0;a->astype=18;a->flags=0;MakeCocoon((u32)a->xpos,(u32)a->ypos);if(o->play_sfx)o->play_sfx(o->user,27);}
void cocoon(AvpAmp *a){if(!a)return;if((u16)(a->xpos>>16)!=(u16)(x_pos>>16)||(u16)(a->ypos>>16)!=(u16)(y_pos>>16)){write_obj_grid(a);set_static(a);return;}a->xpos=(s32)(((u32)a->xpos&0xffff0000u)|0x8000u);a->ypos=(s32)(((u32)a->ypos&0xffff0000u)|0x8000u);a->creature=AC_EGG;make_dead_egg(a);}

static void wait_hug_mode(AvpAmp *a);
static void egg_open2_mode(AvpAmp *a){if(a->energy<=0){a->energy=0;make_dead_egg(a);return;}if(++a->animframe==4){a->mode=wait_hug_mode;a->timer=0;}}
static void egg_open_mode(AvpAmp *a){const AvpRuntimeOps*o=avp_runtime_ops();if(o->play_sfx)o->play_sfx(o->user,28);a->mode=egg_open2_mode;egg_open2_mode(a);}
static void fire_hug_mode(AvpAmp *a){AvpAmp*h;if(!bound_templates||4u>=bound_template_count){set_static(a);return;}h=amp_req();*h=(AvpAmp){0};h->xpos=a->xpos;h->ypos=a->ypos;{AvpAmpPlacement p={(s16)((u32)a->xpos>>8),(s16)((u32)a->ypos>>8),4};apply_template(h,&p);h->xpos=a->xpos;h->ypos=a->ypos;}make_dead_egg(a);}
static void wait_hug_mode(AvpAmp *a){if(!(a->flags&(1u<<AMP_EGGOPEN))){if(a->energy<=0){a->energy=0;make_dead_egg(a);return;}if((u16)a->timer!=20u){a->timer=(s32)((u16)a->timer+1u);return;}}a->mode=fire_hug_mode;}
void eggwait(AvpAmp *a){AvpXY from,to;if(!a||player_type==PT_ALIEN)return;a->flags&=(u16)~(1u<<AMP_EGGOPEN);if(a->energy<a->ldir){a->ldir=a->energy;a->flags|=(1u<<AMP_EGGOPEN);}else{amp_setgrid(a);if(a->energy<=0){a->energy=0;make_dead_egg(a);return;}if(!amp_in_range(a,128))return;}from.x=a->xpos;from.y=a->ypos;to.x=(s32)x_pos;to.y=(s32)y_pos;if(LineOfSight(&from,&to))return;a->mode=egg_open_mode;}

static void queen_delay_mode(AvpAmp *a){if(--a->timer==0)lockxxx(a);}
static void queen_dead_mode(AvpAmp *a){if((a->flags^=(1u<<AMP_ALTFRAME))&(1u<<AMP_ALTFRAME)){if(++a->animframe==3){a->flags&=(u16)~(1u<<AMP_MOTION);a->astype=57;set_static(a);}}}
static void queen_lock_mode(AvpAmp *a){if(a->energy<0)a->energy=0;if(a->energy==0){a->animseq=AS_DEATH;a->animframe=0;a->mode=queen_dead_mode;return;}if(a->energy<a->oldenergy){a->animseq=AS_KNOCKBACK;a->animframe=0;a->timer=2;a->mode=queen_delay_mode;return;}if(amp_in_range(a,90)){if(fight_cb)fight_cb(a,AVP_AMP_FIGHT_CROUCH);return;}if(chase_cb)chase_cb(a,(s32)x_pos,(s32)y_pos);}
void lockxxx(AvpAmp *a){if(!a)return;a->mode=queen_lock_mode;a->animframe=0;a->animseq=AS_STAND;a->timer=3;}
