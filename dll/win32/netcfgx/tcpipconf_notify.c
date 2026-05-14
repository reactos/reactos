#include "precomp.h"

#include <winnls.h>
#include <winsock.h>
#include <iphlpapi.h>
#include <dhcpcsdk.h>
#include <dhcpcapi.h>

#include <inaddr.h>
#include <ip2string.h>
#include <ntstatus.h>

#define IP_STRING_MIN_LENGTH  8   /* 0.0.0.0 + NULL */
#define IP_STRING_MAX_LENGTH  16  /* xxx.xxx.xxx.xxx + NULL */

typedef DWORD (WINAPI *PDHCPFALLBACKREFRESHPARAMS)(PWSTR pAdapterName);

// KEY: Tcpip\Parameter\{InstanceGuid}\IpAddress | DhcpIpAddress
// KEY: Tcpip\Parameter\{InstanceGuid}\SubnetMask | DhcpSubnetMask
// KEY: Tcpip\Parameter\{InstanceGuid}\DefaultGateway | DhcpDefaultGateway
// KEY: Tcpip\Parameter\{InstanceGuid}\NameServer | DhcpNameServer
// KEY: Services\NetBT\Parameters\Interfaces\Tcpip_{INSTANCE_GUID}

typedef struct tagIP_ADDR
{
    DWORD IpAddress;
    union
    {
        DWORD SubnetMask;
        USHORT Metric;
    } u;
    ULONG NTEContext;
    struct tagIP_ADDR *Next;
} IP_ADDR;

typedef enum
{
    METRIC = 1,
    SUBMASK = 2,
    IPADDR = 3
} COPY_TYPE;

typedef struct
{
    DWORD IpAddress;
    DWORD SubnetMask;
    DWORD DefaultGateway;
    DWORD DnsServer1;
    DWORD DnsServer2;
} AlternateConfiguration;

typedef struct
{
    DWORD EnableSecurityFilters;

    DWORD UseDomainNameDevolution;
    LPWSTR szSearchList;

} TcpipSettings;

typedef struct _AdapterSettings
{
    struct _AdapterSettings *pPrev;
    struct _AdapterSettings *pNext;

    WCHAR AdapterName[45];
    GUID AdapterGuid;
    DWORD Index;

    IP_ADDR *OldIp;
    IP_ADDR *NewIp;
    IP_ADDR *OldGw;
    IP_ADDR *NewGw;
    IP_ADDR *OldNs;
    IP_ADDR *NewNs;

    UINT OldDhcpEnabled;
    UINT NewDhcpEnabled;
    UINT DnsDhcpEnabled;

    DWORD OldMetric;
    DWORD NewMetric;
    DWORD OldRegisterAdapterName;
    DWORD NewRegisterAdapterName;
    DWORD OldRegistrationEnabled;
    DWORD NewRegistrationEnabled;

    WCHAR szDomain[100];

    LPWSTR szTCPAllowedPorts;       // KEY: Tcpip\Parameter\{InstanceGuid}\TCPAllowedPorts
    LPWSTR szUDPAllowedPorts;       // KEY: Tcpip\Parameter\{InstanceGuid}\UDPAllowedPorts
    LPWSTR szRawIPAllowedProtocols; // KEY: Tcpip\Parameter\{InstanceGuid}\RawIPAllowedProtocols
    DWORD TCPSize;
    DWORD UDPSize;
    DWORD RawIPSize;

    AlternateConfiguration AltConfig;

} AdapterSettings;

typedef struct
{
    const INetCfgComponentControl *lpVtbl;
    const INetCfgComponentPropertyUi *lpVtblCompPropertyUi;
    const INetCfgComponentSetup *lpVtblCompSetup;
    const ITcpipProperties *lpVtblTcpipProperties;
    LONG ref;
    IUnknown *pUnknown;
    INetCfg *pNCfg;
    INetCfgComponent *pNComp;

    TcpipSettings *pTcpIpSettings;
    AdapterSettings *pAdapterListHead;
    AdapterSettings *pAdapterListTail;
    AdapterSettings *pCurrentAdapter;

} TcpipConfNotifyImpl, *LPTcpipConfNotifyImpl;

typedef struct
{
    BOOL bAdd;
    HWND hDlgCtrl;
    WCHAR szIP[16];
    UINT Metric;
} TcpipGwSettings;

typedef struct
{
    BOOL bAdd;
    HWND hDlgCtrl;
    WCHAR szIP[16];
    WCHAR szMask[16];
} TcpipIpSettings;

typedef struct
{
    BOOL bAdd;
    HWND hDlgCtrl;
    WCHAR szIP[16];
} TcpipDnsSettings;

typedef struct
{
    BOOL bAdd;
    HWND hDlgCtrl;
    LPWSTR Suffix;
} TcpipSuffixSettings;

typedef struct
{
    HWND hDlgCtrl;
    UINT ResId;
    UINT MaxNum;
} TcpipPortSettings;

static __inline LPTcpipConfNotifyImpl impl_from_INetCfgComponentPropertyUi(INetCfgComponentPropertyUi *iface)
{
    return (TcpipConfNotifyImpl*)((char *)iface - FIELD_OFFSET(TcpipConfNotifyImpl, lpVtblCompPropertyUi));
}

static __inline LPTcpipConfNotifyImpl impl_from_INetCfgComponentSetup(INetCfgComponentSetup *iface)
{
    return (TcpipConfNotifyImpl*)((char *)iface - FIELD_OFFSET(TcpipConfNotifyImpl, lpVtblCompSetup));
}

static __inline LPTcpipConfNotifyImpl impl_from_ITcpipProperties(ITcpipProperties *iface)
{
    return (TcpipConfNotifyImpl*)((char *)iface - FIELD_OFFSET(TcpipConfNotifyImpl, lpVtblTcpipProperties));
}

INT GetSelectedItem(HWND hDlgCtrl);
HRESULT InitializeTcpipBasicDlgCtrls(HWND hwndDlg, TcpipConfNotifyImpl *This);
VOID InsertColumnToListView(HWND hDlgCtrl, UINT ResId, UINT SubItem, UINT Size);
INT_PTR StoreTcpipBasicSettings(HWND hwndDlg, TcpipConfNotifyImpl *This, BOOL bApply);
HRESULT Initialize(TcpipConfNotifyImpl *This);
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

static
AdapterSettings*
GetAdapterByGuid(
    TcpipConfNotifyImpl *This,
    GUID *pAdapterGuid)
{
    AdapterSettings *pAdapter;

    pAdapter = This->pAdapterListHead;
    while (pAdapter)
    {
        if (IsEqualGUID(pAdapterGuid, &pAdapter->AdapterGuid))
            return pAdapter;

        pAdapter = pAdapter->pNext;
    }

    return NULL;
}

static
HRESULT
BuildIpAddressString(
    AdapterSettings *pAdapter,
    LPWSTR *ppszIpAddress)
{
    PWSTR pszIpAddress, pString;
    DWORD dwSize;
    IP_ADDR *pAddr;
    IN_ADDR Address;

    if (pAdapter->NewDhcpEnabled)
    {
        dwSize = IP_STRING_MIN_LENGTH;
    }
    else
    {
        dwSize = 0;
        pAddr = pAdapter->NewIp;
        while (pAddr != NULL)
        {
            dwSize++;
            pAddr = pAddr->Next;
        }

        dwSize *=  IP_STRING_MAX_LENGTH;
    }

    pszIpAddress = CoTaskMemAlloc(dwSize * sizeof(WCHAR));
    if (pszIpAddress == NULL)
        return E_OUTOFMEMORY;

    ZeroMemory(pszIpAddress, dwSize * sizeof(WCHAR));

    if (pAdapter->NewDhcpEnabled)
    {
        wcscpy(pszIpAddress, L"0.0.0.0");
    }
    else
    {
        pString = pszIpAddress;
        pAddr = pAdapter->NewIp;
        while (pAddr != NULL)
        {
            if (pString != pszIpAddress)
            {
                *pString = L',';
                pString++;
            }

            Address.S_un.S_addr = htonl(pAddr->IpAddress);
            pString = RtlIpv4AddressToStringW(&Address, pString);

            pAddr = pAddr->Next;
        }
    }

    *ppszIpAddress = pszIpAddress;

    return S_OK;
}

static
HRESULT
BuildSubnetMaskString(
    AdapterSettings *pAdapter,
    LPWSTR *ppszSubnetMask)
{
    PWSTR pszSubnetMask, pString;
    DWORD dwSize;
    IP_ADDR *pAddr;
    IN_ADDR Address;

    /* Build the SubnetMask string */
    if (pAdapter->NewDhcpEnabled)
    {
        dwSize = IP_STRING_MIN_LENGTH;
    }
    else
    {
        dwSize = 0;
        pAddr = pAdapter->NewIp;
        while (pAddr != NULL)
        {
            dwSize++;
            pAddr = pAddr->Next;
        }

        dwSize *= IP_STRING_MAX_LENGTH;
    }

    pszSubnetMask = CoTaskMemAlloc(dwSize * sizeof(WCHAR));
    if (pszSubnetMask == NULL)
        return E_OUTOFMEMORY;

    ZeroMemory(pszSubnetMask, dwSize * sizeof(WCHAR));

    if (pAdapter->NewDhcpEnabled)
    {
        wcscpy(pszSubnetMask, L"0.0.0.0");
    }
    else
    {
        pString = pszSubnetMask;
        pAddr = pAdapter->NewIp;
        while (pAddr != NULL)
        {
            if (pString != pszSubnetMask)
            {
                *pString = L',';
                pString++;
            }

            Address.S_un.S_addr = htonl(pAddr->u.SubnetMask);
            pString = RtlIpv4AddressToStringW(&Address, pString);

            pAddr = pAddr->Next;
        }
    }

    *ppszSubnetMask = pszSubnetMask;

    return S_OK;
}

