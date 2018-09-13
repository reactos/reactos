/****************************************************************************
 *
 *   capdib.c
 *
 *   DIB processing module.
 *
 *   Microsoft Video for Windows Sample Capture Class
 *
 *   Copyright (c) 1992, 1993 Microsoft Corporation.  All Rights Reserved.
 *
 *    You have a royalty-free right to use, modify, reproduce and
 *    distribute the Sample Files (and/or any modified version) in
 *    any way you find useful, provided that you agree that
 *    Microsoft has no warranty obligations or liability for any
 *    Sample Application Files which are modified.
 *
 ***************************************************************************/

#define INC_OLE2
#pragma warning(disable:4103)
#include <windows.h>
#include <windowsx.h>
#include <win32.h>
#include <mmsystem.h>
#include <msvideo.h>
#include <drawdib.h>

#include "ivideo32.h"
#include "avicap.h"
#include "avicapi.h"


//
// Initialize a DIB to the default format of 160x120x8, BI_RGB
//
void SetDefaultCaptureFormat (LPBITMAPINFOHEADER lpbih)
{
    lpbih->biSize              = sizeof (BITMAPINFOHEADER);
    lpbih->biWidth             = 160;
    lpbih->biHeight            = 120;
    lpbih->biBitCount          = 8;
    lpbih->biPlanes            = 1;
    lpbih->biCompression       = BI_RGB;
    lpbih->biSizeImage         = DIBWIDTHBYTES (*lpbih) * lpbih->biHeight;
    lpbih->biXPelsPerMeter     = 0;
    lpbih->biYPelsPerMeter     = 0;
    lpbih->biClrUsed           = 256;
    lpbih->biClrImportant      = 0;
}

//
// Whenever we get a new format from the driver, OR
// start using a new palette, we must reallocate
// our global BITMAPINFOHEADER.  This allows JPEG
// quantization tables to be tacked onto the BITMAPINFO
// or any other format specific stuff.  The color table
// is always offset biSize from the start of the BITMAPINFO.
// Returns: 0 on success, or DV_ERR_... code
//

DWORD AllocNewGlobalBitmapInfo (LPCAPSTREAM lpcs, LPBITMAPINFOHEADER lpbi)
{
    DWORD dwSize;

    dwSize = lpbi->biSize + 256 * sizeof (RGBQUAD);

    // The 256 entry above is HARDWIRED ON PURPOSE
    // If biClrUsed was used instead, we would have to realloc
    // whenever a palette is pasted (during DibNewPalette())!!!

    if (lpcs->lpBitsInfo)
         lpcs->lpBitsInfo = (LPBITMAPINFO) GlobalReAllocPtr (lpcs->lpBitsInfo,
                dwSize, GHND);
    else
         lpcs->lpBitsInfo = (LPBITMAPINFO) GlobalAllocPtr (GHND, dwSize);

    if (!lpcs->lpBitsInfo)
         return (DV_ERR_NOMEM);

    // Copy over the BITMAPINFOHEADER
    CopyMemory (lpcs->lpBitsInfo, lpbi, lpbi->biSize);

    return DV_ERR_OK;
}

//
// Whenever we get a new format from the driver
// allocate a new global bitspace.  This bitspace is used
// in preview mode and single frame capture.
// Returns: 0 on success, or DV_ERR_... code
//

