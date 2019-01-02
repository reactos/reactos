/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for service process environment block
 * PROGRAMMER:      Hermes Belusca-Maito
 */

#include "precomp.h"

#include "svchlp.h"


/*** Service part of the test ***/

static SERVICE_STATUS_HANDLE status_handle;

static void
report_service_status(DWORD dwCurrentState,
                      DWORD dwWin32ExitCode,
                      DWORD dwWaitHint)
{
    BOOL res;
    SERVICE_STATUS status;

    status.dwServiceType   = SERVICE_WIN32_OWN_PROCESS;
    status.dwCurrentState  = dwCurrentState;
    status.dwWin32ExitCode = dwWin32ExitCode;
    status.dwWaitHint      = dwWaitHint;

    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;

    if ( (dwCurrentState == SERVICE_START_PENDING) ||
         (dwCurrentState == SERVICE_STOP_PENDING)  ||
         (dwCurrentState == SERVICE_STOPPED) )
    {
        status.dwControlsAccepted = 0;
    }
    else
    {
        status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    }

#if 0
    if ( (dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED) )
        status.dwCheckPoint = 0;
    else
        status.dwCheckPoint = dwCheckPoint++;
#endif

    res = SetServiceStatus(status_handle, &status);
    service_ok(res, "SetServiceStatus(%d) failed: %lu\n", dwCurrentState, GetLastError());
}

static VOID WINAPI service_handler(DWORD ctrl)
{
    switch(ctrl)
    {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        report_service_status(SERVICE_STOP_PENDING, NO_ERROR, 0);
    default:
        report_service_status(SERVICE_RUNNING, NO_ERROR, 0);
    }
}

static void WINAPI
service_main(DWORD dwArgc, LPWSTR* lpszArgv)
{
    // SERVICE_STATUS_HANDLE status_handle;
    LPWSTR lpEnvironment, lpEnvStr;
    DWORD dwSize;
    PTEB Teb;

    UNREFERENCED_PARAMETER(dwArgc);
    UNREFERENCED_PARAMETER(lpszArgv);

    /* Register our service for control (lpszArgv[0] holds the service name) */
    status_handle = RegisterServiceCtrlHandlerW(lpszArgv[0], service_handler);
    service_ok(status_handle != NULL, "RegisterServiceCtrlHandler failed: %lu\n", GetLastError());
    if (!status_handle)
        return;

    /* Report SERVICE_RUNNING status */
    report_service_status(SERVICE_RUNNING, NO_ERROR, 4000);

    /* Display our current environment for informative purposes */
    lpEnvironment = GetEnvironmentStringsW();
    lpEnvStr = lpEnvironment;
    while (*lpEnvStr)
    {
        service_trace("%S\n", lpEnvStr);
        lpEnvStr += wcslen(lpEnvStr) + 1;
    }
    FreeEnvironmentStringsW(lpEnvironment);

    /* Check the presence of the user-related environment variables */
    dwSize = GetEnvironmentVariableW(L"ALLUSERSPROFILE", NULL, 0);
    service_ok(dwSize != 0, "ALLUSERSPROFILE envvar not found, or GetEnvironmentVariableW failed: %lu\n", GetLastError());
    dwSize = GetEnvironmentVariableW(L"USERPROFILE", NULL, 0);
    service_ok(dwSize != 0, "USERPROFILE envvar not found, or GetEnvironmentVariableW failed: %lu\n", GetLastError());
#if 0 // May not always exist
    dwSize = GetEnvironmentVariableW(L"USERNAME", NULL, 0);
    service_ok(dwSize != 0, "USERNAME envvar not found, or GetEnvironmentVariableW failed: %lu\n", GetLastError());
#endif

    Teb = NtCurrentTeb();
    service_ok(Teb->SubProcessTag != 0, "SubProcessTag is not defined!\n");
    if (Teb->SubProcessTag != 0)
    {
        ULONG (NTAPI *_I_QueryTagInformation)(PVOID, DWORD, PVOID) = (PVOID)GetProcAddress(GetModuleHandle("advapi32.dll"), "I_QueryTagInformation");
        if (_I_QueryTagInformation != NULL)
        {
            /* IN/OUT parameter structure for I_QueryTagInformation() function
             * See: https://wj32.org/wp/2010/03/30/howto-use-i_querytaginformation/
             * See: https://github.com/processhacker/processhacker/blob/master/phnt/include/subprocesstag.h
             */
            struct
            {
                ULONG ProcessId;
                PVOID ServiceTag;
                ULONG TagType;
                PWSTR Buffer;
            } ServiceQuery;

            /* Set our input parameters */
            ServiceQuery.ProcessId = GetCurrentProcessId();
            ServiceQuery.ServiceTag = Teb->SubProcessTag;
            ServiceQuery.TagType = 0;
            ServiceQuery.Buffer = NULL;
            /* Call ADVAPI32 to query the correctness of our tag */
            _I_QueryTagInformation(NULL, 1, &ServiceQuery);

            /* If buffer is not NULL, call succeed */
            if (ServiceQuery.Buffer != NULL)
            {
                /* It should match our service name */
                service_ok(wcscmp(lpszArgv[0], ServiceQuery.Buffer) == 0, "Mismatching info: %S - %S\n", lpszArgv[0], ServiceQuery.Buffer);
                service_ok(ServiceQuery.TagType == 1, "Invalid tag type: %x\n", ServiceQuery.TagType);
                LocalFree(ServiceQuery.Buffer);
            }
        }
    }

    /* Work is done */
    report_service_status(SERVICE_STOPPED, NO_ERROR, 0);
}