static
HRESULT
BuildParametersString(
    AdapterSettings *pAdapter,
    LPWSTR *ppszParameters)
{
    PWSTR pszParameters, pString;
    PWSTR pszDefGateway = NULL;
    PWSTR pszGatewayMetric = NULL;
    PWSTR pszNameServers = NULL;
    DWORD dwCount;
    INT nLength;
    IP_ADDR *pAddr;
    IN_ADDR Address;
    HRESULT hr = S_OK;

    /* Format: "DefGw=;GwMetric=;IfMetric=0;DNS=;WINS=;DynamicUpdate=1;NameRegistration=0;" */

    /* Count Gateway entries */
    dwCount = 0;
    pAddr = pAdapter->NewGw;
    while (pAddr != NULL)
    {
        dwCount++;
        pAddr = pAddr->Next;
    }

    /* Build Default Gateway string */
    pszDefGateway = CoTaskMemAlloc(dwCount * IP_STRING_MAX_LENGTH * sizeof(WCHAR));
    if (pszDefGateway == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    ZeroMemory(pszDefGateway, dwCount * IP_STRING_MAX_LENGTH * sizeof(WCHAR));

    pString = pszDefGateway;
    pAddr = pAdapter->NewGw;
    while (pAddr != NULL)
    {
        if (pString != pszDefGateway)
        {
            *pString = L',';
            pString++;
        }

        Address.S_un.S_addr = htonl(pAddr->u.SubnetMask);
        pString = RtlIpv4AddressToStringW(&Address, pString);

        pAddr = pAddr->Next;
    }

    TRACE("DefaultGateway %S\n", pszDefGateway);

    /* Build Gateway Metric string */
    pszGatewayMetric = CoTaskMemAlloc(dwCount * 5 * sizeof(WCHAR));
    if (pszGatewayMetric == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    ZeroMemory(pszGatewayMetric, dwCount * 5 * sizeof(WCHAR));

    pString = pszGatewayMetric;
    pAddr = pAdapter->NewGw;
    while (pAddr != NULL)
    {
        if (pString != pszGatewayMetric)
        {
            *pString = L',';
            pString++;
        }

        nLength = _swprintf(pString, L"%hu", pAddr->u.Metric);
        pString += nLength;

        pAddr = pAddr->Next;
    }

    TRACE("Gateway Metric %S\n", pszGatewayMetric);

    /* Build the DNS string */
    dwCount = 0;
    pAddr = pAdapter->NewNs;
    while (pAddr != NULL)
    {
        dwCount++;
        pAddr = pAddr->Next;
    }

    pszNameServers = CoTaskMemAlloc(dwCount * IP_STRING_MAX_LENGTH * sizeof(WCHAR));
    if (pszNameServers == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    ZeroMemory(pszNameServers, dwCount * IP_STRING_MAX_LENGTH * sizeof(WCHAR));

    pString = pszNameServers;
    pAddr = pAdapter->NewNs;
    while (pAddr != NULL)
    {
        if (pString != pszNameServers)
        {
            *pString = L',';
            pString++;
        }

        Address.S_un.S_addr = htonl(pAddr->IpAddress);
        pString = RtlIpv4AddressToStringW(&Address, pString);

        pAddr = pAddr->Next;
    }

    TRACE("Name Servers %S\n", pszNameServers);

    /* Get the Parameters string length */
    nLength = _scwprintf(L"DefGw=%s;GwMetric=%s;IfMetric=%lu;DNS=%s;",
                         pszDefGateway,
                         pszGatewayMetric,
                         pAdapter->NewMetric,
                         pszNameServers);

    TRACE("Param string length %d\n", nLength);
    if (nLength == -1)
    {
        hr = E_FAIL;
        goto done;
    }

    /* Allocate the Parameters buffer */
    pszParameters = CoTaskMemAlloc((nLength + 1) * sizeof(WCHAR));
    if (pszParameters == NULL)
        return E_OUTOFMEMORY;

    ZeroMemory(pszParameters, (nLength + 1) * sizeof(WCHAR));

    /* Fill the Parameters buffer */
    _swprintf(pszParameters,
              L"DefGw=%s;GwMetric=%s;IfMetric=%lu;DNS=%s;",
              pszDefGateway,
              pszGatewayMetric,
              pAdapter->NewMetric,
              pszNameServers);

    TRACE("Parameters %S\n", pszParameters);

    *ppszParameters = pszParameters;

done:
    if (pszDefGateway)
        CoTaskMemFree(pszDefGateway);

    if (pszGatewayMetric)
        CoTaskMemFree(pszGatewayMetric);

    if (pszNameServers)
        CoTaskMemFree(pszNameServers);

    return hr;
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
        CheckDlgButton(hwndDlg, AllowButton, BST_CHECKED);
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
        CheckDlgButton(hwndDlg, AllowButton, BST_CHECKED);
    else
        CheckDlgButton(hwndDlg, RestrictButton, BST_CHECKED);
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

    switch(uMsg)
    {
        case WM_INITDIALOG:
            pContext = (TcpipConfNotifyImpl*)lParam;
            InsertColumnToListView(GetDlgItem(hwndDlg, IDC_TCP_LIST), IDS_TCP_PORTS, 0, 100);
            InsertColumnToListView(GetDlgItem(hwndDlg, IDC_UDP_LIST), IDS_UDP_PORTS, 0, 100);
            InsertColumnToListView(GetDlgItem(hwndDlg, IDC_IP_LIST), IDS_IP_PROTO, 0, 100);
            if (pContext->pCurrentAdapter)
            {
                InitFilterListBox(pContext->pCurrentAdapter->szTCPAllowedPorts, hwndDlg, GetDlgItem(hwndDlg, IDC_TCP_LIST), IDC_TCP_ALLOW_ALL, IDC_TCP_RESTRICT, IDC_TCP_ADD, IDC_TCP_DEL);
                InitFilterListBox(pContext->pCurrentAdapter->szUDPAllowedPorts, hwndDlg, GetDlgItem(hwndDlg, IDC_UDP_LIST), IDC_UDP_ALLOW_ALL, IDC_UDP_RESTRICT, IDC_UDP_ADD, IDC_UDP_DEL);
                InitFilterListBox(pContext->pCurrentAdapter->szRawIPAllowedProtocols, hwndDlg, GetDlgItem(hwndDlg, IDC_IP_LIST), IDC_IP_ALLOW_ALL, IDC_IP_RESTRICT, IDC_IP_ADD, IDC_IP_DEL);
                if (pContext->pTcpIpSettings->EnableSecurityFilters)
                    CheckDlgButton(hwndDlg, IDC_USE_FILTER, BST_CHECKED);
             }
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pContext);
            return TRUE;
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                switch (LOWORD(wParam))
                {
                    case IDC_TCP_ALLOW_ALL:
                        if (IsDlgButtonChecked(hwndDlg, IDC_TCP_ALLOW_ALL) == BST_CHECKED)
                        {
                            CheckDlgButton(hwndDlg, IDC_TCP_RESTRICT, BST_UNCHECKED);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_TCP_LIST), FALSE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_TCP_ADD), FALSE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_TCP_DEL), FALSE);
                        }
                        break;
                    case IDC_TCP_RESTRICT:
                        if (IsDlgButtonChecked(hwndDlg, IDC_TCP_RESTRICT) == BST_CHECKED)
                        {
                            CheckDlgButton(hwndDlg, IDC_TCP_ALLOW_ALL, BST_UNCHECKED);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_TCP_LIST), TRUE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_TCP_ADD), TRUE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_TCP_DEL), TRUE);
                        }
                        break;
                    case IDC_UDP_ALLOW_ALL:
                        if (IsDlgButtonChecked(hwndDlg, IDC_UDP_ALLOW_ALL) == BST_CHECKED)
                        {
                            CheckDlgButton(hwndDlg, IDC_UDP_RESTRICT, BST_UNCHECKED);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_UDP_LIST), FALSE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_UDP_ADD), FALSE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_UDP_DEL), FALSE);
                        }
                        break;
                    case IDC_UDP_RESTRICT:
                        if (IsDlgButtonChecked(hwndDlg, IDC_UDP_RESTRICT) == BST_CHECKED)
                        {
                            CheckDlgButton(hwndDlg, IDC_UDP_ALLOW_ALL, BST_UNCHECKED);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_UDP_LIST), TRUE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_UDP_ADD), TRUE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_UDP_DEL), TRUE);
                        }
                        break;
                    case IDC_IP_ALLOW_ALL:
                        if (IsDlgButtonChecked(hwndDlg, IDC_IP_ALLOW_ALL) == BST_CHECKED)
                        {
                            CheckDlgButton(hwndDlg, IDC_IP_RESTRICT, BST_UNCHECKED);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_IP_LIST), FALSE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_IP_ADD), FALSE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_IP_DEL), FALSE);
                        }
                        break;
                    case IDC_IP_RESTRICT:
                        if (IsDlgButtonChecked(hwndDlg, IDC_IP_RESTRICT) == BST_CHECKED)
                        {
                            CheckDlgButton(hwndDlg, IDC_IP_ALLOW_ALL, BST_UNCHECKED);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_IP_LIST), TRUE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_IP_ADD), TRUE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_IP_DEL), TRUE);
                        }
                        break;
                    case IDC_USE_FILTER:
                        if (IsDlgButtonChecked(hwndDlg, IDC_USE_FILTER) == BST_UNCHECKED)
                            DisplayError(IDS_DISABLE_FILTER, IDS_TCPIP, MB_OK);

                        break;
                }
            }
            switch(LOWORD(wParam))
            {
                case IDC_OK:
                    pContext = (TcpipConfNotifyImpl*)GetWindowLongPtr(hwndDlg, DWLP_USER);
                    if (IsDlgButtonChecked(hwndDlg, IDC_USE_FILTER) == BST_CHECKED)
                    {
                        pContext->pTcpIpSettings->EnableSecurityFilters = TRUE;
                        pContext->pCurrentAdapter->szTCPAllowedPorts = CreateFilterList(GetDlgItem(hwndDlg, IDC_TCP_LIST), &pContext->pCurrentAdapter->TCPSize);
                        pContext->pCurrentAdapter->szUDPAllowedPorts = CreateFilterList(GetDlgItem(hwndDlg, IDC_UDP_LIST), &pContext->pCurrentAdapter->UDPSize);
                        pContext->pCurrentAdapter->szRawIPAllowedProtocols = CreateFilterList(GetDlgItem(hwndDlg, IDC_IP_LIST), &pContext->pCurrentAdapter->RawIPSize);
                    }
                    else
                    {
                        pContext->pTcpIpSettings->EnableSecurityFilters = FALSE;
                        if (pContext->pCurrentAdapter->szTCPAllowedPorts)
                        {
                            CoTaskMemFree(pContext->pCurrentAdapter->szTCPAllowedPorts);
                            pContext->pCurrentAdapter->szTCPAllowedPorts = NULL;
                            pContext->pCurrentAdapter->TCPSize = 0;
                        }
                        if (pContext->pCurrentAdapter->szUDPAllowedPorts)
                        {
                            CoTaskMemFree(pContext->pCurrentAdapter->szUDPAllowedPorts);
                            pContext->pCurrentAdapter->szUDPAllowedPorts = NULL;
                            pContext->pCurrentAdapter->UDPSize = 0;
                        }
                        if (pContext->pCurrentAdapter->szRawIPAllowedProtocols)
                        {
                            CoTaskMemFree(pContext->pCurrentAdapter->szRawIPAllowedProtocols);
                            pContext->pCurrentAdapter->szRawIPAllowedProtocols = NULL;
                            pContext->pCurrentAdapter->RawIPSize = 0;
                        }
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
        _swprintf(szBuffer, L"%lu.%lu.%lu.%lu",
                  FIRST_IPADDRESS(dwIpAddr), SECOND_IPADDRESS(dwIpAddr), THIRD_IPADDRESS(dwIpAddr), FOURTH_IPADDRESS(dwIpAddr));

        li.pszText = szBuffer;
        li.iItem = SendMessageW(hDlgCtrl, LVM_INSERTITEMW, 0, (LPARAM)&li);
        if (li.iItem  != -1)
        {
            if (bSubMask)
            {
                dwIpAddr = pAddr->u.SubnetMask;
                _swprintf(szBuffer, L"%lu.%lu.%lu.%lu",
                          FIRST_IPADDRESS(dwIpAddr), SECOND_IPADDRESS(dwIpAddr), THIRD_IPADDRESS(dwIpAddr), FOURTH_IPADDRESS(dwIpAddr));
            }
            else
            {
                if (pAddr->u.Metric)
                    _swprintf(szBuffer, L"%u", pAddr->u.Metric);
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

    if (This->pCurrentAdapter->NewDhcpEnabled)
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
        InsertIpAddressToListView(GetDlgItem(hwndDlg, IDC_IPLIST), This->pCurrentAdapter->NewIp, TRUE);
    }

    EnableWindow(GetDlgItem(hwndDlg, IDC_IPMOD), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_IPDEL), FALSE);

    InsertColumnToListView(GetDlgItem(hwndDlg, IDC_GWLIST), IDS_GATEWAY, 0, 100);
    GetClientRect(GetDlgItem(hwndDlg, IDC_IPLIST), &rect);
    InsertColumnToListView(GetDlgItem(hwndDlg, IDC_GWLIST), IDS_METRIC, 1, (rect.right - rect.left - 100));

    InsertIpAddressToListView(GetDlgItem(hwndDlg, IDC_GWLIST), This->pCurrentAdapter->NewGw, FALSE);

    EnableWindow(GetDlgItem(hwndDlg, IDC_GWMOD), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_GWDEL), FALSE);

    SendDlgItemMessageW(hwndDlg, IDC_IFMETRIC, EM_LIMITTEXT, 4, 0);
    if (This->pCurrentAdapter->NewMetric)
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_IFMETRIC), TRUE);
        SetDlgItemInt(hwndDlg, IDC_IFMETRIC, This->pCurrentAdapter->NewMetric, FALSE);
    }
    else
    {
        CheckDlgButton(hwndDlg, IDC_IFAUTOMETRIC, BST_CHECKED);
        EnableWindow(GetDlgItem(hwndDlg, IDC_IFMETRIC), FALSE);
    }
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
                CheckDlgButton(hwndDlg, IDC_GWAUTOMETRIC, BST_CHECKED);
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
                    SetDlgItemInt(hwndDlg, IDC_GWMETRIC, pGwSettings->Metric, FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_GWMETRIC), TRUE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_GWMETRICTXT), TRUE);
                }
                else
                {
                    CheckDlgButton(hwndDlg, IDC_GWAUTOMETRIC, BST_CHECKED);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_GWMETRIC), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_GWMETRICTXT), FALSE);
                }
            }
            return TRUE;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_GWAUTOMETRIC)
            {
                if (IsDlgButtonChecked(hwndDlg, IDC_GWAUTOMETRIC) == BST_CHECKED)
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_GWMETRIC), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_GWMETRICTXT), FALSE);
                    SendDlgItemMessageW(hwndDlg, IDC_GWMETRIC, WM_SETTEXT, 0, (LPARAM)L"");
                }
                else
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_GWMETRIC), TRUE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_GWMETRICTXT), TRUE);
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

                    if (IsDlgButtonChecked(hwndDlg, IDC_GWAUTOMETRIC) == BST_UNCHECKED)
                        pGwSettings->Metric = GetDlgItemInt(hwndDlg, IDC_GWMETRIC, NULL, FALSE);
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
                            pSettings->Suffix = NULL;
                            break;
                        }

                        if (!VerifyDNSSuffix(pSettings->Suffix))
                        {
                            DisplayError(IDS_DOMAIN_SUFFIX, IDS_TCPIP, MB_ICONWARNING);
                            CoTaskMemFree(pSettings->Suffix);
                            pSettings->Suffix = NULL;
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
                    pCur->u.SubnetMask = GetIpAddressFromStringW(szBuffer);
                else
                    pCur->u.Metric  = _wtoi(szBuffer);
            }

            if (!pLast)
            {
                if (bSubmask)
                    This->pCurrentAdapter->NewIp = pCur;
                else
                    This->pCurrentAdapter->NewGw = pCur;
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
                              (!This->pCurrentAdapter->NewDhcpEnabled);

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
                if (!This->pCurrentAdapter->NewDhcpEnabled && ListView_GetItemCount(GetDlgItem(hwndDlg, IDC_IPLIST)) == 0)
                {
                    DisplayError(IDS_NO_IPADDR_SET, IDS_TCPIP, MB_ICONWARNING);
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }

                if (IsDlgButtonChecked(hwndDlg, IDC_IFAUTOMETRIC) != BST_CHECKED)
                {
                    UINT val = GetDlgItemInt(hwndDlg, IDC_IFMETRIC, NULL, FALSE);
                    if ((val < 1) || (val > 9999))
                    {
                        DisplayError(IDS_METRIC_RANGE, IDS_TCPIP, MB_ICONWARNING);
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                        return TRUE;
                    }
                }
            }
            else if (lppsn->hdr.code == PSN_APPLY)
            {
                This = (TcpipConfNotifyImpl*) GetWindowLongPtr(hwndDlg, DWLP_USER);
                FreeIPAddr(This->pCurrentAdapter->NewGw);
                This->pCurrentAdapter->NewGw = NULL;
                FreeIPAddr(This->pCurrentAdapter->NewIp);
                This->pCurrentAdapter->NewIp = NULL;
                StoreIPSettings(GetDlgItem(hwndDlg, IDC_IPLIST), This, TRUE);
                StoreIPSettings(GetDlgItem(hwndDlg, IDC_GWLIST), This, FALSE);

                if (IsDlgButtonChecked(hwndDlg, IDC_IFAUTOMETRIC) == BST_CHECKED)
                    This->pCurrentAdapter->NewMetric = 0;
                else
                    This->pCurrentAdapter->NewMetric = GetDlgItemInt(hwndDlg, IDC_IFMETRIC, NULL, FALSE);

                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                return TRUE;
            }
            break;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_IFAUTOMETRIC)
            {
                if (IsDlgButtonChecked(hwndDlg, IDC_IFAUTOMETRIC) == BST_CHECKED)
                    EnableWindow(GetDlgItem(hwndDlg, IDC_IFMETRIC), FALSE);
                else
                    EnableWindow(GetDlgItem(hwndDlg, IDC_IFMETRIC), TRUE);
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
                            _swprintf(szBuffer, L"%u", Gw.Metric);
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
                            _swprintf(szBuffer, L"%u", Gw.Metric);
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
            break;
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
    pAddr = This->pCurrentAdapter->NewNs;
    while(pAddr)
    {
        dwIpAddr = pAddr->IpAddress;
        _swprintf(szBuffer, L"%lu.%lu.%lu.%lu",
                  FIRST_IPADDRESS(dwIpAddr), SECOND_IPADDRESS(dwIpAddr), THIRD_IPADDRESS(dwIpAddr), FOURTH_IPADDRESS(dwIpAddr));

        SendDlgItemMessageW(hwndDlg, IDC_DNSADDRLIST, LB_ADDSTRING, 0, (LPARAM)szBuffer);
        pAddr = pAddr->Next;
    }
    SendDlgItemMessageW(hwndDlg, IDC_DNSADDRLIST, LB_SETCURSEL, 0, 0);

    if (This->pCurrentAdapter->NewRegisterAdapterName)
        CheckDlgButton(hwndDlg, IDC_REGSUFFIX, BST_CHECKED);
    else
        EnableWindow(GetDlgItem(hwndDlg, IDC_USESUFFIX), FALSE);

    if (This->pCurrentAdapter->NewRegistrationEnabled)
        CheckDlgButton(hwndDlg, IDC_USESUFFIX, BST_CHECKED);

    if (This->pCurrentAdapter->szDomain[0])
        SendDlgItemMessageW(hwndDlg, IDC_SUFFIX, WM_SETTEXT, 0, (LPARAM)szBuffer);

    if (This->pTcpIpSettings->UseDomainNameDevolution)
        CheckDlgButton(hwndDlg, IDC_TOPPRIMSUFFIX, BST_CHECKED);

    if (!This->pTcpIpSettings->szSearchList || (wcslen(This->pTcpIpSettings->szSearchList) == 0))
    {
        CheckDlgButton(hwndDlg, IDC_PRIMSUFFIX, BST_CHECKED);
        EnableWindow(GetDlgItem(hwndDlg, IDC_DNSSUFFIXADD), FALSE);

        return;
    }

    pList = This->pTcpIpSettings->szSearchList;
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
        CheckDlgButton(hwndDlg, IDC_SELSUFFIX, BST_CHECKED);
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

    FreeIPAddr(This->pCurrentAdapter->NewNs);
    This->pCurrentAdapter->NewNs = NULL;

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
            This->pCurrentAdapter->NewNs = pCur;
        else
            pLast->Next = pCur;

        pLast = pCur;
        pCur = pCur->Next;
    }
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
                if (IsDlgButtonChecked(hwndDlg, IDC_SELSUFFIX) == BST_CHECKED &&
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
                            _swprintf(szBuffer, szFormat, szSuffix);
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

                StoreDNSSettings(GetDlgItem(hwndDlg, IDC_DNSADDRLIST), This);
                if (IsDlgButtonChecked(hwndDlg, IDC_PRIMSUFFIX) == BST_CHECKED)
                {
                    CoTaskMemFree(This->pTcpIpSettings->szSearchList);
                    This->pTcpIpSettings->szSearchList = NULL;
                    if (IsDlgButtonChecked(hwndDlg, IDC_TOPPRIMSUFFIX) == BST_CHECKED)
                        This->pTcpIpSettings->UseDomainNameDevolution = TRUE;
                    else
                        This->pTcpIpSettings->UseDomainNameDevolution = FALSE;
                }
                else
                {
                    CoTaskMemFree(This->pTcpIpSettings->szSearchList);
                    This->pTcpIpSettings->szSearchList = NULL;
                    This->pTcpIpSettings->UseDomainNameDevolution = FALSE;
                    This->pTcpIpSettings->szSearchList = GetListViewEntries(GetDlgItem(hwndDlg, IDC_DNSSUFFIXLIST));
                }

                if (IsDlgButtonChecked(hwndDlg, IDC_REGSUFFIX) == BST_CHECKED)
                {
                    This->pCurrentAdapter->NewRegisterAdapterName = TRUE;
                    if (IsDlgButtonChecked(hwndDlg, IDC_USESUFFIX) == BST_CHECKED)
                        This->pCurrentAdapter->NewRegistrationEnabled = TRUE;
                    else
                        This->pCurrentAdapter->NewRegistrationEnabled = FALSE;
                }
                else
                {
                    This->pCurrentAdapter->NewRegisterAdapterName = FALSE;
                    This->pCurrentAdapter->NewRegistrationEnabled = FALSE;
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
        InitializeTcpipBasicDlgCtrls(hwndDlg, This);
        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
    }
}

