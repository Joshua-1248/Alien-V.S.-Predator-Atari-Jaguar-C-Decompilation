#include <assert.h>
#include <string.h>
#include "amp.h"
#include "player.h"


#include "collision.h"
s16 cur_level=1,num_cocoons;
s16 game_over,key_lock; u8 acs_level;
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

    /* lockxxx: queen_delay=3 means qfight selection occurs on the fourth
     * in-range .lockx update (3,2,1,0,-1/BPL semantics). avp_random()=0
     * selects qswipe: initial frame 2, then 0,1,2,$fe damage 50. */
    a->xpos=a->ypos=0; x_pos=y_pos=0;
    a->energy=a->oldenergy=1000; a->flags=0; a->host_static=0;
    player_type=PT_HUMAN; player_energy=1000;
    lockxxx(a);
    a->mode(a); a->mode(a); a->mode(a);
    assert(a->animseq==AS_STAND);
    a->mode(a);
    assert(a->animseq==AS_FIGHT2 && a->animframe==2);
    a->mode(a); a->mode(a); a->mode(a); a->mode(a);
    assert(player_energy==900); /* 50 * 2 for non-Predator */

    /* Predator queen death: every second play_death update advances a frame;
     * after frame 3 the source locks input for 20 ticks then sets game_over. */
    a->energy=0; a->oldenergy=0; a->flags=0; a->host_static=0;
    player_type=PT_PREDATOR; key_lock=game_over=0;
    lockxxx(a); a->mode(a); /* queendead -> play_death */
    for(int i=0;i<6;i++)a->mode(a);
    assert(a->astype==57 && key_lock==1);
    for(int i=0;i<20;i++)a->mode(a);
    assert(game_over==1 && a->host_static==1);

    return 0;
}
