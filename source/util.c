/*
*   util.c
*/

#include "global.h"

void mystrcat(char* a, char* b){
	while(*a != 0) {
		a++;
	}
	while(*b != 0) {
		*a = *b;
		a++;
		b++;
	}
	*a = 0;
}

void  myitoa(u32 a, char* b){
	u8 i = 0;
	u8 t;
	for (i = 0; i < 8; i++) {
		t = ((a & 0xf0000000) >> 28) + '0';
		a = a << 4;
		if (t > '9') {
			t += 'A' - '9' - 1;
		}
		*b = t;
		b++;
	}
	*b = 0;
}