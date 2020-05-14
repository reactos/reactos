#ifndef _BOOTVID_PCH_
#define _BOOTVID_PCH_

#include <ntifs.h>
#include <ndk/halfuncs.h>
#include <drivers/bootvid/bootvid.h>

/* Arch specific includes */
#if defined(_M_IX86) || defined(_M_AMD64)
#if defined(SARCH_PC98)
#include "i386/pc98/pc98.h"
#else
#include "i386/pc/vga.h"
#include "i386/pc/pc.h"
#endif
#elif defined(_M_ARM)
#include "arm/arm.h"
#else
#error Unknown architecture
#endif

/* Define if FontData has upside down characters */
#undef CHAR_GEN_UPSIDE_DOWN

#define BOOTCHAR_HEIGHT 13
#define BOOTCHAR_WIDTH  8 // Each character line is encoded in a UCHAR.

/* Bitmap Header */
typedef struct tagBITMAPINFOHEADER
{
    ULONG biSize;
    LONG biWidth;
    LONG biHeight;
    USHORT biPlanes;
    USHORT biBitCount;
    ULONG biCompression;
    ULONG biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    ULONG biClrUsed;
    ULONG biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

/* Supported bitmap compression formats */
#define BI_RGB  0
#define BI_RLE4 2

typedef ULONG RGBQUAD;

/*
 * Globals
 */
extern UCHAR VidpTextColor;
extern ULONG VidpCurrentX;
extern ULONG VidpCurrentY;
extern ULONG VidpScrollRegion[4];
extern UCHAR VidpFontData[256 * BOOTCHAR_HEIGHT];
extern const RGBQUAD VidpDefaultPalette[BV_MAX_COLORS];

#define RGB(r, g, b)    ((RGBQUAD)(((UCHAR)(b) | ((USHORT)((UCHAR)(g))<<8)) | (((ULONG)(UCHAR)(r))<<16)))

#define GetRValue(quad)    ((UCHAR)(((quad)>>16) & 0xFF))
#define GetGValue(quad)    ((UCHAR)(((quad)>>8) & 0xFF))
#define GetBValue(quad)    ((UCHAR)((quad) & 0xFF))

#define InitializePalette()    InitPaletteWithTable((PULONG)VidpDefaultPalette, BV_MAX_COLORS)

#ifdef CHAR_GEN_UPSIDE_DOWN
# define GetFontPtr(_Char) &VidpFontData[_Char * BOOTCHAR_HEIGHT] + BOOTCHAR_HEIGHT - 1;
# define FONT_PTR_DELTA (-1)
#else
# define GetFontPtr(_Char) &VidpFontData[_Char * BOOTCHAR_HEIGHT];
# define FONT_PTR_DELTA (1)
#endif

#endif /* _BOOTVID_PCH_ */
