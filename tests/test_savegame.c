#include "savegame.h"
#include "eeprom.h"
#include "maze.h"
#include "player.h"
#include "collectables.h"
#include "weapons.h"
#include "levels.h"
#include "hud.h"
#include <assert.h>
#include <string.h>

/* Minimal game-state definitions needed by savegame.c. */
u32 x_pos,y_pos,centre_angle;
s16 player_type,player_energy;
s32 score;
AvpAmmoInfo ammo_info[6];
s16 show_mt;
u8 acs_level;
u8 cur_weps,old_weps;
s16 cur_level,new_level;
s32 new_x,new_y;
AvpCocoonState cocoon_data[AVP_MAX_COCOONS];

static u32 be32(const u8*p){return ((u32)p[0]<<24)|((u32)p[1]<<16)|((u32)p[2]<<8)|p[3];}
static u16 be16(const u8*p){return (u16)(((u16)p[0]<<8)|p[1]);}
static u32 rol32(u32 v,unsigned n){return (v<<n)|(v>>(32u-n));}
static u32 encoded_cocoon(const AvpCocoonState*c){u16 f=(u16)c->frame&31u;u16 r=(u16)((f>>5)|(u16)(f<<11));return ((c->x&0x3f0000u)<<10)|((c->y&0x3f0000u)<<4)|(((u32)c->level&15u)<<16)|r;}

int main(void)
{
    memset(cartcopy,0,sizeof(cartcopy));memset(SaveCont,0,sizeof(SaveCont));
    x_pos=0x00128000u;y_pos=0x00098000u;centre_angle=0x43000000u;
    score=-12345;player_energy=777;acs_level=9;show_mt=-1;cur_weps=0x2au;cur_level=7;
    player_type=PT_HUMAN;ammo_info[0].cur=11;ammo_info[1].cur=22;ammo_info[2].cur=33;ammo_info[3].cur=44;
    MakeSave(1);
    {u8*p=cartcopy+AVP_SAVE_SIZE;
     u32 pos=((x_pos&0x003ffc00u)<<10)|((y_pos&0x003ffc00u)>>2)|((rol32(centre_angle,8))&0xffu);
     assert(be32(p)==pos);assert(be32(p+4)==(u32)score);
     assert(be16(p+8)==11&&be16(p+10)==22&&be16(p+12)==33&&be16(p+14)==44);
     assert(be32(p+16)==(((u32)(u16)player_energy<<16)|((u32)acs_level<<12)|(1u<<11)|((u32)(cur_weps&0x3eu)<<5)|((u32)cur_level<<2)|0u));}

    player_type=PT_PREDATOR;ammo_info[5].cur=999;MakeSave(2);
    {u8*p=cartcopy+2*AVP_SAVE_SIZE;assert(be16(p+8)==999);assert(be16(p+10)==0);assert(be32(p+12)==0);assert((be32(p+16)&3u)==2u);}

    player_type=PT_ALIEN;
    cocoon_data[0]=(AvpCocoonState){-1,1,3,0x00128000u,0x00098000u};
    cocoon_data[1]=(AvpCocoonState){17,1,4,0x00258000u,0x00118000u};
    cocoon_data[2]=(AvpCocoonState){-2,0,5,0x00308000u,0x00228000u};
    MakeSave(-1);
    {u32 e0=encoded_cocoon(&cocoon_data[0]),e1=encoded_cocoon(&cocoon_data[1]),e2=encoded_cocoon(&cocoon_data[2]);
     assert(be32(SaveCont+8)==(e0|(e1>>21)));
     assert(be32(SaveCont+12)==(((e1&0x001ff800u)<<11)|(e2>>10)));
     assert((be32(SaveCont+16)&3u)==1u);}
    return 0;
}
