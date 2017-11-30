#include "global.h"

#define O_RDONLY 1
#define O_WRONLY 2
#define O_RDWR   3
#define O_CREAT  4

u8 *image_buf = NULL;

struct BitmapHeader{
	char id[2];
	u32  filesize;
	u32  reserved;
	u32  offset;
	u32  headsize;
	u32  width;
	u32  height;
	u16  planes;
	u16  bpp;
	u32  comp;
	u32  bitmapsize;
	u32  hres;
	u32  vres;
	u32  colors;
	u32  impcolors;
} __attribute__((packed));

void put_le32(void *dst, u32 val){
	u8 *dp = (u8*)dst;
	dp[0] = val & 0xff;
	dp[1] = (val >> 8) & 0xff;
	dp[2] = (val >> 16) & 0xff;
	dp[3] = (val >> 24) & 0xff;
}

void put_le16(void *dst, u32 val){
	u8 *dp = (u8*)dst;
	dp[0] = val & 0xff;
	dp[1] = (val >> 8) & 0xff;
}

void bmp_write(u8 *bmp_buf, int width, int height, char *name){
	struct BitmapHeader *bmp;
	int fd, fsize, hsize;

	bmp = (struct BitmapHeader*)bmp_buf;
	hsize = sizeof(struct BitmapHeader);
	fsize = width*height * 3 + sizeof(struct BitmapHeader);

	memset(bmp, 0, hsize);

	bmp->id[0] = 'B';
	bmp->id[1] = 'M';
	put_le32(&bmp->filesize, fsize);
	put_le32(&bmp->offset, hsize);
	put_le32(&bmp->headsize, 0x28);
	put_le32(&bmp->width, width);
	put_le32(&bmp->height, height);
	put_le16(&bmp->planes, 1);
	put_le16(&bmp->bpp, 24);
	put_le32(&bmp->bitmapsize, width*height * 3);
	put_le32(&bmp->hres, 2834);
	put_le32(&bmp->vres, 2834);

	fd = sdf_open(name, O_RDWR | O_CREAT);
	if (fd > 0){
		sdf_write(fd, 0, bmp_buf, fsize);
		sdf_close(fd);
	}
}

void rev_image(u8 *dst, int width, int height, u8 *src, int src_pitch, int format){
	int x, y, bpp;
	u8 *dp, *sp;
	u32 px;

	if ((format & 0x0f) == 0){
		bpp = 4;
	}
	else if ((format & 0x0f) == 1){
		bpp = 3;
	}
	else{
		bpp = 2;
	}
	format &= 0x0f;

	for (y = 0; y < height; y++){
		dp = dst + y*width * 3;
		sp = src + y*bpp;
		for (x = 0; x < width; x++){
			if (format == 0){
				dp[0] = sp[0];
				dp[1] = sp[1];
				dp[2] = sp[2];
			}
			else if (format == 1){
				dp[0] = sp[0];
				dp[1] = sp[1];
				dp[2] = sp[2];
			}
			else if (format == 2){
				px = (sp[1] << 8) | sp[0];
				dp[0] = (px << 3) & 0xf8;
				dp[1] = (px >> 3) & 0xfc;
				dp[2] = (px >> 8) & 0xf8;
			}
			else if (format == 3){
				px = (sp[1] << 8) | sp[0];
				dp[0] = (px << 2) & 0xf8;
				dp[1] = (px >> 3) & 0xf8;
				dp[2] = (px >> 8) & 0xf8;
			}
			else if (format == 4){
				px = (sp[1] << 8) | sp[0];
				dp[0] = (px << 0) & 0xf0;
				dp[1] = (px >> 4) & 0xf0;
				dp[2] = (px >> 8) & 0xf0;
			}
			dp += 3;
			sp += src_pitch;
		}
	}
}

int get_file_index(void){
	int fd;
	char name[64];
	int id = 0;
    
	while (1){
		xsprintf(name, "/top_%04d.bmp", id);
		fd = sdf_open(name, O_RDWR);
		if (fd <= 0) break;
		sdf_close(fd);
		id += 1;
	}

	return id;
}

void allocImageBuf() {
	if (image_buf) return;
	u32 out_addr = plgRequestMemory(0x00100000);
	ntrDebugLog("    out_addr: %08x", out_addr);
	image_buf = (u8*)out_addr;
}

