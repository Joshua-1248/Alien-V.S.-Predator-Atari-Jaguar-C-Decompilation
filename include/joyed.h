#ifndef AVP_JOYED_H
#define AVP_JOYED_H
#include "avp_types.h"
typedef struct JoyEditRecord {
    s32 value;
    s16 x;
    s16 y;
    u16 magnitude;
    u8 pad_0a[10];
} JoyEditRecord;
void MoveData(JoyEditRecord *item,const void *edited_object);
void MoveXYFromJoy(JoyEditRecord *item);
#endif
