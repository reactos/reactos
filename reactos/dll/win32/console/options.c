/* $Id$
 *
 * PROJECT:         ReactOS Console Configuration DLL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/console/options.c
 * PURPOSE:         displays options dialog
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@student.tugraz.at)
 */

#include "console.h"

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
	PConsoleInfo pConInfo;
	LRESULT lResult;
	HWND hDlgCtrl;
    LPPSHNOTIFY lppsn;

	pConInfo = (PConsoleInfo) GetWindowLongPtr(GetParent(hwndDlg), DWLP_USER);

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			return InitializeOptionsDialog(hwndDlg);
		}
		case WM_NOTIFY:
		{
			if (!pConInfo)
			{
				break;
			}
			lppsn = (LPPSHNOTIFY) lParam; 
            if (lppsn->hdr.code == UDN_DELTAPOS)
            {
				hDlgCtrl = GetDlgItem(hwndDlg, IDC_EDIT_BUFFER_SIZE);
				pConInfo->HistoryBufferSize = LOWORD(SendMessage(hDlgCtrl, UDM_GETPOS, 0, 0));

				hDlgCtrl = GetDlgItem(hwndDlg, IDC_EDIT_NUM_BUFFER);
				pConInfo->NumberOfHistoryBuffers = LOWORD(SendMessage(hDlgCtrl, UDM_GETPOS, 0, 0));
				//PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
			}
			break;
		}
		case WM_COMMAND:
		{
			if (!pConInfo)
			{
				break;
			}
			switch(LOWORD(wParam))
			{
				case IDC_RADIO_SMALL_CURSOR:
				{
					pConInfo->CursorSize = 0x0;
					//PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
					break;
				}
				case IDC_RADIO_MEDIUM_CURSOR:
				{
					pConInfo->CursorSize = 0x32;
					//PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
					break;
				}
				case IDC_RADIO_LARGE_CURSOR:
				{				
					pConInfo->CursorSize = 0x64;
					//PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
					break;
				}
				case IDC_RADIO_DISPLAY_WINDOW:
				{
					pConInfo->FullScreen = FALSE;
					//PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
					break;
				}
				case IDC_RADIO_DISPLAY_FULL:
				{
					pConInfo->FullScreen = TRUE;
					//PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
					break;
				}
				case IDC_CHECK_QUICK_EDIT:
				{
                    lResult = SendMessage((HWND)lParam, BM_GETCHECK, (WPARAM)0, (LPARAM)0);
                    if (lResult == BST_CHECKED)
                    {
						pConInfo->QuickEdit = FALSE;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0);
                    }
                    else if (lResult == BST_UNCHECKED)
                    {
						pConInfo->QuickEdit = TRUE;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
                    }
					//PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
					break;
				}
				case IDC_CHECK_INSERT_MODE:
				{
                    lResult = SendMessage((HWND)lParam, BM_GETCHECK, (WPARAM)0, (LPARAM)0);
                    if (lResult == BST_CHECKED)
                    {
						pConInfo->InsertMode = FALSE;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0);
                    }
                    else if (lResult == BST_UNCHECKED)
                    {
						pConInfo->InsertMode = TRUE;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
                    }
					//PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
					break;
				}
				case IDC_CHECK_DISCARD_DUPLICATES:
				{
                   lResult = SendMessage((HWND)lParam, BM_GETCHECK, (WPARAM)0, (LPARAM)0);
                    if (lResult == BST_CHECKED)
                    {
						pConInfo->HistoryNoDup = FALSE;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0);
                    }
                    else if (lResult == BST_UNCHECKED)
                    {
						pConInfo->HistoryNoDup = TRUE;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
                    }
					//PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
					break;
				}
				default:
					break;
			}
			break;
		}
		default:
			break;
	}

	return FALSE;
}