void do_screen_shoot(){
	u32 tl_fbaddr[2];
	u32 bl_fbaddr[2];
	u32 current_fb;
	u32 tl_format, bl_format;
	u32 tl_pitch, bl_pitch;
	u8 *rev_buf;
	u32 fbP2VOffset = 0xc0000000;
	u32 out_addr;
    u32 bmp_index = get_file_index();

	u32 workingBuffer;

	char name[64];

	if (ntrConfig->memMode == NTR_MEMMODE_DEFAULT) {
		if (!image_buf) allocImageBuf();
		if (image_buf == NULL) return;
		workingBuffer = image_buf;
	} else {
		workingBuffer = plgRequestTempBuffer(0xA0000);
	}

	if (workingBuffer == 0) return;

	tl_fbaddr[0] = REG(IoBasePdc + 0x468) + fbP2VOffset;
	tl_fbaddr[1] = REG(IoBasePdc + 0x46c) + fbP2VOffset;
	bl_fbaddr[0] = REG(IoBasePdc + 0x568) + fbP2VOffset;
	bl_fbaddr[1] = REG(IoBasePdc + 0x56c) + fbP2VOffset;
	tl_format = REG(IoBasePdc + 0x470);
	bl_format = REG(IoBasePdc + 0x570);
	tl_pitch = REG(IoBasePdc + 0x490);
	bl_pitch = REG(IoBasePdc + 0x590);

	// TOP screen
	current_fb = REG(IoBasePdc + 0x478);
	current_fb &= 1;
	kmemcpy((void*)(workingBuffer + 0x50000), (void*)tl_fbaddr[current_fb], 240 * 400 * 3);

	rev_buf = workingBuffer + sizeof(struct BitmapHeader);
	rev_image(rev_buf, 400, 240, workingBuffer + 0x50000, tl_pitch, tl_format);
	xsprintf(name, "/top_%04d.bmp", bmp_index);
	bmp_write(workingBuffer, 400, 240, name);


	// Bottom screen
	current_fb = REG(IoBasePdc + 0x578);
	current_fb &= 1;
	kmemcpy((void*)(workingBuffer + 0x50000), (void*)bl_fbaddr[current_fb], 240 * 320 * 3);

	rev_buf = workingBuffer + sizeof(struct BitmapHeader);
	rev_image(rev_buf, 320, 240, workingBuffer + 0x50000, bl_pitch, bl_format);
	xsprintf(name, "/bot_%04d.bmp", bmp_index);
	bmp_write(workingBuffer, 320, 240, name);
    
	clearDisp(MAGENTA);
}

u32 takeScreenShot(){
	vu32 i;
	u32 ret, out_addr;

	controlVideo(CONTROLVIDEO_RELEASEVIDEO, 0, 0, 0);
	svc_sleepThread(100000000);
	do_screen_shoot();
	controlVideo(CONTROLVIDEO_ACQUIREVIDEO, 0, 0, 0);
	return 1;
}

/*
*   Menu functions
*/
extern PLGLOADER_INFO *g_plgInfo;

u32 cpuClockUi(){
	u8 buf[200];
	acquireVideo();
	u8* entries[8];
	u32 res;
	entries[0] = plgTranslate("268MHz, L2 Disabled (Default)");
	entries[1] = plgTranslate("804MHz, L2 Disabled");
	entries[2] = plgTranslate("268MHz, L2 Enabled");
	entries[3] = plgTranslate("804MHz, L2 Enabled (Best)");
	while (1) {
		clear(0, 0, 320, 240, WHITE);
		res = showMenu(plgTranslate("CPU Clock"), 4, entries);
		if (res == -1) break;
		if ((res >= 0) && (res <= 3)) {
			u32 ret = svc_kernelSetState(10, res, 0, 0);
			if (ret != 0) Log("kernelSetState failed: %08x", ret);
			setCpuClockLock(res);
			break;
		}
	}
	releaseVideo();
	return 0;
}

u32 powerMenu() {
	acquireVideo();
	u8* entries[8];
	u32 r;
	vu32 i;
	entries[0] = plgTranslate("Reboot");
	entries[1] = plgTranslate("Power Off");
	while (1) {
		r = showMenu(plgTranslate("Power"), 2, entries);
		if (r == -1) break;
		if (r == 0) {
			plgDoReboot();
			break;
		}
		if (r == 1) {
			plgDoPowerOff();
			break;
		}
	}
	releaseVideo();
	return 1;
}

int GSPPid = 0x14;
int isGSPPatchRequired = 0;
u32 gspPatchAddr = 0;
int bklightValue = 50;

