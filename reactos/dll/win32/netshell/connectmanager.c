#include <precomp.h>

typedef struct tagINetConnectionItem
{
    struct tagINetConnectionItem * Next;
    DWORD dwAdapterIndex;
    NETCON_PROPERTIES    Props;
}INetConnectionItem, *PINetConnectionItem;

typedef struct
{
    const INetConnectionManagerVtbl * lpVtbl;
    const IEnumNetConnectionVtbl    * lpVtblNetConnection;
    LONG                       ref;
    PINetConnectionItem pHead;
    PINetConnectionItem pCurrent;

} INetConnectionManagerImpl, *LPINetConnectionManagerImpl;

typedef struct
{
    const INetConnectionVtbl * lpVtbl;
    LONG                       ref;
    NETCON_PROPERTIES          Props;
    DWORD dwAdapterIndex;
} INetConnectionImpl, *LPINetConnectionImpl;


static __inline LPINetConnectionManagerImpl impl_from_EnumNetConnection(IEnumNetConnection *iface)
{
    return (LPINetConnectionManagerImpl)((char *)iface - FIELD_OFFSET(INetConnectionManagerImpl, lpVtblNetConnection));
}

VOID NormalizeOperStatus(MIB_IFROW *IfEntry, NETCON_PROPERTIES * Props);

