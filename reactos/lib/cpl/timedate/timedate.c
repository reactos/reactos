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
/* $Id$
 *
 * PROJECT:         ReactOS Timedate Control Panel
 * FILE:            lib/cpl/timedate/timedate.c
 * PURPOSE:         ReactOS Timedate Control Panel
 * PROGRAMMER:      Eric Kohl
 */

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>

#include "resource.h"
#include "timedate.h"


typedef struct _TZ_INFO
{
  LONG Bias;
  LONG StandardBias;
  LONG DaylightBias;
  SYSTEMTIME StandardDate;
  SYSTEMTIME DaylightDate;
} TZ_INFO, *PTZ_INFO;

typedef struct _TIMEZONE_ENTRY
{
  struct _TIMEZONE_ENTRY *Prev;
  struct _TIMEZONE_ENTRY *Next;
  WCHAR Description[64];   /* 'Display' */
  WCHAR StandardName[32];  /* 'Std' */
  WCHAR DaylightName[32];  /* 'Dlt' */
  TZ_INFO TimezoneInfo;    /* 'TZI' */
  ULONG Index;             /* 'Index ' */
} TIMEZONE_ENTRY, *PTIMEZONE_ENTRY;


#define NUM_APPLETS	(1)

LONG APIENTRY
Applet(HWND hwnd, UINT uMsg, LONG wParam, LONG lParam);


HINSTANCE hApplet = 0;

PTIMEZONE_ENTRY TimeZoneListHead = NULL;
PTIMEZONE_ENTRY TimeZoneListTail = NULL;


/* Applets */
APPLET Applets[NUM_APPLETS] = 
{
  {IDC_CPLICON, IDS_CPLNAME, IDS_CPLDESCRIPTION, Applet}
};


static VOID
SetLocalSystemTime(HWND hwnd)
{
  SYSTEMTIME Date;
  SYSTEMTIME Time;

  if (DateTime_GetSystemTime(GetDlgItem(hwnd, IDC_DATEPICKER), &Date) != GDT_VALID)
    {
      return;
    }

  if (DateTime_GetSystemTime(GetDlgItem(hwnd, IDC_TIMEPICKER), &Time) != GDT_VALID)
    {
      return;
    }

  Time.wYear = Date.wYear;
  Time.wMonth = Date.wMonth;
  Time.wDayOfWeek = Date.wDayOfWeek;
  Time.wDay = Date.wDay;

  SetLocalTime(&Time);
}


static VOID
SetTimeZoneName(HWND hwnd)
{
  TIME_ZONE_INFORMATION TimeZoneInfo;
  WCHAR TimeZoneString[128];
  WCHAR TimeZoneText[128];
  WCHAR TimeZoneName[128];
  DWORD TimeZoneId;

  TimeZoneId = GetTimeZoneInformation(&TimeZoneInfo);

  LoadString(hApplet, IDS_TIMEZONETEXT, TimeZoneText, 128);

  switch (TimeZoneId)
  {
    case TIME_ZONE_ID_STANDARD:
      wcscpy(TimeZoneName, TimeZoneInfo.StandardName);
      break;

    case TIME_ZONE_ID_DAYLIGHT:
      wcscpy(TimeZoneName, TimeZoneInfo.DaylightName);
      break;

    case TIME_ZONE_ID_UNKNOWN:
      LoadString(hApplet, IDS_TIMEZONEUNKNOWN, TimeZoneName, 128);
      break;

    case TIME_ZONE_ID_INVALID:
    default:
      LoadString(hApplet, IDS_TIMEZONEINVALID, TimeZoneName, 128);
      break;
  }

  wsprintf(TimeZoneString, TimeZoneText, TimeZoneName);
  SendDlgItemMessageW(hwnd, IDC_TIMEZONE, WM_SETTEXT, 0, (LPARAM)TimeZoneString);
}


