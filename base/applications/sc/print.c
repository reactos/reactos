/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:        base/applications/sc/print.c
 * PURPOSE:     print service info
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *              Copyright 2016 Eric Kohl <eric.kohl@reactos.org>
 */

#include "sc.h"

VOID
PrintService(LPCTSTR lpServiceName,
             LPSERVICE_STATUS_PROCESS pStatus,
             BOOL bExtended)
{
    _tprintf(_T("SERVICE_NAME: %s\n"), lpServiceName);

    // Re-use PrintServiceStatus(), as SERVICE_STATUS_PROCESS is in fact an extension of SERVICE_STATUS.
    PrintServiceStatus((LPSERVICE_STATUS)pStatus);

    if (bExtended)
    {
        _tprintf(_T("\tPID                : %lu\n"),
            pStatus->dwProcessId);
        _tprintf(_T("\tFLAGS              : %lu\n"),
            pStatus->dwServiceFlags);
    }

    _tprintf(_T("\n"));
}


VOID
PrintServiceStatus(
    LPSERVICE_STATUS pStatus)
{
    _tprintf(_T("\tTYPE               : %x  "),
        (unsigned int)pStatus->dwServiceType);

    switch (pStatus->dwServiceType)
    {
        case SERVICE_KERNEL_DRIVER:
            _tprintf(_T("KERNEL_DRIVER\n"));
            break;
        case SERVICE_FILE_SYSTEM_DRIVER:
            _tprintf(_T("FILE_SYSTEM_DRIVER\n"));
            break;
        case SERVICE_WIN32_OWN_PROCESS:
            _tprintf(_T("WIN32_OWN_PROCESS\n"));
            break;
        case SERVICE_WIN32_SHARE_PROCESS:
            _tprintf(_T("WIN32_SHARE_PROCESS\n"));
            break;
        case SERVICE_WIN32_OWN_PROCESS + SERVICE_INTERACTIVE_PROCESS:
            _tprintf(_T("WIN32_OWN_PROCESS (interactive)\n"));
            break;
        case SERVICE_WIN32_SHARE_PROCESS + SERVICE_INTERACTIVE_PROCESS:
            _tprintf(_T("WIN32_SHARE_PROCESS (interactive)\n"));
            break;
        default:
            _tprintf(_T("\n"));
    }

    _tprintf(_T("\tSTATE              : %x  "),
        (unsigned int)pStatus->dwCurrentState);

    switch (pStatus->dwCurrentState)
    {
        case SERVICE_STOPPED:
            _tprintf(_T("STOPPED\n"));
            break;
        case SERVICE_START_PENDING:
            _tprintf(_T("START_PENDING\n"));
            break;
        case SERVICE_STOP_PENDING:
            _tprintf(_T("STOP_PENDING\n"));
            break;
        case SERVICE_RUNNING:
            _tprintf(_T("RUNNING\n"));
            break;
        case SERVICE_CONTINUE_PENDING:
            _tprintf(_T("CONTINUE_PENDING\n"));
            break;
        case SERVICE_PAUSE_PENDING:
            _tprintf(_T("PAUSE_PENDING\n"));
            break;
        case SERVICE_PAUSED:
            _tprintf(_T("PAUSED\n"));
            break;
        default:
            _tprintf(_T("\n"));
    }

    _tprintf(_T("\t\t\t\t("));

    if (pStatus->dwControlsAccepted & SERVICE_ACCEPT_STOP)
        _tprintf(_T("STOPPABLE,"));
    else
        _tprintf(_T("NOT_STOPPABLE,"));

    if (pStatus->dwControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE)
        _tprintf(_T("PAUSABLE,"));
    else
        _tprintf(_T("NOT_PAUSABLE,"));

    if (pStatus->dwControlsAccepted & SERVICE_ACCEPT_SHUTDOWN)
        _tprintf(_T("ACCEPTS_SHUTDOWN"));
    else
        _tprintf(_T("IGNORES_SHUTDOWN"));

    _tprintf(_T(")\n"));

    _tprintf(_T("\tWIN32_EXIT_CODE    : %u  (0x%x)\n"),
        (unsigned int)pStatus->dwWin32ExitCode,
        (unsigned int)pStatus->dwWin32ExitCode);
    _tprintf(_T("\tSERVICE_EXIT_CODE  : %u  (0x%x)\n"),
        (unsigned int)pStatus->dwServiceSpecificExitCode,
        (unsigned int)pStatus->dwServiceSpecificExitCode);
    _tprintf(_T("\tCHECKPOINT         : 0x%x\n"),
        (unsigned int)pStatus->dwCheckPoint);
    _tprintf(_T("\tWAIT_HINT          : 0x%x\n"),
        (unsigned int)pStatus->dwWaitHint);
}
