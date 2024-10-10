/*
 * PROJECT:     NetIO driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     A more convenient networking (TCP/IP, UDP/IP) kernel API (NT6+)
 * COPYRIGHT:   Copyright 2023-2024 Johannes Thoma <johannes@johannesthoma.com>
 */

/* TODOs:
    Done: Clean up socket in WskCloseSocket (have RefCount and
    SocketGet / SocketPut functions).
    Done: Remove unnecessary code (like Hook of completion)
    Make it compile with MS VC as well
    Rebase onto latest master
    Squash commits (git rebase -i) one for tdihelpers one for netio

    The whole Listen / Accept mechanism is still missing
    Some minor functions (not used by WinDRBD) are missing
    (like WskControlClient, WskSocketConnect, ...)
    Raw sockets are not supported for now.
    We should have regression tests for that...
*/

#include <ntifs.h>
#include <netiodef.h>
#include <ws2def.h>
#include <wsk.h>
#include <ndis.h>
#include <netio_debug.h>

#include <tdi.h>
#include <tcpioctl.h>
#include <tdikrnl.h>
#include <tdiinfo.h>
#include <tdi_proto.h>
#include <tdiconn.h>

ULONG DebugTraceLevel = MIN_TRACE;

#define TAG_NETIO 'OIEN'

#ifndef TAG_AFD_TDI_CONNECTION_INFORMATION
#define TAG_AFD_TDI_CONNECTION_INFORMATION 'cTfA'
#endif

/* AFD Share Flags */
#define AFD_SHARE_UNIQUE    0x0L
#define AFD_SHARE_REUSE     0x1L
#define AFD_SHARE_WILDCARD  0x2L
#define AFD_SHARE_EXCLUSIVE 0x3L

typedef struct _WSK_SOCKET_INTERNAL
{
    WSK_SOCKET s;

    ADDRESS_FAMILY family;      /* AF_INET or AF_INET6 */
    USHORT type;                /* SOCK_DGRAM, SOCK_STREAM, ... */
    ULONG proto;                /* IPPROTO_UDP, IPPROTO_TCP */
    ULONG flags;                /* WSK_FLAG_LISTEN_SOCKET, ... */
    PVOID user_context;         /* parameter for callbacks, opaque */
    UNICODE_STRING TdiName;     /* \\Devices\\Tcp, \\Devices\\Udp */

    struct sockaddr LocalAddress;
    PFILE_OBJECT LocalAddressFile;
    HANDLE LocalAddressHandle;

    struct sockaddr RemoteAddress;      /* Remeber latest remote connection */

    /* Those exist for connection oriented (TCP/IP) sockets only */
    PFILE_OBJECT ConnectionFile;        /* Returned by TdiOpenConnectionEndpointFile() */
    HANDLE ConnectionHandle;            /* Returned by TdiOpenConnectionEndpointFile() */

    /* Incoming connection callback function: */
    const WSK_CLIENT_LISTEN_DISPATCH *ListenDispatch;
    UINT CallbackMask;

    UINT Flags;                          /* SO_REUSEADDR, ... see ws2def.h */
    UINT RefCount;                       /* See SocketGet/SocketPut TODO: this should be atomic */
} WSK_SOCKET_INTERNAL, *PWSK_SOCKET_INTERNAL;

struct NetioContext
{
    PIRP UserIrp;
    PWSK_SOCKET_INTERNAL socket;
    PTDI_CONNECTION_INFORMATION TargetConnectionInfo;
};

void
SocketGet(PWSK_SOCKET_INTERNAL s)
{
    s->RefCount++;
}

void
SocketPut(PWSK_SOCKET_INTERNAL s)
{
    s->RefCount--;
    if (s->RefCount == 0)
    {
        if (s->LocalAddressHandle != NULL)
        {
            ZwClose(s->LocalAddressHandle);
            s->LocalAddressHandle = NULL;
            s->LocalAddressFile = NULL;
        }
        if (s->ConnectionHandle != NULL)
        {
            ZwClose(s->ConnectionHandle);
            s->ConnectionHandle = NULL;
            s->ConnectionFile = NULL;
        }
        ExFreePoolWithTag(s, TAG_NETIO);
    }
}

