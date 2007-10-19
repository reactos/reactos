/*
 * PROJECT:     ReactOS Network Control Panel
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/cpl/system/tcpip_properties.c
 * PURPOSE:     ReactOS Network Control Panel
 * COPYRIGHT:   Copyright 2004 Gero Kuehn (reactos.filter@gkware.com)
 *              Copyright 2006 Ge van Geldorp <gvg@reactos.org>
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>
#include <windows.h>
#include <iptypes.h>
#include <iphlpapi.h>
#include <commctrl.h>
#include <dhcpcapi.h>
#include <prsht.h>


#ifdef _MSC_VER
#include <cpl.h>
#else

// this is missing on reactos...
#ifndef IPM_SETADDRESS
#define IPM_SETADDRESS (WM_USER+101)
#endif

#endif

#include "resource.h"
#include "ncpa.h"

#define NDEBUG
#include <debug.h>

typedef struct _TCPIP_PROPERTIES_DATA {
    DWORD AdapterIndex;
    char *AdapterName;
    BOOL DhcpEnabled;
    DWORD IpAddress;
    DWORD SubnetMask;
    DWORD Gateway;
    DWORD Dns1;
    DWORD Dns2;
    BOOL OldDhcpEnabled;
    DWORD OldIpAddress;
    DWORD OldSubnetMask;
    DWORD OldGateway;
    DWORD OldDns1;
    DWORD OldDns2;
} TCPIP_PROPERTIES_DATA, *PTCPIP_PROPERTIES_DATA;

void InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc, LPARAM lParam);

DWORD APIENTRY DhcpNotifyConfigChange(LPWSTR ServerName, LPWSTR AdapterName,
                                      BOOL NewIpAddress, DWORD IpIndex,
                                      DWORD IpAddress, DWORD SubnetMask,
                                      int DhcpAction);


static void
ManualDNS(HWND Dlg, BOOL Enabled, UINT uCmd) {
    PTCPIP_PROPERTIES_DATA DlgData =
        (PTCPIP_PROPERTIES_DATA) GetWindowLongPtrW(Dlg, GWL_USERDATA);

    if (! DlgData->OldDhcpEnabled &&
        (uCmd == IDC_USEDHCP || uCmd == IDC_NODHCP)) {
        if (INADDR_NONE != DlgData->OldIpAddress) {
            SendDlgItemMessage(Dlg, IDC_IPADDR, IPM_SETADDRESS, 0,
                               ntohl(DlgData->OldIpAddress));
        }
        if (INADDR_NONE != DlgData->OldSubnetMask) {
            SendDlgItemMessage(Dlg, IDC_SUBNETMASK, IPM_SETADDRESS, 0,
                               ntohl(DlgData->OldSubnetMask));
        }
        if (INADDR_NONE != DlgData->OldGateway) {
            SendDlgItemMessage(Dlg, IDC_DEFGATEWAY, IPM_SETADDRESS, 0,
                               ntohl(DlgData->OldGateway));
        }
    }

    if (INADDR_NONE != DlgData->OldDns1 &&
        (uCmd == IDC_FIXEDDNS || uCmd == IDC_AUTODNS || IDC_NODHCP)) {
        SendDlgItemMessage(Dlg, IDC_DNS1, IPM_SETADDRESS, 0,
                           ntohl(DlgData->OldDns1));
        if (INADDR_NONE != DlgData->OldDns2) {
            SendDlgItemMessage(Dlg, IDC_DNS2, IPM_SETADDRESS, 0,
                               ntohl(DlgData->OldDns2));
        }
    }

    CheckDlgButton(Dlg, IDC_FIXEDDNS,
                   Enabled ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(Dlg, IDC_AUTODNS,
                   Enabled ? BST_UNCHECKED : BST_CHECKED);
    EnableWindow(GetDlgItem(Dlg, IDC_DNS1), Enabled);
    EnableWindow(GetDlgItem(Dlg, IDC_DNS2), FALSE/*Enabled*/);
    if (! Enabled) {
        SendDlgItemMessage(Dlg, IDC_DNS1, IPM_CLEARADDRESS, 0, 0);
        SendDlgItemMessage(Dlg, IDC_DNS2, IPM_CLEARADDRESS, 0, 0);

    }
}