INT_PTR
StoreTcpipAlternateSettings(
    HWND hwndDlg,
    TcpipConfNotifyImpl *This,
    BOOL bApply)
{
    DWORD dwAddr;

    if (IsDlgButtonChecked(hwndDlg, IDC_ALTSTATIC) == BST_CHECKED)
    {
        if (SendDlgItemMessageW(hwndDlg, IDC_ALTIPADDR, IPM_GETADDRESS, 0, (LPARAM)&dwAddr) != 4)
        {
            if (bApply)
            {
                DisplayError(IDS_NO_IPADDR_SET, IDS_TCPIP, MB_ICONWARNING);
                SetFocus(GetDlgItem(hwndDlg, IDC_ALTIPADDR));
                return E_FAIL;
            }
        }

        if (SendDlgItemMessageW(hwndDlg, IDC_ALTSUBNETMASK, IPM_GETADDRESS, 0, (LPARAM)&dwAddr) != 4)
        {
            if (bApply)
                DisplayError(IDS_NO_SUBMASK_SET, IDS_TCPIP, MB_ICONWARNING);
            if (SendDlgItemMessageW(hwndDlg, IDC_ALTIPADDR, IPM_GETADDRESS, 0, (LPARAM)&dwAddr) == 4)
            {
                if (dwAddr <= MAKEIPADDRESS(127, 255, 255, 255))
                    dwAddr = MAKEIPADDRESS(255, 0, 0, 0);
                else if (dwAddr <= MAKEIPADDRESS(191, 255, 255, 255))
                    dwAddr = MAKEIPADDRESS(255, 255, 0, 0);
                else if (dwAddr <= MAKEIPADDRESS(223, 255, 255, 255))
                    dwAddr = MAKEIPADDRESS(255, 255, 255, 0);

                SendDlgItemMessageW(hwndDlg, IDC_ALTSUBNETMASK, IPM_SETADDRESS, 0, (LPARAM)dwAddr);
            }
            if (bApply)
            {
                SetFocus(GetDlgItem(hwndDlg, IDC_ALTSUBNETMASK));
                return E_FAIL;
            }
        }

        if (SendDlgItemMessageW(hwndDlg, IDC_ALTIPADDR, IPM_GETADDRESS, 0, (LPARAM)&dwAddr) == 4)
            This->pCurrentAdapter->AltConfig.IpAddress = dwAddr;

        if (SendDlgItemMessageW(hwndDlg, IDC_ALTSUBNETMASK, IPM_GETADDRESS, 0, (LPARAM)&dwAddr) == 4)
            This->pCurrentAdapter->AltConfig.SubnetMask = dwAddr;

        if (SendDlgItemMessageW(hwndDlg, IDC_ALTDEFGATEWAY, IPM_GETADDRESS, 0, (LPARAM)&dwAddr) == 4)
            This->pCurrentAdapter->AltConfig.DefaultGateway = dwAddr;

        if (SendDlgItemMessageW(hwndDlg, IDC_ALTDNS1, IPM_GETADDRESS, 0, (LPARAM)&dwAddr) == 4)
            This->pCurrentAdapter->AltConfig.DnsServer1 = dwAddr;

        if (SendDlgItemMessageW(hwndDlg, IDC_ALTDNS2, IPM_GETADDRESS, 0, (LPARAM)&dwAddr) == 4)
            This->pCurrentAdapter->AltConfig.DnsServer2 = dwAddr;
    }
    else
    {
        ZeroMemory(&This->pCurrentAdapter->AltConfig, sizeof(AlternateConfiguration));
    }

    return S_OK;
}



