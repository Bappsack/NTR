#define NDS
#define ENABLEWR
#include "global.h"

u32 nsDefaultMemRegion = 0x200;

void sleep(s64 ns);

u32* threadStack = 0;
NTR_CONFIG backupNtrConfig = { 0 };
NTR_CONFIG* ntrConfig;
FS_archive sdmcArchive = { 0x9, (FS_path){ PATH_EMPTY, 1, (u8*)"" } };
Handle fsUserHandle = 0;

u32 StartMode = 0;
u32 IoBasePad;
u32 IoBaseLcd;
u32 IoBasePdc;
u32 ShowDbgFunc;

RT_HOOK HomeFSReadHook;
typedef u32(*FSReadTypeDef) (u32 a1, u32 a2, u32 a3, u32 a4, u32 buffer, u32 size);
extern int _BootArgs[];

u32 ScreenshotHotkey = PAD_SELECT | PAD_START;
u32 NTRMenuHotkey = PAD_X | PAD_Y;

RT_HOOK HomeCardUpdateInitHook;

u32 initValuesFromFIRM() {
	u32 kversion = *(unsigned int *)0x1FF80000;
	ntrConfig->firmVersion = 0;
	if (kversion == 0x022C0600 || kversion == 0x022E0000) {
		ntrConfig->firmVersion = 92;
		ntrConfig->PMSvcRunAddr = 0x00102FEC;
		ntrConfig->ControlMemoryPatchAddr1 = 0xdff884ec;
		ntrConfig->ControlMemoryPatchAddr2 = 0xdff884f0;

	}
	if (kversion == 0x022d0500) {
		ntrConfig->firmVersion = 81;
		ntrConfig->PMSvcRunAddr = 0x0010308C;
		ntrConfig->ControlMemoryPatchAddr1 = 0xdff88158;
		ntrConfig->ControlMemoryPatchAddr2 = 0xdff8815C;
	}
	return ntrConfig->firmVersion;
}

u32 initValuesFromHomeMenu() {
	u32 t = 0;
	u32 hProcess, ret;
	ntrConfig->HomeMenuVersion = 0;
	svc_openProcess(&hProcess, ntrConfig->HomeMenuPid);
	copyRemoteMemory(CURRENT_PROCESS_HANDLE, &t, hProcess, (void*)0x00200000, 4);
	if (t == 0xe59f80f4) {
		ntrConfig->HomeMenuVersion = 92;
		ntrConfig->HomeMenuInjectAddr = 0x131208;
		ntrConfig->HomeFSReadAddr = 0x0012F6EC;
		ntrConfig->HomeCardUpdateInitAddr = 0x139900;
		ntrConfig->HomeFSUHandleAddr = 0x002F0EFC;
		ntrConfig->HomeAptStartAppletAddr = 0x00131C98;

	} 
	if (t == 0xE28DD008) {
		ntrConfig->HomeMenuVersion = 91;
		ntrConfig->HomeMenuInjectAddr = 0x00131208;
		ntrConfig->HomeFSReadAddr = 0x0012F6EC;
		ntrConfig->HomeCardUpdateInitAddr = 0x139900;
		ntrConfig->HomeFSUHandleAddr = 0x002F1EFC;
		ntrConfig->HomeAptStartAppletAddr = 0x00131C98;
	}
	if (t == 0xE1B03F02) {
		ntrConfig->HomeMenuVersion = 90;
		ntrConfig->HomeMenuInjectAddr = 0x00130CFC;
		ntrConfig->HomeFSReadAddr = 0x0012F224;
		ntrConfig->HomeCardUpdateInitAddr = 0x001393F4;
		ntrConfig->HomeFSUHandleAddr = 0x002EFEFC;
		ntrConfig->HomeAptStartAppletAddr = 0x0013178C;
	}
	if (t == 0xE28F2E19) {
		ntrConfig->HomeMenuVersion = 81;
		ntrConfig->HomeMenuInjectAddr = 0x00129098;
		ntrConfig->HomeFSReadAddr = 0x0011AAB8;
		ntrConfig->HomeCardUpdateInitAddr = 0x0013339C;
		ntrConfig->HomeFSUHandleAddr = 0x00278E4C;
		ntrConfig->HomeAptStartAppletAddr = 0x00129BFC;
	}
final:
	svc_closeHandle(hProcess);
	return ntrConfig->HomeMenuVersion;
}

