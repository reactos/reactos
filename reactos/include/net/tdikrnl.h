/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TDI library
 * FILE:        include/net/tdikrnl.h
 * PURPOSE:     TDI definitions for kernel mode drivers
 */
#ifndef __TDIKRNL_H
#define __TDIKRNL_H

#include "tdi.h"

#ifndef STDCALL
#define STDCALL
#endif

typedef struct _TDI_REQUEST_KERNEL
{
    ULONG   RequestFlags;
    PTDI_CONNECTION_INFORMATION RequestConnectionInformation;
    PTDI_CONNECTION_INFORMATION ReturnConnectionInformation;
    PVOID   RequestSpecific;
} TDI_REQUEST_KERNEL, *PTDI_REQUEST_KERNEL;

/* Request codes */
#define TDI_ASSOCIATE_ADDRESS       0x01
#define TDI_DISASSOCIATE_ADDRESS    0x02
#define TDI_CONNECT                 0x03
#define TDI_LISTEN                  0x04
#define TDI_ACCEPT                  0x05
#define TDI_DISCONNECT              0x06
#define TDI_SEND                    0x07
#define TDI_RECEIVE                 0x08
#define TDI_SEND_DATAGRAM           0x09
#define TDI_RECEIVE_DATAGRAM        0x0A
#define TDI_SET_EVENT_HANDLER       0x0B
#define TDI_QUERY_INFORMATION       0x0C
#define TDI_SET_INFORMATION         0x0D
#define TDI_ACTION                  0x0E

#define TDI_DIRECT_SEND             0x27
#define TDI_DIRECT_SEND_DATAGRAM    0x29


#define TDI_TRANSPORT_ADDRESS_FILE  1
#define TDI_CONNECTION_FILE         2
#define TDI_CONTROL_CHANNEL_FILE    3

/* Internal TDI IOCTLS */
#define IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER     TDI_CONTROL_CODE(0x80, METHOD_NEITHER)
#define IOCTL_TDI_QUERY_DIRECT_SENDDG_HANDLER   TDI_CONTROL_CODE(0x81, METHOD_NEITHER)

/* TdiAssociateAddress */
typedef struct _TDI_REQUEST_KERNEL_ASSOCIATE
{
    HANDLE AddressHandle;
} TDI_REQUEST_KERNEL_ASSOCIATE, *PTDI_REQUEST_KERNEL_ASSOCIATE;

/* TdiDisassociateAddress */
typedef TDI_REQUEST_KERNEL TDI_REQUEST_KERNEL_DISASSOCIATE,
    *PTDI_REQUEST_KERNEL_DISASSOCIATE;

/* TdiAccept */
typedef struct _TDI_REQUEST_KERNEL_ACCEPT
{
    PTDI_CONNECTION_INFORMATION RequestConnectionInformation;
    PTDI_CONNECTION_INFORMATION ReturnConnectionInformation;
} TDI_REQUEST_KERNEL_ACCEPT, *PTDI_REQUEST_KERNEL_ACCEPT;

/* TdiReceive */
typedef struct _TDI_REQUEST_KERNEL_RECEIVE
{
    ULONG   ReceiveLength;
    ULONG   ReceiveFlags;
} TDI_REQUEST_KERNEL_RECEIVE, *PTDI_REQUEST_KERNEL_RECEIVE;

/* TdiReceiveDatagram */
typedef struct _TDI_REQUEST_KERNEL_RECEIVEDG
{
    ULONG   ReceiveLength;
    PTDI_CONNECTION_INFORMATION ReceiveDatagramInformation;
    PTDI_CONNECTION_INFORMATION ReturnDatagramInformation;
    ULONG   ReceiveFlags;
} TDI_REQUEST_KERNEL_RECEIVEDG, *PTDI_REQUEST_KERNEL_RECEIVEDG;

/* TdiSend */
typedef struct _TDI_REQUEST_KERNEL_SEND
{
    ULONG   SendLength;
    ULONG   SendFlags;
} TDI_REQUEST_KERNEL_SEND, *PTDI_REQUEST_KERNEL_SEND;

/* TdiSendDatagram */
typedef struct _TDI_REQUEST_KERNEL_SENDDG
{
    ULONG   SendLength;
    PTDI_CONNECTION_INFORMATION SendDatagramInformation;
} TDI_REQUEST_KERNEL_SENDDG, *PTDI_REQUEST_KERNEL_SENDDG;