HRESULT
InitializeTcpipAltDlgCtrls(
    HWND hwndDlg,
    TcpipConfNotifyImpl *This)
{
    SendDlgItemMessageW(hwndDlg, IDC_ALTIPADDR, IPM_SETRANGE, 0, MAKEIPRANGE(1, 223));
    SendDlgItemMessageW(hwndDlg, IDC_ALTIPADDR, IPM_SETRANGE, 1, MAKEIPRANGE(0, 255));
    SendDlgItemMessageW(hwndDlg, IDC_ALTIPADDR, IPM_SETRANGE, 2, MAKEIPRANGE(0, 255));
    SendDlgItemMessageW(hwndDlg, IDC_ALTIPADDR, IPM_SETRANGE, 3, MAKEIPRANGE(0, 255));

    SendDlgItemMessageW(hwndDlg, IDC_ALTSUBNETMASK, IPM_SETRANGE, 0, MAKEIPRANGE(0, 255));
    SendDlgItemMessageW(hwndDlg, IDC_ALTSUBNETMASK, IPM_SETRANGE, 1, MAKEIPRANGE(0, 255));
    SendDlgItemMessageW(hwndDlg, IDC_ALTSUBNETMASK, IPM_SETRANGE, 2, MAKEIPRANGE(0, 255));
    SendDlgItemMessageW(hwndDlg, IDC_ALTSUBNETMASK, IPM_SETRANGE, 3, MAKEIPRANGE(0, 255));

    SendDlgItemMessageW(hwndDlg, IDC_ALTDEFGATEWAY, IPM_SETRANGE, 0, MAKEIPRANGE(1, 223));
    SendDlgItemMessageW(hwndDlg, IDC_ALTDEFGATEWAY, IPM_SETRANGE, 1, MAKEIPRANGE(0, 255));
    SendDlgItemMessageW(hwndDlg, IDC_ALTDEFGATEWAY, IPM_SETRANGE, 2, MAKEIPRANGE(0, 255));
    SendDlgItemMessageW(hwndDlg, IDC_ALTDEFGATEWAY, IPM_SETRANGE, 3, MAKEIPRANGE(0, 255));

    SendDlgItemMessageW(hwndDlg, IDC_ALTDNS1, IPM_SETRANGE, 0, MAKEIPRANGE(1, 223));
    SendDlgItemMessageW(hwndDlg, IDC_ALTDNS1, IPM_SETRANGE, 1, MAKEIPRANGE(0, 255));
    SendDlgItemMessageW(hwndDlg, IDC_ALTDNS1, IPM_SETRANGE, 2, MAKEIPRANGE(0, 255));
    SendDlgItemMessageW(hwndDlg, IDC_ALTDNS1, IPM_SETRANGE, 3, MAKEIPRANGE(0, 255));

    SendDlgItemMessageW(hwndDlg, IDC_ALTDNS2, IPM_SETRANGE, 0, MAKEIPRANGE(1, 223));
    SendDlgItemMessageW(hwndDlg, IDC_ALTDNS2, IPM_SETRANGE, 1, MAKEIPRANGE(0, 255));
    SendDlgItemMessageW(hwndDlg, IDC_ALTDNS2, IPM_SETRANGE, 2, MAKEIPRANGE(0, 255));
    SendDlgItemMessageW(hwndDlg, IDC_ALTDNS2, IPM_SETRANGE, 3, MAKEIPRANGE(0, 255));

    if (This->pCurrentAdapter->AltConfig.IpAddress == 0)
    {
        CheckRadioButton(hwndDlg, IDC_ALTAPIPA, IDC_ALTSTATIC, IDC_ALTAPIPA);
        EnableWindow(GetDlgItem(hwndDlg, IDC_ALTIPADDR), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_ALTSUBNETMASK), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_ALTDEFGATEWAY), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_ALTDNS1), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_ALTDNS2), FALSE);
    }
    else
    {
        CheckRadioButton(hwndDlg, IDC_ALTAPIPA, IDC_ALTSTATIC, IDC_ALTSTATIC);

        /* Set ip address */
        if (This->pCurrentAdapter->AltConfig.IpAddress)
            SendDlgItemMessageW(hwndDlg, IDC_ALTIPADDR, IPM_SETADDRESS, 0, (LPARAM)This->pCurrentAdapter->AltConfig.IpAddress);

        /* Set subnet mask */
        if (This->pCurrentAdapter->AltConfig.SubnetMask)
            SendDlgItemMessageW(hwndDlg, IDC_ALTSUBNETMASK, IPM_SETADDRESS, 0, (LPARAM)This->pCurrentAdapter->AltConfig.SubnetMask);

        /* Set default gateway */
        if (This->pCurrentAdapter->AltConfig.DefaultGateway)
            SendDlgItemMessageW(hwndDlg, IDC_ALTDEFGATEWAY, IPM_SETADDRESS, 0, (LPARAM)This->pCurrentAdapter->AltConfig.DefaultGateway);

        /* Set primary dns server */
        if (This->pCurrentAdapter->AltConfig.DnsServer1)
            SendDlgItemMessageW(hwndDlg, IDC_ALTDNS1, IPM_SETADDRESS, 0, (LPARAM)This->pCurrentAdapter->AltConfig.DnsServer1);

        /* Set secondary dns server */
        if (This->pCurrentAdapter->AltConfig.DnsServer2)
            SendDlgItemMessageW(hwndDlg, IDC_ALTDNS2, IPM_SETADDRESS, 0, (LPARAM)This->pCurrentAdapter->AltConfig.DnsServer2);
    }

    return S_OK;
}

INT_PTR
CALLBACK
TcpipAltConfDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    TcpipConfNotifyImpl *This;
    LPPROPSHEETPAGE page;
    LPNMIPADDRESS lpnmipa;
    LPPSHNOTIFY lppsn;

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            page = (LPPROPSHEETPAGE)lParam;
            This = (TcpipConfNotifyImpl*)page->lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)This);
            InitializeTcpipAltDlgCtrls(hwndDlg, This);
            return TRUE;
        }
        case WM_NOTIFY:
            lppsn = (LPPSHNOTIFY) lParam;
            lpnmipa = (LPNMIPADDRESS) lParam;
            if (lpnmipa->hdr.code == IPN_FIELDCHANGED)
            {
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                if (lpnmipa->hdr.idFrom == IDC_ALTIPADDR)
                {
                    DWORD dwIpAddr;

                    if (SendDlgItemMessageW(hwndDlg, IDC_ALTIPADDR, IPM_GETADDRESS, 0, (LPARAM)&dwIpAddr) == 4)
                    {
                        if (dwIpAddr <= MAKEIPADDRESS(127, 255, 255, 255))
                            SendDlgItemMessageW(hwndDlg, IDC_ALTSUBNETMASK, IPM_SETADDRESS, 0, (LPARAM)MAKEIPADDRESS(255, 0, 0, 0));
                        else if (dwIpAddr <= MAKEIPADDRESS(191, 255, 255, 255))
                            SendDlgItemMessageW(hwndDlg, IDC_ALTSUBNETMASK, IPM_SETADDRESS, 0, (LPARAM)MAKEIPADDRESS(255, 255, 0, 0));
                        else if (dwIpAddr <= MAKEIPADDRESS(223, 255, 255, 255))
                            SendDlgItemMessageW(hwndDlg, IDC_ALTSUBNETMASK, IPM_SETADDRESS, 0, (LPARAM)MAKEIPADDRESS(255, 255, 255, 0));
                     }
                }
            }
            else if (lppsn->hdr.code == PSN_APPLY)
            {
                This = (TcpipConfNotifyImpl*)GetWindowLongPtr(hwndDlg, DWLP_USER);
                if (SUCCEEDED(StoreTcpipAlternateSettings(hwndDlg, This, TRUE)))
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                else
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_INVALID);

                return TRUE;
            }
            break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_ALTAPIPA:
                case IDC_ALTSTATIC:
                {
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        BOOL bStatic = (IsDlgButtonChecked(hwndDlg, IDC_ALTSTATIC) == BST_CHECKED);
                        if (bStatic)
                        {
                            SendDlgItemMessageW(hwndDlg, IDC_ALTIPADDR, IPM_CLEARADDRESS, 0, 0);
                            SendDlgItemMessageW(hwndDlg, IDC_ALTSUBNETMASK, IPM_CLEARADDRESS, 0, 0);
                            SendDlgItemMessageW(hwndDlg, IDC_ALTDEFGATEWAY, IPM_CLEARADDRESS, 0, 0);
                            SendDlgItemMessageW(hwndDlg, IDC_ALTDNS1, IPM_CLEARADDRESS, 0, 0);
                            SendDlgItemMessageW(hwndDlg, IDC_ALTDNS2, IPM_CLEARADDRESS, 0, 0);
                        }

                        EnableWindow(GetDlgItem(hwndDlg, IDC_ALTIPADDR), bStatic);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_ALTSUBNETMASK), bStatic);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_ALTDEFGATEWAY), bStatic);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_ALTDNS1), bStatic);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_ALTDNS2), bStatic);

                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
                }
            }
            break;
        }
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

    if (IsDlgButtonChecked(hwndDlg, IDC_NODHCP) == BST_CHECKED)
    {
        This->pCurrentAdapter->NewDhcpEnabled = FALSE;

        if (SendDlgItemMessageW(hwndDlg, IDC_IPADDR, IPM_GETADDRESS, 0, (LPARAM)&dwIpAddr) != 4)
        {
            if (bApply)
            {
                DisplayError(IDS_NO_IPADDR_SET, IDS_TCPIP, MB_ICONWARNING);
                SetFocus(GetDlgItem(hwndDlg, IDC_IPADDR));
                return E_FAIL;
            }
        }
        if (!This->pCurrentAdapter->NewIp)
        {
            This->pCurrentAdapter->NewIp = (IP_ADDR*)CoTaskMemAlloc(sizeof(IP_ADDR));
            if (!This->pCurrentAdapter->NewIp)
                return E_OUTOFMEMORY;
            ZeroMemory(This->pCurrentAdapter->NewIp, sizeof(IP_ADDR));
        }
        This->pCurrentAdapter->NewIp->IpAddress = dwIpAddr;

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
        This->pCurrentAdapter->NewIp->u.SubnetMask = dwIpAddr;

        if (SendDlgItemMessageW(hwndDlg, IDC_DEFGATEWAY, IPM_GETADDRESS, 0, (LPARAM)&dwIpAddr) == 4)
        {
            if (!This->pCurrentAdapter->NewGw)
            {
                This->pCurrentAdapter->NewGw = (IP_ADDR*)CoTaskMemAlloc(sizeof(IP_ADDR));
                if (!This->pCurrentAdapter->NewGw)
                    return E_OUTOFMEMORY;
                ZeroMemory(This->pCurrentAdapter->NewGw, sizeof(IP_ADDR));
            }

            /* store default gateway */
            This->pCurrentAdapter->NewGw->IpAddress = dwIpAddr;
        }
    }
    else
    {
        This->pCurrentAdapter->NewDhcpEnabled = TRUE;

        /* Delete all configured ip addresses */
        if (This->pCurrentAdapter->NewIp)
        {
            IP_ADDR * pNextIp = This->pCurrentAdapter->NewIp->Next;
            CoTaskMemFree(This->pCurrentAdapter->NewIp);
            This->pCurrentAdapter->NewIp = pNextIp;
        }

        /* Delete all configured gateway addresses */
        if (This->pCurrentAdapter->NewGw)
        {
            IP_ADDR * pNextGw = This->pCurrentAdapter->NewGw->Next;
            CoTaskMemFree(This->pCurrentAdapter->NewGw);
            This->pCurrentAdapter->NewGw = pNextGw;
        }
    }

    if (IsDlgButtonChecked(hwndDlg, IDC_FIXEDDNS) == BST_CHECKED)
    {
        This->pCurrentAdapter->DnsDhcpEnabled = FALSE;

        BOOL bSkip = FALSE;
        if (SendDlgItemMessageW(hwndDlg, IDC_DNS1, IPM_GETADDRESS, 0, (LPARAM)&dwIpAddr) == 4)
        {
            if (!This->pCurrentAdapter->NewNs)
            {
                This->pCurrentAdapter->NewNs = (IP_ADDR*)CoTaskMemAlloc(sizeof(IP_ADDR));
                if (!This->pCurrentAdapter->NewNs)
                    return E_OUTOFMEMORY;
                ZeroMemory(This->pCurrentAdapter->NewNs, sizeof(IP_ADDR));
            }
            This->pCurrentAdapter->NewNs->IpAddress = dwIpAddr;
        }
        else if (This->pCurrentAdapter->NewNs)
        {
            IP_ADDR *pTemp = This->pCurrentAdapter->NewNs->Next;

            CoTaskMemFree(This->pCurrentAdapter->NewNs);
            This->pCurrentAdapter->NewNs = pTemp;
            bSkip = TRUE;
        }

        if (SendDlgItemMessageW(hwndDlg, IDC_DNS2, IPM_GETADDRESS, 0, (LPARAM)&dwIpAddr) == 4)
        {
            if (!This->pCurrentAdapter->NewNs || bSkip)
            {
                if (!This->pCurrentAdapter->NewNs)
                {
                    This->pCurrentAdapter->NewNs = (IP_ADDR*)CoTaskMemAlloc(sizeof(IP_ADDR));
                    if (!This->pCurrentAdapter->NewNs)
                        return E_OUTOFMEMORY;
                    ZeroMemory(This->pCurrentAdapter->NewNs, sizeof(IP_ADDR));
                }
                This->pCurrentAdapter->NewNs->IpAddress = dwIpAddr;
            }
            else if (!This->pCurrentAdapter->NewNs->Next)
            {
                This->pCurrentAdapter->NewNs->Next = (IP_ADDR*)CoTaskMemAlloc(sizeof(IP_ADDR));
                if (!This->pCurrentAdapter->NewNs->Next)
                   return E_OUTOFMEMORY;
                ZeroMemory(This->pCurrentAdapter->NewNs->Next, sizeof(IP_ADDR));
                This->pCurrentAdapter->NewNs->Next->IpAddress = dwIpAddr;
            }
            else
            {
                This->pCurrentAdapter->NewNs->Next->IpAddress = dwIpAddr;
            }
        }
        else
        {
            if (This->pCurrentAdapter->NewNs && This->pCurrentAdapter->NewNs->Next)
            {
                if (This->pCurrentAdapter->NewNs->Next->Next)
                {
                    IP_ADDR *pTemp = This->pCurrentAdapter->NewNs->Next->Next;
                    CoTaskMemFree(This->pCurrentAdapter->NewNs->Next);
                    This->pCurrentAdapter->NewNs->Next = pTemp;
                }
                else
                {
                    CoTaskMemFree(This->pCurrentAdapter->NewNs->Next);
                    This->pCurrentAdapter->NewNs->Next = NULL;
                }
            }
        }
    }
    else
    {
        This->pCurrentAdapter->DnsDhcpEnabled = TRUE;

        /* Delete all configured name server addresses */
        if (This->pCurrentAdapter->NewNs)
        {
            IP_ADDR * pNextNs = This->pCurrentAdapter->NewNs->Next;
            CoTaskMemFree(This->pCurrentAdapter->NewNs);
            This->pCurrentAdapter->NewNs = pNextNs;
        }
    }

    return S_OK;
}