void clearDisp(u32 rgb) {
	u32 i; for ( i = 0; i < 100; i++){
		*(vu32*)(IoBaseLcd + 0x204) = 0x1000000 | ((rgb&0xFF)<<16 | (rgb&0xFF0000)>>16 | rgb&0xFF00);
		svc_sleepThread(5000000);
	}
	*(vu32*)(IoBaseLcd + 0x204) = 0;
}

void setExitFlag() {
	g_nsConfig->exitFlag = 1;
	svc_closeHandle(g_nsConfig->hSOCU);
}

void onFatalError(u32 lr) {
acquireVideo();
Log("fatal. LR: %08x", lr, 0);
releaseVideo();
}

void viewFile(FS_archive arc, char* path) {
	u8 buf[0x5000];
	u32 off = 0;
	u16 entry[0x228];
	u32 i = 0;
	u8* captions[1000];
	u32 entryCount = 0;

	buf[0] = 0;
	FS_path dirPath = (FS_path){ PATH_CHAR, strlen(path) + 1, path };
	Handle dirHandle = 0;
	u32 ret;
	ret = FSUSER_OpenDirectory(fsUserHandle, &dirHandle, arc, dirPath);
	if (ret != 0) {
		xsprintf(buf, "FSUSER_OpenDirectory failed, ret=%08x", ret);
		Log(buf);
		return;
	}
	while (1) {
		u32 nread = 0;
		FSDIR_Read(dirHandle, &nread, 1, (u16*)entry);
		if (!nread) break;
		captions[entryCount] = &buf[off];
		for (i = 0; i < 0x228; i++) {
			u16 t = entry[i];
			if (t == 0) {
				break;
			}
			buf[off] = (u8)t;
			off += 1;
		}
		buf[off] = 0;
		off += 1;
		entryCount += 1;
	}
	if (entryCount == 0) {
		Log("no file found.");
		return;
	}
	while (1) {
		s32 r = showMenu(path, entryCount, captions);
		if (r == -1) {
			break;
		}
		xsprintf((u8*)entry, "%s%s", path, captions[r]);
		viewFile(arc, (u8*)entry);
	}
	FSDIR_Close(dirHandle);
}

void fileManager() {
	FS_archive arc;
	arc = (FS_archive){ 0x567890AB, (FS_path){ PATH_EMPTY, 1, (u8*)"" } };
	u32 ret = FSUSER_OpenArchive(fsUserHandle, &arc);
	if (ret != 0) {
		Log("openArchive failed, ret=%08x", ret);
		return;
	}
	viewFile(arc, "/");
	FSUSER_CloseArchive(fsUserHandle, &arc);
}

void checkExitFlag() {
	if (g_nsConfig->exitFlag) {
		svc_exitThread();
	}
}

u32 HomeFSReadCallback(u32 a1, u32 a2, u32 a3, u32 a4, u32 buffer, u32 size) {
	u32 ret;
	ret = ((FSReadTypeDef)((void*)HomeFSReadHook.callCode))(a1, a2, a3, a4, buffer, size);
	if (size == 0x36C0) {
		if ((*((u32*)(buffer))) == 0x48444d53) { // 'SMDH'
			ntrDebugLog("patching smdh\n");
			*((u32*)(buffer + 0x2018)) = 0x7fffffff;
		}
	}
	return ret;
}

u32 HomeCardUpdateInitCallback() {
	return 0xc821180b; // card update is not needed
}

u32 isFileExist(char* fileName) {
	Handle hFile = 0;
	u32 ret;

	FS_path testPath = (FS_path){ PATH_CHAR, strlen(fileName) + 1, (u8*)fileName };
	ret = FSUSER_OpenFileDirectly(fsUserHandle, &hFile, sdmcArchive, testPath, 1, 0);
	if (ret != 0) {
		return 0;
	}
	FSFILE_Close(hFile);
	return 1;
}

void magicKillProcess(u32 pid) {
	Handle hProcess;
	u32 ret = 0;
	ret = svc_openProcess(&hProcess, pid);
	if (ret != 0) {
		return;
	}
	u32 KProcess = kGetKProcessByHandle(hProcess);
	u32 t = 0;
	kmemcpy(&t, KProcess + 4, 4);
	//Log("refcount: %08x", t, 0);
	t = 1;
	kmemcpy(KProcess + 4, &t, 4);
	svc_closeHandle(hProcess);
}

