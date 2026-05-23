/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/dhcpcapi/dhcpcapi.c
 * PURPOSE:         Client API for DHCP
 * COPYRIGHT:       Copyright 2005 Art Yerkes <ayerkes@speakeasy.net>
 */

#include <rosdhcp.h>
#include <winsvc.h>
#include <dhcpcapi.h>

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

static
DWORD
SetBinaryClassId(
    _In_ PWSTR pszAdapterName)
{
    WCHAR szKeyName[256];
    PWSTR pszUnicodeBuffer = NULL;
    PSZ pszAnsiBuffer = NULL;
    DWORD dwUnicodeSize = 0, dwAnsiSize = 0;
    HKEY hKey = NULL;
    DWORD dwError = ERROR_SUCCESS;

    DPRINT("SetBinaryClassId(%S)\n", pszAdapterName);

    _swprintf(szKeyName,
              L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\%s",
              pszAdapterName);
    DPRINT("KeyName %S\n", szKeyName);

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            szKeyName,
                            0,
                            KEY_QUERY_VALUE | KEY_SET_VALUE,
                            &hKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Error %lu\n", dwError);
        return dwError;
    }

    RegQueryValueExW(hKey,
                     L"DhcpClassId",
                     NULL,
                     NULL,
                     NULL,
                     &dwUnicodeSize);
    DPRINT("UnicodeSize %lu\n", dwUnicodeSize);

    pszUnicodeBuffer = HeapAlloc(GetProcessHeap(), 0, dwUnicodeSize);
    if (pszUnicodeBuffer == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        DPRINT1("Error %lu\n", dwError);
        goto done;
    }

    dwError = RegQueryValueExW(hKey,
                               L"DhcpClassId",
                               NULL,
                               NULL,
                               (PBYTE)pszUnicodeBuffer,
                               &dwUnicodeSize);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Error %lu\n", dwError);
        goto done;
    }

    dwAnsiSize = WideCharToMultiByte(CP_ACP,
                                     0,
                                     pszUnicodeBuffer,
                                     -1,
                                     NULL,
                                     0,
                                     NULL,
                                     NULL);
    DPRINT("AnsiSize %lu\n", dwAnsiSize);

    pszAnsiBuffer = HeapAlloc(GetProcessHeap(), 0, dwAnsiSize);
    if (pszAnsiBuffer == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        DPRINT1("Error %lu\n", dwError);
        goto done;
    }

    WideCharToMultiByte(CP_ACP,
                        0,
                        pszUnicodeBuffer,
                        -1,
                        pszAnsiBuffer,
                        dwAnsiSize,
                        NULL,
                        NULL);
    DPRINT("AnsiBuffer %s\n", pszAnsiBuffer);

    dwError = RegSetValueExW(hKey,
                             L"DhcpClassIdBin",
                             0,
                             REG_BINARY,
                             (PBYTE)pszAnsiBuffer,
                             strlen(pszAnsiBuffer));
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Error %lu\n", dwError);
    }

done:
    if (hKey)
        RegCloseKey(hKey);

    if (pszAnsiBuffer)
        HeapFree(GetProcessHeap(), 0, pszAnsiBuffer);

    if (pszUnicodeBuffer)
        HeapFree(GetProcessHeap(), 0, pszUnicodeBuffer);

    return dwError;
}

/*!
 * Initializes the DHCP interface
 *
 * \param[out] Version
 *        Returns the DHCP Interface Version
 *
 * \return ERROR_SUCCESS on success
 *
 * \remarks DhcpCApiInitialized must be called before any other DHCP Function.
 */
DWORD
APIENTRY
DhcpCApiInitialize(
    _Out_ LPDWORD Version)
{
    *Version = 2;
    return ERROR_SUCCESS;
}

/*!
 * Cleans up the DHCP interface
 *
 * \remarks Other DHCP Functions must not be called after DhcpCApiCleanup.
 */
VOID
APIENTRY
DhcpCApiCleanup(VOID)
{
}

/*!
 * Renews a DHCP Lease
 *
 * \param[in] AdapterName
 *        Name (GUID) of the Adapter
 *
 * \return ERROR_SUCCESS on success
 *
 * \remarks Undocumented by Microsoft
 */
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

/*!
 * Renews a DHCP Lease
 *
 * \param[in] AdapterName
 *        Name (GUID) of the Adapter
 *
 * \return ERROR_SUCCESS on success
 *
 * \remarks Undocumented by Microsoft
 */
