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
/* $Id: ncpa.c,v 1.1 2004/07/18 22:37:07 kuehng Exp $
 *
 * PROJECT:         ReactOS Network Control Panel
 * FILE:            lib/cpl/system/ncpa.c
 * PURPOSE:         ReactOS Network Control Panel
 * PROGRAMMER:      Gero Kuehn (reactos.filter@gkware.com)
 * UPDATE HISTORY:
 *      07-18-2004  Created
 */

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//							Read this first !
//
// This file contains a first attempt for reactos network configuration
// This is:
// - not complete
// - not the way it works on Windows
// 
// A lot of code that can be found here now, will probably be relocated into the OS core or some
// protocol Co-Installers or Notify Objects later when all the required COM
// and "netcfgx.dll" infrastructure (esp. headers and Interfaces) get implemented step by step.
//
// This code is only a first approach to provide a usable network configuration dialogs for
// the new network support in Reactos.
//
// If you intend to extend this code by more, please contact me to avoid duplicate work.
// There are already resources and code for TCP/IP configuration that are not 
// mature enough for committing them to CVS yet.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>
#include <windows.h>

#ifdef __REACTOS__
//#include <Netcfgn.h>
#else
#include <commctrl.h>
#include <cpl.h>
#endif

#include "resource.h"
#include "ncpa.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// useful utilities
#define NCF_HIDDEN 0x08
#define NCF_HAS_UI 0x80

typedef void (ENUMREGKEYCALLBACK)(void *pCookie,HKEY hBaseKey,TCHAR *pszSubKey);
void EnumRegKeys(ENUMREGKEYCALLBACK *pCallback,void *pCookie,HKEY hBaseKey,TCHAR *tpszRegPath)
{
	HKEY hKey;
	int i;
	LONG ret;
	TCHAR tpszName[MAX_PATH];
	DWORD dwNameLen = sizeof(tpszName);
	if(RegOpenKeyEx(hBaseKey,tpszRegPath,0,KEY_ALL_ACCESS,&hKey)!=ERROR_SUCCESS)
	{
		OutputDebugString(_T("EnumRegKeys failed (key not found)\r\n"));
		OutputDebugString(tpszRegPath);
		OutputDebugString(_T("\r\n"));
		return;
	}
	
	for(i=0;;i++)
	{
		TCHAR pszNewPath[MAX_PATH];
		ret = RegEnumKeyEx(hKey,i,tpszName,&dwNameLen,NULL,NULL,NULL,NULL);
		if(ret != ERROR_SUCCESS)
		{
			OutputDebugString(_T("EnumRegKeys: RegEnumKeyEx failed\r\n"));
			break;
		}

		_stprintf(pszNewPath,_T("%s\\%s"),tpszRegPath,tpszName);
		OutputDebugString(_T("EnumRegKeys: Calling user supplied enum function\r\n"));
		pCallback(pCookie,hBaseKey,pszNewPath);

		dwNameLen = sizeof(tpszName);
	}
	RegCloseKey(hKey);
}

void InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc,LPARAM lParam)
{
	ZeroMemory(psp, sizeof(PROPSHEETPAGE));
	psp->dwSize = sizeof(PROPSHEETPAGE);
	psp->dwFlags = PSP_DEFAULT;
	psp->hInstance = hApplet;
#ifdef _MSC_VER
	psp->pszTemplate = MAKEINTRESOURCE(idDlg);
#else
	psp->u1.pszTemplate = MAKEINTRESOURCE(idDlg);
#endif
	psp->pfnDlgProc = DlgProc;
	psp->lParam = lParam;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////



LONG CALLBACK DisplayApplet(VOID);
BOOL CALLBACK NetworkPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
//void DisplayTCPIPProperties(HWND hParent,IP_ADAPTER_INFO *pInfo);

HINSTANCE hApplet = 0;

/* Applets */
APPLET Applets[] = 
{
	{IDI_CPLSYSTEM, IDS_CPLSYSTEMNAME, IDS_CPLSYSTEMDESCRIPTION, DisplayApplet}
};





BOOL FindNICClassKeyForCfgInstance(TCHAR *tpszCfgInst,TCHAR *tpszSubKeyOut)
{
	int i;
	TCHAR tpszSubKey[MAX_PATH];
	TCHAR tpszCfgInst2[MAX_PATH];
	HKEY hKey;
	DWORD dwType,dwSize;
	for(i=0;i<100;i++)
	{
		_stprintf(tpszSubKey,_T("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\%04d"),i);
		if(RegOpenKey(HKEY_LOCAL_MACHINE,tpszSubKey,&hKey)!=ERROR_SUCCESS)
			continue;
		dwType = REG_SZ;
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


void NICPropertyProtocolCallback(void *pCookie,HKEY hBaseKey,TCHAR *tpszSubKey)
{
	HWND hwndDlg;
	DWORD dwCharacteristics;
	HKEY hKey,hNDIKey;
	DWORD dwType,dwSize;
	TCHAR tpszDescription[MAX_PATH];
	TCHAR tpszNotifyObjectCLSID[MAX_PATH];
//	CLSID CLSID_NotifObj;
//	IUnknown *pUnk = NULL;
//	INetCfgComponentControl *pNetCfg;
//	INetCfgComponentPropertyUi *pNetCfgPropUI;
	hwndDlg = (HWND)pCookie;
	
	if(RegOpenKey(HKEY_LOCAL_MACHINE,tpszSubKey,&hKey)!=ERROR_SUCCESS)
		return;
	dwType = REG_DWORD;
	dwSize = sizeof(dwCharacteristics);
	if(RegQueryValueEx(hKey,_T("Characteristics"),NULL,&dwType,(BYTE*)&dwCharacteristics,&dwSize)!= ERROR_SUCCESS)
		return;
	if(dwCharacteristics & NCF_HIDDEN) {
		RegCloseKey(hKey);
		return;
	}

	dwType = REG_SZ;
	dwSize = sizeof(tpszDescription);
	if(RegQueryValueEx(hKey,_T("Description"),NULL,&dwType,(BYTE*)tpszDescription,&dwSize)!= ERROR_SUCCESS)
		return;

	RegOpenKey(hKey,_T("Ndi"),&hNDIKey);
	dwType = REG_SZ;
	dwSize = sizeof(tpszNotifyObjectCLSID);
	if(RegQueryValueEx(hNDIKey,_T("ClsId"),NULL,&dwType,(BYTE*)tpszNotifyObjectCLSID,&dwSize)!= ERROR_SUCCESS)
		;//return;
	RegCloseKey(hNDIKey);

	//
	// Thise code works on Windows... but not on Reactos
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
	SendDlgItemMessage(hwndDlg,IDC_COMPONENTSLIST,LB_ADDSTRING,0,(LPARAM)tpszDescription);
}



BOOL CALLBACK NICPropertyPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PROPSHEETPAGE *pPage = (PROPSHEETPAGE *)GetWindowLong(hwndDlg,GWL_USERDATA);
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
				MessageBox(hwndDlg,_T("NIC Entry not found"),_T("Registry error"),MB_ICONSTOP);

			if(RegOpenKey(HKEY_LOCAL_MACHINE,tpszSubKey,&hKey)!=ERROR_SUCCESS)
				return 0;
			dwType = REG_SZ;
			dwSize = sizeof(tpszDisplayName);
			if(RegQueryValueEx(hKey,_T("DriverDesc"),NULL,&dwType,(BYTE*)tpszDisplayName,&dwSize)!= ERROR_SUCCESS)
				return 0;
			RegCloseKey(hKey);

			SetDlgItemText(hwndDlg,IDC_NETCARDNAME,tpszDisplayName);
			EnableWindow(GetDlgItem(hwndDlg,IDC_CONFIGURE),FALSE);


			SetWindowLong(hwndDlg,GWL_USERDATA,lParam);
			//SetDlgItemTextA(hwndDlg,IDC_NETCARDNAME,Info[pPage->lParam].Description);
			EnumRegKeys(NICPropertyProtocolCallback,hwndDlg,HKEY_LOCAL_MACHINE,_T("System\\CurrentControlSet\\Control\\Network\\{4D36E975-E325-11CE-BFC1-08002BE10318}"));

		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_COMPONENTSLIST:
			if(HIWORD(wParam)!=LBN_DBLCLK)
				break;
			// drop though
		case IDC_PROPERTIES:
			MessageBox(NULL,_T("This control panel is incomplete.\r\nUsually, the \"Notify Object\" for this Network component should be invoked here. Reactos lacks the infrastructure to do this right now.\r\n- C++\r\n- DDK Headers for notify objects\r\n- clean header structure, that allow Windows-Compatible COM C++ Code"),_T("Error"),MB_ICONSTOP);
//			DisplayTCPIPProperties(hwndDlg,&Info[pPage->lParam]);
			break;
		}
		break;
	}
	return FALSE;
}


