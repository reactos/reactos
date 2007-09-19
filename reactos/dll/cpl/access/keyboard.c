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


#define BOUNCETICKS 5
static INT nBounceArray[BOUNCETICKS] = {500, 700, 1000, 1500, 2000};

#define DELAYTICKS 5
static INT nDelayArray[DELAYTICKS] = {300, 700, 1000, 1500, 2000};

#define REPEATTICKS 6
static INT nRepeatArray[REPEATTICKS] = {300, 500, 700, 1000, 1500, 2000};

#define WAITTICKS 10
static INT nWaitArray[WAITTICKS] = {0, 300, 500, 700, 1000, 1500, 2000, 5000, 10000, 20000};


static VOID
EnableFilterKeysTest(PGLOBAL_DATA pGlobalData)
{
    pGlobalData->filterKeys.dwFlags |= FKF_FILTERKEYSON;
    pGlobalData->filterKeys.dwFlags &= ~FKF_INDICATOR;

    SystemParametersInfo(SPI_SETFILTERKEYS,
                         sizeof(FILTERKEYS),
                         &pGlobalData->filterKeys,
                         0);
}


static VOID
DisableFilterKeysTest(PGLOBAL_DATA pGlobalData)
{
    if (pGlobalData->oldFilterKeys.dwFlags & FKF_FILTERKEYSON)
    {
        pGlobalData->filterKeys.dwFlags |= FKF_FILTERKEYSON;
    }
    else
    {
        pGlobalData->filterKeys.dwFlags &= ~FKF_FILTERKEYSON;
    }

    if (pGlobalData->oldFilterKeys.dwFlags & FKF_INDICATOR)
    {
        pGlobalData->filterKeys.dwFlags |= FKF_INDICATOR;
    }
    else
    {
        pGlobalData->filterKeys.dwFlags &= ~FKF_INDICATOR;
    }

    SystemParametersInfo(SPI_SETFILTERKEYS,
                         sizeof(FILTERKEYS),
                         &pGlobalData->filterKeys,
                         0);
}


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
AddComboBoxTime(HWND hwnd, INT nId, INT nTimeMs)
{
    TCHAR szBuffer[32];
    TCHAR szSeconds[20];

    if (LoadString(hApplet, IDS_SECONDS, szSeconds, 20) == 0)
        lstrcpy(szSeconds, L"Seconds");

    _stprintf(szBuffer, _T("%d.%d %s"),
              nTimeMs / 1000, (nTimeMs % 1000) / 100,
              szSeconds);

    SendDlgItemMessage(hwnd, nId, CB_ADDSTRING, 0, (LPARAM)szBuffer);
}