BOOL InitializeOptionsFromReg(TCHAR * Path, PConsoleInfo pConInfo)
{
  HKEY hKey;
  HKEY hSubKey;
  DWORD dwNumSubKeys = 0;
  DWORD dwIndex;
  DWORD dwValueName;
  DWORD dwValue;
  TCHAR szValueName[MAX_PATH];
  DWORD Value;

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
	 dwValue = sizeof(Value);
	 dwValueName = MAX_PATH;

	 if ( RegEnumValue(hSubKey, dwIndex, szValueName, &dwValueName, NULL, NULL, (BYTE*)&Value, &dwValue) != ERROR_SUCCESS)
		break;

	if ( !_tcscmp(szValueName, _T("CursorSize")) )
	{
		if ( Value == 0x32)
			pConInfo->CursorSize = Value;
		else if ( Value == 0x64 )
			pConInfo->CursorSize = Value;
	}
	else if ( !_tcscmp(szValueName, _T("NumberOfHistoryBuffers")) )
	{
		pConInfo->NumberOfHistoryBuffers = Value;
	}
	else if ( !_tcscmp(szValueName, _T("HistoryBufferSize")) )
	{
		pConInfo->HistoryBufferSize = Value;
	}
	else if ( !_tcscmp(szValueName, _T("HistoryNoDup")) )
	{
		pConInfo->HistoryNoDup = Value;
	}
	else if ( !_tcscmp(szValueName, _T("FullScreen")) )
	{
		pConInfo->FullScreen = Value;
	}
	else if ( !_tcscmp(szValueName, _T("QuickEdit")) )
	{
		pConInfo->QuickEdit = Value;
	}
	else if ( !_tcscmp(szValueName, _T("InsertMode")) )
	{
		pConInfo->InsertMode = Value;
	}
  }

  RegCloseKey(hKey);
  RegCloseKey(hSubKey);
  return TRUE;
}

void 
UpdateDialogElements(HWND hwndDlg, PConsoleInfo pConInfo)
{
  HWND hDlgCtrl;
  TCHAR szBuffer[MAX_PATH];

	/* update cursor size */
	if ( pConInfo->CursorSize == 0 )
	{
		/* small cursor */
		hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_SMALL_CURSOR);
		SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

		hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_MEDIUM_CURSOR);
		SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
		hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_LARGE_CURSOR);
		SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
	}
	else if ( pConInfo->CursorSize == 0x32 )
	{
		hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_MEDIUM_CURSOR);
		SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

		hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_SMALL_CURSOR);
		SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
		hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_LARGE_CURSOR);
		SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
	}
	else if ( pConInfo->CursorSize == 0x64 )
	{
		hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_LARGE_CURSOR);
		SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

		hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_SMALL_CURSOR);
		SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
		hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_MEDIUM_CURSOR);
		SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
	}

	/* update num buffers */
	hDlgCtrl = GetDlgItem(hwndDlg, IDC_UPDOWN_NUM_BUFFER);
	SendMessage(hDlgCtrl, UDM_SETRANGE, 0, MAKELONG((short)999, (short)1));
	hDlgCtrl = GetDlgItem(hwndDlg, IDC_EDIT_NUM_BUFFER);
	_stprintf(szBuffer, _T("%d"), pConInfo->NumberOfHistoryBuffers);
	SendMessage(hDlgCtrl, WM_SETTEXT, 0, (LPARAM)szBuffer);

	/* update buffer size */
	hDlgCtrl = GetDlgItem(hwndDlg, IDC_UPDOWN_BUFFER_SIZE);
	SendMessage(hDlgCtrl, UDM_SETRANGE, 0, MAKELONG((short)999, (short)1));
	hDlgCtrl = GetDlgItem(hwndDlg, IDC_EDIT_BUFFER_SIZE);
	_stprintf(szBuffer, _T("%d"), pConInfo->HistoryBufferSize);
	SendMessage(hDlgCtrl, WM_SETTEXT, 0, (LPARAM)szBuffer);
    


	/* update discard duplicates */
	hDlgCtrl = GetDlgItem(hwndDlg, IDC_CHECK_DISCARD_DUPLICATES);
	if ( pConInfo->HistoryNoDup )
		SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
	else
		SendMessage(hDlgCtrl, BM_SETCHECK, (LPARAM)BST_UNCHECKED, 0);

	/* update full/window screen */
	if ( pConInfo->FullScreen )
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
	if ( pConInfo->QuickEdit )
		SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
	else
		SendMessage(hDlgCtrl, BM_SETCHECK, (LPARAM)BST_UNCHECKED, 0);

	/* update insert mode */
	hDlgCtrl = GetDlgItem(hwndDlg, IDC_CHECK_INSERT_MODE);
	if ( pConInfo->InsertMode )
		SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
	else
		SendMessage(hDlgCtrl, BM_SETCHECK, (LPARAM)BST_UNCHECKED, 0);
}


