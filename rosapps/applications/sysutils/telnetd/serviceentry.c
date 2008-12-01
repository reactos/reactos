/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/TelnetD/TelnetD.c
 * PURPOSE:          Printer spooler
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "telnetd.h"
#define DPRINT printf

/* GLOBALS ******************************************************************/

#define SERVICE_NAME TEXT("TelnetD")

SERVICE_STATUS_HANDLE ServiceStatusHandle;


/* FUNCTIONS *****************************************************************/


static DWORD WINAPI
ServiceControlHandler(DWORD dwControl,
                      DWORD dwEventType,
                      LPVOID lpEventData,
                      LPVOID lpContext)
{
    switch (dwControl)
    {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            return ERROR_SUCCESS;

        default :
            return ERROR_CALL_NOT_IMPLEMENTED;
    }
}



static VOID CALLBACK
ServiceMain(DWORD argc, LPTSTR *argv)
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    DPRINT("ServiceMain() called\n");

    ServiceStatusHandle = RegisterServiceCtrlHandlerExW(SERVICE_NAME,
                                                        ServiceControlHandler,
                                                        NULL);

    DPRINT("ServiceMain() done\n");
}


int
wmain(int argc, WCHAR *argv[])
{
    SERVICE_TABLE_ENTRY ServiceTable[2] =
    {
        {SERVICE_NAME, ServiceMain},
        {NULL, NULL}
    };

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    DPRINT("TelnetD: main() started\n");

    StartServiceCtrlDispatcher(ServiceTable);

	telnetd_main();

    DPRINT("TelnetD: main() done\n");

    ExitThread(0);

    return 0;
}

/* EOF */
