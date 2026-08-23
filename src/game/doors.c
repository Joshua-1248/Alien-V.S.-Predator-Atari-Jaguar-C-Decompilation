/* Readable C reconstruction of MAZE/DOORS.S initialization and persistent
 * per-level door-state save/restore logic.
 *
 * Preserves the historical wall-bit table, level-to-level save allocation,
 * 8-byte maze-cell layout and active opening/motion state.  Hardware/world
 * collision and authored access-table queries cross the explicit runtime seam.
 */
#include "doors.h"
#include <stddef.h>
#include <string.h>
#include "player.h"
#include "maze.h"
#include "joypad.h"
#include "avp_runtime.h"

#define CELL 8u
#define LEFT_WALL 0u
#define TOP_WALL 1u
#define LEFT_DOOR 6u
#define TOP_DOOR 7u

u8 fullbits[256];
u8 acs_level;

static u8 *maze_data;
static u16 maze_w,maze_h;
static u8 l2l_area[AVP_L2L_SIZE];
static u16 l2l_used;
static s16 l2l_off[AVP_MAX_LEVEL];

/* Active-door/lift state, represented explicitly instead of raw adjacent BSS. */
static u8 *door1_ptr,*door2_ptr,*lift_ptr;
static s8 door1_dir,door2_dir;
static u8 door1_lim,door2_lim;
static u8 door1_side,door2_side;
static u8 dbl_door,comp_panel,lift_dir;
u8 lift_key;

static const u8 init_wbits[16]={
    AVP_WB_SOLID,AVP_WB_SOLID,AVP_WB_SOLID,AVP_WB_SOLID,
    AVP_WB_DOOR|AVP_WB_2SIDES,
    AVP_WB_DOOR,
    AVP_WB_DOOR,
    AVP_WB_DOOR|AVP_WB_TBDOOR|AVP_WB_EDGEDOOR,
    AVP_WB_DOOR|AVP_WB_TBDOOR,
    AVP_WB_DOOR|AVP_WB_TBDOOR|AVP_WB_EDGEDOOR|AVP_WB_LEFTONLY,
    AVP_WB_DOOR|AVP_WB_TBDOOR|AVP_WB_EDGEDOOR|AVP_WB_2SIDES,
    AVP_WB_DOOR|AVP_WB_TBDOOR|AVP_WB_2SIDES,
    0,AVP_WB_LEFTONLY,AVP_WB_LEFTONLY,AVP_WB_2SIDES
};

extern s16 cur_level;

void avp_doors_bind_maze(u8 *maze,u16 width,u16 height)
{
    maze_data=maze; maze_w=width; maze_h=height;
}

void InitAccess(void) { acs_level=(u8)(player_type==PT_HUMAN ? 0 : 10); }
void ResetAccess(void) { /* access level persists across levels */ }

void InitDoors(void)
{
    unsigned type,n;
    for (type=0;type<16;++type) {
        for (n=0;n<8;++n) {
            fullbits[type*8+n]=init_wbits[type];
            fullbits[0x80u+type*8+n]=init_wbits[type];
        }
    }
    fullbits[0]=0; fullbits[0x80]=0;
    for (n=0;n<AVP_MAX_LEVEL;++n) l2l_off[n]=-1;
    l2l_used=0;
    door1_ptr=door2_ptr=lift_ptr=NULL;
    door1_dir=door2_dir=0;door1_lim=door2_lim=0;door1_side=door2_side=0;
    dbl_door=comp_panel=lift_key=lift_dir=0;
}

static unsigned door_byte_count(void)
{
    unsigned cells=(unsigned)(maze_w)*(unsigned)(maze_h);
    unsigned i,count=0;
    if (!maze_data) return 0;
    for (i=0;i<cells;++i) {
        const u8 *c=maze_data+i*CELL;
        if (fullbits[c[LEFT_WALL]]&AVP_WB_DOOR) ++count;
        if (fullbits[c[TOP_WALL]]&AVP_WB_DOOR) ++count;
    }
    return count;
}

void SaveDoors(void)
{
    unsigned i,cells,need,pos;
    u8 *out;
    if (!maze_data || cur_level<1 || cur_level>AVP_MAX_LEVEL) return;
    need=door_byte_count();
    if (l2l_off[cur_level-1]<0) {
        if ((unsigned)l2l_used+need>AVP_L2L_SIZE) return;
        l2l_off[cur_level-1]=(s16)l2l_used;
        l2l_used=(u16)(l2l_used+need);
    }
    out=l2l_area+(unsigned)l2l_off[cur_level-1];
    cells=(unsigned)(maze_w)*(unsigned)(maze_h);
    pos=0;
    for (i=0;i<cells;++i) {
        u8 *c=maze_data+i*CELL;
        if (fullbits[c[LEFT_WALL]]&AVP_WB_DOOR) out[pos++]=c[LEFT_DOOR];
        if (fullbits[c[TOP_WALL]]&AVP_WB_DOOR) out[pos++]=c[TOP_DOOR];
    }
}

void ResetDoors(void)
{
    unsigned i,cells,pos=0;
    const u8 *in=NULL;
    if (maze_data && cur_level>=1 && cur_level<=AVP_MAX_LEVEL && l2l_off[cur_level-1]>=0)
        in=l2l_area+(unsigned)l2l_off[cur_level-1];
    if (in) {
        cells=(unsigned)(maze_w)*(unsigned)(maze_h);
        for (i=0;i<cells;++i) {
            u8 *c=maze_data+i*CELL;
            if (fullbits[c[LEFT_WALL]]&AVP_WB_DOOR) c[LEFT_DOOR]=in[pos++];
            if (fullbits[c[TOP_WALL]]&AVP_WB_DOOR) c[TOP_DOOR]=in[pos++];
        }
    }
    door1_ptr=door2_ptr=NULL;
    door1_dir=door2_dir=0;door1_lim=door2_lim=0;door1_side=door2_side=0;
    dbl_door=comp_panel=lift_key=lift_dir=0;
    lift_ptr=NULL;
}


