/*----------------------------------------------------------------------+
| msvidc.c - Microsoft Video 1 Compressor                               |
|                                                                       |
| Copyright (c) 1990-1995 Microsoft Corporation.                        |
| Portions Copyright Media Vision Inc.                                  |
| All Rights Reserved.                                                  |
|                                                                       |
| You have a non-exclusive, worldwide, royalty-free, and perpetual      |
| license to use this source code in developing hardware, software      |
| (limited to drivers and other software required for hardware          |
| functionality), and firmware for video display and/or processing      |
| boards.   Microsoft makes no warranties, express or implied, with     |
| respect to the Video 1 codec, including without limitation warranties |
| of merchantability or fitness for a particular purpose.  Microsoft    |
| shall not be liable for any damages whatsoever, including without     |
| limitation consequential damages arising from your use of the Video 1 |
| codec.                                                                |
|                                                                       |
|                                                                       |
+----------------------------------------------------------------------*/
#include <win32.h>
#include <ole2.h>
#include <mmsystem.h>

#ifndef _INC_COMPDDK
#define _INC_COMPDDK    50      /* version number */
#endif

#include <vfw.h>

#ifdef _WIN32
#define abs(x)  ((x) < 0 ? -(x) : (x))
#endif

#ifdef _WIN32
#include <memory.h>     /* for memcpy */
#endif


#ifdef _WIN32
#define _FPInit()       0
#define _FPTerm(x)
#else
void _acrtused2(void) {}
extern LPVOID WINAPI _FPInit(void);
extern void   WINAPI _FPTerm(LPVOID);
#endif

#include "msvidc.h"
#ifdef _WIN32
#include "profile.h"
#endif

#ifndef _WIN32
static BOOL gf286 = FALSE;
#endif

#define FOURCC_MSVC     mmioFOURCC('M','S','V','C')
#define FOURCC_CRAM     mmioFOURCC('C','R','A','M')
#define FOURCC_Cram     mmioFOURCC('C','r','a','m')
#define TWOCC_XX        aviTWOCC('d', 'c')

#define QUALITY_DEFAULT 2500

#define VERSION         0x00010000      // 1.0

ICSTATE   DefaultState = {75};

INT_PTR FAR PASCAL _LOADDS ConfigureDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);

#ifndef _WIN32
//
// put the compress stuff in the "rare" segment
//
#pragma alloc_text(_TEXT, ConfigureDlgProc)
#pragma alloc_text(_TEXT, CompressBegin)
#pragma alloc_text(_TEXT, CompressQuery)
#pragma alloc_text(_TEXT, CompressGetFormat)
#pragma alloc_text(_TEXT, Compress)
#pragma alloc_text(_TEXT, CompressGetSize)
#pragma alloc_text(_TEXT, CompressEnd)
#endif

/*****************************************************************************
 * dither stuff..
 ****************************************************************************/

#include <dith775.h>

LPVOID lpDitherTable;

//////////////////////////////////////////////////////////////////////////////
//
//  Dither16InitScale()
//
//////////////////////////////////////////////////////////////////////////////

#pragma optimize("", off)
STATICFN LPVOID Dither16InitScale()
{
    LPVOID p;
    LPBYTE pbLookup;
    LPWORD pwScale;
    UINT   r,g,b;

    p = GlobalAllocPtr(GMEM_MOVEABLE|GMEM_SHARE, 32768l*2+64000);

    if (p == NULL)
        return NULL;

    pwScale  = (LPWORD)p;

    for (r=0; r<32; r++)
        for (g=0; g<32; g++)
            for (b=0; b<32; b++)
                *pwScale++ = 1600 * r + 40 * g + b;

    pbLookup = (LPBYTE)(((WORD _huge *)p) + 32768l);

    for (r=0; r<40; r++)
        for (g=0; g<40; g++)
            for (b=0; b<40; b++)
                *pbLookup++ = lookup775[35*rlevel[r] + 5*glevel[g] + blevel[b]];

    return p;
}
#pragma optimize("", on)

/*****************************************************************************
 ****************************************************************************/
#ifndef _WIN32
BOOL NEAR PASCAL VideoLoad(void)
{
    gf286 = (BOOL)(GetWinFlags() & WF_CPU286);

#ifdef DEBUG
    gf286 = GetProfileIntA("Debug", "cpu", gf286 ? 286 : 386) == 286;
#endif

    return TRUE;
}
#endif

/*****************************************************************************
 ****************************************************************************/
void NEAR PASCAL VideoFree()
{
    // CompressFrameFree();        // let compression stuff clean up...

    if (lpDitherTable != NULL) {
        GlobalFreePtr(lpDitherTable);
        lpDitherTable = NULL;
    }
}

/*****************************************************************************
 ****************************************************************************/
