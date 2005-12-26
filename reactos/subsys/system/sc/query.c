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
/*
 * TODO:
 * Allow calling of 2 options e.g.:
 *    type= driver state= inactive
 */

#include "sc.h"

#define DEBUG
#include <debug.h>

/* local function decs */
VOID PrintService(BOOL bExtended);
BOOL EnumServices(DWORD ServiceType, DWORD ServiceState);
BOOL QueryService(LPCTSTR ServiceName, BOOL bExtended);

/* global variables */
static ENUM_SERVICE_STATUS_PROCESS *pServiceStatus = NULL;
DWORD NumServices = 0;


BOOL
Query(LPCTSTR ServiceName, LPCTSTR *ServiceArgs, BOOL bExtended)
{
    if (! ServiceName) /* display all running services and drivers */
    {
        /* get default values */
        EnumServices(SERVICE_WIN32, SERVICE_ACTIVE);
        
        /* print default values */
        PrintService(bExtended);
    }
    else if (_tcsicmp(ServiceName, _T("type=")) == 0)
    {
        LPCTSTR Type = *ServiceArgs;
        
        if (_tcsicmp(Type, _T("driver")) == 0)
            EnumServices(SERVICE_DRIVER, SERVICE_ACTIVE);
        else if (_tcsicmp(Type, _T("service")) == 0)
            EnumServices(SERVICE_WIN32, SERVICE_ACTIVE);
        else if (_tcsicmp(Type, _T("all")) == 0)
            EnumServices(SERVICE_DRIVER|SERVICE_WIN32, SERVICE_ACTIVE);
        else
        {
            _tprintf(_T("\nERROR following \"type=\"!\n"));
            _tprintf(_T("Must be \"driver\" or \"service\" or \"all\"\n"));
        }
        
        PrintService(bExtended);
    }
    else if(_tcsicmp(ServiceName, _T("state=")) == 0)
    {
        LPCTSTR State = *ServiceArgs;

        if (_tcsicmp(State, _T("inactive")) == 0)
            EnumServices(SERVICE_WIN32, SERVICE_INACTIVE);
        else if (_tcsicmp(State, _T("all")) == 0)
            EnumServices(SERVICE_WIN32, SERVICE_STATE_ALL);
        else
        {
            _tprintf(_T("\nERROR following \"state=\"!\n"));
            _tprintf(_T("Must be \"active\" or \"inactive\" or \"all\"\n"));
        }
            
        PrintService(bExtended);
    }
/*
    else if(_tcsicmp(ServiceName, _T("bufsize=")))

    else if(_tcsicmp(ServiceName, _T("ri=")))

    else if(_tcsicmp(ServiceName, _T("group=")))
*/
    else /* print only the service requested */
    {
        QueryService(ServiceName, bExtended);
    }
    
    return TRUE;
}


