/* C translation of JOYED_exact_sourcefit.s. */
#include "joyed.h"
#include "joypad.h"
#include <stdint.h>

#define EDIT_OBJECT_SPECIAL ((const void *)(uintptr_t)0x000203F8u)
void MoveData(JoyEditRecord *item,const void *edited_object) {
    unsigned control_off=(edited_object==EDIT_OBJECT_SPECIAL)?19u:13u;
    u32 step=(u16)item->magnitude;
    unsigned shift=(unsigned)((u8)(6u-((const u8*)item)[control_off])) & 31u;
    step=(step>>shift)<<3;
    s32 v=item->value; u32 edge=joy_edge;
    if ((edge&(1u<<20))==0u) v-=(s32)step;
    if ((edge&(1u<<21))==0u) v+=(s32)step;
    if ((edge&(1u<<22))==0u) --v;
    if ((edge&(1u<<23))==0u) ++v;
    item->value=v;
}
void MoveXYFromJoy(JoyEditRecord *item) {
    u32 cur=joy_cur; s16 x=item->x,y=item->y;
    if ((cur&(1u<<20))==0u) --y;
    if ((cur&(1u<<21))==0u) ++y;
    if ((cur&(1u<<22))==0u) --x;
    if ((cur&(1u<<23))==0u) ++x;
    item->x=x; item->y=y;
}