/* TdiSetEventHandler */
typedef struct _TDI_REQUEST_KERNEL_SET_EVENT
{
    LONG    EventType;
    PVOID   EventHandler;
    PVOID   EventContext;
} TDI_REQUEST_KERNEL_SET_EVENT, *PTDI_REQUEST_KERNEL_SET_EVENT;

/* TdiQueryInformation */
typedef struct _TDI_REQUEST_KERNEL_QUERY_INFO
{
    LONG    QueryType;
    PTDI_CONNECTION_INFORMATION RequestConnectionInformation;
} TDI_REQUEST_KERNEL_QUERY_INFORMATION, *PTDI_REQUEST_KERNEL_QUERY_INFORMATION;

/* TdiSetInformation */
typedef struct _TDI_REQUEST_KERNEL_SET_INFO
{
    LONG    SetType;
    PTDI_CONNECTION_INFORMATION RequestConnectionInformation;
} TDI_REQUEST_KERNEL_SET_INFORMATION, *PTDI_REQUEST_KERNEL_SET_INFORMATION;


/* Event types */
#define TDI_EVENT_CONNECT                   0
#define TDI_EVENT_DISCONNECT                1
#define TDI_EVENT_ERROR                     2
#define TDI_EVENT_RECEIVE                   3
#define TDI_EVENT_RECEIVE_DATAGRAM          4
#define TDI_EVENT_RECEIVE_EXPEDITED         5
#define TDI_EVENT_SEND_POSSIBLE             6
#define TDI_EVENT_CHAINED_RECEIVE           7
#define TDI_EVENT_CHAINED_RECEIVE_DATAGRAM  8
#define TDI_EVENT_CHAINED_RECEIVE_EXPEDITED 9

typedef NTSTATUS (*PTDI_IND_CONNECT)(
    IN PVOID  TdiEventContext,
    IN LONG   RemoteAddressLength,
    IN PVOID  RemoteAddress,
    IN LONG   UserDataLength,
    IN PVOID  UserData,
    IN LONG   OptionsLength,
    IN PVOID  Options,
    OUT CONNECTION_CONTEXT  *ConnectionContext,
    OUT PIRP  *AcceptIrp);

NTSTATUS STDCALL TdiDefaultConnectHandler(
    IN PVOID  TdiEventContext,
    IN LONG   RemoteAddressLength,
    IN PVOID  RemoteAddress,
    IN LONG   UserDataLength,
    IN PVOID  UserData,
    IN LONG   OptionsLength,
    IN PVOID  Options,
    OUT CONNECTION_CONTEXT *ConnectionContext,
    OUT PIRP  *AcceptIrp);

typedef NTSTATUS (*PTDI_IND_DISCONNECT)(
    IN PVOID  TdiEventContext,
    IN CONNECTION_CONTEXT  ConnectionContext,
    IN LONG   DisconnectDataLength,
    IN PVOID  DisconnectData,
    IN LONG   DisconnectInformationLength,
    IN PVOID  DisconnectInformation,
    IN ULONG  DisconnectFlags);

NTSTATUS STDCALL TdiDefaultDisconnectHandler(
    IN PVOID  TdiEventContext,
    IN CONNECTION_CONTEXT  ConnectionContext,
    IN LONG   DisconnectDataLength,
    IN PVOID  DisconnectData,
    IN LONG   DisconnectInformationLength,
    IN PVOID  DisconnectInformation,
    IN ULONG  DisconnectFlags);

typedef NTSTATUS (*PTDI_IND_ERROR)(
    IN PVOID     TdiEventContext,
    IN NTSTATUS  Status);

NTSTATUS STDCALL TdiDefaultErrorHandler(
    IN PVOID     TdiEventContext,
    IN NTSTATUS  Status);

typedef NTSTATUS (*PTDI_IND_RECEIVE)(
    IN PVOID   TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG   ReceiveFlags,
    IN ULONG   BytesIndicated,
    IN ULONG   BytesAvailable,
    OUT ULONG  *BytesTaken,
    IN PVOID   Tsdu,
    OUT PIRP   *IoRequestPacket);

NTSTATUS STDCALL  TdiDefaultReceiveHandler(
    IN PVOID   TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG   ReceiveFlags,
    IN ULONG   BytesIndicated,
    IN ULONG   BytesAvailable,
    OUT ULONG  *BytesTaken,
    IN PVOID   Tsdu,
    OUT PIRP   *IoRequestPacket);

