/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS SC utility
 * FILE:        subsys/system/sc/query.c
 * PURPOSE:     control ReactOS services
 * PROGRAMMERS: Ged Murphy (gedmurphy@gmail.com)
 * REVISIONS:
 *           Ged Murphy 20/10/05 Created
 *
 */

#include "sc.h"

/* local function decs */
VOID PrintService(ENUM_SERVICE_STATUS_PROCESS *pSStatus, BOOL bExtended);
INT EnumServices(DWORD ServiceType, DWORD ServiceState);

/* global variables */
static ENUM_SERVICE_STATUS_PROCESS *pServiceStatus = NULL;

BOOL
Query(TCHAR **Args, BOOL bExtended)
{
            
    LPCTSTR ServiceName = *Args;

    if (! *Args)
    {
        printf("Service name %s\n", ServiceName); // test

        /* get default values */
        EnumServices(SERVICE_WIN32, SERVICE_ACTIVE);
        
        /* print default values */
        PrintService(pServiceStatus, bExtended);
    }
    
    if (_tcsicmp(Args[0], _T("type=")) == 0)
    {
        TCHAR *Type = *++Args;
        _tprintf(_T("got type\narg = %s\n"), Type); // test
        if (_tcsicmp(Type, _T("driver")) == 0)
            EnumServices(SERVICE_DRIVER, SERVICE_STATE_ALL);
        else if (_tcsicmp(Type, _T("service")) == 0)
            EnumServices(SERVICE_WIN32, SERVICE_STATE_ALL);
        else if (_tcsicmp(Type, _T("all")) == 0)
            EnumServices(SERVICE_DRIVER|SERVICE_WIN32, SERVICE_STATE_ALL);
        else
        {
            _tprintf(_T("\nERROR following \"type=\"!\n"));
            _tprintf(_T("Must be \"driver\" or \"service\" or \"all\"\n"));
        }
        
        PrintService(pServiceStatus, bExtended);
    }
    else if(_tcsicmp(Args[0], _T("state=")) == 0)
    {
        TCHAR *State = *++Args;

        if (_tcsicmp(State, _T("active")) == 0)
            EnumServices(SERVICE_DRIVER|SERVICE_WIN32, SERVICE_ACTIVE);
        else if (_tcsicmp(State, _T("inactive")) == 0)
            EnumServices(SERVICE_DRIVER|SERVICE_WIN32, SERVICE_INACTIVE);
        else if (_tcsicmp(State, _T("all")) == 0)
            EnumServices(SERVICE_DRIVER|SERVICE_WIN32, SERVICE_STATE_ALL);
        else
        {
            _tprintf(_T("\nERROR following \"state=\"!\n"));
            _tprintf(_T("Must be \"active\" or \"inactive\" or \"all\"\n"));
        }
            
        PrintService(pServiceStatus, bExtended);
    }
    else
    {
        printf("no args\n"); // test
        /* get default values */
        EnumServices(SERVICE_WIN32, SERVICE_ACTIVE);

        /* print default values */
        PrintService(pServiceStatus, bExtended);
    }

/*
    else if(_tcsicmp(Args[0], _T("bufsize=")))

    else if(_tcsicmp(Args[0], _T("ri=")))

    else if(_tcsicmp(Args[0], _T("group=")))
*/


}


INT EnumServices(DWORD ServiceType, DWORD ServiceState)
{
    SC_HANDLE hSc;
    DWORD BufSize = 0;
    DWORD BytesNeeded = 0;
    DWORD NumServices = 0;
    DWORD ResumeHandle = 0;

    /* determine required buffer size */
    if (! EnumServicesStatusEx(hSCManager,
                SC_ENUM_PROCESS_INFO,
                ServiceType,
                ServiceState,
                (LPBYTE)pServiceStatus,
                BufSize,
                &BytesNeeded,
                &NumServices,
                &ResumeHandle,
                0))
    {
        /* Call function again if required size was returned */
        if (GetLastError() == ERROR_MORE_DATA)
        {
            /* reserve memory for service info array */
            pServiceStatus = (ENUM_SERVICE_STATUS_PROCESS *) malloc(BytesNeeded);

            /* fill array with service info */
            if (! EnumServicesStatusEx(hSCManager,
                        SC_ENUM_PROCESS_INFO,
                        SERVICE_DRIVER | SERVICE_WIN32,
                        SERVICE_STATE_ALL,
                        (LPBYTE)pServiceStatus,
                        BufSize,
                        &BytesNeeded,
                        &NumServices,
                        &ResumeHandle,
                        0))
            {
                _tprintf(_T("Second call to EnumServicesStatusEx failed : "));
                ReportLastError();
                return FALSE;
            }
        }
        else /* exit on failure */
        {
            _tprintf(_T("First call to EnumServicesStatusEx failed : "));
            ReportLastError();
            return FALSE;
        }
    }
}


VOID
PrintService(ENUM_SERVICE_STATUS_PROCESS *pServiceStatus,
                    BOOL bExtended)
{
    _tprintf(_T("SERVICE_NAME: %s\n"), pServiceStatus->lpServiceName);
    _tprintf(_T("DISPLAY_NAME: %s\n"), pServiceStatus->lpDisplayName);
    _tprintf(_T("TYPE               : %lu\n"),
        pServiceStatus->ServiceStatusProcess.dwServiceType);
    _tprintf(_T("STATE              : %lu\n"),
        pServiceStatus->ServiceStatusProcess.dwCurrentState);
                        //    (STOPPABLE,NOT_PAUSABLE,ACCEPTS_SHUTDOWN)
    _tprintf(_T("WIN32_EXIT_CODE    : %lu \n"),
        pServiceStatus->ServiceStatusProcess.dwWin32ExitCode);
    _tprintf(_T("SERVICE_EXIT_CODE  : %lu \n"),
        pServiceStatus->ServiceStatusProcess.dwServiceSpecificExitCode);
    _tprintf(_T("CHECKPOINT         : %lu\n"),
        pServiceStatus->ServiceStatusProcess.dwCheckPoint);
    _tprintf(_T("WAIT_HINT          : %lu\n"),
        pServiceStatus->ServiceStatusProcess.dwWaitHint);
    if (bExtended)
    {
        _tprintf(_T("PID                : %lu\n"),
            pServiceStatus->ServiceStatusProcess.dwProcessId);
        _tprintf(_T("FLAGS              : %lu\n"),
            pServiceStatus->ServiceStatusProcess.dwServiceFlags);
    }
}
