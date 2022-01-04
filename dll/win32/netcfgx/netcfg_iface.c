#include "precomp.h"

typedef struct
{
    const INetCfg * lpVtbl;
    const INetCfgLock * lpVtblLock;
    const INetCfgPnpReconfigCallback *lpVtblPnpReconfigCallback;
    LONG                       ref;
    BOOL bInitialized;
    HANDLE hMutex;
    NetCfgComponentItem *pNet;
    NetCfgComponentItem * pService;
    NetCfgComponentItem * pClient;
    NetCfgComponentItem * pProtocol;
} INetCfgImpl, *LPINetCfgImpl;

static __inline LPINetCfgImpl impl_from_INetCfgLock(INetCfgLock *iface)
{
    return (INetCfgImpl*)((char *)iface - FIELD_OFFSET(INetCfgImpl, lpVtblLock));
}

static __inline LPINetCfgImpl impl_from_INetCfgPnpReconfigCallback(INetCfgPnpReconfigCallback *iface)
{
    return (INetCfgImpl*)((char *)iface - FIELD_OFFSET(INetCfgImpl, lpVtblPnpReconfigCallback));
}


HRESULT
WINAPI
INetCfgLock_fnQueryInterface(
    INetCfgLock * iface,
    REFIID iid,
    LPVOID * ppvObj)
{
    INetCfgImpl * This = impl_from_INetCfgLock(iface);
    return INetCfg_QueryInterface((INetCfg*)This, iid, ppvObj);
}


ULONG
WINAPI
INetCfgLock_fnAddRef(
    INetCfgLock * iface)
{
    INetCfgImpl * This = impl_from_INetCfgLock(iface);

    return INetCfg_AddRef((INetCfg*)This);
}

ULONG
WINAPI
INetCfgLock_fnRelease(
    INetCfgLock * iface)
{
    INetCfgImpl * This = impl_from_INetCfgLock(iface);
    return INetCfg_Release((INetCfg*)This);
}

HRESULT
WINAPI
INetCfgLock_fnAcquireWriteLock(
    INetCfgLock * iface,
    DWORD cmsTimeout,
    LPCWSTR pszwClientDescription,
    LPWSTR *ppszwClientDescription)
{
    DWORD dwResult;
    HKEY hKey;
    WCHAR szValue[100];
    INetCfgImpl * This = impl_from_INetCfgLock(iface);

    if (This->bInitialized)
        return NETCFG_E_ALREADY_INITIALIZED;

    dwResult = WaitForSingleObject(This->hMutex, cmsTimeout);
    if (dwResult == WAIT_TIMEOUT)
    {
        if (ppszwClientDescription)
        {
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Network\\NetCfgLockHolder", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                dwResult = sizeof(szValue);
                if (RegQueryValueExW(hKey, NULL, NULL, NULL, (LPBYTE)szValue, &dwResult) == ERROR_SUCCESS)
                {
                     szValue[(sizeof(szValue)/sizeof(WCHAR))-1] = L'\0';
                     *ppszwClientDescription = CoTaskMemAlloc((wcslen(szValue)+1) * sizeof(WCHAR));
                     if (*ppszwClientDescription)
                         wcscpy(*ppszwClientDescription, szValue);
                }
                RegCloseKey(hKey);
            }
        }
        return S_FALSE;
    }
    else if (dwResult == WAIT_OBJECT_0)
    {
        if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Network\\NetCfgLockHolder", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
        {
            RegSetValueExW(hKey, NULL, 0, REG_SZ, (LPBYTE)pszwClientDescription, (wcslen(pszwClientDescription)+1) * sizeof(WCHAR));
            RegCloseKey(hKey);
        }
        return S_OK;
    }

    return E_FAIL;
}

HRESULT
WINAPI
INetCfgLock_fnReleaseWriteLock(
    INetCfgLock * iface)
{
    INetCfgImpl * This = impl_from_INetCfgLock(iface);

    if (This->bInitialized)
        return NETCFG_E_ALREADY_INITIALIZED;


    if (ReleaseMutex(This->hMutex))
        return S_OK;
    else
        return S_FALSE;
}

