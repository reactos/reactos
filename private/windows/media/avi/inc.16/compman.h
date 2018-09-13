/*
*  Header file for comunication with AVI installable compressors/decompressors
*
*  Copyright (c) 1990-1994, Microsoft Corp.  All rights reserved.
*
* Win16:
*
* Installable compressors should be listed in SYSTEM.INI as
* follows:
*
* [Drivers]
*      VIDC.MSSQ = mssqcomp.drv
*      VIDC.XXXX = foodrv.drv
*
* Win32: (NT)
*
* Installable compressors should be listed in the registration database
* under the key
*   HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Drivers32
*      VIDC.MSSQ = mssqcomp.dll
*      VIDC.XXXX = foodrv.dll
*
*
* That is, an identifying FOURCC should be the key, and the value
* should be the driver filename
*
*/

#ifndef _INC_COMPMAN
#define _INC_COMPMAN

#ifndef RC_INVOKED
#ifndef VFWAPI
#ifdef WIN32
    #define VFWAPI  WINAPI
#ifdef WINAPIV
    #define VFWAPIV WINAPIV
#else
    #define VFWAPIV FAR CDECL
#endif
#else
    #define VFWAPI  FAR PASCAL
    #define VFWAPIV FAR CDECL
#endif
#endif
#endif

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

/************************************************************************

    messages and structures.

************************************************************************/

#if !defined HTASK
    #define HTASK HANDLE
#endif
#include "compddk.h"            // include this file for the messages.

// begin_vfw32

/************************************************************************

    ICM function declarations
	
************************************************************************/

BOOL    VFWAPI ICInfo(DWORD fccType, DWORD fccHandler, ICINFO FAR * lpicinfo);
BOOL    VFWAPI ICInstall(DWORD fccType, DWORD fccHandler, LPARAM lParam, LPSTR szDesc, UINT wFlags);
BOOL    VFWAPI ICRemove(DWORD fccType, DWORD fccHandler, UINT wFlags);
LRESULT VFWAPI ICGetInfo(HIC hic, ICINFO FAR *picinfo, DWORD cb);

HIC     VFWAPI ICOpen(DWORD fccType, DWORD fccHandler, UINT wMode);
HIC     VFWAPI ICOpenFunction(DWORD fccType, DWORD fccHandler, UINT wMode, FARPROC lpfnHandler);
LRESULT VFWAPI ICClose(HIC hic);

LRESULT VFWAPI  ICSendMessage(HIC hic, UINT msg, DWORD dw1, DWORD dw2);
#ifndef WIN32
//this function is unsupported on Win32 as it is non-portable.
LRESULT VFWAPIV ICMessage(HIC hic, UINT msg, UINT cb, ...);
#endif


/* Values for wFlags of ICInstall() */
#define ICINSTALL_FUNCTION      0x0001  // lParam is a DriverProc (function ptr)
#define ICINSTALL_DRIVER        0x0002  // lParam is a driver name (string)
#define ICINSTALL_HDRV          0x0004  // lParam is a HDRVR (driver handle)

/************************************************************************

    query macros

************************************************************************/
#define ICMF_CONFIGURE_QUERY     0x00000001
#define ICMF_ABOUT_QUERY         0x00000001

#define ICQueryAbout(hic) \
    (ICSendMessage(hic, ICM_ABOUT, (DWORD) -1, ICMF_ABOUT_QUERY) == ICERR_OK)

#define ICAbout(hic, hwnd) \
    ICSendMessage(hic, ICM_ABOUT, (DWORD)(UINT)(hwnd), 0)

#define ICQueryConfigure(hic) \
    (ICSendMessage(hic, ICM_CONFIGURE, (DWORD) -1, ICMF_CONFIGURE_QUERY) == ICERR_OK)

#define ICConfigure(hic, hwnd) \
    ICSendMessage(hic, ICM_CONFIGURE, (DWORD)(UINT)(hwnd), 0)

/************************************************************************

    get/set state macros
	
************************************************************************/

#define ICGetState(hic, pv, cb) \
    ICSendMessage(hic, ICM_GETSTATE, (DWORD)(LPVOID)(pv), (DWORD)(cb))

#define ICSetState(hic, pv, cb) \
    ICSendMessage(hic, ICM_SETSTATE, (DWORD)(LPVOID)(pv), (DWORD)(cb))

