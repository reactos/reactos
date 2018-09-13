/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    gui.c

Abstract:

    This file implements all access to the registry for WinDbgRm

Author:

    Wesley Witt (wesw) 1-Nov-1993

Environment:

    User Mode

--*/

#include "precomp.h"
#pragma hdrstop


//
// string constants for accessing the registry
// there is a string constant here for each key and each value
// that is accessed in the registry.
//
#define REGKEY_WINDBGRM             "software\\microsoft\\WinDbgRm\\0010"
#define REGKEY_KD_OPTIONS           "Kernel Debugger Options"

#define WS_STR_LONGNAME             "Description"
#define WS_STR_DLLNAME              "Dll_Path"
#define WS_STR_PARAMS               "Parameters"
#define WS_STR_DEFAULT              "Default"

#define WS_STR_KD_VERBOSE           "Verbose"
#define WS_STR_KD_INITIALBP         "Initial Break Point"
#define WS_STR_KD_DEFER             "Defer Symbol Load"
#define WS_STR_KD_MODEM             "Use Modem Controls"
#define WS_STR_KD_PORT              "Port"
#define WS_STR_KD_BAUDRATE          "Baudrate"
#define WS_STR_KD_CACHE             "Cache Size"
#define WS_STR_KD_PLATFORM          "Platform"
#define WS_STR_KD_ENABLE            "Enable"
#define WS_STR_KD_GOEXIT            "Go On Exit"





HKEY
RegGetSubKey(
    HKEY  hKeyWindbgRm,
    LPSTR lpSubKeyName
    )

/*++

Routine Description:

    This function gets a handle to the WINDBGRM registry key.

Arguments:

    None.

Return Value:

    Valid handle   - handle opened ok
    NULL           - could not open the handle

--*/

{
    DWORD   rc;
    HKEY    hSubKey;

    rc = RegOpenKeyEx( hKeyWindbgRm,
                       lpSubKeyName,
                       0,
                       KEY_QUERY_VALUE | KEY_SET_VALUE |
                       KEY_CREATE_SUB_KEY | KEY_ENUMERATE_SUB_KEYS,
                       &hSubKey
                     );

    if (rc != ERROR_SUCCESS) {
        return NULL;
    }

    return hSubKey;
}

// shared by WKSPGetSetProfileProc & WKSPSetupTL
static CIndiv_TL_RM_WKSP * pCTlSelected = NULL;


BOOL
Get_TL_ModuleInfo(
    TCHAR szModuleName[MAX_PATH],
    TCHAR szParams[MAX_PATH]
    )
{
    Assert(pCTlSelected);
    AssertType(*pCTlSelected, CIndiv_TL_RM_WKSP);

    if (!szModuleName && !szParams) {
        return FALSE;
    }

    if (szParams) {
        _tcscpy(szParams, pCTlSelected->m_pszParams);
    }


    if (szModuleName) {
        _tcscpy(szModuleName, pCTlSelected->m_pszDll);
    }

    return TRUE;
}


WKSPSetupTL(
    TLFUNC TLFunc,
    CIndiv_TL_RM_WKSP * pCTl
    )
{
    pCTlSelected = pCTl;

    //
    // Call the TL telling it to configure itself
    //
    TLSS    tlss;

    tlss.fLoad = TRUE;

    //tlss.lParam = NULL;
    tlss.pfnGetModuleInfo = Get_TL_ModuleInfo;

    tlss.fRMAttached = TRUE;

    TLFunc(tlfSetup, 0, 0, (LPARAM) &tlss);

    return 0;
}
