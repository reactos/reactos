#include "precomp.h"

typedef struct tagINetConnectionItem
{
    struct tagINetConnectionItem * Next;
    DWORD dwAdapterIndex;
    NETCON_PROPERTIES    Props;
} INetConnectionItem, *PINetConnectionItem;

class CNetConnectionManager final :
    public INetConnectionManager,
    public IEnumNetConnection
{
    public:
        CNetConnectionManager();
        BOOL EnumerateINetConnections();

        // IUnknown
        virtual HRESULT WINAPI QueryInterface(REFIID riid, LPVOID *ppvOut);
        virtual ULONG WINAPI AddRef();
        virtual ULONG WINAPI Release();

        // INetConnectionManager
        virtual HRESULT WINAPI EnumConnections(NETCONMGR_ENUM_FLAGS Flags, IEnumNetConnection **ppEnum);

        // IEnumNetConnection
        virtual HRESULT WINAPI Next(ULONG celt, INetConnection **rgelt, ULONG *pceltFetched);
        virtual HRESULT WINAPI Skip(ULONG celt);
        virtual HRESULT WINAPI Reset();
        virtual HRESULT WINAPI Clone(IEnumNetConnection **ppenum);

    private:
        LONG ref;
        PINetConnectionItem pHead;
        PINetConnectionItem pCurrent;
};

class CNetConnection final :
    public INetConnection
{
    public:
        CNetConnection(PINetConnectionItem pItem);

        // IUnknown
        virtual HRESULT WINAPI QueryInterface(REFIID riid, LPVOID *ppvOut);
        virtual ULONG WINAPI AddRef();
        virtual ULONG WINAPI Release();

        // INetConnection
        HRESULT WINAPI Connect();
        HRESULT WINAPI Disconnect();
        HRESULT WINAPI Delete();
        HRESULT WINAPI Duplicate(LPCWSTR pszwDuplicateName, INetConnection **ppCon);
        HRESULT WINAPI GetProperties(NETCON_PROPERTIES **ppProps);
        HRESULT WINAPI GetUiObjectClassId(CLSID *pclsid);
        HRESULT WINAPI Rename(LPCWSTR pszwDuplicateName);

    private:
        LONG ref;
        NETCON_PROPERTIES Props;
        DWORD dwAdapterIndex;
};

VOID NormalizeOperStatus(MIB_IFROW *IfEntry, NETCON_PROPERTIES * Props);

CNetConnectionManager::CNetConnectionManager()
{
    ref = 0;
    pHead = NULL;
    pCurrent = NULL;
}

