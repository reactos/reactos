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

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(ncpa);
#ifndef _UNICODE
#define debugstr_aw debugstr_a
#else
#define debugstr_aw debugstr_w
#endif

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
		WARN("EnumRegKeys failed (key not found): %S\n", tpszRegPath);
		return;
	}

	for(i=0;;i++)
	{
		TCHAR pszNewPath[MAX_PATH];
		ret = RegEnumKeyEx(hKey,i,tpszName,&dwNameLen,NULL,NULL,NULL,NULL);
		if(ret != ERROR_SUCCESS)
		{
			WARN("EnumRegKeys: RegEnumKeyEx failed for %S (rc 0x%lx)\n", tpszName, ret);
			break;
		}

		_stprintf(pszNewPath,_T("%s\\%s"),tpszRegPath,tpszName);
		TRACE("EnumRegKeys: Calling user supplied enum function\n");
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
	PGLOBAL_NCPA_DATA pGlobalData = (PGLOBAL_NCPA_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			DWORD dwType,dwSize;
			TCHAR tpszSubKey[MAX_PATH];
			TCHAR tpszDisplayName[MAX_PATH];
			HKEY hKey;

			pGlobalData = (PGLOBAL_NCPA_DATA)((LPPROPSHEETPAGE)lParam)->lParam;
			if (pGlobalData == NULL)
				return FALSE;

			SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);

			if(!FindNICClassKeyForCfgInstance(pGlobalData->CurrentAdapterName, tpszSubKey))
			{
				WARN("NIC Entry not found for '%s'\n", debugstr_w(pGlobalData->CurrentAdapterName));
				MessageBox(hwndDlg,_T("NIC Entry not found"),_T("Registry error"),MB_ICONSTOP);
				MessageBox(hwndDlg,pGlobalData->CurrentAdapterName,tpszSubKey,MB_ICONSTOP);
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

			//SetDlgItemTextA(hwndDlg,IDC_NETCARDNAME,Info[pPage->lParam].Description);
			EnumRegKeys(NICPropertyProtocolCallback,hwndDlg,HKEY_LOCAL_MACHINE,_T("System\\CurrentControlSet\\Control\\Network\\{4D36E975-E325-11CE-BFC1-08002BE10318}"));

			SendDlgItemMessage(hwndDlg, IDC_COMPONENTSLIST, LB_SETCURSEL, 0, 0);
			SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_COMPONENTSLIST, LBN_SELCHANGE), 0);
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
				if(pGlobalData->pCurrentAdapterInfo)
				{
					DisplayTCPIPProperties(hwndDlg, pGlobalData->pCurrentAdapterInfo);
				}
				else
				{
					FIXME("If you see this, then the IPHLPAPI.DLL probably needs more work because GetAdaptersInfo did not return the expected data.\n");
					MessageBox(NULL,_T("If you see this, then the IPHLPAPI.DLL probably needs more work because GetAdaptersInfo did not return the expected data."),_T("Error"),MB_ICONSTOP);
				}
			}
			break;
		}
		break;
	}
	return FALSE;
}


static void
DisplayNICProperties(HWND hParent, GLOBAL_NCPA_DATA* pGlobalData)
{
	PROPSHEETPAGE psp[1];
	PROPSHEETHEADER psh;
	TCHAR tpszSubKey[MAX_PATH];
	HKEY hKey;
	DWORD dwType = REG_SZ;
	TCHAR tpszName[MAX_PATH];
	DWORD dwSize = sizeof(tpszName);
	TCHAR tpszCfgInstanceID[MAX_ADAPTER_NAME_LENGTH];

#ifndef _UNICODE
	WideCharToMultiByte(CP_UTF8, 0, pGlobalData->CurrentAdapterName, -1, tpszCfgInstanceID, MAX_ADAPTER_NAME_LENGTH, 0, 0);
#else
	wcscpy(tpszCfgInstanceID, pGlobalData->CurrentAdapterName);
#endif

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

	InitPropSheetPage(&psp[0], IDD_NETPROPERTIES, NICPropertyPageProc, (LPARAM)pGlobalData);
	PropertySheet(&psh);
	return;
}

