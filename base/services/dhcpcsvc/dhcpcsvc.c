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

static HANDLE PipeHandle = INVALID_HANDLE_VALUE;
extern SOCKET DhcpSocket;

DWORD APIENTRY
DhcpCApiInitialize(LPDWORD Version)
{
    DWORD PipeMode;

    /* Wait for the pipe to be available */
    if (!WaitNamedPipeW(DHCP_PIPE_NAME, NMPWAIT_USE_DEFAULT_WAIT))
    {
        /* No good, we failed */
        return GetLastError();
    }

    /* It's available, let's try to open it */
    PipeHandle = CreateFileW(DHCP_PIPE_NAME,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL);

    /* Check if we succeeded in opening the pipe */
    if (PipeHandle == INVALID_HANDLE_VALUE)
    {
        /* We didn't */
        return GetLastError();
    }

    /* Change the pipe into message mode */
    PipeMode = PIPE_READMODE_MESSAGE;
    if (!SetNamedPipeHandleState(PipeHandle, &PipeMode, NULL, NULL))
    {
        /* Mode change failed */
        CloseHandle(PipeHandle);
        PipeHandle = INVALID_HANDLE_VALUE;
        return GetLastError();
    }
    else
    {
        /* We're good to go */
        *Version = 2;
        return NO_ERROR;
    }
}

VOID APIENTRY
DhcpCApiCleanup(VOID)
{
    CloseHandle(PipeHandle);
    PipeHandle = INVALID_HANDLE_VALUE;
}

DWORD APIENTRY
DhcpQueryHWInfo(DWORD AdapterIndex,
                PDWORD MediaType,
                PDWORD Mtu,
                PDWORD Speed)
{
    COMM_DHCP_REQ Req;
    COMM_DHCP_REPLY Reply;
    DWORD BytesRead;
    BOOL Result;

    ASSERT(PipeHandle != INVALID_HANDLE_VALUE);

    Req.Type = DhcpReqQueryHWInfo;
    Req.AdapterIndex = AdapterIndex;

    Result = TransactNamedPipe(PipeHandle,
                               &Req, sizeof(Req),
                               &Reply, sizeof(Reply),
                               &BytesRead, NULL);
    if (!Result)
    {
        /* Pipe transaction failed */
        return 0;
    }

    if (Reply.Reply == 0)
        return 0;

    *MediaType = Reply.QueryHWInfo.MediaType;
    *Mtu = Reply.QueryHWInfo.Mtu;
    *Speed = Reply.QueryHWInfo.Speed;
    return 1;
}

DWORD APIENTRY
DhcpLeaseIpAddress(DWORD AdapterIndex)
{
    COMM_DHCP_REQ Req;
    COMM_DHCP_REPLY Reply;
    DWORD BytesRead;
    BOOL Result;

    ASSERT(PipeHandle != INVALID_HANDLE_VALUE);

    Req.Type = DhcpReqLeaseIpAddress;
    Req.AdapterIndex = AdapterIndex;

    Result = TransactNamedPipe(PipeHandle,
                               &Req, sizeof(Req),
                               &Reply, sizeof(Reply),
                               &BytesRead, NULL);
    if (!Result)
    {
        /* Pipe transaction failed */
        return 0;
    }

    return Reply.Reply;
}

DWORD APIENTRY
DhcpReleaseIpAddressLease(DWORD AdapterIndex)
{
    COMM_DHCP_REQ Req;
    COMM_DHCP_REPLY Reply;
    DWORD BytesRead;
    BOOL Result;

    ASSERT(PipeHandle != INVALID_HANDLE_VALUE);

    Req.Type = DhcpReqReleaseIpAddress;
    Req.AdapterIndex = AdapterIndex;

    Result = TransactNamedPipe(PipeHandle,
                               &Req, sizeof(Req),
                               &Reply, sizeof(Reply),
                               &BytesRead, NULL);
    if (!Result)
    {
        /* Pipe transaction failed */
        return 0;
    }

    return Reply.Reply;
}

