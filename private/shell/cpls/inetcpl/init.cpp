//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1995                    **
//*********************************************************************

//
//    INIT.C - Initialization code for Internet control panel
//

//    HISTORY:
//    
//    4/3/95    jeremys        Created.
//

#include "inetcplp.h"
// external calls and defs
#include <inetcpl.h>

#define MLUI_INIT
#include <mluisupp.h>

HINSTANCE ghInstance=NULL;

extern HMODULE hOLE32;
DWORD g_dwtlsSecInitFlags;
BOOL g_bMirroredOS = FALSE;
HMODULE g_hOleAcc;
BOOL g_fAttemptedOleAccLoad = FALSE;

STDAPI_(BOOL) LaunchInternetControlPanelAtPage(HWND hDlg, UINT nStartPage);
BOOL IsCompatModeProcess(void);

/*******************************************************************

    NAME:        DllEntryPoint

    SYNOPSIS:    Entry point for DLL.

********************************************************************/
STDAPI_(BOOL) DllMain(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved)
{
    if( fdwReason == DLL_PROCESS_ATTACH ) {
        if (IsCompatModeProcess())
            // Fail loading in compat mode process
            return 0;
        ghInstance = hInstDll;
        MLLoadResources(ghInstance, TEXT("inetcplc.dll"));
#ifndef REPLACE_PROPSHEET_TEMPLATE
        InitMUILanguage(MLGetUILanguage());
#endif

#ifdef DEBUG
        CcshellGetDebugFlags();
#endif
        // Thread local storage used in security.cpp
        g_dwtlsSecInitFlags = TlsAlloc();
        g_bMirroredOS = IS_MIRRORING_ENABLED();
        TlsSetValue(g_dwtlsSecInitFlags, (void *) new SECURITYINITFLAGS);

    } else if (fdwReason == DLL_PROCESS_DETACH) {
        MLFreeResources(ghInstance);

        if (g_hwndUpdate) {
            // we've got this subclassed.
            // if it's still valid as we leave, we need 
            // to destroy it so that it doesn't fault trying to access our info
            DestroyWindow(g_hwndUpdate);
        }
        
        if(hOLE32)
        {
            FreeLibrary(hOLE32);
            hOLE32 = NULL;

        }

        if (g_hOleAcc)
        {
            FreeLibrary(g_hOleAcc);
            g_hOleAcc = NULL;
            g_fAttemptedOleAccLoad = FALSE;
        }

        // free tls used in security.cpp
        if(g_dwtlsSecInitFlags != (DWORD) -1)
        {
            SECURITYINITFLAGS * psif = NULL;
            psif = (SECURITYINITFLAGS *) TlsGetValue(g_dwtlsSecInitFlags);
            if(psif)
            {
                delete psif;
                psif = NULL;
            }
            TlsFree(g_dwtlsSecInitFlags);
        }
    }
    return TRUE;
}


BOOL RunningOnNT()
{
    return !(::GetVersion() & 0x80000000);
}


/*******************************************************************

    NAME:        CPlApplet

    SYNOPSIS:    Entry point for control panel.

********************************************************************/
STDAPI_(LRESULT) CPlApplet         // Control panel applet procedure
(
    HWND        hwndCpl,            // Control panel parent window
    UINT        uMsg,               // message
    LPARAM      lParam1,            // value depends on message
    LPARAM      lParam2             // value depends on message
)
{

    LPNEWCPLINFO lpNewCplInfo = (LPNEWCPLINFO) lParam2;
    LPCPLINFO lpCplInfo = (LPCPLINFO) lParam2;
    DWORD dwNIcons;

    switch (uMsg)
    {
    case CPL_INIT:
        //  Initialization message from Control Panel
        return TRUE;

    case CPL_GETCOUNT:
        /* We always have the main internet CPL icon; on Win95 platforms,
         * we also have the Users icon if mslocusr.dll is present.
         */
        dwNIcons = 1;
        if (!RunningOnNT())
        {
            TCHAR szPath[MAX_PATH];

            // check if mslocusr.dll is present in the system dir
            if (GetSystemDirectory(szPath, ARRAYSIZE(szPath)))
            {
                PathAppend(szPath, TEXT("mslocusr.dll"));
                if (PathFileExists(szPath))
                    dwNIcons++;
            }
        }
        return dwNIcons;

    case CPL_INQUIRE:
        /* CPL #0 is the main Internet CPL, #1 (the only other one we'll ever
         * be asked about) is the Users CPL.
         */
        if (!lParam1) {
            lpCplInfo->idIcon = IDI_INTERNET;
            lpCplInfo->idName = IDS_INTERNET;
            lpCplInfo->idInfo = IDS_DESCRIPTION;
            lpCplInfo->lData = 0;
        }
        else {
            lpCplInfo->idIcon = IDI_USERS;
            lpCplInfo->idName = IDS_USERS;
            lpCplInfo->idInfo = IDS_USERS_DESCRIPTION;
            lpCplInfo->lData = 0;
        }
        return FALSE;

    case CPL_NEWINQUIRE:

        // Return new-style info structure for Control Panel

        // By not responding to NEWINQUIRE, Win95 will not preload our
        // .cpl file; by extension, since we are statically linked to MSHTML's
        // import library, MSHTML will also not be loaded.  If we respond to
        // this, then our cpl and MSHTML (>600k) are both loaded when the
        // control panel is just open.  (IE, they will be loaded even if the
        // user has not selected to invoke our specific cpl applet.

        return TRUE;   // TRUE == we are NOT responding to this
        break;

    case CPL_DBLCLK:

        //
        // This means the user did not specify a particular page
        //
        lParam2 = 0;

        // fall through

    case CPL_STARTWPARMSA:
    case CPL_STARTWPARMSW:

        /* CPL #0 is the main Internet CPL, #1 (the only other one we'll ever
         * be asked about) is the Users CPL.  The Users CPL is loaded from
         * mslocusr.dll dynamically.  The entrypoint is structured as a
         * rundll32 entrypoint.
         */
        if (!lParam1) {
        
            //
            // If lParam2!=NULL, then the user specified a page on the command line
            //
            if (lParam2)
            {
                UINT nPage;
                if (CPL_STARTWPARMSA == uMsg)
                    nPage = StrToIntA((LPSTR)lParam2);
                else
                    nPage = StrToIntW((LPWSTR)lParam2);

                LaunchInternetControlPanelAtPage(hwndCpl, nPage);
                
            }

            //
            // Otherwise request the default page
            //
            else
                LaunchInternetControlPanelAtPage(hwndCpl,DEFAULT_CPL_PAGE);
            
            

        }
        else {
            HINSTANCE hinstMSLU = LoadLibrary(TEXT("mslocusr.dll"));
            if (hinstMSLU != NULL) {
                typedef void (*PFNRUNDLL)(HWND hwndParent, HINSTANCE hinstEXE, LPSTR pszCmdLine, int nCmdShow);
                PFNRUNDLL pfn = (PFNRUNDLL)GetProcAddress(hinstMSLU, "UserCPL");
                if (pfn != NULL) {
                    (*pfn)(hwndCpl, NULL, "", SW_SHOW);
                }
                FreeLibrary(hinstMSLU);
            }
        }
        return TRUE;

    case CPL_EXIT:
        // Control Panel is exiting
        break;

    default:
        break;
    }

    return 0L;
}