INSTINFO * NEAR PASCAL VideoOpen(ICOPEN FAR * icinfo)
{
    INSTINFO *  pinst;

    //
    // refuse to open if we are not being opened as a Video compressor
    //
    if (icinfo->fccType != ICTYPE_VIDEO)
        return NULL;

    pinst = (INSTINFO *)LocalAlloc(LPTR, sizeof(INSTINFO));

    if (!pinst) {
        icinfo->dwError = (DWORD)ICERR_MEMORY;
        return NULL;
    }

    //
    // init structure
    //
    pinst->dwFlags = icinfo->dwFlags;
    pinst->nCompress = 0;
    pinst->nDecompress = 0;
    pinst->nDraw = 0;

    //
    // set the default state.
    //
    SetState(pinst, NULL, 0);

    //
    // return success.
    //
    icinfo->dwError = ICERR_OK;

    return pinst;
}

/*****************************************************************************
 ****************************************************************************/
LONG NEAR PASCAL VideoClose(INSTINFO * pinst)
{
    while (pinst->nCompress > 0)
        CompressEnd(pinst);

    while (pinst->nDecompress > 0)
        DecompressEnd(pinst);

    while (pinst->nDraw > 0)
        DrawEnd(pinst);

    LocalFree((HLOCAL)pinst);

    return 1;
}

/*****************************************************************************
 ****************************************************************************/

#ifndef QueryAbout
BOOL NEAR PASCAL QueryAbout(INSTINFO * pinst)
{
    return TRUE;
}
#endif

LONG NEAR PASCAL About(INSTINFO * pinst, HWND hwnd)
{
    char achDescription[128];
    char achAbout[64];

    LoadStringA(ghModule, IDS_DESCRIPTION, achDescription, sizeof(achDescription));
    LoadStringA(ghModule, IDS_ABOUT, achAbout, sizeof(achAbout));

    MessageBoxA(hwnd,achDescription,achAbout,
    MB_OK|MB_ICONINFORMATION);
    return ICERR_OK;
}

/*****************************************************************************
 ****************************************************************************/
#ifndef QueryConfigure
BOOL NEAR PASCAL QueryConfigure(INSTINFO * pinst)
{
    return TRUE;
}
#endif

LONG NEAR PASCAL Configure(INSTINFO * pinst, HWND hwnd)
{
    return (LONG) DialogBoxParam(ghModule,TEXT("Configure"),hwnd,ConfigureDlgProc, (LONG_PTR)(UINT_PTR)pinst);
}

/*****************************************************************************
 ****************************************************************************/
LONG NEAR PASCAL GetState(INSTINFO * pinst, LPVOID pv, DWORD dwSize)
{
    if (pv == NULL || dwSize == 0)
        return sizeof(ICSTATE);

    if (dwSize < sizeof(ICSTATE))
        return 0;

    *((ICSTATE FAR *)pv) = pinst->CurrentState;

    // return number of bytes copied
    return sizeof(ICSTATE);
}

/*****************************************************************************
 ****************************************************************************/
LONG NEAR PASCAL SetState(INSTINFO * pinst, LPVOID pv, DWORD dwSize)
{
    if (pv == NULL)
        pinst->CurrentState = DefaultState;
    else if (dwSize == sizeof(ICSTATE))
        pinst->CurrentState = *((ICSTATE FAR *)pv);
    else
        return 0;

    // return number of bytes copied
    return sizeof(ICSTATE);
}

#if !defined NUMELMS
 #define NUMELMS(aa) (sizeof(aa)/sizeof((aa)[0]))
#endif

#if defined _WIN32 && !defined UNICODE

int LoadUnicodeString(HINSTANCE hinst, UINT wID, LPWSTR lpBuffer, int cchBuffer)
{
    char    ach[128];
    int     i;

    i = LoadString(hinst, wID, ach, sizeof(ach));

    if (i > 0)
        MultiByteToWideChar(CP_ACP, 0, ach, -1, lpBuffer, cchBuffer);

    return i;
}

#else
#define LoadUnicodeString   LoadString
#endif

/*****************************************************************************
 ****************************************************************************/
LONG NEAR PASCAL GetInfo(INSTINFO * pinst, ICINFO FAR *icinfo, DWORD dwSize)
{
    if (icinfo == NULL)
        return sizeof(ICINFO);

    if (dwSize < sizeof(ICINFO))
        return 0;

    icinfo->dwSize            = sizeof(ICINFO);
    icinfo->fccType           = ICTYPE_VIDEO;
    icinfo->fccHandler        = FOURCC_MSVC;
    icinfo->dwFlags           = VIDCF_QUALITY   |  // supports quality
                                VIDCF_TEMPORAL;    // supports inter-frame
    icinfo->dwVersion         = VERSION;
    icinfo->dwVersionICM      = ICVERSION;

    LoadUnicodeString(ghModule, IDS_DESCRIPTION, icinfo->szDescription, NUMELMS(icinfo->szDescription));
    LoadUnicodeString(ghModule, IDS_NAME, icinfo->szName, NUMELMS(icinfo->szName));

    return sizeof(ICINFO);
}

/*****************************************************************************
 ****************************************************************************/
