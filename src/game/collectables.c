/* Readable C reconstruction of the collection-effect core in MAZE/PLAYER.S:
 * CollectIt, give_energy, give_wep and give_ammo.
 *
 * Includes spatial object-grid lookup/body specialization plus the inventory,
 * security-card, ammo, weapon and energy semantics from the retail source.
 */
#include "collectables.h"
#include "player.h"
#include "weapons.h"
#include "amp.h"
#include "levels.h"
#include "maze.h"
#include <stddef.h>

#define PMED_MAX 1000

AvpAmmoInfo ammo_info[6];
s16 medpak;
s16 show_mt;
/* acs_level is owned by doors.c, matching the original access subsystem. */
extern u8 acs_level;

static AvpCollectNoticeFn notice_fn;
static AvpCollectSfxFn sfx_fn;

enum {
    CSFX_SECURITY=1, CSFX_WEAPON_SHOT=2, CSFX_WEAPON_PULSE=3,
    CSFX_WEAPON_FLAME=4, CSFX_WEAPON_SMART=5, CSFX_PICKUP=6,
    CSFX_MTRACK=7, CSFX_PRED_ENERGY=8
};

void avp_collect_set_callbacks(AvpCollectNoticeFn notice,AvpCollectSfxFn sfx)
{ notice_fn=notice; sfx_fn=sfx; }

typedef struct BodyLoot { s16 x,y,type,amount; } BodyLoot;
#define END_BODY { -1,0,0,0 }
static const BodyLoot l1_bodies[]={ {0x22,0x21,COLL_S7,0},{0x22,0x0e,COLL_PULSERIFLE,60},END_BODY };
static const BodyLoot l2_bodies[]={ {0x1e,0x29,COLL_S3,0},{0x29,0x14,COLL_PULSERIFLE,60},{0x28,0x1b,COLL_FLAMETHROWER,25},END_BODY };
static const BodyLoot l3_bodies[]={ {0x24,0x09,COLL_SHOTGUN,15},{0x22,0x11,COLL_S1,0},{0x08,0x2a,COLL_S2,0},{0x22,0x19,COLL_SHOTAMMO,15},{0x1f,0x1c,COLL_SHOTAMMO,15},{0x1c,0x1e,COLL_SHOTAMMO,15},{0x08,0x08,COLL_SHOTAMMO,15},{0x04,0x23,COLL_FOOD,100},{0x07,0x21,COLL_FOOD,100},{0x21,0x0a,COLL_FOOD,100},{0x24,0x11,COLL_FOOD,100},{0x25,0x21,COLL_MEDAID,300},{0x26,0x28,COLL_MEDAID,300},{0x22,0x28,COLL_MEDAID,300},{0x0e,0x2a,COLL_PULSERIFLE,60},END_BODY };
static const BodyLoot l4_bodies[]={ {0x17,0x11,COLL_S4,0},{0x0c,0x13,COLL_S6,0},{0x2c,0x12,COLL_PULSERIFLE,60},{0x2a,0x18,COLL_FLAMETHROWER,25},{0x1d,0x24,COLL_PULSERIFLE,60},END_BODY };
static const BodyLoot l5_bodies[]={ {0x19,0x27,COLL_S5,0},{0x22,0x0e,COLL_MTRACKER,0},END_BODY };
static const BodyLoot l14_bodies[]={ {0x0b,0x23,COLL_PULSEAMMO,60},{0x09,0x16,COLL_PULSEAMMO,60},{0x10,0x0d,COLL_PULSEAMMO,60},{0x10,0x0a,COLL_MEDAID,300},{0x08,0x0b,COLL_PULSEAMMO,60},{0x2e,0x06,COLL_PULSEAMMO,60},{0x2d,0x26,COLL_PULSEAMMO,60},{0x2d,0x23,COLL_PULSEAMMO,60},{0x2d,0x1b,COLL_PULSEAMMO,60},{0x26,0x09,COLL_PULSEAMMO,60},{0x14,0x0d,COLL_PULSEAMMO,60},{0x17,0x0a,COLL_PULSEAMMO,60},{0x24,0x1b,COLL_PULSEAMMO,60},{0x08,0x1f,COLL_SHOTAMMO,15},{0x0b,0x16,COLL_SHOTAMMO,15},{0x0f,0x06,COLL_SHOTAMMO,15},{0x32,0x08,COLL_SHOTAMMO,15},{0x2f,0x08,COLL_SHOTAMMO,15},{0x26,0x23,COLL_SHOTAMMO,15},{0x14,0x0a,COLL_SHOTAMMO,15},{0x0b,0x03,COLL_GRENADES,8},{0x0a,0x03,COLL_S8,0},{0x21,0x0b,COLL_S10,0},END_BODY };
static const BodyLoot l15_bodies[]={ {0x0b,0x15,COLL_S9,0},{0x17,0x0c,COLL_SHOTAMMO,15},{0x25,0x0a,COLL_SHOTAMMO,15},{0x25,0x17,COLL_SHOTAMMO,15},{0x02,0x08,COLL_MEDAID,300},{0x24,0x17,COLL_MEDAID,300},{0x28,0x12,COLL_MEDAID,300},{0x19,0x0a,COLL_GRENADES,8},{0x2a,0x10,COLL_SMARTGUN,0},END_BODY };
static const BodyLoot no_bodies[]={END_BODY};
static const BodyLoot *cur_bodies=no_bodies;

