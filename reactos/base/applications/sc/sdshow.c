/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/sc/sdshow.c
 * PURPOSE:     Show a service security descriptor
 * COPYRIGHT:   Copyright 2016 Eric Kohl
 *
 */

#include "sc.h"

BOOL SdShow(LPCTSTR ServiceName)
{
    SC_HANDLE hManager = NULL;
    SC_HANDLE hService = NULL;
    BOOL bResult = TRUE;
    DWORD cbBytesNeeded = 0;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    LPTSTR pStringBuffer = NULL;

#ifdef SCDBG
    _tprintf(_T("service to show sd - %s\n\n"), ServiceName);
#endif

    hManager = OpenSCManager(NULL,
                             NULL,
                             SC_MANAGER_CONNECT);
    if (hManager == NULL)
    {
        bResult = FALSE;
        goto done;
    }

    hService = OpenService(hManager, ServiceName, READ_CONTROL);
    if (hService == NULL)
    {
        bResult = FALSE;
        goto done;
    }

    if (!QueryServiceObjectSecurity(hService,
                                    DACL_SECURITY_INFORMATION,
                                    (PSECURITY_DESCRIPTOR)&pSecurityDescriptor,
                                    sizeof(PSECURITY_DESCRIPTOR),
                                    &cbBytesNeeded))
    {
        if (cbBytesNeeded == 0)
        {
            bResult = FALSE;
            goto done;
        }
    }

    pSecurityDescriptor = HeapAlloc(GetProcessHeap(), 0, cbBytesNeeded);
    if (pSecurityDescriptor == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        bResult = FALSE;
        goto done;
    }

    if (!QueryServiceObjectSecurity(hService,
                                    DACL_SECURITY_INFORMATION,
                                    pSecurityDescriptor,
                                    cbBytesNeeded,
                                    &cbBytesNeeded))
    {
        bResult = FALSE;
        goto done;
    }

    if (!ConvertSecurityDescriptorToStringSecurityDescriptor(pSecurityDescriptor,
                                                             SDDL_REVISION_1,
                                                             DACL_SECURITY_INFORMATION,
                                                             &pStringBuffer,
                                                             NULL))
    {
        bResult = FALSE;
        goto done;
    }

    _tprintf(_T("\n%s\n"), pStringBuffer);

done:
    if (bResult == FALSE)
        ReportLastError();

    if (pStringBuffer != NULL)
        LocalFree(pStringBuffer);

    if (pSecurityDescriptor != NULL)
        HeapFree(GetProcessHeap(), 0, pSecurityDescriptor);

    if (hService)
        CloseServiceHandle(hService);

    if (hManager)
        CloseServiceHandle(hManager);

    return bResult;
}
