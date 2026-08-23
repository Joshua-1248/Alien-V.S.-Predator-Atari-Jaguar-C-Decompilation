#include "levels.h"
#include "doors.h"
#include "player.h"
#include "amp.h"
#include <assert.h>
#include <string.h>
#include "hud_message.h"
#include "joypad.h"

u32 new_pos,levfx_ID;
const AvpHudMessageStep avp_msg_pressure[]={{0,{0,0,0}}};
const AvpHudMessageStep avp_msg_notsecure[]={{0,{0,0,0}}};
const AvpHudMessageStep avp_msg_airlocked[]={{0,{0,0,0}}};
const AvpHudMessageStep avp_msg_jammed[]={{0,{0,0,0}}};
const AvpHudMessageStep avp_msg_jlift[]={{0,{0,0,0}}};
const AvpHudMessageStep avp_msg_escape[]={{0,{0,0,0}}};
const AvpHudMessageStep avp_msg_access_denied[]={{0,{0,0,0}}};
void avp_hudmsg_queue(const AvpHudMessageStep *p){(void)p;}
void avp_hudmsg_set_access_code(unsigned n){(void)n;}
u32 DoEffect(unsigned id){return id;}
u32 x_pos,y_pos,centre_angle; s16 player_type; uintptr_t savegame;
volatile u32 joy_cur=0xffffffffu,joy_edge=0xffffffffu;
AvpAmp level1amps[AVP_NUM_AMPS]; AvpAmp *amps_at=level1amps,*amp_data=level1amps;
u16 collmap[AVP_AMP_GRID_W*AVP_AMP_GRID_H];

u32 ccn_xsave,ccn_ysave;
u8 in_title,in_select;
void WaitFXFa(void){}
void StopMusi(void){}
void ResetScale(void){}
void ResetSprites(void){}
void restore_level(void){}
void ResetPlayer(void){}
u8 *avp_collision_maze_data(void){return 0;}
u16 avp_collision_maze_width(void){return 0;}
u16 avp_collision_maze_height(void){return 0;}

void save_level(void){}; 
int main(void){
 AvpLevelInfo lv[15]; u8 maze[4*4*8]; unsigned i;
 memset(lv,0,sizeof(lv)); lv[2].start_x=0x111000; lv[2].start_y=0x222000; lv[2].start_angle=0x40000000;
 avp_levels_set_table(lv,15); avp_levels_set_save_words(0,0); player_type=PT_HUMAN; InitLevels();
 assert(cur_level==3); assert(x_pos==0x111000); assert(y_pos==0x222000);
 /* MAIN.S passes a real slot pointer in savegame; LEVELS.S reads it directly. */
 { u8 slot[20]={0}; u32 packed=(5u<<2); slot[16]=(u8)(packed>>24);slot[17]=(u8)(packed>>16);slot[18]=(u8)(packed>>8);slot[19]=(u8)packed;
   savegame=(uintptr_t)slot; InitLevels(); assert(cur_level==5); savegame=0; }
 InitDoors(); assert(fullbits[0]==0); assert(fullbits[0x20]&AVP_WB_DOOR); assert(fullbits[0xa0]&AVP_WB_DOOR);
 player_type=PT_HUMAN; InitAccess(); assert(acs_level==0);
 player_type=PT_ALIEN; InitAccess(); assert(acs_level==10);
 player_type=PT_PREDATOR; InitAccess(); assert(acs_level==10);
 player_type=PT_HUMAN;
 memset(maze,0,sizeof(maze)); avp_doors_bind_maze(maze,2,2); cur_level=3;
 /* cell 0: centre side door wall id 0x20, opening state 13 */
 maze[0]=0x20; maze[6]=13; SaveDoors(); maze[6]=3; ResetDoors(); assert(maze[6]==13);
 for(i=0;i<AVP_MAX_PANELS;i++) assert(panel_list[i]==0xffu);

 /* DOORS.S DoorKeys local `move_doors` label is only an exit path; the
  * exported MoveDoors:: advances the byte once later in UpdatePlayer. */
 memset(maze,0,sizeof(maze)); avp_doors_bind_maze(maze,2,2); cur_level=3;
 new_pos=0; x_pos=0; y_pos=0; centre_angle=0; /* facing RIGHT (side 2) */
 maze[2]=0x20; /* ordinary door panel on right side of cell 0 */
 joy_edge=0xffffffffu & ~(1u<<FIRE_B); joy_cur=0xffffffffu;
 DoorKeys();
 assert(maze[8+6]==0); /* selection/reversal only: no physical step yet */
 MoveDoors();
 assert(maze[8+6]==4);
 joy_edge=joy_cur=0xffffffffu;
 return 0;
}