typedef NTSTATUS (*PTDI_IND_RECEIVE_DATAGRAM)(
    IN PVOID   TdiEventContext,
    IN LONG    SourceAddressLength,
    IN PVOID   SourceAddress,
    IN LONG    OptionsLength,
    IN PVOID   Options,
    IN ULONG   ReceiveDatagramFlags,
    IN ULONG   BytesIndicated,
    IN ULONG   BytesAvailable,
    OUT ULONG  *BytesTaken,
    IN PVOID   Tsdu,
    OUT PIRP   *IoRequestPacket);

NTSTATUS STDCALL TdiDefaultRcvDatagramHandler(
    IN PVOID   TdiEventContext,
    IN LONG    SourceAddressLength,
    IN PVOID   SourceAddress,
    IN LONG    OptionsLength,
    IN PVOID   Options,
    IN ULONG   ReceiveDatagramFlags,
    IN ULONG   BytesIndicated,
    IN ULONG   BytesAvailable,
    OUT ULONG  *BytesTaken,
    IN PVOID   Tsdu,
    OUT PIRP   *IoRequestPacket);

typedef NTSTATUS (*PTDI_IND_RECEIVE_EXPEDITED)(
    IN PVOID   TdiEventContext,
    IN CONNECTION_CONTEXT  ConnectionContext,
    IN ULONG   ReceiveFlags,
    IN ULONG   BytesIndicated,
    IN ULONG   BytesAvailable,
    OUT ULONG  *BytesTaken,
    IN PVOID   Tsdu,
    OUT PIRP   *IoRequestPacket);

NTSTATUS STDCALL TdiDefaultRcvExpeditedHandler(
    IN PVOID   TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG   ReceiveFlags,
    IN ULONG   BytesIndicated,
    IN ULONG   BytesAvailable,
    OUT ULONG  *BytesTaken,
    IN PVOID   Tsdu,
    OUT PIRP   *IoRequestPacket);

typedef NTSTATUS (*PTDI_IND_CHAINED_RECEIVE)(
    IN PVOID  TdiEventContext,
    IN CONNECTION_CONTEXT  ConnectionContext,
    IN ULONG  ReceiveFlags,
    IN ULONG  ReceiveLength,
    IN ULONG  StartingOffset,
    IN PMDL   Tsdu,
    IN PVOID  TsduDescriptor);

NTSTATUS STDCALL TdiDefaultChainedReceiveHandler(
    IN PVOID  TdiEventContext,
    IN CONNECTION_CONTEXT  ConnectionContext,
    IN ULONG  ReceiveFlags,
    IN ULONG  ReceiveLength,
    IN ULONG  StartingOffset,
    IN PMDL   Tsdu,
    IN PVOID  TsduDescriptor);

typedef NTSTATUS (*PTDI_IND_CHAINED_RECEIVE_DATAGRAM)(
    IN PVOID  TdiEventContext,
    IN LONG   SourceAddressLength,
    IN PVOID  SourceAddress,
    IN LONG   OptionsLength,
    IN PVOID  Options,
    IN ULONG  ReceiveDatagramFlags,
    IN ULONG  ReceiveDatagramLength,
    IN ULONG  StartingOffset,
    IN PMDL   Tsdu,
    IN PVOID  TsduDescriptor);

NTSTATUS STDCALL TdiDefaultChainedRcvDatagramHandler(
    IN PVOID  TdiEventContext,
    IN LONG   SourceAddressLength,
    IN PVOID  SourceAddress,
    IN LONG   OptionsLength,
    IN PVOID  Options,
    IN ULONG  ReceiveDatagramFlags,
    IN ULONG  ReceiveDatagramLength,
    IN ULONG  StartingOffset,
    IN PMDL   Tsdu,
    IN PVOID  TsduDescriptor);

typedef NTSTATUS (*PTDI_IND_CHAINED_RECEIVE_EXPEDITED)(
    IN PVOID  TdiEventContext,
    IN CONNECTION_CONTEXT  ConnectionContext,
    IN ULONG  ReceiveFlags,
    IN ULONG  ReceiveLength,
    IN ULONG  StartingOffset,
    IN PMDL   Tsdu,
    IN PVOID  TsduDescriptor);

NTSTATUS STDCALL TdiDefaultChainedRcvExpeditedHandler(
    IN PVOID  TdiEventContext,
    IN CONNECTION_CONTEXT  ConnectionContext,
    IN ULONG  ReceiveFlags,
    IN ULONG  ReceiveLength,
    IN ULONG  StartingOffset,
    IN PMDL   Tsdu,
    IN PVOID  TsduDescriptor);

