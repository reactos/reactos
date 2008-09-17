#include <precomp.h>

/// CLSID
/// HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID\{7007ACCF-3202-11D1-AAD2-00805FC1270E}
// IID B722BCCB-4E68-101B-A2BC-00AA00404770

#define WM_SHOWSTATUSDLG    (WM_USER+10)
typedef struct
{
    IOleCommandTarget * lpVtbl;
    INetConnectionManager * lpNetMan;
    LONG ref;
}ILanStatusImpl, *LPILanStatusImpl;

typedef struct
{
    INetConnection *pNet;
    HWND hwndDlg;
}LANSTATUSUI_CONTEXT;

VOID
UpdateLanStatusUIDlg(HWND hwndDlg, MIB_IFROW * IfEntry)
{
    WCHAR szFormat[MAX_PATH] = {0};
    WCHAR szBuffer[MAX_PATH] = {0};

    if (IfEntry->dwSpeed < 1000)
    {
        if (LoadStringW(netshell_hInstance, IDS_FORMAT_BIT, szFormat, sizeof(szFormat)/sizeof(WCHAR)))
        {
            swprintf(szBuffer, szFormat, IfEntry->dwSpeed);
            SendDlgItemMessageW(hwndDlg, IDC_SPEED, WM_SETTEXT, 0, (LPARAM)szBuffer);
        }
    }
    else if (IfEntry->dwSpeed < 1000000)
    {
        if (LoadStringW(netshell_hInstance, IDS_FORMAT_KBIT, szFormat, sizeof(szFormat)/sizeof(WCHAR)))
        {
            swprintf(szBuffer, szFormat, IfEntry->dwSpeed/1000);
            SendDlgItemMessageW(hwndDlg, IDC_SPEED, WM_SETTEXT, 0, (LPARAM)szBuffer);
        }
    }
    else if (IfEntry->dwSpeed < 1000000000)
    {
        if (LoadStringW(netshell_hInstance, IDS_FORMAT_MBIT, szFormat, sizeof(szFormat)/sizeof(WCHAR)))
        {
            swprintf(szBuffer, szFormat, IfEntry->dwSpeed/1000000);
            SendDlgItemMessageW(hwndDlg, IDC_SPEED, WM_SETTEXT, 0, (LPARAM)szBuffer);
        }
    }
    else
    {
        if (LoadStringW(netshell_hInstance, IDS_FORMAT_KBIT, szFormat, sizeof(szFormat)/sizeof(WCHAR)))
        {
            swprintf(szBuffer, szFormat, IfEntry->dwSpeed/1000000000);
            SendDlgItemMessageW(hwndDlg, IDC_SPEED, WM_SETTEXT, 0, (LPARAM)szBuffer);
        }
    }

    if (StrFormatByteSizeW(IfEntry->dwInOctets, szBuffer, sizeof(szFormat)/sizeof(WCHAR)))
    {
        SendDlgItemMessageW(hwndDlg, IDC_RECEIVED, WM_SETTEXT, 0, (LPARAM)szBuffer);
    }

    if (StrFormatByteSizeW(IfEntry->dwOutOctets, szBuffer, sizeof(szFormat)/sizeof(WCHAR)))
    {
        SendDlgItemMessageW(hwndDlg, IDC_SEND, WM_SETTEXT, 0, (LPARAM)szBuffer);
    }

    //FIXME
    //set duration

}

