/****************************************************************************
 *
 *   capinit.c
 *
 *   Initialization code.
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

#define MODULE_DEBUG_PREFIX "AVICAP32\\"
#define _INC_MMDEBUG_CODE_ TRUE
#include "MMDEBUG.H"

#if !defined CHICAGO
 #include <ntverp.h>
#endif

#include <mmsystem.h>

#include <msvideo.h>
#include "ivideo32.h"
#include <drawdib.h>
#include "avicap.h"
#include "avicapi.h"

HINSTANCE ghInstDll;
TCHAR szCaptureWindowClass[] = TEXT("ClsCapWin");

#if !defined CHICAGO
  typedef struct tagVS_VERSION
  {
      WORD wTotLen;
      WORD wValLen;
      TCHAR szSig[16];
      VS_FIXEDFILEINFO vffInfo;
  } VS_VERSION;

  typedef struct tagLANGANDCP
  {
      WORD wLanguage;
      WORD wCodePage;
  } LANGANDCP;
#endif
BOOL gfIsRTL;

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
                            CS_GLOBALCLASS | CS_DBLCLKS;
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
BOOL capInternalGetDriverDesc (UINT wDriverIndex,
        LPTSTR lpszName, int cbName,
        LPTSTR lpszVer, int cbVer)
{
   #ifdef CHICAGO
    // This calls into 16-bit AVICAP via a thunk
    return (BOOL) capxGetDriverDescription ((WORD) wDriverIndex,
                lpszName, (WORD) cbName,
                lpszVer, (WORD) cbVer);
   #else
    LPTSTR  lpVersion;
    UINT    wVersionLen;
    BOOL    bRetCode;
    TCHAR   szGetName[MAX_PATH];
    DWORD   dwVerInfoSize;
    DWORD   dwVerHnd;
    TCHAR   szBuf[MAX_PATH];
    BOOL    fGetName;
    BOOL    fGetVersion;

    const static TCHAR szNull[]        = TEXT("");
    const static TCHAR szVideo[]       = TEXT("msvideo");
    const static TCHAR szSystemIni[]   = TEXT("system.ini");
    const static TCHAR szDrivers[]     = TEXT("Drivers32");
          static TCHAR szKey[sizeof(szVideo)/sizeof(TCHAR) + 2];

    fGetName = lpszName != NULL && cbName != 0;
    fGetVersion = lpszVer != NULL && cbVer != 0;

    if (fGetName)
        lpszName[0] = TEXT('\0');
    if (fGetVersion)
        lpszVer [0] = TEXT('\0');

    lstrcpy(szKey, szVideo);
    szKey[sizeof(szVideo)/sizeof(TCHAR) - 1] = TEXT('\0');
    if( wDriverIndex > 0 ) {
        szKey[sizeof(szVideo)/sizeof(TCHAR)] = TEXT('\0');
        szKey[(sizeof(szVideo)/sizeof(TCHAR))-1] = (TCHAR)(TEXT('1') + (wDriverIndex-1) );  // driver ordinal
    }

    if (GetPrivateProfileString(szDrivers, szKey, szNull,
                szBuf, sizeof(szBuf)/sizeof(TCHAR), szSystemIni) < 2)
        return FALSE;

    // Copy in the driver name initially, just in case the driver
    // has omitted a description field.
    if (fGetName)
        lstrcpyn(lpszName, szBuf, cbName);

    // You must find the size first before getting any file info
    dwVerInfoSize = GetFileVersionInfoSize(szBuf, &dwVerHnd);

    if (dwVerInfoSize) {
        LPTSTR   lpstrVffInfo;             // Pointer to block to hold info
        HANDLE  hMem;                     // handle to mem alloc'ed

        // Get a block big enough to hold version info
        hMem          = GlobalAlloc(GMEM_MOVEABLE, dwVerInfoSize);
        lpstrVffInfo  = GlobalLock(hMem);

        // Get the File Version first
        if (GetFileVersionInfo(szBuf, 0L, dwVerInfoSize, lpstrVffInfo)) {
             VS_VERSION FAR *pVerInfo = (VS_VERSION FAR *) lpstrVffInfo;

             // fill in the file version
             wsprintf(szBuf,
                      TEXT("Version:  %d.%d.%d.%d"),
                      HIWORD(pVerInfo->vffInfo.dwFileVersionMS),
                      LOWORD(pVerInfo->vffInfo.dwFileVersionMS),
                      HIWORD(pVerInfo->vffInfo.dwFileVersionLS),
                      LOWORD(pVerInfo->vffInfo.dwFileVersionLS));
             if (fGetVersion)
                lstrcpyn (lpszVer, szBuf, cbVer);
        }

        // Now try to get the FileDescription
        // First try this for the "Translation" entry, and then
        // try the American english translation.
        // Keep track of the string length for easy updating.
        // 040904E4 represents the language ID and the four
        // least significant digits represent the codepage for
        // which the data is formatted.  The language ID is
        // composed of two parts: the low ten bits represent
        // the major language and the high six bits represent
        // the sub language.

        lstrcpy(szGetName, TEXT("\\StringFileInfo\\040904E4\\FileDescription"));

        wVersionLen   = 0;
        lpVersion     = NULL;

        // Look for the corresponding string.
        bRetCode      =  VerQueryValue((LPVOID)lpstrVffInfo,
                        (LPTSTR)szGetName,
                        (void FAR* FAR*)&lpVersion,
                        (UINT FAR *) &wVersionLen);

        if (fGetName && bRetCode && wVersionLen && lpVersion)
           lstrcpyn (lpszName, lpVersion, cbName);

        // Let go of the memory
        GlobalUnlock(hMem);
        GlobalFree(hMem);
    }
    return TRUE;
   #endif
}

#ifdef UNICODE
// ansi thunk for above (called from ansi thunk functions
// for capGetDriverDescriptionA, and WM_GET_DRIVER_NAMEA etc)
BOOL capInternalGetDriverDescA(UINT wDriverIndex,
        LPSTR lpszName, int cbName,
        LPSTR lpszVer, int cbVer)
{
    LPWSTR pName = NULL, pVer = NULL;
    BOOL bRet;

    if (lpszName) {
        pName = LocalAlloc(LPTR, cbName * sizeof(WCHAR));
    }

    if (lpszVer) {
        pVer = LocalAlloc(LPTR, cbVer * sizeof(WCHAR));
    }

    bRet = capInternalGetDriverDesc(
            wDriverIndex,
            pName, cbName,
            pVer, cbVer);

    if (lpszName) {
        WideToAnsi(lpszName, pName, cbName);
    }

    if (lpszVer) {
        WideToAnsi(lpszVer, pVer, cbVer);
    }

    if (pVer) {
        LocalFree(pVer);
    }

    if (pName) {
        LocalFree(pName);
    }

    return bRet;
}
#endif


//
// Exported version
// Get the name and version of the video device
//
// unicode and win-16 version - see ansi thunk below
BOOL VFWAPI capGetDriverDescription (UINT wDriverIndex,
        LPTSTR lpszName, int cbName,
        LPTSTR lpszVer, int cbVer)
{
    return (capInternalGetDriverDesc (wDriverIndex,
        lpszName, cbName,
        lpszVer, cbVer));
}

#ifdef UNICODE
// ansi thunk for above
BOOL VFWAPI capGetDriverDescriptionA(UINT wDriverIndex,
        LPSTR lpszName, int cbName,
        LPSTR lpszVer, int cbVer)
{
    return capInternalGetDriverDescA(wDriverIndex,
        lpszName, cbName, lpszVer, cbVer);
}
#endif


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
    lpcs->sCapDrvCaps.fHasOverlay = FALSE;
    lpcs->sCapDrvCaps.fDriverSuppliesPalettes = FALSE;

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
BOOL CapWinConnectHardware (LPCAPSTREAM lpcs, UINT wDeviceIndex)
{
    DWORD dwError;
    CHANNEL_CAPS VideoCapsExternalOut;
    TCHAR ach1[MAX_PATH];
    TCHAR ach2[MAX_PATH * 3];
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
                        ach1, sizeof (ach1) / sizeof(TCHAR),
                        ach2, sizeof (ach2) / sizeof(TCHAR));
                lstrcat (ach1, TEXT(", "));
                lstrcat (ach1, ach2);

                statusUpdateStatus (lpcs, IDS_CAP_INFO, (LPTSTR) ach1);

                // Make a string of the current task and capture driver
                ach2[0] = '\0';
                if (hInstT = GetWindowInstance (GetParent(lpcs->hwnd)))
                    GetModuleFileName (hInstT, ach2, sizeof (ach2)/sizeof(TCHAR));
                lstrcat (ach2, TEXT(" -AVICAP32- "));
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
        lpcs->sCapDrvCaps.fHasDlgVideoFormat = !videoDialog (lpcs->hVideoIn,
                        lpcs->hwnd, VIDEO_DLG_QUERY);

    if (lpcs->hVideoCapture)
         lpcs->sCapDrvCaps.fHasDlgVideoSource = !videoDialog (lpcs->hVideoCapture,
                        lpcs->hwnd, VIDEO_DLG_QUERY);

    if (lpcs->hVideoDisplay)
         lpcs->sCapDrvCaps.fHasDlgVideoDisplay = !videoDialog (lpcs->hVideoDisplay,
                        lpcs->hwnd, VIDEO_DLG_QUERY);

    // these handles are not supported on WIN32 for the good reason that
    // the videoXXX api set is not published for 32-bit
    // we might want to make use of the handles ourselves...???
    lpcs->sCapDrvCaps.hVideoIn          = NULL;
    lpcs->sCapDrvCaps.hVideoOut         = NULL;
    lpcs->sCapDrvCaps.hVideoExtIn       = NULL;
    lpcs->sCapDrvCaps.hVideoExtOut      = NULL;

    return lpcs->fHardwareConnected;
}



//
// Creates a child window of the capture class
// Normally:
//   Set lpszWindowName to NULL
//   Set dwStyle to WS_CHILD | WS_VISIBLE
//   Set hmenu to a unique child id

// Unicode and Win-16 version. See ansi thunk below
HWND VFWAPI capCreateCaptureWindow (
        LPCTSTR lpszWindowName,
        DWORD dwStyle,
        int x, int y, int nWidth, int nHeight,
        HWND hwndParent, int nID)
{
    DWORD   dwExStyle;

    dwExStyle = gfIsRTL ? WS_EX_LEFTSCROLLBAR | WS_EX_RIGHT | WS_EX_RTLREADING : 0;
    RegisterCaptureClass(ghInstDll);

#ifdef USE_AVIFILE
    AVIFileInit();
#endif

    return CreateWindowEx(dwExStyle,
                szCaptureWindowClass,
                lpszWindowName,
                dwStyle,
                x, y, nWidth, nHeight,
                hwndParent, (HMENU) nID,
                ghInstDll,
                NULL);
}

#ifdef UNICODE
// ansi thunk
HWND VFWAPI capCreateCaptureWindowA (
                LPCSTR lpszWindowName,
                DWORD dwStyle,
                int x, int y, int nWidth, int nHeight,
                HWND hwndParent, int nID)
{
    LPWSTR pw;
    int chsize;
    HWND hwnd;

    if (lpszWindowName == NULL) {
        pw = NULL;
    } else {
        // remember the null
        chsize = lstrlenA(lpszWindowName) + 1;
        pw = LocalLock(LocalAlloc(LPTR, chsize * sizeof(WCHAR)));

        AnsiToWide(pw, lpszWindowName, chsize);
    }

    hwnd = capCreateCaptureWindowW(pw, dwStyle, x, y, nWidth, nHeight,
                hwndParent, nID);

    if (pw != NULL) {
        LocalFree(LocalHandle(pw));
    }
    return(hwnd);
}
#endif


#ifdef CHICAGO

static char pszDll16[] = "AVICAP.DLL";
static char pszDll32[] = "AVICAP32.DLL";

BOOL PASCAL avicapf_ThunkConnect32(LPCSTR pszDll16, LPCSTR pszDll32, HINSTANCE hinst, DWORD dwReason);

BOOL WINAPI DllMain(
    HANDLE hInstance,
    DWORD  dwReason,
    LPVOID reserved)
{
    #if defined DEBUG || defined DEBUG_RETAIL
    DebugSetOutputLevel (GetProfileInt ("Debug", "Avicap32", 0));
    AuxDebugEx (1, DEBUGLINE "DllEntryPoint, %08x,%08x,%08x\r\n", hInstance, dwReason, reserved);
    #endif

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        char   ach[2];
        ghInstDll = hInstance;

        LoadString(ghInstDll, IDS_CAP_RTL, ach, sizeof(ach));
        gfIsRTL = ach[0] == TEXT('1');

        // INLINE_BREAK;
        if (!avicapf_ThunkConnect32(pszDll16, pszDll32, hInstance, dwReason))
            return FALSE;

       #if defined _WIN32 && defined CHICAGO
        // we do this so that we can Get LinPageLock & PageAllocate services
        //
        ;
//        OpenMMDEVLDR();
       #endif

    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {

       #if defined _WIN32 && defined CHICAGO
       ;
//        CloseMMDEVLDR();
       #endif

        return avicapf_ThunkConnect32(pszDll16, pszDll32, hInstance, dwReason);
    }

    return TRUE;
}

#else // this is the NT dll entry point

static char szMSVIDEO[] = "MSVideo";

BOOL DllInstanceInit(HANDLE hInstance, DWORD dwReason, LPVOID reserved)
{
#if 0
    static BOOL bFixedUp = FALSE;
#endif

    if (dwReason == DLL_PROCESS_ATTACH) {
	TCHAR  ach[2];
#if 0
// this hack has been superceded by correct thunking of capGetDriverDescription
// in an nt-supplied 16-bit avicap.dll
	if (!bFixedUp) {
	    HKEY hkey16=NULL;
	    HKEY hkey32=NULL;
	    char achValue[256];
	    DWORD dwType, cbValue = sizeof(achValue);

	    // In order to get 16 bit capture applications to work, a 16 bit
	    // application must believe that there is a capture driver
	    // installed.  Because these applications look in the 16 bit
	    // registry (equates to INI file) then we fudge the situation.
	    // IF there is no information on the 16 bit side, BUT we have
	    // installed a 32 bit driver, then copy the 32 bit driver
	    // information to the 16 bit registry.  Note: this does NOT mean
	    // that the capture will happen in 16 bit.  The 32 bit code will
	    // still get invoked to do the capture.

	    RegCreateKeyA(HKEY_LOCAL_MACHINE,
		"Software\\Microsoft\\Windows NT\\CurrentVersion\\Drivers",
		&hkey16);

	    RegOpenKeyA(HKEY_LOCAL_MACHINE,
		    "Software\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32",
		    &hkey32);

	    if (hkey16 && hkey32) {
		LONG result =
		    RegQueryValueExA(
		    hkey16,
		    szMSVIDEO,
		    NULL,
		    &dwType,
		    achValue,
		    &cbValue);

		// If there is no value stored in the 16 bit section of the
		// registry (equates to INI file) then see if we have a
		// 32 bit driver installed.
		if ((result != ERROR_SUCCESS) && (result != ERROR_MORE_DATA)) {
                    cbValue = sizeof(achValue);
		    if (RegQueryValueExA(
			hkey32,
			szMSVIDEO,
			NULL,
			&dwType,
			achValue,
			&cbValue) == ERROR_SUCCESS) {

			// there is a 32-bit MSVideo and no 16-bit MSVideo -
			// write the 32-bit one into the 16-bit list so that
			// capGetDriverDescription will work
			// cbValue will be set correctly from the previous
			// query call
			RegSetValueExA(
			    hkey16,
			    szMSVIDEO,
			    0,
			    dwType,
			    achValue,
			    cbValue);
		    }
		}
	    }

	    if (hkey16) {
		RegCloseKey(hkey16);
	    }
	    if (hkey32) {
		RegCloseKey(hkey32);
	    }
	    bFixedUp = TRUE;
	}
#endif

	ghInstDll = hInstance;
	DisableThreadLibraryCalls(hInstance);
        LoadString(ghInstDll, IDS_CAP_RTL, ach, NUMELMS(ach));
        gfIsRTL = ach[0] == TEXT('1');
	DebugSetOutputLevel (GetProfileIntA("Debug", "Avicap32", 0));
        videoInitHandleList();
    } else if (dwReason == DLL_PROCESS_DETACH) {
        videoDeleteHandleList();
    }
    return TRUE;
}

#endif // CHICAGO / NT dll entry point


