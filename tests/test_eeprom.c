#include "eeprom.h"
#include <assert.h>
#include <string.h>
static u16 ee[64];
static u16 rd(u16 i){return ee[i];}
static void wr(u16 i,u16 v){ee[i]=v;}
static void hi(u8 *p,unsigned n){if(n){p[0]=0x12;p[1]=0x34;}}
static u32 be32(const u8*p){return ((u32)p[0]<<24)|((u32)p[1]<<16)|((u32)p[2]<<8)|p[3];}
int main(void){
 memset(ee,0xff,sizeof(ee)); avp_eeprom_bind(rd,wr,hi); Init_EE();
 assert(cartcopy[60]==0x12 && cartcopy[61]==0x34); assert(be32(cartcopy+124)==Check_EE());
 cartcopy[0]=0xaa; Write_EE(); memset(cartcopy,0,sizeof(cartcopy)); Read_EE(); assert(cartcopy[0]==0xaa); assert(be32(cartcopy+124)==Check_EE());
 return 0;
}