HRESULT
InitializeTcpipBasicDlgCtrls(
    HWND hwndDlg,
    TcpipConfNotifyImpl *This)
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

    if (This->pCurrentAdapter == NULL)
        return E_FAIL;

    if (This->pCurrentAdapter->NewDhcpEnabled)
    {
        CheckRadioButton(hwndDlg, IDC_USEDHCP, IDC_NODHCP, IDC_USEDHCP);
        EnableWindow(GetDlgItem(hwndDlg, IDC_IPADDR), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SUBNETMASK), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_DEFGATEWAY), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_AUTODNS), TRUE);
    }
    else
    {
        CheckRadioButton(hwndDlg, IDC_USEDHCP, IDC_NODHCP, IDC_NODHCP);

        if (This->pCurrentAdapter->NewIp)
        {
            /* set current ip address */
            SendDlgItemMessageA(hwndDlg, IDC_IPADDR, IPM_SETADDRESS, 0, (LPARAM)This->pCurrentAdapter->NewIp->IpAddress);
            /* set current hostmask */
            SendDlgItemMessageA(hwndDlg, IDC_SUBNETMASK, IPM_SETADDRESS, 0, (LPARAM)This->pCurrentAdapter->NewIp->u.SubnetMask);
        }

        if (This->pCurrentAdapter->NewGw && This->pCurrentAdapter->NewGw->IpAddress)
        {
            /* set current gateway */
            SendDlgItemMessageA(hwndDlg, IDC_DEFGATEWAY, IPM_SETADDRESS, 0, (LPARAM)This->pCurrentAdapter->NewGw->IpAddress);
        }
    }

    if (This->pCurrentAdapter->DnsDhcpEnabled)
    {
        CheckRadioButton(hwndDlg, IDC_AUTODNS, IDC_FIXEDDNS, IDC_AUTODNS);
        EnableWindow(GetDlgItem(hwndDlg, IDC_DNS1), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_DNS2), FALSE);
    }
    else
    {
        CheckRadioButton(hwndDlg, IDC_AUTODNS, IDC_FIXEDDNS, IDC_FIXEDDNS);
        EnableWindow(GetDlgItem(hwndDlg, IDC_DNS1), TRUE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_DNS2), TRUE);
        if (This->pCurrentAdapter->NewNs)
        {
            SendDlgItemMessageW(hwndDlg, IDC_DNS1, IPM_SETADDRESS, 0, (LPARAM)This->pCurrentAdapter->NewNs->IpAddress);
            if (This->pCurrentAdapter->NewNs->Next)
            {
                SendDlgItemMessageW(hwndDlg, IDC_DNS2, IPM_SETADDRESS, 0, (LPARAM)This->pCurrentAdapter->NewNs->Next->IpAddress);
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
           pNew->u.SubnetMask = GetIpAddressFromStringA(pCurrent->IpMask.String);
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
            if (This->pCurrentAdapter)
                InitializeTcpipBasicDlgCtrls(hwndDlg, This);
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
                        if (SendMessageW(GetParent(hwndDlg), PSM_INDEXTOID, 1, 0) == 0)
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
                        if (SendMessageW(GetParent(hwndDlg), PSM_INDEXTOID, 1, 0) != 0)
                        {
                            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_IPADDR), TRUE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_SUBNETMASK), TRUE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_DEFGATEWAY), TRUE);
                            if (IsDlgButtonChecked(hwndDlg, IDC_AUTODNS) == BST_CHECKED)
                            {
                                CheckDlgButton(hwndDlg, IDC_AUTODNS, BST_UNCHECKED);
                            }
                            EnableWindow(GetDlgItem(hwndDlg, IDC_AUTODNS), FALSE);
                            CheckDlgButton(hwndDlg, IDC_FIXEDDNS, BST_CHECKED);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_DNS1), TRUE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_DNS2), TRUE);
                            SendMessageW(GetParent(hwndDlg), PSM_REMOVEPAGE, 1, 0);
                        }
                        break;
                    case IDC_AUTODNS:
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                        SendDlgItemMessageW(hwndDlg, IDC_DNS1, IPM_CLEARADDRESS, 0, 0);
                        SendDlgItemMessageW(hwndDlg, IDC_DNS2, IPM_CLEARADDRESS, 0, 0);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_DNS1), FALSE);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_DNS2), FALSE);
                        break;
                    case IDC_FIXEDDNS:
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_DNS1), TRUE);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_DNS2), TRUE);
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

VOID
FreeSettings(
    TcpipConfNotifyImpl *This)
{
    AdapterSettings *pAdapter;
    IP_ADDR *NextIp;

    if (This == NULL)
        return;

    if (This->pTcpIpSettings)
    {
        if (This->pTcpIpSettings->szSearchList)
            CoTaskMemFree(This->pTcpIpSettings->szSearchList);

        CoTaskMemFree(This->pTcpIpSettings);
    }

    while (This->pAdapterListHead)
    {
        pAdapter = This->pAdapterListHead;

        /* Free IpAdresses */
        while (pAdapter->NewIp)
        {
            NextIp = pAdapter->NewIp->Next;
            CoTaskMemFree(pAdapter->NewIp);
            pAdapter->NewIp = NextIp;
        }

        while (pAdapter->OldIp)
        {
            NextIp = pAdapter->OldIp->Next;
            CoTaskMemFree(pAdapter->OldIp);
            pAdapter->OldIp = NextIp;
        }

        /* Free Gateways */
        while (pAdapter->NewGw)
        {
            NextIp = pAdapter->NewGw->Next;
            CoTaskMemFree(pAdapter->NewGw);
            pAdapter->NewGw = NextIp;
        }

        while (pAdapter->OldGw)
        {
            NextIp = pAdapter->OldGw->Next;
            CoTaskMemFree(pAdapter->OldGw);
            pAdapter->OldGw = NextIp;
        }

        /* Free NameServers */
        while (pAdapter->NewNs)
        {
            NextIp = pAdapter->NewNs->Next;
            CoTaskMemFree(pAdapter->NewNs);
            pAdapter->NewNs = NextIp;
        }

        while (pAdapter->OldNs)
        {
            NextIp = pAdapter->OldNs->Next;
            CoTaskMemFree(pAdapter->OldNs);
            pAdapter->OldNs = NextIp;
        }

        if (pAdapter->szTCPAllowedPorts)
            CoTaskMemFree(pAdapter->szTCPAllowedPorts);

        if (pAdapter->szUDPAllowedPorts)
            CoTaskMemFree(pAdapter->szUDPAllowedPorts);

        if (pAdapter->szRawIPAllowedProtocols)
            CoTaskMemFree(pAdapter->szRawIPAllowedProtocols);


        This->pAdapterListHead = pAdapter->pNext;
        CoTaskMemFree(pAdapter);
    }

    This->pAdapterListTail = NULL;
    This->pCurrentAdapter = NULL;
}

static
BOOL
IpAddressesChanged(
    AdapterSettings *pAdapter)
{
    IP_ADDR *pNew, *pOld;
    BOOL Changed = FALSE;

    pNew = pAdapter->NewIp;
    pOld = pAdapter->OldIp;
    while (pNew && pOld)
    {
        if ((pNew->IpAddress != pOld->IpAddress) ||
            (pNew->u.SubnetMask != pOld->u.SubnetMask))
        {
            Changed = TRUE;
            break;
        }

        pNew = pNew->Next;
        pOld = pOld->Next;
    }

    if (Changed == FALSE)
    {
        if (((pNew == NULL) && (pOld != NULL)) ||
            ((pNew != NULL) && (pOld == NULL)))
            Changed = TRUE;
    }

    return Changed;
}

static
BOOL
GatewaysChanged(
    AdapterSettings *pAdapter)
{
    IP_ADDR *pNew, *pOld;
    BOOL Changed = FALSE;

    pNew = pAdapter->NewGw;
    pOld = pAdapter->OldGw;
    while (pNew && pOld)
    {
        if ((pNew->IpAddress != pOld->IpAddress) ||
            (pNew->u.Metric != pOld->u.Metric))
        {
            Changed = TRUE;
            break;
        }

        pNew = pNew->Next;
        pOld = pOld->Next;
    }

    if (Changed == FALSE)
    {
        if (((pNew == NULL) && (pOld != NULL)) ||
            ((pNew != NULL) && (pOld == NULL)))
            Changed = TRUE;
    }

    return Changed;
}

static
BOOL
NameServersChanged(
    AdapterSettings *pAdapter)
{
    IP_ADDR *pNew, *pOld;
    BOOL Changed = FALSE;

    pNew = pAdapter->NewNs;
    pOld = pAdapter->OldNs;
    while (pNew && pOld)
    {
        if (pNew->IpAddress != pOld->IpAddress)
        {
            Changed = TRUE;
            break;
        }

        pNew = pNew->Next;
        pOld = pOld->Next;
    }

    if (Changed == FALSE)
    {
        if (((pNew == NULL) && (pOld != NULL)) ||
            ((pNew != NULL) && (pOld == NULL)))
            Changed = TRUE;
    }

    return Changed;
}

/***************************************************************
 * INetCfgComponentPropertyUi interface
 */

HRESULT
WINAPI
INetCfgComponentPropertyUi_fnQueryInterface(
    INetCfgComponentPropertyUi *iface,
    REFIID iid,
    LPVOID *ppvObj)
{
    TRACE("INetCfgComponentPropertyUi_fnQueryInterface()\n");
    TcpipConfNotifyImpl *This = impl_from_INetCfgComponentPropertyUi(iface);
    return INetCfgComponentControl_QueryInterface((INetCfgComponentControl*)This, iid, ppvObj);
}

ULONG
WINAPI
INetCfgComponentPropertyUi_fnAddRef(
    INetCfgComponentPropertyUi *iface)
{
    TRACE("INetCfgComponentPropertyUi_fnAddRef()\n");
    TcpipConfNotifyImpl *This = impl_from_INetCfgComponentPropertyUi(iface);
    return INetCfgComponentControl_AddRef((INetCfgComponentControl*)This);
}

ULONG
WINAPI
INetCfgComponentPropertyUi_fnRelease(
    INetCfgComponentPropertyUi *iface)
{
    TRACE("INetCfgComponentPropertyUi_fnRelease()\n");
    TcpipConfNotifyImpl *This = impl_from_INetCfgComponentPropertyUi(iface);
    return INetCfgComponentControl_Release((INetCfgComponentControl*)This);
}

HRESULT
WINAPI
INetCfgComponentPropertyUi_fnQueryPropertyUi(
    INetCfgComponentPropertyUi * iface,
    IUnknown *pUnkReserved)
{
    TRACE("INetCfgComponentPropertyUi_fnQueryPropertyUi()\n");
    return S_OK;
}

