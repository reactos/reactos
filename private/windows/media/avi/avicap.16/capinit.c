/****************************************************************************
 *
 *   capinit.c
 *
 *   Initialization code.
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
#include <ver.h>
#include <mmsystem.h>

//
// define these before any msvideo.h, so our functions get declared right.
//
#ifndef WIN32
#define VFWAPI  FAR PASCAL _loadds
#define VFWAPIV FAR CDECL  _loadds
#endif

#include "msvideo.h"
#include <drawdib.h>
#include "avicap.h"
#include "avicapi.h"

// for correct handling of capGetDriverDescription on NT and Chicago
// this is used by the NT version of avicap.dll (16bit) but not intended for
// public use, hence not in msvideo.h
DWORD WINAPI videoCapDriverDescAndVer (
        DWORD wDriverIndex,
        LPSTR lpszName, UINT cbName,
        LPSTR lpszVer, UINT cbVer);

HINSTANCE ghInst;
BOOL gfIsRTL;
char szCaptureWindowClass[] = "ClsCapWin";

typedef struct tagVS_VERSION
{
    WORD wTotLen;
    WORD wValLen;
    char szSig[16];
    VS_FIXEDFILEINFO vffInfo;
} VS_VERSION;

typedef struct tagLANGANDCP
{
    WORD wLanguage;
    WORD wCodePage;
} LANGANDCP;


BOOL FAR PASCAL RegisterCaptureClass (HINSTANCE hInst)
{
    WNDCLASS cls;

    // If we're already registered, we're OK
    if (GetClassInfo(hInst, szCaptureWindowClass, &cls))
	return TRUE;

    cls.hCursor           = LoadCursor(NULL, IDC_ARROW);
    cls.hIcon             = NULL;
    cls.lpszMenuName      = NULL;
    cls.lpszClassName     = szCaptureWindowClass;
    cls.hbrBackground     = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    cls.hInstance         = hInst;
    cls.style             = CS_HREDRAW|CS_VREDRAW | CS_BYTEALIGNCLIENT |
                            CS_GLOBALCLASS;
    cls.lpfnWndProc       = (WNDPROC) CapWndProc;
    cls.cbClsExtra        = 0;
    // Kludge, VB Status and Error GlobalAlloc'd ptrs + room to grow...
    cls.cbWndExtra        = sizeof (LPCAPSTREAM) + sizeof (DWORD) * 4;

    RegisterClass(&cls);

    return TRUE;
}

//
// Internal version
// Get the name and version of the video device
//
BOOL capInternalGetDriverDesc (WORD wDriverIndex,
        LPSTR lpszName, int cbName,
        LPSTR lpszVer, int cbVer)
{
   return (BOOL) videoCapDriverDescAndVer(
                     wDriverIndex,
                  lpszName, cbName,
                  lpszVer, cbVer);

}

//
// Exported version
// Get the name and version of the video device
//
BOOL VFWAPI capGetDriverDescription (WORD wDriverIndex,
        LPSTR lpszName, int cbName,
        LPSTR lpszVer, int cbVer)
{
    return (capInternalGetDriverDesc (wDriverIndex,
        lpszName, cbName,
        lpszVer, cbVer));
}

//
// Disconnect from hardware resources
//
BOOL CapWinDisconnectHardware(LPCAPSTREAM lpcs)
{
    if( lpcs->hVideoCapture ) {
        videoStreamFini (lpcs->hVideoCapture);
        videoClose( lpcs->hVideoCapture );
    }
    if( lpcs->hVideoDisplay ) {
        videoStreamFini (lpcs->hVideoDisplay);
        videoClose( lpcs->hVideoDisplay );
    }
    if( lpcs->hVideoIn ) {
        videoClose( lpcs->hVideoIn );
    }

    lpcs->fHardwareConnected = FALSE;

    lpcs->hVideoCapture = NULL;
    lpcs->hVideoDisplay = NULL;
    lpcs->hVideoIn = NULL;

    lpcs->sCapDrvCaps.fHasDlgVideoSource = FALSE;
    lpcs->sCapDrvCaps.fHasDlgVideoFormat = FALSE;
    lpcs->sCapDrvCaps.fHasDlgVideoDisplay = FALSE;
    lpcs->sCapDrvCaps.fHasDlgVideoDisplay = FALSE;

    lpcs->sCapDrvCaps.hVideoIn          = NULL;
    lpcs->sCapDrvCaps.hVideoOut         = NULL;
    lpcs->sCapDrvCaps.hVideoExtIn       = NULL;
    lpcs->sCapDrvCaps.hVideoExtOut      = NULL;

    return TRUE;
}

//
// Connect to hardware resources
// Return: TRUE if hardware connected to the stream
//
BOOL CapWinConnectHardware (LPCAPSTREAM lpcs, WORD wDeviceIndex)
{
    DWORD dwError;
    CHANNEL_CAPS VideoCapsExternalOut;
    char ach1[_MAX_CAP_PATH];
    char ach2[_MAX_CAP_PATH * 3];
    CAPINFOCHUNK cic;
    HINSTANCE hInstT;

    lpcs->hVideoCapture = NULL;
    lpcs->hVideoDisplay = NULL;
    lpcs->hVideoIn = NULL;
    lpcs->fHardwareConnected = FALSE;
    lpcs->fUsingDefaultPalette = TRUE;
    lpcs->sCapDrvCaps.fHasDlgVideoSource = FALSE;
    lpcs->sCapDrvCaps.fHasDlgVideoFormat = FALSE;
    lpcs->sCapDrvCaps.fHasDlgVideoDisplay = FALSE;
    lpcs->sCapDrvCaps.wDeviceIndex = wDeviceIndex;

    // Clear any existing capture device name chunk
    cic.fccInfoID = mmioFOURCC ('I','S','F','T');
    cic.lpData = NULL;
    cic.cbData = 0;
    SetInfoChunk (lpcs, &cic);

    // try and open the video hardware!!!
    if( !(dwError = videoOpen( &lpcs->hVideoIn, wDeviceIndex, VIDEO_IN ) ) ) {
        if( !(dwError = videoOpen( &lpcs->hVideoCapture, wDeviceIndex, VIDEO_EXTERNALIN ) ) ) {
            // We don't require the EXTERNALOUT channel,
            // but do require EXTERNALIN and IN
            videoOpen( &lpcs->hVideoDisplay, wDeviceIndex, VIDEO_EXTERNALOUT );
            if( (!dwError) && lpcs->hVideoCapture && lpcs->hVideoIn ) {

                lpcs->fHardwareConnected = TRUE;
                capInternalGetDriverDesc (wDeviceIndex,
                        ach1, sizeof (ach1),
                        ach2, sizeof (ach2));
                lstrcat (ach1, ", ");
                lstrcat (ach1, ach2);

                statusUpdateStatus (lpcs, IDS_CAP_INFO, (LPSTR) ach1);

                // Make a string of the current task and capture driver
                ach2[0] = '\0';
                if (hInstT = GetWindowWord (GetParent (lpcs->hwnd), GWW_HINSTANCE))
                    GetModuleFileName (hInstT, ach2, sizeof (ach2));
                lstrcat (ach2, " -AVICAP- ");
                lstrcat (ach2, ach1);

                // Set software chunk with name of capture device
                if (*ach2) {
                    cic.lpData = ach2;
                    cic.cbData = lstrlen(ach2) + 1;
                    SetInfoChunk (lpcs, &cic);
                }
            }
        }
    }
    if (dwError)
        errorDriverID (lpcs, dwError);

    if(!lpcs->fHardwareConnected) {
       CapWinDisconnectHardware(lpcs);
    }
    else {
        if (lpcs->hVideoDisplay && videoGetChannelCaps (lpcs->hVideoDisplay,
                &VideoCapsExternalOut,
                sizeof (CHANNEL_CAPS)) == DV_ERR_OK) {
            lpcs->sCapDrvCaps.fHasOverlay = (BOOL)(VideoCapsExternalOut.dwFlags &
                (DWORD)VCAPS_OVERLAY);
        }
        else
             lpcs->sCapDrvCaps.fHasOverlay = FALSE;
        // if the hardware doesn't support it, make sure we don't enable
        if (!lpcs->sCapDrvCaps.fHasOverlay)
            lpcs->fOverlayWindow = FALSE;

       // Start the external in channel streaming continuously
       videoStreamInit (lpcs->hVideoCapture, 0L, 0L, 0L, 0L);
    } // end if hardware is available

#if 0
    // if we don't have a powerful machine, disable capture
    if (GetWinFlags() & (DWORD) WF_CPU286)
       CapWinDisconnectHardware(lpcs);
#endif

    if (!lpcs->fHardwareConnected){
        lpcs->fLiveWindow = FALSE;
        lpcs->fOverlayWindow = FALSE;
    }

    if (lpcs->hVideoIn)
        lpcs->sCapDrvCaps.fHasDlgVideoFormat = !videoDialog (lpcs-> hVideoIn,
                        lpcs-> hwnd, VIDEO_DLG_QUERY);

    if (lpcs->hVideoCapture)
         lpcs->sCapDrvCaps.fHasDlgVideoSource = !videoDialog (lpcs-> hVideoCapture,
                        lpcs-> hwnd, VIDEO_DLG_QUERY);

    if (lpcs->hVideoDisplay)
         lpcs->sCapDrvCaps.fHasDlgVideoDisplay = !videoDialog (lpcs-> hVideoDisplay,
                        lpcs-> hwnd, VIDEO_DLG_QUERY);

    lpcs->sCapDrvCaps.hVideoIn          = lpcs->hVideoIn;
    lpcs->sCapDrvCaps.hVideoOut         = NULL;
    lpcs->sCapDrvCaps.hVideoExtIn       = lpcs->hVideoCapture;
    lpcs->sCapDrvCaps.hVideoExtOut      = lpcs->hVideoDisplay;

    return lpcs->fHardwareConnected;
}



//
// Creates a child window of the capture class
// Normally:
//   Set lpszWindowName to NULL
//   Set dwStyle to WS_CHILD | WS_VISIBLE
//   Set hmenu to a unique child id

HWND VFWAPI capCreateCaptureWindow (
        LPCSTR lpszWindowName,
        DWORD dwStyle,
        int x, int y, int nWidth, int nHeight,
        HWND hwndParent, int nID)
{
    DWORD   fdwFlags;

#ifndef WS_EX_LEFTSCROLLBAR
#define WS_EX_LEFTSCROLLBAR 0
#define WS_EX_RIGHT         0
#define WS_EX_RTLREADING    0
#endif

    RegisterCaptureClass(ghInst);
    fdwFlags = gfIsRTL ? WS_EX_LEFTSCROLLBAR | WS_EX_RIGHT | WS_EX_RTLREADING : 0;
    return CreateWindowEx(fdwFlags,
	        szCaptureWindowClass,
                lpszWindowName,
                dwStyle,
                x, y, nWidth, nHeight,
                hwndParent, (HMENU) nID,
	        ghInst,
                NULL);
}


int CALLBACK LibMain(HINSTANCE hinst, WORD wDataSeg, WORD cbHeap,
    LPSTR lpszCmdLine )
{
    char   ach[2];

    ghInst = hinst;
    LoadString(ghInst, IDS_CAP_RTL, ach, sizeof(ach));
    gfIsRTL = ach[0] == '1';
    return TRUE;
}

int FAR PASCAL WEP(int i)
{
    return 1;
}
