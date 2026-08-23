#ifndef AVP_ROM_UTIL_H
#define AVP_ROM_UTIL_H
#include "avp_types.h"

typedef void (*AvpRomGpuLoadFn)(void *user, const void *image);
typedef void (*AvpRomGpuRunFn)(void *user);
typedef void (*AvpRomFatalFn)(void *user, unsigned long code);

typedef struct AvpRomUtilOps {
    AvpRomGpuLoadFn load_gpu;
    AvpRomGpuRunFn run_gpu;
    AvpRomFatalFn fatal;
    void *user;
} AvpRomUtilOps;

void avp_rom_util_bind(const AvpRomUtilOps *ops);

/* Historical ROM/util.o link-visible semantic surfaces. */
void fatal(unsigned long code);
void memzero(void *dst, unsigned long bytes);
void msg(void);
void loadgpu(void *image);
void rungpu(void);

/* GCC helper semantics that occupy the tail of the historical util.o slice. */
s32 avp_rom_mulsi3(s32 a, s32 b);
s32 avp_rom_divsi3(s32 a, s32 b);
s32 avp_rom_modsi3(s32 a, s32 b);

#endif