#define ICGetStateSize(hic) \
    ICGetState(hic, NULL, 0)

/************************************************************************

    get value macros

************************************************************************/
static DWORD dwICValue;

#define ICGetDefaultQuality(hic) \
    (ICSendMessage(hic, ICM_GETDEFAULTQUALITY, (DWORD)(LPVOID)&dwICValue, sizeof(DWORD)), dwICValue)

#define ICGetDefaultKeyFrameRate(hic) \
    (ICSendMessage(hic, ICM_GETDEFAULTKEYFRAMERATE, (DWORD)(LPVOID)&dwICValue, sizeof(DWORD)), dwICValue)

/************************************************************************

    draw window macro
	
************************************************************************/
#define ICDrawWindow(hic, prc) \
    ICSendMessage(hic, ICM_DRAW_WINDOW, (DWORD)(LPVOID)(prc), sizeof(RECT))

/************************************************************************

    compression functions

************************************************************************/
/*
 *  ICCompress()
 *
 *  compress a single frame
 *
 */
DWORD VFWAPIV ICCompress(
    HIC                 hic,
    DWORD               dwFlags,        // flags
    LPBITMAPINFOHEADER  lpbiOutput,     // output format
    LPVOID              lpData,         // output data
    LPBITMAPINFOHEADER  lpbiInput,      // format of frame to compress
    LPVOID              lpBits,         // frame data to compress
    LPDWORD             lpckid,         // ckid for data in AVI file
    LPDWORD             lpdwFlags,      // flags in the AVI index.
    LONG                lFrameNum,      // frame number of seq.
    DWORD               dwFrameSize,    // reqested size in bytes. (if non zero)
    DWORD               dwQuality,      // quality within one frame
    LPBITMAPINFOHEADER  lpbiPrev,       // format of previous frame
    LPVOID              lpPrev);        // previous frame

/*
 *  ICCompressBegin()
 *
 *  start compression from a source format (lpbiInput) to a dest
 *  format (lpbiOuput) is supported.
 *
 */
#define ICCompressBegin(hic, lpbiInput, lpbiOutput) \
    ICSendMessage(hic, ICM_COMPRESS_BEGIN, (DWORD)(LPVOID)(lpbiInput), (DWORD)(LPVOID)(lpbiOutput))

/*
 *  ICCompressQuery()
 *
 *  determines if compression from a source format (lpbiInput) to a dest
 *  format (lpbiOuput) is supported.
 *
 */
#define ICCompressQuery(hic, lpbiInput, lpbiOutput) \
    ICSendMessage(hic, ICM_COMPRESS_QUERY, (DWORD)(LPVOID)(lpbiInput), (DWORD)(LPVOID)(lpbiOutput))

/*
 *  ICCompressGetFormat()
 *
 *  get the output format, (format of compressed data)
 *  if lpbiOutput is NULL return the size in bytes needed for format.
 *
 */
#define ICCompressGetFormat(hic, lpbiInput, lpbiOutput) \
    ICSendMessage(hic, ICM_COMPRESS_GET_FORMAT, (DWORD)(LPVOID)(lpbiInput), (DWORD)(LPVOID)(lpbiOutput))

#define ICCompressGetFormatSize(hic, lpbi) \
    ICCompressGetFormat(hic, lpbi, NULL)

/*
 *  ICCompressSize()
 *
 *  return the maximal size of a compressed frame
 *
 */
#define ICCompressGetSize(hic, lpbiInput, lpbiOutput) \
    ICSendMessage(hic, ICM_COMPRESS_GET_SIZE, (DWORD)(LPVOID)(lpbiInput), (DWORD)(LPVOID)(lpbiOutput))

#define ICCompressEnd(hic) \
    ICSendMessage(hic, ICM_COMPRESS_END, 0, 0)

/************************************************************************

    decompression functions

************************************************************************/

/*
 *  ICDecompress()
 *
 *  decompress a single frame
 *
 */
#define ICDECOMPRESS_HURRYUP    0x80000000L     // don't draw just buffer (hurry up!)

