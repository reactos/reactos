/*
 * Copyright 2004 Gero Kuehn
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * PROJECT:         ReactOS Network Control Panel
 * FILE:            lib/cpl/system/ncpa.c
 * PURPOSE:         ReactOS Network Control Panel
 * PROGRAMMER:      Gero Kuehn (reactos.filter@gkware.com)
 * UPDATE HISTORY:
 *      07-18-2004  Created
 */

/*
 * Read this first !
 *
 * This file contains a first attempt for reactos network configuration
 *
 *  - It is not complete.
 *  - It does not work the way it works on Windows.
 *
 * A lot of code that can be found here now, will probably be relocated into the OS core or some
 * protocol Co-Installers or Notify Objects later when all the required COM
 * and "netcfgx.dll" infrastructure (esp. headers and Interfaces) get implemented step by step.
 *
 * This code is only a first approach to provide a usable network configuration dialogs for
 * the new network support in Reactos.
 *
 * If you intend to extend this code by more, please contact me to avoid duplicate work.
 * There are already resources and code for TCP/IP configuration that are not 
 * mature enough for committing them to CVS yet.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>
#include <windows.h>
#include <iphlpapi.h>
#include <commctrl.h>
#include <cpl.h>

#include <debug.h>

#include "resource.h"
#include "ncpa.h"

#define NCF_VIRTUAL                     0x1
#define NCF_SOFTWARE_ENUMERATED         0x2
#define NCF_PHYSICAL                    0x4
#define NCF_HIDDEN                      0x8
#define NCF_NO_SERVICE                  0x10
#define NCF_NOT_USER_REMOVABLE          0x20
#define NCF_MULTIPORT_INSTANCED_ADAPTER 0x40
#define NCF_HAS_UI                      0x80
#define NCF_FILTER                      0x400
#define NCF_NDIS_PROTOCOL               0x4000

typedef void (ENUMREGKEYCALLBACK)(void *pCookie,HKEY hBaseKey,TCHAR *pszSubKey);

static LONG CALLBACK DisplayApplet(VOID);
static INT_PTR CALLBACK NetworkPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void DisplayTCPIPProperties(HWND hParent,IP_ADAPTER_INFO *pInfo);

HINSTANCE hApplet = 0;

/* Applets */
static APPLET Applets[] =
{
	{IDI_CPLSYSTEM, IDS_CPLSYSTEMNAME, IDS_CPLSYSTEMDESCRIPTION, DisplayApplet}
};


/* useful utilities */
static VOID
EnumRegKeys(ENUMREGKEYCALLBACK *pCallback,PVOID pCookie,HKEY hBaseKey,TCHAR *tpszRegPath)
{
	HKEY hKey;
	INT i;
	LONG ret;
	TCHAR tpszName[MAX_PATH];
	DWORD dwNameLen = sizeof(tpszName);

	if(RegOpenKeyEx(hBaseKey,tpszRegPath,0,KEY_ENUMERATE_SUB_KEYS,&hKey)!=ERROR_SUCCESS)
	{
		DPRINT("EnumRegKeys failed (key not found): %S\n", tpszRegPath);
		return;
	}

	for(i=0;;i++)
	{
		TCHAR pszNewPath[MAX_PATH];
		ret = RegEnumKeyEx(hKey,i,tpszName,&dwNameLen,NULL,NULL,NULL,NULL);
		if(ret != ERROR_SUCCESS)
		{
			DPRINT("EnumRegKeys: RegEnumKeyEx failed for %S (rc 0x%lx)\n", tpszName, ret);
			break;
		}

		_stprintf(pszNewPath,_T("%s\\%s"),tpszRegPath,tpszName);
		DPRINT("EnumRegKeys: Calling user supplied enum function\n");
		pCallback(pCookie,hBaseKey,pszNewPath);

		dwNameLen = sizeof(tpszName);
	}

	RegCloseKey(hKey);
}

