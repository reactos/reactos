/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    sysdm.c 

Abstract:

    Initialization code for System Control Panel Applet

Author:

    Eric Flo (ericflo) 19-Jun-1995

Revision History:

    15-Oct-1997 scotthal
        Complete overhaul

--*/
#include "sysdm.h"
#include <shlobjp.h>


//
// Constants
//
#define SYSDM_NUM_PAGES 5   // Number of PropPages


//
// Global Variables
//
HINSTANCE hInstance;
TCHAR szShellHelp[]       = TEXT("ShellHelp");
TCHAR g_szNull[] = TEXT("");
UINT  uiShellHelp;

TCHAR g_szErrMem[ 200 ];           //  Low memory message
TCHAR g_szSystemApplet[ 100 ];     //  "System Control Panel Applet" title

//
// Function prototypes
//

void 
RunApplet(
    IN HWND hwnd, 
    IN LPTSTR lpCmdLine
);


BOOL 
WINAPI
DllInitialize(
    IN HINSTANCE hInstDLL, 
    IN DWORD dwReason, 
    IN LPVOID lpvReserved
)
/*++

Routine Description:

    Main entry point.

Arguments:

    hInstDLL -
        Supplies DLL instance handle.

    dwReason -
        Supplies the reason DllInitialize() is being called.

    lpvReserved -
        Reserved, NULL.

Return Value:

    BOOL

--*/
{

    if (dwReason == DLL_PROCESS_DETACH)
        MEM_EXIT_CHECK();

    if (dwReason != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    hInstance = hInstDLL;

    return TRUE;
}


LONG 
APIENTRY
CPlApplet( 
    IN HWND hwnd, 
    IN WORD wMsg, 
    IN LPARAM lParam1, 
    IN LPARAM lParam2
)
/*++

Routine Description:

    Control Panel Applet entry point.

Arguments:

    hwnd -
        Supplies window handle.

    wMsg -
        Supplies message being sent.

    lParam1 -
        Supplies parameter to message.

    lParam2 -
        Supplies parameter to message.

Return Value:

    Nonzero if message was handled.
    Zero if message was unhandled. 

--*/
{

    LPCPLINFO lpCPlInfo;

    switch (wMsg) {

        case CPL_INIT:
            uiShellHelp = RegisterWindowMessage (szShellHelp);

            LoadString( hInstance, INITS,   g_szErrMem,       ARRAYSIZE( g_szErrMem ) );
            LoadString( hInstance, INITS+1, g_szSystemApplet, ARRAYSIZE( g_szSystemApplet ) );

            return (LONG) TRUE;

        case CPL_GETCOUNT:
            return 1;

        case CPL_INQUIRE:

            lpCPlInfo = (LPCPLINFO)lParam2;

            lpCPlInfo->idIcon = ID_ICON;
            lpCPlInfo->idName = IDS_NAME;
            lpCPlInfo->idInfo = IDS_INFO;

            return (LONG) TRUE;

        case CPL_DBLCLK:

            lParam2 = 0L;
            // fall through...

        case CPL_STARTWPARMS:
            RunApplet(hwnd, (LPTSTR)lParam2);
            return (LONG) TRUE;
    }
    return (LONG)0;

}


void 
RunApplet(
    IN HWND hwnd, 
    IN LPTSTR lpCmdLine
)
/*++

Routine Description:

    CPL_STARTWPARMS message handler.  Called when the user
    runs the Applet.

    PropSheet initialization occurs here.

Arguments:

    hwnd -
        Supplies window handle.

    lpCmdLine -
        Supplies the command line used to invoke the applet.

Return Value:

    none

--*/
{
    HPROPSHEETPAGE hPages[SYSDM_NUM_PAGES];
    PROPSHEETHEADER psh;
    UINT iPage = 0;


    hPages[iPage] = CreateGeneralPage(hInstance);
    if (hPages[iPage] != NULL) {
        iPage++;
    } // if

    hPages[iPage] = CreateNetIDPage(hInstance);
    if (hPages[iPage] != NULL) {
        iPage++;
    } // if

    hPages[iPage] = CreateHardwarePage(hInstance);
    if (hPages[iPage] != NULL) {
        iPage++;
    } // if

    hPages[iPage] = CreateProfilePage(hInstance);
    if (hPages[iPage] != NULL) {
        iPage++;
    } // if

    hPages[iPage] = CreateAdvancedPage(hInstance);
    if (hPages[iPage] != NULL) {
        iPage++;
    } // if

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = 0;
    psh.hwndParent = hwnd;
    psh.hInstance = hInstance;
    psh.pszCaption = MAKEINTRESOURCE(IDS_TITLE);
    psh.nPages = iPage++;
    psh.nStartPage = ( ( lpCmdLine && *lpCmdLine )? StringToInt( lpCmdLine ) : 0 );
    psh.phpage = hPages;

    if (PropertySheet (&psh) == ID_PSREBOOTSYSTEM) {
        RestartDialog (hwnd, NULL, EWX_REBOOT);
    } // if

    return;
}
