/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    dllinit.cpp

Abstract:

    This module implements the dll related function

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "devmgr.h"
#include "factory.h"

LPCTSTR DEVMGR_DEVICEID_SWITCH  = TEXT("DMDeviceId");
LPCTSTR DEVMGR_MACHINENAME_SWITCH = TEXT("DMMachineName");
LPCTSTR DEVMGR_COMMAND_SWITCH      = TEXT("DMCommand");

//
// DLL main entry point
// INPUT:
//  HINSTANCE hInstance -- module instance handle
//  DWORD     dwReason  -- the reason why we are called.
//  LPVOID    lpReserved -- no used here

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

        // we do not need thread attach/detach calls
        DisableThreadLibraryCalls(hInstance);

        // do must be done
        InitCommonControls();

        INITCOMMONCONTROLSEX icce;
        
        icce.dwSize = sizeof(icce);
        icce.dwICC = ICC_DATE_CLASSES;
        InitCommonControlsEx(&icce);

        // initiailze our global stuff
        InitGlobals(hInstance);

        break;

    case DLL_PROCESS_DETACH:
        // do the clean up here.....
        break;
    }
    return(TRUE);
}

BOOL InitGlobals(
    HINSTANCE hInstance
    )
{
    g_hInstance = hInstance;
    
    // preload memory allocation error message
    TCHAR tszTemp[256];
    ::LoadString(hInstance, IDS_ERROR_NOMEMORY, tszTemp, ARRAYLEN(tszTemp));
    g_MemoryException.SetMessage(tszTemp);
    ::LoadString(hInstance, IDS_NAME_DEVMGR, tszTemp, ARRAYLEN(tszTemp));
    g_MemoryException.SetCaption(tszTemp);
    
    try
    {
        //preload strings
        g_strDevMgr.LoadString(hInstance, IDS_NAME_DEVMGR);

#if DEVL
        // Initialize debug options
        {
            DWORD Type, Size, Options;
            CSafeRegistry regDevMgr;
            Size = sizeof(g_DebugBreakOptions);
                
            if (!regDevMgr.Open(HKEY_LOCAL_MACHINE, REG_PATH_DEVICE_MANAGER) ||
                !regDevMgr.GetValue(REG_VAL_DEVMGR_DEBUGBREAKOPTIONS, &Type, (BYTE*)&g_DebugBreakOptions, &Size) ||
                                    REG_DWORD != Type || sizeof(g_DebugBreakOptions) != Size)
            {
                g_DebugBreakOptions = 0;
            }
        }
#endif
        // parse the command line and establish machine name and etc
        CDMCommandLine CmdLine;
        CmdLine.ParseCommandLine(GetCommandLine());
        g_strStartupMachineName = CmdLine.GetMachineName();
        g_strStartupDeviceId = CmdLine.GetDeviceId();
        g_strStartupCommand = CmdLine.GetCommand();
    }

    catch (CMemoryException* e)
    {
        e->ReportError();
        e->Delete();
        return FALSE;
    }

    return TRUE;
}

//
// Overloaded allocation operators
//
void * __cdecl operator new(
    size_t size)
{
    return ((void *)LocalAlloc(LPTR, size));
}

void __cdecl operator delete(
    void *ptr)
{
    LocalFree(ptr);
}

__cdecl _purecall(void)
{
    return (0);
}


//
// Standard APIs for a OLE server. They are all routed to CClassFactory
// support functions
//
//

STDAPI
DllRegisterServer()
{
    return CClassFactory::RegisterAll();
}

STDAPI
DllUnregisterServer()
{
    return CClassFactory::UnregisterAll();
}


STDAPI
DllCanUnloadNow()
{
    return CClassFactory::CanUnloadNow();
}


STDAPI
DllGetClassObject(
    REFCLSID rclsid,
    REFIID   riid,
    void**   ppv
    )
{
    return CClassFactory::GetClassObject(rclsid, riid, ppv);
}
