/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS User I/O driver
 * FILE:        ndisuio.h
 * PURPOSE:     NDISUIO definitions
 */

#ifndef __NDISUIO_H
#define __NDISUIO_H

#include <ndis.h>
#include <nuiouser.h>

extern PDEVICE_OBJECT GlobalDeviceObject;
extern NDIS_HANDLE GlobalProtocolHandle;
extern LIST_ENTRY GlobalAdapterList;
extern KSPIN_LOCK GlobalAdapterListLock;

typedef struct _NDISUIO_ADAPTER_CONTEXT
{
    /* Asynchronous completion */
    NDIS_STATUS AsyncStatus;
    KEVENT AsyncEvent;

    /* NDIS binding information */
    NDIS_HANDLE BindingHandle;

    /* Reference count information */
    ULONG OpenCount;
    LIST_ENTRY OpenEntryList;

    /* NDIS pools */
    NDIS_HANDLE PacketPoolHandle;
    NDIS_HANDLE BufferPoolHandle;

    /* Receive packet list */
    LIST_ENTRY PacketList;
    KEVENT PacketReadEvent;

    /* Mac options */
    ULONG MacOptions;

    /* Device name */
    UNICODE_STRING DeviceName;

    /* Global list entry */
    LIST_ENTRY ListEntry;

    /* Spin lock */
    KSPIN_LOCK Spinlock;
} NDISUIO_ADAPTER_CONTEXT, *PNDISUIO_ADAPTER_CONTEXT;

typedef struct _NDISUIO_OPEN_ENTRY
{
    /* File object */
    PFILE_OBJECT FileObject;

    /* Tracks how this adapter was opened (write-only or read-write) */
    BOOLEAN WriteOnly;

    /* List entry */
    LIST_ENTRY ListEntry;
} NDISUIO_OPEN_ENTRY, *PNDISUIO_OPEN_ENTRY;

typedef struct _NDISUIO_PACKET_ENTRY
{
    /* Length of data at the end of the struct */
    ULONG PacketLength;

    /* Entry on the packet list */
    LIST_ENTRY ListEntry;

    /* Packet data */
    UCHAR PacketData[1];
} NDISUIO_PACKET_ENTRY, *PNDISUIO_PACKET_ENTRY;

/* NDIS version info */
#define NDIS_MAJOR_VERSION 5
#define NDIS_MINOR_VERSION 0

/* createclose.c */
NTSTATUS
NTAPI
NduDispatchCreate(PDEVICE_OBJECT DeviceObject,
                  PIRP Irp);

NTSTATUS
NTAPI
NduDispatchClose(PDEVICE_OBJECT DeviceObject,
                 PIRP Irp);

/* ioctl.c */
NTSTATUS
NTAPI
NduDispatchDeviceControl(PDEVICE_OBJECT DeviceObject,
                         PIRP Irp);

/* misc.c */
NDIS_STATUS
AllocateAndChainBuffer(PNDISUIO_ADAPTER_CONTEXT AdapterContext,
                       PNDIS_PACKET Packet,
                       PVOID Buffer,
                       ULONG BufferSize,
                       BOOLEAN Front);

PNDIS_PACKET
CreatePacketFromPoolBuffer(PNDISUIO_ADAPTER_CONTEXT AdapterContext,
                           PVOID Buffer,
                           ULONG BufferSize);

VOID
CleanupAndFreePacket(PNDIS_PACKET Packet,
                     BOOLEAN FreePool);

PNDISUIO_ADAPTER_CONTEXT
FindAdapterContextByName(PNDIS_STRING DeviceName);

VOID
ReferenceAdapterContext(PNDISUIO_ADAPTER_CONTEXT AdapterContext);

VOID
DereferenceAdapterContextWithOpenEntry(PNDISUIO_ADAPTER_CONTEXT AdapterContext,
                                       PNDISUIO_OPEN_ENTRY OpenEntry);

/* protocol.c */
VOID
NTAPI
NduOpenAdapterComplete(NDIS_HANDLE ProtocolBindingContext,
                       NDIS_STATUS Status,
                       NDIS_STATUS OpenStatus);

VOID
NTAPI
NduCloseAdapterComplete(NDIS_HANDLE ProtocolBindingContext,
                        NDIS_STATUS Status);

NDIS_STATUS
NTAPI
NduNetPnPEvent(NDIS_HANDLE ProtocolBindingContext,
               PNET_PNP_EVENT NetPnPEvent);

VOID
NTAPI
NduSendComplete(NDIS_HANDLE ProtocolBindingContext,
                PNDIS_PACKET Packet,
                NDIS_STATUS Status);

VOID
NTAPI
NduTransferDataComplete(NDIS_HANDLE ProtocolBindingContext,
                        PNDIS_PACKET Packet,
                        NDIS_STATUS Status,
                        UINT BytesTransferred);

VOID
NTAPI
NduResetComplete(NDIS_HANDLE ProtocolBindingContext,
                 NDIS_STATUS Status);

VOID
NTAPI
NduRequestComplete(NDIS_HANDLE ProtocolBindingContext,
                   PNDIS_REQUEST NdisRequest,
                   NDIS_STATUS Status);

NDIS_STATUS
NTAPI
NduReceive(NDIS_HANDLE ProtocolBindingContext,
           NDIS_HANDLE MacReceiveContext,
           PVOID HeaderBuffer,
           UINT HeaderBufferSize,
           PVOID LookAheadBuffer,
           UINT LookaheadBufferSize,
           UINT PacketSize);

VOID
NTAPI
NduReceiveComplete(NDIS_HANDLE ProtocolBindingContext);

VOID
NTAPI
NduStatus(NDIS_HANDLE ProtocolBindingContext,
          NDIS_STATUS GeneralStatus,
          PVOID StatusBuffer,
          UINT StatusBufferSize);

VOID
NTAPI
NduStatusComplete(NDIS_HANDLE ProtocolBindingContext);

VOID
NTAPI
NduBindAdapter(PNDIS_STATUS Status,
               NDIS_HANDLE BindContext,
               PNDIS_STRING DeviceName,
               PVOID SystemSpecific1,
               PVOID SystemSpecific2);

VOID
NTAPI
NduUnbindAdapter(PNDIS_STATUS Status,
                 NDIS_HANDLE ProtocolBindingContext,
                 NDIS_HANDLE UnbindContext);

/* readwrite.c */
NTSTATUS
NTAPI
NduDispatchRead(PDEVICE_OBJECT DeviceObject,
                PIRP Irp);

NTSTATUS
NTAPI
NduDispatchWrite(PDEVICE_OBJECT DeviceObject,
                 PIRP Irp);

#endif /* __NDISUIO_H */
