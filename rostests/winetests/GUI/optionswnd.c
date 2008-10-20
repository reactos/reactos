/*
 * PROJECT:     ReactOS API Test GUI
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        
 * PURPOSE:     options dialog implementation
 * COPYRIGHT:   Copyright 2008 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include <precomp.h>

static BOOL
OnInitBrowseDialog(HWND hDlg,
                   LPARAM lParam)
{
    PMAIN_WND_INFO pInfo;

    pInfo = (PMAIN_WND_INFO)lParam;

    pInfo->hBrowseDlg = hDlg;

    SetWindowLongPtr(hDlg,
                     GWLP_USERDATA,
                     (LONG_PTR)pInfo);

    return TRUE;
}


BOOL CALLBACK
OptionsDlgProc(HWND hDlg,
              UINT Message,
              WPARAM wParam,
              LPARAM lParam)
{
    PMAIN_WND_INFO pInfo;

    //UNREFERENCED_PARAM(

    /* Get the window context */
    pInfo = (PMAIN_WND_INFO)GetWindowLongPtr(hDlg,
                                             GWLP_USERDATA);
    if (pInfo == NULL && Message != WM_INITDIALOG)
    {
        goto HandleDefaultMessage;
    }

    switch(Message)
    {
        case WM_INITDIALOG:
            return OnInitBrowseDialog(hDlg, lParam);

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    if (SendMessageW(GetDlgItem(hDlg, IDC_RUNONSTART), BM_GETCHECK, 0, 0) == BST_CHECKED)
                    {
                        pInfo->bRunOnStart = TRUE;
                    }
                    else
                    {
                        pInfo->bRunOnStart = FALSE;
                    }

                    if (SendMessageW(GetDlgItem(hDlg, IDC_HIDECONSOLE), BM_GETCHECK, 0, 0) == BST_CHECKED)
                    {
                        pInfo->bHideConsole = TRUE;
                    }
                    else
                    {
                        pInfo->bHideConsole = FALSE;
                    }

                    EndDialog(hDlg,
                              LOWORD(wParam));

                    return TRUE;
                }

                case IDCANCEL:
                {
                    EndDialog(hDlg,
                              LOWORD(wParam));

                    return TRUE;
                }
            }

            break;
        }
HandleDefaultMessage:
        default:
            return FALSE;
    }

    return FALSE;
}
