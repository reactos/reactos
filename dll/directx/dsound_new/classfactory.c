/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Configuration of network devices
 * FILE:            dll/directx/dsound_new/classfactory.c
 * PURPOSE:         IClassFactory implementation
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */


#include "precomp.h"

typedef struct
{
    const IClassFactoryVtbl    *lpVtbl;
    LONG                       ref;
    CLSID                      *rclsid;
    LPFNCREATEINSTANCE         lpfnCI;
    const IID *                riidInst;
} IClassFactoryImpl;


static
HRESULT
WINAPI
IClassFactory_fnQueryInterface(
    LPCLASSFACTORY iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

    *ppvObj = NULL;

    /* check requested interface */
    if(IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IClassFactory))
    {
        *ppvObj = This;
        InterlockedIncrement(&This->ref);
        return S_OK;
    }
    return E_NOINTERFACE;
}

static
ULONG
WINAPI
IClassFactory_fnAddRef(
    LPCLASSFACTORY iface)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

    /* increment reference count */
    ULONG refCount = InterlockedIncrement(&This->ref);

    return refCount;
}

static
ULONG
WINAPI
IClassFactory_fnRelease(
    LPCLASSFACTORY iface)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    /* decrement reference count */
    if (!refCount)
    {
        /* free class factory */
        CoTaskMemFree(This);
        return 0;
    }
    return refCount;
}

static
HRESULT
WINAPI
IClassFactory_fnCreateInstance(
    LPCLASSFACTORY iface,
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    LPVOID *ppvObject)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

    if (!ppvObject)
        return E_INVALIDARG;

    *ppvObject = NULL;

    if ( This->riidInst==NULL || IsEqualCLSID(riid, This->riidInst) || IsEqualCLSID(riid, &IID_IUnknown) )
    {
        /* instantiate object */
        return This->lpfnCI(pUnkOuter, riid, ppvObject);
    }

    return E_NOINTERFACE;
}

static
HRESULT
WINAPI IClassFactory_fnLockServer(
    LPCLASSFACTORY iface,
    BOOL fLock)
{
    //IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    return E_NOTIMPL;
}


static const IClassFactoryVtbl dclfvt =
{
    IClassFactory_fnQueryInterface,
    IClassFactory_fnAddRef,
    IClassFactory_fnRelease,
    IClassFactory_fnCreateInstance,
    IClassFactory_fnLockServer
};


IClassFactory *
IClassFactory_fnConstructor(
    LPFNCREATEINSTANCE lpfnCI,
    PLONG pcRefDll,
    REFIID riidInst)
{
    IClassFactoryImpl* lpclf;

    lpclf = CoTaskMemAlloc(sizeof(IClassFactoryImpl));
    lpclf->ref = 1;
    lpclf->lpVtbl = &dclfvt;
    lpclf->lpfnCI = lpfnCI;

    if (pcRefDll)
        InterlockedIncrement(pcRefDll);
    lpclf->riidInst = riidInst;

    return (LPCLASSFACTORY)lpclf;
}


