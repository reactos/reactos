#include "precomp.h"
#include <devguid.h>

typedef struct
{
    const INetCfgComponent * lpVtbl;
    LONG  ref;
    NetCfgComponentItem * pItem;
}INetCfgComponentImpl;

typedef struct
{
    const IEnumNetCfgComponent * lpVtbl;
    LONG  ref;
    NetCfgComponentItem * pCurrent;
    NetCfgComponentItem * pHead;
}IEnumNetCfgComponentImpl;



HRESULT
STDCALL
INetCfgComponent_fnQueryInterface(
    INetCfgComponent * iface,
    REFIID iid,
    LPVOID * ppvObj)
{
    INetCfgComponentImpl * This = (INetCfgComponentImpl*)iface;
    *ppvObj = NULL;

    if (IsEqualIID (iid, &IID_IUnknown) ||
        IsEqualIID (iid, &IID_INetCfgComponent))
    {
        *ppvObj = This;
        INetCfg_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}


ULONG
STDCALL
INetCfgComponent_fnAddRef(
    INetCfgComponent * iface)
{
    INetCfgComponentImpl * This = (INetCfgComponentImpl*)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    return refCount;
}

ULONG
STDCALL
INetCfgComponent_fnRelease(
    INetCfgComponent * iface)
{
    INetCfgComponentImpl * This = (INetCfgComponentImpl*)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    return refCount;
}

HRESULT
STDCALL
INetCfgComponent_fnGetDisplayName(
    INetCfgComponent * iface,
    LPWSTR * ppszwDisplayName)
{
    LPWSTR szName;
    INetCfgComponentImpl * This = (INetCfgComponentImpl*)iface;

    if (This == NULL || ppszwDisplayName == NULL)
        return E_POINTER;

    szName = CoTaskMemAlloc((wcslen(This->pItem->szDisplayName)+1) * sizeof(WCHAR));
    if (!szName)
        return E_OUTOFMEMORY;

    wcscpy(szName, This->pItem->szDisplayName);
    *ppszwDisplayName = szName;

    return S_OK;
}

HRESULT
STDCALL
INetCfgComponent_fnSetDisplayName(
    INetCfgComponent * iface,
    LPCWSTR ppszwDisplayName)
{
    LPWSTR szName;
    INetCfgComponentImpl * This = (INetCfgComponentImpl*)iface;

    if (This == NULL || ppszwDisplayName == NULL)
        return E_POINTER;

    /* setting name is only supported for network cards */
    if (!IsEqualGUID(&This->pItem->ClassGUID, &GUID_DEVCLASS_NET))
        return E_NOTIMPL;

    /// FIXME
    /// check for invalid characters
    /// check for write lock

    szName = CoTaskMemAlloc((wcslen(ppszwDisplayName)+1) * sizeof(WCHAR));
    if (!szName)
        return E_OUTOFMEMORY;

    wcscpy(szName, ppszwDisplayName);
    CoTaskMemFree(This->pItem->szDisplayName);
    This->pItem->szDisplayName = szName;
    This->pItem->bChanged = TRUE;

    return S_OK;
}

HRESULT
STDCALL
INetCfgComponent_fnGetHelpText(
    INetCfgComponent * iface,
    LPWSTR * ppszwHelpText)
{
    LPWSTR szHelp;
    INetCfgComponentImpl * This = (INetCfgComponentImpl*)iface;

    if (This == NULL || ppszwHelpText == NULL)
        return E_POINTER;

    szHelp = CoTaskMemAlloc((wcslen(This->pItem->szHelpText)+1) * sizeof(WCHAR));
    if (!szHelp)
        return E_OUTOFMEMORY;

    wcscpy(szHelp, This->pItem->szHelpText);
    *ppszwHelpText = szHelp;

    return S_OK;
}

HRESULT
STDCALL
INetCfgComponent_fnGetId(
    INetCfgComponent * iface,
    LPWSTR * ppszwId)
{
    LPWSTR szId;
    INetCfgComponentImpl * This = (INetCfgComponentImpl*)iface;

    if (This == NULL || ppszwId == NULL)
        return E_POINTER;

    szId = CoTaskMemAlloc((wcslen(This->pItem->szId)+1) * sizeof(WCHAR));
    if (!szId)
        return E_OUTOFMEMORY;

     wcscpy(szId, This->pItem->szId);
    *ppszwId = szId;

    return S_OK;
}

HRESULT
STDCALL
INetCfgComponent_fnGetCharacteristics(
    INetCfgComponent * iface,
    DWORD * pdwCharacteristics)
{
    INetCfgComponentImpl * This = (INetCfgComponentImpl*)iface;

    if (This == NULL || pdwCharacteristics == NULL)
        return E_POINTER;

    *pdwCharacteristics = This->pItem->dwCharacteristics;

    return S_OK;
}

HRESULT
STDCALL
INetCfgComponent_fnGetInstanceGuid(
    INetCfgComponent * iface,
    GUID * pGuid)
{
    INetCfgComponentImpl * This = (INetCfgComponentImpl*)iface;

    if (This == NULL || pGuid == NULL)
        return E_POINTER;

    CopyMemory(pGuid, &This->pItem->InstanceId, sizeof(GUID));
    return S_OK;
}

HRESULT
STDCALL
INetCfgComponent_fnGetPnpDevNodeId(
    INetCfgComponent * iface,
    LPWSTR * ppszwDevNodeId)
{
    LPWSTR szNode;
    INetCfgComponentImpl * This = (INetCfgComponentImpl*)iface;

    if (This == NULL || ppszwDevNodeId == NULL)
        return E_POINTER;

    if (!IsEqualGUID(&GUID_DEVCLASS_NET, &This->pItem->ClassGUID))
        return E_NOTIMPL;

    szNode = CoTaskMemAlloc((wcslen(This->pItem->szNodeId)+1) * sizeof(WCHAR));
    if (!szNode)
        return E_OUTOFMEMORY;

    wcscpy(szNode, This->pItem->szNodeId);
    *ppszwDevNodeId = szNode;
    return S_OK;
}

HRESULT
STDCALL
INetCfgComponent_fnGetClassGuid(
    INetCfgComponent * iface,
    GUID * pGuid)
{
    INetCfgComponentImpl * This = (INetCfgComponentImpl*)iface;

    if (This == NULL || pGuid == NULL)
        return E_POINTER;

    CopyMemory(pGuid, &This->pItem->ClassGUID, sizeof(GUID));
    return S_OK;
}

HRESULT
STDCALL
INetCfgComponent_fnGetBindName(
    INetCfgComponent * iface,
    LPWSTR * ppszwBindName)
{
    LPWSTR szBind;
    INetCfgComponentImpl * This = (INetCfgComponentImpl*)iface;

    if (This == NULL || ppszwBindName == NULL)
        return E_POINTER;

    szBind = CoTaskMemAlloc((wcslen(This->pItem->szBindName)+1) * sizeof(WCHAR));
    if (!szBind)
        return E_OUTOFMEMORY;

     wcscpy(szBind, This->pItem->szBindName);
    *ppszwBindName = szBind;

    return S_OK;
}

HRESULT
STDCALL
INetCfgComponent_fnGetDeviceStatus(
    INetCfgComponent * iface,
    ULONG * pStatus)
{
    INetCfgComponentImpl * This = (INetCfgComponentImpl*)iface;

    if (This == NULL || pStatus == NULL)
        return E_POINTER;

    if (!IsEqualGUID(&GUID_DEVCLASS_NET, &This->pItem->ClassGUID))
        return E_UNEXPECTED;

    *pStatus = This->pItem->Status;

    return S_OK;
}

HRESULT
STDCALL
INetCfgComponent_fnOpenParamKey(
    INetCfgComponent * iface,
    HKEY * phkey)
{
    WCHAR szBuffer[200] = L"SYSTEM\\CurrentControlSet\\Services\\";
    INetCfgComponentImpl * This = (INetCfgComponentImpl*)iface;

    if (This == NULL || phkey == NULL)
        return E_POINTER;

    wcscat(szBuffer, This->pItem->szBindName);
    wcscat(szBuffer, L"\\Parameters");

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szBuffer, 0, KEY_READ | KEY_WRITE, phkey) == ERROR_SUCCESS)
        return S_OK;
    else
        return E_FAIL;
}

