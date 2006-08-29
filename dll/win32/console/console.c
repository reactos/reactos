/* $Id$
 *
 * PROJECT:         ReactOS Console Configuration DLL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/console/console.c
 * PURPOSE:         initialization of DLL
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@student.tugraz.at)
 */

#include "console.h"

#define NUM_APPLETS	(1)
#define WM_SETCONSOLE (WM_USER+10)


LONG APIENTRY InitApplet(HWND hwnd, UINT uMsg, LONG wParam, LONG lParam);
INT_PTR CALLBACK OptionsProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK FontProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK LayoutProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ColorsProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

HINSTANCE hApplet = 0;

/* Applets */
APPLET Applets[NUM_APPLETS] = 
{
  {IDC_CPLICON, IDS_CPLNAME, IDS_CPLDESCRIPTION, InitApplet}
};

static COLORREF s_Colors[] =
{
	RGB(0, 0, 0),
	RGB(0, 0, 128),
	RGB(0, 128, 0),
	RGB(0, 128, 128),
	RGB(128, 0, 0),
	RGB(128, 0, 128),
	RGB(128, 128, 0),
	RGB(192, 192, 192),
	RGB(128, 128, 128),
	RGB(0, 0, 255),
	RGB(0, 255, 0),
	RGB(0, 255, 255),
	RGB(255, 0, 0),
	RGB(255, 0, 255),
	RGB(255, 255, 0),
	RGB(255, 255, 255)
};

static void
InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc, LPARAM lParam)
{
  ZeroMemory(psp, sizeof(PROPSHEETPAGE));
  psp->dwSize = sizeof(PROPSHEETPAGE);
  psp->dwFlags = PSP_DEFAULT;
  psp->hInstance = hApplet;
  psp->pszTemplate = MAKEINTRESOURCE(idDlg);
  psp->pfnDlgProc = DlgProc;
  psp->lParam = lParam;
}

PConsoleInfo
InitConsoleInfo()
{
	PConsoleInfo pConInfo;
	STARTUPINFO StartupInfo;
	TCHAR * ptr;
	SIZE_T length;

	pConInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ConsoleInfo));
	if (!pConInfo)
	{
		return NULL;
	}

	/* initialize struct */
	pConInfo->InsertMode = TRUE;
	pConInfo->HistoryBufferSize = 50;
	pConInfo->NumberOfHistoryBuffers = 5;
    pConInfo->ScreenText = RGB(192, 192, 192); 
	pConInfo->ScreenBackground = RGB(0, 0, 0); 
	pConInfo->PopupText = RGB(128, 0, 128); 
	pConInfo->PopupBackground = RGB(255, 255, 255); 
	pConInfo->WindowSize = (DWORD)MAKELONG(80, 25);
	pConInfo->WindowPosition = UINT_MAX;
	pConInfo->ScreenBuffer = MAKELONG(80, 300);
	pConInfo->UseRasterFonts = TRUE;
	pConInfo->FontSize = (DWORD)MAKELONG(8, 12);
	pConInfo->FontWeight = FALSE;
	memcpy(pConInfo->Colors, s_Colors, sizeof(s_Colors));

	GetModuleFileName(NULL, pConInfo->szProcessName, MAX_PATH);
	GetStartupInfo(&StartupInfo);



	if ( StartupInfo.lpTitle )
	{
		if ( _tcslen(StartupInfo.lpTitle) )
		{
			if ( !GetWindowsDirectory(pConInfo->szProcessName, MAX_PATH) )
			{
				HeapFree(GetProcessHeap(), 0, pConInfo);
				return FALSE;
			}
			length = _tcslen(pConInfo->szProcessName);
			if ( !_tcsncmp(pConInfo->szProcessName, StartupInfo.lpTitle, length) )
			{
				// Windows XP SP2 uses unexpanded environment vars to get path
				// i.e. c:\windows\system32\cmd.exe
				// becomes
				// %SystemRoot%_system32_cmd.exe		

				_tcscpy(pConInfo->szProcessName, _T("%SystemRoot%"));
				_tcsncpy(&pConInfo->szProcessName[12], &StartupInfo.lpTitle[length], _tcslen(StartupInfo.lpTitle) - length + 1);
			
				ptr = &pConInfo->szProcessName[12];
				while( (ptr = _tcsstr(ptr, _T("\\"))) )
					ptr[0] = _T('_');
			}
		}
		else
		{
			_tcscpy(pConInfo->szProcessName, _T("Console"));
		}
	}
	else
	{
		_tcscpy(pConInfo->szProcessName, _T("Console"));
	}
	return pConInfo;
}

