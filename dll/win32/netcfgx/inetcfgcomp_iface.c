#include "precomp.h"
#include <devguid.h>

typedef struct
{
    const INetCfgComponent * lpVtbl;
    LONG  ref;
    NetCfgComponentItem * pItem;
    INetCfgComponentPropertyUi * pProperty;
    INetCfg * pNCfg;
}INetCfgComponentImpl;

typedef struct
{
    const IEnumNetCfgComponent * lpVtbl;
    LONG  ref;
    NetCfgComponentItem * pCurrent;
    NetCfgComponentItem * pHead;
    INetCfg * pNCfg;
}IEnumNetCfgComponentImpl;

HRESULT
WINAPI
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
WINAPI
INetCfgComponent_fnAddRef(
    INetCfgComponent * iface)
{
    INetCfgComponentImpl * This = (INetCfgComponentImpl*)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    return refCount;
}

ULONG
WINAPI
INetCfgComponent_fnRelease(
    INetCfgComponent * iface)
{
    INetCfgComponentImpl * This = (INetCfgComponentImpl*)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    if (!refCount)
    {
       CoTaskMemFree(This);
    }
    return refCount;
}

HRESULT
WINAPI
INetCfgComponent_fnGetDisplayName(
    INetCfgComponent * iface,
    LPWSTR * ppszwDisplayName)
{
    LPWSTR szName;
    UINT Length;
    INetCfgComponentImpl * This = (INetCfgComponentImpl*)iface;

    if (This == NULL || ppszwDisplayName == NULL)
        return E_POINTER;

    if (This->pItem->szDisplayName)
        Length = wcslen(This->pItem->szDisplayName)+1;
    else
        Length = 1;

    szName = CoTaskMemAlloc(Length * sizeof(WCHAR));
    if (!szName)
        return E_OUTOFMEMORY;

    if (Length > 1)
        wcscpy(szName, This->pItem->szDisplayName);
    else
        szName[0] = L'\0';

    *ppszwDisplayName = szName;

    return S_OK;
}

HRESULT
WINAPI
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
WINAPI
INetCfgComponent_fnGetHelpText(
    INetCfgComponent * iface,
    LPWSTR * ppszwHelpText)
{
    LPWSTR szHelp;
    UINT Length;
    INetCfgComponentImpl * This = (INetCfgComponentImpl*)iface;

    if (This == NULL || ppszwHelpText == NULL)
        return E_POINTER;

    if (This->pItem->szHelpText)
        Length = wcslen(This->pItem->szHelpText)+1;
    else
        Length = 1;

    szHelp = CoTaskMemAlloc(Length * sizeof(WCHAR));
    if (!szHelp)
        return E_OUTOFMEMORY;

    if (Length > 1)
        wcscpy(szHelp, This->pItem->szHelpText);
    else
        szHelp[0] = L'\0';

    *ppszwHelpText = szHelp;

    return S_OK;
}

HRESULT
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
CreateNotificationObject(
    INetCfgComponentImpl * This,
    INetCfgComponent * iface,
    IUnknown  *pUnk)
{
    WCHAR szName[150];
    HKEY hKey;
    DWORD dwSize, dwType;
    GUID CLSID_NotifyObject;
    LPOLESTR pStr;
    INetCfgComponentPropertyUi * pNCCPU;
    INetCfgComponentControl * pNCCC;
    HRESULT hr;
    LONG lRet;
    CLSID ClassGUID;
    //CLSID InstanceGUID;

    wcscpy(szName,L"SYSTEM\\CurrentControlSet\\Control\\Network\\");

    /* get the Class GUID */
    hr = INetCfgComponent_GetClassGuid(iface, &ClassGUID);
    if (FAILED(hr))
        return hr;

    hr = StringFromCLSID(&ClassGUID, &pStr);
    if (FAILED(hr))
        return hr;

    wcscat(szName, pStr);
    CoTaskMemFree(pStr);
    wcscat(szName, L"\\");

    /* get the Instance GUID */
#if 0
    hr = INetCfgComponent_GetInstanceGuid(iface, &InstanceGUID);
    if (FAILED(hr))
        return hr;

    hr = StringFromCLSID(&InstanceGUID, &pStr);
    if (FAILED(hr))
        return hr;

    wcscat(szName, pStr);
    CoTaskMemFree(pStr);
#else
    // HACK HACK HACK HACK HACK
    wcscat(szName, L"{RandomProtocolGUID_TCPIP}");
#endif

    wcscat(szName, L"\\NDI");

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szName, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return E_FAIL;

    dwSize = sizeof(szName);
    lRet = RegQueryValueExW(hKey, L"ClsID", NULL, &dwType, (LPBYTE)szName, &dwSize);
    RegCloseKey(hKey);

    if (lRet != ERROR_SUCCESS && dwType != REG_SZ)
        return E_FAIL;

    hr = CLSIDFromString(szName, &CLSID_NotifyObject);
    if (FAILED(hr))
        return E_FAIL;

    hr = CoCreateInstance(&CLSID_NotifyObject, NULL, CLSCTX_INPROC_SERVER, &IID_INetCfgComponentPropertyUi, (LPVOID*)&pNCCPU);
    if (FAILED(hr))
        return E_FAIL;

    hr = INetCfgComponentPropertyUi_QueryInterface(pNCCPU, &IID_INetCfgComponentControl, (LPVOID*)&pNCCC);
    if (FAILED(hr))
    {
        INetCfgComponentPropertyUi_Release(pNCCPU);
        return hr;
    }

    hr = INetCfgComponentPropertyUi_QueryPropertyUi(pNCCPU, pUnk);
    if (FAILED(hr))
    {
        INetCfgComponentPropertyUi_Release(pNCCPU);
        return hr;
    }

    hr = INetCfgComponentControl_Initialize(pNCCC, iface, This->pNCfg, FALSE);
    if (FAILED(hr))
    {
        INetCfgComponentControl_Release(pNCCC);
        INetCfgComponentPropertyUi_Release(pNCCPU);
        return hr;
    }

    hr = INetCfgComponentPropertyUi_SetContext(pNCCPU, pUnk);
    if (FAILED(hr))
    {
        INetCfgComponentPropertyUi_Release(pNCCPU);
        return hr;
    }
    This->pProperty = pNCCPU;
    This->pItem->pNCCC = pNCCC;

    return S_OK;
}

