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
#include "mazescrn.h"
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
static const u16 tantab[257]={
#include "tantab.inc"
};
static const u16 icostab[257]={
#include "icostab.inc"
};

u32 fire_angle;

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

/* Historical `maze_width+2`/`maze_height+2` references address the low
 * word of a 32-bit variable; +2 is an address displacement, not arithmetic. */
static size_t row_cells(void) { return (size_t)bound_w; }
static int inside_cell(unsigned x,unsigned y)
{
    return maze_data && x<(size_t)bound_w && y<(size_t)bound_h;
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
    above=(cy?c-row:c); below=(cy+1u<(size_t)bound_h?c+row:c);

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

/* Shared C form of COLLIDE.S enter_mscan/mscan/TestWall.  The scanner
 * keeps the source's local-octant representation: primary movement always
 * advances toward the next "right" wall while secondary movement can cross
 * the local "top" wall first.  Distances and first-cell rounding deliberately
 * follow the 68000 sequence rather than using floating point. */
typedef struct AvpMazeScan {
    int cx,cy;
    int primary_dx,primary_dy;
    int secondary_dx,secondary_dy;
    unsigned primary_wall,secondary_wall;
    u16 local_x,local_y;
    u16 reflect;
    u32 d_step,d_step_full,d_total,d_max;
    u16 y_step,y_step_full;
} AvpMazeScan;

static const u8 *scan_wall(const AvpMazeScan *s,unsigned wall)
{
    if(s->cx<0||s->cy<0||!inside_cell((unsigned)s->cx,(unsigned)s->cy))return NULL;
    return cell_at((unsigned)s->cx,(unsigned)s->cy)+wall;
}

static u32 mul_frac_even(u32 step,u16 frac)
{
    /* Source halves d_step, MULU's by the 16-bit fraction, takes the high
     * word, then doubles.  Preserve the resulting even rounding. */
    u64 p=(u64)(step>>1)*(u64)frac;
    return (u32)((p>>16)<<1);
}

static u32 scan_maze(AvpMazeScan *s)
{
    for(unsigned guard=0;guard<16384u;guard++) {
        const u8 *wall;
        u32 dist;
        u16 coord;
        if(s->d_total>=s->d_max)return 0;

        if(s->local_y<s->y_step) {
            /* phit_top: coord = y/y_step; first pass includes local x. */
            u16 ratio;
            if(s->y_step==0) ratio=0;
            else {
                u32 num=(u32)s->local_y<<15;
                u16 den=(u16)((s->y_step>>1)+1u);
                ratio=(u16)(num/den);
            }
            dist=s->d_total+mul_frac_even(s->d_step,ratio);
            if(dist>=s->d_max)return 0;
            coord=ratio;
            if(s->d_total==0) {
                u16 x=s->local_x,inv=(u16)~x;
                coord=(u16)(x+(u16)(((u32)inv*(u32)ratio)>>16));
            }
            coord^=s->reflect;
            wall=scan_wall(s,s->secondary_wall);
            if(!wall || check_wall(wall,coord,64u))return dist;
            s->cx+=s->secondary_dx;s->cy+=s->secondary_dy;
        }

        s->local_y=(u16)(s->local_y-s->y_step);
        dist=s->d_total+s->d_step;
        if(dist>=s->d_max)return 0;
        coord=(u16)(s->local_y^s->reflect);
        wall=scan_wall(s,s->primary_wall);
        if(!wall || check_wall(wall,coord,64u))return dist;
        s->cx+=s->primary_dx;s->cy+=s->primary_dy;

        if(s->d_total==0) {
            s->d_total=s->d_step;
            s->d_step=s->d_step_full;
            s->y_step=s->y_step_full;
            s->local_x=0;
        } else s->d_total+=s->d_step;
    }
    return 0;
}

static void set_dir(int dir,int *dx,int *dy,unsigned *wall)
{
    *dx=*dy=0;
    switch(dir&3){
    case 0:*dx=-1;*wall=LEFT_WALL;break;
    case 1:*dy=-1;*wall=TOP_WALL;break;
    case 2:*dx= 1;*wall=RIGHT_WALL;break;
    default:*dy=1;*wall=BOTTOM_WALL;break;
    }
}

/* FireDistance from COLLIDE.S.  fire_angle is the original 16.16 angle value
 * and fire_distance is the authored maximum trace distance.  A return of zero
 * means no blocking wall before that maximum. */
u32 FireDistance(void)
{
    static const u8 primary_dir[8]={2,1,1,0,0,3,3,2};
    static const u8 secondary_dir[8]={1,2,0,1,3,0,2,3};
    static const u8 reflect_tab[8]={0,1,0,1,0,1,0,1};
    u16 a=(u16)(fire_angle>>16);
    unsigned oct=(unsigned)(a>>13)&7u;
    unsigned phase=(unsigned)(a&0x1fffu)>>5;
    unsigned idx=(oct&1u)?(256u-phase):phase;
    AvpMazeScan q={0};
    u16 x=(u16)x_pos,y=(u16)y_pos;
    if(idx>256u)idx=256u;
    q.cx=(int)((u32)x_pos>>16);q.cy=(int)((u32)y_pos>>16);
    q.reflect=reflect_tab[oct]?0xffffu:0u;
    switch(oct){
    case 0:q.local_x=x;       q.local_y=y;       break; /* ENE */
    case 1:q.local_x=(u16)~y; q.local_y=(u16)~x; break; /* NNE */
    case 2:q.local_x=(u16)~y; q.local_y=x;       break; /* NNW */
    case 3:q.local_x=(u16)~x; q.local_y=y;       break; /* WNW */
    case 4:q.local_x=(u16)~x; q.local_y=(u16)~y; break; /* WSW */
    case 5:q.local_x=y;       q.local_y=x;       break; /* SSW */
    case 6:q.local_x=y;       q.local_y=(u16)~x; break; /* SSE */
    default:q.local_x=x;      q.local_y=(u16)~y; break; /* ESE */
    }
    set_dir(primary_dir[oct],&q.primary_dx,&q.primary_dy,&q.primary_wall);
    set_dir(secondary_dir[oct],&q.secondary_dx,&q.secondary_dy,&q.secondary_wall);
    q.d_step_full=0x10000u+(u32)icostab[idx];
    q.y_step_full=tantab[idx];
    {
        u16 remain=(u16)~q.local_x;
        q.d_step=mul_frac_even(q.d_step_full,remain);
        q.y_step=(u16)(((u32)q.y_step_full*(u32)remain)>>16);
    }
    q.d_total=0;q.d_max=fire_distance;
    return scan_maze(&q);
}

/* Source-exact control-flow equivalent of LineOfSight.  It uses the same maze
 * scanner as FireDistance, but the source tracks distance along the major
 * axis and derives the minor step from the actual object delta. */
int LineOfSight(const AvpXY *from,const AvpXY *to)
{
    s64 sx=(s64)to->x-(s64)from->x,sy=(s64)to->y-(s64)from->y;
    u32 ax=(u32)(sx<0?-sx:sx),ay=(u32)(sy<0?-sy:sy);
    int primary_dir,secondary_dir;
    AvpMazeScan q={0};
    u16 xf=(u16)from->x,yf=(u16)from->y;
    int major_y;
    if(!inside_cell((u32)from->x>>16,(u32)from->y>>16) ||
       !inside_cell((u32)to->x>>16,(u32)to->y>>16))return 1;
    if((ax|ay)==0)return 0;

    major_y=(ay>=ax);
    if(!major_y){
        q.local_x=(sx<0)?(u16)~xf:xf;
        q.local_y=(sy>=0)?(u16)~yf:yf;
        q.d_max=ax;
        primary_dir=(sx<0)?0:2;
        secondary_dir=(sy>=0)?3:1;
    }else{
        q.local_x=(sy<0)?(u16)~yf:yf;
        q.local_y=(sx>=0)?(u16)~xf:xf;
        q.d_max=ay;
        primary_dir=(sy<0)?1:3;
        secondary_dir=(sx>=0)?2:0;
        {u32 t=ax;ax=ay;ay=t;}
    }
    q.reflect=(u16)(((sx<0)^(sy>=0)^major_y)?0xffffu:0u);
    q.cx=(int)((u32)from->x>>16);q.cy=(int)((u32)from->y>>16);
    set_dir(primary_dir,&q.primary_dx,&q.primary_dy,&q.primary_wall);
    set_dir(secondary_dir,&q.secondary_dx,&q.secondary_dy,&q.secondary_wall);

    /* COLLIDE.S: (delta_y<<8)/((delta_x>>8)+1), with delta_x the major. */
    {
        u32 den=(ax>>8)+1u;
        u64 num=(u64)ay<<8;
        u32 step=(u32)(num/den);
        if(step>0xffffu)step=0xffffu;
        q.y_step_full=(u16)step;
    }
    q.d_step_full=0x10000u;
    q.d_step=(u32)(u16)~q.local_x;
    q.y_step=(u16)(((u32)q.y_step_full*(u32)q.d_step)>>16);
    q.d_total=0;
    return scan_maze(&q)!=0u;
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
