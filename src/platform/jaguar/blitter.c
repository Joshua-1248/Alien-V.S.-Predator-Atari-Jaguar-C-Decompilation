/* High-level C translation of AVP's 68000 blitter support.
 * Oracle: BLITTER_exact_sourcefit.s, retail object SHA-256
 * cc05f274fb43d5e458997065d495f4804a7bd2e3af53fb91e05052c8b736ccc2.
 */
#include "blitter.h"
#include "jaguar_hw.h"
#include <stdint.h>

void WaitBlit(void) {
    while ((AVP_MMIO32(B_CMD) & 1u) == 0u) { }
}

void ByteMove(const void *src, void *dst, u32 count) {
    uintptr_t s=(uintptr_t)src, d=(uintptr_t)dst;
    u32 spix=(u32)(s & 7u), dpix=(u32)(d & 7u);
    WaitBlit();
    AVP_MMIO32(A2_FLAGS)=0x18u;
    AVP_MMIO32(A2_BASE)=(u32)(s & ~(uintptr_t)7u);
    AVP_MMIO32(A2_PIXEL)=spix;
    AVP_MMIO32(A1_FLAGS)=0x18u;
    AVP_MMIO32(A1_BASE)=(u32)(d & ~(uintptr_t)7u);
    AVP_MMIO32(A1_PIXEL)=dpix;
    AVP_MMIO32(B_COUNT)=count | 0x00010000u;
    AVP_MMIO32(B_CMD)=(dpix >= spix) ? 0x01800001u : 0x01800005u;
}

void ByteSet(void *dst, u32 count, u32 pattern) {
    uintptr_t d=(uintptr_t)dst;
    u32 pixel=(u32)(d & 7u);
    u32 p=avp_ror32(pattern,pixel*8u);
    WaitBlit();
    AVP_MMIO32(A1_FLAGS)=0x18u;
    AVP_MMIO32(A1_BASE)=(u32)(d & ~(uintptr_t)7u);
    AVP_MMIO32(A1_PIXEL)=pixel;
    AVP_MMIO32(B_PATD)=p;
    AVP_MMIO32(B_PATD+4u)=p;
    AVP_MMIO32(B_COUNT)=count | 0x00010000u;
    AVP_MMIO32(B_CMD)=0x00010000u;
}

void QuickMove(const void *src, void *dst, u32 count) {
    WaitBlit();
    AVP_MMIO32(A1_FLAGS)=40u; AVP_MMIO32(A1_PIXEL)=0u; AVP_MMIO32(A1_BASE)=(u32)(uintptr_t)dst;
    AVP_MMIO32(A2_FLAGS)=40u; AVP_MMIO32(A2_PIXEL)=0u; AVP_MMIO32(A2_BASE)=(u32)(uintptr_t)src;
    AVP_MMIO32(B_COUNT)=(((count+7u)&~7u)>>2) | 0x00010000u;
    AVP_MMIO32(B_CMD)=0x01800001u;
}

void QuickSet(void *dst, u32 count, u32 pattern) {
    WaitBlit();
    AVP_MMIO32(A1_FLAGS)=0x28u; AVP_MMIO32(A1_PIXEL)=0u; AVP_MMIO32(A1_BASE)=(u32)(uintptr_t)dst;
    AVP_MMIO32(B_PATD)=pattern; AVP_MMIO32(B_PATD+4u)=pattern;
    AVP_MMIO32(B_COUNT)=((count>>3)<<1) | 0x00010000u;
    AVP_MMIO32(B_CMD)=0x00010000u;
}

void QuickClear(void *dst, u32 count) { QuickSet(dst,count,0u); }
