#include "precomp.h"

typedef struct
{
    const INetCfgClass *lpVtbl;
    const INetCfgClassSetup *lpVtblSetup;
    LONG ref;
    GUID ClassGuid;
    INetCfg *pNetCfg;
} INetCfgClassImpl, *LPINetCfgClassImpl;

static __inline LPINetCfgClassImpl impl_from_INetCfgClassSetup(INetCfgClassSetup *iface)
{
    return (INetCfgClassImpl*)((char *)iface - FIELD_OFFSET(INetCfgClassImpl, lpVtblSetup));
}

/***************************************************************
 * INetCfgClassSetup
 */

HRESULT
WINAPI
INetCfgClassSetup_fnQueryInterface(
    INetCfgClassSetup *iface,
    REFIID iid,
    LPVOID *ppvObj)
{
    INetCfgClassImpl *This = impl_from_INetCfgClassSetup(iface);
    return INetCfgClass_QueryInterface((INetCfgClass*)This, iid, ppvObj);
}


ULONG
WINAPI
INetCfgClassSetup_fnAddRef(
    INetCfgClassSetup *iface)
{
    INetCfgClassImpl *This = impl_from_INetCfgClassSetup(iface);
    return INetCfgClass_AddRef((INetCfgClass*)This);
}

ULONG
WINAPI
INetCfgClassSetup_fnRelease(
    INetCfgClassSetup *iface)
{
    INetCfgClassImpl *This = impl_from_INetCfgClassSetup(iface);
    return INetCfgClass_Release((INetCfgClass*)This);
}

HRESULT
WINAPI
INetCfgClassSetup_fnSelectAndInstall(
    _In_ INetCfgClassSetup *iface,
    _In_ HWND hwndParent,
    _In_opt_ OBO_TOKEN *pOboToken,
    _Out_opt_ INetCfgComponent **ppnccItem)
{
//    INetCfgClassImpl *This = impl_from_INetCfgClassSetup(iface);
    return S_OK;
}

HRESULT
WINAPI
INetCfgClassSetup_fnInstall(
    _In_ INetCfgClassSetup *iface,
    _In_ LPCWSTR pszwComponentId,
    _In_opt_ OBO_TOKEN *pOboToken,
    _In_opt_ DWORD dwSetupFlags,
    _In_opt_ DWORD dwUpgradeFromBuildNo,
    _In_opt_ LPCWSTR pszwAnswerFile,
    _In_opt_ LPCWSTR pszwAnswerSections,
    _Out_opt_ INetCfgComponent **ppComponent)
{
//    INetCfgClassImpl *This = impl_from_INetCfgClassSetup(iface);

    if (ppComponent)
        *ppComponent = NULL;

    InstallNetworkComponent(pszwComponentId);

    return S_OK;
}


HRESULT
WINAPI
INetCfgClassSetup_fnDeInstall(
    _In_ INetCfgClassSetup *iface,
    _In_ INetCfgComponent *pComponent,
    _In_opt_ OBO_TOKEN *pOboToken,
    _Out_opt_ LPWSTR *pmszwRefs)
{
//    INetCfgClassImpl *This = impl_from_INetCfgClassSetup(iface);

    return S_OK;
}

static const INetCfgClassSetupVtbl vt_NetCfgClassSetup =
{
    INetCfgClassSetup_fnQueryInterface,
    INetCfgClassSetup_fnAddRef,
    INetCfgClassSetup_fnRelease,
    INetCfgClassSetup_fnSelectAndInstall,
    INetCfgClassSetup_fnInstall,
    INetCfgClassSetup_fnDeInstall
};

/***************************************************************
 * INetCfgClass
 */

HRESULT
WINAPI
INetCfgClass_fnQueryInterface(
    INetCfgClass *iface,
    REFIID iid,
    LPVOID *ppvObj)
{
    INetCfgClassImpl *This = (INetCfgClassImpl*)iface;
    *ppvObj = NULL;

    if (IsEqualIID (iid, &IID_IUnknown) ||
        IsEqualIID (iid, &IID_INetCfgClass))
    {
        *ppvObj = This;
        INetCfgClass_AddRef(iface);
        return S_OK;
    }
    else if (IsEqualIID (iid, &IID_INetCfgClassSetup))
    {
        *ppvObj = (LPVOID)&This->lpVtblSetup;
        INetCfgClass_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG
WINAPI
INetCfgClass_fnAddRef(
    INetCfgClass *iface)
{
    INetCfgClassImpl *This = (INetCfgClassImpl*)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    return refCount;
}

ULONG
WINAPI
INetCfgClass_fnRelease(
    INetCfgClass *iface)
{
    INetCfgClassImpl *This = (INetCfgClassImpl*)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    if (!refCount)
    {
       CoTaskMemFree(This);
    }
    return refCount;
}

HRESULT
WINAPI
INetCfgClass_fnFindComponent(
    INetCfgClass *iface,
    INetCfgComponent **pComponent)
{
//    HRESULT hr;
//    INetCfgClassImpl *This = (INetCfgClassImpl *)iface;


    /* TODO */

    return S_FALSE;
}

HRESULT
WINAPI
INetCfgClass_fnEnumComponents(
    INetCfgClass *iface,
    IEnumNetCfgComponent **ppenumComponent)
{
//    INetCfgClassImpl *This = (INetCfgClassImpl *)iface;


    return E_NOINTERFACE;
}

static const INetCfgClassVtbl vt_NetCfgClass =
{
    INetCfgClass_fnQueryInterface,
    INetCfgClass_fnAddRef,
    INetCfgClass_fnRelease,
    INetCfgClass_fnFindComponent,
    INetCfgClass_fnEnumComponents,
};

HRESULT
WINAPI
INetCfgClass_Constructor(
    IUnknown *pUnkOuter,
    REFIID riid,
    LPVOID *ppv,
    const GUID *pguidClass,
    INetCfg *pNetCfg)
{
    INetCfgClassImpl *This;

    if (!ppv)
        return E_POINTER;

    This = (INetCfgClassImpl *)CoTaskMemAlloc(sizeof(INetCfgClassImpl));
    if (!This)
        return E_OUTOFMEMORY;

    This->ref = 1;
    This->lpVtbl = (const INetCfgClass*)&vt_NetCfgClass;
    This->lpVtblSetup = (const INetCfgClassSetup*)&vt_NetCfgClassSetup;

    memcpy(&This->ClassGuid, pguidClass, sizeof(GUID));
    This->pNetCfg = pNetCfg;

    if (!SUCCEEDED(INetCfgClass_QueryInterface((INetCfgClass*)This, riid, ppv)))
    {
        INetCfgClass_Release((INetCfgClass*)This);
        return E_NOINTERFACE;
    }

    INetCfgClass_Release((INetCfgClass*)This);
    return S_OK;
}