void RefreshNICInfo(HWND hwndDlg, PGLOBAL_NCPA_DATA pGlobalData)
{
	ULONG BufferSize;
	DWORD ErrRet = 0;

	if (pGlobalData->pFirstAdapterInfo)
		HeapFree(GetProcessHeap(), 0, pGlobalData->pFirstAdapterInfo);

	BufferSize = sizeof(IP_ADAPTER_INFO);
	pGlobalData->pFirstAdapterInfo = (PIP_ADAPTER_INFO) HeapAlloc(GetProcessHeap(), 0, BufferSize);

	if (GetAdaptersInfo(pGlobalData->pFirstAdapterInfo, &BufferSize) == ERROR_BUFFER_OVERFLOW)
	{
		HeapFree(GetProcessHeap(), 0, pGlobalData->pFirstAdapterInfo);
		pGlobalData->pFirstAdapterInfo = (PIP_ADAPTER_INFO) HeapAlloc(GetProcessHeap(), 0, BufferSize);
	}

	if ((ErrRet = GetAdaptersInfo(pGlobalData->pFirstAdapterInfo, &BufferSize)) != NO_ERROR)
	{
		ERR("error adapterinfo\n");
		MessageBox(hwndDlg, _T("error adapterinfo") ,_T("ncpa.cpl"),MB_ICONSTOP);

		if (pGlobalData->pFirstAdapterInfo)
			HeapFree(GetProcessHeap(), 0, pGlobalData->pFirstAdapterInfo);
	}
}

void UpdateCurrentAdapterInfo(HWND hwndDlg, PGLOBAL_NCPA_DATA pGlobalData)
{
	PIP_INTERFACE_INFO pInfo;
	ULONG BufferSize = 0;
	DWORD dwRetVal = 0;

	if (!pGlobalData->pCurrentAdapterInfo)
		return;

	BufferSize = sizeof(IP_INTERFACE_INFO);
	pInfo = (PIP_INTERFACE_INFO) HeapAlloc(GetProcessHeap(), 0, BufferSize);
	if (ERROR_INSUFFICIENT_BUFFER == GetInterfaceInfo(pInfo, &BufferSize))
	{
		HeapFree(GetProcessHeap(), 0, pInfo);
		pInfo = (PIP_INTERFACE_INFO) HeapAlloc(GetProcessHeap(), 0, BufferSize);
	}

	dwRetVal = GetInterfaceInfo(pInfo, &BufferSize);
	if (NO_ERROR == dwRetVal)
	{
		DWORD i;

		for (i = 0; i < pInfo->NumAdapters; i++)
		{
			if (0 == wcscmp(pGlobalData->CurrentAdapterName, pInfo->Adapter[i].Name))
			{
				if (pInfo->Adapter[i].Index != pGlobalData->pCurrentAdapterInfo->Index)
				{
					RefreshNICInfo(hwndDlg, pGlobalData);

					pGlobalData->pCurrentAdapterInfo = pGlobalData->pFirstAdapterInfo;
					while (pGlobalData->pCurrentAdapterInfo)
					{
						if (pGlobalData->pCurrentAdapterInfo->Index == pInfo->Adapter[i].Index)
							return;

						pGlobalData->pCurrentAdapterInfo = pGlobalData->pCurrentAdapterInfo->Next;
					}
				}
			}
		}
	}
	else if (ERROR_NO_DATA == dwRetVal)
		WARN("There are no network adapters with IPv4 enabled on the local system\n");
	else
		ERR("GetInterfaceInfo failed.\n");
}

