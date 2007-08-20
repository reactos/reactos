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


typedef struct _SOUNDDATA
{
    SOUNDSENTRY ssSoundSentry;
    BOOL bShowSounds;
} SOUNDDATA, *PSOUNDDATA;


static VOID
OnInitDialog(HWND hwndDlg, PSOUNDDATA pSoundData)
{
    TCHAR szBuffer[256];
    UINT i;

    pSoundData->ssSoundSentry.cbSize = sizeof(SOUNDSENTRY);
    SystemParametersInfo(SPI_GETSOUNDSENTRY,
                         sizeof(SOUNDSENTRY),
                         &pSoundData->ssSoundSentry,
                         0);

    SystemParametersInfo(SPI_GETSHOWSOUNDS,
                         0,
                         &pSoundData->bShowSounds,
                         0);

    /* Add strings to the combo-box */
    for (i = 0; i < 4; i++)
    {
        LoadString(hApplet, IDS_SENTRY_NONE + i, szBuffer, 256);
        SendDlgItemMessage(hwndDlg, IDC_SENTRY_COMBO, CB_ADDSTRING, 0, (LPARAM)szBuffer);
    }

    /* Select a combo-box item */
    SendDlgItemMessage(hwndDlg, IDC_SENTRY_COMBO, CB_SETCURSEL, pSoundData->ssSoundSentry.iWindowsEffect, 0);

    /* Initialize SoundSentry settings */
    if (!(pSoundData->ssSoundSentry.dwFlags & SSF_AVAILABLE))
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_SENTRY_BOX), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SENTRY_TEXT), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SENTRY_COMBO), FALSE);
    }
    else
    {
        if (pSoundData->ssSoundSentry.dwFlags & SSF_SOUNDSENTRYON)
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
    if (pSoundData->bShowSounds)
        CheckDlgButton(hwndDlg, IDC_SSHOW_BOX, BST_CHECKED);
}


/* Property page dialog callback */
INT_PTR CALLBACK
SoundPageProc(HWND hwndDlg,
              UINT uMsg,
              WPARAM wParam,
              LPARAM lParam)
{
    PSOUNDDATA pSoundData;

    pSoundData = (PSOUNDDATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pSoundData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SOUNDDATA));
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pSoundData);

            OnInitDialog(hwndDlg, pSoundData);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_SENTRY_BOX:
                    pSoundData->ssSoundSentry.dwFlags ^= SSF_SOUNDSENTRYON;
                    EnableWindow(GetDlgItem(hwndDlg, IDC_SENTRY_TEXT), (pSoundData->ssSoundSentry.dwFlags & SSF_SOUNDSENTRYON)?TRUE:FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_SENTRY_COMBO), (pSoundData->ssSoundSentry.dwFlags & SSF_SOUNDSENTRYON)?TRUE:FALSE);
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_SENTRY_COMBO:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        pSoundData->ssSoundSentry.iWindowsEffect =
                            (DWORD)SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;

                case IDC_SSHOW_BOX:
                    pSoundData->bShowSounds = !pSoundData->bShowSounds;
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
                                         &pSoundData->ssSoundSentry,
                                         SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
                    SystemParametersInfo(SPI_SETSHOWSOUNDS,
                                         pSoundData->bShowSounds,
                                         0,
                                         SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
                    return TRUE;

                default:
                    break;
            }
            break;

        case WM_DESTROY:
            HeapFree(GetProcessHeap(), 0, pSoundData);
            break;
    }

    return FALSE;
}
