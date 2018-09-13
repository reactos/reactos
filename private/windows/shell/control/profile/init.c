//*************************************************************
//  File name:    INIT.C
//
//  Description:  Initialization code for Profile control panel
//                applet
//
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1992-1994
//  All rights reserved
//
//*************************************************************
#include <windows.h>
#include <cpl.h>
#include "profile.h"


//*************************************************************
//
//  DllInitialize()
//
//  Purpose:    Main entry point
//
//
//  Parameters: HINSTANCE hInstDLL    - Instance handle of DLL
//              DWORD     dwReason    - Reason DLL was called
//              LPVOID    lpvReserved - NULL
//      
//
//  Return:     BOOL
//
//*************************************************************

BOOL DllInitialize(HINSTANCE hInstDLL, DWORD dwReason, LPVOID lpvReserved)
{
    if (dwReason != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    hInstance = hInstDLL;

    DisableThreadLibraryCalls(hInstDLL);

    return TRUE;
}


//*************************************************************
//
//  CPlApplet()
//
//  Purpose:    Control Panel entry point
//
//
//  Parameters: HWND hwnd      - Window handle
//              WORD wMsg      - Control Panel message
//              LPARAM lParam1 - Long parameter
//              LPARAM lParam2 - Long parameter
//
//
//  Return:     LONG
//
//*************************************************************

LONG CPlApplet( HWND hwnd, WORD wMsg, LPARAM lParam1, LPARAM lParam2)
{

    LPNEWCPLINFO lpNewCplInfo;
    LPCPLINFO lpCplInfo;

    switch (wMsg) {

        case CPL_INIT:
            if (CheckProfileType()) {
                uiShellHelp = RegisterWindowMessage (szShellHelp);
                return TRUE;

            } else {
                return FALSE;
            }
            break;

        case CPL_GETCOUNT:
            return (LONG)NUM_APPLETS;

        case CPL_INQUIRE:

            lpCplInfo = (LPCPLINFO)lParam2;

            lpCplInfo->idIcon = ID_ICON;
            lpCplInfo->idName = IDS_NAME;
            lpCplInfo->idInfo = IDS_INFO;
            lpCplInfo->lData  = 0L;

            return (LONG)TRUE;

        case CPL_NEWINQUIRE:

            lpNewCplInfo = (LPNEWCPLINFO)lParam2;

            lpNewCplInfo->hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(ID_ICON));

            if (!LoadString( hInstance, IDS_NAME, lpNewCplInfo->szName,
                            sizeof(lpNewCplInfo->szName))) {
               lpNewCplInfo->szName[0] = TEXT('\0');
            }

            if(!LoadString( hInstance, IDS_INFO, lpNewCplInfo->szInfo,
                            sizeof(lpNewCplInfo->szInfo))) {
               lpNewCplInfo->szInfo[0] = TEXT('\0');
            }

            lpNewCplInfo->dwSize = sizeof(NEWCPLINFO);
            lpNewCplInfo->dwHelpContext = HELP_CONTEXT;
            if(!LoadString( hInstance, IDS_INFO, lpNewCplInfo->szHelpFile,
                            sizeof(lpNewCplInfo->szHelpFile))) {
               lpNewCplInfo->szHelpFile[0] = TEXT('\0');
            }

            return (LONG)TRUE;

        case CPL_DBLCLK:
            RunApplet(hwnd);
            break;
    }
    return (LONG)0;

}

//*************************************************************
//
//  CheckProfileType()
//
//  Purpose:    Checks to see if this user has a floating
//              personal profile.   This is done by looking
//              in the registry for the "ProfileType" entry.
//
//              0 = local non-floating profile
//              1 = personal floating (.usr) profile
//              2 = manditory profile
//
//  Parameters: void
//      
//
//  Return:     BOOL - TRUE if this applet should load
//                     FALSE if not
//
//*************************************************************

BOOL CheckProfileType (void)
{
    LONG  lResult;
    HKEY  hKey;
    DWORD dwType, dwMaxBufferSize;
    TCHAR szTempBuffer [MAX_TEMP_BUFFER];

    //
    // Open the registry key
    //

    lResult = RegOpenKeyEx (HKEY_CURRENT_USER, szProfileRegInfo, 0,
                            KEY_ALL_ACCESS, &hKey);

    if (lResult != ERROR_SUCCESS) {
       return FALSE;
    }

    //
    // Query for the profile path
    //

    dwMaxBufferSize = MAX_TEMP_BUFFER;
    szTempBuffer[0] = TEXT('\0');
    lResult = RegQueryValueEx (hKey, szProfileType, NULL, &dwType,
                              (LPBYTE) szTempBuffer, &dwMaxBufferSize);

    //
    // Close the registry key and return the appropriate response.
    //

    RegCloseKey (hKey);

    if (lResult != ERROR_SUCCESS) {
       return FALSE;
    }

    if (szTempBuffer[0] == PERSONAL_PROFILE_TYPE) {
       return TRUE;

    } else {
       return FALSE;
    }

}