void
InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc,LPARAM lParam)
{
	ZeroMemory(psp, sizeof(PROPSHEETPAGE));
	psp->dwSize = sizeof(PROPSHEETPAGE);
	psp->dwFlags = PSP_DEFAULT;
	psp->hInstance = hApplet;
	psp->pszTemplate = MAKEINTRESOURCE(idDlg);
	psp->pfnDlgProc = DlgProc;
	psp->lParam = lParam;
}



static BOOL
FindNICClassKeyForCfgInstance(TCHAR *tpszCfgInst,TCHAR *tpszSubKeyOut)
{
	int i;
	TCHAR tpszSubKey[MAX_PATH];
	TCHAR tpszCfgInst2[MAX_PATH];
	HKEY hKey;
	DWORD dwType,dwSize;

	for (i = 0; i < 100; i++)
	{
		_stprintf(tpszSubKey,_T("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\%04d"),i);
		if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,tpszSubKey,0,KEY_QUERY_VALUE,&hKey)!=ERROR_SUCCESS)
			continue;
		dwType = REG_SZ;
		dwSize = sizeof(tpszCfgInst2);
		if(RegQueryValueEx(hKey,_T("NetCfgInstanceId"),NULL,&dwType,(BYTE*)tpszCfgInst2,&dwSize)!=ERROR_SUCCESS) {
			RegCloseKey(hKey);
			continue;
		}
		RegCloseKey(hKey);
		if(_tcscmp(tpszCfgInst,tpszCfgInst2)==0) {
			_tcscpy(tpszSubKeyOut,tpszSubKey);
			return TRUE;
		}
	}
	return FALSE;
}


static void
NICPropertyProtocolCallback(void *pCookie,HKEY hBaseKey,TCHAR *tpszSubKey)
{
	HWND hwndDlg;
	DWORD dwCharacteristics;
	HKEY hKey,hNDIKey;
	DWORD dwType,dwSize;
	TCHAR tpszDescription[MAX_PATH];
	TCHAR tpszNotifyObjectCLSID[MAX_PATH];
	TCHAR *tpszSubKeyCopy;
	int nIndex;

	UNREFERENCED_PARAMETER(hBaseKey);

//	CLSID CLSID_NotifObj;
//	IUnknown *pUnk = NULL;
//	INetCfgComponentControl *pNetCfg;
//	INetCfgComponentPropertyUi *pNetCfgPropUI;
	hwndDlg = (HWND)pCookie;

	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,tpszSubKey,0,KEY_QUERY_VALUE,&hKey)!=ERROR_SUCCESS)
		return;

	dwType = REG_DWORD;
	dwSize = sizeof(dwCharacteristics);
	if(RegQueryValueEx(hKey,_T("Characteristics"),NULL,&dwType,(BYTE*)&dwCharacteristics,&dwSize)!= ERROR_SUCCESS)
		return;

	if(dwCharacteristics & NCF_HIDDEN)
	{
		RegCloseKey(hKey);
		return;
	}

	dwType = REG_SZ;
	dwSize = sizeof(tpszDescription);
	if(RegQueryValueEx(hKey,_T("Description"),NULL,&dwType,(BYTE*)tpszDescription,&dwSize)!= ERROR_SUCCESS)
		return;

	RegOpenKeyEx(hKey,_T("Ndi"),0,KEY_QUERY_VALUE,&hNDIKey);
	dwType = REG_SZ;
	dwSize = sizeof(tpszNotifyObjectCLSID);
	if(RegQueryValueEx(hNDIKey,_T("ClsId"),NULL,&dwType,(BYTE*)tpszNotifyObjectCLSID,&dwSize)!= ERROR_SUCCESS)
		;//return;

	RegCloseKey(hNDIKey);

	//
	// This code works on Windows... but not on Reactos
	//

