#include <precomp.h>

typedef enum
{
    NET_TYPE_CLIENT = 1,
    NET_TYPE_SERVICE = 2,
    NET_TYPE_PROTOCOL = 3
}NET_TYPE;

typedef struct
{
    NET_TYPE Type;
    DWORD dwCharacteristics;
    LPWSTR szHelp;
}NET_ITEM, *PNET_ITEM;

typedef struct
{
    INetConnectionPropertyUi *lpVtbl;
    INetConnection * pCon;
    LONG ref;
}INetConnectionPropertyUiImpl, *LPINetConnectionPropertyUiImpl;


/// CLASSID
/// {7007ACC5-3202-11D1-AAD2-00805FC1270E}
/// open network properties and wlan properties

HPROPSHEETPAGE
InitializePropertySheetPage(LPWSTR resname, DLGPROC dlgproc, LPARAM lParam, LPWSTR szTitle)
{
    PROPSHEETPAGEW ppage;

    memset(&ppage, 0x0, sizeof(PROPSHEETPAGEW));
    ppage.dwSize = sizeof(PROPSHEETPAGEW);
    ppage.dwFlags = PSP_DEFAULT;
    ppage.u.pszTemplate = resname;
    ppage.pfnDlgProc = dlgproc;
    ppage.lParam = lParam;
    ppage.hInstance = netshell_hInstance;
    ppage.pszTitle = szTitle;
    if (szTitle)
    {
        ppage.dwFlags |= PSP_USETITLE;
    }
    return CreatePropertySheetPageW(&ppage);
}

HRESULT
LaunchUIDlg(UINT * pResourceIDs, DLGPROC * pDlgs, UINT Count, NETCON_PROPERTIES * pProperties)
{
    HPROPSHEETPAGE * hpsp;
    PROPSHEETHEADERW psh;
    BOOL ret;
    UINT Index, Offset;

    hpsp = CoTaskMemAlloc(Count * sizeof(HPROPSHEETPAGE));
    if (!hpsp)
        return E_OUTOFMEMORY;

    ZeroMemory(hpsp, Count * sizeof(HPROPSHEETPAGE));

    Index = 0;
    Offset = 0;
    do
    {
        hpsp[Offset] = InitializePropertySheetPage(MAKEINTRESOURCEW(pResourceIDs[Index]), pDlgs[Index], (LPARAM)pProperties, NULL);
        if (hpsp[Offset])
            Offset++;

    }while(++Index < Count);

    if (!Offset)
    {
        CoTaskMemFree(hpsp);
        return E_FAIL;
    }


   ZeroMemory(&psh, sizeof(PROPSHEETHEADERW));
   psh.dwSize = sizeof(PROPSHEETHEADERW);
   psh.dwFlags = PSP_DEFAULT | PSH_PROPTITLE;
   psh.pszCaption = pProperties->pszwName;
   psh.hwndParent = NULL;
   psh.u3.phpage = hpsp;
   psh.nPages = Offset;
   psh.hInstance = netshell_hInstance;


   ret = PropertySheetW(&psh);
   CoTaskMemFree(hpsp);

   if (ret < 0)
       return E_FAIL;
   else
       return S_OK;
}

VOID
AddItemToListView(HWND hDlgCtrl, PNET_ITEM pItem, LPWSTR szName)
{
    LVITEMW lvItem;

    ZeroMemory(&lvItem, sizeof(lvItem));
    lvItem.mask  = LVIF_TEXT | LVIF_PARAM;
    lvItem.pszText = szName;
    lvItem.lParam = (LPARAM)pItem;
    lvItem.iItem = ListView_GetItemCount(hDlgCtrl);

    SendMessageW(hDlgCtrl, LVM_INSERTITEMW, 0, (LPARAM)&lvItem);
}

