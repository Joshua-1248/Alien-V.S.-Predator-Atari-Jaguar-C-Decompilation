#ifndef AVP_SPRITES_H
#define AVP_SPRITES_H
#include "avp_types.h"
#include <stddef.h>
typedef struct AvpSpriteRecord { const u8 *src; void *expanded; u32 size; u32 flags; } AvpSpriteRecord;
void avp_sprites_bind(AvpSpriteRecord *records,unsigned count);
void InitSprites(void); void ResetSprites(void); int expand_pic(unsigned index,void *dst,size_t cap,size_t *written);
#endif
