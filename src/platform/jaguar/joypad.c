/* C translation of AVP joypad.o. Active-low Jaguar matrix scan is preserved. */
#include "joypad.h"
#include "jaguar_hw.h"

volatile u32 joy_edge, joy2_edge, joy_cur, joy2_cur;
volatile s8 joy_special_mode, reset_enabled, pad_flag2, pad_flag3;
volatile u16 reset_wait;
volatile u32 reset_time;
volatile u32 frame_counter;
static void default_read_callback(void) { }
void (*x_read)(void)=default_read_callback;
extern void reset(void);

static u32 scan1(void) {
    const u32 force=0xF0FFFFFCu; u32 v=0xFFFFFFFFu,t;
    AVP_MMIO16(JAG_JOYIN)=0x81FEu; t=AVP_MMIO32(JAG_JOYIN)|force; v &= avp_ror32(t,4);
    AVP_MMIO16(JAG_JOYIN)=0x81FDu; t=AVP_MMIO32(JAG_JOYIN)|force; v &= avp_ror32(t,8);
    AVP_MMIO16(JAG_JOYIN)=0x81FBu; t=AVP_MMIO32(JAG_JOYIN)|force; t=avp_rol32(t,6); t=avp_rol32(t,6); v &= t;
    AVP_MMIO16(JAG_JOYIN)=0x81F7u; t=AVP_MMIO32(JAG_JOYIN)|force; v &= avp_rol32(t,8);
    return v;
}
static u32 rol_low_byte(u32 x,unsigned n){ return (x&0xFFFFFF00u)|avp_rol8((u8)x,n); }
static u32 scan2(void) {
    const u32 force=0x0FFFFFF3u; u32 v=0xFFFFFFFFu,t;
    AVP_MMIO16(JAG_JOYIN)=0x817Fu; t=rol_low_byte(AVP_MMIO32(JAG_JOYIN)|force,2); v &= avp_ror32(t,8);
    AVP_MMIO16(JAG_JOYIN)=0x81BFu; t=rol_low_byte(AVP_MMIO32(JAG_JOYIN)|force,2); t=avp_ror32(t,8); v &= avp_ror32(t,4);
    AVP_MMIO16(JAG_JOYIN)=0x81DFu; t=rol_low_byte(AVP_MMIO32(JAG_JOYIN)|force,2); v &= avp_rol32(t,8);
    AVP_MMIO16(JAG_JOYIN)=0x81EFu; t=rol_low_byte(AVP_MMIO32(JAG_JOYIN)|force,2); v &= avp_rol32(t,4);
    return v;
}
static u32 edge_from(u32 oldv,u32 cur){ return (~(oldv^cur)) | cur; }

void readpad(void) {
    u32 p1=scan1(), p2, both, e=edge_from(joy_cur,p1);
    if (joy_special_mode && ((p1&(1u<<OPTION))==0u)) { p1 &= ~(1u<<31); e &= ~(1u<<31); }
    joy_cur=p1; joy_edge=e; both=p1;
    p2=scan2(); e=edge_from(joy2_cur,p2);
    if (joy_special_mode && ((p2&(1u<<OPTION))==0u)) { p2 &= ~(1u<<31); e &= ~(1u<<31); }
    joy2_cur=p2; joy2_edge=e;
    if (reset_enabled) {
        both &= p2; both &= 0x00010001u;
        if (both==0u) {
            if (reset_wait==0u) { reset_wait=0xFFFFu; reset_time=frame_counter; }
            if ((u32)(frame_counter-reset_time) >= 12u) reset();
        } else reset_wait=0u;
    } else reset_wait=0u;
    x_read();
}

void initpad(void) {
    reset_wait=0; reset_enabled=-1; joy_special_mode=-1; pad_flag2=-1; pad_flag3=0;
    joy_cur=joy2_cur=0; x_read=default_read_callback;
}