typedef NTSTATUS (*PTDI_IND_SEND_POSSIBLE)(
    IN PVOID  TdiEventContext,
    IN PVOID  ConnectionContext,
    IN ULONG  BytesAvailable);

NTSTATUS STDCALL TdiDefaultSendPossibleHandler(
    IN PVOID  TdiEventContext,
    IN PVOID  ConnectionContext,
    IN ULONG  BytesAvailable);



/* Macros and functions to build IRPs */

#define TdiBuildBaseIrp(                                                    \
    Irp, DevObj, FileObj, CompRoutine, Contxt, IrpSp, Minor)                \
{                                                                           \
    IrpSp                = IoGetNextIrpStackLocation(Irp);                  \
    IrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;                  \
    IrpSp->MinorFunction = (Minor);                                         \
    IrpSp->DeviceObject  = (DevObj);                                        \
    IrpSp->FileObject    = (FileObj);                                       \
                                                                            \
    if (CompRoutine)                                                        \
        IoSetCompletionRoutine(Irp, CompRoutine, Contxt, TRUE, TRUE, TRUE)  \
    else                                                                    \
        IoSetCompletionRoutine(Irp, NULL, NULL, FALSE, FALSE, FALSE);       \
}

/*
 * VOID TdiBuildAccept(
 *     IN PIRP  Irp,
 *     IN PDEVICE_OBJECT  DevObj,
 *     IN PFILE_OBJECT  FileObj,
 *     IN PVOID  CompRoutine,
 *     IN PVOID  Contxt,
 *     IN PTDI_CONNECTION_INFORMATION  RequestConnectionInfo,
 *     OUT PTDI_CONNECTION_INFORMATION  ReturnConnectionInfo);
 */
#define TdiBuildAccept(                                               \
    Irp, DevObj, FileObj, CompRoutine, Contxt,                        \
    RequestConnectionInfo, ReturnConnectionInfo)                      \
    {                                                                 \
    PTDI_REQUEST_KERNEL_ACCEPT _Request;                              \
    PIO_STACK_LOCATION _IrpSp;                                        \
                                                                      \
    _IrpSp = IoGetNextIrpStackLocation(Irp);                          \
                                                                      \
    TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,                \
                    Contxt, _IrpSp, TDI_ACCEPT);                      \
                                                                      \
    _Request = (PTDI_REQUEST_KERNEL_ACCEPT)&_IrpSp->Parameters;       \
    _Request->RequestConnectionInformation = (RequestConnectionInfo); \
    _Request->ReturnConnectionInformation  = (ReturnConnectionInfo);  \
}

/*
 * VOID TdiBuildAction(
 *     IN PIRP  Irp,
 *     IN PDEVICE_OBJECT  DevObj,
 *     IN PFILE_OBJECT  FileObj,
 *     IN PVOID  CompRoutine,
 *     IN PVOID  Contxt,
 *     IN PMDL  MdlAddr);
 */
#define TdiBuildAction(                                 \
    Irp, DevObj, FileObj, CompRoutine, Contxt, MdlAddr) \
{                                                       \
    PIO_STACK_LOCATION _IrpSp;                          \
                                                        \
    _IrpSp = IoGetNextIrpStackLocation(Irp);            \
                                                        \
    TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,  \
                    Contxt, _IrpSp, TDI_ACTION);        \
                                                        \
    (Irp)->MdlAddress = (MdlAddr);                      \
}

/*
 * VOID TdiBuildAssociateAddress(
 *    IN PIRP  Irp,
 *    IN PDEVICE_OBJECT  DevObj,
 *    IN PFILE_OBJECT  FileObj,
 *    IN PVOID  CompRoutine,
 *    IN PVOID  Contxt,
 *    IN HANDLE  AddrHandle);
 */
#define TdiBuildAssociateAddress(                                  \
    Irp, DevObj, FileObj, CompRoutine, Contxt, AddrHandle)         \
{                                                                  \
    PTDI_REQUEST_KERNEL_ASSOCIATE _Request;                        \
    PIO_STACK_LOCATION _IrpSp;                                     \
                                                                   \
    _IrpSp = IoGetNextIrpStackLocation(Irp);                       \
                                                                   \
    TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,             \
                    Contxt, _IrpSp, TDI_ASSOCIATE_ADDRESS);        \
                                                                   \
    _Request = (PTDI_REQUEST_KERNEL_ASSOCIATE)&_IrpSp->Parameters; \
    _Request->AddressHandle = (HANDLE)(AddrHandle);                \
}

