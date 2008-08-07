/*
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
AllocConsoleInfo()
{
	PConsoleInfo pConInfo;

	pConInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ConsoleInfo));

	return pConInfo;
}

void
InitConsoleDefaults(PConsoleInfo pConInfo)
{
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
	pConInfo->FontWeight = FW_NORMAL;
	memcpy(pConInfo->Colors, s_Colors, sizeof(s_Colors));
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
			hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_APPLY_CURRENT);
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
		pConInfo->AppliedConfig = TRUE;
		SetWindowLong(hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR);
		SendMessage(pConInfo->hConsoleWindow, PM_APPLY_CONSOLE_INFO, (WPARAM)pConInfo, (LPARAM)TRUE);
	}
	else if ( res == IDC_RADIO_APPLY_CURRENT )
	{
		pConInfo->AppliedConfig = TRUE;
		SetWindowLong(hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR);
		SendMessage(pConInfo->hConsoleWindow, PM_APPLY_CONSOLE_INFO, (WPARAM)pConInfo, (LPARAM)TRUE);
	}
}

/* First Applet */
LONG APIENTRY
InitApplet(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam)
{
	PROPSHEETPAGE psp[4];
	PROPSHEETHEADER psh;
	INT i=0;
	PConsoleInfo pConInfo;
	WCHAR szTitle[100];
	PConsoleInfo pSharedInfo = (PConsoleInfo)wParam;

	UNREFERENCED_PARAMETER(uMsg);

	/*
	 * console.dll shares information with win32csr with wParam, lParam
	 *
	 * wParam is a pointer to an ConsoleInfo struct
	 * lParam is a boolean parameter which specifies wheter defaults should be shown
	 */

	pConInfo = AllocConsoleInfo();
	if (!pConInfo)
	{
		return 0;
	}

	if (lParam)
	{
		/* use defaults */
		InitConsoleDefaults(pConInfo);
	}
	else
	{
		if (IsBadReadPtr((const void *)pSharedInfo, sizeof(ConsoleInfo)))
		{
			/* use defaults */
			InitConsoleDefaults(pConInfo);
		}
		else
		{
			pConInfo->InsertMode = pSharedInfo->InsertMode;
			pConInfo->HistoryBufferSize = pSharedInfo->HistoryBufferSize;
			pConInfo->NumberOfHistoryBuffers = pSharedInfo->NumberOfHistoryBuffers;
			pConInfo->ScreenText = pSharedInfo->ScreenText;
			pConInfo->ScreenBackground =  pSharedInfo->ScreenBackground;
			pConInfo->PopupText = pSharedInfo->PopupText;
			pConInfo->PopupBackground = pSharedInfo->PopupBackground;
			pConInfo->WindowSize = pSharedInfo->WindowSize;
			pConInfo->WindowPosition = pSharedInfo->WindowPosition;
			pConInfo->ScreenBuffer = pSharedInfo->ScreenBuffer;
			pConInfo->UseRasterFonts = pSharedInfo->UseRasterFonts;
			pConInfo->FontSize = pSharedInfo->FontSize;
			pConInfo->FontWeight = pSharedInfo->FontWeight;
			memcpy(pConInfo->Colors, pSharedInfo->Colors, sizeof(s_Colors));
		}
	}

	/* console window -> is notified on a property change event */
	pConInfo->hConsoleWindow = hwnd;

	ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
	if(_tcslen(pConInfo->szProcessName))
	{
		psh.dwFlags |= PSH_PROPTITLE;
		psh.pszCaption = pConInfo->szProcessName;
	}
	else
	{
		if (!GetConsoleTitleW(szTitle, sizeof(szTitle)/sizeof(WCHAR)))
		{
			_tcscpy(szTitle, _T("cmd.exe"));
		}
		szTitle[(sizeof(szTitle)/sizeof(WCHAR))-1] = _T('\0');
		psh.pszCaption = szTitle;
	}

	psh.hwndParent = hwnd;
	psh.hInstance = hApplet;
	psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDC_CPLICON));
	psh.nPages = 4;
	psh.nStartPage = 0;
	psh.ppsp = psp;

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
      CPlInfo->idIcon = Applets[0].idIcon;
      CPlInfo->idName = Applets[0].idName;
      CPlInfo->idInfo = Applets[0].idDescription;
      break;
    }
    case CPL_DBLCLK:
    {
      InitApplet(hwndCPl, uMsg, lParam1, lParam2);
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

