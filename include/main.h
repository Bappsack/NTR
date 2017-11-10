#ifndef MAIN_H
#define MAIN_H

#include "3dstypes.h"
#include "..\..\BootNTR\source\ntr_config.h"

extern u32 __c_bss_start;
extern u32 __c_bss_end;

void createpad(void *counter, void *keyY, void *filename, u32 megabytes, u8 padnum);
int main();

extern u32 IoBasePad;
extern u32 IoBaseLcd;
extern u32 IoBasePdc;

extern u32 ShowDbgFunc;

extern u32 KProcessCodesetOffset;
extern u32 KProcessPIDOffset ;
extern u32 KProcessHandleDataOffset;
extern u32 HomeAptStartAppletAddr;


extern NTR_CONFIG* ntrConfig;
 
#endif

