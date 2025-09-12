/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/dhcpcapi/dhcpcapi.c
 * PURPOSE:         Client API for DHCP
 * COPYRIGHT:       Copyright 2005 Art Yerkes <ayerkes@speakeasy.net>
 */

#include <rosdhcp.h>
#include <winsvc.h>

#define NDEBUG
#include <debug.h>

static WCHAR ServiceName[] = L"DHCP";

SERVICE_STATUS_HANDLE ServiceStatusHandle = 0;
SERVICE_STATUS ServiceStatus;
HANDLE hStopEvent = NULL;
HANDLE hAdapterStateChangedEvent = NULL;

extern SOCKET DhcpSocket;


void __RPC_FAR * __RPC_USER MIDL_user_allocate(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}

void __RPC_USER MIDL_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}

handle_t __RPC_USER
PDHCP_SERVER_NAME_bind(
    _In_ PDHCP_SERVER_NAME pszServerName)
{
    handle_t hBinding = NULL;
    LPWSTR pszStringBinding;
    RPC_STATUS status;

    DPRINT("PDHCP_SERVER_NAME_bind() called\n");

    status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      pszServerName,
                                      L"\\pipe\\dhcpcsvc",
                                      NULL,
                                      &pszStringBinding);
    if (status)
    {
        DPRINT1("RpcStringBindingCompose returned 0x%x\n", status);
        return NULL;
    }

    /* Set the binding handle that will be used to bind to the server. */
    status = RpcBindingFromStringBindingW(pszStringBinding,
                                          &hBinding);
    if (status)
    {
        DPRINT1("RpcBindingFromStringBinding returned 0x%x\n", status);
    }

    status = RpcStringFreeW(&pszStringBinding);
    if (status)
    {
        DPRINT1("RpcStringFree returned 0x%x\n", status);
    }

    return hBinding;
}

void __RPC_USER
PDHCP_SERVER_NAME_unbind(
    _In_ PDHCP_SERVER_NAME pszServerName,
    _In_ handle_t hBinding)
{
    RPC_STATUS status;

    DPRINT("PDHCP_SERVER_NAME_unbind() called\n");

    status = RpcBindingFree(&hBinding);
    if (status)
    {
        DPRINT1("RpcBindingFree returned 0x%x\n", status);
    }
}


DWORD
APIENTRY
DhcpCApiInitialize(
    _Out_ LPDWORD Version)
{
    *Version = 2;
    return NO_ERROR;
}

VOID
APIENTRY
DhcpCApiCleanup(VOID)
{
}