static VOID
UpdateNICStatusData(HWND hwndDlg, PGLOBAL_NCPA_DATA pGlobalData)
{
	DWORD dwRet = NO_ERROR;

	if (pGlobalData->pCurrentAdapterInfo)
	{
		if (NULL == pGlobalData->pIfTable)
		{
			pGlobalData->IfTableSize = sizeof(MIB_IFTABLE);
			pGlobalData->pIfTable = (PMIB_IFTABLE)HeapAlloc(GetProcessHeap(), 0, pGlobalData->IfTableSize);
			if (NULL == pGlobalData->pIfTable)
			{
				static BOOL firstError = TRUE;
				if (firstError)
				{
					firstError = FALSE;
					WARN("Out of memory - could not allocate MIB_IFTABLE(1)");
					return;
				}
			}
		}

		/* Call GetIfTable once to see if we have a large enough buffer */
		dwRet = GetIfTable(pGlobalData->pIfTable, &pGlobalData->IfTableSize, FALSE);
		if (ERROR_INSUFFICIENT_BUFFER == dwRet)
		{
			HeapFree(GetProcessHeap(), 0, pGlobalData->pIfTable);

			pGlobalData->pIfTable = (PMIB_IFTABLE)HeapAlloc(GetProcessHeap(), 0, pGlobalData->IfTableSize);
			if (NULL == pGlobalData->pIfTable)
			{
				static BOOL firstError = TRUE;
				if (firstError)
				{
					firstError = FALSE;
					WARN("Out of memory - could not allocate MIB_IFTABLE(2)");
				}

				pGlobalData->IfTableSize = 0;
				return;
			}

			dwRet = GetIfTable(pGlobalData->pIfTable, &pGlobalData->IfTableSize, FALSE);
			if (NO_ERROR != dwRet)
			{
				HeapFree(GetProcessHeap(), 0, pGlobalData->pIfTable);
				pGlobalData->pIfTable = NULL;
				pGlobalData->IfTableSize = 0;
				return;
			}
		}
	}

	if (NO_ERROR == dwRet)
	{
		DWORD i;
		DWORD PktsOut = 0;
		DWORD PktsIn = 0;
		DWORD Mbps = 0;
		DWORD OperStatus = IF_OPER_STATUS_DISCONNECTED;
		PMIB_IFROW pIfRow = NULL;
		TCHAR Buffer[256], LocBuffer[256];
		SYSTEMTIME TimeConnected;

		memset(&TimeConnected, 0, sizeof(TimeConnected));

		if (pGlobalData->pCurrentAdapterInfo)
		{
			UpdateCurrentAdapterInfo(hwndDlg, pGlobalData);

			for (i = 0; i < pGlobalData->pIfTable->dwNumEntries; i++)
			{
				pIfRow = (PMIB_IFROW)&pGlobalData->pIfTable->table[i];

				if (pIfRow->dwIndex == pGlobalData->pCurrentAdapterInfo->Index)
				{
					DWORD DurationSeconds;
					SYSTEMTIME SystemTime;
					FILETIME SystemFileTime;
					ULARGE_INTEGER LargeSystemTime;

					PktsOut = pIfRow->dwOutUcastPkts;
					PktsIn = pIfRow->dwInUcastPkts;
					Mbps = pIfRow->dwSpeed;
					OperStatus = pIfRow->dwOperStatus;

					/* TODO: For some unknown reason, this doesn't correspond to the Windows duration */
					GetSystemTime(&SystemTime);
					SystemTimeToFileTime(&SystemTime, &SystemFileTime);
					LargeSystemTime = *(ULARGE_INTEGER *)&SystemFileTime;
					LargeSystemTime.QuadPart /= 100000ULL;
					DurationSeconds = ((LargeSystemTime.LowPart - pIfRow->dwLastChange) / 100);
					TimeConnected.wSecond = (DurationSeconds % 60);
					TimeConnected.wMinute = (DurationSeconds / 60) % 60;
					TimeConnected.wHour = (DurationSeconds / (60 * 60)) % 24;
					TimeConnected.wDay = DurationSeconds / (60 * 60 * 24);

					break;
				}
			}
		}

		_stprintf(Buffer, L"%u", PktsOut);
		GetNumberFormat(LOCALE_USER_DEFAULT, 0, Buffer, NULL, LocBuffer, sizeof(LocBuffer) / sizeof(LocBuffer[0]));
		SendDlgItemMessage(hwndDlg, IDC_SEND, WM_SETTEXT, 0, (LPARAM)LocBuffer);

		_stprintf(Buffer, L"%u", PktsIn);
		GetNumberFormat(LOCALE_USER_DEFAULT, 0, Buffer, NULL, LocBuffer, sizeof(LocBuffer) / sizeof(LocBuffer[0]));
		SendDlgItemMessage(hwndDlg, IDC_RECEIVED, WM_SETTEXT, 0, (LPARAM)LocBuffer);

		switch (OperStatus)
		{
		case IF_OPER_STATUS_NON_OPERATIONAL:
			OperStatus = IDS_STATUS_NON_OPERATIONAL;
			break;

		case IF_OPER_STATUS_UNREACHABLE:
			OperStatus = IDS_STATUS_UNREACHABLE;
			break;

		case IF_OPER_STATUS_DISCONNECTED:
			OperStatus = IDS_STATUS_DISCONNECTED;
			break;

		case IF_OPER_STATUS_CONNECTING:
			OperStatus = IDS_STATUS_CONNECTING;
			break;

		case IF_OPER_STATUS_CONNECTED:
			OperStatus = IDS_STATUS_CONNECTED;
			break;

		case IF_OPER_STATUS_OPERATIONAL:
			/* TODO: Find sub status, waiting for DHCP address, etc. */
			OperStatus = IDS_STATUS_OPERATIONAL;
			break;

		default:
			WARN("Unknown operation status: %d\n", OperStatus);
			OperStatus = IDS_STATUS_OPERATIONAL;
			break;
		}
		LoadString(hApplet, OperStatus, LocBuffer, sizeof(LocBuffer) / sizeof(LocBuffer[0]));
		SendDlgItemMessage(hwndDlg, IDC_STATUS, WM_SETTEXT, 0, (LPARAM)LocBuffer);

		GetTimeFormat(LOCALE_USER_DEFAULT, 0, &TimeConnected, L"HH':'mm':'ss", LocBuffer, sizeof(LocBuffer) / sizeof(LocBuffer[0]));
		if (0 == TimeConnected.wDay)
		{
			SendDlgItemMessage(hwndDlg, IDC_DURATION, WM_SETTEXT, 0, (LPARAM)LocBuffer);
		}
		else
		{
			TCHAR DayBuffer[256];
			if (1 == TimeConnected.wDay)
			{
				LoadString(hApplet, IDS_DURATION_DAY, DayBuffer, sizeof(DayBuffer) / sizeof(DayBuffer[0]));
			}
			else
			{
				LoadString(hApplet, IDS_DURATION_DAYS, DayBuffer, sizeof(DayBuffer) / sizeof(DayBuffer[0]));
			}
			_sntprintf(Buffer, 256, DayBuffer, TimeConnected.wDay, LocBuffer);
			SendDlgItemMessage(hwndDlg, IDC_DURATION, WM_SETTEXT, 0, (LPARAM)Buffer);
		}

		LoadString(hApplet, IDS_SPEED_MBPS, LocBuffer, sizeof(LocBuffer) / sizeof(LocBuffer[0]));
		_sntprintf(Buffer, 256, LocBuffer, Mbps / 1000000);
		SendDlgItemMessage(hwndDlg, IDC_SPEED, WM_SETTEXT, 0, (LPARAM)Buffer);
	}
	else
	{
		static BOOL firstError = TRUE;
		if (firstError)
		{
			firstError = FALSE;
			ERR("GetIfTable failed with error code: %d\n", dwRet);
			return;
		}
	}
}

