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

static void
UpdateDialogElements(HWND hwndDlg, PCONSOLE_PROPS pConInfo)
{
    PGUI_CONSOLE_INFO GuiInfo = pConInfo->TerminalInfo.TermInfo;
    HWND hDlgCtrl;

    /* Update cursor size */
    if (pConInfo->ci.CursorSize <= 25)
    {
        /* Small cursor */
        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_SMALL_CURSOR);
        SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_MEDIUM_CURSOR);
        SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_LARGE_CURSOR);
        SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
    }
    else if (pConInfo->ci.CursorSize <= 50)
    {
        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_MEDIUM_CURSOR);
        SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_SMALL_CURSOR);
        SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_LARGE_CURSOR);
        SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
    }
    else /* if (pConInfo->ci.CursorSize <= 100) */
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
    SetDlgItemInt(hwndDlg, IDC_EDIT_NUM_BUFFER, pConInfo->ci.NumberOfHistoryBuffers, FALSE);

    /* Update buffer size */
    hDlgCtrl = GetDlgItem(hwndDlg, IDC_UPDOWN_BUFFER_SIZE);
    SendMessage(hDlgCtrl, UDM_SETRANGE, 0, MAKELONG(999, 1));
    SetDlgItemInt(hwndDlg, IDC_EDIT_BUFFER_SIZE, pConInfo->ci.HistoryBufferSize, FALSE);

    /* Update discard duplicates */
    CheckDlgButton(hwndDlg, IDC_CHECK_DISCARD_DUPLICATES,
                   pConInfo->ci.HistoryNoDup ? BST_CHECKED : BST_UNCHECKED);

    /* Update full/window screen */
    if (GuiInfo->FullScreen)
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
                   pConInfo->ci.QuickEdit ? BST_CHECKED : BST_UNCHECKED);

    /* Update insert mode */
    CheckDlgButton(hwndDlg, IDC_CHECK_INSERT_MODE,
                   pConInfo->ci.InsertMode ? BST_CHECKED : BST_UNCHECKED);
}

INT_PTR
CALLBACK
OptionsProc(HWND hwndDlg,
            UINT uMsg,
            WPARAM wParam,
            LPARAM lParam)
{
    PCONSOLE_PROPS pConInfo;
    PGUI_CONSOLE_INFO GuiInfo;

    pConInfo = (PCONSOLE_PROPS)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pConInfo = (PCONSOLE_PROPS)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pConInfo);
            UpdateDialogElements(hwndDlg, pConInfo);
            return TRUE;
        }

        case WM_NOTIFY:
        {
            LPNMUPDOWN  lpnmud =  (LPNMUPDOWN)lParam;
            LPPSHNOTIFY lppsn  = (LPPSHNOTIFY)lParam;

            // if (!pConInfo) break;

            if (lppsn->hdr.code == UDN_DELTAPOS)
            {
                if (lppsn->hdr.idFrom == IDC_UPDOWN_BUFFER_SIZE)
                {
                    lpnmud->iPos = min(max(lpnmud->iPos + lpnmud->iDelta, 1), 999);
                    pConInfo->ci.HistoryBufferSize = lpnmud->iPos;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                }
                else if (lppsn->hdr.idFrom == IDC_UPDOWN_NUM_BUFFER)
                {
                    lpnmud->iPos = min(max(lpnmud->iPos + lpnmud->iDelta, 1), 999);
                    pConInfo->ci.NumberOfHistoryBuffers = lpnmud->iPos;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                }
            }
            else if (lppsn->hdr.code == PSN_APPLY)
            {
                if (!pConInfo->AppliedConfig)
                {
                    return ApplyConsoleInfo(hwndDlg, pConInfo);
                }
                else
                {
                    /* Options have already been applied */
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                    return TRUE;
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            LRESULT lResult;

            if (!pConInfo) break;
            GuiInfo = pConInfo->TerminalInfo.TermInfo;

            switch (LOWORD(wParam))
            {
                case IDC_RADIO_SMALL_CURSOR:
                {
                    pConInfo->ci.CursorSize = 25;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                }
                case IDC_RADIO_MEDIUM_CURSOR:
                {
                    pConInfo->ci.CursorSize = 50;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                }
                case IDC_RADIO_LARGE_CURSOR:
                {
                    pConInfo->ci.CursorSize = 100;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                }
                case IDC_RADIO_DISPLAY_WINDOW:
                {
                    GuiInfo->FullScreen = FALSE;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                }
                case IDC_RADIO_DISPLAY_FULL:
                {
                    GuiInfo->FullScreen = TRUE;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                }
                case IDC_CHECK_QUICK_EDIT:
                {
                    lResult = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0);
                    if (lResult == BST_CHECKED)
                    {
                        pConInfo->ci.QuickEdit = FALSE;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
                    }
                    else if (lResult == BST_UNCHECKED)
                    {
                        pConInfo->ci.QuickEdit = TRUE;
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
                        pConInfo->ci.InsertMode = FALSE;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
                    }
                    else if (lResult == BST_UNCHECKED)
                    {
                        pConInfo->ci.InsertMode = TRUE;
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
                        pConInfo->ci.HistoryNoDup = FALSE;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
                    }
                    else if (lResult == BST_UNCHECKED)
                    {
                        pConInfo->ci.HistoryNoDup = TRUE;
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

                        pConInfo->ci.HistoryBufferSize = sizeBuff;
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

                        pConInfo->ci.NumberOfHistoryBuffers = numBuff;
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