DWORD VFWAPIV ICDecompress(
    HIC                 hic,
    DWORD               dwFlags,    // flags (from AVI index...)
    LPBITMAPINFOHEADER  lpbiFormat, // BITMAPINFO of compressed data
                                    // biSizeImage has the chunk size
    LPVOID              lpData,     // data
    LPBITMAPINFOHEADER  lpbi,       // DIB to decompress to
    LPVOID              lpBits);

/*
 *  ICDecompressBegin()
 *
 *  start compression from a source format (lpbiInput) to a dest
 *  format (lpbiOutput) is supported.
 *
 */
#define ICDecompressBegin(hic, lpbiInput, lpbiOutput) \
    ICSendMessage(hic, ICM_DECOMPRESS_BEGIN, (DWORD)(LPVOID)(lpbiInput), (DWORD)(LPVOID)(lpbiOutput))

/*
 *  ICDecompressQuery()
 *
 *  determines if compression from a source format (lpbiInput) to a dest
 *  format (lpbiOutput) is supported.
 *
 */
#define ICDecompressQuery(hic, lpbiInput, lpbiOutput) \
    ICSendMessage(hic, ICM_DECOMPRESS_QUERY, (DWORD)(LPVOID)(lpbiInput), (DWORD)(LPVOID)(lpbiOutput))

/*
 *  ICDecompressGetFormat()
 *
 *  get the output format, (format of un-compressed data)
 *  if lpbiOutput is NULL return the size in bytes needed for format.
 *
 */
#define ICDecompressGetFormat(hic, lpbiInput, lpbiOutput) \
    ((LONG) ICSendMessage(hic, ICM_DECOMPRESS_GET_FORMAT, (DWORD)(LPVOID)(lpbiInput), (DWORD)(LPVOID)(lpbiOutput)))

#define ICDecompressGetFormatSize(hic, lpbi) \
    ICDecompressGetFormat(hic, lpbi, NULL)

/*
 *  ICDecompressGetPalette()
 *
 *  get the output palette
 *
 */
#define ICDecompressGetPalette(hic, lpbiInput, lpbiOutput) \
    ICSendMessage(hic, ICM_DECOMPRESS_GET_PALETTE, (DWORD)(LPVOID)(lpbiInput), (DWORD)(LPVOID)(lpbiOutput))

#define ICDecompressSetPalette(hic, lpbiPalette) \
    ICSendMessage(hic, ICM_DECOMPRESS_SET_PALETTE, (DWORD)(LPVOID)(lpbiPalette), 0)

#define ICDecompressEnd(hic) \
    ICSendMessage(hic, ICM_DECOMPRESS_END, 0, 0)

/************************************************************************

    decompression (ex) functions

************************************************************************/

// end_vfw32

#ifdef WIN32

// begin_vfw32

//
// on Win16 these functions are macros that call ICMessage. ICMessage will
// not work on NT. rather than add new entrypoints we have given
// them as static inline functions
//

/*
 *  ICDecompressEx()
 *
 *  decompress a single frame
 *
 */
static __inline LRESULT VFWAPI
ICDecompressEx(
            HIC hic,
            DWORD dwFlags,
            LPBITMAPINFOHEADER lpbiSrc,
            LPVOID lpSrc,
            int xSrc,
            int ySrc,
            int dxSrc,
            int dySrc,
            LPBITMAPINFOHEADER lpbiDst,
            LPVOID lpDst,
            int xDst,
            int yDst,
            int dxDst,
            int dyDst)
{
    ICDECOMPRESSEX ic;

    ic.dwFlags = dwFlags;
    ic.lpbiSrc = lpbiSrc;
    ic.lpSrc = lpSrc;
    ic.xSrc = xSrc;
    ic.ySrc = ySrc;
    ic.dxSrc = dxSrc;
    ic.dySrc = dySrc;
    ic.lpbiDst = lpbiDst;
    ic.lpDst = lpDst;
    ic.xDst = xDst;
    ic.yDst = yDst;
    ic.dxDst = dxDst;
    ic.dyDst = dyDst;

    // note that ICM swaps round the length and pointer
    // length in lparam2, pointer in lparam1
    return ICSendMessage(hic, ICM_DECOMPRESSEX, (DWORD)&ic, sizeof(ic));
}


/*
 *  ICDecompressExBegin()
 *
 *  start compression from a source format (lpbiInput) to a dest
 *  format (lpbiOutput) is supported.
 *
 */