HRESULT
WINAPI
INetCfgLock_fnIsWriteLocked(
    INetCfgLock * iface,
    LPWSTR *ppszwClientDescription)
{
    HKEY hKey;
    WCHAR szValue[100];
    DWORD dwSize, dwType;
    HRESULT hr;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Network\\NetCfgLockHolder", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return S_FALSE;

    dwSize = sizeof(szValue);
    if (RegQueryValueExW(hKey, NULL, NULL, &dwType, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS && dwType == REG_SZ)
    {
        hr = S_OK;
        szValue[(sizeof(szValue)/sizeof(WCHAR))-1] = L'\0';
        *ppszwClientDescription = CoTaskMemAlloc((wcslen(szValue)+1) * sizeof(WCHAR));
        if (*ppszwClientDescription)
            wcscpy(*ppszwClientDescription, szValue);
        else
            hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = E_FAIL;
    }
    RegCloseKey(hKey);
    return hr;
}

static const INetCfgLockVtbl vt_NetCfgLock =
{
    INetCfgLock_fnQueryInterface,
    INetCfgLock_fnAddRef,
    INetCfgLock_fnRelease,
    INetCfgLock_fnAcquireWriteLock,
    INetCfgLock_fnReleaseWriteLock,
    INetCfgLock_fnIsWriteLocked
};

/***************************************************************
 * INetCfgPnpReconfigCallback
 */

HRESULT
WINAPI
INetCfgPnpReconfigCallback_fnQueryInterface(
    INetCfgPnpReconfigCallback * iface,
    REFIID iid,
    LPVOID * ppvObj)
{
    INetCfgImpl * This = impl_from_INetCfgPnpReconfigCallback(iface);
    return INetCfg_QueryInterface((INetCfg*)This, iid, ppvObj);
}

ULONG
WINAPI
INetCfgPnpReconfigCallback_fnAddRef(
    INetCfgPnpReconfigCallback * iface)
{
    INetCfgImpl * This = impl_from_INetCfgPnpReconfigCallback(iface);

    return INetCfg_AddRef((INetCfg*)This);
}

ULONG
WINAPI
INetCfgPnpReconfigCallback_fnRelease(
    INetCfgPnpReconfigCallback * iface)
{
    INetCfgImpl * This = impl_from_INetCfgPnpReconfigCallback(iface);
    return INetCfg_Release((INetCfg*)This);
}

HRESULT
WINAPI
INetCfgPnpReconfigCallback_fnSendPnpReconfig(
    INetCfgPnpReconfigCallback * iface,
    NCPNP_RECONFIG_LAYER Layer,
    LPCWSTR pszwUpper,
    LPCWSTR pszwLower,
    PVOID pvData,
    DWORD dwSizeOfData)
{
    /* FIXME */
    return E_NOTIMPL;
}

static const INetCfgPnpReconfigCallbackVtbl vt_NetCfgPnpReconfigCallback =
{
    INetCfgPnpReconfigCallback_fnQueryInterface,
    INetCfgPnpReconfigCallback_fnAddRef,
    INetCfgPnpReconfigCallback_fnRelease,
    INetCfgPnpReconfigCallback_fnSendPnpReconfig
};


/***************************************************************
 * INetCfg
 */

HRESULT
ReadBindingString(
    NetCfgComponentItem *Item)
{
    WCHAR szBuffer[200];
    HKEY hKey;
    DWORD dwType, dwSize;

    if (Item == NULL || Item->szBindName == NULL)
        return S_OK;

    wcscpy(szBuffer, L"SYSTEM\\CurrentControlSet\\Services\\");
    wcscat(szBuffer, Item->szBindName);
    wcscat(szBuffer, L"\\Linkage");

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szBuffer, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = 0;
        RegQueryValueExW(hKey, L"Bind", NULL, &dwType, NULL, &dwSize);

        if (dwSize != 0)
        {
            Item->pszBinding = CoTaskMemAlloc(dwSize);
            if (Item->pszBinding == NULL)
                return E_OUTOFMEMORY;

            RegQueryValueExW(hKey, L"Bind", NULL, &dwType, (LPBYTE)Item->pszBinding, &dwSize);
        }

        RegCloseKey(hKey);
    }

    return S_OK;
}

