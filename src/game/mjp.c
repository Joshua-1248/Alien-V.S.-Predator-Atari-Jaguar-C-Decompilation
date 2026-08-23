/* High-level readable C reconstruction of the retail MJP front-end block.
 * Retail addresses/function boundaries come from the exact MJP reconstruction.
 * Object list phrase encoding and image pixels are platform/resource backends. */
#include "mjp.h"
#include "avp_runtime.h"
#include "joypad.h"
#include "player.h"
#include "objects.h"
#include <string.h>

char mjp_games[4]="xxx";s32 Count_Do;AvpMjpObject *Ob_Addr;u32 Ob_Data;s32 Ob_Pos_X,Ob_Pos_Y,Ob_Height,Ob_Width,Ob_Trans;
static AvpMjpObject *pool;static unsigned pool_cap,pool_count;static const u8 *save_slots;static size_t save_stride=20;static const char *current_text;static unsigned selected_character;
void avp_mjp_bind_objects(AvpMjpObject*o,unsigned n){pool=o;pool_cap=n;pool_count=0;Ob_Addr=NULL;}void avp_mjp_bind_save_slots(const u8*s,size_t stride){save_slots=s;if(stride)save_stride=stride;}
static void evt(unsigned e,s32 a,s32 b){const AvpRuntimeOps*o=avp_runtime_ops();if(o->frontend_event)o->frontend_event(o->user,e,a,b);}
void mjppad(void){readpad();} void Load_GPU(void){evt(0x4d475055,0,0);} void Clear(void){evt(0x434c5231,0,0);} void Clear2(void){evt(0x434c5232,0,0);}void Clear_Bu(void){Clear();Clear2();}
AvpMjpObject *Make_New(void){if(!pool||pool_count>=pool_cap)return NULL;AvpMjpObject*o=&pool[pool_count++];memset(o,0,sizeof(*o));o->data=Ob_Data;o->x=Ob_Pos_X;o->y=Ob_Pos_Y;o->height=Ob_Height;o->width=Ob_Width;o->flags=(u32)Ob_Trans;Ob_Addr=o;return o;}
AvpMjpObject *New_Make(void){return Make_New();}void Make_Sca(void){if(Ob_Addr){Ob_Addr->scaled=1;Ob_Addr->scale_x=Ob_Addr->scale_y=0;}}void Make_Bet(void){Ob_Data=0;Ob_Pos_X=320;Ob_Height=280;Ob_Trans=0;Ob_Width=16;Make_New();if(Ob_Addr){Ob_Addr->flags|=1u<<16;Ob_Addr->flags&=0xffff8fffu;}}
void Change_Y(s32 y){if(Ob_Addr)Ob_Addr->y=y;}void Change_X(s32 x){if(Ob_Addr)Ob_Addr->x=x;}void Change_D(u32 d){if(Ob_Addr)Ob_Addr->data=d;}void Change_S(s32 x,s32 y){if(Ob_Addr){Ob_Addr->scaled=1;Ob_Addr->scale_x=x;Ob_Addr->scale_y=y;}}void Hide_Obj(void){if(Ob_Addr)Ob_Addr->hidden=1;}void Unhide_O(void){if(Ob_Addr)Ob_Addr->hidden=0;}void No_Scale(void){if(Ob_Addr)Ob_Addr->scaled=0;}
void EncodeGa(void){static const char mapx[]="mapx";for(unsigned i=0;i<3;i++){unsigned t=3;if(save_slots){const u8*p=save_slots+i*save_stride;u32 exists=((u32)p[0]<<24)|((u32)p[1]<<16)|((u32)p[2]<<8)|p[3];if(exists){u32 q=((u32)p[16]<<24)|((u32)p[17]<<16)|((u32)p[18]<<8)|p[19];t=q&3u;}}mjp_games[i]=mapx[t];}mjp_games[3]=0;}
void One_Tick(void){VSync();mjppad();evt(0x5449434b,(s32)joy_cur,(s32)joy_edge);}void Waitbl(void){VSync();}void Waitnow(void){VSync();}void Wait_Up(void){do{mjppad();One_Tick();}while((joy_cur&(1u<<JOY_UP))==0u);}void Quick_St(void){evt(0x51535452,0,0);}void Make_Rel(void){evt(0x52454c,0,0);}
void Pause(void){while(Count_Do-->0){mjppad();if(joy_cur&0x22002000u)One_Tick();else{Make_Rel();break;}}}
void New_Text(const char*t){current_text=t;}void Show_Tex(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->draw_text&&current_text)o->draw_text(o->user,0,current_text,0);}void Fame_Tex(void){evt(0x46414d45,0,0);Show_Tex();}
void Pack_Fam(void){evt(0x50464d,0,0);}void Unpack_F(void){evt(0x55464d,0,0);}void Init_Hig(void){evt(0x49484947,0,0);Pack_Fam();}
void Lister4(void){evt(0x4c535434,(s32)pool_count,0);}void Lister3(void){evt(0x4c535433,(s32)pool_count,0);}void Lister(void){evt(0x4c535452,(s32)pool_count,0);}void Update2(void){evt(0x55504432,0,0);}void Update(void){evt(0x55504454,0,0);}void Sel_Upda(void){Update();evt(0x53454c55,(s32)selected_character,0);}void Make_TLi(void){evt(0x544c4953,0,0);}
void Do_Title(void){Clear_Bu();evt(0x5449544c,0,0);One_Tick();}void Do_Selec(void){Clear_Bu();evt(0x53454c45,(s32)selected_character,0);One_Tick();}void End_Sele(void){evt(0x53454c58,(s32)selected_character,0);}void Do_Intro(void){Clear_Bu();evt(0x494e5452,0,0);One_Tick();}void Do_Fame(void){Clear_Bu();Unpack_F();evt(0x46414d45,0,0);One_Tick();}void Do_Escap(void){Clear_Bu();evt(0x45534350,0,0);One_Tick();}void Do_Lose(void){Clear_Bu();evt(0x4c4f5345,0,0);One_Tick();}void Do_Win(void){Clear_Bu();evt(0x57494e,(s32)player_type,0);One_Tick();}
void MTEST(AvpMjpMode m){switch(m){case AVP_MJP_TITLE:Do_Title();break;case AVP_MJP_SELECT:Do_Selec();break;case AVP_MJP_INTRO:Do_Intro();break;case AVP_MJP_FAME:Do_Fame();break;case AVP_MJP_ESCAPE:Do_Escap();break;case AVP_MJP_WIN:Do_Win();break;case AVP_MJP_LOSE:Do_Lose();break;}}
