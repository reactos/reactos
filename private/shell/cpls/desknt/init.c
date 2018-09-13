#include <windows.h>
#include <cpl.h>
#include <commctrl.h>
#include "rc.h"
#include "deskid.h"
#include "desk.h"
#include "setinc.h"

//
// Marks code that will need to be modified if this module is ever
// incorporated into MAIN.CPL instead of being a separate applet.
//
#define NOTINMAIN


#ifdef NOTINMAIN
//#include "..\main\cphelp.h"         //For the help id's.
#endif //NOTINMAIN


//#define szPREVIEW   TEXT("PreviewWndClass")

LONG APIENTRY CPlApplet(
    HWND  hwnd,
    WORD  message,
    DWORD wParam,
    LONG  lParam)
{
    LPCPLINFO lpCPlInfo;
    LPNEWCPLINFO lpNCPlInfo;
    WNDCLASS cls;

    switch (message)
    {
      case CPL_INIT:          // Is any one there ?

        /*
         * Init the common controls
         */
        if (!DeskInitCpl())
            return 0;


        // Init the settings page code
        SettingsCPlApplet( hwnd, message, wParam, lParam );

        /*
         * Load ONE string for emergencies.
         */
        LoadString (hInstance, IDS_DESK_NOMEM, gszNoMem, ARRAYSIZE(gszNoMem));
        LoadString (hInstance, IDS_DISPLAY_TITLE, gszDeskCaption, ARRAYSIZE(gszDeskCaption));

        return !0;

      case CPL_GETCOUNT:        // How many applets do you support ?
        return 1;

      case CPL_INQUIRE:         // Fill CplInfo structure
        lpCPlInfo = (LPCPLINFO)lParam;

        lpCPlInfo->idIcon = IDI_DISPLAY;
        lpCPlInfo->idName = IDS_NAME;
        lpCPlInfo->idInfo = IDS_INFO;
        lpCPlInfo->lData  = 0;
        break;

    case CPL_NEWINQUIRE:

        lpNCPlInfo = (LPNEWCPLINFO)lParam;

        lpNCPlInfo->hIcon = LoadIcon(hInstance, (LPTSTR) MAKEINTRESOURCE(IDI_DISPLAY));
        LoadString(hInstance, IDS_NAME, lpNCPlInfo->szName, ARRAYSIZE(lpNCPlInfo->szName));

        if (!LoadString(hInstance, IDS_INFO, lpNCPlInfo->szInfo, ARRAYSIZE(lpNCPlInfo->szInfo)))
            lpNCPlInfo->szInfo[0] = (TCHAR) 0;

        lpNCPlInfo->dwSize = sizeof( NEWCPLINFO );
        lpNCPlInfo->lData  = 0;
#if 0
        lpNCPlInfo->dwHelpContext = IDH_CHILD_DISPLAY;
        lstrcpy(lpNCPlInfo->szHelpFile, xszControlHlp);
#else
        lpNCPlInfo->dwHelpContext = 0;
        lstrcpy(lpNCPlInfo->szHelpFile, TEXT(""));
#endif

        return TRUE;

      case CPL_DBLCLK:          // You have been chosen to run
        /*
         * One of your applets has been double-clicked.
         *      wParam is an index from 0 to (NUM_APPLETS-1)
         *      lParam is the lData value associated with the applet
         */
        lParam = 0L;
        // fall through...

      case CPL_STARTWPARMS:
        DeskShowPropSheet( hInstance, hwnd, (LPTSTR)lParam );
        return TRUE;            // Tell RunDLL.exe that I succeeded

      case CPL_EXIT:            // You must really die
      case CPL_STOP:            // You must die
        break;

      case CPL_SELECT:          // You have been selected
        /*
         * Sent once for each applet prior to the CPL_EXIT msg.
         *      wParam is an index from 0 to (NUM_APPLETS-1)
         *      lParam is the lData value associated with the applet
         */
        break;

      //
      //  Private message sent when this applet is running under "Setup"
      //
      case CPL_SETUP:

        gbExecMode = EXEC_SETUP;
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


    if (ulReason == DLL_PROCESS_ATTACH)
    {
        SettingsDllInitialize( hmod, ulReason, pctx );

        hInstance = hmod;

        DisableThreadLibraryCalls(hInstance);
    }
        
    return TRUE;
}
