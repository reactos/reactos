/*----------------------------------------------------------------------+
| decmprss.h - Microsoft Video 1 Compressor - decompress header file	|
|									|
| Copyright (c) 1990-1994 Microsoft Corporation.			|
| Portions Copyright Media Vision Inc.					|
| All Rights Reserved.							|
|									|
| You have a non-exclusive, worldwide, royalty-free, and perpetual	|
| license to use this source code in developing hardware, software	|
| (limited to drivers and other software required for hardware		|
| functionality), and firmware for video display and/or processing	|
| boards.   Microsoft makes no warranties, express or implied, with	|
| respect to the Video 1 codec, including without limitation warranties	|
| of merchantability or fitness for a particular purpose.  Microsoft	|
| shall not be liable for any damages whatsoever, including without	|
| limitation consequential damages arising from your use of the Video 1	|
| codec.								|
|									|
|									|
+----------------------------------------------------------------------*/

// width and height of a compression blocks
#define WIDTH_CBLOCK 4
#define HEIGHT_CBLOCK 4

#define EDGE_HEIGHT_CBLOCK 2
#define EDGE_WIDTH_CBLOCK 2
#define EDGE_SUBBLOCKS ((HEIGHT_CBLOCK * WIDTH_CBLOCK) / (EDGE_HEIGHT_CBLOCK * EDGE_WIDTH_CBLOCK))

#define NEXT_BLOCK( row, bpr, height ) (((HPBYTE)row) + (bpr*height))
#define NEXT_BYTE_ROW( row, bpr ) (((HPBYTE)row) + bpr)
#define NEXT_RGBT_PIXEL_ROW( row, bpr ) ((HPRGBTRIPLE)(((HPBYTE)row) + bpr))
#define NEXT_BLOCK_ROW( row, bpr, height ) ((HPRGBTRIPLE)NEXT_BLOCK( row, bpr, height ))

typedef DWORD (FAR PASCAL *DECOMPPROC)(LPBITMAPINFOHEADER lpbiIn,  LPVOID lpIn,
                       LPBITMAPINFOHEADER lpbiOut, LPVOID lpOut, LONG x, LONG y);

// in decmprss.c
DWORD FAR PASCAL DecompressFrame24(LPBITMAPINFOHEADER lpbiIn,  LPVOID lpIn,
                        LPBITMAPINFOHEADER lpbiOut, LPVOID lpOut, LONG x, LONG y);

// in decmprss.c
DWORD FAR PASCAL DecompressFrame8(LPBITMAPINFOHEADER lpbiIn,  LPVOID lpIn,
                       LPBITMAPINFOHEADER lpbiOut, LPVOID lpOut, LONG x, LONG y);

#ifndef _WIN32

// in decram8.asm
DWORD FAR PASCAL DecompressCram8(LPBITMAPINFOHEADER lpbiIn,  LPVOID lpIn,
                       LPBITMAPINFOHEADER lpbiOut, LPVOID lpOut, LONG x, LONG y);

// in decram8.asm
DWORD FAR PASCAL DecompressCram8x2(LPBITMAPINFOHEADER lpbiIn,  LPVOID lpIn,
                       LPBITMAPINFOHEADER lpbiOut, LPVOID lpOut, LONG x, LONG y);

// in decram16.asm
DWORD FAR PASCAL DecompressCram16(LPBITMAPINFOHEADER lpbiIn,  LPVOID lpIn,
                       LPBITMAPINFOHEADER lpbiOut, LPVOID lpOut, LONG x, LONG y);

DWORD FAR PASCAL DecompressCram16x2(LPBITMAPINFOHEADER lpbiIn,  LPVOID lpIn,
                       LPBITMAPINFOHEADER lpbiOut, LPVOID lpOut, LONG x, LONG y);

// in dcram168.asm
DWORD FAR PASCAL DecompressCram168(LPBITMAPINFOHEADER lpbiIn,  LPVOID lpIn,
                       LPBITMAPINFOHEADER lpbiOut, LPVOID lpOut, LONG x, LONG y);

// in decram32.asm
DWORD FAR PASCAL DecompressCram32(LPBITMAPINFOHEADER lpbiIn,  LPVOID lpIn,
                       LPBITMAPINFOHEADER lpbiOut, LPVOID lpOut, LONG x, LONG y);

DWORD FAR PASCAL DecompressCram32x2(LPBITMAPINFOHEADER lpbiIn,  LPVOID lpIn,
                       LPBITMAPINFOHEADER lpbiOut, LPVOID lpOut, LONG x, LONG y);

// in dcram286.asm
DWORD FAR PASCAL DecompressCram8_286(LPBITMAPINFOHEADER lpbiIn,  LPVOID lpIn,
                       LPBITMAPINFOHEADER lpbiOut, LPVOID lpOut, LONG x, LONG y);

// in dcram286.asm
DWORD FAR PASCAL DecompressCram16_286(LPBITMAPINFOHEADER lpbiIn,  LPVOID lpIn,
                       LPBITMAPINFOHEADER lpbiOut, LPVOID lpOut, LONG x, LONG y);
#else
// in decmprss.c
DWORD FAR PASCAL DecompressFrame8X2C(LPBITMAPINFOHEADER lpbiIn,  LPVOID lpIn,
                       LPBITMAPINFOHEADER lpbiOut, LPVOID lpOut, LONG x, LONG y);

// in decmprss.c
DWORD FAR PASCAL DecompressFrame16To8C(LPBITMAPINFOHEADER lpbiIn,  LPVOID lpIn,
                                  LPBITMAPINFOHEADER lpbiOut, LPVOID lpOut, LONG x, LONG y);

// in decmprss.c
DWORD FAR PASCAL DecompressFrame16To555C(LPBITMAPINFOHEADER lpbiIn,  LPVOID lpIn,
                        LPBITMAPINFOHEADER lpbiOut, LPVOID lpOut, LONG x, LONG y);

DWORD FAR PASCAL DecompressFrame16To565C(LPBITMAPINFOHEADER lpbiIn,  LPVOID lpIn,
                        LPBITMAPINFOHEADER lpbiOut, LPVOID lpOut, LONG x, LONG y);

// in decmprss.c
DWORD FAR PASCAL DecompressFrame16To8X2C(LPBITMAPINFOHEADER lpbiIn,  LPVOID lpIn,
                        LPBITMAPINFOHEADER lpbiOut, LPVOID lpOut, LONG x, LONG y);

#endif
