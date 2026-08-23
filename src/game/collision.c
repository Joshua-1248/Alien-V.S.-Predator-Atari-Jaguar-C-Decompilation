/* Readable C translation of the ordinary-68000 parts of MAZE/COLLIDE.S.
 *
 * The Jaguar source uses 16.16 world coordinates, 8-byte maze cells and a
 * 16-bit collision map containing one-based AMP numbers.  This translation
 * keeps those data shapes and the original wall/door tests rather than
 * replacing them with a generic host physics engine.
 */
#include "collision.h"
#include "doors.h"
#include "maze.h"
#include "amp.h"
#include "player.h"
#include <stddef.h>
#include <stdint.h>

#define CELL_SIZE      8u
#define LEFT_WALL      0u
#define TOP_WALL       1u
#define RIGHT_WALL     2u
#define BOTTOM_WALL    3u
#define LEFT_DOOR      6u
#define TOP_DOOR       7u
#define HIT_WALL       1u
#define HIT_DOOR       2u
#define CDOOR_LIM      16u
#define BULK_LIM       14u

static const u16 atantab[513]={
#include "atantab.inc"
};

static u8 *maze_data;
static u16 bound_w,bound_h;

void avp_collision_bind_maze(u8 *maze,u16 width,u16 height)
{
    maze_data=maze; bound_w=width; bound_h=height;
}
u8 *avp_collision_maze_data(void){return maze_data;}
u16 avp_collision_maze_width(void){return bound_w;}
u16 avp_collision_maze_height(void){return bound_h;}

static u32 abs32s(s32 v) { return v < 0 ? (u32)(-(s64)v) : (u32)v; }
static void normalize_pair(u32 *x,u32 *y) {
    while (((*x)&0x40000000u)==0u && ((*y)&0x40000000u)==0u) {
        *x <<= 1; *y <<= 1;
        if (!*x && !*y) break;
    }
    *x >>= 1;
}

u16 Angle(const AvpXY *from,const AvpXY *to) {
    s32 sx=to->x-from->x, sy=to->y-from->y;
    u32 x,y; u16 invert=0,base=0;
    if ((sx|sy)==0) return 0;
    x=abs32s(sx); y=abs32s(sy);
    if (sx<0) { invert=0xffffu; base=0x8000u; }
    if (sy<0) { invert^=0xffffu; base=(u16)(0u-0x4000u); }
    normalize_pair(&x,&y);
    if (x<y) { u32 t=x;x=y;y=t; base=(sy<0)?0xc000u:0x4000u; invert^=0xffffu; }
    if ((x>>16)==0) return base;
    {
        u16 idx=(u16)(((y>>7)/(x>>16))&0xffffu);
        u16 a=atantab[idx]; a=(u16)((a^invert)-invert); return (u16)(a+base);
    }
}

void Vector(const AvpXY *from,const AvpXY *to,s32 *vx,s32 *vy) {
    s32 sx=to->x-from->x, sy=to->y-from->y;
    u32 x,y,xx,yy,den; int negx=sx<0,negy=sy<0;
    if ((sx|sy)==0) { *vx=0x4000; *vy=0; return; }
    x=abs32s(sx); y=abs32s(sy); normalize_pair(&x,&y);
    xx=x>>16; yy=y>>16; den=(((xx*xx)+(yy*yy))>>16)+0x3000u; if (!den) den=1;
    x=(x>>2)/den; y=(y>>2)/den;
    *vx=negx?-(s32)(u16)x:(s32)(u16)x; *vy=negy?-(s32)(u16)y:(s32)(u16)y;
}

static size_t row_cells(void) { return (size_t)bound_w+2u; }
static int inside_cell(unsigned x,unsigned y)
{
    return maze_data && x<row_cells() && y<(size_t)bound_h+2u;
}
static u8 *cell_at(unsigned x,unsigned y)
{
    return maze_data + ((size_t)y*row_cells()+x)*CELL_SIZE;
}

/* Convert a wall-slot address (cell+0..3) to the matching door-open byte.
 * This is the C form of the initialized door_tab[] used by check_wall. */
