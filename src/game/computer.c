/* Source-shaped ordinary-68000 translation of MAZE/COMPUTER.S.
 *
 * The shipping terminal lookup, menu/input rules and progression side effects
 * live here.  Authored terminal text/image tables are resource data and are
 * bound by the ROM importer.  CTEXT/CLUT/Object-Processor pixel operations are
 * Jaguar/backend services, not missing 68000 game logic. */
#include "computer.h"
#include "doors.h"
#include "font.h"
#include "joypad.h"
#include "levels.h"
#include "player.h"
#include "objects.h"
#include "avp_runtime.h"

u32 c_joy_cur,c_joy_edge;
u8 repeat_pad;
s16 cursorpos,picount,pages,menuopts;

static const AvpComputerTerminal *level_terms[15];
static size_t level_term_count[15];
static const AvpComputerContent *base;
static unsigned overlay1=(unsigned)-1,overlay2=(unsigned)-1;
static u8 active;

static const AvpRuntimeOps *ops(void){return avp_runtime_ops();}
static int pressed(u32 v,unsigned bit){return (v&(1u<<bit))!=0u;}
static void event(unsigned e,s32 a,s32 b){const AvpRuntimeOps*o=ops();if(o->frontend_event)o->frontend_event(o->user,e,a,b);}

void avp_computer_bind_level(unsigned level,const AvpComputerTerminal *r,size_t n)
{
    if(level<1u||level>15u)return;
    level_terms[level-1u]=r;level_term_count[level-1u]=n;
}

void cls(void){event(0x434c53u,0,0);}
void InitComp(void){init_font();}

/* Exact COMPUTER.S repeat semantics.  pr_string sets repeat_pad; the next
 * c_readpad consumes/clears it without polling.  xc_readpad complements both
 * pad words so set bits mean pressed inside the computer UI. */
void c_readpad(void)
{
    if(repeat_pad){repeat_pad=0;return;}
    xc_readpad();
}
void xc_readpad(void)
{
    readpad();
    c_joy_cur=~joy_cur;
    c_joy_edge=~joy_edge;
}

void InitCDisplay(void)
{
    active=1;
    event(0x4350414cu,1,0); /* save CLUT / set 320x240 is backend-owned */
    cls();
    SetCObj1();SetCObj2();
}
void RestoreCDisplay(void)
{
    active=0;
    const AvpRuntimeOps *o=ops();
    if(o->gpu_reset_maze)o->gpu_reset_maze(o->user);
    if(o->restore_maze_list)o->restore_maze_list(o->user);
    event(0x4350414cu,0,0); /* restore CLUT */
}
void SetCObj1(void){event(0x434f31u,(s32)overlay1,0);}
void SetCObj2(void){event(0x434f32u,(s32)overlay2,0);}
void NewOver(unsigned id){overlay1=id;if(active)SetCObj1();}

static void set_obj1(int id){overlay1=(unsigned)id;SetCObj1();}
static void set_obj2(int id){overlay2=(unsigned)id;SetCObj2();}
static void print_page(unsigned n)
{
    if(!base||n>=base->pages||!base->page||!base->page[n])return;
    pr_string(base->page[n]);
}
static void wait_release(unsigned bit)
{
    do{VSync();c_readpad();}while(pressed(c_joy_edge,bit));
}
static int menu_step(s16 count)
{
    VSync();c_readpad();
    if(pressed(c_joy_edge,JOY_UP)){--cursorpos;wait_release(JOY_UP);}
    if(pressed(c_joy_edge,JOY_DOWN)){++cursorpos;wait_release(JOY_DOWN);}
    if(cursorpos==count)cursorpos=0;
    if(cursorpos<0)cursorpos=(s16)(count-1);
    event(0x43555253u,cursorpos,0); /* prpoint renderer */
    if(pressed(c_joy_edge,FIRE_C))return -1;
    if(pressed(c_joy_edge,FIRE_B))return 1;
    return 0;
}
static void page_viewer(void)
{
    picount=0;
    cls();print_page(0);
    for(;;){
        VSync();c_readpad();
        if(pressed(c_joy_cur,FIRE_C))return;
        if(pressed(c_joy_cur,FIRE_A)){
            do{c_readpad();}while(pressed(c_joy_cur,FIRE_A));
            if(pages){picount=(s16)((picount+1)%pages);}
            cls();print_page((unsigned)picount);
        }else if(pressed(c_joy_cur,FIRE_B)){
            do{c_readpad();}while(pressed(c_joy_cur,FIRE_B));
            if(pages){--picount;if(picount<0)picount=(s16)(pages-1);}
            cls();print_page((unsigned)picount);
        }else cursor();
    }
}

/* COMPUTER.S fixed-resource viewers (arsenal/species) use the same A/B/C
 * cycling logic but their picture/text tables are authored resources rather
 * than the per-terminal `base` log pages.  The frontend event identifies the
 * exact source table while C preserves the retail cursor/input state machine. */
static void fixed_viewer(unsigned tag,s16 count)
{
    picount=0;
    cls();event(tag,picount,count);
    for(;;){
        VSync();c_readpad();
        if(pressed(c_joy_edge,FIRE_C))return;
        if(pressed(c_joy_edge,FIRE_A)){
            do{c_readpad();}while(pressed(c_joy_cur,FIRE_A));
            ++picount;if(picount==count)picount=0;
            cls();event(tag,picount,count);
        }else if(pressed(c_joy_edge,FIRE_B)){
            do{c_readpad();}while(pressed(c_joy_cur,FIRE_B));
            --picount;if(picount<0)picount=(s16)(count-1);
            cls();event(tag,picount,count);
        }else cursor();
    }
}