void ResetClct(void)
{
    static const BodyLoot *const table[AVP_MAX_LEVEL]={l1_bodies,l2_bodies,l3_bodies,l4_bodies,l5_bodies,no_bodies,no_bodies,no_bodies,no_bodies,no_bodies,no_bodies,no_bodies,no_bodies,l14_bodies,l15_bodies};
    cur_bodies=(cur_level>=1&&cur_level<=AVP_MAX_LEVEL)?table[cur_level-1]:no_bodies;
}

static int collect_info_for(s16 def,s16 *type,s16 *amount)
{
    *type=0;*amount=0;
    switch(def){
    case 11:*type=COLL_MEDAID;*amount=300;break;
    case 12:*type=COLL_SMARTAMMO;*amount=20;break;
    case 15:*type=COLL_FLAMEAMMO;*amount=25;break;
    case 16:*type=COLL_PULSEAMMO;*amount=60;break;
    case 18:*type=-1;break; /* COLL_BODY */
    case 21:*type=COLL_FOOD;*amount=100;break;
    case 23:*type=COLL_SMARTAMMO;*amount=20;break;
    case 24:*type=COLL_PULSEAMMO;*amount=60;break;
    case 25:*type=COLL_SHOTAMMO;*amount=15;break;
    case 38:case 39:case 40:*type=-1;break; /* dead bodies */
    default:break;
    }
    return *type!=0;
}

void Collectables(void)
{
    unsigned x=(unsigned)(x_pos>>16),y=(unsigned)(y_pos>>16),off;
    u16 no;AvpAmp *a;s16 type,amount;int body=0;
    if(x>=AVP_AMP_GRID_W||y>=AVP_AMP_GRID_H)return;
    off=y*AVP_AMP_OBJ_ROW_BYTES+x*2u;
    no=(u16)(((u16)objmap[off]<<8)|objmap[off+1]);
    if(!no||no>AVP_NUM_AMPS)return;
    a=&amp_data[no-1u];
    if(!collect_info_for(a->astype,&type,&amount)){objmap[off]=objmap[off+1]=0;return;}
    if(type==-1){
        const BodyLoot *b;
        body=1;
        if(a->flags&(1u<<AMP_COLLECT)){objmap[off]=objmap[off+1]=0;return;}
        for(b=cur_bodies;b->x>=0;++b)if((unsigned)b->x==x&&(unsigned)b->y==y){type=b->type;amount=b->amount;break;}
        if(b->x<0){
            if(a->astype==39){type=COLL_SHOTAMMO;amount=15;}
            else if(a->astype==38){type=COLL_PULSEAMMO;amount=60;}
            else if(a->astype==40){type=COLL_MEDAID;amount=300;}
            else {objmap[off]=objmap[off+1]=0;return;}
        }
    }
    if(!CollectIt(type,amount))return;
    if(body)a->flags|=(1u<<AMP_COLLECT);else a->mode=NULL;
    objmap[off]=objmap[off+1]=0;
}

static void notify(enum AvpCollectNotice n,unsigned detail)
{ if (notice_fn && n!=AVP_CNOTICE_NONE) notice_fn(n,detail); }
static void sfx(unsigned n)
{ if (sfx_fn && n) sfx_fn(n); }

int avp_give_energy(s16 amount,enum AvpCollectNotice notice,unsigned sound)
{
    s16 *dst=&player_energy;
    s16 cap=max_energy;
    s32 v;
    if (player_type==PT_PREDATOR) { dst=&medpak; cap=PMED_MAX; }
    if (*dst==cap) return 0;
    v=(s32)*dst+(s32)amount;
    if (v>cap) v=cap;
    *dst=(s16)v;
    notify(notice,0);
    sfx(sound);
    return 1;
}

