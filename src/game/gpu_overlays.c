/* Readable 68000-side translation of MAZE/GOVERS.S.
 *
 * The historical routine copied/fixed five Jaguar GPU overlay programs, then
 * installed MAZE0 and invoked the GPU maze-init entry point.  Those RISC
 * program bytes are a separate processor domain.  This C module preserves the
 * 68000 ordering while the runtime backend owns the actual GPU images/fixups. */
#include "gpu_overlays.h"
#include "avp_runtime.h"
#include "maze.h"

static void load_base(void)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    if(o->gpu_load_base)o->gpu_load_base(o->user);
}
static void load_overlay(unsigned i)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    if(o->gpu_load_overlay)o->gpu_load_overlay(o->user,i);
}
static void fix_overlay(unsigned i)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    if(o->gpu_fix_overlay)o->gpu_fix_overlay(o->user,i);
}

void avp_gpu_get_base(void){load_base();}
void avp_gpu_get_overlay(const AvpGpuOverlayDesc *ov)
{
    /* Portable callers can identify an overlay by putting its 1-based index in
     * host_index.  Native Jaguar backends may ignore the descriptor entirely. */
    if(ov)load_overlay(ov->host_index);
}
void avp_gpu_fix_overlay(const AvpGpuOverlayDesc *ov){if(ov)fix_overlay(ov->host_index);}

void InitMGPU(void)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    load_base();
    for(unsigned i=1;i<=5;i++)load_overlay(i);
    if(o->gpu_init_overlays)o->gpu_init_overlays(o->user);
}

void ResetMGPU(void)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    for(unsigned i=1;i<=5;i++)fix_overlay(i);
    if(o->gpu_load_base)o->gpu_load_base(o->user);
    if(o->gpu_set_maze_dimensions)o->gpu_set_maze_dimensions(o->user,maze_width,maze_height);
    if(o->gpu_reset_maze)o->gpu_reset_maze(o->user);
    if(o->gpu_reset_overlays)o->gpu_reset_overlays(o->user);
}
