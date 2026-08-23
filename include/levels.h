#ifndef AVP_LEVELS_H
#define AVP_LEVELS_H
#include "avp_types.h"

#define AVP_MAX_LEVEL 15
#define AVP_MAX_PANELS 48


typedef struct AvpOverlayPlan {
    u8 file;
    s16 centre_x, centre_y;
    u16 width, height;
    u32 dest_pixel;
    u32 blit_count;
    s32 src_step;
    s32 dst_step;
} AvpOverlayPlan;

/* Portable forms of the CPU-side LEVELS.S panel/overlay interpreter.  The
 * backend still owns compressed resource bytes and Jaguar blitter execution. */
int avp_levels_panel_step(u8 panel_file, int *must_transfer);
void avp_levels_overlay_plan(u8 file, s8 xpos, s8 ypos, u16 width, u16 height, AvpOverlayPlan *out);

typedef struct AvpLevelInfo {
    u16 maze_file;
    u32 start_x;
    u32 start_y;
    u32 start_angle;
} AvpLevelInfo;

void InitLevels(void);
void LoadLevel(void);
void LeaveLevel(void);
void SetStart(void);
void FirstPos(void);
void KillBastards(void);
void place_grid(void);
void xMap(void);
void swapper(void);
void ScreenOff(void);
void do_notice(void);
void add_over(void);
void FixDucts(void);
void FixAlien(void);

void avp_levels_set_table(const AvpLevelInfo *table, unsigned count);
void avp_levels_set_save_words(const u8 *save_bytes, unsigned size);

extern s16 cur_level,new_level;
extern s32 new_x,new_y;
extern u8 panel_list[AVP_MAX_PANELS];
extern u32 panel_pos;
extern uintptr_t walls,floors;

#endif
