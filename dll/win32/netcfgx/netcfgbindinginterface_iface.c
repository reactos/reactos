#include "precomp.h"

typedef struct
{
    const INetCfgBindingInterface *lpVtbl;
    LONG ref;
} INetCfgBindingInterfaceImpl;

typedef struct
{
    const IEnumNetCfgBindingInterface *lpVtbl;
    LONG ref;
} IEnumNetCfgBindingInterfaceImpl;


/***************************************************************
 * INetCfgBindingInterface
 */

HRESULT
WINAPI
INetCfgBindingInterface_fnQueryInterface(
    INetCfgBindingInterface *iface,
    REFIID iid,
    LPVOID *ppvObj)
{
    INetCfgBindingInterfaceImpl *This = (INetCfgBindingInterfaceImpl*)iface;
    *ppvObj = NULL;

    if (IsEqualIID (iid, &IID_IUnknown) ||
        IsEqualIID (iid, &IID_INetCfgBindingInterface))
    {
        *ppvObj = This;
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG
WINAPI
INetCfgBindingInterface_fnAddRef(
    INetCfgBindingInterface *iface)
{
    INetCfgBindingInterfaceImpl *This = (INetCfgBindingInterfaceImpl*)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    return refCount;
}

ULONG
WINAPI
INetCfgBindingInterface_fnRelease(
    INetCfgBindingInterface *iface)
{
    INetCfgBindingInterfaceImpl *This = (INetCfgBindingInterfaceImpl*)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    if (!refCount)
    {
       CoTaskMemFree(This);
    }
    return refCount;
}

HRESULT
WINAPI
INetCfgBindingInterface_fnGetName(
    INetCfgBindingInterface *iface,
    LPWSTR *ppszwInterfaceName)
{
    return E_NOTIMPL;
}

HRESULT
WINAPI
INetCfgBindingInterface_fnGetUpperComponent(
    INetCfgBindingInterface *iface,
    INetCfgComponent **ppnccItem)
{
    return E_NOTIMPL;
}

HRESULT
WINAPI
INetCfgBindingInterface_fnGetLowerComponent(
    INetCfgBindingInterface *iface,
    INetCfgComponent **ppnccItem)
{
    return E_NOTIMPL;
}

static const INetCfgBindingInterfaceVtbl vt_NetCfgBindingInterface =
{
    INetCfgBindingInterface_fnQueryInterface,
    INetCfgBindingInterface_fnAddRef,
    INetCfgBindingInterface_fnRelease,
    INetCfgBindingInterface_fnGetName,
    INetCfgBindingInterface_fnGetUpperComponent,
    INetCfgBindingInterface_fnGetLowerComponent,
};

HRESULT
WINAPI
INetCfgBindingInterface_Constructor(
    IUnknown *pUnkOuter,
    REFIID riid,
    LPVOID *ppv)
{
    INetCfgBindingInterfaceImpl *This;

    if (!ppv)
        return E_POINTER;

    This = (INetCfgBindingInterfaceImpl *)CoTaskMemAlloc(sizeof(INetCfgBindingInterfaceImpl));
    if (!This)
        return E_OUTOFMEMORY;

    This->ref = 1;
    This->lpVtbl = (const INetCfgBindingInterface*)&vt_NetCfgBindingInterface;

    if (!SUCCEEDED(INetCfgBindingInterface_QueryInterface((INetCfgBindingInterface*)This, riid, ppv)))
    {
        return E_NOINTERFACE;
    }

    INetCfgBindingInterface_Release((INetCfgBindingInterface*)This);
    return S_OK;
}


/***************************************************************
 * IEnumNetCfgBindingInterface
 */

HRESULT
WINAPI
IEnumNetCfgBindingInterface_fnQueryInterface(
    IEnumNetCfgBindingInterface *iface,
    REFIID iid,
    LPVOID *ppvObj)
{
    IEnumNetCfgBindingInterfaceImpl *This = (IEnumNetCfgBindingInterfaceImpl*)iface;
    *ppvObj = NULL;

    if (IsEqualIID (iid, &IID_IUnknown) ||
        IsEqualIID (iid, &IID_IEnumNetCfgBindingInterface))
    {
        *ppvObj = This;
        return S_OK;
    }

    return E_NOINTERFACE;
}


ULONG
WINAPI
IEnumNetCfgBindingInterface_fnAddRef(
    IEnumNetCfgBindingInterface *iface)
{
    IEnumNetCfgBindingInterfaceImpl *This = (IEnumNetCfgBindingInterfaceImpl*)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    return refCount;
}

ULONG
WINAPI
IEnumNetCfgBindingInterface_fnRelease(
    IEnumNetCfgBindingInterface *iface)
{
    IEnumNetCfgBindingInterfaceImpl *This = (IEnumNetCfgBindingInterfaceImpl*)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    return refCount;
}

HRESULT
WINAPI
IEnumNetCfgBindingInterface_fnNext(
    IEnumNetCfgBindingInterface *iface,
    ULONG celt,
    INetCfgBindingInterface **rgelt,
    ULONG *pceltFetched)
{
#if 0
    IEnumNetCfgBindingInterfaceImpl *This = (IEnumNetCfgBindingInterfaceImpl*)iface;
    HRESULT hr;

    if (!iface || !rgelt)
        return E_POINTER;

    if (celt != 1)
        return E_INVALIDARG;

    if (!This->pCurrent)
        return S_FALSE;

    hr = INetCfgBindingInterface_Constructor(NULL, &IID_INetCfgBindingInterface, (LPVOID*)rgelt);
    if (SUCCEEDED(hr))
    {
        This->pCurrent = This->pCurrent->pNext;
        if (pceltFetched)
            *pceltFetched = 1;
    }
    return hr;
#endif

    return E_NOTIMPL;
}

HRESULT
WINAPI
IEnumNetCfgBindingInterface_fnSkip(
    IEnumNetCfgBindingInterface *iface,
    ULONG celt)
{
#if 0
    IEnumNetCfgBindingInterfaceImpl *This = (IEnumNetCfgBindingInterfaceImpl*)iface;

    if (!This->pCurrent)
        return S_FALSE;

    while (celt-- > 0 && This->pCurrent)
        This->pCurrent = This->pCurrent->pNext;

    if (!celt)
        return S_OK;
    else
        return S_FALSE;
#endif

    return E_NOTIMPL;
}

HRESULT
WINAPI
IEnumNetCfgBindingInterface_fnReset(
    IEnumNetCfgBindingInterface *iface)
{
#if 0
    IEnumNetCfgBindingInterfaceImpl *This = (IEnumNetCfgBindingInterfaceImpl*)iface;

    This->pCurrent = This->pHead;
    return S_OK;
#endif

    return E_NOTIMPL;
}

HRESULT
WINAPI
IEnumNetCfgBindingInterface_fnClone(
    IEnumNetCfgBindingInterface *iface,
    IEnumNetCfgBindingInterface **ppenum)
{
    return E_NOTIMPL;
}

static const IEnumNetCfgBindingInterfaceVtbl vt_EnumNetCfgBindingInterface =
{
    IEnumNetCfgBindingInterface_fnQueryInterface,
    IEnumNetCfgBindingInterface_fnAddRef,
    IEnumNetCfgBindingInterface_fnRelease,
    IEnumNetCfgBindingInterface_fnNext,
    IEnumNetCfgBindingInterface_fnSkip,
    IEnumNetCfgBindingInterface_fnReset,
    IEnumNetCfgBindingInterface_fnClone
};

HRESULT
WINAPI
IEnumNetCfgBindingInterface_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv)
{
    IEnumNetCfgBindingInterfaceImpl *This;

    if (!ppv)
        return E_POINTER;

    This = (IEnumNetCfgBindingInterfaceImpl *)CoTaskMemAlloc(sizeof(IEnumNetCfgBindingInterfaceImpl));
    if (!This)
        return E_OUTOFMEMORY;

    This->ref = 1;
    This->lpVtbl = (const IEnumNetCfgBindingInterface*)&vt_EnumNetCfgBindingInterface;
#if 0
    This->pCurrent = pItem;
    This->pHead = pItem;
    This->pNCfg = pNCfg;
#endif

    if (!SUCCEEDED (IEnumNetCfgBindingInterface_QueryInterface((INetCfgBindingInterface*)This, riid, ppv)))
    {
        IEnumNetCfgBindingInterface_Release((INetCfg*)This);
        return E_NOINTERFACE;
    }

    IEnumNetCfgBindingInterface_Release((IEnumNetCfgBindingInterface*)This);
    return S_OK;
}
