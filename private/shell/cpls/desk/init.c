#include "precomp.h"
#include <shlwapi.h> // for DllInstall prototype
#include "advpub.h"

#define GUID_STR_LEN    40

// global variables
HINSTANCE hInstance;
LONG        g_cRefDll = 0;
extern const CLSID CLSID_CMultiMonConfig;
STDAPI CMultiMonConfig_CreateInstance(IUnknown* pUnkOuter, REFIID riid, OUT LPVOID * ppvOut);

HRESULT CallRegInstall(LPSTR szSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));
    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");
        if (pfnri)
        {
            hr = pfnri(hInstance, szSection, NULL);
        }

        FreeLibrary(hinstAdvPack);
    }

    return hr;
}

STDAPI DllUnregisterServer(void)
{
    // Remove a bunch of stuff from the registry.
    //
    CallRegInstall("UnregDll");

    return NOERROR;
}

STDAPI DllRegisterServer(void)
{
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    if (hinstAdvPack)
    {
        //
        // Add a bunch of stuff to the registry.
        //
        if (FAILED(CallRegInstall("RegDll")))
        {
            goto CleanUp;
        }
        
        FreeLibrary(hinstAdvPack);
    }
    return NOERROR;

CleanUp:        // cleanup stuff if any of our reg stuff fails

    DllUnregisterServer();
    FreeLibrary(hinstAdvPack);
    return E_FAIL;
}

BOOL DllInitialize(IN PVOID hmod, IN ULONG ulReason, IN PCONTEXT pctx OPTIONAL)
{
    UNREFERENCED_PARAMETER(pctx);
    if (ulReason == DLL_PROCESS_ATTACH)
    {
        hInstance = hmod;
        DisableThreadLibraryCalls(hInstance);
#ifdef DEBUG
        CcshellGetDebugFlags();
#endif
    }
        
    return TRUE;
}

BOOL APIENTRY DllMain(IN HANDLE hDll, IN DWORD dwReason, IN LPVOID lpReserved)
{
    ASSERT(0);
    return TRUE;
}

STDAPI_(void) DllAddRef()
{
    InterlockedIncrement(&g_cRefDll);
}

STDAPI_(void) DllRelease()
{
    InterlockedDecrement(&g_cRefDll);
}

typedef struct {
    const IClassFactoryVtbl *cf;
    const CLSID *pclsid;
    HRESULT (STDMETHODCALLTYPE *pfnCreate)(IUnknown *, REFIID, void **);
} OBJ_ENTRY;

extern const IClassFactoryVtbl c_CFVtbl;        // forward

//
// we always do a linear search here so put your most often used things first
//
const OBJ_ENTRY c_clsmap[] = {
    { &c_CFVtbl, &CLSID_CMultiMonConfig,           CMultiMonConfig_CreateInstance },
    // add more entries here
    { NULL, NULL, NULL }
};

// static class factory (no allocs!)

STDMETHODIMP CClassFactory_QueryInterface(IClassFactory *pcf, REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, &IID_IClassFactory) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = (void *)pcf;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    DllAddRef();
    return NOERROR;
}

STDMETHODIMP_(ULONG) CClassFactory_AddRef(IClassFactory *pcf)
{
    DllAddRef();
    return 2;
}

STDMETHODIMP_(ULONG) CClassFactory_Release(IClassFactory *pcf)
{
    DllRelease();
    return 1;
}

STDMETHODIMP CClassFactory_CreateInstance(IClassFactory *pcf, IUnknown *punkOuter, REFIID riid, void **ppvObject)
{
    OBJ_ENTRY *this = IToClass(OBJ_ENTRY, cf, pcf);
    return this->pfnCreate(punkOuter, riid, ppvObject);
}

STDMETHODIMP CClassFactory_LockServer(IClassFactory *pcf, BOOL fLock)
{
    if (fLock)
        DllAddRef();
    else
        DllRelease();
    return S_OK;
}

const IClassFactoryVtbl c_CFVtbl = {
    CClassFactory_QueryInterface, CClassFactory_AddRef, CClassFactory_Release,
    CClassFactory_CreateInstance,
    CClassFactory_LockServer
};

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IClassFactory) || IsEqualIID(riid, &IID_IUnknown))
    {
        const OBJ_ENTRY *pcls;
        for (pcls = c_clsmap; pcls->pclsid; pcls++)
        {
            if (IsEqualIID(rclsid, pcls->pclsid))
            {
                *ppv = (void *)&(pcls->cf);
                DllAddRef();    // Class Factory keeps dll in memory
                return NOERROR;
            }
        }
    }
    // failure
    *ppv = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;;
}

STDAPI DllCanUnloadNow(void)
{
    return g_cRefDll == 0 ? S_OK : S_FALSE;
}