DWORD AllocNewBitSpace (LPCAPSTREAM lpcs, LPBITMAPINFOHEADER lpbih)
{
    DWORD dwSize;

    // Allow room for a RIFF chunk prepended to the actual image,
    // and a junk chunk on the tail end

#define  RESERVE_FOR_RIFF  (512+sizeof(RIFF))

    dwSize = lpbih->biSizeImage + RESERVE_FOR_RIFF;

    if (lpcs->lpBitsUnaligned) {
#ifdef CHICAGO
         vidxFreePreviewBuffer (lpcs->hVideoIn, 
                lpcs->lpBitsUnaligned);
#else
         FreeSectorAlignedMem (lpcs->lpBitsUnaligned);
#endif
         lpcs->lpBitsUnaligned = NULL;
         lpcs->lpBits = NULL;
    }

#ifdef CHICAGO
    if (MMSYSERR_NOERROR == vidxAllocPreviewBuffer (
                lpcs->hVideoIn,
                &lpcs->lpBitsUnaligned,
                dwSize)) {
        lpcs->lpBits = (LPBYTE) (ROUNDUPTOSECTORSIZE(lpcs->lpBitsUnaligned, 512)
                        + sizeof(RIFF));
    }

#else
    if (lpcs->lpBitsUnaligned = ((LPBYTE)AllocSectorAlignedMem (dwSize, 512))) {
        lpcs->lpBits = (LPBYTE) (ROUNDUPTOSECTORSIZE(lpcs->lpBitsUnaligned, 512)
                        + sizeof(RIFF));
    }
#endif

    if (!lpcs->lpBits)
         return (DV_ERR_NOMEM);

    return DV_ERR_OK;
}

//
// Dib Initialization code
// Returns: 0 on success, or DV_ERR_... code
//

DWORD DibInit (LPCAPSTREAM lpcs)
{
    BITMAPINFOHEADER bmih;

    SetDefaultCaptureFormat (&bmih);
    return ((WORD) AllocNewGlobalBitmapInfo (lpcs, &bmih));
}

//
// Fini code to free all bitmap resources
//
void DibFini (LPCAPSTREAM lpcs)
{
    if (lpcs->lpBits) {
#ifdef CHICAGO
        vidxFreePreviewBuffer (lpcs->hVideoIn, lpcs->lpBitsUnaligned);
#else
        FreeSectorAlignedMem (lpcs->lpBitsUnaligned);
#endif
        lpcs->lpBits = NULL;
        lpcs->lpBitsUnaligned = NULL;
    }
    if (lpcs->lpBitsInfo) {
        GlobalFreePtr (lpcs->lpBitsInfo);
        lpcs->lpBitsInfo = NULL;
    }
    lpcs->dxBits = 0;
    lpcs->dyBits = 0;
}

//
// Send a format to the driver.
// Whenever we do a format change, send the driver the
// Source and destination rects.
// Returns: 0 on success, or DV_ERR_... code
//
DWORD SendDriverFormat (LPCAPSTREAM lpcs, LPBITMAPINFOHEADER lpbih, DWORD dwInfoHeaderSize)
{
    RECT rc;
    DWORD dwError = DV_ERR_NOTSUPPORTED;

    rc.left = rc.top = 0;
    rc.right = (int) lpbih->biWidth;
    rc.bottom = (int) lpbih->biHeight;

    if (dwError = videoConfigure(lpcs->hVideoIn,
            DVM_FORMAT,
            VIDEO_CONFIGURE_SET, NULL,
            (LPBITMAPINFOHEADER)lpbih, dwInfoHeaderSize,
            NULL, 0 ) ) {
        return dwError;
    } else {
        videoSetRect (lpcs->hVideoCapture, DVM_DST_RECT, rc);
        videoSetRect (lpcs->hVideoIn, DVM_SRC_RECT, rc);
        videoSetRect (lpcs->hVideoIn, DVM_DST_RECT, rc);
    }
    return dwError;
}


