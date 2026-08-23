#include "rom_util.h"
#include <assert.h>
#include <string.h>

static int loads,runs;
static const void *last_image;
static void on_load(void *u,const void *p){(void)u;loads++;last_image=p;}
static void on_run(void *u){(void)u;runs++;}

int main(void)
{
    unsigned char b[19];
    AvpRomUtilOps ops={on_load,on_run,0,0};
    memset(b,0x5a,sizeof(b));
    memzero(b+3,11);
    for (int i=0;i<19;i++) assert(b[i]==((i>=3&&i<14)?0:0x5a));
    avp_rom_util_bind(&ops);
    loadgpu(b);rungpu();
    assert(loads==1 && runs==1 && last_image==b);
    assert(avp_rom_mulsi3(-12345,6789)==(s32)((u32)-12345*(u32)6789));
    assert(avp_rom_divsi3(-1234567,321)==-1234567/321);
    assert(avp_rom_modsi3(-1234567,321)==-1234567%321);
    assert(avp_rom_divsi3((s32)0x80000000u,-1)==(s32)0x80000000u);
    assert(avp_rom_modsi3((s32)0x80000000u,-1)==0);
    return 0;
}
