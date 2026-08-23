/* Readable C reconstruction of the 68000-side level-state core in
 * MAZE/LEVELS.S: InitLevels, LeaveLevel, SetStart and FirstPos.
 *
 * Resource transfer/panel/GPU work remains in the platform/resource layer;
 * this file owns the ordinary game-state semantics.
 */
#include "levels.h"
#include "player.h"
#include "maze.h"
#include "amp.h"
#include "collision.h"
#include "doors.h"
#include "avp_runtime.h"
#include "sprites.h"
#include "hud.h"
#include "eeprom.h"
#include "main_game.h"
#include "music.h"
#include <stddef.h>

static const AvpLevelInfo *level_table;
static unsigned level_count;
static const u8 *save_data;
static unsigned save_size;

s16 cur_level,new_level;
s32 new_x,new_y;
u8 panel_list[AVP_MAX_PANELS];
u32 panel_pos;
uintptr_t walls,floors;

extern u32 x_pos,y_pos;
extern u32 centre_angle;
extern void SaveDoors(void);
extern void save_level(void);

static u32 be32(const u8 *p)
{
    return ((u32)p[0]<<24)|((u32)p[1]<<16)|((u32)p[2]<<8)|(u32)p[3];
}

static const u8 *active_save(unsigned need)
{
    if(savegame)return (const u8 *)(uintptr_t)savegame;
    if(save_data && save_size>=need)return save_data;
    return NULL;
}

void avp_levels_set_table(const AvpLevelInfo *table,unsigned count)
{
    level_table=table; level_count=count;
}
void avp_levels_set_save_words(const u8 *bytes,unsigned size)
{
    save_data=bytes; save_size=size;
}

static void invalidate_new_x_high_word(void)
{
    /* 68000 `move.w #-1,new_x` writes the high word of the big-endian long,
     * leaving the low word untouched.  Preserve that exact state rather than
     * replacing the entire long with -1. */
    new_x=(s32)(0xffff0000u|((u32)new_x&0x0000ffffu));
}

void FirstPos(void)
{
    const AvpLevelInfo *li;
    invalidate_new_x_high_word();
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
    {
        const u8 *sv=active_save(20);
        if(sv){
        u32 packed=be32(sv+16);
        unsigned saved_level=(packed>>2)&0x0fu;
        /* Retail accepts the packed four-bit level field verbatim.  Do not
         * clamp or "validate" it here; EEPROM checksum validity is handled
         * by the cart/save layer, not LEVELS.S. */
        cur_level=(s16)saved_level;
        }
    }
    FirstPos();
    for (i=0;i<AVP_MAX_PANELS;++i) panel_list[i]=0xffu;
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
        invalidate_new_x_high_word();
    } else {
        /* Retail FORCE_START=1: entering without an explicit connection
         * coordinate falls back to that level's safe start. */
        FirstPos();
    }

    {
        const u8 *sv=active_save(4);
        if(sv){
        u32 packed=be32(sv);
        u32 x=(packed>>10)&0x003ffc00u;
        u32 y=(packed<<2)&0x003ffc00u;
        u32 a=(packed&0xffu)<<24;
        x_pos=x; y_pos=y; centre_angle=a;
        }
    }
}


void LoadLevel(void)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    /* LEVELS.S order: revive file/GPU transfer path, set scale, select level,
     * transfer maze, transfer panels, reset per-level sprite expansion, set
     * entry position, restore AMPs/cocoons, then reset player state. */
    if(o->file_event)o->file_event(o->user,0x554b4750u,(u32)(u16)cur_level,0,0); /* UnKillGPU */
    ResetScale();
    if(o->load_level)o->load_level(o->user,cur_level); /* TransMaze + GetPanels resource seam */
    ResetSprites();
    SetStart();
    restore_level();
    ccn_xsave=ccn_ysave=0;
    KillBastards();
    ResetPlayer();
    new_level=0;
}


/* Remaining active LEVELS.S CPU entry points.  Data transfer/rotation of
 * copyrighted panel images is intentionally a runtime/resource backend task. */
void KillBastards(void)
{
    u16 gx=(u16)(x_pos>>16),gy=(u16)(y_pos>>16);
    for(unsigned i=0;i<AVP_NUM_AMPS;i++){AvpAmp *a=&amps_at[i];if(!a->mode)continue;if((u16)(a->xpos>>16)!=gx||(u16)(a->ypos>>16)!=gy)continue;if(a->creature==AC_COCOON||a->creature==AC_AQUEEN||a->creature==AC_AQSHIELD||a->creature==AC_GEN)continue;a->mode=NULL;}
}

