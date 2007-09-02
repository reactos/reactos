/* $Id$
 *
 * PROJECT:         ReactOS Accessibility Control Panel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/access/keyboard.c
 * PURPOSE:         Keyboard-related accessibility settings
 * COPYRIGHT:       Copyright 2004 Johannes Anderwald (j_anderw@sbox.tugraz.at)
 *                  Copyright 2007 Eric Kohl
 */

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <commctrl.h>
#include <prsht.h>
#include <tchar.h>
#include "resource.h"
#include "access.h"

typedef struct _GLOBAL_DATA
{
    STICKYKEYS stickyKeys;
    STICKYKEYS oldStickyKeys;
    FILTERKEYS filterKeys;
    FILTERKEYS oldFilterKeys;
    TOGGLEKEYS toggleKeys;
    TOGGLEKEYS oldToggleKeys;
    BOOL bKeyboardPref;
} GLOBAL_DATA, *PGLOBAL_DATA;


#define BOUNCETICKS 5
static INT nBounceArray[BOUNCETICKS] = {500, 700, 1000, 1500, 2000};


/* Property page dialog callback */
INT_PTR CALLBACK
StickyKeysDlgProc(HWND hwndDlg,
                  UINT uMsg,
                  WPARAM wParam,
                  LPARAM lParam)
{
    PGLOBAL_DATA pGlobalData;

    pGlobalData = (PGLOBAL_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pGlobalData = (PGLOBAL_DATA)lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);

            memcpy(&pGlobalData->oldStickyKeys,
                   &pGlobalData->stickyKeys,
                   sizeof(STICKYKEYS));

            CheckDlgButton(hwndDlg,
                           IDC_STICKY_ACTIVATE_CHECK,
                           pGlobalData->stickyKeys.dwFlags & SKF_HOTKEYACTIVE ? BST_CHECKED : BST_UNCHECKED);

            CheckDlgButton(hwndDlg,
                           IDC_STICKY_LOCK_CHECK,
                           pGlobalData->stickyKeys.dwFlags & SKF_TRISTATE ? BST_CHECKED : BST_UNCHECKED);

            CheckDlgButton(hwndDlg,
                           IDC_STICKY_UNLOCK_CHECK,
                           pGlobalData->stickyKeys.dwFlags & SKF_TWOKEYSOFF ? BST_CHECKED : BST_UNCHECKED);

            CheckDlgButton(hwndDlg,
                           IDC_STICKY_SOUND_CHECK,
                           pGlobalData->stickyKeys.dwFlags & SKF_AUDIBLEFEEDBACK ? BST_CHECKED : BST_UNCHECKED);

            CheckDlgButton(hwndDlg,
                           IDC_STICKY_STATUS_CHECK,
                           pGlobalData->stickyKeys.dwFlags & SKF_INDICATOR ? BST_CHECKED : BST_UNCHECKED);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_STICKY_ACTIVATE_CHECK:
                    pGlobalData->stickyKeys.dwFlags ^= SKF_HOTKEYACTIVE;
                    break;

                case IDC_STICKY_LOCK_CHECK:
                    pGlobalData->stickyKeys.dwFlags ^= SKF_TRISTATE;
                    break;

                case IDC_STICKY_UNLOCK_CHECK:
                    pGlobalData->stickyKeys.dwFlags ^= SKF_TWOKEYSOFF;
                    break;

                case IDC_STICKY_SOUND_CHECK:
                    pGlobalData->stickyKeys.dwFlags ^= SKF_AUDIBLEFEEDBACK;
                    break;

                case IDC_STICKY_STATUS_CHECK:
                    pGlobalData->stickyKeys.dwFlags ^= SKF_INDICATOR;
                    break;

                case IDOK:
                    EndDialog(hwndDlg,
                              (pGlobalData->stickyKeys.dwFlags != pGlobalData->oldStickyKeys.dwFlags));
                    break;

                case IDCANCEL:
                    EndDialog(hwndDlg, FALSE);
                    break;

                default:
                    break;
            }
            break;
    }

    return FALSE;
}


static VOID
SetDlgItemTime(HWND hwnd, INT nId, INT nTimeMs)
{
    TCHAR szBuffer[32];
    TCHAR szSeconds[20];

    if (LoadString(hApplet, IDS_SECONDS, szSeconds, 20) == 0)
        lstrcpy(szSeconds, L"Seconds");

    _stprintf(szBuffer, _T("%d.%d %s"),
              nTimeMs / 1000, (nTimeMs % 1000) / 100,
              szSeconds);

    SetDlgItemText(hwnd, nId, szBuffer);
}