static __inline LRESULT VFWAPI
ICDecompressExBegin(
            HIC hic,
            DWORD dwFlags,
            LPBITMAPINFOHEADER lpbiSrc,
            LPVOID lpSrc,
            int xSrc,
            int ySrc,
            int dxSrc,
            int dySrc,
            LPBITMAPINFOHEADER lpbiDst,
            LPVOID lpDst,
            int xDst,
            int yDst,
            int dxDst,
            int dyDst)
{
    ICDECOMPRESSEX ic;

    ic.dwFlags = dwFlags;
    ic.lpbiSrc = lpbiSrc;
    ic.lpSrc = lpSrc;
    ic.xSrc = xSrc;
    ic.ySrc = ySrc;
    ic.dxSrc = dxSrc;
    ic.dySrc = dySrc;
    ic.lpbiDst = lpbiDst;
    ic.lpDst = lpDst;
    ic.xDst = xDst;
    ic.yDst = yDst;
    ic.dxDst = dxDst;
    ic.dyDst = dyDst;

    // note that ICM swaps round the length and pointer
    // length in lparam2, pointer in lparam1
    return ICSendMessage(hic, ICM_DECOMPRESSEX_BEGIN, (DWORD)&ic, sizeof(ic));
}

/*
 *  ICDecompressExQuery()
 *
 */
static __inline LRESULT VFWAPI
ICDecompressExQuery(
            HIC hic,
            DWORD dwFlags,
            LPBITMAPINFOHEADER lpbiSrc,
            LPVOID lpSrc,
            int xSrc,
            int ySrc,
            int dxSrc,
            int dySrc,
            LPBITMAPINFOHEADER lpbiDst,
            LPVOID lpDst,
            int xDst,
            int yDst,
            int dxDst,
            int dyDst)
{
    ICDECOMPRESSEX ic;

    ic.dwFlags = dwFlags;
    ic.lpbiSrc = lpbiSrc;
    ic.lpSrc = lpSrc;
    ic.xSrc = xSrc;
    ic.ySrc = ySrc;
    ic.dxSrc = dxSrc;
    ic.dySrc = dySrc;
    ic.lpbiDst = lpbiDst;
    ic.lpDst = lpDst;
    ic.xDst = xDst;
    ic.yDst = yDst;
    ic.dxDst = dxDst;
    ic.dyDst = dyDst;

    // note that ICM swaps round the length and pointer
    // length in lparam2, pointer in lparam1
    return ICSendMessage(hic, ICM_DECOMPRESSEX_QUERY, (DWORD)&ic, sizeof(ic));
}

// end_vfw32

#else

// these macros need to be functions for WIN32 because ICMessage is
// essentially unsupportable on NT

/*
 *  ICDecompressEx()
 *
 *  decompress a single frame
 *
 */
#define ICDecompressEx(hic, dwFlags, lpbiSrc, lpSrc, xSrc, ySrc, dxSrc, dySrc, lpbiDst, lpDst, xDst, yDst, dxDst, dyDst) \
    ICMessage(hic, ICM_DECOMPRESSEX, sizeof(ICDECOMPRESSEX), \
        (DWORD)(dwFlags), \
        (LPBITMAPINFOHEADER)(lpbiSrc), (LPVOID)(lpSrc), \
        (LPBITMAPINFOHEADER)(lpbiDst), (LPVOID)(lpDst), \
        (int)(xDst), (int)(yDst), (int)(dxDst), (int)(dyDst), \
        (int)(xSrc), (int)(ySrc), (int)(dxSrc), (int)(dySrc))

/*
 *  ICDecompressBegin()
 *
 *  start compression from a source format (lpbiInput) to a dest
 *  format (lpbiOutput) is supported.
 *
 */
#define ICDecompressExBegin(hic, dwFlags, lpbiSrc, lpSrc, xSrc, ySrc, dxSrc, dySrc, lpbiDst, lpDst, xDst, yDst, dxDst, dyDst) \
    ICMessage(hic, ICM_DECOMPRESSEX_BEGIN, sizeof(ICDECOMPRESSEX), \
        (DWORD)(dwFlags), \
        (LPBITMAPINFOHEADER)(lpbiSrc), (LPVOID)(lpSrc), \
        (LPBITMAPINFOHEADER)(lpbiDst), (LPVOID)(lpDst), \
        (int)(xDst), (int)(yDst), (int)(dxDst), (int)(dyDst), \
        (int)(xSrc), (int)(ySrc), (int)(dxSrc), (int)(dySrc))

