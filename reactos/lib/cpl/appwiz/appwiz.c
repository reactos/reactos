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
/* $Id: appwiz.c,v 1.1 2004/06/18 20:43:44 kuehng Exp $
 *
 * PROJECT:         ReactOS Software Control Panel
 * FILE:            lib/cpl/system/appwiz.c
 * PURPOSE:         ReactOS Software Control Panel
 * PROGRAMMER:      Gero Kuehn (reactos.filter@gkware.com)
 * UPDATE HISTORY:
 *      06-17-2004  Created
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>
#include <windows.h>

#ifdef _MSC_VER
#include <commctrl.h>
#include <cpl.h>
#endif

#include <stdlib.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>

#include "resource.h"
#include "appwiz.h"

#define NUM_APPLETS	(1)

LONG CALLBACK SystemApplet(VOID);
BOOL CALLBACK GeneralPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ComputerPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
HINSTANCE hApplet = 0;

/* Applets */
APPLET Applets[NUM_APPLETS] = 
{
	{IDI_CPLSYSTEM, IDS_CPLSYSTEMNAME, IDS_CPLSYSTEMDESCRIPTION, SystemApplet}
};

void CallUninstall(HWND hwndDlg)
{
	int nIndex;
	nIndex = SendDlgItemMessage(hwndDlg,IDC_SOFTWARELIST,LB_GETCURSEL,0,0);
	if(nIndex == -1)
		MessageBox(hwndDlg,L"No item selected",L"Error",MB_ICONSTOP);
	else {
		HKEY hKey;
		DWORD dwType = REG_SZ;
		TCHAR pszUninstallString[MAX_PATH];
		DWORD dwSize = MAX_PATH;
		hKey = (HKEY)SendDlgItemMessage(hwndDlg,IDC_SOFTWARELIST,LB_GETITEMDATA,(WPARAM)nIndex,0);
		if(RegQueryValueEx(hKey,L"UninstallString",NULL,&dwType,(BYTE*)pszUninstallString,&dwSize)==ERROR_SUCCESS)
		{
			STARTUPINFO si;
			PROCESS_INFORMATION pi;
			memset(&si,0x00,sizeof(si));
			si.cb = sizeof(si);
			si.wShowWindow = SW_SHOW;
			if(CreateProcess(NULL,pszUninstallString,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi))
			{
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
			}
		} else {
			MessageBox(hwndDlg,L"Unable to read UninstallString. This entry is invalid or has been created by an MSI installer.",L"Error",MB_ICONSTOP);
		}
	}
}

/* Property page dialog callback */
BOOL CALLBACK InstallPageProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
    case WM_INITDIALOG:
		{
			HKEY hKey;
			int i=0;
			TCHAR pszName[MAX_PATH];
			FILETIME FileTime;
			DWORD dwSize = MAX_PATH;
			EnableWindow(GetDlgItem(hwndDlg,IDC_INSTALL),FALSE);			
			if(RegOpenKey(HKEY_LOCAL_MACHINE,L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",&hKey)!=ERROR_SUCCESS) {
				MessageBox(hwndDlg,L"Unable to open Uninstall Key",L"Error",MB_ICONSTOP);
				return FALSE;
			}
			while(RegEnumKeyEx(hKey,i,pszName,&dwSize,NULL,NULL,NULL,&FileTime)==ERROR_SUCCESS)
			{
				HKEY hSubKey;
				if(RegOpenKey(hKey,pszName,&hSubKey)==ERROR_SUCCESS)
				{
					DWORD dwType = REG_SZ;
					TCHAR pszDisplayName[MAX_PATH];
					DWORD dwSize = MAX_PATH;
					ULONG index;
					if(RegQueryValueEx(hSubKey,L"DisplayName",NULL,&dwType,(BYTE*)pszDisplayName,&dwSize)==ERROR_SUCCESS)
					{
						index = SendDlgItemMessage(hwndDlg,IDC_SOFTWARELIST,LB_ADDSTRING,0,(LPARAM)pszDisplayName);
						SendDlgItemMessage(hwndDlg,IDC_SOFTWARELIST,LB_SETITEMDATA,index,(LPARAM)hSubKey);
					} 
				}
				
				dwSize = MAX_PATH;
				i++;
			}
			RegCloseKey(hKey);
			break;
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_SOFTWARELIST:
			if(HIWORD(wParam)==LBN_DBLCLK)
			{
				CallUninstall(hwndDlg);
			}
			break;
		
		case IDC_ADDREMOVE:
			CallUninstall(hwndDlg);
			break;
		}
		break;
	}
	return FALSE;
}

/* Property page dialog callback */
BOOL CALLBACK RosPageProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
    case WM_INITDIALOG:
		{
			
			break;
		}
	}
	return FALSE;
}

static void InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc)
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
}


/* First Applet */

LONG CALLBACK
SystemApplet(VOID)
{
	PROPSHEETPAGE psp[2];
	PROPSHEETHEADER psh;
	TCHAR Caption[1024];
	
	LoadString(hApplet, IDS_CPLSYSTEMNAME, Caption, sizeof(Caption) / sizeof(TCHAR));
	
	ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_PROPTITLE;
	psh.hwndParent = NULL;
	psh.hInstance = hApplet;
#ifdef _MSC_VER
	psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDI_CPLSYSTEM));
#else
	psh.u1.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDI_CPLSYSTEM));
#endif
	psh.pszCaption = Caption;
	psh.nPages = sizeof(psp) / sizeof(PROPSHEETHEADER);
#ifdef _MSC_VER
	psh.nStartPage = 0;
	psh.ppsp = psp;
#else
	psh.u2.nStartPage = 0;
	psh.u3.ppsp = psp;
#endif
	psh.pfnCallback = NULL;
	

	InitPropSheetPage(&psp[0], IDD_PROPPAGEINSTALL, InstallPageProc);
	InitPropSheetPage(&psp[1], IDD_PROPPAGEROSSETUP, RosPageProc);
	
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
			return NUM_APPLETS;
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

