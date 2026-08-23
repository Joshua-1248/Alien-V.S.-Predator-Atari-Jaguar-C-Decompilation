/* High-level C translation of jaguar.o. Register/order semantics follow the retail code. */
#include "jaguar.h"
#include "jaguar_hw.h"
#include "blitter.h"
#include "joypad.h"

volatile u32 jaguar_initial_ptr;
volatile u16 video_vcenter, video_hcenter, screen_half_width, screen_half_height;
volatile u16 top_half_line, top_line, bottom_half_line, bottom_line, video_phase;
volatile u32 one_second_ticks;
volatile u16 min_frame_ticks, video_mul_a, video_mul_b;
extern void InitPal(void), InitObjL(void), InitInts(void), BuildPre(void);
static void VideoHelper_91A2(void){ /* byte-exact Jaguar helper is platform-specific; SetScreen ordering remains here. */ }

void InitJaguar(void) {
    AVP_MMIO32(0x00007F80u)=0;
    AVP_MMIO32(GPU_END)=0x00070007u; AVP_MMIO32(DSP_END)=0x00070007u;
    WaitBlit(); AVP_MMIO32(A1_CLIP)=0;
    AVP_MMIO32(0x00000000u)=0; AVP_MMIO32(0x00000004u)=4;
    AVP_MMIO32(JAG_OLP)=0; jaguar_initial_ptr=0x00009020u;
    AVP_MMIO16(JAG_INT1)=0;
    AVP_MMIO32(GPU_CTRL)=8; AVP_MMIO32(GPU_FLAGS)=0;
    AVP_MMIO32(DSP_CTRL)=8; AVP_MMIO32(DSP_FLAGS)=0;
    initpad();
}

void InitVideo(void) {
    if ((AVP_MMIO16(JAG_CONFIG)&0x10u)==0u) {
        video_vcenter=0x0144; video_hcenter=0x034B; video_mul_a=8; video_mul_b=1; one_second_ticks=0x32; min_frame_ticks=4;
    } else {
        video_vcenter=0x010C; video_hcenter=0x0337; video_mul_a=4; video_mul_b=4; one_second_ticks=0x3C; min_frame_ticks=5;
    }
    SetScreen(320,228); AVP_MMIO16(JAG_BG)=0; AVP_MMIO32(0x00F0002Au)=0;
    InitPal(); InitObjL(); InitInts(); AVP_MMIO16(JAG_VMODE)=0x06C1u;
}

void SetScreen(u32 width,u32 height) {
    u16 hw=(u16)(width>>1), hh=(u16)(height>>1), d2,d3,d4,d5;
    screen_half_width=hw; screen_half_height=hh;
    width=(u16)(hw<<1); height=(u16)(hh<<1);
    d2=video_vcenter; d3=d2; d2=(u16)(d2-(u16)height);
    AVP_MMIO16(0x00F00046u)=d2; top_line=d2; top_half_line=(u16)(d2>>1);
    d3=(u16)(d3+(u16)height); bottom_line=d3; bottom_half_line=(u16)(d3>>1);
    AVP_MMIO16(0x00F00048u)=0xFFFFu;
    d2=4; d5=(u16)(video_mul_a*d2); d4=d2;
    d2=(u16)(d2*(u16)width); d2=(u16)(d2>>1); d3=d2;
    d2=(u16)(-(s16)d2); d2=(u16)(d2+video_hcenter+d4+d5);
    AVP_MMIO16(0x00F00038u)=d2; AVP_MMIO16(0x00F0003Au)=d2;
    video_phase=0; d3|=0x0400u; d4=(u16)(d4*video_mul_b); d3=(u16)(d3-d4+d5);
    AVP_MMIO16(0x00F0003Cu)=d3;
    VideoHelper_91A2(); BuildPre();
}
