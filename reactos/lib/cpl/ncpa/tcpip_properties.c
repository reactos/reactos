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
/* $Id: tcpip_properties.c,v 1.4 2004/10/31 11:54:58 ekohl Exp $
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

extern void InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc);

INT_PTR CALLBACK
TCPIPPropertyPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PROPSHEETPAGE *pPage = (PROPSHEETPAGE *)GetWindowLongPtr(hwndDlg,GWL_USERDATA);
	IP_ADAPTER_INFO *pInfo = NULL;
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

			if(pInfo->DhcpEnabled) {
				CheckDlgButton(hwndDlg,IDC_USEDHCP,BST_CHECKED);
				CheckDlgButton(hwndDlg,IDC_NODHCP,BST_UNCHECKED);
			} else {
				CheckDlgButton(hwndDlg,IDC_USEDHCP,BST_UNCHECKED);
				CheckDlgButton(hwndDlg,IDC_NODHCP,BST_CHECKED);
			} 
			{
				DWORD dwIPAddr;
				DWORD b[4];
				IP_ADDR_STRING *pString;
				pString = &pInfo->IpAddressList;
				while(pString->Next)
					pString = pString->Next;
				sscanf(pString->IpAddress.String,"%d.%d.%d.%d",&b[0],&b[1],&b[2],&b[3]);
				dwIPAddr = b[0]<<24|b[1]<<16|b[2]<<8|b[3];
				SendDlgItemMessage(hwndDlg,IDC_IPADDR,IPM_SETADDRESS,0,dwIPAddr);
				sscanf(pString->IpMask.String,"%d.%d.%d.%d",&b[0],&b[1],&b[2],&b[3]);
				dwIPAddr = b[0]<<24|b[1]<<16|b[2]<<8|b[3];
				SendDlgItemMessage(hwndDlg,IDC_SUBNETMASK,IPM_SETADDRESS,0,dwIPAddr);

				pString = &pInfo->GatewayList;
				while(pString->Next)
					pString = pString->Next;
				sscanf(pString->IpAddress.String,"%d.%d.%d.%d",&b[0],&b[1],&b[2],&b[3]);
				dwIPAddr = b[0]<<24|b[1]<<16|b[2]<<8|b[3];
				SendDlgItemMessage(hwndDlg,IDC_DEFGATEWAY,IPM_SETADDRESS,0,dwIPAddr);

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
					DWORD b[2][4];
					DWORD dwIPAddr;
					RegQueryValueExA(hKey,"NameServer",NULL,&dwType,(BYTE*)pszDNS,&dwSize);
					RegCloseKey(hKey);
					
					sscanf(pszDNS,"%d.%d.%d.%d,%d.%d.%d.%d",&b[0][0],&b[0][1],&b[0][2],&b[0][3],&b[1][0],&b[1][1],&b[1][2],&b[1][3]);
					dwIPAddr = b[0][0]<<24|b[0][1]<<16|b[0][2]<<8|b[0][3];
					SendDlgItemMessage(hwndDlg,IDC_DNS1,IPM_SETADDRESS,0,dwIPAddr);
					dwIPAddr = b[1][0]<<24|b[1][1]<<16|b[1][2]<<8|b[1][3];
					SendDlgItemMessage(hwndDlg,IDC_DNS2,IPM_SETADDRESS,0,dwIPAddr);
					CheckDlgButton(hwndDlg,IDC_FIXEDDNS,TRUE);
					EnableWindow(GetDlgItem(hwndDlg,IDC_DNS1),FALSE);
					EnableWindow(GetDlgItem(hwndDlg,IDC_DNS2),FALSE);
					EnableWindow(GetDlgItem(hwndDlg,IDC_AUTODNS),FALSE);
					EnableWindow(GetDlgItem(hwndDlg,IDC_FIXEDDNS),FALSE);
				}
			}
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
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
