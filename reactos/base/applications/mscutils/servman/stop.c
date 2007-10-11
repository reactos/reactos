/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/stop.c
 * PURPOSE:     Stops running a service
 * COPYRIGHT:   Copyright 2006-2007 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

BOOL
DoStop(PMAIN_WND_INFO Info)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hSc = NULL;
    LPQUERY_SERVICE_CONFIG lpServiceConfig = NULL;
    HWND hProgDlg;
    DWORD BytesNeeded = 0;
    BOOL ret = FALSE;

    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_ENUMERATE_SERVICE);
    if (hSCManager == NULL)
    {
        GetError();
        return FALSE;
    }

    hSc = OpenService(hSCManager,
                      Info->pCurrentService->lpServiceName,
                      SERVICE_QUERY_CONFIG);
    if (hSc)
    {
        if (!QueryServiceConfig(hSc,
                                lpServiceConfig,
                                0,
                                &BytesNeeded))
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                lpServiceConfig = (LPQUERY_SERVICE_CONFIG)HeapAlloc(ProcessHeap,
                                                                    0,
                                                                    BytesNeeded);
                if (lpServiceConfig == NULL)
                    goto cleanup;

                if (QueryServiceConfig(hSc,
                                       lpServiceConfig,
                                       BytesNeeded,
                                       &BytesNeeded))
                {
#if 0
                    if (lpServiceConfig->lpDependencies)
                    {
                        TCHAR str[500];

                        _sntprintf(str, 499, _T("%s depends on this service, implement the dialog to allow closing of other services"),
                                   lpServiceConfig->lpDependencies);
                        MessageBox(NULL, str, NULL, 0);
                        
                        //FIXME: open 'stop other services' box
                    }
                    else
                    {
#endif
                            hProgDlg = CreateProgressDialog(Info->hMainWnd,
                                                            Info->pCurrentService->lpServiceName,
                                                            IDS_PROGRESS_INFO_STOP);
                            if (hProgDlg)
                            {
                                ret = Control(Info,
                                              hProgDlg,
                                              SERVICE_CONTROL_STOP);

                                DestroyWindow(hProgDlg);
                            }
                    //}

                    HeapFree(ProcessHeap,
                             0,
                             lpServiceConfig);

                    lpServiceConfig = NULL;
                }
            }
        }
    }

cleanup:
    if (hSCManager != NULL)
        CloseServiceHandle(hSCManager);
    if (hSc != NULL)
        CloseServiceHandle(hSc);

    return ret;
}