//
// Given a DIB, see if the driver likes it, then
//  allocate the global BITMAPINFOHEADER and bitspace.
//
//
DWORD SetFormatFromDIB (LPCAPSTREAM lpcs, LPBITMAPINFOHEADER lpbih)
{
    DWORD dwError;

    // Fill optional fields in the DIB header
    if (lpbih->biSizeImage == 0)
        lpbih->biSizeImage = DIBWIDTHBYTES (*lpbih) * lpbih->biHeight;

    // Is the format palatized or full-color
    if (lpbih->biBitCount <= 8 && lpbih->biClrUsed == 0)
        lpbih->biClrUsed = (1 << lpbih->biBitCount);     // paletized

    // See if the driver will support it
    if (dwError = SendDriverFormat (lpcs, lpbih, lpbih->biSize) )
        return dwError;

    // Realloc our global header
    if (dwError = AllocNewGlobalBitmapInfo (lpcs, lpbih))
        return dwError;

    // Realloc the bits
    if (dwError = AllocNewBitSpace (lpcs, lpbih))
        return dwError;

    lpcs->dxBits = (int)lpbih->biWidth;
    lpcs->dyBits = (int)lpbih->biHeight;

    lpcs->VidHdr.lpData = lpcs->lpBits;
    lpcs->VidHdr.dwBufferLength = lpbih->biSizeImage;
    lpcs->VidHdr.dwUser = 0;
    lpcs->VidHdr.dwFlags = 0;

    return (DV_ERR_OK);
}


//
// Returns: a LPBITMAPINFO allocated from global memory
//      containing the current format, or NULL on error.
//      Note that this structure can be larger than
//      sizeof (BITMAPINFO), ie. JPEG !!!
//

LPBITMAPINFO DibGetCurrentFormat (LPCAPSTREAM lpcs)
{
    DWORD               dwError;
    DWORD               dwSize = 0;
    LPBITMAPINFO        lpBInfo = NULL;

    if (!lpcs->fHardwareConnected)
        return NULL;

    // How large is the BITMAPINFOHEADER?
    videoConfigure( lpcs->hVideoIn,
            DVM_FORMAT,
             VIDEO_CONFIGURE_GET | VIDEO_CONFIGURE_QUERYSIZE,
             &dwSize, 0, 0, NULL, 0);

    if (!dwSize)
        dwSize = sizeof (BITMAPINFOHEADER);

    if (!(lpBInfo = (LPBITMAPINFO) GlobalAllocPtr (GMEM_MOVEABLE, dwSize)))
         return (NULL);

    if (dwError = videoConfigure( lpcs->hVideoIn,
            DVM_FORMAT,
             VIDEO_CONFIGURE_GET | VIDEO_CONFIGURE_CURRENT, NULL,
             (LPBITMAPINFOHEADER) lpBInfo, dwSize,
             NULL, 0 ) ) {
        // very bad. the driver can't tell us its format. we're hosed.
        GlobalFreePtr (lpBInfo);
        return NULL;
     }

    return (lpBInfo);
}

//
// Main entry point when changing capture formats.
// This is called when the user closes the drivers format dialog.
// Returns: 0 on success, or DV_ERR_... code
//
DWORD DibGetNewFormatFromDriver (LPCAPSTREAM lpcs)
{
    BOOL                f;
    BITMAPINFOHEADER    bih;
    DWORD               dwError;
    LPBITMAPINFO        lpBInfo;

    if (!lpcs->fHardwareConnected)
        return DV_ERR_OK;       // Return OK if no hardware exists

    lpBInfo = DibGetCurrentFormat (lpcs);

    if (lpBInfo == NULL)
        return DV_ERR_NOTSUPPORTED;

    // Set our internal state
    if (dwError = SetFormatFromDIB (lpcs, (LPBITMAPINFOHEADER) lpBInfo)) {
        // Holy shit, couldn't change formats, time to punt!
        // Try to switch back to minimal format (120x160x8)

        errorDriverID (lpcs, dwError);

        SetDefaultCaptureFormat (&bih);
        dwError = SetFormatFromDIB (lpcs, &bih);
    }

    // Force a new frame to be taken, so the DIB contains good
    // data.  Especially important to prevent codecs from exploding!
    if (!dwError)
        videoFrame (lpcs->hVideoIn, &lpcs->VidHdr);

    if (lpBInfo)
        GlobalFreePtr (lpBInfo);

    f = DrawDibBegin(lpcs->hdd,NULL,-1,-1,(LPBITMAPINFOHEADER)(lpcs->lpBitsInfo),-1,-1,0);
    if (!f) {
        errorUpdateError(lpcs, IDS_CAP_AVI_DRAWDIB_ERROR);
    }

    return (dwError);
}

