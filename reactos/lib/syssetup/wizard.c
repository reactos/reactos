/*
 *  ReactOS kernel
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
/* $Id: wizard.c,v 1.10 2004/11/02 15:42:09 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           System setup
 * FILE:              lib/syssetup/wizard.c
 * PROGRAMER:         Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <commctrl.h>

#include <string.h>
#include <tchar.h>
#include <setupapi.h>

#include <syssetup.h>


#include "globals.h"
#include "resource.h"


/* GLOBALS ******************************************************************/

static SETUPDATA SetupData;


/* FUNCTIONS ****************************************************************/

static VOID
CenterWindow(HWND hWnd)
{
  HWND hWndParent;
  RECT rcParent;
  RECT rcWindow;

  hWndParent = GetParent(hWnd);
  if (hWndParent == NULL)
    hWndParent = GetDesktopWindow();

  GetWindowRect(hWndParent, &rcParent);
  GetWindowRect(hWnd, &rcWindow);

  SetWindowPos(hWnd,
	       HWND_TOP,
	       ((rcParent.right - rcParent.left) - (rcWindow.right - rcWindow.left)) / 2,
	       ((rcParent.bottom - rcParent.top) - (rcWindow.bottom - rcWindow.top)) / 2,
	       0,
	       0,
	       SWP_NOSIZE);
}


static HFONT
CreateTitleFont(VOID)
{
  NONCLIENTMETRICS ncm;
  LOGFONT LogFont;
  HDC hdc;
  INT FontSize;
  HFONT hFont;

  ncm.cbSize = sizeof(NONCLIENTMETRICS);
  SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

  LogFont = ncm.lfMessageFont;
  LogFont.lfWeight = FW_BOLD;
  _tcscpy(LogFont.lfFaceName, TEXT("MS Shell Dlg"));

  hdc = GetDC(NULL);
  FontSize = 12;
  LogFont.lfHeight = 0 - GetDeviceCaps (hdc, LOGPIXELSY) * FontSize / 72;
  hFont = CreateFontIndirect(&LogFont);
  ReleaseDC(NULL, hdc);

  return hFont;
}


INT_PTR CALLBACK
WelcomeDlgProc(HWND hwndDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
  switch (uMsg)
    {
      case WM_INITDIALOG:
        {
          PSETUPDATA SetupData;
          HWND hwndControl;
          DWORD dwStyle;

          /* Get pointer to the global setup data */
          SetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;

          hwndControl = GetParent(hwndDlg);

          /* Center the wizard window */
          CenterWindow (hwndControl);

          /* Hide the system menu */
          dwStyle = GetWindowLong(hwndControl, GWL_STYLE);
          SetWindowLong(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);

          /* Hide and disable the 'Cancel' button */
          hwndControl = GetDlgItem(GetParent(hwndDlg), IDCANCEL);
          ShowWindow (hwndControl, SW_HIDE);
          EnableWindow (hwndControl, FALSE);

          /* Set title font */
          SendDlgItemMessage(hwndDlg,
                             IDC_WELCOMETITLE,
                             WM_SETFONT,
                             (WPARAM)SetupData->hTitleFont,
                             (LPARAM)TRUE);
        }
        break;


      case WM_NOTIFY:
        {
          LPNMHDR lpnm = (LPNMHDR)lParam;

          switch (lpnm->code)
            {
              case PSN_SETACTIVE:
                /* Enable the Next button */
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
                break;

              default:
                break;
            }
        }
        break;

      default:
        break;
    }

  return FALSE;
}


