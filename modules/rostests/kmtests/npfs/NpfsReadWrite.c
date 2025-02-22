/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite NPFS Read/Write test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>
#include "npfs.h"

typedef struct _READ_WRITE_TEST_CONTEXT
{
    PCWSTR PipePath;
    BOOLEAN ServerSynchronous;
    BOOLEAN ClientSynchronous;
} READ_WRITE_TEST_CONTEXT, *PREAD_WRITE_TEST_CONTEXT;

#define MAX_INSTANCES   5
#define IN_QUOTA        4096
#define OUT_QUOTA       4096

#define MakeServer(ServerHandle, PipePath, ServerSynchronous)           \
    NpCreatePipeEx(ServerHandle,                                        \
                   PipePath,                                            \
                   BYTE_STREAM,                                         \
                   QUEUE,                                               \
                   BYTE_STREAM,                                         \
                   FILE_SHARE_READ | FILE_SHARE_WRITE,                  \
                   MAX_INSTANCES,                                       \
                   IN_QUOTA,                                            \
                   OUT_QUOTA,                                           \
                   SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,          \
                   FILE_OPEN_IF,                                        \
                   (ServerSynchronous) ? FILE_SYNCHRONOUS_IO_NONALERT   \
                                       : 0,                             \
                   &DefaultTimeout)

#define CheckServer(ServerHandle, State)                                \
    NpCheckServerPipe(ServerHandle,                                     \
                      BYTE_STREAM, QUEUE, BYTE_STREAM, DUPLEX,          \
                      MAX_INSTANCES, 1,                                 \
                      IN_QUOTA, 0,                                      \
                      OUT_QUOTA, OUT_QUOTA,                             \
                      State)

#define CheckClient(ClientHandle, State)                                \
    NpCheckClientPipe(ClientHandle,                                     \
                      BYTE_STREAM, QUEUE, BYTE_STREAM, DUPLEX,          \
                      MAX_INSTANCES, 1,                                 \
                      IN_QUOTA, 0,                                      \
                      OUT_QUOTA, OUT_QUOTA,                             \
                      State)

#define CheckServerQuota(ServerHandle, InQ, OutQ)                       \
    NpCheckServerPipe(ServerHandle,                                     \
                      BYTE_STREAM, QUEUE, BYTE_STREAM, DUPLEX,          \
                      MAX_INSTANCES, 1,                                 \
                      IN_QUOTA, InQ,                                    \
                      OUT_QUOTA, OUT_QUOTA - (OutQ),                    \
                      FILE_PIPE_CONNECTED_STATE)

#define CheckClientQuota(ClientHandle, InQ, OutQ)                       \
    NpCheckClientPipe(ClientHandle,                                     \
                      BYTE_STREAM, QUEUE, BYTE_STREAM, DUPLEX,          \
                      MAX_INSTANCES, 1,                                 \
                      IN_QUOTA, InQ,                                    \
                      OUT_QUOTA, OUT_QUOTA - (OutQ),                    \
                      FILE_PIPE_CONNECTED_STATE)

#define CheckPipeContext(Context, ExpectedStatus, ExpectedBytes) do             \
{                                                                           \
    ok_bool_true(Okay, "CheckPipeContext");                                     \
    ok_eq_hex((Context)->ReadWrite.Status, ExpectedStatus);                 \
    ok_eq_ulongptr((Context)->ReadWrite.BytesTransferred, ExpectedBytes);   \
} while (0)

static
VOID
ConnectPipe(
    IN OUT PTHREAD_CONTEXT Context)
{
    HANDLE ClientHandle;

    ClientHandle = NULL;
    Context->Connect.Status = NpOpenPipeEx(&ClientHandle,
                                           Context->Connect.PipePath,
                                           SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
                                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                                           FILE_OPEN,
                                           Context->Connect.ClientSynchronous ? FILE_SYNCHRONOUS_IO_NONALERT
                                                                              : 0);
    Context->Connect.ClientHandle = ClientHandle;
}

static
VOID
ListenPipe(
    IN OUT PTHREAD_CONTEXT Context)
{
    Context->Listen.Status = NpListenPipe(Context->Listen.ServerHandle);
}

