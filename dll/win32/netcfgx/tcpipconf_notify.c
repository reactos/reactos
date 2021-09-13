#include "precomp.h"

#include <winnls.h>
#include <winsock.h>
#include <iphlpapi.h>
#include <dhcpcsdk.h>
#include <dhcpcapi.h>

typedef struct
{
    DWORD EnableSecurityFilters;
    LPWSTR szTCPAllowedPorts;       // KEY: Tcpip\Parameter\{InstanceGuid}\TCPAllowedPorts
    LPWSTR szUDPAllowedPorts;       // KEY: Tcpip\Parameter\{InstanceGuid}\UDPAllowedPorts
    LPWSTR szRawIPAllowedProtocols; // KEY: Tcpip\Parameter\{InstanceGuid}\RawIPAllowedProtocols
    DWORD IPSize;
    DWORD TCPSize;
    DWORD UDPSize;
}TcpFilterSettings;

// KEY: Tcpip\Parameter\{InstanceGuid}\IpAddress | DhcpIpAddress
// KEY: Tcpip\Parameter\{InstanceGuid}\SubnetMask | DhcpSubnetMask
// KEY: Tcpip\Parameter\{InstanceGuid}\DefaultGateway | DhcpDefaultGateway
// KEY: Tcpip\Parameter\{InstanceGuid}\NameServer | DhcpNameServer
// KEY: Services\NetBT\Parameters\Interfaces\Tcpip_{INSTANCE_GUID}

typedef struct
{
    DWORD RegisterAdapterName;
    DWORD RegistrationEnabled;
    DWORD UseDomainNameDevolution;
    WCHAR szDomain[100];
    LPWSTR szSearchList;
}TcpipAdvancedDNSDlgSettings;

typedef struct tagIP_ADDR
{
    DWORD IpAddress;
    union
    {
        DWORD Subnetmask;
        USHORT Metric;
    }u;
    ULONG NTEContext;
    struct tagIP_ADDR * Next;
}IP_ADDR;

typedef enum
{
    METRIC = 1,
    SUBMASK = 2,
    IPADDR = 3
}COPY_TYPE;

typedef struct
{
    IP_ADDR * Ip;
    IP_ADDR * Ns;
    IP_ADDR * Gw;

    UINT DhcpEnabled;
    UINT AutoconfigActive;
    DWORD Index;
    TcpFilterSettings * pFilter;
    TcpipAdvancedDNSDlgSettings * pDNS;
}TcpipSettings;

typedef struct
{
    const INetCfgComponentPropertyUi * lpVtbl;
    const INetCfgComponentControl * lpVtblCompControl;
    LONG  ref;
    IUnknown * pUnknown;
    INetCfg * pNCfg;
    INetCfgComponent * pNComp;
    TcpipSettings *pCurrentConfig;
    CLSID NetCfgInstanceId;
}TcpipConfNotifyImpl, *LPTcpipConfNotifyImpl;

typedef struct
{
    BOOL bAdd;
    HWND hDlgCtrl;
    WCHAR szIP[16];
    UINT Metric;
}TcpipGwSettings;

typedef struct
{
   BOOL bAdd;
    HWND hDlgCtrl;
    WCHAR szIP[16];
    WCHAR szMask[16];
}TcpipIpSettings;

typedef struct
{
    BOOL bAdd;
    HWND hDlgCtrl;
    WCHAR szIP[16];
}TcpipDnsSettings;

typedef struct
{
    BOOL bAdd;
    HWND hDlgCtrl;
    LPWSTR Suffix;
}TcpipSuffixSettings;

typedef struct
{
    HWND hDlgCtrl;
    UINT ResId;
    UINT MaxNum;
}TcpipPortSettings;

static __inline LPTcpipConfNotifyImpl impl_from_INetCfgComponentControl(INetCfgComponentControl *iface)
{
    return (TcpipConfNotifyImpl*)((char *)iface - FIELD_OFFSET(TcpipConfNotifyImpl, lpVtblCompControl));
}

INT GetSelectedItem(HWND hDlgCtrl);
HRESULT InitializeTcpipBasicDlgCtrls(HWND hwndDlg, TcpipSettings * pCurSettings);
VOID InsertColumnToListView(HWND hDlgCtrl, UINT ResId, UINT SubItem, UINT Size);
INT_PTR StoreTcpipBasicSettings(HWND hwndDlg, TcpipConfNotifyImpl * This, BOOL bApply);
HRESULT Initialize(TcpipConfNotifyImpl * This);
UINT GetIpAddressFromStringW(WCHAR *szBuffer);

VOID
DisplayError(UINT ResTxt, UINT ResTitle, UINT Type)
{
    WCHAR szBuffer[300];
    WCHAR szTitle[100];

    if (LoadStringW(netcfgx_hInstance, ResTxt, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
        szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
    else
        szBuffer[0] = L'\0';

    if (LoadStringW(netcfgx_hInstance, ResTitle, szTitle, sizeof(szTitle)/sizeof(WCHAR)))
        szTitle[(sizeof(szTitle)/sizeof(WCHAR))-1] = L'\0';
    else
        szTitle[0] = L'\0';

    MessageBoxW(NULL, szBuffer, szTitle, Type);
}


/***************************************************************
 * TCP/IP Filter Dialog
 *
 */

INT_PTR
CALLBACK
TcpipFilterPortDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    TcpipPortSettings * pPort;
    UINT Num;
    LVFINDINFOW find;
    LVITEMW li;
    WCHAR szBuffer[100];

    switch(uMsg)
    {
        case WM_INITDIALOG:
            pPort = (TcpipPortSettings*)lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pPort);
            if (LoadStringW(netcfgx_hInstance, pPort->ResId, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
            {
                szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
                SendDlgItemMessageW(hwndDlg, IDC_PORT_DESC, WM_SETTEXT, 0, (LPARAM)szBuffer);
            }

            if (pPort->MaxNum == 65536)
                SendDlgItemMessageW(hwndDlg, IDC_PORT_VAL, EM_LIMITTEXT, 5, 0);
            else
                SendDlgItemMessageW(hwndDlg, IDC_PORT_VAL, EM_LIMITTEXT, 3, 0);

            return TRUE;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hwndDlg, FALSE);
                break;
            }
            else if (LOWORD(wParam) == IDC_OK)
            {
                pPort = (TcpipPortSettings*)GetWindowLongPtr(hwndDlg, DWLP_USER);
                Num = GetDlgItemInt(hwndDlg, IDC_PORT_VAL, NULL, TRUE);
                if (Num > pPort->MaxNum || Num == 0)
                {
                    if (pPort->MaxNum == 65536)
                        DisplayError(IDS_PORT_RANGE, IDS_TCPIP, MB_ICONWARNING);
                    else
                        DisplayError(IDS_PROT_RANGE, IDS_TCPIP, MB_ICONWARNING);

                    SetFocus(GetDlgItem(hwndDlg, IDC_PORT_VAL));
                    break;
                }
                if (GetWindowTextW(GetDlgItem(hwndDlg, IDC_PORT_VAL), szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
                {
                    szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
                    ZeroMemory(&find, sizeof(LVFINDINFOW));
                    find.flags = LVFI_STRING;
                    find.psz = szBuffer;
                    if (SendMessageW(pPort->hDlgCtrl, LVM_FINDITEMW, (WPARAM)-1, (LPARAM)&find) == -1)
                    {
                        ZeroMemory(&li, sizeof(LVITEMW));
                        li.mask = LVIF_TEXT;
                        li.iItem = ListView_GetItemCount(pPort->hDlgCtrl);
                        li.pszText = szBuffer;
                        SendMessageW(pPort->hDlgCtrl, LVM_INSERTITEMW, 0, (LPARAM)&li);
                        EndDialog(hwndDlg, TRUE);
                        break;
                    }
                    DisplayError(IDS_DUP_NUMBER, IDS_PROT_RANGE, MB_ICONWARNING);
                    SetFocus(GetDlgItem(hwndDlg, IDC_PORT_VAL));
                    break;
                }
           }
    }
    return FALSE;
}

VOID
InitFilterListBox(LPWSTR pData, HWND hwndDlg, HWND hDlgCtrl, UINT AllowButton, UINT RestrictButton, UINT AddButton, UINT DelButton)
{
    LVITEMW li;
    LPWSTR pCur;
    INT iItem;

    if (!pData || !_wtoi(pData))
    {
        SendDlgItemMessageW(hwndDlg, AllowButton, BM_SETCHECK, BST_CHECKED, 0);
        EnableWindow(GetDlgItem(hwndDlg, AddButton), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, DelButton), FALSE);
        return;
    }

    pCur = pData;
    memset(&li, 0x0, sizeof(LVITEMW));
    li.mask = LVIF_TEXT;
    iItem = 0;

    while(pCur[0])
    {
        li.pszText = pCur;
        li.iItem = iItem;
        SendMessageW(hDlgCtrl, LVM_INSERTITEMW, 0, (LPARAM)&li);
        iItem++;
        pCur += wcslen(pCur) + 1;
    }

    if (!iItem)
        SendDlgItemMessageW(hwndDlg, AllowButton, BM_SETCHECK, BST_CHECKED, 0);
    else
        SendDlgItemMessageW(hwndDlg, RestrictButton, BM_SETCHECK, BST_CHECKED, 0);
}

LPWSTR
CreateFilterList(
    HWND hDlgCtrl,
    LPDWORD Size)
{
    INT iCount, iIndex;
    LVITEMW li;
    LPWSTR pData, pCur;
    DWORD dwSize;
    WCHAR szBuffer[10];

    iCount = ListView_GetItemCount(hDlgCtrl);
    if (!iCount)
    {
        pData = (LPWSTR)CoTaskMemAlloc(3 * sizeof(WCHAR));
        if (!pData)
            return NULL;
        pData[0] = L'0';
        pData[1] = L'\0';
        pData[2] = L'\0';
        *Size = 3 * sizeof(WCHAR);
        return pData;
    }

    pData = CoTaskMemAlloc((6 * iCount + 1) * sizeof(WCHAR));
    if (!pData)
        return NULL;

    pCur = pData;
    dwSize = 0;
    for(iIndex = 0; iIndex < iCount; iIndex++)
    {
        ZeroMemory(&li, sizeof(LVITEMW));
        li.mask = LVIF_TEXT;
        li.iItem = iIndex;
        li.pszText = szBuffer;
        li.cchTextMax = sizeof(szBuffer) /sizeof(WCHAR);
        if (SendMessageW(hDlgCtrl, LVM_GETITEMW, 0, (LPARAM)&li))
        {
            wcscpy(pCur, szBuffer);
            dwSize += wcslen(szBuffer) + 1;
            pCur += wcslen(szBuffer) + 1;
        }
    }
    pCur[0] = L'\0';
    *Size = (dwSize+1) * sizeof(WCHAR);
    return pData;
}

TcpFilterSettings *
StoreTcpipFilterSettings(
    HWND hwndDlg)
{
    TcpFilterSettings * pFilter;

    pFilter = CoTaskMemAlloc(sizeof(TcpFilterSettings));
    if (!pFilter)
        return NULL;

    if (SendDlgItemMessageW(hwndDlg, IDC_USE_FILTER, BM_GETCHECK, 0, 0) == BST_CHECKED)
        pFilter->EnableSecurityFilters = TRUE;
    else
        pFilter->EnableSecurityFilters = FALSE;

    pFilter->szTCPAllowedPorts = CreateFilterList(GetDlgItem(hwndDlg, IDC_TCP_LIST), &pFilter->TCPSize);
    pFilter->szUDPAllowedPorts = CreateFilterList(GetDlgItem(hwndDlg, IDC_UDP_LIST), &pFilter->UDPSize);
    pFilter->szRawIPAllowedProtocols = CreateFilterList(GetDlgItem(hwndDlg, IDC_IP_LIST), &pFilter->IPSize);

    return pFilter;
}

static
VOID
AddItem(
    HWND hwndDlg,
    HWND hDlgCtrl,
    UINT MaxItem,
    UINT ResId)
{
    TcpipPortSettings Port;

    Port.MaxNum = MaxItem;
    Port.hDlgCtrl = hDlgCtrl;
    Port.ResId = ResId;

    DialogBoxParamW(netcfgx_hInstance, MAKEINTRESOURCEW(IDD_TCPIP_PORT_DLG), hwndDlg, TcpipFilterPortDlg, (LPARAM)&Port);
}

static
VOID
DelItem(
    HWND hDlgCtrl)
{
    INT iIndex = GetSelectedItem(hDlgCtrl);

    if (iIndex != -1)
    {
        (void)ListView_DeleteItem(hDlgCtrl, iIndex);
        return;
    }
    DisplayError(IDS_NOITEMSEL, IDS_TCPIP, MB_ICONWARNING);
}

INT_PTR
CALLBACK
TcpipFilterSettingsDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    TcpipConfNotifyImpl *pContext;
    TcpFilterSettings *pFilter;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            pContext = (TcpipConfNotifyImpl*)lParam;
            InsertColumnToListView(GetDlgItem(hwndDlg, IDC_TCP_LIST), IDS_TCP_PORTS, 0, 100);
            InsertColumnToListView(GetDlgItem(hwndDlg, IDC_UDP_LIST), IDS_UDP_PORTS, 0, 100);
            InsertColumnToListView(GetDlgItem(hwndDlg, IDC_IP_LIST), IDS_IP_PROTO, 0, 100);
            if (pContext->pCurrentConfig->pFilter)
            {
                InitFilterListBox(pContext->pCurrentConfig->pFilter->szTCPAllowedPorts, hwndDlg, GetDlgItem(hwndDlg, IDC_TCP_LIST), IDC_TCP_ALLOW_ALL, IDC_TCP_RESTRICT, IDC_TCP_ADD, IDC_TCP_DEL);
                InitFilterListBox(pContext->pCurrentConfig->pFilter->szUDPAllowedPorts, hwndDlg, GetDlgItem(hwndDlg, IDC_UDP_LIST), IDC_UDP_ALLOW_ALL, IDC_UDP_RESTRICT, IDC_UDP_ADD, IDC_UDP_DEL);
                InitFilterListBox(pContext->pCurrentConfig->pFilter->szRawIPAllowedProtocols, hwndDlg, GetDlgItem(hwndDlg, IDC_IP_LIST), IDC_IP_ALLOW_ALL, IDC_IP_RESTRICT, IDC_IP_ADD, IDC_IP_DEL);
                if (pContext->pCurrentConfig->pFilter->EnableSecurityFilters)
                    SendDlgItemMessageW(hwndDlg, IDC_USE_FILTER, BM_SETCHECK, BST_CHECKED, 0);
             }
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pContext);
            return TRUE;
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                switch (LOWORD(wParam))
                {
                    case IDC_TCP_ALLOW_ALL:
                        if (SendDlgItemMessageW(hwndDlg, IDC_TCP_ALLOW_ALL, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        {
                            SendDlgItemMessageW(hwndDlg, IDC_TCP_RESTRICT, BM_SETCHECK, BST_UNCHECKED, 0);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_TCP_LIST), FALSE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_TCP_ADD), FALSE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_TCP_DEL), FALSE);
                        }
                        break;
                    case IDC_TCP_RESTRICT:
                        if (SendDlgItemMessageW(hwndDlg, IDC_TCP_RESTRICT, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        {
                            SendDlgItemMessageW(hwndDlg, IDC_TCP_ALLOW_ALL, BM_SETCHECK, BST_UNCHECKED, 0);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_TCP_LIST), TRUE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_TCP_ADD), TRUE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_TCP_DEL), TRUE);
                        }
                        break;
                    case IDC_UDP_ALLOW_ALL:
                        if (SendDlgItemMessageW(hwndDlg, IDC_UDP_ALLOW_ALL, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        {
                            SendDlgItemMessageW(hwndDlg, IDC_UDP_RESTRICT, BM_SETCHECK, BST_UNCHECKED, 0);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_UDP_LIST), FALSE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_UDP_ADD), FALSE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_UDP_DEL), FALSE);
                        }
                        break;
                    case IDC_UDP_RESTRICT:
                        if (SendDlgItemMessageW(hwndDlg, IDC_UDP_RESTRICT, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        {
                            SendDlgItemMessageW(hwndDlg, IDC_UDP_ALLOW_ALL, BM_SETCHECK, BST_UNCHECKED, 0);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_UDP_LIST), TRUE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_UDP_ADD), TRUE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_UDP_DEL), TRUE);
                        }
                        break;
                    case IDC_IP_ALLOW_ALL:
                        if (SendDlgItemMessageW(hwndDlg, IDC_IP_ALLOW_ALL, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        {
                            SendDlgItemMessageW(hwndDlg, IDC_IP_RESTRICT, BM_SETCHECK, BST_UNCHECKED, 0);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_IP_LIST), FALSE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_IP_ADD), FALSE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_IP_DEL), FALSE);
                        }
                        break;
                    case IDC_IP_RESTRICT:
                        if (SendDlgItemMessageW(hwndDlg, IDC_IP_RESTRICT, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        {
                            SendDlgItemMessageW(hwndDlg, IDC_IP_ALLOW_ALL, BM_SETCHECK, BST_UNCHECKED, 0);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_IP_LIST), TRUE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_IP_ADD), TRUE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_IP_DEL), TRUE);
                        }
                        break;
                    case IDC_USE_FILTER:
                        if (SendDlgItemMessageW(hwndDlg, IDC_USE_FILTER, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
                            DisplayError(IDS_DISABLE_FILTER, IDS_TCPIP, MB_OK);

                        break;
                }
            }
            switch(LOWORD(wParam))
            {
                case IDC_OK:
                    pContext = (TcpipConfNotifyImpl*)GetWindowLongPtr(hwndDlg, DWLP_USER);
                    pFilter = StoreTcpipFilterSettings(hwndDlg);
                    if (pFilter)
                    {
                        if (pContext->pCurrentConfig->pFilter)
                        {
                            CoTaskMemFree(pContext->pCurrentConfig->pFilter->szTCPAllowedPorts);
                            CoTaskMemFree(pContext->pCurrentConfig->pFilter->szUDPAllowedPorts);
                            CoTaskMemFree(pContext->pCurrentConfig->pFilter->szRawIPAllowedProtocols);
                            CoTaskMemFree(pContext->pCurrentConfig->pFilter);
                        }
                        pContext->pCurrentConfig->pFilter = pFilter;
                    }
                    EndDialog(hwndDlg, (INT_PTR)TRUE);
                    break;
                case IDCANCEL:
                    EndDialog(hwndDlg, FALSE);
                    break;
                case IDC_TCP_ADD:
                    AddItem(hwndDlg, GetDlgItem(hwndDlg, IDC_TCP_LIST), 65536, IDS_TCP_PORTS);
                    break;
                case IDC_TCP_DEL:
                    DelItem(GetDlgItem(hwndDlg, IDC_TCP_LIST));
                    break;
                case IDC_UDP_ADD:
                    AddItem(hwndDlg, GetDlgItem(hwndDlg, IDC_UDP_LIST), 65536, IDS_UDP_PORTS);
                    break;
                case IDC_UDP_DEL:
                    DelItem(GetDlgItem(hwndDlg, IDC_UDP_LIST));
                    break;
                case IDC_IP_ADD:
                    AddItem(hwndDlg, GetDlgItem(hwndDlg, IDC_IP_LIST), 256, IDS_IP_PROTO);
                    break;
                case IDC_IP_DEL:
                    DelItem(GetDlgItem(hwndDlg, IDC_IP_LIST));
                    break;
                default:
                    break;
            }
        default:
            break;
    }

    return FALSE;
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
    ppage.hInstance = netcfgx_hInstance;
    if (szTitle)
    {
        ppage.dwFlags |= PSP_USETITLE;
        ppage.pszTitle = szTitle;
    }
    return CreatePropertySheetPageW(&ppage);
}