DWORD APIENTRY
DhcpRenewIpAddressLease(DWORD AdapterIndex)
{
    COMM_DHCP_REQ Req;
    COMM_DHCP_REPLY Reply;
    DWORD BytesRead;
    BOOL Result;

    ASSERT(PipeHandle != INVALID_HANDLE_VALUE);

    Req.Type = DhcpReqRenewIpAddress;
    Req.AdapterIndex = AdapterIndex;

    Result = TransactNamedPipe(PipeHandle,
                               &Req, sizeof(Req),
                               &Reply, sizeof(Reply),
                               &BytesRead, NULL);
    if (!Result)
    {
        /* Pipe transaction failed */
        return 0;
    }

    return Reply.Reply;
}

DWORD APIENTRY
DhcpStaticRefreshParams(DWORD AdapterIndex,
                        DWORD Address,
                        DWORD Netmask)
{
    COMM_DHCP_REQ Req;
    COMM_DHCP_REPLY Reply;
    DWORD BytesRead;
    BOOL Result;

    ASSERT(PipeHandle != INVALID_HANDLE_VALUE);

    Req.Type = DhcpReqStaticRefreshParams;
    Req.AdapterIndex = AdapterIndex;
    Req.Body.StaticRefreshParams.IPAddress = Address;
    Req.Body.StaticRefreshParams.Netmask = Netmask;

    Result = TransactNamedPipe(PipeHandle,
                               &Req, sizeof(Req),
                               &Reply, sizeof(Reply),
                               &BytesRead, NULL);
    if (!Result)
    {
        /* Pipe transaction failed */
        return 0;
    }

    return Reply.Reply;
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

/*!
 * Get DHCP info for an adapter
 *
 * \param[in] AdapterIndex
 *        Index of the adapter (iphlpapi-style) for which info is
 *        requested
 *
 * \param[out] DhcpEnabled
 *        Returns whether DHCP is enabled for the adapter
 *
 * \param[out] DhcpServer
 *        Returns DHCP server IP address (255.255.255.255 if no
 *        server reached yet), in network byte order
 *
 * \param[out] LeaseObtained
 *        Returns time at which the lease was obtained
 *
 * \param[out] LeaseExpires
 *        Returns time at which the lease will expire
 *
 * \return non-zero on success
 *
 * \remarks This is a ReactOS-only routine
 */
DWORD APIENTRY
DhcpRosGetAdapterInfo(DWORD AdapterIndex,
                      PBOOL DhcpEnabled,
                      PDWORD DhcpServer,
                      time_t* LeaseObtained,
                      time_t* LeaseExpires)
{
    COMM_DHCP_REQ Req;
    COMM_DHCP_REPLY Reply;
    DWORD BytesRead;
    BOOL Result;

    ASSERT(PipeHandle != INVALID_HANDLE_VALUE);

    Req.Type = DhcpReqGetAdapterInfo;
    Req.AdapterIndex = AdapterIndex;

    Result = TransactNamedPipe(PipeHandle,
                               &Req, sizeof(Req),
                               &Reply, sizeof(Reply),
                               &BytesRead, NULL);

    if (Result && Reply.Reply != 0)
        *DhcpEnabled = Reply.GetAdapterInfo.DhcpEnabled;
    else
        *DhcpEnabled = FALSE;

    if (*DhcpEnabled)
    {
        *DhcpServer = Reply.GetAdapterInfo.DhcpServer;
        *LeaseObtained = Reply.GetAdapterInfo.LeaseObtained;
        *LeaseExpires = Reply.GetAdapterInfo.LeaseExpires;
    }
    else
    {
        *DhcpServer = INADDR_NONE;
        *LeaseObtained = 0;
        *LeaseExpires = 0;
    }

    return Reply.Reply;
}


static VOID
UpdateServiceStatus(DWORD dwState)
{
    ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
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

    hPipeThread = PipeInit(hStopEvent);
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
