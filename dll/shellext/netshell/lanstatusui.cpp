/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     CLanStatus: Lan connection status dialog
 * COPYRIGHT:   Copyright 2008 Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "precomp.h"

#include <winsock.h>

CLanStatus::CLanStatus() :
    m_lpNetMan(NULL),
    m_pHead(NULL)
{
}

VOID
UpdateLanStatusUiDlg(
    HWND hwndDlg,
    MIB_IFROW *IfEntry,
    LANSTATUSUI_CONTEXT *pContext)
{
    WCHAR szFormat[MAX_PATH] = {0};
    WCHAR szBuffer[MAX_PATH] = {0};
    SYSTEMTIME TimeConnected;
    DWORD DurationSeconds;
    WCHAR Buffer[100];
    WCHAR DayBuffer[30];
    WCHAR LocBuffer[50];

#if 0
    ULONGLONG Ticks;
#else
    DWORD Ticks;
#endif

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
        if (LoadStringW(netshell_hInstance, IDS_FORMAT_GBIT, szFormat, sizeof(szFormat)/sizeof(WCHAR)))
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

}

VOID
UpdateLanStatus(HWND hwndDlg,  LANSTATUSUI_CONTEXT * pContext)
{
    MIB_IFROW IfEntry;
    HICON hIcon, hOldIcon = NULL;
    NOTIFYICONDATAW nid;
    NETCON_PROPERTIES * pProperties = NULL;

    ZeroMemory(&IfEntry, sizeof(IfEntry));
    IfEntry.dwIndex = pContext->dwAdapterIndex;
    if (GetIfEntry(&IfEntry) != NO_ERROR)
    {
        return;
    }

    hIcon = NULL;
    if (IfEntry.dwOperStatus == MIB_IF_OPER_STATUS_CONNECTED || IfEntry.dwOperStatus == MIB_IF_OPER_STATUS_OPERATIONAL)
    {
        if (pContext->dwInOctets == IfEntry.dwInOctets && pContext->dwOutOctets == IfEntry.dwOutOctets && pContext->Status  != 0)
        {
            hIcon = (HICON)LoadImage(netshell_hInstance, MAKEINTRESOURCE(IDI_NET_IDLE), IMAGE_ICON, 32, 32, LR_SHARED);
            pContext->Status = 0;
        }
        else if (pContext->dwInOctets != IfEntry.dwInOctets && pContext->dwOutOctets != IfEntry.dwOutOctets && pContext->Status  != 1)
        {
            hIcon = (HICON)LoadImage(netshell_hInstance, MAKEINTRESOURCE(IDI_NET_TRANSREC), IMAGE_ICON, 32, 32, LR_SHARED);
            pContext->Status = 1;
        }
        else if (pContext->dwInOctets != IfEntry.dwInOctets && pContext->Status  != 2)
        {
            hIcon = (HICON)LoadImage(netshell_hInstance, MAKEINTRESOURCE(IDI_NET_REC), IMAGE_ICON, 32, 32, LR_SHARED);
            pContext->Status = 2;
        }
        else if (pContext->dwOutOctets != IfEntry.dwOutOctets && pContext->Status  != 3)
        {
            hIcon = (HICON)LoadImage(netshell_hInstance, MAKEINTRESOURCE(IDI_NET_TRANS), IMAGE_ICON, 32, 32, LR_SHARED);
            pContext->Status = 3;
        }
    }
    else if (IfEntry.dwOperStatus == MIB_IF_OPER_STATUS_UNREACHABLE || IfEntry.dwOperStatus == MIB_IF_OPER_STATUS_DISCONNECTED)
    {
        if (pContext->Status != 4)
        {
            hIcon = (HICON)LoadImage(netshell_hInstance, MAKEINTRESOURCE(IDI_NET_OFF), IMAGE_ICON, 32, 32, LR_SHARED);
            pContext->Status = 4;
        }
    }
    else if (IfEntry.dwOperStatus == MIB_IF_OPER_STATUS_NON_OPERATIONAL)
    {
        if (pContext->Status != 5)
        {
            hIcon = (HICON)LoadImage(netshell_hInstance, MAKEINTRESOURCE(IDI_NET_OFF), IMAGE_ICON, 32, 32, LR_SHARED);
            pContext->Status = 5;
        }
    }

    if (hwndDlg && hIcon)
    {
        hOldIcon = (HICON)SendDlgItemMessageW(hwndDlg, IDC_NETSTAT, STM_SETICON, (WPARAM)hIcon, 0);
        if (hOldIcon)
            DestroyIcon(hOldIcon);
    }

    ZeroMemory(&nid, sizeof(nid));
    nid.cbSize = sizeof(nid);
    nid.uID = pContext->uID;
    nid.hWnd = pContext->hwndStatusDlg;
    nid.uVersion = NOTIFYICON_VERSION;

    if (pContext->pNet->GetProperties(&pProperties) == S_OK)
    {
        if (pProperties->dwCharacter & NCCF_SHOW_ICON)
        {
            if (hwndDlg)
                nid.hIcon = (HICON)CopyImage(hIcon, IMAGE_ICON, 16, 16, LR_COPYFROMRESOURCE);
            else
                nid.hIcon = hIcon;

            if (nid.hIcon)
                nid.uFlags |= NIF_ICON;

            nid.uFlags |= NIF_STATE;
            nid.dwState = 0;
            nid.dwStateMask = NIS_HIDDEN;

            if (pProperties->pszwName)
            {
                if (wcslen(pProperties->pszwName) * sizeof(WCHAR) < sizeof(nid.szTip))
                {
                    nid.uFlags |= NIF_TIP;
                    wcscpy(nid.szTip, pProperties->pszwName);
                }
                else
                {
                    CopyMemory(nid.szTip, pProperties->pszwName, sizeof(nid.szTip) - sizeof(WCHAR));
                    nid.szTip[(sizeof(nid.szTip)/sizeof(WCHAR))-1] = L'\0';
                    nid.uFlags |= NIF_TIP;
                }
            }
        }
        else
        {
            nid.uFlags |= NIF_STATE;
            nid.dwState = NIS_HIDDEN;
            nid.dwStateMask = NIS_HIDDEN;

        }
        NcFreeNetconProperties(pProperties);
    }

    Shell_NotifyIconW(NIM_MODIFY, &nid);

    if (nid.uFlags & NIF_ICON)
        DestroyIcon(nid.hIcon);

    pContext->dwInOctets = IfEntry.dwInOctets;
    pContext->dwOutOctets = IfEntry.dwOutOctets;

    if (hwndDlg)
        UpdateLanStatusUiDlg(hwndDlg, &IfEntry, pContext);
}


