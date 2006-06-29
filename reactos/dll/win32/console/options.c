/* $Id$
 *
 * PROJECT:         ReactOS Console Configuration DLL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/console/options.c
 * PURPOSE:         displays options dialog
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@student.tugraz.at)
 */

#include "console.h"


ConsoleInfo g_ConsoleInfo;

BOOLEAN InitializeOptionsDialog();

INT_PTR 
CALLBACK
OptionsProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
			return InitializeOptionsDialog(hwndDlg);

		default:
			break;
	}

	return FALSE;
}

BOOL InitializeOptionsFromReg(TCHAR * Path)
{
  HKEY hKey;
  HKEY hSubKey;
  DWORD dwNumSubKeys = 0;
  DWORD dwIndex;
  DWORD dwValueName;
  DWORD dwBufferSize;
  TCHAR szValueName[MAX_PATH];
  BYTE szBuffer[MAX_PATH];

  if ( RegOpenCurrentUser(KEY_READ, &hKey) != ERROR_SUCCESS )
	 return FALSE;


  if ( RegOpenKeyEx(hKey, Path, 0, KEY_READ, &hSubKey) != ERROR_SUCCESS)
  {
		RegCloseKey(hKey);
		return FALSE;
  }

  RegQueryInfoKey(hKey, NULL, NULL, NULL, &dwNumSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL );

  for (dwIndex = 0; dwIndex < dwNumSubKeys; dwIndex++)
  {
	 dwBufferSize = MAX_PATH;
	 dwValueName = MAX_PATH;

	 if ( RegEnumValue(hSubKey, dwIndex, szValueName, &dwValueName, NULL, NULL, szBuffer, &dwBufferSize) != ERROR_SUCCESS)
		break;

	 if ( !_tcscmp(szValueName, _T("CursorSize")) )
	 {
		int val = _ttoi((TCHAR*)szBuffer);
		if ( val == 0x32)
			g_ConsoleInfo.CursorSize = val;
		else if ( val == 0x64 )
			g_ConsoleInfo.CursorSize = val;
	}
	else if ( !_tcscmp(szValueName, _T("NumberOfHistoryBuffers")) )
	{
			int val = _ttoi((TCHAR*)szBuffer);
			g_ConsoleInfo.NumberOfHistoryBuffers = val;
	 }
	 else if ( !_tcscmp(szValueName, _T("HistoryBufferSize")) )
	 {
		 	int val = _ttoi((TCHAR*)szBuffer);
			g_ConsoleInfo.HistoryBufferSize = val;
	 }
	 else if ( !_tcscmp(szValueName, _T("HistoryNoDup")) )
	 {
		int val = _ttoi((TCHAR*)szBuffer);
		g_ConsoleInfo.HistoryNoDup = val;
	 }
	 else if ( !_tcscmp(szValueName, _T("FullScreen")) )
	 {
		int val = _ttoi((TCHAR*)szBuffer);
		g_ConsoleInfo.FullScreen = val;
	 }
	 else if ( !_tcscmp(szValueName, _T("QuickEdit")) )
	 {
		int val = _ttoi((TCHAR*)szBuffer);
		g_ConsoleInfo.QuickEdit = val;
	 }
	 else if ( !_tcscmp(szValueName, _T("InsertMode")) )
	 {
		int val = _ttoi((TCHAR*)szBuffer);
		g_ConsoleInfo.InsertMode = val;
	 }
  }

  RegCloseKey(hKey);
  RegCloseKey(hSubKey);
  return TRUE;
}

void 
UpdateDialogElements(HWND hwndDlg)
{
  HWND hDlgCtrl;
  TCHAR szBuffer[MAX_PATH];

	/* update cursor size */
	if ( g_ConsoleInfo.CursorSize == 0 )
	{
		/* small cursor */
		hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_SMALL_CURSOR);
		SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

		hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_MEDIUM_CURSOR);
		SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
		hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_LARGE_CURSOR);
		SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
	}
	else if ( g_ConsoleInfo.CursorSize == 0x32 )
	{
		hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_MEDIUM_CURSOR);
		SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

		hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_SMALL_CURSOR);
		SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
		hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_LARGE_CURSOR);
		SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
	}
	else if ( g_ConsoleInfo.CursorSize == 0x64 )
	{
		hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_LARGE_CURSOR);
		SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

		hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_SMALL_CURSOR);
		SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
		hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_MEDIUM_CURSOR);
		SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
	}

	/* update num buffers */
	hDlgCtrl = GetDlgItem(hwndDlg, IDC_EDIT_NUM_BUFFER);
	_stprintf(szBuffer, _T("%d"), g_ConsoleInfo.NumberOfHistoryBuffers);
	SendMessage(hDlgCtrl, WM_SETTEXT, 0, (LPARAM)szBuffer);

	/* update buffer size */
	hDlgCtrl = GetDlgItem(hwndDlg, IDC_EDIT_BUFFER_SIZE);
	_stprintf(szBuffer, _T("%d"), g_ConsoleInfo.HistoryBufferSize);
	SendMessage(hDlgCtrl, WM_SETTEXT, 0, (LPARAM)szBuffer);

	/* update discard duplicates */
	hDlgCtrl = GetDlgItem(hwndDlg, IDC_CHECK_DISCARD_DUPLICATES);
	if ( g_ConsoleInfo.HistoryNoDup )
		SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
	else
		SendMessage(hDlgCtrl, BM_SETCHECK, (LPARAM)BST_UNCHECKED, 0);

	/* update full/window screen */
	if ( g_ConsoleInfo.FullScreen )
	{
		hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_DISPLAY_FULL);
		SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

		hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_DISPLAY_WINDOW);
		SendMessage(hDlgCtrl, BM_SETCHECK, (LPARAM)BST_UNCHECKED, 0);
	}
	else
	{
		hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_DISPLAY_WINDOW);
		SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

		hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_DISPLAY_FULL);
		SendMessage(hDlgCtrl, BM_SETCHECK, (LPARAM)BST_UNCHECKED, 0);
	}

	/* update quick edit */
	hDlgCtrl = GetDlgItem(hwndDlg, IDC_CHECK_QUICK_EDIT);
	if ( g_ConsoleInfo.QuickEdit )
		SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
	else
		SendMessage(hDlgCtrl, BM_SETCHECK, (LPARAM)BST_UNCHECKED, 0);

	/* update insert mode */
	hDlgCtrl = GetDlgItem(hwndDlg, IDC_CHECK_INSERT_MODE);
	if ( g_ConsoleInfo.InsertMode )
		SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
	else
		SendMessage(hDlgCtrl, BM_SETCHECK, (LPARAM)BST_UNCHECKED, 0);
}


BOOLEAN InitializeOptionsDialog(HWND hwndDlg)
{
	STARTUPINFO StartupInfo;

	GetStartupInfo(&StartupInfo);

	if ( StartupInfo.lpTitle )
	{
		if ( InitializeOptionsFromReg(StartupInfo.lpTitle) )
		{
			UpdateDialogElements(hwndDlg);
			return TRUE;
		}
		//TODO
		//
		// Windows XP uses unexpanded environment vars to get path
		// i.e. c:\windows\system32\cmd.exe
		// becomes
		// %SystemRoot%_system32_cmd.exe

	}

	return TRUE;
}
