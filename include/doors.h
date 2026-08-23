#ifndef AVP_DOORS_H
#define AVP_DOORS_H
#include "avp_types.h"
#include "levels.h"

#define AVP_L2L_SIZE 1024
#define AVP_WB_SOLID    0x01u
#define AVP_WB_DOOR     0x02u
#define AVP_WB_LEFTONLY 0x04u
#define AVP_WB_2SIDES   0x08u
#define AVP_WB_TBDOOR   0x10u
#define AVP_WB_EDGEDOOR 0x20u

void InitDoors(void);
void SaveDoors(void);
void ResetDoors(void);
void DoorKeys(void);
void MoveDoors(void);
void InitAccess(void);
void ResetAccess(void);
void avp_doors_bind_maze(u8 *maze,u16 width,u16 height);

extern u8 fullbits[256];
extern u8 acs_level,lift_key;

#endif