VOID
InitializeLANStatusUiDlg(HWND hwndDlg, LANSTATUSUI_CONTEXT * pContext)
{
    WCHAR szBuffer[MAX_PATH] = {0};
    NETCON_PROPERTIES * pProperties;

    if (pContext->pNet->GetProperties(&pProperties) != S_OK)
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

    pContext->dwInOctets = 0;
    pContext->dwOutOctets = 0;

    /* update adapter info */
    pContext->Status = -1;
    UpdateLanStatus(hwndDlg, pContext);
    NcFreeNetconProperties(pProperties);
}

static
VOID
InsertColumnToListView(
    HWND hDlgCtrl,
    UINT ResId,
    UINT SubItem,
    UINT Size)
{
    WCHAR szBuffer[200];
    LVCOLUMNW lc;

    if (!LoadStringW(netshell_hInstance, ResId, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
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

static
VOID
AddIPAddressToListView(
    HWND hDlgCtrl,
    PIP_ADDR_STRING pAddr,
    INT Index)
{
    LVITEMW li;
    PIP_ADDR_STRING pCur;
    WCHAR szBuffer[100];
    UINT SubIndex;

    ZeroMemory(&li, sizeof(LVITEMW));
    li.mask = LVIF_TEXT;
    li.iItem = Index;
    pCur = pAddr;
    SubIndex = 0;

    do
    {
        if (SubIndex)
        {
            ZeroMemory(&li, sizeof(LVITEMW));
            li.mask = LVIF_TEXT;
            li.iItem = Index;
            li.iSubItem = 0;
            li.pszText = (LPWSTR)L"";
            li.iItem = SendMessageW(hDlgCtrl, LVM_INSERTITEMW, 0, (LPARAM)&li);
        }

        if (MultiByteToWideChar(CP_ACP, 0, pCur->IpAddress.String, -1, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
        {
            li.pszText = szBuffer;
            li.iSubItem = 1;
            li.iItem = Index++;
            SendMessageW(hDlgCtrl, LVM_SETITEMW, 0, (LPARAM)&li);
        }
        SubIndex++;
        pCur = pCur->Next;
    } while (pCur && pCur->IpAddress.String[0]);
}

static
INT
InsertItemToListView(
    HWND hDlgCtrl,
    UINT ResId)
{
    LVITEMW li;
    WCHAR szBuffer[100];

    ZeroMemory(&li, sizeof(LVITEMW));
    li.mask = LVIF_TEXT;
    li.iItem = ListView_GetItemCount(hDlgCtrl);
    if (LoadStringW(netshell_hInstance, ResId, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
    {
        li.pszText = szBuffer;
        return (INT)SendMessageW(hDlgCtrl, LVM_INSERTITEMW, 0, (LPARAM)&li);
    }
    return -1;
}

static
BOOL
tmToStr(
    IN struct tm *pTM,
    OUT LPWSTR szBuffer,
    IN UINT nBufferSize)
{
    SYSTEMTIME st;
    CString strBufferDate;
    CString strBufferTime;
    UINT nCharDate, nCharTime;
    BOOL bResult = FALSE;

    st.wYear = pTM->tm_year + 1900;
    st.wMonth = pTM->tm_mon + 1;
    st.wDay = pTM->tm_mday;
    st.wHour = pTM->tm_hour;
    st.wMinute = pTM->tm_min;
    st.wSecond = pTM->tm_sec;

    /* Check required size before cpy/cat */
    nCharDate = GetDateFormatW(LOCALE_USER_DEFAULT, 0, &st, NULL, NULL, 0) + 1;
    nCharTime = GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &st, NULL, NULL, 0) + 1;

    if (GetDateFormatW(LOCALE_USER_DEFAULT, 0, &st, NULL, strBufferDate.GetBuffer(nCharDate), nCharDate) &&
        GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &st, NULL, strBufferTime.GetBuffer(nCharTime), nCharTime))
    {
        StringCbCopy(szBuffer, nBufferSize, strBufferDate);
        StringCbCat(szBuffer, nBufferSize, L" ");
        StringCbCat(szBuffer, nBufferSize, strBufferTime);
        bResult = TRUE;
    }
    strBufferDate.ReleaseBuffer();
    strBufferTime.ReleaseBuffer();

    return bResult;
}

INT_PTR
CALLBACK
LANStatusUiDetailsDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    LANSTATUSUI_CONTEXT * pContext;
    LVITEMW li;
    WCHAR szBuffer[100];
    PIP_ADAPTER_INFO pAdapterInfo, pCurAdapter;
    PIP_PER_ADAPTER_INFO pPerAdapter;
    DWORD dwSize;
    HWND hDlgCtrl;
    RECT rect;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pContext = (LANSTATUSUI_CONTEXT*)lParam;

            hDlgCtrl = GetDlgItem(hwndDlg, IDC_DETAILS);

            /* get client rect */
            GetClientRect(hDlgCtrl, &rect);

            /* calculate column width */
            dwSize = rect.right / 2;

            InsertColumnToListView(hDlgCtrl, IDS_PROPERTY, 0, dwSize);
            InsertColumnToListView(hDlgCtrl, IDS_VALUE, 1, dwSize);

            dwSize = 0;
            pCurAdapter = NULL;
            pAdapterInfo = NULL;
            if (GetAdaptersInfo(NULL, &dwSize) == ERROR_BUFFER_OVERFLOW)
            {
                pAdapterInfo = static_cast<PIP_ADAPTER_INFO>(CoTaskMemAlloc(dwSize));
                if (pAdapterInfo)
                {
                    if (GetAdaptersInfo(pAdapterInfo, &dwSize) == NO_ERROR)
                    {
                        pCurAdapter = pAdapterInfo;
                        while (pCurAdapter && pCurAdapter->Index != pContext->dwAdapterIndex)
                            pCurAdapter = pCurAdapter->Next;

                        if (pCurAdapter->Index != pContext->dwAdapterIndex)
                            pCurAdapter = NULL;
                    }
                }
            }

            ZeroMemory(&li, sizeof(LVITEMW));
            li.mask = LVIF_TEXT;
            li.iSubItem = 1;
            li.pszText = szBuffer;

            if (pCurAdapter)
            {
                li.iItem = InsertItemToListView(hDlgCtrl, IDS_PHYSICAL_ADDRESS);
                if (li.iItem >= 0)
                {
                    swprintf(szBuffer, L"%02x-%02x-%02x-%02x-%02x-%02x",pCurAdapter->Address[0], pCurAdapter->Address[1],
                             pCurAdapter->Address[2], pCurAdapter->Address[3], pCurAdapter->Address[4], pCurAdapter->Address[5]);
                    SendMessageW(hDlgCtrl, LVM_SETITEMW, 0, (LPARAM)&li);
                }
                li.iItem = InsertItemToListView(hDlgCtrl, IDS_IP_ADDRESS);
                if (li.iItem >= 0)
                    if (MultiByteToWideChar(CP_ACP, 0, pCurAdapter->IpAddressList.IpAddress.String, -1, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
                        SendMessageW(hDlgCtrl, LVM_SETITEMW, 0, (LPARAM)&li);

                li.iItem = InsertItemToListView(hDlgCtrl, IDS_SUBNET_MASK);
                if (li.iItem >= 0)
                    if (MultiByteToWideChar(CP_ACP, 0, pCurAdapter->IpAddressList.IpMask.String, -1, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
                        SendMessageW(hDlgCtrl, LVM_SETITEMW, 0, (LPARAM)&li);

                li.iItem = InsertItemToListView(hDlgCtrl, IDS_DEF_GATEWAY);
                if (li.iItem >= 0 && pCurAdapter->GatewayList.IpAddress.String[0] != '0')
                {
                    if (MultiByteToWideChar(CP_ACP, 0, pCurAdapter->GatewayList.IpAddress.String, -1, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
                        SendMessageW(hDlgCtrl, LVM_SETITEMW, 0, (LPARAM)&li);
                }

                li.iItem = InsertItemToListView(hDlgCtrl, IDS_DHCP_SERVER);
                if (li.iItem >= 0 && pCurAdapter->DhcpServer.IpAddress.String[0] != '0')
                {
                    if (MultiByteToWideChar(CP_ACP, 0, pCurAdapter->DhcpServer.IpAddress.String, -1, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
                        SendMessageW(hDlgCtrl, LVM_SETITEMW, 0, (LPARAM)&li);
                }

                li.iItem = InsertItemToListView(hDlgCtrl, IDS_LEASE_OBTAINED);
                if (li.iItem >= 0 && pCurAdapter->LeaseObtained != NULL)
                {
                    struct tm *leaseOptained;

                    leaseOptained = localtime(&pCurAdapter->LeaseObtained);

                    if (tmToStr(leaseOptained, szBuffer, _countof(szBuffer)))
                        SendMessageW(hDlgCtrl, LVM_SETITEMW, 0, (LPARAM)&li);
                }

                li.iItem = InsertItemToListView(hDlgCtrl, IDS_LEASE_EXPIRES);
                if (li.iItem >= 0 && pCurAdapter->LeaseExpires != NULL)
                {
                    struct tm *leaseExpire;

                    leaseExpire = localtime(&pCurAdapter->LeaseExpires);

                    if (tmToStr(leaseExpire, szBuffer, _countof(szBuffer)))
                        SendMessageW(hDlgCtrl, LVM_SETITEMW, 0, (LPARAM)&li);
                }
            }

            dwSize = 0;
            li.iItem = InsertItemToListView(hDlgCtrl, IDS_DNS_SERVERS);
            if (GetPerAdapterInfo(pContext->dwAdapterIndex, NULL, &dwSize) == ERROR_BUFFER_OVERFLOW)
            {
                pPerAdapter = static_cast<PIP_PER_ADAPTER_INFO>(CoTaskMemAlloc(dwSize));
                if (pPerAdapter)
                {
                    if (GetPerAdapterInfo(pContext->dwAdapterIndex, pPerAdapter, &dwSize) == ERROR_SUCCESS)
                    {
                        if (li.iItem >= 0)
                            AddIPAddressToListView(hDlgCtrl, &pPerAdapter->DnsServerList, li.iItem);
                    }
                    CoTaskMemFree(pPerAdapter);
                }
            }

            if (pCurAdapter)
            {
                li.iItem = InsertItemToListView(hDlgCtrl, IDS_WINS_SERVERS);
                if (pCurAdapter->HaveWins)
                {
                    AddIPAddressToListView(hDlgCtrl, &pCurAdapter->PrimaryWinsServer, li.iItem);
                    AddIPAddressToListView(hDlgCtrl, &pCurAdapter->SecondaryWinsServer, li.iItem+1);
                }
            }

            CoTaskMemFree(pAdapterInfo);
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_CLOSE)
            {
                EndDialog(hwndDlg, FALSE);
                break;
            }
    }

    return FALSE;
}

INT_PTR
CALLBACK
LANStatusUiAdvancedDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    WCHAR szBuffer[100] = {0};
    PROPSHEETPAGE *page;
    LANSTATUSUI_CONTEXT * pContext;
    DWORD dwIpAddr;


    switch (uMsg)
    {
        case WM_INITDIALOG:
            page = (PROPSHEETPAGE*)lParam;
            pContext = (LANSTATUSUI_CONTEXT*)page->lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pContext);
            if (pContext->DHCPEnabled)
                LoadStringW(netshell_hInstance, IDS_ASSIGNED_DHCP, szBuffer, sizeof(szBuffer)/sizeof(WCHAR));
            else
                LoadStringW(netshell_hInstance, IDS_ASSIGNED_MANUAL, szBuffer, sizeof(szBuffer)/sizeof(WCHAR));

            szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
            SendDlgItemMessageW(hwndDlg, IDC_DETAILSTYPE, WM_SETTEXT, 0, (LPARAM)szBuffer);


            dwIpAddr = ntohl(pContext->IpAddress);
            swprintf(szBuffer, L"%u.%u.%u.%u", FIRST_IPADDRESS(dwIpAddr), SECOND_IPADDRESS(dwIpAddr),
                     THIRD_IPADDRESS(dwIpAddr), FOURTH_IPADDRESS(dwIpAddr));
            SendDlgItemMessageW(hwndDlg, IDC_DETAILSIP, WM_SETTEXT, 0, (LPARAM)szBuffer);

            dwIpAddr = ntohl(pContext->SubnetMask);
            swprintf(szBuffer, L"%u.%u.%u.%u", FIRST_IPADDRESS(dwIpAddr), SECOND_IPADDRESS(dwIpAddr),
                     THIRD_IPADDRESS(dwIpAddr), FOURTH_IPADDRESS(dwIpAddr));
            SendDlgItemMessageW(hwndDlg, IDC_DETAILSSUBNET, WM_SETTEXT, 0, (LPARAM)szBuffer);

            dwIpAddr = ntohl(pContext->Gateway);
            if (dwIpAddr)
            {
                swprintf(szBuffer, L"%u.%u.%u.%u", FIRST_IPADDRESS(dwIpAddr), SECOND_IPADDRESS(dwIpAddr),
                         THIRD_IPADDRESS(dwIpAddr), FOURTH_IPADDRESS(dwIpAddr));
                SendDlgItemMessageW(hwndDlg, IDC_DETAILSGATEWAY, WM_SETTEXT, 0, (LPARAM)szBuffer);
            }
            return TRUE;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_DETAILS)
            {
                pContext = (LANSTATUSUI_CONTEXT*)GetWindowLongPtr(hwndDlg, DWLP_USER);
                if (pContext)
                {
                    DialogBoxParamW(netshell_hInstance, MAKEINTRESOURCEW(IDD_LAN_NETSTATUSDETAILS), GetParent(hwndDlg),
                                    LANStatusUiDetailsDlg, (LPARAM)pContext);
                }
            }
            break;
        default:
            break;
    }
    return FALSE;
}

VOID
DisableNetworkAdapter(INetConnection * pNet, LANSTATUSUI_CONTEXT * pContext, HWND hwndDlg)
{
    HRESULT hr = pNet->Disconnect();
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    NOTIFYICONDATAW nid;

    PropSheet_PressButton(GetParent(hwndDlg), PSBTN_CANCEL);
    ZeroMemory(&nid, sizeof(nid));
    nid.cbSize = sizeof(nid);
    nid.uID = pContext->uID;
    nid.hWnd = pContext->hwndDlg;
    nid.uFlags = NIF_STATE;
    nid.dwState = NIS_HIDDEN;
    nid.dwStateMask = NIS_HIDDEN;

    Shell_NotifyIconW(NIM_MODIFY, &nid);
}


INT_PTR
CALLBACK
LANStatusUiDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PROPSHEETPAGE *page;
    LANSTATUSUI_CONTEXT * pContext;
    LPPSHNOTIFY lppsn;

    switch (uMsg)
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
                DisableNetworkAdapter(pContext->pNet, pContext, hwndDlg);
                break;
            }
        case WM_NOTIFY:
            lppsn = (LPPSHNOTIFY) lParam;
            if (lppsn->hdr.code == PSN_APPLY || lppsn->hdr.code == PSN_RESET)
            {
                pContext = (LANSTATUSUI_CONTEXT*)GetWindowLongPtr(hwndDlg, DWLP_USER);
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                pContext->hwndDlg = NULL;
                return TRUE;
            }
            break;
    }
    return FALSE;
}

VOID
InitializePropertyDialog(
    LANSTATUSUI_CONTEXT * pContext,
    NETCON_PROPERTIES * pProperties)
{
    DWORD dwSize, dwAdapterIndex, dwResult;
    LPOLESTR pStr;
    IP_ADAPTER_INFO *pAdapterInfo, *pCurAdapter;

    if (FAILED(StringFromCLSID((CLSID)pProperties->guidId, &pStr)))
    {
        return;
    }

    /* get the IfTable */
    dwSize = 0;
    dwResult = GetAdaptersInfo(NULL, &dwSize);
    if (dwResult!= ERROR_BUFFER_OVERFLOW)
    {
        CoTaskMemFree(pStr);
        return;
    }

    pAdapterInfo = static_cast<PIP_ADAPTER_INFO>(CoTaskMemAlloc(dwSize));
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

    pCurAdapter = pAdapterInfo;
    while (pCurAdapter->Index != dwAdapterIndex)
        pCurAdapter = pCurAdapter->Next;


    pContext->IpAddress = inet_addr(pCurAdapter->IpAddressList.IpAddress.String);
    pContext->SubnetMask = inet_addr(pCurAdapter->IpAddressList.IpMask.String);
    pContext->Gateway = inet_addr(pCurAdapter->GatewayList.IpAddress.String);
    pContext->DHCPEnabled = pCurAdapter->DhcpEnabled;
    CoTaskMemFree(pStr);
    CoTaskMemFree(pAdapterInfo);
    pContext->dwAdapterIndex = dwAdapterIndex;
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
            hIcon = LoadIconW(netshell_hInstance, MAKEINTRESOURCEW(IDI_NET_IDLE));
            SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            break;
        }
    }
    return 0;
}

VOID
ShowStatusPropertyDialog(
    LANSTATUSUI_CONTEXT *pContext,
    HWND hwndDlg)
{
    HPROPSHEETPAGE hppages[2];
    PROPSHEETHEADERW pinfo;
    NETCON_PROPERTIES * pProperties = NULL;

    ZeroMemory(&pinfo, sizeof(PROPSHEETHEADERW));
    ZeroMemory(hppages, sizeof(hppages));
    pinfo.dwSize = sizeof(PROPSHEETHEADERW);
    pinfo.dwFlags = PSH_NOCONTEXTHELP | PSH_PROPTITLE | PSH_NOAPPLYNOW |
                    PSH_USEICONID | PSH_USECALLBACK;
    pinfo.phpage = hppages;
    pinfo.hwndParent = hwndDlg;
    pinfo.hInstance = netshell_hInstance;
    pinfo.pszIcon = MAKEINTRESOURCEW(IDI_NET_IDLE);
    pinfo.pfnCallback = PropSheetProc;

    if (pContext->pNet->GetProperties(&pProperties) == S_OK)
    {
        if (pProperties->pszwName)
        {
            pinfo.pszCaption = pProperties->pszwName;
            pinfo.dwFlags |= PSH_PROPTITLE;
        }
        InitializePropertyDialog(pContext, pProperties);
        if (pProperties->MediaType == NCM_LAN && pProperties->Status == NCS_CONNECTED)
        {
            hppages[0] = InitializePropertySheetPage(MAKEINTRESOURCEW(IDD_LAN_NETSTATUS), LANStatusUiDlg, (LPARAM)pContext, NULL);
            if (hppages[0])
               pinfo.nPages++;

            hppages[pinfo.nPages] = InitializePropertySheetPage(MAKEINTRESOURCEW(IDD_LAN_NETSTATUSADVANCED), LANStatusUiAdvancedDlg, (LPARAM)pContext, NULL);
            if (hppages[pinfo.nPages])
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
    LPARAM lParam)
{
    LANSTATUSUI_CONTEXT * pContext;

    switch (uMsg)
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
                UpdateLanStatus(pContext->hwndDlg, pContext);
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

HRESULT
CLanStatus::InitializeNetTaskbarNotifications()
{
    NOTIFYICONDATAW nid;
    HWND hwndDlg;
    CComPtr<INetConnectionManager> pNetConMan;
    CComPtr<IEnumNetConnection> pEnumCon;
    CComPtr<INetConnection> pNetCon;
    NETCON_PROPERTIES* pProps;
    HRESULT hr;
    ULONG Count;
    ULONG Index;
    NOTIFICATION_ITEM * pItem, *pLast = NULL;
    LANSTATUSUI_CONTEXT * pContext;

    TRACE("InitializeNetTaskbarNotifications\n");

    if (m_pHead)
    {
       pItem = m_pHead;
       while (pItem)
       {
           hr = pItem->pNet->GetProperties(&pProps);
           if (SUCCEEDED(hr))
           {
                ZeroMemory(&nid, sizeof(nid));
                nid.cbSize = sizeof(nid);
                nid.uID = pItem->uID;
                nid.hWnd = pItem->hwndDlg;
                nid.uFlags = NIF_STATE;
                if (pProps->dwCharacter & NCCF_SHOW_ICON)
                    nid.dwState = 0;
                else
                    nid.dwState = NIS_HIDDEN;

                nid.dwStateMask = NIS_HIDDEN;
                Shell_NotifyIconW(NIM_MODIFY, &nid);
                NcFreeNetconProperties(pProps);
           }
           pItem = pItem->pNext;
       }
       return S_OK;
    }
    /* get an instance to of IConnectionManager */
    hr = CNetConnectionManager_CreateInstance(IID_PPV_ARG(INetConnectionManager, &pNetConMan));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pNetConMan->EnumConnections(NCME_DEFAULT, &pEnumCon);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    Index = 1;
    while (TRUE)
    {
        pNetCon.Release();
        hr = pEnumCon->Next(1, &pNetCon, &Count);
        if (hr != S_OK)
            break;

        TRACE("new connection\n");
        pItem = static_cast<NOTIFICATION_ITEM*>(CoTaskMemAlloc(sizeof(NOTIFICATION_ITEM)));
        if (!pItem)
            break;

        pContext = static_cast<LANSTATUSUI_CONTEXT*>(CoTaskMemAlloc(sizeof(LANSTATUSUI_CONTEXT)));
        if (!pContext)
        {
            CoTaskMemFree(pItem);
            break;
        }

        ZeroMemory(pContext, sizeof(LANSTATUSUI_CONTEXT));
        pContext->uID = Index;
        pContext->pNet = pNetCon;
        pItem->uID = Index;
        pItem->pNext = NULL;
        pItem->pNet = pNetCon;
        pNetCon->AddRef();
        hwndDlg = CreateDialogParamW(netshell_hInstance, MAKEINTRESOURCEW(IDD_STATUS), NULL, LANStatusDlg, (LPARAM)pContext);
        if (!hwndDlg)
        {
            ERR("CreateDialogParamW failed\n");
            continue;
        }

        ZeroMemory(&nid, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.uID = Index++;
        nid.uFlags = NIF_MESSAGE;
        nid.uVersion = NOTIFYICON_VERSION;
        nid.uCallbackMessage = WM_SHOWSTATUSDLG;
        nid.hWnd = hwndDlg;

        hr = pNetCon->GetProperties(&pProps);
        if (SUCCEEDED(hr))
        {
            CopyMemory(&pItem->guidItem, &pProps->guidId, sizeof(GUID));
            if (!(pProps->dwCharacter & NCCF_SHOW_ICON))
            {
                nid.dwState = NIS_HIDDEN;
                nid.dwStateMask = NIS_HIDDEN;
                nid.uFlags |= NIF_STATE;
            }
            if (pProps->Status == NCS_MEDIA_DISCONNECTED || pProps->Status == NCS_DISCONNECTED || pProps->Status == NCS_HARDWARE_DISABLED)
                nid.hIcon = LoadIcon(netshell_hInstance, MAKEINTRESOURCE(IDI_NET_OFF));
            else if (pProps->Status == NCS_CONNECTED)
                nid.hIcon = LoadIcon(netshell_hInstance, MAKEINTRESOURCE(IDI_NET_IDLE));

            if (nid.hIcon)
                nid.uFlags |= NIF_ICON;

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
                m_pHead = pItem;

            pLast = pItem;
            Index++;
        }
        else
        {
            ERR("Shell_NotifyIconW failed\n");
            CoTaskMemFree(pItem);
        }

        if (nid.uFlags & NIF_ICON)
            DestroyIcon(nid.hIcon);
    }

    m_lpNetMan = pNetConMan;
    return S_OK;
}

HRESULT
CLanStatus::ShowStatusDialogByCLSID(const GUID *pguidCmdGroup)
{
    NOTIFICATION_ITEM *pItem;

    pItem = m_pHead;
    while (pItem)
    {
        if (IsEqualGUID(pItem->guidItem, *pguidCmdGroup))
        {
            SendMessageW(pItem->hwndDlg, WM_SHOWSTATUSDLG, 0, WM_LBUTTONDOWN);
            return S_OK;
        }
        pItem = pItem->pNext;
    }

    ERR("not found\n");
    return E_FAIL;
}

HRESULT
WINAPI
CLanStatus::QueryStatus(
    const GUID *pguidCmdGroup,
    ULONG cCmds,
    OLECMD *prgCmds,
    OLECMDTEXT *pCmdText)
{
    MessageBoxW(NULL, pCmdText->rgwz, L"IOleCommandTarget_fnQueryStatus", MB_OK);
    return E_NOTIMPL;
}

HRESULT
WINAPI
CLanStatus::Exec(
    const GUID *pguidCmdGroup,
    DWORD nCmdID,
    DWORD nCmdexecopt,
    VARIANT *pvaIn,
    VARIANT *pvaOut)
{
    if (pguidCmdGroup)
    {
        if (IsEqualGUID(*pguidCmdGroup, CGID_ShellServiceObject))
        {
            return InitializeNetTaskbarNotifications();
        }
        else
        {
            /* invoke status dialog */
            return ShowStatusDialogByCLSID(pguidCmdGroup);
        }
    }
    return S_OK;
}