static INT_PTR CALLBACK
NICStatusPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PGLOBAL_NCPA_DATA pGlobalData;
	pGlobalData = (PGLOBAL_NCPA_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			pGlobalData = (PGLOBAL_NCPA_DATA)((LPPROPSHEETPAGE)lParam)->lParam;
			if (pGlobalData == NULL)
				return FALSE;

			SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);

			EnableWindow(GetDlgItem(hwndDlg,IDC_ENDISABLE),FALSE);
			pGlobalData->hStatsUpdateTimer = SetTimer(hwndDlg, 1, 1000, NULL);
			UpdateNICStatusData(hwndDlg, pGlobalData);
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_PROPERTIES:
			{
				DisplayNICProperties(hwndDlg, pGlobalData);
			}
			break;
		}
		break;
	case WM_TIMER:
		{
			UpdateNICStatusData(hwndDlg, pGlobalData);
		}
		break;
	case WM_DESTROY:
		{
			KillTimer(hwndDlg, pGlobalData->hStatsUpdateTimer);
			pGlobalData->hStatsUpdateTimer = 0;

			if (pGlobalData->pIfTable)
			{
				HeapFree(GetProcessHeap(), 0, pGlobalData->pIfTable);
				pGlobalData->pIfTable = NULL;
				pGlobalData->IfTableSize = 0;
			}
		}
		break;
	}
	return FALSE;
}

