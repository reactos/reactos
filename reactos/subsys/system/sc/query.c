#include "sc.h"

extern SC_HANDLE hSCManager; /* declared in sc.c */

BOOL
Query(TCHAR **Args, BOOL bExtended)
{
    SC_HANDLE hSc;
    ENUM_SERVICE_STATUS_PROCESS *pServiceStatus = NULL;
    DWORD BufSize = 0;
    DWORD BytesNeeded;
    DWORD NumServices;
    DWORD ResumeHandle;
    INT i;

    /* determine required buffer size */
    EnumServicesStatusEx(hSCManager,
                SC_ENUM_PROCESS_INFO,
                SERVICE_DRIVER | SERVICE_WIN32,
                SERVICE_STATE_ALL,
                (LPBYTE)pServiceStatus,
                BufSize,
                &BytesNeeded,
                &NumServices,
                &ResumeHandle,
                0);

    /* exit on failure */
    if (GetLastError() != ERROR_MORE_DATA)
    {
        ReportLastError();
        return FALSE;
    }
    
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
        dprintf("Call to EnumServicesStatusEx failed : ");
        ReportLastError();
        return FALSE;
    }
/*
    for (i=0; i<NumServices; i++)
    {
        if (Args == NULL)
        {
            if (pServiceStatus[i]->dwServiceType == SERVICE_WIN32 &&
                pServiceStatus[i]->dwServiceState == SERVICE_ACTIVE)
                PrintService(pServiceStatus[i], bExtended);
            continue;
        }
        
        if(_tcsicmp(Args[0], _T("type="))

        else if(_tcsicmp(Args[0], _T("state="))

        else if(_tcsicmp(Args[0], _T("bufsize="))

        else if(_tcsicmp(Args[0], _T("ri="))
        
        else if(_tcsicmp(Args[0], _T("group="))

*/


}


VOID
PrintService(ENUM_SERVICE_STATUS_PROCESS *pServiceStatus,
                    BOOL bExtended)
{
    dprintf("SERVICE_NAME: %s\n", pServiceStatus->lpServiceName);
    dprintf("DISPLAY_NAME: %s\n", pServiceStatus->lpDisplayName);
    dprintf("TYPE               : %lu\n",
        pServiceStatus->ServiceStatusProcess.dwServiceType);
    dprintf("STATE              : %lu\n",
        pServiceStatus->ServiceStatusProcess.dwCurrentState);
                        //    (STOPPABLE,NOT_PAUSABLE,ACCEPTS_SHUTDOWN)
    dprintf("WIN32_EXIT_CODE    : %lu \n",
        pServiceStatus->ServiceStatusProcess.dwWin32ExitCode);
    dprintf("SERVICE_EXIT_CODE  : %lu \n",
        pServiceStatus->ServiceStatusProcess.dwServiceSpecificExitCode);
    dprintf("CHECKPOINT         : %lu\n",
        pServiceStatus->ServiceStatusProcess.dwCheckPoint);
    dprintf("WAIT_HINT          : %lu\n",
        pServiceStatus->ServiceStatusProcess.dwWaitHint);
    if (bExtended)
    {
        dprintf("PID                : %lu\n",
            pServiceStatus->ServiceStatusProcess.dwProcessId);
        dprintf("FLAGS              : %lu\n",
            pServiceStatus->ServiceStatusProcess.dwServiceFlags);
    }
}
