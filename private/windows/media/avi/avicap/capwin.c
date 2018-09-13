/****************************************************************************
 *
 *   capwin.c
 *
 *   Main window proceedure.
 *
 *   Microsoft Video for Windows Sample Capture Class
 *
 *   Copyright (c) 1992 - 1995 Microsoft Corporation.  All Rights Reserved.
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
#include <mmreg.h>
#include <memory.h>

#include "ivideo32.h"
#include "avicap.h"
#include "avicapi.h"
#include "cappal.h"
#include "capdib.h"
#include "dibmap.h"

#ifdef UNICODE
#include <stdlib.h>
#endif

// GetWindowLong assignments
#define GWL_CAPSTREAM   0
#define GWL_CAPVBSTATUS 4       // Used by VB Status callback
#define GWL_CAPVBERROR  8       // Used by VB Error callback
#define GWL_CAP_SPARE1  12      // Room to grow
#define GWL_CAP_SPARE2  16      // Room to grow

#define ID_PREVIEWTIMER 9

//#ifdef _DEBUG
#ifdef PLASTIQUE
    #define MB(lpsz) MessageBoxA(NULL, lpsz, "", MB_OK);
#else
    #define MB(lpsz)
#endif


// #if defined _WIN32 && defined CHICAGO
#if defined NO_LONGER_USED

#include <mmdevldr.h>
#include <vmm.h>
#include "mmdebug.h"

#pragma message (SQUAWK "move these defines later")
#define MMDEVLDR_IOCTL_PAGEALLOCATE  7
#define MMDEVLDR_IOCTL_PAGEFREE      8
#define PageContig      0x00000004
#define PageFixed       0x00000008
//end

HANDLE hMMDevLdr = NULL;

/*****************************************************************************

  @doc INTERNAL

  @function HANDLE | OpenMMDEVLDR | Open a file handle to the MMDEVLDR VxD
  in order to access the DeviceIoControl functions.

  @rdesc opens a shared handle to MMDEVLDR

*****************************************************************************/

VOID WINAPI OpenMMDEVLDR(
    void)
{
    AuxDebugEx (5, DEBUGLINE "OpenMMDEVLDR()r\n");

    if (hMMDevLdr)
        return;

    hMMDevLdr = CreateFile(
        "\\\\.\\MMDEVLDR.VXD", // magic name to attach to an already loaded vxd
        GENERIC_WRITE,
        FILE_SHARE_WRITE,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_GLOBAL_HANDLE,
        NULL);

    AuxDebugEx (5, DEBUGLINE "OpenMMDEVLDR returns %08Xr\n", hMMDevLdr);
    return;
}

VOID WINAPI CloseMMDEVLDR(
    void)
{
    if (! hMMDevLdr)
        return;

    CloseHandle (hMMDevLdr);
    hMMDevLdr = NULL;
}

/*****************************************************************************

  @doc INTERNAL

  @function DWORD | LinPageLock | Call the VMM service LinPageLock via
   DeviceIoControl through MMDEVLDR.

  @parm DWORD | dwStartPage | Starting page of the linear region to lock.

  @parm DWORD | dwPageCount | Number of 4K pages to lock.

  @parm DWORD | fdwLinPageLock | Flags expected by the VMM service.
   @flag PAGEMAPGLOBAL | Return an alias to the locked region which
    is valid in all process contexts.

  @rdesc Meaningless unless PAGEMAPGLOBAL specified. If it was, then the
   return value is the alias pointer to the start of the linear region
   (NOTE: A *POINTER*, NOT a page address). The pointer will be page
   aligned (i.e. the low 12 bits will be zero.)

*****************************************************************************/

DWORD WINAPI LinPageLock(
    DWORD           dwStartPage,
    DWORD           dwPageCount,
    DWORD           fdwLinPageLock)
{
    LOCKUNLOCKPARMS lup;
    DWORD           dwRet;
    DWORD           cbRet;

    AuxDebugEx (6, DEBUGLINE "LinPageLock(%08x,%08x,%08x)\r\n",
                 dwStartPage, dwPageCount, fdwLinPageLock);

    assert (hMMDevLdr != NULL);
    if (INVALID_HANDLE_VALUE == hMMDevLdr)
        return 0;

    lup.dwStartPage = dwStartPage;
    lup.dwPageCount = dwPageCount;
    lup.fdwOperation= fdwLinPageLock;


    if ( ! DeviceIoControl (hMMDevLdr,
                            MMDEVLDR_IOCTL_LINPAGELOCK,
                            &lup,
                            sizeof(lup),
                            &dwRet,
                            sizeof(dwRet),
                            &cbRet,
                            NULL))
    {
        AuxDebug("LinPageLock failed!!!");
        dwRet = 0;
    }

    return dwRet;
}

/*****************************************************************************

  @doc INTERNAL

  @function DWORD | LinPageUnLock | Call the VMM service LinPageUnLock via
   DeviceIoControl through MMDEVLDR.

  @parm DWORD | dwStartPage | Starting page of the linear region to unlock.

  @parm DWORD | dwPageCount | Number of 4K pages to lock.

  @parm DWORD | fdwLinPageLock | Flags expected by the VMM service.
   @flag PAGEMAPGLOBAL | Return an alias to the locked region which
    is valid in all process contexts.

  @comm
   If PAGEMAPGLOBAL was specified on the <f LinPageLock> call, it must
   also be specified here. In this case, <p dwStartPage> should be the
   page address of the returned alias pointer in global memory.

*****************************************************************************/

void WINAPI LinPageUnLock(
    DWORD           dwStartPage,
    DWORD           dwPageCount,
    DWORD           fdwLinPageLock)
{
    LOCKUNLOCKPARMS lup;

    AuxDebugEx (6, DEBUGLINE "LinPageUnLock (%08x,%08x,%08x)\r\n",
                dwStartPage, dwPageCount, fdwLinPageLock);

    assert (hMMDevLdr != NULL);
    assert (INVALID_HANDLE_VALUE != hMMDevLdr);
    if (INVALID_HANDLE_VALUE == hMMDevLdr)
        return;

    lup.dwStartPage = dwStartPage;
    lup.dwPageCount = dwPageCount;
    lup.fdwOperation = fdwLinPageLock;

    DeviceIoControl (hMMDevLdr,
                     MMDEVLDR_IOCTL_LINPAGEUNLOCK,
                     &lup,
                     sizeof(lup),
                     NULL,
                     0,
                     NULL,
                     NULL);
}

