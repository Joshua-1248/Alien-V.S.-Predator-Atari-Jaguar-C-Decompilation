#ifndef AVP_GPU_OVERLAYS_H
#define AVP_GPU_OVERLAYS_H
#include "avp_types.h"
/* Descriptor retained for source-level API compatibility. GPU program storage
 * itself belongs to the target backend. */
typedef struct AvpGpuOverlayDesc { unsigned host_index; } AvpGpuOverlayDesc;
void InitMGPU(void);
void ResetMGPU(void);
void avp_gpu_get_base(void);
void avp_gpu_get_overlay(const AvpGpuOverlayDesc *ov);
void avp_gpu_fix_overlay(const AvpGpuOverlayDesc *ov);
#endif
