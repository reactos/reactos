/*----------------------------------------------------------------------+
| msvidc.h - Microsoft Video 1 Compressor - header file			|
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

#ifndef RC_INVOKED

#ifndef _INC_COMPDDK
#define _INC_COMPDDK    50      /* version number */
#endif

#include <vfw.h>

#include "decmprss.h"  // Must include DECMPRSS.H first
#include "compress.h"
#endif

#define ID_SCROLL   100
#define ID_TEXT     101

#define IDS_DESCRIPTION 42
#define IDS_NAME        43
#define IDS_ABOUT       44

extern HMODULE ghModule;

#define ALIGNULONG(i)     ((i+3)&(~3))                  /* ULONG aligned ! */
#define WIDTHBYTES(i)     ((unsigned)((i+31)&(~31))/8)  /* ULONG aligned ! */
#define DIBWIDTHBYTES(bi) (int)WIDTHBYTES((int)(bi).biWidth * (int)(bi).biBitCount)

#ifndef _WIN32
extern long FAR PASCAL muldiv32(long, long, long);
#endif

// in invcmap.c
LPVOID FAR PASCAL MakeITable(LPRGBQUAD lprgbq, int nColors);

typedef WORD RGB555;
typedef DWORD RGBDWORD;
typedef BYTE HUGE *HPBYTE;
typedef WORD HUGE *HPWORD;
typedef LONG HUGE *HPLONG;
typedef RGBDWORD HUGE *HPRGBDWORD;
typedef RGB555 HUGE *HPRGB555;
typedef RGBTRIPLE HUGE *HPRGBTRIPLE;
typedef RGBQUAD HUGE *HPRGBQUAD;

typedef struct {
    UINT    wTemporalRatio;     // 100 = 1.0, 50 = .50 etc...
}   ICSTATE;

typedef struct {
    DWORD       dwFlags;        // flags from ICOPEN
    DECOMPPROC  DecompressProc; // current decomp proc...
    DECOMPPROC  DecompressTest; // decomp proc...
    ICSTATE     CurrentState;   // current state of compressor.
    int         nCompress;      // count of COMPRESS_BEGIN calls
    int         nDecompress;    // count of DECOMPRESS_BEGIN calls
    int         nDraw;          // count of DRAW_BEGIN calls
    LONG (CALLBACK *Status) (LPARAM lParam, UINT message, LONG l);
    LPARAM	lParam;
    LPBYTE	lpITable;
    RGBQUAD	rgbqOut[256];
} INSTINFO, *PINSTINFO;


#ifdef _WIN32
#define   VideoLoad()   TRUE
#else
BOOL      NEAR PASCAL VideoLoad(void);
#endif
void      NEAR PASCAL VideoFree(void);
INSTINFO *NEAR PASCAL VideoOpen(ICOPEN FAR *icinfo);
LONG      NEAR PASCAL VideoClose(INSTINFO * pinst);
LONG      NEAR PASCAL GetState(INSTINFO * pinst, LPVOID pv, DWORD dwSize);
LONG      NEAR PASCAL SetState(INSTINFO * pinst, LPVOID pv, DWORD dwSize);
LONG      NEAR PASCAL GetInfo(INSTINFO * pinst, ICINFO FAR *icinfo, DWORD dwSize);

#define QueryAbout(x)  (TRUE)
//BOOL    NEAR PASCAL QueryAbout(INSTINFO * pinst);
LONG      NEAR PASCAL About(INSTINFO * pinst, HWND hwnd);
#define QueryConfigure(x)  (TRUE)
//BOOL    NEAR PASCAL QueryConfigure(INSTINFO * pinst);
LONG      NEAR PASCAL Configure(INSTINFO * pinst, HWND hwnd);

LONG      FAR PASCAL CompressBegin(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
LONG      FAR PASCAL CompressQuery(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn,LPBITMAPINFOHEADER lpbiOut);
LONG      FAR PASCAL CompressGetFormat(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
LONG      FAR PASCAL Compress(INSTINFO * pinst,ICCOMPRESS FAR *icinfo, DWORD dwSize);
LONG      FAR PASCAL CompressGetSize(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
LONG      FAR PASCAL CompressEnd(INSTINFO * lpinst);

LONG      NEAR PASCAL DecompressGetFormat(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
LONG      NEAR PASCAL DecompressGetPalette(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
LONG      NEAR PASCAL DecompressBegin(INSTINFO * pinst, DWORD dwFlags, LPBITMAPINFOHEADER lpbiSrc, LPVOID pSrc, int xSrc, int ySrc, int dxSrc, int dySrc, LPBITMAPINFOHEADER lpbiDst, LPVOID pDst, int xDst, int yDst, int dxDst, int dyDst);
LONG      NEAR PASCAL DecompressQuery(INSTINFO * pinst, DWORD dwFlags, LPBITMAPINFOHEADER lpbiSrc, LPVOID pSrc, int xSrc, int ySrc, int dxSrc, int dySrc, LPBITMAPINFOHEADER lpbiDst, LPVOID pDst, int xDst, int yDst, int dxDst, int dyDst);
LONG      NEAR PASCAL Decompress(INSTINFO * pinst, DWORD dwFlags, LPBITMAPINFOHEADER lpbiSrc, LPVOID pSrc, int xSrc, int ySrc, int dxSrc, int dySrc, LPBITMAPINFOHEADER lpbiDst, LPVOID pDst, int xDst, int yDst, int dxDst, int dyDst);
LONG      NEAR PASCAL DecompressEnd(INSTINFO * pinst);

LONG      NEAR PASCAL DrawQuery(INSTINFO * pinst,ICDRAWBEGIN FAR *icinfo, DWORD dwSize);
LONG      NEAR PASCAL DrawBegin(INSTINFO * pinst,ICDRAWBEGIN FAR *icinfo, DWORD dwSize);
LONG      NEAR PASCAL Draw(INSTINFO * pinst,ICDRAW FAR *icinfo, DWORD dwSize);
LONG      NEAR PASCAL DrawEnd(INSTINFO * pinst);

#ifdef DEBUG
    extern void FAR CDECL dprintf(LPSTR, ...);
    // Allow DPF statements to span multiple lines
    #define DPF( _x_ ) dprintf _x_
#else
    #define DPF(x)
#endif

#ifdef DEBUG
	/* Assert() macros */
	#define Assert(expr)		 _Assert((expr), __FILE__, __LINE__)
	#define AssertEval(expr)	 _Assert((expr), __FILE__, __LINE__)

	/* prototypes */
	BOOL FAR PASCAL _Assert(BOOL fExpr, LPSTR szFile, int iLine);

#else
	/* Assert() macros */
	#define Assert(expr)		 (TRUE)
	#define AssertEval(expr)	 (expr)

#endif
