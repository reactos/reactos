/* $Id$
 *
 * PROJECT:         ReactOS System Control Panel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            lib/cpl/system/advanced.c
 * PURPOSE:         Memory, start-up and profiles settings
 * COPYRIGHT:       Copyright 2004 Johannes Anderwald (j_anderw@sbox.tugraz.at)
 * UPDATE HISTORY:
 *      03-04-2004  Created
 */
#include <windows.h>
#include <stdlib.h>
#include <commctrl.h>
#include <prsht.h>
#include "resource.h"
#include "access.h"

typedef struct _GLOBAL_DATA
{
    STICKYKEYS stickyKeys;
    FILTERKEYS filterKeys;
    TOGGLEKEYS toggleKeys;
} GLOBAL_DATA, *PGLOBAL_DATA;


/* Property page dialog callback */
INT_PTR CALLBACK
StickyKeysDlgProc(HWND hwndDlg,
                  UINT uMsg,
                  WPARAM wParam,
                  LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
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


static VOID
OnInitDialog(IN HWND hwndDlg)
{
    PGLOBAL_DATA pGlobalData;

    pGlobalData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GLOBAL_DATA));
    if (pGlobalData == NULL)
        return;

    SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);

    /* Get sticky keys information */
    pGlobalData->stickyKeys.cbSize = sizeof(STICKYKEYS);
    if (!SystemParametersInfo(SPI_GETSTICKYKEYS,
                              sizeof(STICKYKEYS),
                              &pGlobalData->stickyKeys,
                              0))
    {
        return;
    }

    if (pGlobalData->stickyKeys.dwFlags & SKF_STICKYKEYSON)
        SendDlgItemMessage(hwndDlg, IDC_STICKY_BOX, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);

    /* Get filter keys information */
    pGlobalData->filterKeys.cbSize = sizeof(FILTERKEYS);
    if (!SystemParametersInfo(SPI_GETFILTERKEYS,
                              sizeof(FILTERKEYS),
                              &pGlobalData->filterKeys,
                              0))
    {
        return;
    }

    if (pGlobalData->filterKeys.dwFlags & FKF_FILTERKEYSON)
        SendDlgItemMessage(hwndDlg, IDC_FILTER_BOX, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);

    /* Get toggle keys information */
    pGlobalData->toggleKeys.cbSize = sizeof(TOGGLEKEYS);
    if (!SystemParametersInfo(SPI_GETTOGGLEKEYS,
                              sizeof(TOGGLEKEYS),
                              &pGlobalData->toggleKeys,
                              0))
    {
        return;
    }

    if (pGlobalData->toggleKeys.dwFlags & TKF_TOGGLEKEYSON)
        SendDlgItemMessage(hwndDlg, IDC_TOGGLE_BOX, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
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
            OnInitDialog(hwndDlg);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_STICKY_BOX:
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
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_FILTER_BUTTON:
                    break;

                case IDC_TOGGLE_BOX:
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_TOGGLE_BUTTON:
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

                return TRUE;
            }
            break;

        case WM_DESTROY:
            HeapFree(GetProcessHeap(), 0, pGlobalData);
            break;
    }

    return FALSE;
}
