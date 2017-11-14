#include "memory.h"

void write_color(u32 address, u32 rgb){
  write_byte(address, rgb&0xFF);
  write_byte(address+1, (rgb>>8)&0xFF);
  write_byte(address+2, (rgb>>16)&0xFF);
}

void write_byte(u32 address, u8 byte){
  char *a = (char*) address;
  *a = byte;
}

char nibble_to_readable(u8 nibble){
  if (nibble < 10) return nibble+48;
  return nibble+55;
}

u32 byte_to_string(u8 byte, char* ret, u32 max_len){
  if (max_len < 3) return;
  u32 mask = 0x0F;
  u32 i;

  for (i = 0; i < 2; i++){
    ret[1-i] = nibble_to_readable((byte&mask) >> i*4);
    mask = mask << 4;   
  }
  
  ret[2] = 0x00;
  return 0;
}

u32 u32_to_string(u32 byte, char* ret, u32 max_len){
  if (max_len < 9) return;
  u32 mask = 0x0000000F;
  u32 i;

  for (i = 0; i < 8; i++){
    ret[7-i] = nibble_to_readable((byte&mask) >> i*4);
    mask = mask << 4;
  }
  
  ret[8] = 0x00;
  return 0;
}