//	CLSIDFromString(tpszNotifyObjectCLSID,&CLSID_NotifObj);
//	CoCreateInstance(&CLSID_NotifObj,NULL,CLSCTX_INPROC_SERVER,&IID_IUnknown,(void**)&pUnk);
//	pUnk->lpVtbl->QueryInterface(pUnk,&IID_INetCfgComponentControl,(void**)&pNetCfg);
//	pUnk->lpVtbl->QueryInterface(pUnk,&IID_INetCfgComponentPropertyUi,(void**)&pNetCfgPropUI);
	{
		/*
	HRESULT hr;
	hr = pNetCfg->lpVtbl->Initialize(pNetCfg,&NetCfgComponent,&NetCfg,FALSE);
	hr = pNetCfgPropUI->lpVtbl->QueryPropertyUi(pNetCfgPropUI,(INetCfg*)&NetCfg);
	hr = pNetCfgPropUI->lpVtbl->SetContext(pNetCfgPropUI,(INetCfg*)&NetCfgComponent);
	DWORD dwNumPages = 10;
	HPROPSHEETPAGE  *bOut = NULL;
	UINT nPages;
	hr = pNetCfgPropUI->MergePropPages(&dwNumPages,(BYTE**)&bOut,&nPages,GetDesktopWindow(),NULL);
	*/
	}

	RegCloseKey(hKey);
	nIndex = (int) SendDlgItemMessage(hwndDlg,IDC_COMPONENTSLIST,LB_ADDSTRING,0,(LPARAM)tpszDescription);
	tpszSubKeyCopy = _tcsdup(tpszSubKey);
	SendDlgItemMessage(hwndDlg,IDC_COMPONENTSLIST,LB_SETITEMDATA,nIndex,(LPARAM)tpszSubKeyCopy);
}



