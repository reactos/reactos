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
/* $Id: wizard.c,v 1.13 2004/11/12 15:42:36 ekohl Exp $
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
  wcscpy(LogFont.lfFaceName, L"MS Shell Dlg");

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
  WCHAR OwnerName[51];
  WCHAR OwnerOrganization[51];
  HKEY hKey;
  LPNMHDR lpnm;

  switch (uMsg)
    {
      case WM_INITDIALOG:
        {
          SendDlgItemMessageW(hwndDlg, IDC_OWNERNAME, EM_LIMITTEXT, 50, 0);
          SendDlgItemMessageW(hwndDlg, IDC_OWNERORGANIZATION, EM_LIMITTEXT, 50, 0);

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
                  MessageBoxW(hwndDlg,
                              L"Setup cannot continue until you enter your name.",
                              L"ReactOS Setup",
                             MB_ICONERROR | MB_OK);
                  SetWindowLong(hwndDlg, DWL_MSGRESULT, -1);
                  return TRUE;
                }

                OwnerOrganization[0] = 0;
                GetDlgItemTextW(hwndDlg, IDC_OWNERORGANIZATION, OwnerOrganization, 50);

                RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                              L"Software\\Microsoft\\Windows NT\\CurrentVersion",
                              0,
                              KEY_ALL_ACCESS,
                              &hKey);
                /* FIXME: check error code */

                RegSetValueExW(hKey,
                               L"RegisteredOwner",
                               0,
                               REG_SZ,
                               (LPBYTE)OwnerName,
                               (wcslen(OwnerName) + 1) * sizeof(WCHAR));
                /* FIXME: check error code */

                RegSetValueExW(hKey,
                               L"RegisteredOrganization",
                               0,
                               REG_SZ,
                               (LPBYTE)OwnerOrganization,
                               (wcslen(OwnerOrganization) + 1) * sizeof(WCHAR));
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
  WCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
  WCHAR Password1[15];
  WCHAR Password2[15];
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
          SetDlgItemTextW(hwndDlg, IDC_COMPUTERNAME, ComputerName);

          /* Set text limits */
          SendDlgItemMessageW(hwndDlg, IDC_COMPUTERNAME, EM_LIMITTEXT, 64, 0);
          SendDlgItemMessageW(hwndDlg, IDC_ADMINPASSWORD1, EM_LIMITTEXT, 14, 0);
          SendDlgItemMessageW(hwndDlg, IDC_ADMINPASSWORD2, EM_LIMITTEXT, 14, 0);

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
                  MessageBoxW(hwndDlg,
                              L"Setup cannot continue until you enter the name of your computer.",
                              L"ReactOS Setup",
                              MB_ICONERROR | MB_OK);
                  SetWindowLong(hwndDlg, DWL_MSGRESULT, -1);
                  return TRUE;
                }

                /* FIXME: check computer name for invalid characters */

                if (!SetComputerName(ComputerName))
                {
                  MessageBoxW(hwndDlg,
                              L"Setup failed to set the computer name.",
                              L"ReactOS Setup",
                              MB_ICONERROR | MB_OK);
                  SetWindowLong(hwndDlg, DWL_MSGRESULT, -1);
                  return TRUE;
                }

                /* Check admin passwords */
                GetDlgItemTextW(hwndDlg, IDC_ADMINPASSWORD1, Password1, 15);
                GetDlgItemTextW(hwndDlg, IDC_ADMINPASSWORD2, Password2, 15);
                if (wcscmp(Password1, Password2))
                {
                  MessageBoxW(hwndDlg,
                              L"The passwords you entered do not match. Please enter "\
                               "the desired password again.",
                              L"ReactOS Setup",
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
RunInputLocalePage(HWND hwnd)
{
  PROPSHEETPAGEW psp;
  PROPSHEETHEADERW psh;
  HMODULE hDll;
//  WCHAR Caption[256];

  hDll = LoadLibraryW(L"intl.cpl");
  if (hDll == NULL)
    return;

  ZeroMemory(&psp, sizeof(PROPSHEETPAGEW));
  psp.dwSize = sizeof(PROPSHEETPAGEW);
  psp.dwFlags = PSP_DEFAULT;
  psp.hInstance = hDll;
  psp.pszTemplate = MAKEINTRESOURCEW(105); /* IDD_LOCALEPAGE from intl.cpl */
  psp.pfnDlgProc = GetProcAddress(hDll, "LocalePageProc");

//  LoadString(hDll, IDS_CPLNAME, Caption, sizeof(Caption) / sizeof(TCHAR));

  ZeroMemory(&psh, sizeof(PROPSHEETHEADERW));
  psh.dwSize = sizeof(PROPSHEETHEADERW);
  psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_PROPTITLE;
//  psh.hwndParent = hwnd;
//  psh.hInstance = hDll;
//  psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDC_CPLICON));
  psh.pszCaption = L"Title"; //Caption;
  psh.nPages = 1;
  psh.nStartPage = 0;
  psh.ppsp = &psp;

  PropertySheetW(&psh);

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
		    RunInputLocalePage(hwndDlg);
		    /* FIXME: Update input locale name */
		  }
		  break;

		case IDC_CUSTOMLAYOUT:
		  {
//		    RunKeyboardLayoutControlPanel(hwndDlg);
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


static PTIMEZONE_ENTRY
GetLargerTimeZoneEntry(PSETUPDATA SetupData, DWORD Index)
{
  PTIMEZONE_ENTRY Entry;

  Entry = SetupData->TimeZoneListHead;
  while (Entry != NULL)
    {
      if (Entry->Index >= Index)
	return Entry;

      Entry = Entry->Next;
    }

  return NULL;
}


static VOID
CreateTimeZoneList(PSETUPDATA SetupData)
{
  WCHAR szKeyName[256];
  DWORD dwIndex;
  DWORD dwNameSize;
  DWORD dwValueSize;
  LONG lError;
  HKEY hZonesKey;
  HKEY hZoneKey;

  PTIMEZONE_ENTRY Entry;
  PTIMEZONE_ENTRY Current;

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

      Entry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TIMEZONE_ENTRY));
      if (Entry == NULL)
	{
	  RegCloseKey(hZoneKey);
	  break;
	}

      dwValueSize = 64 * sizeof(WCHAR);
      if (RegQueryValueExW(hZoneKey,
			   L"Display",
			   NULL,
			   NULL,
			   (LPBYTE)&Entry->Description,
			   &dwValueSize))
	{
	  RegCloseKey(hZoneKey);
	  break;
	}

      dwValueSize = 32 * sizeof(WCHAR);
      if (RegQueryValueExW(hZoneKey,
			   L"Std",
			   NULL,
			   NULL,
			   (LPBYTE)&Entry->StandardName,
			   &dwValueSize))
	{
	  RegCloseKey(hZoneKey);
	  break;
	}

      dwValueSize = 32 * sizeof(WCHAR);
      if (RegQueryValueExW(hZoneKey,
			   L"Dlt",
			   NULL,
			   NULL,
			   (LPBYTE)&Entry->DaylightName,
			   &dwValueSize))
	{
	  RegCloseKey(hZoneKey);
	  break;
	}

      dwValueSize = sizeof(DWORD);
      if (RegQueryValueExW(hZoneKey,
			   L"Index",
			   NULL,
			   NULL,
			   (LPBYTE)&Entry->Index,
			   &dwValueSize))
	{
	  RegCloseKey(hZoneKey);
	  break;
	}

      dwValueSize = sizeof(TZ_INFO);
      if (RegQueryValueExW(hZoneKey,
			   L"TZI",
			   NULL,
			   NULL,
			   (LPBYTE)&Entry->TimezoneInfo,
			   &dwValueSize))
	{
	  RegCloseKey(hZoneKey);
	  break;
	}

      RegCloseKey(hZoneKey);

      if (SetupData->TimeZoneListHead == NULL &&
	  SetupData->TimeZoneListTail == NULL)
	{
	  Entry->Prev = NULL;
	  Entry->Next = NULL;
	  SetupData->TimeZoneListHead = Entry;
	  SetupData->TimeZoneListTail = Entry;
	}
      else
	{
	  Current = GetLargerTimeZoneEntry(SetupData, Entry->Index);
	  if (Current != NULL)
	    {
	      if (Current == SetupData->TimeZoneListHead)
		{
		  /* Prepend to head */
		  Entry->Prev = NULL;
		  Entry->Next = SetupData->TimeZoneListHead;
		  SetupData->TimeZoneListHead->Prev = Entry;
		  SetupData->TimeZoneListHead = Entry;
		}
	      else
		{
		  /* Insert before current */
		  Entry->Prev = Current->Prev;
		  Entry->Next = Current;
		  Current->Prev->Next = Entry;
		  Current->Prev = Entry;
		}
	    }
	  else
	    {
	      /* Append to tail */
	      Entry->Prev = SetupData->TimeZoneListTail;
	      Entry->Next = NULL;
	      SetupData->TimeZoneListTail->Next = Entry;
	      SetupData->TimeZoneListTail = Entry;
	    }
	}

      dwIndex++;
    }

  RegCloseKey(hZonesKey);
}