/*+ FreeContigMem
 *
 *-==================================================================*/

VOID FreeContigMem (
    DWORD hMemContig)
{
    DWORD dwRet;
    DWORD cbRet;

    assert (hMMDevLdr != NULL);
    assert (INVALID_HANDLE_VALUE != hMMDevLdr);
    if (INVALID_HANDLE_VALUE == hMMDevLdr)
        return;

    DeviceIoControl (hMMDevLdr,
                     MMDEVLDR_IOCTL_PAGEFREE,
                     &hMemContig,
                     sizeof(hMemContig),
                     &dwRet,
                     sizeof(dwRet),
                     &cbRet,
                     NULL);
}

/*+ AllocContigMem
 *
 *-==================================================================*/

LPVOID AllocContigMem (
    DWORD   cbSize,
    LPDWORD phMemContig)
{
    struct _memparms {
       DWORD flags;
       DWORD nPages;
       } mp;
    struct _memret {
       LPVOID lpv;
       DWORD  hMem;
       DWORD  nPages;
       DWORD  dwPhys;
       } mr;
    DWORD  cbRet;

    mr.lpv = NULL;
    *phMemContig = 0;

    mp.nPages = (cbSize + 4095) >> 12;
    mp.flags = PageContig+PageFixed;

    AuxDebugEx (2, DEBUGLINE "Contig allocate %08X pages\r\n", mp.nPages);

    assert (hMMDevLdr != NULL);
    assert (INVALID_HANDLE_VALUE != hMMDevLdr);
    if (INVALID_HANDLE_VALUE == hMMDevLdr)
        return NULL;

    if ( ! DeviceIoControl (hMMDevLdr,
                            MMDEVLDR_IOCTL_PAGEALLOCATE,
                            &mp,
                            sizeof(mp),
                            &mr,
                            sizeof(mr),
                            &cbRet,
                            NULL))
    {
        AuxDebugEx(0, "Contig Allocate failed!!!\r\n");
        mr.lpv = NULL;
        mr.hMem = 0;
        mr.nPages = 0;
        mr.dwPhys = 0;
    }

    *phMemContig = mr.hMem;

    AuxDebugEx(2, "Contig Allocate returns %08X\r\n", mr.lpv);
    return mr.lpv;
}

/*+
 *
 *-================================================================*/

PVOID WINAPI CreateGlobalAlias (
    PVOID   pOriginal,
    DWORD   cbOriginal,
    LPDWORD pnPages)
{
    DWORD   dwStartPage;
    DWORD   dwPageCount;
    DWORD   dwPageOffset;
    DWORD   dwAliasBase;
    PVOID   pAlias;

    AuxDebugEx (6, DEBUGLINE "CreateGlobalAlias(%08X,%08X,..)\r\n",
                pOriginal, cbOriginal);

    dwStartPage  = ((DWORD)pOriginal) >> 12;
    dwPageOffset = ((DWORD)pOriginal) & ((1 << 12)-1);
    dwPageCount  = ((((DWORD)pOriginal) + cbOriginal - 1) >> 12) - dwStartPage + 1;

    *pnPages = 0;
    dwAliasBase = LinPageLock (dwStartPage, dwPageCount, PAGEMAPGLOBAL);
    if ( ! dwAliasBase)
        return NULL;

    pAlias = (PVOID)(dwAliasBase + dwPageOffset);
    *pnPages = dwPageCount;

    AuxDebugEx (6, DEBUGLINE "CreateGlobalAlias returns %08X nPages %d\r\n", pAlias, dwPageCount);
    return pAlias;
}

/*+
 *
 *-================================================================*/

VOID WINAPI FreeGlobalAlias(
    PVOID        pAlias,
    DWORD        nPages)
{
    AuxDebugEx (6, DEBUGLINE "FreeGlobalAlias(%08X,%08X)\r\n", pAlias, nPages);

    LinPageUnLock (((DWORD)pAlias) >> 12, nPages, PAGEMAPGLOBAL);
}
#endif


#if defined _WIN32 && defined CHICAGO


/*+ videoFrame
 *
 *-================================================================*/

DWORD WINAPI videoFrame (
    HVIDEO hVideo,
    LPVIDEOHDR lpVHdr)
{
    return vidxFrame (hVideo, lpVHdr);
}

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

        videoSetRect (lpcs->hVideoDisplay, DVM_DST_RECT, rc);

        // Overlay channel Source rectangle
        SetRect (&rc, lpcs->ptScroll.x, lpcs->ptScroll.y,
                lpcs->ptScroll.x + rc.right - rc.left,
                lpcs->ptScroll.y + rc.bottom - rc.top);
        videoSetRect (lpcs->hVideoDisplay, DVM_SRC_RECT, rc);

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
#ifdef _WIN32
    POINT   ptOrg;
#else
    DWORD   dwOrg;
#endif
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
#ifdef _WIN32
    GetDCOrgEx(hdc, &ptOrg);
#else
    dwOrg = GetDCOrg(hdc);
#endif
    ReleaseDC(lpcs->hwnd, hdc);

    if (wRgn == lpcs->uiRegion &&
#ifdef _WIN32
                ptOrg.x == lpcs->ptRegionOrigin.x &&
		ptOrg.y == lpcs->ptRegionOrigin.y &&
#else
                dwOrg == lpcs->dwRegionOrigin &&
#endif
                EqualRect(&rc, &lpcs->rcRegionRect))
        return;

    lpcs->uiRegion       = wRgn;
#ifdef _WIN32
    lpcs->ptRegionOrigin = ptOrg;
#else
    lpcs->dwRegionOrigin = dwOrg;
