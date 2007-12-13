#ifndef video_h
#define video_h

#include "stdlib.h"

// video helpers
typedef struct {
	BYTE * pData;
	BYTE * pBackdrop;
	int width;
	int height;
	int bpp;
} JPEG;

int BootVideoOverlayString(DWORD * pdwaTopLeftDestination, DWORD m_dwCountBytesPerLineDestination, RGBA rgbaOpaqueness, const char * szString);
void BootVideoChunkedPrint(const char * szBuffer);
int VideoDumpAddressAndData(DWORD dwAds, const BYTE * baData, DWORD dwCountBytesUsable);
unsigned int BootVideoGetStringTotalWidth(const char * szc);
void BootVideoClearScreen(JPEG * pJpeg, int nStartLine, int nEndLine);

void BootVideoJpegBlitBlend(
	BYTE *pDst,
	DWORD dst_width,
	JPEG * pJpeg,
	BYTE *pFront,
	RGBA m_rgbaTransparent,
	BYTE *pBack,
	int x,
	int y
);

bool BootVideoJpegUnpackAsRgb(
	BYTE *pbaJpegFileImage,
	JPEG * pJpeg
);

void BootVideoEnableOutput(BYTE bAvPack);
BYTE * BootVideoGetPointerToEffectiveJpegTopLeft(JPEG * pJpeg);

extern BYTE baBackdrop[60*72*4];
extern JPEG jpegBackdrop;

#endif /* #ifndef video_h */
