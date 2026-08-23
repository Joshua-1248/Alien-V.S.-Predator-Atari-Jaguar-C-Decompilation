#ifndef AVP_WEAPONS_H
#define AVP_WEAPONS_H
#include "avp_types.h"

enum AvpWeaponNotice {
    AVP_WNOTICE_NONE=0,
    AVP_WNOTICE_GAIN_COMBI,
    AVP_WNOTICE_LOSE_COMBI,
    AVP_WNOTICE_GAIN_DISC,
    AVP_WNOTICE_LOSE_DISC,
    AVP_WNOTICE_GAIN_CANNON,
    AVP_WNOTICE_LOSE_CANNON
};

typedef void (*AvpWeaponNoticeFn)(enum AvpWeaponNotice notice);
typedef void (*AvpWeaponSfxFn)(unsigned cue);

void InitPredAvail(void);
void PredAvail(void);
void avp_weapons_set_callbacks(AvpWeaponNoticeFn notice,AvpWeaponSfxFn sfx);

extern u8 cur_weps,old_weps;
extern s16 cur_wepno,new_wepno;

#endif