/***************************************************************
 * TCP/IP Advanced Option Dialog
 *
 */

VOID
InitializeTcpipAdvancedOptDlg(
    HWND hwndDlg,
    TcpipConfNotifyImpl * This)
{
    WCHAR szText[500];
    /* store context */
    SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)This);

    if (LoadStringW(netcfgx_hInstance, IDS_TCPFILTER, szText, sizeof(szText)/sizeof(WCHAR)))
    {
        szText[(sizeof(szText)/sizeof(WCHAR))-1] = L'\0';
        if (SendDlgItemMessageW(hwndDlg, IDC_OPTLIST, LB_ADDSTRING, 0, (LPARAM)szText) != LB_ERR)
            SendDlgItemMessageW(hwndDlg, IDC_OPTLIST, LB_SETCURSEL, 0, 0);
    }

    if (LoadStringW(netcfgx_hInstance, IDS_TCPFILTERDESC, szText, sizeof(szText)/sizeof(WCHAR)))
    {
        szText[(sizeof(szText)/sizeof(WCHAR))-1] = L'\0';
        SendDlgItemMessageW(hwndDlg, IDC_OPTDESC, WM_SETTEXT, 0, (LPARAM)szText);
    }
}



INT_PTR
CALLBACK
TcpipAdvancedOptDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    TcpipConfNotifyImpl * This;
    LPPROPSHEETPAGE page;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            page = (LPPROPSHEETPAGE)lParam;
            This = (TcpipConfNotifyImpl*)page->lParam;
            InitializeTcpipAdvancedOptDlg(hwndDlg, This);
            return TRUE;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_OPTPROP)
            {
                DialogBoxParamW(netcfgx_hInstance,
                                MAKEINTRESOURCEW(IDD_TCPIP_FILTER_DLG),
                                GetParent(hwndDlg),
                                TcpipFilterSettingsDlg,
                                (LPARAM)GetWindowLongPtr(hwndDlg, DWLP_USER));
                break;
            }
    }
    return FALSE;
}

VOID
InsertColumnToListView(
    HWND hDlgCtrl,
    UINT ResId,
    UINT SubItem,
    UINT Size)
{
    WCHAR szBuffer[200];
    LVCOLUMNW lc;

    if (!LoadStringW(netcfgx_hInstance, ResId, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
        return;

    memset(&lc, 0, sizeof(LV_COLUMN) );
    lc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
    lc.iSubItem   = SubItem;
    lc.fmt = LVCFMT_FIXED_WIDTH;
    lc.cx         = Size;
    lc.cchTextMax = wcslen(szBuffer);
    lc.pszText    = szBuffer;

    (void)SendMessageW(hDlgCtrl, LVM_INSERTCOLUMNW, SubItem, (LPARAM)&lc);
}

VOID
InsertIpAddressToListView(
    HWND hDlgCtrl,
    IP_ADDR * pAddr,
    BOOL bSubMask)
{
    WCHAR szBuffer[70];
    DWORD dwIpAddr;
    UINT itemCount = 0;
    LVITEMW li;

    while(pAddr)
    {
        ZeroMemory(&li, sizeof(li));
        li.mask = LVIF_TEXT;
        li.iItem = itemCount;
        li.iSubItem = 0;
        dwIpAddr = pAddr->IpAddress;
        swprintf(szBuffer, L"%lu.%lu.%lu.%lu",
                 FIRST_IPADDRESS(dwIpAddr), SECOND_IPADDRESS(dwIpAddr), THIRD_IPADDRESS(dwIpAddr), FOURTH_IPADDRESS(dwIpAddr));

        li.pszText = szBuffer;
        li.iItem = SendMessageW(hDlgCtrl, LVM_INSERTITEMW, 0, (LPARAM)&li);
        if (li.iItem  != -1)
        {
            if (bSubMask)
            {
                dwIpAddr = pAddr->u.Subnetmask;
                swprintf(szBuffer, L"%lu.%lu.%lu.%lu",
                         FIRST_IPADDRESS(dwIpAddr), SECOND_IPADDRESS(dwIpAddr), THIRD_IPADDRESS(dwIpAddr), FOURTH_IPADDRESS(dwIpAddr));
            }
            else
            {
                if (pAddr->u.Metric)
                    swprintf(szBuffer, L"%u", pAddr->u.Metric);
                else
                    LoadStringW(netcfgx_hInstance, IDS_AUTOMATIC, szBuffer, sizeof(szBuffer)/sizeof(WCHAR));
            }

            li.mask = LVIF_TEXT;
            li.iSubItem = 1;
            li.pszText = szBuffer;
            SendMessageW(hDlgCtrl, LVM_SETITEMW, 0, (LPARAM)&li);
        }
        itemCount++;
        pAddr = pAddr->Next;
    }
}

VOID
InitializeTcpipAdvancedIpDlg(
    HWND hwndDlg,
    TcpipConfNotifyImpl * This)
{
    RECT rect;
    LVITEMW li;
    WCHAR szBuffer[100];

    InsertColumnToListView(GetDlgItem(hwndDlg, IDC_IPLIST), IDS_IPADDR, 0, 100);
    GetClientRect(GetDlgItem(hwndDlg, IDC_IPLIST), &rect);
    InsertColumnToListView(GetDlgItem(hwndDlg, IDC_IPLIST), IDS_SUBMASK, 1, (rect.right - rect.left - 100));

    if (This->pCurrentConfig->DhcpEnabled)
    {
        if (LoadStringW(netcfgx_hInstance, IDS_DHCPACTIVE, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
        {
            ZeroMemory(&li, sizeof(LVITEMW));
            li.mask = LVIF_TEXT;
            li.pszText = szBuffer;
            SendDlgItemMessageW(hwndDlg, IDC_IPLIST, LVM_INSERTITEMW, 0, (LPARAM)&li);
        }
        EnableWindow(GetDlgItem(hwndDlg, IDC_IPADD), FALSE);
    }
    else
    {
        InsertIpAddressToListView(GetDlgItem(hwndDlg, IDC_IPLIST), This->pCurrentConfig->Ip, TRUE);
    }

    EnableWindow(GetDlgItem(hwndDlg, IDC_IPMOD), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_IPDEL), FALSE);

    InsertColumnToListView(GetDlgItem(hwndDlg, IDC_GWLIST), IDS_GATEWAY, 0, 100);
    GetClientRect(GetDlgItem(hwndDlg, IDC_IPLIST), &rect);
    InsertColumnToListView(GetDlgItem(hwndDlg, IDC_GWLIST), IDS_METRIC, 1, (rect.right - rect.left - 100));

    InsertIpAddressToListView(GetDlgItem(hwndDlg, IDC_GWLIST), This->pCurrentConfig->Gw, FALSE);

    EnableWindow(GetDlgItem(hwndDlg, IDC_GWMOD), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_GWDEL), FALSE);

    SendDlgItemMessageW(hwndDlg, IDC_METRIC, EM_LIMITTEXT, 4, 0);
}

INT_PTR
CALLBACK
TcpipAdvGwDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    WCHAR szBuffer[70];
    TcpipGwSettings *pGwSettings;
    DWORD dwIpAddr;
    LPNMIPADDRESS lpnmipa;
    LVFINDINFOW find;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            pGwSettings = (TcpipGwSettings *)lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)lParam);

            SendDlgItemMessageW(hwndDlg, IDC_IPADDR, IPM_SETRANGE, 0, MAKEIPRANGE(1, 223));
            SendDlgItemMessageW(hwndDlg, IDC_IPADDR, IPM_SETRANGE, 1, MAKEIPRANGE(0, 255));
            SendDlgItemMessageW(hwndDlg, IDC_IPADDR, IPM_SETRANGE, 2, MAKEIPRANGE(0, 255));
            SendDlgItemMessageW(hwndDlg, IDC_IPADDR, IPM_SETRANGE, 3, MAKEIPRANGE(0, 255));

            if (pGwSettings->bAdd)
            {
                if (LoadStringW(netcfgx_hInstance, IDS_ADD, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
                {
                    szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
                    SendDlgItemMessageW(hwndDlg, IDC_OK, WM_SETTEXT, 0, (LPARAM)szBuffer);
                }
                EnableWindow(GetDlgItem(hwndDlg, IDC_OK), FALSE);
                SendDlgItemMessageW(hwndDlg, IDC_USEMETRIC, BM_SETCHECK, BST_CHECKED, 0);
            }
            else
            {
                if (LoadStringW(netcfgx_hInstance, IDS_MOD, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
                {
                    szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
                    SendDlgItemMessageW(hwndDlg, IDC_OK, WM_SETTEXT, 0, (LPARAM)szBuffer);
                }

                SendDlgItemMessageW(hwndDlg, IDC_IPADDR, IPM_SETADDRESS, 0, (LPARAM)GetIpAddressFromStringW(pGwSettings->szIP));

                if (pGwSettings->Metric)
                {
                    SetDlgItemInt(hwndDlg, IDC_METRIC, pGwSettings->Metric, FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_METRIC), TRUE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_METRICTXT), TRUE);
                }
                else
                {
                    SendDlgItemMessageW(hwndDlg, IDC_USEMETRIC, BM_SETCHECK, BST_CHECKED, 0);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_METRIC), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_METRICTXT), FALSE);
                }
            }
            return TRUE;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_USEMETRIC)
            {
                if (SendDlgItemMessage(hwndDlg, IDC_USEMETRIC, BM_GETCHECK, 0, 0) == BST_CHECKED)
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_METRIC), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_METRICTXT), FALSE);
                    SendDlgItemMessageW(hwndDlg, IDC_METRIC, WM_SETTEXT, 0, (LPARAM)L"");
                }
                else
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_METRIC), TRUE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_METRICTXT), TRUE);
                }
                break;
            }
            else if (LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hwndDlg, FALSE);
                break;
            }
            else if (LOWORD(wParam) == IDC_OK)
            {
                if (SendDlgItemMessageW(hwndDlg, IDC_IPADDR, IPM_GETADDRESS, 0, (LPARAM)&dwIpAddr) == 4)
                {
                    pGwSettings = (TcpipGwSettings*)GetWindowLongPtr(hwndDlg, DWLP_USER);
                    SendDlgItemMessageW(hwndDlg, IDC_IPADDR, WM_GETTEXT, 16, (LPARAM)pGwSettings->szIP);

                    ZeroMemory(&find, sizeof(LVFINDINFOW));
                    find.flags = LVFI_STRING;
                    find.psz = pGwSettings->szIP;

                    if (SendDlgItemMessage(hwndDlg, IDC_USEMETRIC, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
                        pGwSettings->Metric = GetDlgItemInt(hwndDlg, IDC_METRIC, NULL, FALSE);
                    else
                        pGwSettings->Metric = 0;


                    if (SendMessageW(pGwSettings->hDlgCtrl, LVM_FINDITEMW, (WPARAM)-1, (LPARAM)&find) == -1)
                    {
                        EndDialog(hwndDlg, TRUE);
                        break;
                    }
                    if (!pGwSettings->bAdd)
                    {
                        EndDialog(hwndDlg, FALSE);
                        break;
                    }
                    DisplayError(IDS_DUP_GW, IDS_TCPIP, MB_ICONINFORMATION);
                }
                break;
            }
            break;
        case WM_NOTIFY:
            lpnmipa = (LPNMIPADDRESS) lParam;
            if (lpnmipa->hdr.code == IPN_FIELDCHANGED)
            {
                if (lpnmipa->hdr.idFrom == IDC_IPADDR)
                {
                    if (SendDlgItemMessageW(hwndDlg, IDC_IPADDR, IPM_GETADDRESS, 0, (LPARAM)&dwIpAddr) == 4)
                        EnableWindow(GetDlgItem(hwndDlg, IDC_OK), TRUE);
                }
            }
            break;
    }
    return FALSE;
}

BOOL
GetGWListEntry(HWND hDlgCtrl, INT Index, TcpipGwSettings * pGwSettings)
{
    LVITEMW li;
    WCHAR szBuffer[30];
    BOOL bRet;

    ZeroMemory(&li, sizeof(LVITEMW));
    li.mask = LVIF_TEXT;
    li.cchTextMax = 16;
    li.pszText = pGwSettings->szIP;
    li.iItem = Index;

    if (!SendMessageW(hDlgCtrl, LVM_GETITEMW, 0, (LPARAM)&li))
        return FALSE;
    li.pszText = szBuffer;
    li.cchTextMax = 30;
    li.iSubItem = 1;

    bRet = SendMessageW(hDlgCtrl, LVM_GETITEMW, 0, (LPARAM)&li);
    if (bRet)
    {
        pGwSettings->Metric = _wtoi(szBuffer);
    }

    return bRet;
}

