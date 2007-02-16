/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/servman/start.c
 * PURPOSE:     Start a service
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

static BOOL
DoStartService(PMAIN_WND_INFO Info)
{
    HWND hProgBar;
    SC_HANDLE hSCManager;
    SC_HANDLE hSc;
    SERVICE_STATUS_PROCESS ServiceStatus;
    DWORD BytesNeeded = 0;
    INT ArgCount = 0;
    DWORD dwStartTickCount, dwOldCheckPoint;


    /* set the progress bar range and step */
    hProgBar = GetDlgItem(Info->hProgDlg,
                          IDC_SERVCON_PROGRESS);

    SendMessage(hProgBar,
                PBM_SETRANGE,
                0,
                MAKELPARAM(0, PROGRESSRANGE));

    SendMessage(hProgBar,
                PBM_SETSTEP,
                (WPARAM)1,
                0);

    /* open handle to the SCM */
    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL)
    {
        GetError();
        return FALSE;
    }

    /* get a handle to the service requested for starting */
    hSc = OpenService(hSCManager,
                      Info->CurrentService->lpServiceName,
                      SERVICE_ALL_ACCESS);
    if (hSc == NULL)
    {
        GetError();
        return FALSE;
    }

    /* start the service opened */
    if (! StartService(hSc,
                       ArgCount,
                       NULL))
    {
        GetError();
        return FALSE;
    }

    /* query the state of the service */
    if (! QueryServiceStatusEx(hSc,
                               SC_STATUS_PROCESS_INFO,
                               (LPBYTE)&ServiceStatus,
                               sizeof(SERVICE_STATUS_PROCESS),
                               &BytesNeeded))
    {
        GetError();
        return FALSE;
    }

    /* Save the tick count and initial checkpoint. */
    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ServiceStatus.dwCheckPoint;

    /* loop whilst service is not running */
    /* FIXME: needs more control adding. 'Loop' is temparary */
    while (ServiceStatus.dwCurrentState != SERVICE_RUNNING)
    {
        DWORD dwWaitTime;

        dwWaitTime = ServiceStatus.dwWaitHint / 10;

        if( dwWaitTime < 500 )
            dwWaitTime = 500;
        else if ( dwWaitTime > 5000 )
            dwWaitTime = 5000;

        /* increment the progress bar */
        SendMessage(hProgBar, PBM_STEPIT, 0, 0);

        /* wait before checking status */
        Sleep(ServiceStatus.dwWaitHint / 8);

        /* check status again */
        if (! QueryServiceStatusEx(hSc,
                                   SC_STATUS_PROCESS_INFO,
                                   (LPBYTE)&ServiceStatus,
                                   sizeof(SERVICE_STATUS_PROCESS),
                                   &BytesNeeded))
        {
            GetError();
            return FALSE;
        }

        if (ServiceStatus.dwCheckPoint > dwOldCheckPoint)
        {
            /* The service is making progress. increment the progress bar */
            SendMessage(hProgBar, PBM_STEPIT, 0, 0);
            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ServiceStatus.dwCheckPoint;
        }
        else
        {
            if(GetTickCount() - dwStartTickCount > ServiceStatus.dwWaitHint)
            {
                /* No progress made within the wait hint */
                break;
            }
        }
    }

    CloseServiceHandle(hSc);

    if (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
    {
        SendMessage(hProgBar,
                    PBM_DELTAPOS,
                    PROGRESSRANGE,
                    0);
        Sleep(1000);
        return TRUE;
    }
    else
        return FALSE;

}




BOOL
DoStart(PMAIN_WND_INFO Info)
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

        /* write the info to the progress dialog */
        LoadString(hInstance,
                   IDS_PROGRESS_INFO_START,
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

    /* start the service */
    if ( DoStartService(Info) )
    {
        LVITEM item;
        TCHAR szStatus[64];
        TCHAR buf[25];

        LoadString(hInstance,
                   IDS_SERVICES_STARTED,
                   szStatus,
                   sizeof(szStatus) / sizeof(TCHAR));
        item.pszText = szStatus;
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
                      IDS_SERVICES_STARTED,
                      buf,
                      sizeof(buf) / sizeof(TCHAR));

            SendDlgItemMessageW(Info->PropSheet->hwndGenDlg,
                                IDC_SERV_STATUS,
                                WM_SETTEXT,
                                0,
                                (LPARAM)buf);
        }
    }

    SendMessage(hProgDlg,
                WM_DESTROY,
                0,
                0);

    return TRUE;
}
