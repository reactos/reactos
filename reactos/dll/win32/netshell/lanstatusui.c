#include <precomp.h>

/// CLSID
/// HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID\{7007ACCF-3202-11D1-AAD2-00805FC1270E}
// IID B722BCCB-4E68-101B-A2BC-00AA00404770

#define WM_SHOWSTATUSDLG    (WM_USER+10)

typedef struct tagNotificationItem
{
    struct tagNotificationItem *pNext;
    CLSID guidItem;
    UINT uID;
    HWND hwndDlg;
}NOTIFICATION_ITEM;

typedef struct
{
    IOleCommandTarget * lpVtbl;
    INetConnectionManager * lpNetMan;
    LONG ref;
    NOTIFICATION_ITEM * pHead;
}ILanStatusImpl, *LPILanStatusImpl;

typedef struct
{
    INetConnection *pNet;
    HWND hwndStatusDlg;         /* LanStatusDlg window */
    HWND hwndDlg;               /* status dialog window */
    DWORD dwAdapterIndex;
    UINT_PTR nIDEvent;
    DWORD dwInOctets;
    DWORD dwOutOctets;
    UINT uID;
    UINT Status;
}LANSTATUSUI_CONTEXT;

VOID
UpdateLanStatusUIDlg(HWND hwndDlg,  LANSTATUSUI_CONTEXT * pContext)
{
    WCHAR szFormat[MAX_PATH] = {0};
    WCHAR szBuffer[MAX_PATH] = {0};
    MIB_IFROW IfEntry;
    SYSTEMTIME TimeConnected;
    DWORD DurationSeconds;
    WCHAR Buffer[100];
    WCHAR DayBuffer[30];
    WCHAR LocBuffer[50];
    HICON hIcon, hOldIcon;
    NOTIFYICONDATAW nid;

#if 0
    ULONGLONG Ticks;
#else
    DWORD Ticks;
#endif

    ZeroMemory(&IfEntry, sizeof(IfEntry));
    IfEntry.dwIndex = pContext->dwAdapterIndex;
    if(GetIfEntry(&IfEntry) != NO_ERROR)
    {
        return;
    }

    if (IfEntry.dwSpeed < 1000)
    {
        if (LoadStringW(netshell_hInstance, IDS_FORMAT_BIT, szFormat, sizeof(szFormat)/sizeof(WCHAR)))
        {
            swprintf(szBuffer, szFormat, IfEntry.dwSpeed);
            SendDlgItemMessageW(hwndDlg, IDC_SPEED, WM_SETTEXT, 0, (LPARAM)szBuffer);
        }
    }
    else if (IfEntry.dwSpeed < 1000000)
    {
        if (LoadStringW(netshell_hInstance, IDS_FORMAT_KBIT, szFormat, sizeof(szFormat)/sizeof(WCHAR)))
        {
            swprintf(szBuffer, szFormat, IfEntry.dwSpeed/1000);
            SendDlgItemMessageW(hwndDlg, IDC_SPEED, WM_SETTEXT, 0, (LPARAM)szBuffer);
        }
    }
    else if (IfEntry.dwSpeed < 1000000000)
    {
        if (LoadStringW(netshell_hInstance, IDS_FORMAT_MBIT, szFormat, sizeof(szFormat)/sizeof(WCHAR)))
        {
            swprintf(szBuffer, szFormat, IfEntry.dwSpeed/1000000);
            SendDlgItemMessageW(hwndDlg, IDC_SPEED, WM_SETTEXT, 0, (LPARAM)szBuffer);
        }
    }
    else
    {
        if (LoadStringW(netshell_hInstance, IDS_FORMAT_KBIT, szFormat, sizeof(szFormat)/sizeof(WCHAR)))
        {
            swprintf(szBuffer, szFormat, IfEntry.dwSpeed/1000000000);
            SendDlgItemMessageW(hwndDlg, IDC_SPEED, WM_SETTEXT, 0, (LPARAM)szBuffer);
        }
    }

    if (StrFormatByteSizeW(IfEntry.dwInOctets, szBuffer, sizeof(szFormat)/sizeof(WCHAR)))
    {
        SendDlgItemMessageW(hwndDlg, IDC_RECEIVED, WM_SETTEXT, 0, (LPARAM)szBuffer);
    }

    if (StrFormatByteSizeW(IfEntry.dwOutOctets, szBuffer, sizeof(szFormat)/sizeof(WCHAR)))
    {
        SendDlgItemMessageW(hwndDlg, IDC_SEND, WM_SETTEXT, 0, (LPARAM)szBuffer);
    }

#if 0
    Ticks = GetTickCount64();
#else
    Ticks = GetTickCount();
#endif

    DurationSeconds = Ticks / 1000;
    TimeConnected.wSecond = (DurationSeconds % 60);
    TimeConnected.wMinute = (DurationSeconds / 60) % 60;
    TimeConnected.wHour = (DurationSeconds / (60 * 60)) % 24;
    TimeConnected.wDay = DurationSeconds / (60 * 60 * 24);

    if (!GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &TimeConnected, L"HH':'mm':'ss", LocBuffer, sizeof(LocBuffer) / sizeof(LocBuffer[0])))
        return;

    if (!TimeConnected.wDay)
    {
        SendDlgItemMessageW(hwndDlg, IDC_DURATION, WM_SETTEXT, 0, (LPARAM)LocBuffer);
    }
    else
    {
        if (TimeConnected.wDay == 1)
        {
            if (!LoadStringW(netshell_hInstance, IDS_DURATION_DAY, DayBuffer, sizeof(DayBuffer) / sizeof(DayBuffer[0])))
                DayBuffer[0] = L'\0';
        }
        else
        {
            if (!LoadStringW(netshell_hInstance, IDS_DURATION_DAYS, DayBuffer, sizeof(DayBuffer) / sizeof(DayBuffer[0])))
                DayBuffer[0] = L'\0';
        }
        swprintf(Buffer, DayBuffer, TimeConnected.wDay, LocBuffer);
        SendDlgItemMessageW(hwndDlg, IDC_DURATION, WM_SETTEXT, 0, (LPARAM)Buffer);
    }

    hIcon = NULL;
    if (IfEntry.dwOperStatus == MIB_IF_OPER_STATUS_CONNECTED || IfEntry.dwOperStatus == MIB_IF_OPER_STATUS_OPERATIONAL)
    {
        if (pContext->dwInOctets == IfEntry.dwInOctets && pContext->dwOutOctets == IfEntry.dwOutOctets && pContext->Status  != 0)
        {
            hIcon = LoadImage(netshell_hInstance, MAKEINTRESOURCE(IDI_NET_IDLE), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
            pContext->Status = 0;
        }
        else if (pContext->dwInOctets != IfEntry.dwInOctets && pContext->dwOutOctets != IfEntry.dwOutOctets && pContext->Status  != 1)
        {
            pContext->Status = 1;
            hIcon = LoadImage(netshell_hInstance, MAKEINTRESOURCE(IDI_NET_TRANSREC), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
        }
        else if (pContext->dwInOctets != IfEntry.dwInOctets && pContext->Status  != 2)
        {
            hIcon = LoadImage(netshell_hInstance, MAKEINTRESOURCE(IDI_NET_REC), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
            pContext->Status = 2; 
        }
        else if (pContext->dwOutOctets != IfEntry.dwOutOctets && pContext->Status  != 3)
        {
            hIcon = LoadImage(netshell_hInstance, MAKEINTRESOURCE(IDI_NET_TRANS), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
            pContext->Status = 3;
        }
    }
    else if (IfEntry.dwOperStatus == MIB_IF_OPER_STATUS_UNREACHABLE || MIB_IF_OPER_STATUS_DISCONNECTED)
    {
        if (pContext->Status != 4)
        {
            hIcon = LoadImage(netshell_hInstance, MAKEINTRESOURCE(IDI_NET_OFF), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
            pContext->Status = 4;
        }
    }
    else if (MIB_IF_OPER_STATUS_NON_OPERATIONAL)
    {
        if (pContext->Status != 5)
        {
            hIcon = LoadImage(netshell_hInstance, MAKEINTRESOURCE(IDI_NET_OFF), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
            pContext->Status = 5;
        }
    }

    if (hIcon)
    {
        hOldIcon = (HICON)SendDlgItemMessageW(hwndDlg, IDC_NETSTAT, STM_SETICON, (WPARAM)hIcon, 0);

        ZeroMemory(&nid, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.uID = pContext->uID;
        nid.hWnd = pContext->hwndStatusDlg;
        nid.uFlags = NIF_ICON;
        nid.u.uVersion = 3;
        nid.hIcon = CopyImage(hIcon, IMAGE_ICON, 0, 0, 0);

        Shell_NotifyIconW(NIM_MODIFY, &nid);
        if (hOldIcon)
            DestroyIcon(hOldIcon);
    }
    pContext->dwInOctets = IfEntry.dwInOctets;
    pContext->dwOutOctets = IfEntry.dwOutOctets;
}


VOID
InitializeLANStatusUiDlg(HWND hwndDlg, LANSTATUSUI_CONTEXT * pContext)
{
    WCHAR szBuffer[MAX_PATH] = {0};
    NETCON_PROPERTIES * pProperties = NULL;
    DWORD dwSize, dwAdapterIndex, dwResult;
    LPOLESTR pStr;
    IP_ADAPTER_INFO * pAdapterInfo;

    if (INetConnection_GetProperties(pContext->pNet, &pProperties) != NOERROR)
        return;

    if (pProperties->Status == NCS_DISCONNECTED)
        LoadStringW(netshell_hInstance, IDS_STATUS_UNREACHABLE, szBuffer, MAX_PATH);
    else if (pProperties->Status == NCS_MEDIA_DISCONNECTED)
        LoadStringW(netshell_hInstance, IDS_STATUS_DISCONNECTED, szBuffer, MAX_PATH);
    else if (pProperties->Status == NCS_CONNECTING)
        LoadStringW(netshell_hInstance, IDS_STATUS_CONNECTING, szBuffer, MAX_PATH);
    else if (pProperties->Status == NCS_CONNECTED)
         LoadStringW(netshell_hInstance, IDS_STATUS_CONNECTED, szBuffer, MAX_PATH);

    SendDlgItemMessageW(hwndDlg, IDC_STATUS, WM_SETTEXT, 0, (LPARAM)szBuffer);

    if (FAILED(StringFromCLSID(&pProperties->guidId, &pStr)))
    {
        NcFreeNetconProperties(pProperties);
        return;
    }
    NcFreeNetconProperties(pProperties);

    /* get the IfTable */
    dwSize = 0;
    dwResult = GetAdaptersInfo(NULL, &dwSize); 
    if (dwResult!= ERROR_BUFFER_OVERFLOW)
    {
        CoTaskMemFree(pStr);
        return;
    }

    pAdapterInfo = (PIP_ADAPTER_INFO)CoTaskMemAlloc(dwSize);
    if (!pAdapterInfo)
    {
        CoTaskMemFree(pAdapterInfo);
        CoTaskMemFree(pStr);
        return;
    }

    if (GetAdaptersInfo(pAdapterInfo, &dwSize) != NO_ERROR)
    {
        CoTaskMemFree(pAdapterInfo);
        CoTaskMemFree(pStr);
        return;
    }

    if (!GetAdapterIndexFromNetCfgInstanceId(pAdapterInfo, pStr, &dwAdapterIndex))
    {
        CoTaskMemFree(pAdapterInfo);
        CoTaskMemFree(pStr);
        return;
    }
    CoTaskMemFree(pStr);
    pContext->dwAdapterIndex = dwAdapterIndex;
    pContext->dwInOctets = 0;
    pContext->dwOutOctets = 0;

    /* update adapter info */
    pContext->Status = -1;
    UpdateLanStatusUIDlg(hwndDlg, pContext);
    CoTaskMemFree(pAdapterInfo);
}

INT_PTR
CALLBACK
LANStatusUiDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    PROPSHEETPAGE *page;
    LANSTATUSUI_CONTEXT * pContext;
    LPPSHNOTIFY lppsn;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            page = (PROPSHEETPAGE*)lParam;
            pContext = (LANSTATUSUI_CONTEXT*)page->lParam;
            pContext->hwndDlg = hwndDlg;
            InitializeLANStatusUiDlg(hwndDlg, pContext);
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pContext);
            return TRUE;
        case WM_COMMAND:
            pContext = (LANSTATUSUI_CONTEXT*)GetWindowLongPtr(hwndDlg, DWLP_USER);
            if (LOWORD(wParam) == IDC_STATUS_PROPERTIES)
            {
                if (pContext)
                {
                    ShowNetConnectionProperties(pContext->pNet, GetParent(pContext->hwndDlg)); 
                    BringWindowToTop(GetParent(pContext->hwndDlg));
                }
                break;
            }
            else if (LOWORD(wParam) == IDC_ENDISABLE)
            {
                //FIXME
                // disable network adapter
                break;
            }
        case WM_NOTIFY:
            lppsn = (LPPSHNOTIFY) lParam;
            if (lppsn->hdr.code == PSN_APPLY || lppsn->hdr.code == PSN_RESET)
            {
                pContext = (LANSTATUSUI_CONTEXT*)GetWindowLongPtr(hwndDlg, DWLP_USER);
                SetWindowLong(hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR);
                pContext->hwndDlg = NULL;
                return TRUE;
            }
            break;
    }
    return FALSE;
}

VOID
ShowStatusPropertyDialog(
    LANSTATUSUI_CONTEXT * pContext,
    HWND hwndDlg)
{
    HPROPSHEETPAGE hppages[2];
    PROPSHEETHEADERW pinfo;
    NETCON_PROPERTIES * pProperties = NULL;

    ZeroMemory(&pinfo, sizeof(PROPSHEETHEADERW));
    ZeroMemory(hppages, sizeof(hppages));
    pinfo.dwSize = sizeof(PROPSHEETHEADERW);
    pinfo.dwFlags = PSH_NOCONTEXTHELP | PSH_PROPTITLE | PSH_NOAPPLYNOW;
    pinfo.u3.phpage = hppages;
    pinfo.hwndParent = hwndDlg;

    if (INetConnection_GetProperties(pContext->pNet, &pProperties) == NOERROR)
    {
        if (pProperties->pszwName)
        {
            pinfo.pszCaption = pProperties->pszwName;
            pinfo.dwFlags |= PSH_PROPTITLE;
        }

        if (pProperties->MediaType == NCM_LAN && pProperties->Status == NCS_CONNECTED)
        {
            hppages[0] = InitializePropertySheetPage(MAKEINTRESOURCEW(IDD_LAN_NETSTATUS), LANStatusUiDlg, (LPARAM)pContext, NULL);
            if (hppages[0])
               pinfo.nPages++;

            if (pinfo.nPages)
            {
                PropertySheetW(&pinfo);
            }
        }
        else if (pProperties->Status == NCS_MEDIA_DISCONNECTED || pProperties->Status == NCS_DISCONNECTED ||
                 pProperties->Status == NCS_HARDWARE_DISABLED)
        {
            ShowNetConnectionProperties(pContext->pNet, pContext->hwndDlg);
        }
        NcFreeNetconProperties(pProperties);
    }
}

INT_PTR
CALLBACK
LANStatusDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    LANSTATUSUI_CONTEXT * pContext;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            pContext = (LANSTATUSUI_CONTEXT *)lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)lParam);
            pContext->nIDEvent = SetTimer(hwndDlg, 0xFABC, 1000, NULL);
            return TRUE;
        case WM_TIMER:
            pContext = (LANSTATUSUI_CONTEXT*)GetWindowLongPtr(hwndDlg, DWLP_USER);
            if (wParam == (WPARAM)pContext->nIDEvent)
            {
                UpdateLanStatusUIDlg(pContext->hwndDlg, pContext);
            }
            break;
        case WM_SHOWSTATUSDLG:
            if (LOWORD(lParam) == WM_LBUTTONDOWN)
            {
                pContext = (LANSTATUSUI_CONTEXT*)GetWindowLongPtr(hwndDlg, DWLP_USER);
                if (!pContext)
                    break;

                if (pContext->hwndDlg)
                {
                    ShowWindow(GetParent(pContext->hwndDlg), SW_SHOW);
                    BringWindowToTop(GetParent(pContext->hwndDlg));
                }
                else
                {
                    ShowStatusPropertyDialog(pContext, hwndDlg);
                }
                break;
            }
            break;
    }
    return FALSE;
}
static 
HRESULT
InitializeNetTaskbarNotifications(
    ILanStatusImpl * This)
{
    NOTIFYICONDATAW nid;
    HWND hwndDlg;
    INetConnectionManager * INetConMan;
    IEnumNetConnection * IEnumCon;
    INetConnection * INetCon;
    NETCON_PROPERTIES* pProps;
    HRESULT hr;
    ULONG Count;
    ULONG Index;
    NOTIFICATION_ITEM * pItem, *pLast = NULL;
    LANSTATUSUI_CONTEXT * pContext;

    if (This->pHead)
        return S_OK;

    /* get an instance to of IConnectionManager */

    //hr = CoCreateInstance(&CLSID_ConnectionManager, NULL, CLSCTX_INPROC_SERVER, &IID_INetConnectionManager, (LPVOID*)&INetConMan);

    hr = INetConnectionManager_Constructor(NULL, &IID_INetConnectionManager, (LPVOID*)&INetConMan);
    if (FAILED(hr))
        return hr;

    hr = INetConnectionManager_EnumConnections(INetConMan, NCME_DEFAULT, &IEnumCon);
    if (FAILED(hr))
    {
        INetConnectionManager_Release(INetConMan);
        return hr;
    }

    Index = 1;
    do
    {
        hr = IEnumNetConnection_Next(IEnumCon, 1, &INetCon, &Count);
        if (hr == S_OK)
        {
            pItem = (NOTIFICATION_ITEM*)CoTaskMemAlloc(sizeof(NOTIFICATION_ITEM));
            if (!pItem)
                break;

            pContext = (LANSTATUSUI_CONTEXT*)CoTaskMemAlloc(sizeof(LANSTATUSUI_CONTEXT));
            if (!pContext)
            {
                CoTaskMemFree(pItem);
                break;
            }

            pItem->pNext = NULL;
            ZeroMemory(pContext, sizeof(LANSTATUSUI_CONTEXT));
            pContext->uID = Index;
            pContext->pNet = INetCon;

            hwndDlg = CreateDialogParamW(netshell_hInstance, MAKEINTRESOURCEW(IDD_STATUS), NULL, LANStatusDlg, (LPARAM)pContext);
            if (hwndDlg)
            {
                ZeroMemory(&nid, sizeof(nid));
                nid.cbSize = sizeof(nid);
                nid.uID = Index++;
                nid.uFlags = NIF_ICON | NIF_MESSAGE;
                nid.u.uVersion = 3;
                nid.uCallbackMessage = WM_SHOWSTATUSDLG;
                nid.hWnd = hwndDlg;

                hr = INetConnection_GetProperties(INetCon, &pProps);
                if (SUCCEEDED(hr))
                {
                    CopyMemory(&pItem->guidItem, &pProps->guidId, sizeof(GUID));
                    if (!(pProps->dwCharacter & NCCF_SHOW_ICON))
                    {
                        nid.dwState = NIS_HIDDEN;
                    }
                    if (pProps->Status == NCS_MEDIA_DISCONNECTED || pProps->Status == NCS_DISCONNECTED || pProps->Status == NCS_HARDWARE_DISABLED)
                        nid.hIcon = LoadIcon(netshell_hInstance, MAKEINTRESOURCE(IDI_NET_OFF));
                    else if (pProps->Status == NCS_CONNECTED)
                        nid.hIcon = LoadIcon(netshell_hInstance, MAKEINTRESOURCE(IDI_NET_IDLE));


                    wcscpy(nid.szTip, pProps->pszwName);
                    nid.uFlags |= NIF_TIP;
                }
                pContext->hwndStatusDlg = hwndDlg;
                pItem->hwndDlg = hwndDlg;

                if (Shell_NotifyIconW(NIM_ADD, &nid))
                {
                    if (pLast)
                        pLast->pNext = pItem;
                    else
                        This->pHead = pItem;

                    pLast = pItem;
                    Index++;
                }
                else
                {
                    CoTaskMemFree(pItem);
                }
            }
        }
    }while(hr == S_OK);

    This->lpNetMan = INetConMan;
    IEnumNetConnection_Release(IEnumCon);
    return S_OK;
}