INT_PTR CALLBACK
OwnerPageDlgProc(HWND hwndDlg,
                 UINT uMsg,
                 WPARAM wParam,
                 LPARAM lParam)
{
  TCHAR OwnerName[51];
  TCHAR OwnerOrganization[51];
  HKEY hKey;
  LPNMHDR lpnm;

  switch (uMsg)
    {
      case WM_INITDIALOG:
        {
          SendDlgItemMessage(hwndDlg, IDC_OWNERNAME, EM_LIMITTEXT, 50, 0);
          SendDlgItemMessage(hwndDlg, IDC_OWNERORGANIZATION, EM_LIMITTEXT, 50, 0);

          /* Set focus to owner name */
          SetFocus(GetDlgItem(hwndDlg, IDC_OWNERNAME));
        }
        break;


      case WM_NOTIFY:
        {
          lpnm = (LPNMHDR)lParam;

          switch (lpnm->code)
            {
              case PSN_SETACTIVE:
                /* Enable the Back and Next buttons */
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                break;

              case PSN_WIZNEXT:
                OwnerName[0] = 0;
                if (GetDlgItemText(hwndDlg, IDC_OWNERNAME, OwnerName, 50) == 0)
                {
                  MessageBox(hwndDlg,
                             _T("Setup cannot continue until you enter your name."),
                             _T("ReactOS Setup"),
                             MB_ICONERROR | MB_OK);
                  SetWindowLong(hwndDlg, DWL_MSGRESULT, -1);
                  return TRUE;
                }

                OwnerOrganization[0] = 0;
                GetDlgItemText(hwndDlg, IDC_OWNERORGANIZATION, OwnerOrganization, 50);

                RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             _T("Software\\Microsoft\\Windows NT\\CurrentVersion"),
                             0,
                             KEY_ALL_ACCESS,
                             &hKey);
                /* FIXME: check error code */

                RegSetValueEx(hKey,
                              _T("RegisteredOwner"),
                              0,
                              REG_SZ,
                              OwnerName,
                              (_tcslen(OwnerName) + 1) * sizeof(TCHAR));
                /* FIXME: check error code */

                RegSetValueEx(hKey,
                              _T("RegisteredOrganization"),
                              0,
                              REG_SZ,
                              OwnerOrganization,
                              (_tcslen(OwnerOrganization) + 1) * sizeof(TCHAR));
                /* FIXME: check error code */

                RegCloseKey(hKey);
                break;

              default:
                break;
            }
        }
        break;

      default:
        break;
    }

  return FALSE;
}


INT_PTR CALLBACK
ComputerPageDlgProc(HWND hwndDlg,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam)
{
  TCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
  TCHAR Password1[15];
  TCHAR Password2[15];
  DWORD Length;
  LPNMHDR lpnm;

  switch (uMsg)
    {
      case WM_INITDIALOG:
        {
          /* Retrieve current computer name */
          Length = MAX_COMPUTERNAME_LENGTH + 1;
          GetComputerName(ComputerName, &Length);

          /* Display current computer name */
          SetDlgItemText(hwndDlg, IDC_COMPUTERNAME, ComputerName);

          /* Set text limits */
          SendDlgItemMessage(hwndDlg, IDC_COMPUTERNAME, EM_LIMITTEXT, 64, 0);
          SendDlgItemMessage(hwndDlg, IDC_ADMINPASSWORD1, EM_LIMITTEXT, 14, 0);
          SendDlgItemMessage(hwndDlg, IDC_ADMINPASSWORD2, EM_LIMITTEXT, 14, 0);

          /* Set focus to computer name */
          SetFocus(GetDlgItem(hwndDlg, IDC_COMPUTERNAME));
        }
        break;


      case WM_NOTIFY:
        {
          lpnm = (LPNMHDR)lParam;

          switch (lpnm->code)
            {
              case PSN_SETACTIVE:
                /* Enable the Back and Next buttons */
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                break;

              case PSN_WIZNEXT:
                if (GetDlgItemText(hwndDlg, IDC_COMPUTERNAME, ComputerName, 64) == 0)
                {
                  MessageBox(hwndDlg,
                             _T("Setup cannot continue until you enter the name of your computer."),
                             _T("ReactOS Setup"),
                             MB_ICONERROR | MB_OK);
                  SetWindowLong(hwndDlg, DWL_MSGRESULT, -1);
                  return TRUE;
                }

                /* FIXME: check computer name for invalid characters */

                if (!SetComputerName(ComputerName))
                {
                  MessageBox(hwndDlg,
                             _T("Setup failed to set the computer name."),
                             _T("ReactOS Setup"),
                             MB_ICONERROR | MB_OK);
                  SetWindowLong(hwndDlg, DWL_MSGRESULT, -1);
                  return TRUE;
                }

                /* Check admin passwords */
                GetDlgItemText(hwndDlg, IDC_ADMINPASSWORD1, Password1, 15);
                GetDlgItemText(hwndDlg, IDC_ADMINPASSWORD2, Password2, 15);
                if (_tcscmp(Password1, Password2))
                {
                  MessageBox(hwndDlg,
                             _T("The passwords you entered do not match. Please enter "\
                                "the desired password again."),
                             _T("ReactOS Setup"),
                             MB_ICONERROR | MB_OK);
                  SetWindowLong(hwndDlg, DWL_MSGRESULT, -1);
                  return TRUE;
                }

                /* FIXME: check password for invalid characters */

                /* FIXME: Set admin password */
                break;

              default:
                break;
            }
        }
        break;

      default:
        break;
    }

  return FALSE;
}


