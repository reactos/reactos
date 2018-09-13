/*++

Copyright (c) 1995-1996 Microsoft Corporation

Module Name:

    main

Abstract:

    Trivial WinMain() function, creates tabbed dialog "property pages"
    and then creates the dialog... interesting code is in dialog procedures.

Author:

    Steve Firebaugh (stevefir)         31-Dec-1995

Revision History:

    SPORDER.EXE, DLL, & LIB were shipped in Win32 SDK along with NT4.

Comments:

    Code is generally ready to be compiled with UNICODE defined, however,
     we do not make use of this because EXE and DLL must also work on
     Windows 95.

--*/


#include <windows.h>
#include <winsock2.h>
#include <commctrl.h>
#include "globals.h"


HINSTANCE ghInst;


int
APIENTRY
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
    )
{

    PROPSHEETPAGE psp[3];
    PROPSHEETHEADER psh;
    WSADATA WSAData;
    int iTab = 0;
    int r;
    DWORD dwWait;
    HANDLE hMutex;
    TCHAR pszMutextName[] = TEXT("sporder.exe");


    DBGOUT((TEXT("checked build.\n")));

    //
    // It is possible that we will have multiple instances running at the
    //  same time... what we really want is the first to finish before the
    //  second really gets going... for that reason, wait here on mutex
    //

    hMutex = CreateMutex (NULL, FALSE, pszMutextName);
    hMutex = OpenMutex (SYNCHRONIZE, FALSE, pszMutextName);
    dwWait = WaitForSingleObject (hMutex, 0);
    if (dwWait == WAIT_TIMEOUT)
    {
        OutputDebugString (TEXT("WaitForSingleObject, WAIT_TIMEOUT\n"));
        return TRUE;
    }

    //
    // Do global initializations.
    //

    ghInst = hInstance;
    InitCommonControls();
    memset (psp, 0, sizeof (psp));
    memset (&psh, 0, sizeof (psh));


    if (WSAStartup(MAKEWORD (2,2),&WSAData) == SOCKET_ERROR) {
      OutputDebugString (TEXT("WSAStartup failed\n"));
      return -1;
    }


    psp[iTab].dwSize = sizeof(PROPSHEETPAGE);
    psp[iTab].dwFlags = PSP_USETITLE;
    psp[iTab].hInstance = ghInst;
    psp[iTab].pszTemplate = TEXT("WS2SPDlg");
    psp[iTab].pszIcon = TEXT("");
    psp[iTab].pfnDlgProc = (DLGPROC) SortDlgProc;
    psp[iTab].pszTitle = TEXT("Service Providers");
    psp[iTab].lParam = 0;
    iTab++;


    psp[iTab].dwSize = sizeof(PROPSHEETPAGE);
    psp[iTab].dwFlags = PSP_USETITLE;
    psp[iTab].hInstance = ghInst;
    psp[iTab].pszTemplate = TEXT("RNRSPDlg");
    psp[iTab].pszIcon = TEXT("");
    psp[iTab].pfnDlgProc = RNRDlgProc;
    psp[iTab].pszTitle = TEXT("Name Resolution ");
    psp[iTab].lParam = 0;
    iTab++;

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE ; // | PSH_NOAPPLYNOW  ; // | PSH_HASHELP ;
    psh.hwndParent = NULL;
    psh.hInstance = ghInst;
    psh.pszIcon = TEXT("");
    psh.pszCaption = TEXT("Windows Sockets Configuration");
    psh.nPages = iTab;
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;

    //
    // Finally display the dialog with the property sheets.
    //

//
// Sundown: Possible truncation here from INT_PTR to int in the return value.
//          However, WinMain returns an exit value which is still a 32bit value.
//

    r = (int)PropertySheet(&psh);

    //
    // Cleanup sockets, release mutex, and close handle
    //

    WSACleanup ();
    ReleaseMutex (hMutex);
    CloseHandle (hMutex);

    return r;
}


#if DBG
void
_cdecl
DbgPrint(
    PTCH Format,
    ...
    )
/*++

  Write debug output messages if compiled with DEBUG

--*/
{
    TCHAR buffer[MAX_PATH];

    va_list marker;
    va_start (marker,Format);
    wvsprintf (buffer,Format, marker);
    OutputDebugString (TEXT("SPORDER.EXE: "));
    OutputDebugString (buffer);

    return;
}
#endif