NTSTATUS NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI
NetioComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
    struct NetioContext *c = (struct NetioContext *)Context;
    PIRP UserIrp = c->UserIrp;

    UserIrp->IoStatus.Status = Irp->IoStatus.Status;
    UserIrp->IoStatus.Information = Irp->IoStatus.Information;

    IoCompleteRequest(UserIrp, IO_NETWORK_INCREMENT);

    SocketPut(c->socket);
    if (c->TargetConnectionInfo != NULL)
    {
        ExFreePoolWithTag(c->TargetConnectionInfo, TAG_AFD_TDI_CONNECTION_INFORMATION);
    }
    ExFreePoolWithTag(c, TAG_NETIO);

    return STATUS_SUCCESS;
}

static NTSTATUS WSKAPI
WskControlSocket(
    _In_ PWSK_SOCKET Socket,
    _In_ WSK_CONTROL_SOCKET_TYPE RequestType,
    _In_ ULONG ControlCode,
    _In_ ULONG Level,
    _In_ SIZE_T InputSize,
    _In_reads_bytes_opt_(InputSize) PVOID InputBuffer,
    _In_ SIZE_T OutputSize,
    _Out_writes_bytes_opt_(OutputSize) PVOID OutputBuffer,
    _Out_opt_ SIZE_T * OutputSizeReturned,
    _Inout_opt_ PIRP Irp)
{
    PWSK_SOCKET_INTERNAL s = (PWSK_SOCKET_INTERNAL)Socket;
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;

    if (s == NULL)
    {
        DbgPrint("WskControlSocket: Socket is NULL\n");
        status = STATUS_INVALID_PARAMETER;
        goto err_out;
    }
    IoSetNextIrpStackLocation(Irp);

    switch (RequestType)
    {
        case WskSetOption:
            switch (Level)
            {
                case SOL_SOCKET:
                    switch (ControlCode)
                    {
                        case SO_REUSEADDR:     /* add more supported flags here */
                            if (InputBuffer == NULL)
                            {
                                DbgPrint("WskControlSocket: Need an InputBuffer for this operation\n");
                                status = STATUS_INVALID_PARAMETER;
                                break;
                            }
                            if (InputSize < 4)
                            {
                                DbgPrint("WskControlSocket: InputBuffer too small for this operation\n");
                                status = STATUS_INVALID_PARAMETER;
                                break;
                            }
                            UINT flag = *(UINT *)InputBuffer;
                            if (flag != 0)
                            {
                                s->Flags |= ControlCode;
                            }
                            else
                            {
                                s->Flags &= ~ControlCode;
                            }
                            status = STATUS_SUCCESS;
                            break;
                            /* Windows specific. This sets the mask for callback functions: */
                        case SO_WSK_EVENT_CALLBACK:
                            if (InputBuffer == NULL)
                            {
                                DbgPrint("WskControlSocket: Need an InputBuffer for this operation\n");
                                status = STATUS_INVALID_PARAMETER;
                                break;
                            }
                            if (InputSize < sizeof(WSK_EVENT_CALLBACK_CONTROL))
                            {
                                DbgPrint("WskControlSocket: InputBuffer too small for this operation\n");
                                status = STATUS_INVALID_PARAMETER;
                                break;
                            }
                            WSK_EVENT_CALLBACK_CONTROL *c = (WSK_EVENT_CALLBACK_CONTROL *) InputBuffer;

                            s->CallbackMask = c->EventMask;
                            /* TODO: and start listening here? */

                            status = STATUS_SUCCESS;
                            break;

                        default:
                            DbgPrint("WskControlSocket: ControlCode %d Not implemented\n", ControlCode);
                    }
                    break;
                default:
                    DbgPrint("WskControlSocket: Level %d Not implemented\n", Level);
            }
            break;

        case WskGetOption:
        case WskIoctl:
        default:
            DbgPrint("WskControlSocket: Option %d Not implemented\n", RequestType);
    }

err_out:
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return STATUS_PENDING;
}