static INT_PTR CALLBACK
NICPropertyPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PROPSHEETPAGE *pPage = (PROPSHEETPAGE *)GetWindowLongPtr(hwndDlg,GWL_USERDATA);
	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			TCHAR *tpszCfgInstanceID;
			DWORD dwType,dwSize;
			TCHAR tpszSubKey[MAX_PATH];
			TCHAR tpszDisplayName[MAX_PATH];
			HKEY hKey;
			pPage = (PROPSHEETPAGE *)lParam;
			tpszCfgInstanceID = (TCHAR*)pPage->lParam;
			if(!FindNICClassKeyForCfgInstance(tpszCfgInstanceID,tpszSubKey))
			{
				MessageBox(hwndDlg,_T("NIC Entry not found"),_T("Registry error"),MB_ICONSTOP);
				MessageBox(hwndDlg,tpszCfgInstanceID,tpszSubKey,MB_ICONSTOP);
			}

			if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,tpszSubKey,0,KEY_QUERY_VALUE,&hKey)!=ERROR_SUCCESS)
				return 0;
			dwType = REG_SZ;
			dwSize = sizeof(tpszDisplayName);
			if(RegQueryValueEx(hKey,_T("DriverDesc"),NULL,&dwType,(BYTE*)tpszDisplayName,&dwSize)!= ERROR_SUCCESS)
				return 0;
			RegCloseKey(hKey);

			SetDlgItemText(hwndDlg,IDC_NETCARDNAME,tpszDisplayName);
			EnableWindow(GetDlgItem(hwndDlg,IDC_CONFIGURE),FALSE);

			SetWindowLongPtr(hwndDlg,GWL_USERDATA,(DWORD_PTR)lParam);
			//SetDlgItemTextA(hwndDlg,IDC_NETCARDNAME,Info[pPage->lParam].Description);
			EnumRegKeys(NICPropertyProtocolCallback,hwndDlg,HKEY_LOCAL_MACHINE,_T("System\\CurrentControlSet\\Control\\Network\\{4D36E975-E325-11CE-BFC1-08002BE10318}"));

			SendDlgItemMessage(hwndDlg, IDC_COMPONENTSLIST, LB_SETCURSEL, 0, 0);
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_COMPONENTSLIST:
			if(HIWORD(wParam)==LBN_SELCHANGE)
			{
				TCHAR *tpszSubKey;
				TCHAR tpszHelpKey[MAX_PATH];
				TCHAR tpszHelpText[MAX_PATH];
				HKEY hNDIKey;
				DWORD dwType,dwSize;
				HWND hListBox = GetDlgItem(hwndDlg,IDC_COMPONENTSLIST);
				tpszSubKey = (TCHAR*)SendMessage(hListBox,LB_GETITEMDATA,SendMessage(hListBox,LB_GETCURSEL,0,0),0);
				if(!tpszSubKey)
					break;
				_stprintf(tpszHelpKey,_T("%s\\Ndi"),tpszSubKey);

				RegOpenKeyEx(HKEY_LOCAL_MACHINE,tpszHelpKey,0,KEY_QUERY_VALUE,&hNDIKey);
				dwType = REG_SZ;
				dwSize = sizeof(tpszHelpText);
				if(RegQueryValueEx(hNDIKey,_T("HelpText"),NULL,&dwType,(BYTE*)tpszHelpText,&dwSize)!= ERROR_SUCCESS)
					;//return;
				RegCloseKey(hNDIKey);

				SetDlgItemText(hwndDlg,IDC_DESCRIPTION,tpszHelpText);
				
			}
			if(HIWORD(wParam)!=LBN_DBLCLK)
				break;
			// drop though
		case IDC_PROPERTIES:
			{
				TCHAR *tpszSubKey = NULL;
				TCHAR tpszNDIKey[MAX_PATH];
				TCHAR tpszClsIDText[MAX_PATH];
				TCHAR *tpszTCPIPClsID = _T("{A907657F-6FDF-11D0-8EFB-00C04FD912B2}");
				HKEY hNDIKey;
				DWORD dwType,dwSize;
				HWND hListBox = GetDlgItem(hwndDlg,IDC_COMPONENTSLIST);
				int iListBoxIndex = (int) SendMessage(hListBox,LB_GETCURSEL,0,0);
				if(iListBoxIndex != LB_ERR) 
					tpszSubKey = (TCHAR*)SendMessage(hListBox,LB_GETITEMDATA,iListBoxIndex,0);
				if(!tpszSubKey)
					break;
				_stprintf(tpszNDIKey,_T("%s\\Ndi"),tpszSubKey);

				RegOpenKeyEx(HKEY_LOCAL_MACHINE,tpszNDIKey,0,KEY_QUERY_VALUE,&hNDIKey);
				dwSize = sizeof(tpszClsIDText);
				if(RegQueryValueEx(hNDIKey,_T("ClsId"),NULL,&dwType,(BYTE*)tpszClsIDText,&dwSize)!= ERROR_SUCCESS || dwType != REG_SZ)
					;//return;
				RegCloseKey(hNDIKey);

				if(_tcscmp(tpszTCPIPClsID,tpszClsIDText)==0)
				{
					IP_ADAPTER_INFO Adapters[64];
					IP_ADAPTER_INFO *pAdapter;
					TCHAR *tpszCfgInstanceID;
					DWORD dwSize = sizeof(Adapters);
					memset(&Adapters,0x00,sizeof(Adapters));
					if(GetAdaptersInfo(Adapters,&dwSize)!=ERROR_SUCCESS)
						break;;
					pAdapter = Adapters;
					tpszCfgInstanceID = (TCHAR*)pPage->lParam;
					while(pAdapter)
					{
						TCHAR tpszAdapterName[MAX_PATH];
						swprintf(tpszAdapterName,L"%S",pAdapter->AdapterName);
						DPRINT("IPHLPAPI returned: %S\n", tpszAdapterName);
						if(_tcscmp(tpszAdapterName,tpszCfgInstanceID)==0)
						{
							DisplayTCPIPProperties(hwndDlg,pAdapter);
							break;
						} else
						{
							DPRINT("... which is not the TCPIP property sheet\n");
						}
						pAdapter = pAdapter->Next;
						if(!pAdapter)
						{
							MessageBox(NULL,_T("If you see this, then the IPHLPAPI.DLL probably needs more work because GetAdaptersInfo did not return the expected data."),_T("Error"),MB_ICONSTOP);
						}
					}

				} else
				{
					MessageBox(NULL,_T("This control panel is incomplete.\r\nUsually, the \"Notify Object\" for this Network component should be invoked here. Reactos lacks the infrastructure to do this right now.\r\n- C++\r\n- DDK Headers for notify objects\r\n- clean header structure, that allow Windows-Compatible COM C++ Code"),_T("Error"),MB_ICONSTOP);
				}
				
			}
			break;
		}
		break;
	}
	return FALSE;
}