static
HRESULT
WINAPI
INetConnectionManager_fnQueryInterface(
    INetConnectionManager * iface,
    REFIID iid,
    LPVOID * ppvObj)
{
    INetConnectionManagerImpl * This = (INetConnectionManagerImpl*)iface;
    *ppvObj = NULL;

    if (IsEqualIID (iid, &IID_IUnknown) ||
        IsEqualIID (iid, &IID_INetConnectionManager))
    {
        *ppvObj = This;
        INetConnectionManager_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static
ULONG
WINAPI
INetConnectionManager_fnAddRef(
    INetConnectionManager * iface)
{
    INetConnectionManagerImpl * This = (INetConnectionManagerImpl*)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    return refCount;
}

static
ULONG
WINAPI
INetConnectionManager_fnRelease(
    INetConnectionManager * iface)
{
    INetConnectionManagerImpl * This = (INetConnectionManagerImpl*)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    if (!refCount) 
    {
        CoTaskMemFree (This);
    }
    return refCount;
}

static
HRESULT
WINAPI
INetConnectionManager_fnEnumConnections(
    INetConnectionManager * iface,
    NETCONMGR_ENUM_FLAGS Flags,
    IEnumNetConnection **ppEnum)
{
    INetConnectionManagerImpl * This = (INetConnectionManagerImpl*)iface;

    if (!ppEnum)
        return E_POINTER;

    if (Flags != NCME_DEFAULT)
        return E_FAIL;

    *ppEnum = (IEnumNetConnection *)&This->lpVtblNetConnection;
    INetConnectionManager_AddRef(iface);
    return S_OK;
}

static const INetConnectionManagerVtbl vt_NetConnectionManager =
{
    INetConnectionManager_fnQueryInterface,
    INetConnectionManager_fnAddRef,
    INetConnectionManager_fnRelease,
    INetConnectionManager_fnEnumConnections,
};

/***************************************************************
 * INetConnection Interface
 */

static
HRESULT
WINAPI
INetConnection_fnQueryInterface(
    INetConnection * iface,
    REFIID iid,
    LPVOID * ppvObj)
{
    INetConnectionImpl * This = (INetConnectionImpl*)iface;
    *ppvObj = NULL;

    if (IsEqualIID (iid, &IID_IUnknown) ||
        IsEqualIID (iid, &IID_INetConnection))
    {
        *ppvObj = This;
        INetConnection_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static
ULONG
WINAPI
INetConnection_fnAddRef(
    INetConnection * iface)
{
    INetConnectionImpl * This = (INetConnectionImpl*)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    return refCount;
}

static
ULONG
WINAPI
INetConnection_fnRelease(
    INetConnection * iface)
{
    INetConnectionImpl * This = (INetConnectionImpl*)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    if (!refCount) 
    {
        CoTaskMemFree(This->Props.pszwName);
        CoTaskMemFree(This->Props.pszwDeviceName);
        CoTaskMemFree(This);
    }
    return refCount;
}

static
HRESULT
WINAPI
INetConnection_fnConnect(
    INetConnection * iface)
{
    return E_NOTIMPL;
}

static
HRESULT
WINAPI
INetConnection_fnDisconnect(
    INetConnection * iface)
{
    return E_NOTIMPL;
}


static
HRESULT
WINAPI
INetConnection_fnDelete(
    INetConnection * iface)
{
    return E_NOTIMPL;
}

static
HRESULT
WINAPI
INetConnection_fnDuplicate(
    INetConnection * iface,
    LPCWSTR pszwDuplicateName,
    INetConnection **ppCon)
{
    return E_NOTIMPL;
}

static
HRESULT
WINAPI
INetConnection_fnGetProperties(
    INetConnection * iface,
    NETCON_PROPERTIES **ppProps)
{
    MIB_IFROW IfEntry;
    HKEY hKey;
    LPOLESTR pStr;
    WCHAR szName[140];
    DWORD dwShowIcon, dwType, dwSize;
    NETCON_PROPERTIES * pProperties;
    HRESULT hr;

    INetConnectionImpl * This = (INetConnectionImpl*)iface;

    if (!ppProps)
        return E_POINTER;

    pProperties = CoTaskMemAlloc(sizeof(NETCON_PROPERTIES));
    if (!pProperties)
        return E_OUTOFMEMORY;


    CopyMemory(pProperties, &This->Props, sizeof(NETCON_PROPERTIES));
    pProperties->pszwName = NULL;

    if (This->Props.pszwDeviceName)
    {
        pProperties->pszwDeviceName = CoTaskMemAlloc((wcslen(This->Props.pszwDeviceName)+1)*sizeof(WCHAR));
        if (pProperties->pszwDeviceName)
            wcscpy(pProperties->pszwDeviceName, This->Props.pszwDeviceName);
    }

    *ppProps = pProperties;

    /* get updated adapter characteristics */
    ZeroMemory(&IfEntry, sizeof(IfEntry));
    IfEntry.dwIndex = This->dwAdapterIndex;
    if(GetIfEntry(&IfEntry) != NO_ERROR)
        return NOERROR;

    NormalizeOperStatus(&IfEntry, pProperties);


    hr = StringFromCLSID(&This->Props.guidId, &pStr);
    if (SUCCEEDED(hr))
    {
        wcscpy(szName, L"SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\");
        wcscat(szName, pStr);
        wcscat(szName, L"\\Connection");

        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szName, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            dwSize = sizeof(dwShowIcon);
            if (RegQueryValueExW(hKey, L"ShowIcon", NULL, &dwType, (LPBYTE)&dwShowIcon, &dwSize) == ERROR_SUCCESS && dwType == REG_DWORD)
            {
                if (dwShowIcon)
                    pProperties->dwCharacter |= NCCF_SHOW_ICON;
                else
                    pProperties->dwCharacter &= ~NCCF_SHOW_ICON;
            }
            dwSize = sizeof(szName);
            if (RegQueryValueExW(hKey, L"Name", NULL, &dwType, (LPBYTE)szName, &dwSize) == ERROR_SUCCESS)
            {
                /* use updated name */
                dwSize = wcslen(szName) + 1;
                pProperties->pszwName = CoTaskMemAlloc(dwSize * sizeof(WCHAR));
                if (pProperties->pszwName)
                    CopyMemory(pProperties->pszwName, szName, dwSize * sizeof(WCHAR));
            }
            else
            {
                /* use cached name */
                if (This->Props.pszwName)
                {
                    pProperties->pszwName = CoTaskMemAlloc((wcslen(This->Props.pszwName)+1)*sizeof(WCHAR));
                    if (pProperties->pszwName)
                        wcscpy(pProperties->pszwName, This->Props.pszwName);
                }
            }
            RegCloseKey(hKey);
        }
        CoTaskMemFree(pStr);
    }


    return NOERROR;
}

static
HRESULT
WINAPI
INetConnection_fnGetUiObjectClassId(
    INetConnection * iface,
    CLSID *pclsid)
{

    INetConnectionImpl * This = (INetConnectionImpl*)iface;

    if (This->Props.MediaType == NCM_LAN)
    {
        CopyMemory(pclsid, &CLSID_LANConnectUI, sizeof(CLSID));
        return S_OK;
    }

    return E_NOTIMPL;
}

static
HRESULT
WINAPI
INetConnection_fnRename(
    INetConnection * iface,
    LPCWSTR pszwDuplicateName)
{
    return E_NOTIMPL;
}


static const INetConnectionVtbl vt_NetConnection =
{
    INetConnection_fnQueryInterface,
    INetConnection_fnAddRef,
    INetConnection_fnRelease,
    INetConnection_fnConnect,
    INetConnection_fnDisconnect,
    INetConnection_fnDelete,
    INetConnection_fnDuplicate,
    INetConnection_fnGetProperties,
    INetConnection_fnGetUiObjectClassId,
    INetConnection_fnRename
};

HRESULT WINAPI IConnection_Constructor (INetConnection **ppv, PINetConnectionItem pItem)
{
    INetConnectionImpl *This;

    if (!ppv)
        return E_POINTER;

    This = (INetConnectionImpl *) CoTaskMemAlloc(sizeof (INetConnectionImpl));
    if (!This)
        return E_OUTOFMEMORY;

    This->ref = 1;
    This->lpVtbl = &vt_NetConnection;
    This->dwAdapterIndex = pItem->dwAdapterIndex;
    CopyMemory(&This->Props, &pItem->Props, sizeof(NETCON_PROPERTIES));

    if (pItem->Props.pszwName)
    {
        This->Props.pszwName = CoTaskMemAlloc((wcslen(pItem->Props.pszwName)+1)*sizeof(WCHAR));
        if (This->Props.pszwName)
            wcscpy(This->Props.pszwName, pItem->Props.pszwName);
    }

    if (pItem->Props.pszwDeviceName)
    {
        This->Props.pszwDeviceName = CoTaskMemAlloc((wcslen(pItem->Props.pszwDeviceName)+1)*sizeof(WCHAR));
        if (This->Props.pszwDeviceName)
            wcscpy(This->Props.pszwDeviceName, pItem->Props.pszwDeviceName);
    }

    *ppv = (INetConnection *)This;


    return S_OK;
}


/***************************************************************
 * IEnumNetConnection Interface
 */

static
HRESULT
WINAPI
IEnumNetConnection_fnQueryInterface(
    IEnumNetConnection * iface,
    REFIID iid,
    LPVOID * ppvObj)
{
    INetConnectionManagerImpl * This =  impl_from_EnumNetConnection(iface);
    *ppvObj = NULL;

    if (IsEqualIID (iid, &IID_IUnknown) ||
        IsEqualIID (iid, &IID_INetConnectionManager))
    {
        *ppvObj = This;
        INetConnectionManager_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static
ULONG
WINAPI
IEnumNetConnection_fnAddRef(
    IEnumNetConnection * iface)
{
    INetConnectionManagerImpl * This =  impl_from_EnumNetConnection(iface);
    ULONG refCount = InterlockedIncrement(&This->ref);

    return refCount;
}

static
ULONG
WINAPI
IEnumNetConnection_fnRelease(
    IEnumNetConnection * iface)
{
    INetConnectionManagerImpl * This =  impl_from_EnumNetConnection(iface);
    ULONG refCount = InterlockedDecrement(&This->ref);

    if (!refCount) 
    {
        CoTaskMemFree (This);
    }
    return refCount;
}

static
HRESULT
WINAPI
IEnumNetConnection_fnNext(
    IEnumNetConnection * iface,
    ULONG celt,
    INetConnection **rgelt,
    ULONG *pceltFetched)
{
    INetConnectionManagerImpl * This =  impl_from_EnumNetConnection(iface);
    HRESULT hr;

    if (!pceltFetched || !rgelt)
        return E_POINTER;

    if (celt != 1)
        return E_FAIL;

    if (!This->pCurrent)
        return S_FALSE;

    hr = IConnection_Constructor(rgelt, This->pCurrent);
    This->pCurrent = This->pCurrent->Next;

    return hr;
}

static
HRESULT
WINAPI
IEnumNetConnection_fnSkip(
    IEnumNetConnection * iface,
    ULONG celt)
{
    INetConnectionManagerImpl * This =  impl_from_EnumNetConnection(iface);

    while(This->pCurrent && celt-- > 0)
        This->pCurrent = This->pCurrent->Next;

    if (celt)
       return S_FALSE;
    else
       return NOERROR;

}

static
HRESULT
WINAPI
IEnumNetConnection_fnReset(
    IEnumNetConnection * iface)
{
    INetConnectionManagerImpl * This =  impl_from_EnumNetConnection(iface);

    This->pCurrent = This->pHead;
    return NOERROR;
}

static
HRESULT
WINAPI
IEnumNetConnection_fnClone(
    IEnumNetConnection * iface,
    IEnumNetConnection **ppenum)
{
    return E_NOTIMPL;
}

static const IEnumNetConnectionVtbl vt_EnumNetConnection =
{
    IEnumNetConnection_fnQueryInterface,
    IEnumNetConnection_fnAddRef,
    IEnumNetConnection_fnRelease,
    IEnumNetConnection_fnNext,
    IEnumNetConnection_fnSkip,
    IEnumNetConnection_fnReset,
    IEnumNetConnection_fnClone
};


BOOL
GetAdapterIndexFromNetCfgInstanceId(PIP_ADAPTER_INFO pAdapterInfo, LPWSTR szNetCfg, PDWORD pIndex)
{
    WCHAR szBuffer[50];
    IP_ADAPTER_INFO * pCurrentAdapter;

    pCurrentAdapter = pAdapterInfo;
    while(pCurrentAdapter)
    {
        szBuffer[0] = L'\0';
        if (MultiByteToWideChar(CP_ACP, 0, pCurrentAdapter->AdapterName, -1, szBuffer, sizeof(szBuffer)/sizeof(szBuffer[0])))
        {
            szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
        }
        if (!wcsicmp(szBuffer, szNetCfg))
        {
            *pIndex = pCurrentAdapter->Index;
            return TRUE;
        }
        pCurrentAdapter = pCurrentAdapter->Next;
    }
    return FALSE;
}

VOID
NormalizeOperStatus(
    MIB_IFROW *IfEntry,
    NETCON_PROPERTIES    * Props)
{
    switch(IfEntry->dwOperStatus)
    {
        case MIB_IF_OPER_STATUS_NON_OPERATIONAL:
            Props->Status = NCS_HARDWARE_DISABLED;
            break;
        case MIB_IF_OPER_STATUS_UNREACHABLE:
            Props->Status = NCS_DISCONNECTED;
            break;
        case MIB_IF_OPER_STATUS_DISCONNECTED:
            Props->Status = NCS_MEDIA_DISCONNECTED;
            break;
        case MIB_IF_OPER_STATUS_CONNECTING:
            Props->Status = NCS_CONNECTING;
            break;
        case MIB_IF_OPER_STATUS_CONNECTED:
            Props->Status = NCS_CONNECTED;
            break;
        case MIB_IF_OPER_STATUS_OPERATIONAL:
            Props->Status = NCS_CONNECTED;
            break;
        default:
            break;
    }
}


static
BOOL
EnumerateINetConnections(INetConnectionManagerImpl *This)
{
    DWORD dwSize, dwResult, dwIndex, dwAdapterIndex, dwShowIcon;
    MIB_IFTABLE *pIfTable;
    MIB_IFROW IfEntry;
    IP_ADAPTER_INFO * pAdapterInfo;
    HDEVINFO hInfo;
    SP_DEVINFO_DATA DevInfo;
    HKEY hSubKey;
    WCHAR szNetCfg[50];
    WCHAR szAdapterNetCfg[50];
    WCHAR szDetail[200] = L"SYSTEM\\CurrentControlSet\\Control\\Class\\";
    WCHAR szName[130] = L"SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\";
    PINetConnectionItem pNew;
    PINetConnectionItem pCurrent = NULL;

    /* get the IfTable */
    dwSize = 0;
    if (GetIfTable(NULL, &dwSize, TRUE) != ERROR_INSUFFICIENT_BUFFER)
        return FALSE;

    pIfTable = (PMIB_IFTABLE)CoTaskMemAlloc(dwSize);
    if (!pIfTable)
        return FALSE;

    dwResult = GetIfTable(pIfTable, &dwSize, TRUE);
    if (dwResult != NO_ERROR)
    {
        CoTaskMemFree(pIfTable);
        return FALSE;
    }

    dwSize = 0;
    dwResult = GetAdaptersInfo(NULL, &dwSize); 
    if (dwResult!= ERROR_BUFFER_OVERFLOW)
    {
        CoTaskMemFree(pIfTable);
        return FALSE;
    }

    pAdapterInfo = (PIP_ADAPTER_INFO)CoTaskMemAlloc(dwSize);
    if (!pAdapterInfo)
    {
        CoTaskMemFree(pIfTable);
        return FALSE;
    }

    if (GetAdaptersInfo(pAdapterInfo, &dwSize) != NO_ERROR)
    {
        CoTaskMemFree(pIfTable);
        CoTaskMemFree(pAdapterInfo);
        return FALSE;
    }


    hInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_NET, NULL, NULL, DIGCF_PRESENT );
    if (!hInfo)
    {
        CoTaskMemFree(pIfTable);
        CoTaskMemFree(pAdapterInfo);
        return FALSE;
    }

    dwIndex = 0;
    do
    {

        ZeroMemory(&DevInfo, sizeof(SP_DEVINFO_DATA));
        DevInfo.cbSize = sizeof(DevInfo);

        /* get device info */
        if (!SetupDiEnumDeviceInfo(hInfo, dwIndex++, &DevInfo))
            break;

        /* get device software registry path */
        if (!SetupDiGetDeviceRegistryPropertyW(hInfo, &DevInfo, SPDRP_DRIVER, NULL, (LPBYTE)&szDetail[39], sizeof(szDetail)/sizeof(WCHAR) - 40, &dwSize))
            break;

        /* open device registry key */
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szDetail, 0, KEY_READ, &hSubKey) != ERROR_SUCCESS)
            break;

        /* query NetCfgInstanceId for current device */
        dwSize = sizeof(szNetCfg);
        if (RegQueryValueExW(hSubKey, L"NetCfgInstanceId", NULL, NULL, (LPBYTE)szNetCfg, &dwSize) != ERROR_SUCCESS)
        {
            RegCloseKey(hSubKey);
            break;
        }
        RegCloseKey(hSubKey);

        /* get the current adapter index from NetCfgInstanceId */
        if (!GetAdapterIndexFromNetCfgInstanceId(pAdapterInfo, szNetCfg, &dwAdapterIndex))
            continue;

        /* get detailed adapter info */
        ZeroMemory(&IfEntry, sizeof(IfEntry));
        IfEntry.dwIndex = dwAdapterIndex;
        if(GetIfEntry(&IfEntry) != NO_ERROR)
            break;

        /* allocate new INetConnectionItem */
        pNew = CoTaskMemAlloc(sizeof(INetConnectionItem));
        if (!pNew)
            break;

        ZeroMemory(pNew, sizeof(INetConnectionItem));
        pNew->dwAdapterIndex = dwAdapterIndex;
        /* store NetCfgInstanceId */
        CLSIDFromString(szNetCfg, &pNew->Props.guidId);
        NormalizeOperStatus(&IfEntry, &pNew->Props);

        switch(IfEntry.dwType)
        {
            case IF_TYPE_ETHERNET_CSMACD:
                pNew->Props.MediaType = NCM_LAN;
                break;
            case IF_TYPE_IEEE80211:
                pNew->Props.MediaType = NCM_SHAREDACCESSHOST_RAS;
                break;
            default:
                break;
        }
        /*  open network connections details */
        wcscpy(&szName[80], szNetCfg);
        wcscpy(&szName[118], L"\\Connection");

        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szName, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS)
        {
            /* retrieve name of connection */
            dwSize = sizeof(szAdapterNetCfg);
            if (RegQueryValueExW(hSubKey, L"Name", NULL, NULL, (LPBYTE)szAdapterNetCfg, &dwSize) == ERROR_SUCCESS)
            {
                pNew->Props.pszwName = CoTaskMemAlloc((wcslen(szAdapterNetCfg)+1) * sizeof(WCHAR));
                if (pNew->Props.pszwName)
                    wcscpy(pNew->Props.pszwName, szAdapterNetCfg);
            }
            dwSize = sizeof(dwShowIcon);
            if (RegQueryValueExW(hSubKey, L"ShowIcon", NULL, NULL, (LPBYTE)&dwShowIcon, &dwSize) == ERROR_SUCCESS)
            {
                if (dwShowIcon)
                    pNew->Props.dwCharacter |= NCCF_SHOW_ICON;
            }
            RegCloseKey(hSubKey);
        }
        if (SetupDiGetDeviceRegistryPropertyW(hInfo, &DevInfo, SPDRP_DEVICEDESC, NULL, (PBYTE)szNetCfg, sizeof(szNetCfg)/sizeof(WCHAR), &dwSize))
        {
            szNetCfg[(sizeof(szNetCfg)/sizeof(WCHAR))-1] = L'\0';
            pNew->Props.pszwDeviceName = CoTaskMemAlloc((wcslen(szNetCfg)+1) * sizeof(WCHAR));
            if (pNew->Props.pszwDeviceName)
                wcscpy(pNew->Props.pszwDeviceName, szNetCfg);
        }

        if (pCurrent)
            pCurrent->Next = pNew;
        else
            This->pHead = pNew;

        pCurrent = pNew;
    }while(TRUE);

    CoTaskMemFree(pIfTable);
    CoTaskMemFree(pAdapterInfo);
    SetupDiDestroyDeviceInfoList(hInfo);

    This->pCurrent = This->pHead;
    return TRUE;
}

HRESULT WINAPI INetConnectionManager_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv)
{
    INetConnectionManagerImpl *sf;

    if (!ppv)
        return E_POINTER;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    sf = (INetConnectionManagerImpl *) CoTaskMemAlloc(sizeof (INetConnectionManagerImpl));
    if (!sf)
        return E_OUTOFMEMORY;

    sf->ref = 1;
    sf->lpVtbl = &vt_NetConnectionManager;
    sf->lpVtblNetConnection = &vt_EnumNetConnection;
    sf->pHead = NULL;
    sf->pCurrent = NULL;


    if (!SUCCEEDED (INetConnectionManager_QueryInterface ((INetConnectionManager*)sf, riid, ppv)))
    {
        INetConnectionManager_Release((INetConnectionManager*)sf);
        return E_NOINTERFACE;
    }

    INetConnectionManager_Release((INetConnectionManager*)sf);
    EnumerateINetConnections(sf);
    return S_OK;
}


