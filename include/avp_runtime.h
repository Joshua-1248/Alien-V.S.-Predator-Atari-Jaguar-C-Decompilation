#ifndef AVP_RUNTIME_H
#define AVP_RUNTIME_H
#include "avp_types.h"

/* Host/Jaguar service boundary for the readable C decompilation.
 * The 1994 source called Jaguar MMIO/GPU/DSP/file routines directly.  The C
 * reconstruction keeps game-side ordering in C and routes processor-specific
 * work through this table. */
typedef struct AvpRuntimeOps {
    void (*wait_vblank)(void *user);
    void (*screen_off)(void *user);
    void (*set_maze_list)(void *user);
    void (*restore_maze_list)(void *user);
    void (*swap_screens)(void *user);
    void (*pre_frame)(void *user);
    void (*post_frame)(void *user);
    void (*pause)(void *user);
    void (*gpu_draw_screen)(void *user);
    void (*gpu_reset_maze)(void *user);
    void (*gpu_init_overlays)(void *user);
    void (*gpu_reset_overlays)(void *user);
    /* GOVERS.S CPU-side orchestration.  The actual GPU RISC images stay in
     * the GPU/native backend and are not part of the 68000->C denominator. */
    void (*gpu_load_base)(void *user);
    void (*gpu_load_overlay)(void *user, unsigned overlay);
    void (*gpu_fix_overlay)(void *user, unsigned overlay);
    void (*gpu_set_maze_dimensions)(void *user, u32 width, u32 height);
    void (*init_screen_overlays)(void *user);
    void (*init_sprites)(void *user);
    void (*reset_sprites)(void *user);
    void (*init_computer)(void *user);
    void (*reset_map)(void *user);
    void (*ambient)(void *user);
    void (*kill_ambient)(void *user);
    void (*kill_sounds)(void *user);
    void (*play_sfx)(void *user, unsigned id);
    void (*play_sfx_params)(void *user, unsigned id, s32 left, s32 right);
    void (*start_music)(void *user, unsigned id);
    void (*stop_music)(void *user);
    void (*set_message)(void *user, unsigned id);
    void (*load_level)(void *user, s16 level);
    void (*leave_level)(void *user, s16 level);
    int  (*safe_pos)(void *user, s32 x, s32 y, s16 height, u16 width);
    int  (*amp_collision)(void *user, s32 x, s32 y);
    int  (*door_access)(void *user, u32 door_offset, u8 panel, u8 current_access);
    void (*area_damage)(void *user, s32 x, s32 y, u32 radius, s16 damage, u16 kill_flags);
    int  (*fire_distance)(void *user, s32 x, s32 y, u16 angle, u32 *distance_out);
    void (*draw_text)(void *user, unsigned line, const char *text, unsigned x);
    void (*draw_number)(void *user, s32 value,unsigned digits,unsigned x,unsigned y,unsigned cw,unsigned ch);
    void (*frontend_event)(void *user, unsigned event, s32 arg0, s32 arg1);
    void (*object_event)(void *user, unsigned event, u32 arg0, u32 arg1, u32 arg2);
    void (*file_event)(void *user, unsigned event, u32 arg0, u32 arg1, u32 arg2);
    void *user;
} AvpRuntimeOps;

void avp_runtime_bind(const AvpRuntimeOps *ops);
const AvpRuntimeOps *avp_runtime_ops(void);
void avp_runtime_reset_ops(void);

#endif