static VOID
DestroyTimeZoneList(PSETUPDATA SetupData)
{
  PTIMEZONE_ENTRY Entry;

  while (SetupData->TimeZoneListHead != NULL)
    {
      Entry = SetupData->TimeZoneListHead;

      SetupData->TimeZoneListHead = Entry->Next;
      if (SetupData->TimeZoneListHead != NULL)
	{
	  SetupData->TimeZoneListHead->Prev = NULL;
	}

      HeapFree(GetProcessHeap(), 0, Entry);
    }

  SetupData->TimeZoneListTail = NULL;
}


static VOID
ShowTimeZoneList(HWND hwnd, PSETUPDATA SetupData)
{
  PTIMEZONE_ENTRY Entry;

  Entry = SetupData->TimeZoneListHead;
  while (Entry != NULL)
    {
      SendMessageW(hwnd,
		   CB_ADDSTRING,
		   0,
		   (LPARAM)Entry->Description);


      Entry = Entry->Next;
    }

  SendMessageW(hwnd,
	       CB_SETCURSEL,
	       (WPARAM)0, // index
	       0);
}


static VOID
SetLocalTimeZone(HWND hwnd, PSETUPDATA SetupData)
{
  TIME_ZONE_INFORMATION TimeZoneInformation;
  PTIMEZONE_ENTRY Entry;
  DWORD dwIndex;
  DWORD i;

  dwIndex = SendMessage(hwnd,
			CB_GETCURSEL,
			0,
			0);

  i = 0;
  Entry = SetupData->TimeZoneListHead;
  while (i < dwIndex)
    {
      if (Entry == NULL)
	return;

      i++;
      Entry = Entry->Next;
    }

  wcscpy(TimeZoneInformation.StandardName,
	 Entry->StandardName);
  wcscpy(TimeZoneInformation.DaylightName,
	 Entry->DaylightName);

  TimeZoneInformation.Bias = Entry->TimezoneInfo.Bias;
  TimeZoneInformation.StandardBias = Entry->TimezoneInfo.StandardBias;
  TimeZoneInformation.DaylightBias = Entry->TimezoneInfo.DaylightBias;

  memcpy(&TimeZoneInformation.StandardDate,
	 &Entry->TimezoneInfo.StandardDate,
	 sizeof(SYSTEMTIME));
  memcpy(&TimeZoneInformation.DaylightDate,
	 &Entry->TimezoneInfo.DaylightDate,
	 sizeof(SYSTEMTIME));

  /* Set time zone information */
  SetTimeZoneInformation(&TimeZoneInformation);
}


