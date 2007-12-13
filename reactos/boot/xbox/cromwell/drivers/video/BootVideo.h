#ifndef _BootVideo_H_
#define _BootVideo_H_

enum {
	VIDEO_MODE_UNKNOWN=-1,
	VIDEO_MODE_640x480=0,
	VIDEO_MODE_640x576,
	VIDEO_MODE_720x576,
	VIDEO_MODE_800x600,
	VIDEO_MODE_1024x576,
	VIDEO_MODE_COUNT
};

typedef struct {
		// fill on entry
	int m_nVideoModeIndex; // fill on entry to BootVgaInitializationKernel(), eg, VIDEO_MODE_800x600
	BYTE m_fForceEncoderLumaAndChromaToZeroInitially; // fill on entry to BootVgaInitializationKernel(), 0=mode change visible immediately, !0=initially forced to black raster
	DWORD m_dwFrameBufferStart; // frame buffer start address, set to zero to use default
	BYTE * volatile m_pbBaseAddressVideo; // base address of video, usually 0xfd000000
		// filled on exit
	DWORD width; // everything else filled by BootVgaInitializationKernel() on return
	DWORD height;
	DWORD xmargin;
	DWORD ymargin;
	BYTE m_bAvPack;
	DWORD m_dwVideoFadeupTimer;
	double hoc;
	double voc;
	BYTE m_bBPP;
} CURRENT_VIDEO_MODE_DETAILS;

void BootVgaInitializationKernelNG(CURRENT_VIDEO_MODE_DETAILS * pvmode);

#endif // _BootVideo_H_
