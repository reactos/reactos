/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Helper functions for NPFS tests
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>
#include "npfs.h"

NTSTATUS
NpCreatePipeEx(
    OUT PHANDLE ServerHandle,
    IN PCWSTR PipePath,
    IN ULONG ReadMode,
    IN ULONG CompletionMode,
    IN ULONG NamedPipeType,
    IN ULONG ShareAccess,
    IN ULONG MaximumInstances,
    IN ULONG InboundQuota,
    IN ULONG OutboundQuota,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG Disposition,
    IN ULONG CreateOptions,
    IN PLARGE_INTEGER DefaultTimeout OPTIONAL)
{
    UNICODE_STRING ObjectName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NAMED_PIPE_CREATE_PARAMETERS Params;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    RtlInitUnicodeString(&ObjectName, PipePath);
    InitializeObjectAttributes(&ObjectAttributes,
                               &ObjectName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Params.NamedPipeType = NamedPipeType;
    Params.ReadMode = ReadMode;
    Params.CompletionMode = CompletionMode;
    Params.MaximumInstances = MaximumInstances;
    Params.InboundQuota = InboundQuota;
    Params.OutboundQuota = OutboundQuota;
    if (DefaultTimeout)
    {
        Params.DefaultTimeout.QuadPart = DefaultTimeout->QuadPart;
        Params.TimeoutSpecified = TRUE;
    }
    else
    {
        Params.DefaultTimeout.QuadPart = 0;
        Params.TimeoutSpecified = FALSE;
    }

    RtlFillMemory(&IoStatusBlock, sizeof(IoStatusBlock), 0x55);
    Status = IoCreateFile(ServerHandle,
                          DesiredAccess,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL, /* AllocationSize */
                          0, /* FileAttributes */
                          ShareAccess,
                          Disposition,
                          CreateOptions,
                          NULL, /* EaBuffer */
                          0, /* EaLength */
                          CreateFileTypeNamedPipe,
                          &Params,
                          0);
    if (NT_SUCCESS(Status))
    {
        ok_eq_hex(IoStatusBlock.Status, Status);
        ok_eq_ulongptr(IoStatusBlock.Information, FILE_CREATED);
    }
    else
    {
        ok_eq_hex(IoStatusBlock.Status, 0x55555555UL);
        ok_eq_ulongptr(IoStatusBlock.Information, 0x5555555555555555ULL);
    }
    return Status;
}

NTSTATUS
NpCreatePipe(
    OUT PHANDLE ServerHandle,
    PCWSTR PipePath,
    ULONG ReadMode,
    ULONG CompletionMode,
    ULONG NamedPipeType,
    ULONG NamedPipeConfiguration,
    ULONG MaximumInstances,
    ULONG InboundQuota,
    ULONG OutboundQuota)
{
    ULONG ShareAccess;
    LARGE_INTEGER DefaultTimeout;

    if (NamedPipeConfiguration == FILE_PIPE_INBOUND)
        ShareAccess = FILE_SHARE_WRITE;
    else if (NamedPipeConfiguration == FILE_PIPE_OUTBOUND)
        ShareAccess = FILE_SHARE_READ;
    else if (NamedPipeConfiguration == FILE_PIPE_FULL_DUPLEX)
        ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE;

    DefaultTimeout.QuadPart = -50 * 1000 * 10;

    return NpCreatePipeEx(ServerHandle,
                          PipePath,
                          ReadMode,
                          CompletionMode,
                          NamedPipeType,
                          ShareAccess,
                          MaximumInstances,
                          InboundQuota,
                          OutboundQuota,
                          SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
                          FILE_OPEN_IF,
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          &DefaultTimeout);
}

NTSTATUS
NpOpenPipeEx(
    OUT PHANDLE ClientHandle,
    IN PCWSTR PipePath,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG ShareAccess,
    IN ULONG Disposition,
    IN ULONG CreateOptions)
{
    UNICODE_STRING ObjectName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    RtlInitUnicodeString(&ObjectName, PipePath);
    InitializeObjectAttributes(&ObjectAttributes,
                               &ObjectName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    RtlFillMemory(&IoStatusBlock, sizeof(IoStatusBlock), 0x55);
    Status = IoCreateFile(ClientHandle,
                          DesiredAccess,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL, /* AllocationSize */
                          0, /* FileAttributes */
                          ShareAccess,
                          Disposition,
                          CreateOptions,
                          NULL, /* EaBuffer */
                          0, /* EaLength */
                          CreateFileTypeNone,
                          NULL,
                          0);
    if (NT_SUCCESS(Status))
    {
        ok(Status != STATUS_PENDING, "IoCreateFile returned pending\n");
        ok_eq_hex(IoStatusBlock.Status, Status);
        ok_eq_ulongptr(IoStatusBlock.Information, FILE_OPENED);
    }
    else
    {
        ok_eq_hex(IoStatusBlock.Status, 0x55555555UL);
        ok_eq_ulongptr(IoStatusBlock.Information, 0x5555555555555555ULL);
    }
    return Status;
}

NTSTATUS
NpOpenPipe(
    OUT PHANDLE ClientHandle,
    IN PCWSTR PipePath,
    IN ULONG NamedPipeConfiguration)
{
    ULONG ShareAccess;

    if (NamedPipeConfiguration == FILE_PIPE_INBOUND)
        ShareAccess = FILE_SHARE_WRITE;
    else if (NamedPipeConfiguration == FILE_PIPE_OUTBOUND)
        ShareAccess = FILE_SHARE_READ;
    else if (NamedPipeConfiguration == FILE_PIPE_FULL_DUPLEX)
        ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE;

    return NpOpenPipeEx(ClientHandle,
                        PipePath,
                        SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
                        ShareAccess,
                        FILE_OPEN,
                        FILE_SYNCHRONOUS_IO_NONALERT);
}

NTSTATUS
NpControlPipe(
    IN HANDLE ServerHandle,
    IN ULONG FsControlCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    RtlFillMemory(&IoStatusBlock, sizeof(IoStatusBlock), 0x55);
    Status = ZwFsControlFile(ServerHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             FsControlCode,
                             InputBuffer,
                             InputBufferLength,
                             NULL,
                             0);
    if (Status == STATUS_PENDING)
    {
        Status = ZwWaitForSingleObject(ServerHandle,
                                       FALSE,
                                       NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = IoStatusBlock.Status;
    }
    if (NT_SUCCESS(Status))
    {
        ok_eq_hex(IoStatusBlock.Status, Status);
        ok_eq_ulongptr(IoStatusBlock.Information, 0);
    }
    else
    {
        ok_eq_hex(IoStatusBlock.Status, 0x55555555UL);
        ok_eq_ulongptr(IoStatusBlock.Information, 0x5555555555555555ULL);
    }
    return Status;
}

NTSTATUS
NpWaitPipe(
    IN PCWSTR PipeName,
    IN PLARGE_INTEGER Timeout)
{
    NTSTATUS Status;
    HANDLE RootHandle;
    UNICODE_STRING RootDirectoryName = RTL_CONSTANT_STRING(DEVICE_NAMED_PIPE);
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_PIPE_WAIT_FOR_BUFFER WaitForBuffer;
    ULONG NameLength;
    ULONG BufferSize;

    InitializeObjectAttributes(&ObjectAttributes,
                               &RootDirectoryName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    RtlFillMemory(&IoStatusBlock, sizeof(IoStatusBlock), 0x55);
    Status = IoCreateFile(&RootHandle,
                          FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          0,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN,
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0,
                          CreateFileTypeNone,
                          NULL,
                          0);
    if (!NT_SUCCESS(Status))
    {
        ok_eq_hex(IoStatusBlock.Status, 0x55555555UL);
        ok_eq_ulongptr(IoStatusBlock.Information, 0x5555555555555555ULL);
        return Status;
    }
    ok(Status != STATUS_PENDING, "IoCreateFile returned pending\n");
    ok_eq_hex(IoStatusBlock.Status, Status);
    ok_eq_ulongptr(IoStatusBlock.Information, FILE_OPENED);

    NameLength = wcslen(PipeName) * sizeof(WCHAR);
    BufferSize = FIELD_OFFSET(FILE_PIPE_WAIT_FOR_BUFFER,
                              Name[NameLength / sizeof(WCHAR)]);
    WaitForBuffer = ExAllocatePoolWithTag(NonPagedPool, BufferSize, 'WPmK');
    if (WaitForBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    if (Timeout)
    {
        WaitForBuffer->Timeout.QuadPart = Timeout->QuadPart;
        WaitForBuffer->TimeoutSpecified = TRUE;
    }
    else
    {
        WaitForBuffer->Timeout.QuadPart = 0;
        WaitForBuffer->TimeoutSpecified = FALSE;
    }
    WaitForBuffer->NameLength = NameLength;
    RtlCopyMemory(WaitForBuffer->Name, PipeName, NameLength);
    Status = NpControlPipe(RootHandle,
                           FSCTL_PIPE_WAIT,
                           WaitForBuffer,
                           BufferSize);
    ExFreePoolWithTag(WaitForBuffer, 'WPmK');
    return Status;
}

NTSTATUS
NpReadPipe(
    IN HANDLE PipeHandle,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG_PTR BytesRead)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN PendingReturned = FALSE;

    RtlFillMemory(&IoStatusBlock, sizeof(IoStatusBlock), 0x55);
    Status = ZwReadFile(PipeHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        Buffer,
                        BufferSize,
                        NULL,
                        NULL);
    if (Status == STATUS_PENDING)
    {
        Status = ZwWaitForSingleObject(PipeHandle,
                                       FALSE,
                                       NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = IoStatusBlock.Status;
        PendingReturned = TRUE;
    }
    if (NT_SUCCESS(Status))
    {
        ok_eq_hex(IoStatusBlock.Status, Status);
        *BytesRead = IoStatusBlock.Information;
    }
    else
    {
        if (PendingReturned)
        {
            ok_eq_hex(IoStatusBlock.Status, Status);
            ok_eq_ulongptr(IoStatusBlock.Information, 0);
        }
        else
        {
            ok_eq_hex(IoStatusBlock.Status, 0x55555555UL);
            ok_eq_ulongptr(IoStatusBlock.Information, 0x5555555555555555ULL);
        }
        *BytesRead = 0;
    }
    return Status;
}

NTSTATUS
NpWritePipe(
    IN HANDLE PipeHandle,
    IN const VOID *Buffer,
    IN ULONG BufferSize,
    OUT PULONG_PTR BytesWritten)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    RtlFillMemory(&IoStatusBlock, sizeof(IoStatusBlock), 0x55);
    Status = ZwWriteFile(PipeHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         (PVOID)Buffer,
                         BufferSize,
                         NULL,
                         NULL);
    if (Status == STATUS_PENDING)
    {
        Status = ZwWaitForSingleObject(PipeHandle,
                                       FALSE,
                                       NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = IoStatusBlock.Status;
    }
    if (NT_SUCCESS(Status))
    {
        ok_eq_hex(IoStatusBlock.Status, Status);
        *BytesWritten = IoStatusBlock.Information;
    }
    else
    {
        ok_eq_hex(IoStatusBlock.Status, 0x55555555UL);
        ok_eq_ulongptr(IoStatusBlock.Information, 0x5555555555555555ULL);
        *BytesWritten = 0;
    }
    return Status;
}

static
BOOLEAN
CheckBuffer(
    PVOID Buffer,
    SIZE_T Size,
    UCHAR Value)
{
    PUCHAR Array = Buffer;
    SIZE_T i;

    for (i = 0; i < Size; i++)
        if (Array[i] != Value)
        {
            trace("Expected %x, found %x at offset %lu\n", Value, Array[i], (ULONG)i);
            return FALSE;
        }
    return TRUE;
}

#define ok_eq_print_(value, expected, spec, FileAndLine) \
                                            KmtOk((value) == (expected), FileAndLine, #value " = " spec ", expected " spec "\n", value, expected)
#define ok_eq_ulong_(value, expected)       ok_eq_print_(value, expected, "%lu", FileAndLine)
#define ok_eq_ulonglong_(value, expected)   ok_eq_print_(value, expected, "%I64u", FileAndLine)
#ifndef _WIN64
#define ok_eq_ulongptr_(value, expected)    ok_eq_print_(value, (ULONG_PTR)(expected), "%lu", FileAndLine)
#elif defined _WIN64
#define ok_eq_ulongptr_(value, expected)    ok_eq_print_(value, (ULONG_PTR)(expected), "%I64u", FileAndLine)
#endif
#define ok_eq_hex_(value, expected)         ok_eq_print_(value, expected, "0x%08lx", FileAndLine)

VOID
NpCheckServerPipe_(
    IN HANDLE ServerHandle,
    /* PipeInformation */
    IN ULONG ReadMode,
    IN ULONG CompletionMode,
    /* PipeLocalInformation */
    IN ULONG NamedPipeType,
    IN ULONG NamedPipeConfiguration,
    IN ULONG MaximumInstances,
    IN ULONG CurrentInstances,
    IN ULONG InboundQuota,
    IN ULONG ReadDataAvailable,
    IN ULONG OutboundQuota,
    IN ULONG WriteQuotaAvailable,
    IN ULONG NamedPipeState,
    /* PipeRemoteInformation */
    /* */
    IN PCSTR FileAndLine)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_PIPE_INFORMATION PipeInfo;
    FILE_PIPE_LOCAL_INFORMATION PipeLocalInfo;
    FILE_PIPE_REMOTE_INFORMATION PipeRemoteInfo;

    RtlFillMemory(&IoStatusBlock, sizeof(IoStatusBlock), 0x55);
    RtlFillMemory(&PipeInfo, sizeof(PipeInfo), 0x55);
    Status = ZwQueryInformationFile(ServerHandle,
                                    &IoStatusBlock,
                                    &PipeInfo,
                                    sizeof(PipeInfo),
                                    FilePipeInformation);
    ok_eq_hex_(Status, STATUS_SUCCESS);
    ok_eq_hex_(IoStatusBlock.Status, STATUS_SUCCESS);
    ok_eq_ulongptr_(IoStatusBlock.Information, sizeof(PipeInfo));
    ok_eq_ulong_(PipeInfo.ReadMode, ReadMode);
    ok_eq_ulong_(PipeInfo.CompletionMode, CompletionMode);

    RtlFillMemory(&IoStatusBlock, sizeof(IoStatusBlock), 0x55);
    RtlFillMemory(&PipeLocalInfo, sizeof(PipeLocalInfo), 0x55);
    Status = ZwQueryInformationFile(ServerHandle,
                                    &IoStatusBlock,
                                    &PipeLocalInfo,
                                    sizeof(PipeLocalInfo),
                                    FilePipeLocalInformation);
    ok_eq_hex_(Status, STATUS_SUCCESS);
    ok_eq_hex_(IoStatusBlock.Status, STATUS_SUCCESS);
    ok_eq_ulongptr_(IoStatusBlock.Information, sizeof(PipeLocalInfo));
    ok_eq_ulong_(PipeLocalInfo.NamedPipeType, NamedPipeType);
    ok_eq_ulong_(PipeLocalInfo.NamedPipeConfiguration, NamedPipeConfiguration);
    ok_eq_ulong_(PipeLocalInfo.MaximumInstances, MaximumInstances);
    ok_eq_ulong_(PipeLocalInfo.CurrentInstances, CurrentInstances);
    ok_eq_ulong_(PipeLocalInfo.InboundQuota, InboundQuota);
    ok_eq_ulong_(PipeLocalInfo.ReadDataAvailable, ReadDataAvailable);
    ok_eq_ulong_(PipeLocalInfo.OutboundQuota, OutboundQuota);
    ok_eq_ulong_(PipeLocalInfo.WriteQuotaAvailable, WriteQuotaAvailable);
    ok_eq_ulong_(PipeLocalInfo.NamedPipeState, NamedPipeState);
    ok_eq_ulong_(PipeLocalInfo.NamedPipeEnd, (ULONG)FILE_PIPE_SERVER_END);

    RtlFillMemory(&IoStatusBlock, sizeof(IoStatusBlock), 0x55);
    RtlFillMemory(&PipeRemoteInfo, sizeof(PipeRemoteInfo), 0x55);
    Status = ZwQueryInformationFile(ServerHandle,
                                    &IoStatusBlock,
                                    &PipeRemoteInfo,
                                    sizeof(PipeRemoteInfo),
                                    FilePipeInformation);
    ok_eq_hex_(Status, STATUS_SUCCESS);
    ok_eq_hex_(IoStatusBlock.Status, STATUS_SUCCESS);
    ok_eq_ulongptr_(IoStatusBlock.Information, RTL_SIZEOF_THROUGH_FIELD(FILE_PIPE_REMOTE_INFORMATION, CollectDataTime));
    ok_eq_ulonglong_(PipeRemoteInfo.CollectDataTime.QuadPart, 0ULL);
    ok_eq_ulong_(PipeRemoteInfo.MaximumCollectionCount, 0x55555555UL);
}

VOID
NpCheckClientPipe_(
    IN HANDLE ClientHandle,
    /* PipeInformation */
    IN ULONG ReadMode,
    IN ULONG CompletionMode,
    /* PipeLocalInformation */
    IN ULONG NamedPipeType,
    IN ULONG NamedPipeConfiguration,
    IN ULONG MaximumInstances,
    IN ULONG CurrentInstances,
    IN ULONG InboundQuota,
    IN ULONG ReadDataAvailable,
    IN ULONG OutboundQuota,
    IN ULONG WriteQuotaAvailable,
    IN ULONG NamedPipeState,
    /* PipeRemoteInformation */
    /* */
    IN PCSTR FileAndLine)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_PIPE_INFORMATION PipeInfo;
    FILE_PIPE_LOCAL_INFORMATION PipeLocalInfo;
    FILE_PIPE_REMOTE_INFORMATION PipeRemoteInfo;

    RtlFillMemory(&IoStatusBlock, sizeof(IoStatusBlock), 0x55);
    RtlFillMemory(&PipeInfo, sizeof(PipeInfo), 0x55);
    Status = ZwQueryInformationFile(ClientHandle,
                                    &IoStatusBlock,
                                    &PipeInfo,
                                    sizeof(PipeInfo),
                                    FilePipeInformation);
    ok_eq_hex_(Status, STATUS_SUCCESS);
    ok_eq_hex_(IoStatusBlock.Status, STATUS_SUCCESS);
    ok_eq_ulongptr_(IoStatusBlock.Information, sizeof(PipeInfo));
    ok_eq_ulong_(PipeInfo.ReadMode, ReadMode);
    ok_eq_ulong_(PipeInfo.CompletionMode, CompletionMode);

    RtlFillMemory(&IoStatusBlock, sizeof(IoStatusBlock), 0x55);
    RtlFillMemory(&PipeLocalInfo, sizeof(PipeLocalInfo), 0x55);
    Status = ZwQueryInformationFile(ClientHandle,
                                    &IoStatusBlock,
                                    &PipeLocalInfo,
                                    sizeof(PipeLocalInfo),
                                    FilePipeLocalInformation);
    ok_eq_hex_(Status, STATUS_SUCCESS);
    ok_eq_hex_(IoStatusBlock.Status, STATUS_SUCCESS);
    ok_eq_ulongptr_(IoStatusBlock.Information, sizeof(PipeLocalInfo));
    ok_eq_ulong_(PipeLocalInfo.NamedPipeType, NamedPipeType);
    ok_eq_ulong_(PipeLocalInfo.NamedPipeConfiguration, NamedPipeConfiguration);
    ok_eq_ulong_(PipeLocalInfo.MaximumInstances, MaximumInstances);
    ok_eq_ulong_(PipeLocalInfo.CurrentInstances, CurrentInstances);
    ok_eq_ulong_(PipeLocalInfo.InboundQuota, InboundQuota);
    ok_eq_ulong_(PipeLocalInfo.ReadDataAvailable, ReadDataAvailable);
    ok_eq_ulong_(PipeLocalInfo.OutboundQuota, OutboundQuota);
    ok_eq_ulong_(PipeLocalInfo.WriteQuotaAvailable, WriteQuotaAvailable);
    ok_eq_ulong_(PipeLocalInfo.NamedPipeState, NamedPipeState);
    ok_eq_ulong_(PipeLocalInfo.NamedPipeEnd, (ULONG)FILE_PIPE_CLIENT_END);

    RtlFillMemory(&IoStatusBlock, sizeof(IoStatusBlock), 0x55);
    RtlFillMemory(&PipeRemoteInfo, sizeof(PipeRemoteInfo), 0x55);
    Status = ZwQueryInformationFile(ClientHandle,
                                    &IoStatusBlock,
                                    &PipeRemoteInfo,
                                    sizeof(PipeRemoteInfo),
                                    FilePipeInformation);
    ok_eq_hex_(Status, STATUS_SUCCESS);
    ok_eq_hex_(IoStatusBlock.Status, STATUS_SUCCESS);
    ok_eq_ulongptr_(IoStatusBlock.Information, RTL_SIZEOF_THROUGH_FIELD(FILE_PIPE_REMOTE_INFORMATION, CollectDataTime));
    ok_eq_ulonglong_(PipeRemoteInfo.CollectDataTime.QuadPart, 0ULL);
    ok_eq_ulong_(PipeRemoteInfo.MaximumCollectionCount, 0x55555555UL);
}

VOID
NpQueryPipe_(
    IN HANDLE PipeHandle,
    IN NTSTATUS ExpectedStatus,
    IN PCSTR FileAndLine)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_PIPE_INFORMATION PipeInfo;
    FILE_PIPE_LOCAL_INFORMATION PipeLocalInfo;
    FILE_PIPE_REMOTE_INFORMATION PipeRemoteInfo;

    ASSERT(!NT_SUCCESS(ExpectedStatus));

    RtlFillMemory(&IoStatusBlock, sizeof(IoStatusBlock), 0x55);
    RtlFillMemory(&PipeInfo, sizeof(PipeInfo), 0x55);
    Status = ZwQueryInformationFile(PipeHandle,
                                    &IoStatusBlock,
                                    &PipeInfo,
                                    sizeof(PipeInfo),
                                    FilePipeInformation);
    ok_eq_hex_(Status, ExpectedStatus);
    ok_bool_true(CheckBuffer(&IoStatusBlock, sizeof(IoStatusBlock), 0x55), "CheckBuffer returned");
    ok_bool_true(CheckBuffer(&PipeInfo, sizeof(PipeInfo), 0x55), "CheckBuffer returned");

    RtlFillMemory(&IoStatusBlock, sizeof(IoStatusBlock), 0x55);
    RtlFillMemory(&PipeLocalInfo, sizeof(PipeLocalInfo), 0x55);
    Status = ZwQueryInformationFile(PipeHandle,
                                    &IoStatusBlock,
                                    &PipeLocalInfo,
                                    sizeof(PipeLocalInfo),
                                    FilePipeLocalInformation);
    ok_eq_hex_(Status, ExpectedStatus);
    ok_bool_true(CheckBuffer(&IoStatusBlock, sizeof(IoStatusBlock), 0x55), "CheckBuffer returned");
    ok_bool_true(CheckBuffer(&PipeLocalInfo, sizeof(PipeLocalInfo), 0x55), "CheckBuffer returned");

    RtlFillMemory(&IoStatusBlock, sizeof(IoStatusBlock), 0x55);
    RtlFillMemory(&PipeRemoteInfo, sizeof(PipeRemoteInfo), 0x55);
    Status = ZwQueryInformationFile(PipeHandle,
                                    &IoStatusBlock,
                                    &PipeRemoteInfo,
                                    sizeof(PipeRemoteInfo),
                                    FilePipeInformation);
    ok_eq_hex_(Status, ExpectedStatus);
    ok_bool_true(CheckBuffer(&IoStatusBlock, sizeof(IoStatusBlock), 0x55), "CheckBuffer returned");
    ok_bool_true(CheckBuffer(&PipeRemoteInfo, sizeof(PipeRemoteInfo), 0x55), "CheckBuffer returned");
}

static KSTART_ROUTINE PipeWorkerThread;
static
VOID
NTAPI
PipeWorkerThread(
    IN PVOID ThreadContext)
{
    PTHREAD_CONTEXT Context = ThreadContext;
    PVOID WaitEvents[2] = { &Context->ThreadDoneEvent,
                            &Context->StartWorkEvent };
    NTSTATUS Status;

    while (TRUE)
    {
        Status = KeWaitForMultipleObjects(RTL_NUMBER_OF(WaitEvents),
                                          WaitEvents,
                                          WaitAny,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          NULL,
                                          NULL);
        if (Status == STATUS_WAIT_0)
            break;
        ASSERT(Status == STATUS_WAIT_1);

        Context->Work(Context);

        KeSetEvent(&Context->WorkCompleteEvent, IO_NO_INCREMENT, TRUE);
    }
}

VOID
StartWorkerThread(
    OUT PTHREAD_CONTEXT Context)
{
    KeInitializeEvent(&Context->ThreadDoneEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&Context->StartWorkEvent, SynchronizationEvent, FALSE);
    KeInitializeEvent(&Context->WorkCompleteEvent, NotificationEvent, TRUE);

    Context->Thread = KmtStartThread(PipeWorkerThread, Context);
}

VOID
FinishWorkerThread(
    IN PTHREAD_CONTEXT Context)
{
    KmtFinishThread(Context->Thread, &Context->ThreadDoneEvent);
}

BOOLEAN
WaitForWork(
    IN PTHREAD_CONTEXT Context,
    IN ULONG MilliSeconds)
{
    LARGE_INTEGER Timeout;
    NTSTATUS Status;

    Timeout.QuadPart = -10 * 1000 * (LONGLONG)MilliSeconds;
    Status = KeWaitForSingleObject(&Context->WorkCompleteEvent,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   &Timeout);
    ok(Status == STATUS_SUCCESS || Status == STATUS_TIMEOUT, "Wait status %lx\n", Status);
    return Status != STATUS_TIMEOUT;
}

BOOLEAN
TriggerWork(
    IN PTHREAD_CONTEXT Context,
    IN ULONG MilliSeconds)
{
    NTSTATUS Status;

    Status = KeWaitForSingleObject(&Context->WorkCompleteEvent,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    KeResetEvent(&Context->WorkCompleteEvent);
    KeSetEvent(&Context->StartWorkEvent, IO_NO_INCREMENT, TRUE);
    return WaitForWork(Context, MilliSeconds);
}
