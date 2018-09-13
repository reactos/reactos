/****************************************************************************
 *
 *   cappal.c
 *
 *   Palette processing module.
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
#include "cappal.h"
#include "capdib.h"
#include "dibmap.h"

//
// Allocate and initialize palette resources at Window create time
//
BOOL PalInit (LPCAPSTREAM lpcs)
{
    return (PalGetPaletteFromDriver (lpcs));
}

//
// FreePaletteCache - free the RGB555 Xlate table
//
void FreePaletteCache (LPCAPSTREAM lpcs)
{
    if (lpcs->lpCacheXlateTable) {
        GlobalFreePtr (lpcs->lpCacheXlateTable);
	lpcs->lpCacheXlateTable = NULL;
    }
}

//
// Release palette resources at Window destroy time
//
void PalFini (LPCAPSTREAM lpcs)
{
    PalDeleteCurrentPalette (lpcs);

    FreePaletteCache (lpcs);
}

//
// Delete our palette if it isn't the system default palette
//
void PalDeleteCurrentPalette (LPCAPSTREAM lpcs)
{
    if (lpcs->hPalCurrent &&
            (lpcs->hPalCurrent != GetStockObject(DEFAULT_PALETTE)))
        DeleteObject (lpcs->hPalCurrent);
    lpcs->hPalCurrent = NULL;
}

//
// Get the current palette (from the driver)
// Returns: TRUE if the driver can supply a palette
//

BOOL PalGetPaletteFromDriver (LPCAPSTREAM lpcs)
{
    FCLOGPALETTE        pal;

    PalDeleteCurrentPalette (lpcs);

    pal.palVersion = 0x0300;
    pal.palNumEntries = 256;

    lpcs->sCapDrvCaps.fDriverSuppliesPalettes = FALSE;  // assume the worst

    if (lpcs->fHardwareConnected) {
        if (videoConfigure (lpcs->hVideoIn,
                DVM_PALETTE,
                VIDEO_CONFIGURE_GET | VIDEO_CONFIGURE_CURRENT, NULL,
                (LPVOID)&pal, sizeof(pal),
                NULL, 0 ) == DV_ERR_OK) {
            if (lpcs->hPalCurrent = CreatePalette ((LPLOGPALETTE) &pal))
                lpcs->sCapDrvCaps.fDriverSuppliesPalettes = TRUE;
        }
    }
    if (!lpcs->hPalCurrent)
        lpcs->hPalCurrent = GetStockObject (DEFAULT_PALETTE);

    DibNewPalette (lpcs, lpcs->hPalCurrent);

    return (lpcs->sCapDrvCaps.fDriverSuppliesPalettes);
}

//
// Set the current palette used for capture by sending a copy to the driver
// and then copying the entries to out DIB.
// This may also be called when reconnecting a driver and using a cached
// copy of the palette.
// Returns TRUE on success, or FALSE on failure.
//
DWORD PalSendPaletteToDriver (LPCAPSTREAM lpcs, HPALETTE hpal, LPBYTE lpXlateTable)
{
    short               nColors;
    FCLOGPALETTE        pal;
    HCURSOR             hOldCursor;

    // The following can take a while so repaint our parent
    UpdateWindow (GetParent (lpcs-> hwnd));
    UpdateWindow (lpcs->hwnd);

    if (!hpal)
        return FALSE;

    // Allocate a xlate table cache?
    if (lpXlateTable) {
        if (lpcs->lpCacheXlateTable == NULL) {
            lpcs->lpCacheXlateTable = GlobalAllocPtr (GHND, 0x8000l);
            if (!lpcs->lpCacheXlateTable)
                return FALSE;
        }

        // If we're not using the cached table, update the cache
        if (lpcs->lpCacheXlateTable != lpXlateTable)
            _fmemcpy (lpcs->lpCacheXlateTable, lpXlateTable, (UINT) 0x8000l);
    }
    else {
        FreePaletteCache (lpcs);
    }

    // Don't destroy the current palette when reconnecting...
    if (hpal != lpcs->hPalCurrent) {
        PalDeleteCurrentPalette (lpcs);
        lpcs->hPalCurrent = hpal;
    }

    GetObject(hpal, sizeof(short), (LPVOID)&nColors);

    if( nColors <= 1 ) {    //!!>
        return( FALSE );
    }

    nColors = min(256, nColors);

    hOldCursor = SetCursor (lpcs-> hWaitCursor);

    statusUpdateStatus (lpcs, IDS_CAP_STAT_PALETTE_BUILD);

    pal.palVersion = 0x0300;
    pal.palNumEntries = nColors;

    GetPaletteEntries(hpal, 0, nColors, pal.palPalEntry);

    if (lpcs-> fHardwareConnected) {

        // first try to send both the xlate table and the palette
        if ((!lpXlateTable) || (videoConfigure( lpcs->hVideoIn,
                    DVM_PALETTERGB555,
                    VIDEO_CONFIGURE_SET, NULL,
                    (LPLOGPALETTE)&pal, sizeof(pal),
                    lpXlateTable, (DWORD) 0x8000) != 0)) {

            // else send just the palette and make the driver build the table
            if (videoConfigure( lpcs->hVideoIn,
                    DVM_PALETTE,
                    VIDEO_CONFIGURE_SET, NULL,
                    (LPLOGPALETTE)&pal, sizeof(pal),
                    NULL, 0 )) {
                // Scrncap doesn't support setting a palette, so
                // delete the palette cache
                FreePaletteCache (lpcs);
            }
        }
    }

    // Supermac wants us to get the palette again, they might have
    // mucked with it!
    PalGetPaletteFromDriver (lpcs);

    // Since the palette has changed, delete any existing compression
    // output format;  this forces a new output format to be selected
    if (lpcs->CompVars.lpbiOut) {
        GlobalFreePtr (lpcs->CompVars.lpbiOut);
        lpcs->CompVars.lpbiOut = NULL;
    }
    if (lpcs->CompVars.hic) {
        if (ICSeqCompressFrameStart(&lpcs->CompVars, lpcs->lpBitsInfo) == 0) {
            errorUpdateError (lpcs, IDS_CAP_COMPRESSOR_ERROR);
        }
    }

    InvalidateRect (lpcs->hwnd, NULL, TRUE);
    UpdateWindow (lpcs->hwnd);

    SetCursor (hOldCursor);
    statusUpdateStatus (lpcs, 0);

    return (TRUE);
}

//
// CopyPalette, makes a copy of a GDI logical palette
// Returns: a handle to the newly created palette, or NULL if error
//

HPALETTE CopyPalette (HPALETTE hpal)
{
    LPLOGPALETTE        lppal;
    short               nNumEntries;

    if (!hpal)
        return NULL;

    GetObject (hpal,sizeof(short),(LPVOID)&nNumEntries);

    if (nNumEntries == 0)
        return NULL;

    lppal = (LPLOGPALETTE) GlobalAllocPtr (GHND,
                sizeof(LOGPALETTE) + nNumEntries * sizeof(PALETTEENTRY));

    if (!lppal)
        return NULL;

    lppal->palVersion    = 0x300;
    lppal->palNumEntries = nNumEntries;

    GetPaletteEntries(hpal,0,nNumEntries,lppal->palPalEntry);

    hpal = CreatePalette(lppal);

    GlobalFreePtr (lppal);

    return hpal;
}


//
// Allocate resources needed for palette capture
// Returns DV_ERR_OK on success, or DV_ERR... on failure.
// Note: if Init fails, you MUST call the Fini function to
// release resources.
//
DWORD CapturePaletteInit (LPCAPSTREAM lpcs, LPCAPPAL lpcp)
{
    DWORD dwError = DV_ERR_OK;

    lpcp->lpBits = NULL;
    lpcp->lp16to8 = NULL;
    lpcp->lpHistogram = NULL;
    lpcp->lpbiSave = NULL;
    lpcp->wNumFrames = 0;

    // Init an RGB16 header
    lpcp->bi16.biSize         = sizeof(BITMAPINFOHEADER);
    lpcp->bi16.biWidth        = lpcs->dxBits;
    lpcp->bi16.biHeight       = lpcs->dyBits;
    lpcp->bi16.biPlanes       = 1;
    lpcp->bi16.biBitCount     = 16;
    lpcp->bi16.biCompression  = BI_RGB;
    lpcp->bi16.biSizeImage    = DIBWIDTHBYTES(lpcp->bi16) * lpcp->bi16.biHeight;
    lpcp->bi16.biXPelsPerMeter= 0;
    lpcp->bi16.biYPelsPerMeter= 0;
    lpcp->bi16.biClrUsed      = 0;
    lpcp->bi16.biClrImportant = 0;

    // Allocate memory for the histogram, DIB, and XLate table
    lpcp->lpBits  = GlobalAllocPtr (GHND, lpcp->bi16.biSizeImage);
    lpcp->lp16to8 = GlobalAllocPtr (GHND, 0x8000l);
    lpcp->lpHistogram = InitHistogram(NULL);

    if (!lpcp->lpBits || !lpcp->lp16to8 || !lpcp->lpHistogram) {
        dwError = DV_ERR_NOMEM;
        goto PalInitError;
    }

    // Init the video header
    lpcp->vHdr.lpData = lpcp->lpBits;
    lpcp->vHdr.dwBufferLength = lpcp->bi16.biSizeImage;
    lpcp->vHdr.dwUser = 0;
    lpcp->vHdr.dwFlags = 0;

    // Save the current format
    lpcp->lpbiSave = DibGetCurrentFormat (lpcs);

    // Make sure we can set the format to 16 bit RGB
    if(dwError = videoConfigure( lpcs->hVideoIn, DVM_FORMAT,
            VIDEO_CONFIGURE_SET, NULL,
            (LPBITMAPINFOHEADER)&lpcp->bi16, sizeof(BITMAPINFOHEADER),
            NULL, 0 ) ) {
        goto PalInitError;
    }

    // Put everything back the way it was
    if (dwError = videoConfigure( lpcs->hVideoIn, DVM_FORMAT,
            VIDEO_CONFIGURE_SET, NULL,
            (LPBITMAPINFOHEADER)lpcp->lpbiSave, lpcp->lpbiSave->bmiHeader.biSize,
            NULL, 0 )) {
        goto PalInitError;
    }

PalInitError:
    return dwError;
}

//
// Free resources used for palette capture
//
DWORD CapturePaletteFini (LPCAPSTREAM lpcs, LPCAPPAL lpcp)
{
    if (lpcp->lpBits) {
        GlobalFreePtr (lpcp->lpBits);
        lpcp->lpBits = NULL;
    }
    if (lpcp->lp16to8) {
        GlobalFreePtr (lpcp->lp16to8);
        lpcp->lp16to8 = NULL;
    }
    if (lpcp->lpHistogram) {
        FreeHistogram(lpcp->lpHistogram);
        lpcp->lpHistogram = NULL;
    }
    if (lpcp->lpbiSave) {
        GlobalFreePtr (lpcp->lpbiSave);
        lpcp->lpbiSave = NULL;
    }
    return DV_ERR_OK;
}

//
//  CapturePaletteFrames() The workhorse of capture palette.
//
DWORD CapturePaletteFrames (LPCAPSTREAM lpcs, LPCAPPAL lpcp, int nCount)
{
    int j;
    DWORD dwError;

    // switch to RGB16 format
    if (dwError = videoConfigure( lpcs->hVideoIn,
                DVM_FORMAT,
                VIDEO_CONFIGURE_SET, NULL,
                (LPBITMAPINFOHEADER)&lpcp->bi16, sizeof(BITMAPINFOHEADER),
                NULL, 0 ))
        goto CaptureFramesError;

    for (j = 0; j < nCount; j++){
        // Get a frame
        dwError = videoFrame(lpcs->hVideoIn, &lpcp->vHdr);

        // Let the user see it
        InvalidateRect (lpcs->hwnd, NULL, TRUE);
        UpdateWindow (lpcs->hwnd);

        // Histogram it
        DibHistogram(&lpcp->bi16, lpcp->lpBits, 0, 0, -1, -1, lpcp->lpHistogram);
        lpcp->wNumFrames++;
    }

    dwError = videoConfigure( lpcs->hVideoIn,
                DVM_FORMAT,
                VIDEO_CONFIGURE_SET, NULL,
                (LPBITMAPINFOHEADER)lpcp->lpbiSave,
                lpcp->lpbiSave->bmiHeader.biSize,
                NULL, 0 );

//    videoFrame( lpcs->hVideoIn, &lpcs->VidHdr );

CaptureFramesError:
    return dwError;
}

//
//  CapturePaletteAuto() capture a palette from the video source
//  without user intervention.
//  Returns TRUE on success, FALSE on error
//
BOOL CapturePaletteAuto (LPCAPSTREAM lpcs, int nCount, int nColors)
{
    HPALETTE    hpal;
    HCURSOR     hOldCursor;
    DWORD       dwError = DV_ERR_OK;
    CAPPAL      cappal;
    LPCAPPAL    lpcp;

    lpcp = &cappal;

    if (!lpcs->sCapDrvCaps.fDriverSuppliesPalettes)
        return FALSE;

    if (nColors <= 0 || nColors > 256)
        return FALSE;

    lpcp->wNumColors = max (nColors, 2);  // at least 2 colors

    if (nCount <= 0)
        return FALSE;

    if (dwError = CapturePaletteInit (lpcs, lpcp))
        goto PalAutoExit;

    hOldCursor = SetCursor(lpcs->hWaitCursor);

    CapturePaletteFrames (lpcs, lpcp, nCount);

    /* we grabbed a frame, time to compute a palette */
    statusUpdateStatus(lpcs, IDS_CAP_STAT_OPTPAL_BUILD);

    // The HPALETTE returned in the following becomes
    // our "global" palette, hence is not deleted here.
    hpal = HistogramPalette(lpcp->lpHistogram, lpcp->lp16to8, lpcp->wNumColors);

    // Send driver both the pal and xlate table
    PalSendPaletteToDriver(lpcs, hpal, (LPBYTE)lpcp->lp16to8 );

    videoFrame( lpcs->hVideoIn, &lpcs->VidHdr );  // Update the display with a new image

    SetCursor(hOldCursor);

    InvalidateRect(lpcs->hwnd, NULL, TRUE);
    UpdateWindow(lpcs->hwnd);
    lpcs->fUsingDefaultPalette = FALSE;