HRESULT
EnumClientServiceProtocol(HKEY hKey, const GUID * pGuid, NetCfgComponentItem ** pHead)
{
    DWORD dwIndex = 0;
    DWORD dwSize;
    DWORD dwType;
    WCHAR szName[100];
    WCHAR szText[100];
    HKEY hSubKey, hNDIKey;
    NetCfgComponentItem * pLast = NULL, *pCurrent;

    *pHead = NULL;
    do
    {
        szText[0] = L'\0';

        dwSize = sizeof(szName)/sizeof(WCHAR);
        if (RegEnumKeyExW(hKey, dwIndex++, szName, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
        {
            pCurrent = CoTaskMemAlloc(sizeof(NetCfgComponentItem));
            if (!pCurrent)
                return E_OUTOFMEMORY;

            ZeroMemory(pCurrent, sizeof(NetCfgComponentItem));
            CopyMemory(&pCurrent->ClassGUID, pGuid, sizeof(GUID));

            if (FAILED(CLSIDFromString(szName, &pCurrent->InstanceId)))
            {
                /// ReactOS tcpip guid is not yet generated
                //CoTaskMemFree(pCurrent);
                //return E_FAIL;
            }
            if (RegOpenKeyExW(hKey, szName, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS)
            {
                /* retrieve Characteristics */
                dwSize = sizeof(DWORD);

                RegQueryValueExW(hSubKey, L"Characteristics", NULL, &dwType, (LPBYTE)&pCurrent->dwCharacteristics, &dwSize);
                if (dwType != REG_DWORD)
                    pCurrent->dwCharacteristics = 0;

                /* retrieve ComponentId */
                dwSize = sizeof(szText);
                if (RegQueryValueExW(hSubKey, L"ComponentId", NULL, &dwType, (LPBYTE)szText, &dwSize) == ERROR_SUCCESS)
                {
                    if (dwType == REG_SZ)
                    {
                        szText[(sizeof(szText)/sizeof(WCHAR))-1] = L'\0';
                        pCurrent->szId = CoTaskMemAlloc((wcslen(szText)+1)* sizeof(WCHAR));
                        if (pCurrent->szId)
                            wcscpy(pCurrent->szId, szText);
                    }
                }

                /* retrieve Description */
                dwSize = sizeof(szText);
                if (RegQueryValueExW(hSubKey, L"Description", NULL, &dwType, (LPBYTE)szText, &dwSize) == ERROR_SUCCESS)
                {
                    if (dwType == REG_SZ)
                    {
                        szText[(sizeof(szText)/sizeof(WCHAR))-1] = L'\0';
                        pCurrent->szDisplayName = CoTaskMemAlloc((wcslen(szText)+1)* sizeof(WCHAR));
                        if (pCurrent->szDisplayName)
                            wcscpy(pCurrent->szDisplayName, szText);
                    }
                }

                if (RegOpenKeyExW(hSubKey, L"NDI", 0, KEY_READ, &hNDIKey) == ERROR_SUCCESS)
                {
                    /* retrieve HelpText */
                    dwSize = sizeof(szText);
                    if (RegQueryValueExW(hNDIKey, L"HelpText", NULL, &dwType, (LPBYTE)szText, &dwSize) == ERROR_SUCCESS)
                    {
                        if (dwType == REG_SZ)
                        {
                            szText[(sizeof(szText)/sizeof(WCHAR))-1] = L'\0';
                            pCurrent->szHelpText = CoTaskMemAlloc((wcslen(szText)+1)* sizeof(WCHAR));
                            if (pCurrent->szHelpText)
                                wcscpy(pCurrent->szHelpText, szText);
                        }
                    }

                    /* retrieve Service */
                    dwSize = sizeof(szText);
                    if (RegQueryValueExW(hNDIKey, L"Service", NULL, &dwType, (LPBYTE)szText, &dwSize) == ERROR_SUCCESS)
                    {
                        if (dwType == REG_SZ)
                        {
                            szText[(sizeof(szText)/sizeof(WCHAR))-1] = L'\0';
                            pCurrent->szBindName = CoTaskMemAlloc((wcslen(szText)+1)* sizeof(WCHAR));
                            if (pCurrent->szBindName)
                                wcscpy(pCurrent->szBindName, szText);
                        }
                    }
                    RegCloseKey(hNDIKey);
                }
                RegCloseKey(hSubKey);

                ReadBindingString(pCurrent);

                if (!pLast)
                    *pHead = pCurrent;
                else
                    pLast->pNext = pCurrent;

                pLast = pCurrent;
            }
        }
        else
           break;

    }while(TRUE);
    return S_OK;
}



HRESULT
EnumerateNetworkComponent(
    const GUID *pGuid, NetCfgComponentItem ** pHead)
{
    HKEY hKey;
    LPOLESTR pszGuid;
    HRESULT hr;
    WCHAR szName[150];

    hr = StringFromCLSID(pGuid, &pszGuid);
    if (SUCCEEDED(hr))
    {
        swprintf(szName, L"SYSTEM\\CurrentControlSet\\Control\\Network\\%s", pszGuid);
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szName, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            hr = EnumClientServiceProtocol(hKey, pGuid, pHead);
            RegCloseKey(hKey);
        }
        CoTaskMemFree(pszGuid);
    }
    return hr;
}

HRESULT
EnumerateNetworkAdapter(NetCfgComponentItem ** pHead)
{
    DWORD dwSize, dwIndex;
    HDEVINFO hInfo;
    SP_DEVINFO_DATA DevInfo;
    HKEY hKey;
    WCHAR szNetCfg[50];
    WCHAR szAdapterNetCfg[MAX_DEVICE_ID_LEN];
    WCHAR szDetail[200] = L"SYSTEM\\CurrentControlSet\\Control\\Class\\";
    WCHAR szName[130] = L"SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\";
    NetCfgComponentItem * pLast = NULL, *pCurrent;

    hInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_NET, NULL, NULL, DIGCF_PRESENT );
    if (!hInfo)
    {
        return E_FAIL;
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
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szDetail, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
            break;

        /* query NetCfgInstanceId for current device */
        dwSize = sizeof(szNetCfg);
        if (RegQueryValueExW(hKey, L"NetCfgInstanceId", NULL, NULL, (LPBYTE)szNetCfg, &dwSize) != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            break;
        }

        /* allocate new INetConnectionItem */
        pCurrent = CoTaskMemAlloc(sizeof(NetCfgComponentItem));
        if (!pCurrent)
            break;

        ZeroMemory(pCurrent, sizeof(NetCfgComponentItem));
        CopyMemory(&pCurrent->ClassGUID, &GUID_DEVCLASS_NET, sizeof(GUID));
        CLSIDFromString(szNetCfg, &pCurrent->InstanceId); //FIXME

        /* set bind name */
        pCurrent->szBindName = CoTaskMemAlloc((wcslen(szNetCfg)+1) *sizeof(WCHAR));
        if (pCurrent->szBindName)
            wcscpy(pCurrent->szBindName, szNetCfg);

        /* retrieve ComponentId */
        dwSize = sizeof(szAdapterNetCfg);
        if (RegQueryValueExW(hKey, L"ComponentId", NULL, NULL, (LPBYTE)szAdapterNetCfg, &dwSize) == ERROR_SUCCESS)
        {
            pCurrent->szId = CoTaskMemAlloc((wcslen(szAdapterNetCfg)+1) * sizeof(WCHAR));
            if (pCurrent->szId)
                wcscpy(pCurrent->szId, szAdapterNetCfg);
        }
        /* set INetCfgComponent::GetDisplayName */
        dwSize = sizeof(szAdapterNetCfg);
        if (RegQueryValueExW(hKey, L"DriverDesc", NULL, NULL, (LPBYTE)szAdapterNetCfg, &dwSize) == ERROR_SUCCESS)
        {
            pCurrent->szDisplayName = CoTaskMemAlloc((wcslen(szAdapterNetCfg)+1) * sizeof(WCHAR));
            if (pCurrent->szDisplayName)
                wcscpy(pCurrent->szDisplayName, szAdapterNetCfg);
        }

        RegCloseKey(hKey);
        /*  open network connections details */
        wcscpy(&szName[80], szNetCfg);
        wcscpy(&szName[118], L"\\Connection");

        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szName, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            /* retrieve pnp instance id */
            dwSize = sizeof(szAdapterNetCfg);
            if (RegQueryValueExW(hKey, L"PnpInstanceID", NULL, NULL, (LPBYTE)szAdapterNetCfg, &dwSize) == ERROR_SUCCESS)
            {
                pCurrent->szNodeId = CoTaskMemAlloc((wcslen(szAdapterNetCfg)+1) * sizeof(WCHAR));
                if (pCurrent->szNodeId)
                    wcscpy(pCurrent->szNodeId, szAdapterNetCfg);
            }
            RegCloseKey(hKey);
        }

        if (SetupDiGetDeviceRegistryPropertyW(hInfo, &DevInfo, SPDRP_DEVICEDESC, NULL, (PBYTE)szNetCfg, sizeof(szNetCfg)/sizeof(WCHAR), &dwSize))
        {
            szNetCfg[(sizeof(szNetCfg)/sizeof(WCHAR))-1] = L'\0';
            pCurrent->szDisplayName = CoTaskMemAlloc((wcslen(szNetCfg)+1) * sizeof(WCHAR));
            if (pCurrent->szDisplayName)
                wcscpy(pCurrent->szDisplayName, szNetCfg);
        }

        if (pLast)
            pLast->pNext = pCurrent;
        else
            *pHead = pCurrent;

        pLast = pCurrent;

    }while(TRUE);

    SetupDiDestroyDeviceInfoList(hInfo);
    return NOERROR;
}


