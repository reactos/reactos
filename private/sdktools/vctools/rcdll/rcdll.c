/****************************************************************************/
/*                                                                          */
/*  RCDLL.C -                                                               */
/*                                                                          */
/*    Windows 3.5 Resource Compiler - Main Module                           */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

#include "rc.h"
#include <setjmp.h>


/* Module handle */
HINSTANCE hInstance;
HWND      hWndCaller;

RC_MESSAGE_CALLBACK  lpfnMessageCallback;
RC_PARSE_CALLBACK lpfnParseCallback;


/* Function prototypes */
int __cdecl rc_main(int, char**);
int __cdecl rcpp_main(int argc, PWCHAR*argv);


#ifdef __cplusplus
extern "C"
#endif
BOOL
WINAPI
DllMain(
    HINSTANCE hDll,
    DWORD dwReason,
    LPVOID lpReserved
    )
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        hInstance = hDll;
        DisableThreadLibraryCalls(hDll);
    }
    return TRUE;
}


#ifdef __cplusplus
extern "C"
#endif
int CALLBACK
RC(
    HWND hWnd,
    int fStatus,
    RC_MESSAGE_CALLBACK lpfnMsg,
    RC_PARSE_CALLBACK lpfnParse,
    int argc,
    char**argv
    )
{
    hWndCaller     = hWnd;

    lpfnMessageCallback = lpfnMsg;
    lpfnParseCallback = lpfnParse;

    return (rc_main(argc, argv));
}


int
RCPP(
    int argc,
    PCHAR *argv,
    PCHAR env
    )
{
    WCHAR    **wargv;

    wargv = UnicodeCommandLine(argc, argv);
    return rcpp_main(argc, wargv);
}


void
SendWarning(
    PSTR str
    )
{
    if (lpfnMessageCallback)
        (*lpfnMessageCallback)(0, 0, str);

    if (hWndCaller) {
        if (SendMessageA(hWndCaller, WM_RC_ERROR, FALSE, (LPARAM)str) != 0) {
            quit("\n");
        }
    }
}

void
SendError(
    PSTR str
    )
{
    static int cErrThisLine = 0;
    static int LastRow = 0;

    if (lpfnMessageCallback)
        (*lpfnMessageCallback)(0, 0, str);

    if (hWndCaller) {
        if (SendMessageA(hWndCaller, WM_RC_ERROR, FALSE, (LPARAM)str) != 0) {
            quit("\n");
        }
    }

    if (token.row == LastRow) {
        if (++cErrThisLine > 4 && strcmp(str, "\n"))
            quit("\n");
    } else {
        LastRow = token.row;
        cErrThisLine = 0;
    }
}


void
UpdateStatus(
    unsigned nCode,
    unsigned long dwStatus
    )
{
    if (hWndCaller) {
        if (SendMessageA(hWndCaller, WM_RC_STATUS, nCode, dwStatus) != 0) {
            quit("\n");
        }
    }
}
