/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock Helper DLL for TCP/IP
 * FILE:        wshtcpip.c
 * PURPOSE:     DLL entry
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */
#include <wshtcpip.h>

#ifdef DBG

/* See debug.h for debug/trace constants */
DWORD DebugTraceLevel = MAX_TRACE;

#endif /* DBG */

/* To make the linker happy */
VOID STDCALL KeBugCheck (ULONG	BugCheckCode) {}


BOOL
EXPORT
DllMain(HANDLE hInstDll,
        ULONG dwReason,
        PVOID Reserved)
{
    WSH_DbgPrint(MIN_TRACE, ("DllMain of wshtcpip.dll\n"));

    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        /* Don't need thread attach notifications
           so disable them to improve performance */
        DisableThreadLibraryCalls(hInstDll);
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


INT
EXPORT
WSHAddressToString(
    IN      LPSOCKADDR Address,
    IN      INT AddressLength,
    IN      LPWSAPROTOCOL_INFOW ProtocolInfo    OPTIONAL,
    OUT     LPWSTR AddressString,
    IN OUT  LPDWORD AddressStringLength)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSHEnumProtocols(
    IN      LPINT lpiProtocols  OPTIONAL,
    IN      LPWSTR lpTransportKeyName,
    IN OUT  LPVOID lpProtocolBuffer,
    IN OUT  LPDWORD lpdwBufferLength)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSHGetBroadcastSockaddr(
    IN  PVOID HelperDllSocketContext,
    OUT PSOCKADDR Sockaddr,
    OUT PINT SockaddrLength)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSHGetProviderGuid(
    IN  LPWSTR ProviderName,
    OUT LPGUID ProviderGuid)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSHGetSockaddrType(
    IN  PSOCKADDR Sockaddr,
    IN  DWORD SockaddrLength,
    OUT PSOCKADDR_INFO SockaddrInfo)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSHGetSocketInformation(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle,
    IN  HANDLE TdiAddressObjectHandle,
    IN  HANDLE TdiConnectionObjectHandle,
    IN  INT Level,
    IN  INT OptionName,
    OUT PCHAR OptionValue,
    OUT INT OptionLength)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSHGetWildcardSockaddr(
    IN  PVOID HelperDllSocketContext,
    OUT PSOCKADDR Sockaddr,
    OUT PINT SockaddrLength)
{
    UNIMPLEMENTED

    return 0;
}


DWORD
EXPORT
WSHGetWinsockMapping(
    OUT PWINSOCK_MAPPING Mapping,
    IN  DWORD MappingLength)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSHGetWSAProtocolInfo(
    IN  LPWSTR ProviderName,
    OUT LPWSAPROTOCOL_INFOW* ProtocolInfo,
    OUT LPDWORD ProtocolInfoEntries)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSHIoctl(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle,
    IN  HANDLE TdiAddressObjectHandle,
    IN  HANDLE TdiConnectionObjectHandle,
    IN  DWORD IoControlCode,
    IN  LPVOID InputBuffer,
    IN  DWORD InputBufferLength,
    IN  LPVOID OutputBuffer,
    IN  DWORD OutputBufferLength,
    OUT LPDWORD NumberOfBytesReturned,
    IN  LPWSAOVERLAPPED Overlapped,
    IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine,
    OUT LPBOOL NeedsCompletion)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSHJoinLeaf(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle,
    IN  HANDLE TdiAddressObjectHandle,
    IN  HANDLE TdiConnectionObjectHandle,
    IN  PVOID LeafHelperDllSocketContext,
    IN  SOCKET LeafSocketHandle,
    IN  PSOCKADDR Sockaddr,
    IN  DWORD SockaddrLength,
    IN  LPWSABUF CallerData,
    IN  LPWSABUF CalleeData,
    IN  LPQOS SocketQOS,
    IN  LPQOS GroupQOS,
    IN  DWORD Flags)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSHNotify(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle,
    IN  HANDLE TdiAddressObjectHandle,
    IN  HANDLE TdiConnectionObjectHandle,
    IN  DWORD NotifyEvent)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSHOpenSocket(
    IN OUT  PINT AddressFamily,
    IN OUT  PINT SocketType,
    IN OUT  PINT Protocol,
    OUT     PUNICODE_STRING TransportDeviceName,
    OUT     PVOID HelperDllSocketContext,
    OUT     PDWORD NotificationEvents)
/*
 * FUNCTION: Opens a socket
 */
{
    return WSHOpenSocket2(AddressFamily,
                          SocketType,
                          Protocol,
                          0,
                          0,
                          TransportDeviceName,
                          HelperDllSocketContext,
                          NotificationEvents);
}


INT
EXPORT
WSHOpenSocket2(
    OUT PINT AddressFamily,
    IN  OUT PINT SocketType,
    IN  OUT PINT Protocol,
    IN  GROUP Group,
    IN  DWORD Flags,
    OUT PUNICODE_STRING TransportDeviceName,
    OUT PVOID *HelperDllSocketContext,
    OUT PDWORD NotificationEvents)
/*
 * FUNCTION: Opens a socket
 * ARGUMENTS:
 *     AddressFamily          = Address of buffer with address family (updated)
 *     SocketType             = Address of buffer with type of socket (updated)
 *     Protocol               = Address of buffer with protocol number (updated)
 *     Group                  = Socket group
 *     Flags                  = Socket flags
 *     TransportDeviceName    = Address of buffer to place name of transport device
 *     HelperDllSocketContext = Address of buffer to place socket context pointer
 *     NotificationEvents     = Address of buffer to place flags for event notification
 * RETURNS:
 *     Status of operation
 * NOTES:
 *     Mapping tripple is returned in an canonicalized form
 */
{
    PSOCKET_CONTEXT Context;
    UNICODE_STRING String;
    NTSTATUS Status;

    WSH_DbgPrint(MAX_TRACE, ("\n"));

    switch (*SocketType) {
    case SOCK_STREAM:
        RtlInitUnicodeString(&String, DD_TCP_DEVICE_NAME);
        break;

    case SOCK_DGRAM:
        RtlInitUnicodeString(&String, DD_UDP_DEVICE_NAME);
        break;

    case SOCK_RAW:
        if ((*Protocol < 0) || (*Protocol > 255))
          return WSAEINVAL;

        RtlInitUnicodeString(&String, DD_RAW_IP_DEVICE_NAME);
        break;

    default:
        return WSAEINVAL;
    }

    RtlInitUnicodeString(TransportDeviceName, NULL);

    TransportDeviceName->MaximumLength = String.Length +        /* Transport device name */
                                         (4 * sizeof(WCHAR) +   /* Separator and protocol */
                                         sizeof(UNICODE_NULL)); /* Terminating null */

    TransportDeviceName->Buffer = HeapAlloc(
        GetProcessHeap(),
        0,
        TransportDeviceName->MaximumLength);

    if (!TransportDeviceName->Buffer)
        return WSAENOBUFS;

    /* Append the transport device name */
    Status = RtlAppendUnicodeStringToString(TransportDeviceName, &String);

    if (*SocketType == SOCK_RAW) {
        /* Append a separator */
        TransportDeviceName->Buffer[TransportDeviceName->Length / sizeof(WCHAR)] = OBJ_NAME_PATH_SEPARATOR;
        TransportDeviceName->Length += sizeof(WCHAR);
        TransportDeviceName->Buffer[TransportDeviceName->Length / sizeof(WCHAR)] = UNICODE_NULL;

        /* Append the protocol number */
        String.Buffer = TransportDeviceName->Buffer + (TransportDeviceName->Length / sizeof(WCHAR));
        String.Length = 0;
        String.MaximumLength = TransportDeviceName->MaximumLength - TransportDeviceName->Length;

        Status = RtlIntegerToUnicodeString((ULONG)*Protocol, 10, &String);

        TransportDeviceName->Length += String.Length;
    }

    /* Setup a socket context area */

    Context = HeapAlloc(GetProcessHeap(), 0, sizeof(SOCKET_CONTEXT));
    if (!Context) {
        RtlFreeUnicodeString(TransportDeviceName);
        return WSAENOBUFS;
    }

    Context->AddressFamily = *AddressFamily;
    Context->SocketType    = *SocketType;
    Context->Protocol      = *Protocol;
    Context->Flags         = Flags;

    *HelperDllSocketContext = Context;

    return NO_ERROR;
}


INT
EXPORT
WSHSetSocketInformation(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle,
    IN  HANDLE TdiAddressObjectHandle,
    IN  HANDLE TdiConnectionObjectHandle,
    IN  INT Level,
    IN  INT OptionName,
    IN  PCHAR OptionValue,
    IN  INT OptionLength)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSHStringToAddress(
    IN      LPWSTR AddressString,
    IN      DWORD AddressFamily,
    IN      LPWSAPROTOCOL_INFOW ProtocolInfo    OPTIONAL,
    OUT     LPSOCKADDR Address,
    IN OUT  LPDWORD AddressStringLength)
{
    UNIMPLEMENTED

    return 0;
}

/* EOF */