INT_PTR
CALLBACK
TcpipAddIpDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    LPNMIPADDRESS lpnmipa;
    DWORD dwIpAddr;
    TcpipIpSettings *pIpSettings;
    WCHAR szBuffer[50];
    LVFINDINFOW find;
    LRESULT lResult;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            pIpSettings = (TcpipIpSettings*)lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)lParam);

            SendDlgItemMessageW(hwndDlg, IDC_IPADDR, IPM_SETRANGE, 0, MAKEIPRANGE(1, 223));
            SendDlgItemMessageW(hwndDlg, IDC_IPADDR, IPM_SETRANGE, 1, MAKEIPRANGE(0, 255));
            SendDlgItemMessageW(hwndDlg, IDC_IPADDR, IPM_SETRANGE, 2, MAKEIPRANGE(0, 255));
            SendDlgItemMessageW(hwndDlg, IDC_IPADDR, IPM_SETRANGE, 3, MAKEIPRANGE(0, 255));
            SendDlgItemMessageW(hwndDlg, IDC_SUBNETMASK, IPM_SETRANGE, 0, MAKEIPRANGE(0, 255));
            SendDlgItemMessageW(hwndDlg, IDC_SUBNETMASK, IPM_SETRANGE, 1, MAKEIPRANGE(0, 255));
            SendDlgItemMessageW(hwndDlg, IDC_SUBNETMASK, IPM_SETRANGE, 2, MAKEIPRANGE(0, 255));
            SendDlgItemMessageW(hwndDlg, IDC_SUBNETMASK, IPM_SETRANGE, 3, MAKEIPRANGE(0, 255));

            if (pIpSettings->bAdd)
            {
                if (LoadStringW(netcfgx_hInstance, IDS_ADD, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
                {
                    szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
                    SendDlgItemMessageW(hwndDlg, IDC_OK, WM_SETTEXT, 0, (LPARAM)szBuffer);
                }
                EnableWindow(GetDlgItem(hwndDlg, IDC_OK), FALSE);
            }
            else
            {
                if (LoadStringW(netcfgx_hInstance, IDS_MOD, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
                {
                    szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
                    SendDlgItemMessageW(hwndDlg, IDC_OK, WM_SETTEXT, 0, (LPARAM)szBuffer);
                }

                SendDlgItemMessageW(hwndDlg, IDC_IPADDR, IPM_SETADDRESS, 0, (LPARAM)GetIpAddressFromStringW(pIpSettings->szIP));
                SendDlgItemMessageW(hwndDlg, IDC_SUBNETMASK, IPM_SETADDRESS, 0, (LPARAM)GetIpAddressFromStringW(pIpSettings->szMask));
            }
            return TRUE;
        case WM_NOTIFY:
            lpnmipa = (LPNMIPADDRESS) lParam;
            if (lpnmipa->hdr.code == IPN_FIELDCHANGED)
            {
                if (lpnmipa->hdr.idFrom == IDC_IPADDR)
                {
                    if (SendDlgItemMessageW(hwndDlg, IDC_IPADDR, IPM_GETADDRESS, 0, (LPARAM)&dwIpAddr) == 4)
                    {
                        if (dwIpAddr <= MAKEIPADDRESS(127, 255, 255, 255))
                            SendDlgItemMessageW(hwndDlg, IDC_SUBNETMASK, IPM_SETADDRESS, 0, (LPARAM)MAKEIPADDRESS(255, 0, 0, 0));
                        else if (dwIpAddr <= MAKEIPADDRESS(191, 255, 255, 255))
                            SendDlgItemMessageW(hwndDlg, IDC_SUBNETMASK, IPM_SETADDRESS, 0, (LPARAM)MAKEIPADDRESS(255, 255, 0, 0));
                        else if (dwIpAddr <= MAKEIPADDRESS(223, 255, 255, 255))
                            SendDlgItemMessageW(hwndDlg, IDC_SUBNETMASK, IPM_SETADDRESS, 0, (LPARAM)MAKEIPADDRESS(255, 255, 255, 0));
                        EnableWindow(GetDlgItem(hwndDlg, IDC_OK), TRUE);
                     }
                }
            }
            break;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_OK)
            {
                pIpSettings = (TcpipIpSettings*)GetWindowLongPtr(hwndDlg, DWLP_USER);
                SendDlgItemMessageW(hwndDlg, IDC_IPADDR, WM_GETTEXT, 16, (LPARAM)pIpSettings->szIP);
                SendDlgItemMessageW(hwndDlg, IDC_SUBNETMASK, WM_GETTEXT, 16, (LPARAM)pIpSettings->szMask);

                ZeroMemory(&find, sizeof(LVFINDINFOW));
                find.flags = LVFI_STRING;
                find.psz = pIpSettings->szIP;
                lResult = SendMessageW(pIpSettings->hDlgCtrl, LVM_FINDITEMW, (WPARAM)-1, (LPARAM)&find);

                if (lResult == -1)
                {
                    EndDialog(hwndDlg, TRUE);
                    break;
                }
                else if (!pIpSettings->bAdd)
                {
                    EndDialog(hwndDlg, FALSE);
                    break;
                }
                DisplayError(IDS_DUP_IPADDR, IDS_TCPIP, MB_ICONINFORMATION);
                break;
            }
            else if (LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hwndDlg, FALSE);
            }
            break;
    }
    return FALSE;
}

BOOL
VerifyDNSSuffix(
    LPWSTR szName)
{
    UINT Index;
    UINT Length = wcslen(szName);

    for(Index = 0; Index < Length; Index++)
        if (iswalnum(szName[Index]) == 0 && szName[Index] != '.' && szName[Index] != '-')
            return FALSE;

    return TRUE;
}

INT_PTR
CALLBACK
TcpipAddSuffixDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    WCHAR szBuffer[100];
    TcpipSuffixSettings * pSettings;
    LRESULT lLength;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            pSettings = (TcpipSuffixSettings*)lParam;
            if (!pSettings->bAdd)
            {
                SendDlgItemMessageW(hwndDlg, IDC_SUFFIX, WM_SETTEXT, 0, (LPARAM)pSettings->Suffix);
                if (LoadStringW(netcfgx_hInstance, IDS_MOD, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
                {
                    szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
                    SendDlgItemMessageW(hwndDlg, IDC_OK, WM_SETTEXT, 0, (LPARAM)szBuffer);
                }
                CoTaskMemFree(pSettings->Suffix);
                pSettings->Suffix = NULL;
            }
            else
            {
                if (LoadStringW(netcfgx_hInstance, IDS_ADD, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
                {
                    szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
                    SendDlgItemMessageW(hwndDlg, IDC_OK, WM_SETTEXT, 0, (LPARAM)szBuffer);
                }
            }
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pSettings);
            return TRUE;
        case WM_COMMAND:
           if (LOWORD(wParam) == IDCANCEL)
           {
               EndDialog(hwndDlg, FALSE);
               break;
           }
           else if (LOWORD(wParam) == IDC_OK)
           {
               lLength = SendDlgItemMessageW(hwndDlg, IDC_SUFFIX, WM_GETTEXTLENGTH, 0, 0);
               if (lLength)
               {
                   pSettings = (TcpipSuffixSettings*) GetWindowLongPtr(hwndDlg, DWLP_USER);
                   pSettings->Suffix = (LPWSTR)CoTaskMemAlloc((lLength + 1)* sizeof(WCHAR));
                   if (pSettings->Suffix)
                   {
                       SendDlgItemMessageW(hwndDlg, IDC_SUFFIX, WM_GETTEXT, lLength + 1, (LPARAM)pSettings->Suffix);
                       if (SendMessageW(pSettings->hDlgCtrl, LB_FINDSTRING, 0, (LPARAM)pSettings->Suffix) != LB_ERR)
                       {
                           DisplayError(IDS_DUP_SUFFIX, IDS_TCPIP, MB_ICONWARNING);
                           CoTaskMemFree(pSettings->Suffix);
                           break;
                       }

                       if (!VerifyDNSSuffix(pSettings->Suffix))
                       {
                           DisplayError(IDS_DOMAIN_SUFFIX, IDS_TCPIP, MB_ICONWARNING);
                           CoTaskMemFree(pSettings->Suffix);
                           break;
                       }
                       EndDialog(hwndDlg, TRUE);
                   }
               }
               break;
           }
    }
    return FALSE;
}


INT
GetSelectedItem(HWND hDlgCtrl)
{
    LVITEMW li;
    UINT iItemCount, iIndex;

    iItemCount = ListView_GetItemCount(hDlgCtrl);
    if (!iItemCount)
        return -1;

    for (iIndex = 0; iIndex < iItemCount; iIndex++)
    {
        ZeroMemory(&li, sizeof(LVITEMW));
        li.mask = LVIF_STATE;
        li.stateMask = (UINT)-1;
        li.iItem = iIndex;
        if (SendMessageW(hDlgCtrl, LVM_GETITEMW, 0, (LPARAM)&li))
        {
            if (li.state & LVIS_SELECTED)
                return iIndex;
        }
    }
    return -1;
}


BOOL
GetIPListEntry(HWND hDlgCtrl, INT Index, TcpipIpSettings * pIpSettings)
{
    LVITEMW li;

    ZeroMemory(&li, sizeof(LVITEMW));
    li.mask = LVIF_TEXT;
    li.cchTextMax = 16;
    li.pszText = pIpSettings->szIP;
    li.iItem = Index;

    if (!SendMessageW(hDlgCtrl, LVM_GETITEMW, 0, (LPARAM)&li))
        return FALSE;

    ZeroMemory(&li, sizeof(LVITEMW));
    li.mask = LVIF_TEXT;
    li.cchTextMax = 16;
    li.pszText = pIpSettings->szMask;
    li.iSubItem = 1;
    li.iItem = Index;

    return SendMessageW(hDlgCtrl, LVM_GETITEMW, 0, (LPARAM)&li);
}

VOID
DeleteItemFromList(HWND hDlgCtrl)
{
    LVITEMW li;

    memset(&li, 0x0, sizeof(LVITEMW));
    li.iItem = GetSelectedItem(hDlgCtrl);
    if (li.iItem < 0)
    {
        DisplayError(IDS_NOITEMSEL, IDS_TCPIP, MB_ICONINFORMATION);
        SetFocus(hDlgCtrl);
    }
    else
    {
        (void)ListView_DeleteItem(hDlgCtrl, li.iItem);
    }
}

UINT
GetIpAddressFromStringW(
    WCHAR * szBuffer)
{
    DWORD dwIpAddr = 0;
    INT Val;
    UINT Index = 3;
    LPWSTR pLast = szBuffer;
    LPWSTR pNext = szBuffer;


    while((pNext = wcschr(pNext, L'.')))
    {
        pNext[0] = L'\0';
        Val = _wtoi(pLast);
        dwIpAddr |= (Val << Index * 8);
        Index--;
        pNext++;
        pLast = pNext;
    }
    dwIpAddr |= _wtoi(pLast);

    return dwIpAddr;
}

UINT
GetIpAddressFromStringA(
    char * sBuffer)
{
    WCHAR szIp[16];

    if (MultiByteToWideChar(CP_ACP, 0, sBuffer, -1, szIp, 16))
    {
        szIp[15] = L'\0';
       return GetIpAddressFromStringW(szIp);
    }
    return (UINT)-1;
}


VOID
FreeIPAddr(IP_ADDR *pAddr)
{
    IP_ADDR *pNext;

    if (!pAddr)
        return;

    while(pAddr)
    {
        pNext = pAddr->Next;
        CoTaskMemFree(pAddr);
        pAddr = pNext;
    }
}

BOOL
GetListViewItem(HWND hDlgCtrl, UINT Index, UINT SubIndex, WCHAR * szBuffer, UINT BufferSize)
{
    LVITEMW li;

    ZeroMemory(&li, sizeof(LVITEMW));
    li.mask = LVIF_TEXT;
    li.pszText = szBuffer;
    li.iItem = Index;
    li.iSubItem = SubIndex;
    li.cchTextMax = BufferSize;
    return SendMessageW(hDlgCtrl, LVM_GETITEMW, 0, (LPARAM)&li);
}

VOID
StoreIPSettings(
    HWND hDlgCtrl,
    TcpipConfNotifyImpl * This,
    BOOL bSubmask)
{
    WCHAR szBuffer[30];

    INT iIndex, iCount;
    IP_ADDR *pCur, *pLast;

    iCount = ListView_GetItemCount(hDlgCtrl);
    if (!iCount)
    {
        return;
    }

    pLast = NULL;
    for(iIndex = 0; iIndex < iCount; iIndex++)
    {
        if (GetListViewItem(hDlgCtrl, iIndex, 0, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
        {
            pCur = (IP_ADDR*)CoTaskMemAlloc(sizeof(IP_ADDR));
            if (!pCur)
                break;
            ZeroMemory(pCur, sizeof(IP_ADDR));

            szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
            pCur->IpAddress = GetIpAddressFromStringW(szBuffer);

            if (GetListViewItem(hDlgCtrl, iIndex, 1, szBuffer, sizeof(szBuffer)/sizeof(WCHAR) ))
            {
                szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
                if (bSubmask)
                    pCur->u.Subnetmask = GetIpAddressFromStringW(szBuffer);
                else
                    pCur->u.Metric  = _wtoi(szBuffer);
            }

            if (!pLast)
            {
                if (bSubmask)
                    This->pCurrentConfig->Ip = pCur;
                else
                    This->pCurrentConfig->Gw = pCur;
            }
            else
            {
                pLast->Next = pCur;
            }

            pLast = pCur;
        }
    }
}


INT_PTR
CALLBACK
TcpipAdvancedIpDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    TcpipConfNotifyImpl * This;
    LPPROPSHEETPAGE page;
    INT_PTR res;
    WCHAR szBuffer[200];
    LPPSHNOTIFY lppsn;
    TcpipGwSettings Gw;
    TcpipIpSettings Ip;

    LVITEMW li;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            page = (LPPROPSHEETPAGE)lParam;
            This = (TcpipConfNotifyImpl*)page->lParam;
            InitializeTcpipAdvancedIpDlg(hwndDlg, This);
            SetWindowLongPtr(hwndDlg, DWLP_USER, (INT_PTR)This);
            return TRUE;
        case WM_NOTIFY:
            lppsn = (LPPSHNOTIFY) lParam;
            if (lppsn->hdr.code == LVN_ITEMCHANGED)
            {
                LPNMLISTVIEW lplv = (LPNMLISTVIEW)lParam;
                BOOL bEnable;

                if (lplv->hdr.idFrom == IDC_IPLIST)
                {
                    This = (TcpipConfNotifyImpl*)GetWindowLongPtr(hwndDlg, DWLP_USER);

                    bEnable = ((lplv->uNewState & LVIS_SELECTED) != 0) &&
                              (!This->pCurrentConfig->DhcpEnabled);

                    EnableWindow(GetDlgItem(hwndDlg, IDC_IPMOD), bEnable);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_IPDEL), bEnable);
                }
                else if (lplv->hdr.idFrom == IDC_GWLIST)
                {
                    bEnable = ((lplv->uNewState & LVIS_SELECTED) != 0);

                    EnableWindow(GetDlgItem(hwndDlg, IDC_GWMOD), bEnable);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_GWDEL), bEnable);
                }
            }
            else if (lppsn->hdr.code == PSN_KILLACTIVE)
            {
                This = (TcpipConfNotifyImpl*)GetWindowLongPtr(hwndDlg, DWLP_USER);
                if (!This->pCurrentConfig->DhcpEnabled && ListView_GetItemCount(GetDlgItem(hwndDlg, IDC_IPLIST)) == 0)
                {
                    DisplayError(IDS_NO_IPADDR_SET, IDS_TCPIP, MB_ICONWARNING);
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }
            }
            else if (lppsn->hdr.code == PSN_APPLY)
            {
                This = (TcpipConfNotifyImpl*) GetWindowLongPtr(hwndDlg, DWLP_USER);
                FreeIPAddr(This->pCurrentConfig->Gw);
                This->pCurrentConfig->Gw = NULL;
                FreeIPAddr(This->pCurrentConfig->Ip);
                This->pCurrentConfig->Ip = NULL;
                StoreIPSettings(GetDlgItem(hwndDlg, IDC_IPLIST), This, TRUE);
                StoreIPSettings(GetDlgItem(hwndDlg, IDC_GWLIST), This, FALSE);
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                return TRUE;
            }
            break;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_AUTOMETRIC)
            {
                if (SendDlgItemMessageW(hwndDlg, IDC_AUTOMETRIC, BM_GETCHECK, 0, 0) == BST_CHECKED)
                    EnableWindow(GetDlgItem(hwndDlg, IDC_METRIC), FALSE);
                else
                    EnableWindow(GetDlgItem(hwndDlg, IDC_METRIC), TRUE);
            }
            else if (LOWORD(wParam) == IDC_IPADD)
            {
                Ip.bAdd = TRUE;
                Ip.hDlgCtrl = GetDlgItem(hwndDlg, IDC_IPLIST);
                res = DialogBoxParamW(netcfgx_hInstance, MAKEINTRESOURCEW(IDD_TCPIPADDIP_DLG), hwndDlg, TcpipAddIpDlg, (LPARAM)&Ip);
                if (res)
                {
                    memset(&li, 0x0, sizeof(LVITEMW));
                    li.mask = LVIF_TEXT | LVIF_PARAM;
                    li.iItem = ListView_GetItemCount(GetDlgItem(hwndDlg, IDC_IPLIST));
                    li.pszText = Ip.szIP;
                    li.iItem = SendDlgItemMessageW(hwndDlg, IDC_IPLIST, LVM_INSERTITEMW, 0, (LPARAM)&li);
                    if (li.iItem  != -1)
                    {
                        li.mask = LVIF_TEXT;
                        li.iSubItem = 1;
                        li.pszText = Ip.szMask;
                        SendDlgItemMessageW(hwndDlg, IDC_IPLIST, LVM_SETITEMW, 0, (LPARAM)&li);
                    }
                }
            }
            else if (LOWORD(wParam) == IDC_IPMOD)
            {
                memset(&li, 0x0, sizeof(LVITEMW));
                li.iItem = GetSelectedItem(GetDlgItem(hwndDlg, IDC_IPLIST));
                if (li.iItem < 0)
                {
                    /* no item selected */
                    DisplayError(IDS_NOITEMSEL, IDS_TCPIP, MB_ICONINFORMATION);
                    SetFocus(GetDlgItem(hwndDlg, IDC_IPLIST));
                    break;
                }
                Ip.bAdd = FALSE;
                Ip.hDlgCtrl = GetDlgItem(hwndDlg, IDC_IPLIST);
                if (GetIPListEntry(GetDlgItem(hwndDlg, IDC_IPLIST), li.iItem, &Ip))
                {
                    res = DialogBoxParamW(netcfgx_hInstance, MAKEINTRESOURCEW(IDD_TCPIPADDIP_DLG), hwndDlg, TcpipAddIpDlg, (LPARAM)&Ip);
                    if (res)
                    {
                            li.mask = LVIF_TEXT;
                            li.pszText = Ip.szIP;
                            SendDlgItemMessageW(hwndDlg, IDC_IPLIST, LVM_SETITEMW, 0, (LPARAM)&li);
                            li.pszText = Ip.szMask;
                            li.iSubItem = 1;
                            SendDlgItemMessageW(hwndDlg, IDC_IPLIST, LVM_SETITEMW, 0, (LPARAM)&li);
                    }
                }
            }
            else if (LOWORD(wParam) == IDC_IPDEL)
            {
                DeleteItemFromList(GetDlgItem(hwndDlg, IDC_IPLIST));
                break;
            }
            else if (LOWORD(wParam) == IDC_GWADD)
            {
                Gw.bAdd = TRUE;
                Gw.hDlgCtrl = GetDlgItem(hwndDlg, IDC_GWLIST);
                res = DialogBoxParamW(netcfgx_hInstance, MAKEINTRESOURCEW(IDD_TCPIPGW_DLG), hwndDlg, TcpipAdvGwDlg, (LPARAM)&Gw);
                if (res)
                {
                    memset(&li, 0x0, sizeof(LVITEMW));
                    li.mask = LVIF_TEXT;
                    li.iItem = ListView_GetItemCount(GetDlgItem(hwndDlg, IDC_GWLIST));
                    li.pszText = Gw.szIP;
                    li.iItem = SendDlgItemMessageW(hwndDlg, IDC_GWLIST, LVM_INSERTITEMW, 0, (LPARAM)&li);
                    if (li.iItem >= 0)
                    {
                        if (Gw.Metric)
                        {
                            swprintf(szBuffer, L"%u", Gw.Metric);
                            li.iSubItem = 1;
                            li.pszText = szBuffer;
                            SendDlgItemMessageW(hwndDlg, IDC_GWLIST, LVM_SETITEMW, 0, (LPARAM)&li);
                        }
                        else
                        {
                            if (LoadStringW(netcfgx_hInstance, IDS_AUTOMATIC, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
                            {
                                szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
                                li.iSubItem = 1;
                                li.pszText = szBuffer;
                                SendDlgItemMessageW(hwndDlg, IDC_GWLIST, LVM_SETITEMW, 0, (LPARAM)&li);
                            }
                        }
                    }
                }
                break;
            }
            else if (LOWORD(wParam) == IDC_GWMOD)
            {
                memset(&li, 0x0, sizeof(LVITEMW));
                li.iItem = GetSelectedItem(GetDlgItem(hwndDlg, IDC_GWLIST));
                if (li.iItem < 0)
                {
                    /* no item selected */
                    DisplayError(IDS_NOITEMSEL, IDS_TCPIP, MB_ICONINFORMATION);
                    SetFocus(GetDlgItem(hwndDlg, IDC_GWLIST));
                    break;
                }
                if (GetGWListEntry(GetDlgItem(hwndDlg, IDC_GWLIST), li.iItem, &Gw))
                {
                    Gw.bAdd = FALSE;
                    Gw.hDlgCtrl = GetDlgItem(hwndDlg, IDC_GWLIST);
                    res = DialogBoxParamW(netcfgx_hInstance, MAKEINTRESOURCEW(IDD_TCPIPGW_DLG), hwndDlg, TcpipAdvGwDlg, (LPARAM)&Gw);
                    if (res)
                    {
                        li.mask = LVIF_TEXT;
                        li.pszText = Gw.szIP;
                        (void)SendDlgItemMessageW(hwndDlg, IDC_GWLIST, LVM_SETITEMW, 0, (LPARAM)&li);
                        if (Gw.Metric)
                        {
                            swprintf(szBuffer, L"%u", Gw.Metric);
                            li.iSubItem = 1;
                            li.pszText = szBuffer;
                            SendDlgItemMessageW(hwndDlg, IDC_GWLIST, LVM_SETITEMW, 0, (LPARAM)&li);
                        }
                        else
                        {
                            if (LoadStringW(netcfgx_hInstance, IDS_AUTOMATIC, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
                            {
                                szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
                                li.iSubItem = 1;
                                li.pszText = szBuffer;
                                SendDlgItemMessageW(hwndDlg, IDC_GWLIST, LVM_SETITEMW, 0, (LPARAM)&li);
                            }
                        }
                    }
                }
                break;
            }
            else if (LOWORD(wParam) == IDC_GWDEL)
            {
                DeleteItemFromList(GetDlgItem(hwndDlg, IDC_GWLIST));
                break;
            }
    }
    return FALSE;
}

INT_PTR
CALLBACK
TcpipAddDNSDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    TcpipDnsSettings * pSettings;
    WCHAR szBuffer[100];
    DWORD dwIpAddr;
    LPNMIPADDRESS lpnmipa;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            pSettings = (TcpipDnsSettings*)lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)lParam);
            if (!pSettings->bAdd)
            {
                if (LoadStringW(netcfgx_hInstance, IDS_MOD, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
                {
                    szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
                    SendDlgItemMessageW(hwndDlg, IDC_OK, WM_SETTEXT, 0, (LPARAM)szBuffer);
                }
                SendDlgItemMessageW(hwndDlg, IDC_IPADDR, WM_SETTEXT, 0, (LPARAM)pSettings->szIP);
                EnableWindow(GetDlgItem(hwndDlg, IDC_OK), TRUE);
            }
            else
            {
                if (LoadStringW(netcfgx_hInstance, IDS_ADD, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
                {
                    szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
                    SendDlgItemMessageW(hwndDlg, IDC_OK, WM_SETTEXT, 0, (LPARAM)szBuffer);
                }
                EnableWindow(GetDlgItem(hwndDlg, IDC_OK), FALSE);
            }
            return TRUE;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hwndDlg, FALSE);
                break;
            }
            else if (LOWORD(wParam) == IDC_OK)
            {
                pSettings = (TcpipDnsSettings*)GetWindowLongPtr(hwndDlg, DWLP_USER);
                SendDlgItemMessageW(hwndDlg, IDC_IPADDR, WM_GETTEXT, 16, (LPARAM)pSettings->szIP);
                if (SendMessageW(pSettings->hDlgCtrl, LB_FINDSTRING, 0, (LPARAM)pSettings->szIP) == LB_ERR)
                {
                    if (pSettings->bAdd)
                        SendMessageW(pSettings->hDlgCtrl, LB_ADDSTRING, 0, (LPARAM)pSettings->szIP);
                    EndDialog(hwndDlg, TRUE);
                    break;
                }
                if (!pSettings->bAdd)
                {
                    EndDialog(hwndDlg, FALSE);
                    break;
                }
                DisplayError(IDS_DUP_SUFFIX, IDS_TCPIP, MB_ICONERROR);
                break;
            }
            break;
        case WM_NOTIFY:
            lpnmipa = (LPNMIPADDRESS) lParam;
            if (lpnmipa->hdr.code == IPN_FIELDCHANGED)
            {
                if (lpnmipa->hdr.idFrom == IDC_IPADDR)
                {
                    if (SendDlgItemMessageW(hwndDlg, IDC_IPADDR, IPM_GETADDRESS, 0, (LPARAM)&dwIpAddr) == 4)
                        EnableWindow(GetDlgItem(hwndDlg, IDC_OK), TRUE);
                }
            }
            break;
    }
    return FALSE;
}



