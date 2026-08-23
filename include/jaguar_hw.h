#ifndef AVP_JAGUAR_HW_H
#define AVP_JAGUAR_HW_H
#include "avp_types.h"

#define AVP_MMIO8(a)  (*(volatile u8  *)(uintptr_t)(a))
#define AVP_MMIO16(a) (*(volatile u16 *)(uintptr_t)(a))
#define AVP_MMIO32(a) (*(volatile u32 *)(uintptr_t)(a))

enum {
    JAG_BASE      = 0x00F00000u,
    JAG_OLP       = 0x00F00020u,
    JAG_VMODE     = 0x00F00028u,
    JAG_BG        = 0x00F00058u,
    JAG_INT1      = 0x00F000E0u,
    JAG_CONFIG    = 0x00F14002u,
    JAG_JOYIN     = 0x00F14000u,
    GPU_FLAGS     = 0x00F02100u,
    GPU_END       = 0x00F0210Cu,
    GPU_CTRL      = 0x00F02114u,
    DSP_FLAGS     = 0x00F1A100u,
    DSP_END       = 0x00F1A10Cu,
    DSP_CTRL      = 0x00F1A114u,
    A1_BASE       = 0x00F02200u,
    A1_FLAGS      = 0x00F02204u,
    A1_CLIP       = 0x00F02208u,
    A1_PIXEL      = 0x00F0220Cu,
    A2_BASE       = 0x00F02224u,
    A2_FLAGS      = 0x00F02228u,
    A2_PIXEL      = 0x00F02230u,
    B_CMD         = 0x00F02238u,
    B_COUNT       = 0x00F0223Cu,
    B_PATD        = 0x00F02268u,
};
#endif
