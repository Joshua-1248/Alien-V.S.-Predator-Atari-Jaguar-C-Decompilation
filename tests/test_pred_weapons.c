#include "weapons.h"
#include "player.h"
#include <assert.h>
s32 score; static int notice_count,last_notice,last_sfx;
static void n(enum AvpWeaponNotice x){++notice_count;last_notice=x;}
static void s(unsigned x){last_sfx=(int)x;}
int main(void){
 avp_weapons_set_callbacks(n,s); cur_weps=(1u<<4); cur_wepno=4; new_wepno=0; InitPredAvail();
 score=149999; PredAvail(); assert(cur_weps==(1u<<4));
 score=150000; PredAvail(); assert(cur_weps&(1u<<1)); assert(new_wepno==1); assert(last_notice==AVP_WNOTICE_GAIN_COMBI); assert(last_sfx==2);
 score=800000; PredAvail(); assert(cur_weps&(1u<<3)); assert(cur_weps&(1u<<2)); assert(new_wepno==2);
 cur_wepno=2; score=100000; PredAvail(); assert(!(cur_weps&(1u<<2))); assert(!(cur_weps&(1u<<3))); assert(!(cur_weps&(1u<<1))); assert(new_wepno==3); assert(last_sfx==1);
 return 0;
}