/*
 * VOID TdiBuildConnect(
 *    IN PIRP  Irp,
 *    IN PDEVICE_OBJECT  DevObj,
 *    IN PFILE_OBJECT  FileObj,
 *    IN PVOID  CompRoutine,
 *    IN PVOID  Contxt,
 *    IN PLARGE_INTEGER  Time,
 *    IN PTDI_CONNECTION_INFORMATION  RequestConnectionInfo,
 *    OUT PTDI_CONNECTION_INFORMATION  ReturnConnectionInfo); 
 */
#define TdiBuildConnect(                                              \
    Irp, DevObj, FileObj, CompRoutine, Contxt,                        \
    Time, RequestConnectionInfo, ReturnConnectionInfo)                \
{                                                                     \
    PTDI_REQUEST_KERNEL _Request;                                     \
    PIO_STACK_LOCATION _IrpSp;                                        \
                                                                      \
    _IrpSp = IoGetNextIrpStackLocation(Irp);                          \
                                                                      \
    TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,                \
                    Contxt, _IrpSp, TDI_CONNECT);                     \
                                                                      \
    _Request = (PTDI_REQUEST_KERNEL)&_IrpSp->Parameters;              \
    _Request->RequestConnectionInformation = (RequestConnectionInfo); \
    _Request->ReturnConnectionInformation  = (ReturnConnectionInfo);  \
    _Request->RequestSpecific              = (PVOID)(Time);           \
}

/*
 * VOID TdiBuildDisassociateAddress(
 *     IN PIRP  Irp,
 *     IN PDEVICE_OBJECT  DevObj,
 *     IN PFILE_OBJECT  FileObj,
 *     IN PVOID  CompRoutine,
 *     IN PVOID  Contxt);
 */
#define TdiBuildDisassociateAddress(                                  \
    Irp, DevObj, FileObj, CompRoutine, Contxt)                        \
{                                                                     \
    PIO_STACK_LOCATION _IrpSp;                                        \
                                                                      \
    _IrpSp = IoGetNextIrpStackLocation(Irp);                          \
                                                                      \
    TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,                \
                    Contxt, _IrpSp, TDI_DISASSOCIATE_ADDRESS);        \
}

/*
 * VOID TdiBuildDisconnect(
 *     IN PIRP  Irp,
 *     IN PDEVICE_OBJECT  DevObj,
 *     IN PFILE_OBJECT  FileObj,
 *     IN PVOID  CompRoutine,
 *     IN PVOID  Contxt,
 *     IN PLARGE_INTEGER  Time,
 *     IN PULONG  Flags,
 *     IN PTDI_CONNECTION_INFORMATION  RequestConnectionInfo,
 *     OUT PTDI_CONNECTION_INFORMATION  ReturnConnectionInfo); 
 */
#define TdiBuildDisconnect(                                           \
    Irp, DevObj, FileObj, CompRoutine, Contxt, Time,                  \
    Flags, RequestConnectionInfo, ReturnConnectionInfo)               \
{                                                                     \
    PTDI_REQUEST_KERNEL _Request;                                     \
    PIO_STACK_LOCATION _IrpSp;                                        \
                                                                      \
    _IrpSp = IoGetNextIrpStackLocation(Irp);                          \
                                                                      \
    TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,                \
                    Contxt, _IrpSp, TDI_DISCONNECT);                  \
                                                                      \
    _Request = (PTDI_REQUEST_KERNEL)&_IrpSp->Parameters;              \
    _Request->RequestConnectionInformation = (RequestConnectionInfo); \
    _Request->ReturnConnectionInformation  = (ReturnConnectionInfo);  \
    _Request->RequestSpecific = (PVOID)(Time);                        \
    _Request->RequestFlags    = (Flags);                              \
}

/*
 * PIRP TdiBuildInternalDeviceControlIrp(
 *     IN CCHAR IrpSubFunction,
 *     IN PDEVICE_OBJECT DeviceObject,
 *     IN PFILE_OBJECT FileObject,
 *     IN PKEVENT Event,
 *     IN PIO_STATUS_BLOCK IoStatusBlock);
 */