/* Property page dialog callback */
INT_PTR CALLBACK
DateTimePageProc(HWND hwndDlg,
		 UINT uMsg,
		 WPARAM wParam,
		 LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_NOTIFY:
      {
          LPNMHDR lpnm = (LPNMHDR)lParam;

          switch (lpnm->code)
            {
              case DTN_DATETIMECHANGE:
              case MCN_SELECT:
                /* Enable the 'Apply' button */
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                break;

              case PSN_SETACTIVE:
                SetTimeZoneName(hwndDlg);
                return 0;

              case PSN_APPLY:
                SetLocalSystemTime(hwndDlg);
                SetWindowLong(hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR);
                return TRUE;

              default:
                break;
            }
      }
      break;
  }

  return FALSE;
}


static PTIMEZONE_ENTRY
GetLargerTimeZoneEntry(DWORD Index)
{
  PTIMEZONE_ENTRY Entry;

  Entry = TimeZoneListHead;
  while (Entry != NULL)
    {
      if (Entry->Index >= Index)
	return Entry;

      Entry = Entry->Next;
    }

  return NULL;
}


static VOID
CreateTimeZoneList(VOID)
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

      if (TimeZoneListHead == NULL &&
	  TimeZoneListTail == NULL)
	{
	  Entry->Prev = NULL;
	  Entry->Next = NULL;
	  TimeZoneListHead = Entry;
	  TimeZoneListTail = Entry;
	}
      else
	{
	  Current = GetLargerTimeZoneEntry(Entry->Index);
	  if (Current != NULL)
	    {
	      if (Current == TimeZoneListHead)
		{
		  /* Prepend to head */
		  Entry->Prev = NULL;
		  Entry->Next = TimeZoneListHead;
		  TimeZoneListHead->Prev = Entry;
		  TimeZoneListHead = Entry;
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
	      Entry->Prev = TimeZoneListTail;
	      Entry->Next = NULL;
	      TimeZoneListTail->Next = Entry;
	      TimeZoneListTail = Entry;
	    }
	}

      dwIndex++;
    }

  RegCloseKey(hZonesKey);
}


static VOID
DestroyTimeZoneList(VOID)
{
  PTIMEZONE_ENTRY Entry;

  while (TimeZoneListHead != NULL)
    {
      Entry = TimeZoneListHead;

      TimeZoneListHead = Entry->Next;
      if (TimeZoneListHead != NULL)
	{
	  TimeZoneListHead->Prev = NULL;
	}

      HeapFree(GetProcessHeap(), 0, Entry);
    }

  TimeZoneListTail = NULL;
}


static VOID
ShowTimeZoneList(HWND hwnd)
{
  TIME_ZONE_INFORMATION TimeZoneInfo;
  PTIMEZONE_ENTRY Entry;
  DWORD dwIndex;
  DWORD i;

  GetTimeZoneInformation(&TimeZoneInfo);

  dwIndex = 0;
  i = 0;
  Entry = TimeZoneListHead;
  while (Entry != NULL)
    {
      SendMessageW(hwnd,
		   CB_ADDSTRING,
		   0,
		   (LPARAM)Entry->Description);

      if (!wcscmp(Entry->StandardName, TimeZoneInfo.StandardName))
	dwIndex = i;

      i++;
      Entry = Entry->Next;
    }

  SendMessageW(hwnd,
	       CB_SETCURSEL,
	       (WPARAM)dwIndex,
	       0);
}


static VOID
SetLocalTimeZone(HWND hwnd)
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
  Entry = TimeZoneListHead;
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