HRESULT
WINAPI
INetCfgComponent_fnRaisePropertyUi(
    INetCfgComponent * iface,
    IN HWND  hwndParent,
    IN DWORD  dwFlags,
    IN IUnknown  *pUnk)
{
    HRESULT hr;
    DWORD dwDefPages;
    UINT Pages;
    PROPSHEETHEADERW pinfo;
    HPROPSHEETPAGE * hppages;
    INT_PTR iResult;
    INetCfgComponentImpl * This = (INetCfgComponentImpl*)iface;

    if (!This->pProperty)
    {
         hr = CreateNotificationObject(This,iface, pUnk);
         if (FAILED(hr))
             return hr;
    }

    if (dwFlags == NCRP_QUERY_PROPERTY_UI)
        return S_OK;

    dwDefPages = 0;
    Pages = 0;

    hr = INetCfgComponentPropertyUi_MergePropPages(This->pProperty, &dwDefPages, (BYTE**)&hppages, &Pages, hwndParent, NULL);
    if (FAILED(hr) || !Pages)
    {
        return hr;
    }
    ZeroMemory(&pinfo, sizeof(PROPSHEETHEADERW));
    pinfo.dwSize = sizeof(PROPSHEETHEADERW);
    pinfo.dwFlags = PSH_NOCONTEXTHELP | PSH_PROPTITLE | PSH_NOAPPLYNOW;
    pinfo.u3.phpage = hppages;
    pinfo.hwndParent = hwndParent;
    pinfo.nPages = Pages;
    pinfo.pszCaption = This->pItem->szDisplayName;

    iResult = PropertySheetW(&pinfo);
    CoTaskMemFree(hppages);
    if (iResult > 0)
    {
        /* indicate that settings should be stored */
        This->pItem->bChanged = TRUE;
        return S_OK;
    }
    return S_FALSE;
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
WINAPI
INetCfgComponent_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv, NetCfgComponentItem * pItem, INetCfg * pNCfg)
{
    INetCfgComponentImpl *This;

    if (!ppv)
        return E_POINTER;

    This = (INetCfgComponentImpl *) CoTaskMemAlloc(sizeof (INetCfgComponentImpl));
    if (!This)
        return E_OUTOFMEMORY;

    This->ref = 1;
    This->lpVtbl = (const INetCfgComponent*)&vt_NetCfgComponent;
    This->pProperty = NULL;
    This->pItem = pItem;
    This->pNCfg = pNCfg;

    if (!SUCCEEDED (INetCfgComponent_QueryInterface ((INetCfgComponent*)This, riid, ppv)))
    {
        INetCfgComponent_Release((INetCfg*)This);
        return E_NOINTERFACE;
    }

    INetCfgComponent_Release((INetCfgComponent*)This);
    return S_OK;
}


/***************************************************************
 * IEnumNetCfgComponent
 */

HRESULT
WINAPI
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
WINAPI
IEnumNetCfgComponent_fnAddRef(
    IEnumNetCfgComponent * iface)
{
    IEnumNetCfgComponentImpl * This = (IEnumNetCfgComponentImpl*)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    return refCount;
}

ULONG
WINAPI
IEnumNetCfgComponent_fnRelease(
    IEnumNetCfgComponent * iface)
{
    IEnumNetCfgComponentImpl * This = (IEnumNetCfgComponentImpl*)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    return refCount;
}

HRESULT
WINAPI
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

    hr = INetCfgComponent_Constructor (NULL, &IID_INetCfgComponent, (LPVOID*)rgelt, This->pCurrent, This->pNCfg);
    if (SUCCEEDED(hr))
    {
        This->pCurrent = This->pCurrent->pNext;
        if (pceltFetched)
            *pceltFetched = 1;
    }
    return hr;
}

HRESULT
WINAPI
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
WINAPI
IEnumNetCfgComponent_fnReset(
    IEnumNetCfgComponent * iface)
{
    IEnumNetCfgComponentImpl * This = (IEnumNetCfgComponentImpl*)iface;

    This->pCurrent = This->pHead;
    return S_OK;
}

HRESULT
WINAPI
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
WINAPI
IEnumNetCfgComponent_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv, NetCfgComponentItem * pItem, INetCfg * pNCfg)
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
    This->pNCfg = pNCfg;

    if (!SUCCEEDED (IEnumNetCfgComponent_QueryInterface ((INetCfgComponent*)This, riid, ppv)))
    {
        IEnumNetCfgComponent_Release((INetCfg*)This);
        return E_NOINTERFACE;
    }

    IEnumNetCfgComponent_Release((IEnumNetCfgComponent*)This);
    return S_OK;
}