void DisplayNICProperties(HWND hParent,TCHAR *tpszCfgInstanceID)
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
	if(RegOpenKey(HKEY_LOCAL_MACHINE,tpszSubKey,&hKey)!=ERROR_SUCCESS)
		return;
	if(RegQueryValueEx(hKey,_T("Name"),NULL,&dwType,(BYTE*)tpszName,&dwSize)!=ERROR_SUCCESS)
		_stprintf(tpszName,_T("[ERROR]"));
		//_stprintf(tpszName,_T("[ERROR]") _T(__FILE__) _T(" %d"),__LINE__ );
	else
		_tcscat(tpszName,_T(" Properties"));
	RegCloseKey(hKey);
	
	
	ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
	psh.hwndParent = NULL;
	psh.hInstance = hApplet;
#ifdef _MSC_VER
	psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDI_CPLSYSTEM));
#else
	psh.u1.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDI_CPLSYSTEM));
#endif
	psh.pszCaption = tpszName;
	psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
#ifdef _MSC_VER
	psh.nStartPage = 0;
	psh.ppsp = psp;
#else
	psh.u2.nStartPage = 0;
	psh.u3.ppsp = psp;
#endif
	psh.pfnCallback = NULL;
	

	InitPropSheetPage(&psp[0], IDD_NETPROPERTIES, NICPropertyPageProc,(LPARAM)tpszCfgInstanceID);
	PropertySheet(&psh)	;
	return;
}



BOOL CALLBACK NICStatusPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
    case WM_INITDIALOG:	
		{
			PROPSHEETPAGE *psp= (PROPSHEETPAGE *)lParam;
			EnableWindow(GetDlgItem(hwndDlg,IDC_ENDISABLE),FALSE);
			SetWindowLong(hwndDlg,DWL_USER,psp->lParam);
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

void DisplayNICStatus(HWND hParent,TCHAR *tpszCfgInstanceID)
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
	if(RegOpenKey(HKEY_LOCAL_MACHINE,tpszSubKey,&hKey)!=ERROR_SUCCESS)
		return;
	if(RegQueryValueEx(hKey,_T("Name"),NULL,&dwType,(BYTE*)tpszName,&dwSize)!=ERROR_SUCCESS)
		_stprintf(tpszName,_T("[ERROR]"));
		//_stprintf(tpszName,_T("[ERROR]") _T(__FILE__) _T(" %d"),__LINE__ );
	else
		_tcscat(tpszName,_T(" Status"));
	RegCloseKey(hKey);

	ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
	psh.hwndParent = NULL;
	psh.hInstance = hApplet;
	// FIX THESE REACTOS HEADERS !!!!!!!!!
#ifdef _MSC_VER
	psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDI_CPLSYSTEM));
#else
	psh.u1.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDI_CPLSYSTEM));
#endif
	psh.pszCaption = tpszName;//Caption;
	psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
#ifdef _MSC_VER
	psh.nStartPage = 0;
	psh.ppsp = psp;
#else
	psh.u2.nStartPage = 0;
	psh.u3.ppsp = psp;
#endif
	psh.pfnCallback = NULL;
	

	InitPropSheetPage(&psp[0], IDD_CARDPROPERTIES, NICStatusPageProc,(LPARAM)tpszCfgInstanceID);
	PropertySheet(&psh)	;
	return;
}

//
// IPHLPAPI does not provide a list of all adapters
//
/*
void EnumAdapters(HWND hwndDlg)
{
	IP_ADAPTER_INFO *pInfo;
	ULONG size=sizeof(Info);
	TCHAR pszText[MAX_ADAPTER_NAME_LENGTH + 4];
	int nIndex;

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
*/



void NetAdapterCallback(void *pCookie,HKEY hBaseKey,TCHAR *tpszSubKey)
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

	OutputDebugString(_T("NetAdapterCallback\r\n"));
	OutputDebugString(tpszSubKey);
	OutputDebugString(_T("\r\n"));
	if(RegOpenKeyEx(hBaseKey,tpszSubKey,0,KEY_ALL_ACCESS,&hKey)!=ERROR_SUCCESS)
		return;

	OutputDebugString(_T("NetAdapterCallback: Reading Characteristics\r\n"));
	dwType = REG_DWORD;
	dwSize = sizeof(dwCharacteristics);
	if(RegQueryValueEx(hKey,_T("Characteristics"),NULL,&dwType,(BYTE*)&dwCharacteristics,&dwSize)!=ERROR_SUCCESS)
		dwCharacteristics = 0;


	if(dwCharacteristics & NCF_HIDDEN)
		return;
