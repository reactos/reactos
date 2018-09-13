/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dll.cpp

Abstract:

    This module contains the dll entrypoints for the published app
    query form.

Author:

    Dave Hastings (daveh) creation-date 08-Nov-1997

Revision History:


--*/

#include "pubquery.h"

#include <advpub.h>

#define INITGUID
#include <initguid.h>
#include "cmnquery.h"
#include "cmnquryp.h"
#include "dsquery.h"
#include <guid.h>
#include "guidp.h"

HINSTANCE Instance;
LONG g_RefCount;

HRESULT CallRegInstall(LPSTR szSection);

STDAPI_(BOOL)
DllMain(
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID Reserved
    )
/*++

Routine Description:

    This routine is called by the loader at dll load, unload, and thread attach.

Arguments:

    hInstance - Supplies the handle of this instance.
    dwReason - Supplies the reason for the call

Return Value:

    TRUE for success
--*/
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        Instance = hInstance;
    }

    return TRUE;
}

STDAPI
DllCanUnloadNow(
    VOID
    )
/*++

Routine Description:

    This function is called by COM to determine if the dll can
    be unloaded.

Arguments:

    None.

Return Value:

    S_OK if the dll can be unloaded
--*/
{
    return g_RefCount ? S_FALSE : S_OK;
}

STDAPI
DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID *ppv
    )
/*++

Routine Description:

    This function returns the class factory of the published application
    query form.

Arguments:

    rclsid - Supplies the CLSID of the object to return the class factory of.
    riid - Supplies the IID of the interface to return.
    ppv - Returns the specified interface.

Return Value:

    
--*/
{
    HRESULT hr;    

    *ppv = NULL;

    if (IsEqualCLSID(rclsid, CLSID_PublishedApplicationQuery)) {

        CPublishedApplicationQueryFormClassFactory *ClassFactory = new CPublishedApplicationQueryFormClassFactory;

        if (ClassFactory == NULL) {
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to create CPublishedApplicationQueryFormClassFactory");
        }

        hr = ClassFactory->QueryInterface(riid, ppv);

        if (FAILED(hr)) {
            delete ClassFactory;
        }
    } else {
        ExitGracefully(hr, E_NOINTERFACE, "Object doesn't support the given CLSID");
    }

exit_gracefully:

    return hr;
}

STDAPI DllRegisterServer(VOID)
{
    return CallRegInstall("RegDll");
}

STDAPI DllUnregisterServer(VOID)
{
    return CallRegInstall("UnRegDll");
}
// Stolen from DavidDv's example
/*-----------------------------------------------------------------------------
/ CallRegInstall
/ --------------
/   Call ADVPACK for the given section of our resource based INF>
/
/ In:
/   szSection = section name to invoke
/
/ Out:
/   HRESULT:
/----------------------------------------------------------------------------*/
HRESULT CallRegInstall(LPSTR szSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");

#ifdef UNICODE
        if ( pfnri )
        {
            STRENTRY seReg[] =
            {
                // These two NT-specific entries must be at the end
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
            };
            STRTABLE stReg = { ARRAYSIZE(seReg), seReg };

            hr = pfnri(Instance, szSection, &stReg);
        }
#else
        if (pfnri)
        {
            hr = pfnri(GLOBAL_HINSTANCE, szSection, NULL);
        }

#endif
        FreeLibrary(hinstAdvPack);
    }

    return hr;
}