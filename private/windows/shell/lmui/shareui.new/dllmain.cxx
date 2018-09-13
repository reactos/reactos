//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       dllmain.hxx
//
//  Contents:   DLL initialization entrypoint and global variables
//
//  History:    4-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include <locale.h>
#include "util.hxx"

const TCHAR c_szShellIDList[] = CFSTR_SHELLIDLIST;

//+--------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   Win32 DLL initialization function
//
//  Arguments:  [hInstance] - Handle to this dll
//              [dwReason]  - Reason this function was called.  Can be
//                            Process/Thread Attach/Detach.
//
//  Returns:    BOOL    - TRUE if no error.  FALSE otherwise
//
//  History:    4-Apr-95 BruceFo  Created
//
//---------------------------------------------------------------------------

extern "C"
BOOL
DllMain(
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID lpReserved
    )
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
    {
#if DBG == 1
        InitializeDebugging();
        SharingInfoLevel = DEB_ERROR | DEB_TRACE;
//         SharingInfoLevel = DEB_ERROR;
        SetWin4AssertLevel(ASSRT_BREAK | ASSRT_MESSAGE);
#endif // DBG == 1

        appDebugOut((DEB_TRACE, "shareui.dll: DllMain enter\n"));

        // Disable thread notification from OS
        DisableThreadLibraryCalls(hInstance);
        g_hInstance = hInstance;
        InitCommonControls();   // get up/down control
        setlocale(LC_CTYPE, ""); // set the C runtime library locale, for string operations
        g_cfHIDA = RegisterClipboardFormat(c_szShellIDList);

        // Determine the maximum number of users
        g_uiMaxUsers = IsWorkstationProduct()
                            ? MAX_USERS_ON_WORKSTATION
                            : MAX_USERS_ON_SERVER
                            ;

        break;
    }

    case DLL_PROCESS_DETACH:
        appDebugOut((DEB_TRACE, "shareui.dll: DllMain leave\n"));
        UnloadSFMSupportLibrary();
        UnloadFPNWSupportLibrary();
        break;
    }

    return TRUE;
}
