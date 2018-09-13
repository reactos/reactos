//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dll.cpp
//
//  Core entry points for the DLL
//
//--------------------------------------------------------------------------

#include "aclpriv.h"


/*----------------------------------------------------------------------------
/ Globals
/----------------------------------------------------------------------------*/

HINSTANCE hModule = NULL;
HINSTANCE g_hGetUserLib = NULL;

UINT UM_SIDLOOKUPCOMPLETE = 0;
UINT g_cfDsSelectionList = 0;
UINT g_cfSidInfoList = 0;


/*-----------------------------------------------------------------------------
/ DllMain
/ -------
/   Main entry point.  We are passed reason codes and assored other
/   information when loaded or closed down.
/
/ In:
/   hInstance = our instance handle
/   dwReason = reason code
/   pReserved = depends on the reason code.
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
STDAPI_(BOOL)
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*pReserved*/)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        hModule = hInstance;
        DebugProcessAttach();
        TraceSetMaskFromRegKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\AclUI"));
#ifndef DEBUG
        DisableThreadLibraryCalls(hInstance);
#endif
        RegisterCheckListWndClass();

        UM_SIDLOOKUPCOMPLETE = RegisterWindowMessage(TEXT("ACLUI SID Lookup Complete"));
#if(_WIN32_WINNT >= 0x0500)
        g_cfDsSelectionList = RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);
#endif
        g_cfSidInfoList = RegisterClipboardFormat(CFSTR_ACLUI_SID_INFO_LIST);
    	break;

    case DLL_PROCESS_DETACH:
        FreeSidCache();
        if (g_hGetUserLib)
            FreeLibrary(g_hGetUserLib);
        DebugProcessDetach();
        break;

    case DLL_THREAD_DETACH:
        DebugThreadDetach();
        break;
    }

    return TRUE;
}