LONG FAR PASCAL CompressQuery(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    if (lpbiOut)
         DPF(("CompressQuery %dx%dx%d --> %dx%dx%d'%4.4hs'", (int)lpbiIn->biWidth, (int)lpbiIn->biHeight, (int)lpbiIn->biBitCount, (int)lpbiOut->biWidth, (int)lpbiOut->biHeight, (int)lpbiOut->biBitCount, (LPSTR)&lpbiOut->biCompression));
    else
        DPF(("CompressQuery %dx%dx%d", (int)lpbiIn->biWidth, (int)lpbiIn->biHeight, (int)lpbiIn->biBitCount));

    //
    // determine if the input DIB data is in a format we like.
    //
    if (lpbiIn == NULL ||
        !(lpbiIn->biBitCount == 8  ||
          lpbiIn->biBitCount == 16 ||
          lpbiIn->biBitCount == 24 ||
          lpbiIn->biBitCount == 32) ||
        lpbiIn->biPlanes != 1 ||
        lpbiIn->biWidth < 4 ||
        lpbiIn->biHeight < 4 ||
        lpbiIn->biCompression != BI_RGB)
        return ICERR_BADFORMAT;

    //
    //  are we being asked to query just the input format?
    //
    if (lpbiOut == NULL)
        return ICERR_OK;

    //
    // make sure we can handle the format to compress to also.
    //
    if (!(lpbiOut->biCompression == FOURCC_MSVC ||  // must be 'MSVC' or 'CRAM'
          lpbiOut->biCompression == FOURCC_CRAM) ||
        !(lpbiOut->biBitCount == 16 ||              // must be 8 or 16
          lpbiOut->biBitCount == 8) ||
        (lpbiOut->biPlanes != 1) ||
        (lpbiOut->biWidth & ~3)  != (lpbiIn->biWidth & ~3)   || // must be 1:1 (no stretch)
        (lpbiOut->biHeight & ~3) != (lpbiIn->biHeight & ~3))
        return ICERR_BADFORMAT;

    return ICERR_OK;
}

/*****************************************************************************
 ****************************************************************************/
LONG FAR PASCAL CompressGetFormat(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    LONG l;

    if (l = CompressQuery(pinst, lpbiIn, NULL))
        return l;

    if (lpbiIn->biBitCount == 8)
    {
        //
        // if lpbiOut == NULL then, return the size required to hold a output
        // format
        //

        l = lpbiIn->biSize + (int)lpbiIn->biClrUsed * sizeof(RGBQUAD);

        if (lpbiOut == NULL)
            return l;

        hmemcpy(lpbiOut, lpbiIn, (int)l);

        lpbiOut->biBitCount    = 8;
    }
    else
    {
        //
        // if lpbiOut == NULL then, return the size required to hold a output
        // format
        //
        if (lpbiOut == NULL)
            return (int)lpbiIn->biSize;

        *lpbiOut = *lpbiIn;

        lpbiOut->biClrUsed     = 0;
        lpbiOut->biBitCount    = 16;
    }

    lpbiOut->biWidth       = lpbiIn->biWidth  & ~3;
    lpbiOut->biHeight      = lpbiIn->biHeight & ~3;
    lpbiOut->biCompression = FOURCC_CRAM;
    lpbiOut->biSizeImage   = CompressGetSize(pinst, lpbiIn, lpbiOut);

    return ICERR_OK;
}

/*****************************************************************************
 ****************************************************************************/

LONG FAR PASCAL CompressBegin(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    LONG l;

    if (l = CompressQuery(pinst, lpbiIn, lpbiOut))
        return l;

    DPF(("CompressBegin %dx%dx%d --> %dx%dx%d'%4.4ls'", (int)lpbiIn->biWidth, (int)lpbiIn->biHeight, (int)lpbiIn->biBitCount, (int)lpbiOut->biWidth, (int)lpbiOut->biHeight, (int)lpbiOut->biBitCount,(LPSTR)&lpbiOut->biCompression));

    //
    // initialize for compression, for real....
    //
    pinst->nCompress = 1;

    return CompressFrameBegin(lpbiIn, lpbiOut, &pinst->lpITable, pinst->rgbqOut);
}

/*****************************************************************************
 ****************************************************************************/
LONG FAR PASCAL CompressGetSize(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    int dx,dy;

    dx = (int)lpbiIn->biWidth;
    dy = (int)lpbiIn->biHeight;

    /* maximum compressed size *** your code here *** */

    if (lpbiOut->biBitCount == 8)
        // worst case size of data 10 bytes per 16 pixels (8 colors + mask)
        // remember the EOF code!
        return ((DWORD)dx * (DWORD)dy * 10l) / 16l + 2l;
    else
        // worst case size of data 18 bytes per 16 pixels (8 colors + mask)
        // remember the EOF code!
        return ((DWORD)dx * (DWORD)dy * 10l) / 8l + 2l; // 10/8 ~= 18/16
////////return ((DWORD)dx * (DWORD)dy * 18l) / 16l + 2l;
}

/*****************************************************************************
 ****************************************************************************/