VOID
InitializeLANStatusUiDlg(HWND hwndDlg, INetConnection * pNet)
{
    WCHAR szBuffer[MAX_PATH] = {0};
    NETCON_PROPERTIES * pProperties = NULL;
    MIB_IFROW IfEntry;
    DWORD dwSize, dwAdapterIndex, dwResult;
    LPOLESTR pStr;
    IP_ADAPTER_INFO * pAdapterInfo;

    if (INetConnection_GetProperties(pNet, &pProperties) != NOERROR)
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

    /* get detailed adapter info */
    ZeroMemory(&IfEntry, sizeof(IfEntry));
    IfEntry.dwIndex = dwAdapterIndex;
    if(GetIfEntry(&IfEntry) != NO_ERROR)
    {
        CoTaskMemFree(pAdapterInfo);
        return;
    }

    UpdateLanStatusUIDlg(hwndDlg, &IfEntry);

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
            InitializeLANStatusUiDlg(hwndDlg, pContext->pNet);
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pContext);
            return TRUE;
        case WM_COMMAND:
            pContext = (LANSTATUSUI_CONTEXT*)GetWindowLongPtr(hwndDlg, DWLP_USER);
            if (LOWORD(wParam) == IDC_STATUS_PROPERTIES)
            {
                //FIXME
                // show net connection property dialog
                //
                if (pContext)
                    ShowNetConnectionProperties(pContext->pNet, pContext->hwndDlg); 
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
                DestroyWindow(pContext->hwndDlg);
                pContext->hwndDlg = NULL;
                return PSNRET_NOERROR;
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
    HWND hwnd;

    ZeroMemory(&pinfo, sizeof(PROPSHEETHEADERW));
    ZeroMemory(hppages, sizeof(hppages));
    pinfo.dwSize = sizeof(PROPSHEETHEADERW);
    pinfo.dwFlags = PSH_NOCONTEXTHELP | PSH_PROPTITLE | PSH_NOAPPLYNOW | PSH_MODELESS;
    pinfo.u3.phpage = hppages;
    pinfo.hwndParent = hwndDlg;

    if (INetConnection_GetProperties(pContext->pNet, &pProperties) == NOERROR)
    {
        if (pProperties->pszwName)
        {
            pinfo.pszCaption = pProperties->pszwName;
            pinfo.dwFlags |= PSH_PROPTITLE;
        }

        if (pProperties->MediaType == NCM_LAN)
        {
            hppages[0] = InitializePropertySheetPage(MAKEINTRESOURCEW(IDD_LAN_NETSTATUS), LANStatusUiDlg, (LPARAM)pContext, NULL);
            if (hppages[0])
               pinfo.nPages++;

            if (pinfo.nPages)
            {
                hwnd = (HWND)PropertySheetW(&pinfo);
                if (hwnd)
                {
                    pContext->hwndDlg = hwnd;
                }
            }
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
            pContext = (LANSTATUSUI_CONTEXT*)CoTaskMemAlloc(sizeof(LANSTATUSUI_CONTEXT));
            if (!pContext)
                return FALSE;
            pContext->hwndDlg = NULL;
            pContext->pNet = (INetConnection*)lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pContext);
            return TRUE;
        case WM_SHOWSTATUSDLG:
            if (LOWORD(lParam) == WM_LBUTTONDOWN)
            {
                pContext = (LANSTATUSUI_CONTEXT*)GetWindowLongPtr(hwndDlg, DWLP_USER);
                if (!pContext)
                    break;

                if (pContext->hwndDlg)
                {
                    ShowWindow(pContext->hwndDlg, SW_SHOW);
                    BringWindowToTop(pContext->hwndDlg);
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
InitializeNetConnectTray(
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

    /* get an instance to of IConnectionManager */
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
            hwndDlg = CreateDialogParamW(netshell_hInstance, MAKEINTRESOURCEW(IDD_STATUS), NULL, LANStatusDlg, (LPARAM)INetCon);
            if (hwndDlg)
            {
                ZeroMemory(&nid, sizeof(nid));
                nid.cbSize = sizeof(nid);
                nid.uID = Index++;
                nid.uFlags = NIF_ICON | NIF_MESSAGE;
                nid.u.uVersion = 3;
                nid.uCallbackMessage = WM_SHOWSTATUSDLG;
                nid.hWnd = hwndDlg;
                nid.hIcon = LoadIcon(netshell_hInstance, MAKEINTRESOURCE(IDI_SHELL_NETWORK_FOLDER)); //FIXME
                hr = INetConnection_GetProperties(INetCon, &pProps);
                if (SUCCEEDED(hr))
                {
                    if (!(pProps->dwCharacter & NCCF_SHOW_ICON))
                    {
                        nid.dwState = NIS_HIDDEN;
                    }
                }
                if (Shell_NotifyIconW(NIM_ADD, &nid))
                    Index++;
            }
        }
    }while(hr == S_OK);

    This->lpNetMan = INetConMan;
    IEnumNetConnection_Release(IEnumCon);
    return S_OK;
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
    ILanStatusImpl * This =  (ILanStatusImpl*)iface;
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
IOleCommandTarget_fnQueryStatus(
    IOleCommandTarget * iface,
    const GUID *pguidCmdGroup,
    ULONG cCmds,
    OLECMD *prgCmds,
    OLECMDTEXT *pCmdText)
{
    MessageBoxW(NULL, L"222222222222222222222", L"IOleCommandTarget_fnQueryStatus", MB_OK);
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
            return InitializeNetConnectTray(This);
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

    if (FAILED(IOleCommandTarget_fnQueryInterface ((IOleCommandTarget*)This, riid, ppv)))
    {
        IOleCommandTarget_Release((IUnknown*)This);
        return E_NOINTERFACE;
    }
    IOleCommandTarget_Release((IUnknown*)This);
    return S_OK;
}