static
VOID
ReadPipe(
    IN OUT PTHREAD_CONTEXT Context)
{
    Context->ReadWrite.Status = NpReadPipe(Context->ReadWrite.PipeHandle,
                                           Context->ReadWrite.Buffer,
                                           Context->ReadWrite.BufferSize,
                                           (PULONG_PTR)&Context->ReadWrite.BytesTransferred);
}

static
VOID
WritePipe(
    IN OUT PTHREAD_CONTEXT Context)
{
    Context->ReadWrite.Status = NpWritePipe(Context->ReadWrite.PipeHandle,
                                            Context->ReadWrite.Buffer,
                                            Context->ReadWrite.BufferSize,
                                            (PULONG_PTR)&Context->ReadWrite.BytesTransferred);
}

static
BOOLEAN
CheckConnectPipe(
    IN PTHREAD_CONTEXT Context,
    IN PCWSTR PipePath,
    IN BOOLEAN ClientSynchronous,
    IN ULONG MilliSeconds)
{
    Context->Work = ConnectPipe;
    Context->Connect.PipePath = PipePath;
    Context->Connect.ClientSynchronous = ClientSynchronous;
    return TriggerWork(Context, MilliSeconds);
}

static
BOOLEAN
CheckListenPipe(
    IN PTHREAD_CONTEXT Context,
    IN HANDLE ServerHandle,
    IN ULONG MilliSeconds)
{
    Context->Work = ListenPipe;
    Context->Listen.ServerHandle = ServerHandle;
    return TriggerWork(Context, MilliSeconds);
}

static
BOOLEAN
CheckReadPipe(
    IN PTHREAD_CONTEXT Context,
    IN HANDLE PipeHandle,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    IN ULONG MilliSeconds)
{
    Context->Work = ReadPipe;
    Context->ReadWrite.PipeHandle = PipeHandle;
    Context->ReadWrite.Buffer = Buffer;
    Context->ReadWrite.BufferSize = BufferSize;
    return TriggerWork(Context, MilliSeconds);
}

static
BOOLEAN
CheckWritePipe(
    IN PTHREAD_CONTEXT Context,
    IN HANDLE PipeHandle,
    IN const VOID *Buffer,
    IN ULONG BufferSize,
    IN ULONG MilliSeconds)
{
    Context->Work = WritePipe;
    Context->ReadWrite.PipeHandle = PipeHandle;
    Context->ReadWrite.Buffer = (PVOID)Buffer;
    Context->ReadWrite.BufferSize = BufferSize;
    return TriggerWork(Context, MilliSeconds);
}