static BOOL start_service(PCSTR service_nameA, PCWSTR service_nameW)
{
    BOOL res;

    SERVICE_TABLE_ENTRYW servtbl[] =
    {
        { (PWSTR)service_nameW, service_main },
        { NULL, NULL }
    };

    res = StartServiceCtrlDispatcherW(servtbl);
    service_ok(res, "StartServiceCtrlDispatcherW failed: %lu\n", GetLastError());
    return res;
}


/*** Tester part of the test ***/

static void
my_test_server(PCSTR service_nameA,
               PCWSTR service_nameW,
               void *param)
{
    BOOL res;
    SC_HANDLE hSC = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS ServiceStatus;

    /* Open the SCM */
    hSC = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSC)
    {
        skip("OpenSCManagerW failed with error %lu!\n", GetLastError());
        return;
    }

    /* First create ourselves as a service running in the default LocalSystem account */
    hService = register_service_exW(hSC, L"ServiceEnv", service_nameW, NULL,
                                    SERVICE_ALL_ACCESS,
                                    SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
                                    SERVICE_DEMAND_START,
                                    SERVICE_ERROR_IGNORE,
                                    NULL, NULL, NULL,
                                    NULL, NULL);
    if (!hService)
    {
        skip("CreateServiceW failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    /* Start it */
    if (!StartServiceW(hService, 0, NULL))
    {
        skip("StartServiceW failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    /* Wait for the service to stop by itself */
    do
    {
        Sleep(100);
        ZeroMemory(&ServiceStatus, sizeof(ServiceStatus));
        res = QueryServiceStatus(hService, &ServiceStatus);
    } while (res && ServiceStatus.dwCurrentState != SERVICE_STOPPED);
    ok(res, "QueryServiceStatus failed: %lu\n", GetLastError());
    ok(ServiceStatus.dwCurrentState == SERVICE_STOPPED, "ServiceStatus.dwCurrentState = %lx\n", ServiceStatus.dwCurrentState);

    /* Be sure the service is really stopped */
    res = ControlService(hService, SERVICE_CONTROL_STOP, &ServiceStatus);
    if (!res && ServiceStatus.dwCurrentState != SERVICE_STOPPED &&
        ServiceStatus.dwCurrentState != SERVICE_STOP_PENDING &&
        GetLastError() != ERROR_SERVICE_NOT_ACTIVE)
    {
        skip("ControlService failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

#if 0
    trace("Service stopped. Going to restart it...\n");

    /* Now change the service configuration to make it start under the NetworkService account */
    if (!ChangeServiceConfigW(hService,
                              SERVICE_NO_CHANGE,
                              SERVICE_NO_CHANGE,
                              SERVICE_NO_CHANGE,
                              NULL, NULL, NULL, NULL,
                              L"NT AUTHORITY\\NetworkService", L"",
                              NULL))
    {
        skip("ChangeServiceConfigW failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    /* Start it */
    if (!StartServiceW(hService, 0, NULL))
    {
        skip("StartServiceW failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    /* Wait for the service to stop by itself */
    do
    {
        Sleep(100);
        ZeroMemory(&ServiceStatus, sizeof(ServiceStatus));
        res = QueryServiceStatus(hService, &ServiceStatus);
    } while (res && ServiceStatus.dwCurrentState != SERVICE_STOPPED);
    ok(res, "QueryServiceStatus failed: %lu\n", GetLastError());
    ok(ServiceStatus.dwCurrentState == SERVICE_STOPPED, "ServiceStatus.dwCurrentState = %lx\n", ServiceStatus.dwCurrentState);

    /* Be sure the service is really stopped */
    res = ControlService(hService, SERVICE_CONTROL_STOP, &ServiceStatus);
    if (!res && ServiceStatus.dwCurrentState != SERVICE_STOPPED &&
        ServiceStatus.dwCurrentState != SERVICE_STOP_PENDING &&
        GetLastError() != ERROR_SERVICE_NOT_ACTIVE)
    {
        skip("ControlService failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }
#endif

Cleanup:
    if (hService)
    {
        res = DeleteService(hService);
        ok(res, "DeleteService failed: %lu\n", GetLastError());
        CloseServiceHandle(hService);
    }

    if (hSC)
        CloseServiceHandle(hSC);
}

START_TEST(ServiceEnv)
{
    int argc;
    char** argv;
    PTEB Teb;

    /* Check whether this test is started as a separated service process */
    argc = winetest_get_mainargs(&argv);
    if (argc >= 3)
    {
        service_process(start_service, argc, argv);
        return;
    }

    Teb = NtCurrentTeb();
    ok(Teb->SubProcessTag == 0, "SubProcessTag is defined: %p\n", Teb->SubProcessTag);

    /* We are started as the real test */
    test_runner(my_test_server, NULL);
    // trace("Returned from test_runner\n");
}
