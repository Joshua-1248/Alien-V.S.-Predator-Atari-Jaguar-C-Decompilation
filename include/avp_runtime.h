#ifndef AVP_RUNTIME_H
#define AVP_RUNTIME_H
#include "avp_types.h"

/* Host/Jaguar service boundary for the readable C decompilation.
 * The 1994 source called Jaguar MMIO/GPU/DSP/file routines directly.  The C
 * reconstruction keeps game-side ordering in C and routes processor-specific
 * work through this table. */
typedef struct AvpRuntimeOps {
    void (*wait_vblank)(void *user);
    void (*wait_blitter)(void *user);
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
    /* 68000 AVPSOUND allocates stable effect handles.  These optional host
     * hooks let a native mixer mirror the historical per-voice operations. */
    void (*stop_sfx)(void *user, u32 handle);
    void (*mod_sfx)(void *user, u32 handle, s32 value);
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
    void (*draw_font_glyph)(void *user, unsigned brush, unsigned source_x, unsigned width, unsigned x, unsigned y, int clear);
    void (*draw_number)(void *user, s32 value,unsigned digits,unsigned x,unsigned y,unsigned cw,unsigned ch);
    void (*frontend_event)(void *user, unsigned event, s32 arg0, s32 arg1);
    int  (*frontend_choice)(void *user, unsigned mode, s32 *choice_inout);
    /* Optional text-entry bridge.  The MJP C state machine owns score insertion,
     * cursor rules and EEPROM packing; a host UI may only supply six entered
     * characters here.  Return nonzero when out_name was filled. */
    int  (*frontend_fame_name)(void *user, char out_name[7]);
    /* MJP Object Processor transitions expose a hardware-owned completion flag
     * that the 68000 polls.  Return nonzero once the requested transition phase
     * has reached the retail completion state (flag value 2).  With no backend
     * installed, the portable controller treats the transition as synchronously
     * complete after its first polling tick. */
    int  (*frontend_transition_done)(void *user, unsigned phase);
    /* MJP authored text is retail data, not code.  A ROM/resource backend can
     * resolve the original retail text address to its locally extracted byte
     * stream.  Streams may contain NUL-separated lines and a final empty line. */
    const char *(*frontend_text)(void *user, u32 retail_address);
    void (*frontend_load_resource)(void *user, unsigned mode, unsigned file_index, unsigned role);
    void (*object_event)(void *user, unsigned event, u32 arg0, u32 arg1, u32 arg2);
    void (*file_event)(void *user, unsigned event, u32 arg0, u32 arg1, u32 arg2);
    void *user;
} AvpRuntimeOps;

void avp_runtime_bind(const AvpRuntimeOps *ops);
const AvpRuntimeOps *avp_runtime_ops(void);
void avp_runtime_reset_ops(void);

#endif
