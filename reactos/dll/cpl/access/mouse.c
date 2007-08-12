/* $Id$
 *
 * PROJECT:         ReactOS System Control Panel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/access/mouse.c
 * PURPOSE:         Memory, start-up and profiles settings
 * COPYRIGHT:       Copyright 2004 Johannes Anderwald (j_anderw@sbox.tugraz.at)
 * UPDATE HISTORY:
 *      03-04-2004  Created
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


INT_PTR CALLBACK
MouseKeysDlgProc(HWND hwndDlg,
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
