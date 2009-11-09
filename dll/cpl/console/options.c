/*
 * PROJECT:         ReactOS Console Configuration DLL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/console/options.c
 * PURPOSE:         displays options dialog
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@student.tugraz.at)
 */

#include "console.h"

static
void
UpdateDialogElements(HWND hwndDlg, PConsoleInfo pConInfo);

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

	pConInfo = (PConsoleInfo) GetWindowLongPtr(hwndDlg, DWLP_USER);

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			pConInfo = (PConsoleInfo) ((LPPROPSHEETPAGE)lParam)->lParam;
			SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pConInfo);
			UpdateDialogElements(hwndDlg, pConInfo);
			return TRUE;
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
				PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
			}
			else if (lppsn->hdr.code == PSN_APPLY)
			{
				if (!pConInfo->AppliedConfig)
				{
					ApplyConsoleInfo(hwndDlg, pConInfo);
				}
				else
				{
					/* options have already been applied */
					SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR);
					return TRUE;
				}
				return TRUE;
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
					PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
					break;
				}
				case IDC_RADIO_MEDIUM_CURSOR:
				{
					pConInfo->CursorSize = 0x32;
					PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
					break;
				}
				case IDC_RADIO_LARGE_CURSOR:
				{
					pConInfo->CursorSize = 0x64;
					PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
					break;
				}
				case IDC_RADIO_DISPLAY_WINDOW:
				{
					pConInfo->FullScreen = FALSE;
					PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
					break;
				}
				case IDC_RADIO_DISPLAY_FULL:
				{
					pConInfo->FullScreen = TRUE;
					PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
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
					PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
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
					PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
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
					PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
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

static
void
UpdateDialogElements(HWND hwndDlg, PConsoleInfo pConInfo)
{
  HWND hDlgCtrl;
  TCHAR szBuffer[MAX_PATH];

	/* update cursor size */
	if ( pConInfo->CursorSize == 0)
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


