#include "global.h"

Handle hCurrentProcess = 0;
u32 currentPid = 0;

u32 getCurrentProcessId() {
	svc_getProcessId(&currentPid, 0xffff8001);
	return currentPid;
}

u32 getCurrentProcessHandle() {
	u32 handle = 0;
	u32 ret;

	if (hCurrentProcess != 0) {
		return hCurrentProcess;
	}
	svc_getProcessId(&currentPid, 0xffff8001);
	ret = svc_openProcess(&handle, currentPid);
	if (ret != 0) {
		Log("openProcess failed, ret: %08x", ret);
		return 0;
	}
	hCurrentProcess = handle;
	return hCurrentProcess;
}

u32 getCurrentProcessKProcess() {
	return kGetCurrentKProcess();
}

u32 mapRemoteMemory(Handle hProcess, u32 addr, u32 size) {
	u32 outAddr = 0;
	u32 ret;
	u32 newKP = kGetKProcessByHandle(hProcess);
	u32 oldKP = kGetCurrentKProcess();

	kSetCurrentKProcess(newKP);
	ret = svc_controlMemory(&outAddr, addr, addr, size, NS_DEFAULT_MEM_REGION + 3, 3);
	kSetCurrentKProcess(oldKP);
    
	if (ret != 0) {
		Log("svc_controlMemory failed: %08x", ret);
		return ret;
	}
	if (outAddr != addr) {
		Log("outAddr: %08x, addr: %08x", outAddr, addr);
		return 0;
	}
    
	return 0;
}

u32 mapRemoteMemoryInSysRegion(Handle hProcess, u32 addr, u32 size) {
	u32 outAddr = 0;
	u32 ret;
	u32 newKP = kGetKProcessByHandle(hProcess);
	u32 oldKP = kGetCurrentKProcess();

	u32 oldPid = kSwapProcessPid(newKP, 1);

	kSetCurrentKProcess(newKP);
	ret = svc_controlMemory(&outAddr, addr, addr, size, NS_DEFAULT_MEM_REGION + 3, 3);
	kSetCurrentKProcess(oldKP);
	kSwapProcessPid(newKP, oldPid);
	if (ret != 0) {
		Log("svc_controlMemory failed: %08x", ret);
		return ret;
	}
	if (outAddr != addr) {
		Log("outAddr: %08x, addr: %08x", outAddr, addr);
		return 0;
	}
    
	return 0;
}


u32 controlMemoryInSysRegion(u32* outAddr, u32 addr0, u32 addr1, u32 size, u32 op, u32 perm) {
	u32 currentKP = kGetCurrentKProcess();
	u32 oldPid = kSwapProcessPid(currentKP, 1);
	u32 ret = svc_controlMemory(outAddr, addr0, addr1, size, op, perm);
	kSwapProcessPid(currentKP, oldPid);
	return ret;
}

u32 protectRemoteMemory(Handle hProcess, void* addr, u32 size) {
	u32 outAddr = 0;

	return svc_controlProcessMemory(hProcess, addr, addr, size, 6, 7);
}

u32 protectMemory(void* addr, u32 size) {
	return protectRemoteMemory(getCurrentProcessHandle(), addr, size);
}