//
// Main entry point when changing capture formats via App message.
// Returns: TRUE on success, or FALSE if format not supported
//
BOOL DibNewFormatFromApp (LPCAPSTREAM lpcs, LPBITMAPINFO lpbiNew, UINT dwSize)
{
    BOOL                f;
    DWORD               dwError;
    LPBITMAPINFO        lpBInfo;

    if (!lpcs->fHardwareConnected)
        return FALSE;

    lpBInfo = DibGetCurrentFormat (lpcs);  // Allocs memory!!!

    if (lpBInfo == NULL)
        return FALSE;

    // Set our internal state
    if (dwError = SetFormatFromDIB (lpcs, (LPBITMAPINFOHEADER) lpbiNew)) {
        // Driver didn't accept the format,
        // switch back to the original

        errorDriverID (lpcs, dwError);

        SetFormatFromDIB (lpcs, (LPBITMAPINFOHEADER)lpBInfo);
    }

    // Force a new frame to be taken, so the DIB contains good
    // data.  Especially important to prevent codecs from exploding!
    videoFrame (lpcs->hVideoIn, &lpcs->VidHdr);

    if (lpBInfo)
        GlobalFreePtr (lpBInfo);

    f = DrawDibBegin(lpcs->hdd,NULL,-1,-1,(LPBITMAPINFOHEADER)(lpcs->lpBitsInfo),-1,-1,0);
    if (!f) {
        errorUpdateError(lpcs, IDS_CAP_AVI_DRAWDIB_ERROR);
        //errorDriverID (lpcs, IDS_CAP_AVI_DRAWDIB_ERROR);
    }

    return (dwError == DV_ERR_OK);
}


void xlatClut8 (BYTE HUGE *pb, DWORD dwSize, BYTE HUGE *xlat)
{
    DWORD dw;

    for (dw = 0; dw < dwSize; dw++, ((BYTE huge *)pb)++)
        *pb = xlat[*pb];
}

//
// DibNewPalette
//
// Performs three functions:
// 1. Updates the biClrUsed field if biBitCount <= 8.
// 2. Remaps BI_RGB images through a LUT when a new palette is assigned.
// 3. Copies the palette entries into our global BITMAPINFO
//
// Returns: TRUE on success
//
DWORD DibNewPalette (LPCAPSTREAM lpcs, HPALETTE hPalNew)
{
    LPBITMAPINFOHEADER  lpbi;
    int                 n;
    short               nColors;
    BYTE FAR *          lpBits;
    RGBQUAD FAR *       lpRgb;
    BYTE                xlat[256];
    DWORD               dwSize;
    PALETTEENTRY        pe;

    if (!hPalNew || !lpcs->lpBits || !lpcs->lpBitsInfo)
        return FALSE;

    lpbi   = &(lpcs->lpBitsInfo->bmiHeader);
    lpRgb  = (RGBQUAD FAR *)((LPSTR)lpbi + (UINT)lpbi->biSize);
    lpBits = lpcs->lpBits;

    GetObject(hPalNew, sizeof(short), (LPSTR) &nColors);
    if (nColors > 256)
        nColors = 256;

    // Get the palette entries regardless of the compression
    // Supermac uses non BI_RGB with a palette!

    if (lpbi->biBitCount == 8) {
        for (n=0; n<nColors; n++) {
            GetPaletteEntries(hPalNew, n, 1, &pe);
            lpRgb[n].rgbRed   = pe.peRed;
            lpRgb[n].rgbGreen = pe.peGreen;
            lpRgb[n].rgbBlue  = pe.peBlue;
        }
    }

    if (lpbi->biBitCount == 8 && lpbi->biCompression == BI_RGB) {

        //
        //  build a xlat table. from the old Palette to the new palette.
        //
        for (n=0; n<(int)lpbi->biClrUsed; n++) {
            xlat[n] = (BYTE)GetNearestPaletteIndex(hPalNew,
                RGB(lpRgb[n].rgbRed,lpRgb[n].rgbGreen,lpRgb[n].rgbBlue));
        }

        //
        // translate the DIB bits
        //
        if ((dwSize = lpbi->biSizeImage) == 0)
            dwSize = lpbi->biHeight * DIBWIDTHBYTES(*lpbi);

        switch ((WORD)lpbi->biCompression)
        {
            case BI_RGB:
                xlatClut8(lpBits, dwSize, xlat);
        }
    }

    // Fix for Supermac, force biClrUsed to the number of palette entries
    // even if non-BI_RGB formats.

    if (lpbi->biBitCount <= 8)
        lpbi->biClrUsed = nColors;

    return TRUE;
}


