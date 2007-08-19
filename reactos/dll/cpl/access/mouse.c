/* $Id$
 *
 * PROJECT:         ReactOS Accessibility Control Panel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/access/mouse.c
 * PURPOSE:         Mouse-related accessibility settings
 * COPYRIGHT:       Copyright 2004 Johannes Anderwald (j_anderw@sbox.tugraz.at)
 *                  Copyright 2007 Eric Kohl
 */

#include <windows.h>
#include <stdlib.h>
#include <commctrl.h>
#include <prsht.h>
#include <tchar.h>
#include "resource.h"
#include "access.h"

typedef struct _GLOBAL_DATA
{
    MOUSEKEYS mouseKeys;
} GLOBAL_DATA, *PGLOBAL_DATA;


#define SPEEDTICKS 9
#define ACCELTICKS 9

static INT nSpeedArray[SPEEDTICKS] = {10, 20, 30, 40, 60, 80, 120, 180, 360};


INT_PTR CALLBACK
MouseKeysDlgProc(HWND hwndDlg,
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

            CheckDlgButton(hwndDlg,
                           IDC_MOUSEKEYS_ACTIVATE_CHECK,
                           pGlobalData->mouseKeys.dwFlags & MKF_HOTKEYACTIVE ? BST_CHECKED : BST_UNCHECKED);

            /* Set the number of ticks for the speed trackbar */
            SendDlgItemMessage(hwndDlg, IDC_MOUSEKEYS_SPEED_TRACK, TBM_SETRANGE,
                               TRUE, MAKELONG(0, SPEEDTICKS - 1));

            /* Calculate the matching tick */
            for (i = 0; i < SPEEDTICKS; i++)
            {
                if (pGlobalData->mouseKeys.iMaxSpeed <= nSpeedArray[i])
                    break;
            }

            /* Set the thumb */
            SendDlgItemMessage(hwndDlg, IDC_MOUSEKEYS_SPEED_TRACK, TBM_SETPOS, TRUE, i);

            /* Set the number of ticks for the accelleration trackbar */
            SendDlgItemMessage(hwndDlg, IDC_MOUSEKEYS_ACCEL_TRACK, TBM_SETRANGE,
                               TRUE, MAKELONG(0, ACCELTICKS - 1));

            /* Calculate the matching tick */
            i = (ACCELTICKS + 1) - pGlobalData->mouseKeys.iTimeToMaxSpeed / 500;
            if (i > ACCELTICKS - 1)
                i = ACCELTICKS - 1;

            if (i < 0)
                i = 0;

            /* Set the thumb */
            SendDlgItemMessage(hwndDlg, IDC_MOUSEKEYS_ACCEL_TRACK, TBM_SETPOS, TRUE, i);

            CheckDlgButton(hwndDlg,
                           IDC_MOUSEKEYS_SPEED_CHECK,
                           pGlobalData->mouseKeys.dwFlags & MKF_MODIFIERS ? BST_CHECKED : BST_UNCHECKED);

            CheckRadioButton(hwndDlg,
                             IDC_MOUSEKEYS_ON_RADIO,
                             IDC_MOUSEKEYS_OFF_RADIO,
                             pGlobalData->mouseKeys.dwFlags & MKF_REPLACENUMBERS ? IDC_MOUSEKEYS_ON_RADIO : IDC_MOUSEKEYS_OFF_RADIO);

            CheckDlgButton(hwndDlg,
                           IDC_MOUSEKEYS_STATUS_CHECK,
                           pGlobalData->mouseKeys.dwFlags & MKF_INDICATOR ? BST_CHECKED : BST_UNCHECKED);
            break;

        case WM_HSCROLL:
            switch (GetWindowLong((HWND) lParam, GWL_ID))
            {
                case IDC_MOUSEKEYS_SPEED_TRACK:
                    i = SendDlgItemMessage(hwndDlg, IDC_MOUSEKEYS_SPEED_TRACK, TBM_GETPOS, 0, 0);
                    if (i >= 0 && i < SPEEDTICKS)
                        pGlobalData->mouseKeys.iMaxSpeed = nSpeedArray[i];
                    break;

                case IDC_MOUSEKEYS_ACCEL_TRACK:
                    i = SendDlgItemMessage(hwndDlg, IDC_MOUSEKEYS_ACCEL_TRACK, TBM_GETPOS, 0, 0);
                    if (i >= 0 && i < ACCELTICKS)
                        pGlobalData->mouseKeys.iTimeToMaxSpeed = (ACCELTICKS + 1 - i) * 500;
                    break;
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_MOUSEKEYS_ACTIVATE_CHECK:
                    pGlobalData->mouseKeys.dwFlags ^= MKF_HOTKEYACTIVE;
                    break;

                case IDC_MOUSEKEYS_SPEED_CHECK:
                    pGlobalData->mouseKeys.dwFlags ^= MKF_MODIFIERS;
                    break;

                case IDC_MOUSEKEYS_ON_RADIO:
                    pGlobalData->mouseKeys.dwFlags |= MKF_REPLACENUMBERS;
                    break;

                case IDC_MOUSEKEYS_OFF_RADIO:
                    pGlobalData->mouseKeys.dwFlags &= ~MKF_REPLACENUMBERS;
                    break;

                case IDC_MOUSEKEYS_STATUS_CHECK:
                    pGlobalData->mouseKeys.dwFlags ^= MKF_INDICATOR;
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
MousePageProc(HWND hwndDlg,
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

            /* Get mouse keys information */
            pGlobalData->mouseKeys.cbSize = sizeof(MOUSEKEYS);
            SystemParametersInfo(SPI_GETMOUSEKEYS,
                                 sizeof(MOUSEKEYS),
                                 &pGlobalData->mouseKeys,
                                 0);

            /* Set the checkbox */
            CheckDlgButton(hwndDlg,
                           IDC_MOUSE_BOX,
                           pGlobalData->mouseKeys.dwFlags & MKF_MOUSEKEYSON ? BST_CHECKED : BST_UNCHECKED);
            return TRUE;


        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_MOUSE_BOX:
                    pGlobalData->mouseKeys.dwFlags ^= MKF_MOUSEKEYSON;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_MOUSE_BUTTON:
                    if (DialogBoxParam(hApplet,
                                       MAKEINTRESOURCE(IDD_MOUSEKEYSOPTIONS),
                                       hwndDlg,
                                       (DLGPROC)MouseKeysDlgProc,
                                       (LPARAM)pGlobalData))
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
                SystemParametersInfo(SPI_SETMOUSEKEYS,
                                     sizeof(MOUSEKEYS),
                                     &pGlobalData->mouseKeys,
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
