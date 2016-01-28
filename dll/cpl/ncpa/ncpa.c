/*
 * PROJECT:         ReactOS Network Control Panel
 * FILE:            dll/cpl/ncpa/ncpa.c
 * PURPOSE:         ReactOS Network Control Panel
 * PROGRAMMER:      Gero Kuehn (reactos.filter@gkware.com)
 * UPDATE HISTORY:
 *      07-18-2004  Created
 */

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <shellapi.h>
#include <cpl.h>

LONG CALLBACK
DisplayApplet(VOID)
{
	WCHAR szParameters[] = L"/n,::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{7007ACC7-3202-11D1-AAD2-00805FC1270E}";

	return (INT_PTR) ShellExecuteW(NULL, L"open", L"explorer.exe", szParameters, NULL, SW_SHOWDEFAULT) > 32;
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
			return 1;
		}
	case CPL_DBLCLK:
		{
			DisplayApplet();
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
		break;
	}

	return TRUE;
}
