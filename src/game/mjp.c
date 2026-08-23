/* High-level readable C reconstruction of the retail MJP front-end block.
 * Retail addresses/function boundaries come from the exact MJP reconstruction.
 * Object list phrase encoding and image pixels are platform/resource backends. */
#include "mjp.h"
#include "avp_runtime.h"
#include "joypad.h"
#include "player.h"
#include "objects.h"
#include "eeprom.h"
#include <string.h>

char mjp_games[4]="xxx";s32 Count_Do,Count,Choice;AvpMjpObject *Ob_Addr;u32 Ob_Data;s32 Ob_Pos_X,Ob_Pos_Y,Ob_Height,Ob_Width,Ob_Trans,Ob_Scale;
/* Retail MJP global at $0004BE24.  The host object builder keeps it as a normal
 * signed Y coordinate instead of exposing its historical RAM address. */
s32 High_Line;
static u32 mjp_pad;
static AvpMjpMode current_mjp_mode=AVP_MJP_TITLE;
static int escape_skip_enabled;
static s32 mjp_pad_wait;
static AvpMjpObject *pool;static unsigned pool_cap,pool_count;static const u8 *save_slots;static size_t save_stride=20;static const char *current_text;static unsigned selected_character;
static int text_abort;

typedef struct AvpMjpFameEntry { char species; char name[7]; s32 score; } AvpMjpFameEntry;
static AvpMjpFameEntry fame[5];
static const char fame_alphabet[33]="ABCDEFGHIJKLMNOPQRSTUVWXYZ!#&$. ";
static const AvpMjpFameEntry fame_default[5]={
    {'m',"MIKE  ",1220000},{'a',"ANDY  ",800000},{'p',"PURPLE",666000},
    {'a',"JAMES ",400000},{'m',"KEONI ",200000}
};

static u32 load_be32_mjp(const u8 *p){return ((u32)p[0]<<24)|((u32)p[1]<<16)|((u32)p[2]<<8)|p[3];}
static void store_be32_mjp(u8 *p,u32 v){p[0]=(u8)(v>>24);p[1]=(u8)(v>>16);p[2]=(u8)(v>>8);p[3]=(u8)v;}
static unsigned fame_species_code(char c){return c=='m'?0u:(c=='a'?1u:2u);}
static char fame_species_char(unsigned v){static const char map[4]={'m','a','p','p'};return map[v&3u];}
static unsigned fame_char_index(char c){for(unsigned i=0;i<32u;i++)if(fame_alphabet[i]==c)return i;return 31u;}
static u32 fame_pack_word(const AvpMjpFameEntry *e){
    static const unsigned sh[6]={25,20,15,10,5,0};
    u32 v=(u32)fame_species_code(e->species)<<30;
    for(unsigned i=0;i<6u;i++)v|=(u32)fame_char_index(e->name[i])<<sh[i];
    return v;
}
static void fame_unpack_word(AvpMjpFameEntry *e,u32 v){
    static const unsigned sh[6]={25,20,15,10,5,0};
    e->species=fame_species_char(v>>30);
    for(unsigned i=0;i<6u;i++)e->name[i]=fame_alphabet[(v>>sh[i])&31u];
    e->name[6]=0;
}

