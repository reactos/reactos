/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/sc/print.c
 * PURPOSE:     print service info
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "sc.h"

VOID
PrintService(LPCTSTR lpServiceName,
             LPSERVICE_STATUS_PROCESS pStatus,
             BOOL bExtended)
{
    _tprintf(_T("SERVICE_NAME: %s\n"), lpServiceName);

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

        default : _tprintf(_T("\n")); break;
    }

    _tprintf(_T("\tSTATE              : %x  "),
        (unsigned int)pStatus->dwCurrentState);

    switch (pStatus->dwCurrentState)
    {
        case 1 : _tprintf(_T("STOPPED\n")); break;
        case 2 : _tprintf(_T("START_PENDING\n")); break;
        case 3 : _tprintf(_T("STOP_PENDING\n")); break;
        case 4 : _tprintf(_T("RUNNING\n")); break;
        case 5 : _tprintf(_T("CONTINUE_PENDING\n")); break;
        case 6 : _tprintf(_T("PAUSE_PENDING\n")); break;
        case 7 : _tprintf(_T("PAUSED\n")); break;
        default : _tprintf(_T("\n")); break;
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

    if (bExtended)
    {
        _tprintf(_T("\tPID                : %lu\n"),
            pStatus->dwProcessId);
        _tprintf(_T("\tFLAGS              : %lu\n"),
            pStatus->dwServiceFlags);
    }

    _tprintf(_T("\n"));
}