LONG FAR PASCAL Compress(INSTINFO * pinst, ICCOMPRESS FAR *icinfo, DWORD dwSize)
{
    LONG l;
    LPBITMAPINFOHEADER lpbiIn  = icinfo->lpbiInput;
    LPBITMAPINFOHEADER lpbiOut = icinfo->lpbiOutput;
    DWORD threshold;
    DWORD thresholdPrev;
    DWORD dwQualityPrev;
    DWORD dwQuality;
    BOOL  fBegin;
    LPVOID smag;
    PCELLS compressTemp;

    if (l = CompressQuery(pinst, icinfo->lpbiInput, icinfo->lpbiOutput))
        return l;

    //
    // check for being called without a BEGIN message, and do the begin for
    // the caller
    //
    if (fBegin = (pinst->nCompress == 0))
    {
        if (l = CompressBegin(pinst, icinfo->lpbiInput, icinfo->lpbiOutput))
            return l;
    }

    smag = _FPInit();

    DPF(("Compress %dx%dx%d --> %dx%dx%d'%4.4ls'", (int)lpbiIn->biWidth, (int)lpbiIn->biHeight, (int)lpbiIn->biBitCount, (int)lpbiOut->biWidth, (int)lpbiOut->biHeight, (int)lpbiOut->biBitCount, (LPSTR)&lpbiOut->biCompression));

    if (icinfo->dwQuality == ICQUALITY_DEFAULT)
        dwQuality = QUALITY_DEFAULT;
    else
        dwQuality = ICQUALITY_HIGH - icinfo->dwQuality;

    dwQualityPrev = MulDiv((UINT)dwQuality,100,pinst->CurrentState.wTemporalRatio);

    threshold     = QualityToThreshold(dwQuality);
    thresholdPrev = QualityToThreshold(dwQualityPrev);

    if (pinst->Status)
        pinst->Status(pinst->lParam, ICSTATUS_START, 0);

    // For Win16, this needs to be in the data segment so we
    // can use a near pointer to it.
    compressTemp = (PCELLS) LocalAlloc(LPTR, sizeof(CELLS));

    if (!compressTemp)
        return ICERR_MEMORY;

    if (lpbiOut->biBitCount == 8)
        l = CompressFrame8(
                  icinfo->lpbiInput,        // DIB header to compress
                  icinfo->lpInput,          // DIB bits to compress
                  icinfo->lpOutput,         // put compressed data here
                  threshold,                // edge threshold
                  thresholdPrev,            // inter-frame threshold
                  icinfo->lpbiPrev,         // previous frame
                  icinfo->lpPrev,           // previous frame
                  pinst->Status,            // status callback
                  pinst->lParam,
                  compressTemp,
                  pinst->lpITable,
                  pinst->rgbqOut);
    else
        l = CompressFrame16(
                  icinfo->lpbiInput,        // DIB header to compress
                  icinfo->lpInput,          // DIB bits to compress
                  icinfo->lpOutput,         // put compressed data here
                  threshold,                // edge threshold
                  thresholdPrev,            // inter-frame threshold
                  icinfo->lpbiPrev,         // previous frame
                  icinfo->lpPrev,           // previous frame
                  pinst->Status,            // status callback
                  pinst->lParam,
                  compressTemp);

    LocalFree((HLOCAL) compressTemp);

    if (pinst->Status)
        pinst->Status(pinst->lParam, ICSTATUS_END, 0);

    _FPTerm(smag);

    if (l == -1)
        return ICERR_ERROR;

    lpbiOut->biWidth       = lpbiIn->biWidth  & ~3;
    lpbiOut->biHeight      = lpbiIn->biHeight & ~3;
    lpbiOut->biCompression = FOURCC_CRAM;
    lpbiOut->biSizeImage   = l;
////lpbiOut->biBitCount    = 16;

    //
    // return the chunk id
    //
    if (icinfo->lpckid)
        *icinfo->lpckid = TWOCC_XX;

    //
    // set the AVI index flags,
    //
    //    make it a keyframe?
    //
    if (icinfo->lpdwFlags) {
        *icinfo->lpdwFlags = AVIIF_TWOCC;

        if (icinfo->lpbiPrev == NULL || numberOfSkips == 0)
            *icinfo->lpdwFlags |= AVIIF_KEYFRAME;
    }

    if (fBegin)
        CompressEnd(pinst);

    return ICERR_OK;
}

/*****************************************************************************
 ****************************************************************************/
LONG FAR PASCAL CompressEnd(INSTINFO * pinst)
{
    if (pinst->nCompress == 0)
        return ICERR_ERROR;

    pinst->nCompress = 0;

    return CompressFrameEnd(&pinst->lpITable);
}

/*****************************************************************************
 *
 *  decompress tables
 *
 *      indexed by:
 *          SRC:     0=8 bit Cram 1=16 bit Cram
 *          STRETCH: 0=1:1, 1=1:2
 *          DST:     0=8, 1=16, 2=24, 3=32
 *
 ****************************************************************************/

#ifdef _WIN32

DECOMPPROC  DecompressWin32[2][2][5] = {
        DecompressFrame8,               // Cram8  1:1 to 8
        NULL,                           // Cram8  1:1 to 16 (555)
        NULL,                           // Cram8  1:1 to 24
        NULL,                           // Cram8  1:1 to 32
        NULL,                           // Cram8  1:1 to 565

        DecompressFrame8X2C,            // Cram8  1:2 to 8
        NULL,                           // Cram8  1:2 to 16 (555)
        NULL,                           // Cram8  1:2 to 24
        NULL,                           // Cram8  1:2 to 32
        NULL,                           // Cram8  1:2 to 565

        DecompressFrame16To8C,          // Cram16 1:1 to 8
        DecompressFrame16To555C,        // Cram16 1:1 to 16 (555)
        DecompressFrame24,              // Cram16 1:1 to 24
        NULL,                           // Cram16 1:1 to 32
        DecompressFrame16To565C,        // Cram16 1:1 to 565

        DecompressFrame16To8X2C,        // Cram16 1:2 to 8
        NULL,                           // Cram16 1:2 to 16 (555)
        NULL,                           // Cram16 1:2 to 24
        NULL,                           // Cram16 1:2 to 32
        NULL};                          // Cram16 1:2 to 565

