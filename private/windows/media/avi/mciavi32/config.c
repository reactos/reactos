/******************************************************************************

   Copyright (C) Microsoft Corporation 1985-1995. All rights reserved.

   Title:   config.c - Multimedia Systems Media Control Interface
            driver for AVI - configuration dialog.

*****************************************************************************/
#include "graphic.h"

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof((x))/sizeof((x)[0]))
#endif

/*
#ifndef _WIN32
#define SZCODE char _based(_segname("_CODE"))
#else
#define SZCODE const TCHAR
#endif
*/

SZCODE szDEFAULTVIDEO[] = TEXT ("DefaultVideo");
SZCODE szSEEKEXACT[]    = TEXT ("AccurateSeek");
SZCODE szZOOMBY2[]              = TEXT ("ZoomBy2");
//SZCODE szSTUPIDMODE[] = TEXT ("DontBufferOffscreen");
SZCODE szSKIPFRAMES[]   = TEXT ("SkipFrames");
SZCODE szUSEAVIFILE[]   = TEXT ("UseAVIFile");
SZCODE szNOSOUND[]              = TEXT("NoSound");

const TCHAR szIni[]     = TEXT ("MCIAVI");

SZCODE gszMCIAVIOpt[]   = TEXT ("Software\\Microsoft\\Multimedia\\Video For Windows\\MCIAVI");
SZCODE gszDefVideoOpt[] = TEXT ("DefaultOptions");

#ifdef _WIN32
/* Registry values are stored as REG_DWORD */
int sz1 = 1;
int sz0 = 0;
#else
SZCODE sz1[] = TEXT("1");
SZCODE sz0[] = TEXT("0");
#endif

SZCODE szIntl[]         = TEXT ("Intl");
SZCODE szDecimal[]      = TEXT ("sDecimal");
SZCODE szThousand[] = TEXT ("sThousand");

SZCODE szDrawDib[]      = TEXT("DrawDib");
SZCODE szDVA[]          = TEXT("DVA");


typedef BOOL (WINAPI *SHOWMMCPLPROPSHEET)(
                                                        HWND hwndParent,
                                                        LPCSTR szPropSheetID,
                                                        LPSTR  szTabName,
                                                        LPSTR  szCaption);

/* Make sure we only have one configure box up at a time.... */
HWND    ghwndConfig = NULL;

//      Converts a Wide byte character string to a single byte character string
BOOL FAR PASCAL UnicodeToAnsi (
        char * pszDest,
        TCHAR * pszSrc,
        UINT cchMaxLen)
{

        if ((pszDest == NULL) ||
                (pszSrc == NULL) ||
                (cchMaxLen == 0))
                return FALSE;

        WideCharToMultiByte (CP_ACP, 0, pszSrc, -1,
                             pszDest, cchMaxLen,
                             NULL, NULL);
        return TRUE;
}



DWORD ReadOptionsFromReg(void)
{
        HKEY    hkVideoOpt;
        DWORD   dwType;
        DWORD   dwOpt;
        DWORD   cbSize;

    if (RegCreateKey(HKEY_CURRENT_USER, (LPTSTR)gszMCIAVIOpt, &hkVideoOpt))
                return 0;

        cbSize = sizeof(DWORD);
        if (RegQueryValueEx(hkVideoOpt,(LPTSTR)gszDefVideoOpt,
                                                NULL, &dwType, (LPBYTE)&dwOpt, &cbSize))
        {
                dwOpt = 0;
                RegSetValueEx(hkVideoOpt, (LPTSTR)gszDefVideoOpt, 0, REG_DWORD,(LPBYTE)&dwOpt, sizeof(DWORD));
        }

        RegCloseKey(hkVideoOpt);

        return dwOpt;
}


