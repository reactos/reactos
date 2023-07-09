#include <windows.h>
#include <cpl.h>
#include <cplp.h>
#include "settings.h"
#include "setinc.h"

TCHAR szControlHlp[] = TEXT("control.hlp");
UINT wHelpMessage;

EXEC_MODE gbExecMode = EXEC_NORMAL;
EXEC_INVALID_SUBMODE gbInvalidMode = NOT_INVALID;

HINSTANCE ghmod;

HWND hwndDevModeNotify = (HWND)0;

LONG APIENTRY SettingsCPlApplet(
    HWND  hwnd,
    WORD  message,
    DWORD wParam,
    LONG  lParam)
{
    LPCPLINFO lpCPlInfo;
    LPNEWCPLINFO lpNCPlInfo;

    switch (message)
    {
      case CPL_INIT:

        if (wParam == CPL_INIT_DEVMODE_TAG) {
            if (IsWindow((HWND)lParam)) {
                hwndDevModeNotify = (HWND)lParam;
            }
        }
        return TRUE;
#if 0
      case CPL_DBLCLK:          // You have been chosen to run
        /*
         * One of your applets has been double-clicked.
         *      wParam is an index from 0 to (NUM_APPLETS-1)
         *      lParam is the lData value associated with the applet
         */
        DisplayDialogBox((HINSTANCE)ghmod,
            hwnd);
        break;
#endif

    }

    return 0L;
}


BOOL SettingsDllInitialize(
    IN PVOID hmod,
    IN ULONG ulReason,
    IN PCONTEXT pctx OPTIONAL)
{
    UNREFERENCED_PARAMETER(pctx);

    ghmod = hmod;

    wHelpMessage   = RegisterWindowMessage(TEXT("ShellHelp"));

    return TRUE;
}


VOID __cdecl _purecall( void ) {
    MessageBox(GetDesktopWindow(),
        TEXT("Pure virtual function was called!"),
        NULL, MB_ICONSTOP | MB_OK );
}