static KSTART_ROUTINE TestReadWrite;
static
VOID
NTAPI
TestReadWrite(
    IN PVOID Context)
{
    PREAD_WRITE_TEST_CONTEXT TestContext = Context;
    PCWSTR PipePath = TestContext->PipePath;
    BOOLEAN ServerSynchronous = TestContext->ServerSynchronous;
    BOOLEAN ClientSynchronous = TestContext->ClientSynchronous;
    NTSTATUS Status;
    HANDLE ServerHandle;
    LARGE_INTEGER DefaultTimeout;
    THREAD_CONTEXT ConnectContext;
    THREAD_CONTEXT ListenContext;
    THREAD_CONTEXT ClientReadContext;
    THREAD_CONTEXT ClientWriteContext;
    THREAD_CONTEXT ServerReadContext;
    THREAD_CONTEXT ServerWriteContext;
    BOOLEAN Okay;
    HANDLE ClientHandle;
    UCHAR ReadBuffer[128];
    UCHAR WriteBuffer[128];

    StartWorkerThread(&ConnectContext);
    StartWorkerThread(&ListenContext);
    StartWorkerThread(&ClientReadContext);
    StartWorkerThread(&ClientWriteContext);
    StartWorkerThread(&ServerReadContext);
    StartWorkerThread(&ServerWriteContext);

    DefaultTimeout.QuadPart = -50 * 1000 * 10;

    /* Server should start out listening */
    Status = MakeServer(&ServerHandle, PipePath, ServerSynchronous);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckServer(ServerHandle, FILE_PIPE_LISTENING_STATE);

    Okay = CheckWritePipe(&ServerWriteContext, ServerHandle, NULL, 0, 100);
    ok_bool_true(Okay, "CheckWritePipe returned");
    ok_eq_ulongptr(ServerWriteContext.ReadWrite.BytesTransferred, 0);
    ok_eq_hex(ServerWriteContext.ReadWrite.Status, STATUS_PIPE_LISTENING);

    Okay = CheckReadPipe(&ServerReadContext, ServerHandle, NULL, 0, 100);
    ok_bool_true(Okay, "CheckReadPipe returned");
    ok_eq_ulongptr(ServerReadContext.ReadWrite.BytesTransferred, 0);
    ok_eq_hex(ServerReadContext.ReadWrite.Status, STATUS_PIPE_LISTENING);

    /* Connect a client */
    Okay = CheckConnectPipe(&ConnectContext, PipePath, ClientSynchronous, 100);
    ok_bool_true(Okay, "CheckConnectPipe returned");
    ok_eq_hex(ConnectContext.Connect.Status, STATUS_SUCCESS);
    ClientHandle = ConnectContext.Connect.ClientHandle;
    CheckClient(ClientHandle, FILE_PIPE_CONNECTED_STATE);
    CheckServer(ServerHandle, FILE_PIPE_CONNECTED_STATE);

    /** Server to client, write first, 1 byte */
    WriteBuffer[0] = 'A';
    ReadBuffer[0] = 'X';
    Okay = CheckWritePipe(&ServerWriteContext, ServerHandle, WriteBuffer, 1, 100);
    CheckPipeContext(&ServerWriteContext, STATUS_SUCCESS, 1);
    CheckServerQuota(ServerHandle, 0, 1); CheckClientQuota(ClientHandle, 1, 0);
    Okay = CheckReadPipe(&ClientReadContext, ClientHandle, ReadBuffer, 1, 100);
    CheckPipeContext(&ClientReadContext, STATUS_SUCCESS, 1);
    ok_eq_uint(ReadBuffer[0], 'A');
    CheckServerQuota(ServerHandle, 0, 0); CheckClientQuota(ClientHandle, 0, 0);

    /** Server to client, read first, 1 byte */
    WriteBuffer[0] = 'B';
    ReadBuffer[0] = 'X';
    Okay = CheckReadPipe(&ClientReadContext, ClientHandle, ReadBuffer, 1, 100);
    ok_bool_false(Okay, "CheckReadPipe returned");
    CheckServerQuota(ServerHandle, 0, 1);
    Okay = CheckWritePipe(&ServerWriteContext, ServerHandle, WriteBuffer, 1, 100);
    CheckPipeContext(&ServerWriteContext, STATUS_SUCCESS, 1);
    Okay = WaitForWork(&ClientReadContext, 100);
    CheckPipeContext(&ClientReadContext, STATUS_SUCCESS, 1);
    ok_eq_uint(ReadBuffer[0], 'B');
    CheckServerQuota(ServerHandle, 0, 0); CheckClientQuota(ClientHandle, 0, 0);

    /** Client to server, write first, 1 byte */
    WriteBuffer[0] = 'C';
    ReadBuffer[0] = 'X';
    Okay = CheckWritePipe(&ClientWriteContext, ClientHandle, WriteBuffer, 1, 100);
    CheckPipeContext(&ClientWriteContext, STATUS_SUCCESS, 1);
    CheckClientQuota(ClientHandle, 0, 1); CheckServerQuota(ServerHandle, 1, 0);
    Okay = CheckReadPipe(&ServerReadContext, ServerHandle, ReadBuffer, 1, 100);
    CheckPipeContext(&ServerReadContext, STATUS_SUCCESS, 1);
    ok_eq_uint(ReadBuffer[0], 'C');
    CheckClientQuota(ClientHandle, 0, 0); CheckServerQuota(ServerHandle, 0, 0);

    /** Client to server, read first, 1 byte */
    WriteBuffer[0] = 'D';
    ReadBuffer[0] = 'X';
    Okay = CheckReadPipe(&ServerReadContext, ServerHandle, ReadBuffer, 1, 100);
    ok_bool_false(Okay, "CheckReadPipe returned");
    CheckClientQuota(ClientHandle, 0, 1);
    Okay = CheckWritePipe(&ClientWriteContext, ClientHandle, WriteBuffer, 1, 100);
    CheckPipeContext(&ClientWriteContext, STATUS_SUCCESS, 1);
    Okay = WaitForWork(&ServerReadContext, 100);
    CheckPipeContext(&ServerReadContext, STATUS_SUCCESS, 1);
    ok_eq_uint(ReadBuffer[0], 'D');
    CheckClientQuota(ClientHandle, 0, 0); CheckServerQuota(ServerHandle, 0, 0);

    /** Server to client, write 0 bytes */
    Okay = CheckWritePipe(&ServerWriteContext, ServerHandle, (PVOID)1, 0, 100);
    CheckPipeContext(&ServerWriteContext, STATUS_SUCCESS, 0);
    CheckServerQuota(ServerHandle, 0, 0); CheckClientQuota(ClientHandle, 0, 0);

    /** Client to Server, write 0 bytes */
    Okay = CheckWritePipe(&ClientWriteContext, ClientHandle, (PVOID)1, 0, 100);
    CheckPipeContext(&ClientWriteContext, STATUS_SUCCESS, 0);
    CheckClientQuota(ClientHandle, 0, 0); CheckServerQuota(ServerHandle, 0, 0);

    /** Server to client, read 0 bytes blocks, write 0 bytes does not unblock, write 1 byte unblocks */
    WriteBuffer[0] = 'E';
    ReadBuffer[0] = 'X';
    Okay = CheckReadPipe(&ClientReadContext, ClientHandle, (PVOID)1, 0, 100);
    ok_bool_false(Okay, "CheckReadPipe returned");
    CheckServerQuota(ServerHandle, 0, 0);
    Okay = CheckWritePipe(&ServerWriteContext, ServerHandle, (PVOID)1, 0, 100);
    CheckPipeContext(&ServerWriteContext, STATUS_SUCCESS, 0);
    Okay = WaitForWork(&ClientReadContext, 100);
    ok_bool_false(Okay, "WaitForWork returned");
    CheckServerQuota(ServerHandle, 0, 0);
    Okay = CheckWritePipe(&ServerWriteContext, ServerHandle, WriteBuffer, 1, 100);
    CheckPipeContext(&ServerWriteContext, STATUS_SUCCESS, 1);
    Okay = WaitForWork(&ClientReadContext, 100);
    CheckPipeContext(&ClientReadContext, STATUS_SUCCESS, 0);
    ok_eq_uint(ReadBuffer[0], 'X');
    CheckServerQuota(ServerHandle, 0, 1); CheckClientQuota(ClientHandle, 1, 0);
    Okay = CheckReadPipe(&ClientReadContext, ClientHandle, ReadBuffer, 1, 100);
    CheckPipeContext(&ClientReadContext, STATUS_SUCCESS, 1);
    ok_eq_uint(ReadBuffer[0], 'E');
    CheckServerQuota(ServerHandle, 0, 0); CheckClientQuota(ClientHandle, 0, 0);

    /** Client to server, read 0 bytes blocks, write 0 bytes does not unblock, write 1 byte unblocks */
    WriteBuffer[0] = 'F';
    ReadBuffer[0] = 'X';
    Okay = CheckReadPipe(&ServerReadContext, ServerHandle, (PVOID)1, 0, 100);
    ok_bool_false(Okay, "CheckReadPipe returned");
    CheckClientQuota(ClientHandle, 0, 0);
    Okay = CheckWritePipe(&ClientWriteContext, ClientHandle, (PVOID)1, 0, 100);
    CheckPipeContext(&ClientWriteContext, STATUS_SUCCESS, 0);
    Okay = WaitForWork(&ServerReadContext, 100);
    ok_bool_false(Okay, "WaitForWork returned");
    CheckClientQuota(ClientHandle, 0, 0);
    Okay = CheckWritePipe(&ClientWriteContext, ClientHandle, WriteBuffer, 1, 100);
    CheckPipeContext(&ClientWriteContext, STATUS_SUCCESS, 1);
    Okay = WaitForWork(&ServerReadContext, 100);
    CheckPipeContext(&ServerReadContext, STATUS_SUCCESS, 0);
    ok_eq_uint(ReadBuffer[0], 'X');
    CheckClientQuota(ClientHandle, 0, 1); CheckServerQuota(ServerHandle, 1, 0);
    Okay = CheckReadPipe(&ServerReadContext, ServerHandle, ReadBuffer, 1, 100);
    CheckPipeContext(&ServerReadContext, STATUS_SUCCESS, 1);
    ok_eq_uint(ReadBuffer[0], 'F');
    CheckClientQuota(ClientHandle, 0, 0); CheckServerQuota(ServerHandle, 0, 0);

    /** Disconnect server with pending read on client */
    WriteBuffer[0] = 'G';
    ReadBuffer[0] = 'X';
    Okay = CheckReadPipe(&ClientReadContext, ClientHandle, ReadBuffer, 1, 100);
    ok_bool_false(Okay, "CheckReadPipe returned");
    CheckServerQuota(ServerHandle, 0, 1);
    Status = NpDisconnectPipe(ServerHandle);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Okay = WaitForWork(&ClientReadContext, 100);
    CheckPipeContext(&ClientReadContext, STATUS_PIPE_DISCONNECTED, 0);
    ok_eq_uint(ReadBuffer[0], 'X');

    /* Read from server when disconnected */
    Okay = CheckReadPipe(&ServerReadContext, ServerHandle, ReadBuffer, 1, 100);
    CheckPipeContext(&ServerReadContext, STATUS_PIPE_DISCONNECTED, 0);

    /* Write to server when disconnected */
    Okay = CheckWritePipe(&ServerWriteContext, ServerHandle, WriteBuffer, 1, 100);
    CheckPipeContext(&ServerWriteContext, STATUS_PIPE_DISCONNECTED, 0);

    /* Read from client when disconnected */
    Okay = CheckReadPipe(&ClientReadContext, ClientHandle, ReadBuffer, 1, 100);
    CheckPipeContext(&ClientReadContext, STATUS_PIPE_DISCONNECTED, 0);

    /* Write to client when disconnected */
    Okay = CheckWritePipe(&ClientWriteContext, ClientHandle, WriteBuffer, 1, 100);
    CheckPipeContext(&ClientWriteContext, STATUS_PIPE_DISCONNECTED, 0);
    Status = ObCloseHandle(ClientHandle, KernelMode);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Restore the connection */
    Okay = CheckListenPipe(&ListenContext, ServerHandle, 100);
    ok_bool_false(Okay, "CheckListenPipe returned");
    Okay = CheckConnectPipe(&ConnectContext, PipePath, ClientSynchronous, 100);
    ok_bool_true(Okay, "CheckConnectPipe returned");
    ok_eq_hex(ConnectContext.Connect.Status, STATUS_SUCCESS);
    Okay = WaitForWork(&ListenContext, 100);
    ok_bool_true(Okay, "WaitForWork returned");
    ok_eq_hex(ListenContext.Listen.Status, STATUS_SUCCESS);
    ClientHandle = ConnectContext.Connect.ClientHandle;
    CheckClient(ClientHandle, FILE_PIPE_CONNECTED_STATE);
    CheckServer(ServerHandle, FILE_PIPE_CONNECTED_STATE);

    /** Close server with pending read on client */
    WriteBuffer[0] = 'H';
    ReadBuffer[0] = 'X';
    Okay = CheckReadPipe(&ClientReadContext, ClientHandle, ReadBuffer, 1, 100);
    ok_bool_false(Okay, "CheckReadPipe returned");
    Status = ObCloseHandle(ServerHandle, KernelMode);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Okay = WaitForWork(&ClientReadContext, 100);
    CheckPipeContext(&ClientReadContext, STATUS_PIPE_BROKEN, 0);
    ok_eq_uint(ReadBuffer[0], 'X');

    /* Read from client when closed */
    Okay = CheckReadPipe(&ClientReadContext, ClientHandle, ReadBuffer, 1, 100);
    CheckPipeContext(&ClientReadContext, STATUS_PIPE_BROKEN, 0);

    /* Write to client when closed */
    Okay = CheckWritePipe(&ClientWriteContext, ClientHandle, WriteBuffer, 1, 100);
    CheckPipeContext(&ClientWriteContext, STATUS_PIPE_CLOSING, 0);
    Status = ObCloseHandle(ClientHandle, KernelMode);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Restore the connection */
    Status = MakeServer(&ServerHandle, PipePath, ServerSynchronous);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Okay = CheckConnectPipe(&ConnectContext, PipePath, ClientSynchronous, 100);
    ok_bool_true(Okay, "CheckConnectPipe returned");
    ok_eq_hex(ConnectContext.Connect.Status, STATUS_SUCCESS);
    ClientHandle = ConnectContext.Connect.ClientHandle;
    CheckClient(ClientHandle, FILE_PIPE_CONNECTED_STATE);
    CheckServer(ServerHandle, FILE_PIPE_CONNECTED_STATE);

    /** Close client with pending read on server */
    WriteBuffer[0] = 'I';
    ReadBuffer[0] = 'X';
    Okay = CheckReadPipe(&ServerReadContext, ServerHandle, ReadBuffer, 1, 100);
    ok_bool_false(Okay, "CheckReadPipe returned");
    Status = ObCloseHandle(ClientHandle, KernelMode);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Okay = WaitForWork(&ServerReadContext, 100);
    CheckPipeContext(&ServerReadContext, STATUS_PIPE_BROKEN, 0);
    ok_eq_uint(ReadBuffer[0], 'X');

    /* Read from server when closed */
    Okay = CheckReadPipe(&ServerReadContext, ServerHandle, ReadBuffer, 1, 100);
    CheckPipeContext(&ServerReadContext, STATUS_PIPE_BROKEN, 0);

    /* Write to server when closed */
    Okay = CheckWritePipe(&ServerWriteContext, ServerHandle, WriteBuffer, 1, 100);
    CheckPipeContext(&ServerWriteContext, STATUS_PIPE_CLOSING, 0);
    Status = ObCloseHandle(ServerHandle, KernelMode);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Restore the connection */
    Status = MakeServer(&ServerHandle, PipePath, ServerSynchronous);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Okay = CheckConnectPipe(&ConnectContext, PipePath, ClientSynchronous, 100);
    ok_bool_true(Okay, "CheckConnectPipe returned");
    ok_eq_hex(ConnectContext.Connect.Status, STATUS_SUCCESS);
    ClientHandle = ConnectContext.Connect.ClientHandle;
    CheckClient(ClientHandle, FILE_PIPE_CONNECTED_STATE);
    CheckServer(ServerHandle, FILE_PIPE_CONNECTED_STATE);

    /** Write to server and disconnect, then read from client */
    WriteBuffer[0] = 'J';
    ReadBuffer[0] = 'X';
    Okay = CheckWritePipe(&ServerWriteContext, ServerHandle, WriteBuffer, 1, 100);
    CheckPipeContext(&ServerWriteContext, STATUS_SUCCESS, 1);
    CheckServerQuota(ServerHandle, 0, 1); CheckClientQuota(ClientHandle, 1, 0);
    Status = NpDisconnectPipe(ServerHandle);
    ok_eq_hex(Status, STATUS_SUCCESS);
    NpQueryPipe(ClientHandle, STATUS_PIPE_DISCONNECTED);
    CheckServer(ServerHandle, FILE_PIPE_DISCONNECTED_STATE);
    Okay = CheckReadPipe(&ClientReadContext, ClientHandle, ReadBuffer, 1, 100);
    CheckPipeContext(&ClientReadContext, STATUS_PIPE_DISCONNECTED, 0);
    ok_eq_uint(ReadBuffer[0], 'X');
    Okay = CheckReadPipe(&ClientReadContext, ClientHandle, ReadBuffer, 1, 100);
    CheckPipeContext(&ClientReadContext, STATUS_PIPE_DISCONNECTED, 0);
    Status = ObCloseHandle(ClientHandle, KernelMode);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Restore the connection */
    Okay = CheckListenPipe(&ListenContext, ServerHandle, 100);
    ok_bool_false(Okay, "CheckListenPipe returned");
    Okay = CheckConnectPipe(&ConnectContext, PipePath, ClientSynchronous, 100);
    ok_bool_true(Okay, "CheckConnectPipe returned");
    ok_eq_hex(ConnectContext.Connect.Status, STATUS_SUCCESS);
    Okay = WaitForWork(&ListenContext, 100);
    ok_bool_true(Okay, "WaitForWork returned");
    ok_eq_hex(ListenContext.Listen.Status, STATUS_SUCCESS);
    ClientHandle = ConnectContext.Connect.ClientHandle;
    CheckClient(ClientHandle, FILE_PIPE_CONNECTED_STATE);
    CheckServer(ServerHandle, FILE_PIPE_CONNECTED_STATE);

    /** Write to server and close, then read from client */
    WriteBuffer[0] = 'K';
    ReadBuffer[0] = 'X';
    Okay = CheckWritePipe(&ServerWriteContext, ServerHandle, WriteBuffer, 1, 100);
    CheckPipeContext(&ServerWriteContext, STATUS_SUCCESS, 1);
    CheckServerQuota(ServerHandle, 0, 1); CheckClientQuota(ClientHandle, 1, 0);
    Status = ObCloseHandle(ServerHandle, KernelMode);
    ok_eq_hex(Status, STATUS_SUCCESS);
    NpCheckClientPipe(ClientHandle,
                      BYTE_STREAM, QUEUE, BYTE_STREAM, DUPLEX,
                      MAX_INSTANCES, 1,
                      IN_QUOTA, 1,
                      OUT_QUOTA, OUT_QUOTA,
                      FILE_PIPE_CLOSING_STATE);
    Okay = CheckReadPipe(&ClientReadContext, ClientHandle, ReadBuffer, 1, 100);
    CheckPipeContext(&ClientReadContext, STATUS_SUCCESS, 1);
    ok_eq_uint(ReadBuffer[0], 'K');
    Okay = CheckReadPipe(&ClientReadContext, ClientHandle, ReadBuffer, 1, 100);
    CheckPipeContext(&ClientReadContext, STATUS_PIPE_BROKEN, 0);
    Status = ObCloseHandle(ClientHandle, KernelMode);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Restore the connection */
    Status = MakeServer(&ServerHandle, PipePath, ServerSynchronous);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Okay = CheckConnectPipe(&ConnectContext, PipePath, ClientSynchronous, 100);
    ok_bool_true(Okay, "CheckConnectPipe returned");
    ok_eq_hex(ConnectContext.Connect.Status, STATUS_SUCCESS);
    ClientHandle = ConnectContext.Connect.ClientHandle;
    CheckClient(ClientHandle, FILE_PIPE_CONNECTED_STATE);
    CheckServer(ServerHandle, FILE_PIPE_CONNECTED_STATE);


    /** Write to client and close, then read from server */
    WriteBuffer[0] = 'L';
    ReadBuffer[0] = 'X';
    Okay = CheckWritePipe(&ClientWriteContext, ClientHandle, WriteBuffer, 1, 100);
    CheckPipeContext(&ClientWriteContext, STATUS_SUCCESS, 1);
    CheckClientQuota(ClientHandle, 0, 1); CheckServerQuota(ServerHandle, 1, 0);
    Status = ObCloseHandle(ClientHandle, KernelMode);
    ok_eq_hex(Status, STATUS_SUCCESS);
    NpCheckServerPipe(ServerHandle,
                      BYTE_STREAM, QUEUE, BYTE_STREAM, DUPLEX,
                      MAX_INSTANCES, 1,
                      IN_QUOTA, 1,
                      OUT_QUOTA, OUT_QUOTA,
                      FILE_PIPE_CLOSING_STATE);
    Okay = CheckReadPipe(&ServerReadContext, ServerHandle, ReadBuffer, 1, 100);
    CheckPipeContext(&ServerReadContext, STATUS_SUCCESS, 1);
    ok_eq_uint(ReadBuffer[0], 'L');
    Okay = CheckReadPipe(&ServerReadContext, ServerHandle, ReadBuffer, 1, 100);
    CheckPipeContext(&ServerReadContext, STATUS_PIPE_BROKEN, 0);
    Status = ObCloseHandle(ServerHandle, KernelMode);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Restore the connection */
    Status = MakeServer(&ServerHandle, PipePath, ServerSynchronous);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Okay = CheckConnectPipe(&ConnectContext, PipePath, ClientSynchronous, 100);
    ok_bool_true(Okay, "CheckConnectPipe returned");
    ok_eq_hex(ConnectContext.Connect.Status, STATUS_SUCCESS);
    ClientHandle = ConnectContext.Connect.ClientHandle;
    CheckClient(ClientHandle, FILE_PIPE_CONNECTED_STATE);
    CheckServer(ServerHandle, FILE_PIPE_CONNECTED_STATE);

    /** Write to client and disconnect server, then read from server */
    WriteBuffer[0] = 'M';
    ReadBuffer[0] = 'X';
    Okay = CheckWritePipe(&ClientWriteContext, ClientHandle, WriteBuffer, 1, 100);
    CheckPipeContext(&ClientWriteContext, STATUS_SUCCESS, 1);
    CheckClientQuota(ClientHandle, 0, 1); CheckServerQuota(ServerHandle, 1, 0);
    Status = NpDisconnectPipe(ServerHandle);
    ok_eq_hex(Status, STATUS_SUCCESS);
    NpQueryPipe(ClientHandle, STATUS_PIPE_DISCONNECTED);
    CheckServer(ServerHandle, FILE_PIPE_DISCONNECTED_STATE);
    Okay = CheckReadPipe(&ServerReadContext, ServerHandle, ReadBuffer, 1, 100);
    CheckPipeContext(&ServerReadContext, STATUS_PIPE_DISCONNECTED, 0);
    ok_eq_uint(ReadBuffer[0], 'X');
    Okay = CheckReadPipe(&ServerReadContext, ServerHandle, ReadBuffer, 1, 100);
    CheckPipeContext(&ServerReadContext, STATUS_PIPE_DISCONNECTED, 0);
    Status = ObCloseHandle(ClientHandle, KernelMode);
    ok_eq_hex(Status, STATUS_SUCCESS);

    Status = ObCloseHandle(ServerHandle, KernelMode);
    ok_eq_hex(Status, STATUS_SUCCESS);

    FinishWorkerThread(&ServerWriteContext);
    FinishWorkerThread(&ServerReadContext);
    FinishWorkerThread(&ClientWriteContext);
    FinishWorkerThread(&ClientReadContext);
    FinishWorkerThread(&ListenContext);
    FinishWorkerThread(&ConnectContext);
}

START_TEST(NpfsReadWrite)
{
    PKTHREAD Thread;
    READ_WRITE_TEST_CONTEXT TestContext;

    TestContext.PipePath = DEVICE_NAMED_PIPE L"\\KmtestNpfsReadWriteTestPipe";

    TestContext.ServerSynchronous = TRUE;
    TestContext.ClientSynchronous = TRUE;
    Thread = KmtStartThread(TestReadWrite, &TestContext);
    KmtFinishThread(Thread, NULL);

    TestContext.ServerSynchronous = FALSE;
    TestContext.ClientSynchronous = TRUE;
    Thread = KmtStartThread(TestReadWrite, &TestContext);
    KmtFinishThread(Thread, NULL);

    TestContext.ServerSynchronous = TRUE;
    TestContext.ClientSynchronous = FALSE;
    Thread = KmtStartThread(TestReadWrite, &TestContext);
    KmtFinishThread(Thread, NULL);

    TestContext.ServerSynchronous = FALSE;
    TestContext.ClientSynchronous = FALSE;
    Thread = KmtStartThread(TestReadWrite, &TestContext);
    KmtFinishThread(Thread, NULL);
}