void place_grid(void)
{
    enum { LEFT=0,TOP=1,RIGHT=2,BOTTOM=3,FLOOR=5 };
    typedef struct Ban {u8 x,y,wall;} Ban;
    static const Ban l1[]={{18,2,RIGHT},{0,0,0xff}};
    static const Ban l3[]={{36,9,LEFT},{36,10,LEFT},{0,0,0xff}};
    static const Ban l5[]={{25,5,RIGHT},{21,51,RIGHT},{0,0,0xff}};
    static const Ban l14[]={{24,24,LEFT},{25,19,TOP},{0,0,0xff}};
    static const Ban l15[]={{22,27,LEFT},{0,0,0xff}};
    static const Ban *const bans[AVP_MAX_LEVEL]={l1,NULL,l3,NULL,l5,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,l14,l15};
    u8 saved[2]={0,0};unsigned saved_n=0;
    u8 *maze=avp_collision_maze_data();u16 w=avp_collision_maze_width(),h=avp_collision_maze_height();
    const Ban *bl=(cur_level>=1&&cur_level<=AVP_MAX_LEVEL)?bans[cur_level-1]:NULL;
    for(unsigned i=0;i<AVP_AMP_GRID_W*AVP_AMP_GRID_H;i++)collmap[i]=0xffffu;
    if(!maze)return;

    /* BRIG_BAN is 1 in the retail source: temporarily remove a handful of
     * authored walls while constructing the random-placement safety map. */
    if(bl)for(const Ban *b=bl;b->wall!=0xff && saved_n<2u;b++,saved_n++){
        if(b->x<w&&b->y<h){u8 *c=maze+((size_t)b->y*w+b->x)*8u;saved[saved_n]=c[b->wall];c[b->wall]=0;}
    }

    /* Stage one: FLOOR is byte 5 of each 8-byte maze cell.  A zero floor
     * produces -1; any non-zero floor produces 0. */
    for(unsigned y=0;y<64u;y++)for(unsigned x=0;x<64u;x++){
        if(x<w&&y<h){u8 *c=maze+((size_t)y*w+x)*8u;collmap[y*64u+x]=c[FLOOR]?0u:0xffffu;}
    }

    /* Stage two: fail squares on the impossible side of inconsistent/open
     * wall pairs. This preserves the exact asymmetry in LEVELS.S. */
    for(unsigned y=0;y<64u;y++)for(unsigned x=0;x<64u;x++){
        unsigned i=y*64u+x;int bad=0;u8 *c;
        if(collmap[i]||x>=w||y>=h)continue;
        c=maze+((size_t)y*w+x)*8u;
        if(!c[LEFT]){
            if(x==0)bad=1;else {u8 *n=maze+((size_t)y*w+x-1u)*8u;if(n[RIGHT])bad=1;}
        }
        if(!bad&&!c[TOP]){
            if(y==0)bad=1;else {u8 *n=maze+((size_t)(y-1u)*w+x)*8u;if(n[BOTTOM])bad=1;}
        }
        if(!bad&&!c[RIGHT]){
            if(x+1u>=w)bad=1;else {u8 *n=maze+((size_t)y*w+x+1u)*8u;if(n[LEFT])bad=1;}
        }
        if(!bad&&!c[BOTTOM]){
            if(y+1u>=h)bad=1;else {u8 *n=maze+((size_t)(y+1u)*w+x)*8u;if(n[TOP])bad=1;}
        }
        if(bad)collmap[i]=0xffffu;
    }

    /* Stage three (replace): repeatedly fail any good square connected to a
     * failed square through a zero wall on the current square. */
    for(;;){
        unsigned changed=0;
        for(unsigned y=0;y<64u;y++)for(unsigned x=0;x<64u;x++){
            unsigned i=y*64u+x;u8 *c;int bad=0;
            if(collmap[i]||x>=w||y>=h)continue;
        c=maze+((size_t)y*w+x)*8u;
            if(!c[LEFT]   && (x==0      || collmap[i-1u]))bad=1;
            if(!bad&&!c[TOP]    && (y==0      || collmap[i-64u]))bad=1;
            if(!bad&&!c[RIGHT]  && (x+1u>=64u || collmap[i+1u]))bad=1;
            if(!bad&&!c[BOTTOM] && (y+1u>=64u || collmap[i+64u]))bad=1;
            if(bad){collmap[i]=0xffffu;++changed;}
        }
        if(!changed)break;
    }

    /* Player's current whole-cell position is never eligible. */
    {unsigned px=(u32)x_pos>>16,py=(u32)y_pos>>16;if(px<64u&&py<64u)collmap[py*64u+px]=0xffffu;}

    /* Restore the BRIG_BAN walls byte-for-byte. */
    if(bl){unsigned k=0;for(const Ban *b=bl;b->wall!=0xff && k<saved_n;b++,k++)if(b->x<w&&b->y<h)maze[((size_t)b->y*w+b->x)*8u+b->wall]=saved[k];}
}

void xMap(void){/* TEST_PLACE-only diagnostic visualizer; excluded from retail behavior. */}

void swapper(void)
{
    /* Retail builds LEVELS.S with DUNGEON=0, therefore MAZE_SWAPS=0 and the
     * entire swapper block is assembled out.  Keep the historical entry as a
     * compatibility symbol, but do not invent a shipping runtime operation. */
}

