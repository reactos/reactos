/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:            cmdContinue.c
 * PURPOSE:
 *
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#include "net.h"

int cmdContinue(int argc, wchar_t *argv[])
{
    int errorCode = 0;
    SC_HANDLE hManager, hService;
    SERVICE_STATUS status;
    if(argc != 3)
    {
        puts("Usage: NET CONTINUE <Service Name>");
        return 1;
    }

    hManager=OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ENUMERATE_SERVICE);
    if(hManager == NULL)
    {
        printf("[OpenSCManager] Error: %d\n", errorCode = GetLastError());
        return errorCode;
    }
    
    hService = OpenService(hManager, argv[2], SERVICE_PAUSE_CONTINUE);
    
    if(hService == NULL)
    {
        printf("[OpenService] Error: %d\n", errorCode=GetLastError());
        CloseServiceHandle(hManager);
        return errorCode;
    }

    if(!ControlService(hService, SERVICE_CONTROL_CONTINUE, &status))
    {
        printf("[ControlService] Error: %d\n", errorCode=GetLastError());
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hManager);
    
    return errorCode;
}

/* EOF */

