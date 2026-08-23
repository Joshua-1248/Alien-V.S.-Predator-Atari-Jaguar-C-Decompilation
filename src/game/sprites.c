/* C reconstruction of SPRITES/sprites.o orchestration. Sprite payloads remain
 * user-ROM-derived; this module owns expansion state and delegates codec work. */
#include "sprites.h"
#include "files.h"
#include "avp_runtime.h"
#include <string.h>
static AvpSpriteRecord *recs;static unsigned nrecs;
void avp_sprites_bind(AvpSpriteRecord*r,unsigned n){recs=r;nrecs=n;}
void InitSprites(void){if(recs)for(unsigned i=0;i<nrecs;i++)recs[i].expanded=NULL;const AvpRuntimeOps*o=avp_runtime_ops();if(o->init_sprites)o->init_sprites(o->user);}
void ResetSprites(void){if(recs)for(unsigned i=0;i<nrecs;i++)recs[i].expanded=NULL;const AvpRuntimeOps*o=avp_runtime_ops();if(o->reset_sprites)o->reset_sprites(o->user);}
int expand_pic(unsigned i,void*d,size_t cap,size_t*w){if(!recs||i>=nrecs||!recs[i].src)return 0;int ok=TransFil(recs[i].src,d,cap,w);if(ok)recs[i].expanded=d;return ok;}
