#ifndef AVP_MJP_H
#define AVP_MJP_H
#include "avp_types.h"
#include <stddef.h>
typedef struct AvpMjpObject {u32 data;s32 x,y;s32 height,width;u32 flags;s32 scale_x,scale_y;u8 hidden,scaled;} AvpMjpObject;
typedef enum AvpMjpMode {AVP_MJP_TITLE,AVP_MJP_SELECT,AVP_MJP_INTRO,AVP_MJP_FAME,AVP_MJP_ESCAPE,AVP_MJP_WIN,AVP_MJP_LOSE} AvpMjpMode;
void avp_mjp_bind_objects(AvpMjpObject *objects,unsigned capacity);void avp_mjp_bind_save_slots(const u8 *slots,size_t stride);
void MTEST(AvpMjpMode mode);void mjppad(void);void Load_GPU(void);void Make_Bet(void);void EncodeGa(void);void Clear(void);void Clear2(void);void Do_Fame(void);void Pack_Fam(void);void Unpack_F(void);void Init_Hig(void);void New_Text(const char *text);void Show_Tex(void);void Fame_Tex(void);void Do_Intro(void);void Do_Lose(void);void Do_Escap(void);void One_Tick(void);AvpMjpObject *Make_New(void);void Make_Sca(void);AvpMjpObject *New_Make(void);void Lister4(void);void Change_Y(s32 y);void Hide_Obj(void);void Unhide_O(void);void No_Scale(void);void Change_D(u32 data);void Change_X(s32 x);void Change_S(s32 sx,s32 sy);void Sel_Upda(void);void Lister3(void);void Make_TLi(void);void Update2(void);void Update(void);void Lister(void);void Do_Selec(void);void End_Sele(void);void Pause(void);void Make_Rel(void);void Do_Title(void);void Waitbl(void);void Waitnow(void);void Quick_St(void);void Clear_Bu(void);void Wait_Up(void);void Do_Win(void);
extern char mjp_games[4];extern s32 Count_Do;extern AvpMjpObject *Ob_Addr;extern u32 Ob_Data;extern s32 Ob_Pos_X,Ob_Pos_Y,Ob_Height,Ob_Width,Ob_Trans;
#endif
