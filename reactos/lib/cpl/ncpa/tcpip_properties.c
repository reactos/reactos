/*
 *  ReactOS
 *  Copyright (C) 2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * PROJECT:         ReactOS Network Control Panel
 * FILE:            lib/cpl/system/tcpip_properties.c
 * PURPOSE:         ReactOS Network Control Panel
 * PROGRAMMER:      Gero Kuehn (reactos.filter@gkware.com)
 * UPDATE HISTORY:
 *      08-15-2004  Created
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

void InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc);

static void
EnableDHCP( HWND hwndDlg, BOOL Enabled ) {
    CheckDlgButton(hwndDlg,IDC_USEDHCP,Enabled ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwndDlg,IDC_NODHCP,Enabled ? BST_UNCHECKED : BST_CHECKED);
}

static void
ManualDNS( HWND hwndDlg, BOOL Enabled ) {
    CheckDlgButton(hwndDlg,IDC_FIXEDDNS,Enabled ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwndDlg,IDC_AUTODNS,Enabled ? BST_UNCHECKED : BST_CHECKED);
    EnableWindow(GetDlgItem(hwndDlg,IDC_DNS1),Enabled);
    EnableWindow(GetDlgItem(hwndDlg,IDC_DNS2),Enabled);
}

static BOOL
GetAddressFromField( HWND hwndDlg, UINT CtlId,
                     DWORD *dwIPAddr,
                     const char **AddressString ) {
    LRESULT lResult;
    struct in_addr inIPAddr;

    *AddressString = NULL;

    lResult = SendMessage(GetDlgItem(hwndDlg, IDC_IPADDR), IPM_GETADDRESS, 0, 
                          (ULONG_PTR)dwIPAddr);
    if( lResult != 4 ) return FALSE;
    
    inIPAddr.s_addr = *dwIPAddr;
    *AddressString = inet_ntoa(inIPAddr);
    if( !*AddressString ) return FALSE;

    return TRUE;
}

static BOOL
InternTCPIPSettings( HWND hwndDlg ) {
    PROPSHEETPAGE *pPage = (PROPSHEETPAGE *)GetWindowLongPtr(hwndDlg,GWL_USERDATA);
    IP_ADAPTER_INFO *pInfo = NULL;
    TCHAR pszRegKey[MAX_PATH];
    HKEY hKey = NULL;
    DWORD IpAddress, NetMask, Gateway;
    const char *AddressString;
    BOOL RetVal = FALSE;
    BOOL DhcpEnabled = FALSE;
    MIB_IPFORWARDROW RowToAdd = { 0 };

    if(pPage)
        pInfo = (IP_ADAPTER_INFO *)pPage->lParam;
    
    if( !pPage || !pInfo ) goto cleanup;

    _stprintf(pszRegKey,_T("SYSTEM\\CurrentControlSet\\Services\\TCPIP\\Parameters\\Interfaces\\%S"),pInfo->AdapterName);
    if(RegOpenKey(HKEY_LOCAL_MACHINE,pszRegKey,&hKey)!=ERROR_SUCCESS) goto cleanup;

    if( !GetAddressFromField
        ( hwndDlg, IDC_IPADDR, &IpAddress, &AddressString ) ||
        RegSetValueEx
        ( hKey, _T("IPAddress"), 0, REG_SZ, (const BYTE*)AddressString,
          strlen(AddressString) ) != ERROR_SUCCESS )
        goto cleanup;

    if( !GetAddressFromField
        ( hwndDlg, IDC_SUBNETMASK, &NetMask, &AddressString ) ||
        RegSetValueEx
        ( hKey, _T("SubnetMask"), 0, REG_SZ, (const BYTE*)AddressString,
          strlen(AddressString) ) != ERROR_SUCCESS )
        goto cleanup;

    if( DhcpEnabled ) /* FIXME - DhcpEnabled is never initialized at this point! */
        DhcpLeaseIpAddress( pInfo->Index );
    else {
        DhcpReleaseIpAddressLease( pInfo->Index );
        DhcpStaticRefreshParams( pInfo->Index, IpAddress, NetMask );
    }

    if( !GetAddressFromField
        ( hwndDlg, IDC_DEFGATEWAY, &Gateway, &AddressString ) ||
        RegSetValueEx
        ( hKey, _T("DefaultGateway"), 0, REG_SZ, (const BYTE*)AddressString,
          strlen(AddressString) ) != ERROR_SUCCESS )
        goto cleanup;

    /* If not on DHCP then add a default gateway, assuming one was specified */
    if( !DhcpEnabled ) {
        RowToAdd.dwForwardMask = 0;
        RowToAdd.dwForwardMetric1 = 1;
        RowToAdd.dwForwardNextHop = Gateway;
        CreateIpForwardEntry( &RowToAdd );
    }

cleanup:
    if( hKey ) RegCloseKey( hKey );

    return RetVal;
}