#define DOOR_STEP 4
#define DOOR_LIM_DEFAULT 48
#define D2_SPECIAL (AVP_WB_LEFTONLY|AVP_WB_EDGEDOOR|AVP_WB_TBDOOR)
static int is_pressed(u32 v,unsigned bit){return (v&(1u<<bit))==0u;}
static unsigned facing_side(void)
{
    u32 d=(u32)(0u-centre_angle);d=(d<<3)|(d>>29);
    u16 w=(u16)d;w=(u16)(w+5u);w=(u16)((w>>1)|(w<<15));return w&3u;
}
static u8 door_limit(u8 panel)
{
    static const u8 type_default[16]={48,48,48,48,56,48,52,112,60,92,92,48,48,48,48,48};
    static const u8 type_alt[16]={48,48,48,48,56,48,56,112,60,100,100,48,48,48,48,48};
    unsigned type=(panel&0x78u)>>3,idx=panel&7u;
    return idx?type_alt[type]:type_default[type];
}
static u8 *door_byte_for(u8 *cell,unsigned side)
{
    size_t row=(size_t)(maze_w)*CELL;
    switch(side&3u){case 0:return cell+LEFT_DOOR;case 1:return cell+TOP_DOOR;case 2:return cell+CELL+LEFT_DOOR;default:return cell+row+TOP_DOOR;}
}
static int access_ok(u8 *door,u8 panel)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    if(*door!=0)return 1; /* source always allows closing an open door */
    if(o->door_access){u32 off=(u32)(door-maze_data);return o->door_access(o->user,off,panel,acs_level)==0;}
    return 1;
}
static int install_door(u8 *ptr,u8 panel,unsigned side,int slot)
{
    s8 dir=(s8)(*ptr? -DOOR_STEP:DOOR_STEP);u8 lim=door_limit(panel);
    if(slot==1){door1_ptr=ptr;door1_dir=dir;door1_lim=lim;door1_side=(u8)side;}
    else {door2_ptr=ptr;door2_dir=dir;door2_lim=lim;door2_side=(u8)side;}
    return 1;
}
void DoorKeys(void)
{
    u32 key=lift_key?joy_cur:joy_edge;unsigned side;size_t gx,gy,rowidx;u8 *cell,*panelp,*door;u8 panel,flags;
    if(!maze_data)return;
    if(!is_pressed(key,FIRE_B)){if(lift_key)lift_key=0;return;}
    gx=x_pos>>16;gy=y_pos>>16;if(gx>=(size_t)(maze_w)||gy>=(size_t)(maze_h))return;
    rowidx=gy*(size_t)(maze_w)+gx;cell=maze_data+rowidx*CELL;side=facing_side();panelp=cell+side;panel=*panelp;
    if(!panel){lift_key=0;return;}
    flags=fullbits[panel];
    if(!(flags&AVP_WB_DOOR)){
        /* Computer/lift/duct panels are non-door special walls.  Keep the
         * decision in 68000 C and delegate the actual terminal/lift action. */
        const AvpRuntimeOps *o=avp_runtime_ops();lift_key=1;comp_panel=panel;
        if(o->object_event)o->object_event(o->user,0x444f4f52u,(u32)(panelp-maze_data),panel,(u32)side);
        return;
    }
    door=door_byte_for(cell,side);
    if(door1_ptr==door){door1_dir=(s8)-door1_dir;return;}if(door2_ptr==door){door2_dir=(s8)-door2_dir;return;}
    if(!access_ok(door,panel)){const AvpRuntimeOps *o=avp_runtime_ops();if(o->set_message)o->set_message(o->user,6);return;}
    if((flags&D2_SPECIAL)==D2_SPECIAL){
        ptrdiff_t step;size_t row=(size_t)(maze_w)*CELL;
        if(door1_ptr||door2_ptr)return;
        switch(side){case 0:step=-(ptrdiff_t)row;break;case 1:step=CELL;break;case 2:step=(ptrdiff_t)row;break;default:step=-CELL;break;}
        if(panel&0x80u)step=-step;
        install_door(door,panel,side,1);install_door(door+step,panel,side,2);dbl_door=0xffu;
        {const AvpRuntimeOps *o=avp_runtime_ops();if(o->play_sfx)o->play_sfx(o->user,12);}
        return;
    }
    if(!door1_ptr)install_door(door,panel,side,1);else if(!door2_ptr)install_door(door,panel,side,2);else return;
    {const AvpRuntimeOps *o=avp_runtime_ops();if(o->play_sfx)o->play_sfx(o->user,11);}
    lift_key=0;
}
void MoveDoors(void)
{
    /* Keep the source's byte-step/limit semantics.  The Jaguar implementation
     * additionally checks player/AMP collision and reverses a closing door; host
     * collision backends can model that while normal movement remains identical. */
    if(door1_ptr){u8 *p=door1_ptr;int v=(int)*p+(int)door1_dir;if(v<=0){*p=0;door1_ptr=NULL;}else if(v>=door1_lim){*p=door1_lim;door1_ptr=NULL;}else *p=(u8)v;}
    if(door2_ptr){u8 *p=door2_ptr;int v=(int)*p+(int)door2_dir;if(v<=0){*p=0;door2_ptr=NULL;dbl_door=0;}else if(v>=door2_lim){*p=door2_lim;door2_ptr=NULL;dbl_door=0;}else *p=(u8)v;}
}