#endif

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

    lpcs->dwSize = sizeof (CAPSTREAM);
    lpcs->uiVersion = CAPSTREAM_VERSION;
    lpcs->hwnd = hwnd;
    lpcs->hInst = ghInstDll;
    lpcs->hWaitCursor = LoadCursor(NULL, IDC_WAIT);
    lpcs->hdd = DrawDibOpen();
    lpcs->fAudioHardware = !!waveOutGetNumDevs();    // force 1 or 0


    // Video defaults
    lpcs->sCapParms.dwRequestMicroSecPerFrame = 66667;   // 15fps
    lpcs->sCapParms.vKeyAbort          = VK_ESCAPE;
    lpcs->sCapParms.fAbortLeftMouse    = TRUE;
    lpcs->sCapParms.fAbortRightMouse   = TRUE;
    lpcs->sCapParms.wNumVideoRequested = MIN_VIDEO_BUFFERS;
    lpcs->sCapParms.wPercentDropForError = 10;   // error msg if dropped > 10%
    lpcs->sCapParms.wChunkGranularity  = 0;
    lpcs->fCaptureFlags |= CAP_fCapturingToDisk;

    // Audio defaults to 11K, 8bit, Mono
    lpcs->sCapParms.fCaptureAudio = lpcs->fAudioHardware;
    lpcs->sCapParms.wNumAudioRequested = DEF_WAVE_BUFFERS;

    wfex.wFormatTag = WAVE_FORMAT_PCM;
    wfex.nChannels = 1;
    wfex.nSamplesPerSec = 11025;
    wfex.nAvgBytesPerSec = 11025;
    wfex.nBlockAlign = 1;
    wfex.wBitsPerSample = 8;
    wfex.cbSize = 0;
    SendMessage (hwnd, WM_CAP_SET_AUDIOFORMAT, 0, (LONG)(LPVOID)&wfex);

    // Palette defaults
    lpcs->nPaletteColors = 256;

    // Capture defaults
    lpcs->sCapParms.fUsingDOSMemory = FALSE;
    lstrcpy (lpcs->achFile, TEXT("C:\\CAPTURE.AVI"));    // Default capture file
    lpcs->fCapFileExists = fileCapFileIsAVI (lpcs->achFile);

    // Allocate index to 32K frames plus proportionate number of audio chunks
    lpcs->sCapParms.dwIndexSize = (32768ul + (32768ul / 15));
    lpcs->sCapParms.fDisableWriteCache = FALSE;

#ifdef NEW_COMPMAN
    // Init the COMPVARS structure
    lpcs->CompVars.cbSize = sizeof (COMPVARS);
    lpcs->CompVars.dwFlags = 0;
#endif

    return lpcs;
}

//
// Destroy our little world
//
void CapWinDestroy (LPCAPSTREAM lpcs)
{
    // Uh, oh.  Somebodys trying to kill us while capturing
    if (lpcs->fCaptureFlags & CAP_fCapturingNow) {
	if (lpcs->fCaptureFlags & CAP_fFrameCapturingNow) {
	    // Single frame capture in progress
	    SingleFrameCaptureClose (lpcs);
	}
	else {
	    // Streaming capture in progress, OR
	    // MCI step capture in progress

	    lpcs->fCaptureFlags |= CAP_fAbortCapture;
#ifdef _WIN32
	    // wait for capture thread to go away

	    // we must have a capture thread
	    WinAssert(lpcs->hThreadCapture != 0);
	    while (MsgWaitForMultipleObjects(1, &lpcs->hThreadCapture, FALSE,
		INFINITE, QS_SENDMESSAGE) != WAIT_OBJECT_0) {
		MSG msg;

		// just a single peekmessage with NOREMOVE will
		// process the inter-thread send and not affect the queue
		PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
	    }
	    CloseHandle(lpcs->hThreadCapture);
	    lpcs->hThreadCapture = 0;

	    // it should have stopped capturing
	    WinAssert(!(lpcs->fCaptureFlags & CAP_fCapturingNow));

#else
	    while (lpcs->fCapturingNow)
		Yield ();
#endif
    	}
    }

    if (lpcs->idTimer)
        KillTimer(lpcs->hwnd, lpcs->idTimer);

    PalFini (lpcs);
    DibFini (lpcs);

    CapWinDisconnectHardware (lpcs);

    DrawDibClose (lpcs->hdd);

    if (lpcs->lpWaveFormat)
        GlobalFreePtr (lpcs->lpWaveFormat);

#ifdef NEW_COMPMAN
    if (lpcs->CompVars.hic)
        ICCompressorFree(&lpcs->CompVars);
#endif

    if (lpcs->lpInfoChunks)
        GlobalFreePtr(lpcs->lpInfoChunks);

    WinAssert (!lpcs->pAsync);
    GlobalFreePtr (lpcs);       // Free the instance memory
}

