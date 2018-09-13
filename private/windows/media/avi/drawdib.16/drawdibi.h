/**************************************************************************

    DRAWDIBI.H   - internal DrawDib include file

**************************************************************************/

#ifndef WIN32
    #define VFWAPI  FAR PASCAL _loadds
    #define VFWAPIV FAR CDECL _loadds
#endif

/**************************************************************************
includes
**************************************************************************/

#include <win32.h>      // for Win32 and Win16
#include <memory.h>     // for _fmemcmp
#include <compman.h>

#include "drawdib.h"
#include "dither.h"
#include "stretch.h"
#include "lockbm.h"
#include "setdi.h"
#include "dciman.h"

/**************************************************************************
**************************************************************************/

#define DDF_OURFLAGS        0xFFFFC001l  /* internal flags */
#define DDF_MEMORYDC        0x00008000l  /* drawing to a memory DC */
#define DDF_WANTKEY         0x00004000l  /* wait for a key frame */
#define DDF_STRETCH         0x00010000l  /* we need to stretch */
#define DDF_DITHER          0x00020000l  /* we need to dither */
#define DDF_BITMAP          0x00040000l  /* Display driver sucks! */
#define DDF_X               0x00080000l  /* */
#define DDF_IDENTITYPAL     0x00100000l  /* 1:1 palette mapping */
#define DDF_CANBITMAPX      0x00200000l  /* can decompress to bitmap */
#define DDF_CANSCREENX      0x00400000l  /* we can decompress/draw to screen */
#define DDF_Y               0x00800000l  /* */
#define DDF_DIRTY           0x01000000l  /* decompress buffer is dirty (not valid) */
#define DDF_HUGEBITMAP      0x02000000l  /* decompressing to a HUGE bitmap */
#define DDF_XLATSOURCE      0x04000000l  /* need to xlat source cord. */
#define DDF_CLIPPED         0x08000000l  /* currently clipped */
#define DDF_NEWPALETTE      0x10000000l  /* palette needs mapped */
#define DDF_CLIPCHECK       0x20000000l  /* we care about clipping */
#define DDF_CANDRAWX        0x40000000l  /* we can draw direct to screen */
#define DDF_CANSETPAL       0x80000000l  /* codec supports ICM_SETPALETTE */
#define DDF_NAKED           0x00000001l  /* dont need GDI to translate */

#define DDF_USERFLAGS       0x00003FFEl  /* the user/called gives these, see .h */

/* these flags change what DrawDibBegin does */
#define DDF_BEGINFLAGS      (DDF_JUSTDRAWIT | DDF_BUFFER | DDF_ANIMATE | DDF_FULLSCREEN | DDF_HALFTONE)

/**************************************************************************

flags, a little more info for people who are not me

    DDF_OURFLAGS        these are internal state flags, not passed in by
                        the user.

    DDF_STRETCH         the current draw requires us to stretch, if GDI
                        is stretching this bit is clear.

    DDF_DITHER          the current draw requires a format conversion
                        note a 16->24 32->24 conversion is also called
                        a dither, again if GDI is taking care of it this
                        bit is clear.

    DDF_BITMAP          the display driver sucks and we are converting
                        DIB to BMPs before drawing

    DDF_CANBITMAPX      we can decompress to bitmaps.

    DDF_BITMAPX         we are decompressing directly into a bitmap

    DDF_IDENTITYPAL     the palette is a identity palette.

    DDF_CANSCREENX      we can decompress to screen with the current draw
                        params.

    DDF_SCREENX         we are currently decompressing to the screen.

    DDF_DIRTY           the decompress buffer is dirty, ie does not
                        match what *should* be on the screen.

    DDF_HUGEBITMAP      we are decompressing into a huge bitmap, and
                        then calling FlatToHuge...

    DDF_XLATSOURCE      the source cordinates need remapping after
                        decompression, (basicly the decompressor is
                        doing a stretch...)

    DDF_UPDATE          the buffer is valid but needs drawn to the screen.
                        this will get set when DDF_DONTDRAW is passed, and
                        we are decompressing to memory

                        another way to put it is, if DDF_UPDATE is set
                        the screen is out of sync with our internal
                        buffer (the internal buffer is more correct)

    DDF_CLIPPED         we are clipped

    DDF_NEWPALETTE      we need to build new palette map

    DDF_CLIPCHECK       please check for clipping changes.
    DDF_W
    DDF_Q

    DDF_USERFLAGS       these flags are defined in the API, the user will pass
                        these to us.

    DDF_BEGINFLAGS      these flags will effect what DrawDibBegin() does

**************************************************************************/

/**************************************************************************
**************************************************************************/

#ifdef DEBUG
    #define DPF( x ) ddprintf x
    #define DEBUG_RETAIL
#else
    #define DPF(x)
#endif
    
#ifdef DEBUG_RETAIL
    #define MODNAME "DRAWDIB"

    extern void FAR cdecl ddprintf(LPSTR szFormat, ...);

    #define RPF( x ) ddprintf x
#else
    #define RPF(X)
#endif

/**************************************************************************
*  The biXXXXX elements are grouped at the end to minimise the chance of
*  overwriting non bitmap data (i.e. pointers).  IF the code was totally
*  clean this would be irrelevant, however it does increase robustness.
**************************************************************************/