INT_PTR CALLBACK
BounceKeysDlgProc(HWND hwndDlg,
                  UINT uMsg,
                  WPARAM wParam,
                  LPARAM lParam)
{
    PGLOBAL_DATA pGlobalData;
    INT i;

    pGlobalData = (PGLOBAL_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pGlobalData = (PGLOBAL_DATA)lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);

            /* Set the range */
            SendDlgItemMessage(hwndDlg, IDC_BOUNCE_TIME_TRACK, TBM_SETRANGE,
                               TRUE, MAKELONG(0, BOUNCETICKS - 1));

            /* Determine the current thumb position */
            if (pGlobalData->filterKeys.iBounceMSec == 0)
                pGlobalData->filterKeys.iBounceMSec = nBounceArray[0];

            for (i = 0; i < BOUNCETICKS; i++)
            {
                 if (pGlobalData->filterKeys.iBounceMSec < nBounceArray[i])
                     break;
            }
            i--;

            /* Set the thumb position */
            SendDlgItemMessage(hwndDlg, IDC_BOUNCE_TIME_TRACK, TBM_SETPOS, TRUE, i);

            /* Set the bounce delay */
            SetDlgItemTime(hwndDlg, IDC_BOUNCE_TIME_EDIT, nBounceArray[i]);
            break;

        case WM_HSCROLL:
            switch (GetWindowLong((HWND) lParam, GWL_ID))
            {
                case IDC_BOUNCE_TIME_TRACK:
                    i = SendDlgItemMessage(hwndDlg, IDC_BOUNCE_TIME_TRACK, TBM_GETPOS, 0, 0);
                    if (i >= 0 && i < BOUNCETICKS)
                    {
                        /* Update the bounce delay */
                        pGlobalData->filterKeys.iBounceMSec = nBounceArray[i];
                        SetDlgItemTime(hwndDlg, IDC_BOUNCE_TIME_EDIT, nBounceArray[i]);
                    }
                    break;
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hwndDlg, TRUE);
                    break;

                case IDCANCEL:
                    EndDialog(hwndDlg, FALSE);
                    break;

                default:
                    break;
            }
            break;
    }

    return FALSE;
}


INT_PTR CALLBACK
RepeatKeysDlgProc(HWND hwndDlg,
                  UINT uMsg,
                  WPARAM wParam,
                  LPARAM lParam)
{
    PGLOBAL_DATA pGlobalData;

    pGlobalData = (PGLOBAL_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pGlobalData = (PGLOBAL_DATA)lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hwndDlg, TRUE);
                    break;

                case IDCANCEL:
                    EndDialog(hwndDlg, FALSE);
                    break;

                default:
                    break;
            }
            break;
    }

    return FALSE;
}


