#include <windows.h>
#include <cpl.h>

//
// Marks code that will need to be modified if this module is ever
// incorporated into MAIN.CPL instead of being a separate applet.
//
#define NOTINMAIN


#ifdef NOTINMAIN
#include "..\main\cphelp.h"         //For the help id's.
#endif //NOTINMAIN


#include "cursors.h"

#define szPREVIEW   "SPreviewWndClass"

HANDLE ghmod;
int gcxCursor, gcyCursor;


LONG APIENTRY CPlApplet(
    HWND  hwnd,
    WORD  message,
    DWORD wParam,
    LONG  lParam)
{
    LPCPLINFO lpCPlInfo;
    LPNEWCPLINFO lpNCPlInfo;
    WNDCLASS cls;

    switch (message) {
    case CPL_INIT:              // Is any one there ?
        gcxCursor = GetSystemMetrics(SM_CXCURSOR);
        gcyCursor = GetSystemMetrics(SM_CYCURSOR);

        /*
         * Load ONE string for emergencies.
         */
        LoadString(ghmod, IDS_CUR_NOMEM, gszNoMem,
                sizeof(gszNoMem) / sizeof(TCHAR));

        /*
         * Register a new window class to handle the cursor preview.
         */
        cls.style = CS_GLOBALCLASS;
        cls.lpfnWndProc = PreviewWndProc;
        cls.cbClsExtra = 0;
        cls.cbWndExtra = 0;
        cls.hInstance = ghmod;
        cls.hIcon = NULL;
        cls.hCursor = NULL;
        cls.hbrBackground = NULL;
        cls.lpszMenuName = NULL;
        cls.lpszClassName = szPREVIEW;
        RegisterClass(&cls);
        return TRUE;

    case CPL_GETCOUNT :         // How many applets do you support ?
        return 1;

    case CPL_INQUIRE :          // Fill CplInfo structure
        lpCPlInfo = (LPCPLINFO)lParam;

        lpCPlInfo->idIcon = CURSORSICON;
        lpCPlInfo->idName = IDS_NAME;
        lpCPlInfo->idInfo = IDS_INFO;
        lpCPlInfo->lData  = 0;
        break;

    case CPL_NEWINQUIRE:

        lpNCPlInfo = (LPNEWCPLINFO)lParam;

        lpNCPlInfo->hIcon = LoadIcon(ghmod, MAKEINTRESOURCE(CURSORSICON));
        LoadString(ghmod, IDS_NAME, lpNCPlInfo->szName, sizeof(lpNCPlInfo->szName));

        if (!LoadString(ghmod, IDS_INFO, lpNCPlInfo->szInfo, sizeof(lpNCPlInfo->szInfo)))
            lpNCPlInfo->szInfo[0] = 0;

        lpNCPlInfo->dwSize = sizeof(NEWCPLINFO);
        lpNCPlInfo->lData  = 0;
        lpNCPlInfo->dwHelpContext = IDH_CHILD_CURSORS;
        strcpy(lpNCPlInfo->szHelpFile, xszControlHlp);

        return TRUE;

    case CPL_DBLCLK:            // You have been chosen to run
        /*
         * One of your applets has been double-clicked.
         *      wParam is an index from 0 to (NUM_APPLETS-1)
         *      lParam is the lData value associated with the applet
         */
        DialogBox(ghmod, MAKEINTRESOURCE(DLG_CURSORS), hwnd, (DLGPROC)CursorsDlgProc);
        break;

    case CPL_EXIT:              // You must really die
    case CPL_STOP:              // You must die
        UnregisterClass(szPREVIEW, ghmod);
        break;

    case CPL_SELECT:            // You have been selected
        /*
         * Sent once for each applet prior to the CPL_EXIT msg.
         *      wParam is an index from 0 to (NUM_APPLETS-1)
         *      lParam is the lData value associated with the applet
         */
        break;
    }

    return 0L;
}


BOOL DllInitialize(
    IN PVOID hmod,
    IN ULONG ulReason,
    IN PCONTEXT pctx OPTIONAL)
{
    UNREFERENCED_PARAMETER(pctx);

    if (ulReason != DLL_PROCESS_ATTACH)
        return TRUE;

    ghmod = hmod;

    return TRUE;
}


#ifdef CAIROBLD
// hacks for Cairo build environment
// 4/08/93 - ErikGav
BOOL DllEntryPoint(
    IN PVOID hmod,
    IN ULONG ulReason,
    IN PCONTEXT pctx OPTIONAL)
{
    return DllInitialize(hmod,ulReason,pctx);
}

void CDECL RegisterWithCommnot(void)
{
}

void CDECL DeRegisterWithCommnot(void)
{
}
#endif

