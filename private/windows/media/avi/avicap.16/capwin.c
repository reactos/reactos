/****************************************************************************
 *
 *   capwin.c
 *
 *   Main window proceedure.
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

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <msvideo.h>
#include <drawdib.h>
#include <mmreg.h>
#include <memory.h>
#include "avicap.h"
#include "avicapi.h"
#include "cappal.h"
#include "capdib.h"
#include "dibmap.h"

// GetWindowLong assignments
#define GWL_CAPSTREAM   0
#define GWL_CAPVBSTATUS 4       // Used by VB Status callback
#define GWL_CAPVBERROR  8       // Used by VB Error callback
#define GWL_CAP_SPARE1  12      // Room to grow
#define GWL_CAP_SPARE2  16      // Room to grow

#define ID_PREVIEWTIMER 9

//#ifdef _DEBUG
#ifdef PLASTIQUE
    #define MB(lpsz) MessageBox(NULL, lpsz, "", MB_OK);
#else
    #define MB(lpsz)
#endif


//
// Set the overlay rectangles on capture cards which support
// overlay, and then enable/disable the key color.
//
static void SetOverlayRectangles (LPCAPSTREAM lpcs)
{
    HDC hdc;
    BOOL fVisible;
    RECT rc;

    if (!lpcs->hVideoDisplay)
        return;

    hdc = GetDC (lpcs->hwnd);
    fVisible = (GetClipBox (hdc, &rc) != NULLREGION);
    ReleaseDC (lpcs->hwnd, hdc);

    if (!fVisible)  // disable the overlay if iconic
        videoStreamFini (lpcs->hVideoDisplay);
    else {
        // Destination
        GetClientRect (lpcs->hwnd, &rc);
        ClientToScreen (lpcs->hwnd, (LPPOINT)&rc);
        ClientToScreen (lpcs->hwnd, (LPPOINT)&rc+1);
        videoMessage (lpcs->hVideoDisplay,
                        DVM_DST_RECT,
                        (DWORD) (LPVOID) &rc, VIDEO_CONFIGURE_SET);

        // Overlay channel Source rectangle
        SetRect (&rc, lpcs->ptScroll.x, lpcs->ptScroll.y,
                lpcs->ptScroll.x + rc.right - rc.left,
                lpcs->ptScroll.y + rc.bottom - rc.top);
        videoMessage (lpcs->hVideoDisplay,
                        DVM_SRC_RECT,
                        (DWORD) (LPVOID) &rc, VIDEO_CONFIGURE_SET);

        videoStreamInit (lpcs->hVideoDisplay, 0L, 0L, 0L, 0L);
    }
}

// WM_POSITIONCHANGED and WM_POSITIONCHANGING don't do enough to
// handle clipping of the overlay window on the Intel board,
// which keys on black.  Do this routine on WM_PAINT and
// WM_ENTERIDLE messages.

void CheckWindowMove(LPCAPSTREAM lpcs, HDC hdcWnd, BOOL fForce)
{
    UINT    wRgn;
    RECT    rc;
    DWORD   dwOrg;
    HDC     hdc;
    BOOL    f;

    if (!lpcs->hwnd || !lpcs->hVideoDisplay || !lpcs->fOverlayWindow)
        return;

    //
    //  when the screen is locked for update by a window move operation
    //  we dont want to turn off the video.
    //
    //  we can tell if the screen is locked by checking a DC to the screen.
    //
    hdc = GetDC(NULL);
    f = GetClipBox(hdc, &rc) == NULLREGION;
    ReleaseDC(NULL, hdc);

    if (f) {
        lpcs->uiRegion = (UINT) -1;
        return;
    }

    if (fForce)
        lpcs->uiRegion = (UINT) -1;

    hdc = GetDC (lpcs->hwnd);
    wRgn = GetClipBox(hdc, &rc);
    dwOrg = GetDCOrg(hdc);
    ReleaseDC(lpcs->hwnd, hdc);

    if (wRgn == lpcs->uiRegion &&
                dwOrg == lpcs->dwRegionOrigin &&
                EqualRect(&rc, &lpcs->rcRegionRect))
        return;

    lpcs->uiRegion       = wRgn;
    lpcs->dwRegionOrigin = dwOrg;
    lpcs->rcRegionRect   = rc;

    SetOverlayRectangles (lpcs);

    if (hdcWnd)
        videoUpdate (lpcs->hVideoDisplay, lpcs->hwnd, hdcWnd);
    else
        InvalidateRect (lpcs->hwnd, NULL, TRUE);
}

//
// Create our little world
//
LPCAPSTREAM CapWinCreate (HWND hwnd)
{
    LPCAPSTREAM lpcs;
    WAVEFORMATEX wfex;

    if (!(lpcs = (LPCAPSTREAM) GlobalAllocPtr (GHND, sizeof (CAPSTREAM))))
        return NULL;

    SetWindowLong (hwnd, GWL_CAPSTREAM, (LONG)lpcs);

    lpcs-> dwSize = sizeof (CAPSTREAM);
    lpcs-> uiVersion = CAPSTREAM_VERSION;
    lpcs-> hwnd = hwnd;
    lpcs-> hInst = ghInst;
    lpcs-> hWaitCursor = LoadCursor(NULL, IDC_WAIT);
    lpcs-> hdd = DrawDibOpen();
    lpcs-> fAudioHardware = !!waveOutGetNumDevs();    // force 1 or 0


    // Video defaults
    lpcs-> sCapParms.dwRequestMicroSecPerFrame = 66667;   // 15fps
    lpcs-> sCapParms.vKeyAbort          = VK_ESCAPE;
    lpcs-> sCapParms.fAbortLeftMouse    = TRUE;
    lpcs-> sCapParms.fAbortRightMouse   = TRUE;
    lpcs-> sCapParms.wNumVideoRequested = MIN_VIDEO_BUFFERS;
    lpcs-> fCapturingToDisk             = TRUE;
    lpcs-> sCapParms.wPercentDropForError = 10;   // error msg if dropped > 10%
    lpcs-> sCapParms.wChunkGranularity  = 2048;

    // Audio defaults to 11K, 8bit, Mono
    lpcs-> sCapParms.fCaptureAudio = lpcs-> fAudioHardware;
    lpcs-> sCapParms.wNumAudioRequested = DEF_WAVE_BUFFERS;

    wfex.wFormatTag = WAVE_FORMAT_PCM;
    wfex.nChannels = 1;
    wfex.nSamplesPerSec = 11025;
    wfex.nAvgBytesPerSec = 11025;
    wfex.nBlockAlign = 1;
    wfex.wBitsPerSample = 8;
    wfex.cbSize = 0;
    SendMessage (hwnd, WM_CAP_SET_AUDIOFORMAT, 0, (LONG)(LPVOID)&wfex);

    // Palette defaults
    lpcs-> nPaletteColors = 256;

    // Capture defaults
    lpcs-> sCapParms.fUsingDOSMemory = FALSE;
    lstrcpy (lpcs-> achFile, "C:\\CAPTURE.AVI");    // Default capture file
    lpcs->fCapFileExists = fileCapFileIsAVI (lpcs->achFile);

    // Allocate index to 32K frames plus proportionate number of audio chunks
    lpcs->sCapParms.dwIndexSize = (32768ul + (32768ul / 15));

    lpcs->sCapParms.fDisableWriteCache = TRUE;

    // Init the COMPVARS structure
    lpcs->CompVars.cbSize = sizeof (COMPVARS);
    lpcs->CompVars.dwFlags = 0;

    return lpcs;
}

//
// Destroy our little world
//
void CapWinDestroy (LPCAPSTREAM lpcs)
{
    // Uh, oh.  Somebodys trying to kill us while capturing
    if (lpcs->fCapturingNow && lpcs->fFrameCapturingNow) {
        // Single frame capture in progress
        SingleFrameCaptureClose (lpcs);
    }
    else if (lpcs->fCapturingNow) {
        // Streaming capture in progress, OR
        // MCI step capture in progress

        lpcs->fAbortCapture = TRUE;
        while (lpcs->fCapturingNow)
            Yield ();
    }

    if (lpcs->idTimer)
        KillTimer(lpcs->hwnd, lpcs->idTimer);

    PalFini (lpcs);
    DibFini (lpcs);

    CapWinDisconnectHardware (lpcs);

    DrawDibClose (lpcs->hdd);

    if (lpcs->lpWaveFormat)
        GlobalFreePtr (lpcs-> lpWaveFormat);

    if (lpcs->CompVars.hic)
        ICCompressorFree(&lpcs->CompVars);

    if (lpcs->lpInfoChunks)
        GlobalFreePtr(lpcs->lpInfoChunks);

    GlobalFreePtr (lpcs);       // Free the instance memory
}

WORD GetSizeOfWaveFormat (LPWAVEFORMATEX lpwf)
{
    WORD wSize;

    if (lpwf == NULL)
        return sizeof (PCMWAVEFORMAT);

    if (lpwf->wFormatTag == WAVE_FORMAT_PCM)
        wSize = sizeof (PCMWAVEFORMAT);
    else
        wSize = sizeof (WAVEFORMATEX) + lpwf -> cbSize;

    return wSize;
}

// Returns TRUE if we got a new frame, else FALSE
// if fForce, then always get a new frame
BOOL GetAFrameThenCallback (LPCAPSTREAM lpcs, BOOL fForce)
{
    BOOL fOK = FALSE;
    static BOOL fRecursion = FALSE;
    BOOL fVisible;
    RECT rc;
    HDC  hdc;

    if (fRecursion)
        return FALSE;

    if (!lpcs->sCapDrvCaps.fCaptureInitialized)
        return fOK;

    fRecursion = TRUE;

    // Update the preview window if we got a timer and not saving to disk
    if (lpcs->fOverlayWindow)
        CheckWindowMove(lpcs, NULL, FALSE);

    if ((!lpcs->fCapturingNow) || lpcs->fStepCapturingNow || lpcs->fFrameCapturingNow) {
        hdc = GetDC (lpcs->hwnd);
        fVisible = (GetClipBox (hdc, &rc) != NULLREGION);
        ReleaseDC (lpcs->hwnd, hdc);

        if (fForce || (fVisible && (lpcs->fLiveWindow || lpcs->CallbackOnVideoFrame))) {
            videoFrame (lpcs->hVideoIn, &lpcs->VidHdr );
            fOK = TRUE;

            if (lpcs->CallbackOnVideoFrame)
                (*(lpcs->CallbackOnVideoFrame)) (lpcs->hwnd, &lpcs->VidHdr);

            if (fForce || lpcs->fLiveWindow) {
                InvalidateRect (lpcs->hwnd, NULL, TRUE);
                UpdateWindow (lpcs->hwnd);
            }
        } // if visible
    } // if we're not streaming

    fRecursion = FALSE;

    return fOK;
}

// Clear the Status and Error strings via callback
void FAR PASCAL ClearStatusAndError (LPCAPSTREAM lpcs)
{
    statusUpdateStatus(lpcs, NULL);     // Clear status
    errorUpdateError(lpcs, NULL);       // Clear error

}

// Process class specific commands >= WM_USER

DWORD PASCAL ProcessCommandMessages (LPCAPSTREAM lpcs, unsigned msg, WORD wParam, LPARAM lParam)
{
    DWORD dwReturn = 0L;
    DWORD dwT;

    switch (msg) {
        // Don't clear status and error on the following innocuous msgs
        case WM_CAP_GET_CAPSTREAMPTR:
        case WM_CAP_GET_USER_DATA:
        case WM_CAP_DRIVER_GET_NAME:
        case WM_CAP_DRIVER_GET_VERSION:
        case WM_CAP_DRIVER_GET_CAPS:
        case WM_CAP_GET_AUDIOFORMAT:
        case WM_CAP_GET_VIDEOFORMAT:
        case WM_CAP_GET_STATUS:
        case WM_CAP_SET_SEQUENCE_SETUP:
        case WM_CAP_GET_SEQUENCE_SETUP:
        case WM_CAP_GET_MCI_DEVICE:
            break;

        default:
            ClearStatusAndError (lpcs);
            break;
    }

    switch (msg) {
    case WM_CAP_GET_CAPSTREAMPTR:
        // return a pointer to the CAPSTREAM
        return (DWORD) (LPVOID) lpcs;

    case WM_CAP_GET_USER_DATA:
	return lpcs->lUser;

    case WM_CAP_DRIVER_GET_NAME:
        // Return the name of the capture driver in use
        // wParam is the length of the buffer pointed to by lParam
        if (!lpcs->fHardwareConnected)
            return FALSE;
        return (capInternalGetDriverDesc (lpcs->sCapDrvCaps.wDeviceIndex,
                (LPSTR) lParam, (int) wParam, NULL, 0));

    case WM_CAP_DRIVER_GET_VERSION:
        // Return the version of the capture driver in use as text
        // wParam is the length of the buffer pointed to by lParam
        if (!lpcs->fHardwareConnected)
            return FALSE;
        return (capInternalGetDriverDesc (lpcs->sCapDrvCaps.wDeviceIndex,
                NULL, 0, (LPSTR) lParam, (int) wParam));

    case WM_CAP_DRIVER_GET_CAPS:
        // wParam is the size of the CAPDRIVERCAPS struct
        // lParam points to a CAPDRIVERCAPS struct
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (wParam <= sizeof (CAPDRIVERCAPS) &&
                !IsBadWritePtr ((LPVOID) lParam, (UINT) wParam)) {
            dwT = min (wParam, sizeof (CAPDRIVERCAPS));
            _fmemcpy ((LPVOID) lParam, (LPVOID) &lpcs-> sCapDrvCaps, (WORD) dwT);
            dwReturn = TRUE;
        }
        break;

    case WM_CAP_FILE_GET_CAPTURE_FILE:
        // wParam is the size
        // lParam points to a buffer in which capture file name is copied
        if (lParam) {
            lstrcpyn ((LPSTR) lParam, lpcs->achFile, wParam);
            dwReturn = TRUE;
        }
        break;

    case WM_CAP_GET_AUDIOFORMAT:
        // if lParam == NULL, return the size
        // if lParam != NULL, wParam is the size, return bytes copied
        if (lpcs->lpWaveFormat == NULL)
            return FALSE;
        dwT = GetSizeOfWaveFormat ((LPWAVEFORMATEX) lpcs->lpWaveFormat);
        if (lParam == NULL)
            return (dwT);
        else {
            if (wParam < (WORD) dwT)
                return FALSE;
            else {
                hmemcpy ((LPVOID) lParam, (LPVOID) lpcs->lpWaveFormat, dwT);
                dwReturn = dwT;
            }
        }
        break;

    case WM_CAP_GET_MCI_DEVICE:
        // wParam is the size
        // lParam points to a buffer in which capture file name is copied
        if (lParam) {
            lstrcpyn ((LPSTR) lParam, lpcs->achMCIDevice, wParam);
            dwReturn = TRUE;
        }
        break;

    case WM_CAP_GET_STATUS:
        // wParam is the size of the CAPSTATUS struct pointed to by lParam
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (IsBadWritePtr ((LPVOID) lParam, (UINT) wParam))
            return FALSE;

        if (wParam >= sizeof (CAPSTATUS)) {
            LPCAPSTATUS lpcc = (LPCAPSTATUS) lParam;

            lpcc-> fLiveWindow          = lpcs-> fLiveWindow;
            lpcc-> fOverlayWindow       = lpcs-> fOverlayWindow;
            lpcc-> fScale               = lpcs-> fScale;
            lpcc-> ptScroll             = lpcs-> ptScroll;
            lpcc-> fUsingDefaultPalette = lpcs-> fUsingDefaultPalette;
            lpcc-> fCapFileExists       = lpcs-> fCapFileExists;
            lpcc-> fAudioHardware       = lpcs-> fAudioHardware;
            lpcc-> uiImageWidth         = lpcs-> dxBits;
            lpcc-> uiImageHeight        = lpcs-> dyBits;

            // The following are updated dynamically during capture
            lpcc-> dwCurrentVideoFrame          = lpcs-> dwVideoChunkCount;
            lpcc-> dwCurrentVideoFramesDropped  = lpcs-> dwFramesDropped;
            if (lpcs->lpWaveFormat != NULL) {
                lpcc-> dwCurrentWaveSamples         =
                        muldiv32 (lpcs-> dwWaveBytes,
                                  lpcs-> lpWaveFormat-> nSamplesPerSec,
                                  lpcs-> lpWaveFormat-> nAvgBytesPerSec);
            }
            lpcc-> dwCurrentTimeElapsedMS       = lpcs-> dwTimeElapsedMS;

            // Added post alpha release
            lpcc-> fCapturingNow        = lpcs-> fCapturingNow;
            lpcc-> hPalCurrent          = lpcs-> hPalCurrent;
            lpcc-> dwReturn             = lpcs-> dwReturn;
            lpcc-> wNumVideoAllocated   = lpcs-> iNumVideo;
            lpcc-> wNumAudioAllocated   = lpcs-> iNumAudio;

            dwReturn = TRUE;
        }
        break;

    case WM_CAP_GET_SEQUENCE_SETUP:
        // wParam is sizeof CAPTUREPARMS
        // lParam = LPCAPTUREPARMS
        if (wParam <= sizeof (CAPTUREPARMS) &&
                !IsBadWritePtr ((LPVOID) lParam, (UINT) wParam)) {
            dwT = min (wParam, sizeof (CAPTUREPARMS));
            _fmemcpy ((LPVOID) lParam, (LPVOID) &lpcs->sCapParms, (WORD) dwT);
            dwReturn = TRUE;
        }
        break;

    case WM_CAP_STOP:
        // Stop capturing a sequence
        if (lpcs-> fCapturingNow) {
            lpcs-> fStopCapture = TRUE;
            dwReturn = TRUE;
        }
        break;

    case WM_CAP_ABORT:
        // Stop capturing a sequence
        if (lpcs-> fCapturingNow) {
            lpcs-> fAbortCapture = TRUE;
            dwReturn = TRUE;
        }
        break;

    case WM_CAP_GET_VIDEOFORMAT:
        // if lParam == NULL, return the size
        // if lParam != NULL, wParam is the size, return bytes copied
        if (!lpcs->fHardwareConnected)
            return FALSE;
        dwT = ((LPBITMAPINFOHEADER)lpcs->lpBitsInfo)-> biSize +
	      ((LPBITMAPINFOHEADER)lpcs->lpBitsInfo)->biClrUsed * sizeof(RGBQUAD);
        if (lParam == NULL)
            return dwT;
        else {
            if (wParam < (WORD) dwT)
                return FALSE;
            else {
                hmemcpy ((LPVOID) lParam, (LPVOID) lpcs->lpBitsInfo, dwT);
                dwReturn = dwT;
            }
        }
        break;

    case WM_CAP_SINGLE_FRAME_OPEN:
        // wParam is not used
        // lParam is not used
        if (!lpcs->fHardwareConnected)
            return FALSE;
        return SingleFrameCaptureOpen (lpcs);

    case WM_CAP_SINGLE_FRAME_CLOSE:
        // wParam is not used
        // lParam is not used
        if (!lpcs->fHardwareConnected)
            return FALSE;
        return SingleFrameCaptureClose (lpcs);

    case WM_CAP_SINGLE_FRAME:
        // wParam is not used
        // lParam is not used
        if (!lpcs->fHardwareConnected)
            return FALSE;
        return SingleFrameCapture (lpcs);

    case WM_CAP_SET_CALLBACK_STATUS:
        // Set the status callback proc
        if (lParam != NULL && IsBadCodePtr ((FARPROC) lParam))
            return FALSE;
        lpcs->CallbackOnStatus = (CAPSTATUSCALLBACK) lParam;
        return TRUE;

    case WM_CAP_SET_CALLBACK_ERROR:
        // Set the error callback proc
        if (lParam != NULL && IsBadCodePtr ((FARPROC) lParam))
            return FALSE;
        lpcs->CallbackOnError = (CAPERRORCALLBACK) lParam;
        return TRUE;

    case WM_CAP_SET_CALLBACK_FRAME:
        // Set the callback proc for single frame during preview
        if (lParam != NULL && IsBadCodePtr ((FARPROC) lParam))
            return FALSE;
        lpcs->CallbackOnVideoFrame = (CAPVIDEOCALLBACK) lParam;
        return TRUE;

    default:
        break;
    }

    // Once we start capturing, don't change anything
    if (lpcs-> fCapturingNow)
        return dwReturn;

    switch (msg) {

    case WM_CAP_SET_CALLBACK_YIELD:
        // Set the callback proc for wave buffer processing to net
        if (lParam != NULL && IsBadCodePtr ((FARPROC) lParam))
            return FALSE;
        lpcs->CallbackOnYield = (CAPYIELDCALLBACK) lParam;
        return TRUE;

    case WM_CAP_SET_CALLBACK_VIDEOSTREAM:
        // Set the callback proc for video buffer processing to net
        if (lParam != NULL && IsBadCodePtr ((FARPROC) lParam))
            return FALSE;
        lpcs->CallbackOnVideoStream = (CAPVIDEOCALLBACK) lParam;
        return TRUE;

    case WM_CAP_SET_CALLBACK_WAVESTREAM:
        // Set the callback proc for wave buffer processing to net
        if (lParam != NULL && IsBadCodePtr ((FARPROC) lParam))
            return FALSE;
        lpcs->CallbackOnWaveStream = (CAPWAVECALLBACK) lParam;
        return TRUE;

    case WM_CAP_SET_CALLBACK_CAPCONTROL:
        // Set the callback proc for frame accurate capture start/stop
        if (lParam != NULL && IsBadCodePtr ((FARPROC) lParam))
            return FALSE;
        lpcs->CallbackOnControl = (CAPCONTROLCALLBACK) lParam;
        return TRUE;

    case WM_CAP_SET_USER_DATA:
	lpcs->lUser = lParam;
	return TRUE;

    case WM_CAP_DRIVER_CONNECT:
        // Connect to a device
        // wParam contains the index of the driver in system.ini

        // If the same driver ID is requested, skip the request
        // Prevents multiple Inits from VB apps
        if (lpcs->fHardwareConnected &&
                (lpcs->sCapDrvCaps.wDeviceIndex == wParam))
            return TRUE;

        // First disconnect from any (possibly) existing device
        SendMessage (lpcs->hwnd, WM_CAP_DRIVER_DISCONNECT, 0, 0l);

        // and then connect to the new device
        if (CapWinConnectHardware (lpcs, (WORD) wParam /*wDeviceIndex*/)) {
            if (!DibGetNewFormatFromDriver (lpcs)) {  // Allocate our bitspace
                PalGetPaletteFromDriver (lpcs);
        	InvalidateRect(lpcs->hwnd, NULL, TRUE);
                lpcs->sCapDrvCaps.fCaptureInitialized = TRUE; // everything AOK!
                dwReturn = TRUE;
            }
        }
        break;

    case WM_CAP_DRIVER_DISCONNECT:
        MB ("About to disconnect from driver");
        // Disconnect from a device
        // wParam and lParam unused
        if (!lpcs->fHardwareConnected)
            return FALSE;
        CapWinDisconnectHardware (lpcs);
        DibFini (lpcs);
        PalFini (lpcs);
        InvalidateRect(lpcs->hwnd, NULL, TRUE);
        lpcs->sCapDrvCaps.fCaptureInitialized = FALSE;
        dwReturn = TRUE;
        break;

    case WM_CAP_FILE_SET_CAPTURE_FILE:
        // lParam points to the name of the capture file
        if (lParam) {
            BOOL fAlreadyExists;        // Don't create a file if new name
            OFSTRUCT of;
            HANDLE hFile;

            // Check for valid file names...
            if ((hFile = OpenFile ((LPSTR) lParam, &of, OF_WRITE)) == -1) {
                if ((hFile = OpenFile ((LPSTR) lParam, &of, OF_CREATE | OF_WRITE)) == -1)
                    return FALSE;
                fAlreadyExists = FALSE;
            }
            else
                fAlreadyExists = TRUE;

            _lclose (hFile);
            lstrcpyn (lpcs->achFile, (LPSTR) lParam, sizeof (lpcs->achFile));
            lpcs->fCapFileExists = fileCapFileIsAVI (lpcs->achFile);
            if (!fAlreadyExists)
                OpenFile ((LPSTR) lParam, &of, OF_DELETE);
            dwReturn = TRUE;
        }
        break;

    case WM_CAP_FILE_ALLOCATE:
        // lParam contains the size to preallocate the capture file in bytes
        return fileAllocCapFile(lpcs, lParam);

    case WM_CAP_FILE_SAVEAS:
        // lParam points to the name of the SaveAs file
        if (lParam) {
            lstrcpyn (lpcs->achSaveAsFile, (LPSTR) lParam,
                        sizeof (lpcs->achSaveAsFile));
            return (fileSaveCopy(lpcs));
        }
        break;

    case WM_CAP_FILE_SET_INFOCHUNK:
        // wParam is not used
        // lParam is an LPCAPINFOCHUNK
        if (lParam) {
            return (SetInfoChunk(lpcs, (LPCAPINFOCHUNK) lParam));
        }
        break;

    case WM_CAP_FILE_SAVEDIB:
        // lParam points to the name of the DIB file
        if (lParam) {
            if (lpcs-> fOverlayWindow)
                GetAFrameThenCallback (lpcs, TRUE /*fForce*/);

            return (fileSaveDIB(lpcs, (LPSTR)lParam));
        }
        break;

    case WM_CAP_EDIT_COPY:
        // Copy the current image and palette to the clipboard
        // wParam and lParam unused
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (lpcs-> fOverlayWindow)
            GetAFrameThenCallback (lpcs, TRUE /*fForce*/);

        if (lpcs->sCapDrvCaps.fCaptureInitialized && OpenClipboard (lpcs->hwnd)) {
            EmptyClipboard();

            // put a copy of the current palette in the clipboard
            if (lpcs->hPalCurrent && lpcs->lpBitsInfo->bmiHeader.biBitCount <= 8)
                SetClipboardData(CF_PALETTE, CopyPalette (lpcs->hPalCurrent));

            // make a packed DIB out of the current image
            if (lpcs-> lpBits && lpcs->lpBitsInfo ) {
                if (SetClipboardData (CF_DIB, CreatePackedDib (lpcs->lpBitsInfo,
                        lpcs-> lpBits, lpcs-> hPalCurrent)))
                    dwReturn = TRUE;
                else
                    errorUpdateError (lpcs, IDS_CAP_OUTOFMEM);
            }

            CloseClipboard();
        }
        break;

    case WM_CAP_SET_AUDIOFORMAT:
        {
            // wParam is unused
            // lParam is LPWAVEFORMAT or LPWAVEFORMATEX
            WORD wSize;
            LPWAVEFORMATEX lpwf = (LPWAVEFORMATEX) lParam;
            UINT uiError;

            // Verify the waveformat is valid
            uiError = waveInOpen((LPHWAVEIN)NULL,
                        (UINT)WAVE_MAPPER, lpwf,
                        NULL /*hWndCallback */, 0L,
                        WAVE_FORMAT_QUERY);

            if (uiError) {
                errorUpdateError (lpcs, IDS_CAP_WAVE_OPEN_ERROR);
                return FALSE;
            }

            if (lpcs->lpWaveFormat)
                GlobalFreePtr (lpcs-> lpWaveFormat);

            wSize = GetSizeOfWaveFormat (lpwf);
            if (lpcs-> lpWaveFormat = (LPWAVEFORMATEX)
                    GlobalAllocPtr (GHND, sizeof (CAPSTREAM))) {
                hmemcpy (lpcs->lpWaveFormat, lpwf, (LONG) wSize);
            }
            dwReturn = TRUE;
        }
        break;

    case WM_CAP_DLG_VIDEOSOURCE:
        // Show the dialog which controls the video source
        // NTSC vs PAL, input channel selection, etc.
        // wParam and lParam are unused
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (lpcs-> sCapDrvCaps.fHasDlgVideoSource) {
            videoDialog (lpcs->hVideoCapture, lpcs->hwnd, 0L );
            // Changing from NTSC to PAL could affect image dimensions!!!
            DibGetNewFormatFromDriver (lpcs);
            PalGetPaletteFromDriver (lpcs);

            // May need to inform parent of new layout here!
            InvalidateRect(lpcs->hwnd, NULL, TRUE);
            UpdateWindow(lpcs->hwnd);
        }
        return (lpcs-> sCapDrvCaps.fHasDlgVideoSource);

    case WM_CAP_DLG_VIDEOFORMAT:
        // Show the format dialog, user selects dimensions, depth, compression
        // wParam and lParam are unused
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (lpcs->sCapDrvCaps.fHasDlgVideoFormat) {
            videoDialog (lpcs->hVideoIn, lpcs->hwnd, 0L );
            DibGetNewFormatFromDriver (lpcs);
            PalGetPaletteFromDriver (lpcs);

            // May need to inform parent of new layout here!
            InvalidateRect(lpcs->hwnd, NULL, TRUE);
            UpdateWindow(lpcs->hwnd);
        }
        return (lpcs-> sCapDrvCaps.fHasDlgVideoFormat);

    case WM_CAP_DLG_VIDEODISPLAY:
        // Show the dialog which controls output.
        // This dialog only affects the presentation, never the data format
        // wParam and lParam are unused
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (lpcs->sCapDrvCaps.fHasDlgVideoDisplay)
            videoDialog (lpcs->hVideoDisplay, lpcs->hwnd, 0L);
        return (lpcs->sCapDrvCaps.fHasDlgVideoDisplay);

    case WM_CAP_DLG_VIDEOCOMPRESSION:
        // Show the dialog which selects video compression options.
        // wParam and lParam are unused
        if (!lpcs->fHardwareConnected)
            return FALSE;
        ICCompressorChoose(
                lpcs->hwnd,            // parent window for dialog
                ICMF_CHOOSE_KEYFRAME,  // want "key frame every" box
                lpcs->lpBitsInfo,      // input format (optional)
                NULL,                  // input data (optional)
                &lpcs->CompVars,       // data about the compressor/dlg
                NULL);                 // title bar (optional)
        return TRUE;

    case WM_CAP_SET_VIDEOFORMAT:
        // wParam is the size of the BITMAPINFO
        // lParam is an LPBITMAPINFO
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (IsBadReadPtr ((LPVOID) lParam, (UINT) wParam))
            return FALSE;

        return (DibNewFormatFromApp (lpcs, (LPBITMAPINFO) lParam, (WORD) wParam));

    case WM_CAP_SET_PREVIEW:
        // if wParam, enable preview via drawdib
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (wParam) {
            // turn off the overlay, if it is in use
            if (lpcs-> fOverlayWindow)
                SendMessage(lpcs->hwnd, WM_CAP_SET_OVERLAY, 0, 0L);
            lpcs->fLiveWindow = TRUE;
            statusUpdateStatus(lpcs, IDS_CAP_STAT_LIVE_MODE);
         } // endif enabling preview
         else {
            lpcs->fLiveWindow = FALSE;
        }
        InvalidateRect (lpcs->hwnd, NULL, TRUE);
        return TRUE;

    case WM_CAP_SET_OVERLAY:
        // if wParam, enable overlay in hardware
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (wParam && lpcs->sCapDrvCaps.fHasOverlay) {
            if (lpcs-> fLiveWindow)   // turn off preview mode
                SendMessage(lpcs->hwnd, WM_CAP_SET_PREVIEW, 0, 0L);
            lpcs->fOverlayWindow = TRUE;
            statusUpdateStatus(lpcs, IDS_CAP_STAT_OVERLAY_MODE);
        }
        else {
            lpcs->fOverlayWindow = FALSE;
            videoStreamFini (lpcs->hVideoDisplay); // disable overlay on hardware
        }
        InvalidateRect (lpcs->hwnd, NULL, TRUE);
        return (lpcs->sCapDrvCaps.fHasOverlay);

    case WM_CAP_SET_PREVIEWRATE:
        // wParam contains preview update rate in mS.
        // if wParam == 0 no timer is in use.
        if (lpcs->idTimer) {
            KillTimer(lpcs->hwnd, ID_PREVIEWTIMER);
            lpcs->idTimer = NULL;
        }
        if (wParam != 0) {
            lpcs->idTimer = SetTimer (lpcs->hwnd, ID_PREVIEWTIMER,
                        (UINT) wParam, NULL);
        }
        lpcs->uTimeout = (UINT) wParam;
        dwReturn = TRUE;
        break;

    case WM_CAP_GRAB_FRAME:
        // grab a single frame
        // wParam and lParam unused
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (lpcs->sCapDrvCaps.fCaptureInitialized) {

           dwReturn = (DWORD) GetAFrameThenCallback (lpcs, TRUE /*fForce*/);

           // disable live and overlay mode when capturing a single frame
           if (lpcs->fLiveWindow)
               SendMessage(lpcs->hwnd, WM_CAP_SET_PREVIEW, 0, 0L);
           else if (lpcs->fOverlayWindow)
               SendMessage(lpcs->hwnd, WM_CAP_SET_OVERLAY, 0, 0L);
        }
        break;

    case WM_CAP_GRAB_FRAME_NOSTOP:
        // grab a single frame, but don't change state of overlay/preview
        // wParam and lParam unused
        if (!lpcs->fHardwareConnected)
            return FALSE;
        dwReturn = (LONG) GetAFrameThenCallback (lpcs, TRUE /*fForce*/);
        break;

    case WM_CAP_SEQUENCE:
        // This is the main entry for streaming video capture
        // wParam is unused
        // lParam is unused
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (lpcs->sCapDrvCaps.fCaptureInitialized) {
            lpcs-> fCapturingToDisk = TRUE;
            return (AVICapture(lpcs));
        }
        break;

    case WM_CAP_SEQUENCE_NOFILE:
        // wParam is unused
        // lParam is unused
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (lpcs->sCapDrvCaps.fCaptureInitialized) {
            lpcs-> fCapturingToDisk = FALSE;
            return (AVICapture(lpcs));
        }
        break;

    case WM_CAP_SET_SEQUENCE_SETUP:
        // wParam is sizeof CAPTUREPARMS
        // lParam = LPCAPTUREPARMS
        // The following were added after the Beta, init in case the client
        // has a smaller structure and doesn't access them.
        lpcs->sCapParms.dwAudioBufferSize = 0;
        lpcs->sCapParms.fDisableWriteCache = TRUE;

        if (wParam <= sizeof (CAPTUREPARMS)) {
            dwT = min (sizeof (CAPTUREPARMS), wParam);
            if (IsBadReadPtr ((LPVOID) lParam, (UINT) dwT))
                break;

            _fmemcpy ((LPVOID) &lpcs->sCapParms, (LPVOID) lParam, (WORD) dwT);

            // Validate stuff that isn't handled elsewhere
            if (lpcs->sCapParms.wChunkGranularity < 16)
                lpcs->sCapParms.wChunkGranularity = 16;
            if (lpcs->sCapParms.wChunkGranularity > 16384)
                lpcs->sCapParms.wChunkGranularity = 16384;

            if (lpcs->sCapParms.fLimitEnabled && (lpcs->sCapParms.wTimeLimit == 0))
                lpcs->sCapParms.wTimeLimit = 1;

            // Force Step MCI off if not using MCI control
            if (lpcs->sCapParms.fStepMCIDevice && !lpcs->sCapParms.fMCIControl)
                    lpcs->sCapParms.fStepMCIDevice = FALSE;

            // Prevent audio capture if no audio hardware
            lpcs-> sCapParms.fCaptureAudio =
                lpcs-> fAudioHardware && lpcs-> sCapParms.fCaptureAudio;

            // Limit audio buffers
            lpcs-> sCapParms.wNumAudioRequested =
                min (MAX_WAVE_BUFFERS, lpcs->sCapParms.wNumAudioRequested);

            // Limit video buffers
            lpcs-> sCapParms.wNumVideoRequested =
                min (MAX_VIDEO_BUFFERS, lpcs->sCapParms.wNumVideoRequested);

            dwReturn = TRUE;
        }
        break;

    case WM_CAP_SET_MCI_DEVICE:
        // lParam points to the name of the capture file
        if (IsBadReadPtr ((LPVOID) lParam, 1))
            return FALSE;
        if (lParam) {
            lstrcpyn (lpcs->achMCIDevice, (LPSTR) lParam, sizeof (lpcs->achMCIDevice));
            dwReturn = TRUE;
        }
        break;

    case WM_CAP_SET_SCROLL:
        // lParam is an LPPOINT which contains the new scroll position
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (IsBadReadPtr ((LPVOID) lParam, sizeof (POINT)))
            return FALSE;
        {
            LPPOINT lpP = (LPPOINT) lParam;

            if (lpP->x < lpcs-> dxBits && lpP->y < lpcs-> dyBits) {
                lpcs->ptScroll = *lpP;
                InvalidateRect (lpcs->hwnd, NULL, TRUE);
                dwReturn = TRUE;
            }
        }
        break;

    case WM_CAP_SET_SCALE:
        // if wParam, Scale the window to the client region?
        if (!lpcs->fHardwareConnected)
            return FALSE;
        lpcs->fScale = (BOOL) wParam;
        return TRUE;

    case WM_CAP_PAL_OPEN:
        // Open a new palette
        // wParam is unused
        // lParam contains an LPSTR to the file
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (IsBadReadPtr ((LPVOID) lParam, 1))
            return FALSE;
        return fileOpenPalette(lpcs, (LPSTR) lParam /*lpszFileName*/);

    case WM_CAP_PAL_SAVE:
        // Save the current palette in a file
        // wParam is unused
        // lParam contains an LPSTR to the file
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (IsBadReadPtr ((LPVOID) lParam, 1))
            return FALSE;
        return fileSavePalette(lpcs, (LPSTR) lParam /*lpszFileName*/);

    case WM_CAP_PAL_AUTOCREATE:
        // Automatically capture a palette
        // wParam contains a count of the number of frames to average
        // lParam contains the number of colors desired in the palette
        if (!lpcs->fHardwareConnected)
            return FALSE;
        return CapturePaletteAuto (lpcs, (int) wParam, (int) lParam);

    case WM_CAP_PAL_MANUALCREATE:
        // Manually capture a palette
        // wParam contains TRUE for each frame to capture, FALSE when done
        // lParam contains the number of colors desired in the palette
        if (!lpcs->fHardwareConnected)
            return FALSE;
        return CapturePaletteManual (lpcs, (BOOL) wParam, (int) lParam);

    case WM_CAP_PAL_PASTE:
        // Paste a palette from the clipboard, send to the driver
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (lpcs->sCapDrvCaps.fCaptureInitialized && OpenClipboard(lpcs->hwnd)) {
            HANDLE  hPal;

            hPal = GetClipboardData(CF_PALETTE);
            CloseClipboard();
            if (hPal) {
                PalSendPaletteToDriver (lpcs, CopyPalette(hPal),  NULL /* XlateTable */);
                InvalidateRect(lpcs->hwnd, NULL, TRUE);
                dwReturn = TRUE;
            }
        }
        break;

    default:
        break;
    }
    return dwReturn;
}


