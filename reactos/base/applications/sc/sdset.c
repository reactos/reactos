/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/sc/sdset.c
 * PURPOSE:     Set a service security descriptor
 * COPYRIGHT:   Copyright 2016 Eric Kohl
 *
 */

#include "sc.h"

BOOL SdSet(LPCTSTR ServiceName, LPCTSTR StringSecurityDescriptor)
{
    SC_HANDLE hManager = NULL;
    SC_HANDLE hService = NULL;
    BOOL bResult = TRUE;
    ULONG ulSecurityDescriptorSize = 0;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;

#ifdef SCDBG
    _tprintf(_T("service to set sd - %s\n\n"), ServiceName);
#endif

    hManager = OpenSCManager(NULL,
                             NULL,
                             SC_MANAGER_CONNECT);
    if (hManager == NULL)
    {
        _tprintf(_T("[SC] OpenSCManager FAILED %lu:\n\n"), GetLastError());
        bResult = FALSE;
        goto done;
    }

    hService = OpenService(hManager, ServiceName, WRITE_DAC);
    if (hService == NULL)
    {
        _tprintf(_T("[SC] OpenService FAILED %lu:\n\n"), GetLastError());
        bResult = FALSE;
        goto done;
    }

    if (!ConvertStringSecurityDescriptorToSecurityDescriptor(StringSecurityDescriptor,
                                                             SDDL_REVISION_1,
                                                             &pSecurityDescriptor,
                                                             &ulSecurityDescriptorSize))
    {
        _tprintf(_T("[SC] ConvertStringSecurityDescriptorToSecurityDescriptor FAILED %lu:\n\n"), GetLastError());
        bResult = FALSE;
        goto done;
    }

    if (!SetServiceObjectSecurity(hService,
                                  DACL_SECURITY_INFORMATION,
                                  pSecurityDescriptor))
    {
        _tprintf(_T("[SC] SetServiceObjectSecurity FAILED %lu:\n\n"), GetLastError());
        bResult = FALSE;
        goto done;
    }

done:
    if (bResult == FALSE)
        ReportLastError();

    if (pSecurityDescriptor != NULL)
        LocalFree(pSecurityDescriptor);

    if (hService)
        CloseServiceHandle(hService);

    if (hManager)
        CloseServiceHandle(hManager);

    return bResult;
}