RT_HOOK HomeSetMemorySizeHook;
typedef u32(*SetMemorySizeTypedef) (u32);

u32 HomeSetMemorySizeCallback(u32 size) {
	size -= 0x00100000;
	return ((SetMemorySizeTypedef)((void*)HomeSetMemorySizeHook.callCode))(size);
}

void threadStart() {
	volatile vu32* ptr;
	Handle testFileHandle = 0;
	u32 i, ret;

	rtInitHook(&HomeFSReadHook, ntrConfig->HomeFSReadAddr, (u32)HomeFSReadCallback);
	rtEnableHook(&HomeFSReadHook);
	rtInitHook(&HomeCardUpdateInitHook, ntrConfig->HomeCardUpdateInitAddr, (u32)HomeCardUpdateInitCallback);
	rtEnableHook(&HomeCardUpdateInitHook);

	fsUserHandle = *((u32*)ntrConfig->HomeFSUHandleAddr);
	sdmcArchive = (FS_archive){ 0x9, (FS_path){ PATH_EMPTY, 1, (u8*)"" } };
	ret = initDirectScreenAccess();
	if (ret != 0) clearDisp(RED);
	g_nsConfig->initMode = NS_INITMODE_FROMBOOT;
	initSrv();
	nsInitDebug();

	if (isFileExist("/debug.flag")) startDebugger();
    
	svc_sleepThread(1000000000);
	plgInitFromInjectHOME();
    
	ntrToolsMain();
    
    int waitCnt = 0;
	while (1) {
		if (getKey() == NTRMenuHotkey) {
			if (allowDirectScreenAccess) {
				plgShowMainMenu();
			}
		}
		if (getKey() == ScreenshotHotkey) do_screen_shoot();
		svc_sleepThread(100000000);
		if ((waitCnt += 1) % 10 == 0) lockCpuClock();
		checkExitFlag();
	}
}

void initConfigureMemory() {
	u32 ret;
	u32 outAddr;
	
	g_nsConfig = (void*) NS_CONFIGURE_ADDR;
	ret = protectMemory((void*)NS_CONFIGURE_ADDR, 0x1000);
	if (ret != 0) {

		ret = svc_controlMemory(&outAddr, NS_CONFIGURE_ADDR, 0, 0x1000, 3, 3);
		if (ret != 0) {
			Log("init cfg memory failed");
			return; 
		}
		
		memset(g_nsConfig, 0, sizeof(NS_CONFIG));
		g_nsConfig->initMode = 0;
		return;
	}
	
}

void startupFromInject() {
	clearDisp(BLUE);
	startDebugger();
	clearDisp(RED);
	svc_exitThread(0);
}

void injectToHomeMenu() {
	NS_CONFIG cfg;
	memset(&cfg, 0, sizeof(NS_CONFIG));
	Handle hProcess = 0;
	svc_openProcess(&hProcess, ntrConfig->HomeMenuPid);

	u32* bootArgs = arm11BinStart + 4;
	bootArgs[0] = 1;

	nsAttachProcess(hProcess, ntrConfig->HomeMenuInjectAddr, &cfg, 1);
	svc_closeHandle(hProcess);
}

void doSomething() {
	u32 i; for (i = 0; i < 10; i++) svc_sleepThread(10000000);
}

void loadParams() {

	KProcessCodesetOffset = ntrConfig->KProcessCodesetOffset;
	KProcessHandleDataOffset = ntrConfig->KProcessHandleDataOffset;
	KProcessPIDOffset = ntrConfig->KProcessPIDOffset;
	IoBasePad = ntrConfig->IoBasePad;
	IoBaseLcd = ntrConfig->IoBaseLcd;
	IoBasePdc = ntrConfig->IoBasePdc;
	nsDefaultMemRegion = 0x200;
	if (ntrConfig->memMode == NTR_MEMMODE_BASE) {
		nsDefaultMemRegion = 0x300;
	}
}