PalAutoExit:
    CapturePaletteFini (lpcs, lpcp);
    statusUpdateStatus(lpcs, 0);

   // If an error happened, display it
   if (dwError)
        errorDriverID (lpcs, dwError);

    return (dwError == DV_ERR_OK);
}

//
//  CapturePaletteManual() capture a palette from the video source
//  with user intervention.
//  fGrab is TRUE on all but the last frame captured
//  Returns TRUE on success, FALSE on error
//
BOOL CapturePaletteManual (LPCAPSTREAM lpcs, BOOL fGrab, int nColors)
{
    HPALETTE    hpal;
    HCURSOR     hOldCursor;
    LPCAPPAL    lpcp;
    DWORD       dwError = DV_ERR_OK;

    if (!lpcs->sCapDrvCaps.fDriverSuppliesPalettes)
        return FALSE;

    hOldCursor = SetCursor(lpcs->hWaitCursor);

    // We're initializing for the first time, so alloc everything
    if (lpcs->lpCapPal == NULL) {

        if (lpcp = (LPCAPPAL) GlobalAllocPtr (GHND, sizeof(CAPPAL))) {
            lpcs->lpCapPal = lpcp;

            if (nColors == 0)
                nColors = 256;
            lpcp->wNumColors = min (nColors, 256);
            dwError = CapturePaletteInit (lpcs, lpcp);
        }
        else
            dwError = IDS_CAP_OUTOFMEM;
    }
    lpcp = lpcs->lpCapPal;

    if (dwError != DV_ERR_OK)
        goto PalManualExit;

    // Add a frame to the histogram
    // Handle the case of telling us to stop before we ever started
    if (fGrab || !fGrab && (lpcp->wNumFrames == 0)) {
        CapturePaletteFrames (lpcs, lpcp, 1);
        lpcs->fUsingDefaultPalette = FALSE;
    }
    // All done, send the new palette to the driver
    if (!fGrab) {
        statusUpdateStatus(lpcs, IDS_CAP_STAT_OPTPAL_BUILD);

        // The HPALETTE returned in the following becomes
        // our "global" palette, hence is not deleted here.
        hpal = HistogramPalette(lpcp->lpHistogram,
                        lpcp->lp16to8, lpcp->wNumColors);

        // Send driver both the pal and xlate table
        PalSendPaletteToDriver(lpcs, hpal, (LPBYTE)lpcp->lp16to8 );
    }

    videoFrame( lpcs->hVideoIn, &lpcs->VidHdr );  // Update the display with a new image
    InvalidateRect(lpcs->hwnd, NULL, TRUE);
    UpdateWindow(lpcs->hwnd);

PalManualExit:
    if (!fGrab || (dwError != DV_ERR_OK)) {
        if (lpcp != NULL) {
            CapturePaletteFini (lpcs, lpcp);
            GlobalFreePtr (lpcp);
            lpcs->lpCapPal = NULL;
        }
    }

    SetCursor(hOldCursor);
    statusUpdateStatus(lpcs, 0);

    // If an error happened, display it
    if (dwError) {
        errorUpdateError (lpcs, (UINT) dwError);
    }

    return (dwError == DV_ERR_OK);
}