static NTSTATUS WSKAPI
WskCloseSocket(_In_ PWSK_SOCKET Socket, _Inout_ PIRP Irp)
{
    NTSTATUS status = STATUS_SUCCESS;
    PWSK_SOCKET_INTERNAL s = (PWSK_SOCKET_INTERNAL)Socket;

    IoSetNextIrpStackLocation(Irp);

    SocketPut(s);

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return STATUS_PENDING;
}

static PTRANSPORT_ADDRESS
TdiTransportAddressFromSocketAddress(PSOCKADDR SocketAddress)
{
    PTRANSPORT_ADDRESS ta;

    ta = ExAllocatePoolWithTag(NonPagedPool, sizeof(*ta) + sizeof(struct sockaddr), TAG_NETIO);
    if (ta == NULL)
    {
        DbgPrint("TdiTransportAddressFromSocketAddress: Out of memory\n");
        return NULL;
    }

    ta->TAAddressCount = 1;
    ta->Address[0].AddressLength = sizeof(SocketAddress->sa_data);
    ta->Address[0].AddressType = SocketAddress->sa_family;
    memcpy(&ta->Address[0].Address[0], &SocketAddress->sa_data, ta->Address[0].AddressLength);

    return ta;
}

static PTDI_CONNECTION_INFORMATION
TdiConnectionInfoFromSocketAddress(PSOCKADDR SocketAddress)
{
    PTRANSPORT_ADDRESS TargetAddress;
    PTDI_CONNECTION_INFORMATION ConnectionInformation = NULL;
    NTSTATUS status;

    TargetAddress = TdiTransportAddressFromSocketAddress(SocketAddress);
    if (TargetAddress == NULL)
        return NULL;

    status = TdiBuildConnectionInfo(&ConnectionInformation, TargetAddress);
    ExFreePoolWithTag(TargetAddress, TAG_NETIO);

    if (!NT_SUCCESS(status))
    {
        if (ConnectionInformation != NULL)
        {
            ExFreePoolWithTag(ConnectionInformation, TAG_AFD_TDI_CONNECTION_INFORMATION);
        }
        return NULL;
    }
    return ConnectionInformation;
}

static NTSTATUS WSKAPI
WskBind(_In_ PWSK_SOCKET Socket, _In_ PSOCKADDR LocalAddress, _Reserved_ ULONG Flags, _Inout_ PIRP Irp)
{
    NTSTATUS status;
    PWSK_SOCKET_INTERNAL s = (PWSK_SOCKET_INTERNAL)Socket;
    PTRANSPORT_ADDRESS ta = TdiTransportAddressFromSocketAddress(LocalAddress);

    IoSetNextIrpStackLocation(Irp);

    if (ta == NULL)
    {
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto err_out;
    }
    if (s->LocalAddressHandle != NULL)
    {
        ZwClose(s->LocalAddressHandle);
        s->LocalAddressHandle = NULL;
        s->LocalAddressFile = NULL;
    }

    status = TdiOpenAddressFile(&s->TdiName,
                                ta, AFD_SHARE_REUSE, &s->LocalAddressHandle, &s->LocalAddressFile);
    if (NT_SUCCESS(status))
    {
        memcpy(&s->LocalAddress, LocalAddress, sizeof(s->LocalAddress));
    }
    ExFreePoolWithTag(ta, TAG_NETIO);

err_out:
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return STATUS_PENDING;
}

enum direction
{
    DIR_SEND,
    DIR_RECEIVE
};