static VOID
SetKeyboardLayoutName(HWND hwnd)
{
#if 0
  WCHAR szLayoutPath[256];
  WCHAR szLocaleName[32];
  DWORD dwLocaleSize;
  HKEY hKey;

  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
		    L"SYSTEM\\CurrentControlSet\\Control\\NLS\\Locale",
		    0,
		    KEY_ALL_ACCESS,
		    &hKey))
    return;

  dwValueSize = 16 * sizeof(WCHAR);
  if (RegQueryValueExW(hKey,
		       NULL,
		       NULL,
		       NULL,
		       szLocaleName,
		       &dwLocaleSize))
    {
      RegCloseKey(hKey);
      return;
    }

  wcscpy(szLayoutPath,
	 L"SYSTEM\\CurrentControlSet\\Control\\KeyboardLayouts\\"
  wcscat(szLayoutPath,
	 szLocaleName);

  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
		    szLayoutPath,
		    0,
		    KEY_ALL_ACCESS,
		    &hKey))
    return;

  dwValueSize = 32 * sizeof(WCHAR);
  if (RegQueryValueExW(hKey,
		       L"Layout Text",
		       NULL,
		       NULL,
		       szLocaleName,
		       &dwLocaleSize))
    {
      RegCloseKey(hKey);
      return;
    }

  RegCloseKey(hKey);
#endif
}

static VOID
RunLocalePage(VOID)
{
  PROPSHEETPAGE psp;
  PROPSHEETHEADER psh;
  HMODULE hDll;
//  TCHAR Caption[256];

  hDll = LoadLibraryW(L"intl.cpl");

  ZeroMemory(&psp, sizeof(PROPSHEETPAGE));
  psp.dwSize = sizeof(PROPSHEETPAGE);
  psp.dwFlags = PSP_DEFAULT;
  psp.hInstance = hDll;
  psp.pszTemplate = MAKEINTRESOURCE(IDD_LOCALEPAGE);
  psp.pfnDlgProc = GetProcAddress(hDll, "LocalePageProc");


//  LoadString(hDll, IDS_CPLNAME, Caption, sizeof(Caption) / sizeof(TCHAR));

  ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
  psh.dwSize = sizeof(PROPSHEETHEADER);
  psh.dwFlags =  PSH_PROPSHEETPAGE; // | PSH_PROPTITLE;
  psh.hwndParent = NULL;
  psh.hInstance = hDll;
//  psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDC_CPLICON));
//  psh.pszCaption = Caption;
  psh.nPages = 1;
  psh.nStartPage = 0;
  psh.ppsp = &psp;

  PropertySheet(&psh);

  FreeLibrary(hDll);
}