static void
DisplayNICProperties(HWND hParent,TCHAR *tpszCfgInstanceID)
{
	PROPSHEETPAGE psp[1];
	PROPSHEETHEADER psh;
	TCHAR tpszSubKey[MAX_PATH];
	HKEY hKey;
	DWORD dwType = REG_SZ;
	TCHAR tpszName[MAX_PATH];
	DWORD dwSize = sizeof(tpszName);

	// Get the "Name" for this Connection
	_stprintf(tpszSubKey,_T("System\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\%s\\Connection"),tpszCfgInstanceID);
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,tpszSubKey,0,KEY_QUERY_VALUE,&hKey)!=ERROR_SUCCESS)
		return;
	if(RegQueryValueEx(hKey,_T("Name"),NULL,&dwType,(BYTE*)tpszName,&dwSize)!=ERROR_SUCCESS)
		_stprintf(tpszName,_T("[ERROR]"));
	else
		_tcscat(tpszName,_T(" Properties"));
	RegCloseKey(hKey);
	
	
	ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
	psh.hwndParent = hParent;
	psh.hInstance = hApplet;
	psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDI_CPLSYSTEM));
	psh.pszCaption = tpszName;
	psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
	psh.nStartPage = 0;
	psh.ppsp = psp;
	psh.pfnCallback = NULL;
	

	InitPropSheetPage(&psp[0], IDD_NETPROPERTIES, NICPropertyPageProc,(LPARAM)tpszCfgInstanceID);
	PropertySheet(&psh);
	return;
}

static INT_PTR CALLBACK
NICStatusPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			PROPSHEETPAGE *psp= (PROPSHEETPAGE *)lParam;
			EnableWindow(GetDlgItem(hwndDlg,IDC_ENDISABLE),FALSE);
			SetWindowLongPtr(hwndDlg,DWL_USER,(DWORD_PTR)psp->lParam);
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_PROPERTIES:
			{
				TCHAR *tpszCfgInstance;
				tpszCfgInstance = (TCHAR*)GetWindowLong(hwndDlg,DWL_USER);
				DisplayNICProperties(hwndDlg,tpszCfgInstance);
			}
			break;
		}
		break;
	}
	return FALSE;
}

static INT_PTR CALLBACK
NICSupportPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	
	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			TCHAR Buffer[64];

			PIP_ADAPTER_INFO pAdapterInfo = NULL;			
			ULONG    adaptOutBufLen;
			
			DWORD ErrRet = 0;
		
    		pAdapterInfo = (IP_ADAPTER_INFO *) malloc( sizeof(IP_ADAPTER_INFO) );
    		adaptOutBufLen = sizeof(IP_ADAPTER_INFO);		
    		
		    if (GetAdaptersInfo( pAdapterInfo, &adaptOutBufLen) == ERROR_BUFFER_OVERFLOW) 
			{
		       free(pAdapterInfo);
		       pAdapterInfo = (IP_ADAPTER_INFO *) malloc (adaptOutBufLen);
		    }
		
		    if ((ErrRet = GetAdaptersInfo(pAdapterInfo, &adaptOutBufLen)) != NO_ERROR)
			{
				MessageBox(hwndDlg, _T("error adapterinfo") ,_T("ncpa.cpl"),MB_ICONSTOP);

				if (pAdapterInfo) free(pAdapterInfo);
				return FALSE;
			}    		
			
			if (pAdapterInfo)
			{
				/*FIXME: select the correct adapter info!!*/
				_stprintf(Buffer, _T("%S"), pAdapterInfo->IpAddressList.IpAddress.String);
				SendDlgItemMessage(hwndDlg, IDC_DETAILSIP, WM_SETTEXT, 0, (LPARAM)Buffer);
				_stprintf(Buffer, _T("%S"), pAdapterInfo->IpAddressList.IpMask.String);
				SendDlgItemMessage(hwndDlg, IDC_DETAILSSUBNET, WM_SETTEXT, 0, (LPARAM)Buffer);
				_stprintf(Buffer, _T("%S"), pAdapterInfo->GatewayList.IpAddress.String);
				SendDlgItemMessage(hwndDlg, IDC_DETAILSGATEWAY, WM_SETTEXT, 0, (LPARAM)Buffer);
				
				free(pAdapterInfo);
			}
			

			
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_PROPERTIES:
			{
			}
			break;
		case IDC_DETAILS:
			{
				MessageBox(hwndDlg,_T("not implemented: show detail window"),_T("ncpa.cpl"),MB_ICONSTOP);
			}
			break;
			
		}
		break;
	}
	return FALSE;
}


