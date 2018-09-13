//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       dllmain.cxx
//
//  Contents:   DLL initialization entrypoint and global variables
//
//  History:    4-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

//--------------------------------------------------------------------------
// Globals used elsewhere

UINT        g_NonOLEDLLRefs = 0;
HINSTANCE   g_hInstance = NULL;

//--------------------------------------------------------------------------
// Debugging

DECLARE_INFOLEVEL(NetObjectsUI)

//--------------------------------------------------------------------------

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
//         NetObjectsUIInfoLevel = DEB_ERROR | DEB_TRACE;
        NetObjectsUIInfoLevel = DEB_ERROR;
        SetWin4AssertLevel(ASSRT_BREAK | ASSRT_MESSAGE);
#endif // DBG == 1

        appDebugOut((DEB_TRACE, "ntlanui2.dll: DllMain enter\n"));

        // Disable thread notification from OS
        DisableThreadLibraryCalls(hInstance);
        g_hInstance = hInstance;
        InitCommonControls();
        break;
    }

    case DLL_PROCESS_DETACH:
        appDebugOut((DEB_TRACE, "ntlanui2.dll: DllMain leave\n"));
        break;
    }

    return TRUE;
}

extern HRESULT PropDummyFunction();
HRESULT Linkage()
{
    return PropDummyFunction();
}