INT_PTR CALLBACK
LocalePageDlgProc(HWND hwndDlg,
                  UINT uMsg,
                  WPARAM wParam,
                  LPARAM lParam)
{
  PSETUPDATA SetupData;

  /* Retrieve pointer to the global setup data */
  SetupData = (PSETUPDATA)GetWindowLongPtr (hwndDlg, GWL_USERDATA);

  switch (uMsg)
    {
      case WM_INITDIALOG:
        {
          /* Save pointer to the global setup data */
          SetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
          SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)SetupData);


          SetKeyboardLayoutName(GetDlgItem(hwndDlg, IDC_LAYOUTTEXT));

        }
        break;

      case WM_COMMAND:
	if (HIWORD(wParam) == BN_CLICKED)
	  {
	    switch (LOWORD(wParam))
	      {
		case IDC_CUSTOMLOCALE:
		  {
		    RunLocalePage();
		  }
		  break;
	      }
	  }
	break;

      case WM_NOTIFY:
        {
          LPNMHDR lpnm = (LPNMHDR)lParam;

          switch (lpnm->code)
            {
              case PSN_SETACTIVE:
                /* Enable the Back and Next buttons */
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                break;

              case PSN_WIZNEXT:
                break;

              default:
                break;
            }
        }
        break;

      default:
        break;
    }

  return FALSE;
}


static VOID
InitTimeZoneList(HWND hwnd)
{
  WCHAR szKeyName[256];
  WCHAR szValue[256];
  DWORD dwIndex;
  DWORD dwNameSize;
  DWORD dwValueSize;
  LONG lError;
  HKEY hZonesKey;
  HKEY hZoneKey;

  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
		    L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones",
		    0,
		    KEY_ALL_ACCESS,
		    &hZonesKey))
    return;

  dwIndex = 0;
  while (TRUE)
    {
      dwNameSize = 256;
      lError = RegEnumKeyExW(hZonesKey,
			     dwIndex,
			     szKeyName,
			     &dwNameSize,
			     NULL,
			     NULL,
			     NULL,
			     NULL);
      if (lError != ERROR_SUCCESS && lError != ERROR_MORE_DATA)
	break;

      if (RegOpenKeyExW(hZonesKey,
			szKeyName,
			0,
			KEY_ALL_ACCESS,
			&hZoneKey))
	break;

      dwValueSize = 256 * sizeof(WCHAR);
      if (RegQueryValueExW(hZoneKey,
			   L"Display",
			   NULL,
			   NULL,
			   (LPBYTE)szValue,
			   &dwValueSize))
	{
	  RegCloseKey(hZoneKey);
	  break;
	}

      SendMessageW(hwnd,
		   CB_ADDSTRING,
		   0,
		   (LPARAM)szValue);

      RegCloseKey(hZoneKey);

      dwIndex++;
    }

  RegCloseKey(hZonesKey);

  SendMessageW(hwnd,
	       CB_SETCURSEL,
	       (WPARAM)0, // index
	       0);

#if 0
      SendMessage(hwnd,
		  CB_ADDSTRING,
		  0,
		  (LPARAM)"Test0");
      SendMessage(hwnd,
		  CB_ADDSTRING,
		  0,
		  (LPARAM)"Test1");
      SendMessage(hwnd,
		  CB_ADDSTRING,
		  0,
		  (LPARAM)"Test2");
      SendMessage(hwnd,
		  CB_ADDSTRING,
		  0,
		  (LPARAM)"Test3");

      SendMessage(hwnd,
		  CB_SETCURSEL,
		  (WPARAM)0, // index
		  0);
#endif
}


static VOID
SetLocalDateTime(HWND hwnd)
{
  SYSTEMTIME Date;
  SYSTEMTIME Time;
  SYSTEMTIME SystemTime;

  if (DateTime_GetSystemTime(GetDlgItem(hwnd, IDC_DATEPICKER), &Date) == GDT_VALID)
    {
      if (DateTime_GetSystemTime(GetDlgItem(hwnd, IDC_TIMEPICKER), &Time) == GDT_VALID)
        {
          SystemTime.wYear = Date.wYear;
          SystemTime.wMonth = Date.wMonth;
          SystemTime.wDayOfWeek = Date.wDayOfWeek;
          SystemTime.wDay = Date.wDay;
          SystemTime.wHour = Time.wHour;
          SystemTime.wMinute = Time.wMinute;
          SystemTime.wSecond = Time.wSecond;
          SystemTime.wMilliseconds = Time.wMilliseconds;

          SetLocalTime(&SystemTime);
        }
    }
}


