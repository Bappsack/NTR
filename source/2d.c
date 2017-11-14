#include "2d.h"
#include "font.h"
#include "memory.h"
#include "string.h"

void paint_pixel(u32 x, u32 y, u32 rgb, u32 screen){
    if (x >= BOTTOM_WIDTH || y >= BOTTOM_HEIGHT) return;
    u32 coord = 720*x+720-(y*3);
    write_color(coord+screen, rgb);
}

void paint_letter(u8 letter, u32 x, u32 y, u32 rgbTxt, u32 rgbBg, u32 screen){
	u32 i;
	u32 k;
	u32 c;
	u8 mask;
	u8* _letter;
	u8 l;
	if ((letter < 32) || (letter > 127)) {
		letter = '?';
	}
	c = (letter - 32) * 12;

	for (i = 0; i < 12; i++){
		mask = 0b10000000;
		l = font[i + c];
		for (k = 0; k < 8; k++){
			if ((mask >> k) & l){
				paint_pixel(k + x, i + y, rgbTxt, screen);
			}
			else {
				paint_pixel(k + x, i + y, rgbBg, screen);
			}
		}
	}
}

void paint_square(u32 x, u32 y, u32 rgb, u32 w, u32 h, u32 screen){
  u32 x1, y1;
  for (x1 = x; x1 < x+w; x1++){
    for (y1 = y; y1 < y+h; y1++){
        paint_pixel(x1, y1, rgb, screen);   
    }   
  }
}

void clear(u32 x, u32 y, u32 xs, u32 ys, u32 rgb){
  paint_square(x, y, rgb, xs, ys, BOTTOM_FRAME1);
  paint_square(x, y, rgb, xs, ys, BOTTOM_FRAME2);
}