INT_PTR 
CALLBACK
ApplyProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
	HWND hDlgCtrl;

	UNREFERENCED_PARAMETER(lParam);

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
#if 0
			hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_APPLY_CURRENT);
#else
			hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_APPLY_ALL);
#endif
			SendMessage(hDlgCtrl, BM_SETCHECK, BST_CHECKED, 0);
			return TRUE;
		}
		case WM_COMMAND:
		{
			if (LOWORD(wParam) == IDOK)
			{
				hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_APPLY_CURRENT);
				if ( SendMessage(hDlgCtrl, BM_GETCHECK, 0, 0) == BST_CHECKED )
					EndDialog(hwndDlg, IDC_RADIO_APPLY_CURRENT);
				else
					EndDialog(hwndDlg, IDC_RADIO_APPLY_ALL);
			}
			else if (LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hwndDlg, IDCANCEL);
			}
			break;
		}
		default:
			break;
	}
	return FALSE;

}

void
ApplyConsoleInfo(HWND hwndDlg, PConsoleInfo pConInfo)
{
	INT_PTR res = 0;

	res = DialogBox(hApplet, MAKEINTRESOURCE(IDD_APPLYOPTIONS), hwndDlg, ApplyProc);

	if (res == IDCANCEL)
	{
		/* dont destroy when user presses cancel */
		SetWindowLong(hwndDlg, DWL_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
	}
	else if ( res == IDC_RADIO_APPLY_ALL )
	{
		/* apply options */
		WriteConsoleOptions(pConInfo);
		pConInfo->AppliedConfig = TRUE;
		SetWindowLong(hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR);
	}
	else if ( res == IDC_RADIO_APPLY_CURRENT )
	{
		/*
		 * TODO:
		 * exchange info in some private way with win32csr
		 */
		pConInfo->AppliedConfig = TRUE;
		SetWindowLong(hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR);
	}
}

/* First Applet */
LONG APIENTRY
InitApplet(HWND hwnd, UINT uMsg, LONG wParam, LONG lParam)	
{
	PROPSHEETPAGE psp[4];
	PROPSHEETHEADER psh;
	INT i=0;
	PConsoleInfo pConInfo;

	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(uMsg);
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);

	pConInfo = InitConsoleInfo();

	ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_PROPTITLE | PSH_NOAPPLYNOW;
	psh.hwndParent = NULL;
	psh.hInstance = hApplet;
	psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDC_CPLICON));
	psh.pszCaption = 0;
	psh.nPages = 4;
	psh.nStartPage = 0;
	psh.ppsp = psp;
	psh.pszCaption = pConInfo->szProcessName;
  
	InitPropSheetPage(&psp[i++], IDD_PROPPAGEOPTIONS, (DLGPROC) OptionsProc, (LPARAM)pConInfo);
	InitPropSheetPage(&psp[i++], IDD_PROPPAGEFONT, (DLGPROC) FontProc, (LPARAM)pConInfo);
	InitPropSheetPage(&psp[i++], IDD_PROPPAGELAYOUT, (DLGPROC) LayoutProc, (LPARAM)pConInfo);
	InitPropSheetPage(&psp[i++], IDD_PROPPAGECOLORS, (DLGPROC) ColorsProc, (LPARAM)pConInfo);

	return (PropertySheet(&psh) != -1);
}

/* Control Panel Callback */
LONG CALLBACK
CPlApplet(
	HWND hwndCPl,
	UINT uMsg,
	LPARAM lParam1,
	LPARAM lParam2)
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
      CPlInfo->idIcon = Applets[i].idIcon;
      CPlInfo->idName = Applets[i].idName;
      CPlInfo->idInfo = Applets[i].idDescription;
      break;
    }
    case CPL_DBLCLK:
    {
      Applets[i].AppletProc(hwndCPl, uMsg, lParam1, lParam2);
      break;
    }
  }
  return FALSE;
}


INT
WINAPI
DllMain(
	HINSTANCE hinstDLL,
	DWORD     dwReason,
	LPVOID    lpvReserved)
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