HRESULT
WINAPI
INetCfgComponentPropertyUi_fnSetContext(
    INetCfgComponentPropertyUi * iface,
    IUnknown *pUnkReserved)
{
    TRACE("INetCfgComponentPropertyUi_fnSetContext()\n");
    INetLanConnectionUiInfo * pLanInfo;
    GUID Guid;
    LPOLESTR pAdapterName = NULL;
    AdapterSettings *pAdapter = NULL;
    HRESULT hr;
    TcpipConfNotifyImpl *This = impl_from_INetCfgComponentPropertyUi(iface);

    if (!iface || !pUnkReserved)
        return E_POINTER;

    if (pUnkReserved)
    {
        hr = IUnknown_QueryInterface(pUnkReserved, &IID_INetLanConnectionUiInfo, (LPVOID*)&pLanInfo);
        if (FAILED(hr))
            return hr;

        INetLanConnectionUiInfo_GetDeviceGuid(pLanInfo, &Guid);

        IUnknown_Release(pUnkReserved);

        StringFromCLSID(&Guid, &pAdapterName);

        pAdapter = This->pAdapterListHead;
        while (pAdapter)
        {
            if (!_wcsicmp(pAdapter->AdapterName, pAdapterName))
            {
                This->pCurrentAdapter = pAdapter;
                break;
            }
            pAdapter = pAdapter->pNext;
        }

        CoTaskMemFree(pAdapterName);
    }
    else
    {
        This->pCurrentAdapter = NULL;
    }

    This->pUnknown = pUnkReserved;

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
LoadTcpipSettings(
    TcpipConfNotifyImpl *This)
{
    TcpipSettings *pSettings;
    HKEY hKey = NULL;
    DWORD dwSize, dwVal;
    HRESULT hr = S_OK;

    pSettings = (TcpipSettings*)CoTaskMemAlloc(sizeof(TcpipSettings));
    if (!pSettings)
        return E_FAIL;

    ZeroMemory(pSettings, sizeof(TcpipSettings));

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        hr = E_FAIL;
        goto done;
    }

    dwSize = sizeof(DWORD);
    RegQueryValueExW(hKey, L"UseDomainNameDevolution", NULL, NULL, (LPBYTE)&pSettings->UseDomainNameDevolution, &dwSize);

    dwSize = 0;
    RegQueryValueExW(hKey, L"SearchList", NULL, NULL, NULL, &dwSize);
    if (dwSize)
    {
        pSettings->szSearchList = (LPWSTR)CoTaskMemAlloc(dwSize);
        if (pSettings->szSearchList)
        {
            if (RegQueryValueExW(hKey, L"SearchList", NULL, NULL, (LPBYTE)pSettings->szSearchList, &dwSize) != ERROR_SUCCESS)
            {
                CoTaskMemFree(pSettings->szSearchList);
                pSettings->szSearchList = NULL;
            }
        }
    }

    dwSize = sizeof(DWORD);
    if (RegQueryValueExW(hKey, L"EnableSecurityFilters", NULL, NULL, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS)
        pSettings->EnableSecurityFilters = dwVal;

done:
    if (hKey)
        RegCloseKey(hKey);

    if (hr == S_OK)
    {
        This->pTcpIpSettings = pSettings;
    }
    else
    {
        CoTaskMemFree(pSettings);
    }

    return hr;
}

HRESULT
ParseNameServer(
    HKEY hAdapterKey,
    AdapterSettings *pAdapter)
{
    PWSTR pszBuffer = NULL, pStart, pEnd;
    DWORD dwSize;
    IN_ADDR Address;
    IP_ADDR *pNewAddrEntry, *pOldAddrEntry, *pNewLast = NULL, *pOldLast = NULL;
    HRESULT hr = S_OK;
    NTSTATUS Status;

    dwSize = 0;
    RegQueryValueExW(hAdapterKey, L"NameServer", NULL, NULL, NULL, &dwSize);
    if (dwSize == 0)
        return S_OK;

    pszBuffer = CoTaskMemAlloc(dwSize);
    if (pszBuffer == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    if (RegQueryValueExW(hAdapterKey, L"NameServer", NULL, NULL, (PBYTE)pszBuffer, &dwSize) != ERROR_SUCCESS)
    {
        hr = E_FAIL;
        goto done;
    }

    pStart = pszBuffer;
    for (;;)
    {
        Status = RtlIpv4StringToAddressW(pStart, TRUE, (LPCWSTR *)&pEnd, &Address);
        TRACE("RtlIpv4StringToAddressW Status 0x%08lx\n", Status);
        if (Status != 0)
        {
            hr = E_FAIL;
            break;
        }

        TRACE("Adress: %lx\n", Address.S_un.S_addr);

        pNewAddrEntry = CoTaskMemAlloc(sizeof(IP_ADDR));
        if (pNewAddrEntry)
        {
            ZeroMemory(pNewAddrEntry, sizeof(IP_ADDR));
            pNewAddrEntry->IpAddress = ntohl(Address.S_un.S_addr);

            if (!pNewLast)
                pAdapter->NewNs = pNewAddrEntry;
            else
                pNewLast->Next = pNewAddrEntry;

            pNewLast = pNewAddrEntry;
        }

        pOldAddrEntry = CoTaskMemAlloc(sizeof(IP_ADDR));
        if (pOldAddrEntry)
        {
            ZeroMemory(pOldAddrEntry, sizeof(IP_ADDR));
            pOldAddrEntry->IpAddress = ntohl(Address.S_un.S_addr);

            if (!pOldLast)
                pAdapter->OldNs = pOldAddrEntry;
            else
                pOldLast->Next = pOldAddrEntry;

            pOldLast = pOldAddrEntry;
        }

        if (*pEnd == UNICODE_NULL)
            break;

        pStart = pEnd + 1;
    }

done:
    if (pszBuffer)
        CoTaskMemFree(pszBuffer);

    return hr;
}

HRESULT
LoadAdapterSettings(
    TcpipConfNotifyImpl *This,
    LPOLESTR pAdapterName,
    IP_ADAPTER_INFO *pAdapterInfo)
{
    AdapterSettings *pAdapter;
    HKEY hAdapterKey = NULL, hConfigKey;
    WCHAR szKeyName[200];
    DWORD dwSize;
    HRESULT hr = S_OK;

    pAdapter = (AdapterSettings*)CoTaskMemAlloc(sizeof(AdapterSettings));
    if (!pAdapter)
        return E_FAIL;

    ZeroMemory(pAdapter, sizeof(AdapterSettings));

    _swprintf(szKeyName, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\%s", pAdapterName);
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szKeyName, 0, KEY_READ, &hAdapterKey) != ERROR_SUCCESS)
    {
        hr = E_FAIL;
        goto done;
    }

    wcscpy(pAdapter->AdapterName, pAdapterName);
    CLSIDFromString(pAdapter->AdapterName, &pAdapter->AdapterGuid);

    pAdapter->OldDhcpEnabled = pAdapterInfo->DhcpEnabled;
    pAdapter->NewDhcpEnabled = pAdapterInfo->DhcpEnabled;
    pAdapter->Index = pAdapterInfo->Index;

    if (!pAdapterInfo->DhcpEnabled)
    {
        CopyIpAddrString(&pAdapterInfo->IpAddressList, &pAdapter->NewIp, SUBMASK, NULL);
        CopyIpAddrString(&pAdapterInfo->IpAddressList, &pAdapter->OldIp, SUBMASK, NULL);
    }

    CopyIpAddrString(&pAdapterInfo->GatewayList, &pAdapter->NewGw, METRIC, NULL);
    CopyIpAddrString(&pAdapterInfo->GatewayList, &pAdapter->OldGw, METRIC, NULL);

    ParseNameServer(hAdapterKey, pAdapter);

    pAdapter->DnsDhcpEnabled = (pAdapter->NewDhcpEnabled && (pAdapter->NewNs == NULL));

    /* InterfaceMetric */
    dwSize = sizeof(DWORD);
    if (RegQueryValueExW(hAdapterKey, L"InterfaceMetric", NULL, NULL, (LPBYTE)&pAdapter->NewMetric, &dwSize) != ERROR_SUCCESS)
        pAdapter->NewMetric = 0;
    pAdapter->OldMetric = pAdapter->NewMetric;

    /* DNS */
    dwSize = sizeof(DWORD);
    RegQueryValueExW(hAdapterKey, L"RegisterAdapterName", NULL, NULL, (LPBYTE)&pAdapter->NewRegisterAdapterName, &dwSize);
    pAdapter->OldRegisterAdapterName = pAdapter->NewRegisterAdapterName;

    dwSize = sizeof(DWORD);
    RegQueryValueExW(hAdapterKey, L"RegistrationEnabled", NULL, NULL, (LPBYTE)&pAdapter->NewRegistrationEnabled, &dwSize);
    pAdapter->OldRegistrationEnabled = pAdapter->NewRegistrationEnabled;

    dwSize = sizeof(pAdapter->szDomain);
    RegQueryValueExW(hAdapterKey, L"Domain", NULL, NULL, (LPBYTE)pAdapter->szDomain, &dwSize);

    /* Filter */
    pAdapter->szTCPAllowedPorts = LoadTcpFilterSettingsFromRegistry(hAdapterKey, L"TCPAllowedPorts", &pAdapter->TCPSize);
    pAdapter->szUDPAllowedPorts = LoadTcpFilterSettingsFromRegistry(hAdapterKey, L"UDPAllowedPorts", &pAdapter->UDPSize);
    pAdapter->szRawIPAllowedProtocols = LoadTcpFilterSettingsFromRegistry(hAdapterKey, L"RawIPAllowedProtocols", &pAdapter->RawIPSize);

    if (This->pAdapterListHead == NULL)
    {
        This->pAdapterListHead = pAdapter;
        This->pAdapterListTail = pAdapter;
    }
    else
    {
        This->pAdapterListTail->pNext = pAdapter;
        pAdapter->pPrev = This->pAdapterListTail;
        This->pAdapterListTail = pAdapter;
    }

    /* Read the alternate configuration, if available */
    dwSize = 0;
    RegQueryValueExW(hAdapterKey, L"ActiveConfigurations", NULL, NULL, NULL, &dwSize);
    if (dwSize)
    {
        _swprintf(szKeyName, L"SYSTEM\\CurrentControlSet\\Services\\DHCP\\Configurations\\Alternate_%s", pAdapterName);
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szKeyName, 0, KEY_READ, &hConfigKey) == ERROR_SUCCESS)
        {
            dwSize = sizeof(AlternateConfiguration);
            RegQueryValueExW(hConfigKey, L"Options", NULL, NULL, (LPBYTE)&pAdapter->AltConfig, &dwSize);
            RegCloseKey(hConfigKey);
        }
    }

done:
    if (hAdapterKey)
        RegCloseKey(hAdapterKey);

    return hr;
}


HRESULT
Initialize(TcpipConfNotifyImpl * This)
{
    DWORD dwSize;
    WCHAR szBuffer[50];
    IP_ADAPTER_INFO *pCurrentAdapter;
    IP_ADAPTER_INFO *pInfo = NULL;
    HRESULT hr = S_OK;

    if (This->pAdapterListHead)
        return S_OK;

    dwSize = 0;
    if (GetAdaptersInfo(NULL, &dwSize) != ERROR_BUFFER_OVERFLOW)
        return E_FAIL;

    pInfo = CoTaskMemAlloc(dwSize);
    if (!pInfo)
        return E_FAIL;

    if (GetAdaptersInfo(pInfo, &dwSize) != ERROR_SUCCESS)
    {
        CoTaskMemFree(pInfo);
        return E_FAIL;
    }

    hr = LoadTcpipSettings(This);

    pCurrentAdapter = pInfo;
    while (pCurrentAdapter)
    {
        szBuffer[0] = L'\0';
        if (MultiByteToWideChar(CP_ACP, 0, pCurrentAdapter->AdapterName, -1, szBuffer, sizeof(szBuffer)/sizeof(szBuffer[0])))
        {
            szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
        }

        TRACE("Adapter: %S\n", szBuffer);

        LoadAdapterSettings(This,
                            szBuffer,
                            pCurrentAdapter);

        pCurrentAdapter = pCurrentAdapter->Next;
    }

    CoTaskMemFree(pInfo);

    if (FAILED(hr))
    {
        FreeSettings(This);
    }

    return hr;
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
    TRACE("INetCfgComponentPropertyUi_fnMergePropPages()\n");
    HPROPSHEETPAGE * hppages;
    UINT NumPages;
    TcpipConfNotifyImpl *This = impl_from_INetCfgComponentPropertyUi(iface);

    if (This->pCurrentAdapter->NewDhcpEnabled)
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
    TRACE("INetCfgComponentPropertyUi_fnValidateProperties()\n");
    return S_OK;
}