static const u8 *door_for_wall(const u8 *wall)
{
    uintptr_t off=(uintptr_t)(wall-maze_data);
    unsigned slot=(unsigned)(off & 3u);
    const u8 *base=wall-slot;
    size_t row=row_cells()*CELL_SIZE;
    switch(slot) {
    case LEFT_WALL:   return base+LEFT_DOOR;
    case TOP_WALL:    return base+TOP_DOOR;
    case RIGHT_WALL:  return base+CELL_SIZE+LEFT_DOOR;
    default:          return base+row+TOP_DOOR;
    }
}

/* Source check_wall. `coord` is the 16-bit fractional coordinate on the wall
 * (one cell == 0x10000); `height` is in sprite pixels. */
static u8 check_wall(const u8 *wall,u16 coord,u8 height)
{
    u8 wallno=*wall,flags,pos,open;
    if (!wallno) return 0;

    /* asr.b #7,d7 / eor.b d7,d1 reflects the low byte when bit 7 of the
     * wall number is set. Then >>8 and >>1 converts 16.16 fraction to 0..127. */
    pos=(u8)(coord>>9);
    if (wallno&0x80u) pos^=0x7fu;
    flags=fullbits[wallno];
    if (flags&AVP_WB_SOLID) return HIT_WALL;
    if ((flags&AVP_WB_LEFTONLY) && pos<BULK_LIM) return HIT_WALL;
    if (flags&AVP_WB_2SIDES) {
        unsigned edge=(pos<64u)?pos:(127u-pos);
        if (edge<BULK_LIM) return HIT_WALL;
    }
    if (!(flags&AVP_WB_DOOR)) return 0;

    open=*door_for_wall(wall);
    if (!(flags&AVP_WB_TBDOOR)) {
        if (flags&AVP_WB_EDGEDOOR) {
            unsigned edge=128u+CDOOR_LIM-(unsigned)open;
            return pos<=edge ? HIT_DOOR:0;
        } else {
            unsigned lo=64u+CDOOR_LIM-(unsigned)open;
            int hi=64-(int)CDOOR_LIM+(int)open;
            return (pos<=lo || (int)pos>=hi) ? HIT_DOOR:0;
        }
    }
    if (flags&AVP_WB_EDGEDOOR) return height>=open ? HIT_DOOR:0;
    {
        int lo=64-(int)open,hi=64+(int)open;
        return ((int)height<=lo || (int)height>=hi) ? HIT_DOOR:0;
    }
}

static int outside_solid(u8 wallno,int from_left)
{
    u8 f=fullbits[wallno];
    if (f&(AVP_WB_SOLID|AVP_WB_2SIDES)) return 1;
    if (!(f&AVP_WB_LEFTONLY)) return 0;
    return from_left ? ((wallno&0x80u)==0u) : ((wallno&0x80u)!=0u);
}

u8 SafePos(u8 height,s32 xpos,s32 ypos,u16 width)
{
    unsigned cx=(u32)xpos>>16,cy=(u32)ypos>>16;
    u16 fx=(u16)xpos,fy=(u16)ypos;
    u16 near=(u16)(width<<9),far=(u16)~near;
    u8 lr=0,tb=0,r;
    u8 *c,*above,*below;
    size_t row;
    if (!inside_cell(cx,cy)) return HIT_WALL;
    c=cell_at(cx,cy); row=row_cells()*CELL_SIZE;
    above=(cy?c-row:c); below=(cy+1u<(size_t)bound_h+2u?c+row:c);

    if (fx<near) {
        if (fy<near && outside_solid(above[LEFT_WALL],1)) return HIT_WALL;
        if (fy>far && outside_solid(below[LEFT_WALL],0)) return HIT_WALL;
        r=check_wall(c+LEFT_WALL,(u16)~fy,height); if(r==HIT_WALL)return r; lr=r;
    }
    if (fy<near) {
        /* The assembly addresses TOP_WALL-MAZE_BLOCK_SIZE(a4), i.e. the
         * top wall slot of the cell immediately to the left. */
        if (fx<near && cx && outside_solid((c-CELL_SIZE)[TOP_WALL],0)) return HIT_WALL;
        if (fx>far && inside_cell(cx+1u,cy) && outside_solid((c+CELL_SIZE)[TOP_WALL],1)) return HIT_WALL;
        r=check_wall(c+TOP_WALL,fx,height); if(r==HIT_WALL)return r; tb=r;
    }
    if (fx>far) {
        if (fy<near && outside_solid(above[RIGHT_WALL],0)) return HIT_WALL;
        if (fy>far && outside_solid(below[RIGHT_WALL],1)) return HIT_WALL;
        r=check_wall(c+RIGHT_WALL,fy,height); if(r==HIT_WALL)return r; lr=r;
    }
    if (fy>far) {
        if (fx<near && cx && outside_solid((c-CELL_SIZE)[BOTTOM_WALL],1)) return HIT_WALL;
        if (fx>far && inside_cell(cx+1u,cy) && outside_solid((c+CELL_SIZE)[BOTTOM_WALL],0)) return HIT_WALL;
        r=check_wall(c+BOTTOM_WALL,(u16)~fx,height); if(r==HIT_WALL)return r; tb=r;
    }
    return (u8)(lr|tb);
}