void avp_mjp_bind_objects(AvpMjpObject*o,unsigned n){pool=o;pool_cap=n;pool_count=0;Ob_Addr=NULL;}void avp_mjp_bind_save_slots(const u8*s,size_t stride){save_slots=s;if(stride)save_stride=stride;}
static void evt(unsigned e,s32 a,s32 b){const AvpRuntimeOps*o=avp_runtime_ops();if(o->frontend_event)o->frontend_event(o->user,e,a,b);}
void mjppad(void){readpad();mjp_pad=~joy_cur;}
void Load_GPU(void){evt(0x4d475055,0,0);}
void Clear(void){Waitbl();evt(0x434c5231,0,0);}
void Clear2(void){Waitbl();evt(0x434c5232,0,0);}
static void clear2_target(unsigned target){Waitbl();evt(0x434c5232,(s32)target,0x118);}
void Clear_Bu(void){for(unsigned i=0;i<3u;i++)clear2_target(i);}
AvpMjpObject *Make_New(void){if(!pool||pool_count>=pool_cap)return NULL;AvpMjpObject*o=&pool[pool_count++];memset(o,0,sizeof(*o));o->data=Ob_Data;o->x=Ob_Pos_X;o->y=Ob_Pos_Y;o->height=Ob_Height;o->width=Ob_Width;o->flags=(u32)Ob_Trans;Ob_Addr=o;return o;}
AvpMjpObject *New_Make(void){
    /* Retail $01E2F2 modifies the object already addressed by Ob_Addr.  It does
     * not allocate another list object.  Ob_Scale is encoded in both scale
     * bytes; the position compensation keeps the scaled bitmap centred in the
     * same way as the original phrase edits. */
    if(!Ob_Addr)return NULL;
    Ob_Addr->scaled=1;
    Ob_Addr->scale_x=Ob_Scale;
    Ob_Addr->scale_y=Ob_Scale;
    {
        s32 d=Ob_Scale-8;
        Ob_Addr->x-=d*2;
        Ob_Addr->y-=d;
    }
    return Ob_Addr;
}void Make_Sca(void){if(Ob_Addr){Ob_Addr->scaled=1;Ob_Addr->scale_x=Ob_Addr->scale_y=0;}}void Make_Bet(void){Ob_Data=0;Ob_Pos_Y=High_Line;Ob_Pos_X=320;Ob_Height=280;Ob_Trans=0;Ob_Width=16;Make_New();if(Ob_Addr){Ob_Addr->flags|=1u<<16;Ob_Addr->flags&=0xffff8fffu;}}
void Change_Y(s32 y){if(Ob_Addr)Ob_Addr->y=y;}void Change_X(s32 x){if(Ob_Addr)Ob_Addr->x=x;}void Change_D(u32 d){if(Ob_Addr)Ob_Addr->data=d;}void Change_S(s32 x,s32 y){if(Ob_Addr){Ob_Addr->scaled=1;Ob_Addr->scale_x=x;Ob_Addr->scale_y=y;}}void Hide_Obj(void){if(Ob_Addr)Ob_Addr->hidden=1;}void Unhide_O(void){if(Ob_Addr)Ob_Addr->hidden=0;}void No_Scale(void){if(Ob_Addr)Ob_Addr->scaled=0;}
void EncodeGa(void){static const char mapx[]="mapx";const u8 *slots=save_slots?save_slots:cartcopy;size_t stride=save_slots?save_stride:AVP_SAVE_SIZE;for(unsigned i=0;i<3;i++){unsigned t=3;const u8*p=slots+i*stride;u32 exists=((u32)p[0]<<24)|((u32)p[1]<<16)|((u32)p[2]<<8)|p[3];if(exists){u32 q=((u32)p[16]<<24)|((u32)p[17]<<16)|((u32)p[18]<<8)|p[19];t=q&3u;}mjp_games[i]=mapx[t];}mjp_games[3]=0;}
void One_Tick(void){VSync();evt(0x5449434b,0,0);}
void Waitbl(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->wait_blitter)o->wait_blitter(o->user);else evt(0x424c4954,0,0);}
void Waitnow(void){do{mjppad();}while((mjp_pad&(1u<<PAUSE))==0u);}
void Wait_Up(void){const u32 release_mask=(1u<<FIRE_A)|(1u<<FIRE_B)|(1u<<FIRE_C);do{One_Tick();mjppad();}while(mjp_pad&release_mask);}
void Quick_St(void){One_Tick();evt(0x51535452,0,0);}
void Make_Rel(void){
    /* Retail $01F082 builds nine 16-pixel-wide background/list strips. */
    evt(0x52454c,9,0);
    if(pool){for(unsigned i=0;i<9u;i++)Make_Bet();}
}
void Pause(void){
    /* Retail $01F05E: a confirm/skip press returns immediately; it does not
     * perform Make_Rel.  Otherwise consume exactly Count_Do ticks. */
    while(Count_Do!=0){mjppad();if(mjp_pad&0x22002000u)return;One_Tick();--Count_Do;}
}
static int mjp_typewriter_sound_mode(void)
{
    return current_mjp_mode==AVP_MJP_SELECT ||
           current_mjp_mode==AVP_MJP_INTRO ||
           current_mjp_mode==AVP_MJP_ESCAPE ||
           current_mjp_mode==AVP_MJP_SIM_TERMINATED;
}
static void mjp_text_char(unsigned line,unsigned x,unsigned char c)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    if(o->draw_text){char t[2]={(char)c,0};o->draw_text(o->user,line,t,x);}
}
void New_Text(const char*t){current_text=t;}
void Show_Tex(void)
{
    /* Retail $01D1E0 walks a sequence of NUL-terminated lines terminated by
     * an empty line.  Its MJP font is seven pixels wide; '@' advances by half
     * that width, and space/'~'/'@' all select the blank underscore cell.
     * Outside Hall of Fame, text advances one VBL per glyph.  A/B/C makes the
     * remainder of a non-intro message fast; in Intro it aborts the message. */
    const unsigned glyph_width=7u;
    const AvpRuntimeOps *o=avp_runtime_ops();
    const unsigned char *p=(const unsigned char*)current_text;
    unsigned line=0;
    int fast=(current_mjp_mode==AVP_MJP_FAME);
    text_abort=0;
    if(!p)return;
    for(;;){
        unsigned x=0;
        while(*p){
            unsigned char c=*p++;
            unsigned char draw=(c==' '||c=='~'||c=='@')?'_':c;
            if(mjp_typewriter_sound_mode() && !fast && c!=' ' && c!=0 && o->frontend_event)
                o->frontend_event(o->user,0x4d4a5041u,0x000221B8, -1); /* MJPA */
            mjp_text_char(line,x,draw);
            x+=(c=='@')?(glyph_width>>1):glyph_width;
            if(!fast){
                One_Tick();mjppad();
                if(mjp_pad&0x22002000u){
                    if(current_mjp_mode==AVP_MJP_INTRO){text_abort=1;return;}
                    fast=1;
                }
            }
        }
        /* The retail renderer emits its underscore/blank cursor cell at each
         * line terminator before testing for the final empty line. */
        mjp_text_char(line,x,'_');
        ++p;
        if(*p==0)return;
        ++line;
    }
}
void Fame_Tex(void){evt(0x46414d45,0,0);Show_Tex();}
void Pack_Fam(void){
    /* Retail $01CCE2: five {packed identity/name, score} pairs at cartcopy+60. */
    u8 *p=cartcopy+AVP_N_SAVES*AVP_SAVE_SIZE;
    for(unsigned i=0;i<5u;i++){store_be32_mjp(p,fame_pack_word(&fame[i]));store_be32_mjp(p+4,(u32)fame[i].score);p+=8;}
}
void Unpack_F(void){
    /* Retail $01CD1E/$01CDC6. */
    const u8 *p=cartcopy+AVP_N_SAVES*AVP_SAVE_SIZE;
    for(unsigned i=0;i<5u;i++){fame_unpack_word(&fame[i],load_be32_mjp(p));fame[i].score=(s32)load_be32_mjp(p+4);p+=8;}
}
void Init_Hig(void){
    /* Retail $01D18A copies the five default 23-byte display records and five
     * score longs, then Pack_Fam writes their packed EEPROM form. */
    memcpy(fame,fame_default,sizeof(fame));
    Pack_Fam();
}
void Lister4(void){evt(0x4c535434,(s32)pool_count,0);}void Lister3(void){evt(0x4c535433,(s32)pool_count,0);}void Lister(void){evt(0x4c535452,(s32)pool_count,0);}void Update2(void){evt(0x55504432,0,0);}void Update(void){evt(0x55504454,0,0);}void Sel_Upda(void){Update();evt(0x53454c55,(s32)selected_character,0);}void Make_TLi(void){evt(0x544c4953,0,0);}
static int get_choice(AvpMjpMode m)
{
    const AvpRuntimeOps*o=avp_runtime_ops();
    return o->frontend_choice?o->frontend_choice(o->user,(unsigned)m,&Choice):0;
}
static int mjp_pressed(unsigned bit){return (mjp_pad&(1u<<bit))!=0u;}
static void wait_bit_release(unsigned bit){do{One_Tick();mjppad();}while(mjp_pressed(bit));}