HRESULT
ShowStatusDialogByCLSID(
    ILanStatusImpl * This,
    const GUID *pguidCmdGroup)
{
    NOTIFICATION_ITEM * pItem;

    pItem = This->pHead;
    while(pItem)
    {
        if (IsEqualGUID(&pItem->guidItem, pguidCmdGroup))
        {
            SendMessageW(pItem->hwndDlg, WM_SHOWSTATUSDLG, 0, WM_LBUTTONDOWN);
            return S_OK;
        }
        pItem = pItem->pNext;
    }
    return E_FAIL;
}
static
HRESULT
WINAPI
IOleCommandTarget_fnQueryInterface(
    IOleCommandTarget * iface,
    REFIID iid,
    LPVOID * ppvObj)
{
    ILanStatusImpl * This =  (ILanStatusImpl*)iface;
    *ppvObj = NULL;

    if (IsEqualIID (iid, &IID_IUnknown) ||
        IsEqualIID (iid, &IID_IOleCommandTarget))
    {
        *ppvObj = This;
        IUnknown_AddRef(iface);
        return S_OK;
    }
    MessageBoxW(NULL, L"IOleCommandTarget_fnQueryInterface", NULL, MB_OK);
    return E_NOINTERFACE;
}

static
ULONG
WINAPI
IOleCommandTarget_fnAddRef(
    IOleCommandTarget * iface)
{
    ILanStatusImpl * This =  (ILanStatusImpl*)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    return refCount;
}

