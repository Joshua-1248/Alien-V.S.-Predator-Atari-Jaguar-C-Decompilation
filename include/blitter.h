#ifndef AVP_BLITTER_H
#define AVP_BLITTER_H
#include "avp_types.h"
void WaitBlit(void);
void ByteMove(const void *src, void *dst, u32 count);
void ByteSet(void *dst, u32 count, u32 pattern);
void QuickMove(const void *src, void *dst, u32 count);
void QuickSet(void *dst, u32 count, u32 pattern);
void QuickClear(void *dst, u32 count);
#endif