HRESULT
WINAPI
CNetConnectionManager::QueryInterface(
    REFIID iid,
    LPVOID *ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(iid, IID_IUnknown) ||
        IsEqualIID(iid, IID_INetConnectionManager))
    {
        *ppvObj = (INetConnectionManager*)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG
WINAPI
CNetConnectionManager::AddRef()
{
    ULONG refCount = InterlockedIncrement(&ref);

    return refCount;
}

ULONG
WINAPI
CNetConnectionManager::Release()
{
    ULONG refCount = InterlockedDecrement(&ref);

    if (!refCount)
        delete this;

    return refCount;
}

HRESULT
WINAPI
CNetConnectionManager::EnumConnections(
    NETCONMGR_ENUM_FLAGS Flags,
    IEnumNetConnection **ppEnum)
{
    TRACE("EnumConnections\n");

    if (!ppEnum)
        return E_POINTER;

    if (Flags != NCME_DEFAULT)
        return E_FAIL;

    *ppEnum = (IEnumNetConnection*)this;
    AddRef();
    return S_OK;
}

/***************************************************************
 * INetConnection Interface
 */

CNetConnection::CNetConnection(PINetConnectionItem pItem)
{
    ref = 0;
    dwAdapterIndex = pItem->dwAdapterIndex;
    CopyMemory(&Props, &pItem->Props, sizeof(NETCON_PROPERTIES));

    if (pItem->Props.pszwName)
    {
        Props.pszwName = (LPWSTR)CoTaskMemAlloc((wcslen(pItem->Props.pszwName)+1)*sizeof(WCHAR));
        if (Props.pszwName)
            wcscpy(Props.pszwName, pItem->Props.pszwName);
    }

    if (pItem->Props.pszwDeviceName)
    {
        Props.pszwDeviceName = (LPWSTR)CoTaskMemAlloc((wcslen(pItem->Props.pszwDeviceName)+1)*sizeof(WCHAR));
        if (Props.pszwDeviceName)
            wcscpy(Props.pszwDeviceName, pItem->Props.pszwDeviceName);
    }
}

HRESULT
WINAPI
CNetConnection::QueryInterface(
    REFIID iid,
    LPVOID * ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(iid, IID_IUnknown) ||
        IsEqualIID(iid, IID_INetConnection))
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG
WINAPI
CNetConnection::AddRef()
{
    ULONG refCount = InterlockedIncrement(&ref);

    return refCount;
}

ULONG
WINAPI
CNetConnection::Release()
{
    ULONG refCount = InterlockedDecrement(&ref);

    if (!refCount)
    {
        CoTaskMemFree(Props.pszwName);
        CoTaskMemFree(Props.pszwDeviceName);
        delete this;
    }

    return refCount;
}

HRESULT
WINAPI
CNetConnection::Connect()
{
    return E_NOTIMPL;
}

HRESULT
WINAPI
CNetConnection::Disconnect()
{
    return E_NOTIMPL;
}

HRESULT
WINAPI
CNetConnection::Delete()
{
    return E_NOTIMPL;
}

HRESULT
WINAPI
CNetConnection::Duplicate(
    LPCWSTR pszwDuplicateName,
    INetConnection **ppCon)
{
    return E_NOTIMPL;
}

HRESULT
WINAPI
CNetConnection::GetProperties(NETCON_PROPERTIES **ppProps)
{
    MIB_IFROW IfEntry;
    HKEY hKey;
    LPOLESTR pStr;
    WCHAR szName[140];
    DWORD dwShowIcon, dwType, dwSize;
    NETCON_PROPERTIES * pProperties;
    HRESULT hr;

    if (!ppProps)
        return E_POINTER;

    pProperties = (NETCON_PROPERTIES*)CoTaskMemAlloc(sizeof(NETCON_PROPERTIES));
    if (!pProperties)
        return E_OUTOFMEMORY;

    CopyMemory(pProperties, &Props, sizeof(NETCON_PROPERTIES));
    pProperties->pszwName = NULL;

    if (Props.pszwDeviceName)
    {
        pProperties->pszwDeviceName = (LPWSTR)CoTaskMemAlloc((wcslen(Props.pszwDeviceName)+1)*sizeof(WCHAR));
        if (pProperties->pszwDeviceName)
            wcscpy(pProperties->pszwDeviceName, Props.pszwDeviceName);
    }

    *ppProps = pProperties;

    /* get updated adapter characteristics */
    ZeroMemory(&IfEntry, sizeof(IfEntry));
    IfEntry.dwIndex = dwAdapterIndex;
    if(GetIfEntry(&IfEntry) != NO_ERROR)
        return NOERROR;

    NormalizeOperStatus(&IfEntry, pProperties);


    hr = StringFromCLSID((CLSID)Props.guidId, &pStr);
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
                pProperties->pszwName = (LPWSTR)CoTaskMemAlloc(dwSize * sizeof(WCHAR));
                if (pProperties->pszwName)
                    CopyMemory(pProperties->pszwName, szName, dwSize * sizeof(WCHAR));
            }
            else
            {
                /* use cached name */
                if (Props.pszwName)
                {
                    pProperties->pszwName = (LPWSTR)CoTaskMemAlloc((wcslen(Props.pszwName)+1)*sizeof(WCHAR));
                    if (pProperties->pszwName)
                        wcscpy(pProperties->pszwName, Props.pszwName);
                }
            }
            RegCloseKey(hKey);
        }
        CoTaskMemFree(pStr);
    }

    return S_OK;
}

HRESULT
WINAPI
CNetConnection::GetUiObjectClassId(CLSID *pclsid)
{
    if (Props.MediaType == NCM_LAN)
    {
        CopyMemory(pclsid, &CLSID_LANConnectUI, sizeof(CLSID));
        return S_OK;
    }

    return E_NOTIMPL;
}

