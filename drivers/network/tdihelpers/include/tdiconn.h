#pragma once

typedef VOID *PTDI_CONNECTION_INFO_PAIR;

PTRANSPORT_ADDRESS TaCopyTransportAddress(PTRANSPORT_ADDRESS OtherAddress);
PTRANSPORT_ADDRESS TaBuildNullTransportAddress(UINT AddressType);
UINT TaLengthOfAddress(PTA_ADDRESS Addr);
UINT TaLengthOfTransportAddress(PTRANSPORT_ADDRESS Addr);
VOID TaCopyTransportAddressInPlace(PTRANSPORT_ADDRESS Target, PTRANSPORT_ADDRESS Source);
UINT TdiAddressSizeFromType(UINT Type);
UINT TdiAddressSizeFromName(PTRANSPORT_ADDRESS Name);
NTSTATUS TdiBuildConnectionInfoInPlace(PTDI_CONNECTION_INFORMATION ConnInfo, PTRANSPORT_ADDRESS Name);
NTSTATUS TdiBuildConnectionInfo(PTDI_CONNECTION_INFORMATION * ConnectionInfo, PTRANSPORT_ADDRESS Name);
NTSTATUS TdiBuildNullConnectionInfoInPlace(PTDI_CONNECTION_INFORMATION ConnInfo, ULONG Type);
NTSTATUS TdiBuildNullConnectionInfo(PTDI_CONNECTION_INFORMATION * ConnectionInfo, ULONG Type);

/* Taken from afd.h: */
/* tdi.c */

NTSTATUS TdiOpenAddressFile(PUNICODE_STRING DeviceName,
    PTRANSPORT_ADDRESS Name,
    ULONG ShareType,
    PHANDLE AddressHandle,
    PFILE_OBJECT * AddressObject);

NTSTATUS TdiAssociateAddressFile(HANDLE AddressHandle, PFILE_OBJECT ConnectionObject);

NTSTATUS TdiDisassociateAddressFile(PFILE_OBJECT ConnectionObject);

NTSTATUS TdiListen(
    PIRP * Irp,
    PFILE_OBJECT ConnectionObject,
    PTDI_CONNECTION_INFORMATION * RequestConnectionInfo,
    PTDI_CONNECTION_INFORMATION * ReturnConnectionInfo,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID CompletionContext);

NTSTATUS TdiReceive(
    PIRP * Irp,
    PFILE_OBJECT ConnectionObject,
    USHORT Flags,
    PCHAR Buffer,
    UINT BufferLength,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID CompletionContext);

NTSTATUS TdiSend(
    PIRP * Irp,
    PFILE_OBJECT ConnectionObject,
    USHORT Flags,
    PCHAR Buffer,
    UINT BufferLength,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID CompletionContext);

NTSTATUS TdiReceiveDatagram(
    PIRP * Irp,
    PFILE_OBJECT TransportObject,
    USHORT Flags,
    PCHAR Buffer,
    UINT BufferLength,
    PTDI_CONNECTION_INFORMATION From,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID CompletionContext);

NTSTATUS TdiSendDatagram(
    PIRP * Irp,
    PFILE_OBJECT TransportObject,
    PCHAR Buffer,
    UINT BufferLength,
    PTDI_CONNECTION_INFORMATION To,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID CompletionContext);

NTSTATUS TdiQueryMaxDatagramLength(PFILE_OBJECT FileObject, PUINT MaxDatagramLength);
