/* $Id$
 *
 * PROJECT:         ReactOS ODBC Control Panel Applet
 * FILE:            lib/cpl/main/main.c
 * PURPOSE:         applet initialization
 * PROGRAMMER:      Johannes Anderwald
 */

#include "odbccp32.h"

HINSTANCE hApplet = 0;

VOID
InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc)
{
	ZeroMemory(psp, sizeof(PROPSHEETPAGE));
	
	psp->dwSize = sizeof(PROPSHEETPAGE);
	psp->dwFlags = PSP_DEFAULT;
	psp->hInstance = hApplet;
	psp->pszTemplate = MAKEINTRESOURCE(idDlg);
	psp->pfnDlgProc = DlgProc;
}


LONG
APIENTRY
AppletProc(HWND hwnd, UINT uMsg, LONG wParam, LONG lParam)
{
	PROPSHEETPAGE psp[7];
	PROPSHEETHEADER psh;
	TCHAR szBuffer[256];

	UNREFERENCED_PARAMETER(lParam);
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(uMsg);
	UNREFERENCED_PARAMETER(hwnd);

	ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
	psh.dwFlags =  PSH_PROPSHEETPAGE;
	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.hwndParent = NULL;
	psh.hInstance = hApplet;
	psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDC_CPLICON));
	psh.nStartPage = 0;
	psh.ppsp = psp;
	psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);

	if (LoadString(hApplet, IDS_CPLNAME, szBuffer, sizeof(szBuffer) / sizeof(TCHAR)) < 256)
	{
		psh.dwFlags |= PSH_PROPTITLE;
		psh.pszCaption = szBuffer;
	}

	InitPropSheetPage(&psp[0], IDD_USERDSN, UserDSNProc);
	InitPropSheetPage(&psp[1], IDD_SYSTEMDSN, SystemDSNProc);
	InitPropSheetPage(&psp[2], IDD_FILEDSN, FileDSNProc);
	InitPropSheetPage(&psp[3], IDD_DRIVERS, DriversProc);
	InitPropSheetPage(&psp[4], IDD_CONNTRACE, TraceProc);
	InitPropSheetPage(&psp[5], IDD_CONNPOOL, PoolProc);
	InitPropSheetPage(&psp[6], IDD_ABOUT, AboutProc);
	return (LONG)(PropertySheet(&psh) != -1);
}


LONG
CALLBACK
CPlApplet(HWND hwndCpl,
		  UINT uMsg,
		  LPARAM lParam1,
		  LPARAM lParam2)
{
	switch(uMsg)
	{
		case CPL_INIT:
			return TRUE;

		case CPL_GETCOUNT:
			return 1;

		case CPL_INQUIRE:
		{
			CPLINFO *CPlInfo = (CPLINFO*)lParam2;

			CPlInfo->lData = lParam1;
			CPlInfo->idIcon = CPL_ICON;
			CPlInfo->idName = CPL_NAME;
			CPlInfo->idInfo = CPL_INFO;
			break;
		}

		case CPL_DBLCLK:
		{
			AppletProc(hwndCpl, uMsg, lParam1, lParam2);
			break;
		}
	}
	return FALSE;
}


BOOL
WINAPI
DllMain(HINSTANCE hinstDLL,
		DWORD dwReason,
		LPVOID lpReserved)
{
	INITCOMMONCONTROLSEX InitControls;
	UNREFERENCED_PARAMETER(lpReserved);

	switch(dwReason)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:

			InitControls.dwSize = sizeof(INITCOMMONCONTROLSEX);
			InitControls.dwICC = ICC_LISTVIEW_CLASSES | ICC_UPDOWN_CLASS | ICC_BAR_CLASSES;
			InitCommonControlsEx(&InitControls);

			hApplet = hinstDLL;
			break;
  }

  return TRUE;
}