int checkBacklightSupported() {

	Handle hGSPProcess = 0;
	u32 ret;
	u8 buf[16] = { 0 };
	static u8 desiredHeader[16] = { 0x30, 0x40, 0x2D, 0xE9, 0x00, 0x40, 0xA0, 0xE1, 0x01, 0x5C, 0x80, 0xE2, 0x8C, 0x01, 0xD0, 0xE5 };

	if (!(ntrConfig->isNew3DS)) {
		return 1;
	}

	ret = svc_openProcess(&hGSPProcess, GSPPid);
	if (ret != 0) {
		return 0;
	}
	gspPatchAddr = 0x0010740C;
	copyRemoteMemory(getCurrentProcessHandle(), buf, hGSPProcess, gspPatchAddr, 16);

	if (memcmp(buf, desiredHeader, 16) == 0) {
		goto requirePatch;
	}

	// 11.3.0
	gspPatchAddr = 0x0010743C;
	copyRemoteMemory(getCurrentProcessHandle(), buf, hGSPProcess, gspPatchAddr, 16);

	if (memcmp(buf, desiredHeader, 16) == 0) {
		goto requirePatch;
	}

	svc_closeHandle(hGSPProcess);
	return 0;

requirePatch:
	svc_closeHandle(hGSPProcess);
	isGSPPatchRequired = 1;
	return 1;
}


void reliableWriteU8(vu8* addr, u8 data) {
	while (1) {
		*addr = data;
		mdelay(1);
		if ((*addr) == data) {
			return;
		}
	}
}

int patchGSP() {
	if (!isGSPPatchRequired) {
		return 0;
	}
	u32 ret;
	Handle hGSPProcess = 0;
	u32 buf[4] = { 0, 0 };
	ret = writeRemoteProcessMemory(GSPPid, gspPatchAddr + 0x68, 4, buf);
	ret = writeRemoteProcessMemory(GSPPid, gspPatchAddr + 0xC8, 4, buf);
	
	if (ret == 0) {
		isGSPPatchRequired = 0;
	}
	return ret;
}


void updateBklight() {
	u32 t;
	t = bklightValue * 2;
	if (bklightValue == 1) {
		t = 1;
	}
	if (bklightValue == 100) {
		t = 0xff;
	}

	// http://3dbrew.org/wiki/LCD_Registers

	*(vu32*)(IoBaseLcd + 0x240) = t;
	*(vu32*)(IoBaseLcd + 0xa40) = t;
}

void adjustBklight(int adj) {
	bklightValue += adj;
	if (bklightValue < 1) {
		bklightValue = 1;
	}
	if (bklightValue > 100) {
		bklightValue = 100;
	}
	updateBklight();
}

u32 backlightMenu() {  
	if (isGSPPatchRequired) {
		patchGSP();
	}
	u8 buf[50];
	while (1) {
		clear(0, 0, 320, 240, WHITE);
		xsprintf(buf, "backlight: %d", bklightValue);
		print(buf, 10, 10, RED);
		print(plgTranslate("Press Up/Down/Left/Right to adjust."), 10, 220, BLUE);
		updateScreen();
		u32 key = waitKey();
		if (key == BUTTON_DU) {
			adjustBklight(1);
		}
		if (key == BUTTON_DD) {
			adjustBklight(-1);
		}
		if (key == BUTTON_DL) {
			adjustBklight(-10);
		}
		if (key == BUTTON_DR) {
			adjustBklight(10);
		}
		if (key == BUTTON_B) {
			break;
		}
	}
	return 1;
}

u32 nightShiftUi() {
	int configUpdated = 0;
	u8* entries[11];
	u32 r;
	entries[0] = plgTranslate("Disabled");
	entries[1] = plgTranslate("Reduce Blue Light Level 1");
	entries[2] = plgTranslate("Reduce Blue Light Level 2");
	entries[3] = plgTranslate("Reduce Blue Light Level 3");
	entries[4] = plgTranslate("Reduce Blue Light Level 4");
	entries[5] = plgTranslate("Reduce Blue Light Level 5");
	entries[6] = plgTranslate("Invert Colors");
	entries[7] = plgTranslate("Grayscale");
	entries[8] = plgTranslate("Hint: Must be enabled before starting game. Will set CPU to L2+804MHz on New3DS.");
    
    acquireVideo();
	while (1) {
		clear(0, 0, 320, 240, WHITE);
		r = showMenu(plgTranslate("Screen Filter"), 8, entries);
		if (r == -1) break;
		if ((r >= 0) && (r <= 7)) {
			g_plgInfo->nightShiftLevel = r;
			if (r > 0) {
				if (ntrConfig->isNew3DS) {
					setCpuClockLock(3);
				}
			}
			configUpdated = 1;
			break;
		}
	}
	releaseVideo();
    
	if (configUpdated) {
		plgTryUpdateConfig();
	}
	return 1;
}

