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
    INetConnectionPropertyUi2 *lpVtbl;
    INetLanConnectionUiInfo * lpLanConUiInfoVtbl;
    INetConnection * pCon;
    INetCfgLock *NCfgLock;
    INetCfg * pNCfg;
    NETCON_PROPERTIES * pProperties;
    LONG ref;
}INetConnectionPropertyUiImpl, *LPINetConnectionPropertyUiImpl;

static LPINetConnectionPropertyUiImpl __inline impl_from_NetLanConnectionUiInfo(INetLanConnectionUiInfo *iface)
{
    return (LPINetConnectionPropertyUiImpl)((char *)iface - FIELD_OFFSET(INetConnectionPropertyUiImpl, lpLanConUiInfoVtbl));
}

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
    if (szTitle)
    {
        ppage.dwFlags |= PSP_USETITLE;
        ppage.pszTitle = szTitle;
    }
    return CreatePropertySheetPageW(&ppage);
}

VOID
AddItemToListView(HWND hDlgCtrl, PNET_ITEM pItem, LPWSTR szName, BOOL bChecked)
{
    LVITEMW lvItem;

    ZeroMemory(&lvItem, sizeof(lvItem));
    lvItem.mask  = LVIF_TEXT | LVIF_PARAM;
    lvItem.pszText = szName;
    lvItem.lParam = (LPARAM)pItem;
    lvItem.iItem = ListView_GetItemCount(hDlgCtrl);
    lvItem.iItem = SendMessageW(hDlgCtrl, LVM_INSERTITEMW, 0, (LPARAM)&lvItem);
    ListView_SetCheckState(hDlgCtrl, lvItem.iItem, bChecked);
}

BOOL
GetINetCfgComponent(INetCfg * pNCfg, INetConnectionPropertyUiImpl * This, INetCfgComponent ** pOut)
{
    LPWSTR pName;
    HRESULT hr;
    INetCfgComponent * pNCg;
    ULONG Fetched;
    IEnumNetCfgComponent * pEnumCfg;

    hr = INetCfg_EnumComponents(pNCfg, &GUID_DEVCLASS_NET, &pEnumCfg);
    if (FAILED(hr))
    {
        return FALSE;
    }

    while(IEnumNetCfgComponent_Next(pEnumCfg, 1, &pNCg, &Fetched) == S_OK)
    {
       hr = INetCfgComponent_GetDisplayName(pNCg, &pName);
       if (SUCCEEDED(hr))
       {
           if (!wcsicmp(pName, This->pProperties->pszwDeviceName))
           {
               *pOut = pNCg;
               IEnumNetCfgComponent_Release(pEnumCfg);
               return TRUE;
           }
           CoTaskMemFree(pName);
       }
       INetCfgComponent_Release(pNCg);
    }
    IEnumNetCfgComponent_Release(pEnumCfg);
    return FALSE;
}

VOID
EnumComponents(HWND hDlgCtrl, INetConnectionPropertyUiImpl * This, INetCfg * pNCfg, const GUID * CompGuid, UINT Type)
{
    HRESULT hr;
    IEnumNetCfgComponent * pENetCfg;
    INetCfgComponent  *pNCfgComp, *pAdapterCfgComp;
    INetCfgComponentBindings * pCompBind;
    ULONG Num;
    DWORD dwCharacteristics;
    LPOLESTR pDisplayName, pHelpText;
    PNET_ITEM pItem;
    BOOL bChecked;

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
          bChecked = TRUE; //ReactOS hack
          hr = INetCfgComponent_QueryInterface(pNCfgComp, &IID_INetCfgComponentBindings, (LPVOID*)&pCompBind);
          if (SUCCEEDED(hr))
          {
              if (GetINetCfgComponent(pNCfg, This, &pAdapterCfgComp))
              {
                  hr = INetCfgComponentBindings_IsBoundTo(pCompBind, pAdapterCfgComp);
                  if (hr == S_OK)
                      bChecked = TRUE;
                  else
                      bChecked = FALSE;
                  INetCfgComponent_Release(pAdapterCfgComp);
                  INetCfgComponentBindings_Release(pCompBind);
              }
          }
          pItem = CoTaskMemAlloc(sizeof(NET_ITEM));
          if (!pItem)
              continue;
          pItem->dwCharacteristics = dwCharacteristics;
          pItem->szHelp = pHelpText;
          pItem->Type = Type;
          pItem->pNCfgComp = pNCfgComp;
          pItem->NumPropDialogOpen = 0;


          AddItemToListView(hDlgCtrl, pItem, pDisplayName, bChecked);
          CoTaskMemFree(pDisplayName);
    }
    IEnumNetCfgComponent_Release(pENetCfg);
}