/*--------------------------------------------------------------+
| fileSavePalette - save the current palette in a file  	|
|								|
+--------------------------------------------------------------*/
BOOL FAR PASCAL fileSavePalette(LPCAPSTREAM lpcs, LPTSTR lpszFileName)
{
    HPALETTE            hpal;
    HMMIO               hmmio;
    WORD	        w;
    HCURSOR             hOldCursor;
    MMCKINFO            ckRiff;
    MMCKINFO            ck;
    short               nColors;
    FCLOGPALETTE        pal;
    BOOL                fOK = FALSE;

    if ((hpal = lpcs->hPalCurrent) == NULL)
        return FALSE;

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
                errorUpdateError (lpcs, IDS_CAP_READONLYFILE, (LPTSTR) lpszFileName);
		mmioClose(hmmio, 0);
		return FALSE;
	    } else {
		/* even weirder error has occured here, give CANTOPEN */
                errorUpdateError (lpcs, IDS_CAP_CANTOPEN, (LPTSTR) lpszFileName);
		return FALSE;
	    }
	}
    }

    hOldCursor = SetCursor( lpcs-> hWaitCursor );

    /* Seek to beginning of file, so we can write the header. */
    mmioSeek(hmmio, 0, SEEK_SET);

    /* Create RIFF chunk */
    ckRiff.fccType = mmioFOURCC('P','A','L',' ');
    if(mmioCreateChunk (hmmio,&ckRiff,MMIO_CREATERIFF)) {
         goto FileError;
    }

    /* Create Palette chunk */
    ck.cksize = 0;
    ck.ckid = mmioFOURCC('d','a','t','a');
    if(mmioCreateChunk(hmmio,&ck,0)) {
         goto FileError;
    }

    // Get the palette data here
    GetObject(hpal, sizeof(short), (LPVOID)&nColors);

    pal.palVersion = 0x0300;
    pal.palNumEntries = nColors;

    GetPaletteEntries(hpal, 0, nColors, pal.palPalEntry);

    // Calc the size of the logpalette
    // which is the sizeof palVersion + sizeof palNumEntries + colors
    w = sizeof (WORD) + sizeof (WORD) + nColors * sizeof (PALETTEENTRY);

    // Write out the palette
    if(mmioWrite(hmmio, (LPBYTE)&pal, (DWORD) w) != (LONG) w) {
        goto FileError;
    }

    if(mmioAscend(hmmio, &ck, 0)) {
        goto FileError;
    }

    if(mmioAscend(hmmio, &ckRiff, 0)) {
        goto FileError;
    }

    fOK = TRUE;