//	if(!(dwCharacteristics & NCF_HAS_UI))
//		return;

	OutputDebugString(_T("NetAdapterCallback: Reading DriverDesc\r\n"));
	dwType = REG_SZ;
	dwSize = sizeof(tpszDisplayName);
	if(RegQueryValueEx(hKey,_T("DriverDesc"),NULL,&dwType,(BYTE*)tpszDisplayName,&dwSize)!= ERROR_SUCCESS)
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
	

	nIndex = SendDlgItemMessage(hwndDlg,IDC_NETCARDLIST,LB_ADDSTRING,0,(LPARAM)tpszDisplayName);
	SendDlgItemMessage(hwndDlg,IDC_NETCARDLIST,LB_SETITEMDATA,nIndex,(LPARAM)ptpszCfgInstanceID);
	RegCloseKey(hKey);
}


void EnumAdapters(HWND hwndDlg)
{
	TCHAR *tpszRegPath = _T("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}");

	EnumRegKeys(NetAdapterCallback,hwndDlg,HKEY_LOCAL_MACHINE,tpszRegPath);
	return;
}


BOOL CALLBACK NetworkPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	NMHDR* pnmh;
	int nIndex;
	switch(uMsg)
	{
    case WM_INITDIALOG:	
		{
			EnableWindow(GetDlgItem(hwndDlg,IDC_ADD),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_REMOVE),FALSE);

			SendDlgItemMessage(hwndDlg,IDC_NETCARDLIST,LB_SETCURSEL,0,0);
		
			EnumAdapters(hwndDlg);
		}
		break;

	case WM_NOTIFY:
		pnmh=(NMHDR*)lParam;
		switch(pnmh->code) {
		case PSN_APPLY:
		case PSN_RESET:
			{
				while(SendDlgItemMessage(hwndDlg,IDC_NETCARDLIST,LB_GETCOUNT,0,0)>0)
				{
					TCHAR *tpszString;
					tpszString = (TCHAR*)SendDlgItemMessage(hwndDlg,IDC_NETCARDLIST,LB_GETITEMDATA,0,0);
					if(tpszString)
						free(tpszString);
					SendDlgItemMessage(hwndDlg,IDC_NETCARDLIST,LB_DELETESTRING,0,0);
				}
			}
			break;
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_NETCARDLIST:
			if(HIWORD(wParam)==LBN_DBLCLK) {
				nIndex = SendDlgItemMessage(hwndDlg,IDC_NETCARDLIST,LB_GETCURSEL,0,0);
				if(nIndex!=-1)
					DisplayNICStatus(hwndDlg,(TCHAR*)SendDlgItemMessage(hwndDlg,IDC_NETCARDLIST,LB_GETITEMDATA,nIndex,0));
			}
			break;
		case IDC_PROPERTIES:
			nIndex = SendDlgItemMessage(hwndDlg,IDC_NETCARDLIST,LB_GETCURSEL,0,0);
			if(nIndex!=-1)
				DisplayNICStatus(hwndDlg,(TCHAR*)SendDlgItemMessage(hwndDlg,IDC_NETCARDLIST,LB_GETITEMDATA,nIndex,0));
			break;
		}
		break;
	}
	return FALSE;
}



/* First Applet */
LONG CALLBACK DisplayApplet(VOID)
{
	PROPSHEETPAGE psp[1];
	PROPSHEETHEADER psh;
	TCHAR Caption[1024];
	
	LoadString(hApplet, IDS_CPLSYSTEMNAME, Caption, sizeof(Caption) / sizeof(TCHAR));
	
	ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags =  PSH_PROPSHEETPAGE;
	psh.hwndParent = NULL;
	psh.hInstance = hApplet;
#ifdef _MSC_VER
	psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDI_CPLSYSTEM));
#else
	psh.u1.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDI_CPLSYSTEM));
#endif
	psh.pszCaption = Caption;
	psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
#ifdef _MSC_VER
	psh.nStartPage = 0;
	psh.ppsp = psp;
#else
	psh.u2.nStartPage = 0;
	psh.u3.ppsp = psp;
#endif
	psh.pfnCallback = NULL;
	

	InitPropSheetPage(&psp[0], IDD_PROPPAGENETWORK, NetworkPageProc,0);
	
	return (LONG)(PropertySheet(&psh) != -1);
}

/* Control Panel Callback */
LONG CALLBACK CPlApplet(HWND hwndCPl, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
	int i = (int)lParam1;
	
	switch(uMsg)
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
			CPlInfo->idIcon = Applets[i].idIcon;
			CPlInfo->idName = Applets[i].idName;
			CPlInfo->idInfo = Applets[i].idDescription;
			break;
		}
    case CPL_DBLCLK:
		{
			Applets[i].AppletProc();
			break;
		}
	}
	return FALSE;
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
	switch(dwReason)
	{
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
		hApplet = hinstDLL;
		break;
	}
	return TRUE;
}

