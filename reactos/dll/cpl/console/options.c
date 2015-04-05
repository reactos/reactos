/*
 * PROJECT:         ReactOS Console Configuration DLL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/console/options.c
 * PURPOSE:         Options dialog
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "console.h"

#define NDEBUG
#include <debug.h>

static VOID
UpdateDialogElements(HWND hwndDlg, PCONSOLE_STATE_INFO pConInfo)
{
    HWND hDlgCtrl;

    /* Update cursor size */
    if (pConInfo->CursorSize <= 25)
    {
        /* Small cursor */
        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_SMALL_CURSOR);
        SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_MEDIUM_CURSOR);
        SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_LARGE_CURSOR);
        SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
    }
    else if (pConInfo->CursorSize <= 50)
    {
        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_MEDIUM_CURSOR);
        SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_SMALL_CURSOR);
        SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_LARGE_CURSOR);
        SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
    }
    else /* if (pConInfo->CursorSize <= 100) */
    {
        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_LARGE_CURSOR);
        SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_SMALL_CURSOR);
        SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_MEDIUM_CURSOR);
        SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
    }

    /* Update num buffers */
    hDlgCtrl = GetDlgItem(hwndDlg, IDC_UPDOWN_NUM_BUFFER);
    SendMessage(hDlgCtrl, UDM_SETRANGE, 0, MAKELONG(999, 1));
    SetDlgItemInt(hwndDlg, IDC_EDIT_NUM_BUFFER, pConInfo->NumberOfHistoryBuffers, FALSE);

    /* Update buffer size */
    hDlgCtrl = GetDlgItem(hwndDlg, IDC_UPDOWN_BUFFER_SIZE);
    SendMessage(hDlgCtrl, UDM_SETRANGE, 0, MAKELONG(999, 1));
    SetDlgItemInt(hwndDlg, IDC_EDIT_BUFFER_SIZE, pConInfo->HistoryBufferSize, FALSE);

    /* Update discard duplicates */
    CheckDlgButton(hwndDlg, IDC_CHECK_DISCARD_DUPLICATES,
                   pConInfo->HistoryNoDup ? BST_CHECKED : BST_UNCHECKED);

    /* Update full/window screen */
    if (pConInfo->FullScreen)
    {
        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_DISPLAY_FULL);
        SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_DISPLAY_WINDOW);
        SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
    }
    else
    {
        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_DISPLAY_WINDOW);
        SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_DISPLAY_FULL);
        SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
    }

    /* Update quick edit */
    CheckDlgButton(hwndDlg, IDC_CHECK_QUICK_EDIT,
                   pConInfo->QuickEdit ? BST_CHECKED : BST_UNCHECKED);

    /* Update insert mode */
    CheckDlgButton(hwndDlg, IDC_CHECK_INSERT_MODE,
                   pConInfo->InsertMode ? BST_CHECKED : BST_UNCHECKED);
}

INT_PTR
CALLBACK
OptionsProc(HWND hwndDlg,
            UINT uMsg,
            WPARAM wParam,
            LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            UpdateDialogElements(hwndDlg, ConInfo);
            return TRUE;
        }

        case WM_NOTIFY:
        {
            LPNMUPDOWN  lpnmud =  (LPNMUPDOWN)lParam;
            LPPSHNOTIFY lppsn  = (LPPSHNOTIFY)lParam;

            if (lppsn->hdr.code == UDN_DELTAPOS)
            {
                if (lppsn->hdr.idFrom == IDC_UPDOWN_BUFFER_SIZE)
                {
                    lpnmud->iPos = min(max(lpnmud->iPos + lpnmud->iDelta, 1), 999);
                    ConInfo->HistoryBufferSize = lpnmud->iPos;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                }
                else if (lppsn->hdr.idFrom == IDC_UPDOWN_NUM_BUFFER)
                {
                    lpnmud->iPos = min(max(lpnmud->iPos + lpnmud->iDelta, 1), 999);
                    ConInfo->NumberOfHistoryBuffers = lpnmud->iPos;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                }
            }
            else if (lppsn->hdr.code == PSN_APPLY)
            {
                ApplyConsoleInfo(hwndDlg);
                return TRUE;
            }
            break;
        }

        case WM_COMMAND:
        {
            LRESULT lResult;

            switch (LOWORD(wParam))
            {
                case IDC_RADIO_SMALL_CURSOR:
                {
                    ConInfo->CursorSize = 25;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                }
                case IDC_RADIO_MEDIUM_CURSOR:
                {
                    ConInfo->CursorSize = 50;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                }
                case IDC_RADIO_LARGE_CURSOR:
                {
                    ConInfo->CursorSize = 100;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                }
                case IDC_RADIO_DISPLAY_WINDOW:
                {
                    ConInfo->FullScreen = FALSE;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                }
                case IDC_RADIO_DISPLAY_FULL:
                {
                    ConInfo->FullScreen = TRUE;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                }
                case IDC_CHECK_QUICK_EDIT:
                {
                    lResult = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0);
                    if (lResult == BST_CHECKED)
                    {
                        ConInfo->QuickEdit = FALSE;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
                    }
                    else if (lResult == BST_UNCHECKED)
                    {
                        ConInfo->QuickEdit = TRUE;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
                    }
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                }
                case IDC_CHECK_INSERT_MODE:
                {
                    lResult = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0);
                    if (lResult == BST_CHECKED)
                    {
                        ConInfo->InsertMode = FALSE;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
                    }
                    else if (lResult == BST_UNCHECKED)
                    {
                        ConInfo->InsertMode = TRUE;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
                    }
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                }
                case IDC_CHECK_DISCARD_DUPLICATES:
                {
                   lResult = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0);
                    if (lResult == BST_CHECKED)
                    {
                        ConInfo->HistoryNoDup = FALSE;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
                    }
                    else if (lResult == BST_UNCHECKED)
                    {
                        ConInfo->HistoryNoDup = TRUE;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
                    }
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                }
                case IDC_EDIT_BUFFER_SIZE:
                {
                    if (HIWORD(wParam) == EN_KILLFOCUS)
                    {
                        DWORD sizeBuff;

                        sizeBuff = GetDlgItemInt(hwndDlg, IDC_EDIT_BUFFER_SIZE, NULL, FALSE);
                        sizeBuff = min(max(sizeBuff, 1), 999);

                        ConInfo->HistoryBufferSize = sizeBuff;
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
                }
                case IDC_EDIT_NUM_BUFFER:
                {
                    if (HIWORD(wParam) == EN_KILLFOCUS)
                    {
                        DWORD numBuff;

                        numBuff = GetDlgItemInt(hwndDlg, IDC_EDIT_NUM_BUFFER, NULL, FALSE);
                        numBuff = min(max(numBuff, 1), 999);

                        ConInfo->NumberOfHistoryBuffers = numBuff;
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
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