#define TdiBuildInternalDeviceControlIrp( \
    IrpSubFunction, DeviceObject,         \
    FileObject, Event, IoStatusBlock)     \
    IoBuildDeviceIoControlRequest(        \
        0x00000003, DeviceObject,         \
        NULL, 0, NULL, 0,                 \
        TRUE, Event, IoStatusBlock)

/*
 * VOID TdiBuildListen(
 *     IN PIRP  Irp,
 *     IN PDEVICE_OBJECT  DevObj,
 *     IN PFILE_OBJECT  FileObj,
 *     IN PVOID  CompRoutine,
 *     IN PVOID  Contxt,
 *     IN ULONG  Flags,
 *     IN PTDI_CONNECTION_INFORMATION  RequestConnectionInfo,
 *     OUT PTDI_CONNECTION_INFORMATION  ReturnConnectionInfo); 
 */
#define TdiBuildListen(                                               \
    Irp, DevObj, FileObj, CompRoutine, Contxt,                        \
    Flags, RequestConnectionInfo, ReturnConnectionInfo)               \
{                                                                     \
    PTDI_REQUEST_KERNEL _Request;                                     \
    PIO_STACK_LOCATION _IrpSp;                                        \
                                                                      \
    _IrpSp = IoGetNextIrpStackLocation(Irp);                          \
                                                                      \
    TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,                \
                    Contxt, _IrpSp, TDI_LISTEN);                      \
                                                                      \
    _Request = (PTDI_REQUEST_KERNEL)&_IrpSp->Parameters;              \
    _Request->RequestConnectionInformation = (RequestConnectionInfo); \
    _Request->ReturnConnectionInformation  = (ReturnConnectionInfo);  \
    _Request->RequestFlags = (Flags);                                 \
}

VOID STDCALL TdiBuildNetbiosAddress(
    IN PUCHAR NetbiosName,
    IN BOOLEAN IsGroupName,
    IN OUT PTA_NETBIOS_ADDRESS NetworkName);

NTSTATUS STDCALL TdiBuildNetbiosAddressEa(
    IN PUCHAR Buffer,
    IN BOOLEAN IsGroupName,
    IN PUCHAR NetbiosName);

/*
 * VOID TdiBuildQueryInformation(
 *     IN PIRP  Irp,
 *     IN PDEVICE_OBJECT  DevObj,
 *     IN PFILE_OBJECT  FileObj,
 *     IN PVOID  CompRoutine,
 *     IN PVOID  Contxt,
 *     IN UINT  QType,
 *     IN PMDL  MdlAddr);
 */
#define TdiBuildQueryInformation(                                          \
    Irp, DevObj, FileObj, CompRoutine, Contxt, QType, MdlAddr)             \
{                                                                          \
    PTDI_REQUEST_KERNEL_QUERY_INFORMATION _Request;                        \
    PIO_STACK_LOCATION _IrpSp;                                             \
                                                                           \
    _IrpSp = IoGetNextIrpStackLocation(Irp);                               \
                                                                           \
    TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,                     \
                    Contxt, _IrpSp, TDI_QUERY_INFORMATION);                \
                                                                           \
    _Request = (PTDI_REQUEST_KERNEL_QUERY_INFORMATION)&_IrpSp->Parameters; \
    _Request->RequestConnectionInformation = NULL;                         \
    _Request->QueryType = (ULONG)(QType);                                  \
    (Irp)->MdlAddress   = (MdlAddr);                                       \
}

/*
 * VOID TdiBuildReceive(
 *     IN PIRP  Irp,
 *     IN PDEVICE_OBJECT  DevObj,
 *     IN PFILE_OBJECT  FileObj,
 *     IN PVOID  CompRoutine,
 *     IN PVOID  Contxt,
 *     IN PMDL  MdlAddr,
 *     IN ULONG  InFlags, 
 *     IN ULONG  ReceiveLen); 
 */
#define TdiBuildReceive(                                         \
    Irp, DevObj, FileObj, CompRoutine, Contxt,                   \
    MdlAddr, InFlags, ReceiveLen)                                \
{                                                                \
    PTDI_REQUEST_KERNEL_RECEIVE _Request;                        \
    PIO_STACK_LOCATION _IrpSp;                                   \
                                                                 \
    _IrpSp = IoGetNextIrpStackLocation(Irp);                     \
                                                                 \
    TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,           \
                    Contxt, _IrpSp, TDI_RECEIVE);                \
                                                                 \
    _Request = (PTDI_REQUEST_KERNEL_RECEIVE)&_IrpSp->Parameters; \
    _Request->ReceiveFlags  = (InFlags);                         \
    _Request->ReceiveLength = (ReceiveLen);                      \
    (Irp)->MdlAddress       = (MdlAddr);                         \
}