static void
EnableDHCP(HWND Dlg, BOOL Enabled, UINT uCmd) {
    CheckDlgButton(Dlg, IDC_USEDHCP, Enabled ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(Dlg, IDC_NODHCP, Enabled ? BST_UNCHECKED : BST_CHECKED);
    EnableWindow(GetDlgItem(Dlg, IDC_IPADDR), ! Enabled);
    EnableWindow(GetDlgItem(Dlg, IDC_SUBNETMASK), ! Enabled);
    EnableWindow(GetDlgItem(Dlg, IDC_DEFGATEWAY), ! Enabled);
    EnableWindow(GetDlgItem(Dlg, IDC_AUTODNS), Enabled);
    if (Enabled) {
        SendDlgItemMessage(Dlg, IDC_IPADDR, IPM_CLEARADDRESS, 0, 0);
        SendDlgItemMessage(Dlg, IDC_SUBNETMASK, IPM_CLEARADDRESS, 0, 0);
        SendDlgItemMessage(Dlg, IDC_DEFGATEWAY, IPM_CLEARADDRESS, 0, 0);
    } else {
        ManualDNS(Dlg, TRUE, uCmd);
    }
}

static void
ShowError(HWND Parent, UINT MsgId)
{
    WCHAR Error[32], Msg[64];

    if (0 == LoadStringW((HINSTANCE) GetWindowLongPtrW(Parent, GWLP_HINSTANCE),
                         IDS_ERROR, Error, sizeof(Error) / sizeof(Error[0]))) {
        wcscpy(Error, L"Error");
    }
    if (0 == LoadStringW((HINSTANCE) GetWindowLongPtrW(Parent, GWLP_HINSTANCE),
                         MsgId, Msg, sizeof(Msg) / sizeof(Msg[0]))) {
        wcscpy(Msg, L"Unknown error");
    }
    MessageBoxW(Parent, Msg, Error, MB_OK | MB_ICONSTOP);
}

static
BOOL GetAddressFromField( HWND hwndDlg, UINT CtlId,
                          DWORD *dwIPAddr,
                          const char **AddressString ) {
    LRESULT lResult;
    struct in_addr inIPAddr;

    *AddressString = NULL;

    lResult = SendMessage(GetDlgItem(hwndDlg, CtlId), IPM_GETADDRESS, 0,
                          (ULONG_PTR)dwIPAddr);
    if( lResult != 4 ) return FALSE;

    *dwIPAddr = htonl(*dwIPAddr);
    inIPAddr.s_addr = *dwIPAddr;
    *AddressString = inet_ntoa(inIPAddr);
    if( !*AddressString ) return FALSE;

    return TRUE;
}

static BOOL
ValidateAndStore(HWND Dlg, PTCPIP_PROPERTIES_DATA DlgData)
{
    DWORD IpAddress;
    MIB_IPFORWARDROW RowToAdd = { 0 }, RowToRem = { 0 };

    DlgData->DhcpEnabled = (BST_CHECKED ==
                            IsDlgButtonChecked(Dlg, IDC_USEDHCP));
    if (! DlgData->DhcpEnabled) {
	DhcpReleaseIpAddressLease( DlgData->AdapterIndex );

        if (4 != SendMessageW(GetDlgItem(Dlg, IDC_IPADDR), IPM_GETADDRESS,
                              0, (LPARAM) &IpAddress)) {
            ShowError(Dlg, IDS_ENTER_VALID_IPADDRESS);
            SetFocus(GetDlgItem(Dlg, IDC_IPADDR));
            return FALSE;
        }
        DlgData->IpAddress = htonl(IpAddress);
        if (4 != SendMessageW(GetDlgItem(Dlg, IDC_SUBNETMASK), IPM_GETADDRESS,
                              0, (LPARAM) &IpAddress)) {
            ShowError(Dlg, IDS_ENTER_VALID_SUBNET);
            SetFocus(GetDlgItem(Dlg, IDC_SUBNETMASK));
            return FALSE;
        }
        DlgData->SubnetMask = htonl(IpAddress);
        if (4 != SendMessageW(GetDlgItem(Dlg, IDC_DEFGATEWAY), IPM_GETADDRESS,
                              0, (LPARAM) &IpAddress)) {
            DlgData->Gateway = INADDR_NONE;
        } else {
            DlgData->Gateway = htonl(IpAddress);
        }
	DhcpStaticRefreshParams
	    ( DlgData->AdapterIndex, DlgData->IpAddress, DlgData->SubnetMask );

	RowToRem.dwForwardMask = 0;
	RowToRem.dwForwardMetric1 = 1;
	RowToRem.dwForwardNextHop = DlgData->OldGateway;

	DeleteIpForwardEntry( &RowToRem );

	RowToAdd.dwForwardMask = 0;
	RowToAdd.dwForwardMetric1 = 1;
	RowToAdd.dwForwardNextHop = DlgData->Gateway;

	CreateIpForwardEntry( &RowToAdd );
        ASSERT(BST_CHECKED == IsDlgButtonChecked(Dlg, IDC_FIXEDDNS));
    } else {
        DlgData->IpAddress = INADDR_NONE;
        DlgData->SubnetMask = INADDR_NONE;
        DlgData->Gateway = INADDR_NONE;
	DhcpLeaseIpAddress( DlgData->AdapterIndex );
    }

    if (BST_CHECKED == IsDlgButtonChecked(Dlg, IDC_FIXEDDNS)) {
        if (4 != SendMessageW(GetDlgItem(Dlg, IDC_DNS1), IPM_GETADDRESS,
                              0, (LPARAM) &IpAddress)) {
            DlgData->Dns1 = INADDR_NONE;
        } else {
            DlgData->Dns1 = htonl(IpAddress);
        }
        if (4 != SendMessageW(GetDlgItem(Dlg, IDC_DNS2), IPM_GETADDRESS,
                              0, (LPARAM) &IpAddress)) {
            DlgData->Dns2 = INADDR_NONE;
        } else {
            DlgData->Dns2 = htonl(IpAddress);
        }
    } else {
        DlgData->Dns1 = INADDR_NONE;
        DlgData->Dns2 = INADDR_NONE;
    }

    return TRUE;
}

static BOOL
InternTCPIPSettings(HWND Dlg, PTCPIP_PROPERTIES_DATA DlgData) {
    /*BOOL Changed;
    BOOL IpChanged;
    int DhcpAction;
    LPWSTR AdapterName;*/
    BOOL SetIpAddressByDhcp;
    BOOL SetDnsByDhcp;
    TCHAR pszRegKey[MAX_PATH];
    const char *AddressString;
    DWORD Address = 0;
    LONG rc;
    HKEY hKey = NULL;
    BOOL ret = FALSE;

    if (! ValidateAndStore(Dlg, DlgData)) {
        /* Should never happen, we should have validated at PSN_KILLACTIVE */
        ASSERT(FALSE);
        return FALSE;
    }

    SetIpAddressByDhcp = IsDlgButtonChecked(Dlg, IDC_USEDHCP);
    SetDnsByDhcp = IsDlgButtonChecked(Dlg, IDC_AUTODNS);

    /* Save parameters in HKLM\SYSTEM\CurrentControlSet\Services\{GUID}\Parameters\Tcpip */
    _stprintf(pszRegKey,_T("SYSTEM\\CurrentControlSet\\Services\\%S\\Parameters\\Tcpip"), DlgData->AdapterName);
    rc = RegOpenKey(HKEY_LOCAL_MACHINE, pszRegKey, &hKey);
    if (rc != ERROR_SUCCESS)
        goto cleanup;
    if (SetIpAddressByDhcp)
        AddressString = "0.0.0.0";
    else if (!GetAddressFromField(Dlg, IDC_IPADDR, &Address, &AddressString))
        goto cleanup;
    rc = RegSetValueExA(hKey, "IPAddress", 0, REG_SZ, (const BYTE*)AddressString, strlen(AddressString) + 1);
    if (rc != ERROR_SUCCESS)
        goto cleanup;
    if (!SetIpAddressByDhcp && !GetAddressFromField(Dlg, IDC_SUBNETMASK, &Address, &AddressString))
        goto cleanup;
    rc = RegSetValueExA(hKey, "SubnetMask", 0, REG_SZ, (const BYTE*)AddressString, strlen(AddressString) + 1);
    if (rc != ERROR_SUCCESS)
        goto cleanup;
    if (!SetIpAddressByDhcp && !GetAddressFromField(Dlg, IDC_DEFGATEWAY, &Address, &AddressString))
        goto cleanup;
    rc = RegSetValueExA(hKey, "DefaultGateway", 0, REG_SZ, (const BYTE*)AddressString, strlen(AddressString) + 1);
    if (rc != ERROR_SUCCESS)
        goto cleanup;
    RegCloseKey(hKey);

    /* Save parameters in HKLM\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters */
    rc = RegOpenKey(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), &hKey);
    if (rc != ERROR_SUCCESS)
        goto cleanup;
    if (DlgData->Dns1 == INADDR_NONE)
    {
        rc = RegDeleteValue(hKey, _T("NameServer"));
        if (rc != ERROR_SUCCESS && rc != ERROR_FILE_NOT_FOUND)
            goto cleanup;
    }
    else
    {
        if (!GetAddressFromField(Dlg, IDC_DNS1, &Address, &AddressString))
            goto cleanup;
        rc = RegSetValueExA(hKey, "NameServer", 0, REG_SZ, (const BYTE*)AddressString, strlen(AddressString) + 1);
        if (rc != ERROR_SUCCESS)
            goto cleanup;
    }

    // arty ... Not needed anymore ... We update the address live now
    //MessageBox(NULL, TEXT("You need to reboot before the new parameters take effect."), TEXT("Reboot required"), MB_OK | MB_ICONWARNING);
    ret = TRUE;

cleanup:
    if (hKey != NULL)
        RegCloseKey(hKey);
    return ret;
#if 0
    Changed = FALSE;
    if (DlgData->DhcpEnabled) {
        Changed = ! DlgData->OldDhcpEnabled;
        IpChanged = FALSE;
        DhcpAction = 1;
    } else {
        Changed = DlgData->OldDhcpEnabled ||
                  DlgData->IpAddress != DlgData->OldIpAddress ||
                  DlgData->SubnetMask != DlgData->OldSubnetMask;
        IpChanged = DlgData->OldDhcpEnabled ||
                    DlgData->IpAddress != DlgData->OldIpAddress;
        DhcpAction = 2;
    }

    if (Changed) {
        AdapterName = HeapAlloc(GetProcessHeap(), 0,
                                (strlen(DlgData->AdapterName) + 1) *
                                sizeof(WCHAR));
        if (NULL == AdapterName) {
            ShowError(Dlg, IDS_OUT_OF_MEMORY);
            return FALSE;
        }
        MultiByteToWideChar(CP_THREAD_ACP, 0, DlgData->AdapterName, -1,
                            AdapterName, strlen(DlgData->AdapterName) + 1);
        if (0 == DhcpNotifyConfigChange(NULL, AdapterName, IpChanged,
                                        DlgData->AdapterIndex,
                                        DlgData->IpAddress,
                                        DlgData->SubnetMask, DhcpAction)) {
            HeapFree(GetProcessHeap(), 0, AdapterName);
            ShowError(Dlg, IDS_CANNOT_SAVE_CHANGES);
            return FALSE;
        }
        HeapFree(GetProcessHeap(), 0, AdapterName);
    }

    /* FIXME Save default gateway and DNS entries */
    return TRUE;
#endif
}

