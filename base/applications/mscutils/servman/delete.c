/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/delete.c
 * PURPOSE:     Delete an existing service
 * COPYRIGHT:   Copyright 2006-2007 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

static BOOL
DoDeleteService(PMAIN_WND_INFO Info,
                HWND hDlg)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hSc;
    BOOL bRet = FALSE;

    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_ALL_ACCESS);
    if (hSCManager)
    {
        hSc = OpenService(hSCManager,
                          Info->pCurrentService->lpServiceName,
                          DELETE);
        if (hSc)
        {
            if (DeleteService(hSc))
            {
                LPTSTR lpSuccess;

                /* report success to user */
                if (AllocAndLoadString(&lpSuccess,
                                       hInstance,
                                       IDS_DELETE_SUCCESS))
                {
                    DisplayString(lpSuccess);

                    HeapFree(ProcessHeap,
                             0,
                             lpSuccess);
                }

                bRet = TRUE;
            }

            CloseServiceHandle(hSc);
        }

        CloseServiceHandle(hSCManager);
    }

    return bRet;
}


BOOL CALLBACK
DeleteDialogProc(HWND hDlg,
                 UINT message,
                 WPARAM wParam,
                 LPARAM lParam)
{
    PMAIN_WND_INFO Info = NULL;
    HICON hIcon = NULL;

    /* Get the window context */
    Info = (PMAIN_WND_INFO)GetWindowLongPtr(hDlg,
                                            GWLP_USERDATA);
    if (Info == NULL && message != WM_INITDIALOG)
    {
        return FALSE;
    }

    switch (message)
    {
        case WM_INITDIALOG:
        {
            LPTSTR lpDescription;

            Info = (PMAIN_WND_INFO)lParam;
            if (Info != NULL)
            {
                SetWindowLongPtr(hDlg,
                                 GWLP_USERDATA,
                                 (LONG_PTR)Info);

                hIcon = (HICON)LoadImage(hInstance,
                                         MAKEINTRESOURCE(IDI_SM_ICON),
                                         IMAGE_ICON,
                                         16,
                                         16,
                                         0);
                if (hIcon)
                {
                    SendMessage(hDlg,
                                WM_SETICON,
                                ICON_SMALL,
                                (LPARAM)hIcon);
                    DestroyIcon(hIcon);
                }

                SendDlgItemMessage(hDlg,
                                   IDC_DEL_NAME,
                                   WM_SETTEXT,
                                   0,
                                   (LPARAM)Info->pCurrentService->lpDisplayName);

                lpDescription = GetServiceDescription(Info->pCurrentService->lpServiceName);
                if (lpDescription)
                {
                    SendDlgItemMessage(hDlg,
                                       IDC_DEL_DESC,
                                       WM_SETTEXT,
                                       0,
                                       (LPARAM)lpDescription);
                    HeapFree(ProcessHeap,
                             0,
                             lpDescription);
                }

                return TRUE;
            }

            return FALSE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    if (DoDeleteService(Info, hDlg))
                    {
                        (void)ListView_DeleteItem(Info->hListView,
                                                  Info->SelectedItem);
                        UpdateServiceCount(Info);
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
        }
    }

    return FALSE;
}
