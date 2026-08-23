/* Readable C reconstruction of MAZE/AVPCART.S.
 * Original attribution retained in provenance: AJ Whittaker (Aug 1994) and
 * MJS Beaton (Sep 1994).  EEPROM primitive I/O is supplied by a backend.
 */
#include "eeprom.h"
#include <string.h>

#define REVISION 12u
#define CHECKPOS (AVP_EE_SIZE-4u)

uintptr_t savegame;
u8 SaveCont[AVP_SAVE_SIZE];
u8 cartcopy[AVP_EE_SIZE];

static AvpEeReadWordFn read_word;
static AvpEeWriteWordFn write_word;
static AvpInitHighFn init_high;

void avp_eeprom_bind(AvpEeReadWordFn rd,AvpEeWriteWordFn wr,AvpInitHighFn high)
{ read_word=rd; write_word=wr; init_high=high; }
/* EEPRIM.S hardware primitives are intentionally represented as backend-bound
 * C entry points.  AVPCART.S owns the save/checksum policy; these two names
 * preserve the historical low-level interface without copying Jaguar GPIO
 * bit-banging into portable game code. */
u16 eeread(u16 index)
{ return read_word ? read_word((u16)(index&63u)) : 0u; }
int eewrite(u16 index,u16 value)
{ if(!write_word)return -1; write_word((u16)(index&63u),value); return 0; }


static u32 load_be32(const u8 *p)
{ return ((u32)p[0]<<24)|((u32)p[1]<<16)|((u32)p[2]<<8)|p[3]; }
static void store_be32(u8 *p,u32 v)
{ p[0]=(u8)(v>>24);p[1]=(u8)(v>>16);p[2]=(u8)(v>>8);p[3]=(u8)v; }

u32 Check_EE(void)
{
    u32 total=0;
    unsigned i;
    /* dbra #CART_LONGS-2 executes 31 times: longs 0..30. */
    for (i=0;i<31;++i) total+=load_be32(cartcopy+i*4u);
    total^=(0x1234faceu-REVISION);
    return total;
}

void Read_EE(void)
{
    unsigned i;
    if (!read_word) return;
    for (i=0;i<AVP_EE_SIZE/2u;++i) {
        u16 v=eeread((u16)i);
        cartcopy[i*2u]=(u8)(v>>8);
        cartcopy[i*2u+1u]=(u8)v;
    }
}

void Write_EE(void)
{
    unsigned i;
    u32 sum=Check_EE();
    store_be32(cartcopy+CHECKPOS,sum);
    if (!write_word) return;
    for (i=0;i<AVP_EE_SIZE/2u;++i) {
        u16 v=(u16)(((u16)cartcopy[i*2u]<<8)|cartcopy[i*2u+1u]);
        (void)eewrite((u16)i,v);
    }
}

void Default_EE(void)
{
    memset(cartcopy,0,sizeof(cartcopy));
    if (init_high) init_high(cartcopy+AVP_N_SAVES*AVP_SAVE_SIZE,
                             CHECKPOS-AVP_N_SAVES*AVP_SAVE_SIZE);
    else {
        /* Retail Init_High/Init_Hig defaults, already in the packed EEPROM
         * representation used by MJP Pack_Fam: MIKE/ANDY/PURPLE/JAMES/KEONI. */
        static const u32 packed[10]={
            0x188513ffu,0x00129da0u,0x40d1e3ffu,0x000c3500u,
            0x9f48bd64u,0x000a2990u,0x5206125fu,0x00061a80u,
            0x1447351fu,0x00030d40u
        };
        u8 *p=cartcopy+AVP_N_SAVES*AVP_SAVE_SIZE;
        for(unsigned i=0;i<10u;i++,p+=4)store_be32(p,packed[i]);
    }
    store_be32(cartcopy+CHECKPOS,Check_EE());
}

void Init_EE(void)
{
    Read_EE();
    if (Check_EE()!=load_be32(cartcopy+CHECKPOS)) Default_EE();
}

void Trash_EE(void) { Default_EE(); Write_EE(); }