VOID
InitializeLANPropertiesUIDlg(HWND hwndDlg, INetConnectionPropertyUiImpl * This)
{
    HRESULT hr;
    INetCfg * pNCfg;
    INetCfgLock * pNCfgLock;
    HWND hDlgCtrl = GetDlgItem(hwndDlg, IDC_COMPONENTSLIST);
    LVCOLUMNW lc;
    RECT rc;
    DWORD dwStyle;
    LPWSTR pDisplayName;
    LVITEMW li;

    SendDlgItemMessageW(hwndDlg, IDC_NETCARDNAME, WM_SETTEXT, 0, (LPARAM)This->pProperties->pszwDeviceName);
    if (This->pProperties->dwCharacter & NCCF_SHOW_ICON)
    {
        /* check show item on taskbar*/
        SendDlgItemMessageW(hwndDlg, IDC_SHOWTASKBAR, BM_SETCHECK, BST_CHECKED, 0);
    }
    if (This->pProperties->dwCharacter & NCCF_NOTIFY_DISCONNECTED)
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

    hr = INetCfgLock_QueryInterface(pNCfg, &IID_INetCfgLock, (LPVOID*)&pNCfgLock);
    hr = INetCfgLock_AcquireWriteLock(pNCfgLock, 100, L"", &pDisplayName);
    if (hr == S_FALSE)
    {
        CoTaskMemFree(pDisplayName);
        return;
    }

    This->NCfgLock = pNCfgLock;
    hr = INetCfg_Initialize(pNCfg, NULL);
    if (FAILED(hr))
    {
        INetCfg_Release(pNCfg);
        return;
    }

    EnumComponents(hDlgCtrl, This, pNCfg, &GUID_DEVCLASS_NETCLIENT, NET_TYPE_CLIENT);
    EnumComponents(hDlgCtrl, This, pNCfg, &GUID_DEVCLASS_NETSERVICE, NET_TYPE_SERVICE);
    EnumComponents(hDlgCtrl, This, pNCfg, &GUID_DEVCLASS_NETTRANS, NET_TYPE_PROTOCOL);
    This->pNCfg = pNCfg;

    ZeroMemory(&li, sizeof(li));
    li.mask = LVIF_STATE;
    li.stateMask = (UINT)-1;
    li.state = LVIS_FOCUSED|LVIS_SELECTED;
    (void)SendMessageW(hDlgCtrl, LVM_SETITEMW, 0, (LPARAM)&li);
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
    INetCfgComponent * pNCfgComp;
    PNET_ITEM pItem;

    hDlgCtrl = GetDlgItem(hwndDlg, IDC_COMPONENTSLIST);
    Count = ListView_GetItemCount(hDlgCtrl);
    if (!Count)
        return;

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
        return;
    }

    pItem = (PNET_ITEM)lvItem.lParam;
    pNCfgComp = (INetCfgComponent*) pItem->pNCfgComp;

    hr = INetCfgComponent_RaisePropertyUi(pNCfgComp, GetParent(hwndDlg), NCRP_QUERY_PROPERTY_UI, (IUnknown*)This);
    if (SUCCEEDED(hr))
    {
        hr = INetCfgComponent_RaisePropertyUi(pNCfgComp, GetParent(hwndDlg), NCRP_SHOW_PROPERTY_UI, (IUnknown*)This);
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
    LPNMLISTVIEW lppl;
    LVITEMW li;
    PNET_ITEM pItem;
    INetConnectionPropertyUiImpl * This;
    LPPSHNOTIFY lppsn;
    HRESULT hr;

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
            lppsn = (LPPSHNOTIFY) lParam;
            if (lppsn->hdr.code == PSN_APPLY)
            {
                This = (INetConnectionPropertyUiImpl*)GetWindowLongPtr(hwndDlg, DWLP_USER);
                if (This->pNCfg)
                {
                    hr = INetCfg_Apply(This->pNCfg);
                    if (SUCCEEDED(hr))
                        return PSNRET_NOERROR;
                    else
                        return PSNRET_INVALID;
                }
                return PSNRET_NOERROR;
            }
            else if (lppsn->hdr.code == PSN_APPLY)
            {
                This = (INetConnectionPropertyUiImpl*)GetWindowLongPtr(hwndDlg, DWLP_USER);
                if (This->pNCfg)
                {
                    hr = INetCfg_Cancel(This->pNCfg);
                    if (SUCCEEDED(hr))
                        return PSNRET_NOERROR;
                    else
                        return PSNRET_INVALID;
                }
                return PSNRET_NOERROR;
            }
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
INetConnectionPropertyUi2_fnQueryInterface(
    INetConnectionPropertyUi2 * iface,
    REFIID iid,
    LPVOID * ppvObj)
{
    //LPOLESTR pStr;
    INetConnectionPropertyUiImpl * This =  (INetConnectionPropertyUiImpl*)iface;
    *ppvObj = NULL;

    if (IsEqualIID (iid, &IID_IUnknown) ||
        IsEqualIID (iid, &IID_INetConnectionPropertyUi) ||
        IsEqualIID (iid, &IID_INetConnectionPropertyUi2))
    {
        *ppvObj = This;
        IUnknown_AddRef(iface);
        return S_OK;
    }
    else if (IsEqualIID(iid, &IID_INetLanConnectionUiInfo))
    {
        *ppvObj = &This->lpLanConUiInfoVtbl;
        IUnknown_AddRef(iface);
        return S_OK;
    }
    //StringFromCLSID(iid, &pStr);
    //MessageBoxW(NULL, pStr, L"INetConnectionPropertyUi_fnQueryInterface", MB_OK);
    //CoTaskMemFree(pStr);
    return E_NOINTERFACE;
}

static
ULONG
WINAPI
INetConnectionPropertyUi2_fnAddRef(
    INetConnectionPropertyUi2 * iface)
{
    INetConnectionPropertyUiImpl * This =  (INetConnectionPropertyUiImpl*)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    return refCount;
}

static
ULONG
WINAPI
INetConnectionPropertyUi2_fnRelease(
    INetConnectionPropertyUi2 * iface)
{
    INetConnectionPropertyUiImpl * This =  (INetConnectionPropertyUiImpl*)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    if (!refCount) 
    {
        if (This->pNCfg)
        {
            INetCfg_Uninitialize(This->pNCfg);
            INetCfg_Release(This->pNCfg);
        }
        if (This->NCfgLock)
        {
            INetCfgLock_Release(This->NCfgLock);
        }
        if (This->pProperties)
        {
            NcFreeNetconProperties(This->pProperties);
        }
        CoTaskMemFree (This);
    }
    return refCount;
}

static
HRESULT
WINAPI
INetConnectionPropertyUi2_fnSetConnection(
    INetConnectionPropertyUi2 * iface,
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
INetConnectionPropertyUi2_fnAddPages(
    INetConnectionPropertyUi2 * iface,
    HWND hwndParent, 
    LPFNADDPROPSHEETPAGE pfnAddPage,
    LPARAM lParam)
{
    HPROPSHEETPAGE hProp;
    BOOL ret;
    HRESULT hr = E_FAIL;
    INITCOMMONCONTROLSEX initEx;
    INetConnectionPropertyUiImpl * This =  (INetConnectionPropertyUiImpl*)iface;


    initEx.dwSize = sizeof(initEx);
    initEx.dwICC = ICC_LISTVIEW_CLASSES;
    if(!InitCommonControlsEx(&initEx))
        return E_FAIL;

    hr = INetConnection_GetProperties(This->pCon, &This->pProperties);
    if (FAILED(hr))
        return hr;

	hProp = InitializePropertySheetPage(MAKEINTRESOURCEW(IDD_NETPROPERTIES), LANPropertiesUIDlg, (LPARAM)This, This->pProperties->pszwName);
    if (hProp)
    {
        ret = (*pfnAddPage)(hProp, lParam);
        if (ret)
        {
            hr = NOERROR;
        }
        else
        {
            DestroyPropertySheetPage(hProp);
        }
    }
    return hr;
}

static
HRESULT
WINAPI
INetConnectionPropertyUi2_fnGetIcon(
    INetConnectionPropertyUi2 * iface,
    DWORD dwSize,
    HICON *phIcon)
{
    return E_NOTIMPL;
}

static const INetConnectionPropertyUi2Vtbl vt_NetConnectionPropertyUi =
{
    INetConnectionPropertyUi2_fnQueryInterface,
    INetConnectionPropertyUi2_fnAddRef,
    INetConnectionPropertyUi2_fnRelease,
    INetConnectionPropertyUi2_fnSetConnection,
    INetConnectionPropertyUi2_fnAddPages,
    INetConnectionPropertyUi2_fnGetIcon,
};

static
HRESULT
STDCALL
INetLanConnectionUiInfo_fnQueryInterface(
    INetLanConnectionUiInfo * iface,
    REFIID iid,
    LPVOID * ppvObj)
{
    INetConnectionPropertyUiImpl * This =  impl_from_NetLanConnectionUiInfo(iface);
    return INetConnectionPropertyUi_QueryInterface((INetConnectionPropertyUi*)This, iid, ppvObj);
}

static
ULONG
STDCALL
INetLanConnectionUiInfo_fnAddRef(
    INetLanConnectionUiInfo * iface)
{
    INetConnectionPropertyUiImpl * This =  impl_from_NetLanConnectionUiInfo(iface);
    return INetConnectionPropertyUi_AddRef((INetConnectionPropertyUi*)This);
}

static
ULONG
STDCALL
INetLanConnectionUiInfo_fnRelease(
    INetLanConnectionUiInfo * iface)
{
    INetConnectionPropertyUiImpl * This =  impl_from_NetLanConnectionUiInfo(iface);
    return INetConnectionPropertyUi_Release((INetConnectionPropertyUi*)This);
}

static
HRESULT
STDCALL
INetLanConnectionUiInfo_fnGetDeviceGuid(
    INetLanConnectionUiInfo * iface,
    GUID * pGuid)
{
    INetConnectionPropertyUiImpl * This =  impl_from_NetLanConnectionUiInfo(iface);
    CopyMemory(pGuid, &This->pProperties->guidId, sizeof(GUID));
    return S_OK;
}

static const INetLanConnectionUiInfoVtbl vt_NetLanConnectionUiInfo =
{
    INetLanConnectionUiInfo_fnQueryInterface,
    INetLanConnectionUiInfo_fnAddRef,
    INetLanConnectionUiInfo_fnRelease,
    INetLanConnectionUiInfo_fnGetDeviceGuid,
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
    This->NCfgLock = NULL;
    This->pProperties = NULL;
    This->lpVtbl = (INetConnectionPropertyUi2*)&vt_NetConnectionPropertyUi;
    This->lpLanConUiInfoVtbl = (INetLanConnectionUiInfo*)&vt_NetLanConnectionUiInfo;

    if (!SUCCEEDED (INetConnectionPropertyUi2_fnQueryInterface ((INetConnectionPropertyUi2*)This, riid, ppv)))
    {
        IUnknown_Release((IUnknown*)This);
        return E_NOINTERFACE;
    }

    IUnknown_Release((IUnknown*)This);
    return S_OK;
}
