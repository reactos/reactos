/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/sc/create.c
 * PURPOSE:     Create a service
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "sc.h"

BOOL Create(LPCTSTR ServiceName, LPCTSTR *ServiceArgs)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hSc;
    BOOL bRet = FALSE;

    DWORD dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    DWORD dwStartType = SERVICE_DEMAND_START;
    DWORD dwErrorControl = SERVICE_ERROR_NORMAL;
    LPCTSTR lpBinaryPathName = NULL;
    LPCTSTR lpLoadOrderGroup = NULL;
    DWORD dwTagId = 0;
    LPCTSTR lpDependencies = NULL;
    LPCTSTR lpServiceStartName = NULL;
    LPCTSTR lpPassword = NULL;

    /* quick hack to get it working */
    lpBinaryPathName = *ServiceArgs;

#ifdef SCDBG
    _tprintf(_T("service name - %s\n"), ServiceName);
    _tprintf(_T("display name - %s\n"), ServiceName);
    _tprintf(_T("service type - %lu\n"), dwServiceType);
    _tprintf(_T("start type - %lu\n"), dwStartType);
    _tprintf(_T("error control - %lu\n"), dwErrorControl);
    _tprintf(_T("Binary path - %s\n"), lpBinaryPathName);
    _tprintf(_T("load order group - %s\n"), lpLoadOrderGroup);
    _tprintf(_T("tag - %lu\n"), dwTagId);
    _tprintf(_T("dependincies - %s\n"), lpDependencies);
    _tprintf(_T("account start name - %s\n"), lpServiceStartName);
    _tprintf(_T("account password - %s\n"), lpPassword);
#endif

    if (!ServiceName)
    {
        CreateUsage();
        return FALSE;
    }

    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_CREATE_SERVICE);
    if (hSCManager == NULL)
    {
        ReportLastError();
        return FALSE;
    }

    hSc = CreateService(hSCManager,
                        ServiceName,
                        ServiceName,
                        SERVICE_ALL_ACCESS,
                        dwServiceType,
                        dwStartType,
                        dwErrorControl,
                        lpBinaryPathName,
                        lpLoadOrderGroup,
                        &dwTagId,
                        lpDependencies,
                        lpServiceStartName,
                        lpPassword);

    if (hSc == NULL)
    {
        ReportLastError();
        CloseServiceHandle(hSCManager);
    }
    else
    {
        _tprintf(_T("[SC] CreateService SUCCESS\n"));

        CloseServiceHandle(hSc);
        CloseServiceHandle(hSCManager);
        bRet = TRUE;
    }

    return bRet;
}
