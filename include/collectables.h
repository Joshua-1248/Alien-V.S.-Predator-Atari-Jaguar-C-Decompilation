#ifndef AVP_COLLECTABLES_H
#define AVP_COLLECTABLES_H
#include "avp_types.h"

enum AvpCollectType {
    COLL_NONE=0,
    COLL_S1=1,COLL_S2,COLL_S3,COLL_S4,COLL_S5,COLL_S6,COLL_S7,COLL_S8,COLL_S9,COLL_S10,
    COLL_SHOTGUN,COLL_SHOTAMMO,COLL_PULSERIFLE,COLL_PULSEAMMO,COLL_GRENADES,
    COLL_FLAMETHROWER,COLL_FLAMEAMMO,COLL_SMARTGUN,COLL_SMARTAMMO,COLL_MTRACKER,
    COLL_FOOD,COLL_MEDAID
};

enum AvpCollectNotice {
    AVP_CNOTICE_NONE=0, AVP_CNOTICE_CARD, AVP_CNOTICE_SHOTGUN, AVP_CNOTICE_SHOT_AMMO,
    AVP_CNOTICE_PULSE, AVP_CNOTICE_PULSE_AMMO, AVP_CNOTICE_FLAME, AVP_CNOTICE_FLAME_AMMO,
    AVP_CNOTICE_SMART, AVP_CNOTICE_SMART_AMMO, AVP_CNOTICE_MTRACKER,
    AVP_CNOTICE_FOOD, AVP_CNOTICE_MEDAID
};

typedef struct AvpAmmoInfo {
    u16 cur,max,old,rescale;
    s16 x,y;
    u16 dec,inc;
} AvpAmmoInfo;

typedef void (*AvpCollectNoticeFn)(enum AvpCollectNotice notice,unsigned detail);
typedef void (*AvpCollectSfxFn)(unsigned cue);

void ResetClct(void);
void Collectables(void);
int CollectIt(s16 type,s16 amount);
int avp_give_energy(s16 amount,enum AvpCollectNotice notice,unsigned sfx);
int avp_give_ammo(unsigned weapon,s16 amount,enum AvpCollectNotice notice,unsigned sfx);
void avp_give_weapon(unsigned weapon,s16 initial_ammo,enum AvpCollectNotice ammo_notice,
                     enum AvpCollectNotice weapon_notice,unsigned weapon_sfx,unsigned ammo_sfx);
void avp_collect_set_callbacks(AvpCollectNoticeFn notice,AvpCollectSfxFn sfx);

extern AvpAmmoInfo ammo_info[6];
extern s16 medpak;
extern s16 show_mt;
extern u8 acs_level;

#endif
