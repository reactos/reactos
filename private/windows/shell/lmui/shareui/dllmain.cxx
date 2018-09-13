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
#include <setupapi.h>
#include "resource.h"
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
//         SharingInfoLevel = DEB_ERROR | DEB_TRACE;
        SharingInfoLevel = DEB_ERROR;
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
        break;
    }

    return TRUE;
}


//
// Procedure for uninstalling this DLL (given an INF file). Note: this DLL
// really should dynamically load setupapi.dll, to avoid its overhead all the
// time.
//
void WINAPI
UninstallW(
  HWND hwndStub,
  HINSTANCE hInstance,
  LPTSTR lpszCmdLine,
  int nCmdShow
  )
{
    RUNDLLPROC pfnCheckAPI = UninstallW;  // let compiler check the prototype

    if (!lpszCmdLine || lstrlen(lpszCmdLine) >= MAX_PATH)
    {
        return;
    }

    TCHAR szSure[200];
    LoadString(g_hInstance, IDS_SUREUNINST, szSure, ARRAYLEN(szSure));
    TCHAR szTitle[200];
    LoadString(g_hInstance, IDS_MSGTITLE, szTitle, ARRAYLEN(szTitle));

    if (MessageBox(hwndStub, szSure, szTitle, MB_YESNO | MB_ICONSTOP) != IDYES)
    {
        return;
    }

    HINF hinf = SetupOpenInfFile(
                  lpszCmdLine,  // should be the name of the inf
                  NULL,         // optional Version section CLASS info
                  INF_STYLE_WIN4,
                  NULL);        // optional error line info
    if (INVALID_HANDLE_VALUE == hinf)
    {
        appDebugOut((DEB_ERROR,
            "SetupOpenInfFile failed, 0x%x\n",
            GetLastError()));
        return;
    }

    PVOID pContext = SetupInitDefaultQueueCallback(hwndStub);

    BOOL ret = SetupInstallFromInfSection(
                  hwndStub,     // optional, handle of a parent window
                  hinf,         // handle to the INF file
                  TEXT("DefaultUninstall"), // section of the INF file to install
                  SPINST_REGISTRY | SPINST_FILES,   // which lines to install from section
                  HKEY_CURRENT_USER,    // optional, key for registry installs
                  NULL,                 // optional, path for source files
                  SP_COPY_FORCE_NEWER,  // optional, specifies copy behavior
                  SetupDefaultQueueCallback, // optional, specifies callback routine
                  pContext,             // optional, callback routine context
                  NULL,                 // optional, device information set
                  NULL);                // optional, device info structure
    if (!ret)
    {
      appDebugOut((DEB_ERROR,
          "SetupInstallFromInfSection failed, 0x%x\n",
          GetLastError()));
    }

    SetupCloseInfFile(hinf);
}
