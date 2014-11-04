/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite NPFS helper declarations
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#ifndef _KMTEST_NPFS_H_
#define _KMTEST_NPFS_H_

#define DEVICE_NAMED_PIPE L"\\Device\\NamedPipe"

#define BYTE_STREAM FILE_PIPE_BYTE_STREAM_MODE
C_ASSERT(FILE_PIPE_BYTE_STREAM_MODE == FILE_PIPE_BYTE_STREAM_TYPE);
#define MESSAGE     FILE_PIPE_MESSAGE_MODE
C_ASSERT(FILE_PIPE_MESSAGE_MODE == FILE_PIPE_MESSAGE_TYPE);
#define QUEUE       FILE_PIPE_QUEUE_OPERATION
#define COMPLETE    FILE_PIPE_COMPLETE_OPERATION
#define INBOUND     FILE_PIPE_INBOUND
#define OUTBOUND    FILE_PIPE_OUTBOUND
#define DUPLEX      FILE_PIPE_FULL_DUPLEX

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
    IN PLARGE_INTEGER DefaultTimeout OPTIONAL);

NTSTATUS
NpCreatePipe(
    OUT PHANDLE ServerHandle,
    IN PCWSTR PipePath,
    IN ULONG ReadMode,
    IN ULONG CompletionMode,
    IN ULONG NamedPipeType,
    IN ULONG NamedPipeConfiguration,
    IN ULONG MaximumInstances,
    IN ULONG InboundQuota,
    IN ULONG OutboundQuota);

NTSTATUS
NpOpenPipeEx(
    OUT PHANDLE ClientHandle,
    IN PCWSTR PipePath,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG ShareAccess,
    IN ULONG Disposition,
    IN ULONG CreateOptions);

NTSTATUS
NpOpenPipe(
    OUT PHANDLE ClientHandle,
    IN PCWSTR PipePath,
    IN ULONG NamedPipeConfiguration);

NTSTATUS
NpControlPipe(
    IN HANDLE PipeHandle,
    IN ULONG FsControlCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength);

#define NpListenPipe(ServerHandle)      NpControlPipe(ServerHandle, FSCTL_PIPE_LISTEN, NULL, 0)
#define NpDisconnectPipe(ServerHandle)  NpControlPipe(ServerHandle, FSCTL_PIPE_DISCONNECT, NULL, 0)

NTSTATUS
NpWaitPipe(
    IN PCWSTR PipeName,
    IN PLARGE_INTEGER Timeout);

NTSTATUS
NpReadPipe(
    IN HANDLE PipeHandle,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG_PTR BytesRead);

NTSTATUS
NpWritePipe(
    IN HANDLE PipeHandle,
    IN const VOID *Buffer,
    IN ULONG BufferSize,
    OUT PULONG_PTR BytesWritten);

#define NpCheckServerPipe(h, rm, cm, npt, npc, mi, ci, iq, rsa, oq, wqa, nps) \
    NpCheckServerPipe__(h, rm, cm, npt, npc, mi, ci, iq, rsa, oq, wqa, nps, __FILE__, __LINE__)

#define NpCheckServerPipe__(h, rm, cm, npt, npc, mi, ci, iq, rsa, oq, wqa, nps, file, line) \
    NpCheckServerPipe_(h, rm, cm, npt, npc, mi, ci, iq, rsa, oq, wqa, nps, file ":" KMT_STRINGIZE(line))

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
    IN PCSTR FileAndLine);

#define NpCheckClientPipe(h, rm, cm, npt, npc, mi, ci, iq, rsa, oq, wqa, nps) \
    NpCheckClientPipe__(h, rm, cm, npt, npc, mi, ci, iq, rsa, oq, wqa, nps, __FILE__, __LINE__)

#define NpCheckClientPipe__(h, rm, cm, npt, npc, mi, ci, iq, rsa, oq, wqa, nps, file, line) \
    NpCheckClientPipe_(h, rm, cm, npt, npc, mi, ci, iq, rsa, oq, wqa, nps, file ":" KMT_STRINGIZE(line))

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
    IN PCSTR FileAndLine);

#define NpQueryPipe(h, es) \
    NpQueryPipe__(h, es, __FILE__, __LINE__)

#define NpQueryPipe__(h, es, file, line) \
    NpQueryPipe_(h, es, file ":" KMT_STRINGIZE(line))

VOID
NpQueryPipe_(
    IN HANDLE Handle,
    IN NTSTATUS ExpectedStatus,
    IN PCSTR FileAndLine);


struct _THREAD_CONTEXT;
typedef VOID (WORK_FUNCTION)(IN OUT struct _THREAD_CONTEXT *);
typedef WORK_FUNCTION *PWORK_FUNCTION;

typedef struct _THREAD_CONTEXT
{
    volatile PWORK_FUNCTION Work;
    volatile union
    {
        struct
        {
            PCWSTR PipePath;
            BOOLEAN ClientSynchronous;
            HANDLE ClientHandle;
            NTSTATUS Status;
        } Connect;
        struct
        {
            HANDLE ServerHandle;
            NTSTATUS Status;
        } Listen;
        struct
        {
            HANDLE PipeHandle;
            PVOID Buffer;
            ULONG BufferSize;
            ULONG_PTR BytesTransferred;
            NTSTATUS Status;
        } ReadWrite;
    };
    KEVENT ThreadDoneEvent;
    KEVENT StartWorkEvent;
    KEVENT WorkCompleteEvent;
    PKTHREAD Thread;
} THREAD_CONTEXT, *PTHREAD_CONTEXT;

VOID
StartWorkerThread(
    OUT PTHREAD_CONTEXT Context);

VOID
FinishWorkerThread(
    IN PTHREAD_CONTEXT Context);

BOOLEAN
WaitForWork(
    IN PTHREAD_CONTEXT Context,
    IN ULONG MilliSeconds);

BOOLEAN
TriggerWork(
    IN PTHREAD_CONTEXT Context,
    IN ULONG MilliSeconds);

#endif /* !defined _KMTEST_NPFS_H_ */