/*
 *  ICDecompressExQuery()
 *
 */
#define ICDecompressExQuery(hic, dwFlags, lpbiSrc, lpSrc, xSrc, ySrc, dxSrc, dySrc, lpbiDst, lpDst, xDst, yDst, dxDst, dyDst) \
    ICMessage(hic, ICM_DECOMPRESSEX_QUERY,  sizeof(ICDECOMPRESSEX), \
        (DWORD)(dwFlags), \
        (LPBITMAPINFOHEADER)(lpbiSrc), (LPVOID)(lpSrc), \
        (LPBITMAPINFOHEADER)(lpbiDst), (LPVOID)(lpDst), \
        (int)(xDst), (int)(yDst), (int)(dxDst), (int)(dyDst), \
        (int)(xSrc), (int)(ySrc), (int)(dxSrc), (int)(dySrc))
#endif

// begin_vfw32

#define ICDecompressExEnd(hic) \
    ICSendMessage(hic, ICM_DECOMPRESSEX_END, 0, 0)

/************************************************************************

    drawing functions

************************************************************************/

/*
 *  ICDrawBegin()
 *
 *  start decompressing data with format (lpbiInput) directly to the screen
 *
 *  return zero if the decompressor supports drawing.
 *
 */

#define ICDRAW_QUERY        0x00000001L   // test for support
#define ICDRAW_FULLSCREEN   0x00000002L   // draw to full screen
#define ICDRAW_HDC          0x00000004L   // draw to a HDC/HWND

DWORD VFWAPIV ICDrawBegin(
        HIC                 hic,
        DWORD               dwFlags,        // flags
        HPALETTE            hpal,           // palette to draw with
        HWND                hwnd,           // window to draw to
        HDC                 hdc,            // HDC to draw to
        int                 xDst,           // destination rectangle
        int                 yDst,
        int                 dxDst,
        int                 dyDst,
        LPBITMAPINFOHEADER  lpbi,           // format of frame to draw
        int                 xSrc,           // source rectangle
        int                 ySrc,
        int                 dxSrc,
        int                 dySrc,
        DWORD               dwRate,         // frames/second = (dwRate/dwScale)
        DWORD               dwScale);

/*
 *  ICDraw()
 *
 *  decompress data directly to the screen
 *
 */

#define ICDRAW_HURRYUP      0x80000000L   // don't draw just buffer (hurry up!)
#define ICDRAW_UPDATE       0x40000000L   // don't draw just update screen

DWORD VFWAPIV ICDraw(
        HIC                 hic,
        DWORD               dwFlags,        // flags
        LPVOID		    lpFormat,       // format of frame to decompress
        LPVOID              lpData,         // frame data to decompress
        DWORD               cbData,         // size of data
        LONG                lTime);         // time to draw this frame

// end_vfw32

#ifdef WIN32

// begin_vfw32

// ICMessage is not supported on Win32, so provide a static inline function
// to do the same job
static __inline LRESULT VFWAPI
ICDrawSuggestFormat(
            HIC hic,
            LPBITMAPINFOHEADER lpbiIn,
            LPBITMAPINFOHEADER lpbiOut,
            int dxSrc,
            int dySrc,
            int dxDst,
            int dyDst,
            HIC hicDecomp)
{
    ICDRAWSUGGEST ic;

    ic.lpbiIn = lpbiIn;
    ic.lpbiSuggest = lpbiOut;
    ic.dxSrc = dxSrc;
    ic.dySrc = dySrc;
    ic.dxDst = dxDst;
    ic.dyDst = dyDst;
    ic.hicDecompressor = hicDecomp;

    // note that ICM swaps round the length and pointer
    // length in lparam2, pointer in lparam1
    return ICSendMessage(hic, ICM_DRAW_SUGGESTFORMAT, (DWORD)&ic, sizeof(ic));
}

// end_vfw32

