#include "mjp.h"
#include "avp_runtime.h"
#include "avp_types.h"
#include "joypad.h"
#include "eeprom.h"
#include "player.h"
#include <assert.h>
#include <string.h>

volatile u32 joy_edge,joy2_edge,joy_cur,joy2_cur;
volatile s8 joy_special_mode,reset_enabled,pad_flag2,pad_flag3;
volatile u16 reset_wait;
volatile u32 reset_time,frame_counter;
void (*x_read)(void);
s16 player_type;
s32 score;
u8 cartcopy[AVP_EE_SIZE];
static unsigned ee_writes;
void Write_EE(void){++ee_writes;}

static unsigned vblanks,blits,loads;
static u16 files_seen[16];
static unsigned modes_seen[16],roles_seen[16];
static s32 choice_reply;
static char fame_draw[5][2][16];

static int scripted_pad;
void readpad(void){ if(!scripted_pad)joy_cur=0xffffffffu; joy_edge=joy_cur; }
void VSync(void){ ++vblanks; }

static void wait_blit(void *u){(void)u;++blits;}
static void load_resource(void *u,unsigned mode,unsigned file,unsigned role)
{
    (void)u;
    assert(loads<16u);
    modes_seen[loads]=mode;files_seen[loads]=(u16)file;roles_seen[loads]=role;++loads;
}
static int choose(void *u,unsigned mode,s32 *choice)
{
    (void)u;(void)mode;*choice=choice_reply;return 1;
}


static void draw_text(void *u,unsigned line,const char *text,unsigned x)
{
    (void)u;
    if(line<5u){unsigned col=(x>=100u)?1u:0u;strncpy(fame_draw[line][col],text,15u);fame_draw[line][col][15]=0;}
}

static int fame_name(void *u,char out[7])
{
    (void)u;memcpy(out,"ABCDEF",7);return 1;
}