HRESULT
WINAPI
CNetConnection::Rename(LPCWSTR pszwDuplicateName)
{
    WCHAR szName[140];
    LPOLESTR pStr;
    DWORD dwSize;
    HKEY hKey;
    HRESULT hr;

    if (pszwDuplicateName == NULL || wcslen(pszwDuplicateName) == 0)
        return S_OK;

    if (Props.pszwName)
    {
        CoTaskMemFree(Props.pszwName);
        Props.pszwName = NULL;
    }

    dwSize = (wcslen(pszwDuplicateName) + 1) * sizeof(WCHAR);
    Props.pszwName = (LPWSTR)CoTaskMemAlloc(dwSize);
    if (Props.pszwName == NULL)
        return E_OUTOFMEMORY;

    wcscpy(Props.pszwName, pszwDuplicateName);

    hr = StringFromCLSID((CLSID)Props.guidId, &pStr);
    if (SUCCEEDED(hr))
    {
        wcscpy(szName, L"SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\");
        wcscat(szName, pStr);
        wcscat(szName, L"\\Connection");

        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szName, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
        {
            RegSetValueExW(hKey, L"Name", NULL, REG_SZ, (LPBYTE)Props.pszwName, dwSize);
            RegCloseKey(hKey);
        }

        CoTaskMemFree(pStr);
    }

    return hr;
}

HRESULT WINAPI IConnection_Constructor(INetConnection **ppv, PINetConnectionItem pItem)
{
    if (!ppv)
        return E_POINTER;

    CNetConnection *pConnection = new CNetConnection(pItem);
    if (!pConnection)
        return E_OUTOFMEMORY;

    pConnection->AddRef();
    *ppv = (INetConnection *)pConnection;

    return S_OK;
}


/***************************************************************
 * IEnumNetConnection Interface
 */

HRESULT
WINAPI
CNetConnectionManager::Next(
    ULONG celt,
    INetConnection **rgelt,
    ULONG *pceltFetched)
{
    HRESULT hr;

    if (!pceltFetched || !rgelt)
        return E_POINTER;

    if (celt != 1)
        return E_FAIL;

    if (!pCurrent)
        return S_FALSE;

    hr = IConnection_Constructor(rgelt, pCurrent);
    pCurrent = pCurrent->Next;

    return hr;
}

HRESULT
WINAPI
CNetConnectionManager::Skip(ULONG celt)
{
    while(pCurrent && celt-- > 0)
        pCurrent = pCurrent->Next;

    if (celt)
       return S_FALSE;
    else
       return S_OK;

}

HRESULT
WINAPI
CNetConnectionManager::Reset()
{
    pCurrent = pHead;
    return S_OK;
}

HRESULT
WINAPI
CNetConnectionManager::Clone(IEnumNetConnection **ppenum)
{
    return E_NOTIMPL;
}

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
        if (!_wcsicmp(szBuffer, szNetCfg))
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

BOOL
CNetConnectionManager::EnumerateINetConnections()
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
        PINetConnectionItem pNew = (PINetConnectionItem)CoTaskMemAlloc(sizeof(INetConnectionItem));
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
                pNew->Props.pszwName = (LPWSTR)CoTaskMemAlloc((wcslen(szAdapterNetCfg)+1) * sizeof(WCHAR));
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

        /* Get the adapter device description */
        dwSize = 0;
        SetupDiGetDeviceRegistryPropertyW(hInfo, &DevInfo, SPDRP_DEVICEDESC, NULL, NULL, 0, &dwSize);
        if (dwSize != 0)
        {
            pNew->Props.pszwDeviceName = (LPWSTR)CoTaskMemAlloc(dwSize);
            if (pNew->Props.pszwDeviceName)
                SetupDiGetDeviceRegistryPropertyW(hInfo, &DevInfo, SPDRP_DEVICEDESC, NULL, (PBYTE)pNew->Props.pszwDeviceName, dwSize, &dwSize);
        }

        if (pCurrent)
            pCurrent->Next = pNew;
        else
            pHead = pNew;

        pCurrent = pNew;
    }while(TRUE);

    CoTaskMemFree(pIfTable);
    CoTaskMemFree(pAdapterInfo);
    SetupDiDestroyDeviceInfoList(hInfo);

    this->pCurrent = pHead;
    return TRUE;
}

HRESULT WINAPI INetConnectionManager_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID * ppv)
{
    TRACE("INetConnectionManager_Constructor\n");

    if (!ppv)
        return E_POINTER;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    CNetConnectionManager *pConnectionMgr = new CNetConnectionManager;
    if (!pConnectionMgr)
        return E_OUTOFMEMORY;

    pConnectionMgr->AddRef();
    HRESULT hr = pConnectionMgr->QueryInterface(riid, ppv);

    if (SUCCEEDED(hr))
        pConnectionMgr->EnumerateINetConnections();

    pConnectionMgr->Release();

    return hr;
}


