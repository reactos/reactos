#ifndef _BOOTVID_PCH_
#define _BOOTVID_PCH_

#include <ntddk.h>
#include <drivers/bootvid/bootvid.h>

/* Define if FontData has upside down characters */
#undef CHAR_GEN_UPSIDE_DOWN

#define BOOTCHAR_HEIGHT             13

/* Command Stream Definitions */
#define CMD_STREAM_WRITE            0x0
#define CMD_STREAM_WRITE_ARRAY      0x2
#define CMD_STREAM_USHORT           0x4
#define CMD_STREAM_READ             0x8

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

VOID
NTAPI
InitializePalette(VOID);

/* Globals */
extern USHORT AT_Initialization[];
extern ULONG curr_x;
extern ULONG curr_y;
extern ULONG_PTR VgaRegisterBase;
extern ULONG_PTR VgaBase;
extern UCHAR FontData[256 * BOOTCHAR_HEIGHT];

#endif /* _BOOTVID_PCH_ */