static VOID
DisplayNICStatus(HWND hParent,TCHAR *tpszCfgInstanceID)
{
	PROPSHEETPAGE psp[2];
	PROPSHEETHEADER psh;
	TCHAR tpszSubKey[MAX_PATH];
	HKEY hKey;
	DWORD dwType = REG_SZ;
	TCHAR tpszName[MAX_PATH];
	DWORD dwSize = sizeof(tpszName);

	// Get the "Name" for this Connection
	_stprintf(tpszSubKey,_T("System\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\%s\\Connection"),tpszCfgInstanceID);
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,tpszSubKey,0,KEY_QUERY_VALUE,&hKey)!=ERROR_SUCCESS)
		return;

	if (RegQueryValueEx(hKey,_T("Name"),NULL,&dwType,(BYTE*)tpszName,&dwSize)!=ERROR_SUCCESS)
		_stprintf(tpszName,_T("[ERROR]"));
		//_stprintf(tpszName,_T("[ERROR]") _T(__FILE__) _T(" %d"),__LINE__ );
	else
		_tcscat(tpszName,_T(" Status"));
	RegCloseKey(hKey);

	ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
	psh.hwndParent = hParent;
	psh.hInstance = hApplet;
	// FIX THESE REACTOS HEADERS !!!!!!!!!
	psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDI_CPLSYSTEM));
	psh.pszCaption = tpszName;//Caption;
	psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
	psh.nStartPage = 0;
	psh.ppsp = psp;
	psh.pfnCallback = NULL;
	
	InitPropSheetPage(&psp[0], IDD_CARDPROPERTIES, NICStatusPageProc, (LPARAM)tpszCfgInstanceID);
	InitPropSheetPage(&psp[1], IDD_CARDSUPPORT, NICSupportPageProc, (LPARAM)tpszCfgInstanceID);
	 
	PropertySheet(&psh);
	return;
}

//
// IPHLPAPI does not provide a list of all adapters
//
#if 0
static VOID
EnumAdapters(HWND hwndDlg)
{
	TCHAR pszText[MAX_ADAPTER_NAME_LENGTH + 4];
	IP_ADAPTER_INFO *pInfo;
	ULONG size;
	INT nIndex;

	size=sizeof(Info);
	if(GetAdaptersInfo(Info,&size)!=ERROR_SUCCESS)
	{
		MessageBox(hwndDlg,L"IPHLPAPI.DLL failed to provide Adapter information",L"Error",MB_ICONSTOP);
		return;
	}

	pInfo = &Info[0];
	while(pInfo)
	{
		swprintf(pszText,L"%S",Info[0].Description);
		nIndex = SendDlgItemMessage(hwndDlg,IDC_NETCARDLIST,LB_ADDSTRING,0,(LPARAM)pszText);
		SendDlgItemMessage(hwndDlg,IDC_NETCARDLIST,LB_SETITEMDATA,nIndex,(LPARAM)pInfo);
		pInfo = pInfo->Next;
	}
}
#endif



