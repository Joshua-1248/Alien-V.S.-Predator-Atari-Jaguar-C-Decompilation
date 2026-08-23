#ifndef AVP_LEVELS_H
#define AVP_LEVELS_H
#include "avp_types.h"

#define AVP_MAX_LEVEL 15
#define AVP_MAX_PANELS 48

typedef struct AvpLevelInfo {
    u16 maze_file;
    u32 start_x;
    u32 start_y;
    u32 start_angle;
} AvpLevelInfo;

void InitLevels(void);
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
void FixAlien(void);

void avp_levels_set_table(const AvpLevelInfo *table, unsigned count);
void avp_levels_set_save_words(const u8 *save_bytes, unsigned size);

extern s16 cur_level,new_level;
extern s32 new_x,new_y;
extern u32 panel_list[AVP_MAX_PANELS/4];

#endif
