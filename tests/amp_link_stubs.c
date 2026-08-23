#include "avp_types.h"
#include "collision.h"
u8 cur_weps;
s16 hugkill;
void Vector(const AvpXY *from,const AvpXY *to,s32 *vx,s32 *vy)
{
    *vx=to->x-from->x;*vy=to->y-from->y;
}

void HumanPain(void){}

void AreaDamage(s32 xpos,s32 ypos,s32 radius,s16 damage,u16 damage_flags)
{
    (void)xpos;(void)ypos;(void)radius;(void)damage;(void)damage_flags;
}

u8 SafePos(u8 height,s32 xpos,s32 ypos,u16 width)
{
    (void)height;(void)xpos;(void)ypos;(void)width;
    return AVP_COLL_SAFE;
}
int avp_doors_cell_is_liftside(s32 xpos,s32 ypos){(void)xpos;(void)ypos;return 0;}
