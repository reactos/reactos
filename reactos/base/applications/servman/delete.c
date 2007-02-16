/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/servman/delete.c
 * PURPOSE:     Delete an existing service
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

static BOOL
DoDeleteService(PMAIN_WND_INFO Info,
                HWND hDlg)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hSc;

    /* open handle to the SCM */
    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL)
    {
        GetError();
        return FALSE;
    }

    /* get a handle to the service requested for deleting */
    hSc = OpenService(hSCManager,
                      Info->CurrentService->lpServiceName,
                      DELETE);
    if (hSc == NULL)
    {
        GetError();
        CloseServiceHandle(hSCManager);
        return FALSE;
    }

    /* delete the service opened */
    if (! DeleteService(hSc))
    {
        GetError();
        CloseServiceHandle(hSCManager);
        CloseServiceHandle(hSc);
        return FALSE;
    }


    CloseServiceHandle(hSCManager);
    CloseServiceHandle(hSc);

    return TRUE;
}


BOOL CALLBACK
DeleteDialogProc(HWND hDlg,
                 UINT message,
                 WPARAM wParam,
                 LPARAM lParam)
{
    PMAIN_WND_INFO Info = NULL;
    HICON hIcon = NULL;
    TCHAR Buf[1000];
    LVITEM item;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            Info = (PMAIN_WND_INFO)lParam;

            hIcon = (HICON) LoadImage(hInstance,
                              MAKEINTRESOURCE(IDI_SM_ICON),
                              IMAGE_ICON,
                              16,
                              16,
                              0);

            SendMessage(hDlg,
                        WM_SETICON,
                        ICON_SMALL,
                        (LPARAM)hIcon);

            SendDlgItemMessage(hDlg,
                               IDC_DEL_NAME,
                               WM_SETTEXT,
                               0,
                               (LPARAM)Info->CurrentService->lpDisplayName);


            item.mask = LVIF_TEXT;
            item.iItem = Info->SelectedItem;
            item.iSubItem = 1;
            item.pszText = Buf;
            item.cchTextMax = sizeof(Buf);
            SendMessage(Info->hListView,
                        LVM_GETITEM,
                        0,
                        (LPARAM)&item);

            SendDlgItemMessage(hDlg,
                               IDC_DEL_DESC,
                               WM_SETTEXT,
                               0,
                               (LPARAM)Buf);

            SetFocus(GetDlgItem(hDlg, IDCANCEL));

            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    if (DoDeleteService(Info, hDlg))
                        (void)ListView_DeleteItem(Info->hListView,
                                                  Info->SelectedItem);

                    DestroyIcon(hIcon);
                    EndDialog(hDlg,
                              LOWORD(wParam));
                    return TRUE;
                }

                case IDCANCEL:
                {
                    DestroyIcon(hIcon);
                    EndDialog(hDlg,
                              LOWORD(wParam));
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}
