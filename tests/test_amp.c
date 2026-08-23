#include "amp.h"
#include <assert.h>

#include "collision.h"
s16 cur_level=1,num_cocoons;
s16 game_over,key_lock; u8 acs_level;
void MakeCocoon(u32 x,u32 y){(void)x;(void)y;}
int LineOfSight(const AvpXY *from,const AvpXY *to){(void)from;(void)to;return 0;}
s32 score;
u32 x_pos,y_pos;u16 invisflag; s16 player_energy,player_type,use_cocoon;
u16 avp_random(void){return 0;} s32 sin_d0(u16 a){(void)a;return 0;} s32 cos_d0(u16 a){(void)a;return 0x4000;}
static void dummy(AvpAmp*a){(void)a;}
int main(void){unsigned i;for(i=0;i<AVP_NUM_AMPS;++i){level1amps[i].mode=dummy;level1amps[i].flags=0xffff;}InitAMPs();assert(levels_visit==0);for(i=0;i<AVP_NUM_AMPS;++i)assert(level1amps[i].mode==0);build_level();{AvpAmp*a=amp_req();assert(a==&level1amps[0]);assert(a->flags==0&&a->astype==-1);} {AvpAmp*b=amp_req();assert(b==&level1amps[1]);amp_release(2);assert(amp_req()==&level1amps[1]);}return 0;}