#else

DECOMPPROC  Decompress386[2][2][5] = {
        DecompressCram8,                // Cram8  1:1 to 8
        NULL,                           // Cram8  1:1 to 16 (555)
        NULL,                           // Cram8  1:1 to 24
        NULL,                           // Cram8  1:1 to 32
        NULL,                           // Cram8  1:1 to 565

        DecompressCram8x2,              // Cram8  1:2 to 8
        NULL,                           // Cram8  1:2 to 16 (555)
        NULL,                           // Cram8  1:2 to 24
        NULL,                           // Cram8  1:2 to 32
        NULL,                           // Cram8  1:2 to 565

        DecompressCram168,              // Cram16 1:1 to 8
        DecompressCram16,               // Cram16 1:1 to 16 (555)
        NULL,                           // Cram16 1:1 to 24
        NULL /* DecompressCram32 */,    // Cram16 1:1 to 32
        NULL /* DecompressFrame16To565C */,     // Cram16 1:1 to 565

        NULL,                           // Cram16 1:2 to 8
        DecompressCram16x2,             // Cram16 1:2 to 16 (555)
        NULL,                           // Cram16 1:2 to 24
        NULL,                           // Cram16 1:2 to 32
        NULL};                          // Cram16 1:2 to 565


DECOMPPROC  Decompress286[2][2][5] = {
        DecompressCram8_286,            // Cram8  1:1 to 8
        NULL,                           // Cram8  1:1 to 16 (555)
        NULL,                           // Cram8  1:1 to 24
        NULL,                           // Cram8  1:1 to 32
        NULL,                           // Cram8  1:1 to 565

        NULL,                           // Cram8  1:2 to 8
        NULL,                           // Cram8  1:2 to 16 (555)
        NULL,                           // Cram8  1:2 to 24
        NULL,                           // Cram8  1:2 to 32
        NULL,                           // Cram8  1:2 to 565

        NULL,                           // Cram16 1:1 to 8
        DecompressCram16_286,           // Cram16 1:1 to 16 (555)
        NULL,                           // Cram16 1:1 to 24
        NULL,                           // Cram16 1:1 to 32
        NULL,                           // Cram16 1:1 to 565

        NULL,                           // Cram16 1:2 to 8
        NULL,                           // Cram16 1:2 to 16 (555)
        NULL,                           // Cram16 1:2 to 24
        NULL,                           // Cram16 1:2 to 32
        NULL};                          // Cram16 1:2 to 565
#endif

/*****************************************************************************
 ****************************************************************************/
LONG NEAR PASCAL DecompressQueryFmt(
    INSTINFO * pinst,
    LPBITMAPINFOHEADER lpbiSrc)
{
    //
    // determine if the input DIB data is in a format we like.
    //
    if (lpbiSrc == NULL ||
        !(lpbiSrc->biBitCount == 16 || lpbiSrc->biBitCount == 8) ||
        (lpbiSrc->biPlanes != 1) ||
        !(lpbiSrc->biCompression == FOURCC_MSVC ||
          lpbiSrc->biCompression == FOURCC_CRAM))
        return ICERR_BADFORMAT;

    return ICERR_OK;
}

/*****************************************************************************
 ****************************************************************************/
