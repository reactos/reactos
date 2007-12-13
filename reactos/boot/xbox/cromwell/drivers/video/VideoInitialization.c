/**
 * Common library functions for video initialization
 *
 * Oliver Schwartz, May 2003
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifdef JUSTVIDEO
#include <stdio.h>
#include <math.h>
#endif
#include <boot.h>

#include "VideoInitialization.h"

extern BYTE VIDEO_AV_MODE;
// functions defined elsewhere
int I2CTransmitByteGetReturn(BYTE bPicAddressI2cFormat, BYTE bDataToWrite);
int I2CTransmitWord(BYTE bPicAddressI2cFormat, WORD wDataToWrite);

// internally used structures

typedef struct {
	long h_blanki;
	long h_blanko;
	long v_blanki;
	long v_blanko;
	long vscale;
} BLANKING_PARAMETER;

#ifndef JUSTVIDEO
static double fabs(double d) {
	if (d > 0) return d;
	else return -d;
}
#endif

static BYTE NvGetCrtc(BYTE * pbRegs, int nIndex) {
	pbRegs[0x6013d4]=nIndex;
	return pbRegs[0x6013d5];
}

static void NvSetCrtc(BYTE * pbRegs, int nIndex, BYTE b) {
	pbRegs[0x6013d4]=nIndex;
	pbRegs[0x6013d5]=b;
}

xbox_tv_encoding DetectVideoStd(void) {
	xbox_tv_encoding videoStd;
	BYTE b=I2CTransmitByteGetReturn(0x54, 0x5A); // the eeprom defines the TV standard for the box

	if(b == 0x40) {
		videoStd = TV_ENC_NTSC;
	} else {
		videoStd = TV_ENC_PALBDGHI;
	}

	return videoStd;
}

xbox_av_type DetectAvType(void) {
	xbox_av_type avType;

	switch (VIDEO_AV_MODE) {
		case 0: avType = AV_SCART_RGB; break;
		case 1: avType = AV_HDTV; break;
		case 2: avType = AV_VGA_SOG; break;
		case 4: avType = AV_SVIDEO; break;
		case 6: avType = AV_COMPOSITE; break;
         	case 7: avType = AV_VGA; break;
		default: avType = AV_COMPOSITE; break;
	}
	return avType;
}

void SetGPURegister(const GPU_PARAMETER* gpu, BYTE *pbRegs) {
	BYTE b;
	DWORD m = 0;
	/* All input from GPU is now RGB */
	*((DWORD *)&pbRegs[0x680630]) = 0; 
	/* Needed to prevent purple borders in RGB mode */
	*((DWORD *)&pbRegs[0x6808c4]) = 0;
	*((DWORD *)&pbRegs[0x68084c]) =0;

	/* YUV values
	*((DWORD *)&pbRegs[0x680630]) = 2; 
	*((DWORD *)&pbRegs[0x6808c4]) = 0x40801080;
	*((DWORD *)&pbRegs[0x68084c]) =0x00801080;
	*/
	
	// NVHDISPEND
	*((DWORD *)&pbRegs[0x680820]) = gpu->crtchdispend - 1;
	// NVHTOTAL
	*((DWORD *)&pbRegs[0x680824]) = gpu->nvhtotal;
	// NVHCRTC
	*((DWORD *)&pbRegs[0x680828]) = gpu->xres - 1;
	// NVHVALIDSTART
	*((DWORD *)&pbRegs[0x680834]) = 0;
	// NVHSYNCSTART
	*((DWORD *)&pbRegs[0x68082c]) = gpu->nvhstart;
	// NVHSYNCEND = NVHSYNCSTART + 32
	*((DWORD *)&pbRegs[0x680830]) = gpu->nvhstart+32;
	// NVHVALIDEND
	*((DWORD *)&pbRegs[0x680838]) = gpu->xres - 1;
	// CRTC_HSYNCSTART = h_total - 32 (heuristic)
	m = gpu->nvhtotal - 32;
	NvSetCrtc(pbRegs, 4, m/8);
	// CRTC_HSYNCEND = CRTC_HSYNCSTART + 16
	NvSetCrtc(pbRegs, 5, (NvGetCrtc(pbRegs, 5)&0xe0) | ((((m + 16)/8)-1)&0x1f) );
	// CRTC_HTOTAL = nvh_total (heuristic)
	NvSetCrtc(pbRegs, 0, (gpu->nvhtotal/8)-5);
	// CRTC_HBLANKSTART = crtchdispend
	NvSetCrtc(pbRegs, 2, ((gpu->crtchdispend)/8)-1);
	// CRTC_HBLANKEND = CRTC_HTOTAL = nvh_total
	NvSetCrtc(pbRegs, 3, (NvGetCrtc(pbRegs, 3)&0xe0) |(((gpu->nvhtotal/8)-1)&0x1f));
	NvSetCrtc(pbRegs, 5, (NvGetCrtc(pbRegs, 5)&(~0x80)) | ((((gpu->nvhtotal/8)-1)&0x20)<<2) );
	// CRTC_HDISPEND
	NvSetCrtc(pbRegs, 0x17, (NvGetCrtc(pbRegs, 0x17)&0x7f));
	NvSetCrtc(pbRegs, 1, ((gpu->crtchdispend)/8)-1);
	NvSetCrtc(pbRegs, 2, ((gpu->crtchdispend)/8)-1);
	NvSetCrtc(pbRegs, 0x17, (NvGetCrtc(pbRegs, 0x17)&0x7f)|0x80);
	// CRTC_LINESTRIDE = (xres / 8) * pixelDepth
	m=(gpu->xres / 8) * gpu->pixelDepth;
	NvSetCrtc(pbRegs, 0x19, (NvGetCrtc(pbRegs, 0x19)&0x1f) | ((m >> 3) & 0xe0));
	NvSetCrtc(pbRegs, 0x13, (m & 0xff));
	// NVVDISPEND
	*((DWORD *)&pbRegs[0x680800]) = gpu->yres - 1;
	// NVVTOTAL
	*((DWORD *)&pbRegs[0x680804]) = gpu->nvvtotal;
	// NVVCRTC
	*((DWORD *)&pbRegs[0x680808]) = gpu->yres - 1;
	// NVVVALIDSTART
	*((DWORD *)&pbRegs[0x680814]) = 0;
	// NVVSYNCSTART
	*((DWORD *)&pbRegs[0x68080c])=gpu->nvvstart;
	// NVVSYNCEND = NVVSYNCSTART + 3
	*((DWORD *)&pbRegs[0x680810])=(gpu->nvvstart+3);
	// NVVVALIDEND
	*((DWORD *)&pbRegs[0x680818]) = gpu->yres - 1;
	// CRTC_VSYNCSTART
	b = NvGetCrtc(pbRegs, 7) & 0x7b;
	NvSetCrtc(pbRegs, 7, b | ((gpu->crtcvstart >> 2) & 0x80) | ((gpu->crtcvstart >> 6) & 0x04));
	NvSetCrtc(pbRegs, 0x10, (gpu->crtcvstart & 0xff));
	// CRTC_VTOTAL
	b = NvGetCrtc(pbRegs, 7) & 0xde;
	NvSetCrtc(pbRegs, 7, b | ((gpu->crtcvtotal >> 4) & 0x20) | ((gpu->crtcvtotal >> 8) & 0x01));
	NvSetCrtc(pbRegs, 6, (gpu->crtcvtotal & 0xff));
	// CRTC_VBLANKEND = CRTC_VTOTAL
	b = NvGetCrtc(pbRegs, 0x16) & 0x80;
	NvSetCrtc(pbRegs, 0x16, b |(gpu->crtcvtotal & 0x7f));
	// CRTC_VDISPEND = yres
	b = NvGetCrtc(pbRegs, 7) & 0xbd;
	NvSetCrtc(pbRegs, 7, b | (((gpu->yres - 1) >> 3) & 0x40) | (((gpu->yres - 1) >> 7) & 0x02));
	NvSetCrtc(pbRegs, 0x12, ((gpu->yres - 1) & 0xff));
	// CRTC_VBLANKSTART
	b = NvGetCrtc(pbRegs, 9) & 0xdf;
	NvSetCrtc(pbRegs, 9, b | (((gpu->yres - 1)>> 4) & 0x20));
	b = NvGetCrtc(pbRegs, 7) & 0xf7;
	NvSetCrtc(pbRegs, 7, b | (((gpu->yres - 1) >> 5) & 0x08));
	NvSetCrtc(pbRegs, 0x15, ((gpu->yres - 1) & 0xff));
	// CRTC_LINECOMP
	m = 0x3ff; // 0x3ff = disable
	b = NvGetCrtc(pbRegs, 7) & 0xef;
	NvSetCrtc(pbRegs, 7, b | ((m>> 4) & 0x10));
	b = NvGetCrtc(pbRegs, 9) & 0xbf;
	NvSetCrtc(pbRegs, 9, b | ((m >> 3) & 0x40));
	NvSetCrtc(pbRegs, 0x18, (m & 0xff));
	// CRTC_REPAINT1
	if (gpu->xres < 1280) {
		b = 0x04;
	}
	else {
		b = 0x00;
	}
	NvSetCrtc(pbRegs, 0x1a, b);
	// Overflow bits
	/*
	b = ((hTotal   & 0x040) >> 2)
		| ((vDisplay & 0x400) >> 7)
		| ((vStart   & 0x400) >> 8)
		| ((vDisplay & 0x400) >> 9)
		| ((vTotal   & 0x400) >> 10);
	*/
	b = (((gpu->nvhtotal / 8 - 5) & 0x040) >> 2)
		| (((gpu->yres - 1) & 0x400) >> 7)
		| ((gpu->crtcvstart & 0x400) >> 8)
		| (((gpu->yres - 1) & 0x400) >> 9)
		| ((gpu->crtcvtotal & 0x400) >> 10);
	NvSetCrtc(pbRegs, 0x25, b);

	b = gpu->pixelDepth;
	if (b >= 3) b = 3;
	/* switch pixel mode to TV */
	b  |= 0x80;
	NvSetCrtc(pbRegs, 0x28, b);

	b = NvGetCrtc(pbRegs, 0x2d) & 0xe0;
	if ((gpu->nvhtotal / 8 - 1) >= 260) {
		b |= 0x01;
	}
	NvSetCrtc(pbRegs, 0x2d, b);
}

