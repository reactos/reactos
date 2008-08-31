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
    INetCfgComponent  *pNCfgComp;
    UINT NumPropDialogOpen;
}NET_ITEM, *PNET_ITEM;

typedef struct
{
    INetConnectionPropertyUi *lpVtbl;
    INetConnection * pCon;
    INetCfg * pNCfg;
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
          pItem->pNCfgComp = pNCfgComp;
          pItem->NumPropDialogOpen = 0;
          AddItemToListView(hDlgCtrl, pItem, pDisplayName);
          CoTaskMemFree(pDisplayName);
    }
    IEnumNetCfgComponent_Release(pENetCfg);
}


VOID
InitializeLANPropertiesUIDlg(HWND hwndDlg, INetConnectionPropertyUiImpl * This)
{
    HRESULT hr;
    INetCfg * pNCfg;
    HWND hDlgCtrl = GetDlgItem(hwndDlg, IDC_COMPONENTSLIST);
    LVCOLUMNW lc;
    RECT rc;
    DWORD dwStyle;
    NETCON_PROPERTIES * pProperties;

    hr = INetConnection_GetProperties(This->pCon, &pProperties);
    if (FAILED(hr))
        return;

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
    This->pNCfg = pNCfg;
}

VOID
ShowNetworkComponentProperties(
    HWND hwndDlg,
    INetConnectionPropertyUiImpl * This)
{
    LVITEMW lvItem;
    HWND hDlgCtrl;
    UINT Index, Count;
    HRESULT hr;
    INetConnectionPropertyUi * pConUI = NULL;
    INetCfgComponent * pNCfgComp;
    PNET_ITEM pItem;
    INetCfgLock * pLock;
    LPWSTR pLockApp;

    hDlgCtrl = GetDlgItem(hwndDlg, IDC_COMPONENTSLIST);
    Count = ListView_GetItemCount(hDlgCtrl);
    if (!Count)
        return;


    hr = CoCreateInstance(&CLSID_LANConnectUI, 
                          NULL,
                          CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD, 
                          &IID_INetConnectionPropertyUi, 
                          (LPVOID)&pConUI);

    if (FAILED(hr))
    {
        return;
    }
    hr = INetConnectionPropertyUi_SetConnection(pConUI, This->pCon);
    if (FAILED(hr))
    {
        INetConnectionPropertyUi_Release(pConUI);
        return;
    }

    ZeroMemory(&lvItem, sizeof(LVITEMW));
    lvItem.mask = LVIF_PARAM | LVIF_STATE;
    lvItem.stateMask = (UINT)-1;
    for (Index = 0; Index < Count; Index++)
    {
        lvItem.iItem = Index;
        if (SendMessageW(hDlgCtrl, LVM_GETITEMW, 0, (LPARAM)&lvItem))
        {
           if (lvItem.state & LVIS_SELECTED)
               break;
        }
    }

    if (!(lvItem.state & LVIS_SELECTED))
    {
        INetConnectionPropertyUi_Release(pConUI);
        return;
    }

    pItem = (PNET_ITEM)lvItem.lParam;
    pNCfgComp = (INetCfgComponent*) pItem->pNCfgComp;

    hr = INetCfg_QueryInterface(This->pNCfg, &IID_INetCfgLock, (LPVOID*)&pLock);
    if (FAILED(hr))
        return;

    hr = INetCfgLock_AcquireWriteLock(pLock, 10000, L"", &pLockApp);
    if (FAILED(hr))
    {
        CoTaskMemFree(pLockApp);
        INetConnectionPropertyUi_Release(pConUI);
        return;
    }


    hr = INetCfgComponent_RaisePropertyUi(pNCfgComp, GetParent(hwndDlg), NCRP_QUERY_PROPERTY_UI, (IUnknown*)pConUI);
    if (SUCCEEDED(hr))
    {
        hr = INetCfgComponent_RaisePropertyUi(pNCfgComp, GetParent(hwndDlg), NCRP_SHOW_PROPERTY_UI, (IUnknown*)pConUI);
    }

    INetCfgLock_ReleaseWriteLock(pLock);
    INetConnectionPropertyUi_Release(pConUI);

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
    INetConnectionPropertyUiImpl * This;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            page = (PROPSHEETPAGE*)lParam;
            This = (INetConnectionPropertyUiImpl*)page->lParam;
            InitializeLANPropertiesUIDlg(hwndDlg, This);
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)This);
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
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_PROPERTIES)
            {
                This = (INetConnectionPropertyUiImpl*) GetWindowLongPtr(hwndDlg, DWLP_USER);
                ShowNetworkComponentProperties(hwndDlg, This);
                return FALSE;
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
    BOOL ret;
    HRESULT hr = E_FAIL;
    INITCOMMONCONTROLSEX initEx;
    NETCON_PROPERTIES * pProperties;
    INetConnectionPropertyUiImpl * This =  (INetConnectionPropertyUiImpl*)iface;


    initEx.dwSize = sizeof(initEx);
    initEx.dwICC = ICC_LISTVIEW_CLASSES;
    if(!InitCommonControlsEx(&initEx))
        return E_FAIL;

    hr = INetConnection_GetProperties(This->pCon, &pProperties);
    if (FAILED(hr))
        return hr;

    hProp = InitializePropertySheetPage(MAKEINTRESOURCEW(IDD_NETPROPERTIES), LANPropertiesUIDlg, (LPARAM)This, pProperties->pszwName);
    if (hProp)
    {
        ret = (*pfnAddPage)(hProp, lParam);
        if (ret)
        {
            hr = NOERROR;
        }
        DestroyPropertySheetPage(hProp);
    }
    //NcFreeNetconProperties(pProperties);
    return hr;
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
    This->pNCfg = NULL;
    This->lpVtbl = (INetConnectionPropertyUi*)&vt_NetConnectionPropertyUi;

    if (!SUCCEEDED (INetConnectionPropertyUi_fnQueryInterface ((INetConnectionPropertyUi*)This, riid, ppv)))
    {
        IUnknown_Release((IUnknown*)This);
        return E_NOINTERFACE;
    }

    IUnknown_Release((IUnknown*)This);
    return S_OK;
}