static BOOL
GetLocalSystemTime(HWND hwnd, PSETUPDATA SetupData)
{
  SYSTEMTIME Date;
  SYSTEMTIME Time;

  if (DateTime_GetSystemTime(GetDlgItem(hwnd, IDC_DATEPICKER), &Date) != GDT_VALID)
    {
      return FALSE;
    }

  if (DateTime_GetSystemTime(GetDlgItem(hwnd, IDC_TIMEPICKER), &Time) != GDT_VALID)
    {
      return FALSE;
    }

  SetupData->SystemTime.wYear = Date.wYear;
  SetupData->SystemTime.wMonth = Date.wMonth;
  SetupData->SystemTime.wDayOfWeek = Date.wDayOfWeek;
  SetupData->SystemTime.wDay = Date.wDay;
  SetupData->SystemTime.wHour = Time.wHour;
  SetupData->SystemTime.wMinute = Time.wMinute;
  SetupData->SystemTime.wSecond = Time.wSecond;
  SetupData->SystemTime.wMilliseconds = Time.wMilliseconds;

  return TRUE;
}


static VOID
SetAutoDaylightInfo(HWND hwnd)
{
  HKEY hKey;
  DWORD dwValue = 1;

  if (SendMessage(hwnd, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
    {
      if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
			L"SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation",
			0,
			KEY_SET_VALUE,
			&hKey))
	  return;

      RegSetValueExW(hKey,
		     L"DisableAutoDaylightTimeSet",
		     0,
		     REG_DWORD,
		     (LPBYTE)&dwValue,
		     sizeof(DWORD));
      RegCloseKey(hKey);
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

          CreateTimeZoneList(SetupData);

          ShowTimeZoneList(GetDlgItem(hwndDlg, IDC_TIMEZONELIST),
                           SetupData);

          SendDlgItemMessage(hwndDlg, IDC_AUTODAYLIGHT, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
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
                  GetLocalSystemTime(hwndDlg, SetupData);
                  SetLocalTimeZone(GetDlgItem(hwndDlg, IDC_TIMEZONELIST),
                                   SetupData);
                  SetAutoDaylightInfo(GetDlgItem(hwndDlg, IDC_AUTODAYLIGHT));
                  SetLocalTime(&SetupData->SystemTime);
                }
                break;

              default:
                break;
            }
        }
        break;

      case WM_DESTROY:
        DestroyTimeZoneList(SetupData);
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
