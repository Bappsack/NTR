#ifndef MEMORY_H
#define MEMORY_H
#include "math.h"
#include "types.h"


void write_color(u32 address, u32 rgb);
void write_byte(u32 address, u8 byte);
char nibble_to_readable(u8 nibble);
u32 byte_to_string(u8 byte, char* ret, u32 max_len);
u32 u32_to_string(u32 byte, char* ret, u32 max_len);

#endif