static char fame_prev_char(char c)
{
    switch(c){
    case 'A':return ' '; case ' ':return '&'; case '&':return '#'; case '#':return '$';
    case '$':return '!'; case '!':return '.'; case '.':return 'Z'; default:return (char)(c-1);
    }
}
static char fame_next_char(char c)
{
    switch(c){
    case 'Z':return '.'; case '.':return '!'; case '!':return '$'; case '$':return '#';
    case '#':return '&'; case '&':return ' '; case ' ':return 'A'; default:return (char)(c+1);
    }
}
static void fame_repeat_wait(u32 held,s32 *repeat_delay)
{
    s32 count=*repeat_delay;
    while(count-- > 0){
        One_Tick();mjppad();
        if(mjp_pad!=held){*repeat_delay=27;return;}
    }
    if(*repeat_delay!=7)*repeat_delay-=10;
}
static void fame_edit_name(unsigned slot)
{
    const AvpRuntimeOps*o=avp_runtime_ops();
    char host_name[7];
    if(o->frontend_fame_name && o->frontend_fame_name(o->user,host_name)){
        for(unsigned i=0;i<6u;i++)fame[slot].name[i]=host_name[i]?host_name[i]:' ';
        fame[slot].name[6]=0;
        return;
    }
    unsigned cursor=0;
    char current='A';
    u32 last_pad=~0u;
    s32 repeat_delay=27;
    memset(fame[slot].name,' ',6);fame[slot].name[6]=0;
    for(;;){
        One_Tick();mjppad();
        u32 pad=mjp_pad;
        if(pad!=last_pad)repeat_delay=27;
        if(pad&(1u<<JOY_UP)){current=fame_prev_char(current);last_pad=pad;fame_repeat_wait(pad,&repeat_delay);continue;}
        if(pad&(1u<<JOY_DOWN)){current=fame_next_char(current);last_pad=pad;fame_repeat_wait(pad,&repeat_delay);continue;}
        if(pad&(1u<<JOY_LEFT)){
            if(cursor){fame[slot].name[cursor]=current;--cursor;current=fame[slot].name[cursor];}
            last_pad=pad;fame_repeat_wait(pad,&repeat_delay);continue;
        }
        if(pad&(1u<<JOY_RIGHT)){
            if(cursor<5u){fame[slot].name[cursor]=current;++cursor;current=fame[slot].name[cursor];}
            last_pad=pad;fame_repeat_wait(pad,&repeat_delay);continue;
        }
        evt(0x464e414du,(s32)cursor,(s32)(unsigned char)current);
        if(pad&0x22002000u){fame[slot].name[cursor]=current;break;}
        last_pad=pad;
    }
}