void initParamsFromLegacyBootNtr() {
	ntrConfig = &(backupNtrConfig);
	ShowDbgFunc = _BootArgs[1];
	fsUserHandle = _BootArgs[2];
	arm11BinStart = _BootArgs[3];
	u32 size = _BootArgs[4];
	arm11BinSize = rtAlignToPageSize(size);
	ntrConfig->PMPid = 2;
	ntrConfig->HomeMenuPid = 0xf;
	initValuesFromHomeMenu();
	initValuesFromFIRM();
	ntrConfig->IoBasePad = 0xfffc2000;
	ntrConfig->IoBaseLcd = 0xfffc4000;
	ntrConfig->IoBasePdc = 0xfffbc000;
	ntrConfig->KMMUHaxAddr = 0xfffba000;
	ntrConfig->KMMUHaxSize = 0x00010000;

	ntrConfig->KProcessHandleDataOffset = 0xdc;
	ntrConfig->KProcessPIDOffset = 0xBC;
	ntrConfig->KProcessCodesetOffset = 0xB8;

	loadParams();
}

void initParamsFromBootNtr() {
	ntrConfig = (void*)_BootArgs[2];
	ShowDbgFunc = ntrConfig->ShowDbgFunc;
	fsUserHandle = ntrConfig->fsUserHandle;
	arm11BinStart = ntrConfig->arm11BinStart;
	arm11BinSize = ntrConfig->arm11BinSize;
    
	loadParams();
}

void initParamsFromInject() {
	ntrConfig = &(g_nsConfig->ntrConfig);
    
	loadParams();
}

void initFromSoc() {
	remotePlayMain();
}

void bootNtrStartMode(){
    // load from bootNTR
    if (_BootArgs[1] == 0xb00d)
        initParamsFromBootNtr();
    else
        initParamsFromLegacyBootNtr();
    
    Log("Starting %s...", NTR_CFW_VERSION);
    
    if ((ntrConfig->HomeMenuVersion == 0) || (ntrConfig->firmVersion == 0)) {
        Log("firmware or homemenu version not supported");
        Log("kversion: %08x", *(unsigned*)0x1FF80000);
        if ((_BootArgs[1] != 0xb00d)) {
            while (1) svc_sleepThread(1000000000);
        }

    } else {
        Log("do kernelhax");
        kDoKernelHax();
        clearDisp(GREEN);
        Log("homemenu ver: %08x", ntrConfig->HomeMenuVersion);
        injectToHomeMenu();
    }
}

void injectStartMode(){
    // init from inject
    initConfigureMemory();
    initParamsFromInject();
    nsInitDebug();

    u32 oldPC = g_nsConfig->startupInfo[2];

    memcpy((void*)oldPC, g_nsConfig->startupInfo, 8);
    rtFlushInstructionCache((void*)oldPC, 8);
    rtGenerateJumpCode(oldPC, (void*)_ReturnToUser);
    rtFlushInstructionCache((void*)_ReturnToUser, 8);
    doSomething();

    u32 currentPid = getCurrentProcessId();
    Handle handle;

    if (currentPid == 0x1a) {
        clearDisp(MAGENTA);
        remotePlayMain();
    }

    if (currentPid == ntrConfig->HomeMenuPid) {
        // load from HomeMenu
        threadStack = (u32*)((u32)NS_CONFIGURE_ADDR + 0x1000);
        svc_createThread(&handle, (void*)threadStart, 0, &threadStack[(THREAD_STACK_SIZE / 4) - 10], 0x10, 1);
    } else {

        threadStack = (u32*)((u32)NS_CONFIGURE_ADDR + 0x1000);
        if (g_nsConfig->startupCommand == NS_STARTCMD_DEBUG) {
            svc_createThread(&handle, (void*)startupFromInject, 0, &threadStack[(THREAD_STACK_SIZE / 4) - 10], 0x3f, 0xFFFFFFFE);
            svc_sleepThread(1000000000);
        }
        if (currentPid == ntrConfig->PMPid) {
            // load from pm
            initFromInjectPM();
        }
        else {
            if (g_nsConfig->startupCommand == NS_STARTCMD_INJECTGAME) {
                clearDisp(GREEN);
                initFromInjectGame();
            }
        }
    }
}

void ntrMain() {
	switch(_BootArgs[0]){   //StartMode
        case 0: bootNtrStartMode(); break;
        case 1: injectStartMode(); break;
    }
}