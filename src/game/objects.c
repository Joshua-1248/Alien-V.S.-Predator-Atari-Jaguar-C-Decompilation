/* Portable C representation of the OBJECTS/Object Processor 68000 support.
 * Jaguar phrase encoding itself is a platform backend concern; list construction
 * and mutation semantics stay here. */
#include "objects.h"
#include "avp_runtime.h"
#include <string.h>
static AvpObject *objs;static unsigned cap;unsigned avp_object_count;static s16 pending_branch=-1;
void avp_objects_bind(AvpObject*s,unsigned c){objs=s;cap=c;avp_object_count=0;pending_branch=-1;}
static AvpObject*push(AvpObjectType t){if(!objs||avp_object_count>=cap)return NULL;AvpObject*o=&objs[avp_object_count++];memset(o,0,sizeof(*o));o->type=t;o->next=-1;return o;}
void InitPal(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->object_event)o->object_event(o->user,1,0x00ff,0x0100,256);}void InitInts(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->object_event)o->object_event(o->user,2,0,0,0);}void VSync(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->wait_vblank)o->wait_vblank(o->user);}void BuildPre(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->object_event)o->object_event(o->user,3,0,0,0);}
void ResetList(void){avp_object_count=0;pending_branch=-1;}void InitObjL(void){BuildPre();ResetList();StopList();}void BuildList(void){BuildAt(NULL);}void BuildAt(const u8*c){ResetList();if(!c)return;while(*c){switch(*c++){case 1:AddBranc();break;case 2:AddStop();break;case 3:AddUnsca();break;default:return;}}}
void UknBranc(void){pending_branch=-1;AddBranc();}void AddBranc(void){AvpObject*o=push(AVP_OBJ_BRANCH);if(o)pending_branch=(s16)(avp_object_count-1);}void BranchOb(void){AddBranc();}void FixBranc(void){if(pending_branch>=0&&(unsigned)pending_branch<avp_object_count)objs[pending_branch].next=(s16)avp_object_count;pending_branch=-1;}
void AddStop(void){StopList();}void StopList(void){(void)push(AVP_OBJ_STOP);}void StopObj(void){StopList();}
void AddFade(u16 f){AvpObject*o=push(AVP_OBJ_BITMAP);if(o)o->flags=f;}void AddScale(u16 sx,u16 sy){if(avp_object_count){AvpObject*o=&objs[avp_object_count-1];o->type=AVP_OBJ_SCALED;o->scale_x=sx;o->scale_y=sy;}}void AddUnsca(void){if(avp_object_count)objs[avp_object_count-1].type=AVP_OBJ_BITMAP;}
void AddObjec(u32 d,u16 x,u16 y,u16 w,u16 h){AvpObject*o=push(AVP_OBJ_BITMAP);if(o){o->data=d;o->x=x;o->y=y;o->w=w;o->h=h;}}void EditObje(unsigned i,const AvpObject*o){if(objs&&o&&i<avp_object_count)objs[i]=*o;}
