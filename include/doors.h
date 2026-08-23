#ifndef AVP_DOORS_H
#define AVP_DOORS_H
#include "avp_types.h"
#include "levels.h"
#include <stddef.h>

#define AVP_L2L_SIZE 1024
#define AVP_WB_SOLID    0x01u
#define AVP_WB_DOOR     0x02u
#define AVP_WB_LEFTONLY 0x04u
#define AVP_WB_2SIDES   0x08u
#define AVP_WB_TBDOOR   0x10u
#define AVP_WB_EDGEDOOR 0x20u

/* LEVDOOR.S is authored level metadata rather than executable control flow.
 * These typed records let the retail tables be supplied by the ROM/resource
 * layer while DOORS.S decision/order semantics remain ordinary C. */
enum AvpDoorAccessCode {
    AVP_ACC_UNUSED=-1,
    AVP_ACC_EXIT=-2,
    AVP_ACC_JAMMED=-3,
    AVP_ACC_ESCAPE=-4
};
typedef struct AvpDoorAccessEntry { u16 door_offset; s16 code; } AvpDoorAccessEntry;
typedef struct AvpDoorExitEntry { u16 panel_offset,x,y,level; } AvpDoorExitEntry;
typedef struct AvpDoorInterlockEntry { u16 door_offset; const u16 *required_offsets; size_t required_count; } AvpDoorInterlockEntry;
typedef struct AvpDoorLevelMeta {
    const AvpDoorAccessEntry *access; size_t access_count;
    const AvpDoorExitEntry *exits; size_t exit_count;
    const AvpDoorInterlockEntry *interlocks; size_t interlock_count;
} AvpDoorLevelMeta;
typedef struct AvpDoorPanelIds {
    u8 lift_back,lift_side,wall2,airduct1,computer1,computer2,medical_computer;
} AvpDoorPanelIds;

void InitDoors(void);
void SaveDoors(void);
void ResetDoors(void);
void DoorKeys(void);
void MoveDoors(void);
void InitAccess(void);
void ResetAccess(void);
void avp_doors_bind_maze(u8 *maze,u16 width,u16 height);
void avp_doors_bind_level_meta(const AvpDoorLevelMeta *levels,unsigned count);
void avp_doors_bind_panel_ids(const AvpDoorPanelIds *ids);
int avp_doors_cell_is_liftside(s32 xpos,s32 ypos);

extern u8 fullbits[256];
extern u8 acs_level,lift_key,comp_panel;
extern u16 comp_offset;

#endif