u8 AllowedMoves(u16 x,u16 y)
{
    u8 m=0,*c;
    if(!inside_cell(x,y))return 0;
    c=cell_at(x,y);
    /* Historical quick path: wall numbers 1..31 block; all others, including
     * closed doors, are considered traversable by this coarse path finder. */
    for(unsigned i=0;i<4;i++) if(c[i]==0 || c[i]>=0x20u) m|=(u8)(1u<<i);
    return m;
}

u16 NMoves(u16 x,u16 y,u8 dir,u16 wanted)
{
    u16 n=0;
    while(n<wanted && inside_cell(x,y)) {
        u8 *c=cell_at(x,y),w=c[dir&3u];
        if(w>=1u && w<=0x1fu)break;
        switch(dir&3u){case 0:if(!x)return n;--x;break;case 1:if(!y)return n;--y;break;case 2:++x;break;default:++y;break;}
        ++n;
    }
    return n;
}

static const s32 coll_widths[40]={
    (50<<9)*85/100,(50<<9)*85/100,(50<<9)*85/100,
    (40<<9)*85/100,(40<<9)*85/100,50<<9,
    0,0,0,0,0,
    (40<<9)*85/100,(40<<9)*85/100,(40<<9)*85/100,(40<<9)*85/100,(40<<9)*85/100,(40<<9)*85/100,
    (10<<9)*85/100,(40<<9)*85/100,(40<<9)*85/100,(35<<9)*85/100,(40<<9)*85/100,(35<<9)*85/100,
    (40<<9)*85/100,(40<<9)*85/100,(40<<9)*85/100,(40<<9)*85/100,(40<<9)*85/100,(40<<9)*85/100,
    0,0,0,0,0,(50<<9)*85/100,(40<<9)*85/100,(40<<9)*85/100,(80<<9)*85/100,(40<<9)*85/100,(80<<9)*85/100
};

int AMPCollisions(s32 xpos,s32 ypos)
{
    int cx=(int)((u32)xpos>>16),cy=(int)((u32)ypos>>16);
    for(int oy=-1;oy<=1;oy++) for(int ox=-1;ox<=1;ox++) {
        int x=cx+ox,y=cy+oy; u16 n; AvpAmp *a; s32 w,dx,dy;
        if(x<0||y<0||x>=AVP_AMP_GRID_W||y>=AVP_AMP_GRID_H)continue;
        n=collmap[y*AVP_AMP_GRID_W+x]; if(!n||n>AVP_NUM_AMPS)continue;
        a=&amp_data[n-1u]; if(a->creature>=40u)continue; w=coll_widths[a->creature]; if(!w)continue;
        dx=a->xpos-xpos;if(dx<0)dx=-dx;dy=a->ypos-ypos;if(dy<0)dy=-dy;
        if(dx<w&&dy<w)return 1;
    }
    return 0;
}

/* A conservative source-shaped LOS walker.  It follows cell-boundary crossings
 * in parameter order and tests the exact wall slot/door aperture at each
 * crossing.  Return 0 means visible, matching the 68000 routine. */