/* DibPaint(LPCAPSTREAM lpcs, hdc)
 *
 * Paint the current DIB into the window;
 */
void DibPaint(LPCAPSTREAM lpcs, HDC hdc)
{
    RECT        rc;
    BOOL        fOK;

    fOK = (lpcs->lpBits != NULL);

    if (fOK) {
        if (lpcs->fScale) {
            GetClientRect(lpcs->hwnd, &rc);
            fOK = DrawDibDraw(lpcs->hdd, hdc, 0, 0,
                  rc.right - rc.left, rc.bottom - rc.top,
                  (LPBITMAPINFOHEADER)lpcs->lpBitsInfo, lpcs->lpBits,
                   0, 0, -1, -1,
#if defined _WIN32 && defined UNICODE
		    0 	// we don't support BACKGROUNDPAL yet in drawdib
#else
		   DDF_BACKGROUNDPAL
#endif
		   );
        }
        else
            fOK = DrawDibDraw(lpcs->hdd, hdc, 0, 0,
                lpcs->dxBits, lpcs->dyBits,
                (LPBITMAPINFOHEADER)lpcs->lpBitsInfo, lpcs->lpBits,
                0, 0, -1, -1,
#if defined _WIN32 && defined UNICODE
		    0 	// we don't support BACKGROUNDPAL yet in drawdib
#else
		   DDF_BACKGROUNDPAL
#endif
		   );

    }
    if (!fOK) {
        SelectObject(hdc, GetStockObject(BLACK_BRUSH));
        GetClientRect(lpcs->hwnd, &rc);
        PatBlt(hdc, 0, 0, rc.right, rc.bottom, PATCOPY);
    }
}

/*
 *
 * CreatePackedDib() - return the current DIB in packed (ie CF_DIB) format
 *
 */

