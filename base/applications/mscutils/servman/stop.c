/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/servman/stop.c
 * PURPOSE:     Stops a service
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

BOOL DoStop(PMAIN_WND_INFO Info)
{
    HWND hProgDlg;
    TCHAR ProgDlgBuf[100];

    /* open the progress dialog */
    hProgDlg = CreateDialog(hInstance,
                            MAKEINTRESOURCE(IDD_DLG_PROGRESS),
                            Info->hMainWnd,
                            (DLGPROC)ProgressDialogProc);
    if (hProgDlg != NULL)
    {
        ShowWindow(hProgDlg,
                   SW_SHOW);

        /* write the  info to the progress dialog */
        LoadString(hInstance,
                   IDS_PROGRESS_INFO_STOP,
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
                           (LPARAM)Info->CurrentService->lpServiceName);
    }

    if ( Control(Info, SERVICE_CONTROL_STOP) )
    {
        LVITEM item;
        TCHAR buf[25];

        item.pszText = _T('\0');
        item.iItem = Info->SelectedItem;
        item.iSubItem = 2;
        SendMessage(Info->hListView,
                    LVM_SETITEMTEXT,
                    item.iItem,
                    (LPARAM) &item);

        /* change dialog status */
        if (Info->PropSheet != NULL)
        {
            LoadString(hInstance,
                       IDS_SERVICES_STOPPED,
                       buf,
                       sizeof(buf) / sizeof(TCHAR));

            SendDlgItemMessageW(Info->PropSheet->hwndGenDlg,
                                IDC_SERV_STATUS, WM_SETTEXT,
                                0,
                                (LPARAM)buf);
        }
    }

    SendMessage(hProgDlg, WM_DESTROY, 0, 0);

    return TRUE;
}