u32 copyRemoteMemory(Handle hDst, void* ptrDst, Handle hSrc, void* ptrSrc, u32 size) {
	u8 dmaConfig[80] = {0, 0, 4};
	u32 hdma = 0;
	u32 state, i, ret;

	ret = svc_flushProcessDataCache(hSrc, (u32)ptrSrc, size);
	if (ret != 0) {
		ntrDebugLog("svc_flushProcessDataCache(hSrc) failed.\n");
		return ret;
	}
	ret = svc_flushProcessDataCache(hDst, (u32)ptrDst, size);
	if (ret != 0) {
		ntrDebugLog("svc_flushProcessDataCache(hDst) failed.\n");
		return ret;
	}

	ret = svc_startInterProcessDma(&hdma, hDst, ptrDst, hSrc, ptrSrc, size, dmaConfig);
	if (ret != 0) {
		return ret;
	}
	for (i = 0; i < 10000; i++ ) {
		state = 0;
		ret = svc_getDmaState(&state, hdma);
		if (ntrConfig->InterProcessDmaFinishState == 0) {
			if (state == 0xfff54204 || state == 0xfff04504 || state == 0xfff04204) break;
		}
		else {
			if (state == ntrConfig->InterProcessDmaFinishState) break;
		}

		svc_sleepThread(1000000);
	}

	if (i >= 10000) {
		Log("readRemoteMemory time out %08x", state);
		return 1;
	}

	svc_closeHandle(hdma);
	ret = svc_invalidateProcessDataCache(hDst, (u32)ptrDst, size);
	if (ret != 0) return ret;
	return 0;
}

u32 getProcessTIDByHandle(u32 hProcess, u32 tid[]) {
	u8 bufKProcess[0x100], bufKCodeSet[0x100];
	u32  pKCodeSet, pKProcess, ret;

	pKProcess = kGetKProcessByHandle(hProcess);
	kmemcpy((void*)bufKProcess, (void*)pKProcess, 0x100);
	pKCodeSet = *(u32*)(&bufKProcess[KProcessCodesetOffset]);
	kmemcpy((void*)bufKCodeSet, (void*)pKCodeSet, 0x100);
	u32* pTid = (u32*)(&bufKCodeSet[0x5c]);
	tid[0] = pTid[0];
	tid[1] = pTid[1];
}


u32 getProcessInfo(u32 pid, u8* pname, u32 tid[], u32* kpobj) {
	u8 bufKProcess[0x100], bufKCodeSet[0x100];
	u32 hProcess, pKCodeSet, pKProcess, ret;
	u8 buf[300];

	ret = 0;
	ret = svc_openProcess(&hProcess, pid);
	if (ret != 0) {
		ntrDebugLog("openProcess failed: %08x\n", ret);
		goto final;
	}

	pKProcess = kGetKProcessByHandle(hProcess);
	kmemcpy((void*)bufKProcess, (void*)pKProcess, 0x100);
	pKCodeSet = *(u32*)(&bufKProcess[KProcessCodesetOffset]);
	kmemcpy((void*)bufKCodeSet, (void*)pKCodeSet, 0x100);
	bufKCodeSet[0x5A] = 0;
	u8* pProcessName = &bufKCodeSet[0x50];
	u32* pTid = (u32*)(&bufKCodeSet[0x5c]);
	tid[0] = pTid[0];
	tid[1] = pTid[1];
	strcpy(pname, pProcessName);
	*kpobj = pKProcess;

	final:
	if (hProcess) svc_closeHandle(hProcess);
	return ret;
}

u32 writeRemoteProcessMemory(int pid, u32 addr, u32 size, u32* buf) {
	u32 hProcess = 0, ret;
	ret = svc_openProcess(&hProcess, pid);
	if (ret != 0) return ret;
    
	ret = rtCheckRemoteMemoryRegionSafeForWrite(hProcess, addr, size);
	if (ret != 0) goto final;
    
	ret = copyRemoteMemory(hProcess, (void*)addr, 0xffff8001, buf, size);
	if (ret != 0) goto final;
    
	final:
	if (hProcess) svc_closeHandle(hProcess);
    
	return ret;
}

void initSMPatch() {
	u32 hProcess = 0, ret;
	u32 buf[20];
	//write(0x101820,(0x01,0x00,0xA0,0xE3,0x1E,0xFF,0x2F,0xE1),pid=0x3)
	buf[0] = 0xe3a00001;
	buf[1] = 0xe12fff1e;
	ret = writeRemoteProcessMemory(3, 0x101820, 8, buf);
	if (ret != 0) Log("patch sm failed: %08x", ret);
}