static s32 title_pad_wait;
static int title_save_menu;
static void title_move_up(void){if(Choice>0)--Choice;evt(0x544d5550u,Choice,-1);}
static void title_move_down(void){if(Choice<3)++Choice;evt(0x544d444eu,Choice,1);}
static void title_input_loop(void)
{
    title_pad_wait=0;
    for(;;){
        One_Tick();++title_pad_wait;mjppad();
        if(mjp_pressed(JOY_UP)){title_move_up();title_pad_wait=0;wait_bit_release(JOY_UP);continue;}
        if(mjp_pressed(JOY_DOWN)){title_move_down();title_pad_wait=0;wait_bit_release(JOY_DOWN);continue;}
        if(mjp_pad&0x22002000u){title_pad_wait=0;return;}
        if(!title_save_menu && title_pad_wait>=0x320){Choice=4;return;}
    }
}
static int title_save_slot(void)
{
    title_save_menu=1;Choice=0;EncodeGa();evt(0x53415645u,0,0);Wait_Up();
    for(;;){
        title_input_loop();
        if(Choice==3){title_save_menu=0;return 0;}
        if(Choice>=0 && Choice<3 && mjp_games[Choice]!='x'){
            Choice=-(Choice+1);title_save_menu=0;return 1;
        }
    }
}
void Do_Title(void)
{
    Clear_Bu();EncodeGa();evt(0x5449544cu,0,0);title_save_menu=0;Choice=0;
    /* Retail title setup advances at least one VBL before the input selector. */
    One_Tick();
    /* A native menu may report the final retail Choice directly.  Otherwise
     * execute the recovered selector, including the 800-tick attract timeout
     * and the negative save-slot encoding. */
    if(get_choice(AVP_MJP_TITLE)){
        if(Choice<0){s32 slot=-Choice-1;if(slot<0||slot>=3||mjp_games[slot]=='x')Choice=0;}
        else if(Choice!=0&&Choice!=2&&Choice!=4)Choice=0;
        return;
    }
    Wait_Up();
    for(;;){
        title_input_loop();
        if(Choice==0||Choice==2||Choice==4)return;
        if(Choice==1){if(title_save_slot())return;Choice=0;continue;}
        if(Choice==3){evt(0x43524544u,0,0);Wait_Up();Choice=0;continue;}
    }
}

