/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/progress.c
 * PURPOSE:     Progress dialog box message handler
 * COPYRIGHT:   Copyright 2006-2010 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

#define PROGRESSRANGE 20

VOID
CompleteProgressBar(HWND hProgDlg)
{
    HWND hProgBar;
    UINT Pos = 0;

    /* Get a handle to the progress bar */
    hProgBar = GetDlgItem(hProgDlg,
                          IDC_SERVCON_PROGRESS);
    if (hProgBar)
    {
        /* Get the current position */
        Pos = SendMessageW(hProgBar,
                           PBM_GETPOS,
                           0,
                           0);

        /* Loop until we hit the max */
        while (Pos <= PROGRESSRANGE)
        {
            /* Increment the progress bar */
            SendMessageW(hProgBar,
                         PBM_DELTAPOS,
                         Pos,
                         0);

            /* Wait for 15ms, it gives it a smooth feel */
            Sleep(15);
            Pos++;
        }
    }
}

VOID
IncrementProgressBar(HWND hProgDlg,
                     UINT NewPos)
{
    HWND hProgBar;

    /* Get a handle to the progress bar */
    hProgBar = GetDlgItem(hProgDlg,
                          IDC_SERVCON_PROGRESS);
    if (hProgBar)
    {
        /* Do we want to increment the default amount? */
        if (NewPos == DEFAULT_STEP)
        {
            /* Yes, use the step value we set on create */
            SendMessageW(hProgBar,
                         PBM_STEPIT,
                         0,
                         0);
        }
        else
        {
            /* No, use the value passed */
            SendMessageW(hProgBar,
                         PBM_SETPOS,
                         NewPos,
                         0);
        }
    }
}

VOID
InitializeProgressDialog(HWND hProgDlg,
                         LPWSTR lpServiceName)
{
    /* Write the service name to the dialog */
    SendDlgItemMessageW(hProgDlg,
                        IDC_SERVCON_NAME,
                        WM_SETTEXT,
                        0,
                        (LPARAM)lpServiceName);

    /* Set the progress bar to the start */
    SendDlgItemMessageW(hProgDlg,
                        IDC_SERVCON_PROGRESS,
                        PBM_SETPOS,
                        0,
                        0);
}

INT_PTR CALLBACK
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

            /* Get a handle to the progress bar */
            hProgBar = GetDlgItem(hDlg,
                                  IDC_SERVCON_PROGRESS);

            /* Set the progress bar range */
            SendMessageW(hProgBar,
                         PBM_SETRANGE,
                         0,
                         MAKELPARAM(0, PROGRESSRANGE));

            /* Set the progress bar step */
            SendMessageW(hProgBar,
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
                     UINT LabelId)
{
    HWND hProgDlg;
    LPWSTR lpProgStr;

    /* open the progress dialog */
    hProgDlg = CreateDialogW(hInstance,
                             MAKEINTRESOURCEW(IDD_DLG_PROGRESS),
                             hParent,
                             ProgressDialogProc);
    if (hProgDlg != NULL)
    {
        /* Load the label Id */
        if (AllocAndLoadString(&lpProgStr,
                               hInstance,
                               LabelId))
        {
            /* Write it to the dialog */
            SendDlgItemMessageW(hProgDlg,
                                IDC_SERVCON_INFO,
                                WM_SETTEXT,
                                0,
                                (LPARAM)lpProgStr);

            HeapFree(GetProcessHeap(),
                     0,
                     lpProgStr);
        }
    }

    return hProgDlg;
}

BOOL
DestroyProgressDialog(HWND hwnd,
                      BOOL bComplete)
{
    BOOL bRet = FALSE;

    if (hwnd)
    {
        if (bComplete)
        {
            /* Complete the progress bar */
            CompleteProgressBar(hwnd);

            /* Wait, for asthetics */
            Sleep(500);
        }

        bRet = DestroyWindow(hwnd);
    }

    return bRet;
}
