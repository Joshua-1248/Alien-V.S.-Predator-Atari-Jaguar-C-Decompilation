#ifndef AVP_TYPES_H
#define AVP_TYPES_H
#include <stdint.h>
#include <stddef.h>

typedef int8_t   s8;
typedef uint8_t  u8;
typedef int16_t  s16;
typedef uint16_t u16;
typedef int32_t  s32;
typedef uint32_t u32;
typedef int64_t  s64;
typedef uint64_t u64;

static inline u32 avp_rol32(u32 v, unsigned n) { n &= 31u; return n ? (v << n) | (v >> (32u-n)) : v; }
static inline u32 avp_ror32(u32 v, unsigned n) { n &= 31u; return n ? (v >> n) | (v << (32u-n)) : v; }
static inline u8 avp_rol8(u8 v, unsigned n) { n &= 7u; return n ? (u8)((u8)(v << n) | (u8)(v >> (8u-n))) : v; }
#endif
