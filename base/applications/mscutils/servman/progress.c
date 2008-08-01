/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/progress.c
 * PURPOSE:     Progress dialog box message handler
 * COPYRIGHT:   Copyright 2006-2007 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

#define PROGRESSRANGE 20

VOID
CompleteProgressBar(HWND hProgDlg)
{
    HWND hProgBar;

    hProgBar = GetDlgItem(hProgDlg,
                          IDC_SERVCON_PROGRESS);

    if (hProgBar)
    {
        INT pos = 0;

        pos = SendMessage(hProgBar,
                          PBM_GETPOS,
                          0,
                          0);

        for (; pos <= PROGRESSRANGE; pos++)
        {
            SendMessage(hProgBar,
                        PBM_DELTAPOS,
                        pos,
                        0);
            Sleep(15);
        }
    }
}

VOID
IncrementProgressBar(HWND hProgDlg)
{
    HWND hProgBar;

    hProgBar = GetDlgItem(hProgDlg,
                          IDC_SERVCON_PROGRESS);

    if (hProgBar)
    {
        SendMessage(hProgBar,
                    PBM_STEPIT,
                    0,
                    0);
    }
}

BOOL CALLBACK
ProgressDialogProc(HWND hDlg,
                   UINT Message,
                   WPARAM wParam,
                   LPARAM lParam)
{
    switch(Message)
    {
        case WM_INITDIALOG:
        {
            HWND hProgBar;

            /* set the progress bar range and step */
            hProgBar = GetDlgItem(hDlg,
                                  IDC_SERVCON_PROGRESS);
            SendMessage(hProgBar,
                        PBM_SETRANGE,
                        0,
                        MAKELPARAM(0, PROGRESSRANGE));

            SendMessage(hProgBar,
                        PBM_SETSTEP,
                        (WPARAM)1,
                        0);
        }
        break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    DestroyWindow(hDlg);
                break;

            }
        break;

        default:
            return FALSE;
    }

    return TRUE;

}

HWND
CreateProgressDialog(HWND hParent,
                     LPTSTR lpServiceName,
                     UINT Event)
{
    HWND hProgDlg;
    TCHAR ProgDlgBuf[100];

    /* open the progress dialog */
    hProgDlg = CreateDialog(hInstance,
                            MAKEINTRESOURCE(IDD_DLG_PROGRESS),
                            hParent,
                            (DLGPROC)ProgressDialogProc);
    if (hProgDlg != NULL)
    {
        /* write the  info to the progress dialog */
        LoadString(hInstance,
                   Event,
                   ProgDlgBuf,
                   sizeof(ProgDlgBuf) / sizeof(TCHAR));

        SendDlgItemMessage(hProgDlg,
                           IDC_SERVCON_INFO,
                           WM_SETTEXT,
                           0,
                           (LPARAM)ProgDlgBuf);

        /* write the service name to the progress dialog */
        SendDlgItemMessage(hProgDlg,
                           IDC_SERVCON_NAME,
                           WM_SETTEXT,
                           0,
                           (LPARAM)lpServiceName);
    }

    return hProgDlg;
}