BOOLEAN InitializeOptionsDialog(HWND hwndDlg)
{
	PConsoleInfo pConInfo = (PConsoleInfo) GetWindowLongPtr(GetParent(hwndDlg), DWLP_USER);

	if (!pConInfo)
		return FALSE;

	InitializeOptionsFromReg(pConInfo->szProcessName, pConInfo);
	UpdateDialogElements(hwndDlg, pConInfo);
	return TRUE;
}

BOOL WriteConsoleOptions(PConsoleInfo pConInfo)
{
	HKEY hKey;
	HKEY hSubKey;

	if ( RegOpenCurrentUser(KEY_READ | KEY_SET_VALUE, &hKey) != ERROR_SUCCESS )
	 return FALSE;


	if ( RegOpenKeyEx(hKey, pConInfo->szProcessName, 0, KEY_READ | KEY_SET_VALUE, &hSubKey) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return FALSE;
	}

	if ( pConInfo->CursorSize  == 0x0)
		RegDeleteKey(hSubKey, _T("CursorSize"));
	else
		RegSetValueEx(hSubKey, _T("CursorSize"), 0, REG_DWORD, (const BYTE *)&pConInfo->CursorSize, sizeof(DWORD));

	if ( pConInfo->NumberOfHistoryBuffers == 0x5 )
		RegDeleteKey(hSubKey, _T("NumberOfHistoryBuffers"));
	else
		RegSetValueEx(hSubKey, _T("NumberOfHistoryBuffers"), 0, REG_DWORD, (const BYTE *)&pConInfo->NumberOfHistoryBuffers, sizeof(DWORD));

	if ( pConInfo->HistoryBufferSize == 50 )
		RegDeleteKey(hSubKey, _T("HistoryBufferSize"));
	else
		RegSetValueEx(hSubKey, _T("HistoryBufferSize"), 0, REG_DWORD, (const BYTE *)&pConInfo->HistoryBufferSize, sizeof(DWORD));

	if ( pConInfo->FullScreen == FALSE )
		RegDeleteKey(hSubKey, _T("FullScreen"));
	else
		RegSetValueEx(hSubKey, _T("FullScreen"), 0, REG_DWORD, (const BYTE *)&pConInfo->FullScreen, sizeof(DWORD));

	if ( pConInfo->QuickEdit == FALSE)
		RegDeleteKey(hSubKey, _T("QuickEdit"));
	else
		RegSetValueEx(hSubKey, _T("QuickEdit"), 0, REG_DWORD, (const BYTE *)&pConInfo->QuickEdit, sizeof(DWORD));

	if ( pConInfo->InsertMode == TRUE )
		RegDeleteKey(hSubKey, _T("InsertMode"));
	else
		RegSetValueEx(hSubKey, _T("InsertMode"), 0, REG_DWORD, (const BYTE *)&pConInfo->InsertMode, sizeof(DWORD));

	if ( pConInfo->HistoryNoDup == FALSE )
		RegDeleteKey(hSubKey, _T("HistoryNoDup"));
	else
		RegSetValueEx(hSubKey, _T("HistoryNoDup"), 0, REG_DWORD, (const BYTE *)&pConInfo->HistoryNoDup, sizeof(DWORD));

	RegCloseKey(hKey);
	RegCloseKey(hSubKey);

	return TRUE;
}