VOID
InitializeTcpipAdvancedDNSDlg(
    HWND hwndDlg,
    TcpipConfNotifyImpl * This)
{
    WCHAR szBuffer[200];
    LPWSTR pFirst, pSep, pList;
    IP_ADDR * pAddr;
    DWORD dwIpAddr;

    /* insert DNS addresses */
    pAddr = This->pCurrentConfig->Ns;
    while(pAddr)
    {
        dwIpAddr = pAddr->IpAddress;
        swprintf(szBuffer, L"%lu.%lu.%lu.%lu",
                 FIRST_IPADDRESS(dwIpAddr), SECOND_IPADDRESS(dwIpAddr), THIRD_IPADDRESS(dwIpAddr), FOURTH_IPADDRESS(dwIpAddr));

        SendDlgItemMessageW(hwndDlg, IDC_DNSADDRLIST, LB_ADDSTRING, 0, (LPARAM)szBuffer);
        pAddr = pAddr->Next;
    }
    SendDlgItemMessageW(hwndDlg, IDC_DNSADDRLIST, LB_SETCURSEL, 0, 0);

    if (!This->pCurrentConfig->pDNS)
        return;

    if (This->pCurrentConfig->pDNS->RegisterAdapterName)
        SendDlgItemMessageW(hwndDlg, IDC_REGSUFFIX, BM_SETCHECK, BST_CHECKED, 0);
    else
        EnableWindow(GetDlgItem(hwndDlg, IDC_USESUFFIX), FALSE);

    if (This->pCurrentConfig->pDNS->RegistrationEnabled)
        SendDlgItemMessageW(hwndDlg, IDC_USESUFFIX, BM_SETCHECK, BST_CHECKED, 0);

    if (This->pCurrentConfig->pDNS->szDomain[0])
        SendDlgItemMessageW(hwndDlg, IDC_SUFFIX, WM_SETTEXT, 0, (LPARAM)szBuffer);

    if (This->pCurrentConfig->pDNS->UseDomainNameDevolution)
        SendDlgItemMessageW(hwndDlg, IDC_TOPPRIMSUFFIX, BM_SETCHECK, BST_CHECKED, 0);

    if (!This->pCurrentConfig->pDNS->szSearchList || (wcslen(This->pCurrentConfig->pDNS->szSearchList) == 0))
    {
        SendDlgItemMessageW(hwndDlg, IDC_PRIMSUFFIX, BM_SETCHECK, BST_CHECKED, 0);
        EnableWindow(GetDlgItem(hwndDlg, IDC_DNSSUFFIXADD), FALSE);

        return;
    }

    pList = This->pCurrentConfig->pDNS->szSearchList;
    if (wcslen(pList))
    {
        pFirst = pList;
        do
        {
            pSep = wcschr(pFirst, L',');
            if (pSep)
            {
                pSep[0] = L'\0';
                SendDlgItemMessageW(hwndDlg, IDC_DNSSUFFIXLIST, LB_ADDSTRING, 0, (LPARAM)pFirst);
                pFirst = pSep + 1;
                pSep[0] = L',';
            }
            else
            {
                SendDlgItemMessageW(hwndDlg, IDC_DNSSUFFIXLIST, LB_ADDSTRING, 0, (LPARAM)pFirst);
                break;
            }
        }while(TRUE);

        EnableWindow(GetDlgItem(hwndDlg, IDC_TOPPRIMSUFFIX), FALSE);
        SendDlgItemMessageW(hwndDlg, IDC_SELSUFFIX, BM_SETCHECK, BST_CHECKED, 0);
        SendDlgItemMessageW(hwndDlg, IDC_DNSSUFFIXLIST, LB_SETCURSEL, 0, 0);
    }
}

VOID
ToggleUpDown(HWND hwndDlg, HWND hDlgCtrl, UINT UpButton, UINT DownButton, UINT ModButton, UINT DelButton)
{
    LRESULT lResult, lCount;

    lResult = SendMessageW(hDlgCtrl, LB_GETCURSEL, 0, 0);
    lCount = SendMessageW(hDlgCtrl, LB_GETCOUNT, 0, 0);
    if (lResult != LB_ERR)
    {
        if (lResult == 0)
            EnableWindow(GetDlgItem(hwndDlg, UpButton), FALSE);
        else
             EnableWindow(GetDlgItem(hwndDlg, UpButton), TRUE);

        if (lResult < lCount -1)
             EnableWindow(GetDlgItem(hwndDlg, DownButton), TRUE);
        else
             EnableWindow(GetDlgItem(hwndDlg, DownButton), FALSE);
    }

    if (lCount)
    {
        EnableWindow(GetDlgItem(hwndDlg, ModButton), TRUE);
        EnableWindow(GetDlgItem(hwndDlg, DelButton), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hwndDlg, ModButton), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, DelButton), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, UpButton), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, DownButton), FALSE);
    }
}

VOID
MoveItem(
    HWND hDlgCtrl,
    INT pos)
{
    WCHAR szBuffer[100];
    LRESULT lResult;

    lResult = SendMessageW(hDlgCtrl, LB_GETCURSEL, 0, 0);
    if (lResult != LB_ERR)
    {
        if (SendMessageW(hDlgCtrl, LB_GETTEXTLEN, (WPARAM)lResult, 0) < sizeof(szBuffer)/sizeof(WCHAR) - 1)
        {
            if (SendMessageW(hDlgCtrl, LB_GETTEXT, (WPARAM)lResult, (LPARAM)szBuffer) != LB_ERR)
            {
                SendMessageW(hDlgCtrl, LB_DELETESTRING, (WPARAM)lResult, 0);
                SendMessageW(hDlgCtrl, LB_INSERTSTRING, (WPARAM)lResult + pos, (LPARAM)szBuffer);
                SendMessageW(hDlgCtrl, LB_SETCURSEL, (WPARAM)lResult + pos, 0);
            }
        }
    }
}
VOID
RemoveItem(
    HWND hDlgCtrl)
{
    LRESULT lResult, lCount;

    lResult = SendMessageW(hDlgCtrl, LB_GETCURSEL, 0, 0);
    if (lResult != LB_ERR)
    {
        SendMessageW(hDlgCtrl, LB_DELETESTRING, (WPARAM)lResult, 0);
        lCount = SendMessageW(hDlgCtrl, LB_GETCOUNT, 0, 0);
        if (lResult + 1 < lCount)
            SendMessageW(hDlgCtrl, LB_SETCURSEL, (WPARAM)lResult, 0);
        else
            SendMessageW(hDlgCtrl, LB_SETCURSEL, (WPARAM)lCount-1, 0);
    }
}

LPWSTR
GetListViewEntries(
    HWND hDlgCtrl)
{
    DWORD dwSize;
    INT iCount, iIndex;
    INT_PTR lResult;
    LPWSTR pszSearchList, pItem;

    iCount = SendMessageW(hDlgCtrl, LB_GETCOUNT, 0, 0);
    if (!iCount || iCount == LB_ERR)
        return NULL; //BUGBUG

    dwSize = 0;

    for (iIndex = 0; iIndex < iCount; iIndex++)
    {
        lResult = SendMessageW(hDlgCtrl, LB_GETTEXTLEN, iIndex, 0);
        if (lResult == LB_ERR)
            return NULL;

        dwSize += lResult + 1;
    }

    pszSearchList = (LPWSTR)CoTaskMemAlloc((dwSize + 1) * sizeof(WCHAR));
    if (!pszSearchList)
        return NULL;

    pItem = pszSearchList;
    for (iIndex = 0; iIndex < iCount; iIndex++)
    {
        lResult = SendMessageW(hDlgCtrl, LB_GETTEXT, iIndex, (LPARAM)pItem);
        if (lResult == LB_ERR)
        {
            CoTaskMemFree(pszSearchList);
            return NULL;
        }
        dwSize -= lResult + 1;
        pItem += wcslen(pItem);
        if (iIndex != iCount -1)
        {
            pItem[0] = L',';
            pItem++;
        }
    }
    pItem[0] = L'\0';
    return pszSearchList;
}

VOID
StoreDNSSettings(
    HWND hDlgCtrl,
    TcpipConfNotifyImpl *This)
{
    INT iCount, iIndex;
    WCHAR Ip[16];
    IP_ADDR *pCur, *pLast;

    FreeIPAddr(This->pCurrentConfig->Ns);
    This->pCurrentConfig->Ns = NULL;

    iCount = SendMessageW(hDlgCtrl, LB_GETCOUNT, 0, 0);
    if (!iCount || iCount == LB_ERR)
    {
        return;
    }

    pLast = NULL;
    for(iIndex = 0; iIndex < iCount; iIndex++)
    {
        if (SendMessageW(hDlgCtrl, LB_GETTEXT, iIndex, (LPARAM)Ip) == LB_ERR)
            break;

        pCur = CoTaskMemAlloc(sizeof(IP_ADDR));
        if (!pCur)
            break;
        ZeroMemory(pCur, sizeof(IP_ADDR));
        pCur->IpAddress = GetIpAddressFromStringW(Ip);

        if (!pLast)
            This->pCurrentConfig->Ns = pCur;
        else
            pLast->Next = pCur;

        pLast = pCur;
        pCur = pCur->Next;
    }
    This->pCurrentConfig->AutoconfigActive = FALSE;
}