LONG NEAR PASCAL DecompressQuery(
    INSTINFO * pinst,
    DWORD dwFlags,
    LPBITMAPINFOHEADER lpbiSrc,
    LPVOID pSrc,
    int xSrc,
    int ySrc,
    int dxSrc,
    int dySrc,
    LPBITMAPINFOHEADER lpbiDst,
    LPVOID pDst,
    int xDst,
    int yDst,
    int dxDst,
    int dyDst)
{
#ifndef _WIN32
    DWORD biSizeImage;
#endif
    DECOMPPROC fn;
    int s,d,n;

    //
    // determine if the input DIB data is in a format we like.
    //
    if (DecompressQueryFmt(pinst, lpbiSrc))
        return ICERR_BADFORMAT;

    //
    // allow (-1) as a default width/height
    //
    if (dxSrc == -1)
        dxSrc = (int)lpbiSrc->biWidth;

    if (dySrc == -1)
        dySrc = (int)lpbiSrc->biHeight;

    //
    //  we cant clip the source.
    //
    if (xSrc != 0 || ySrc != 0)
        return ICERR_BADPARAM;

    if ((dxSrc != (int)lpbiSrc->biWidth) || (dySrc != (int)lpbiSrc->biHeight))
        return ICERR_BADPARAM;

    //
    //  are we being asked to query just the input format?
    //
    if (lpbiDst == NULL)
        return ICERR_OK;

    //
    // allow (-1) as a default width/height
    //
    if (dxDst == -1)
        dxDst = (int)lpbiDst->biWidth;

    if (dyDst == -1)
        dyDst = abs((int)lpbiDst->biHeight);

#ifndef _WIN32
    if (gf286)
        biSizeImage = (DWORD)(UINT)abs((int)lpbiDst->biHeight)*(DWORD)(WORD)DIBWIDTHBYTES(*lpbiDst);
#endif

    s = lpbiSrc->biBitCount/8-1;    //  s = 0,1

#ifdef _WIN32
    // Can't support 16:32 access in our C version, of course....
    if (lpbiDst->biCompression == BI_1632) {
        return ICERR_BADFORMAT;
    }
#endif

    if (lpbiDst->biBitCount != 8 && lpbiDst->biBitCount != 16 && lpbiDst->biBitCount != 24 && lpbiDst->biBitCount != 32)
    {
        return ICERR_BADFORMAT;
    }

    // must be full dib or a '1632' DIB
    if (lpbiDst->biCompression != BI_RGB &&
        lpbiDst->biCompression != BI_1632) {
	    if (lpbiDst->biCompression != BI_BITFIELDS) {
		DPF(("MSVIDC asked to decompress to '%.4hs'!", &lpbiDst->biCompression));
		return ICERR_BADFORMAT;
	    }
	    
            // allow 565 dibs
            if ((lpbiDst->biBitCount == 16) &&
                (((LPDWORD)(lpbiDst+1))[0] == 0x00f800) &&
                (((LPDWORD)(lpbiDst+1))[1] == 0x0007e0) &&
                (((LPDWORD)(lpbiDst+1))[2] == 0x00001f) ) {

                    // ok - its 565 format
                    d = 4;
            } else {
                DPF(("Bad bitmask (%lX %lX %lX) in %d-bit BI_BITMAP case!",
                     lpbiDst->biBitCount,
                     ((LPDWORD)(lpbiDst+1))[0],
                     ((LPDWORD)(lpbiDst+1))[1],
                     ((LPDWORD)(lpbiDst+1))[2]));
                return ICERR_BADFORMAT;
            }
    } else {
        d = lpbiDst->biBitCount/8-1;    //  d = 0,1,2,3

        if (lpbiDst->biCompression == BI_1632 && lpbiDst->biBitCount == 16) {

            if ((((LPDWORD)(lpbiDst+1))[0] == 0x007400) &&
                (((LPDWORD)(lpbiDst+1))[1] == 0x0003f0) &&
                (((LPDWORD)(lpbiDst+1))[2] == 0x00000f) ) {
                    // ok - it's 555 format
            } else if ((((LPDWORD)(lpbiDst+1))[0] == 0x00f800) &&
                (((LPDWORD)(lpbiDst+1))[1] == 0x0007e0) &&
                (((LPDWORD)(lpbiDst+1))[2] == 0x00001f) ) {

                    // ok - it's 565 format
                    d = 4;
            } else {
                DPF(("Bad bitmask (%lX %lX %lX) in 16-bit BI_1632 case!",
                     ((LPDWORD)(lpbiDst+1))[0],
                     ((LPDWORD)(lpbiDst+1))[1],
                     ((LPDWORD)(lpbiDst+1))[2]));
                return ICERR_BADFORMAT;
            }
        }

        // What about 24-bit BI_1632?  Should we check the masks?
    }

    //
    //  n = 0 for 1:1, 1 for 1:2
    //
    if (dxDst  == dxSrc && dyDst == dySrc)
        n = 0;
    else if (dxDst == dxSrc*2 && dyDst == dySrc*2)
        n = 1;
    else
        return ICERR_BADSIZE;

#ifdef DEBUG
    DPF(("DecompressQuery %dx%dx%d [%d,%d,%d,%d] --> %dx%dx%d (565) [%d,%d,%d,%d]",
        (int)lpbiSrc->biWidth, (int)lpbiSrc->biHeight, (int)lpbiSrc->biBitCount,
        xSrc, ySrc, dxSrc, dySrc,
        (int)lpbiDst->biWidth, (int)lpbiDst->biHeight, (int)lpbiDst->biBitCount,
        d == 4 ? "(565)" : "",
        xDst, yDst, dxDst, dyDst));

#endif

#ifdef _WIN32
    fn = DecompressWin32[s][n][d];
#else
    if (gf286)
    {
        fn = Decompress286[s][n][d];

        if (fn && biSizeImage > 64l*1024)
            fn = fn==DecompressCram8_286 ? DecompressFrame8 : NULL;
    }
    else
    {
        fn = Decompress386[s][n][d];
    }
#endif

    if (fn == NULL)
        return ICERR_BADFORMAT;

    pinst->DecompressTest = fn;     // return this to DecompressBegin.

    return ICERR_OK;
}

/*****************************************************************************
 ****************************************************************************/