HRESULT
WINAPI
INetCfgComponentPropertyUi_fnApplyProperties(
    INetCfgComponentPropertyUi * iface)
{
    TRACE("INetCfgComponentPropertyUi_fnApplyProperties()\n");
    return S_OK;
}

HRESULT
WINAPI
INetCfgComponentPropertyUi_fnCancelProperties(
    INetCfgComponentPropertyUi * iface)
{
    TRACE("INetCfgComponentPropertyUi_fnCancelProperties()\n");
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
    INetCfgComponentControl *iface,
    REFIID iid,
    LPVOID *ppvObj)
{
    TRACE("INetCfgComponentControl_fnQueryInterface()\n");
    TcpipConfNotifyImpl *This = (TcpipConfNotifyImpl*)iface;

    *ppvObj = NULL;

    if (IsEqualIID (iid, &IID_IUnknown) ||
        IsEqualIID (iid, &IID_INetCfgComponentControl))
    {
        *ppvObj = This;
        INetCfgComponentControl_AddRef(iface);
        return S_OK;
    }
    else if (IsEqualIID(iid, &IID_INetCfgComponentPropertyUi))
    {
        *ppvObj = (LPVOID*)&This->lpVtblCompPropertyUi;
        INetCfgComponentControl_AddRef(iface);
        return S_OK;
    }
    else if (IsEqualIID(iid, &IID_INetCfgComponentSetup))
    {
        *ppvObj = (LPVOID*)&This->lpVtblCompSetup;
        INetCfgComponentControl_AddRef(iface);
        return S_OK;
    }
    else if (IsEqualIID(iid, &IID_ITcpipProperties))
    {
        *ppvObj = (LPVOID*)&This->lpVtblTcpipProperties;
        INetCfgComponentControl_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG
WINAPI
INetCfgComponentControl_fnAddRef(
    INetCfgComponentControl *iface)
{
    TRACE("INetCfgComponentControl_fnAddRef()\n");
    TcpipConfNotifyImpl *This = (TcpipConfNotifyImpl*)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    return refCount;
}

ULONG
WINAPI
INetCfgComponentControl_fnRelease(
    INetCfgComponentControl *iface)
{
    TRACE("INetCfgComponentControl_fnRelease()\n");
    TcpipConfNotifyImpl * This = (TcpipConfNotifyImpl*)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    if (!refCount)
    {
        FreeSettings(This);
        CoTaskMemFree(This);
    }
    return refCount;
}

HRESULT
WINAPI
INetCfgComponentControl_fnInitialize(
    INetCfgComponentControl *iface,
    INetCfgComponent *pIComp,
    INetCfg *pINetCfg,
    BOOL fInstalling)
{
    TRACE("INetCfgComponentControl_fnInitialize()\n");
    TcpipConfNotifyImpl * This = (TcpipConfNotifyImpl*)iface;
    HRESULT hr;

    This->pNCfg = pINetCfg;
    This->pNComp = pIComp;

    hr = Initialize(This);
    if (FAILED(hr))
    {
        ERR("INetCfgComponentControl_fnInitialize failed\n");
        return hr;
    }

    TRACE("INetCfgComponentControl_fnInitialize success\n");

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
            _swprintf(szBuffer, L"%lu.%lu.%lu.%lu",
                      FIRST_IPADDRESS(dwIpAddr), SECOND_IPADDRESS(dwIpAddr), THIRD_IPADDRESS(dwIpAddr), FOURTH_IPADDRESS(dwIpAddr));
        }else if (Type == SUBMASK)
        {
            dwIpAddr = pTemp->u.SubnetMask;
            _swprintf(szBuffer, L"%lu.%lu.%lu.%lu",
                      FIRST_IPADDRESS(dwIpAddr), SECOND_IPADDRESS(dwIpAddr), THIRD_IPADDRESS(dwIpAddr), FOURTH_IPADDRESS(dwIpAddr));
        }
        else if (Type == METRIC)
        {
            _swprintf(szBuffer, L"%u", pTemp->u.Metric);
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
            _swprintf(pStr, L"%lu.%lu.%lu.%lu",
                      FIRST_IPADDRESS(dwIpAddr), SECOND_IPADDRESS(dwIpAddr), THIRD_IPADDRESS(dwIpAddr), FOURTH_IPADDRESS(dwIpAddr));
        }else if (Type == SUBMASK)
        {
            dwIpAddr = pTemp->u.SubnetMask;
            _swprintf(pStr, L"%lu.%lu.%lu.%lu",
                      FIRST_IPADDRESS(dwIpAddr), SECOND_IPADDRESS(dwIpAddr), THIRD_IPADDRESS(dwIpAddr), FOURTH_IPADDRESS(dwIpAddr));
        }
        else if (Type == METRIC)
        {
            _swprintf(pStr, L"%u", pTemp->u.Metric);
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
    WCHAR szBuffer[200];
    LPOLESTR pStr;
    DWORD dwSize;
    AdapterSettings *pAdapter;

    TRACE("INetCfgComponentControl_fnApplyRegistryChanges()\n");

    TcpipConfNotifyImpl * This = (TcpipConfNotifyImpl*)iface;

    if (This->pTcpIpSettings)
    {
        if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
        {
            RegSetValueExW(hKey, L"UseDomainNameDevolution", 0, REG_DWORD, (LPBYTE)&This->pTcpIpSettings->UseDomainNameDevolution, sizeof(DWORD));
            RegSetValueExW(hKey, L"SearchList", 0, REG_SZ, (LPBYTE)This->pTcpIpSettings->szSearchList,
                       (wcslen(This->pTcpIpSettings->szSearchList) + 1) * sizeof(WCHAR));
            RegSetValueExW(hKey, L"EnableSecurityFilters", 0, REG_DWORD,
                      (LPBYTE)&This->pTcpIpSettings->EnableSecurityFilters, sizeof(DWORD));

            RegCloseKey(hKey);
        }
    }

    pAdapter = This->pAdapterListHead;
    while (pAdapter)
    {
        _swprintf(szBuffer, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\%s", pAdapter->AdapterName);

        if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, szBuffer, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
        {
            if (pAdapter->NewMetric != pAdapter->OldMetric)
            {
                if (pAdapter->NewMetric)
                    RegSetValueExW(hKey, L"InterfaceMetric", 0, REG_DWORD, (LPBYTE)&pAdapter->NewMetric, sizeof(DWORD));
                else
                    RegDeleteValueW(hKey, L"InterfaceMetric");
            }

            if (pAdapter->NewRegisterAdapterName != pAdapter->OldRegisterAdapterName)
                RegSetValueExW(hKey, L"RegisterAdapterName", 0, REG_DWORD, (LPBYTE)&pAdapter->NewRegisterAdapterName, sizeof(DWORD));

            if (pAdapter->NewRegistrationEnabled != pAdapter->OldRegistrationEnabled)
                RegSetValueExW(hKey, L"RegistrationEnabled", 0, REG_DWORD, (LPBYTE)&pAdapter->NewRegistrationEnabled, sizeof(DWORD));

            RegSetValueExW(hKey, L"Domain", 0, REG_SZ, (LPBYTE)pAdapter->szDomain,
                       (wcslen(pAdapter->szDomain) + 1) * sizeof(WCHAR));

            if ((pAdapter->NewDhcpEnabled != pAdapter->OldDhcpEnabled) ||
                IpAddressesChanged(pAdapter))
            {
                RegSetValueExW(hKey, L"EnableDHCP", 0, REG_DWORD, (LPBYTE)&pAdapter->NewDhcpEnabled, sizeof(DWORD));
                if (pAdapter->NewDhcpEnabled)
                {
                    RegSetValueExW(hKey, L"IPAddress", 0, REG_MULTI_SZ, (LPBYTE)L"0.0.0.0\0", 9 * sizeof(WCHAR));
                    RegSetValueExW(hKey, L"SubnetMask", 0, REG_MULTI_SZ, (LPBYTE)L"0.0.0.0\0", 9 * sizeof(WCHAR));
                }
                else
                {
                    pStr = CreateMultiSzString(pAdapter->NewIp, IPADDR, &dwSize, FALSE);
                    if (pStr)
                    {
                        RegSetValueExW(hKey, L"IPAddress", 0, REG_MULTI_SZ, (LPBYTE)pStr, dwSize);
                        CoTaskMemFree(pStr);
                    }

                    pStr = CreateMultiSzString(pAdapter->NewIp, SUBMASK, &dwSize, FALSE);
                    if (pStr)
                    {
                        RegSetValueExW(hKey, L"SubnetMask", 0, REG_MULTI_SZ, (LPBYTE)pStr, dwSize);
                        CoTaskMemFree(pStr);
                    }
                }
            }

            if (GatewaysChanged(pAdapter))
            {
                if (pAdapter->NewGw)
                {
                    pStr = CreateMultiSzString(pAdapter->NewGw, IPADDR, &dwSize, FALSE);
                    if (pStr)
                    {
                        RegSetValueExW(hKey, L"DefaultGateway", 0, REG_MULTI_SZ, (LPBYTE)pStr, dwSize);
                        CoTaskMemFree(pStr);
                    }

                    pStr = CreateMultiSzString(pAdapter->NewGw, METRIC, &dwSize, FALSE);
                    if (pStr)
                    {
                        RegSetValueExW(hKey, L"DefaultGatewayMetric", 0, REG_MULTI_SZ, (LPBYTE)pStr, dwSize);
                        CoTaskMemFree(pStr);
                    }
                }
                else
                {
                    RegSetValueExW(hKey, L"DefaultGateway", 0, REG_MULTI_SZ, (LPBYTE)L"", 1 * sizeof(WCHAR));
                    RegSetValueExW(hKey, L"DefaultGatewayMetric", 0, REG_MULTI_SZ, (LPBYTE)L"\0", sizeof(WCHAR) * 2);
                }
            }

            if (NameServersChanged(pAdapter))
            {
                if (!pAdapter->NewNs)
                {
                    RegDeleteValueW(hKey, L"NameServer");
                }
                else
                {
                    pStr = CreateMultiSzString(pAdapter->NewNs, IPADDR, &dwSize, TRUE);
                    if (pStr)
                    {
                        RegSetValueExW(hKey, L"NameServer", 0, REG_SZ, (LPBYTE)pStr, dwSize);
                        //RegDeleteValueW(hKey, L"DhcpNameServer");
                        CoTaskMemFree(pStr);
                    }
                }
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

#if 0
            if (!RtlEqualMemory(&pCurrentConfig->AltConfig, &pOldConfig->AltConfig, sizeof(AlternateConfiguration)))
            {
                if (pCurrentConfig->AltConfig.IpAddress == 0)
                {
                    RegDeleteValueW(hKey, L"ActiveConfigurations");
                }
                else
                {
                    HKEY hConfigKey;

                    dwSize = (wcslen(L"Alternate_") + wcslen(pAdapterName) + 2) * sizeof(WCHAR);
                    pStr = CoTaskMemAlloc(dwSize);
                    if (pStr)
                    {
                        ZeroMemory(pStr, dwSize);
                        _swprintf(pStr, L"Alternate_%s", pAdapterName);
                        RegSetValueExW(hKey, L"ActiveConfigurations", 0, REG_MULTI_SZ, (LPBYTE)pStr, dwSize);
                        CoTaskMemFree(pStr);
                    }

                    _swprintf(szBuffer, L"SYSTEM\\CurrentControlSet\\Services\\DHCP\\Configurations\\Alternate_%s", pAdapterName);
                    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, szBuffer, 0, NULL, 0, KEY_WRITE, NULL, &hConfigKey, NULL) == ERROR_SUCCESS)
                    {
                        RegSetValueExW(hConfigKey, L"Options", 0, REG_BINARY, (LPBYTE)&pCurrentConfig->AltConfig, sizeof(AlternateConfiguration));
                        RegCloseKey(hConfigKey);
                    }
                }
            }
#endif

            RegCloseKey(hKey);
        }

        pAdapter = pAdapter->pNext;
    }

    return S_OK;
}

HRESULT
WINAPI
INetCfgComponentControl_fnApplyPnpChanges(
    INetCfgComponentControl *iface,
    INetCfgPnpReconfigCallback *pICallback)
{
    ULONG NTEInstance;
    DWORD DhcpApiVersion;
    DWORD dwSize;
    AdapterSettings *pAdapter;

    TRACE("INetCfgComponentControl_fnApplyPnpChanges()\n");

    TcpipConfNotifyImpl *This = (TcpipConfNotifyImpl*)iface;

    pAdapter = This->pAdapterListHead;
    while (pAdapter)
    {
        if (pAdapter->NewDhcpEnabled != pAdapter->OldDhcpEnabled)
        {
            if (pAdapter->NewDhcpEnabled)
            {
                /* Static IP --> DHCP */

                /* Delete this adapter's current IP address */
                DeleteIPAddress(pAdapter->OldIp->NTEContext);
            }
            else
            {
                /* DHCP --> Static IP */

                /* Open the DHCP API if DHCP is enabled */
                if (DhcpCApiInitialize(&DhcpApiVersion) == NO_ERROR)
                {
                    /* We have to tell DHCP about this */
                    DhcpStaticRefreshParams(pAdapter->Index,
                                            htonl(pAdapter->NewIp->IpAddress),
                                            htonl(pAdapter->NewIp->u.SubnetMask));

                    /* Close the API */
                    DhcpCApiCleanup();
                }
            }
        }
        else
        {
            if (IpAddressesChanged(pAdapter))
            {
                /* Static IP --> Static IP */

                /* Delete this adapter's current static IP address */
                DeleteIPAddress(pAdapter->OldIp->NTEContext);

                /* Add the static IP address via the standard IPHLPAPI function */
                AddIPAddress(htonl(pAdapter->NewIp->IpAddress),
                             htonl(pAdapter->NewIp->u.SubnetMask),
                             pAdapter->Index,
                             &pAdapter->NewIp->NTEContext,
                             &NTEInstance);
            }
        }

        if ((pAdapter->NewDhcpEnabled != pAdapter->OldDhcpEnabled) ||
            IpAddressesChanged(pAdapter) ||
            GatewaysChanged(pAdapter))
        {
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
                            if (pIpForwardTable->table[Index].dwForwardIfIndex == pAdapter->Index &&
                                pIpForwardTable->table[Index].dwForwardDest == 0)
                            {
                                DeleteIpForwardEntry(&pIpForwardTable->table[Index]);
                            }
                        }
                    }
                    CoTaskMemFree(pIpForwardTable);
                }
            }

            if (pAdapter->NewGw)
            {
                MIB_IPFORWARDROW RouterMib;
                ZeroMemory(&RouterMib, sizeof(MIB_IPFORWARDROW));

                RouterMib.dwForwardMetric1 = 1;
                RouterMib.dwForwardIfIndex = pAdapter->Index;
                RouterMib.dwForwardNextHop = htonl(pAdapter->NewGw->IpAddress);

                //TODO
                // add multiple gw addresses when required
                CreateIpForwardEntry(&RouterMib);
            }
        }

