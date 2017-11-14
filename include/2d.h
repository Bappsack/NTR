#ifndef D2_H
#define D2_H

#define BOTTOM_HEIGHT 240
#define BOTTOM_WIDTH 320

#define BOTTOM_FRAME1 bottomFrameBuffer
#define BOTTOM_FRAME2 bottomFrameBuffer
#define BOTTOM_FRAME_SIZE	(320 * 240 * 3)

#include "types.h"
#include "math.h"

void paint_pixel(u32 x, u32 y, u32 rgb, u32 screen);
void paint_letter(u8 letter, u32 x, u32 y, u32 rgbTxt, u32 rgbBg, u32 screen);
void paint_square(u32 x, u32 y, u32 rgb, u32 h, u32 w, u32 screen);
void clear(u32 x, u32 y, u32 xs, u32 ys, u32 rgb);

extern u32 bottomFrameBuffer;
#endif