HRESULT
STDCALL
INetCfgComponent_fnRaisePropertyUi(
    INetCfgComponent * iface,
    IN HWND  hwndParent,
    IN DWORD  dwFlags,
    IN IUnknown  *pUnk)
{

    return E_NOTIMPL;
}
static const INetCfgComponentVtbl vt_NetCfgComponent =
{
    INetCfgComponent_fnQueryInterface,
    INetCfgComponent_fnAddRef,
    INetCfgComponent_fnRelease,
    INetCfgComponent_fnGetDisplayName,
    INetCfgComponent_fnSetDisplayName,
    INetCfgComponent_fnGetHelpText,
    INetCfgComponent_fnGetId,
    INetCfgComponent_fnGetCharacteristics,
    INetCfgComponent_fnGetInstanceGuid,
    INetCfgComponent_fnGetPnpDevNodeId,
    INetCfgComponent_fnGetClassGuid,
    INetCfgComponent_fnGetBindName,
    INetCfgComponent_fnGetDeviceStatus,
    INetCfgComponent_fnOpenParamKey,
    INetCfgComponent_fnRaisePropertyUi
};

HRESULT
STDCALL
INetCfgComponent_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv, NetCfgComponentItem * pItem)
{
    INetCfgComponentImpl *This;

    if (!ppv)
        return E_POINTER;

    This = (INetCfgComponentImpl *) CoTaskMemAlloc(sizeof (INetCfgComponentImpl));
    if (!This)
        return E_OUTOFMEMORY;

    This->ref = 1;
    This->lpVtbl = (const INetCfgComponent*)&vt_NetCfgComponent;
    This->pItem = pItem;

    if (!SUCCEEDED (INetCfgComponent_QueryInterface ((INetCfgComponent*)This, riid, ppv)))
    {
        INetCfgComponent_Release((INetCfg*)This);
        return E_NOINTERFACE;
    }

    INetCfgComponent_Release((INetCfgComponent*)This);
    return S_OK;


    return S_OK;
}