INT_PTR CALLBACK
DateTimePageDlgProc(HWND hwndDlg,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam)
{
  PSETUPDATA SetupData;

  /* Retrieve pointer to the global setup data */
  SetupData = (PSETUPDATA)GetWindowLongPtr (hwndDlg, GWL_USERDATA);

  switch (uMsg)
    {
      case WM_INITDIALOG:
        {
          /* Save pointer to the global setup data */
          SetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
          SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)SetupData);

          InitTimeZoneList(GetDlgItem(hwndDlg, IDC_TIMEZONELIST));
        }
        break;


      case WM_NOTIFY:
        {
          LPNMHDR lpnm = (LPNMHDR)lParam;

          switch (lpnm->code)
            {
              case PSN_SETACTIVE:
                /* Enable the Back and Next buttons */
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                break;

              case PSN_WIZNEXT:
                {
//                SetTimeZoneInformation();

                  SetLocalDateTime(hwndDlg);
                }
                break;

              default:
                break;
            }
        }
        break;

      default:
        break;
    }

  return FALSE;
}


INT_PTR CALLBACK
ProcessPageDlgProc(HWND hwndDlg,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam)
{
  PSETUPDATA SetupData;

  /* Retrieve pointer to the global setup data */
  SetupData = (PSETUPDATA)GetWindowLongPtr (hwndDlg, GWL_USERDATA);

  switch (uMsg)
    {
      case WM_INITDIALOG:
        {
          /* Save pointer to the global setup data */
          SetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
          SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)SetupData);
        }
        break;

      case WM_TIMER:
         {
           INT Position;
           HWND hWndProgress;

           hWndProgress = GetDlgItem(hwndDlg, IDC_PROCESSPROGRESS);
           Position = SendMessage(hWndProgress, PBM_GETPOS, 0, 0);
           if (Position == 300)
           {
             PropSheet_PressButton(GetParent(hwndDlg), PSBTN_NEXT);
           }
           else
           {
             SendMessage(hWndProgress, PBM_SETPOS, Position + 1, 0);
           }
         }
         return TRUE;

      case WM_NOTIFY:
        {
          LPNMHDR lpnm = (LPNMHDR)lParam;

          switch (lpnm->code)
            {
              case PSN_SETACTIVE:
                /* Disable the Back and Next buttons */
                PropSheet_SetWizButtons(GetParent(hwndDlg), 0);

                SendDlgItemMessage(hwndDlg, IDC_PROCESSPROGRESS, PBM_SETRANGE, 0,
                                   MAKELPARAM(0, 300)); 
                SetTimer(hwndDlg, 0, 50, NULL);
                break;

              case PSN_WIZNEXT:

                /* Enable the Back and Next buttons */
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                break;

              default:
                break;
            }
        }
        break;

      default:
        break;
    }

  return FALSE;
}



INT_PTR CALLBACK
FinishDlgProc(HWND hwndDlg,
              UINT uMsg,
              WPARAM wParam,
              LPARAM lParam)
{

  switch (uMsg)
    {
      case WM_INITDIALOG:
        {
          PSETUPDATA SetupData;

          /* Get pointer to the global setup data */
          SetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;

          /* Set title font */
          SendDlgItemMessage(hwndDlg,
                             IDC_FINISHTITLE,
                             WM_SETFONT,
                             (WPARAM)SetupData->hTitleFont,
                             (LPARAM)TRUE);
        }
        break;

      case WM_NOTIFY:
        {
          LPNMHDR lpnm = (LPNMHDR)lParam;

          switch (lpnm->code)
            {
              case PSN_SETACTIVE:
                /* Enable the correct buttons on for the active page */
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_FINISH);
                break;

              case PSN_WIZBACK:
                /* Handle a Back button click, if necessary */
                break;

              case PSN_WIZFINISH:
                /* Handle a Finish button click, if necessary */
                break;

              default:
                break;
            }
        }
        break;

      default:
        break;
    }

  return FALSE;
}


