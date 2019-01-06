/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for service networking
 * PROGRAMMER:      Pierre Schweitzer
 */

#include "precomp.h"

#include "svchlp.h"

#define WIN32_NO_STATUS
#include <iphlpapi.h>
#include <winsock2.h>

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

static DWORD GetExtendedTcpTableWithAlloc(PVOID *TcpTable, BOOL Order, DWORD Family, TCP_TABLE_CLASS Class)
{
    DWORD ret;
    DWORD Size = 0;

    *TcpTable = NULL;

    ret = GetExtendedTcpTable(*TcpTable, &Size, Order, Family, Class, 0);
    if (ret == ERROR_INSUFFICIENT_BUFFER)
    {
        *TcpTable = HeapAlloc(GetProcessHeap(), 0, Size);
        if (*TcpTable == NULL)
        {
            return ERROR_OUTOFMEMORY;
        }

        ret = GetExtendedTcpTable(*TcpTable, &Size, Order, Family, Class, 0);
        if (ret != NO_ERROR)
        {
            HeapFree(GetProcessHeap(), 0, *TcpTable);
            *TcpTable = NULL;
        }
    }

    return ret;
}

static DWORD GetExtendedUdpTableWithAlloc(PVOID *UdpTable, BOOL Order, DWORD Family, UDP_TABLE_CLASS Class)
{
    DWORD ret;
    DWORD Size = 0;

    *UdpTable = NULL;

    ret = GetExtendedUdpTable(*UdpTable, &Size, Order, Family, Class, 0);
    if (ret == ERROR_INSUFFICIENT_BUFFER)
    {
        *UdpTable = HeapAlloc(GetProcessHeap(), 0, Size);
        if (*UdpTable == NULL)
        {
            return ERROR_OUTOFMEMORY;
        }

        ret = GetExtendedUdpTable(*UdpTable, &Size, Order, Family, Class, 0);
        if (ret != NO_ERROR)
        {
            HeapFree(GetProcessHeap(), 0, *UdpTable);
            *UdpTable = NULL;
        }
    }

    return ret;
}

static void
test_tcp(LPWSTR svc_name, DWORD service_tag)
{
    SOCKET sock;
    SOCKADDR_IN server;
    PMIB_TCPTABLE_OWNER_MODULE TcpTableOwnerMod;
    DWORD i, ret;
    BOOLEAN Found;
    DWORD Pid = GetCurrentProcessId();

    sock = socket(AF_INET, SOCK_STREAM, 0);
    service_ok(sock != INVALID_SOCKET, "Socket creation failed!\n");

    ZeroMemory(&server, sizeof(SOCKADDR_IN));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(9876);

    ret = bind(sock, (SOCKADDR*)&server, sizeof(SOCKADDR_IN));
    service_ok(ret != SOCKET_ERROR, "binding failed\n");

    ret = listen(sock, SOMAXCONN);
    service_ok(ret != SOCKET_ERROR, "listening failed\n");

    ret = GetExtendedTcpTableWithAlloc((PVOID *)&TcpTableOwnerMod, TRUE, AF_INET, TCP_TABLE_OWNER_MODULE_LISTENER);
    service_ok(ret == ERROR_SUCCESS, "GetExtendedTcpTableWithAlloc failed: %x\n", ret);
    if (ret == ERROR_SUCCESS)
    {
        service_ok(TcpTableOwnerMod->dwNumEntries > 0, "No TCP connections?!\n");

        Found = FALSE;
        for (i = 0; i < TcpTableOwnerMod->dwNumEntries; ++i)
        {
            if (TcpTableOwnerMod->table[i].dwState == MIB_TCP_STATE_LISTEN &&
                TcpTableOwnerMod->table[i].dwLocalAddr == 0 &&
                TcpTableOwnerMod->table[i].dwLocalPort == htons(9876) &&
                TcpTableOwnerMod->table[i].dwRemoteAddr == 0)
            {
                Found = TRUE;
                break;
            }
        }

        service_ok(Found, "Our socket wasn't found!\n");
        if (Found)
        {
            DWORD Size = 0;
            PTCPIP_OWNER_MODULE_BASIC_INFO BasicInfo = NULL;

            service_ok(TcpTableOwnerMod->table[i].dwOwningPid == Pid, "Invalid owner\n");
            service_ok((DWORD)(TcpTableOwnerMod->table[i].OwningModuleInfo[0]) == service_tag, "Invalid tag: %x - %x\n", (DWORD)TcpTableOwnerMod->table[i].OwningModuleInfo[0], service_tag);

            ret = GetOwnerModuleFromTcpEntry(&TcpTableOwnerMod->table[i], TCPIP_OWNER_MODULE_INFO_BASIC, BasicInfo, &Size);
            service_ok(ret == ERROR_INSUFFICIENT_BUFFER, "GetOwnerModuleFromTcpEntry failed with: %x\n", ret);
            if (ERROR_INSUFFICIENT_BUFFER == ERROR_INSUFFICIENT_BUFFER)
            {
                BasicInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);
                service_ok(BasicInfo != NULL, "HeapAlloc failed\n");

                ret = GetOwnerModuleFromTcpEntry(&TcpTableOwnerMod->table[i], TCPIP_OWNER_MODULE_INFO_BASIC, BasicInfo, &Size);
                service_ok(ret == ERROR_SUCCESS, "GetOwnerModuleFromTcpEntry failed with: %x\n", ret);
                service_ok(_wcsicmp(svc_name, BasicInfo->pModulePath) == 0, "Mismatching names (%S, %S)\n", svc_name, BasicInfo->pModulePath);
                service_ok(_wcsicmp(svc_name, BasicInfo->pModuleName) == 0, "Mismatching names (%S, %S)\n", svc_name, BasicInfo->pModuleName);
            }
        }

        HeapFree(GetProcessHeap(), 0, TcpTableOwnerMod);
    }

    closesocket(sock);
}

