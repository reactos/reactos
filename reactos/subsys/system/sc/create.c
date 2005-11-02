/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS SC utility
 * FILE:        subsys/system/sc/create.c
 * PURPOSE:     control ReactOS services
 * PROGRAMMERS: Ged Murphy (gedmurphy@gmail.com)
 * REVISIONS:
 *           Ged Murphy 20/10/05 Created
 *
 */

#include "sc.h"

BOOL Create(TCHAR **Args)
{
    SC_HANDLE hSc;
    LPCTSTR ServiceName = *Args;
    LPCTSTR BinaryPathName = *++Args;
    LPCTSTR *Options = (LPCTSTR *)++Args;
    
    /* testing */
    printf("service to create - %s\n", ServiceName);
    printf("Binary path - %s\n", BinaryPathName);
    printf("Arguments :\n");
    while (*Options)
    {
        printf("%s\n", *Options);
        Options++;
    }


    hSc = CreateService(hSCManager,
                        ServiceName,
                        ServiceName,
                        SERVICE_ALL_ACCESS,
                        SERVICE_WIN32_OWN_PROCESS,
                        SERVICE_DEMAND_START,
                        SERVICE_ERROR_NORMAL,
                        BinaryPathName,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL);

    if (hSc == NULL)
    {
        _tprintf(_T("CreateService failed\n"));
        ReportLastError();
        return FALSE;
    }
    else
    {
        CloseServiceHandle(hSc);
        return TRUE;
    }
}
