/*
 * PROJECT:         ReactOS Console Configuration DLL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/console/options.c
 * PURPOSE:         displays options dialog
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@student.tugraz.at)
 */

#include "console.h"

#define NDEBUG
#include <debug.h>

static
void
UpdateDialogElements(HWND hwndDlg, PCONSOLE_PROPS pConInfo);

INT_PTR
CALLBACK
OptionsProc(HWND hwndDlg,
            UINT uMsg,
            WPARAM wParam,
            LPARAM lParam)
{
    PCONSOLE_PROPS pConInfo;
    LRESULT lResult;
    HWND hDlgCtrl;
    LPPSHNOTIFY lppsn;

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
            if (!pConInfo) break;

            lppsn = (LPPSHNOTIFY) lParam;
            if (lppsn->hdr.code == UDN_DELTAPOS)
            {
                hDlgCtrl = GetDlgItem(hwndDlg, IDC_EDIT_BUFFER_SIZE);
                pConInfo->ci.HistoryBufferSize = LOWORD(SendMessage(hDlgCtrl, UDM_GETPOS, 0, 0));

                hDlgCtrl = GetDlgItem(hwndDlg, IDC_EDIT_NUM_BUFFER);
                pConInfo->ci.NumberOfHistoryBuffers = LOWORD(SendMessage(hDlgCtrl, UDM_GETPOS, 0, 0));
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
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
            if (!pConInfo) break;

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
                    pConInfo->ci.FullScreen = FALSE;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                }
                case IDC_RADIO_DISPLAY_FULL:
                {
                    pConInfo->ci.FullScreen = TRUE;
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
UpdateDialogElements(HWND hwndDlg, PCONSOLE_PROPS pConInfo)
{
    HWND hDlgCtrl;
    TCHAR szBuffer[MAX_PATH];

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
    SendMessage(hDlgCtrl, UDM_SETRANGE, 0, MAKELONG((short)999, (short)1));
    hDlgCtrl = GetDlgItem(hwndDlg, IDC_EDIT_NUM_BUFFER);
    _stprintf(szBuffer, _T("%d"), pConInfo->ci.NumberOfHistoryBuffers);
    SendMessage(hDlgCtrl, WM_SETTEXT, 0, (LPARAM)szBuffer);

    /* Update buffer size */
    hDlgCtrl = GetDlgItem(hwndDlg, IDC_UPDOWN_BUFFER_SIZE);
    SendMessage(hDlgCtrl, UDM_SETRANGE, 0, MAKELONG((short)999, (short)1));
    hDlgCtrl = GetDlgItem(hwndDlg, IDC_EDIT_BUFFER_SIZE);
    _stprintf(szBuffer, _T("%d"), pConInfo->ci.HistoryBufferSize);
    SendMessage(hDlgCtrl, WM_SETTEXT, 0, (LPARAM)szBuffer);

    /* Update discard duplicates */
    hDlgCtrl = GetDlgItem(hwndDlg, IDC_CHECK_DISCARD_DUPLICATES);
    if (pConInfo->ci.HistoryNoDup)
        SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
    else
        SendMessage(hDlgCtrl, BM_SETCHECK, (LPARAM)BST_UNCHECKED, 0);

    /* Update full/window screen */
    if (pConInfo->ci.FullScreen)
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

    /* Update quick edit */
    hDlgCtrl = GetDlgItem(hwndDlg, IDC_CHECK_QUICK_EDIT);
    if (pConInfo->ci.QuickEdit)
        SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
    else
        SendMessage(hDlgCtrl, BM_SETCHECK, (LPARAM)BST_UNCHECKED, 0);

    /* Update insert mode */
    hDlgCtrl = GetDlgItem(hwndDlg, IDC_CHECK_INSERT_MODE);
    if (pConInfo->ci.InsertMode)
        SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
    else
        SendMessage(hDlgCtrl, BM_SETCHECK, (LPARAM)BST_UNCHECKED, 0);
}