static INT_PTR CALLBACK
TCPIPPropertyPageProc(HWND Dlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPPROPSHEETPAGEW Page;
    PTCPIP_PROPERTIES_DATA DlgData;
    LPNMHDR Nmhdr;

    DlgData = (PTCPIP_PROPERTIES_DATA) GetWindowLongPtrW(Dlg, GWL_USERDATA);
    switch(uMsg) {
    case WM_INITDIALOG:
        Page = (LPPROPSHEETPAGEW) lParam;
        DlgData = (PTCPIP_PROPERTIES_DATA) Page->lParam;
        SetWindowLongPtrW(Dlg, GWL_USERDATA, Page->lParam);

        EnableWindow(GetDlgItem(Dlg, IDC_ADVANCED), FALSE);

        EnableDHCP(Dlg, DlgData->OldDhcpEnabled, 0);

        if (! DlgData->OldDhcpEnabled)
        {
            if (INADDR_NONE != DlgData->OldIpAddress) {
                SendDlgItemMessage(Dlg, IDC_IPADDR, IPM_SETADDRESS, 0,
                                   ntohl(DlgData->OldIpAddress));
            }
            if (INADDR_NONE != DlgData->OldSubnetMask) {
                SendDlgItemMessage(Dlg, IDC_SUBNETMASK, IPM_SETADDRESS, 0,
                                   ntohl(DlgData->OldSubnetMask));
            }
            if (INADDR_NONE != DlgData->OldGateway) {
                SendDlgItemMessage(Dlg, IDC_DEFGATEWAY, IPM_SETADDRESS, 0,
                                   ntohl(DlgData->OldGateway));
            }
        }

        if (INADDR_NONE != DlgData->OldDns1) {
            SendDlgItemMessage(Dlg, IDC_DNS1, IPM_SETADDRESS, 0,
                               ntohl(DlgData->OldDns1));
            if (INADDR_NONE != DlgData->OldDns2) {
                SendDlgItemMessage(Dlg, IDC_DNS2, IPM_SETADDRESS, 0,
                                   ntohl(DlgData->OldDns2));
            }
        }

        if (DlgData->OldDhcpEnabled)
        {
            ManualDNS(Dlg, INADDR_NONE != DlgData->OldDns1, 0);
        }

        break;

    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case IDC_FIXEDDNS:
            ManualDNS(Dlg, TRUE, LOWORD(wParam));
            return TRUE;

        case IDC_AUTODNS:
            ManualDNS(Dlg, FALSE, LOWORD(wParam));
            return TRUE;

        case IDC_USEDHCP:
            EnableDHCP(Dlg, TRUE, LOWORD(wParam));
            return TRUE;

        case IDC_NODHCP:
            EnableDHCP(Dlg, FALSE, LOWORD(wParam));
            return TRUE;
        }
        break;

    case WM_NOTIFY:
        Nmhdr = (LPNMHDR) lParam;

        switch(Nmhdr->code) {
        case PSN_KILLACTIVE:
            /* Do validation here, must set FALSE to continue */
            SetWindowLongPtrW(Dlg, DWL_MSGRESULT,
                              (LONG_PTR) ! ValidateAndStore(Dlg, DlgData));
            return TRUE;

        case PSN_APPLY:
            /* Set the IP Address and DNS Information so we won't
             * be doing all this for nothing */
            SetWindowLongPtrW(Dlg, DWL_MSGRESULT,
                              InternTCPIPSettings(Dlg, DlgData) ?
                              PSNRET_NOERROR : PSNRET_INVALID);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

static BOOL
LoadDataFromInfo(PTCPIP_PROPERTIES_DATA DlgData, IP_ADAPTER_INFO *Info)
{
    IP_ADDR_STRING *pString;
    WCHAR RegKey[MAX_PATH];
    HKEY hKey;
    char Dns[MAX_PATH];
    DWORD Size;
    DWORD Type;
    char *NextDnsServer;

    DlgData->AdapterName = Info->AdapterName;
    DlgData->AdapterIndex = Info->Index;

    DlgData->OldDhcpEnabled = Info->DhcpEnabled;

    pString = &Info->IpAddressList;
    while (NULL != pString->Next) {
        pString = pString->Next;
    }
    DlgData->OldIpAddress = inet_addr(pString->IpAddress.String);
    DlgData->OldSubnetMask = inet_addr(pString->IpMask.String);
    pString = &Info->GatewayList;
    while (NULL != pString->Next) {
        pString = pString->Next;
    }
    DlgData->OldGateway = inet_addr(pString->IpAddress.String);

    DlgData->OldDns1 = INADDR_NONE;
    DlgData->OldDns2 = INADDR_NONE;
    swprintf(RegKey,
             L"SYSTEM\\CurrentControlSet\\Services\\TCPIP\\Parameters\\Interfaces\\%S",
             Info->AdapterName);
    if (ERROR_SUCCESS == RegOpenKeyW(HKEY_LOCAL_MACHINE, RegKey, &hKey)) {
        Size = sizeof(Dns);
        RegQueryValueExA(hKey, "NameServer", NULL, &Type, (BYTE *)Dns,
                         &Size);
        RegCloseKey(hKey);

        if ('\0' != Dns[0]) {
            DlgData->OldDns1 = inet_addr(Dns);
            NextDnsServer = strchr(Dns, ',');
            if (NULL != NextDnsServer && '\0' != *NextDnsServer) {
                DlgData->OldDns2 = inet_addr(NextDnsServer);
            }
        }
    }

    return TRUE;
}

void
DisplayTCPIPProperties(HWND hParent, IP_ADAPTER_INFO *pInfo)
{
    PROPSHEETPAGEW psp[1];
    PROPSHEETHEADERW psh;
    INITCOMMONCONTROLSEX cce;
    TCPIP_PROPERTIES_DATA DlgData;
    LPTSTR tpszCaption = NULL;
    INT StrLen;

    HWND hListBox = GetDlgItem(hParent, IDC_COMPONENTSLIST);
    int iListBoxIndex = (int) SendMessage(hListBox, LB_GETCURSEL, 0, 0);

    if(iListBoxIndex != LB_ERR)
    {
        StrLen = SendMessage(hListBox, LB_GETTEXTLEN, iListBoxIndex, 0);

        if (StrLen != LB_ERR)
        {
            TCHAR suffix[] = _T(" Properties");
            INT HeapSize = ((StrLen + 1) + (_tcslen(suffix) + 1)) * sizeof(TCHAR);
            tpszCaption = (LPTSTR)HeapAlloc(GetProcessHeap(), 0, HeapSize);
            SendMessage(hListBox, LB_GETTEXT, iListBoxIndex, (LPARAM)tpszCaption);
            _tcscat(tpszCaption, suffix);
        }
    }

    if (! LoadDataFromInfo(&DlgData, pInfo))
    {
        ShowError(hParent, IDS_CANNOT_LOAD_CONFIG);
        return;
    }

    cce.dwSize = sizeof(INITCOMMONCONTROLSEX);
    cce.dwICC = ICC_INTERNET_CLASSES;
    InitCommonControlsEx(&cce);

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
    psh.hwndParent = hParent;
    psh.hInstance = hApplet;
    psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDI_CPLSYSTEM));
    psh.pszCaption = tpszCaption;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = psp;
    psh.pfnCallback = NULL;

    InitPropSheetPage(&psp[0], IDD_TCPIPPROPERTIES, TCPIPPropertyPageProc, (LPARAM) &DlgData);

    if (PropertySheetW(&psh) == -1)
    {
        ShowError(hParent, IDS_CANNOT_CREATE_PROPSHEET);
    }

    if (tpszCaption != NULL)
        HeapFree(GetProcessHeap(), 0, tpszCaption);

    return;
}