void ScreenOff(void)
{
    const AvpRuntimeOps *o=avp_runtime_ops();

    /* LEVELS.S::ScreenOff exits immediately unless the title/front-end owns
     * the screen.  These two flags are ordinary 68000 state, not renderer
     * state, so preserve the exact gating in portable C. */
    if(!in_title)return;
    in_title=0;

    if(in_select){
        in_select=0;

        /* The historical Comp_Font/Buffer1/This_Pic assignments and
         * End_Select list/pixel work are front-end resource/OP operations.
         * Let the backend tear that presentation down, then preserve the CPU
         * synchronization/audio state that follows it. */
        if(o->frontend_event)o->frontend_event(o->user,0x53454e44u,0,0); /* SEND: End_Select presentation */
        WaitFXFa();
        StopMusi();
        if(o->frontend_event)o->frontend_event(o->user,0x5545564fu,0x7fff,0); /* UEV O: UEBERVOLUME */
    }

    /* OLP/VMODE and SetScreenSize are Jaguar display hardware state. */
    if(o->screen_off)o->screen_off(o->user);
}


int avp_levels_panel_step(u8 panel_file, int *must_transfer)
{
    /* LEVELS.S::itrans: panel_pos is a 16-bit running index.  The sublevel
     * pseudo-panel ($80) is deliberately never reused/skipped because its
     * overlay depends on cur_level. */
    u16 pos=(u16)panel_pos;
    int transfer=1;
    panel_pos=(panel_pos&0xffff0000u)|(u16)(pos+1u);
    if(pos<AVP_MAX_PANELS){
        if(panel_file!=0x80u && panel_list[pos]==panel_file)transfer=0;
        if(transfer)panel_list[pos]=panel_file;
    }
    if(must_transfer)*must_transfer=transfer;
    return (int)pos;
}

void avp_levels_overlay_plan(u8 file,s8 xpos,s8 ypos,u16 width,u16 height,AvpOverlayPlan *out)
{
    /* LEVELS.S::add_over ordinary-68000 arithmetic.  JPEG width/height are
     * resource results supplied by the backend; the resulting pixel/count and
     * phrase-step values are CPU-owned and reproduced here exactly. */
    if(!out)return;
    out->file=file;out->width=width;out->height=height;
    if(file==0){out->centre_x=out->centre_y=0;out->dest_pixel=out->blit_count=0;out->src_step=out->dst_step=0;return;}
    s16 x=(s16)xpos+64-(s16)(width>>1);
    s16 y=(s16)ypos+64-(s16)(height>>1);
    out->centre_x=x;out->centre_y=y;
    out->dest_pixel=((u32)(u16)y<<16)|(u16)x;
    out->blit_count=((u32)height<<16)|width;
    /* d0 starts as (1<<16)|1; when dest pixel offset is non-zero the low
     * word gains four before negation.  A1_STEP then adds d2&3. */
    {
        s16 low=(s16)(1 + (((u16)x&3u)?4:0));
        s16 src=(s16)-low;
        s16 dst=(s16)(src+((u16)x&3u));
        out->src_step=(s32)(((u32)1u<<16)|(u16)src);
        out->dst_step=(s32)(((u32)1u<<16)|(u16)dst);
    }
}

void do_notice(void)
{
    /* Remaining authored overlay-resource selection is tracked as an open
     * closure item.  This event must not be treated as proof that the CPU
     * routine is semantically complete. */
    const AvpRuntimeOps*o=avp_runtime_ops();
    if(o->file_event)o->file_event(o->user,0x4e4f5443u,(u32)(u16)cur_level,0,0);
}

void add_over(void)
{
    /* xTransFile/blitter execution is a resource/hardware seam, but the
     * source-side overlay coordinate/count math still requires explicit proof
     * before this routine can be closed in the RE #7 matrix. */
    const AvpRuntimeOps*o=avp_runtime_ops();
    if(o->file_event)o->file_event(o->user,0x4f564552u,0,0,0);
}

void FixDucts(void)
{
    /* LEVELS.S::FixDucts is ordinary CPU pointer state: air ducts reuse the
     * wall panel set as their floor panel set.  The pointed-to image data is
     * resource-owned, but the alias itself belongs in the 68000 C core. */
    floors=walls;
}

void FixAlien(void)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    u16 d0=(u16)panel_pos;
    u8 original;
    unsigned base,i;

    /* LEVELS.S advances panel_pos by three before deciding whether the three
     * rotated copies already match the source panel. */
    panel_pos=(panel_pos&0xffff0000u)|(u16)(d0+3u);

    /* d0-1 indexes the source panel entry; the following three bytes are the
     * E/S/W rotated copies.  Valid retail level data keeps this inside the
     * MAX_PANELS table. */
    base=(unsigned)(u16)(d0-1u);
    if(base+3u>=AVP_MAX_PANELS)return;
    original=panel_list[base];

    if(panel_list[base+1u]==original &&
       panel_list[base+2u]==original &&
       panel_list[base+3u]==original)
        return;

    for(i=1;i<=3u;i++)panel_list[base+i]=original;

    /* The three CRY16 rotations themselves are fixed Jaguar blitter programs.
     * Publish only that hardware operation after the CPU table semantics are
     * complete. */
    if(o->file_event)o->file_event(o->user,0x414c494eu,base,3u,0);
}