static VOID
GetAutoDaylightInfo(HWND hwnd)
{
  HKEY hKey;

  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
		    L"SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation",
		    0,
		    KEY_SET_VALUE,
		    &hKey))
    return;

  if (RegQueryValueExW(hKey,
		       L"DisableAutoDaylightTimeSet",
		       NULL,
		       NULL,
		       NULL,
		       NULL))
    {
      SendMessage(hwnd, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
    }
  else
    {
      SendMessage(hwnd, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
    }

  RegCloseKey(hKey);
}


static VOID
SetAutoDaylightInfo(HWND hwnd)
{
  HKEY hKey;
  DWORD dwValue = 1;

  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
		    L"SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation",
		    0,
		    KEY_SET_VALUE,
		    &hKey))
    return;

  if (SendMessage(hwnd, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
    {
      RegSetValueExW(hKey,
		     L"DisableAutoDaylightTimeSet",
		     0,
		     REG_DWORD,
		     (LPBYTE)&dwValue,
		     sizeof(DWORD));
    }
  else
    {
      RegDeleteValueW(hKey,
		      L"DisableAutoDaylightTimeSet");
    }

  RegCloseKey(hKey);
}


/* Property page dialog callback */
INT_PTR CALLBACK
TimeZonePageProc(HWND hwndDlg,
		 UINT uMsg,
		 WPARAM wParam,
		 LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG:
      CreateTimeZoneList();
      ShowTimeZoneList(GetDlgItem(hwndDlg, IDC_TIMEZONELIST));
      GetAutoDaylightInfo(GetDlgItem(hwndDlg, IDC_AUTODAYLIGHT));
      break;

    case WM_COMMAND:
      if ((LOWORD(wParam) == IDC_TIMEZONELIST && HIWORD(wParam) == CBN_SELCHANGE) ||
          (LOWORD(wParam) == IDC_AUTODAYLIGHT && HIWORD(wParam) == BN_CLICKED))
        {
          /* Enable the 'Apply' button */
          PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
        }
      break;

    case WM_DESTROY:
      DestroyTimeZoneList();
      break;

    case WM_NOTIFY:
      {
          LPNMHDR lpnm = (LPNMHDR)lParam;

          switch (lpnm->code)
            {
              case PSN_APPLY:
                SetAutoDaylightInfo(GetDlgItem(hwndDlg, IDC_AUTODAYLIGHT));
                SetLocalTimeZone(GetDlgItem(hwndDlg, IDC_TIMEZONELIST));
                SetWindowLong(hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR);
                return TRUE;

              default:
                break;
            }

      }
      break;
  }

  return FALSE;
}


static VOID
InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc)
{
  ZeroMemory(psp, sizeof(PROPSHEETPAGE));
  psp->dwSize = sizeof(PROPSHEETPAGE);
  psp->dwFlags = PSP_DEFAULT;
  psp->hInstance = hApplet;
  psp->pszTemplate = MAKEINTRESOURCE(idDlg);
  psp->pfnDlgProc = DlgProc;
}


LONG APIENTRY
Applet(HWND hwnd, UINT uMsg, LONG wParam, LONG lParam)
{
  PROPSHEETHEADER psh;
  PROPSHEETPAGE psp[3];
  TCHAR Caption[256];

  LoadString(hApplet, IDS_CPLNAME, Caption, sizeof(Caption) / sizeof(TCHAR));

  ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
  psh.dwSize = sizeof(PROPSHEETHEADER);
  psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_PROPTITLE;
  psh.hwndParent = NULL;
  psh.hInstance = hApplet;
  psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDC_CPLICON));
  psh.pszCaption = Caption;
  psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
  psh.nStartPage = 0;
  psh.ppsp = psp;

  InitPropSheetPage(&psp[0], IDD_DATETIMEPAGE, DateTimePageProc);
  InitPropSheetPage(&psp[1], IDD_TIMEZONEPAGE, TimeZonePageProc);

  return (LONG)(PropertySheet(&psh) != -1);
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
      return NUM_APPLETS;

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


BOOL STDCALL
DllMain(HINSTANCE hinstDLL,
	DWORD dwReason,
	LPVOID lpReserved)
{
  switch (dwReason)
  {
    case DLL_PROCESS_ATTACH:
      {
	INITCOMMONCONTROLSEX InitControls;

	InitControls.dwSize = sizeof(INITCOMMONCONTROLSEX);
	InitControls.dwICC = ICC_DATE_CLASSES | ICC_PROGRESS_CLASS | ICC_UPDOWN_CLASS;
	InitCommonControlsEx(&InitControls);

	hApplet = hinstDLL;
      }
      break;
  }

  return TRUE;
}

/* EOF */
