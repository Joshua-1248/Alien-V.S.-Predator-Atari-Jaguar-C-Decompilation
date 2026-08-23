/* Readable C reconstruction of the 68000-side level-state core in
 * MAZE/LEVELS.S: InitLevels, LeaveLevel, SetStart and FirstPos.
 *
 * Resource transfer/panel/GPU work remains in the platform/resource layer;
 * this file owns the ordinary game-state semantics.
 */
#include "levels.h"
#include "player.h"
#include <stddef.h>

static const AvpLevelInfo *level_table;
static unsigned level_count;
static const u8 *save_data;
static unsigned save_size;

s16 cur_level,new_level;
s32 new_x=-1,new_y;
u32 panel_list[AVP_MAX_PANELS/4];

extern u32 x_pos,y_pos;
extern u32 centre_angle;
extern void SaveDoors(void);
extern void save_level(void);

static u32 be32(const u8 *p)
{
    return ((u32)p[0]<<24)|((u32)p[1]<<16)|((u32)p[2]<<8)|(u32)p[3];
}

void avp_levels_set_table(const AvpLevelInfo *table,unsigned count)
{
    level_table=table; level_count=count;
}
void avp_levels_set_save_words(const u8 *bytes,unsigned size)
{
    save_data=bytes; save_size=size;
}

void FirstPos(void)
{
    const AvpLevelInfo *li;
    new_x=-1;
    if (!level_table || cur_level<1 || (unsigned)cur_level>level_count) return;
    li=&level_table[(unsigned)cur_level-1u];
    x_pos=li->start_x;
    y_pos=li->start_y;
    centre_angle=li->start_angle;
}

void InitLevels(void)
{
    static const s16 starts[3]={3,14,15};
    unsigned pi=(unsigned)player_type>>2;
    unsigned i;
    if (pi>2) pi=0;
    cur_level=starts[pi];
    if (save_data && save_size>=20) {
        u32 packed=be32(save_data+16);
        unsigned saved_level=(packed>>2)&0x0fu;
        if (saved_level>=1 && saved_level<=AVP_MAX_LEVEL) cur_level=(s16)saved_level;
    }
    FirstPos();
    for (i=0;i<AVP_MAX_PANELS/4;++i) panel_list[i]=0xffffffffu;
}

void LeaveLevel(void)
{
    SaveDoors();
    save_level();
    cur_level=new_level;
}

void SetStart(void)
{
    if (new_x>=0) {
        x_pos=(u32)new_x;
        y_pos=(u32)new_y;
        new_x=-1;
    } else {
        /* Retail FORCE_START=1: entering without an explicit connection
         * coordinate falls back to that level's safe start. */
        FirstPos();
    }

    if (save_data && save_size>=4) {
        u32 packed=be32(save_data);
        u32 x=(packed>>10)&0x003ffc00u;
        u32 y=(packed<<2)&0x003ffc00u;
        u32 a=(packed&0xffu)<<24;
        x_pos=x; y_pos=y; centre_angle=a;
    }
}