typedef struct {
    UINT                wSize;          /* MANDATORY: this MUST be the first field */
    ULONG               ulFlags;
    UINT                wError;

    #define DECOMPRESS_NONE   0
    #define DECOMPRESS_BITMAP 1
    #define DECOMPRESS_SCREEN 2
    #define DECOMPRESS_BUFFER 3
    int                 iDecompress;

    int                 dxSrc;
    int                 dySrc;
    int                 dxDst;
    int                 dyDst;

    HPALETTE            hpal;
    HPALETTE            hpalCopy;
    HPALETTE            hpalDraw;
    HPALETTE            hpalDrawLast;   /* hpalDraw for last DrawDibBegin */
    int                 ClrUsed;        /* number of colors used! */
    int                 iAnimateStart;  /* colors we can change */
    int                 iAnimateLen;
    int                 iAnimateEnd;

    int                 iPuntFrame;     /* how many frames we blew off */

    /*
     * set to DIB_RGB_COLORS, DIB_PAL_COLORS, or if on Win32 and 1:1 palette
     * DIB_PAL_INDICES (see DrawdibCheckPalette())
     *
     */
    UINT                uiPalUse;

    DITHERPROC          DitherProc;

    LPBYTE              pbBuffer;       /* decompress buffer */
    LPBYTE              pbStretch;      /* stretched bits. */

    //
    //  note we alias the stretch buffer for bitmaps too.
    //
    #define             biBitmap    biStretch
    #define             pbBitmap    pbStretch

    SETDI               sd;             /* for SetBitmap */
    HBITMAP             hbmDraw;        /* for drawing DIBs on the VGA!!! */
    HDC                 hdcDraw;
    HDC                 hdcLast;        /* hdc last call to DrawDibBegin */
    LPVOID              lpDIBSection;   /* pointer to dib section bits */

    LPBYTE              pbDither;       /* bits we will dither to */
    LPVOID              lpDitherTable;  /* for dithering */

    HIC                 hic;            /* decompressor */

#ifdef DEBUG_RETAIL
    DRAWDIBTIME         ddtime;
#endif


    LPBITMAPINFOHEADER  lpbi;           /* source dib format */
    RGBQUAD (FAR       *lpargbqIn)[256];/* source dib colors */
    BITMAPINFOHEADER    biBuffer;       /* decompress format */
    RGBQUAD             argbq[256];     /* drawdib colors */
    BITMAPINFOHEADER    biStretch;      /* stretched DIB */
    DWORD               smag[3];        /* room for masks */
    BITMAPINFOHEADER    biDraw;         /* DIB we will draw */
    WORD                aw[512];        /* either index's or RGBQs */
    BYTE                ab[256];        /* pallete mapping (!!!needed?) */

#ifndef _WIN32
    HTASK               htask;
#endif
}   DRAWDIB_STRUCT, *PDD;
/**************************************************************************
**************************************************************************/

extern DRAWDIB_STRUCT   gdd;
extern UINT             gwScreenBitDepth;
extern BOOL             gf286;


/**************************************************************************
**************************************************************************/

// flags for <wFlags> parameter of DisplayDib()
#define DISPLAYDIB_NOPALETTE        0x0010  // don't set palette
#define DISPLAYDIB_NOCENTER         0x0020  // don't center image
#define DISPLAYDIB_NOWAIT           0x0040  // don't wait before returning
#define DISPLAYDIB_NOIMAGE          0x0080  // don't draw image
#define DISPLAYDIB_ZOOM2            0x0100  // stretch by 2
#define DISPLAYDIB_DONTLOCKTASK     0x0200  // don't lock current task
#define DISPLAYDIB_TEST             0x0400  // testing the command
#define DISPLAYDIB_BEGIN            0x8000  // start of multiple calls
#define DISPLAYDIB_END              0x4000  // end of multiple calls

#define DISPLAYDIB_MODE_DEFAULT     0x0000

UINT (FAR PASCAL *DisplayDib)(LPBITMAPINFOHEADER lpbi, LPSTR lpBits, UINT wFlags);
UINT (FAR PASCAL *DisplayDibEx)(LPBITMAPINFOHEADER lpbi, int x, int y, LPSTR lpBits, UINT wFlags);

/**************************************************************************
**************************************************************************/

#ifdef DEBUG_RETAIL
    extern DWORD FAR PASCAL timeGetTime(void);

    #define TIMEINC()        pdd->ddtime.timeCount++
    #define TIMESTART(time)  pdd->ddtime.time -= timeGetTime()
    #define TIMEEND(time)    pdd->ddtime.time += timeGetTime()
#else
    #define TIMEINC()
    #define TIMESTART(time)
    #define TIMEEND(time)
#endif

/**************************************************************************
**************************************************************************/

#define WIDTHBYTES(i)     ((unsigned)((i+31)&(~31))/8)  /* ULONG aligned ! */
#define DIBWIDTHBYTES(bi) (UINT)WIDTHBYTES((int)(bi).biWidth * (int)(bi).biBitCount)
#define DIBSIZEIMAGE(bi)  ((DWORD)(UINT)(bi).biHeight * (DWORD)(UINT)DIBWIDTHBYTES(bi))

#define PUSHBI(bi) (int)(bi).biWidth, (int)(bi).biHeight, (int)(bi).biBitCount

/**************************************************************************
**************************************************************************/

//#define MEASURE_PERFORMANCE

#if defined(MEASURE_PERFORMANCE) && defined(WIN32) && defined(DEBUG)

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

static LARGE_INTEGER PC1;    /* current counter value    */
static LARGE_INTEGER PC2;    /* current counter value    */
static LARGE_INTEGER PC3;    /* current counter value    */

#define abs(x)  ((x) < 0 ? -(x) : (x))

static VOID StartCounting(VOID)
{
    QueryPerformanceCounter(&PC1);
    return;
}

static VOID EndCounting(LPSTR szId)
{
    QueryPerformanceCounter(&PC2);
    PC3 = I64Sub(PC2,PC1);
    DPF(("%s: %d ticks", szId, PC3.LowPart));
    return;
}

#else

#define StartCounting()
#define EndCounting(x)

#endif
