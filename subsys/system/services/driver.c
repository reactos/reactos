/*
 * driver.c
 */

/* INCLUDES *****************************************************************/

#include "services.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

NTSTATUS
ScmLoadDriver(PSERVICE lpService)
{
    WCHAR szDriverPath[MAX_PATH];
    UNICODE_STRING DriverPath;
    NTSTATUS Status;

    /* Build the driver path */
    wcscpy(szDriverPath,
           L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
    wcscat(szDriverPath,
           lpService->lpServiceName);

    RtlInitUnicodeString(&DriverPath,
                         szDriverPath);

    /* FIXME: Acquire privilege */

    DPRINT("  Path: %wZ\n", &DriverPath);
    Status = NtLoadDriver(&DriverPath);

    /* FIXME: Release privilege */

    return Status;
}


DWORD
ScmUnloadDriver(PSERVICE lpService)
{
    WCHAR szDriverPath[MAX_PATH];
    UNICODE_STRING DriverPath;
    NTSTATUS Status;
    DWORD dwError = ERROR_SUCCESS;

    /* Build the driver path */
    wcscpy(szDriverPath,
           L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
    wcscat(szDriverPath,
           lpService->lpServiceName);

    RtlInitUnicodeString(&DriverPath,
                         szDriverPath);

    /* FIXME: Acquire privilege */

    Status = NtUnloadDriver(&DriverPath);

    /* FIXME: Release privilege */

    if (!NT_SUCCESS(Status))
    {
        dwError = RtlNtStatusToDosError(Status);
    }

    return dwError;
}


DWORD
ScmControlDriver(PSERVICE lpService,
                 DWORD dwControl,
                 LPSERVICE_STATUS lpServiceStatus)
{
    DWORD dwError;

    DPRINT("ScmControlDriver() called\n");

    switch (dwControl)
    {
        case SERVICE_CONTROL_STOP:
            if (lpService->Status.dwCurrentState != SERVICE_RUNNING)
            {
                dwError = ERROR_INVALID_SERVICE_CONTROL;
                goto done;
            }

            dwError = ScmUnloadDriver(lpService);
            if (dwError == ERROR_SUCCESS)
            {
                lpService->Status.dwControlsAccepted = 0;
                lpService->Status.dwCurrentState = SERVICE_STOPPED;
            }
            break;

        case SERVICE_CONTROL_INTERROGATE:
            dwError = ERROR_INVALID_SERVICE_CONTROL;
            break;

        default:
            dwError = ERROR_INVALID_SERVICE_CONTROL;
    }

done:;
    DPRINT("ScmControlDriver() done (Erorr: %lu)\n", dwError);

    return dwError;
}

/* EOF */
