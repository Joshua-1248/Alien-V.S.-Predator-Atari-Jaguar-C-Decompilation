#ifndef AVP_JOYPAD_H
#define AVP_JOYPAD_H
#include "avp_types.h"
enum { KEY_HASH=0, KEY_9=1, KEY_6=2, KEY_3=3, KEY_0=4, KEY_8=5, KEY_5=6, KEY_2=7,
       OPTION=9, FIRE_C=13, KEY_STAR=16, KEY_7=17, KEY_4=18, KEY_1=19,
       JOY_UP=20, JOY_DOWN=21, JOY_LEFT=22, JOY_RIGHT=23, FIRE_B=25, PAUSE=28, FIRE_A=29 };
extern volatile u32 joy_edge, joy2_edge, joy_cur, joy2_cur;
extern volatile s8 joy_special_mode, reset_enabled, pad_flag2, pad_flag3;
extern volatile u16 reset_wait;
extern volatile u32 reset_time;
extern volatile u32 frame_counter;
extern void (*x_read)(void);
void readpad(void);
void initpad(void);
#endif
