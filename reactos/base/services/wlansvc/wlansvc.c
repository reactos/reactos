/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/wlansvc/wlansvc.c
 * PURPOSE:          WLAN Service
 * PROGRAMMER:       Christoph von Wittich
 */

/* INCLUDES *****************************************************************/

#define WIN32_NO_STATUS
#include <windows.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

#define SERVICE_NAME L"WLAN Service"

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
ServiceMain(DWORD argc, LPWSTR *argv)
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
    SERVICE_TABLE_ENTRYW ServiceTable[2] =
    {
        {SERVICE_NAME, ServiceMain},
        {NULL, NULL}
    };

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    DPRINT("wlansvc: main() started\n");

    StartServiceCtrlDispatcherW(ServiceTable);

    DPRINT("wlansvc: main() done\n");

    ExitThread(0);

    return 0;
}

/* EOF */