static void
test_udp(LPWSTR svc_name, DWORD service_tag)
{
    SOCKET sock;
    SOCKADDR_IN server;
    PMIB_UDPTABLE_OWNER_MODULE UdpTableOwnerMod;
    DWORD i, ret;
    BOOLEAN Found;
    DWORD Pid = GetCurrentProcessId();

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    service_ok(sock != INVALID_SOCKET, "Socket creation failed!\n");

    ZeroMemory(&server, sizeof(SOCKADDR_IN));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(9876);

    ret = bind(sock, (SOCKADDR*)&server, sizeof(SOCKADDR_IN));
    service_ok(ret != SOCKET_ERROR, "binding failed\n");

    ret = GetExtendedUdpTableWithAlloc((PVOID *)&UdpTableOwnerMod, TRUE, AF_INET, UDP_TABLE_OWNER_MODULE);
    service_ok(ret == ERROR_SUCCESS, "GetExtendedUdpTableWithAlloc failed: %x\n", ret);
    if (ret == ERROR_SUCCESS)
    {
        service_ok(UdpTableOwnerMod->dwNumEntries > 0, "No TCP connections?!\n");

        Found = FALSE;
        for (i = 0; i < UdpTableOwnerMod->dwNumEntries; ++i)
        {
            if (UdpTableOwnerMod->table[i].dwLocalAddr == 0 &&
                UdpTableOwnerMod->table[i].dwLocalPort == htons(9876))
            {
                Found = TRUE;
                break;
            }
        }

        service_ok(Found, "Our socket wasn't found!\n");
        if (Found)
        {
            DWORD Size = 0;
            PTCPIP_OWNER_MODULE_BASIC_INFO BasicInfo = NULL;

            service_ok(UdpTableOwnerMod->table[i].dwOwningPid == Pid, "Invalid owner\n");
            service_ok((DWORD)(UdpTableOwnerMod->table[i].OwningModuleInfo[0]) == service_tag, "Invalid tag: %x - %x\n", (DWORD)UdpTableOwnerMod->table[i].OwningModuleInfo[0], service_tag);

            ret = GetOwnerModuleFromUdpEntry(&UdpTableOwnerMod->table[i], TCPIP_OWNER_MODULE_INFO_BASIC, BasicInfo, &Size);
            service_ok(ret == ERROR_INSUFFICIENT_BUFFER, "GetOwnerModuleFromUdpEntry failed with: %x\n", ret);
            if (ret == ERROR_INSUFFICIENT_BUFFER)
            {
                BasicInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);
                service_ok(BasicInfo != NULL, "HeapAlloc failed\n");

                ret = GetOwnerModuleFromUdpEntry(&UdpTableOwnerMod->table[i], TCPIP_OWNER_MODULE_INFO_BASIC, BasicInfo, &Size);
                service_ok(ret == ERROR_SUCCESS, "GetOwnerModuleFromUdpEntry failed with: %x\n", ret);
                service_ok(_wcsicmp(svc_name, BasicInfo->pModulePath) == 0, "Mismatching names (%S, %S)\n", svc_name, BasicInfo->pModulePath);
                service_ok(_wcsicmp(svc_name, BasicInfo->pModuleName) == 0, "Mismatching names (%S, %S)\n", svc_name, BasicInfo->pModuleName);
            }
        }

        HeapFree(GetProcessHeap(), 0, UdpTableOwnerMod);
    }

    closesocket(sock);
}

static void WINAPI
service_main(DWORD dwArgc, LPWSTR* lpszArgv)
{
    // SERVICE_STATUS_HANDLE status_handle;
    PTEB Teb;
    WSADATA wsaData;

    UNREFERENCED_PARAMETER(dwArgc);

    /* Register our service for control (lpszArgv[0] holds the service name) */
    status_handle = RegisterServiceCtrlHandlerW(lpszArgv[0], service_handler);
    service_ok(status_handle != NULL, "RegisterServiceCtrlHandler failed: %lu\n", GetLastError());
    if (!status_handle)
        return;

    /* Report SERVICE_RUNNING status */
    report_service_status(SERVICE_RUNNING, NO_ERROR, 4000);

    /* Check our tag is not 0 */
    Teb = NtCurrentTeb();
    service_ok(Teb->SubProcessTag != 0, "SubProcessTag is not defined!\n");

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        skip("Failed to init WS2\n");
        goto quit;
    }

    test_tcp(lpszArgv[0], PtrToUlong(Teb->SubProcessTag));
    test_udp(lpszArgv[0], PtrToUlong(Teb->SubProcessTag));

    WSACleanup();
quit:
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
    hService = register_service_exW(hSC, L"ServiceNetwork", service_nameW, NULL,
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

START_TEST(ServiceNetwork)
{
    int argc;
    char** argv;

    /* Check whether this test is started as a separated service process */
    argc = winetest_get_mainargs(&argv);
    if (argc >= 3)
    {
        service_process(start_service, argc, argv);
        return;
    }

    /* We are started as the real test */
    test_runner(my_test_server, NULL);
    // trace("Returned from test_runner\n");
}
