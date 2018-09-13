/*----------------------------------------------------------------------------
/ Title;
/   dll.cpp
/
/ Authors;
/   David De Vorchik (daviddv)
/
/ Notes;
/   Core entry points for the DLL
/----------------------------------------------------------------------------*/
#include "pch.h"
#include "query.h"
#include <advpub.h>         // For REGINSTALL
#pragma hdrstop

#define INITGUID
#include <initguid.h>
#include "cmnquery.h"
#include "dsquery.h"
#include "iids.h"


/*----------------------------------------------------------------------------
/ Globals
/----------------------------------------------------------------------------*/

HINSTANCE g_hInstance = 0;

HRESULT CallRegInstall(LPSTR szSection);


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
STDAPI_(BOOL) DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID pReserved )
{
    if ( dwReason == DLL_PROCESS_ATTACH )
    {
        GLOBAL_HINSTANCE = hInstance;
    }

    return TRUE;
}


/*-----------------------------------------------------------------------------
/ DllCanUnloadNow
/ ---------------
/   Called by the outside world to determine if our DLL can be unloaded. If we
/   have any objects in existance then we must not unload.
/
/ In:
/   -
/ Out:
/   BOOL inidicate unload state.
/----------------------------------------------------------------------------*/
STDAPI DllCanUnloadNow( void )
{
    return GLOBAL_REFCOUNT ? S_FALSE : S_OK;
}


/*-----------------------------------------------------------------------------
/ DllGetClassObject
/ -----------------
/   Given a class ID and an interface ID, return the relevant object.  This used
/   by the outside world to access the objects contained here in.
/
/ In:
/   rCLISD = class ID required
/   riid = interface within that class required
/   ppVoid -> receives the newly created object.
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
STDAPI DllGetClassObject(REFCLSID rCLSID, REFIID riid, LPVOID* ppVoid)
{
    HRESULT hr = E_OUTOFMEMORY;

    *ppVoid = NULL;

    if ( IsEqualIID(rCLSID, CLSID_ExampleQueryForm) )
    {
        CExampleQueryFormClassFactory* pClassFactory = new CExampleQueryFormClassFactory;

        if ( !pClassFactory )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to get CExampleQueryForm's class factory");

        hr = pClassFactory->QueryInterface(riid, ppVoid);

        if ( FAILED(hr) )
            delete pClassFactory;
    }
    else
    {
        ExitGracefully(hr, E_NOINTERFACE, "Object doesn't support the given CLISD");
    }

exit_gracefully:

    return hr;
}


/*-----------------------------------------------------------------------------
/ Dll[Unregister/Register]Server
/ ------------------------------
/   Called to allow us to setup the registry entries that we use, this
/   takes advantage of the ADVPACK APIs and loads our .inf data from
/   our resource block.
/
/ In:
/   -
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
STDAPI DllRegisterServer(VOID)
{
    return CallRegInstall("RegDll");
}

STDAPI DllUnregisterServer(VOID)
{
    return CallRegInstall("UnRegDll");
}    


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
            
            hr = pfnri(GLOBAL_HINSTANCE, szSection, &stReg);
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