LONG NEAR PASCAL DecompressGetFormat(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    LONG l;
    int dx,dy;

    if (l = DecompressQueryFmt(pinst, lpbiIn))
        return l;

    //
    // if lpbiOut == NULL then, return the size required to hold a output
    // format
    //
    if (lpbiOut == NULL)
        return (int)lpbiIn->biSize + (int)lpbiIn->biClrUsed * sizeof(RGBQUAD);

    hmemcpy(lpbiOut, lpbiIn,
        (int)lpbiIn->biSize + (int)lpbiIn->biClrUsed * sizeof(RGBQUAD));

    dx = (int)lpbiIn->biWidth & ~3;
    dy = (int)lpbiIn->biHeight & ~3;

    lpbiOut->biWidth       = dx;
    lpbiOut->biHeight      = dy;
    lpbiOut->biBitCount    = lpbiIn->biBitCount;    // convert 8->8 16->16
    lpbiOut->biPlanes      = 1;

    lpbiOut->biCompression = BI_RGB;
    lpbiOut->biSizeImage   = (DWORD)(WORD)abs(dy)*(DWORD)(WORD)DIBWIDTHBYTES(*lpbiOut);

    return ICERR_OK;
}

/*****************************************************************************
 ****************************************************************************/
LONG NEAR PASCAL DecompressBegin(
    INSTINFO * pinst,
    DWORD dwFlags,
    LPBITMAPINFOHEADER lpbiSrc,
    LPVOID pSrc,
    int xSrc,
    int ySrc,
    int dxSrc,
    int dySrc,
    LPBITMAPINFOHEADER lpbiDst,
    LPVOID pDst,
    int xDst,
    int yDst,
    int dxDst,
    int dyDst)
{
    LONG l;

    if (l = DecompressQuery(pinst, dwFlags, lpbiSrc, pSrc, xSrc, ySrc, dxSrc, dySrc, lpbiDst, pDst, xDst, yDst, dxDst, dyDst))
        return l;

    pinst->DecompressProc = pinst->DecompressTest;

    //
    //  make sure biSizeImage is set, the decompress code needs it to be
    //
    if (lpbiDst->biSizeImage == 0)
        lpbiDst->biSizeImage = (DWORD)(WORD)abs((int)lpbiDst->biHeight)*(DWORD)(WORD)DIBWIDTHBYTES(*lpbiDst);

    //
    // init the dither tables !!! call MSVIDEO, dont have code here!!!
    //
    if (lpbiSrc->biBitCount == 16 &&
        lpbiDst->biBitCount == 8)
    {
        if (lpDitherTable == NULL)
            lpDitherTable = Dither16InitScale();

        if (lpDitherTable == NULL)
            return ICERR_MEMORY;
    }

    pinst->nDecompress = 1;

    return ICERR_OK;
}

/*****************************************************************************
 *
 *  Decompress
 *
 *  we can assume certain things here because DecompressQuery() only lets
 *  valid stuff in.
 *
 *      the source rect is always the entire source.
 *      the dest rect is either 1:1 or 1:2
 *
 ****************************************************************************/
LONG NEAR PASCAL Decompress(
    INSTINFO * pinst,
    DWORD dwFlags,
    LPBITMAPINFOHEADER lpbiSrc,
    LPVOID pSrc,
    int xSrc,
    int ySrc,
    int dxSrc,
    int dySrc,
    LPBITMAPINFOHEADER lpbiDst,
    LPVOID pDst,
    int xDst,
    int yDst,
    int dxDst,
    int dyDst)
{
    //
    //  if we are called without a begin do the begin now, but dont make
    //  the begin "stick"
    //
    if (pinst->nDecompress == 0)
    {
        LONG err;

        if (err = DecompressBegin(pinst, dwFlags, lpbiSrc, pSrc, xSrc, ySrc, dxSrc, dySrc, lpbiDst, pDst, xDst, yDst, dxDst, dyDst))
            return err;

        pinst->nDecompress = 0;
    }

#ifdef DEBUG
    if (lpbiDst->biSizeImage == 0)
        DebugBreak();

    if (pinst->DecompressProc == NULL)
        DebugBreak();
#endif

    (*pinst->DecompressProc)(lpbiSrc,pSrc,lpbiDst,pDst,xDst,yDst);

    return ICERR_OK;
}

/*****************************************************************************
 *
 * DecompressGetPalette() implements ICM_GET_PALETTE
 *
 * This function has no Compress...() equivalent
 *
 * It is used to pull the palette from a frame in order to possibly do
 * a palette change.
 *
 ****************************************************************************/
LONG NEAR PASCAL DecompressGetPalette(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    LONG l;
    int i;
    RGBQUAD FAR * prgb;

    DPF(("DecompressGetPalette()"));

    if (l = DecompressQueryFmt(pinst, lpbiIn))
        return l;

    if (lpbiOut->biBitCount != 8)
        return ICERR_BADFORMAT;

    //
    // if you decompress full-color to 8 bit you need to put the "dither"
    // palette in lpbiOut
    //
    if (lpbiIn->biBitCount != 8)
    {
        lpbiOut->biClrUsed = 256;

        prgb = (LPVOID)(lpbiOut + 1);

        for (i=0; i<256; i++)
        {
            prgb[i].rgbRed      = dpal775[i][0];
            prgb[i].rgbGreen    = dpal775[i][1];
            prgb[i].rgbBlue     = dpal775[i][2];
            prgb[i].rgbReserved = 0;
        }

        return ICERR_OK;
    }

    if (lpbiIn->biClrUsed == 0)
        lpbiIn->biClrUsed = 256;

    //
    // return the 8bit palette used for decompression.
    //
    hmemcpy(
        (LPBYTE)lpbiOut + (int)lpbiOut->biSize,
        (LPBYTE)lpbiIn + (int)lpbiIn->biSize,
        (int)lpbiIn->biClrUsed * sizeof(RGBQUAD));

    lpbiOut->biClrUsed = lpbiIn->biClrUsed;

    return ICERR_OK;
}