DWORD
APIENTRY
DhcpAcquireParametersByBroadcast(
    _In_ PWSTR AdapterName)
{
    DWORD ret;

    DPRINT("DhcpAcquireParametersByBroadcast(%S)\n", AdapterName);

    RpcTryExcept
    {
        ret = Client_AcquireParametersByBroadcast(NULL, AdapterName);
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
DhcpDeRegisterParamChange(
    _In_ DWORD Flags,
    _In_ LPVOID Reserved,
    _In_ LPVOID Event)
{
    UNIMPLEMENTED;
    return 0;
}

/*!
 * Enumerates the DHCP user classes for the given adapter
 *
 * \param[in] Unknown1
 *        Unknown
 *
 * \param[in] AdapterName
 *        Name (GUID) of the Adapter
 *
 * \param[in] Unknown3
 *        Unknown
 *
 * \param[in] Unknown4
 *        Unknown
 *
 * \return ERROR_SUCCESS on success
 *
 * \remarks Undocumented by Microsoft
 */
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

/*!
 * Notify the DHCP client to refresh its fallback configuration
 *
 * \param[in] AdapterName
 *        Name (GUID) of the Adapter
 *
 * \return ERROR_SUCCESS on success
 *
 * \remarks Undocumented by Microsoft
 */
DWORD
APIENTRY
DhcpFallbackRefreshParams(
    _In_ PWSTR AdapterName)
{
    DWORD ret;

    DPRINT("DhcpFallbackRefreshParams(%S)\n", AdapterName);

    RpcTryExcept
    {
        ret = Client_FallbackRefreshParams(NULL, AdapterName);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}

/*!
 * Notify the DHCP client of PNP events
 *
 * \param[in] Unknown1
 *        Unknown
 *
 * \param[in] Unknown2
 *        Unknown
 *
 * \param[in] AdapterName
 *        Name (GUID) of the Adapter
 *
 * \param[in] PnpEvent
 *        Unknown
 *
 * \param[in] Unknown5
 *        Unknown
 *
 * \return ERROR_SUCCESS on success
 *
 * \remarks Undocumented by Microsoft
 */
DWORD
APIENTRY
DhcpHandlePnPEvent(
    _In_ DWORD Unknown1,
    _In_ DWORD Unknown2,
    _In_ PWSTR AdapterName,
    _In_ PDHCP_PNP_EVENT PnpEvent,
    _In_ DWORD Unknown5)
{
    DWORD dwError = ERROR_SUCCESS;

    DPRINT1("DhcpHandlePnPEvent(%lx %lx %S %p %lx)\n",
            Unknown1, Unknown2, AdapterName, PnpEvent, Unknown5);

    if ((Unknown1 != 0) || (Unknown2 != 1) || (PnpEvent == NULL) || (Unknown5 != 0))
        return ERROR_INVALID_PARAMETER;

    if (PnpEvent->Unknown5)
    {
        dwError = SetBinaryClassId(AdapterName);
    }

    return dwError;
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

 * \param[in] IpIndex
 *        ...
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
 * \return ERROR_SUCCESS on success
 *
 * \remarks Undocumented by Microsoft
 */
DWORD
APIENTRY
DhcpNotifyConfigChange(
    _In_ LPWSTR ServerName,
    _In_ LPWSTR AdapterName,
    _In_ BOOL NewIpAddress,
    _In_ DWORD IpIndex,
    _In_ DWORD IpAddress,
    _In_ DWORD SubnetMask,
    _In_ INT DhcpAction)
{
    DPRINT("DhcpNotifyConfigChange(%S %S %lu %lu %lu %lu %d)\n",
           ServerName, AdapterName, NewIpAddress, IpIndex, IpAddress, SubnetMask, DhcpAction);
    return DhcpNotifyConfigChangeEx(ServerName, AdapterName, NewIpAddress,
                                    IpIndex, IpAddress, SubnetMask, DhcpAction, 0);
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

 * \param[in] IpIndex
 *        ...
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
 * \param[in] Unknown8
 *        Unknown
 *
 * \return ERROR_SUCCESS on success
 *
 * \remarks Undocumented by Microsoft
 */
DWORD
APIENTRY
DhcpNotifyConfigChangeEx(
    _In_ LPWSTR ServerName,
    _In_ LPWSTR AdapterName,
    _In_ BOOL NewIpAddress,
    _In_ DWORD IpIndex,
    _In_ DWORD IpAddress,
    _In_ DWORD SubnetMask,
    _In_ INT DhcpAction,
    _In_ DWORD Unknown8)
{
    DWORD ret = ERROR_SUCCESS;

    DPRINT1("DHCPCSVC: DhcpNotifyConfigChangeEx not implemented yet\n");
    DPRINT1("DhcpNotifyConfigChangeEx(%S %S %lu %lu %lu %lu %d %lu)\n",
            ServerName, AdapterName, NewIpAddress, IpIndex, IpAddress, SubnetMask, DhcpAction, Unknown8);

    if (AdapterName == NULL)
        return ERROR_INVALID_PARAMETER;

    if (DhcpAction == 1) // Enable DHCP
    {
        if ((NewIpAddress != FALSE) ||
            (IpIndex != 0) ||
            (IpAddress != 0) ||
            (SubnetMask != 0))
            return ERROR_INVALID_PARAMETER;
    }
    else if (DhcpAction == 2) // Disable DHCP
    {
        if ((NewIpAddress == FALSE) ||
            (IpIndex != 0) ||
            (IpAddress == 0) ||
            (SubnetMask == 0))
            return ERROR_INVALID_PARAMETER;
    }
    else if (DhcpAction == 0) // Do not modify
    {

    }
    else
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (DhcpAction == 1) // Enable DHCP
    {
        /* TODO: Remove static IP address(es) */

        RpcTryExcept
        {
            ret = Server_EnableDhcp(ServerName, AdapterName, TRUE);
        }
        RpcExcept(EXCEPTION_EXECUTE_HANDLER)
        {
            ret = I_RpcMapWin32Status(RpcExceptionCode());
        }
        RpcEndExcept;
    }
    else if (DhcpAction == 2) // Disable DHCP
    {
        RpcTryExcept
        {
            ret = Server_EnableDhcp(ServerName, AdapterName, FALSE);
        }
        RpcExcept(EXCEPTION_EXECUTE_HANDLER)
        {
            ret = I_RpcMapWin32Status(RpcExceptionCode());
        }
        RpcEndExcept;
    }
    else if (DhcpAction == 0) // Do not modify
    {

    }

    return ret;
}

DWORD
APIENTRY
DhcpRegisterParamChange(
    _In_ DWORD Flags,
    _In_ LPVOID Reserved,
    _In_ LPWSTR AdapterName,
    _In_ LPDHCPCAPI_CLASSID ClassId,
    _In_ DHCPCAPI_PARAMS_ARRAY Params,
    _Inout_ LPVOID Handle)
{
    DPRINT1("DhcpRegisterParamChange(%lx %p %S)\n", Flags, Reserved, AdapterName);

    if (Flags != DHCPCAPI_REGISTER_HANDLE_EVENT)
        return ERROR_INVALID_PARAMETER;

    if ((Reserved != NULL) || (AdapterName == NULL) || (Handle == NULL))
        return ERROR_INVALID_PARAMETER;

    UNIMPLEMENTED;
    return 0;
}

/*!
 * Releases a DHCP Lease
 *
 * \param[in] AdapterName
 *        Name (GUID) of the Adapter
 *
 * \return ERROR_SUCCESS on success
 *
 * \remarks Undocumented by Microsoft
 */
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

/*!
 * Removes all DNS Registrations which were added by the DHCP Client
 *
 * \return ERROR_SUCCESS on success
 */
DWORD
APIENTRY
DhcpRemoveDNSRegistrations(VOID)
{
    DWORD ret;

    DPRINT("DhcpRemoveDNSRegistrations()\n");

    RpcTryExcept
    {
        ret = Client_RemoveDNSRegistrations(NULL);
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
DhcpRequestParams(
    _In_ DWORD Flags,
    _In_ PVOID Reserved,
    _In_ LPWSTR AdapterName,
    _In_ LPDHCPCAPI_CLASSID ClassId,
    _In_ DHCPCAPI_PARAMS_ARRAY SendParams,
    _Inout_ DHCPCAPI_PARAMS_ARRAY RecdParams,
    _In_ LPBYTE Buffer,
    _Inout_ LPDWORD pSize,
    _In_ LPWSTR RequestIdStr)
{
    DWORD ret = ERROR_SUCCESS;

    DPRINT1("DhcpRequestParams(%lx %p %S %p %p %p %p %p %S)\n",
            Flags, Reserved, AdapterName, ClassId, SendParams,
            RecdParams, Buffer, pSize, RequestIdStr);

    if ((Reserved != NULL) || (AdapterName == NULL))
        return ERROR_INVALID_PARAMETER;

    if (ClassId != NULL)
    {
        if (ClassId->Flags != 0)
            return ERROR_INVALID_PARAMETER;

        if ((ClassId->Data == NULL) || (ClassId->nBytesData == 0))
            return ERROR_INVALID_PARAMETER;
    }

    RpcTryExcept
    {
        ret = Client_RequestParams(NULL,
                                   AdapterName,
                                   ClassId,
                                   0,
                                   0,
                                   0);
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

DWORD
APIENTRY
DhcpUndoRequestParams(
    _In_ DWORD  Flags,
    _In_ LPVOID Reserved,
    _In_ LPWSTR AdapterName,
    _In_ LPWSTR RequestIdStr)
{
    DPRINT1("DhcpUndoRequestParams(%lx %p %S %S)\n",
            Flags, Reserved, AdapterName, RequestIdStr);

    if ((Flags != 0) || (Reserved != NULL) || (RequestIdStr == NULL))
        return ERROR_INVALID_PARAMETER;

    UNIMPLEMENTED;
    return STATUS_SUCCESS;
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