static INT_PTR CALLBACK
NICSupportPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PGLOBAL_NCPA_DATA pGlobalData;
	pGlobalData = (PGLOBAL_NCPA_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			pGlobalData = (PGLOBAL_NCPA_DATA)((LPPROPSHEETPAGE)lParam)->lParam;
			if (pGlobalData == NULL)
				return FALSE;

			SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);

			if (pGlobalData->pCurrentAdapterInfo)
			{
				TCHAR Buffer[64];

				if (pGlobalData->pCurrentAdapterInfo->DhcpEnabled)
					LoadString(hApplet, IDS_ASSIGNED_DHCP, Buffer, sizeof(Buffer) / sizeof(TCHAR));
				else
					LoadString(hApplet, IDS_ASSIGNED_MANUAL, Buffer, sizeof(Buffer) / sizeof(TCHAR));

				SendDlgItemMessage(hwndDlg, IDC_DETAILSTYPE, WM_SETTEXT, 0, (LPARAM)Buffer);
				_stprintf(Buffer, _T("%S"), pGlobalData->pCurrentAdapterInfo->IpAddressList.IpAddress.String);
				SendDlgItemMessage(hwndDlg, IDC_DETAILSIP, WM_SETTEXT, 0, (LPARAM)Buffer);
				_stprintf(Buffer, _T("%S"), pGlobalData->pCurrentAdapterInfo->IpAddressList.IpMask.String);
				SendDlgItemMessage(hwndDlg, IDC_DETAILSSUBNET, WM_SETTEXT, 0, (LPARAM)Buffer);
				_stprintf(Buffer, _T("%S"), pGlobalData->pCurrentAdapterInfo->GatewayList.IpAddress.String);
				SendDlgItemMessage(hwndDlg, IDC_DETAILSGATEWAY, WM_SETTEXT, 0, (LPARAM)Buffer);
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
				FIXME("Not implemented: show detail window\n");
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
	PGLOBAL_NCPA_DATA pGlobalData;
	PIP_ADAPTER_INFO pInfo;
	WCHAR wcsAdapterName[MAX_ADAPTER_NAME];

	pGlobalData = (PGLOBAL_NCPA_DATA)GetWindowLongPtr(hParent, DWLP_USER);

#ifndef _UNICODE
	MultiByteToWideChar(CP_UTF8, 0, tpszCfgInstanceID, -1, pGlobalData->CurrentAdapterName, MAX_ADAPTER_NAME);
#else
	wcscpy(pGlobalData->CurrentAdapterName, tpszCfgInstanceID);
#endif

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

	RefreshNICInfo(hParent, pGlobalData);

	pGlobalData->pCurrentAdapterInfo = NULL;
	pInfo = pGlobalData->pFirstAdapterInfo;
	while (pInfo)
	{
		MultiByteToWideChar(CP_UTF8, 0, pInfo->AdapterName, -1, wcsAdapterName, MAX_ADAPTER_NAME);
		if (0 == wcscmp(wcsAdapterName, pGlobalData->CurrentAdapterName))
		{
			pGlobalData->pCurrentAdapterInfo = pInfo;
			break;
		}

		pInfo = pInfo->Next;
	}

	InitPropSheetPage(&psp[0], IDD_CARDPROPERTIES, NICStatusPageProc, (LPARAM)pGlobalData);
	InitPropSheetPage(&psp[1], IDD_CARDSUPPORT, NICSupportPageProc, (LPARAM)pGlobalData);

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
		WARN("IPHLPAPI.DLL failed to provide Adapter information\n");
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

	TRACE("NetAdapterCallback: %s\n", debugstr_aw(tpszSubKey));

	if(RegOpenKeyEx(hBaseKey,tpszSubKey,0,KEY_QUERY_VALUE,&hKey)!=ERROR_SUCCESS)
		return;

	TRACE("NetAdapterCallback: Reading Characteristics\n");
	dwType = REG_DWORD;
	dwSize = sizeof(dwCharacteristics);
	if(RegQueryValueEx(hKey,_T("Characteristics"),NULL,&dwType,(BYTE*)&dwCharacteristics,&dwSize)!=ERROR_SUCCESS)
		dwCharacteristics = 0;
	if (dwCharacteristics & NCF_HIDDEN)
		return;
	if (!(dwCharacteristics & NCF_VIRTUAL) && !(dwCharacteristics & NCF_PHYSICAL))
		return;

	TRACE("NetAdapterCallback: Reading DriverDesc\n");
	dwType = REG_SZ;
	dwSize = sizeof(tpszDisplayName);
	if (RegQueryValueEx(hKey,_T("DriverDesc"),NULL,&dwType,(BYTE*)tpszDisplayName,&dwSize)!= ERROR_SUCCESS)
		_tcscpy(tpszDisplayName,_T("Unnamed Adapter"));
	TRACE("Network card: '%s'\n", debugstr_aw(tpszDisplayName));

	// get the link to the Enum Subkey (currently unused)
	//dwType = REG_SZ;
	//dwSize = sizeof(tpszDeviceID);
	//if(RegQueryValueEx(hKey,_T("MatchingDeviceId"),NULL,&dwType,(BYTE*)tpszDeviceID,&dwSize) != ERROR_SUCCESS) {
	//	WARN("Missing MatchingDeviceId Entry\n");
	//	MessageBox(hwndDlg,_T("Missing MatchingDeviceId Entry"),_T("Registry Problem"),MB_ICONSTOP);
	//	return;
	//}

	// get the card configuration GUID
	dwType = REG_SZ;
	dwSize = sizeof(tpszCfgInstanceID);
	if(RegQueryValueEx(hKey,_T("NetCfgInstanceId"),NULL,&dwType,(BYTE*)tpszCfgInstanceID,&dwSize) != ERROR_SUCCESS)
	{
		ERR("Missing NetCfgInstanceId Entry\n");
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
			PGLOBAL_NCPA_DATA pGlobalData = (PGLOBAL_NCPA_DATA)((LPPROPSHEETPAGE)lParam)->lParam;
			if (pGlobalData == NULL)
				return FALSE;

			SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);

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
	PGLOBAL_NCPA_DATA pGlobalData;
	PROPSHEETPAGE psp[1];
	PROPSHEETHEADER psh = {0};
	TCHAR Caption[1024];
	int Ret;

	LoadString(hApplet, IDS_CPLSYSTEMNAME, Caption, sizeof(Caption) / sizeof(TCHAR));

	pGlobalData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GLOBAL_NCPA_DATA));
	if (pGlobalData == NULL)
		return 0;

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

	InitPropSheetPage(&psp[0], IDD_PROPPAGENETWORK, NetworkPageProc, (LPARAM)pGlobalData);

	Ret = PropertySheet(&psh);

	HeapFree(GetProcessHeap(), 0, pGlobalData);

	return (LONG)(Ret != -1);
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
