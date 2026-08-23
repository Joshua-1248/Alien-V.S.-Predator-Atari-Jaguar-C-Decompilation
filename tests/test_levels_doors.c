#include "levels.h"
#include "doors.h"
#include "player.h"
#include "amp.h"
#include <assert.h>
#include <string.h>
u32 x_pos,y_pos,centre_angle; s16 player_type; u32 savegame;
volatile u32 joy_cur=0xffffffffu,joy_edge=0xffffffffu;
AvpAmp level1amps[AVP_NUM_AMPS]; AvpAmp *amps_at=level1amps,*amp_data=level1amps;
u16 collmap[AVP_AMP_GRID_W*AVP_AMP_GRID_H];
u8 *avp_collision_maze_data(void){return 0;}
u16 avp_collision_maze_width(void){return 0;}
u16 avp_collision_maze_height(void){return 0;}

void save_level(void){}; 
int main(void){
 AvpLevelInfo lv[15]; u8 maze[4*4*8]; unsigned i;
 memset(lv,0,sizeof(lv)); lv[2].start_x=0x111000; lv[2].start_y=0x222000; lv[2].start_angle=0x40000000;
 avp_levels_set_table(lv,15); avp_levels_set_save_words(0,0); player_type=PT_HUMAN; InitLevels();
 assert(cur_level==3); assert(x_pos==0x111000); assert(y_pos==0x222000);
 InitDoors(); assert(fullbits[0]==0); assert(fullbits[0x20]&AVP_WB_DOOR); assert(fullbits[0xa0]&AVP_WB_DOOR);
 player_type=PT_HUMAN; InitAccess(); assert(acs_level==0);
 player_type=PT_ALIEN; InitAccess(); assert(acs_level==10);
 player_type=PT_PREDATOR; InitAccess(); assert(acs_level==10);
 player_type=PT_HUMAN;
 memset(maze,0,sizeof(maze)); avp_doors_bind_maze(maze,2,2); cur_level=3;
 /* cell 0: centre side door wall id 0x20, opening state 13 */
 maze[0]=0x20; maze[6]=13; SaveDoors(); maze[6]=3; ResetDoors(); assert(maze[6]==13);
 for(i=0;i<12;i++) assert(panel_list[i]==0xffffffffu);
 return 0;
}