int avp_give_ammo(unsigned weapon,s16 amount,enum AvpCollectNotice notice,unsigned sound)
{
    AvpAmmoInfo *a;
    s32 v;
    if (weapon<1 || weapon>6) return 0;
    a=&ammo_info[weapon-1];
    if (a->cur==a->max) return 0;
    v=(s32)a->cur+(s32)amount;
    if (v>a->max) v=a->max;
    a->cur=(u16)v;
    notify(notice,weapon);
    sfx(sound);
    return 1;
}

void avp_give_weapon(unsigned weapon,s16 initial_ammo,enum AvpCollectNotice ammo_notice,
                     enum AvpCollectNotice weapon_notice,unsigned weapon_sfx,unsigned ammo_sfx)
{
    if (weapon>7) return;
    if (!(cur_weps&(1u<<weapon))) {
        cur_weps=(u8)(cur_weps|(1u<<weapon));
        if ((s16)weapon>cur_wepno) new_wepno=(s16)weapon;
        notify(weapon_notice,weapon);
        if (weapon_sfx) { sfx(weapon_sfx); ammo_sfx=0; }
    }
    /* Assembly deliberately falls through into give_ammo even when the weapon
     * was already owned. */
    (void)avp_give_ammo(weapon,initial_ammo,ammo_notice,ammo_sfx);
}

int CollectIt(s16 type,s16 amount)
{
    unsigned pick=CSFX_PICKUP;

    /* Food and med-aid are the only collection classes reachable by Predator;
     * all cards/weapons/ammo are skipped by the .nrg_only branch. */
    if (player_type!=PT_PREDATOR) {
        if (type>=COLL_S1 && type<=COLL_S10) {
            if ((u8)type>acs_level) {
                acs_level=(u8)type;
                notify(AVP_CNOTICE_CARD,(unsigned)type);
                sfx(CSFX_SECURITY);
            }
            /* A card is removed even if the player already has this/higher
             * access: original .kill path always succeeds. */
            return 1;
        }
        switch(type) {
        case COLL_SHOTGUN:
            avp_give_weapon(1,amount,AVP_CNOTICE_SHOT_AMMO,AVP_CNOTICE_SHOTGUN,
                            CSFX_WEAPON_SHOT,CSFX_PICKUP); return 1;
        case COLL_SHOTAMMO:
            return avp_give_ammo(1,amount,AVP_CNOTICE_SHOT_AMMO,CSFX_PICKUP);
        case COLL_PULSERIFLE:
            avp_give_weapon(2,amount,AVP_CNOTICE_PULSE_AMMO,AVP_CNOTICE_PULSE,
                            CSFX_WEAPON_PULSE,CSFX_PICKUP); return 1;
        case COLL_PULSEAMMO:
            return avp_give_ammo(2,amount,AVP_CNOTICE_PULSE_AMMO,CSFX_PICKUP);
        case COLL_FLAMETHROWER:
            avp_give_weapon(3,amount,AVP_CNOTICE_FLAME_AMMO,AVP_CNOTICE_FLAME,
                            CSFX_WEAPON_FLAME,CSFX_PICKUP); return 1;
        case COLL_FLAMEAMMO:
            return avp_give_ammo(3,amount,AVP_CNOTICE_FLAME_AMMO,CSFX_PICKUP);
        case COLL_SMARTGUN:
            avp_give_weapon(4,amount,AVP_CNOTICE_SMART_AMMO,AVP_CNOTICE_SMART,
                            CSFX_WEAPON_SMART,CSFX_PICKUP); return 1;
        case COLL_SMARTAMMO:
            return avp_give_ammo(4,amount,AVP_CNOTICE_SMART_AMMO,CSFX_PICKUP);
        case COLL_MTRACKER:
            show_mt=-1; notify(AVP_CNOTICE_MTRACKER,0); sfx(CSFX_MTRACK); return 1;
        default: break;
        }
    }

    if (type==COLL_FOOD)
        return avp_give_energy(amount,AVP_CNOTICE_FOOD,
                               player_type==PT_PREDATOR?CSFX_PRED_ENERGY:pick);
    if (type==COLL_MEDAID)
        return avp_give_energy(amount,AVP_CNOTICE_MEDAID,
                               player_type==PT_PREDATOR?CSFX_PRED_ENERGY:pick);
    return 0;
}
