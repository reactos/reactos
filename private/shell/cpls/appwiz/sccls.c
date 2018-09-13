#include "priv.h"
#include "sccls.h"

extern const IClassFactoryVtbl c_CFVtbl;        // forward

//
// This array holds information needed for ClassFactory.
//
// BUGBUG: this table should be ordered in most-to-least used order
//
const OBJECTINFO g_ObjectInfo[] =
{
    
    &c_CFVtbl, &CLSID_ShellAppManager,         CShellAppManager_CreateInstance,
        COCREATEONLY,

#ifndef DOWNLEVEL_PLATFORM
    &c_CFVtbl, &CLSID_DarwinAppPublisher,      CDarwinAppPublisher_CreateInstance,
        COCREATEONLY,
#endif //DOWNLEVEL_PLATFORM

    &c_CFVtbl, &CLSID_EnumInstalledApps,       CEnumInstalledApps_CreateInstance,
        COCREATEONLY, 

    NULL, NULL, NULL, NULL, NULL, 0, 0,0,
} ;


// static class factory (no allocs!)

STDMETHODIMP CClassFactory_QueryInterface(IClassFactory *pcf, REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, &IID_IClassFactory) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = (void *)pcf;
        DllAddRef();
        return NOERROR;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
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

STDMETHODIMP CClassFactory_CreateInstance(IClassFactory *pcf, IUnknown *punkOuter, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (punkOuter && !IsEqualIID(riid, &IID_IUnknown))
    {
        // It is technically illegal to aggregate an object and request
        // any interface other than IUnknown. Enforce this.
        //
        return CLASS_E_NOAGGREGATION;
    }
    else
    {
        OBJECTINFO *this = IToClass(OBJECTINFO, cf, pcf);
        IUnknown *punk;
        HRESULT hres;
        
        if (punkOuter) {

            if (!(this->dwClassFactFlags & OIF_ALLOWAGGREGATION))
                return CLASS_E_NOAGGREGATION;
        }

        // if we're aggregated, then we know we're looking for an
        // IUnknown so we should return punk directly. otherwise
        // we need to QI.
        //
        hres = this->pfnCreateInstance(punkOuter, &punk, this);
        if (SUCCEEDED(hres))
        {
            if (punkOuter)
            {
                *ppv = (LPVOID)punk;
            }
            else
            {
                hres = punk->lpVtbl->QueryInterface(punk, riid, ppv);
                punk->lpVtbl->Release(punk);
            }
        }
    
        ASSERT(FAILED(hres) ? *ppv == NULL : TRUE);
        return hres;
    }
}

STDMETHODIMP CClassFactory_LockServer(IClassFactory *pcf, BOOL fLock)
{
    extern LONG g_cRefThisDll;

    if (fLock)
        DllAddRef();
    else
        DllRelease();
    TraceMsg(DM_TRACE, "sccls: LockServer(%s) to %d", fLock ? TEXT("LOCK") : TEXT("UNLOCK"), g_cRefThisDll);
    return S_OK;
}

const IClassFactoryVtbl c_CFVtbl = {
    CClassFactory_QueryInterface, CClassFactory_AddRef, CClassFactory_Release,
    CClassFactory_CreateInstance,
    CClassFactory_LockServer
};


STDAPI GetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    HRESULT hres = CLASS_E_CLASSNOTAVAILABLE;
    
    extern IClassFactory *CInstClassFactory_Create(const CLSID *pInstID);

    if (IsEqualIID(riid, &IID_IClassFactory) || IsEqualIID(riid, &IID_IUnknown))
    {
        const OBJECTINFO *pcls;
        for (pcls = g_ObjectInfo; pcls->pclsid; pcls++)
        {
            if (IsEqualGUID(rclsid, pcls->pclsid))
            {
                *ppv = (void *)&(pcls->cf);
                DllAddRef();        // class factory holds DLL ref count
                return NOERROR;
            }
        }
    }

    *ppv = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;
}


