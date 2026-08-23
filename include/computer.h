#ifndef AVP_COMPUTER_H
#define AVP_COMPUTER_H
#include "avp_types.h"
void cls(void);void InitComp(void);void Computer(void);void c_readpad(void);void xc_readpad(void);void RestoreCDisplay(void);void InitCDisplay(void);void SetCObj1(void);void SetCObj2(void);void NewOver(unsigned id);
extern u32 c_joy_cur,c_joy_edge;extern u8 repeat_pad;
#endif