UINT GetSizeOfWaveFormat (LPWAVEFORMATEX lpwf)
{
    UINT wSize;

    if ((lpwf == NULL) || (lpwf->wFormatTag == WAVE_FORMAT_PCM))
        wSize = sizeof (PCMWAVEFORMAT);
    else
        wSize = sizeof (WAVEFORMATEX) + lpwf->cbSize;

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

    if ((!(lpcs->fCaptureFlags & CAP_fCapturingNow))
       || (lpcs->fCaptureFlags & CAP_fStepCapturingNow)
       || (lpcs->fCaptureFlags & CAP_fFrameCapturingNow)) {
        hdc = GetDC (lpcs->hwnd);
        fVisible = (GetClipBox (hdc, &rc) != NULLREGION);
        ReleaseDC (lpcs->hwnd, hdc);

        if (fForce || (fVisible && (lpcs->fLiveWindow || lpcs->CallbackOnVideoFrame))) {
            videoFrame (lpcs->hVideoIn, &lpcs->VidHdr);
            fOK = TRUE;

            if (lpcs->CallbackOnVideoFrame)
                lpcs->CallbackOnVideoFrame(lpcs->hwnd, &lpcs->VidHdr);

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
__inline void FAR PASCAL ClearStatusAndError (LPCAPSTREAM lpcs)
{
    statusUpdateStatus(lpcs, 0);     // Clear status
    errorUpdateError(lpcs, 0);       // Clear error

}

// Process class specific commands >= WM_USER

DWORD PASCAL ProcessCommandMessages (LPCAPSTREAM lpcs, UINT msg, UINT wParam, LPARAM lParam)
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
        case WM_CAP_SET_PREVIEWRATE:
        case WM_CAP_SET_SCROLL:
#ifdef UNICODE
        // ...or on the ansi thunks for these messages
        case WM_CAP_DRIVER_GET_NAMEA:
        case WM_CAP_DRIVER_GET_VERSIONA:
        case WM_CAP_GET_MCI_DEVICEA:
#endif
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

    // unicode and win-16 version - see ansi thunk below
    case WM_CAP_DRIVER_GET_NAME:
        // Return the name of the capture driver in use
        // wParam is the length of the buffer pointed to by lParam
        if (!lpcs->fHardwareConnected)
            return FALSE;
        return (capInternalGetDriverDesc (lpcs->sCapDrvCaps.wDeviceIndex,
                (LPTSTR) lParam, (int) wParam, NULL, 0));

    // unicode and win-16 version - see ansi thunk below
    case WM_CAP_DRIVER_GET_VERSION:
        // Return the version of the capture driver in use as text
        // wParam is the length of the buffer pointed to by lParam
        if (!lpcs->fHardwareConnected)
            return FALSE;
        return (capInternalGetDriverDesc (lpcs->sCapDrvCaps.wDeviceIndex,
                NULL, 0, (LPTSTR) lParam, (int) wParam));

#ifdef UNICODE
    // ansi/unicode thunk versions of the above entrypoint
    case WM_CAP_DRIVER_GET_NAMEA:
        if (!lpcs->fHardwareConnected)
            return FALSE;
        return capInternalGetDriverDescA(lpcs->sCapDrvCaps.wDeviceIndex,
                (LPSTR) lParam, (int) wParam, NULL, 0);

    // ansi/unicode thunk versions of the above entrypoint
    case WM_CAP_DRIVER_GET_VERSIONA:
        if (!lpcs->fHardwareConnected)
            return FALSE;
        return capInternalGetDriverDescA(lpcs->sCapDrvCaps.wDeviceIndex,
                NULL, 0, (LPSTR) lParam, (int) wParam);
#endif


    case WM_CAP_DRIVER_GET_CAPS:
        // wParam is the size of the CAPDRIVERCAPS struct
        // lParam points to a CAPDRIVERCAPS struct
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (wParam <= sizeof (CAPDRIVERCAPS) &&
                !IsBadWritePtr ((LPVOID) lParam, (UINT) wParam)) {
            dwT = min (wParam, sizeof (CAPDRIVERCAPS));
            _fmemcpy ((LPVOID) lParam, (LPVOID) &lpcs->sCapDrvCaps, (UINT) dwT);
            dwReturn = TRUE;
        }
        break;


    // unicode and win-16 version - see ansi thunk below
    case WM_CAP_FILE_GET_CAPTURE_FILE:
        // wParam is the size (in characters)
        // lParam points to a buffer in which capture file name is copied
        if (lParam) {
            lstrcpyn ((LPTSTR) lParam, lpcs->achFile, wParam);
            dwReturn = TRUE;
        }
        break;
#ifdef UNICODE
    // ansi/unicode thunk
    case WM_CAP_FILE_GET_CAPTURE_FILEA:
        if (lParam) {
            Iwcstombs((LPSTR) lParam, lpcs->achFile, (int) wParam);
            dwReturn = TRUE;
        }
        break;
#endif


    case WM_CAP_GET_AUDIOFORMAT:
        // if lParam == NULL, return the size
        // if lParam != NULL, wParam is the size, return bytes copied
        if (lpcs->lpWaveFormat == NULL)
            return FALSE;
        dwT = GetSizeOfWaveFormat ((LPWAVEFORMATEX) lpcs->lpWaveFormat);
        if (lParam == 0)
            return (dwT);
        else {
            if (wParam < (UINT) dwT)
                return FALSE;
            else {
                hmemcpy ((LPVOID) lParam, (LPVOID) lpcs->lpWaveFormat, dwT);
                dwReturn = dwT;
            }
        }
        break;

    // unicode and win-16 version - see ansi thunk below
    case WM_CAP_GET_MCI_DEVICE:
        // wParam is the size in characters
        // lParam points to a buffer in which MCI device name is copied
        if (lParam) {
            lstrcpyn ((LPTSTR) lParam, lpcs->achMCIDevice, wParam);
            dwReturn = TRUE;
        }
        break;
#ifdef UNICODE
    // ansi thunk of above
    case WM_CAP_GET_MCI_DEVICEA:
        if (lParam) {
            Iwcstombs( (LPSTR) lParam, lpcs->achMCIDevice, (int) wParam);
            dwReturn = TRUE;
        }
        break;
#endif

    case WM_CAP_GET_STATUS:
        // wParam is the size of the CAPSTATUS struct pointed to by lParam
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (IsBadWritePtr ((LPVOID) lParam, (UINT) wParam))
            return FALSE;

        if (wParam >= sizeof (CAPSTATUS)) {
            LPCAPSTATUS lpcc = (LPCAPSTATUS) lParam;

            lpcc->fLiveWindow          = lpcs->fLiveWindow;
            lpcc->fOverlayWindow       = lpcs->fOverlayWindow;
            lpcc->fScale               = lpcs->fScale;
            lpcc->ptScroll             = lpcs->ptScroll;
            lpcc->fUsingDefaultPalette = lpcs->fUsingDefaultPalette;
            lpcc->fCapFileExists       = lpcs->fCapFileExists;
            lpcc->fAudioHardware       = lpcs->fAudioHardware;
            lpcc->uiImageWidth         = lpcs->dxBits;
            lpcc->uiImageHeight        = lpcs->dyBits;

            // The following are updated dynamically during capture
            lpcc->dwCurrentVideoFrame          = lpcs->dwVideoChunkCount;
            lpcc->dwCurrentVideoFramesDropped  = lpcs->dwFramesDropped;
            if (lpcs->lpWaveFormat != NULL) {
            lpcc->dwCurrentWaveSamples         =
                  MulDiv (lpcs->dwWaveBytes,
                          lpcs->lpWaveFormat->nSamplesPerSec,
                          lpcs->lpWaveFormat->nAvgBytesPerSec);
            }
            lpcc->dwCurrentTimeElapsedMS       = lpcs->dwTimeElapsedMS;

            // Added post alpha release
	    if (lpcs->fCaptureFlags & CAP_fCapturingNow) {
		lpcc->fCapturingNow    = TRUE;
	    } else {
		lpcc->fCapturingNow    = FALSE;
	    }
            lpcc->hPalCurrent          = lpcs->hPalCurrent;
            lpcc->dwReturn             = lpcs->dwReturn;
            lpcc->wNumVideoAllocated   = lpcs->iNumVideo;
            lpcc->wNumAudioAllocated   = lpcs->iNumAudio;

            dwReturn = TRUE;
        }
        break;

    case WM_CAP_GET_SEQUENCE_SETUP:
        // wParam is sizeof CAPTUREPARMS
        // lParam = LPCAPTUREPARMS
        if (wParam <= sizeof (CAPTUREPARMS) &&
                !IsBadWritePtr ((LPVOID) lParam, (UINT) wParam)) {
            dwT = min (wParam, sizeof (CAPTUREPARMS));
            _fmemcpy ((LPVOID) lParam, (LPVOID) &lpcs->sCapParms, (UINT) dwT);
            dwReturn = TRUE;
        }
        break;

    case WM_CAP_STOP:
        // Stop capturing a sequence
        if (lpcs->fCaptureFlags & CAP_fCapturingNow) {
            lpcs->fCaptureFlags |= CAP_fStopCapture;
            dwReturn = TRUE;
        }
        break;

    case WM_CAP_ABORT:
        // Stop capturing a sequence
        if (lpcs->fCaptureFlags & CAP_fCapturingNow) {
            lpcs->fCaptureFlags |= CAP_fAbortCapture;
            dwReturn = TRUE;
        }
        break;

    case WM_CAP_GET_VIDEOFORMAT:
        // if lParam == NULL, return the size
        // if lParam != NULL, wParam is the size, return bytes copied
        if (!lpcs->fHardwareConnected)
            return FALSE;
        dwT = ((LPBITMAPINFOHEADER)lpcs->lpBitsInfo)->biSize +
	      ((LPBITMAPINFOHEADER)lpcs->lpBitsInfo)->biClrUsed * sizeof(RGBQUAD);
        if (lParam == 0)
            return dwT;
        else {
            if (wParam < (UINT) dwT)
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

    // unicode and win-16 version - see ansi thunk below
    case WM_CAP_SET_CALLBACK_STATUS:
        // Set the status callback proc
        if (lParam != 0 && IsBadCodePtr ((FARPROC) lParam))
            return FALSE;
        lpcs->CallbackOnStatus = (CAPSTATUSCALLBACK) lParam;
	lpcs->fLastStatusWasNULL = TRUE;
#ifdef UNICODE
        lpcs->fUnicode &= ~VUNICODE_STATUSISANSI;
#endif
        return TRUE;

#ifdef UNICODE
    // ansi thunk for above
    case WM_CAP_SET_CALLBACK_STATUSA:
        // Set the status callback proc
        if (lParam != 0 && IsBadCodePtr ((FARPROC) lParam))
            return FALSE;
        lpcs->CallbackOnStatus = (CAPSTATUSCALLBACK) lParam;
	lpcs->fLastStatusWasNULL = TRUE;
        lpcs->fUnicode |= VUNICODE_STATUSISANSI;
        return TRUE;
#endif

    // unicode and win-16 version - see ansi version below
    case WM_CAP_SET_CALLBACK_ERROR:
        // Set the error callback proc
        if (lParam != 0 && IsBadCodePtr ((FARPROC) lParam))
            return FALSE;
        lpcs->CallbackOnError = (CAPERRORCALLBACK) lParam;
	lpcs->fLastErrorWasNULL = TRUE;
#ifdef UNICODE
        lpcs->fUnicode &= ~VUNICODE_ERRORISANSI;
#endif
        return TRUE;


#ifdef UNICODE
    // ansi version of above
    case WM_CAP_SET_CALLBACK_ERRORA:
        // Set the error callback proc
        if (lParam != 0 && IsBadCodePtr ((FARPROC) lParam))
            return FALSE;
        lpcs->CallbackOnError = (CAPERRORCALLBACK) lParam;
	lpcs->fLastErrorWasNULL = TRUE;
        lpcs->fUnicode |= VUNICODE_ERRORISANSI;
        return TRUE;
#endif

    case WM_CAP_SET_CALLBACK_FRAME:
        // Set the callback proc for single frame during preview
        if (lParam != 0 && IsBadCodePtr ((FARPROC) lParam))
            return FALSE;
        lpcs->CallbackOnVideoFrame = (CAPVIDEOCALLBACK) lParam;
        return TRUE;

    default:
        break;
    }

    // Once we start capturing, don't change anything
    if (lpcs->fCaptureFlags & CAP_fCapturingNow)
        return dwReturn;

    switch (msg) {

    case WM_CAP_SET_CALLBACK_YIELD:
        // Set the callback proc for wave buffer processing to net
        if (lParam != 0 && IsBadCodePtr ((FARPROC) lParam))
            return FALSE;
        lpcs->CallbackOnYield = (CAPYIELDCALLBACK) lParam;
        return TRUE;

    case WM_CAP_SET_CALLBACK_VIDEOSTREAM:
        // Set the callback proc for video buffer processing to net
        if (lParam != 0 && IsBadCodePtr ((FARPROC) lParam))
            return FALSE;
        lpcs->CallbackOnVideoStream = (CAPVIDEOCALLBACK) lParam;
        return TRUE;

    case WM_CAP_SET_CALLBACK_WAVESTREAM:
        // Set the callback proc for wave buffer processing to net
        if (lParam != 0 && IsBadCodePtr ((FARPROC) lParam))
            return FALSE;
        lpcs->CallbackOnWaveStream = (CAPWAVECALLBACK) lParam;
        return TRUE;

    case WM_CAP_SET_CALLBACK_CAPCONTROL:
        // Set the callback proc for frame accurate capture start/stop
        if (lParam != 0 && IsBadCodePtr ((FARPROC) lParam))
            return FALSE;
        lpcs->CallbackOnControl = (CAPCONTROLCALLBACK) lParam;
        return TRUE;

    case WM_CAP_SET_USER_DATA:
	lpcs->lUser = lParam;
	return TRUE;

    case WM_CAP_DRIVER_CONNECT:
        // Connect to a device
        // wParam contains the index of the driver

        // If the same driver ID is requested, skip the request
        // Prevents multiple Inits from VB apps
        if (lpcs->fHardwareConnected &&
                (lpcs->sCapDrvCaps.wDeviceIndex == wParam))
            return TRUE;

        // First disconnect from any (possibly) existing device
        SendMessage (lpcs->hwnd, WM_CAP_DRIVER_DISCONNECT, 0, 0l);

        // and then connect to the new device
        if (CapWinConnectHardware (lpcs, (UINT) wParam /*wDeviceIndex*/)) {
            if (!DibGetNewFormatFromDriver (lpcs)) {  // Allocate our bitspace
                // Use the cached palette if available
                if (lpcs->hPalCurrent && lpcs->lpCacheXlateTable) {
                    PalSendPaletteToDriver (lpcs, lpcs->hPalCurrent, lpcs->lpCacheXlateTable);
                }
                else
                    PalGetPaletteFromDriver (lpcs);

                // Get a frame using the possibly cached palette
                videoFrame (lpcs->hVideoIn, &lpcs->VidHdr);
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
        /* PalFini (lpcs); keep the palette cached for reconnections */
        InvalidateRect(lpcs->hwnd, NULL, TRUE);
        lpcs->sCapDrvCaps.fCaptureInitialized = FALSE;
        dwReturn = TRUE;
        break;

    // unicode and win-16 version - see ansi thunk below
    case WM_CAP_FILE_SET_CAPTURE_FILE:
        // lParam points to the name of the capture file
        if (lParam) {
            BOOL fAlreadyExists;        // Don't create a file if new name
#ifndef _WIN32
            OFSTRUCT of;
#endif
            HANDLE hFile;

            // Check for valid file names...
#ifdef _WIN32
    // can't use OpenFile for UNICODE names
            if ((hFile = CreateFile(
                            (LPTSTR) lParam,
                            GENERIC_WRITE,
                            0,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL)) == INVALID_HANDLE_VALUE) {
                if ((hFile = CreateFile(
                                (LPTSTR) lParam,
                                GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_NEW,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL)) == INVALID_HANDLE_VALUE) {
#else

            if ((hFile = OpenFile ((LPTSTR) lParam, &of, OF_WRITE)) == -1) {
                if ((hFile = OpenFile ((LPTSTR) lParam, &of, OF_CREATE | OF_WRITE)) == -1) {
#endif
                    return FALSE;
                }
                fAlreadyExists = FALSE;
            }
            else
                fAlreadyExists = TRUE;

#ifdef _WIN32
            CloseHandle(hFile);
#else
            _lclose (hFile);
#endif
            lstrcpyn (lpcs->achFile, (LPTSTR) lParam, NUMELMS(lpcs->achFile));
            lpcs->fCapFileExists = fileCapFileIsAVI (lpcs->achFile);

            if (!fAlreadyExists) {
		// Delete the file created by CREATE_NEW (or OF_CREATE)
		// when verifying that we can write to this file location
#ifdef _WIN32
                DeleteFile ((LPTSTR) lParam);
#else
                OpenFile ((LPTSTR) lParam, &of, OF_DELETE);
#endif
            }
            dwReturn = TRUE;
        }
        break;

#ifdef UNICODE
    // Ansi thunk for above.
    case WM_CAP_FILE_SET_CAPTURE_FILEA:
        // lParam points to the name of the capture file
        if (lParam) {
            LPWSTR pw;
            int chsize;

            // remember the null
            chsize = lstrlenA( (LPSTR) lParam) + 1;
            pw = LocalAlloc(LPTR, chsize * sizeof(WCHAR));
	    if (pw) {
                Imbstowcs(pw, (LPSTR) lParam, chsize);
                dwReturn = ProcessCommandMessages(lpcs, WM_CAP_FILE_SET_CAPTURE_FILEW,
                                0, (LPARAM)pw);
                LocalFree(pw);
	    }
        }
        break;
#endif

    case WM_CAP_FILE_ALLOCATE:
        // lParam contains the size to preallocate the capture file in bytes
        return fileAllocCapFile(lpcs, lParam);

    // unicode and win-16 version - see ansi thunk below
    case WM_CAP_FILE_SAVEAS:
        // lParam points to the name of the SaveAs file
        if (lParam) {
            lstrcpyn (lpcs->achSaveAsFile, (LPTSTR) lParam,
                        NUMELMS(lpcs->achSaveAsFile));
            return (fileSaveCopy(lpcs));
        }
        break;

#ifdef UNICODE
    // ansi thunk for above
    case WM_CAP_FILE_SAVEASA:
        // lParam points to the name of the SaveAs file
        if (lParam) {
            LPWSTR pw;
            int chsize;

            // remember the null
            chsize = lstrlenA( (LPSTR) lParam)+1;
            pw = LocalAlloc(LPTR, chsize * sizeof(WCHAR));
	    if (pw) {
                Imbstowcs(pw, (LPSTR) lParam, chsize);
                dwReturn = ProcessCommandMessages(lpcs, WM_CAP_FILE_SAVEASW,
                                0, (LPARAM)pw);
                LocalFree(pw);
	    }
        }
        break;
#endif

    case WM_CAP_FILE_SET_INFOCHUNK:
        // wParam is not used
        // lParam is an LPCAPINFOCHUNK
        if (lParam) {
            return (SetInfoChunk(lpcs, (LPCAPINFOCHUNK) lParam));
        }
        break;

    // unicode and win-16 version - see ansi thunk below
    case WM_CAP_FILE_SAVEDIB:
        // lParam points to the name of the DIB file
        if (lParam) {
            if (lpcs->fOverlayWindow)
                GetAFrameThenCallback (lpcs, TRUE /*fForce*/);

            return (fileSaveDIB(lpcs, (LPTSTR)lParam));
        }
        break;

#ifdef UNICODE
    // ansi thunk for above
    case WM_CAP_FILE_SAVEDIBA:
        if (lParam) {
            LPWSTR pw;
            int chsize;

            if (lpcs->fOverlayWindow)
                GetAFrameThenCallback (lpcs, TRUE /*fForce*/);

            // remember the null
            chsize = lstrlenA( (LPSTR) lParam)+1;
            pw = LocalAlloc(LPTR, chsize * sizeof(WCHAR));
	    if (pw) {
                Imbstowcs(pw, (LPSTR) lParam, chsize);
                dwReturn = fileSaveDIB(lpcs, pw);
                LocalFree(pw);
	    }
        }
        break;
#endif


    case WM_CAP_EDIT_COPY:
        // Copy the current image and palette to the clipboard
        // wParam and lParam unused
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (lpcs->fOverlayWindow)
            GetAFrameThenCallback (lpcs, TRUE /*fForce*/);

        if (lpcs->sCapDrvCaps.fCaptureInitialized && OpenClipboard (lpcs->hwnd)) {
            EmptyClipboard();

            // put a copy of the current palette in the clipboard
            if (lpcs->hPalCurrent && lpcs->lpBitsInfo->bmiHeader.biBitCount <= 8)
                SetClipboardData(CF_PALETTE, CopyPalette (lpcs->hPalCurrent));

            // make a packed DIB out of the current image
            if (lpcs->lpBits && lpcs->lpBitsInfo ) {
                if (SetClipboardData (CF_DIB, CreatePackedDib (lpcs->lpBitsInfo,
                        lpcs->lpBits, lpcs->hPalCurrent)))
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
            UINT wSize;
            LPWAVEFORMATEX lpwf = (LPWAVEFORMATEX) lParam;
            UINT uiError;

            // Verify the waveformat is valid
            uiError = waveInOpen(NULL, WAVE_MAPPER, lpwf, 0, 0L,WAVE_FORMAT_QUERY);

            if (uiError) {
                errorUpdateError (lpcs, IDS_CAP_WAVE_OPEN_ERROR);
                return FALSE;
            }

            if (lpcs->lpWaveFormat)
                GlobalFreePtr (lpcs->lpWaveFormat);

            wSize = GetSizeOfWaveFormat (lpwf);
            if (lpcs->lpWaveFormat = (LPWAVEFORMATEX)
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
        if (lpcs->dwDlgsActive & VDLG_VIDEOSOURCE)
            return FALSE;
        if (lpcs->sCapDrvCaps.fHasDlgVideoSource) {
	    lpcs->dwDlgsActive |= VDLG_VIDEOSOURCE;
            videoDialog (lpcs->hVideoCapture, lpcs->hwnd, 0L );
            // Changing from NTSC to PAL could affect image dimensions!!!
            DibGetNewFormatFromDriver (lpcs);
            PalGetPaletteFromDriver (lpcs);

            // May need to inform parent of new layout here!
            InvalidateRect(lpcs->hwnd, NULL, TRUE);
            UpdateWindow(lpcs->hwnd);
	    lpcs->dwDlgsActive &= ~VDLG_VIDEOSOURCE;
        }
        return (lpcs->sCapDrvCaps.fHasDlgVideoSource);

    case WM_CAP_DLG_VIDEOFORMAT:
        // Show the format dialog, user selects dimensions, depth, compression
        // wParam and lParam are unused
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (lpcs->dwDlgsActive & VDLG_VIDEOFORMAT)
            return FALSE;
        if (lpcs->sCapDrvCaps.fHasDlgVideoFormat) {
	    lpcs->dwDlgsActive |= VDLG_VIDEOFORMAT;
            videoDialog (lpcs->hVideoIn, lpcs->hwnd, 0L );
            DibGetNewFormatFromDriver (lpcs);
            PalGetPaletteFromDriver (lpcs);

            // May need to inform parent of new layout here!
            InvalidateRect(lpcs->hwnd, NULL, TRUE);
            UpdateWindow(lpcs->hwnd);
	    lpcs->dwDlgsActive &= ~VDLG_VIDEOFORMAT;
        }
        return (lpcs->sCapDrvCaps.fHasDlgVideoFormat);

    case WM_CAP_DLG_VIDEODISPLAY:
        // Show the dialog which controls output.
        // This dialog only affects the presentation, never the data format
        // wParam and lParam are unused
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (lpcs->dwDlgsActive & VDLG_VIDEODISPLAY)
            return FALSE;
        if (lpcs->sCapDrvCaps.fHasDlgVideoDisplay) {
	    lpcs->dwDlgsActive |= VDLG_VIDEODISPLAY;
            videoDialog (lpcs->hVideoDisplay, lpcs->hwnd, 0L);
	    lpcs->dwDlgsActive &= ~VDLG_VIDEODISPLAY;
        }
        return (lpcs->sCapDrvCaps.fHasDlgVideoDisplay);

    case WM_CAP_DLG_VIDEOCOMPRESSION:
#ifndef NEW_COMPMAN
	return FALSE;
#else
        // Show the dialog which selects video compression options.
        // wParam and lParam are unused
        if (!lpcs->fHardwareConnected)
            return FALSE;
	if (lpcs->dwDlgsActive & VDLG_COMPRESSION)
            return FALSE;
	lpcs->dwDlgsActive |= VDLG_COMPRESSION;
        ICCompressorChoose(
                lpcs->hwnd,            // parent window for dialog
                ICMF_CHOOSE_KEYFRAME,  // want "key frame every" box
                lpcs->lpBitsInfo,      // input format (optional)
                NULL,                  // input data (optional)
                &lpcs->CompVars,       // data about the compressor/dlg
                NULL);                 // title bar (optional)
	lpcs->dwDlgsActive &= ~VDLG_COMPRESSION;
        return TRUE;
#endif

    case WM_CAP_SET_VIDEOFORMAT:
        // wParam is the size of the BITMAPINFO
        // lParam is an LPBITMAPINFO
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (IsBadReadPtr ((LPVOID) lParam, (UINT) wParam))
            return FALSE;

        return (DibNewFormatFromApp (lpcs, (LPBITMAPINFO) lParam, (UINT) wParam));

    case WM_CAP_SET_PREVIEW:
        // if wParam, enable preview via drawdib
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (wParam) {
            // turn off the overlay, if it is in use
            if (lpcs->fOverlayWindow)
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
            if (lpcs->fLiveWindow)   // turn off preview mode
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
            lpcs->idTimer = 0;
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
            lpcs->fCaptureFlags |= CAP_fCapturingToDisk;
            return (AVICapture(lpcs));
        }
        break;

    case WM_CAP_SEQUENCE_NOFILE:
        // wParam is unused
        // lParam is unused
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (lpcs->sCapDrvCaps.fCaptureInitialized) {
            lpcs->fCaptureFlags &= ~CAP_fCapturingToDisk;
            return (AVICapture(lpcs));
        }
        break;

    case WM_CAP_SET_SEQUENCE_SETUP:
        // wParam is sizeof CAPTUREPARMS
        // lParam = LPCAPTUREPARMS
        // The following were added after the Beta, init in case the client
        // has a smaller structure and doesn't access them.
	// WHICH BETA ?? (SteveDav)  We should change the comment to include a date

        lpcs->sCapParms.dwAudioBufferSize = 0;
        lpcs->sCapParms.fDisableWriteCache = TRUE;
        lpcs->sCapParms.AVStreamMaster = AVSTREAMMASTER_AUDIO;

        if (wParam <= sizeof (CAPTUREPARMS)) {
            dwT = min (sizeof (CAPTUREPARMS), wParam);
            if (IsBadReadPtr ((LPVOID) lParam, (UINT) dwT))
                break;

            _fmemcpy ((LPVOID) &lpcs->sCapParms, (LPVOID) lParam, (UINT) dwT);

            // Validate stuff that isn't handled elsewhere
            if (lpcs->sCapParms.wChunkGranularity != 0 &&
                lpcs->sCapParms.wChunkGranularity < 16)
                lpcs->sCapParms.wChunkGranularity = 16;
            if (lpcs->sCapParms.wChunkGranularity > 16384)
                lpcs->sCapParms.wChunkGranularity = 16384;

            if (lpcs->sCapParms.fLimitEnabled && (lpcs->sCapParms.wTimeLimit == 0))
                lpcs->sCapParms.wTimeLimit = 1;

            // Force Step MCI off if not using MCI control
            if (lpcs->sCapParms.fStepMCIDevice && !lpcs->sCapParms.fMCIControl)
                    lpcs->sCapParms.fStepMCIDevice = FALSE;

            // Prevent audio capture if no audio hardware
            lpcs->sCapParms.fCaptureAudio =
                lpcs->fAudioHardware && lpcs->sCapParms.fCaptureAudio;

            // Limit audio buffers
            lpcs->sCapParms.wNumAudioRequested =
                min (MAX_WAVE_BUFFERS, lpcs->sCapParms.wNumAudioRequested);

            // Limit video buffers
            lpcs->sCapParms.wNumVideoRequested =
                min (MAX_VIDEO_BUFFERS, lpcs->sCapParms.wNumVideoRequested);

            dwReturn = TRUE;
        }
        break;

    // unicode and win-16 version - see ansi thunk below
    case WM_CAP_SET_MCI_DEVICE:
        // lParam points to the name of the MCI Device
        if (IsBadStringPtr ((LPVOID) lParam, 1))
            return FALSE;
        if (lParam) {
            lstrcpyn (lpcs->achMCIDevice, (LPTSTR) lParam, NUMELMS(lpcs->achMCIDevice));
            dwReturn = TRUE;
        }
        break;
#ifdef UNICODE
    // ansi thunk for above
    case WM_CAP_SET_MCI_DEVICEA:
        // lParam points to Ansi name of MCI device
        if (lParam) {
            //remember the null
            int chsize = lstrlenA( (LPSTR) lParam)+1;
            Imbstowcs(lpcs->achMCIDevice, (LPSTR) lParam,
                min(chsize, NUMELMS(lpcs->achMCIDevice)));
            dwReturn = TRUE;
        }
        break;
#endif


    case WM_CAP_SET_SCROLL:
        // lParam is an LPPOINT which points to the new scroll position
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (IsBadReadPtr ((LPVOID) lParam, sizeof (POINT)))
            return FALSE;

        {
            LPPOINT lpP = (LPPOINT) lParam;

            if (lpP->x < lpcs->dxBits && lpP->y < lpcs->dyBits) {
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

    // unicode and win-16 version - see ansi thunk below
    case WM_CAP_PAL_OPEN:
        // Open a new palette
        // wParam is unused
        // lParam contains an LPTSTR to the file
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (IsBadStringPtr ((LPVOID) lParam, 1))
            return FALSE;
        return fileOpenPalette(lpcs, (LPTSTR) lParam /*lpszFileName*/);
#ifdef UNICODE
    // ansi thunk for above
    case WM_CAP_PAL_OPENA:
        // lParam contains (ANSI) lpstr for filename
        if (lParam) {
            // remember the null
            int chsize = lstrlenA( (LPSTR) lParam)+1;
            LPWSTR pw = LocalAlloc(LPTR, chsize * sizeof(WCHAR));
	    if (pw) {
                Imbstowcs(pw, (LPSTR) lParam, chsize);
                dwReturn = fileOpenPalette(lpcs, pw);
                LocalFree(pw);
	    }
        }
        break;
#endif

    // unicode and win-16 version - see ansi thunk below
    case WM_CAP_PAL_SAVE:
        // Save the current palette in a file
        // wParam is unused
        // lParam contains an LPTSTR to the file
        if (!lpcs->fHardwareConnected)
            return FALSE;
        if (IsBadStringPtr ((LPVOID) lParam, 1))
            return FALSE;
        return fileSavePalette(lpcs, (LPTSTR) lParam /*lpszFileName*/);
#ifdef UNICODE
    // ansi thunk for above
    case WM_CAP_PAL_SAVEA:
        // lParam contains (ANSI) lpstr for filename
        if (lParam) {
            // remember the null
            int chsize = lstrlenA( (LPSTR) lParam)+1;
            LPWSTR pw = LocalAlloc(LPTR, chsize * sizeof(WCHAR));
	    if (pw) {
                Imbstowcs(pw, (LPSTR) lParam, chsize);
                dwReturn = fileSavePalette(lpcs, pw);
                LocalFree(pw);
	    }
        }
        break;
#endif


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
LONG FAR PASCAL LOADDS EXPORT CapWndProc (HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
    LPCAPSTREAM lpcs;
    PAINTSTRUCT ps;
    HDC         hdc;
    MSG         PMsg;
    int         f;

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

        // Added VFW 1.1b, Clear the queue of additional timer msgs!!!

        // Even in Win32, processing frame timers can swamp all other
        // activity in the app, so clear the queue after each frame is done.

        // This successfully corrected a problem with the "Hit OK to continue"
        // dialog not appearing bug due to app message queue
        // swamping with timer messages at large
        // image dimensions and preview rates.

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
#ifdef _WIN32
            SetWindowOrgEx(hdc, lpcs->ptScroll.x, lpcs->ptScroll.y, NULL);
#else
            SetWindowOrg(hdc, lpcs->ptScroll.x, lpcs->ptScroll.y);
#endif
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
    WORD  wSize;
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