VOID
EnumClientServiceProtocol(HWND hDlgCtrl, HKEY hKey, UINT Type)
{
    DWORD dwIndex = 0;
    DWORD dwSize;
    DWORD dwRetVal;
    DWORD dwCharacteristics;
    WCHAR szName[60];
    WCHAR szText[40];
    WCHAR szHelp[200];
    HKEY hSubKey;
    static WCHAR szNDI[] = L"\\Ndi";
    PNET_ITEM pItem;

    do
    {
        szHelp[0] = L'\0';
        dwCharacteristics = 0;
        szText[0] = L'\0';

        dwSize = sizeof(szName)/sizeof(WCHAR);
        if (RegEnumKeyExW(hKey, dwIndex++, szName, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
        {
            /* Get Help Text */
            if (dwSize + (sizeof(szNDI)/sizeof(WCHAR)) + 1 < sizeof(szName)/sizeof(WCHAR))
            {
                wcscpy(&szName[dwSize], szNDI);
                if (RegOpenKeyExW(hKey, szName, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS)
                {
#if 0
                    if (RegLoadMUIStringW(hSubKey, L"HelpText", szHelp, sizeof(szHelp)/sizeof(WCHAR), &dwRetVal, 0, NULL) == ERROR_SUCCESS)
                        szHelp[(sizeof(szHelp)/sizeof(WCHAR))-1] = L'\0';
#else
                    DWORD dwType;
                    dwRetVal = sizeof(szHelp);
                    if (RegQueryValueExW(hSubKey, L"HelpText", NULL, &dwType, (LPBYTE)szHelp, &dwRetVal) == ERROR_SUCCESS)
                        szHelp[(sizeof(szHelp)/sizeof(WCHAR))-1] = L'\0';
#endif
                    RegCloseKey(hSubKey);
                }
                szName[dwSize] = L'\0';
            }
            /* Get Characteristics */
            if (RegOpenKeyExW(hKey, szName, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS)
            {
                dwSize = sizeof(dwCharacteristics);
                if (RegQueryValueExW(hSubKey, L"Characteristics", NULL, NULL, (LPBYTE)&dwCharacteristics, &dwSize) == ERROR_SUCCESS)
                {
                    if (dwCharacteristics & NCF_HIDDEN)
                    {
                        RegCloseKey(hSubKey);
                        continue;
                    }
                }
                /* Get the Client/Procotol/Service name */
                dwSize = sizeof(szText);
                if (RegQueryValueExW(hSubKey, L"Description", NULL, NULL, (LPBYTE)szText, &dwSize) == ERROR_SUCCESS)
                {
                    szText[(sizeof(szText)/sizeof(WCHAR))-1] = L'\0';
                }
                RegCloseKey(hSubKey);
            }

            pItem = CoTaskMemAlloc(sizeof(NET_ITEM));
            if (!pItem)
                continue;

            pItem->dwCharacteristics = dwCharacteristics;
            pItem->Type = Type;
            pItem->szHelp = CoTaskMemAlloc((wcslen(szHelp)+1)*sizeof(WCHAR));
            if (pItem->szHelp)
                wcscpy(pItem->szHelp, szHelp);

            AddItemToListView(hDlgCtrl, pItem, szText);
        }
        else
           break;

    }while(TRUE);

}


VOID
InitializeLANPropertiesUIDlg(HWND hwndDlg)
{
    static WCHAR szNetClient[] = L"SYSTEM\\CurrentControlSet\\Control\\Network\\{4d36e973-e325-11ce-bfc1-08002be10318}";
    static WCHAR szNetService[] = L"SYSTEM\\CurrentControlSet\\Control\\Network\\{4d36e974-e325-11ce-bfc1-08002be10318}";
    static WCHAR szNetProtocol[] =L"System\\CurrentControlSet\\Control\\Network\\{4D36E975-E325-11CE-BFC1-08002BE10318}";
    HKEY hKey;
    HWND hDlgCtrl;

    hDlgCtrl = GetDlgItem(hwndDlg, IDC_COMPONENTSLIST);
    /* Enumerate Clients */
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szNetClient, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        EnumClientServiceProtocol(hwndDlg, hKey, NET_TYPE_CLIENT);
        RegCloseKey(hKey);
    }

    /* Enumerate Service */
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szNetService, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        EnumClientServiceProtocol(hwndDlg, hKey, NET_TYPE_SERVICE);
        RegCloseKey(hKey);
    }

    /* Enumerate Protocol */
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szNetProtocol, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        EnumClientServiceProtocol(hwndDlg, hKey, NET_TYPE_PROTOCOL);
        RegCloseKey(hKey);
    }


}


INT_PTR
CALLBACK
LANPropertiesUIDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    PROPSHEETPAGE *page;
    NETCON_PROPERTIES * pProperties = (NETCON_PROPERTIES*)GetWindowLongPtr(hwndDlg, DWLP_USER);
    switch(uMsg)
    {
        case WM_INITDIALOG:
            page = (PROPSHEETPAGE*)lParam;
            pProperties = (NETCON_PROPERTIES*)page->lParam;
            SendDlgItemMessageW(hwndDlg, IDC_NETCARDNAME, WM_SETTEXT, 0, (LPARAM)pProperties->pszwDeviceName);
            if (pProperties->dwCharacter & NCCF_SHOW_ICON)
            {
                /* check show item on taskbar*/
                SendDlgItemMessageW(hwndDlg, IDC_SHOWTASKBAR, BM_SETCHECK, BST_CHECKED, 0);
            }
            if (pProperties->dwCharacter & NCCF_NOTIFY_DISCONNECTED)
            {
                /* check notify item */
                SendDlgItemMessageW(hwndDlg, IDC_NOTIFYNOCONNECTION, BM_SETCHECK, BST_CHECKED, 0);
            }
            InitializeLANPropertiesUIDlg(hwndDlg);
            return TRUE;
    }
    return FALSE;
}


HRESULT
ShowLANConnectionPropertyDialog(NETCON_PROPERTIES * pProperties)
{
    UINT ResourceId[1] = { IDD_NETPROPERTIES };
    DLGPROC Dlgs[1] = {LANPropertiesUIDlg};
    INITCOMMONCONTROLSEX ic;


    ic.dwSize = sizeof(INITCOMMONCONTROLSEX);
    ic.dwICC  = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&ic);

    return LaunchUIDlg(ResourceId, Dlgs, 1, pProperties);
}