static unsigned carousel_phase,carousel_direction;
static unsigned carousel_species[3];
static void carousel_rotate(void)
{
    if(carousel_direction){
        carousel_phase+=4u;
        if(carousel_phase>=128u){
            unsigned a=carousel_species[0],b=carousel_species[1],c=carousel_species[2];
            carousel_phase=0;carousel_species[0]=c;carousel_species[1]=a;carousel_species[2]=b;
        }
    }else{
        if(carousel_phase>=4u)carousel_phase-=4u;
        else{
            unsigned a=carousel_species[0],b=carousel_species[1],c=carousel_species[2];
            carousel_phase=124u;carousel_species[0]=b;carousel_species[1]=c;carousel_species[2]=a;
        }
    }
    evt(0x53454c52u,(s32)carousel_phase,(s32)carousel_direction);
}
void Do_Selec(void)
{
    Clear_Bu();carousel_phase=0;carousel_direction=0;
    carousel_species[0]=PT_PREDATOR;carousel_species[1]=PT_HUMAN;carousel_species[2]=PT_ALIEN;
    evt(0x53454c45u,0,0);
    if(get_choice(AVP_MJP_SELECT)){
        if(Choice!=PT_HUMAN&&Choice!=PT_ALIEN&&Choice!=PT_PREDATOR)Choice=PT_HUMAN;
        selected_character=(unsigned)Choice;return;
    }
    for(;;){
        carousel_rotate();One_Tick();mjppad();
        if(mjp_pressed(JOY_LEFT)){carousel_direction=1u;continue;}
        if(mjp_pressed(JOY_RIGHT)){carousel_direction=0u;continue;}
        if(mjp_pad&((1u<<FIRE_A)|(1u<<FIRE_B)|(1u<<FIRE_C))){
            unsigned chosen=carousel_direction?carousel_species[2]:carousel_species[1];
            Choice=(s32)chosen;selected_character=chosen;return;
        }
    }
}
void End_Sele(void){evt(0x53454c58,(s32)selected_character,0);}

/* MJP local $01E170.  The retail loop executes One_Tick, increments Pad_Wait,
 * and returns only when Pad_Wait is strictly greater than the programmed
 * count.  A nominal 300 wait therefore consumes 301 ticks.  Skip polling is
 * mode-dependent in the original: Intro and Simulation Terminated always
 * accept A/B/C, while Escape only does so after its final BEE0 transition. */
static int mjp_timed_wait(s32 count)
{
    mjp_pad_wait=0;
    for(;;){
        One_Tick();
        ++mjp_pad_wait;
        if(mjp_pad_wait>count)return 0;
        if(current_mjp_mode==AVP_MJP_INTRO ||
           current_mjp_mode==AVP_MJP_SIM_TERMINATED ||
           (current_mjp_mode==AVP_MJP_ESCAPE && escape_skip_enabled)){
            mjppad();
            if(mjp_pad&0x22002000u){mjp_pad_wait=-1;return -1;}
        }
    }
}
static int mjp_wait_300(void){return mjp_timed_wait(300);}
static void mjp_wait_exact(unsigned ticks){while(ticks--)One_Tick();}
static void mjp_scene_begin(unsigned tag)
{
    /* Retail common local $01D740: Quick_St, Make_Rel, build the two initial
     * display objects, execute the extra $01D7D2 VBL, then load the GPU helper.
     * Phrase packing remains a backend seam; CPU ordering/timing does not. */
    Quick_St();Make_Rel();evt(0x4d4a5042u,(s32)tag,0); /* MJPB */
    One_Tick();Load_GPU();
}
static void mjp_scene_end(unsigned tag)
{
    /* Shared $01DB62 cleanup: clear the back buffer, then Clear_Bu. */
    evt(0x4d4a5045u,(s32)tag,0); /* MJPE */
    Clear();Clear_Bu();
}
static int mjp_text_id(u32 text_id)
{
    /* Authored text bytes stay resource-side; the retail pointer is retained
     * as the stable identity of the exact string selected by the 68000. */
    const AvpRuntimeOps *o=avp_runtime_ops();
    evt(0x4d4a5054u,(s32)text_id,0); /* MJPT */
    if(o->frontend_text){New_Text(o->frontend_text(o->user,text_id));Show_Tex();}
    else text_abort=0;
    return text_abort;
}
static void mjp_audio_id(u32 sound_id,s32 command)
{
    /* $009C6C performs the actual sound/backend operation. */
    evt(0x4d4a5041u,(s32)sound_id,command); /* MJPA */
}
static void mjp_transition(unsigned from,unsigned to)
{
    /* Retail local $01DF2C builds one scaled object per VBL for D5=0..7.
     * Phrase packing and its final OP busy-wait are hardware-owned, but those
     * eight One_Tick calls are CPU-visible presentation timing and must remain. */
    evt(0x4d4a5050u,(s32)from,(s32)to); /* MJPP */
    for(unsigned i=0;i<8u;++i)One_Tick();
    evt(0x4d4a5053u,(s32)from,(s32)to); /* MJPS: synchronous OP completion seam */
}
static void mjp_wait_win_transition(unsigned phase)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    /* Do_Win's two OP loops differ from $01DF2C: they call One_Tick on every
     * pass before Change_D and then poll a Jaguar OP status word for value 2.
     * Keep that loop in C and delegate only the hardware status bit. */
    do{
        One_Tick();
        evt(0x4d4a5750u,(s32)phase,0); /* MJWP */
        if(!o->frontend_transition_done)break;
    }while(!o->frontend_transition_done(o->user,phase));
}
static int mjp_page(unsigned from,unsigned to,u32 text_id)
{
    /* Retail local $01E1F2: transition, clear, show text, then $01E166. */
    mjp_transition(from,to);Clear();if(mjp_text_id(text_id)<0 || text_abort)return -1;
    return mjp_wait_300();
}

