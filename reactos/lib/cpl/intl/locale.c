/*
 *  ReactOS
 *  Copyright (C) 2004, 2005 ReactOS Team
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
 * PROJECT:         ReactOS International Control Panel
 * FILE:            lib/cpl/intl/locale.c
 * PURPOSE:         Locale property page
 * PROGRAMMER:      Eric Kohl
 *                  Klemens Friedl
 */

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>

#include "intl.h"
#include "resource.h"


// FIXME:
//        * change registry function (-> "HKCR\MIME\Database\Rfc1766")



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
  ULONG Index;             /* 'Index' */
} TIMEZONE_ENTRY, *PTIMEZONE_ENTRY;



PTIMEZONE_ENTRY TimeZoneListHead = NULL;
PTIMEZONE_ENTRY TimeZoneListTail = NULL;




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
	  RegCloseKey(hZonesKey);
	  break;
	}

      dwValueSize = 64 * sizeof(WCHAR);
      if (RegQueryValueExW(hZonesKey,
			   L"Display",
			   NULL,
			   NULL,
			   (LPBYTE)&Entry->Description,
			   &dwValueSize))
	{
	  RegCloseKey(hZonesKey);
	  break;
	}

      dwValueSize = 32 * sizeof(WCHAR);
      if (RegQueryValueExW(hZonesKey,
			   L"Std",
			   NULL,
			   NULL,
			   (LPBYTE)&Entry->StandardName,
			   &dwValueSize))
	{
	  RegCloseKey(hZonesKey);
	  break;
	}

      dwValueSize = 32 * sizeof(WCHAR);
      if (RegQueryValueExW(hZonesKey,
			   L"Dlt",
			   NULL,
			   NULL,
			   (LPBYTE)&Entry->DaylightName,
			   &dwValueSize))
	{
	  RegCloseKey(hZonesKey);
	  break;
	}

      dwValueSize = sizeof(DWORD);
      if (RegQueryValueExW(hZonesKey,
			   L"Index",
			   NULL,
			   NULL,
			   (LPBYTE)&Entry->Index,
			   &dwValueSize))
	{
	  RegCloseKey(hZonesKey);
	  break;
	}

      dwValueSize = sizeof(TZ_INFO);
      if (RegQueryValueExW(hZonesKey,
			   L"TZI",
			   NULL,
			   NULL,
			   (LPBYTE)&Entry->TimezoneInfo,
			   &dwValueSize))
	{
	  RegCloseKey(hZonesKey);
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




/* Property page dialog callback */
INT_PTR CALLBACK
LocalePageProc(HWND hwndDlg,
	       UINT uMsg,
	       WPARAM wParam,
	       LPARAM lParam)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:

      CreateTimeZoneList();
      ShowTimeZoneList(GetDlgItem(hwndDlg, IDC_LANGUAGELIST));

      break;
  }
  return FALSE;
}


/* EOF */