/* Property page dialog callback */
INT_PTR CALLBACK
FilterKeysDlgProc(HWND hwndDlg,
                  UINT uMsg,
                  WPARAM wParam,
                  LPARAM lParam)
{
    PGLOBAL_DATA pGlobalData;

    pGlobalData = (PGLOBAL_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pGlobalData = (PGLOBAL_DATA)lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);

            memcpy(&pGlobalData->oldFilterKeys,
                   &pGlobalData->filterKeys,
                   sizeof(FILTERKEYS));

            CheckDlgButton(hwndDlg,
                           IDC_FILTER_ACTIVATE_CHECK,
                           pGlobalData->filterKeys.dwFlags & FKF_HOTKEYACTIVE ? BST_CHECKED : BST_UNCHECKED);

            if (pGlobalData->filterKeys.iBounceMSec != 0)
            {
                CheckRadioButton(hwndDlg, IDC_FILTER_BOUNCE_RADIO, IDC_FILTER_REPEAT_RADIO, IDC_FILTER_BOUNCE_RADIO);
                EnableWindow(GetDlgItem(hwndDlg, IDC_FILTER_BOUNCE_BUTTON), TRUE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_FILTER_REPEAT_BUTTON), FALSE);
            }
            else
            {
                CheckRadioButton(hwndDlg, IDC_FILTER_BOUNCE_RADIO, IDC_FILTER_REPEAT_RADIO, IDC_FILTER_REPEAT_RADIO);
                EnableWindow(GetDlgItem(hwndDlg, IDC_FILTER_BOUNCE_BUTTON), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_FILTER_REPEAT_BUTTON), TRUE);
            }

            CheckDlgButton(hwndDlg,
                           IDC_FILTER_SOUND_CHECK,
                           pGlobalData->filterKeys.dwFlags & FKF_CLICKON ? BST_CHECKED : BST_UNCHECKED);

            CheckDlgButton(hwndDlg,
                           IDC_FILTER_STATUS_CHECK,
                           pGlobalData->filterKeys.dwFlags & FKF_INDICATOR ? BST_CHECKED : BST_UNCHECKED);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_FILTER_ACTIVATE_CHECK:
                    pGlobalData->filterKeys.dwFlags ^= FKF_HOTKEYACTIVE;
                    break;

                case IDC_FILTER_BOUNCE_RADIO:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_FILTER_BOUNCE_BUTTON), TRUE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_FILTER_REPEAT_BUTTON), FALSE);
                    break;

                case IDC_FILTER_REPEAT_RADIO:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_FILTER_BOUNCE_BUTTON), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_FILTER_REPEAT_BUTTON), TRUE);
                    break;

                case IDC_FILTER_BOUNCE_BUTTON:
                    DialogBoxParam(hApplet,
                                   MAKEINTRESOURCE(IDD_BOUNCEKEYSOPTIONS),
                                   hwndDlg,
                                   (DLGPROC)BounceKeysDlgProc,
                                   (LPARAM)pGlobalData);
                    break;

                case IDC_FILTER_SOUND_CHECK:
                    pGlobalData->filterKeys.dwFlags ^= FKF_CLICKON;
                    break;

                case IDC_FILTER_REPEAT_BUTTON:
                    DialogBoxParam(hApplet,
                                   MAKEINTRESOURCE(IDD_REPEATKEYSOPTIONS),
                                   hwndDlg,
                                   (DLGPROC)RepeatKeysDlgProc,
                                   (LPARAM)pGlobalData);
                    break;

                case IDC_FILTER_STATUS_CHECK:
                    pGlobalData->filterKeys.dwFlags ^= FKF_INDICATOR;
                    break;

                case IDOK:
                    EndDialog(hwndDlg,
                              (pGlobalData->filterKeys.dwFlags != pGlobalData->oldFilterKeys.dwFlags));
                    break;

                case IDCANCEL:
                    EndDialog(hwndDlg, FALSE);
                    break;

                default:
                    break;
            }
            break;
    }

    return FALSE;
}


/* Property page dialog callback */
INT_PTR CALLBACK
ToggleKeysDlgProc(HWND hwndDlg,
                  UINT uMsg,
                  WPARAM wParam,
                  LPARAM lParam)
{
    PGLOBAL_DATA pGlobalData;

    pGlobalData = (PGLOBAL_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pGlobalData = (PGLOBAL_DATA)lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);

            memcpy(&pGlobalData->oldToggleKeys,
                   &pGlobalData->toggleKeys,
                   sizeof(STICKYKEYS));

            CheckDlgButton(hwndDlg,
                           IDC_TOGGLE_ACTIVATE_CHECK,
                           pGlobalData->toggleKeys.dwFlags & TKF_HOTKEYACTIVE ? BST_CHECKED : BST_UNCHECKED);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_TOGGLE_ACTIVATE_CHECK:
                    pGlobalData->toggleKeys.dwFlags ^= TKF_HOTKEYACTIVE;
                    break;

                case IDOK:
                    EndDialog(hwndDlg,
                              (pGlobalData->toggleKeys.dwFlags != pGlobalData->oldToggleKeys.dwFlags));
                    break;

                case IDCANCEL:
                    EndDialog(hwndDlg, FALSE);
                    break;

                default:
                    break;
            }
            break;
    }

    return FALSE;
}