VOID
InstallWizard(VOID)
{
  PROPSHEETHEADER psh;
  HPROPSHEETPAGE ahpsp[6];
  PROPSHEETPAGE psp;

  /* Clear setup data */
  ZeroMemory(&SetupData, sizeof(SETUPDATA));

  /* Create the Welcome page */
  ZeroMemory (&psp, sizeof(PROPSHEETPAGE));
  psp.dwSize = sizeof(PROPSHEETPAGE);
  psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
  psp.hInstance = hDllInstance;
  psp.lParam = (LPARAM)&SetupData;
  psp.pfnDlgProc = WelcomeDlgProc;
  psp.pszTemplate = MAKEINTRESOURCE(IDD_WELCOMEPAGE);
  ahpsp[0] = CreatePropertySheetPage(&psp);

  /* Create the Owner page */
  psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
  psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_OWNERTITLE);
  psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_OWNERSUBTITLE);
  psp.pszTemplate = MAKEINTRESOURCE(IDD_OWNERPAGE);
  psp.pfnDlgProc = OwnerPageDlgProc;
  ahpsp[1] = CreatePropertySheetPage(&psp);

  /* Create the Computer page */
  psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
  psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_COMPUTERTITLE);
  psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_COMPUTERSUBTITLE);
  psp.pfnDlgProc = ComputerPageDlgProc;
  psp.pszTemplate = MAKEINTRESOURCE(IDD_COMPUTERPAGE);
  ahpsp[2] = CreatePropertySheetPage(&psp);


  /* Create the Locale page */
  psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
  psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_LOCALETITLE);
  psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_LOCALESUBTITLE);
  psp.pfnDlgProc = LocalePageDlgProc;
  psp.pszTemplate = MAKEINTRESOURCE(IDD_LOCALEPAGE);
  ahpsp[3] = CreatePropertySheetPage(&psp);


  /* Create the DateTime page */
  psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
  psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_DATETIMETITLE);
  psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_DATETIMESUBTITLE);
  psp.pfnDlgProc = DateTimePageDlgProc;
  psp.pszTemplate = MAKEINTRESOURCE(IDD_DATETIMEPAGE);
  ahpsp[4] = CreatePropertySheetPage(&psp);


  /* Create the Process page */
  psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
  psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_PROCESSTITLE);
  psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_PROCESSSUBTITLE);
  psp.pfnDlgProc = ProcessPageDlgProc;
  psp.pszTemplate = MAKEINTRESOURCE(IDD_PROCESSPAGE);
  ahpsp[5] = CreatePropertySheetPage(&psp);


  /* Create the Finish page */
  psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
  psp.pfnDlgProc = FinishDlgProc;
  psp.pszTemplate = MAKEINTRESOURCE(IDD_FINISHPAGE);
  ahpsp[6] = CreatePropertySheetPage(&psp);

  /* Create the property sheet */
  psh.dwSize = sizeof(PROPSHEETHEADER);
  psh.dwFlags = PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
  psh.hInstance = hDllInstance;
  psh.hwndParent = NULL;
  psh.nPages = 7;
  psh.nStartPage = 0;
  psh.phpage = ahpsp;
  psh.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK);
  psh.pszbmHeader = MAKEINTRESOURCE(IDB_HEADER);

  /* Create title font */
  SetupData.hTitleFont = CreateTitleFont();

  /* Display the wizard */
  PropertySheet(&psh);

  DeleteObject(SetupData.hTitleFont);
}

/* EOF */