/*
 * VOID TdiBuildReceiveDatagram(
 *     IN PIRP  Irp,
 *     IN PDEVICE_OBJECT  DevObj,
 *     IN PFILE_OBJECT  FileObj,
 *     IN PVOID  CompRoutine,
 *     IN PVOID  Contxt,
 *     IN PMDL  MdlAddr,
 *     IN ULONG  ReceiveLen,
 *     IN PTDI_CONNECTION_INFORMATION  ReceiveDatagramInfo,
 *     OUT PTDI_CONNECTION_INFORMATION  ReturnInfo,
 *     ULONG InFlags); 
 */
#define TdiBuildReceiveDatagram(                                   \
    Irp, DevObj, FileObj, CompRoutine, Contxt, MdlAddr,            \
    ReceiveLen, ReceiveDatagramInfo, ReturnInfo, InFlags)          \
{                                                                  \
    PTDI_REQUEST_KERNEL_RECEIVEDG _Request;                        \
    PIO_STACK_LOCATION _IrpSp;                                     \
                                                                   \
    _IrpSp = IoGetNextIrpStackLocation(Irp);                       \
                                                                   \
    TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,             \
                    Contxt, _IrpSp, TDI_RECEIVE_DATAGRAM);         \
                                                                   \
    _Request = (PTDI_REQUEST_KERNEL_RECEIVEDG)&_IrpSp->Parameters; \
    _Request->ReceiveDatagramInformation = (ReceiveDatagramInfo);  \
    _Request->ReturnDatagramInformation  = (ReturnInfo);           \
    _Request->ReceiveLength = (ReceiveLen);                        \
    _Request->ReceiveFlags  = (InFlags);                           \
    (Irp)->MdlAddress       = (MdlAddr);                           \
}

/*
 * VOID TdiBuildSend(
 *     IN PIRP  Irp,
 *     IN PDEVICE_OBJECT  DevObj,
 *     IN PFILE_OBJECT  FileObj,
 *     IN PVOID  CompRoutine,
 *     IN PVOID  Contxt,
 *     IN PMDL  MdlAddr,
 *     IN ULONG  InFlags,
 *     IN ULONG  SendLen);
 */
#define TdiBuildSend(                                         \
    Irp, DevObj, FileObj, CompRoutine, Contxt,                \
    MdlAddr, InFlags, SendLen)                                \
{                                                             \
    PTDI_REQUEST_KERNEL_SEND _Request;                        \
    PIO_STACK_LOCATION _IrpSp;                                \
                                                              \
    _IrpSp = IoGetNextIrpStackLocation(Irp);                  \
                                                              \
    TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,        \
                    Contxt, _IrpSp, TDI_SEND);                \
                                                              \
    _Request = (PTDI_REQUEST_KERNEL_SEND)&_IrpSp->Parameters; \
    _Request->SendFlags  = (InFlags);                         \
    _Request->SendLength = (SendLen);                         \
    (Irp)->MdlAddress    = (MdlAddr);                         \
}

/*
 * VOID TdiBuildSendDatagram(
 *     IN PIRP  Irp,
 *     IN PDEVICE_OBJECT  DevObj,
 *     IN PFILE_OBJECT  FileObj,
 *     IN PVOID  CompRoutine,
 *     IN PVOID  Contxt,
 *     IN PMDL  MdlAddr,
 *     IN ULONG  SendLen,
 *     IN PTDI_CONNECTION_INFORMATION  SendDatagramInfo); 
 */
#define TdiBuildSendDatagram(                                   \
    Irp, DevObj, FileObj, CompRoutine, Contxt,                  \
    MdlAddr, SendLen, SendDatagramInfo)                         \
{                                                               \
    PTDI_REQUEST_KERNEL_SENDDG _Request;                        \
    PIO_STACK_LOCATION _IrpSp;                                  \
                                                                \
    _IrpSp = IoGetNextIrpStackLocation(Irp);                    \
                                                                \
    TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,          \
                    Contxt, _IrpSp, TDI_SEND_DATAGRAM);         \
                                                                \
    _Request = (PTDI_REQUEST_KERNEL_SENDDG)&_IrpSp->Parameters; \
    _Request->SendDatagramInformation = (SendDatagramInfo);     \
    _Request->SendLength = (SendLen);                           \
    (Irp)->MdlAddress    = (MdlAddr);                           \
}