INT_PTR CALLBACK
BounceKeysDlgProc(HWND hwndDlg,
                  UINT uMsg,
                  WPARAM wParam,
                  LPARAM lParam)
{
    PGLOBAL_DATA pGlobalData;
    INT i, n;

    pGlobalData = (PGLOBAL_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pGlobalData = (PGLOBAL_DATA)lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);

            /* Determine the current bounce time */
            if (pGlobalData->filterKeys.iBounceMSec == 0)
                pGlobalData->filterKeys.iBounceMSec = nBounceArray[0];

            for (n = 0; n < BOUNCETICKS; n++)
            {
                 if (pGlobalData->filterKeys.iBounceMSec < nBounceArray[n])
                     break;
            }
            n--;

            for (i = 0; i < BOUNCETICKS; i++)
            {
                 AddComboBoxTime(hwndDlg, IDC_BOUNCE_TIME_COMBO, nBounceArray[i]);
            }

            SendDlgItemMessage(hwndDlg, IDC_BOUNCE_TIME_COMBO, CB_SETCURSEL, n, 0);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_BOUNCE_TIME_COMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        i = SendDlgItemMessage(hwndDlg, IDC_BOUNCE_TIME_COMBO, CB_GETCURSEL, 0, 0);
                        if (i != CB_ERR)
                        {
                            pGlobalData->filterKeys.iBounceMSec = nBounceArray[i];
                        }
                    }
                    break;

                case IDC_BOUNCE_TEST_EDIT:
                    switch (HIWORD(wParam))
                    {
                        case EN_SETFOCUS:
                            EnableFilterKeysTest(pGlobalData);
                            break;

                        case EN_KILLFOCUS:
                            DisableFilterKeysTest(pGlobalData);
                            break;
                    }
                    break;

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
    INT i, n;

    pGlobalData = (PGLOBAL_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pGlobalData = (PGLOBAL_DATA)lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);

            CheckRadioButton(hwndDlg,
                             IDC_REPEAT_NOREPEAT_RADIO,
                             IDC_REPEAT_REPEAT_RADIO,
                             (pGlobalData->filterKeys.iDelayMSec == 0) ? IDC_REPEAT_NOREPEAT_RADIO : IDC_REPEAT_REPEAT_RADIO);

            /* Initialize the delay combobox */
            for (n = 0; n < DELAYTICKS; n++)
            {
                 if (pGlobalData->filterKeys.iDelayMSec < nDelayArray[n])
                     break;
            }
            n--;

            for (i = 0; i < DELAYTICKS; i++)
            {
                 AddComboBoxTime(hwndDlg, IDC_REPEAT_DELAY_COMBO, nDelayArray[i]);
            }

            SendDlgItemMessage(hwndDlg, IDC_REPEAT_DELAY_COMBO, CB_SETCURSEL, n, 0);

            /* Initialize the repeat combobox */
            for (n = 0; n < REPEATTICKS; n++)
            {
                 if (pGlobalData->filterKeys.iRepeatMSec < nRepeatArray[n])
                     break;
            }
            n--;

            for (i = 0; i < REPEATTICKS; i++)
            {
                 AddComboBoxTime(hwndDlg, IDC_REPEAT_REPEAT_COMBO, nRepeatArray[i]);
            }

            SendDlgItemMessage(hwndDlg, IDC_REPEAT_REPEAT_COMBO, CB_SETCURSEL, n, 0);

            /* Disable the delay and repeat comboboxes if needed */
            if (pGlobalData->filterKeys.iDelayMSec == 0)
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_REPEAT_DELAY_COMBO), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_REPEAT_REPEAT_COMBO), FALSE);
            }

            /* Initialize the wait combobox */
            for (n = 0; n < WAITTICKS; n++)
            {
                 if (pGlobalData->filterKeys.iWaitMSec < nWaitArray[n])
                     break;
            }
            n--;

            for (i = 0; i < WAITTICKS; i++)
            {
                 AddComboBoxTime(hwndDlg, IDC_REPEAT_WAIT_COMBO, nWaitArray[i]);
            }

            SendDlgItemMessage(hwndDlg, IDC_REPEAT_WAIT_COMBO, CB_SETCURSEL, n, 0);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_REPEAT_NOREPEAT_RADIO:
                    pGlobalData->filterKeys.iDelayMSec = 0;
                    pGlobalData->filterKeys.iRepeatMSec = 0;
                    EnableWindow(GetDlgItem(hwndDlg, IDC_REPEAT_DELAY_COMBO), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_REPEAT_REPEAT_COMBO), FALSE);
                    break;

                case IDC_REPEAT_REPEAT_RADIO:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_REPEAT_DELAY_COMBO), TRUE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_REPEAT_REPEAT_COMBO), TRUE);
                    break;

                case IDC_REPEAT_DELAY_COMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        i = SendDlgItemMessage(hwndDlg, IDC_REPEAT_DELAY_COMBO, CB_GETCURSEL, 0, 0);
                        if (i != CB_ERR)
                        {
                            pGlobalData->filterKeys.iDelayMSec = nDelayArray[i];
                        }
                    }
                    break;

                case IDC_REPEAT_REPEAT_COMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        i = SendDlgItemMessage(hwndDlg, IDC_REPEAT_REPEAT_COMBO, CB_GETCURSEL, 0, 0);
                        if (i != CB_ERR)
                        {
                            pGlobalData->filterKeys.iRepeatMSec = nRepeatArray[i];
                        }
                    }
                    break;

                case IDC_REPEAT_WAIT_COMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        i = SendDlgItemMessage(hwndDlg, IDC_REPEAT_WAIT_COMBO, CB_GETCURSEL, 0, 0);
                        if (i != CB_ERR)
                        {
                            pGlobalData->filterKeys.iWaitMSec = nWaitArray[i];
                        }
                    }
                    break;

                case IDC_REPEAT_TEST_EDIT:
                    switch (HIWORD(wParam))
                    {
                        case EN_SETFOCUS:
                            EnableFilterKeysTest(pGlobalData);
                            break;

                        case EN_KILLFOCUS:
                            DisableFilterKeysTest(pGlobalData);
                            break;
                    }
                    break;

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

                case IDC_FILTER_TEST_EDIT:
                    switch (HIWORD(wParam))
                    {
                        case EN_SETFOCUS:
                            EnableFilterKeysTest(pGlobalData);
                            break;

                        case EN_KILLFOCUS:
                            DisableFilterKeysTest(pGlobalData);
                            break;
                    }
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
            pGlobalData = (PGLOBAL_DATA)((LPPROPSHEETPAGE)lParam)->lParam;
            if (pGlobalData == NULL)
                return FALSE;

            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);

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
    }

    return FALSE;
}
