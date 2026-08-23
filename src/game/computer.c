/* MAZE/COMPUTER.S orchestration. The large terminal page corpus is ROM-owned;
 * menu state and input repeat behavior are represented in portable C. */
#include "computer.h"
#include "joypad.h"
#include "avp_runtime.h"
u32 c_joy_cur,c_joy_edge;u8 repeat_pad;static unsigned overlay_id;static u8 active;
void cls(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->frontend_event)o->frontend_event(o->user,0x434c53,0,0);}void InitComp(void){repeat_pad=0;c_joy_cur=c_joy_edge=0;overlay_id=0;active=0;const AvpRuntimeOps*o=avp_runtime_ops();if(o->init_computer)o->init_computer(o->user);}void xc_readpad(void){readpad();c_joy_cur=joy_cur;c_joy_edge=joy_edge;}void c_readpad(void){u32 old=c_joy_cur;xc_readpad();if(repeat_pad&&c_joy_cur==old)c_joy_edge=c_joy_cur;}
void InitCDisplay(void){active=1;cls();SetCObj1();SetCObj2();}void RestoreCDisplay(void){active=0;const AvpRuntimeOps*o=avp_runtime_ops();if(o->restore_maze_list)o->restore_maze_list(o->user);}void SetCObj1(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->frontend_event)o->frontend_event(o->user,0x434f31,overlay_id,0);}void SetCObj2(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->frontend_event)o->frontend_event(o->user,0x434f32,overlay_id,0);}void NewOver(unsigned id){overlay_id=id;if(active){SetCObj1();SetCObj2();}}
void Computer(void){InitCDisplay();const AvpRuntimeOps*o=avp_runtime_ops();if(o->frontend_event)o->frontend_event(o->user,0x434f4d50,overlay_id,0);RestoreCDisplay();}
