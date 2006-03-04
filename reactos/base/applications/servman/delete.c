/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/servman/delete.c
 * PURPOSE:     Delete an existing service
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "servman.h"

extern HINSTANCE hInstance;
extern HWND hListView;

BOOL DoDeleteService(HWND hDlg)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hSc;
    ENUM_SERVICE_STATUS_PROCESS *Service = NULL;

    /* open handle to the SCM */
    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL)
    {
        GetError(0);
        return FALSE;
    }

    /* copy pointer to selected service */
    Service = GetSelectedService();

    /* get a handle to the service requested for starting */
    hSc = OpenService(hSCManager, Service->lpServiceName, DELETE);
    if (hSc == NULL)
    {
        GetError(0);
        return FALSE;
    }

    /* start the service opened */
    if (! DeleteService(hSc))
    {
        GetError(0);
        return FALSE;
    }


    CloseServiceHandle(hSCManager);
    CloseServiceHandle(hSc);

    return TRUE;
}


#ifdef _MSC_VER
#pragma warning(disable : 4100)
#endif
BOOL CALLBACK
DeleteDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    ENUM_SERVICE_STATUS_PROCESS *Service = NULL;
    HICON hIcon = NULL;
    TCHAR Buf[1000];
    LVITEM item;

    switch (message)
    {
    case WM_INITDIALOG:

        hIcon = LoadImage(hInstance, MAKEINTRESOURCE(IDI_SM_ICON), IMAGE_ICON, 16, 16, 0);
        SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

        /* get pointer to selected service */
        Service = GetSelectedService();

        SendDlgItemMessage(hDlg, IDC_DEL_NAME, WM_SETTEXT, 0, (LPARAM)Service->lpDisplayName);


        item.mask = LVIF_TEXT;
        item.iItem = GetSelectedItem();
        item.iSubItem = 1;
        item.pszText = Buf;
        item.cchTextMax = sizeof(Buf);
        SendMessage(hListView, LVM_GETITEM, 0, (LPARAM)&item);

        SendDlgItemMessage(hDlg, IDC_DEL_DESC, WM_SETTEXT, 0,
                (LPARAM)Buf);

        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            case IDOK:
                if (DoDeleteService(hDlg))
                    (void)ListView_DeleteItem(hListView, GetSelectedItem());

                DestroyIcon(hIcon);
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;

            case IDCANCEL:
                DestroyIcon(hIcon);
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
        }
    }

    return FALSE;
}
