#include <precomp.h>

/// CLASSID
/// {7007ACC5-3202-11D1-AAD2-00805FC1270E}
/// open network properties and wlan properties

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

VOID
AddItemToListView(HWND hDlgCtrl, PNET_ITEM pItem, LPWSTR szName)
{
    LVITEMW lvItem;

    ZeroMemory(&lvItem, sizeof(lvItem));
    lvItem.mask  = LVIF_TEXT | LVIF_PARAM;
    lvItem.pszText = szName;
    lvItem.lParam = (LPARAM)pItem;
    lvItem.iItem = ListView_GetItemCount(hDlgCtrl);
    lvItem.iItem = SendMessageW(hDlgCtrl, LVM_INSERTITEMW, 0, (LPARAM)&lvItem);
    ListView_SetCheckState(hDlgCtrl, lvItem.iItem, TRUE);
}

VOID
EnumClientServiceProtocol(HWND hDlgCtrl, HKEY hKey, UINT Type)
{
    DWORD dwIndex = 0;
    DWORD dwSize;
    DWORD dwRetVal;
    DWORD dwCharacteristics;
    WCHAR szName[100];
    WCHAR szText[100];
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

            if (!wcslen(szText))
                continue;


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
EnumComponents(HWND hDlgCtrl, INetCfg * pNCfg, const GUID * CompGuid, UINT Type)
{
    HRESULT hr;
    IEnumNetCfgComponent * pENetCfg;
    INetCfgComponent  *pNCfgComp;
    ULONG Num;
    DWORD dwCharacteristics;
    LPOLESTR pDisplayName, pHelpText;
    PNET_ITEM pItem;


    hr = INetCfg_EnumComponents(pNCfg, CompGuid, &pENetCfg);
    if (FAILED(hr))
    {
        INetCfg_Release(pNCfg);
        return;
    }
    while(IEnumNetCfgComponent_Next(pENetCfg, 1, &pNCfgComp, &Num) == S_OK)
    {
          hr = INetCfgComponent_GetCharacteristics(pNCfgComp, &dwCharacteristics);
          if (SUCCEEDED(hr) && (dwCharacteristics & NCF_HIDDEN))
          {
              INetCfgComponent_Release(pNCfgComp);
              continue;
          }
          pDisplayName = NULL;
          pHelpText = NULL;
          hr = INetCfgComponent_GetDisplayName(pNCfgComp, &pDisplayName);
          hr = INetCfgComponent_GetHelpText(pNCfgComp, &pHelpText);

            pItem = CoTaskMemAlloc(sizeof(NET_ITEM));
            if (!pItem)
                continue;
          pItem->dwCharacteristics = dwCharacteristics;
          pItem->szHelp = pHelpText;
          pItem->Type = Type;
          AddItemToListView(hDlgCtrl, pItem, pDisplayName);
          CoTaskMemFree(pDisplayName);

          INetCfgComponent_Release(pNCfgComp);
    }
    IEnumNetCfgComponent_Release(pENetCfg);
}


VOID
InitializeLANPropertiesUIDlg(HWND hwndDlg)
{
    HRESULT hr;
    INetCfg * pNCfg;
    HWND hDlgCtrl = GetDlgItem(hwndDlg, IDC_COMPONENTSLIST);
    LVCOLUMNW lc;
    RECT rc;
    DWORD dwStyle;

    memset(&lc, 0, sizeof(LV_COLUMN));
    lc.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
    lc.fmt = LVCFMT_FIXED_WIDTH;
    if (GetClientRect(hDlgCtrl, &rc))
    {
        lc.mask |= LVCF_WIDTH;
        lc.cx = rc.right - rc.left;
    }
    lc.pszText    = L"";
    (void)SendMessageW(hDlgCtrl, LVM_INSERTCOLUMNW, 0, (LPARAM)&lc);
    dwStyle = (DWORD) SendMessage(hDlgCtrl, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
    dwStyle = dwStyle | LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES;
    SendMessage(hDlgCtrl, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle);



    hr = CoCreateInstance(&CLSID_CNetCfg, NULL, CLSCTX_INPROC_SERVER, &IID_INetCfg, (LPVOID*)&pNCfg);
    if (FAILED(hr))
        return;

    hr = INetCfg_Initialize(pNCfg, NULL);
    if (FAILED(hr))
    {
        INetCfg_Release(pNCfg);
        return;
    }

    EnumComponents(hDlgCtrl, pNCfg, &GUID_DEVCLASS_NETCLIENT, NET_TYPE_CLIENT);
    EnumComponents(hDlgCtrl, pNCfg, &GUID_DEVCLASS_NETSERVICE, NET_TYPE_SERVICE);
    EnumComponents(hDlgCtrl, pNCfg, &GUID_DEVCLASS_NETTRANS, NET_TYPE_PROTOCOL);

    INetCfg_Release(pNCfg);
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
    LPNMLISTVIEW lppl;
    LVITEMW li;
    PNET_ITEM pItem;
    NETCON_PROPERTIES * pProperties;

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
        case WM_NOTIFY:
            lppl = (LPNMLISTVIEW) lParam;
            if (lppl->hdr.code == LVN_ITEMCHANGING)
            {
                    ZeroMemory(&li, sizeof(li));
                    li.mask = LVIF_PARAM;
                    li.iItem = lppl->iItem;
                    if (!SendMessageW(lppl->hdr.hwndFrom, LVM_GETITEMW, 0, (LPARAM)&li))
                        return TRUE;

                    pItem = (PNET_ITEM)li.lParam;
                    if (!pItem)
                        return TRUE;

                    if (!(lppl->uOldState & LVIS_FOCUSED) && (lppl->uNewState & LVIS_FOCUSED))
                    {
                        /* new focused item */
                        if (pItem->dwCharacteristics & NCF_NOT_USER_REMOVABLE)
                            EnableWindow(GetDlgItem(hwndDlg, IDC_UNINSTALL), FALSE);
                        else
                            EnableWindow(GetDlgItem(hwndDlg, IDC_UNINSTALL), TRUE);

                        if (pItem->dwCharacteristics & NCF_HAS_UI)
                            EnableWindow(GetDlgItem(hwndDlg, IDC_PROPERTIES), TRUE);
                        else
                            EnableWindow(GetDlgItem(hwndDlg, IDC_PROPERTIES), FALSE);

                        SendDlgItemMessageW(hwndDlg, IDC_DESCRIPTION, WM_SETTEXT, 0, (LPARAM)pItem->szHelp);
                    }
             }
             break;
    }
    return FALSE;
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
