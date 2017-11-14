#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define U64_MAX	UINT64_MAX
#define RGB(r,g,b) (unsigned)((r<<16) | (g<<8) | b)

#define WHITE 0xFFFFFF
#define BLACK 0
#define RED 0xFF0000
#define GREEN 0xFF00
#define BLUE 0xFF
#define DARKGREY 0x080808
#define MAGENTA 0xFF00FF

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef volatile u8 vu8;
typedef volatile u16 vu16;
typedef volatile u32 vu32;
typedef volatile u64 vu64;

typedef volatile s8 vs8;
typedef volatile s16 vs16;
typedef volatile s32 vs32;
typedef volatile s64 vs64;

typedef u32 Handle;
typedef s32 Result;

struct POINT {
	int x, y;
};
typedef struct POINT point;

#endif
