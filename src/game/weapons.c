/* Readable C reconstruction of the Predator honour-system weapon availability
 * routines in MAZE/MAZESCRN.S (InitPredAvail / PredAvail).
 *
 * Weapon numbers deliberately retain the original bit positions:
 *   1 combi-stick, 2 shoulder cannon, 3 smart disc, 4 base weapon.
 */
#include "weapons.h"
#include "player.h"
#include <stddef.h>

u8 cur_weps,old_weps;
s16 cur_wepno,new_wepno;

struct PredAward {
    s32 score;
    u8 weapon;
    enum AvpWeaponNotice gain,lose;
};

static const struct PredAward awards[]={
    {0,4,AVP_WNOTICE_NONE,AVP_WNOTICE_NONE},
    {150000,1,AVP_WNOTICE_GAIN_COMBI,AVP_WNOTICE_LOSE_COMBI},
    {350000,3,AVP_WNOTICE_GAIN_DISC,AVP_WNOTICE_LOSE_DISC},
    {750000,2,AVP_WNOTICE_GAIN_CANNON,AVP_WNOTICE_LOSE_CANNON}
};
static unsigned pred_index;
static AvpWeaponNoticeFn notice_fn;
static AvpWeaponSfxFn sfx_fn;

enum { SFX_PRED_LOSE=1, SFX_PRED_GAIN=2 };

void avp_weapons_set_callbacks(AvpWeaponNoticeFn notice,AvpWeaponSfxFn sfx)
{ notice_fn=notice; sfx_fn=sfx; }

void InitPredAvail(void) { pred_index=0; }

void PredAvail(void)
{
    s32 sc=score;
    int changed=0;

    /* Falling below the threshold represented by pred_ptr repeatedly removes
     * weapons and walks to the preceding award record. */
    while (pred_index>0 && sc<awards[pred_index].score) {
        const struct PredAward *a=&awards[pred_index];
        cur_weps=(u8)(cur_weps & (u8)~(1u<<a->weapon));
        if (notice_fn) notice_fn(a->lose);
        if (cur_wepno==(s16)a->weapon)
            new_wepno=(s16)awards[pred_index-1].weapon;
        --pred_index;
        changed=-1;
    }
    if (changed) {
        if (sfx_fn) sfx_fn(SFX_PRED_LOSE);
        return;
    }

    /* Rising can cross more than one threshold in a single update. */
    while (pred_index+1<sizeof(awards)/sizeof(awards[0]) &&
           sc>=awards[pred_index+1].score) {
        const struct PredAward *a=&awards[++pred_index];
        cur_weps=(u8)(cur_weps | (u8)(1u<<a->weapon));
        new_wepno=(s16)a->weapon;
        if (notice_fn) notice_fn(a->gain);
        changed=1;
    }
    if (changed && sfx_fn) sfx_fn(SFX_PRED_GAIN);
}
