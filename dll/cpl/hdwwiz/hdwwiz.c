/*
 * ReactOS New devices installation
 * Copyright (C) 2005 ReactOS Team
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
 * PROJECT:         ReactOS Add hardware control panel
 * FILE:            lib/cpl/hdwwiz/hdwwiz.c
 * PURPOSE:         ReactOS Add hardware control panel
 * PROGRAMMER:      Hervé Poussineau (hpoussin@reactos.org)
 */

#include <windows.h>
#include <commctrl.h>
#include <setupapi.h>
#include <cpl.h>
#include <tchar.h>
#include <stdio.h>

#include "resource.h"
#include "hdwwiz.h"

LONG APIENTRY Applet(HWND hwnd, UINT uMsg, LONG wParam, LONG lParam);
HINSTANCE hApplet = 0;

/* Applets */
APPLET Applets[] = 
{
	{IDI_CPLICON, IDS_CPLNAME, IDS_CPLDESCRIPTION, Applet}
};

typedef BOOL (*PINSTALL_NEW_DEVICE)(HWND, LPGUID, PDWORD);

LONG APIENTRY
Applet(HWND hwnd, UINT uMsg, LONG wParam, LONG lParam)
{
	HMODULE hNewDev = NULL;
	PINSTALL_NEW_DEVICE InstallNewDevice;
	DWORD Reboot;
	BOOL ret;
	LONG rc;

	UNREFERENCED_PARAMETER(lParam);
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(uMsg);

	hNewDev = LoadLibrary(_T("newdev.dll"));
	if (!hNewDev)
	{
		rc = 1;
		goto cleanup;
	}

	InstallNewDevice = (PINSTALL_NEW_DEVICE)GetProcAddress(hNewDev, (LPCSTR)"InstallNewDevice");
	if (!InstallNewDevice)
	{
		rc = 2;
		goto cleanup;
	}

	ret = InstallNewDevice(hwnd, NULL, &Reboot);
	if (!ret)
	{
		rc = 3;
		goto cleanup;
	}

	if (Reboot != DI_NEEDRESTART && Reboot != DI_NEEDREBOOT)
	{
		/* We're done with installation */
		rc = 0;
		goto cleanup;
	}

	/* We need to reboot */
	if (SetupPromptReboot(NULL, hwnd, FALSE) == -1)
	{
		/* User doesn't want to reboot, or an error occurred */
		rc = 5;
		goto cleanup;
	}

	rc = 0;

cleanup:
	if (hNewDev != NULL)
		FreeLibrary(hNewDev);
	return rc;
}

/* Control Panel Callback */
LONG CALLBACK
CPlApplet(HWND hwndCpl,
	UINT uMsg,
	LPARAM lParam1,
	LPARAM lParam2)
{
	int i = (int)lParam1;

	switch (uMsg)
	{
		case CPL_INIT:
			return TRUE;

		case CPL_GETCOUNT:
			return sizeof(Applets)/sizeof(Applets[0]);

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
			Applets[i].AppletProc(hwndCpl, uMsg, lParam1, lParam2);
			break;
		}
	}
	return FALSE;
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(lpvReserved);
	switch(dwReason)
	{
		case DLL_PROCESS_ATTACH:
			hApplet = hinstDLL;
			break;
	}
	return TRUE;
}