static
ULONG
WINAPI
IOleCommandTarget_fnRelease(
    IOleCommandTarget * iface)
{
#if 0
    ILanStatusImpl * This =  (ILanStatusImpl*)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    if (!refCount) 
    {
        CoTaskMemFree (This);
    }
    return refCount;
#else
    return 1;
#endif
}

static
HRESULT
WINAPI
IOleCommandTarget_fnQueryStatus(
    IOleCommandTarget * iface,
    const GUID *pguidCmdGroup,
    ULONG cCmds,
    OLECMD *prgCmds,
    OLECMDTEXT *pCmdText)
{
    MessageBoxW(NULL, pCmdText->rgwz, L"IOleCommandTarget_fnQueryStatus", MB_OK);
    return E_NOTIMPL;
}

static
HRESULT
WINAPI
IOleCommandTarget_fnExec(
    IOleCommandTarget * iface,
    const GUID *pguidCmdGroup,
    DWORD nCmdID,
    DWORD nCmdexecopt,
    VARIANT *pvaIn,
    VARIANT *pvaOut)
{
    ILanStatusImpl * This =  (ILanStatusImpl*)iface;

    if (pguidCmdGroup)
    {
        if (IsEqualIID(pguidCmdGroup, &CGID_ShellServiceObject))
        {
            return InitializeNetTaskbarNotifications(This);
        }
        else
        {
            /* invoke status dialog */
            return ShowStatusDialogByCLSID(This, pguidCmdGroup);
        }
    }
    return S_OK;
}


static const IOleCommandTargetVtbl vt_OleCommandTarget =
{
    IOleCommandTarget_fnQueryInterface,
    IOleCommandTarget_fnAddRef,
    IOleCommandTarget_fnRelease,
    IOleCommandTarget_fnQueryStatus,
    IOleCommandTarget_fnExec,
};


HRESULT WINAPI LanConnectStatusUI_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv)
{
    ILanStatusImpl * This;
    static ILanStatusImpl *cached_This = NULL;

    if (!ppv)
        return E_POINTER;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    This = (ILanStatusImpl *) CoTaskMemAlloc(sizeof (ILanStatusImpl));
    if (!This)
        return E_OUTOFMEMORY;

    This->ref = 1;
    This->lpVtbl = (IOleCommandTarget*)&vt_OleCommandTarget;
    This->lpNetMan = NULL;
    This->pHead = NULL;

    if (InterlockedCompareExchangePointer((void *)&cached_This, This, NULL) != NULL)
    {
        CoTaskMemFree(This);
    }

    return IOleCommandTarget_fnQueryInterface ((IOleCommandTarget*)cached_This, riid, ppv);
}

