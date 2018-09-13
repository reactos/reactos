/**************************************************************************

    DRAWDIB.H   - routines for drawing DIBs to the screen.

    Copyright (c) 1990-1995, Microsoft Corp.  All rights reserved.

    this code handles stretching and dithering with custom code.

    the following DIB formats are supported:

        8bpp
        16bpp
        24bpp

    drawing to:

        16 color DC         (will dither 8bpp down)
        256 (palletized) DC (will dither 16 and 24bpp down)
        Full-color DC       (will just draw it!)

**************************************************************************/

#ifndef _INC_DRAWDIB
#define _INC_DRAWDIB

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RC_INVOKED
#ifndef VFWAPI
    #define VFWAPI  WINAPI
#ifdef WINAPIV
    #define VFWAPIV WINAPIV
#else
    #define VFWAPIV FAR CDECL
#endif
#endif
#endif

// begin_vfw32

typedef HANDLE HDRAWDIB; /* hdd */

/*********************************************************************

  DrawDib Flags

**********************************************************************/
#define DDF_0001            0x0001          /* ;Internal */
#define DDF_UPDATE          0x0002          /* re-draw the last DIB */
#define DDF_SAME_HDC        0x0004          /* HDC same as last call (all setup) */
#define DDF_SAME_DRAW       0x0008          /* draw params are the same */
#define DDF_DONTDRAW        0x0010          /* dont draw frame, just decompress */
#define DDF_ANIMATE         0x0020          /* allow palette animation */
#define DDF_BUFFER          0x0040          /* always buffer image */
#define DDF_JUSTDRAWIT      0x0080          /* just draw it with GDI */
#define DDF_FULLSCREEN      0x0100          /* use DisplayDib */
#define DDF_BACKGROUNDPAL   0x0200	    /* Realize palette in background */
#define DDF_NOTKEYFRAME     0x0400          /* this is a partial frame update, hint */
#define DDF_HURRYUP         0x0800          /* hurry up please! */
#define DDF_HALFTONE        0x1000          /* always halftone */
#define DDF_2000            0x2000          /* ;Internal */

#define DDF_PREROLL         DDF_DONTDRAW    /* Builing up a non-keyframe */
#define DDF_SAME_DIB        DDF_SAME_DRAW
#define DDF_SAME_SIZE       DDF_SAME_DRAW

/*********************************************************************

    DrawDib functions
	
*********************************************************************/
/*							// ;Internal
**  DrawDibInit()					// ;Internal
**							// ;Internal
*/							// ;Internal
extern BOOL VFWAPI DrawDibInit(void);			// ;Internal
							// ;Internal
/*
**  DrawDibOpen()
**
*/
extern HDRAWDIB VFWAPI DrawDibOpen(void);

/*
**  DrawDibClose()
**
*/
extern 
BOOL 
VFWAPI 
DrawDibClose(
    IN HDRAWDIB hdd
    );

/*
** DrawDibGetBuffer()
**
*/
extern 
LPVOID 
VFWAPI 
DrawDibGetBuffer(
    IN HDRAWDIB hdd, 
    OUT LPBITMAPINFOHEADER lpbi, 
    IN DWORD dwSize, 
    IN DWORD dwFlags
    );

/*							// ;Internal
**  DrawDibError()					// ;Internal
*/							// ;Internal
extern UINT VFWAPI DrawDibError(HDRAWDIB hdd);		// ;Internal
							// ;Internal
/*
**  DrawDibGetPalette()
**
**  get the palette used for drawing DIBs
**
*/
extern 
HPALETTE 
VFWAPI 
DrawDibGetPalette(
    IN HDRAWDIB hdd
    );


/*
**  DrawDibSetPalette()
**
**  get the palette used for drawing DIBs
**
*/
extern 
BOOL 
VFWAPI 
DrawDibSetPalette(
    IN HDRAWDIB hdd, 
    IN HPALETTE hpal
    );

/*
**  DrawDibChangePalette()
*/
extern 
BOOL 
VFWAPI 
DrawDibChangePalette(
    IN HDRAWDIB hdd, 
    IN int iStart, 
    IN int iLen, 
    IN LPPALETTEENTRY lppe
    );

/*
**  DrawDibRealize()
**
**  realize the palette in a HDD
**
*/
extern 
UINT 
VFWAPI 
DrawDibRealize(
    IN HDRAWDIB hdd, 
    IN HDC hdc, 
    IN BOOL fBackground
    );

/*
**  DrawDibStart()
**
**  start of streaming playback
**
*/
extern 
BOOL 
VFWAPI 
DrawDibStart(
    IN HDRAWDIB hdd, 
    IN DWORD rate
    );

/*
**  DrawDibStop()
**
**  start of streaming playback
**
*/
extern 
BOOL 
VFWAPI 
DrawDibStop(
    IN HDRAWDIB hdd
    );

/*
**  DrawDibBegin()
**
**  prepare to draw
**
*/
extern
BOOL 
VFWAPI 
DrawDibBegin(
    IN HDRAWDIB hdd,
    IN HDC      hdc,
    IN int      dxDst,
    IN int      dyDst,
    IN LPBITMAPINFOHEADER lpbi,
    IN int      dxSrc,
    IN int      dySrc,
    IN UINT     wFlags
    );

/*
**  DrawDibDraw()
**
**  actualy draw a DIB to the screen.
**
*/
extern 
BOOL 
VFWAPI 
DrawDibDraw(
    IN HDRAWDIB hdd,
    IN HDC      hdc,
    IN int      xDst,
    IN int      yDst,
    IN int      dxDst,
    IN int      dyDst,
    IN LPBITMAPINFOHEADER lpbi,
    IN LPVOID   lpBits,
    IN int      xSrc,
    IN int      ySrc,
    IN int      dxSrc,
    IN int      dySrc,
    IN UINT     wFlags
    );

/*
**  DrawDibUpdate()
**
**  redraw the last image (may only be valid with DDF_BUFFER)
*/
#define DrawDibUpdate(hdd, hdc, x, y) \
        DrawDibDraw(hdd, hdc, x, y, 0, 0, NULL, NULL, 0, 0, 0, 0, DDF_UPDATE)

/*
**  DrawDibEnd()
*/
extern 
BOOL 
VFWAPI 
DrawDibEnd(
    IN HDRAWDIB hdd
    );

/*
**  DrawDibTime()  [for debugging purposes only]
*/
typedef struct {
    LONG    timeCount;
    LONG    timeDraw;
    LONG    timeDecompress;
    LONG    timeDither;
    LONG    timeStretch;
    LONG    timeBlt;
    LONG    timeSetDIBits;
}   DRAWDIBTIME, FAR *LPDRAWDIBTIME;

BOOL 
VFWAPI 
DrawDibTime(
    IN HDRAWDIB hdd, 
    OUT LPDRAWDIBTIME lpddtime
    );

/* display profiling */
#define PD_CAN_DRAW_DIB         0x0001      /* if you can draw at all */
#define PD_CAN_STRETCHDIB       0x0002      /* basicly RC_STRETCHDIB */
#define PD_STRETCHDIB_1_1_OK    0x0004      /* is it fast? */
#define PD_STRETCHDIB_1_2_OK    0x0008      /* ... */
#define PD_STRETCHDIB_1_N_OK    0x0010      /* ... */

LRESULT
VFWAPI 
DrawDibProfileDisplay(
    IN LPBITMAPINFOHEADER lpbi
    );

// end_vfw32

#ifdef __cplusplus
}
#endif

#endif // _INC_DRAWDIB
