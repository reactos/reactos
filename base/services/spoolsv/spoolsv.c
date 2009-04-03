/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/spoolsv/spoolsv.c
 * PURPOSE:          Printer spooler
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#define WIN32_NO_STATUS
#include <windows.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

#define SERVICE_NAME TEXT("Spooler")

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

    DPRINT("Spoolsv: main() started\n");

    StartServiceCtrlDispatcher(ServiceTable);

    DPRINT("Spoolsv: main() done\n");

    ExitThread(0);

    return 0;
}

/* EOF */