static void
NetAdapterCallback(void *pCookie,HKEY hBaseKey,TCHAR *tpszSubKey)
{
	TCHAR tpszDisplayName[MAX_PATH];
	//TCHAR tpszDeviceID[MAX_PATH];
	TCHAR tpszCfgInstanceID[MAX_PATH];
	TCHAR *ptpszCfgInstanceID;
	HKEY hKey;
	DWORD dwType = REG_SZ;
	DWORD dwSize = sizeof(tpszDisplayName);
	int nIndex;
	HWND hwndDlg = (HWND)pCookie;
	DWORD dwCharacteristics;

	DPRINT("NetAdapterCallback: %S\n", tpszSubKey);

	if(RegOpenKeyEx(hBaseKey,tpszSubKey,0,KEY_QUERY_VALUE,&hKey)!=ERROR_SUCCESS)
		return;

	DPRINT("NetAdapterCallback: Reading Characteristics\n");
	dwType = REG_DWORD;
	dwSize = sizeof(dwCharacteristics);
	if(RegQueryValueEx(hKey,_T("Characteristics"),NULL,&dwType,(BYTE*)&dwCharacteristics,&dwSize)!=ERROR_SUCCESS)
		dwCharacteristics = 0;

	if (dwCharacteristics & NCF_HIDDEN)
		return;
		
	if (!(dwCharacteristics & NCF_VIRTUAL) && !(dwCharacteristics & NCF_PHYSICAL))
		return;
		
	DPRINT("NetAdapterCallback: Reading DriverDesc\n");
	dwType = REG_SZ;
	dwSize = sizeof(tpszDisplayName);
	if (RegQueryValueEx(hKey,_T("DriverDesc"),NULL,&dwType,(BYTE*)tpszDisplayName,&dwSize)!= ERROR_SUCCESS)
		_tcscpy(tpszDisplayName,_T("Unnamed Adapter"));

	// get the link to the Enum Subkey (currently unused)
	//dwType = REG_SZ;
	//dwSize = sizeof(tpszDeviceID);
	//if(RegQueryValueEx(hKey,_T("MatchingDeviceId"),NULL,&dwType,(BYTE*)tpszDeviceID,&dwSize) != ERROR_SUCCESS) {
	//	MessageBox(hwndDlg,_T("Missing MatchingDeviceId Entry"),_T("Registry Problem"),MB_ICONSTOP);
	//	return;
	//}

	// get the card configuration GUID
	dwType = REG_SZ;
	dwSize = sizeof(tpszCfgInstanceID);
	if(RegQueryValueEx(hKey,_T("NetCfgInstanceId"),NULL,&dwType,(BYTE*)tpszCfgInstanceID,&dwSize) != ERROR_SUCCESS) {
		MessageBox(hwndDlg,_T("Missing NetCfgInstanceId Entry"),_T("Registry Problem"),MB_ICONSTOP);
		return;
	}

	ptpszCfgInstanceID = _tcsdup(tpszCfgInstanceID);
	//
	// **TODO** **FIXME** TBD
	// At this point, we should verify, if the device listed here
	// really represents a device that is currently connected to the system
	//
	// How is this done properly ?
	

	nIndex = (int) SendDlgItemMessage(hwndDlg,IDC_NETCARDLIST,LB_ADDSTRING,0,(LPARAM)tpszDisplayName);
	SendDlgItemMessage(hwndDlg,IDC_NETCARDLIST,LB_SETITEMDATA,nIndex,(LPARAM)ptpszCfgInstanceID);
	RegCloseKey(hKey);
}


static void
EnumAdapters(HWND hwndDlg)
{
	LPTSTR lpRegPath = _T("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}");

	EnumRegKeys(NetAdapterCallback,hwndDlg,HKEY_LOCAL_MACHINE,lpRegPath);
	return;
}