static VOID
OnInitDialog(IN HWND hwndDlg, PGLOBAL_DATA pGlobalData)
{
    /* Get sticky keys information */
    pGlobalData->stickyKeys.cbSize = sizeof(STICKYKEYS);
    if (!SystemParametersInfo(SPI_GETSTICKYKEYS,
                              sizeof(STICKYKEYS),
                              &pGlobalData->stickyKeys,
                              0))
    {
        return;
    }

    /* Get filter keys information */
    pGlobalData->filterKeys.cbSize = sizeof(FILTERKEYS);
    if (!SystemParametersInfo(SPI_GETFILTERKEYS,
                              sizeof(FILTERKEYS),
                              &pGlobalData->filterKeys,
                              0))
    {
        return;
    }

    /* Get toggle keys information */
    pGlobalData->toggleKeys.cbSize = sizeof(TOGGLEKEYS);
    if (!SystemParametersInfo(SPI_GETTOGGLEKEYS,
                              sizeof(TOGGLEKEYS),
                              &pGlobalData->toggleKeys,
                              0))
    {
        return;
    }

    /* Get keyboard preference information */
    if (!SystemParametersInfo(SPI_GETKEYBOARDPREF,
                              0,
                              &pGlobalData->bKeyboardPref,
                              0))
    {
        return;
    }

    CheckDlgButton(hwndDlg,
                   IDC_STICKY_BOX,
                   pGlobalData->stickyKeys.dwFlags & SKF_STICKYKEYSON ? BST_CHECKED : BST_UNCHECKED);

    CheckDlgButton(hwndDlg,
                   IDC_FILTER_BOX,
                   pGlobalData->filterKeys.dwFlags & FKF_FILTERKEYSON ? BST_CHECKED : BST_UNCHECKED);

    CheckDlgButton(hwndDlg,
                   IDC_TOGGLE_BOX,
                   pGlobalData->toggleKeys.dwFlags & TKF_TOGGLEKEYSON ? BST_CHECKED : BST_UNCHECKED);

    CheckDlgButton(hwndDlg,
                   IDC_KEYBOARD_EXTRA,
                   pGlobalData->bKeyboardPref ? BST_CHECKED : BST_UNCHECKED);
}


/* Property page dialog callback */
INT_PTR CALLBACK
KeyboardPageProc(HWND hwndDlg,
                 UINT uMsg,
                 WPARAM wParam,
                 LPARAM lParam)
{
    PGLOBAL_DATA pGlobalData;
    LPPSHNOTIFY lppsn;

    pGlobalData = (PGLOBAL_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pGlobalData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GLOBAL_DATA));
            if (pGlobalData == NULL)
                return FALSE;

            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);
            OnInitDialog(hwndDlg, pGlobalData);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_STICKY_BOX:
                    pGlobalData->stickyKeys.dwFlags ^= SKF_STICKYKEYSON;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_STICKY_BUTTON:
                    if (DialogBoxParam(hApplet,
                                       MAKEINTRESOURCE(IDD_STICKYKEYSOPTIONS),
                                       hwndDlg,
                                       (DLGPROC)StickyKeysDlgProc,
                                       (LPARAM)pGlobalData))
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_FILTER_BOX:
                    pGlobalData->filterKeys.dwFlags ^= FKF_FILTERKEYSON;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_FILTER_BUTTON:
                    if (DialogBoxParam(hApplet,
                                       MAKEINTRESOURCE(IDD_FILTERKEYSOPTIONS),
                                       hwndDlg,
                                       (DLGPROC)FilterKeysDlgProc,
                                       (LPARAM)pGlobalData))
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_TOGGLE_BOX:
                    pGlobalData->toggleKeys.dwFlags ^= TKF_TOGGLEKEYSON;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_TOGGLE_BUTTON:
                    if (DialogBoxParam(hApplet,
                                       MAKEINTRESOURCE(IDD_TOGGLEKEYSOPTIONS),
                                       hwndDlg,
                                       (DLGPROC)ToggleKeysDlgProc,
                                       (LPARAM)pGlobalData))
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_KEYBOARD_EXTRA:
                    pGlobalData->bKeyboardPref = !pGlobalData->bKeyboardPref;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                default:
                    break;
            }
            break;

        case WM_NOTIFY:
            lppsn = (LPPSHNOTIFY)lParam;
            if (lppsn->hdr.code == PSN_APPLY)
            {
                SystemParametersInfo(SPI_SETSTICKYKEYS,
                                     sizeof(STICKYKEYS),
                                     &pGlobalData->stickyKeys,
                                     SPIF_UPDATEINIFILE | SPIF_SENDCHANGE /*0*/);

                SystemParametersInfo(SPI_SETFILTERKEYS,
                                     sizeof(FILTERKEYS),
                                     &pGlobalData->filterKeys,
                                     SPIF_UPDATEINIFILE | SPIF_SENDCHANGE /*0*/);

                SystemParametersInfo(SPI_SETTOGGLEKEYS,
                                     sizeof(TOGGLEKEYS),
                                     &pGlobalData->toggleKeys,
                                     SPIF_UPDATEINIFILE | SPIF_SENDCHANGE /*0*/);

                SystemParametersInfo(SPI_SETKEYBOARDPREF,
                                     pGlobalData->bKeyboardPref,
                                     NULL,
                                     SPIF_UPDATEINIFILE | SPIF_SENDCHANGE /*0*/);

                return TRUE;
            }
            break;

        case WM_DESTROY:
            HeapFree(GetProcessHeap(), 0, pGlobalData);
            break;
    }

    return FALSE;
}