/*****************************************************************************
 ****************************************************************************/
LONG NEAR PASCAL DecompressEnd(INSTINFO * pinst)
{
    if (pinst->nDecompress == 0)
        return ICERR_ERROR;

    pinst->nDecompress = 0;
    return ICERR_OK;
}

/*****************************************************************************
 ****************************************************************************/
LONG NEAR PASCAL DrawBegin(INSTINFO * pinst,ICDRAWBEGIN FAR *icinfo, DWORD dwSize)
{
    return ICERR_UNSUPPORTED;
}

/*****************************************************************************
 ****************************************************************************/
LONG NEAR PASCAL Draw(INSTINFO * pinst, ICDRAW FAR *icinfo, DWORD dwSize)
{
    return ICERR_UNSUPPORTED;
}

/*****************************************************************************
 ****************************************************************************/
LONG NEAR PASCAL DrawEnd(INSTINFO * pinst)
{
    return ICERR_UNSUPPORTED;
}

/*****************************************************************************
 ****************************************************************************/

INT_PTR FAR PASCAL _LOADDS ConfigureDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int     i;
    HWND    hsb;
    TCHAR   ach[10];
    static  TCHAR chDecimal = TEXT('.');

    static  INSTINFO *pinst;

    #define SCROLL_MIN  1       // 0.00
    #define SCROLL_MAX  100     // 1.00

    switch (msg)
    {
        case WM_COMMAND:
            switch (wParam)
            {
                case IDOK:
                    hsb = GetDlgItem(hdlg,ID_SCROLL);
                    pinst->CurrentState.wTemporalRatio = GetScrollPos(hsb,SB_CTL);
                    EndDialog(hdlg,TRUE);
                    break;

                case IDCANCEL:
                    EndDialog(hdlg,FALSE);
                    break;
            }
            break;

        case WM_HSCROLL:
            hsb = GET_WM_HSCROLL_HWND(wParam, lParam);

            i = GetScrollPos(hsb,SB_CTL);

            switch (GET_WM_HSCROLL_CODE(wParam, lParam))
            {
                case SB_LINEDOWN:      i += 1; break;
                case SB_LINEUP:        i -= 1; break;
                case SB_PAGEDOWN:      i += 10; break;
                case SB_PAGEUP:        i -= 10; break;

                case SB_THUMBTRACK:
                case SB_THUMBPOSITION:
                    i = (int)GET_WM_HSCROLL_POS(wParam, lParam);
                    break;

                default:
                    return TRUE;
            }

            i = max(SCROLL_MIN,min(SCROLL_MAX,i));
            SetScrollPos(hsb,SB_CTL,i,TRUE);
            wsprintf(ach, TEXT("%d%c%02d"), i/100, chDecimal, i%100);
            SetDlgItemText(hdlg,ID_TEXT,ach);
            return TRUE;

        case WM_INITDIALOG:
            pinst = (INSTINFO *)lParam;

            ach[0] = chDecimal;
            ach[1] = 0;
            GetProfileString(TEXT("intl"), TEXT("sDecimal"), ach, ach, sizeof(ach));
            chDecimal = ach[0];

            hsb = GetDlgItem(hdlg,ID_SCROLL);
            i = pinst->CurrentState.wTemporalRatio;

            SetScrollRange(hsb,SB_CTL,SCROLL_MIN, SCROLL_MAX, TRUE);
            SetScrollPos(hsb,SB_CTL,i,TRUE);
            wsprintf(ach, TEXT("%d%c%02d"), i/100, chDecimal, i%100);
            SetDlgItemText(hdlg,ID_TEXT,ach);
            return TRUE;
    }
    return FALSE;
}

/*****************************************************************************
 ****************************************************************************/

#ifdef DEBUG

#define _WINDLL
#include <stdarg.h>
#ifdef _WIN32
#define GetProfileIntA  mmGetProfileIntA
#endif

void FAR CDECL dprintf(LPSTR szFormat, ...)
{
    char ach[128];
    static BOOL fDebug = -1;

    va_list va;
    va_start(va, szFormat);

    if (fDebug == -1)
        fDebug = GetProfileIntA("Debug", "MSVIDC", FALSE);

    if (!fDebug)
        return;

#ifdef _WIN32
    wsprintfA(ach, "MSVIDC32: (tid %x) ", GetCurrentThreadId());
    wvsprintfA(ach+strlen(ach),szFormat,va);
#else
    lstrcpyA(ach, "MSVIDC: ");
    wvsprintfA(ach+8,szFormat,va);
#endif
    lstrcatA(ach, "\r\n");

    OutputDebugStringA(ach);
    va_end(va);
}

#endif