static NTSTATUS WSKAPI
WskSendTo(
    _In_ PWSK_SOCKET Socket,
    _In_ PWSK_BUF Buffer,
    _Reserved_ ULONG Flags,
    _In_opt_ PSOCKADDR RemoteAddress,
    _In_ ULONG ControlInfoLength,
    _In_reads_bytes_opt_(ControlInfoLength) PCMSGHDR ControlInfo,
    _Inout_ PIRP Irp)
{
    PIRP tdiIrp = NULL;
    PWSK_SOCKET_INTERNAL s = (PWSK_SOCKET_INTERNAL)Socket;
    PTDI_CONNECTION_INFORMATION TargetConnectionInfo;
    NTSTATUS status;
    void *BufferData;
    struct NetioContext *nc;

    IoSetNextIrpStackLocation(Irp);

    status = STATUS_INSUFFICIENT_RESOURCES;
    nc = ExAllocatePoolWithTag(NonPagedPool, sizeof(*nc), TAG_NETIO);
    if (nc == NULL)
    {
        goto err_out;
    }
    nc->socket = s;
    nc->UserIrp = Irp;

    TargetConnectionInfo = TdiConnectionInfoFromSocketAddress(RemoteAddress);
    if (TargetConnectionInfo == NULL)
    {
        goto err_out_free_nc;
    }
    nc->TargetConnectionInfo = TargetConnectionInfo;

    BufferData = MmGetSystemAddressForMdlSafe(Buffer->Mdl, NormalPagePriority);
    if (BufferData == NULL)
    {
        DbgPrint("Error mapping MDL\n");
        goto err_out_free_nc_and_tci;
    }

    IoMarkIrpPending(Irp);
    SocketGet(s);

    /* This will create a tdiIrp: */
    status = TdiSendDatagram(&tdiIrp, s->LocalAddressFile, ((char *)BufferData) + Buffer->Offset,
                        Buffer->Length, TargetConnectionInfo, NetioComplete, nc);

    /* If allocating tdiIrp fails we get here.
     * Call the IoCompletion of the application's Irp so this Irp
     * gets freed.
     */
    if (!NT_SUCCESS(status))
    {
        SocketPut(s);
        goto err_out_free_nc_and_tci_unmap;
    }
    return STATUS_PENDING;

err_out_free_nc_and_tci_unmap:
    /* TODO: implement freeing Buffer mmap */

err_out_free_nc_and_tci:
    ExFreePoolWithTag(TargetConnectionInfo, TAG_AFD_TDI_CONNECTION_INFORMATION);

err_out_free_nc:
    ExFreePoolWithTag(nc, TAG_NETIO);

err_out:
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return STATUS_PENDING;
}