FileError:
    mmioClose( hmmio, 0 );

    SetCursor( hOldCursor );

    if (!fOK)
        errorUpdateError (lpcs, IDS_CAP_ERRORPALSAVE, (LPTSTR) lpszFileName);

    return fOK;
}


/*--------------------------------------------------------------+
| fileOpenPalette - use a new palette from the specified file   |
|								|
+--------------------------------------------------------------*/
BOOL FAR PASCAL fileOpenPalette(LPCAPSTREAM lpcs, LPTSTR lpszFileName)
{
    HPALETTE            hpal;
    HMMIO               hmmio;
    WORD	        w;
    HCURSOR             hOldCursor;
    MMCKINFO            ckRiff;
    MMCKINFO            ck;
    FCLOGPALETTE        pal;
    BOOL                fOK = FALSE;

    if ((hpal = lpcs->hPalCurrent) == NULL)
        return FALSE;

    hmmio = mmioOpen(lpszFileName, NULL, MMIO_READ);
    if( !hmmio ) {
        errorUpdateError (lpcs, IDS_CAP_ERRORPALOPEN, (LPTSTR) lpszFileName);
        return FALSE;
    }

    hOldCursor = SetCursor( lpcs-> hWaitCursor );

    /* Seek to beginning of file, so we can read the header. */
    mmioSeek(hmmio, 0, SEEK_SET);

    /* Find the RIFF chunk */
    ckRiff.fccType = mmioFOURCC('P','A','L',' ');
    if(mmioDescend (hmmio, &ckRiff, NULL, MMIO_FINDRIFF)) {
         goto PalOpenError;
    }

    /* Find the data chunk */
    ck.cksize = 0;
    ck.ckid = mmioFOURCC('d','a','t','a');
    if(mmioDescend (hmmio, &ck, &ckRiff, MMIO_FINDCHUNK)) {
         goto PalOpenError;
    }

    // First read just the Version and number of entries
    // which is the sizeof palVersion + sizeof palNumEntries
    w = sizeof (WORD) + sizeof (WORD);
    if(mmioRead(hmmio, (LPBYTE)&pal, (DWORD) w) != (LONG) w) {
        goto PalOpenError;
    }

    // Do a bit of checking
    if ((pal.palVersion != 0x0300) || (pal.palNumEntries > 256))
        goto PalOpenError;

    // Now get the actual palette data
    // which is the sizeof palVersion + sizeof palNumEntries
    w = pal.palNumEntries * sizeof (PALETTEENTRY);
    if(mmioRead(hmmio, (LPBYTE)&pal.palPalEntry, (DWORD) w) != (LONG) w) {
        goto PalOpenError;
    }

    if (hpal = CreatePalette ((LPLOGPALETTE) &pal)) {
        PalSendPaletteToDriver (lpcs, hpal, NULL /*lpXlateTable */);
        fOK = TRUE;
    }

    videoFrame( lpcs->hVideoIn, &lpcs->VidHdr );  // grab a new frame

PalOpenError:
    mmioClose( hmmio, 0 );

    SetCursor( hOldCursor );
    InvalidateRect(lpcs->hwnd, NULL, TRUE);
    UpdateWindow(lpcs->hwnd);		// update the display with new frame

    if (!fOK)
        errorUpdateError (lpcs, IDS_CAP_ERRORPALOPEN, (LPTSTR) lpszFileName);
    else
    lpcs->fUsingDefaultPalette = FALSE;

    return fOK;
}