DWORD FAR PASCAL ReadConfigInfo(void)
{
    DWORD       dwOptions = 0L;

        dwOptions = ReadOptionsFromReg();

        //
    // ask the display device if it can do 256 color.
    //
#ifndef _WIN32
    int         i;

/*
**      Bugbug - not apparently used, so why do it ?!?
**
    HDC         hdc;
    hdc = GetDC(NULL);
    i = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);
    ReleaseDC(NULL, hdc);
**
**
*/

    i = mmGetProfileInt(szIni, szDEFAULTVIDEO,
                (i < 8 && (GetWinFlags() & WF_CPU286)) ? 240 : 0);

    if (i >= 200)
                dwOptions |= MCIAVIO_USEVGABYDEFAULT;
#endif

////if (mmGetProfileInt(szIni, szSEEKEXACT, 1))
        // Note: We always want this option.
        dwOptions |= MCIAVIO_SEEKEXACT;

//    if (mmGetProfileInt(szIni, szZOOMBY2, 0))
//              dwOptions |= MCIAVIO_ZOOMBY2;

////if (mmGetProfileInt(szIni, szFAILIFNOWAVE, 0))
////    dwOptions |= MCIAVIO_FAILIFNOWAVE;

//  if (mmGetProfileInt(szIni, szSTUPIDMODE, 0))
//              dwOptions |= MCIAVIO_STUPIDMODE;

        // Note:  These settings are still in WIN.INI, not the registry
        //        I know, we're stupid.
    if (mmGetProfileInt(szIni, szSKIPFRAMES, 1))
        dwOptions |= MCIAVIO_SKIPFRAMES;

    if (mmGetProfileInt(szIni, szUSEAVIFILE, 0))
        dwOptions |= MCIAVIO_USEAVIFILE;

    if (mmGetProfileInt(szIni, szNOSOUND, 0))
        dwOptions |= MCIAVIO_NOSOUND;

    if (mmGetProfileInt(szDrawDib, szDVA, TRUE))
        dwOptions |= MCIAVIO_USEDCI;

        return dwOptions;
}

void FAR PASCAL WriteConfigInfo(DWORD dwOptions)
{
#ifndef _WIN32
    // !!! This shouldn't get written out if it is the default!
    mmWriteProfileString(szIni, szDEFAULTVIDEO,
         (dwOptions & MCIAVIO_USEVGABYDEFAULT) ? szVIDEO240 : szVIDEOWINDOW);
#endif

////mmWriteProfileInt(szIni, szSEEKEXACT,
////        (dwOptions & MCIAVIO_SEEKEXACT) ? sz1 : sz0);

//    mmWriteProfileInt(szIni, szZOOMBY2,
//          (dwOptions & MCIAVIO_ZOOMBY2) ? sz1 : sz0);

    mmWriteProfileInt(szDrawDib, szDVA,
            (dwOptions & MCIAVIO_USEDCI) ? sz1 : sz0);

////mmWriteProfileInt(szIni, szFAILIFNOWAVE,
////        (dwOptions & MCIAVIO_FAILIFNOWAVE) ? sz1 : sz0);

//  mmWriteProfileInt(szIni, szSTUPIDMODE,
//            (dwOptions & MCIAVIO_STUPIDMODE) ? sz1 : sz0);

    mmWriteProfileInt(szIni, szSKIPFRAMES,
            (dwOptions & MCIAVIO_SKIPFRAMES) ? sz1 : sz0);

    mmWriteProfileInt(szIni, szUSEAVIFILE,
            (dwOptions & MCIAVIO_USEAVIFILE) ? sz1 : sz0);

    mmWriteProfileInt(szIni, szNOSOUND,
            (dwOptions & MCIAVIO_NOSOUND) ? sz1 : sz0);
}