#if 0
        /* Notify the DHCP client of the changed alternate configuration */
        if (!RtlEqualMemory(&pCurrentConfig->AltConfig, &pOldConfig->AltConfig, sizeof(AlternateConfiguration)))
        {
            HMODULE hDhcpModule = LoadLibraryW(L"dhcpcsvc.dll");
            if (hDhcpModule)
            {
                PDHCPFALLBACKREFRESHPARAMS pFunc = (PDHCPFALLBACKREFRESHPARAMS)GetProcAddress(hDhcpModule, "DhcpFallbackRefreshParams");
                (pFunc)(pAdapter->AdapterName);

                FreeLibrary(hDhcpModule);
            }
        }
#endif

        /* Notify the dnscache service if the name server list changed */
        if (NameServersChanged(pAdapter))
        {
            SC_HANDLE hManager, hService;
            SERVICE_STATUS ServiceStatus;

            TRACE("Notify the dnscache service!\n");

            hManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
            if (hManager)
            {
                hService = OpenServiceW(hManager, L"Dnscache", SERVICE_PAUSE_CONTINUE);
                if (hService)
                {
                    ControlService(hService, SERVICE_CONTROL_PARAMCHANGE, &ServiceStatus);
                    CloseServiceHandle(hService);
                }

                CloseServiceHandle(hManager);
            }
        }

        pAdapter = pAdapter->pNext;
    }

    return S_OK;
}

HRESULT
WINAPI
INetCfgComponentControl_fnCancelChanges(
    INetCfgComponentControl * iface)
{
    TRACE("INetCfgComponentControl_fnCancelChanges()\n");
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


/***************************************************************
 * INetCfgComponentSetup interface
 */

HRESULT
WINAPI
INetCfgComponentSetup_fnQueryInterface(
    INetCfgComponentSetup *iface,
    REFIID iid,
    LPVOID *ppvObj)
{
    TRACE("INetCfgComponentSetup_fnQueryInterface()\n");
    TcpipConfNotifyImpl *This = impl_from_INetCfgComponentSetup(iface);
    return INetCfgComponentControl_QueryInterface((INetCfgComponentControl*)This, iid, ppvObj);
}

ULONG
WINAPI
INetCfgComponentSetup_fnAddRef(
    INetCfgComponentSetup *iface)
{
    TRACE("INetCfgComponentSetup_fnAddRef()\n");
    TcpipConfNotifyImpl *This = impl_from_INetCfgComponentSetup(iface);
    return INetCfgComponentControl_AddRef((INetCfgComponentControl*)This);
}

ULONG
WINAPI
INetCfgComponentSetup_fnRelease(
    INetCfgComponentSetup *iface)
{
    TRACE("INetCfgComponentSetup_fnRelease()\n");
    TcpipConfNotifyImpl *This = impl_from_INetCfgComponentSetup(iface);
    return INetCfgComponentControl_Release((INetCfgComponentControl*)This);
}

HRESULT
WINAPI
INetCfgComponentSetup_fnInstall(
    INetCfgComponentSetup *iface,
    DWORD dwSetupFlags)
{
    TRACE("INetCfgComponentSetup_fnInstall()\n");
    return S_OK;
}

HRESULT
WINAPI
INetCfgComponentSetup_fnUpgrade(
    INetCfgComponentSetup *iface,
    DWORD dwSetupFlags,
    DWORD dwUpgradeFromBuildNo)
{
    TRACE("INetCfgComponentSetup_fnUpgrade()\n");
    return S_OK;
}

HRESULT
WINAPI
INetCfgComponentSetup_fnReadAnswerFile(
    INetCfgComponentSetup *iface,
    LPCWSTR pszwAnswerFile,
    LPCWSTR pszwAnswerSections)
{
    TRACE("INetCfgComponentSetup_fnReadAnswerFile()\n");
    return S_OK;
}

HRESULT
WINAPI
INetCfgComponentSetup_fnRemoving(
    INetCfgComponentSetup *iface)
{
    TRACE("INetCfgComponentSetup_fnRemoving()\n");
    return S_OK;
}

static const INetCfgComponentSetupVtbl vt_NetCfgComponentSetup =
{
    INetCfgComponentSetup_fnQueryInterface,
    INetCfgComponentSetup_fnAddRef,
    INetCfgComponentSetup_fnRelease,
    INetCfgComponentSetup_fnInstall,
    INetCfgComponentSetup_fnUpgrade,
    INetCfgComponentSetup_fnReadAnswerFile,
    INetCfgComponentSetup_fnRemoving
};


/***************************************************************
 * ITcpipProperties interface
 */

HRESULT
WINAPI
ITcpipProperties_fnQueryInterface(
    ITcpipProperties *iface,
    REFIID iid,
    LPVOID *ppvObj)
{
    TRACE("ITcpipProperties_fnQueryInterface()\n");
    TcpipConfNotifyImpl *This = impl_from_ITcpipProperties(iface);
    return INetCfgComponentControl_QueryInterface((INetCfgComponentControl*)This, iid, ppvObj);
}

ULONG
WINAPI
ITcpipProperties_fnAddRef(
    ITcpipProperties *iface)
{
    TRACE("ITcpipProperties_fnAddRef()\n");
    TcpipConfNotifyImpl *This = impl_from_ITcpipProperties(iface);
    return INetCfgComponentControl_AddRef((INetCfgComponentControl*)This);
}

ULONG
WINAPI
ITcpipProperties_fnRelease(
    ITcpipProperties *iface)
{
    TRACE("ITcpipProperties_fnRelease()\n");
    TcpipConfNotifyImpl *This = impl_from_ITcpipProperties(iface);
    return INetCfgComponentControl_Release((INetCfgComponentControl*)This);
}

HRESULT
WINAPI
ITcpipProperties_fnUnknown1(
    ITcpipProperties *iface,
    GUID *pAdapterName,
    PTCPIP_PROPERTIES *ppProperties)
{
    AdapterSettings *pAdapter;
    PTCPIP_PROPERTIES pProperties;
    PWSTR pszIpAddress = NULL;
    PWSTR pszSubnetMask = NULL;
    PWSTR pszParameters = NULL;
    PWSTR pPtr;
    DWORD dwSize;
    HRESULT hr = S_OK;

    ERR("ITcpipProperties_fnUnknown1(%s %p)\n", wine_dbgstr_guid(pAdapterName), ppProperties);
    TcpipConfNotifyImpl *This = impl_from_ITcpipProperties(iface);

    pAdapter = GetAdapterByGuid(This, pAdapterName);
    if (pAdapter == NULL)
        return E_FAIL;

    /* Build the IpAddress string */
    hr = BuildIpAddressString(pAdapter, &pszIpAddress);
    if (FAILED(hr))
        goto done;

    TRACE("IpAddress string: %S\n", pszIpAddress);

    /* Build the Parameters string */
    hr = BuildSubnetMaskString(pAdapter, &pszSubnetMask);
    if (FAILED(hr))
        goto done;

    TRACE("SubnetMask string: %S\n", pszSubnetMask);

    /* Build the Parameters string */
    hr = BuildParametersString(pAdapter, &pszParameters);
    if (FAILED(hr))
        goto done;

    TRACE("Parameters string: %S\n", pszParameters);

    dwSize = sizeof(TCPIP_PROPERTIES) +
             ((wcslen(pszIpAddress) + 1) * sizeof(WCHAR)) +
             ((wcslen(pszSubnetMask) + 1) * sizeof(WCHAR)) +
             ((wcslen(pszParameters) + 1) * sizeof(WCHAR));

    pProperties = CoTaskMemAlloc(dwSize);
    if (pProperties == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
 
    ZeroMemory(pProperties, dwSize);

    pProperties->dwDhcp = (pAdapter->NewDhcpEnabled) ? DWORD_MAX : 0;

    pPtr = (PWSTR)(pProperties + 1);

    pProperties->pszIpAddress = pPtr;
    wcscpy(pProperties->pszIpAddress, pszIpAddress);
    pPtr += (wcslen(pszIpAddress) + 1);

    pProperties->pszSubnetMask = pPtr;
    wcscpy(pProperties->pszSubnetMask, pszSubnetMask);
    pPtr += (wcslen(pszSubnetMask) + 1);

    pProperties->pszParameters = pPtr;
    wcscpy(pProperties->pszParameters, pszParameters);

    *ppProperties = pProperties;

done:
    if (pszIpAddress)
        CoTaskMemFree(pszIpAddress);

    if (pszSubnetMask)
        CoTaskMemFree(pszSubnetMask);

    if (pszParameters)
        CoTaskMemFree(pszParameters);

    return hr;
}

static const ITcpipPropertiesVtbl vt_TcpipProperties =
{
    ITcpipProperties_fnQueryInterface,
    ITcpipProperties_fnAddRef,
    ITcpipProperties_fnRelease,
    ITcpipProperties_fnUnknown1,
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
    This->lpVtbl = (const INetCfgComponentControl*)&vt_NetCfgComponentControl;
    This->lpVtblCompPropertyUi = (const INetCfgComponentPropertyUi*)&vt_NetCfgComponentPropertyUi;
    This->lpVtblCompSetup = (const INetCfgComponentSetup*)&vt_NetCfgComponentSetup;
    This->lpVtblTcpipProperties = (const ITcpipProperties*)&vt_TcpipProperties;
    This->pNCfg = NULL;
    This->pUnknown = NULL;
    This->pNComp = NULL;
    This->pTcpIpSettings = NULL;
    This->pAdapterListHead = NULL;
    This->pAdapterListTail = NULL;
    This->pCurrentAdapter = NULL;

    if (!SUCCEEDED (INetCfgComponentControl_QueryInterface ((INetCfgComponentControl*)This, riid, ppv)))
    {
        INetCfgComponentControl_Release((INetCfgComponentControl*)This);
        return E_NOINTERFACE;
    }

    INetCfgComponentControl_Release((INetCfgComponentControl*)This);
    return S_OK;
}