int LineOfSight(const AvpXY *from,const AvpXY *to)
{
    int x=(int)((u32)from->x>>16),y=(int)((u32)from->y>>16);
    int ex=(int)((u32)to->x>>16),ey=(int)((u32)to->y>>16);
    int sx=(to->x>=from->x)?1:-1,sy=(to->y>=from->y)?1:-1;
    int64_t dx=(int64_t)to->x-from->x,dy=(int64_t)to->y-from->y;
    uint64_t adx=dx<0?(uint64_t)-dx:(uint64_t)dx,ady=dy<0?(uint64_t)-dy:(uint64_t)dy;
    uint64_t tx_num,ty_num;
    if(!inside_cell((unsigned)x,(unsigned)y)||!inside_cell((unsigned)ex,(unsigned)ey))return 1;
    tx_num = adx ? (sx>0 ? (uint64_t)(0x10000u-(u16)from->x) : (uint64_t)(u16)from->x) : UINT64_MAX;
    ty_num = ady ? (sy>0 ? (uint64_t)(0x10000u-(u16)from->y) : (uint64_t)(u16)from->y) : UINT64_MAX;
    while(x!=ex||y!=ey) {
        /* Compare tx_num/adx vs ty_num/ady without floating point. */
        int cross_x;
        if(!adx)cross_x=0; else if(!ady)cross_x=1; else cross_x=(tx_num*ady<=ty_num*adx);
        if(cross_x) {
            u8 *c=cell_at((unsigned)x,(unsigned)y); u16 coord;
            uint64_t tnum=tx_num;
            int64_t yy=(int64_t)from->y + (dy*(int64_t)tnum)/(int64_t)adx;
            coord=(u16)yy;
            if(check_wall(c+(sx>0?RIGHT_WALL:LEFT_WALL),coord,64u))return 1;
            x+=sx;tx_num+=0x10000u;
        } else {
            u8 *c=cell_at((unsigned)x,(unsigned)y); u16 coord;
            uint64_t tnum=ty_num;
            int64_t xx=(int64_t)from->x + (dx*(int64_t)tnum)/(int64_t)ady;
            coord=(u16)xx;
            if(check_wall(c+(sy>0?BOTTOM_WALL:TOP_WALL),(sy>0?(u16)~coord:coord),64u))return 1;
            y+=sy;ty_num+=0x10000u;
        }
        if(x<0||y<0||!inside_cell((unsigned)x,(unsigned)y))return 1;
    }
    return 0;
}

static s32 rough_dist(s32 ax,s32 ay,s32 bx,s32 by)
{
    s32 dx=ax-bx,dy=ay-by;if(dx<0)dx=-dx;if(dy<0)dy=-dy;
    if(dx>dy){s32 t=dx;dx=dy;dy=t;}return dy+(dx>>2);
}

void AreaAmpDamage(s32 xpos,s32 ypos,s32 radius,s16 damage,u16 damage_flags)
{
    AvpXY centre={xpos,ypos};
    for(unsigned i=0;i<AVP_NUM_AMPS;i++) {
        AvpAmp *a=&amp_data[i];AvpXY p={a->xpos,a->ypos};
        if(!a->mode && !a->host_static)continue;
        if(!(a->flags&(1u<<AMP_KILLABLE)))continue;
        if(rough_dist(a->xpos,a->ypos,xpos,ypos)>radius)continue;
        if(LineOfSight(&p,&centre))continue;
        a->energy=(s16)(a->energy-damage);a->flags|=damage_flags;
    }
}

void AreaPlDamage(s32 xpos,s32 ypos,s32 radius,s16 damage)
{
    AvpXY p={(s32)x_pos,(s32)y_pos},centre={xpos,ypos};
    if(rough_dist((s32)x_pos,(s32)y_pos,xpos,ypos)>radius)return;
    if(LineOfSight(&p,&centre))return;
    player_energy=(s16)(player_energy-damage);
}

void AreaDamage(s32 xpos,s32 ypos,s32 radius,s16 damage,u16 damage_flags)
{
    AreaPlDamage(xpos,ypos,radius,damage);AreaAmpDamage(xpos,ypos,radius,damage,damage_flags);
}
