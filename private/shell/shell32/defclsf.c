//
// This file contains the implementation of SHCreateDefClassObject
//

#include "shellprv.h"
#pragma  hdrstop

typedef struct
{
    IClassFactory      cf;
    UINT               cRef;            // Reference count
    DWORD              dwFlags;         // Flags to control creation...
    LPFNCREATEINSTANCE pfnCreateInstance;          // CreateInstance callback entry
    UINT *        pcRefDll;     // Reference count of the DLL
    const IID *   riidInst;             // Optional interface for instance
} CClassFactory;

STDMETHODIMP CClassFactory_QueryInterface(IClassFactory *pcf, REFIID riid, void **ppvObj)
{
    CClassFactory *this = IToClass(CClassFactory, cf, pcf);
    if (IsEqualIID(riid, &IID_IClassFactory) || IsEqualIID(riid, &IID_IUnknown))
    {
        InterlockedIncrement(&this->cRef);
        *ppvObj = (LPVOID) (LPCLASSFACTORY) &this->cf;
        return NOERROR;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CClassFactory_AddRef(IClassFactory *pcf)
{
    CClassFactory *this = IToClass(CClassFactory, cf, pcf);
    return this->cRef;
}

STDMETHODIMP_(ULONG) CClassFactory_Release(IClassFactory *pcf)
{
    CClassFactory *this = IToClass(CClassFactory, cf, pcf);
    if (--this->cRef > 0)
	return this->cRef;

    LocalFree((HLOCAL)this);

    return 0;
}

STDMETHODIMP CClassFactory_CreateInstance(IClassFactory *pcf, LPUNKNOWN pUnkOuter, REFIID riid, void **ppvObject)
{
    CClassFactory *this = IToClass(CClassFactory, cf, pcf);

    *ppvObject = NULL;

    // BUGBUG: the pUnkOuter and riidInst stuff is bogus. the CreateInstance
    // call should handle all of this

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    if (this->riidInst == NULL || IsEqualIID(riid, this->riidInst) || IsEqualIID(riid, &IID_IUnknown))
    {
        return this->pfnCreateInstance(pUnkOuter, riid, ppvObject);
    }

    return E_NOINTERFACE;
}

STDMETHODIMP CClassFactory_LockServer(IClassFactory *pcf, BOOL fLock)
{
    CClassFactory *this = IToClass(CClassFactory, cf, pcf);
    if (this->pcRefDll)
    {
        if (fLock)
	    this->pcRefDll++;
        else
	    this->pcRefDll--;
    }
    return S_OK;
}

const IClassFactoryVtbl c_vtblAppUIClassFactory = {
    CClassFactory_QueryInterface, CClassFactory_AddRef, CClassFactory_Release,
    CClassFactory_CreateInstance,
    CClassFactory_LockServer
};

//
// creates a simple default implementation of IClassFactory
//
// Parameters:
//  riid     -- Specifies the interface to the class object
//  ppv      -- Specifies the pointer to LPVOID where the class object pointer
//               will be returned.
//  pfnCreateInstance   -- Specifies the callback entry for instanciation.
//  pcRefDll -- Specifies the address to the DLL reference count (optional)
//  riidInst -- Specifies the interface to the instance (optional).
//
// Notes:
//   The riidInst will be specified only if the instance of the class
//  support only one interface.
//
// BUGBUG: we would like to get rid of this
// this API called by MMSYS.CPL, RNAUI.DLL, SYNCUI.DLL

STDAPI SHCreateDefClassObject(REFIID riid, void **ppv, LPFNCREATEINSTANCE pfnCreateInstance, UINT *pcRefDll, REFIID riidInst)
{
    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IClassFactory))
    {
        CClassFactory *pacf = (CClassFactory *)LocalAlloc(LPTR, SIZEOF(CClassFactory));
        if (pacf)
        {
            pacf->cf.lpVtbl = &c_vtblAppUIClassFactory;
            pacf->cRef++;  // pacf->cRef=0; (generates smaller code)
            pacf->pcRefDll = pcRefDll;
            pacf->pfnCreateInstance = pfnCreateInstance;
            pacf->riidInst = riidInst;

            (IClassFactory *)*ppv = &pacf->cf;
            return NOERROR;
        }
        return E_OUTOFMEMORY;
    }
    return E_NOINTERFACE;
}
