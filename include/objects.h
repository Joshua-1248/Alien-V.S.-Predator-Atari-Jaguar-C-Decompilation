#ifndef AVP_OBJECTS_H
#define AVP_OBJECTS_H
#include "avp_types.h"
#include <stddef.h>
typedef enum AvpObjectType {AVP_OBJ_STOP=0,AVP_OBJ_BRANCH=1,AVP_OBJ_BITMAP=2,AVP_OBJ_SCALED=3} AvpObjectType;
typedef struct AvpObject {AvpObjectType type;u32 data;u16 x,y,w,h;u16 flags;u16 scale_x,scale_y;s16 next;} AvpObject;
void avp_objects_bind(AvpObject *storage,unsigned capacity);
void InitPal(void);void InitInts(void);void VSync(void);void BuildPre(void);void UknBranc(void);void AddBranc(void);void FixBranc(void);void BranchOb(void);void InitObjL(void);void ResetList(void);void BuildList(void);void BuildAt(const u8 *commands);void AddStop(void);void StopList(void);void StopObj(void);void AddFade(u16 fade);void AddScale(u16 sx,u16 sy);void AddUnsca(void);void AddObjec(u32 data,u16 x,u16 y,u16 w,u16 h);void EditObje(unsigned index,const AvpObject *obj);
extern unsigned avp_object_count;
#endif
