/*
 * Readable semantic reconstruction of the retail ROM-side util.o module.
 *
 * The final ROM symbol table places util.o at $8031F8..$8033AF (440 bytes):
 *   fatal, memzero, msg, loadgpu, rungpu, and the GCC 32-bit arithmetic
 *   helpers used by the bootstrap.  This file represents their behavior in
 *   portable C; it is not claimed to reproduce the original instruction
 *   encoding or the developers' missing UNZIP/UTIL.S text.
 *
 * Fixed Jaguar GPU/blitter operations are a backend boundary here.  The
 * byte-exact preservation repository remains the oracle for their exact
 * register-level implementation.
 */
#include "rom_util.h"
#include <stdlib.h>
#include <string.h>

static AvpRomUtilOps g_rom_ops;

void avp_rom_util_bind(const AvpRomUtilOps *ops)
{
    if (ops) g_rom_ops=*ops;
    else memset(&g_rom_ops,0,sizeof(g_rom_ops));
}

void fatal(unsigned long code)
{
    if (g_rom_ops.fatal) g_rom_ops.fatal(g_rom_ops.user,code);
    /* The retail routine executes ILLEGAL and therefore cannot return. */
    abort();
}

void memzero(void *dst, unsigned long bytes)
{
    unsigned char *p=(unsigned char *)dst;
    while (bytes--) *p++=0;
}

void msg(void)
{
    /* Retail util.o contains an RTS-only message hook. */
}

void loadgpu(void *image)
{
    if (g_rom_ops.load_gpu) g_rom_ops.load_gpu(g_rom_ops.user,image);
}

void rungpu(void)
{
    if (g_rom_ops.run_gpu) g_rom_ops.run_gpu(g_rom_ops.user);
}

s32 avp_rom_mulsi3(s32 a,s32 b)
{
    /* Avoid relying on host signed-overflow behavior. */
    u32 ua=(u32)a, ub=(u32)b;
    return (s32)(ua*ub);
}

s32 avp_rom_divsi3(s32 a,s32 b)
{
    if (b==0) fatal(0);
    /* The historical helper is a signed 32-bit quotient. */
    if (a==(s32)0x80000000u && b==-1) return (s32)0x80000000u;
    return a/b;
}

s32 avp_rom_modsi3(s32 a,s32 b)
{
    if (b==0) fatal(0);
    if (a==(s32)0x80000000u && b==-1) return 0;
    return a%b;
}