void Do_Intro(void)
{
    static const struct {u8 from,to;u32 text;} pages[]={
        {0,1,0x0002AA89u},{1,2,0x0002AB0Fu},{2,3,0x0002AB83u},
        {3,4,0x0002AC14u},{4,5,0x0002ACA6u},{5,6,0x0002AD25u},
        {6,0,0x0002ADA3u}
    };
    mjp_scene_begin(0x494e5452u); /* INTR */
    /* $01D846 clears Buffer1, Buffer2 and Back_Buf before its first transition. */
    Clear();Clear();Clear();
    mjp_transition(0,0);mjp_text_id(0x0002AA18u);
    if(text_abort || mjp_wait_300()<0){mjp_scene_end(0x494e5452u);return;}
    for(unsigned i=0;i<sizeof(pages)/sizeof(pages[0]);++i){
        if(mjp_page(pages[i].from,pages[i].to,pages[i].text)<0){
            mjp_scene_end(0x494e5452u);return;
        }
    }
    /* Normal Intro completion takes the shared $01DB40 transition before the
     * $01DB62 cleanup; an early skip jumps directly to cleanup above. */
    mjp_transition(0,0);
    mjp_scene_end(0x494e5452u);
}
void Do_Fame(void)
{
    /* Both retail entry paths run Quick_St/Make_Rel before the screen-specific
     * work; the title-only (-1) branch then bypasses score insertion. */
    Quick_St();Make_Rel();Clear_Bu();evt(0x46414d45u,0,0);Unpack_F();
    /* Title-screen Hall of Fame: A requests the hidden EEPROM erase sentinel,
     * B exits.  Host menus may report the same -99 sentinel directly. */
    if(player_type==-1){
        Choice=0;if(get_choice(AVP_MJP_FAME)){if(Choice==-99)player_type=-99;return;}
        Wait_Up();
        for(;;){One_Tick();mjppad();if(mjp_pressed(FIRE_A)){player_type=-99;return;}if(mjp_pressed(FIRE_B))return;}
    }

    unsigned insert=0;
    while(insert<5u && fame[insert].score>=score)++insert;
    if(insert==5u){
        evt(0x464e4f4eu,score,5);Choice=0;
        if(get_choice(AVP_MJP_FAME))return;
        Wait_Up();do{One_Tick();mjppad();}while((mjp_pad&0x22002000u)==0u);
        return;
    }
    for(unsigned i=4u;i>insert;--i)fame[i]=fame[i-1u];
    fame[insert].score=score;
    fame[insert].species=player_type==PT_HUMAN?'m':(player_type==PT_ALIEN?'a':'p');
    memset(fame[insert].name,' ',6);fame[insert].name[6]=0;
    Count=(s32)insert;evt(0x46494e53u,score,(s32)insert);
    fame_edit_name(insert);Pack_Fam();Write_EE();
}
void Do_Escap(void)
{
    /* Retail $01DB78..$01DF2A.  The first pass is intentionally unskippable.
     * On normal completion BEE0 becomes one and the assembly recursively JSRs
     * its local $01DBDE body; subsequent passes are skippable.  A loop plus an
     * unwind-clear count preserves that control flow without recursive C. */
    unsigned unwind_clears=0;
    escape_skip_enabled=0;
    mjp_scene_begin(0x45534350u); /* ESCP */
    Clear();Clear();              /* Buffer1 / Buffer2 */

    for(;;){
        Clear();                  /* $01DBDE Back_Buf */
        mjp_transition(0,0);      /* current screen -> first escape page */
        Clear();

        mjp_text_id(0x0002ADDAu);mjp_audio_id(0x00022848u,0x2600);
        mjp_wait_exact(70);
        mjp_text_id(0x0002AE01u);mjp_audio_id(0x00022898u,0x2600);
        mjp_wait_exact(70);
        mjp_text_id(0x0002AE28u);mjp_audio_id(0x000228E8u,0x2600);
        if(mjp_timed_wait(30)<0)break;

        mjp_transition(0,1);Clear();
        mjp_text_id(0x0002AE4Fu);mjp_audio_id(0x00022938u,0x2600);
        mjp_wait_exact(70);
        mjp_text_id(0x0002AE76u);mjp_audio_id(0x00022988u,0x2600);
        if(mjp_timed_wait(30)<0)break;

        mjp_transition(1,2);Clear();mjp_text_id(0x0002AE9Du);
        mjp_audio_id(0x00021718u,-1);
        if(mjp_wait_300()<0)break;
        if(mjp_wait_300()<0)break;
        if(mjp_page(2,3,0x0002AEC4u)<0)break;
        if(mjp_page(3,4,0x0002AF39u)<0)break;
        if(mjp_wait_300()<0)break;
        if(mjp_wait_300()<0)break;
        if(mjp_wait_300()<0)break;

        Clear();mjp_transition(4,0);
        escape_skip_enabled=1;   /* retail $01DF14 */
        ++unwind_clears;         /* one $01DF24 clear after this nested call */
    }

    while(unwind_clears--)Clear();
    evt(0x4d4a5045u,0x45534350u,0);
}