BOOL
QueryService(LPCTSTR ServiceName, BOOL bExtended)
{
    SERVICE_STATUS_PROCESS *pServiceInfo = NULL;
    SC_HANDLE hSc;
    DWORD BufSiz = 0;
    DWORD BytesNeeded = 0;
    DWORD Ret;

    hSc = OpenService(hSCManager, ServiceName, SERVICE_QUERY_STATUS);

    if (hSc == NULL)
    {
        _tprintf(_T("QueryService: openService failed\n"));
        ReportLastError();
        return FALSE;
    }

    Ret = QueryServiceStatusEx(hSc,
                SC_STATUS_PROCESS_INFO,
                NULL,
                BufSiz,
                &BytesNeeded);

    if ((Ret != 0) || (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
    {
        _tprintf(_T("QueryService: First call to QueryServiceStatusEx failed : "));
        ReportLastError();
        return FALSE;
    }
    else /* Call function again if required size was returned */
    {
        /* reserve memory for service info array */
        pServiceInfo = (SERVICE_STATUS_PROCESS *)
            HeapAlloc(GetProcessHeap(), 0, BytesNeeded);
        if (pServiceInfo == NULL)
        {
            _tprintf(_T("QueryService: Failed to allocate memory : "));
            ReportLastError();
            return FALSE;
        }

        /* fill array with service info */
        if (! QueryServiceStatusEx(hSc,
                    SC_STATUS_PROCESS_INFO,
                    (LPBYTE)pServiceInfo,
                    BytesNeeded,
                    &BytesNeeded))
        {
            _tprintf(_T("QueryService: Second call to QueryServiceStatusEx failed : "));
            ReportLastError();
            HeapFree(GetProcessHeap(), 0, pServiceInfo);
            return FALSE;
        }
    }

    
    _tprintf(_T("SERVICE_NAME: %s\n"), ServiceName);

    _tprintf(_T("\tTYPE               : %x  "),
         (unsigned int)pServiceInfo->dwServiceType);
    switch (pServiceInfo->dwServiceType)
    {
        case 1 : _tprintf(_T("KERNEL_DRIVER\n")); break;
        case 2 : _tprintf(_T("FILE_SYSTEM_DRIVER\n")); break;
        case 16 : _tprintf(_T("WIN32_OWN_PROCESS\n")); break;
        case 32 : _tprintf(_T("WIN32_SHARE_PROCESS\n")); break;
        default : _tprintf(_T("\n")); break;
    }

    _tprintf(_T("\tSTATE              : %x  "),
        (unsigned int)pServiceInfo->dwCurrentState);

    switch (pServiceInfo->dwCurrentState)
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

//        _tprintf(_T("\n\taccepted         : 0x%x\n\n"),
//            pServiceStatus[i].ServiceStatusProcess.dwControlsAccepted);
//            (STOPPABLE,NOT_PAUSABLE,ACCEPTS_SHUTDOWN)

    _tprintf(_T("\tWIN32_EXIT_CODE    : %d  (0x%x)\n"),
        (unsigned int)pServiceInfo->dwWin32ExitCode,
        (unsigned int)pServiceInfo->dwWin32ExitCode);
    _tprintf(_T("\tSERVICE_EXIT_CODE  : %d  (0x%x)\n"),
        (unsigned int)pServiceInfo->dwServiceSpecificExitCode,
        (unsigned int)pServiceInfo->dwServiceSpecificExitCode);
    _tprintf(_T("\tCHECKPOINT         : 0x%x\n"),
        (unsigned int)pServiceInfo->dwCheckPoint);
    _tprintf(_T("\tWAIT_HINT          : 0x%x\n"),
        (unsigned int)pServiceInfo->dwWaitHint);
    if (bExtended)
    {
        _tprintf(_T("\tPID                : %lu\n"),
            pServiceInfo->dwProcessId);
        _tprintf(_T("\tFLAGS              : %lu\n"),
            pServiceInfo->dwServiceFlags);
    }
    
    HeapFree(GetProcessHeap(), 0, pServiceInfo);

    return TRUE;
}


BOOL
EnumServices(DWORD ServiceType, DWORD ServiceState)
{
    DWORD BufSize = 0;
    DWORD BytesNeeded = 0;
    DWORD ResumeHandle = 0;
    DWORD Ret;

    /* determine required buffer size */
    Ret = EnumServicesStatusEx(hSCManager,
                SC_ENUM_PROCESS_INFO,
                ServiceType,
                ServiceState,
                (LPBYTE)pServiceStatus,
                BufSize,
                &BytesNeeded,
                &NumServices,
                &ResumeHandle,
                0);

    if ((Ret != 0) && (GetLastError() != ERROR_MORE_DATA))
    {
        _tprintf(_T("EnumServices: First call to EnumServicesStatusEx failed : "));
        ReportLastError();
        return FALSE;
    }
    else /* Call function again if required size was returned */
    {
        /* reserve memory for service info array */
        pServiceStatus = (ENUM_SERVICE_STATUS_PROCESS *)
                HeapAlloc(GetProcessHeap(), 0, BytesNeeded);
        if (pServiceStatus == NULL)
        {
            _tprintf(_T("EnumServices: Failed to allocate memory : "));
            ReportLastError();
            return FALSE;
        }

        /* fill array with service info */
        if (! EnumServicesStatusEx(hSCManager,
                    SC_ENUM_PROCESS_INFO,
                    ServiceType,
                    ServiceState,
                    (LPBYTE)pServiceStatus,
                    BytesNeeded,
                    &BytesNeeded,
                    &NumServices,
                    &ResumeHandle,
                    0))
        {
            _tprintf(_T("EnumServices: Second call to EnumServicesStatusEx failed : "));
            ReportLastError();
            HeapFree(GetProcessHeap(), 0, pServiceStatus);
            return FALSE;
        }
    }

    return TRUE;
}


VOID
PrintService(BOOL bExtended)
{
    DWORD i;
    
    for (i=0; i < NumServices; i++)
    {

        _tprintf(_T("SERVICE_NAME: %s\n"), pServiceStatus[i].lpServiceName);
        _tprintf(_T("DISPLAY_NAME: %s\n"), pServiceStatus[i].lpDisplayName);

        _tprintf(_T("\tTYPE               : %x  "),
            (unsigned int)pServiceStatus[i].ServiceStatusProcess.dwServiceType);
        switch (pServiceStatus[i].ServiceStatusProcess.dwServiceType)
        {
            case 1 : _tprintf(_T("KERNEL_DRIVER\n")); break;
            case 2 : _tprintf(_T("FILE_SYSTEM_DRIVER\n")); break;
            case 16 : _tprintf(_T("WIN32_OWN_PROCESS\n")); break;
            case 32 : _tprintf(_T("WIN32_SHARE_PROCESS\n")); break;
            default : _tprintf(_T("\n")); break;
        }

        _tprintf(_T("\tSTATE              : %x  "),
            (unsigned int)pServiceStatus[i].ServiceStatusProcess.dwCurrentState);

        switch (pServiceStatus[i].ServiceStatusProcess.dwCurrentState)
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

    //        _tprintf(_T("\n\taccepted         : 0x%x\n\n"),
    //            pServiceStatus[i].ServiceStatusProcess.dwControlsAccepted);
    //            (STOPPABLE,NOT_PAUSABLE,ACCEPTS_SHUTDOWN)

        _tprintf(_T("\tWIN32_EXIT_CODE    : %d  (0x%x)\n"),
            (unsigned int)pServiceStatus[i].ServiceStatusProcess.dwWin32ExitCode,
            (unsigned int)pServiceStatus[i].ServiceStatusProcess.dwWin32ExitCode);
        _tprintf(_T("\tSERVICE_EXIT_CODE  : %d  (0x%x)\n"),
            (unsigned int)pServiceStatus[i].ServiceStatusProcess.dwServiceSpecificExitCode,
            (unsigned int)pServiceStatus[i].ServiceStatusProcess.dwServiceSpecificExitCode);
        _tprintf(_T("\tCHECKPOINT         : 0x%x\n"),
            (unsigned int)pServiceStatus[i].ServiceStatusProcess.dwCheckPoint);
        _tprintf(_T("\tWAIT_HINT          : 0x%x\n"),
            (unsigned int)pServiceStatus[i].ServiceStatusProcess.dwWaitHint);
        if (bExtended)
        {
            _tprintf(_T("\tPID                : %lu\n"),
                pServiceStatus[i].ServiceStatusProcess.dwProcessId);
            _tprintf(_T("\tFLAGS              : %lu\n"),
                pServiceStatus[i].ServiceStatusProcess.dwServiceFlags);
        }

            _tprintf(_T("\n"));
    }
    
    _tprintf(_T("number : %lu\n"), NumServices);
}