static INT_PTR CALLBACK
NetworkPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	NMHDR* pnmh;
	int nIndex;
	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			EnableWindow(GetDlgItem(hwndDlg,IDC_ADD),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_REMOVE),FALSE);

			EnumAdapters(hwndDlg);
			SendDlgItemMessage(hwndDlg,IDC_NETCARDLIST,LB_SETCURSEL,0,0);
		}
		break;

	case WM_NOTIFY:
		pnmh=(NMHDR*)lParam;
		switch(pnmh->code)
		{
		case PSN_APPLY:
		case PSN_RESET:
			while(SendDlgItemMessage(hwndDlg,IDC_NETCARDLIST,LB_GETCOUNT,0,0)>0)
			{
				TCHAR *tpszString;

				tpszString = (TCHAR*)SendDlgItemMessage(hwndDlg,IDC_NETCARDLIST,LB_GETITEMDATA,0,0);
				if(tpszString)
					free(tpszString);
				SendDlgItemMessage(hwndDlg,IDC_NETCARDLIST,LB_DELETESTRING,0,0);
			}
			break;
		}
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_NETCARDLIST:
			if(HIWORD(wParam)==LBN_DBLCLK) {
				nIndex = (int) SendDlgItemMessage(hwndDlg,IDC_NETCARDLIST,LB_GETCURSEL,0,0);
				if(nIndex!=-1)
					DisplayNICStatus(hwndDlg,(TCHAR*)SendDlgItemMessage(hwndDlg,IDC_NETCARDLIST,LB_GETITEMDATA,nIndex,0));
			}
			break;

		case IDC_PROPERTIES:
			nIndex = (int) SendDlgItemMessage(hwndDlg,IDC_NETCARDLIST,LB_GETCURSEL,0,0);
			if(nIndex!=-1)
				DisplayNICStatus(hwndDlg,(TCHAR*)SendDlgItemMessage(hwndDlg,IDC_NETCARDLIST,LB_GETITEMDATA,nIndex,0));
			break;
		}
		break;
	}

	return FALSE;
}



/* First Applet */
static LONG CALLBACK
DisplayApplet(VOID)
{
	PROPSHEETPAGE psp[1];
	PROPSHEETHEADER psh = {0};
	TCHAR Caption[1024];

	LoadString(hApplet, IDS_CPLSYSTEMNAME, Caption, sizeof(Caption) / sizeof(TCHAR));

	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags =  PSH_PROPSHEETPAGE;
	psh.hwndParent = NULL;
	psh.hInstance = hApplet;
	psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDI_CPLSYSTEM));
	psh.pszCaption = Caption;
	psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
	psh.nStartPage = 0;
	psh.ppsp = psp;
	psh.pfnCallback = NULL;

	InitPropSheetPage(&psp[0], IDD_PROPPAGENETWORK, NetworkPageProc,0);

	return (LONG)(PropertySheet(&psh) != -1);
}

/* Control Panel Callback */
LONG CALLBACK
CPlApplet(HWND hwndCPl, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
	UNREFERENCED_PARAMETER(hwndCPl);
	switch (uMsg)
	{
	case CPL_INIT:
		{
			return TRUE;
		}

	case CPL_GETCOUNT:
		{
			return sizeof(Applets)/sizeof(APPLET);
		}

	case CPL_INQUIRE:
		{
			CPLINFO *CPlInfo = (CPLINFO*)lParam2;
			CPlInfo->lData = 0;
			CPlInfo->idIcon = Applets[(int)lParam1].idIcon;
			CPlInfo->idName = Applets[(int)lParam1].idName;
			CPlInfo->idInfo = Applets[(int)lParam1].idDescription;
			break;
		}

	case CPL_DBLCLK:
		{
			Applets[(int)lParam1].AppletProc();
			break;
		}
	}

	return FALSE;
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(lpvReserved);

	switch(dwReason)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
		hApplet = hinstDLL;
		break;
	}

	return TRUE;
}