u32 themesMenu(){
    u32 r;
    u8* entries[3];
	entries[0] = plgTranslate("Greyscale");
	entries[1] = plgTranslate("Mist");
	entries[2] = plgTranslate("Classic");
	entries[3] = plgTranslate("Default");
    
    acquireVideo();
    while (1) {
		clear(0, 0, 320, 240, WHITE);
		r = showMenu(plgTranslate("Themes"), 4, entries);
		if (r == -1) break;
		if (r == 0) {setTheme(0x999999, 0x111111, 0x444444); break;}
		if (r == 1) {setTheme(0x8080, 0xD0EAF0, 0x40E0D0); break;}
        if (r == 2) {setTheme(WHITE, BLACK, RED); break;}
        if (r == 3) {setTheme(DARKGREY, GREEN, 0xEE9629); break;}
	}
	releaseVideo();

	return 1;
}

u32 infoMenu(){   
    acquireVideo();
    Log(plgTranslate(
        "Kernel: %d.%d.%d\n"
        "Firm: %d.%d.%d\n"
        "Unit: %s\n"
        "Mac Addr: %02X:%02X:%02X:%02X:%02X:%02X\n"
    ),
    *(u8*)0x1FF80003, *(u8*)0x1FF80002, *(u8*)0x1FF80001,
    *(u8*)0x1FF80063, *(u8*)0x1FF80062, *(u8*)0x1FF80061,
    *(u8*)0x1FF80015 == 0 ? "Prod" : "Dev",
    *(u8*)0x1FF81060,*(u8*)0x1FF81061,*(u8*)0x1FF81062,*(u8*)0x1FF81063,*(u8*)0x1FF81064,*(u8*)0x1FF81065
    );
    releaseVideo();
    
    return 1;
}

void ntrToolsMain() {    
	ntrDebugLog("initializing screenshot plugin\n");
    
    if (ntrConfig->isNew3DS)
		plgRegisterMenuEntry(1, plgTranslate("CPU Clock"), cpuClockUi);
    
	plgRegisterMenuEntry(1, plgTranslate("Take Screenshot"), takeScreenShot);
	
	if (checkBacklightSupported())
		plgRegisterMenuEntry(1, plgTranslate("Backlight"), backlightMenu);

	plgRegisterMenuEntry(1, plgTranslate("Screen Filter"), nightShiftUi);
    plgRegisterMenuEntry(1, plgTranslate("Themes"), themesMenu);
    plgRegisterMenuEntry(1, plgTranslate("Info"), infoMenu);
    plgRegisterMenuEntry(1, plgTranslate("Power"), powerMenu);
}

/*
*   IPCs
*/
void plgDoReboot() {
	Handle hNSS = 0;
	u32 ret;
	ret = srv_getServiceHandle(NULL, &hNSS, "ns:s");
	if (ret != 0) {
		Log("open ns:s failed: %08x", ret);
		return;
	}
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = 0x00100180;
	cmdbuf[1] = 0x00000000;
	cmdbuf[2] = 0x00000000;
	cmdbuf[3] = 0x00000000;
	cmdbuf[4] = 0x00000000;
	cmdbuf[5] = 0x00000000;
	cmdbuf[6] = 0x00000000;
	svc_sendSyncRequest(hNSS);
	svc_closeHandle(hNSS);
}

void plgDoPowerOff() {
	Handle hNSS = 0;
	u32 ret;
	ret = srv_getServiceHandle(NULL, &hNSS, "ns:s");
	if (ret != 0) {
		Log("open ns:s failed: %08x", ret);
		return;
	}
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = 0x000E0000;
	cmdbuf[1] = 0x00000000;
	cmdbuf[2] = 0x00000000;
	cmdbuf[3] = 0x00000000;
	cmdbuf[4] = 0x00000000;
	cmdbuf[5] = 0x00000000;
	cmdbuf[6] = 0x00000000;
	svc_sendSyncRequest(hNSS);
	svc_closeHandle(hNSS);
}

/*
*   File handlers
*/
int sdf_open(char *filename, int mode){
	FS_archive sdmcArchive = { 0x9, (FS_path){ PATH_EMPTY, 1, (u8*)"" } };

	Handle hd;
	Result retv;
	FS_path filePath;

	filePath.type = PATH_CHAR;
	filePath.size = strlen(filename) + 1;
	filePath.data = (u8*)filename;

	retv = FSUSER_OpenFileDirectly(fsUserHandle, &hd, sdmcArchive, filePath, mode, 0);
	if (retv < 0) return retv;

	return hd;
}

int sdf_read(int fd, int offset, void *buf, int size){
	u32 read_size;
	Result retv;

	retv = FSFILE_Read(fd, &read_size, offset, (u32*)buf, size);
	if (retv < 0)
		return retv;

	return read_size;
}

int sdf_write(int fd, int offset, void *buf, int size){
	u32 write_size;
	Result retv;

	retv = FSFILE_Write(fd, &write_size, offset, (u32*)buf, size, 0);
	if (retv < 0)
		return retv;

	return write_size;
}

int sdf_close(int fd){
	return FSFILE_Close(fd);
}