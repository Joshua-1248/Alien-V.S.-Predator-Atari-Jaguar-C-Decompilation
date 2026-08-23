#ifndef AVP_EEPROM_H
#define AVP_EEPROM_H
#include "avp_types.h"

#define AVP_EE_SIZE 128
#define AVP_SAVE_SIZE 20
#define AVP_N_SAVES 3

typedef u16 (*AvpEeReadWordFn)(u16 index);
typedef void (*AvpEeWriteWordFn)(u16 index,u16 value);
typedef void (*AvpInitHighFn)(u8 *packed_scores,unsigned bytes);

void avp_eeprom_bind(AvpEeReadWordFn rd,AvpEeWriteWordFn wr,AvpInitHighFn init_high);
void Init_EE(void);
void Default_EE(void);
void Trash_EE(void);
u32 Check_EE(void);
void Read_EE(void);
void Write_EE(void);

extern u8 cartcopy[AVP_EE_SIZE];
extern u8 SaveCont[AVP_SAVE_SIZE];
extern u32 savegame;

#endif