void Do_Lose(void)
{
    const int terminated=(current_mjp_mode==AVP_MJP_SIM_TERMINATED);
    mjp_scene_begin(0x4c4f5345u); /* LOSE */
    Clear();Clear();Clear();
    mjp_transition(0,0);Clear();
    mjp_text_id(terminated?0x0002AFC3u:0x0002B036u);
    if(terminated){
        if(mjp_wait_300()<0)goto done;
        if(mjp_wait_300()<0)goto done;
    }else{
        /* Base Explodes: exactly 600 ticks, then no timeout while waiting for
         * any A/B/C press. */
        mjp_wait_exact(600);
        do{One_Tick();mjppad();}while((mjp_pad&0x22002000u)==0u);
    }
done:
    mjp_transition(0,0);mjp_scene_end(0x4c4f5345u);
}

void Do_Win(void)
{
    /* Exact retail routine is $01FED8..$020190.  An earlier recovered-boundary
     * manifest incorrectly truncated it at $01FEFF; direct binary disassembly
     * shows the continuation and final RTS at $020190. */
    evt(0x57494e20u,(s32)player_type,0x4220); /* WIN: display-state $4220 */
    Clear_Bu();Make_Rel();

    /* Build the full-screen win object, load the transition GPU helper and poll
     * the OP completion state while Change_D tracks the backend-updated source. */
    evt(0x4d4a5700u,(s32)current_mjp_mode,0); /* MJW0 */
    One_Tick();                              /* retail $01FF4A */
    Load_GPU();
    mjp_wait_win_transition(0);

    /* Retail then builds the second full-screen object.  Alien win alone adds
     * the authored $2B7D8 text record at left=35, top=177, line advance=23. */
    evt(0x4d4a5701u,(s32)current_mjp_mode,0); /* MJW1 */
    if(current_mjp_mode==AVP_MJP_ALIEN_WIN){
        evt(0x4d4a574cu,35,177);              /* MJWL */
        evt(0x4d4a5741u,23,0);                /* MJWA */
        mjp_text_id(0x0002B7D8u);
        evt(0x46414d45u,0,0);                 /* Fame_Tex presentation */
    }

    /* $02007C: exactly 600 VBLs.  Retail calls mjppad on every one of
     * those ticks even though the wait itself cannot be skipped, then enters
     * a separate unbounded A/B/C wait which also refreshes the pad each VBL. */
    for(unsigned i=0;i<600u;++i){One_Tick();mjppad();}
    do{One_Tick();mjppad();}while((mjp_pad&0x22002000u)==0u);

    /* The two blitter register programs are pure Jaguar presentation work.
     * Preserve their ordering around the second OP transition. */
    Waitbl();evt(0x4d4a5742u,0,0);            /* MJWB phase 0 */
    Waitbl();evt(0x4d4a5742u,1,0);            /* MJWB phase 1 */
    evt(0x4d4a5702u,(s32)current_mjp_mode,0); /* MJW2 */
    mjp_wait_win_transition(1);
}
typedef struct AvpMjpResource {u16 file;u8 role;} AvpMjpResource;
static const AvpMjpResource res_title[]={{208,0},{207,1},{237,2},{236,3},{233,4},{205,5},{232,6},{234,7},{238,8}};
static const AvpMjpResource res_select[]={{235,0},{206,1},{215,2},{214,3},{213,4},{210,5},{209,6}};
static const AvpMjpResource res_intro[]={{230,0},{211,1},{212,2},{224,3},{219,4},{225,5},{220,6},{235,7}};
static const AvpMjpResource res_fame_base[]={{234,0},{235,1}};
static const AvpMjpResource res_escape[]={{216,0},{222,1},{217,2},{223,3},{218,4},{235,5}};
static const AvpMjpResource res_predwin[]={{226,0},{234,1}};
static const AvpMjpResource res_alienwin[]={{221,0},{234,1}};
static const AvpMjpResource res_terminated[]={{230,0},{235,1}};
static const AvpMjpResource res_explodes[]={{231,0},{235,1}};
static void load_mode_resources(AvpMjpMode m){
    const AvpMjpResource *r=NULL;unsigned n=0;const AvpRuntimeOps*o=avp_runtime_ops();
    switch(m){
    case AVP_MJP_TITLE:r=res_title;n=sizeof(res_title)/sizeof(*r);break;
    case AVP_MJP_SELECT:r=res_select;n=sizeof(res_select)/sizeof(*r);break;
    case AVP_MJP_INTRO:r=res_intro;n=sizeof(res_intro)/sizeof(*r);break;
    case AVP_MJP_FAME:r=res_fame_base;n=sizeof(res_fame_base)/sizeof(*r);break;
    case AVP_MJP_ESCAPE:r=res_escape;n=sizeof(res_escape)/sizeof(*r);break;
    case AVP_MJP_PRED_WIN:r=res_predwin;n=sizeof(res_predwin)/sizeof(*r);break;
    case AVP_MJP_ALIEN_WIN:r=res_alienwin;n=sizeof(res_alienwin)/sizeof(*r);break;
    case AVP_MJP_SIM_TERMINATED:r=res_terminated;n=sizeof(res_terminated)/sizeof(*r);break;
    case AVP_MJP_BASE_EXPLODES:r=res_explodes;n=sizeof(res_explodes)/sizeof(*r);break;
    }
    if(o->frontend_load_resource){
        for(unsigned i=0;i<n;i++)o->frontend_load_resource(o->user,(unsigned)m,r[i].file,r[i].role);
        if(m==AVP_MJP_FAME){
            unsigned file=0;
            if(player_type==PT_HUMAN)file=229u;
            else if(player_type==PT_ALIEN)file=228u;
            else if(player_type==PT_PREDATOR)file=227u;
            if(file)o->frontend_load_resource(o->user,(unsigned)m,file,2u);
        }
    }
}
void MTEST(AvpMjpMode m){
    current_mjp_mode=m;
    /* Exact recovered retail mode map and FILES resource identity.  Decoding and
     * pixel storage are resource/backend work; the 68000-side selection of the
     * assets and controller remains explicit here. */
    load_mode_resources(m);
    switch(m){
    case AVP_MJP_TITLE:Do_Title();break;case AVP_MJP_SELECT:Do_Selec();break;
    case AVP_MJP_INTRO:Do_Intro();break;case AVP_MJP_FAME:Do_Fame();break;
    case AVP_MJP_ESCAPE:Do_Escap();break;
    case AVP_MJP_PRED_WIN:case AVP_MJP_ALIEN_WIN:Do_Win();break;
    case AVP_MJP_SIM_TERMINATED:case AVP_MJP_BASE_EXPLODES:Do_Lose();break;
    }
}
