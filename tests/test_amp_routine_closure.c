#include <assert.h>
#include <string.h>
#include "amp.h"
#include "player.h"


#include "collision.h"
s16 cur_level=1,num_cocoons;
void MakeCocoon(u32 x,u32 y){(void)x;(void)y;}
int LineOfSight(const AvpXY *from,const AvpXY *to){(void)from;(void)to;return 0;}
u32 x_pos,y_pos;
u16 invisflag;
s16 player_energy,player_type,use_cocoon;
s32 score;
u16 avp_random(void){return 0;}
s32 sin_d0(u16 a){(void)a;return 0;}
s32 cos_d0(u16 a){(void)a;return 0x4000;}

int main(void)
{
    AvpAmp *a;
    InitAMPs();
    build_level();
    a=amp_req();

    QFRAME(a,3);
    assert(a->animframe==3);

    memset(collmap,0,sizeof(collmap));
    a->xpos=(s32)(7u<<16);
    a->ypos=(s32)(11u<<16);
    amp_setgrid(a);
    assert(collmap[11u*AVP_AMP_GRID_W+7u]==1u);

    score=0;
    player_type=PT_HUMAN;
    a->creature=AC_ALIEN;
    a->flags=(u16)(1u<<AMP_PHIT);
    do_score(a);
    assert(score==10000);

    score=0;
    player_type=PT_ALIEN;
    a->creature=AC_HUMAN;
    a->flags=(u16)(1u<<AMP_PHIT);
    do_score(a);
    assert(score==20000);

    score=100000;
    player_type=PT_PREDATOR;
    a->creature=AC_HUMAN;
    a->flags=(u16)((1u<<AMP_PHIT)|(1u<<AMP_INVHIT));
    do_score(a);
    assert(score==90000); /* -5000, doubled because not close = -10000 */

    score=100;
    a->flags=(u16)((1u<<AMP_PHIT)|(1u<<AMP_INVHIT));
    a->creature=AC_PREDATOR;
    do_score(a);
    assert(score==0); /* source clamps negative score */

    return 0;
}
