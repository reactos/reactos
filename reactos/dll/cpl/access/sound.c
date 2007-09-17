/* $Id$
 *
 * PROJECT:         ReactOS Accessibility Control Panel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/access/sound.c
 * PURPOSE:         Sound-related acessibility settings
 * COPYRIGHT:       Copyright 2004 Johannes Anderwald (j_anderw@sbox.tugraz.at)
 *                  Copyright 2007 Eric Kohl
 */

#include <windows.h>
#include <commctrl.h>
#include <prsht.h>
#include <stdlib.h>
#include "resource.h"
#include "access.h"


static VOID
OnInitDialog(HWND hwndDlg, PGLOBAL_DATA pGlobalData)
{
    TCHAR szBuffer[256];
    UINT i;

    pGlobalData->ssSoundSentry.cbSize = sizeof(SOUNDSENTRY);
    SystemParametersInfo(SPI_GETSOUNDSENTRY,
                         sizeof(SOUNDSENTRY),
                         &pGlobalData->ssSoundSentry,
                         0);

    SystemParametersInfo(SPI_GETSHOWSOUNDS,
                         0,
                         &pGlobalData->bShowSounds,
                         0);

    /* Add strings to the combo-box */
    for (i = 0; i < 4; i++)
    {
        LoadString(hApplet, IDS_SENTRY_NONE + i, szBuffer, 256);
        SendDlgItemMessage(hwndDlg, IDC_SENTRY_COMBO, CB_ADDSTRING, 0, (LPARAM)szBuffer);
    }

    /* Select a combo-box item */
    SendDlgItemMessage(hwndDlg, IDC_SENTRY_COMBO, CB_SETCURSEL, pGlobalData->ssSoundSentry.iWindowsEffect, 0);

    /* Initialize SoundSentry settings */
    if (!(pGlobalData->ssSoundSentry.dwFlags & SSF_AVAILABLE))
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_SENTRY_BOX), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SENTRY_TEXT), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SENTRY_COMBO), FALSE);
    }
    else
    {
        if (pGlobalData->ssSoundSentry.dwFlags & SSF_SOUNDSENTRYON)
        {
            CheckDlgButton(hwndDlg, IDC_SENTRY_BOX, BST_CHECKED);
        }
        else
        {
            EnableWindow(GetDlgItem(hwndDlg, IDC_SENTRY_TEXT), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SENTRY_COMBO), FALSE);
        }
    }

    /* Initialize ShowSounds settings */
    if (pGlobalData->bShowSounds)
        CheckDlgButton(hwndDlg, IDC_SSHOW_BOX, BST_CHECKED);
}


/* Property page dialog callback */
INT_PTR CALLBACK
SoundPageProc(HWND hwndDlg,
              UINT uMsg,
              WPARAM wParam,
              LPARAM lParam)
{
    PGLOBAL_DATA pGlobalData;

    pGlobalData = (PGLOBAL_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pGlobalData = (PGLOBAL_DATA)((LPPROPSHEETPAGE)lParam)->lParam;

            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);

            OnInitDialog(hwndDlg, pGlobalData);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_SENTRY_BOX:
                    pGlobalData->ssSoundSentry.dwFlags ^= SSF_SOUNDSENTRYON;
                    EnableWindow(GetDlgItem(hwndDlg, IDC_SENTRY_TEXT), (pGlobalData->ssSoundSentry.dwFlags & SSF_SOUNDSENTRYON)?TRUE:FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_SENTRY_COMBO), (pGlobalData->ssSoundSentry.dwFlags & SSF_SOUNDSENTRYON)?TRUE:FALSE);
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_SENTRY_COMBO:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        pGlobalData->ssSoundSentry.iWindowsEffect =
                            (DWORD)SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;

                case IDC_SSHOW_BOX:
                    pGlobalData->bShowSounds = !pGlobalData->bShowSounds;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                default:
                    break;
            }
            break;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case PSN_APPLY:
                    SystemParametersInfo(SPI_SETSOUNDSENTRY,
                                         sizeof(SOUNDSENTRY),
                                         &pGlobalData->ssSoundSentry,
                                         SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
                    SystemParametersInfo(SPI_SETSHOWSOUNDS,
                                         pGlobalData->bShowSounds,
                                         0,
                                         SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
                    return TRUE;

                default:
                    break;
            }
            break;
    }

    return FALSE;
}
