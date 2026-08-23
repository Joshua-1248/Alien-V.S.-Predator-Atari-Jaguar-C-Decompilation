#include <assert.h>
#include <string.h>
#include "collision.h"
#include "amp.h"
#include "player.h"
#include "levels.h"

/* globals required by collision/player-facing link */
u32 x_pos,y_pos,centre_angle,fire_distance; s16 player_energy,max_energy; u16 maze_width,maze_height;
AvpAmp level1amps[AVP_NUM_AMPS],*amp_data=level1amps,*amps_at=level1amps;
u16 collmap[AVP_AMP_GRID_W*AVP_AMP_GRID_H]; u8 objmap[AVP_AMP_OBJ_ROW_BYTES*AVP_AMP_OBJ_H];
u16 levels_visit,discflag; s16 cur_level; u8 fullbits[256];

int main(void){
    /* 4x4 maze, matching the historical width-as-row-stride layout. */
    u8 maze[4*4*8]; memset(maze,0,sizeof maze); memset(fullbits,0,sizeof fullbits); fullbits[1]=1; avp_collision_bind_maze(maze,4,4);
    maze_width=4;maze_height=4;
    /* Open room is safe. */
    assert(SafePos(64,(1<<16)|0x8000,(1<<16)|0x8000,10)==AVP_COLL_SAFE);
    /* solid left wall type 1 catches near-left position */
    maze[(1*4+1)*8+0]=1;
    assert(SafePos(64,(1<<16)|0x0100,(1<<16)|0x8000,10)==AVP_COLL_WALL);
    maze[(1*4+1)*8+0]=0;

    /* quick path movement: wall number 1 blocks, 0 and >=0x20 pass */
    maze[(1*4+1)*8+2]=1;
    assert((AllowedMoves(1,1)&(1u<<2))==0);
    maze[(1*4+1)*8+2]=0x20;
    assert(AllowedMoves(1,1)&(1u<<2));

    /* AMP collision uses one-based collmap IDs and source collision widths. */
    memset(collmap,0,sizeof collmap); memset(level1amps,0,sizeof level1amps);
    level1amps[0].creature=AC_ALIEN; level1amps[0].xpos=(1<<16)|0x8000; level1amps[0].ypos=(1<<16)|0x8000;
    collmap[1*AVP_AMP_GRID_W+1]=1;
    assert(AMPCollisions((1<<16)|0x8000,(1<<16)|0x8000));
    assert(!AMPCollisions((3<<16)|0x8000,(3<<16)|0x8000));

    /* LOS in an open room succeeds; a solid right wall blocks crossing. */
    { AvpXY a={(1<<16)|0x8000,(1<<16)|0x8000},b={(2<<16)|0x8000,(1<<16)|0x8000};
      assert(LineOfSight(&a,&b)==0);
      maze[(1*4+1)*8+2]=1;
      assert(LineOfSight(&a,&b)!=0);
    }
    return 0;
}
