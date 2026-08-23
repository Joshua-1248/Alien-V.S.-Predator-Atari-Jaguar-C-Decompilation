#include "rom_bootstrap.h"
#include <assert.h>

static int prep,allocs,inits,inflates,jumps;
static u8 *expected_out;
static int prepare(void *u){(void)u;return 0;}
static void prep_hw(void *u){(void)u;prep++;}
static int init_a(void *u){(void)u;allocs++;return 0;}
static int init_i(void *u){(void)u;inits++;return 0;}
static int inflate(void *u,const u8 *in,u8 **out){(void)u;assert(in);inflates++;*out+=7;expected_out=*out;return 0;}
static void jump(void *u,void *entry){(void)u;assert(entry==(void *)0x1234);jumps++;}
int main(void)
{
    u8 a=1,b=2,ram[32];
    AvpRomBootstrapOps ops={prep_hw,init_a,init_i,inflate,jump,0};
    AvpRomBootstrapImage img={&a,&b,ram,(void *)0x1234};
    (void)prepare;
    assert(avp_rom_bootstrap_run(&ops,&img)==0);
    assert(prep==1&&allocs==1&&inits==1&&inflates==2&&jumps==1);
    assert(expected_out==ram+14);
    return 0;
}
