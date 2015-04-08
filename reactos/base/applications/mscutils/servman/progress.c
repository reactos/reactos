/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/progress.c
 * PURPOSE:     Progress dialog box message handler
 * COPYRIGHT:   Copyright 2006-2015 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

#define PROGRESS_RANGE      20
#define PROGRESS_STEP_MAX   15

typedef struct _PROGRESS_DATA
{
    HWND hDlg;
    HWND hProgress;
    LPWSTR ServiceName;
    ULONG Action;
    BOOL StopDepends;
    LPWSTR ServiceList;

} PROGRESS_DATA, *PPROGRESS_DATA;


static VOID
ResetProgressDialog(HWND hDlg,
                    LPWSTR ServiceName,
                    ULONG LabelId)
{
    LPWSTR lpProgStr;

    /* Load the label Id */
    if (AllocAndLoadString(&lpProgStr,
                           hInstance,
                           LabelId))
    {
        /* Write it to the dialog */
        SendDlgItemMessageW(hDlg,
                            IDC_SERVCON_INFO,
                            WM_SETTEXT,
                            0,
                            (LPARAM)lpProgStr);

        LocalFree(lpProgStr);
    }

    /* Write the service name to the dialog */
    SendDlgItemMessageW(hDlg,
                        IDC_SERVCON_NAME,
                        WM_SETTEXT,
                        0,
                        (LPARAM)ServiceName);

    /* Set the progress bar to the start */
    SendDlgItemMessageW(hDlg,
                        IDC_SERVCON_PROGRESS,
                        PBM_SETPOS,
                        0,
                        0);
}

unsigned int __stdcall ActionThread(void* Param)
{
    PPROGRESS_DATA ProgressData = (PPROGRESS_DATA)Param;

    if (ProgressData->Action == ACTION_START)
    {
        /* Setup the progress dialog for this action */
        ResetProgressDialog(ProgressData->hDlg,
                            ProgressData->ServiceName,
                            IDS_PROGRESS_INFO_START);

        /* Start the service */
        if (DoStartService(ProgressData->ServiceName,
                           ProgressData->hProgress,
                           NULL))
        {
            /* We're done, slide the progress bar up to the top */
            CompleteProgressBar(ProgressData->hProgress);
        }
    }
    else if (ProgressData->Action == ACTION_STOP || ProgressData->Action == ACTION_RESTART)
    {
        /* Check if there are and dependants to stop */
        if (ProgressData->StopDepends && ProgressData->ServiceList)
        {
            LPWSTR lpStr = ProgressData->ServiceList;

            /* Loop through all the services in the list */
            for (;;)
            {
                /* Break when we hit the double null */
                if (*lpStr == L'\0' && *(lpStr + 1) == L'\0')
                    break;

                /* If this isn't our first time in the loop we'll
                have been left on a null char */
                if (*lpStr == L'\0')
                    lpStr++;

                ResetProgressDialog(ProgressData->hDlg,
                                    lpStr,
                                    IDS_PROGRESS_INFO_STOP);

                /* Stop the requested service */
                if (DoStopService(ProgressData->ServiceName,
                                  ProgressData->hProgress))
                {
                    CompleteProgressBar(ProgressData->hProgress);
                }

                /* Move onto the next string */
                while (*lpStr != L'\0')
                    lpStr++;
            }
        }

        ResetProgressDialog(ProgressData->hDlg,
                            ProgressData->ServiceName,
                            IDS_PROGRESS_INFO_STOP);

        if (DoStopService(ProgressData->ServiceName,
                          ProgressData->hProgress))
        {
            CompleteProgressBar(ProgressData->hProgress);
        }


        /* If this was a restart, we'll need to start the service back up */
        if (ProgressData->Action == ACTION_RESTART)
        {
            /* Setup the progress dialog for this action */
            ResetProgressDialog(ProgressData->hDlg,
                                ProgressData->ServiceName,
                                IDS_PROGRESS_INFO_START);

            /* Start the service */
            if (DoStartService(ProgressData->ServiceName,
                               ProgressData->hProgress,
                               NULL))
            {
                /* We're done, slide the progress bar up to the top */
                CompleteProgressBar(ProgressData->hProgress);
            }
        }
    }
    else if (ProgressData->Action == ACTION_PAUSE)
    {
        /* Setup the progress dialog for this action */
        ResetProgressDialog(ProgressData->hDlg,
                            ProgressData->ServiceName,
                            IDS_PROGRESS_INFO_PAUSE);

        /* Pause the service */
        if (DoControlService(ProgressData->ServiceName,
                             ProgressData->hProgress,
                             SERVICE_CONTROL_PAUSE))
        {
            /* We're done, slide the progress bar up to the top */
            CompleteProgressBar(ProgressData->hProgress);
        }
    }
    else if (ProgressData->Action == ACTION_RESUME)
    {
        /* Setup the progress dialog for this action */
        ResetProgressDialog(ProgressData->hDlg,
                            ProgressData->ServiceName,
                            IDS_PROGRESS_INFO_RESUME);

        /* resume the service */
        if (DoControlService(ProgressData->ServiceName,
                             ProgressData->hProgress,
                             SERVICE_CONTROL_CONTINUE))
        {
            /* We're done, slide the progress bar up to the top */
            CompleteProgressBar(ProgressData->hProgress);
        }
    }


    EndDialog(ProgressData->hDlg, IDOK);

    _endthreadex(0);
    return 0;
}