HRESULT
FindNetworkComponent(
    NetCfgComponentItem * pHead,
    LPCWSTR pszwComponentId,
    INetCfgComponent **pComponent,
    INetCfg * iface)
{
    while(pHead)
    {
        if (!_wcsicmp(pHead->szId, pszwComponentId))
        {
            return INetCfgComponent_Constructor(NULL, &IID_INetCfgComponent, (LPVOID*)pComponent, pHead, iface);
        }
        pHead = pHead->pNext;
    }
    return S_FALSE;
}



HRESULT
WINAPI
INetCfg_fnQueryInterface(
    INetCfg * iface,
    REFIID iid,
    LPVOID * ppvObj)
{
    INetCfgImpl * This = (INetCfgImpl*)iface;
    *ppvObj = NULL;

    if (IsEqualIID (iid, &IID_IUnknown) ||
        IsEqualIID (iid, &IID_INetCfg))
    {
        *ppvObj = This;
        INetCfg_AddRef(iface);
        return S_OK;
    }
    else if (IsEqualIID (iid, &IID_INetCfgLock))
    {
        if (This->bInitialized)
            return NETCFG_E_ALREADY_INITIALIZED;

        *ppvObj = (LPVOID)&This->lpVtblLock;
        This->hMutex = CreateMutexW(NULL, FALSE, L"NetCfgLock");

        INetCfgLock_AddRef(iface);
        return S_OK;
    }
    else if (IsEqualIID (iid, &IID_INetCfgPnpReconfigCallback))
    {
        if (This->bInitialized)
            return NETCFG_E_ALREADY_INITIALIZED;

        *ppvObj = (LPVOID)&This->lpVtblPnpReconfigCallback;
        INetCfgPnpReconfigCallback_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG
WINAPI
INetCfg_fnAddRef(
    INetCfg * iface)
{
    INetCfgImpl * This = (INetCfgImpl*)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    return refCount;
}

ULONG
WINAPI
INetCfg_fnRelease(
    INetCfg * iface)
{
    INetCfgImpl * This = (INetCfgImpl*)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    if (!refCount)
    {
        CoTaskMemFree (This);
    }
    return refCount;
}

HRESULT
WINAPI
INetCfg_fnInitialize(
    INetCfg * iface,
    PVOID pReserved)
{
    HRESULT hr;
    INetCfgImpl *This = (INetCfgImpl *)iface;

    if (This->bInitialized)
        return NETCFG_E_ALREADY_INITIALIZED;

    hr = EnumerateNetworkAdapter(&This->pNet);
    if (FAILED(hr))
        return hr;


    hr = EnumerateNetworkComponent(&GUID_DEVCLASS_NETCLIENT, &This->pClient);
    if (FAILED(hr))
        return hr;

    hr = EnumerateNetworkComponent(&GUID_DEVCLASS_NETSERVICE, &This->pService);
    if (FAILED(hr))
        return hr;


    hr = EnumerateNetworkComponent(&GUID_DEVCLASS_NETTRANS, &This->pProtocol);
    if (FAILED(hr))
        return hr;

    This->bInitialized = TRUE;
    return S_OK;
}

VOID
ApplyOrCancelChanges(
    NetCfgComponentItem *pHead,
    const CLSID * lpClassGUID,
    BOOL bApply)
{
    HKEY hKey;
    WCHAR szName[200];
    LPOLESTR pszGuid;

    while(pHead)
    {
        if (pHead->bChanged)
        {
            if (IsEqualGUID(lpClassGUID, &GUID_DEVCLASS_NET))
            {
                if (bApply)
                {
                    if (StringFromCLSID(&pHead->InstanceId, &pszGuid) == NOERROR)
                    {
                        swprintf(szName, L"SYSTEM\\CurrentControlSet\\Control\\Network\\%s", pszGuid);
                        CoTaskMemFree(pszGuid);

                        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szName, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
                        {
                            RegSetValueExW(hKey, NULL, 0, REG_SZ, (LPBYTE)pHead->szDisplayName, (wcslen(pHead->szDisplayName)+1) * sizeof(WCHAR));
                            RegCloseKey(hKey);
                        }
                    }
                }
            }
            else if (pHead->pNCCC)
            {
                if (bApply)
                {
                    INetCfgComponentControl_ApplyRegistryChanges(pHead->pNCCC);
                    //FIXME
                    // implement INetCfgPnpReconfigCallback and pass it to
                    //INetCfgComponentControl_ApplyPnpChanges(pHead->pNCCC, NULL);
                }
                else
                {
                    INetCfgComponentControl_CancelChanges(pHead->pNCCC);
                }
            }
        }
        pHead = pHead->pNext;
    }
}

VOID
FreeComponentItem(NetCfgComponentItem *pItem)
{
    CoTaskMemFree(pItem->szDisplayName);
    CoTaskMemFree(pItem->szHelpText);
    CoTaskMemFree(pItem->szId);
    CoTaskMemFree(pItem->szBindName);
    CoTaskMemFree(pItem->szNodeId);
    CoTaskMemFree(pItem->pszBinding);
    CoTaskMemFree(pItem);
}

HRESULT
WINAPI
INetCfg_fnUninitialize(
    INetCfg * iface)
{
    INetCfgImpl *This = (INetCfgImpl *)iface;
    NetCfgComponentItem *pItem;

    if (!This->bInitialized)
        return NETCFG_E_NOT_INITIALIZED;

    /* Free the services */
    while (This->pService != NULL)
    {
        pItem = This->pService;
        This->pService = pItem->pNext;
        FreeComponentItem(pItem);
    }

    /* Free the clients */
    while (This->pClient != NULL)
    {
        pItem = This->pClient;
        This->pClient = pItem->pNext;
        FreeComponentItem(pItem);
    }

    /* Free the protocols */
    while (This->pProtocol != NULL)
    {
        pItem = This->pProtocol;
        This->pProtocol = pItem->pNext;
        FreeComponentItem(pItem);
    }

    /* Free the adapters */
    while (This->pNet != NULL)
    {
        pItem = This->pNet;
        This->pNet = pItem->pNext;
        FreeComponentItem(pItem);
    }

    This->bInitialized = FALSE;

    return S_OK;
}


HRESULT
WINAPI
INetCfg_fnApply(
    INetCfg * iface)
{
    INetCfgImpl *This = (INetCfgImpl *)iface;

    if (!This->bInitialized)
        return NETCFG_E_NOT_INITIALIZED;

    ApplyOrCancelChanges(This->pNet, &GUID_DEVCLASS_NET, TRUE);
    ApplyOrCancelChanges(This->pClient, &GUID_DEVCLASS_NETCLIENT, TRUE);
    ApplyOrCancelChanges(This->pService, &GUID_DEVCLASS_NETSERVICE, TRUE);
    ApplyOrCancelChanges(This->pProtocol, &GUID_DEVCLASS_NETTRANS, TRUE);

    return S_OK;
}

HRESULT
WINAPI
INetCfg_fnCancel(
    INetCfg * iface)
{
    INetCfgImpl *This = (INetCfgImpl *)iface;

    if (!This->bInitialized)
        return NETCFG_E_NOT_INITIALIZED;

    ApplyOrCancelChanges(This->pClient, &GUID_DEVCLASS_NETCLIENT, FALSE);
    ApplyOrCancelChanges(This->pService, &GUID_DEVCLASS_NETSERVICE, FALSE);
    ApplyOrCancelChanges(This->pProtocol, &GUID_DEVCLASS_NETTRANS, FALSE);

    return S_OK;
}

HRESULT
WINAPI
INetCfg_fnEnumComponents(
    INetCfg * iface,
    const GUID *pguidClass,
    IEnumNetCfgComponent **ppenumComponent)
{
    INetCfgImpl *This = (INetCfgImpl *)iface;

    if (!This->bInitialized)
        return NETCFG_E_NOT_INITIALIZED;

    if (IsEqualGUID(&GUID_DEVCLASS_NET, pguidClass))
        return IEnumNetCfgComponent_Constructor (NULL, &IID_IEnumNetCfgComponent, (LPVOID*)ppenumComponent, This->pNet, iface);
    else if (IsEqualGUID(&GUID_DEVCLASS_NETCLIENT, pguidClass))
        return IEnumNetCfgComponent_Constructor (NULL, &IID_IEnumNetCfgComponent, (LPVOID*)ppenumComponent, This->pClient, iface);
    else if (IsEqualGUID(&GUID_DEVCLASS_NETSERVICE, pguidClass))
        return IEnumNetCfgComponent_Constructor (NULL, &IID_IEnumNetCfgComponent, (LPVOID*)ppenumComponent, This->pService, iface);
    else if (IsEqualGUID(&GUID_DEVCLASS_NETTRANS, pguidClass))
        return IEnumNetCfgComponent_Constructor (NULL, &IID_IEnumNetCfgComponent, (LPVOID*)ppenumComponent, This->pProtocol, iface);
    else
       return E_NOINTERFACE;
}


HRESULT
WINAPI
INetCfg_fnFindComponent(
    INetCfg * iface,
    LPCWSTR pszwComponentId,
    INetCfgComponent **pComponent)
{
    HRESULT hr;
    INetCfgImpl *This = (INetCfgImpl *)iface;

    if (!This->bInitialized)
        return NETCFG_E_NOT_INITIALIZED;

    hr = FindNetworkComponent(This->pClient, pszwComponentId, pComponent, iface);
    if (hr == S_OK)
        return hr;

    hr = FindNetworkComponent(This->pService, pszwComponentId, pComponent, iface);
    if (hr == S_OK)
        return hr;

    hr = FindNetworkComponent(This->pProtocol, pszwComponentId, pComponent, iface);
    if (hr == S_OK)
        return hr;

    return S_FALSE;
}

HRESULT
WINAPI
INetCfg_fnQueryNetCfgClass(
    INetCfg * iface,
    const GUID *pguidClass,
    REFIID riid,
    void **ppvObject)
{
    return E_FAIL;
}

static const INetCfgVtbl vt_NetCfg =
{
    INetCfg_fnQueryInterface,
    INetCfg_fnAddRef,
    INetCfg_fnRelease,
    INetCfg_fnInitialize,
    INetCfg_fnUninitialize,
    INetCfg_fnApply,
    INetCfg_fnCancel,
    INetCfg_fnEnumComponents,
    INetCfg_fnFindComponent,
    INetCfg_fnQueryNetCfgClass,
};

HRESULT WINAPI INetCfg_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv)
{
    INetCfgImpl *This;

    if (!ppv)
        return E_POINTER;

    This = (INetCfgImpl *) CoTaskMemAlloc(sizeof (INetCfgImpl));
    if (!This)
        return E_OUTOFMEMORY;

    This->ref = 1;
    This->lpVtbl = (const INetCfg*)&vt_NetCfg;
    This->lpVtblLock = (const INetCfgLock*)&vt_NetCfgLock;
    This->lpVtblPnpReconfigCallback = (const INetCfgPnpReconfigCallback*)&vt_NetCfgPnpReconfigCallback;
    This->hMutex = NULL;
    This->bInitialized = FALSE;
    This->pNet = NULL;
    This->pClient = NULL;
    This->pService = NULL;
    This->pProtocol = NULL;

    if (!SUCCEEDED (INetCfg_QueryInterface ((INetCfg*)This, riid, ppv)))
    {
        INetCfg_Release((INetCfg*)This);
        return E_NOINTERFACE;
    }

    INetCfg_Release((INetCfg*)This);
    return S_OK;
}