static INT_PTR CALLBACK
TCPIPPropertyPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PROPSHEETPAGE *pPage = (PROPSHEETPAGE *)GetWindowLongPtr(hwndDlg,GWL_USERDATA);
    IP_ADAPTER_INFO *pInfo = NULL;
    int StaticDNS = 0;
    char *NextDNSServer;

    if(pPage)
        pInfo = (IP_ADAPTER_INFO *)pPage->lParam;
    switch(uMsg)
    {
    case WM_INITDIALOG:	
    {
        pPage = (PROPSHEETPAGE *)lParam;
        pInfo = (IP_ADAPTER_INFO *)pPage->lParam;
        EnableWindow(GetDlgItem(hwndDlg,IDC_ADVANCED),FALSE);
        SetWindowLongPtr(hwndDlg,GWL_USERDATA,(DWORD_PTR)pPage->lParam);

        EnableDHCP( hwndDlg, pInfo->DhcpEnabled );

        {
            DWORD dwIPAddr;
            IP_ADDR_STRING *pString;
            pString = &pInfo->IpAddressList;
            while(pString->Next)
                pString = pString->Next;
            dwIPAddr = inet_addr(pString->IpAddress.String);
            SendDlgItemMessage(hwndDlg,IDC_IPADDR,IPM_SETADDRESS,0,
                               ntohl(dwIPAddr));
            dwIPAddr = inet_addr(pString->IpMask.String);
            SendDlgItemMessage(hwndDlg,IDC_SUBNETMASK,IPM_SETADDRESS,0,
                               ntohl(dwIPAddr));

            pString = &pInfo->GatewayList;
            while(pString->Next)
                pString = pString->Next;
            dwIPAddr = inet_addr(pString->IpAddress.String);
            SendDlgItemMessage(hwndDlg,IDC_DEFGATEWAY,IPM_SETADDRESS,0,
                               ntohl(dwIPAddr));

        }
        {
            TCHAR pszRegKey[MAX_PATH];
            HKEY hKey;
            _stprintf(pszRegKey,_T("SYSTEM\\CurrentControlSet\\Services\\TCPIP\\Parameters\\Interfaces\\%S"),pInfo->AdapterName);
            if(RegOpenKey(HKEY_LOCAL_MACHINE,pszRegKey,&hKey)==ERROR_SUCCESS)
            {
                char pszDNS[MAX_PATH];
                DWORD dwSize = sizeof(pszDNS);
                DWORD dwType = REG_SZ;
                DWORD dwIPAddr;
                RegQueryValueExA(hKey,"NameServer",NULL,&dwType,(BYTE*)pszDNS,&dwSize);
                RegCloseKey(hKey);

                NextDNSServer = pszDNS;

                while( NextDNSServer && StaticDNS < 2 ) {
                    dwIPAddr = inet_addr(NextDNSServer);
                    if( dwIPAddr != INADDR_NONE ) {
                        SendDlgItemMessage(hwndDlg,IDC_DNS1 + StaticDNS,
                                           IPM_SETADDRESS,0,ntohl(dwIPAddr));
                        StaticDNS++;
                    }
                    NextDNSServer = strchr( pszDNS, ',' );
                    if( NextDNSServer )
                        NextDNSServer++;
                }

                ManualDNS( hwndDlg, StaticDNS );
            }
        }
    }
    break;
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDC_FIXEDDNS:
            ManualDNS( hwndDlg, TRUE );
            break;

        case IDC_AUTODNS:
            ManualDNS( hwndDlg, FALSE );
            break;

        case IDC_USEDHCP:
            EnableDHCP( hwndDlg, TRUE );
            break;

        case IDC_NODHCP:
            EnableDHCP( hwndDlg, FALSE );
            break;

        case IDOK:
            /* Set the IP Address and DNS Information so we won't
             * be doing all this for nothing */
            InternTCPIPSettings( hwndDlg );
            break;
        }
        break;
    }
    return FALSE;
}

void DisplayTCPIPProperties(HWND hParent,IP_ADAPTER_INFO *pInfo)
{
	PROPSHEETPAGE psp[1];
	PROPSHEETHEADER psh;
	INITCOMMONCONTROLSEX cce;

	cce.dwSize = sizeof(INITCOMMONCONTROLSEX);
	cce.dwICC = ICC_INTERNET_CLASSES;
	InitCommonControlsEx(&cce);

	ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
	psh.hwndParent = hParent;
	psh.hInstance = hApplet;
	psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDI_CPLSYSTEM));
	psh.pszCaption = NULL;//Caption;
	psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
	psh.nStartPage = 0;
	psh.ppsp = psp;
	psh.pfnCallback = NULL;
	

	InitPropSheetPage(&psp[0], IDD_TCPIPPROPERTIES, TCPIPPropertyPageProc);
	psp[0].lParam = (LPARAM)pInfo;

	if (PropertySheet(&psh) == -1)
	{
		MessageBox(hParent,_T("Unable to create property sheet"),_T("Error"),MB_ICONSTOP);
	}

	return;
}
