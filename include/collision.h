#ifndef AVP_COLLISION_H
#define AVP_COLLISION_H
#include "avp_types.h"

typedef struct AvpXY { s32 x,y; } AvpXY;

enum AvpCollisionResult { AVP_COLL_SAFE=0, AVP_COLL_WALL=1, AVP_COLL_DOOR=2 };

/* 0x0000 = +X, 0x4000 = +Y, 0x8000 = -X, 0xC000 = -Y. */
u16 Angle(const AvpXY *from,const AvpXY *to);
void Vector(const AvpXY *from,const AvpXY *to,s32 *vx,s32 *vy);

void avp_collision_bind_maze(u8 *maze,u16 width,u16 height);
u8 SafePos(u8 height,s32 xpos,s32 ypos,u16 width);
u8 AllowedMoves(u16 x,u16 y);
u16 NMoves(u16 x,u16 y,u8 dir,u16 wanted);
int LineOfSight(const AvpXY *from,const AvpXY *to); /* 0 = visible */
int AMPCollisions(s32 xpos,s32 ypos);               /* nonzero = hit */
void AreaAmpDamage(s32 xpos,s32 ypos,s32 radius,s16 damage,u16 damage_flags);
void AreaPlDamage(s32 xpos,s32 ypos,s32 radius,s16 damage);
void AreaDamage(s32 xpos,s32 ypos,s32 radius,s16 damage,u16 damage_flags);

#endif