/***************************************************************
 * IEnumNetCfgComponent
 */

HRESULT
STDCALL
IEnumNetCfgComponent_fnQueryInterface(
    IEnumNetCfgComponent * iface,
    REFIID iid,
    LPVOID * ppvObj)
{
    IEnumNetCfgComponentImpl * This = (IEnumNetCfgComponentImpl*)iface;
    *ppvObj = NULL;

    if (IsEqualIID (iid, &IID_IUnknown) ||
        IsEqualIID (iid, &IID_IEnumNetCfgComponent))
    {
        *ppvObj = This;
        INetCfg_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}


ULONG
STDCALL
IEnumNetCfgComponent_fnAddRef(
    IEnumNetCfgComponent * iface)
{
    IEnumNetCfgComponentImpl * This = (IEnumNetCfgComponentImpl*)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    return refCount;
}

ULONG
STDCALL
IEnumNetCfgComponent_fnRelease(
    IEnumNetCfgComponent * iface)
{
    IEnumNetCfgComponentImpl * This = (IEnumNetCfgComponentImpl*)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    return refCount;
}

HRESULT
STDCALL
IEnumNetCfgComponent_fnNext(
    IEnumNetCfgComponent * iface,
    ULONG celt,
    INetCfgComponent **rgelt,
    ULONG *pceltFetched)
{
    HRESULT hr;
    IEnumNetCfgComponentImpl * This = (IEnumNetCfgComponentImpl*)iface;

    if (!iface || !rgelt)
        return E_POINTER;

    if (celt != 1)
        return E_INVALIDARG;

    if (!This->pCurrent)
        return S_FALSE;

    hr = INetCfgComponent_Constructor (NULL, &IID_INetCfgComponent, (LPVOID*)rgelt, This->pCurrent);
    if (SUCCEEDED(hr))
    {
        This->pCurrent = This->pCurrent->pNext;
        if (pceltFetched)
            *pceltFetched = 1;
    }
    return hr;
}

HRESULT
STDCALL
IEnumNetCfgComponent_fnSkip(
    IEnumNetCfgComponent * iface,
    ULONG celt)
{
    IEnumNetCfgComponentImpl * This = (IEnumNetCfgComponentImpl*)iface;

    if (!This->pCurrent)
        return S_FALSE;

    while(celt-- > 0 && This->pCurrent)
        This->pCurrent = This->pCurrent->pNext;

    if (!celt)
        return S_OK;
    else
        return S_FALSE;
}

HRESULT
STDCALL
IEnumNetCfgComponent_fnReset(
    IEnumNetCfgComponent * iface)
{
    IEnumNetCfgComponentImpl * This = (IEnumNetCfgComponentImpl*)iface;

    This->pCurrent = This->pHead;
    return S_OK;
}

HRESULT
STDCALL
IEnumNetCfgComponent_fnClone(
    IEnumNetCfgComponent * iface,
    IEnumNetCfgComponent **ppenum)
{
    return E_NOTIMPL;
}

static const IEnumNetCfgComponentVtbl vt_EnumNetCfgComponent =
{
    IEnumNetCfgComponent_fnQueryInterface,
    IEnumNetCfgComponent_fnAddRef,
    IEnumNetCfgComponent_fnRelease,
    IEnumNetCfgComponent_fnNext,
    IEnumNetCfgComponent_fnSkip,
    IEnumNetCfgComponent_fnReset,
    IEnumNetCfgComponent_fnClone
};

HRESULT
STDCALL
IEnumNetCfgComponent_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv, NetCfgComponentItem * pItem)
{
    IEnumNetCfgComponentImpl *This;

    if (!ppv)
        return E_POINTER;

    This = (IEnumNetCfgComponentImpl *) CoTaskMemAlloc(sizeof (IEnumNetCfgComponentImpl));
    if (!This)
        return E_OUTOFMEMORY;

    This->ref = 1;
    This->lpVtbl = (const IEnumNetCfgComponent*)&vt_EnumNetCfgComponent;
    This->pCurrent = pItem;
    This->pHead = pItem;

    if (!SUCCEEDED (IEnumNetCfgComponent_QueryInterface ((INetCfgComponent*)This, riid, ppv)))
    {
        IEnumNetCfgComponent_Release((INetCfg*)This);
        return E_NOINTERFACE;
    }

    IEnumNetCfgComponent_Release((IEnumNetCfgComponent*)This);
    return S_OK;


    return S_OK;
}