INT_PTR
CALLBACK
TcpipAdvancedDnsDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    TcpipConfNotifyImpl * This;
    LPPROPSHEETPAGE page;
    TcpipDnsSettings Dns;
    LRESULT lIndex, lLength;
    TcpipSuffixSettings Suffix;
    LPPSHNOTIFY lppsn;
    WCHAR szSuffix[100];
    WCHAR szFormat[200];
    WCHAR szBuffer[300];


    switch(uMsg)
    {
        case WM_INITDIALOG:
            page = (LPPROPSHEETPAGE)lParam;
            This = (TcpipConfNotifyImpl*)page->lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (INT_PTR)This);
            InitializeTcpipAdvancedDNSDlg(hwndDlg, This);
            ToggleUpDown(hwndDlg, GetDlgItem(hwndDlg, IDC_DNSADDRLIST), IDC_DNSADDRUP, IDC_DNSADDRDOWN, IDC_DNSADDRMOD, IDC_DNSADDRDEL);
            ToggleUpDown(hwndDlg, GetDlgItem(hwndDlg, IDC_DNSSUFFIXLIST), IDC_DNSSUFFIXUP, IDC_DNSSUFFIXDOWN, IDC_DNSSUFFIXMOD, IDC_DNSSUFFIXDEL);
            return TRUE;
        case WM_NOTIFY:
            lppsn = (LPPSHNOTIFY) lParam;
            if (lppsn->hdr.code == PSN_KILLACTIVE)
            {
                if (SendDlgItemMessageW(hwndDlg, IDC_SELSUFFIX, BM_GETCHECK, 0, 0) == BST_CHECKED &&
                    SendDlgItemMessageW(hwndDlg, IDC_DNSSUFFIXLIST, LB_GETCOUNT, 0, 0) == 0)
                {
                    DisplayError(IDS_NO_SUFFIX, IDS_TCPIP, MB_ICONWARNING);
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }
                if (SendDlgItemMessageW(hwndDlg, IDC_SUFFIX, WM_GETTEXT, sizeof(szSuffix)/sizeof(WCHAR), (LPARAM)szSuffix))
                {
                    szSuffix[(sizeof(szSuffix)/sizeof(WCHAR))-1] = L'\0';
                    if (VerifyDNSSuffix(szSuffix) == FALSE)
                    {
                        if (LoadStringW(netcfgx_hInstance, IDS_DNS_SUFFIX, szFormat, sizeof(szFormat)/sizeof(WCHAR)))
                        {
                            szFormat[(sizeof(szFormat)/sizeof(WCHAR))-1] = L'\0';
                            swprintf(szBuffer, szFormat, szSuffix);
                            if (LoadStringW(netcfgx_hInstance, IDS_TCPIP, szFormat, sizeof(szFormat)/sizeof(WCHAR)))
                                szFormat[(sizeof(szFormat)/sizeof(WCHAR))-1] = L'\0';
                            else
                                szFormat[0] = L'\0';

                            MessageBoxW(hwndDlg, szBuffer, szFormat, MB_ICONWARNING);
                            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                            SetFocus(GetDlgItem(hwndDlg, IDC_SUFFIX));
                            return TRUE;
                        }
                    }
                }
            }
            else if (lppsn->hdr.code == PSN_APPLY)
            {
                 This = (TcpipConfNotifyImpl*)GetWindowLongPtr(hwndDlg, DWLP_USER);
                 if (!This->pCurrentConfig->pDNS)
                   break;

                 StoreDNSSettings(GetDlgItem(hwndDlg, IDC_DNSADDRLIST), This);
                 if (SendDlgItemMessageW(hwndDlg, IDC_PRIMSUFFIX, BM_GETCHECK, 0, 0) == BST_CHECKED)
                 {
                     CoTaskMemFree(This->pCurrentConfig->pDNS->szSearchList);
                     This->pCurrentConfig->pDNS->szSearchList = NULL;
                     if (SendDlgItemMessageW(hwndDlg, IDC_TOPPRIMSUFFIX, BM_GETCHECK, 0, 0) == BST_CHECKED)
                         This->pCurrentConfig->pDNS->UseDomainNameDevolution = TRUE;
                     else
                         This->pCurrentConfig->pDNS->UseDomainNameDevolution = FALSE;
                 }
                 else
                 {
                     CoTaskMemFree(This->pCurrentConfig->pDNS->szSearchList);
                     This->pCurrentConfig->pDNS->szSearchList = NULL;
                     This->pCurrentConfig->pDNS->UseDomainNameDevolution = FALSE;
                     This->pCurrentConfig->pDNS->szSearchList = GetListViewEntries(GetDlgItem(hwndDlg, IDC_DNSSUFFIXLIST));
                 }

                 if (SendDlgItemMessageW(hwndDlg, IDC_REGSUFFIX, BM_GETCHECK, 0, 0) == BST_CHECKED)
                 {
                     This->pCurrentConfig->pDNS->RegisterAdapterName = TRUE;
                     if (SendDlgItemMessageW(hwndDlg, IDC_USESUFFIX, BM_GETCHECK, 0, 0) == BST_CHECKED)
                         This->pCurrentConfig->pDNS->RegistrationEnabled = TRUE;
                     else
                         This->pCurrentConfig->pDNS->RegistrationEnabled = FALSE;
                 }
                 else
                 {
                     This->pCurrentConfig->pDNS->RegisterAdapterName = FALSE;
                     This->pCurrentConfig->pDNS->RegistrationEnabled = FALSE;
                 }
            }
            break;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_DNSADDRLIST && HIWORD(wParam) == LBN_SELCHANGE)
            {
                ToggleUpDown(hwndDlg, (HWND)lParam, IDC_DNSADDRUP, IDC_DNSADDRDOWN, IDC_DNSADDRMOD, IDC_DNSADDRDEL);
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                break;
            }
            else if (LOWORD(wParam) == IDC_DNSSUFFIXLIST && HIWORD(wParam) == LBN_SELCHANGE)
            {
                ToggleUpDown(hwndDlg, (HWND)lParam, IDC_DNSSUFFIXUP, IDC_DNSSUFFIXDOWN, IDC_DNSSUFFIXMOD, IDC_DNSSUFFIXDEL);
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                break;
            }
            else if (LOWORD(wParam) == IDC_PRIMSUFFIX && HIWORD(wParam) == BN_CLICKED)
            {
                if (SendMessageW((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED)
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_DNSSUFFIXUP), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_DNSSUFFIXDOWN), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_DNSSUFFIXADD), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_DNSSUFFIXMOD), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_DNSSUFFIXDEL), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_TOPPRIMSUFFIX), TRUE);
                    SendDlgItemMessageW(hwndDlg, IDC_DNSSUFFIXLIST, LB_RESETCONTENT, 0, 0);
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                }
            }
            else if (LOWORD(wParam) == IDC_SELSUFFIX && HIWORD(wParam) == BN_CLICKED)
            {
                if (SendMessageW((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED)
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_DNSSUFFIXADD), TRUE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_TOPPRIMSUFFIX), FALSE);
                    ToggleUpDown(hwndDlg, (HWND)lParam, IDC_DNSSUFFIXUP, IDC_DNSSUFFIXDOWN, IDC_DNSSUFFIXMOD, IDC_DNSSUFFIXDEL);
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                }
                break;
            }
            else if (LOWORD(wParam) == IDC_REGSUFFIX && HIWORD(wParam) == BN_CLICKED)
            {
                if (SendMessageW((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED)
                    EnableWindow(GetDlgItem(hwndDlg, IDC_USESUFFIX), TRUE);
                else
                    EnableWindow(GetDlgItem(hwndDlg, IDC_USESUFFIX), FALSE);
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            }
            else if (LOWORD(wParam) == IDC_DNSADDRUP && HIWORD(wParam) == BN_CLICKED)
            {
                MoveItem(GetDlgItem(hwndDlg, IDC_DNSADDRLIST), -1);
                ToggleUpDown(hwndDlg, GetDlgItem(hwndDlg, IDC_DNSADDRLIST), IDC_DNSADDRUP, IDC_DNSADDRDOWN, IDC_DNSADDRMOD, IDC_DNSADDRDEL);
                SetFocus(GetDlgItem(hwndDlg, IDC_DNSADDRLIST));
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                break;
            }
            else if (LOWORD(wParam) == IDC_DNSADDRDOWN && HIWORD(wParam) == BN_CLICKED)
            {
                MoveItem(GetDlgItem(hwndDlg, IDC_DNSADDRLIST), 1);
                ToggleUpDown(hwndDlg, GetDlgItem(hwndDlg, IDC_DNSADDRLIST), IDC_DNSADDRUP, IDC_DNSADDRDOWN, IDC_DNSADDRMOD, IDC_DNSADDRDEL);
                SetFocus(GetDlgItem(hwndDlg, IDC_DNSADDRLIST));
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                break;
            }
            else if (LOWORD(wParam) == IDC_DNSSUFFIXUP && HIWORD(wParam) == BN_CLICKED)
            {
                MoveItem(GetDlgItem(hwndDlg, IDC_DNSSUFFIXLIST), -1);
                ToggleUpDown(hwndDlg, GetDlgItem(hwndDlg, IDC_DNSSUFFIXLIST), IDC_DNSSUFFIXUP, IDC_DNSSUFFIXDOWN, IDC_DNSSUFFIXMOD, IDC_DNSSUFFIXDEL);
                SetFocus(GetDlgItem(hwndDlg, IDC_DNSSUFFIXLIST));
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                break;
            }
            else if (LOWORD(wParam) == IDC_DNSSUFFIXDOWN && HIWORD(wParam) == BN_CLICKED)
            {
                MoveItem(GetDlgItem(hwndDlg, IDC_DNSSUFFIXLIST), 1);
                ToggleUpDown(hwndDlg, GetDlgItem(hwndDlg, IDC_DNSSUFFIXLIST), IDC_DNSSUFFIXUP, IDC_DNSSUFFIXDOWN, IDC_DNSSUFFIXMOD, IDC_DNSSUFFIXDEL);
                SetFocus(GetDlgItem(hwndDlg, IDC_DNSSUFFIXLIST));
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                break;
            }
            else if (LOWORD(wParam) == IDC_DNSADDRDEL && HIWORD(wParam) == BN_CLICKED)
            {
                RemoveItem(GetDlgItem(hwndDlg, IDC_DNSADDRLIST));
                ToggleUpDown(hwndDlg, GetDlgItem(hwndDlg, IDC_DNSADDRLIST), IDC_DNSADDRUP, IDC_DNSADDRDOWN, IDC_DNSADDRMOD, IDC_DNSADDRDEL);
                SetFocus(GetDlgItem(hwndDlg, IDC_DNSADDRLIST));
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                break;
            }
            else if (LOWORD(wParam) == IDC_DNSSUFFIXDEL && HIWORD(wParam) == BN_CLICKED)
            {
                RemoveItem(GetDlgItem(hwndDlg, IDC_DNSSUFFIXLIST));
                ToggleUpDown(hwndDlg, GetDlgItem(hwndDlg, IDC_DNSSUFFIXLIST), IDC_DNSSUFFIXUP, IDC_DNSSUFFIXDOWN, IDC_DNSSUFFIXMOD, IDC_DNSSUFFIXDEL);
                SetFocus(GetDlgItem(hwndDlg, IDC_DNSSUFFIXLIST));
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                break;
            }
            else if (LOWORD(wParam) == IDC_DNSADDRADD && HIWORD(wParam) == BN_CLICKED)
            {
                 Dns.bAdd = TRUE;
                 Dns.hDlgCtrl = GetDlgItem(hwndDlg, IDC_DNSADDRLIST);
                 if (DialogBoxParamW(netcfgx_hInstance, MAKEINTRESOURCEW(IDD_TCPIPDNS_DLG), NULL, TcpipAddDNSDlg, (LPARAM)&Dns))
                 {
                     ToggleUpDown(hwndDlg, GetDlgItem(hwndDlg, IDC_DNSADDRLIST), IDC_DNSADDRUP, IDC_DNSADDRDOWN, IDC_DNSADDRMOD, IDC_DNSADDRDEL);
                     PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                 }
                 break;
            }
            else if (LOWORD(wParam) == IDC_DNSADDRMOD && HIWORD(wParam) == BN_CLICKED)
            {
                 lIndex = SendDlgItemMessage(hwndDlg, IDC_DNSADDRLIST, LB_GETCURSEL, 0, 0);
                 if (lIndex != LB_ERR)
                 {
                     Dns.bAdd = FALSE;
                     Dns.hDlgCtrl = GetDlgItem(hwndDlg, IDC_DNSADDRLIST);
                     SendDlgItemMessageW(hwndDlg, IDC_DNSADDRLIST, LB_GETTEXT, (WPARAM)lIndex, (LPARAM)Dns.szIP);
                     if (DialogBoxParamW(netcfgx_hInstance, MAKEINTRESOURCEW(IDD_TCPIPDNS_DLG), NULL, TcpipAddDNSDlg, (LPARAM)&Dns))
                     {
                         SendDlgItemMessageW(hwndDlg, IDC_DNSADDRLIST, LB_DELETESTRING, lIndex, 0);
                         SendDlgItemMessageW(hwndDlg, IDC_DNSADDRLIST, LB_INSERTSTRING, lIndex, (LPARAM)Dns.szIP);
                         SendDlgItemMessageW(hwndDlg, IDC_DNSADDRLIST, LB_SETCURSEL, lIndex, 0);
                         SetFocus(GetDlgItem(hwndDlg, IDC_DNSADDRLIST));
                         PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                     }
                 }
                 break;
            }
            else if (LOWORD(wParam) == IDC_DNSSUFFIXADD && HIWORD(wParam) == BN_CLICKED)
            {
                Suffix.bAdd = TRUE;
                Suffix.hDlgCtrl = GetDlgItem(hwndDlg, IDC_DNSSUFFIXLIST);
                Suffix.Suffix = NULL;
                if (DialogBoxParamW(netcfgx_hInstance, MAKEINTRESOURCEW(IDD_TCPIPSUFFIX_DLG), NULL, TcpipAddSuffixDlg, (LPARAM)&Suffix))
                {
                    ToggleUpDown(hwndDlg, GetDlgItem(hwndDlg, IDC_DNSSUFFIXLIST), IDC_DNSSUFFIXUP, IDC_DNSSUFFIXDOWN, IDC_DNSSUFFIXMOD, IDC_DNSSUFFIXDEL);
                    lIndex = SendDlgItemMessageW(hwndDlg, IDC_DNSSUFFIXLIST, LB_ADDSTRING, 0, (LPARAM)Suffix.Suffix);
                    if (lIndex != LB_ERR)
                        SendDlgItemMessageW(hwndDlg, IDC_DNSSUFFIXLIST, LB_SETCURSEL, lIndex, 0);
                    SetFocus(GetDlgItem(hwndDlg, IDC_DNSSUFFIXLIST));
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    CoTaskMemFree(Suffix.Suffix);
                }
                break;
            }
            else if (LOWORD(wParam) == IDC_DNSSUFFIXMOD && HIWORD(wParam) == BN_CLICKED)
            {
                lIndex = SendDlgItemMessage(hwndDlg, IDC_DNSSUFFIXLIST, LB_GETCURSEL, 0, 0);
                if (lIndex != LB_ERR)
                {
                    Suffix.bAdd = FALSE;
                    Suffix.hDlgCtrl = GetDlgItem(hwndDlg, IDC_DNSSUFFIXLIST);
                    lLength = SendMessageW(Suffix.hDlgCtrl, LB_GETTEXTLEN, lIndex, 0);
                    if (lLength != LB_ERR)
                    {
                        Suffix.Suffix = (LPWSTR)CoTaskMemAlloc((lLength + 1) * sizeof(WCHAR));
                        if (Suffix.Suffix)
                        {
                            SendMessageW(Suffix.hDlgCtrl, LB_GETTEXT, lIndex, (LPARAM)Suffix.Suffix);
                            Suffix.Suffix[lLength] = L'\0';
                            if (DialogBoxParamW(netcfgx_hInstance, MAKEINTRESOURCEW(IDD_TCPIPSUFFIX_DLG), NULL, TcpipAddSuffixDlg, (LPARAM)&Suffix))
                            {
                                if (Suffix.Suffix)
                                {
                                    SendDlgItemMessageW(hwndDlg, IDC_DNSSUFFIXLIST, LB_DELETESTRING, lIndex, 0);
                                    SendDlgItemMessageW(hwndDlg, IDC_DNSSUFFIXLIST, LB_INSERTSTRING, lIndex, (LPARAM)Suffix.Suffix);
                                    SendDlgItemMessageW(hwndDlg, IDC_DNSSUFFIXLIST, LB_SETCURSEL, lIndex, 0);
                                    SetFocus(GetDlgItem(hwndDlg, IDC_DNSSUFFIXLIST));
                                    CoTaskMemFree(Suffix.Suffix);
                                }
                                ToggleUpDown(hwndDlg, GetDlgItem(hwndDlg, IDC_DNSSUFFIXLIST), IDC_DNSSUFFIXUP, IDC_DNSSUFFIXDOWN, IDC_DNSSUFFIXMOD, IDC_DNSSUFFIXDEL);
                                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                            }
                        }
                    }
                 }
                 break;
            }
    }
    return FALSE;
}

static int CALLBACK
PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    // NOTE: This callback is needed to set large icon correctly.
    HICON hIcon;
    switch (uMsg)
    {
        case PSCB_INITIALIZED:
        {
            hIcon = LoadIconW(netcfgx_hInstance, MAKEINTRESOURCEW(IDI_NETWORK));
            SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            break;
        }
    }
    return 0;
}

VOID
LaunchAdvancedTcpipSettings(
    HWND hwndDlg,
    TcpipConfNotifyImpl * This)
{
    PROPSHEETHEADERW pinfo;
    HPROPSHEETPAGE hppages[3];
    WCHAR szBuffer[100];

    hppages[0] = InitializePropertySheetPage(MAKEINTRESOURCEW(IDD_TCPIP_ADVIP_DLG), TcpipAdvancedIpDlg, (LPARAM)This, NULL);
    hppages[1] = InitializePropertySheetPage(MAKEINTRESOURCEW(IDD_TCPIP_ADVDNS_DLG), TcpipAdvancedDnsDlg, (LPARAM)This, NULL);
    hppages[2] = InitializePropertySheetPage(MAKEINTRESOURCEW(IDD_TCPIP_ADVOPT_DLG), TcpipAdvancedOptDlg, (LPARAM)This, NULL);


    if (LoadStringW(netcfgx_hInstance, IDS_TCPIP, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
        szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
    else
        szBuffer[0] = L'\0';

    ZeroMemory(&pinfo, sizeof(PROPSHEETHEADERW));
    pinfo.dwSize = sizeof(PROPSHEETHEADERW);
    pinfo.dwFlags = PSH_NOCONTEXTHELP | PSH_PROPTITLE | PSH_NOAPPLYNOW |
                    PSH_USEICONID | PSH_USECALLBACK;
    pinfo.u3.phpage = hppages;
    pinfo.nPages = 3;
    pinfo.hwndParent = hwndDlg;
    pinfo.hInstance = netcfgx_hInstance;
    pinfo.pszCaption = szBuffer;
    pinfo.u.pszIcon = MAKEINTRESOURCEW(IDI_NETWORK);
    pinfo.pfnCallback = PropSheetProc;

    StoreTcpipBasicSettings(hwndDlg, This, FALSE);
    if (PropertySheetW(&pinfo) > 0)
    {
        InitializeTcpipBasicDlgCtrls(hwndDlg, This->pCurrentConfig);
        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
    }
}

INT_PTR
CALLBACK
TcpipAltConfDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
            return TRUE;
    }
    return FALSE;
}