#else
#define ICDrawSuggestFormat(hic,lpbiIn,lpbiOut,dxSrc,dySrc,dxDst,dyDst,hicDecomp) \
        ICMessage(hic, ICM_DRAW_SUGGESTFORMAT, sizeof(ICDRAWSUGGEST),   \
            (LPBITMAPINFOHEADER)(lpbiIn),(LPBITMAPINFOHEADER)(lpbiOut), \
            (int)(dxSrc),(int)(dySrc),(int)(dxDst),(int)(dyDst), (HIC)(hicDecomp))
#endif

// begin_vfw32

/*
 *  ICDrawQuery()
 *
 *  determines if the compressor is willing to render the specified format.
 *
 */
#define ICDrawQuery(hic, lpbiInput) \
    ICSendMessage(hic, ICM_DRAW_QUERY, (DWORD)(LPVOID)(lpbiInput), 0L)

#define ICDrawChangePalette(hic, lpbiInput) \
    ICSendMessage(hic, ICM_DRAW_CHANGEPALETTE, (DWORD)(LPVOID)(lpbiInput), 0L)

#define ICGetBuffersWanted(hic, lpdwBuffers) \
    ICSendMessage(hic, ICM_GETBUFFERSWANTED, (DWORD)(LPVOID)(lpdwBuffers), 0)

#define ICDrawEnd(hic) \
    ICSendMessage(hic, ICM_DRAW_END, 0, 0)

#define ICDrawStart(hic) \
    ICSendMessage(hic, ICM_DRAW_START, 0, 0)

#define ICDrawStartPlay(hic, lFrom, lTo) \
    ICSendMessage(hic, ICM_DRAW_START_PLAY, (DWORD)(lFrom), (DWORD)(lTo))

#define ICDrawStop(hic) \
    ICSendMessage(hic, ICM_DRAW_STOP, 0, 0)

#define ICDrawStopPlay(hic) \
    ICSendMessage(hic, ICM_DRAW_STOP_PLAY, 0, 0)

#define ICDrawGetTime(hic, lplTime) \
    ICSendMessage(hic, ICM_DRAW_GETTIME, (DWORD)(LPVOID)(lplTime), 0)

#define ICDrawSetTime(hic, lTime) \
    ICSendMessage(hic, ICM_DRAW_SETTIME, (DWORD)lTime, 0)

#define ICDrawRealize(hic, hdc, fBackground) \
    ICSendMessage(hic, ICM_DRAW_REALIZE, (DWORD)(UINT)(HDC)(hdc), (DWORD)(BOOL)(fBackground))

#define ICDrawFlush(hic) \
    ICSendMessage(hic, ICM_DRAW_FLUSH, 0, 0)

#define ICDrawRenderBuffer(hic) \
    ICSendMessage(hic, ICM_DRAW_RENDERBUFFER, 0, 0)

/************************************************************************

    Status callback functions

************************************************************************/

/*
 *  ICSetStatusProc()
 *
 *  Set the status callback function
 *
 */

// end_vfw32

#ifdef WIN32

// begin_vfw32

// ICMessage is not supported on NT
static __inline LRESULT VFWAPI
ICSetStatusProc(
            HIC hic,
            DWORD dwFlags,
            LRESULT lParam,
            LONG (CALLBACK *fpfnStatus)(LPARAM, UINT, LONG) )
{
    ICSETSTATUSPROC ic;

    ic.dwFlags = dwFlags;
    ic.lParam = lParam;
    ic.Status = fpfnStatus;

    // note that ICM swaps round the length and pointer
    // length in lparam2, pointer in lparam1
    return ICSendMessage(hic, ICM_SET_STATUS_PROC, (DWORD)&ic, sizeof(ic));
}

// end_vfw32

#else

#define ICSetStatusProc(hic, dwFlags, lParam, fpfnStatus) \
    ICMessage(hic, ICM_SET_STATUS_PROC, sizeof(ICSETSTATUSPROC), \
        (DWORD)(dwFlags), \
	(LRESULT)(lParam), \
	(LONG ((CALLBACK *) ()))(fpfnStatus))
#endif

// begin_vfw32

/************************************************************************

helper routines for DrawDib and MCIAVI...

************************************************************************/

#define ICDecompressOpen(fccType, fccHandler, lpbiIn, lpbiOut) \
    ICLocate(fccType, fccHandler, lpbiIn, lpbiOut, ICMODE_DECOMPRESS)

