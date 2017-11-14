#ifndef MAIN_H
#define MAIN_H

#include "types.h"

void threadStart(void);

#define THREAD_STACK_SIZE 0x4000

#define NTR_MEMMODE_DEFAULT (0)
#define NTR_MEMMODE_BASE (3)

typedef struct _NTR_CONFIG {
	u32 bootNTRVersion;
	u32 isNew3DS;
	u32 firmVersion;

	u32 IoBasePad;
	u32 IoBaseLcd;
	u32 IoBasePdc;
	u32 PMSvcRunAddr;
	u32 PMPid;
	u32 HomeMenuPid;
	
	u32 HomeMenuVersion;
	u32 HomeMenuInjectAddr ; // FlushDataCache Function
	u32 HomeFSReadAddr ;
	u32 HomeFSUHandleAddr;
	u32 HomeCardUpdateInitAddr;
	u32 HomeAptStartAppletAddr ;
	
	u32 KProcessHandleDataOffset;
	u32 KProcessPIDOffset;
	u32 KProcessCodesetOffset;
	u32 ControlMemoryPatchAddr1;
	u32 ControlMemoryPatchAddr2;
	u32 KernelFreeSpaceAddr_Optional;
	u32 KMMUHaxAddr;
	u32 KMMUHaxSize;
	u32 InterProcessDmaFinishState;
	u32 fsUserHandle;
	u32 arm11BinStart;
	u32 arm11BinSize;
	u32 ShowDbgFunc;

	u32 memMode;
	char ntrFilePath[32];
} NTR_CONFIG;

extern u32 __c_bss_start;
extern u32 __c_bss_end;

extern u32 IoBasePad;
extern u32 IoBaseLcd;
extern u32 IoBasePdc;

extern u32 ShowDbgFunc;

extern u32 KProcessCodesetOffset;
extern u32 KProcessPIDOffset ;
extern u32 KProcessHandleDataOffset;
extern u32 HomeAptStartAppletAddr;

extern NTR_CONFIG* ntrConfig;

void createpad(void *counter, void *keyY, void *filename, u32 megabytes, u8 padnum);
void ntrMain(void);
void _ReturnToUser(void);
 
#endif