VOID
AddAlternativeDialog(
    HWND hDlg,
    TcpipConfNotifyImpl * This)
{
    HPROPSHEETPAGE hpage;

    hpage = InitializePropertySheetPage(MAKEINTRESOURCEW(IDD_TCPIP_ALTCF_DLG), TcpipAltConfDlg, (LPARAM)This, NULL);
    if (!hpage)
        return;

    SendMessageW(hDlg, PSM_INSERTPAGE, 1, (LPARAM)hpage);
}

INT_PTR
StoreTcpipBasicSettings(
    HWND hwndDlg,
    TcpipConfNotifyImpl * This,
    BOOL bApply)
{
    DWORD dwIpAddr;

    if (SendDlgItemMessageW(hwndDlg, IDC_NODHCP, BM_GETCHECK, 0, 0) == BST_CHECKED)
    {
        This->pCurrentConfig->DhcpEnabled = FALSE;
        if (SendDlgItemMessageW(hwndDlg, IDC_IPADDR, IPM_GETADDRESS, 0, (LPARAM)&dwIpAddr) != 4)
        {
            if (bApply)
            {
                DisplayError(IDS_NO_IPADDR_SET, IDS_TCPIP, MB_ICONWARNING);
                SetFocus(GetDlgItem(hwndDlg, IDC_IPADDR));
                return E_FAIL;
            }
        }
        if (!This->pCurrentConfig->Ip)
        {
            This->pCurrentConfig->Ip = (IP_ADDR*)CoTaskMemAlloc(sizeof(IP_ADDR));
            if (!This->pCurrentConfig->Ip)
                return E_OUTOFMEMORY;
            ZeroMemory(This->pCurrentConfig->Ip, sizeof(IP_ADDR));
        }
        This->pCurrentConfig->Ip->IpAddress = dwIpAddr;

        if (SendDlgItemMessageW(hwndDlg, IDC_SUBNETMASK, IPM_GETADDRESS, 0, (LPARAM)&dwIpAddr) != 4)
        {
            if (bApply)
                DisplayError(IDS_NO_SUBMASK_SET, IDS_TCPIP, MB_ICONWARNING);
            if (SendDlgItemMessageW(hwndDlg, IDC_IPADDR, IPM_GETADDRESS, 0, (LPARAM)&dwIpAddr) == 4)
            {
                if (dwIpAddr <= MAKEIPADDRESS(127, 255, 255, 255))
                    dwIpAddr = MAKEIPADDRESS(255, 0, 0, 0);
                else if (dwIpAddr <= MAKEIPADDRESS(191, 255, 255, 255))
                    dwIpAddr = MAKEIPADDRESS(255, 255, 0, 0);
                else if (dwIpAddr <= MAKEIPADDRESS(223, 255, 255, 255))
                    dwIpAddr = MAKEIPADDRESS(255, 255, 255, 0);

                SendDlgItemMessageW(hwndDlg, IDC_SUBNETMASK, IPM_SETADDRESS, 0, (LPARAM)dwIpAddr);
            }
            if (bApply)
            {
                SetFocus(GetDlgItem(hwndDlg, IDC_SUBNETMASK));
                return E_FAIL;
            }
        }
        /* store subnetmask */
        This->pCurrentConfig->Ip->u.Subnetmask = dwIpAddr;
    }
    else
    {
        This->pCurrentConfig->DhcpEnabled = TRUE;
    }

    if (SendDlgItemMessageW(hwndDlg, IDC_DEFGATEWAY, IPM_GETADDRESS, 0, (LPARAM)&dwIpAddr) == 4)
    {
        if (!This->pCurrentConfig->Gw)
        {
            This->pCurrentConfig->Gw = (IP_ADDR*)CoTaskMemAlloc(sizeof(IP_ADDR));
            if (!This->pCurrentConfig->Gw)
                return E_OUTOFMEMORY;
            ZeroMemory(This->pCurrentConfig->Gw, sizeof(IP_ADDR));
        }
        /* store default gateway */
        This->pCurrentConfig->Gw->IpAddress = dwIpAddr;
    }
    else
    {
        if (This->pCurrentConfig->Gw)
        {
            IP_ADDR * pNextGw = This->pCurrentConfig->Gw->Next;
            CoTaskMemFree(This->pCurrentConfig->Gw);
            This->pCurrentConfig->Gw = pNextGw;
        }
    }

    if (SendDlgItemMessageW(hwndDlg, IDC_FIXEDDNS, BM_GETCHECK, 0, 0) == BST_CHECKED)
    {
        BOOL bSkip = FALSE;
        This->pCurrentConfig->AutoconfigActive = FALSE;
        if (SendDlgItemMessageW(hwndDlg, IDC_DNS1, IPM_GETADDRESS, 0, (LPARAM)&dwIpAddr) == 4)
        {
            if (!This->pCurrentConfig->Ns)
            {
                This->pCurrentConfig->Ns = (IP_ADDR*)CoTaskMemAlloc(sizeof(IP_ADDR));
                if (!This->pCurrentConfig->Ns)
                    return E_OUTOFMEMORY;
                ZeroMemory(This->pCurrentConfig->Ns, sizeof(IP_ADDR));
            }
            This->pCurrentConfig->Ns->IpAddress = dwIpAddr;
        }
        else if (This->pCurrentConfig->Ns)
        {
            IP_ADDR *pTemp = This->pCurrentConfig->Ns->Next;

            CoTaskMemFree(This->pCurrentConfig->Ns);
            This->pCurrentConfig->Ns = pTemp;
            bSkip = TRUE;
        }


        if (SendDlgItemMessageW(hwndDlg, IDC_DNS2, IPM_GETADDRESS, 0, (LPARAM)&dwIpAddr) == 4)
        {
            if (!This->pCurrentConfig->Ns || bSkip)
            {
                if (!This->pCurrentConfig->Ns)
                {
                    This->pCurrentConfig->Ns = (IP_ADDR*)CoTaskMemAlloc(sizeof(IP_ADDR));
                    if (!This->pCurrentConfig->Ns)
                        return E_OUTOFMEMORY;
                    ZeroMemory(This->pCurrentConfig->Ns, sizeof(IP_ADDR));
                }
                This->pCurrentConfig->Ns->IpAddress = dwIpAddr;
            }
            else if (!This->pCurrentConfig->Ns->Next)
            {
                This->pCurrentConfig->Ns->Next = (IP_ADDR*)CoTaskMemAlloc(sizeof(IP_ADDR));
                if (!This->pCurrentConfig->Ns->Next)
                   return E_OUTOFMEMORY;
                ZeroMemory(This->pCurrentConfig->Ns->Next, sizeof(IP_ADDR));
                This->pCurrentConfig->Ns->Next->IpAddress = dwIpAddr;
            }
            else
            {
                This->pCurrentConfig->Ns->Next->IpAddress = dwIpAddr;
            }
        }
        else
        {
            if (This->pCurrentConfig->Ns && This->pCurrentConfig->Ns->Next)
            {
                if (This->pCurrentConfig->Ns->Next->Next)
                {
                    IP_ADDR *pTemp = This->pCurrentConfig->Ns->Next->Next;
                    CoTaskMemFree(This->pCurrentConfig->Ns->Next);
                    This->pCurrentConfig->Ns->Next = pTemp;
                }
                else
                {
                    CoTaskMemFree(This->pCurrentConfig->Ns->Next);
                    This->pCurrentConfig->Ns->Next = NULL;
                }
            }
        }
    }
    else
    {
        This->pCurrentConfig->AutoconfigActive = TRUE;
    }
    return S_OK;
}

HRESULT
InitializeTcpipBasicDlgCtrls(
    HWND hwndDlg,
    TcpipSettings * pCurSettings)
{
    SendDlgItemMessageW(hwndDlg, IDC_IPADDR, IPM_SETRANGE, 0, MAKEIPRANGE(1, 223));
    SendDlgItemMessageW(hwndDlg, IDC_IPADDR, IPM_SETRANGE, 1, MAKEIPRANGE(0, 255));
    SendDlgItemMessageW(hwndDlg, IDC_IPADDR, IPM_SETRANGE, 2, MAKEIPRANGE(0, 255));
    SendDlgItemMessageW(hwndDlg, IDC_IPADDR, IPM_SETRANGE, 3, MAKEIPRANGE(0, 255));

    SendDlgItemMessageW(hwndDlg, IDC_SUBNETMASK, IPM_SETRANGE, 0, MAKEIPRANGE(0, 255));
    SendDlgItemMessageW(hwndDlg, IDC_SUBNETMASK, IPM_SETRANGE, 1, MAKEIPRANGE(0, 255));
    SendDlgItemMessageW(hwndDlg, IDC_SUBNETMASK, IPM_SETRANGE, 2, MAKEIPRANGE(0, 255));
    SendDlgItemMessageW(hwndDlg, IDC_SUBNETMASK, IPM_SETRANGE, 3, MAKEIPRANGE(0, 255));

    SendDlgItemMessageW(hwndDlg, IDC_DEFGATEWAY, IPM_SETRANGE, 0, MAKEIPRANGE(1, 223));
    SendDlgItemMessageW(hwndDlg, IDC_DEFGATEWAY, IPM_SETRANGE, 1, MAKEIPRANGE(0, 255));
    SendDlgItemMessageW(hwndDlg, IDC_DEFGATEWAY, IPM_SETRANGE, 2, MAKEIPRANGE(0, 255));
    SendDlgItemMessageW(hwndDlg, IDC_DEFGATEWAY, IPM_SETRANGE, 3, MAKEIPRANGE(0, 255));

    SendDlgItemMessageW(hwndDlg, IDC_DNS1, IPM_SETRANGE, 0, MAKEIPRANGE(1, 223));
    SendDlgItemMessageW(hwndDlg, IDC_DNS1, IPM_SETRANGE, 1, MAKEIPRANGE(0, 255));
    SendDlgItemMessageW(hwndDlg, IDC_DNS1, IPM_SETRANGE, 2, MAKEIPRANGE(0, 255));
    SendDlgItemMessageW(hwndDlg, IDC_DNS1, IPM_SETRANGE, 3, MAKEIPRANGE(0, 255));

    SendDlgItemMessageW(hwndDlg, IDC_DNS2, IPM_SETRANGE, 0, MAKEIPRANGE(1, 223));
    SendDlgItemMessageW(hwndDlg, IDC_DNS2, IPM_SETRANGE, 1, MAKEIPRANGE(0, 255));
    SendDlgItemMessageW(hwndDlg, IDC_DNS2, IPM_SETRANGE, 2, MAKEIPRANGE(0, 255));
    SendDlgItemMessageW(hwndDlg, IDC_DNS2, IPM_SETRANGE, 3, MAKEIPRANGE(0, 255));

    if (pCurSettings->DhcpEnabled)
    {
        SendDlgItemMessageW(hwndDlg, IDC_USEDHCP, BM_SETCHECK, BST_CHECKED, 0);
        EnableWindow(GetDlgItem(hwndDlg, IDC_IPADDR), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SUBNETMASK), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_DEFGATEWAY), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_AUTODNS), TRUE);
    }
    else
    {
        SendDlgItemMessageW(hwndDlg, IDC_NODHCP, BM_SETCHECK, BST_CHECKED, 0);

        if (pCurSettings->Ip)
        {
            /* set current ip address */
            SendDlgItemMessageA(hwndDlg, IDC_IPADDR, IPM_SETADDRESS, 0, (LPARAM)pCurSettings->Ip->IpAddress);
            /* set current hostmask */
            SendDlgItemMessageA(hwndDlg, IDC_SUBNETMASK, IPM_SETADDRESS, 0, (LPARAM)pCurSettings->Ip->u.Subnetmask);
        }
    }

    if (pCurSettings->Gw && pCurSettings->Gw->IpAddress)
    {
        /* set current gateway */
        SendDlgItemMessageA(hwndDlg, IDC_DEFGATEWAY, IPM_SETADDRESS, 0, (LPARAM)pCurSettings->Gw->IpAddress);
    }

    if (pCurSettings->AutoconfigActive)
    {
        SendDlgItemMessageW(hwndDlg, IDC_AUTODNS, BM_SETCHECK, BST_CHECKED, 0);
        EnableWindow(GetDlgItem(hwndDlg, IDC_DNS1), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_DNS2), FALSE);
    }
    else
    {
        SendDlgItemMessageW(hwndDlg, IDC_FIXEDDNS, BM_SETCHECK, BST_CHECKED, 0);
        EnableWindow(GetDlgItem(hwndDlg, IDC_DNS1), TRUE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_DNS2), TRUE);
        if (pCurSettings->Ns)
        {
            SendDlgItemMessageW(hwndDlg, IDC_DNS1, IPM_SETADDRESS, 0, (LPARAM)pCurSettings->Ns->IpAddress);
            if (pCurSettings->Ns->Next)
            {
                SendDlgItemMessageW(hwndDlg, IDC_DNS2, IPM_SETADDRESS, 0, (LPARAM)pCurSettings->Ns->Next->IpAddress);
            }
            else
            {
                SendDlgItemMessageW(hwndDlg, IDC_DNS2, IPM_CLEARADDRESS, 0, 0);
            }
        }
        else
        {
            SendDlgItemMessageW(hwndDlg, IDC_DNS1, IPM_CLEARADDRESS, 0, 0);
            SendDlgItemMessageW(hwndDlg, IDC_DNS2, IPM_CLEARADDRESS, 0, 0);
        }
    }

    return S_OK;
}

HRESULT
CopyIpAddrString(
    IP_ADDR_STRING * pSrc,
    IP_ADDR ** pTarget,
    COPY_TYPE Type,
    LPWSTR szMetric)
{
    IP_ADDR_STRING * pCurrent;
    IP_ADDR *pNew, *pLast;

    pCurrent = pSrc;
    pLast = NULL;

    while(pCurrent)
    {
        pNew = CoTaskMemAlloc(sizeof(IP_ADDR));
        if (!pNew)
        {
           break;
        }
        ZeroMemory(pNew, sizeof(IP_ADDR));
        pNew->IpAddress = GetIpAddressFromStringA(pCurrent->IpAddress.String);
        if (!pNew->IpAddress)
        {
            CoTaskMemFree(pNew);
            return E_FAIL;
        }

       if (Type == SUBMASK)
       {
           pNew->u.Subnetmask = GetIpAddressFromStringA(pCurrent->IpMask.String);
           pNew->NTEContext = pCurrent->Context;
       }
       else if (Type == METRIC)
       {
           if (szMetric && szMetric[0] != L'\0')
           {
               pNew->u.Metric = _wtoi(szMetric);
               szMetric += wcslen(szMetric) + 1;
           }
       }

        if (!pLast)
            *pTarget = pNew;
        else
            pLast->Next = pNew;

        pLast = pNew;
        pCurrent = pCurrent->Next;

    }
    pLast->Next = NULL;
    return S_OK;
}



