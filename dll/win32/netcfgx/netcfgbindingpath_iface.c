#include "precomp.h"

typedef struct
{
    const INetCfgBindingPath *lpVtbl;
    LONG ref;
} INetCfgBindingPathImpl;

typedef struct
{
    const IEnumNetCfgBindingPath *lpVtbl;
    LONG ref;
} IEnumNetCfgBindingPathImpl;


/***************************************************************
 * INetCfgBindingPath
 */

HRESULT
WINAPI
INetCfgBindingPath_fnQueryInterface(
    INetCfgBindingPath *iface,
    REFIID iid,
    LPVOID *ppvObj)
{
    INetCfgBindingPathImpl *This = (INetCfgBindingPathImpl*)iface;
    *ppvObj = NULL;

    if (IsEqualIID (iid, &IID_IUnknown) ||
        IsEqualIID (iid, &IID_INetCfgBindingPath))
    {
        *ppvObj = This;
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG
WINAPI
INetCfgBindingPath_fnAddRef(
    INetCfgBindingPath * iface)
{
    INetCfgBindingPathImpl * This = (INetCfgBindingPathImpl*)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    return refCount;
}

ULONG
WINAPI
INetCfgBindingPath_fnRelease(
    INetCfgBindingPath *iface)
{
    INetCfgBindingPathImpl *This = (INetCfgBindingPathImpl*)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    if (!refCount)
    {
       CoTaskMemFree(This);
    }
    return refCount;
}

HRESULT
WINAPI
INetCfgBindingPath_fnIsSamePathAs(
    INetCfgBindingPath *iface,
    INetCfgBindingPath *pPath)
{
    return E_NOTIMPL;
}

HRESULT
WINAPI
INetCfgBindingPath_fnIsSubPathOf(
    INetCfgBindingPath *iface,
    INetCfgBindingPath *pPath)
{
    return E_NOTIMPL;
}

HRESULT
WINAPI
INetCfgBindingPath_fnIsEnabled(
    INetCfgBindingPath *iface)
{
    return E_NOTIMPL;
}

HRESULT
WINAPI
INetCfgBindingPath_fnEnable(
    INetCfgBindingPath *iface,
    BOOL fEnable)
{
    return E_NOTIMPL;
}

HRESULT
WINAPI
INetCfgBindingPath_fnGetPathToken(
    INetCfgBindingPath *iface,
    LPWSTR *ppszwPathToken)
{
    return E_NOTIMPL;
}

HRESULT
WINAPI
INetCfgBindingPath_fnGetOwner(
    INetCfgBindingPath *iface,
    INetCfgComponent **ppComponent)
{
    return E_NOTIMPL;
}

HRESULT
WINAPI
INetCfgBindingPath_fnGetDepth(
    INetCfgBindingPath *iface,
    ULONG *pcInterfaces)
{
    return E_NOTIMPL;
}

HRESULT
WINAPI
INetCfgBindingPath_fnEnumBindingInterfaces(
    INetCfgBindingPath *iface,
    IEnumNetCfgBindingInterface **ppenumInterface)
{
    return IEnumNetCfgBindingInterface_Constructor(NULL, &IID_IEnumNetCfgBindingInterface, (LPVOID *)ppenumInterface);
}

static const INetCfgBindingPathVtbl vt_NetCfgBindingPath =
{
    INetCfgBindingPath_fnQueryInterface,
    INetCfgBindingPath_fnAddRef,
    INetCfgBindingPath_fnRelease,
    INetCfgBindingPath_fnIsSamePathAs,
    INetCfgBindingPath_fnIsSubPathOf,
    INetCfgBindingPath_fnIsEnabled,
    INetCfgBindingPath_fnEnable,
    INetCfgBindingPath_fnGetPathToken,
    INetCfgBindingPath_fnGetOwner,
    INetCfgBindingPath_fnGetDepth,
    INetCfgBindingPath_fnEnumBindingInterfaces,
};

HRESULT
WINAPI
INetCfgBindingPath_Constructor(
    IUnknown *pUnkOuter,
    REFIID riid,
    LPVOID *ppv)
{
    INetCfgBindingPathImpl *This;

    if (!ppv)
        return E_POINTER;

    This = (INetCfgBindingPathImpl *)CoTaskMemAlloc(sizeof(INetCfgBindingPathImpl));
    if (!This)
        return E_OUTOFMEMORY;

    This->ref = 1;
    This->lpVtbl = (const INetCfgBindingPath*)&vt_NetCfgBindingPath;

    if (!SUCCEEDED (INetCfgBindingPath_QueryInterface ((INetCfgBindingPath*)This, riid, ppv)))
    {
        return E_NOINTERFACE;
    }

    INetCfgBindingPath_Release((INetCfgBindingPath*)This);
    return S_OK;
}


/***************************************************************
 * IEnumNetCfgBindingPath
 */

HRESULT
WINAPI
IEnumNetCfgBindingPath_fnQueryInterface(
    IEnumNetCfgBindingPath *iface,
    REFIID iid,
    LPVOID *ppvObj)
{
    IEnumNetCfgBindingPathImpl *This = (IEnumNetCfgBindingPathImpl*)iface;
    *ppvObj = NULL;

    if (IsEqualIID (iid, &IID_IUnknown) ||
        IsEqualIID (iid, &IID_IEnumNetCfgBindingPath))
    {
        *ppvObj = This;
        return S_OK;
    }

    return E_NOINTERFACE;
}


ULONG
WINAPI
IEnumNetCfgBindingPath_fnAddRef(
    IEnumNetCfgBindingPath *iface)
{
    IEnumNetCfgBindingPathImpl *This = (IEnumNetCfgBindingPathImpl*)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    return refCount;
}

ULONG
WINAPI
IEnumNetCfgBindingPath_fnRelease(
    IEnumNetCfgBindingPath *iface)
{
    IEnumNetCfgBindingPathImpl *This = (IEnumNetCfgBindingPathImpl*)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    return refCount;
}

HRESULT
WINAPI
IEnumNetCfgBindingPath_fnNext(
    IEnumNetCfgBindingPath *iface,
    ULONG celt,
    INetCfgBindingPath **rgelt,
    ULONG *pceltFetched)
{
#if 0
    IEnumNetCfgBindingPathImpl *This = (IEnumNetCfgBindingPathImpl*)iface;
    HRESULT hr;

    if (!iface || !rgelt)
        return E_POINTER;

    if (celt != 1)
        return E_INVALIDARG;

    if (!This->pCurrent)
        return S_FALSE;

    hr = INetCfgBindingPath_Constructor (NULL, &IID_INetCfgComponent, (LPVOID*)rgelt);
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
IEnumNetCfgBindingPath_fnSkip(
    IEnumNetCfgBindingPath *iface,
    ULONG celt)
{
#if 0
    IEnumNetCfgBindingPathImpl *This = (IEnumNetCfgBindingPathImpl*)iface;

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
IEnumNetCfgBindingPath_fnReset(
    IEnumNetCfgBindingPath *iface)
{
#if 0
    IEnumNetCfgBindingPathImpl *This = (IEnumNetCfgBindingPathImpl*)iface;

    This->pCurrent = This->pHead;
    return S_OK;
#endif

    return E_NOTIMPL;
}

HRESULT
WINAPI
IEnumNetCfgBindingPath_fnClone(
    IEnumNetCfgBindingPath *iface,
    IEnumNetCfgBindingPath **ppenum)
{
    return E_NOTIMPL;
}

static const IEnumNetCfgBindingPathVtbl vt_EnumNetCfgBindingPath =
{
    IEnumNetCfgBindingPath_fnQueryInterface,
    IEnumNetCfgBindingPath_fnAddRef,
    IEnumNetCfgBindingPath_fnRelease,
    IEnumNetCfgBindingPath_fnNext,
    IEnumNetCfgBindingPath_fnSkip,
    IEnumNetCfgBindingPath_fnReset,
    IEnumNetCfgBindingPath_fnClone
};

HRESULT
WINAPI
IEnumNetCfgBindingPath_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv, DWORD dwFlags)
{
    IEnumNetCfgBindingPathImpl *This;

    if (!ppv)
        return E_POINTER;

    This = (IEnumNetCfgBindingPathImpl *)CoTaskMemAlloc(sizeof(IEnumNetCfgBindingPathImpl));
    if (!This)
        return E_OUTOFMEMORY;

    This->ref = 1;
    This->lpVtbl = (const IEnumNetCfgBindingPath*)&vt_EnumNetCfgBindingPath;
#if 0
    This->pCurrent = pItem;
    This->pHead = pItem;
    This->pNCfg = pNCfg;
#endif

    if (!SUCCEEDED (IEnumNetCfgBindingPath_QueryInterface((INetCfgBindingPath*)This, riid, ppv)))
    {
        IEnumNetCfgBindingPath_Release((INetCfg*)This);
        return E_NOINTERFACE;
    }

    IEnumNetCfgBindingPath_Release((IEnumNetCfgBindingPath*)This);
    return S_OK;
}
