/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS DVB
 * FILE:            dll/directx/msvidctl/msvidctl.cpp
 * PURPOSE:         ReactOS DVB Initialization
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */

#include "precomp.h"

static INTERFACE_TABLE InterfaceTable[] =
{
    {&CLSID_SystemTuningSpaces, CTuningSpaceContainer_fnConstructor},
    {NULL, NULL}
};

extern "C"
BOOL
WINAPI
DllMain(
    HINSTANCE hInstDLL,
    DWORD fdwReason,
    LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            CoInitialize(NULL);

#ifdef MSDVBNP_TRACE
            OutputDebugStringW(L"MSVIDCTL::DllMain()\n");
#endif

            DisableThreadLibraryCalls(hInstDLL);
            break;
    default:
        break;
    }

    return TRUE;
}


extern "C"
KSDDKAPI
HRESULT
WINAPI
DllUnregisterServer(void)
{
    return S_OK;
}

extern "C"
KSDDKAPI
HRESULT
WINAPI
DllRegisterServer(void)
{
    return S_OK;
}

KSDDKAPI
HRESULT
WINAPI
DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID *ppv)
{
    UINT i;
    HRESULT hres = E_OUTOFMEMORY;
    IClassFactory * pcf = NULL;	

    if (!ppv)
        return E_INVALIDARG;

    *ppv = NULL;

    for (i = 0; InterfaceTable[i].riid; i++) 
    {
        if (IsEqualIID(*InterfaceTable[i].riid, rclsid)) 
        {
            pcf = CClassFactory_fnConstructor(InterfaceTable[i].lpfnCI, NULL, NULL);
            break;
        }
    }

    if (!pcf) 
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    hres = pcf->QueryInterface(riid, ppv);
    pcf->Release();

    return hres;
}

KSDDKAPI
HRESULT
WINAPI
DllCanUnloadNow(void)
{
    return S_OK;
}