BOOL FAR PASCAL ConfigDialog(HWND hWnd, NPMCIGRAPHIC npMCI)
{
        #define MAX_WINDOWS 10
    HWND                                hWndActive[MAX_WINDOWS];
    BOOL                                fResult = FALSE;
    INT                                 ii;
    HWND                                hWndTop;
    HINSTANCE                   hInst;
    SHOWMMCPLPROPSHEET  fnShow;
    DWORD                               dwOptionFlags;
    TCHAR                               szBuffer[128];
    char                                szCaption[128];
        char                            szTab[40];
        UINT                cchLen;

        // Bugbug:  Remove UnicodeToAnsi Gymnastics as soon as MMSYS.CPL is
        //                      completely Unicode Enabled
    LoadString(ghModule, IDS_VIDEOCAPTION, szBuffer, ARRAYSIZE(szBuffer));
        if (! UnicodeToAnsi (szCaption, szBuffer, ARRAYSIZE(szCaption)))
                return FALSE;

    //Maybe the user is trying to get this dialog back because it is lost somewhere on
    //his/her desktop. Bring back to the top. (SetFocus does not work across threads).
    if (ghwndConfig)
        {
        BringWindowToTop(FindWindow(NULL, (LPCTSTR)szBuffer));
        return FALSE;
    }

    if (hWnd == NULL)
        hWnd = GetActiveWindow();

    //
    // Enumerate all the Top level windows of this task and disable them!
    //
    for (hWndTop = GetWindow(GetDesktopWindow(), GW_CHILD), ii=0;
         hWndTop && ii < MAX_WINDOWS;
         hWndTop = GetWindow(hWndTop, GW_HWNDNEXT)) {

        if (IsWindowEnabled(hWndTop) &&
            IsWindowVisible(hWndTop) &&
                (HTASK)GetWindowTask(hWndTop) == GetCurrentTask() &&
                hWndTop != hWnd)
                {
                        // don't disable our parent
            hWndActive[ii++] = hWndTop;
            EnableWindow(hWndTop, FALSE);
        }
    }

    //
    // Don't let anyone try to bring up another config sheet
    //
    if (hWnd)
                ghwndConfig = hWnd;
    else
                ghwndConfig = (HWND)0x800;    // just in case - make sure it's non-zero

    //
    // Bring up the MCIAVI configure sheet from inside mmsys.cpl
    //
        hInst = LoadLibrary (TEXT ("mmsys.cpl"));
    if (hInst)
        {
                fnShow = (SHOWMMCPLPROPSHEET)GetProcAddress(hInst, "ShowMMCPLPropertySheet");

                if (fnShow)
                {
                        // Note: This string is not localizable
                        static const char szVideo[] = "VIDEO";

                        // Bugbug:  Remove UnicodeToAnsi Gymnastics as soon as MMSYS.CPL is
                        //                      completely Unicode Enabled
                        LoadString(ghModule, IDS_VIDEO, szBuffer, ARRAYSIZE(szBuffer));
                        if (UnicodeToAnsi (szTab, szBuffer, ARRAYSIZE(szTab)))
                        {
                                fResult = fnShow(hWnd, (LPSTR)szVideo, (LPSTR)szTab, (LPSTR)szCaption);

                                //
                                // Make sure the dialog changes get picked up right away
                                // Only change those possibly affected by the dialog.
                                // !!! This is a little hacky; knowing which ones get changed
                                //
                                if (npMCI && fResult)
                                {
                                        dwOptionFlags = ReadConfigInfo();
                                        npMCI->dwOptionFlags &= ~MCIAVIO_WINDOWSIZEMASK;
                                        npMCI->dwOptionFlags &= ~MCIAVIO_ZOOMBY2;
                                        npMCI->dwOptionFlags &= ~MCIAVIO_USEVGABYDEFAULT;
                                        npMCI->dwOptionFlags |= dwOptionFlags &
                                                                                        (MCIAVIO_WINDOWSIZEMASK | MCIAVIO_ZOOMBY2 |
                                                                                         MCIAVIO_USEVGABYDEFAULT);
                                }
                        }
                }

                FreeLibrary(hInst);
        }

    //
    // Restore all windows
    //
    while (ii-- > 0)
        EnableWindow(hWndActive[ii], TRUE);

    if (hWnd)
        SetActiveWindow(hWnd);

    ghwndConfig = NULL;

    return fResult;
}

// This function should be called only to verify that we should be using
//
//
//

#ifdef _WIN32

TCHAR szWow32[] = TEXT("wow32.dll");
CHAR szWOWUseMciavi16Proc[] = "WOWUseMciavi16";

typedef BOOL (*PFNWOWUSEMCIAVI16PROC)(VOID);

BOOL FAR PASCAL WowUseMciavi16(VOID)
{
   HMODULE hWow32;
   BOOL fUseMciavi16 = FALSE;
   PFNWOWUSEMCIAVI16PROC pfnWOWUseMciavi16Proc;

   if (NULL != (hWow32 = GetModuleHandle(szWow32))) {
      pfnWOWUseMciavi16Proc = (PFNWOWUSEMCIAVI16PROC)GetProcAddress(hWow32, szWOWUseMciavi16Proc);
      if (NULL != pfnWOWUseMciavi16Proc) {
         fUseMciavi16 = (*pfnWOWUseMciavi16Proc)();
      }
   }

   return(fUseMciavi16);
}

#endif