DWORD
APIENTRY
DhcpAcquireParameters(
    _In_ PWSTR AdapterName)
{
    DWORD ret;

    DPRINT("DhcpAcquireParameters(%S)\n", AdapterName);

    RpcTryExcept
    {
        ret = Client_AcquireParameters(NULL, AdapterName);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}

DWORD
APIENTRY
DhcpReleaseParameters(
    _In_ PWSTR AdapterName)
{
    DWORD ret;

    DPRINT("DhcpReleaseParameters(%S)\n", AdapterName);

    RpcTryExcept
    {
        ret = Client_ReleaseParameters(NULL, AdapterName);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}

DWORD
APIENTRY
DhcpEnumClasses(
    _In_ DWORD Unknown1,
    _In_ PWSTR AdapterName,
    _In_ DWORD Unknown3,
    _In_ DWORD Unknown4)
{
    DPRINT1("DhcpEnumClasses(%lx %S %lx %lx)\n",
            Unknown1, AdapterName, Unknown3, Unknown4);
    return 0;
}

DWORD
APIENTRY
DhcpHandlePnPEvent(
    _In_ DWORD Unknown1,
    _In_ DWORD Unknown2,
    _In_ PWSTR AdapterName,
    _In_ DWORD Unknown4,
    _In_ DWORD Unknown5)
{
    DPRINT1("DhcpHandlePnPEvent(%lx %lx %S %lx %lx)\n",
            Unknown1, Unknown2, AdapterName, Unknown4, Unknown5);
    return 0;
}

DWORD APIENTRY
DhcpQueryHWInfo(DWORD AdapterIndex,
                PDWORD MediaType,
                PDWORD Mtu,
                PDWORD Speed)
{
    DWORD ret;

    DPRINT("DhcpQueryHWInfo()\n");

    RpcTryExcept
    {
        ret = Client_QueryHWInfo(NULL, AdapterIndex, MediaType, Mtu, Speed);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return (ret == ERROR_SUCCESS) ? 1 : 0;
}

DWORD
APIENTRY
DhcpStaticRefreshParams(DWORD AdapterIndex,
                        DWORD Address,
                        DWORD Netmask)
{
    DWORD ret;

    DPRINT("DhcpStaticRefreshParams()\n");

    RpcTryExcept
    {
        ret = Client_StaticRefreshParams(NULL, AdapterIndex, Address, Netmask);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return (ret == ERROR_SUCCESS) ? 1 : 0;
}

/*!
 * Set new TCP/IP parameters and notify DHCP client service of this
 *
 * \param[in] ServerName
 *        NULL for local machine
 *
 * \param[in] AdapterName
 *        IPHLPAPI name of adapter to change
 *
 * \param[in] NewIpAddress
 *        TRUE if IP address changes
 *
 * \param[in] IpAddress
 *        New IP address (network byte order)
 *
 * \param[in] SubnetMask
 *        New subnet mask (network byte order)
 *
 * \param[in] DhcpAction
 *        0 - don't modify
 *        1 - enable DHCP
 *        2 - disable DHCP
 *
 * \return non-zero on success
 *
 * \remarks Undocumented by Microsoft
 */
DWORD APIENTRY
DhcpNotifyConfigChange(LPWSTR ServerName,
                       LPWSTR AdapterName,
                       BOOL NewIpAddress,
                       DWORD IpIndex,
                       DWORD IpAddress,
                       DWORD SubnetMask,
                       INT DhcpAction)
{
    DPRINT1("DHCPCSVC: DhcpNotifyConfigChange not implemented yet\n");
    return 0;
}

DWORD APIENTRY
DhcpRequestParams(DWORD Flags,
                  PVOID Reserved,
                  LPWSTR AdapterName,
                  LPDHCPCAPI_CLASSID ClassId,
                  DHCPCAPI_PARAMS_ARRAY SendParams,
                  DHCPCAPI_PARAMS_ARRAY RecdParams,
                  LPBYTE Buffer,
                  LPDWORD pSize,
                  LPWSTR RequestIdStr)
{
    UNIMPLEMENTED;
    return 0;
}

static VOID
UpdateServiceStatus(DWORD dwState)
{
    ServiceStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
    ServiceStatus.dwCurrentState = dwState;

    if (dwState == SERVICE_RUNNING)
        ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_STOP;
    else
        ServiceStatus.dwControlsAccepted = 0;

    ServiceStatus.dwWin32ExitCode = 0;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwCheckPoint = 0;

    if (dwState == SERVICE_START_PENDING ||
        dwState == SERVICE_STOP_PENDING)
        ServiceStatus.dwWaitHint = 1000;
    else
        ServiceStatus.dwWaitHint = 0;

    SetServiceStatus(ServiceStatusHandle,
                     &ServiceStatus);
}

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
            UpdateServiceStatus(SERVICE_STOP_PENDING);
            if (hStopEvent != NULL)
                SetEvent(hStopEvent);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_INTERROGATE:
            SetServiceStatus(ServiceStatusHandle,
                             &ServiceStatus);
            return ERROR_SUCCESS;

        default:
            return ERROR_CALL_NOT_IMPLEMENTED;
    }
}

VOID WINAPI
ServiceMain(DWORD argc, LPWSTR* argv)
{
    HANDLE hPipeThread = INVALID_HANDLE_VALUE;
    HANDLE hDiscoveryThread = INVALID_HANDLE_VALUE;
    DWORD ret;

    ServiceStatusHandle = RegisterServiceCtrlHandlerExW(ServiceName,
                                                        ServiceControlHandler,
                                                        NULL);
    if (!ServiceStatusHandle)
    {
        DPRINT1("DHCPCSVC: Unable to register service control handler (%lx)\n", GetLastError());
        return;
    }

    UpdateServiceStatus(SERVICE_START_PENDING);

    /* Create the stop event */
    hStopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (hStopEvent == NULL)
    {
        UpdateServiceStatus(SERVICE_STOPPED);
        return;
    }

    hAdapterStateChangedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hAdapterStateChangedEvent == NULL)
    {
        CloseHandle(hStopEvent);
        UpdateServiceStatus(SERVICE_STOPPED);
        return;
    }

    UpdateServiceStatus(SERVICE_START_PENDING);

    if (!init_client())
    {
        DbgPrint("DHCPCSVC: init_client() failed!\n");
        CloseHandle(hAdapterStateChangedEvent);
        CloseHandle(hStopEvent);
        UpdateServiceStatus(SERVICE_STOPPED);
        return;
    }

    UpdateServiceStatus(SERVICE_START_PENDING);

    hPipeThread = InitRpc();
    if (hPipeThread == INVALID_HANDLE_VALUE)
    {
        DbgPrint("DHCPCSVC: PipeInit() failed!\n");
        stop_client();
        CloseHandle(hAdapterStateChangedEvent);
        CloseHandle(hStopEvent);
        UpdateServiceStatus(SERVICE_STOPPED);
        return;
    }

    hDiscoveryThread = StartAdapterDiscovery(hStopEvent);
    if (hDiscoveryThread == INVALID_HANDLE_VALUE)
    {
        DbgPrint("DHCPCSVC: StartAdapterDiscovery() failed!\n");
        ShutdownRpc();
        stop_client();
        CloseHandle(hAdapterStateChangedEvent);
        CloseHandle(hStopEvent);
        UpdateServiceStatus(SERVICE_STOPPED);
        return;
    }

    DH_DbgPrint(MID_TRACE,("DHCP Service Started\n"));

    UpdateServiceStatus(SERVICE_RUNNING);

    DH_DbgPrint(MID_TRACE,("Going into dispatch()\n"));
    DH_DbgPrint(MID_TRACE,("DHCPCSVC: DHCP service is starting up\n"));

    dispatch(hStopEvent);

    DH_DbgPrint(MID_TRACE,("DHCPCSVC: DHCP service is shutting down\n"));

    ShutdownRpc();
    stop_client();

    DPRINT("Wait for pipe thread to close! %p\n", hPipeThread);
    if (hPipeThread != INVALID_HANDLE_VALUE)
    {
        DPRINT("Waiting for pipe thread\n");
        ret = WaitForSingleObject(hPipeThread, 5000);
        DPRINT("Done %lx\n", ret);
    }

    DPRINT("Wait for discovery thread to close! %p\n", hDiscoveryThread);
    if (hDiscoveryThread != INVALID_HANDLE_VALUE)
    {
        DPRINT("Waiting for discovery thread\n");
        ret = WaitForSingleObject(hDiscoveryThread, 5000);
        DPRINT("Done %lx\n", ret);
    }

    DPRINT("Closing events!\n");
    CloseHandle(hAdapterStateChangedEvent);
    CloseHandle(hStopEvent);

    if (DhcpSocket != INVALID_SOCKET)
        closesocket(DhcpSocket);

    CloseHandle(hDiscoveryThread);
    CloseHandle(hPipeThread);

    DPRINT("Done!\n");

    UpdateServiceStatus(SERVICE_STOPPED);
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL,
        DWORD fdwReason,
        LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}

/* EOF */