HANDLE CreatePackedDib (LPBITMAPINFO lpBitsInfo, LPSTR lpSrcBits, HPALETTE hPalette)
{
    HANDLE              hdib;
    LPBITMAPINFO        lpbi;
    int                 i;
    DWORD               dwSize;
    PALETTEENTRY        pe;
    LPBYTE              lpBits;
    RGBQUAD FAR *       lpRgb;

    // If the data is compressed, let ICM do the work for us...
    if (lpBitsInfo->bmiHeader.biCompression != BI_RGB &&
         lpBitsInfo->bmiHeader.biCompression != BI_RLE8 &&
        (lpBitsInfo->bmiHeader.biBitCount != 8 ||
         lpBitsInfo->bmiHeader.biBitCount != 24 )) {

        LPBITMAPINFO lpOutFormat = NULL;
        HANDLE hPackedDIBOut = NULL;

        if (!(lpOutFormat = (LPBITMAPINFO)GlobalAllocPtr(
                        GMEM_MOVEABLE, sizeof (BITMAPINFOHEADER) +
                        256 * sizeof (RGBQUAD))))
            return NULL;

        CopyMemory (lpOutFormat, lpBitsInfo, sizeof (BITMAPINFOHEADER));

        // Try to get an RGB format
        lpOutFormat->bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
        lpOutFormat->bmiHeader.biCompression = BI_RGB;
        lpOutFormat->bmiHeader.biClrUsed = 0;
        lpOutFormat->bmiHeader.biClrImportant = 0;

        // Uh, oh, force to a 24-bit DIB if > 8 BPP
        if (lpBitsInfo->bmiHeader.biBitCount <= 8)
            lpOutFormat->bmiHeader.biBitCount = 8;
        else
            lpOutFormat->bmiHeader.biBitCount = 24;

        lpOutFormat->bmiHeader.biSizeImage =
                WIDTHBYTES (lpOutFormat->bmiHeader.biWidth *
                (lpOutFormat->bmiHeader.biBitCount == 8 ? 1 : 3)) *
                lpOutFormat->bmiHeader.biHeight;

        hPackedDIBOut = ICImageDecompress (
                NULL,           /*hic*/
                0,              /*uiFlags*/
                lpBitsInfo,     /*lpbiIn*/
                lpSrcBits,      /*lpBits*/
                lpOutFormat);   /*use default format chosen by compressor*/

        if (lpOutFormat)
            GlobalFreePtr (lpOutFormat);

        return (hPackedDIBOut);
    }

    dwSize = lpBitsInfo->bmiHeader.biSize +
              lpBitsInfo->bmiHeader.biClrUsed * sizeof(RGBQUAD) +
              lpBitsInfo->bmiHeader.biSizeImage;

    hdib = GlobalAlloc(GMEM_MOVEABLE, dwSize);

    if (!hdib)
         return NULL;

    lpbi = (LPVOID)GlobalLock(hdib);

    //
    // copy the header
    //
    CopyMemory (lpbi, lpBitsInfo, lpBitsInfo->bmiHeader.biSize);

    //
    // copy the color table
    //
    lpRgb  = (RGBQUAD FAR *)((LPSTR)lpbi + (WORD)lpbi->bmiHeader.biSize);
    for (i=0; i < (int)lpBitsInfo->bmiHeader.biClrUsed; i++) {
        GetPaletteEntries(hPalette, i, 1, &pe);
        lpRgb[i].rgbRed   = pe.peRed;
        lpRgb[i].rgbGreen = pe.peGreen;
        lpRgb[i].rgbBlue  = pe.peBlue;
        lpRgb[i].rgbReserved = 0;
    }

    //
    // copy the bits.
    //
    lpBits  =   (LPBYTE)lpbi +
                lpbi->bmiHeader.biSize +
                lpbi->bmiHeader.biClrUsed * sizeof(RGBQUAD);

    CopyMemory (lpBits, lpSrcBits,
                lpbi->bmiHeader.biSizeImage);

    GlobalUnlock (hdib);

    return hdib;
 }


 /*---------------------------------------------------------------------+
 | dibIsWritable() - return TRUE if the dib format is writable,                  |
 |                     by out dibWrite() function, FALSE if not.                 |
 |                                                                               |
 +---------------------------------------------------------------------*/
BOOL FAR PASCAL dibIsWritable (LPBITMAPINFO lpBitsInfo)
{
    if (!lpBitsInfo)
        return FALSE;

     // For now, just assume that all capture formats have an installed
     // codec which can convert to RGB.  In the future, each time the
     // format is changed, test that the codec actually accepts the format.

     return TRUE;
 }


 /*---------------------------------------------------------------------+
 | dibWrite() - write out the DIB to a file. The global header is       |
 |                in <glpBitsInfo> and the actual dib bits are in                |
 |                <glpBits>.  If it is palettized then the palette is in         |
 |                <ghPalCurrent>.                                                |
 |                                                                               |
 |  We won't do error reporting in this function, let the caller take   |
 |  care of that along with Opening and Closing the HMMIO.              |
 |                                                                               |
 +---------------------------------------------------------------------*/