static NTSTATUS WSKAPI
WskReceiveFrom(
    _In_ PWSK_SOCKET Socket,
    _In_ PWSK_BUF Buffer,
    _Reserved_ ULONG Flags,
    _Out_opt_ PSOCKADDR RemoteAddress,
    _Inout_ PULONG ControlLength,
    _Out_writes_bytes_opt_(*ControlLength) PCMSGHDR ControlInfo,
    _Out_opt_ PULONG ControlFlags, _Inout_ PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS WSKAPI
WskReleaseUdp(_In_ PWSK_SOCKET Socket, _In_ PWSK_DATAGRAM_INDICATION DatagramIndication)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS WSKAPI
WskReleaseTcp(_In_ PWSK_SOCKET Socket, _In_ PWSK_DATA_INDICATION DataIndication)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS WSKAPI
WskGetLocalAddress(_In_ PWSK_SOCKET Socket, _Out_ PSOCKADDR LocalAddress, _Inout_ PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS WSKAPI
WskGetRemoteAddress(_In_ PWSK_SOCKET Socket, _Out_ PSOCKADDR RemoteAddress, _Inout_ PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS WSKAPI
WskSocketConnect(
    _In_ PWSK_CLIENT Client,
    _In_ USHORT SocketType,
    _In_ ULONG Protocol,
    _In_ PSOCKADDR LocalAddress,
    _In_ PSOCKADDR RemoteAddress,
    _In_ ULONG Flags,
    _In_opt_ PVOID SocketContext,
    _In_opt_ const WSK_CLIENT_CONNECTION_DISPATCH * Dispatch,
    _In_opt_ PEPROCESS OwningProcess,
    _In_opt_ PETHREAD OwningThread,
    _In_opt_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Inout_ PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS WSKAPI
WskControlClient(
    _In_ PWSK_CLIENT Client,
    _In_ ULONG ControlCode,
    _In_ SIZE_T InputSize,
    _In_reads_bytes_opt_(InputSize) PVOID InputBuffer,
    _In_ SIZE_T OutputSize,
    _Out_writes_bytes_opt_(OutputSize) PVOID OutputBuffer,
    _Out_opt_ SIZE_T * OutputSizeReturned, _Inout_opt_ PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

static WSK_PROVIDER_DATAGRAM_DISPATCH UdpDispatch = {
    .WskControlSocket = WskControlSocket,
    .WskCloseSocket = WskCloseSocket,
    .WskBind = WskBind,
    .WskSendTo = WskSendTo,
    .WskReceiveFrom = WskReceiveFrom,
    .WskRelease = WskReleaseUdp,
    .WskGetLocalAddress = WskGetLocalAddress,
};

/* Connection oriented routines (TCP/IP): */

static NTSTATUS WSKAPI
WskConnect(_In_ PWSK_SOCKET Socket, _In_ PSOCKADDR RemoteAddress, _Reserved_ ULONG Flags, _Inout_ PIRP Irp)
{
    PTDI_CONNECTION_INFORMATION TargetConnectionInfo, Ignored;
    PIRP tdiIrp;
    PWSK_SOCKET_INTERNAL s = (PWSK_SOCKET_INTERNAL)Socket;
    NTSTATUS status;
    struct NetioContext *nc;

    IoSetNextIrpStackLocation(Irp);

    status = STATUS_INVALID_PARAMETER;
    if (s->LocalAddressHandle == NULL)
    {
        DbgPrint("LocalAddressHandle is not set, need to bind your socket before connecting\n");
        goto err_out;
    }

    status = STATUS_INSUFFICIENT_RESOURCES;
    nc = ExAllocatePoolWithTag(NonPagedPool, sizeof(*nc), TAG_NETIO);
    if (nc == NULL)
    {
        goto err_out;
    }
    nc->socket = s;
    nc->UserIrp = Irp;

    tdiIrp = NULL;

    status = TdiAssociateAddressFile(s->LocalAddressHandle, s->ConnectionFile);
    if (!NT_SUCCESS(status))
    {
        goto err_out_free_nc;
    }

    TargetConnectionInfo = TdiConnectionInfoFromSocketAddress(RemoteAddress);
    if (TargetConnectionInfo == NULL)
    {
        goto err_out_free_nc;
    }
    nc->TargetConnectionInfo = TargetConnectionInfo;

    /* TODO: @thomasfabber: is this correct? */
    Ignored = TdiConnectionInfoFromSocketAddress(RemoteAddress);
    if (Ignored == NULL)
    {
        goto err_out_free_nc_and_tci;
    }

    IoMarkIrpPending(Irp);
    SocketGet(s);

    status = TdiConnect(&tdiIrp, s->ConnectionFile, TargetConnectionInfo, Ignored, NetioComplete, nc);

    /* If allocating tdiIrp fails we get here.
     * Call the IoCompletion of the application's Irp so this Irp
     * gets freed.
     */
    if (!NT_SUCCESS(status))
    {
        SocketPut(s);
        goto err_out_free_nc_and_tci;
    }
    return STATUS_PENDING;

err_out_free_nc_and_tci:
    ExFreePoolWithTag(TargetConnectionInfo, TAG_AFD_TDI_CONNECTION_INFORMATION);

err_out_free_nc:
    ExFreePoolWithTag(nc, TAG_NETIO);

err_out:
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return STATUS_PENDING;
}

static NTSTATUS WSKAPI
WskStreamIo(
    _In_ PWSK_SOCKET Socket,
    _In_ PWSK_BUF Buffer,
    _In_ ULONG Flags,
     _Inout_ PIRP Irp,
    _In_ enum direction Direction)
{
    PIRP tdiIrp = NULL;
    PWSK_SOCKET_INTERNAL s = (PWSK_SOCKET_INTERNAL)Socket;
    NTSTATUS status;
    void *BufferData;
    struct NetioContext *nc;

    IoSetNextIrpStackLocation(Irp);
    status = STATUS_INSUFFICIENT_RESOURCES;

    nc = ExAllocatePoolWithTag(NonPagedPool, sizeof(*nc), TAG_NETIO);
    if (nc == NULL)
    {
        goto err_out;
    }
    nc->socket = s;
    nc->UserIrp = Irp;
    nc->TargetConnectionInfo = NULL;

    BufferData = MmGetSystemAddressForMdlSafe(Buffer->Mdl, NormalPagePriority);
    if (BufferData == NULL)
    {
        DbgPrint("Error mapping MDL\n");
        goto err_out_free_nc;
    }

    IoMarkIrpPending(Irp);
    SocketGet(s);

    if (Direction == DIR_SEND)
    {
        /* This will create a tdiIrp: */
        status = TdiSend(&tdiIrp, s->ConnectionFile, 0, ((char *)BufferData) + Buffer->Offset,
                    Buffer->Length, NetioComplete, nc);
    }
    else
    {
        status = TdiReceive(&tdiIrp, s->ConnectionFile, 0, ((char *)BufferData) + Buffer->Offset,
                       Buffer->Length, NetioComplete, nc);
    }

    /* If allocating tdiIrp fails we get here.
     * Call the IoCompletion of the application's Irp so this Irp
     * gets freed.
     */
    if (!NT_SUCCESS(status))
    {
        goto err_out_free_nc_unmap;
    }
    return STATUS_PENDING;

err_out_free_nc_unmap:
    /* TODO: implement freeing Buffer mmap */

err_out_free_nc:
    ExFreePoolWithTag(nc, TAG_NETIO);

err_out:
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return STATUS_PENDING;
}

static NTSTATUS WSKAPI
WskSend(_In_ PWSK_SOCKET Socket, _In_ PWSK_BUF Buffer, _In_ ULONG Flags, _Inout_ PIRP Irp)
{
    return WskStreamIo(Socket, Buffer, Flags, Irp, DIR_SEND);
}

static NTSTATUS WSKAPI
WskReceive(_In_ PWSK_SOCKET Socket, _In_ PWSK_BUF Buffer, _In_ ULONG Flags, _Inout_ PIRP Irp)
{
    return WskStreamIo(Socket, Buffer, Flags, Irp, DIR_RECEIVE);
}

static NTSTATUS WSKAPI
WskDisconnect(_In_ PWSK_SOCKET Socket, _In_opt_ PWSK_BUF Buffer, _In_ ULONG Flags, _Inout_ PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


static WSK_PROVIDER_CONNECTION_DISPATCH TcpDispatch = {
    .WskControlSocket = WskControlSocket,
    .WskCloseSocket = WskCloseSocket,
    .WskBind = WskBind,
    .WskConnect = WskConnect,
    .WskGetLocalAddress = WskGetLocalAddress,
    .WskGetRemoteAddress = WskGetRemoteAddress,
    .WskSend = WskSend,
    .WskReceive = WskReceive,
    .WskDisconnect = WskDisconnect,
    .WskRelease = WskReleaseTcp,
};

static NTSTATUS WSKAPI
WskSocket(
    _In_ PWSK_CLIENT Client,
    _In_ ADDRESS_FAMILY AddressFamily,
    _In_ USHORT SocketType,
    _In_ ULONG Protocol,
    _In_ ULONG Flags,
    _In_opt_ PVOID SocketContext,
    _In_opt_ const VOID * Dispatch,
    _In_opt_ PEPROCESS OwningProcess,
    _In_opt_ PETHREAD OwningThread,
    _In_opt_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Inout_ PIRP Irp)
{
    PWSK_SOCKET_INTERNAL s;
    NTSTATUS status;

    IoSetNextIrpStackLocation(Irp);

    if (AddressFamily != AF_INET)
    {
        DbgPrint("Address family %d not supported\n", AddressFamily);
        status = STATUS_NOT_SUPPORTED;
        goto err_out;
    }
    switch (SocketType)
    {
        case SOCK_DGRAM:
            if (Protocol != IPPROTO_UDP)
            {
                DbgPrint("SOCK_DGRAM only supports IPPROTO_UDP\n");
                status = STATUS_INVALID_PARAMETER;
                goto err_out;
            }
            if (Flags != WSK_FLAG_DATAGRAM_SOCKET)
            {
                DbgPrint("SOCK_DGRAM flags must be WSK_FLAG_DATAGRAM_SOCKET\n");
                status = STATUS_INVALID_PARAMETER;
                goto err_out;
            }
            break;

        case SOCK_STREAM:
            if (Protocol != IPPROTO_TCP)
            {
                DbgPrint("SOCK_STREAM only supports IPPROTO_TCP\n");
                status = STATUS_INVALID_PARAMETER;
                goto err_out;
            }
            if ((Flags != WSK_FLAG_CONNECTION_SOCKET) && (Flags != WSK_FLAG_LISTEN_SOCKET))
            {
                DbgPrint("SOCK_STREAM flags must be either WSK_FLAG_CONNECTION_SOCKET or WSK_FLAG_LISTEN_SOCKET\n");
                status = STATUS_INVALID_PARAMETER;
                goto err_out;
            }
            break;

        case SOCK_RAW:
            DbgPrint("SOCK_RAW not supported\n");
            status = STATUS_NOT_SUPPORTED;
            goto err_out;

        default:
            status = STATUS_INVALID_PARAMETER;
            goto err_out;
    }

    s = ExAllocatePoolWithTag(NonPagedPool, sizeof(*s), TAG_NETIO);
    if (s == NULL)
    {
        DbgPrint("WskSocket: Out of memory\n");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto err_out;
    }
    s->family = AddressFamily;
    s->type = SocketType;
    s->proto = Protocol;
    s->flags = Flags;
    s->user_context = SocketContext;
    s->LocalAddressHandle = NULL;
    s->LocalAddressFile = NULL;
    s->Flags = 0;
    s->ListenDispatch = Dispatch;
    s->RefCount = 1;            /* SocketPut() is in WskCloseSocket */
    s->ConnectionHandle = NULL;

    switch (SocketType)
    {
        case SOCK_DGRAM:
            s->s.Dispatch = &UdpDispatch;
            RtlInitUnicodeString(&s->TdiName, L"\\Device\\Udp");
            break;
        case SOCK_STREAM:
            s->s.Dispatch = &TcpDispatch;
            RtlInitUnicodeString(&s->TdiName, L"\\Device\\Tcp");

            status = TdiOpenConnectionEndpointFile(&s->TdiName, &s->ConnectionHandle, &s->ConnectionFile);
            if (status != STATUS_SUCCESS)
            {
                DbgPrint("Could not open TDI handle, status is %x\n", status);
                ExFreePoolWithTag(s, TAG_NETIO);
                goto err_out;
            }
            if (Flags == WSK_FLAG_LISTEN_SOCKET && s->ListenDispatch == NULL)
            {
                DbgPrint("Warning: no callbacks given for listen socket\n");
            }
            break;

        default:
            DbgPrint("Socket type not yet supported\n");
            /* A little bit later this probably crashes */
    }

    Irp->IoStatus.Information = (ULONG_PTR) s;
    status = STATUS_SUCCESS;

err_out:
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return STATUS_PENDING;
}

static WSK_PROVIDER_DISPATCH provider_dispatch = {
    .Version = 0,
    .Reserved = 0,
    .WskSocket = WskSocket,
    .WskSocketConnect = WskSocketConnect,
    .WskControlClient = WskControlClient
};

NTSTATUS WSKAPI
WskRegister(_In_ PWSK_CLIENT_NPI client_npi, _Out_ PWSK_REGISTRATION reg)
{
    reg->ReservedRegistrationState = 42;
    reg->ReservedRegistrationContext = NULL;
    KeInitializeSpinLock(&reg->ReservedRegistrationLock);

    return STATUS_SUCCESS;
}

NTSTATUS WSKAPI
WskCaptureProviderNPI(_In_ PWSK_REGISTRATION reg, _In_ ULONG wait, _Out_ PWSK_PROVIDER_NPI npi)
{
    DbgPrint("WskCaptureProviderNPI\n");
    npi->Client = NULL;
    npi->Dispatch = &provider_dispatch;

    return STATUS_SUCCESS;
}

VOID WSKAPI
WskReleaseProviderNPI(_In_ PWSK_REGISTRATION reg)
{
    DbgPrint("WskReleaseProviderNPI\n");
    /* noop */
}

VOID WSKAPI
WskDeregister(_In_ PWSK_REGISTRATION reg)
{
    DbgPrint("WskDeregister\n");
    /* noop */
}
