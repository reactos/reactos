/**************************************************************************

    DRAWDIB.H   - routines for drawing DIBs to the screen.

    Copyright (c) 1990-1993, Microsoft Corp.  All rights reserved.

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

#ifndef VFWAPI
    #define VFWAPI  FAR PASCAL
    #define VFWAPIV FAR CDECL
#endif

typedef HANDLE HDRAWDIB; /* hdd */

/*********************************************************************

  DrawDib Flags

**********************************************************************/
#define DDF_0001            0x0001          /* */ /* Internal */
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
#define DDF_2000            0x2000          /* */ /* Internal */

#define DDF_PREROLL         DDF_DONTDRAW    /* Builing up a non-keyframe */
#define DDF_SAME_DIB        DDF_SAME_DRAW
#define DDF_SAME_SIZE       DDF_SAME_DRAW

/*********************************************************************

    DrawDib functions
	
*********************************************************************/
/*
**  DrawDibInit()
**
*/
extern BOOL VFWAPI DrawDibInit(void);

/*
**  DrawDibOpen()
**
*/
extern HDRAWDIB VFWAPI DrawDibOpen(void);

/*
**  DrawDibClose()
**
*/
extern BOOL VFWAPI DrawDibClose(HDRAWDIB hdd);

/*
** DrawDibGetBuffer()
**
*/
extern LPVOID VFWAPI DrawDibGetBuffer(HDRAWDIB hdd, LPBITMAPINFOHEADER lpbi, DWORD dwSize, DWORD dwFlags);

/*
**  DrawDibError()
*/
extern UINT VFWAPI DrawDibError(HDRAWDIB hdd);

/*
**  DrawDibGetPalette()
**
**  get the palette used for drawing DIBs
**
*/
extern HPALETTE VFWAPI DrawDibGetPalette(HDRAWDIB hdd);


/*
**  DrawDibSetPalette()
**
**  get the palette used for drawing DIBs
**
*/
extern BOOL VFWAPI DrawDibSetPalette(HDRAWDIB hdd, HPALETTE hpal);

/*
**  DrawDibChangePalette()
*/
extern BOOL VFWAPI DrawDibChangePalette(HDRAWDIB hdd, int iStart, int iLen, LPPALETTEENTRY lppe);

/*
**  DrawDibRealize()
**
**  realize the palette in a HDD
**
*/
extern UINT VFWAPI DrawDibRealize(HDRAWDIB hdd, HDC hdc, BOOL fBackground);

/*
**  DrawDibStart()
**
**  start of streaming playback
**
*/
extern BOOL VFWAPI DrawDibStart(HDRAWDIB hdd, DWORD rate);

/*
**  DrawDibStop()
**
**  start of streaming playback
**
*/
extern BOOL VFWAPI DrawDibStop(HDRAWDIB hdd);

/*
**  DrawDibBegin()
**
**  prepare to draw
**
*/
extern BOOL VFWAPI DrawDibBegin(HDRAWDIB hdd,
                                    HDC      hdc,
                                    int      dxDst,
                                    int      dyDst,
                                    LPBITMAPINFOHEADER lpbi,
                                    int      dxSrc,
                                    int      dySrc,
                                    UINT     wFlags);
/*
**  DrawDibDraw()
**
**  actualy draw a DIB to the screen.
**
*/
extern BOOL VFWAPI DrawDibDraw(HDRAWDIB hdd,
                                   HDC      hdc,
                                   int      xDst,
                                   int      yDst,
                                   int      dxDst,
                                   int      dyDst,
                                   LPBITMAPINFOHEADER lpbi,
                                   LPVOID   lpBits,
                                   int      xSrc,
                                   int      ySrc,
                                   int      dxSrc,
                                   int      dySrc,
                                   UINT     wFlags);

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
extern BOOL VFWAPI DrawDibEnd(HDRAWDIB hdd);

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

BOOL VFWAPI DrawDibTime(HDRAWDIB hdd, LPDRAWDIBTIME lpddtime);

/* display profiling */
#define PD_CAN_DRAW_DIB         0x0001      /* if you can draw at all */
#define PD_CAN_STRETCHDIB       0x0002      /* basicly RC_STRETCHDIB */
#define PD_STRETCHDIB_1_1_OK    0x0004      /* is it fast? */
#define PD_STRETCHDIB_1_2_OK    0x0008      /* ... */
#define PD_STRETCHDIB_1_N_OK    0x0010      /* ... */

DWORD VFWAPI DrawDibProfileDisplay(LPBITMAPINFOHEADER lpbi);

#ifdef __cplusplus
}
#endif

#endif // _INC_DRAWDIB