/*
 * VOID TdiBuildSetEventHandler(
 *     IN PIRP  Irp,
 *     IN PDEVICE_OBJECT  DevObj,
 *     IN PFILE_OBJECT  FileObj,
 *     IN PVOID  CompRoutine,
 *     IN PVOID  Contxt,
 *     IN INT  InEventType,
 *     IN PVOID  InEventHandler,
 *     IN PVOID  InEventContext);
 */
#define TdiBuildSetEventHandler(                                   \
    Irp, DevObj, FileObj, CompRoutine, Contxt,                     \
    InEventType, InEventHandler, InEventContext)                   \
{                                                                  \
    PTDI_REQUEST_KERNEL_SET_EVENT _Request;                        \
    PIO_STACK_LOCATION _IrpSp;                                     \
                                                                   \
    _IrpSp = IoGetNextIrpStackLocation(Irp);                       \
                                                                   \
    TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,             \
                    Contxt, _IrpSp, TDI_SET_EVENT_HANDLER);        \
                                                                   \
    _Request = (PTDI_REQUEST_KERNEL_SET_EVENT)&_IrpSp->Parameters; \
    _Request->EventType    = (InEventType);                        \
    _Request->EventHandler = (PVOID)(InEventHandler);              \
    _Request->EventContext = (PVOID)(InEventContext);              \
}

/*
 * VOID TdiBuildSetInformation(
 *     IN PIRP  Irp,
 *     IN PDEVICE_OBJECT  DevObj,
 *     IN PFILE_OBJECT  FileObj,
 *     IN PVOID  CompRoutine,
 *     IN PVOID  Contxt,
 *     IN UINT  SType,
 *     IN PMDL  MdlAddr);
 */
#define TdiBuildSetInformation(                                          \
    Irp, DevObj, FileObj, CompRoutine, Contxt, SType, MdlAddr)           \
{                                                                        \
    PTDI_REQUEST_KERNEL_SET_INFORMATION _Request;                        \
    PIO_STACK_LOCATION _IrpSp;                                           \
                                                                         \
    _IrpSp = IoGetNextIrpStackLocation(Irp);                             \
                                                                         \
    TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,                   \
                    Contxt, _IrpSp, TDI_SET_INFORMATION);                \
                                                                         \
    _Request = (PTDI_REQUEST_KERNEL_SET_INFORMATION)&_IrpSp->Parameters; \
    _Request->RequestConnectionInformation = NULL;                       \
    _Request->SetType = (ULONG)(SType);                                  \
    (Irp)->MdlAddress = (MdlAddr);                                       \
}



/* TDI functions */

/*
 * VOID TdiCompleteRequest(
 *     IN PIRP Irp,
 *     IN NTSTATUS Status);
 */
#define TdiCompleteRequest(Irp, Status)             \
{                                                   \
    (Irp)->IoStatus.Status = (Status);              \
    IoCompleteRequest((Irp), IO_NETWORK_INCREMENT); \
}

NTSTATUS STDCALL TdiCopyBufferToMdl(
    IN PVOID SourceBuffer,
    IN ULONG SourceOffset,
    IN ULONG SourceBytesToCopy,
    IN PMDL DestinationMdlChain,
    IN ULONG DestinationOffset,
    IN PULONG BytesCopied);

/*
 * VOID TdiCopyLookaheadData(
 *     IN PVOID Destination,
 *     IN PVOID Source,
 *     IN ULONG Length,
 *     IN ULONG ReceiveFlags);
 */
#define TdiCopyLookaheadData(Destination, Source, Length, ReceiveFlags) \
    RtlCopyMemory(Destination, Source, Length)

NTSTATUS STDCALL TdiCopyMdlToBuffer(
    IN PMDL SourceMdlChain,
    IN ULONG SourceOffset,
    IN PVOID DestinationBuffer,
    IN ULONG DestinationOffset,
    IN ULONG DestinationBufferSize,
    OUT PULONG BytesCopied);

VOID STDCALL TdiMapBuffer(
    IN PMDL MdlChain);

NTSTATUS STDCALL TdiMapUserRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp);

VOID STDCALL TdiReturnChainedReceives(
    IN PVOID *TsduDescriptors,
    IN ULONG  NumberOfTsdus);

VOID STDCALL TdiUnmapBuffer(
    IN PMDL MdlChain);

#endif /* __TDIKRNL_H */

/* EOF */