int main(void)
{
    AvpRuntimeOps ops;
    memset(&ops,0,sizeof(ops));
    ops.wait_blitter=wait_blit;
    ops.frontend_load_resource=load_resource;
    ops.frontend_choice=choose;
    ops.draw_text=draw_text;
    avp_runtime_bind(&ops);

    choice_reply=4;
    MTEST(AVP_MJP_TITLE);
    assert(loads==9u);
    { const u16 want[9]={208,207,237,236,233,205,232,234,238};
      for(unsigned i=0;i<9u;i++){assert(files_seen[i]==want[i]);assert(modes_seen[i]==AVP_MJP_TITLE);assert(roles_seen[i]==i);} }
    assert(Choice==4);
    assert(blits==3u); /* Clear_Bu clears back/front/alternate retail buffers. */
    assert(vblanks==1u);

    /* MAIN.S hidden EEPROM-clear contract: player_type is primed to -1 and
     * MJP returns -99 when the Hall-of-Fame clear action is chosen. */
    loads=blits=vblanks=0;
    player_type=-1;choice_reply=-99;
    MTEST(AVP_MJP_FAME);
    assert(player_type==-99);
    assert(loads==2u);
    { const u16 want[2]={234,235};
      for(unsigned i=0;i<2u;i++)assert(files_seen[i]==want[i]); }
    assert(blits==3u && vblanks==1u);
    /* Exact retail Hall-of-Fame default packing. */
    memset(cartcopy,0,sizeof(cartcopy));
    Init_Hig();
    memset(fame_draw,0,sizeof(fame_draw));
    player_type=PT_HUMAN;score=0;choice_reply=0;Do_Fame();
    /* Retail $01CC70 produces fixed ten-character score fields. */
    assert(strcmp(fame_draw[0][1],"   1220000")==0);
    assert(strcmp(fame_draw[1][1],"    800000")==0);
    assert(strcmp(fame_draw[2][1],"    600000")==0);
    { const u32 want[10]={0x188513ffu,0x00129da0u,0x40d1e3ffu,0x000c3500u,
                          0x9f48bd64u,0x000927c0u,0x5206125fu,0x00061a80u,
                          0x1447351fu,0x00030d40u};
      const u8 *p=cartcopy+60;
      for(unsigned i=0;i<10u;i++,p+=4){u32 v=((u32)p[0]<<24)|((u32)p[1]<<16)|((u32)p[2]<<8)|p[3];assert(v==want[i]);}
    }
    Unpack_F();
    Pack_Fam(); /* round trip must remain byte exact */
    { const u8 *p=cartcopy+60; assert(p[0]==0x18 && p[1]==0x85 && p[2]==0x13 && p[3]==0xff); }

    /* A post-game score insertion is owned by MJP C, not by the host UI.
     * The host supplies only the six entered characters. */
    ops.frontend_fame_name=fame_name;avp_runtime_bind(&ops);
    player_type=PT_PREDATOR;score=2000000;
    MTEST(AVP_MJP_FAME);
    { const u8 *p=cartcopy+60;
      u32 ident=((u32)p[0]<<24)|((u32)p[1]<<16)|((u32)p[2]<<8)|p[3];
      u32 sc=((u32)p[4]<<24)|((u32)p[5]<<16)|((u32)p[6]<<8)|p[7];
      assert(ident==0x80110c85u);
      assert(sc==2000000u);
    }

    /* Mode 3 loads exactly one species backdrop, and none for player_type=-1. */
    loads=0;player_type=PT_HUMAN;score=0;choice_reply=0;MTEST(AVP_MJP_FAME);
    assert(loads==3u && files_seen[0]==234 && files_seen[1]==235 && files_seen[2]==229);
    loads=0;player_type=PT_ALIEN;MTEST(AVP_MJP_FAME);
    assert(loads==3u && files_seen[2]==228);
    loads=0;player_type=PT_PREDATOR;MTEST(AVP_MJP_FAME);
    assert(loads==3u && files_seen[2]==227);

    /* Strict comparison: tying the fifth score does not insert or write EEPROM. */
    Init_Hig();ee_writes=0;player_type=PT_HUMAN;score=200000;
    Do_Fame();assert(ee_writes==0u);

    /* A qualifying score shifts the table, starts the six-character name at A,
     * and commits/Pack_Fam/Write_EE on A/B/C. */
    Init_Hig();ee_writes=0;player_type=PT_HUMAN;score=1300000;
    joy_cur=~(1u<<FIRE_A);scripted_pad=1; /* editor sees confirm */
    Do_Fame();
    assert(ee_writes==1u);
    { const u8 *p=cartcopy+60;
      u32 packed=((u32)p[0]<<24)|((u32)p[1]<<16)|((u32)p[2]<<8)|p[3];
      u32 sc=((u32)p[4]<<24)|((u32)p[5]<<16)|((u32)p[6]<<8)|p[7];
      assert((packed>>30)==0u);assert(sc==1300000u);
    }
    scripted_pad=0;joy_cur=0xffffffffu;

    /* Make_New and New_Make are both in-place in retail: the caller selects
     * Ob_Addr explicitly; neither routine owns an allocator. */
    {
        AvpMjpObject objs[20];
        memset(objs,0,sizeof(objs));
        avp_mjp_bind_objects(objs,20);
        Ob_Addr=&objs[0];
        Ob_Data=0x1000;Ob_Pos_X=100;Ob_Pos_Y=50;Ob_Height=20;Ob_Width=40;Ob_Trans=0;
        AvpMjpObject *a=Make_New();
        assert(a==&objs[0]);
        Ob_Scale=12;
        AvpMjpObject *b=New_Make();
        assert(b==a);
        assert(b->scaled==1u && b->scale_x==12 && b->scale_y==12);
        assert(b->x==92 && b->y==46);
        Ob_Addr=&objs[1];
        assert(Make_New()==&objs[1]);

        memset(objs,0,sizeof(objs));
        Make_Rel();
        for(unsigned i=0;i<9u;i++)assert(objs[1u+i*2u].width==16);
        for(unsigned i=0;i<9u;i++)assert(objs[i*2u].width==0);
    }

    /* Recovered $01E166 semantics: Intro and mode 7 can be skipped on the
     * first timed-wait tick.  Quick_St itself consumes the preceding tick. */
    scripted_pad=1;joy_cur=~(1u<<FIRE_A);vblanks=0;loads=0;
    MTEST(AVP_MJP_INTRO);
    assert(loads==8u);
    assert(vblanks==11u);

    vblanks=0;loads=0;
    MTEST(AVP_MJP_SIM_TERMINATED);
    assert(loads==2u);
    assert(vblanks==19u);

    /* Base Explodes does not use the skippable $01E166 path: retail waits
     * exactly 600 ticks, then waits for A/B/C.  With A already held the final
     * input loop consumes one more tick, in addition to Quick_St. */
    vblanks=0;loads=0;
    MTEST(AVP_MJP_BASE_EXPLODES);
    assert(loads==2u);
    assert(vblanks==619u);

    /* Escape keeps BEE0 clear throughout its timed presentation.  A held from
     * entry therefore cannot shorten its 70/30/300-tick sequence. */
    vblanks=0;loads=0;
    MTEST(AVP_MJP_ESCAPE);
    assert(loads==6u);
    assert(vblanks==2578u);

    /* Full Do_Win continues through $020190.  With the hardware completion
     * backend absent, each OP poll resolves after its first CPU tick. */
    vblanks=0;loads=0;blits=0;player_type=PT_ALIEN;
    MTEST(AVP_MJP_ALIEN_WIN);
    assert(loads==2u);
    assert(vblanks==604u); /* setup tick + first OP poll + 600 hold + held-A input tick + second OP poll */
    assert(blits==5u);     /* three-buffer Clear_Bu plus two explicit retail Waitbl phases */

    scripted_pad=0;joy_cur=0xffffffffu;

    avp_runtime_reset_ops();
    return 0;
}