BOOL FAR PASCAL dibWrite(LPCAPSTREAM lpcs, HMMIO hmmio)
 {
     BITMAPFILEHEADER   bfh;
     DWORD              dw;
     HANDLE             hPackedDib = NULL;
     LPBITMAPINFO       lpbi = NULL;
     BOOL               fOK = FALSE;

     /* do some checking */
    WinAssert(hmmio != 0);

    if (!lpcs->lpBits || !lpcs->lpBitsInfo)
        return FALSE;

    // Create a packed DIB, converting from a compressed format,
    // if necessary.
    hPackedDib = CreatePackedDib (lpcs->lpBitsInfo,
                        lpcs->lpBits,
                        lpcs->hPalCurrent);

    lpbi = (LPBITMAPINFO) GlobalLock (hPackedDib);

    if (!lpbi)
        goto WriteError;

    /* initialize the bitmap file header */
    bfh.bfType = 'B' | 'M' << 8;
    bfh.bfSize = sizeof(bfh) + sizeof(BITMAPINFOHEADER) +
        lpbi->bmiHeader.biSizeImage +
        (lpbi->bmiHeader.biBitCount > 8 ? 0 : (lpbi->bmiHeader.biClrUsed * sizeof(RGBQUAD)));

    bfh.bfReserved1 = bfh.bfReserved2 = 0;
    bfh.bfOffBits = bfh.bfSize - lpbi->bmiHeader.biSizeImage ;

    // dw is the size of the BITMAPINFO + color table + image
    dw = bfh.bfSize - sizeof(bfh);

    /* write out the file header portion */
    if (mmioWrite(hmmio, (HPSTR)&bfh, (LONG)sizeof(BITMAPFILEHEADER)) !=
                sizeof(BITMAPFILEHEADER)){
         goto WriteError;
    }

    /* now write out the header and bits */
    if (mmioWrite(hmmio, (HPSTR)lpbi, (LONG) dw) == (LONG) dw) {
         fOK = TRUE;
    }

WriteError:
    if (lpbi)
        GlobalUnlock (hPackedDib);
    if (hPackedDib)
        GlobalFree (hPackedDib);

    return fOK;
}

/*--------------------------------------------------------------+
| fileSaveDIB - save the frame as a DIB                         |
|   Top level routine to save a single frame                    |
+--------------------------------------------------------------*/
BOOL FAR PASCAL fileSaveDIB(LPCAPSTREAM lpcs, LPTSTR lpszFileName)
{
    HMMIO               hmmio;
    HCURSOR             hOldCursor;
    BOOL                fOK;

    hmmio = mmioOpen(lpszFileName, NULL, MMIO_WRITE);
    if( !hmmio ) {
	/* try and create */
        hmmio = mmioOpen(lpszFileName, NULL, MMIO_CREATE | MMIO_WRITE);
	if( !hmmio ) {
	    /* find out if the file was read only or we are just */
	    /* totally hosed up here.				 */
	    hmmio = mmioOpen(lpszFileName, NULL, MMIO_READ);
	    if (hmmio){
		/* file was read only, error on it */
                errorUpdateError (lpcs, IDS_CAP_READONLYFILE, (LPSTR)lpszFileName);
		mmioClose(hmmio, 0);
		return FALSE;
	    } else {
		/* even weirder error has occured here, give CANTOPEN */
                errorUpdateError (lpcs, IDS_CAP_CANTOPEN, (LPSTR) lpszFileName);
		return FALSE;
	    }
	}
    }

    hOldCursor = SetCursor( lpcs->hWaitCursor );

    mmioSeek(hmmio, 0, SEEK_SET);

    fOK = dibWrite(lpcs, hmmio);

    mmioClose( hmmio, 0 );

    SetCursor( hOldCursor );

    if (!fOK)
       errorUpdateError (lpcs, IDS_CAP_ERRORDIBSAVE, (LPSTR) lpszFileName);

    return fOK;
}