INT_PTR
CALLBACK
TcpipBasicDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    TcpipConfNotifyImpl * This;
    LPPROPSHEETPAGE page;
    LPNMIPADDRESS lpnmipa;
    LPPSHNOTIFY lppsn;
    DWORD dwIpAddr;


    switch(uMsg)
    {
        case WM_INITDIALOG:
            page = (LPPROPSHEETPAGE)lParam;
            This = (TcpipConfNotifyImpl*)page->lParam;
            if (This->pCurrentConfig)
                InitializeTcpipBasicDlgCtrls(hwndDlg, This->pCurrentConfig);
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)This);
            return TRUE;
        case WM_NOTIFY:
            lppsn = (LPPSHNOTIFY) lParam;
            lpnmipa = (LPNMIPADDRESS) lParam;
            if (lpnmipa->hdr.code == IPN_FIELDCHANGED)
            {
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                if (lpnmipa->hdr.idFrom == IDC_IPADDR)
                {
                    if (SendDlgItemMessageW(hwndDlg, IDC_IPADDR, IPM_GETADDRESS, 0, (LPARAM)&dwIpAddr) == 4)
                    {
                        if (dwIpAddr <= MAKEIPADDRESS(127, 255, 255, 255))
                            SendDlgItemMessageW(hwndDlg, IDC_SUBNETMASK, IPM_SETADDRESS, 0, (LPARAM)MAKEIPADDRESS(255, 0, 0, 0));
                        else if (dwIpAddr <= MAKEIPADDRESS(191, 255, 255, 255))
                            SendDlgItemMessageW(hwndDlg, IDC_SUBNETMASK, IPM_SETADDRESS, 0, (LPARAM)MAKEIPADDRESS(255, 255, 0, 0));
                        else if (dwIpAddr <= MAKEIPADDRESS(223, 255, 255, 255))
                            SendDlgItemMessageW(hwndDlg, IDC_SUBNETMASK, IPM_SETADDRESS, 0, (LPARAM)MAKEIPADDRESS(255, 255, 255, 0));
                     }
                }
            }
            else if (lppsn->hdr.code == PSN_APPLY)
            {
                This = (TcpipConfNotifyImpl*)GetWindowLongPtr(hwndDlg, DWLP_USER);
                if (SUCCEEDED(StoreTcpipBasicSettings(hwndDlg, This, TRUE)))
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                else
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_INVALID);

                return TRUE;
            }
            break;
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                switch (LOWORD(wParam))
                {
                    case IDC_USEDHCP:
                        if (SendDlgItemMessageW(hwndDlg, IDC_USEDHCP, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        {
                            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                            SendDlgItemMessageW(hwndDlg, IDC_IPADDR, IPM_CLEARADDRESS, 0, 0);
                            SendDlgItemMessageW(hwndDlg, IDC_SUBNETMASK, IPM_CLEARADDRESS, 0, 0);
                            SendDlgItemMessageW(hwndDlg, IDC_DEFGATEWAY, IPM_CLEARADDRESS, 0, 0);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_IPADDR), FALSE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_SUBNETMASK), FALSE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_DEFGATEWAY), FALSE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_AUTODNS), TRUE);
                            AddAlternativeDialog(GetParent(hwndDlg), (TcpipConfNotifyImpl*)GetWindowLongPtr(hwndDlg, DWLP_USER));
                        }
                        break;
                    case IDC_NODHCP:
                        if (SendDlgItemMessageW(hwndDlg, IDC_NODHCP, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        {
                            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_IPADDR), TRUE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_SUBNETMASK), TRUE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_DEFGATEWAY), TRUE);
                            if (SendDlgItemMessageW(hwndDlg, IDC_AUTODNS, BM_GETCHECK, 0, 0) == BST_CHECKED)
                            {
                                SendDlgItemMessageW(hwndDlg, IDC_AUTODNS, BM_SETCHECK, BST_UNCHECKED, 0);
                            }
                            EnableWindow(GetDlgItem(hwndDlg, IDC_AUTODNS), FALSE);
                            SendDlgItemMessageW(hwndDlg, IDC_FIXEDDNS, BM_SETCHECK, BST_CHECKED, 0);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_DNS1), TRUE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_DNS2), TRUE);
                            SendMessageW(GetParent(hwndDlg), PSM_REMOVEPAGE, 1, 0);
                        }
                        break;
                    case IDC_AUTODNS:
                        if (SendDlgItemMessageW(hwndDlg, IDC_AUTODNS, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        {
                            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                            SendDlgItemMessageW(hwndDlg, IDC_DNS1, IPM_CLEARADDRESS, 0, 0);
                            SendDlgItemMessageW(hwndDlg, IDC_DNS2, IPM_CLEARADDRESS, 0, 0);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_DNS1), FALSE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_DNS2), FALSE);
                        }
                        break;
                    case IDC_FIXEDDNS:
                        if (SendDlgItemMessageW(hwndDlg, IDC_FIXEDDNS, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        {
                            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_DNS1), TRUE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_DNS2), TRUE);
                        }
                        break;
                    case IDC_ADVANCED:
                        LaunchAdvancedTcpipSettings(hwndDlg, (TcpipConfNotifyImpl*)GetWindowLongPtr(hwndDlg, DWLP_USER));
                        break;
                }
                break;
            }
        default:
            break;

    }
    return FALSE;
}

/***************************************************************
 * INetCfgComponentPropertyUi interface
 */

HRESULT
WINAPI
INetCfgComponentPropertyUi_fnQueryInterface(
    INetCfgComponentPropertyUi * iface,
    REFIID iid,
    LPVOID * ppvObj)
{
    //LPOLESTR pStr;
    TcpipConfNotifyImpl * This = (TcpipConfNotifyImpl*)iface;

    *ppvObj = NULL;


    if (IsEqualIID (iid, &IID_IUnknown) ||
        IsEqualIID (iid, &IID_INetCfgComponentPropertyUi))
    {
        *ppvObj = This;
        INetCfgComponentPropertyUi_AddRef(iface);
        return S_OK;
    }
    else if (IsEqualIID(iid, &IID_INetCfgComponentControl))
    {
        *ppvObj = (LPVOID*)&This->lpVtblCompControl;
        INetCfgComponentPropertyUi_AddRef(iface);
        return S_OK;
    }

    //StringFromCLSID(iid, &pStr);
    //MessageBoxW(NULL, pStr, L"INetConnectionPropertyUi_fnQueryInterface", MB_OK);
    //CoTaskMemFree(pStr);

    return E_NOINTERFACE;
}


ULONG
WINAPI
INetCfgComponentPropertyUi_fnAddRef(
    INetCfgComponentPropertyUi * iface)
{
    TcpipConfNotifyImpl * This = (TcpipConfNotifyImpl*)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    return refCount;
}

ULONG
WINAPI
INetCfgComponentPropertyUi_fnRelease(
    INetCfgComponentPropertyUi * iface)
{
    TcpipConfNotifyImpl * This = (TcpipConfNotifyImpl*)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    if (!refCount)
    {
       CoTaskMemFree(This);
    }
    return refCount;
}

HRESULT
WINAPI
INetCfgComponentPropertyUi_fnQueryPropertyUi(
    INetCfgComponentPropertyUi * iface,
    IUnknown *pUnkReserved)
{
    INetLanConnectionUiInfo * pLanInfo;
    HRESULT hr;
    TcpipConfNotifyImpl * This = (TcpipConfNotifyImpl*)iface;

    hr = IUnknown_QueryInterface(pUnkReserved, &IID_INetLanConnectionUiInfo, (LPVOID*)&pLanInfo);
    if (FAILED(hr))
        return hr;

    INetLanConnectionUiInfo_GetDeviceGuid(pLanInfo, &This->NetCfgInstanceId);

    //FIXME
    // check if tcpip is enabled on that binding */
    IUnknown_Release(pUnkReserved);
    return S_OK;
}

HRESULT
WINAPI
INetCfgComponentPropertyUi_fnSetContext(
    INetCfgComponentPropertyUi * iface,
    IUnknown *pUnkReserved)
{
    TcpipConfNotifyImpl * This = (TcpipConfNotifyImpl*)iface;

    if (!iface || !pUnkReserved)
        return E_POINTER;

    This->pUnknown = pUnkReserved;
    return S_OK;
}

HRESULT
LoadDNSSettings(
    TcpipConfNotifyImpl * This)
{
    LPOLESTR pStr;
    WCHAR szBuffer[200];
    HKEY hKey;
    DWORD dwSize;


    This->pCurrentConfig->pDNS = (TcpipAdvancedDNSDlgSettings*) CoTaskMemAlloc(sizeof(TcpipAdvancedDNSDlgSettings));
    if (!This->pCurrentConfig->pDNS)
        return E_FAIL;

    ZeroMemory(This->pCurrentConfig->pDNS, sizeof(TcpipAdvancedDNSDlgSettings));

    if (FAILED(StringFromCLSID(&This->NetCfgInstanceId, &pStr)))
        return E_FAIL;

    swprintf(szBuffer, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\%s", pStr);
    CoTaskMemFree(pStr);
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szBuffer, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(DWORD);
        RegQueryValueExW(hKey, L"RegisterAdapterName", NULL, NULL, (LPBYTE)&This->pCurrentConfig->pDNS->RegisterAdapterName, &dwSize);

        dwSize = sizeof(DWORD);
        RegQueryValueExW(hKey, L"RegistrationEnabled", NULL, NULL, (LPBYTE)&This->pCurrentConfig->pDNS->RegistrationEnabled, &dwSize);

        dwSize = sizeof(This->pCurrentConfig->pDNS->szDomain);
        RegQueryValueExW(hKey, L"Domain", NULL, NULL, (LPBYTE)This->pCurrentConfig->pDNS->szDomain, &dwSize);

        RegCloseKey(hKey);
    }

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(DWORD);
        RegQueryValueExW(hKey, L"UseDomainNameDevolution", NULL, NULL, (LPBYTE)&This->pCurrentConfig->pDNS->UseDomainNameDevolution, &dwSize);

        dwSize = 0;
        if (RegQueryValueExW(hKey, L"SearchList", NULL, NULL, NULL, &dwSize) == ERROR_SUCCESS)
        {
            This->pCurrentConfig->pDNS->szSearchList = (LPWSTR)CoTaskMemAlloc(dwSize);
            if (This->pCurrentConfig->pDNS->szSearchList)
            {
                if (RegQueryValueExW(hKey, L"SearchList", NULL, NULL, (LPBYTE)This->pCurrentConfig->pDNS->szSearchList, &dwSize) != ERROR_SUCCESS)
                {
                    CoTaskMemFree(This->pCurrentConfig->pDNS->szSearchList);
                    This->pCurrentConfig->pDNS->szSearchList = NULL;
                }
            }
        }
        RegCloseKey(hKey);
    }
    return S_OK;
}

LPWSTR
LoadTcpFilterSettingsFromRegistry(HKEY hKey, LPCWSTR szName, LPDWORD Size)
{
    DWORD dwSize;
    LPWSTR pData;

    if (RegQueryValueExW(hKey, szName, NULL, NULL, NULL, &dwSize) != ERROR_SUCCESS)
        return NULL;

    pData = (LPWSTR)CoTaskMemAlloc(dwSize);
    if (!pData)
        return NULL;

    if (RegQueryValueExW(hKey, szName, NULL, NULL, (LPBYTE)pData, &dwSize) != ERROR_SUCCESS)
    {
        CoTaskMemFree(pData);
        return NULL;
    }
    *Size = dwSize;
    return pData;
}

HRESULT
LoadFilterSettings(
    TcpipConfNotifyImpl * This)
{
    HKEY hKey;
    TcpFilterSettings *pFilter;
    WCHAR szBuffer[200];
    LPOLESTR pStr;
    DWORD dwVal, dwSize;

    pFilter = (TcpFilterSettings*)CoTaskMemAlloc(sizeof(TcpFilterSettings));
    if (!pFilter)
        return E_FAIL;

    ZeroMemory(pFilter, sizeof(TcpFilterSettings));
    This->pCurrentConfig->pFilter = pFilter;


    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"EnableSecurityFilters", NULL, NULL, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS)
            pFilter->EnableSecurityFilters = dwVal;
        RegCloseKey(hKey);
    }
    else
    {
        pFilter->EnableSecurityFilters = FALSE;
    }

    if (FAILED(StringFromCLSID(&This->NetCfgInstanceId, &pStr)))
        return E_FAIL;

    swprintf(szBuffer, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\%s", pStr);
    CoTaskMemFree(pStr);
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szBuffer, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        return S_OK;
    }
    pFilter->szTCPAllowedPorts = LoadTcpFilterSettingsFromRegistry(hKey, L"TCPAllowedPorts", &pFilter->TCPSize);
    pFilter->szUDPAllowedPorts = LoadTcpFilterSettingsFromRegistry(hKey, L"UDPAllowedPorts", &pFilter->UDPSize);
    pFilter->szRawIPAllowedProtocols = LoadTcpFilterSettingsFromRegistry(hKey, L"RawIPAllowedProtocols", &pFilter->IPSize);
    RegCloseKey(hKey);
    return S_OK;
}


HRESULT
Initialize(TcpipConfNotifyImpl * This)
{
    DWORD dwSize, Status;
    WCHAR szBuffer[50];
    IP_ADAPTER_INFO * pCurrentAdapter;
    IP_ADAPTER_INFO * pInfo;
    PIP_PER_ADAPTER_INFO pPerInfo;
    IP_PER_ADAPTER_INFO Info;
    LPOLESTR pStr;
    HRESULT hr;
    BOOL bFound;
    TcpipSettings * pCurSettings;
    ULONG uLength;

    if (This->pCurrentConfig)
        return S_OK;

    hr = StringFromCLSID(&This->NetCfgInstanceId, &pStr);
    if (FAILED(hr))
        return hr;


    dwSize = 0;
    if (GetAdaptersInfo(NULL, &dwSize) != ERROR_BUFFER_OVERFLOW)
    {
        CoTaskMemFree(pStr);
        return E_FAIL;
    }

    pInfo = CoTaskMemAlloc(dwSize);
    if (!pInfo)
    {
        CoTaskMemFree(pStr);
        return E_FAIL;
    }

    if (GetAdaptersInfo(pInfo, &dwSize) != ERROR_SUCCESS)
    {
        CoTaskMemFree(pStr);
        CoTaskMemFree(pInfo);
        return E_FAIL;
    }

    pCurrentAdapter = pInfo;
    bFound = FALSE;
    while(pCurrentAdapter)
    {
        szBuffer[0] = L'\0';
        if (MultiByteToWideChar(CP_ACP, 0, pCurrentAdapter->AdapterName, -1, szBuffer, sizeof(szBuffer)/sizeof(szBuffer[0])))
        {
            szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
        }
        if (!_wcsicmp(szBuffer, pStr))
        {
            bFound = TRUE;
            break;
        }
        pCurrentAdapter = pCurrentAdapter->Next;
    }
    CoTaskMemFree(pStr);

    if (!bFound)
    {
        CoTaskMemFree(pInfo);
        return E_FAIL;
    }

    pCurSettings = CoTaskMemAlloc(sizeof(TcpipSettings));
    if (!pCurSettings)
    {
        CoTaskMemFree(pInfo);
        return E_FAIL;
    }

    ZeroMemory(pCurSettings, sizeof(TcpipSettings));
    This->pCurrentConfig = pCurSettings;
    pCurSettings->DhcpEnabled = pCurrentAdapter->DhcpEnabled;
    pCurSettings->Index = pCurrentAdapter->Index;

    if (!pCurrentAdapter->DhcpEnabled)
    {
        CopyIpAddrString(&pCurrentAdapter->IpAddressList, &pCurSettings->Ip, SUBMASK, NULL);
    }

    CopyIpAddrString(&pCurrentAdapter->GatewayList, &pCurSettings->Gw, METRIC, NULL);

    uLength = sizeof(IP_PER_ADAPTER_INFO);
    ZeroMemory(&Info, sizeof(IP_PER_ADAPTER_INFO));

    if (GetPerAdapterInfo(pCurrentAdapter->Index, &Info, &uLength) == ERROR_BUFFER_OVERFLOW)
    {
        pPerInfo = (PIP_PER_ADAPTER_INFO)CoTaskMemAlloc(uLength);
        if (pPerInfo)
        {
            Status = GetPerAdapterInfo(pCurrentAdapter->Index, pPerInfo, &uLength);
            if (Status == NOERROR)
            {
                if (!pPerInfo->AutoconfigActive)
                {
                    CopyIpAddrString(&pPerInfo->DnsServerList, &pCurSettings->Ns, IPADDR, NULL);
                }
                pCurSettings->AutoconfigActive = pPerInfo->AutoconfigActive;
            }
            CoTaskMemFree(pPerInfo);
        }
    }
    else
    {
        if (!Info.AutoconfigActive)
        {
            CopyIpAddrString(&Info.DnsServerList, &pCurSettings->Ns, IPADDR, NULL);
        }
        pCurSettings->AutoconfigActive = Info.AutoconfigActive;
    }

    if (FAILED(LoadFilterSettings(This)))
        return E_FAIL;

    if (FAILED(LoadDNSSettings(This)))
        return E_FAIL;

    CoTaskMemFree(pInfo);

    return S_OK;
}

HRESULT
WINAPI
INetCfgComponentPropertyUi_fnMergePropPages(
    INetCfgComponentPropertyUi * iface,
    DWORD *pdwDefPages,
    BYTE **pahpspPrivate,
    UINT *pcPages,
    HWND hwndParent,
    LPCWSTR *pszStartPage)
{
    HPROPSHEETPAGE * hppages;
    UINT NumPages;
    HRESULT hr;
    TcpipConfNotifyImpl * This = (TcpipConfNotifyImpl*)iface;

    hr = Initialize(This);
    if (FAILED(hr))
        return hr;

    if (This->pCurrentConfig->DhcpEnabled)
        NumPages = 2;
    else
        NumPages = 1;

    hppages = (HPROPSHEETPAGE*) CoTaskMemAlloc(sizeof(HPROPSHEETPAGE) * NumPages);
    if (!hppages)
        return E_FAIL;

    hppages[0] = InitializePropertySheetPage(MAKEINTRESOURCEW(IDD_TCPIP_BASIC_DLG), TcpipBasicDlg, (LPARAM)This, NULL);
    if (!hppages[0])
    {
        CoTaskMemFree(hppages);
        return E_FAIL;
    }
    if (NumPages == 2)
    {
        hppages[1] = InitializePropertySheetPage(MAKEINTRESOURCEW(IDD_TCPIP_ALTCF_DLG), TcpipAltConfDlg, (LPARAM)This, NULL);
        if (!hppages[1])
        {
            DestroyPropertySheetPage(hppages[0]);
            CoTaskMemFree(hppages);
            return E_FAIL;
        }
    }

    *pahpspPrivate = (BYTE*)hppages;
    *pcPages = NumPages;

    return S_OK;
}

HRESULT
WINAPI
INetCfgComponentPropertyUi_fnValidateProperties(
    INetCfgComponentPropertyUi * iface,
    HWND hwndDlg)
{
MessageBoxW(NULL, L"INetCfgComponentPropertyUi_fnValidateProperties", NULL, MB_OK);
    return S_OK;
}

HRESULT
WINAPI
INetCfgComponentPropertyUi_fnApplyProperties(
    INetCfgComponentPropertyUi * iface)
{
MessageBoxW(NULL, L"INetCfgComponentPropertyUi_fnApplyProperties", NULL, MB_OK);
    return S_OK;
}


HRESULT
WINAPI
INetCfgComponentPropertyUi_fnCancelProperties(
    INetCfgComponentPropertyUi * iface)
{
//MessageBoxW(NULL, L"INetCfgComponentPropertyUi_fnCancelProperties", NULL, MB_OK);
    return S_OK;
}

