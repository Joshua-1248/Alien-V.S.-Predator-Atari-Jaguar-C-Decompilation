#include "collectables.h"
#include "weapons.h"
#include "player.h"
#include "amp.h"
#include "levels.h"
#include "maze.h"
#include <assert.h>
s16 player_type,player_energy,max_energy; u8 cur_weps,old_weps; s16 cur_wepno,new_wepno; u8 acs_level;
AvpAmp amps[AVP_NUM_AMPS],*amp_data=amps,*amps_at=amps; u8 objmap[AVP_AMP_OBJ_ROW_BYTES*AVP_AMP_OBJ_H]; u16 collmap[AVP_AMP_GRID_W*AVP_AMP_GRID_H]; s16 cur_level=1; u32 x_pos,y_pos;
static int notices;
static void n(enum AvpCollectNotice x,unsigned d){(void)x;(void)d;++notices;}
static void s(unsigned x){(void)x;}
int main(void){
 avp_collect_set_callbacks(n,s); player_type=PT_HUMAN; player_energy=500; max_energy=1000; acs_level=0;
 assert(CollectIt(COLL_S3,0)==1 && acs_level==3); assert(CollectIt(COLL_S2,0)==1 && acs_level==3);
 ammo_info[0].max=20; ammo_info[0].cur=0; cur_weps=0; cur_wepno=0; new_wepno=0;
 assert(CollectIt(COLL_SHOTGUN,8)==1); assert(cur_weps&(1u<<1)); assert(new_wepno==1); assert(ammo_info[0].cur==8);
 ammo_info[0].cur=20; assert(CollectIt(COLL_SHOTAMMO,5)==0);
 assert(CollectIt(COLL_MEDAID,600)==1); assert(player_energy==1000); assert(CollectIt(COLL_MEDAID,1)==0);
 player_type=PT_PREDATOR; medpak=0; assert(CollectIt(COLL_SHOTGUN,8)==0); assert(CollectIt(COLL_FOOD,200)==1); assert(medpak==200);
 return 0;
}
