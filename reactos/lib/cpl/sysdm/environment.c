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
/* $Id: environment.c,v 1.1 2004/07/02 20:28:00 ekohl Exp $
 *
 * PROJECT:         ReactOS System Control Panel
 * FILE:            lib/cpl/sysdm/environment.c
 * PURPOSE:         Environment variable settings
 * PROGRAMMER:      Eric Kohl
 */

#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <tchar.h>

#include "resource.h"
#include "sysdm.h"


static VOID
SetEnvironmentVariables(HWND hwndListView,
			HKEY hRootKey,
			LPTSTR lpSubKeyName)
{
  HKEY hKey;
  DWORD dwValues;
  DWORD dwMaxValueNameLength;
  DWORD dwMaxValueDataLength;
  DWORD i;
  LPTSTR lpName;
  LPTSTR lpData;
  LPTSTR lpExpandData;
  DWORD dwNameLength;
  DWORD dwDataLength;
  DWORD dwType;

  LV_ITEM lvi;
  int nIndex;

  if (RegOpenKeyEx(hRootKey,
		   lpSubKeyName,
		   0,
		   KEY_READ,
		   &hKey))
    return;

  if (RegQueryInfoKey(hKey,
		      NULL,
		      NULL,
		      NULL,
		      NULL,
		      NULL,
		      NULL,
		      &dwValues,
		      &dwMaxValueNameLength,
		      &dwMaxValueDataLength,
		      NULL,
		      NULL))
  {
    RegCloseKey(hKey);
    return;
  }

  lpName = GlobalAlloc(GPTR, (dwMaxValueNameLength + 1) * sizeof(TCHAR));
  if (lpName == NULL)
  {
    RegCloseKey(hKey);
    return;
  }

  lpData = GlobalAlloc(GPTR, (dwMaxValueDataLength + 1) * sizeof(TCHAR));
  if (lpData == NULL)
  {
    GlobalFree(lpData);
    RegCloseKey(hKey);
    return;
  }

  for (i = 0; i < dwValues; i++)
  {
    dwNameLength = dwMaxValueNameLength + 1;
    dwDataLength = dwMaxValueDataLength + 1;
    if (RegEnumValue(hKey,
		     i,
		     lpName,
		     &dwNameLength,
		     NULL,
		     &dwType,
		     (LPBYTE)lpData,
		     &dwDataLength))
    {
      GlobalFree(lpName);
      GlobalFree(lpData);
      RegCloseKey(hKey);
      return;
    }

    if (dwType == REG_EXPAND_SZ)
    {
      lpExpandData = GlobalAlloc(GPTR, MAX_PATH * sizeof(TCHAR));
      if (lpExpandData == NULL)
      {
        GlobalFree(lpName);
        GlobalFree(lpData);
        RegCloseKey(hKey);
        return;
      }

      ExpandEnvironmentStrings(lpData,
			       lpExpandData,
			       MAX_PATH);
    }

    memset(&lvi, 0x00, sizeof(lvi));
    lvi.mask = LVIF_TEXT | LVIF_STATE;
    lvi.pszText = lpName;
    lvi.state=0;
    nIndex = ListView_InsertItem(hwndListView, &lvi);

    if (dwType == REG_EXPAND_SZ)
    {
      ListView_SetItemText(hwndListView, nIndex, 1, lpExpandData);
      GlobalFree(lpExpandData);
    }
    else
    {
      ListView_SetItemText(hwndListView, nIndex, 1, lpData);
    }
  }

  GlobalFree(lpName);
  GlobalFree(lpData);
  RegCloseKey(hKey);
}


static VOID
SetListViewColumns(HWND hwndListView)
{
  RECT rect;
  LV_COLUMN column;

  GetClientRect(hwndListView, &rect);

  memset(&column, 0x00, sizeof(column));
  column.mask=LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM|LVCF_TEXT;
  column.fmt=LVCFMT_LEFT;
  column.cx = (rect.right - rect.left) / 3;
  column.iSubItem = 0;
  column.pszText = _T("Variable");
  ListView_InsertColumn(hwndListView, 0, &column);

  column.cx = (rect.right - rect.left) - ((rect.right - rect.left) / 3) - 1;
  column.iSubItem = 1;
  column.pszText = _T("Value");
  ListView_InsertColumn(hwndListView, 1, &column);
}


static VOID
OnInitDialog(HWND hwndDlg)
{
  HWND hwndListView;

  /* Set user environment variables */
  hwndListView = GetDlgItem(hwndDlg, IDC_USER_VARIABLE_LIST);

  SetListViewColumns(hwndListView);

  SetEnvironmentVariables(hwndListView,
			  HKEY_CURRENT_USER,
			  _T("Environment"));

  ListView_SetColumnWidth(hwndListView,2,LVSCW_AUTOSIZE_USEHEADER);
  ListView_Update(hwndListView,0);


  /* Set system environment variables */
  hwndListView = GetDlgItem(hwndDlg, IDC_SYSTEM_VARIABLE_LIST);

  SetListViewColumns(hwndListView);

  SetEnvironmentVariables(hwndListView,
			  HKEY_LOCAL_MACHINE,
			  _T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"));

  ListView_SetColumnWidth(hwndListView,2,LVSCW_AUTOSIZE_USEHEADER);
  ListView_Update(hwndListView,0);
}


/* Environment dialog procedure */
BOOL CALLBACK
EnvironmentDlgProc(HWND hwndDlg,
		   UINT uMsg,
		   WPARAM wParam,
		   LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG:
      OnInitDialog(hwndDlg);
      break;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_USER_VARIABLE_NEW:
        case IDC_USER_VARIABLE_EDIT:
        case IDC_USER_VARIABLE_DELETE:
          break;

        case IDC_SYSTEM_VARIABLE_NEW:
        case IDC_SYSTEM_VARIABLE_EDIT:
        case IDC_SYSTEM_VARIABLE_DELETE:
          break;

        case IDOK:
          EndDialog(hwndDlg, 0);
          return TRUE;

        case IDCANCEL:
          EndDialog(hwndDlg, 0);
          return TRUE;
      }
      break;
  }

  return FALSE;
}

/* EOF */