#define ICDrawOpen(fccType, fccHandler, lpbiIn) \
    ICLocate(fccType, fccHandler, lpbiIn, NULL, ICMODE_DRAW)

HIC  VFWAPI ICLocate(DWORD fccType, DWORD fccHandler, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut, WORD wFlags);
HIC  VFWAPI ICGetDisplayFormat(HIC hic, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut, int BitDepth, int dx, int dy);

/************************************************************************
Higher level functions
************************************************************************/

HANDLE VFWAPI ICImageCompress(
        HIC                 hic,        // compressor to use
        UINT                uiFlags,    // flags (none yet)
        LPBITMAPINFO	    lpbiIn,     // format to compress from
        LPVOID              lpBits,     // data to compress
        LPBITMAPINFO        lpbiOut,    // compress to this (NULL ==> default)
        LONG                lQuality,   // quality to use
        LONG FAR *          plSize);     // compress to this size (0=whatever)

HANDLE VFWAPI ICImageDecompress(
        HIC                 hic,        // compressor to use
        UINT                uiFlags,    // flags (none yet)
        LPBITMAPINFO        lpbiIn,     // format to decompress from
        LPVOID              lpBits,     // data to decompress
        LPBITMAPINFO        lpbiOut);   // decompress to this (NULL ==> default)

//
// Structure used by ICSeqCompressFrame and ICCompressorChoose routines
// Make sure this matches the autodoc in icm.c!
//
typedef struct {
    LONG		cbSize;		// set to sizeof(COMPVARS) before
					// calling ICCompressorChoose
    DWORD		dwFlags;	// see below...
    HIC			hic;		// HIC of chosen compressor
    DWORD               fccType;	// basically ICTYPE_VIDEO
    DWORD               fccHandler;	// handler of chosen compressor or
					// "" or "DIB "
    LPBITMAPINFO	lpbiIn;		// input format
    LPBITMAPINFO	lpbiOut;	// output format - will compress to this
    LPVOID		lpBitsOut;
    LPVOID		lpBitsPrev;
    LONG		lFrame;
    LONG		lKey;		// key frames how often?
    LONG		lDataRate;	// desired data rate KB/Sec
    LONG		lQ;		// desired quality
    LONG		lKeyCount;
    LPVOID		lpState;	// state of compressor
    LONG		cbState;	// size of the state
} COMPVARS, FAR *PCOMPVARS;

// FLAGS for dwFlags element of COMPVARS structure:
// set this flag if you initialize COMPVARS before calling ICCompressorChoose
#define ICMF_COMPVARS_VALID	0x00000001	// COMPVARS contains valid data

//
//  allows user to choose compressor, quality etc...
//
BOOL VFWAPI ICCompressorChoose(
        HWND        hwnd,               // parent window for dialog
        UINT        uiFlags,            // flags
        LPVOID      pvIn,               // input format (optional)
        LPVOID      lpData,             // input data (optional)
        PCOMPVARS   pc,                 // data about the compressor/dlg
        LPSTR       lpszTitle);         // dialog title (optional)

// defines for uiFlags
#define ICMF_CHOOSE_KEYFRAME	0x0001	// show KeyFrame Every box
#define ICMF_CHOOSE_DATARATE	0x0002	// show DataRate box
#define ICMF_CHOOSE_PREVIEW	0x0004	// allow expanded preview dialog
#define ICMF_CHOOSE_ALLCOMPRESSORS	0x0008	// don't only show those that
						// can handle the input format
						// or input data

BOOL VFWAPI ICSeqCompressFrameStart(PCOMPVARS pc, LPBITMAPINFO lpbiIn);
void VFWAPI ICSeqCompressFrameEnd(PCOMPVARS pc);

LPVOID VFWAPI ICSeqCompressFrame(
    PCOMPVARS               pc,         // set by ICCompressorChoose
    UINT                    uiFlags,    // flags
    LPVOID                  lpBits,     // input DIB bits
    BOOL FAR 		    *pfKey,	// did it end up being a key frame?
    LONG FAR		    *plSize);	// size to compress to/of returned image

void VFWAPI ICCompressorFree(PCOMPVARS pc);

// end_vfw32

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif	/* __cplusplus */

#endif /* _INC_COMPMAN */
