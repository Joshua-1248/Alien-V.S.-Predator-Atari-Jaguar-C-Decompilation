#include "amp.h"
#include "player.h"
#include <assert.h>
u32 x_pos=0x120000,y_pos=0x340000;u16 invisflag=0x0800; volatile u16 seed0=1,seed1=2; s16 player_energy,player_type,use_cocoon;
s32 sin_d0(u16 a){(void)a;return 0;} s32 cos_d0(u16 a){(void)a;return 0x4000;}
u16 avp_random(void){u16 r=seed0;seed0=(u16)(seed0+1);return r;}
static s32 tx,ty;static void chase(AvpAmp*a,s32 x,s32 y){(void)a;tx=x;ty=y;}
int main(void){AvpAmp*a;build_level();a=player_weapon(0x4000,0,0,77,0,0);assert(a==&level1amps[0]);assert(a->creature==AC_SMART&&a->yoffset==20);assert(a->xvel==-1&&a->yvel==77);assert((a->flags&(1u<<AMP_PLAYER))!=0);assert(a->timer==1);
 avp_amp_set_chase_callback(chase);a->flags=0;a->xvector=0;invisflag=1;lostplayer(a);assert(tx==0x7fff0000||tx==0);invisflag=0;lostplayer(a);assert(tx==(s32)x_pos&&ty==(s32)y_pos);return 0;}
