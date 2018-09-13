#define CONST_VTABLE

#include <windows.h>
#include "defclsf.h"

#pragma intrinsic( memcmp ) // for debug, to avoid all CRT


UINT g_cRefDll = 0;

typedef struct
{
    IClassFactory cf;		
    UINT cRef;
    HRESULT (*pfnCreate)(IUnknown *, REFIID, void **);
    const IID *riid;
} CClassFactory;

STDMETHODIMP CClassFactory_QueryInterface(IClassFactory *pcf, REFIID riid, void **ppv)
{
    CClassFactory * this = IToClass(CClassFactory, cf, pcf);
    if (IsEqualIID(riid, &IID_IClassFactory) || 
        IsEqualIID(riid, &IID_IUnknown))
    {
	*ppv = &this->cf;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    this->cRef++;
    return S_OK;
}

STDMETHODIMP_(ULONG) CClassFactory_AddRef(IClassFactory *pcf)
{
    CClassFactory * this = IToClass(CClassFactory, cf, pcf);
    return ++this->cRef;
}

STDMETHODIMP_(ULONG) CClassFactory_Release(IClassFactory *pcf)
{
    CClassFactory * this = IToClass(CClassFactory, cf, pcf);
    if (--this->cRef > 0)
	return this->cRef;

    LocalFree((HLOCAL)this);

    // g_cRefDll--;
    return 0;
}

STDMETHODIMP CClassFactory_CreateInstance(IClassFactory *pcf, IUnknown *punkOuter, REFIID riid, void **ppv)
{
    CClassFactory * this = IToClass(CClassFactory, cf, pcf);

    *ppv = NULL;

    if (punkOuter)
	return CLASS_E_NOAGGREGATION;

    if (this->riid == NULL || 
        IsEqualIID(riid, this->riid) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
	return this->pfnCreate(punkOuter, riid, ppv);
    }

    return E_NOINTERFACE;
}

STDMETHODIMP CClassFactory_LockServer(IClassFactory *pcf, BOOL fLock)
{
    // CClassFactory * this = IToClass(CClassFactory, cf, pcf);

    if (fLock)
	g_cRefDll++;
    else
	g_cRefDll--;

    return S_OK;
}

#pragma data_seg(".text")
IClassFactoryVtbl c_ClassFactoryVtbl = {
    CClassFactory_QueryInterface, CClassFactory_AddRef, CClassFactory_Release,
    CClassFactory_CreateInstance,
    CClassFactory_LockServer
};
#pragma data_seg()

//
// creates a simple default implementation of IClassFactory
//
// Parameters:
//  pfnCreate   Specifies the callback entry for instanciation.
//  pcRefDll	Specifies the address to the DLL reference count (optional)
//  riid	Specifies the interface to the instance (optional).
//  ppv		Specifies the pointer to LPVOID where the class object pointer
//              will be returned.
//
// Notes:
//   The riidInst will be specified only if the instance of the class
//  support only one interface.
//
STDAPI CreateClassFactory(HRESULT (*pfnCreate)(IUnknown *, REFIID, void **), REFIID riid, void **ppv)
{
    CClassFactory *pacf = (CClassFactory *)LocalAlloc(LPTR, sizeof(CClassFactory));
    if (pacf)
    {
	pacf->cf.lpVtbl = &c_ClassFactoryVtbl;
	pacf->cRef++;  // pacf->cRef=0; (generates smaller code)
	pacf->pfnCreate = pfnCreate;
	pacf->riid = riid;

	*ppv = &pacf->cf;

	// g_cRefDll++;

	return S_OK;
    }

    *ppv = NULL;
    return E_OUTOFMEMORY;
}