/*--------------------------------------------------------------+
| ****************** THE WINDOW PROCEDURE ********************* |
+--------------------------------------------------------------*/
LONG FAR PASCAL _export _loadds CapWndProc (HWND hwnd, unsigned msg, WORD wParam, LONG lParam)
{
    LPCAPSTREAM lpcs;
    PAINTSTRUCT ps;
    HDC         hdc;
    int         f;
    MSG         PMsg;

    lpcs = (LPCAPSTREAM) GetWindowLong (hwnd, GWL_CAPSTREAM);

    if (msg >= WM_CAP_START && msg <= WM_CAP_END)
        return (ProcessCommandMessages (lpcs, msg, wParam, lParam));

    switch (msg) {

    case WM_CREATE:
        lpcs = CapWinCreate (hwnd);
        break;

    case WM_TIMER:
        // Update the preview window if we got a timer and not saving to disk
        GetAFrameThenCallback (lpcs, FALSE /*fForce*/);

        // Added VFW 1.1b, Clear the queue of additional timer msgs???
        // Trying to correct "Hit OK to continue" dialog not appearing bug
        // due to app message queue continuously being full at large
        // image dimensions.
        PeekMessage (&PMsg, hwnd, WM_TIMER, WM_TIMER,PM_REMOVE|PM_NOYIELD);
        break;

    case WM_CLOSE:
        break;

    case WM_DESTROY:
        CapWinDestroy (lpcs);
        break;

    case WM_PALETTECHANGED:
        if (lpcs->hdd == NULL)
            break;

        hdc = GetDC(hwnd);
        if (f = DrawDibRealize(lpcs->hdd, hdc, TRUE /*fBackground*/))
            InvalidateRect(hwnd,NULL,TRUE);
        ReleaseDC(hwnd,hdc);
        return f;

    case WM_QUERYNEWPALETTE:
        if (lpcs->hdd == NULL)
            break;
        hdc = GetDC(hwnd);
        f = DrawDibRealize(lpcs->hdd, hdc, FALSE);
        ReleaseDC(hwnd, hdc);

        if (f)
            InvalidateRect(hwnd, NULL, TRUE);
        return f;

    case WM_SIZE:
    case WM_MOVE:
        if (lpcs->fOverlayWindow)    // Make the driver paint the key color
            InvalidateRect(hwnd, NULL, TRUE);
        break;

    case WM_WINDOWPOSCHANGED:
        if (lpcs->fOverlayWindow)    // Make the driver paint the key color
            InvalidateRect(hwnd, NULL, TRUE);
        return 0;

    case WM_ERASEBKGND:
        return 0;  // don't bother to erase it

    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        if (lpcs->fOverlayWindow) {
            CheckWindowMove(lpcs, ps.hdc, TRUE);
        }
        else {
            SetWindowOrg(hdc, lpcs->ptScroll.x, lpcs->ptScroll.y);
            DibPaint(lpcs, hdc);
        }
        EndPaint(hwnd, &ps);
        break;

    default:
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

#if 0
void dummyTest ()
{
    HWND hwnd;
    FARPROC fpProc;
    DWORD dwSize;
    WORD wSize;
    BOOL f;
    int i;
    char szName[80];
    char szVer[80];
    DWORD dwMS;
    int iFrames, iColors;
    char s;
    LPPOINT lpP;

    capSetCallbackOnError(hwnd, fpProc);
    capSetCallbackOnStatus(hwnd, fpProc);
    capSetCallbackOnYield(hwnd, fpProc);
    capSetCallbackOnFrame(hwnd, fpProc);
    capSetCallbackOnVideoStream(hwnd, fpProc);
    capSetCallbackOnWaveStream(hwnd, fpProc);

    capDriverConnect(hwnd, i);
    capDriverDisconnect(hwnd);
    capDriverGetName(hwnd, szName, wSize);
    capDriverGetVersion(hwnd, szVer, wSize);
    capDriverGetCaps(hwnd, s, wSize);

    capFileSetCaptureFile(hwnd, szName);
    capFileGetCaptureFile(hwnd, szName, wSize);
    capFileAlloc(hwnd, dwSize);
    capFileSaveAs(hwnd, szName);

    capEditCopy(hwnd);

    capSetAudioFormat(hwnd, s, wSize);
    capGetAudioFormat(hwnd, s, wSize);
    capGetAudioFormatSize(hwnd);

    capDlgVideoFormat(hwnd);
    capDlgVideoSource(hwnd);
    capDlgVideoDisplay(hwnd);

    capPreview(hwnd, f);
    capPreviewRate(hwnd, dwMS);
    capOverlay(hwnd, f);
    capPreviewScale(hwnd, f);
    capGetStatus(hwnd, s, wSize);
    capSetScrollPos(hwnd, lpP);

    capGrabFrame(hwnd);
    capGrabFrameNoStop(hwnd);
    capCaptureSequence(hwnd);
    capCaptureSequenceNoFile(hwnd);
    capCaptureGetSetup(hwnd, s, wSize);
    capCaptureSetSetup(hwnd, s, wSize);

    capCaptureSingleFrameOpen(hwnd);
    capCaptureSingleFrameClose(hwnd);
    capCaptureSingleFrame(hwnd);

    capSetMCIDeviceName(hwnd, szName);
    capGetMCIDeviceName(hwnd, szName, wSize);

    capPalettePaste(hwnd);
    capPaletteAuto(hwnd, iFrames, iColors);
}

#endif



