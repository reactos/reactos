#include "pch.h"
#pragma hdrstop

#define INITGUID
#include <initguid.h>
#include "iids.h"


//
// globals 
//

HINSTANCE g_hInstance = NULL;


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
EXTERN_C BOOL DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID pReserved)
{
    switch ( dwReason )
    {
        case DLL_PROCESS_ATTACH:
            ClassCache_Init();
            TraceSetMaskFromCLSID(CLSID_DsPropertyPages);
            g_hInstance = hInstance;
            break;

        case DLL_PROCESS_DETACH:
            ClassCache_Discard();
            break;
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
STDAPI DllCanUnloadNow(VOID)
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
/   ppvObject -> receives the newly created object.
/
/ Out:
/   -
/----------------------------------------------------------------------------*/

CF_TABLE_BEGIN(g_ObjectInfo)

    CF_TABLE_ENTRY( &CLSID_DsPropertyPages, CDsPropertyPages_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_DsDomainTreeBrowser, CDsDomainTreeBrowser_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_DsVerbs, CDsVerbs_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_DsDisplaySpecifier, CDsDisplaySpecifier_CreateInstance, COCREATEONLY),

CF_TABLE_END(g_ObjectInfo)

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        for (LPCOBJECTINFO pcls = g_ObjectInfo; pcls->pclsid; pcls++)
        {
            if (IsEqualGUID(rclsid, *(pcls->pclsid)))
            {
                *ppv = (void*)pcls;
                InterlockedIncrement(&GLOBAL_REFCOUNT);
                return NOERROR;
            }
        }
    }

    *ppv = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;
}


/*-----------------------------------------------------------------------------
/ DllRegisterServer // DllUnregisterServer
/ ----------------------------------------
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
    return CallRegInstall(GLOBAL_HINSTANCE, "RegDll");
}

STDAPI DllUnregisterServer(VOID)
{
    return CallRegInstall(GLOBAL_HINSTANCE, "UnRegDll");
}

STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    return S_OK;
}


/*-----------------------------------------------------------------------------
/ Compile external stub functions for:
/   - multi monitor support
/   - delay loading
/----------------------------------------------------------------------------*/

#define COMPILE_MULTIMON_STUBS
#include "multimon.h"

#if 0 
#define COMPILE_DELAYLOAD_STUBS
#include "shdload.h"
#endif