int avp_computer_first_aid(void)
{
    static const s16 target[7]={500,500,500,750,750,750,1000};
    unsigned a=acs_level;
    if(player_energy==1000)return 0;
    if(a<4u||a>10u)return -1;
    s16 t=target[a-4u];
    /* Source compares target to current and only writes when target >= current. */
    if(t>=player_energy){player_energy=t;return 1;}
    return 0;
}
int avp_computer_can_destruct(const AvpComputerContent *content)
{
    return content&&content->allows_destruct&&!destruct_flag&&acs_level>=10u;
}
void avp_computer_arm_destruct(void)
{
    destruct_flag=1;
    const AvpRuntimeOps *o=ops();if(o->play_sfx)o->play_sfx(o->user,0); /* selfdest */
}
int avp_computer_launch_pod(void)
{
    if(!destruct_flag)return 0;
    launch_flag=1;return 1;
}

static void scheme(void)
{
    /* Retail waits until the low word of the computer-pad state is clear,
     * clears the text plane, selects the authored map image by cur_level, then
     * remains until C.  Image lookup itself is resource data. */
    do{c_readpad();}while((c_joy_cur&0xffffu)!=0u);
    cls();
    event(0x5343484du,cur_level,0);
    for(;;){VSync();c_readpad();if(pressed(c_joy_edge,FIRE_C))return;cursor();}
}
static void warn_destruct(void)
{
    cls();event(0x5741524eu,0,0);
    for(;;){
        VSync();c_readpad();
        if(pressed(c_joy_edge,FIRE_C))return;
        if(pressed(c_joy_edge,FIRE_A)){
            /* COMPUTER.S falls into `destruct`: arm once, replace the warning
             * screen, then remain there until C is pressed. */
            avp_computer_arm_destruct();
            cls();event(0x44455354u,1,0);
            for(;;){VSync();c_readpad();if(pressed(c_joy_edge,FIRE_C))return;}
        }
        cursor();
    }
}
static void generic(void)
{
    if(base&&base->access9_alternate&&acs_level>=9u)base=base->access9_alternate;
    pages=base?(s16)base->pages:0;
    for(;;){
        cursorpos=0;menuopts=(s16)(avp_computer_can_destruct(base)?3:2);
        set_obj1(-1);set_obj2(-1);cls();event(0x47454e4du,menuopts,0);
        for(;;){int r=menu_step(menuopts);if(r<0)return;if(!r)continue;
            if(cursorpos==0)scheme();
            else if(cursorpos==1)page_viewer();
            else warn_destruct();
            break;
        }
    }
}
static void armoury(void)
{
    for(;;){
        cursorpos=0;menuopts=2;set_obj1(-1);set_obj2(-1);cls();event(0x41524d4du,0,0);
        for(;;){int r=menu_step(2);if(r<0)return;if(!r)continue;
            if(cursorpos==0)fixed_viewer(0x4152534eu,4); /* arsenal/weaptab */
            else page_viewer();                         /* genlog/base */
            break;
        }
    }
}
static void medical_logs(void)
{
    for(;;){
        cursorpos=0;menuopts=2;set_obj1(-1);set_obj2(-1);cls();event(0x4d45444cu,0,0);
        for(;;){int r=menu_step(2);if(r<0)return;if(!r)continue;
            if(cursorpos==0)page_viewer();                 /* genlog/base */
            else fixed_viewer(0x53504543u,6);             /* species/creatab */
            break;
        }
    }
}
static void medical(void)
{
    for(;;){
        cursorpos=0;menuopts=2;set_obj1(-1);set_obj2(-1);cls();event(0x4d45444du,0,0);
        for(;;){int r=menu_step(2);if(r<0)return;if(!r)continue;
            if(cursorpos==0){
                int a=avp_computer_first_aid();
                event(0x46414944u,a,player_energy);
                for(;;){VSync();c_readpad();if(pressed(c_joy_edge,FIRE_C))break;cursor();}
            }else medical_logs();
            break;
        }
    }
}
static void podcomp(void)
{
    if(!destruct_flag){event(0x504f444eu,0,0);for(;;){VSync();c_readpad();if(pressed(c_joy_edge,FIRE_C))return;}}
    cursorpos=0;menuopts=1;event(0x504f444du,0,0);
    for(;;){int r=menu_step(1);if(r<0)return;if(!r)continue;avp_computer_launch_pod();event(0x4c41554eu,1,0);
        for(;;){VSync();c_readpad();if(pressed(c_joy_edge,FIRE_B))return;}
    }
}

void Computer(void)
{
    const AvpComputerTerminal *t=NULL;
    const AvpRuntimeOps *o=ops();
    if(o->file_event)o->file_event(o->user,0x4c5a5742u,0,0,0); /* SetLZWBuffer/ResetFGPU seam */
    InitCDisplay();cls();cls();
    if(cur_level>=1&&cur_level<=15){
        const AvpComputerTerminal *r=level_terms[cur_level-1];
        size_t n=level_term_count[cur_level-1];
        for(size_t i=0;i<n;i++)if(r[i].comp_offset==comp_offset){t=&r[i];break;}
    }
    if(t){
        base=t->content;pages=base?(s16)base->pages:0;
        switch(t->handler){
        case AVP_COMP_GENERIC:generic();break;
        case AVP_COMP_ARMOURY:armoury();break;
        case AVP_COMP_MEDICAL:medical();break;
        case AVP_COMP_POD:podcomp();break;
        }
    }
    RestoreCDisplay();
    comp_panel=0;
}