static const INetCfgComponentPropertyUiVtbl vt_NetCfgComponentPropertyUi =
{
    INetCfgComponentPropertyUi_fnQueryInterface,
    INetCfgComponentPropertyUi_fnAddRef,
    INetCfgComponentPropertyUi_fnRelease,
    INetCfgComponentPropertyUi_fnQueryPropertyUi,
    INetCfgComponentPropertyUi_fnSetContext,
    INetCfgComponentPropertyUi_fnMergePropPages,
    INetCfgComponentPropertyUi_fnValidateProperties,
    INetCfgComponentPropertyUi_fnApplyProperties,
    INetCfgComponentPropertyUi_fnCancelProperties
};

/***************************************************************
 * INetCfgComponentControl interface
 */

HRESULT
WINAPI
INetCfgComponentControl_fnQueryInterface(
    INetCfgComponentControl * iface,
    REFIID iid,
    LPVOID * ppvObj)
{
    TcpipConfNotifyImpl * This = impl_from_INetCfgComponentControl(iface);
    return INetCfgComponentPropertyUi_QueryInterface((INetCfgComponentPropertyUi*)This, iid, ppvObj);
}

ULONG
WINAPI
INetCfgComponentControl_fnAddRef(
    INetCfgComponentControl * iface)
{
    TcpipConfNotifyImpl * This = impl_from_INetCfgComponentControl(iface);
    return INetCfgComponentPropertyUi_AddRef((INetCfgComponentPropertyUi*)This);
}

ULONG
WINAPI
INetCfgComponentControl_fnRelease(
    INetCfgComponentControl * iface)
{
    TcpipConfNotifyImpl * This = impl_from_INetCfgComponentControl(iface);
    return INetCfgComponentPropertyUi_Release((INetCfgComponentPropertyUi*)This);
}

HRESULT
WINAPI
INetCfgComponentControl_fnInitialize(
    INetCfgComponentControl * iface,
    INetCfgComponent *pIComp,
    INetCfg *pINetCfg,
    BOOL fInstalling)
{
    TcpipConfNotifyImpl * This = impl_from_INetCfgComponentControl(iface);

    This->pNCfg = pINetCfg;
    This->pNComp = pIComp;

    return S_OK;
}

static
LPWSTR
CreateMultiSzString(IP_ADDR * pAddr, COPY_TYPE Type, LPDWORD Size, BOOL bComma)
{
    LPWSTR pStr, pRet;
    DWORD dwSize, dwIpAddr;
    WCHAR szBuffer[30];
    IP_ADDR *pTemp = pAddr;


    dwSize = 0;
    while(pTemp)
    {
        if (Type == IPADDR)
        {
            dwIpAddr = pTemp->IpAddress;
            swprintf(szBuffer, L"%lu.%lu.%lu.%lu",
                    FIRST_IPADDRESS(dwIpAddr), SECOND_IPADDRESS(dwIpAddr), THIRD_IPADDRESS(dwIpAddr), FOURTH_IPADDRESS(dwIpAddr));
        }else if (Type == SUBMASK)
        {
            dwIpAddr = pTemp->u.Subnetmask;
            swprintf(szBuffer, L"%lu.%lu.%lu.%lu",
                    FIRST_IPADDRESS(dwIpAddr), SECOND_IPADDRESS(dwIpAddr), THIRD_IPADDRESS(dwIpAddr), FOURTH_IPADDRESS(dwIpAddr));
        }
        else if (Type == METRIC)
        {
            swprintf(szBuffer, L"%u", pTemp->u.Metric);
        }

        dwSize += wcslen(szBuffer) + 1;
        pTemp = pTemp->Next;
    }

    if (!dwSize)
        return NULL;

    pStr = pRet = CoTaskMemAlloc((dwSize+1) * sizeof(WCHAR));
    if(!pStr)
       return NULL;

    pTemp = pAddr;
    while(pTemp)
    {
        if (Type == IPADDR)
        {
            dwIpAddr = pTemp->IpAddress;
            swprintf(pStr, L"%lu.%lu.%lu.%lu",
                    FIRST_IPADDRESS(dwIpAddr), SECOND_IPADDRESS(dwIpAddr), THIRD_IPADDRESS(dwIpAddr), FOURTH_IPADDRESS(dwIpAddr));
        }else if (Type == SUBMASK)
        {
            dwIpAddr = pTemp->u.Subnetmask;
            swprintf(pStr, L"%lu.%lu.%lu.%lu",
                    FIRST_IPADDRESS(dwIpAddr), SECOND_IPADDRESS(dwIpAddr), THIRD_IPADDRESS(dwIpAddr), FOURTH_IPADDRESS(dwIpAddr));
        }
        else if (Type == METRIC)
        {
            swprintf(pStr, L"%u", pTemp->u.Metric);
        }

        if (bComma)
        {
            pStr += wcslen(pStr);
            if (pTemp->Next)
            {
                pStr[0] = L',';
                pStr++;
            }
        }
        else
        {
            pStr += wcslen(pStr) + 1;
        }
        pTemp = pTemp->Next;
    }
    pStr[0] = L'\0';
    *Size = (dwSize+1) * sizeof(WCHAR);
    return pRet;
}


HRESULT
WINAPI
INetCfgComponentControl_fnApplyRegistryChanges(
    INetCfgComponentControl * iface)
{
    HKEY hKey;
    LPOLESTR pStr;
    DWORD dwSize;
    WCHAR szBuffer[200];
    TcpipSettings * pCurrentConfig, *pOldConfig;
    ULONG NTEInstance;
    DWORD DhcpApiVersion;

    TcpipConfNotifyImpl * This = impl_from_INetCfgComponentControl(iface);

    pCurrentConfig = This->pCurrentConfig;
    This->pCurrentConfig = NULL;

    if (FAILED(Initialize(This)))
    {
        This->pCurrentConfig = pCurrentConfig;
        return E_FAIL;
    }
    pOldConfig = This->pCurrentConfig;
    This->pCurrentConfig = pCurrentConfig;

    //MessageBoxW(NULL, L"INetCfgComponentControl_fnApplyRegistryChanges", NULL, MB_OK);


    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        if (pCurrentConfig->pDNS)
        {
            RegSetValueExW(hKey, L"UseDomainNameDevolution", 0, REG_DWORD, (LPBYTE)&pCurrentConfig->pDNS->UseDomainNameDevolution, sizeof(DWORD));
            RegSetValueExW(hKey, L"SearchList", 0, REG_SZ, (LPBYTE)pCurrentConfig->pDNS->szSearchList,
                       (wcslen(pCurrentConfig->pDNS->szSearchList)+1) * sizeof(WCHAR));
        }
        if (pCurrentConfig->pFilter)
        {
            RegSetValueExW(hKey, L"EnableSecurityFilters", 0, REG_DWORD,
                      (LPBYTE)&pCurrentConfig->pFilter->EnableSecurityFilters, sizeof(DWORD));
        }
        RegCloseKey(hKey);
    }

    if (FAILED(StringFromCLSID(&This->NetCfgInstanceId, &pStr)))
        return E_FAIL;

    swprintf(szBuffer, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\%s", pStr);
    CoTaskMemFree(pStr);

    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, szBuffer, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        if (pCurrentConfig->pDNS)
        {
            RegSetValueExW(hKey, L"RegisterAdapterName", 0, REG_DWORD, (LPBYTE)&This->pCurrentConfig->pDNS->RegisterAdapterName, sizeof(DWORD));
            RegSetValueExW(hKey, L"RegistrationEnabled", 0, REG_DWORD, (LPBYTE)&This->pCurrentConfig->pDNS->RegistrationEnabled, sizeof(DWORD));
            RegSetValueExW(hKey, L"Domain", 0, REG_SZ, (LPBYTE)This->pCurrentConfig->pDNS->szDomain,
                       (wcslen(This->pCurrentConfig->pDNS->szDomain)+1) * sizeof(WCHAR));
        }
#if 0
        if (pCurrentConfig->pFilter)
        {
            if (pCurrentConfig->pFilter->szTCPAllowedPorts)
            {
                RegSetValueExW(hKey, L"TCPAllowedPorts", 0, REG_MULTI_SZ,
                       (LPBYTE)pCurrentConfig->pFilter->szTCPAllowedPorts,
                        pCurrentConfig->pFilter->TCPSize);
            }
            if (pCurrentConfig->pFilter->szUDPAllowedPorts)
            {
                RegSetValueExW(hKey, L"UDPAllowedPorts", 0, REG_MULTI_SZ,
                       (LPBYTE)pCurrentConfig->pFilter->szUDPAllowedPorts,
                        pCurrentConfig->pFilter->UDPSize);
            }
            if (pCurrentConfig->pFilter->szRawIPAllowedProtocols)
            {
                RegSetValueExW(hKey, L"RawIPAllowedProtocols", 0, REG_MULTI_SZ,
                       (LPBYTE)pCurrentConfig->pFilter->szRawIPAllowedProtocols,
                        pCurrentConfig->pFilter->IPSize);
            }
        }
#endif
        RegSetValueExW(hKey, L"EnableDHCP", 0, REG_DWORD, (LPBYTE)&pCurrentConfig->DhcpEnabled, sizeof(DWORD));
        if (pCurrentConfig->DhcpEnabled)
        {
            RegSetValueExW(hKey, L"IPAddress", 0, REG_MULTI_SZ, (LPBYTE)L"0.0.0.0\0", 9 * sizeof(WCHAR));
            RegSetValueExW(hKey, L"SubnetMask", 0, REG_MULTI_SZ, (LPBYTE)L"0.0.0.0\0", 9 * sizeof(WCHAR));
            if (!pOldConfig->DhcpEnabled)
            {
                /* Delete this adapter's current IP address */
                DeleteIPAddress(pOldConfig->Ip->NTEContext);

                /* Delete all default routes for this adapter */
                dwSize = 0;
                if (GetIpForwardTable(NULL, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER)
                {
                    DWORD Index;
                    PMIB_IPFORWARDTABLE pIpForwardTable = (PMIB_IPFORWARDTABLE)CoTaskMemAlloc(dwSize);
                    if (pIpForwardTable)
                    {
                        if (GetIpForwardTable(pIpForwardTable, &dwSize, FALSE) == NO_ERROR)
                        {
                            for (Index = 0; Index < pIpForwardTable->dwNumEntries; Index++)
                            {
                                if (pIpForwardTable->table[Index].dwForwardIfIndex == pOldConfig->Index &&
                                    pIpForwardTable->table[Index].dwForwardDest == 0)
                                {
                                    DeleteIpForwardEntry(&pIpForwardTable->table[Index]);
                                }
                            }
                        }
                        CoTaskMemFree(pIpForwardTable);
                    }
                }
            }
        }
        else
        {
            /* Open the DHCP API if DHCP is enabled */
            if (pOldConfig->DhcpEnabled && DhcpCApiInitialize(&DhcpApiVersion) == NO_ERROR)
            {
                /* We have to tell DHCP about this */
                DhcpStaticRefreshParams(pCurrentConfig->Index,
                                        htonl(pCurrentConfig->Ip->IpAddress),
                                        htonl(pCurrentConfig->Ip->u.Subnetmask));

                /* Close the API */
                DhcpCApiCleanup();
            }
            else
            {
                /* Delete this adapter's current static IP address */
                DeleteIPAddress(pOldConfig->Ip->NTEContext);

                /* Add the static IP address via the standard IPHLPAPI function */
                AddIPAddress(htonl(pCurrentConfig->Ip->IpAddress),
                             htonl(pCurrentConfig->Ip->u.Subnetmask),
                             pCurrentConfig->Index,
                             &pCurrentConfig->Ip->NTEContext,
                             &NTEInstance);
            }

            pStr = CreateMultiSzString(pCurrentConfig->Ip, IPADDR, &dwSize, FALSE);
            if(pStr)
            {
                RegSetValueExW(hKey, L"IPAddress", 0, REG_MULTI_SZ, (LPBYTE)pStr, dwSize);
                CoTaskMemFree(pStr);
            }

            pStr = CreateMultiSzString(pCurrentConfig->Ip, SUBMASK, &dwSize, FALSE);
            if(pStr)
            {
                RegSetValueExW(hKey, L"SubnetMask", 0, REG_MULTI_SZ, (LPBYTE)pStr, dwSize);
                CoTaskMemFree(pStr);
            }

            /* Delete all default routes for this adapter */
            dwSize = 0;
            if (GetIpForwardTable(NULL, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER)
            {
                DWORD Index;
                PMIB_IPFORWARDTABLE pIpForwardTable = (PMIB_IPFORWARDTABLE)CoTaskMemAlloc(dwSize);
                if (pIpForwardTable)
                {
                    if (GetIpForwardTable(pIpForwardTable, &dwSize, FALSE) == NO_ERROR)
                    {
                        for (Index = 0; Index < pIpForwardTable->dwNumEntries; Index++)
                        {
                            if (pIpForwardTable->table[Index].dwForwardIfIndex == pOldConfig->Index &&
                                pIpForwardTable->table[Index].dwForwardDest == 0)
                            {
                                DeleteIpForwardEntry(&pIpForwardTable->table[Index]);
                            }
                        }
                    }
                    CoTaskMemFree(pIpForwardTable);
                }
            }
        }

        if (pCurrentConfig->Gw)
        {
            MIB_IPFORWARDROW RouterMib;
            ZeroMemory(&RouterMib, sizeof(MIB_IPFORWARDROW));

            RouterMib.dwForwardMetric1 = 1;
            RouterMib.dwForwardIfIndex = pOldConfig->Index;
            RouterMib.dwForwardNextHop = htonl(pCurrentConfig->Gw->IpAddress);

            //TODO
            // add multiple gw addresses when required

            if (CreateIpForwardEntry(&RouterMib) == NO_ERROR)
            {
                pStr = CreateMultiSzString(pCurrentConfig->Gw, IPADDR, &dwSize, FALSE);
                if(pStr)
                {
                    RegSetValueExW(hKey, L"DefaultGateway", 0, REG_MULTI_SZ, (LPBYTE)pStr, dwSize);
                    CoTaskMemFree(pStr);
                }

                pStr = CreateMultiSzString(pCurrentConfig->Gw, METRIC, &dwSize, FALSE);
                if(pStr)
                {
                    RegSetValueExW(hKey, L"DefaultGatewayMetric", 0, REG_MULTI_SZ, (LPBYTE)pStr, dwSize);
                    CoTaskMemFree(pStr);
                }
            }
        }
        else
        {
            RegSetValueExW(hKey, L"DefaultGateway", 0, REG_MULTI_SZ, (LPBYTE)L"", 1 * sizeof(WCHAR));
            RegSetValueExW(hKey, L"DefaultGatewayMetric", 0, REG_MULTI_SZ, (LPBYTE)L"\0", sizeof(WCHAR) * 2);
        }

        if (!pCurrentConfig->Ns || pCurrentConfig->AutoconfigActive)
        {
            RegDeleteValueW(hKey, L"NameServer");
        }
        else
        {
            pStr = CreateMultiSzString(pCurrentConfig->Ns, IPADDR, &dwSize, TRUE);
            if(pStr)
            {
                RegSetValueExW(hKey, L"NameServer", 0, REG_SZ, (LPBYTE)pStr, dwSize);
                //RegDeleteValueW(hKey, L"DhcpNameServer");
                CoTaskMemFree(pStr);
            }
        }

        RegCloseKey(hKey);
    }
    return S_OK;
}

HRESULT
WINAPI
INetCfgComponentControl_fnApplyPnpChanges(
    INetCfgComponentControl * iface,
    INetCfgPnpReconfigCallback *pICallback)
{
    //MessageBoxW(NULL, L"INetCfgComponentControl_fnApplyPnpChanges", NULL, MB_OK);
    return S_OK;
}

HRESULT
WINAPI
INetCfgComponentControl_fnCancelChanges(
    INetCfgComponentControl * iface)
{
    //MessageBoxW(NULL, L"INetCfgComponentControl_fnCancelChanges", NULL, MB_OK);
    return S_OK;
}

static const INetCfgComponentControlVtbl vt_NetCfgComponentControl =
{
    INetCfgComponentControl_fnQueryInterface,
    INetCfgComponentControl_fnAddRef,
    INetCfgComponentControl_fnRelease,
    INetCfgComponentControl_fnInitialize,
    INetCfgComponentControl_fnApplyRegistryChanges,
    INetCfgComponentControl_fnApplyPnpChanges,
    INetCfgComponentControl_fnCancelChanges
};

HRESULT
WINAPI
TcpipConfigNotify_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv)
{
    TcpipConfNotifyImpl *This;

    if (!ppv)
        return E_POINTER;

    This = (TcpipConfNotifyImpl *) CoTaskMemAlloc(sizeof (TcpipConfNotifyImpl));
    if (!This)
        return E_OUTOFMEMORY;

    This->ref = 1;
    This->lpVtbl = (const INetCfgComponentPropertyUi*)&vt_NetCfgComponentPropertyUi;
    This->lpVtblCompControl = (const INetCfgComponentControl*)&vt_NetCfgComponentControl;
    This->pNCfg = NULL;
    This->pUnknown = NULL;
    This->pNComp = NULL;
    This->pCurrentConfig = NULL;

    if (!SUCCEEDED (INetCfgComponentPropertyUi_QueryInterface ((INetCfgComponentPropertyUi*)This, riid, ppv)))
    {
        INetCfgComponentPropertyUi_Release((INetCfgComponentPropertyUi*)This);
        return E_NOINTERFACE;
    }

    INetCfgComponentPropertyUi_Release((INetCfgComponentPropertyUi*)This);
    return S_OK;
}