static BOOL
InitDialog(HWND hDlg,
           UINT Message,
           WPARAM wParam,
           LPARAM lParam)
{
    PPROGRESS_DATA ProgressData = (PPROGRESS_DATA)lParam;
    HANDLE hThread;

    ProgressData->hDlg = hDlg;

    /* Get a handle to the progress bar */
    ProgressData->hProgress = GetDlgItem(hDlg,
                                         IDC_SERVCON_PROGRESS);
    if (!ProgressData->hProgress)
        return FALSE;

    /* Set the progress bar range */
    SendMessageW(ProgressData->hProgress,
                 PBM_SETRANGE,
                 0,
                 MAKELPARAM(0, PROGRESS_RANGE));

    /* Set the progress bar step */
    SendMessageW(ProgressData->hProgress,
                 PBM_SETSTEP,
                 (WPARAM)1,
                 0);

    /* Create a thread to handle the service control */
    hThread = (HANDLE)_beginthreadex(NULL, 0, &ActionThread, ProgressData, 0, NULL);
    if (!hThread) return FALSE;

    CloseHandle(hThread);

    return TRUE;
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
            return InitDialog(hDlg,
                              Message,
                              wParam,
                              lParam);
        }

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hDlg, wParam);
                    break;

            }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

VOID
CompleteProgressBar(HANDLE hProgress)
{
    HWND hProgBar = (HWND)hProgress;
    UINT Pos = 0;

    /* Get the current position */
    Pos = SendMessageW(hProgBar,
                       PBM_GETPOS,
                       0,
                       0);

    /* Loop until we hit the max */
    while (Pos <= PROGRESS_RANGE)
    {
        /* Increment the progress bar */
        SendMessageW(hProgBar,
                     PBM_DELTAPOS,
                     Pos,
                     0);

        /* Wait for 15ms to give it a smooth feel */
        Sleep(15);
        Pos++;
    }
}

VOID
IncrementProgressBar(HANDLE hProgress,
                     UINT Step)
{
    HWND hProgBar = (HWND)hProgress;
    UINT Position;

    /* Don't allow the progress to reach to complete*/
    Position = SendMessageW(hProgBar,
                            PBM_GETPOS,
                            0,
                            0);
    if (Position < PROGRESS_STEP_MAX)
    {
        /* Do we want to increment the default amount? */
        if (Step == DEFAULT_STEP)
        {
            /* Use the step value we set on create */
            SendMessageW(hProgBar,
                         PBM_STEPIT,
                         0,
                         0);
        }
        else
        {
            /* Use the value passed */
            SendMessageW(hProgBar,
                         PBM_SETPOS,
                         Step,
                         0);
        }
    }
}

BOOL
RunActionWithProgress(HWND hParent,
                      LPWSTR ServiceName,
                      LPWSTR DisplayName,
                      UINT Action)
{
    PROGRESS_DATA ProgressData;
    LPWSTR ServiceList;
    BOOL StopDepends;
    INT_PTR Result;

    StopDepends = FALSE;
    ServiceList = NULL;


    /* Check if we'll be stopping the service */
    if (Action == ACTION_STOP || Action == ACTION_RESTART)
    {
        /* Does the service have any dependent services which need stopping first */
        ServiceList = GetListOfServicesToStop(ServiceName);
        if (ServiceList)
        {
            /* Ask the user if they want to stop the dependants */
            StopDepends = CreateStopDependsDialog(hParent,
                                                  ServiceName,
                                                  DisplayName,
                                                  ServiceList);

            /* Exit early if the user decided not to stop the dependants */
            if (StopDepends == FALSE)
                return FALSE;
        }
    }

    ProgressData.hDlg = NULL;
    ProgressData.ServiceName = ServiceName;
    ProgressData.Action = Action;
    ProgressData.StopDepends = StopDepends;
    ProgressData.ServiceList = ServiceList;

    Result = DialogBoxParamW(hInstance,
                             MAKEINTRESOURCEW(IDD_DLG_PROGRESS),
                             hParent,
                             ProgressDialogProc,
                             (LPARAM)&ProgressData);

    if (ServiceList)
        HeapFree(GetProcessHeap(), 0, ServiceList);

    return (Result == IDOK);
}