INT_PTR
CALLBACK
LANStatusUIDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    NETCON_PROPERTIES * pProperties = (NETCON_PROPERTIES*)GetWindowLongPtr(hwndDlg, DWLP_USER);
    switch(uMsg)
    {
        case WM_INITDIALOG:
            pProperties = (NETCON_PROPERTIES*)lParam;
            return TRUE;
    }
    return FALSE;
}


HRESULT
ShowLANConnectionStatusDialog(NETCON_PROPERTIES * pProperties)
{
    UINT ResourceId[1] = { IDD_NETSTATUS };
    DLGPROC Dlgs[1] = {LANStatusUIDlg};
    return LaunchUIDlg(ResourceId, Dlgs, 1, pProperties);
}

static
HRESULT
WINAPI
INetConnectionPropertyUi_fnQueryInterface(
    INetConnectionPropertyUi * iface,
    REFIID iid,
    LPVOID * ppvObj)
{
    INetConnectionPropertyUiImpl * This =  (INetConnectionPropertyUiImpl*)iface;
    *ppvObj = NULL;

    if (IsEqualIID (iid, &IID_IUnknown) ||
        IsEqualIID (iid, &IID_INetConnectionPropertyUi))
    {
        *ppvObj = This;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static
ULONG
WINAPI
INetConnectionPropertyUi_fnAddRef(
    INetConnectionPropertyUi * iface)
{
    INetConnectionPropertyUiImpl * This =  (INetConnectionPropertyUiImpl*)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    return refCount;
}

static
ULONG
WINAPI
INetConnectionPropertyUi_fnRelease(
    INetConnectionPropertyUi * iface)
{
    INetConnectionPropertyUiImpl * This =  (INetConnectionPropertyUiImpl*)iface;
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
INetConnectionPropertyUi_fnSetConnection(
    INetConnectionPropertyUi * iface,
    INetConnection *pCon)
{
    INetConnectionPropertyUiImpl * This =  (INetConnectionPropertyUiImpl*)iface;

    if (!pCon)
        return E_POINTER;

    if (This->pCon)
        INetConnection_Release(This->pCon);

    This->pCon = pCon;
    INetConnection_AddRef(This->pCon);
    return S_OK;
}

static
HRESULT
WINAPI
INetConnectionPropertyUi_fnAddPages(
    INetConnectionPropertyUi * iface,
    HWND hwndParent, 
    LPFNADDPROPSHEETPAGE pfnAddPage,
    LPARAM lParam)
{
    HPROPSHEETPAGE hProp;
    NETCON_PROPERTIES * pProperties;
    BOOL ret;
    HRESULT hr;
    INITCOMMONCONTROLSEX initEx;
    INetConnectionPropertyUiImpl * This =  (INetConnectionPropertyUiImpl*)iface;


    initEx.dwSize = sizeof(initEx);
    initEx.dwICC = ICC_LISTVIEW_CLASSES;


    if(!InitCommonControlsEx(&initEx))
        return E_FAIL;



    hr = INetConnection_GetProperties(This->pCon, &pProperties);
    if (FAILED(hr))
        return hr;

    hProp = InitializePropertySheetPage(MAKEINTRESOURCEW(IDD_NETPROPERTIES), LANPropertiesUIDlg, (LPARAM)pProperties, pProperties->pszwName);
    if (hProp)
    {
        ret = (*pfnAddPage)(hProp, lParam);
        if (ret)
        {
            return NOERROR;
        }
        DestroyPropertySheetPage(hProp);
    }
    return E_FAIL;
}

static const INetConnectionPropertyUiVtbl vt_NetConnectionPropertyUi =
{
    INetConnectionPropertyUi_fnQueryInterface,
    INetConnectionPropertyUi_fnAddRef,
    INetConnectionPropertyUi_fnRelease,
    INetConnectionPropertyUi_fnSetConnection,
    INetConnectionPropertyUi_fnAddPages,
};

HRESULT WINAPI LanConnectUI_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv)
{
    INetConnectionPropertyUiImpl * This;

    if (!ppv)
        return E_POINTER;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    This = (INetConnectionPropertyUiImpl *) CoTaskMemAlloc(sizeof (INetConnectionPropertyUiImpl));
    if (!This)
        return E_OUTOFMEMORY;

    This->ref = 1;
    This->pCon = NULL;
    This->lpVtbl = (INetConnectionPropertyUi*)&vt_NetConnectionPropertyUi;

    if (!SUCCEEDED (INetConnectionPropertyUi_fnQueryInterface ((INetConnectionPropertyUi*)This, riid, ppv)))
    {
        IUnknown_Release((IUnknown*)This);
        return E_NOINTERFACE;
    }

    IUnknown_Release((IUnknown*)This);
    return S_OK;
}
