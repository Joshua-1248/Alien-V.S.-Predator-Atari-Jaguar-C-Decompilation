#ifndef AVP_JAGUAR_H
#define AVP_JAGUAR_H
#include "avp_types.h"
extern volatile u32 jaguar_initial_ptr;
extern volatile u16 video_vcenter, video_hcenter, screen_half_width, screen_half_height;
extern volatile u16 top_half_line, top_line, bottom_half_line, bottom_line, video_phase;
extern volatile u32 one_second_ticks;
extern volatile u16 min_frame_ticks, video_mul_a, video_mul_b;
void InitJaguar(void);
void InitVideo(void);
void SetScreen(u32 width,u32 height);
#endif
