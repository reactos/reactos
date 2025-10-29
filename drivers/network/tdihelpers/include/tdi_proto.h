#pragma once

NTSTATUS TdiConnect(
    PIRP * PendingIrp,
    PFILE_OBJECT ConnectionObject,
    PTDI_CONNECTION_INFORMATION ConnectionCallInfo,
    PTDI_CONNECTION_INFORMATION ConnectionReturnInfo,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID CompletionContext);

NTSTATUS TdiOpenConnectionEndpointFile(
    PUNICODE_STRING DeviceName,
    PHANDLE ConnectionHandle,
    PFILE_OBJECT * ConnectionObject);

NTSTATUS TdiCloseDevice(HANDLE Handle, PFILE_OBJECT FileObject);

NTSTATUS TdiDisconnect(
    PIRP * Irp,
    PFILE_OBJECT TransportObject,
    PLARGE_INTEGER Time,
    USHORT Flags,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID CompletionContext,
    PTDI_CONNECTION_INFORMATION RequestConnectionInfo,
    PTDI_CONNECTION_INFORMATION ReturnConnectionInfo);

NTSTATUS TdiQueryInformation(PFILE_OBJECT FileObject, LONG QueryType, PMDL MdlBuffer);

NTSTATUS TdiSetEventHandler(PFILE_OBJECT FileObject, LONG EventType, PVOID Handler, PVOID Context);

NTSTATUS TdiQueryDeviceControl(
    PFILE_OBJECT FileObject,
    ULONG IoControlCode,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength,
    PULONG Return);
