/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite NPFS Connect test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>
#include "npfs.h"

#define MAX_INSTANCES   5
#define IN_QUOTA        4096
#define OUT_QUOTA       4096

#define CheckServer(ServerHandle, State)                        \
    NpCheckServerPipe(ServerHandle,                             \
                      BYTE_STREAM, QUEUE, BYTE_STREAM, DUPLEX,  \
                      MAX_INSTANCES, 1,                         \
                      IN_QUOTA, 0,                              \
                      OUT_QUOTA, OUT_QUOTA,                     \
                      State)

#define CheckClient(ClientHandle, State)                        \
    NpCheckClientPipe(ClientHandle,                             \
                      BYTE_STREAM, QUEUE, BYTE_STREAM, DUPLEX,  \
                      MAX_INSTANCES, 1,                         \
                      IN_QUOTA, 0,                              \
                      OUT_QUOTA, OUT_QUOTA,                     \
                      State)

static
VOID
ConnectPipe(
    IN OUT PTHREAD_CONTEXT Context)
{
    HANDLE ClientHandle;

    ClientHandle = NULL;
    Context->Connect.Status = NpOpenPipe(&ClientHandle,
                                         Context->Connect.PipePath,
                                         FILE_PIPE_FULL_DUPLEX);
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
BOOLEAN
CheckConnectPipe(
    IN PTHREAD_CONTEXT Context,
    IN PCWSTR PipePath,
    IN ULONG MilliSeconds)
{
    Context->Work = ConnectPipe;
    Context->Connect.PipePath = PipePath;
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
VOID
TestConnect(
    IN HANDLE ServerHandle,
    IN PCWSTR PipePath)
{
    NTSTATUS Status;
    THREAD_CONTEXT ConnectContext;
    THREAD_CONTEXT ListenContext;
    BOOLEAN Okay;
    HANDLE ClientHandle;

    StartWorkerThread(&ConnectContext);
    StartWorkerThread(&ListenContext);

    /* Server should start out listening */
    CheckServer(ServerHandle, FILE_PIPE_LISTENING_STATE);

    /* Connect a client */
    ClientHandle = NULL;
    Okay = CheckConnectPipe(&ConnectContext, PipePath, 100);
    ok_bool_true(Okay, "CheckConnectPipe returned");
    ok_eq_hex(ConnectContext.Connect.Status, STATUS_SUCCESS);
    if (NT_SUCCESS(ConnectContext.Connect.Status))
    {
        ClientHandle = ConnectContext.Connect.ClientHandle;
        CheckClient(ClientHandle, FILE_PIPE_CONNECTED_STATE);
    }
    CheckServer(ServerHandle, FILE_PIPE_CONNECTED_STATE);

    /* Connect another client */
    Okay = CheckConnectPipe(&ConnectContext, PipePath, 100);
    ok_bool_true(Okay, "CheckConnectPipe returned");
    ok_eq_hex(ConnectContext.Connect.Status, STATUS_PIPE_NOT_AVAILABLE);
    if (NT_SUCCESS(ConnectContext.Connect.Status))
        ObCloseHandle(ConnectContext.Connect.ClientHandle, KernelMode);
    CheckClient(ClientHandle, FILE_PIPE_CONNECTED_STATE);
    CheckServer(ServerHandle, FILE_PIPE_CONNECTED_STATE);

    /* Disconnecting the client should fail */
    Status = NpDisconnectPipe(ClientHandle);
    ok_eq_hex(Status, STATUS_ILLEGAL_FUNCTION);
    CheckClient(ClientHandle, FILE_PIPE_CONNECTED_STATE);
    CheckServer(ServerHandle, FILE_PIPE_CONNECTED_STATE);

    /* Listening on the client should fail */
    Status = NpListenPipe(ClientHandle);
    ok_eq_hex(Status, STATUS_ILLEGAL_FUNCTION);
    CheckClient(ClientHandle, FILE_PIPE_CONNECTED_STATE);
    CheckServer(ServerHandle, FILE_PIPE_CONNECTED_STATE);

    /* Close client */
    if (ClientHandle)
        ObCloseHandle(ClientHandle, KernelMode);
    CheckServer(ServerHandle, FILE_PIPE_CLOSING_STATE);

    /* Connecting a client now should fail */
    Okay = CheckConnectPipe(&ConnectContext, PipePath, 100);
    ok_bool_true(Okay, "CheckConnectPipe returned");
    ok_eq_hex(ConnectContext.Connect.Status, STATUS_PIPE_NOT_AVAILABLE);
    if (NT_SUCCESS(ConnectContext.Connect.Status))
        ObCloseHandle(ConnectContext.Connect.ClientHandle, KernelMode);
    CheckServer(ServerHandle, FILE_PIPE_CLOSING_STATE);

    /* Listening should fail */
    Okay = CheckListenPipe(&ListenContext, ServerHandle, 100);
    ok_bool_true(Okay, "CheckListenPipe returned");
    if (!skip(Okay, "Listen succeeded unexpectedly\n"))
        CheckServer(ServerHandle, FILE_PIPE_CLOSING_STATE);

    /* Disconnect server */
    Status = NpDisconnectPipe(ServerHandle);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckServer(ServerHandle, FILE_PIPE_DISCONNECTED_STATE);

    /* Disconnecting again should fail */
    Status = NpDisconnectPipe(ServerHandle);
    ok_eq_hex(Status, STATUS_PIPE_DISCONNECTED);
    CheckServer(ServerHandle, FILE_PIPE_DISCONNECTED_STATE);

    /* Connecting a client now should fail */
    Okay = CheckConnectPipe(&ConnectContext, PipePath, 100);
    ok_bool_true(Okay, "CheckConnectPipe returned");
    ok_eq_hex(ConnectContext.Connect.Status, STATUS_PIPE_NOT_AVAILABLE);
    if (NT_SUCCESS(ConnectContext.Connect.Status))
        ObCloseHandle(ConnectContext.Connect.ClientHandle, KernelMode);
    CheckServer(ServerHandle, FILE_PIPE_DISCONNECTED_STATE);

    /**************************************************************************/
    /* Now listen again */
    Okay = CheckListenPipe(&ListenContext, ServerHandle, 100);
    ok_bool_false(Okay, "CheckListenPipe returned");
    //blocks: CheckServer(ServerHandle, FILE_PIPE_LISTENING_STATE);

    /* Connect client */
    ClientHandle = NULL;
    Okay = CheckConnectPipe(&ConnectContext, PipePath, 100);
    ok_bool_true(Okay, "CheckConnectPipe returned");
    ok_eq_hex(ConnectContext.Connect.Status, STATUS_SUCCESS);
    if (NT_SUCCESS(ConnectContext.Connect.Status))
    {
        ClientHandle = ConnectContext.Connect.ClientHandle;
        CheckClient(ClientHandle, FILE_PIPE_CONNECTED_STATE);
    }
    Okay = WaitForWork(&ListenContext, 100);
    ok_bool_true(Okay, "WaitForWork returned");
    ok_eq_hex(ListenContext.Listen.Status, STATUS_SUCCESS);
    CheckServer(ServerHandle, FILE_PIPE_CONNECTED_STATE);

    /* Listening again should fail */
    Okay = CheckListenPipe(&ListenContext, ServerHandle, 100);
    ok_bool_true(Okay, "CheckListenPipe returned");
    ok_eq_hex(ListenContext.Listen.Status, STATUS_PIPE_CONNECTED);
    CheckClient(ClientHandle, FILE_PIPE_CONNECTED_STATE);
    CheckServer(ServerHandle, FILE_PIPE_CONNECTED_STATE);

    /* Disconnect server */
    Status = NpDisconnectPipe(ServerHandle);
    ok_eq_hex(Status, STATUS_SUCCESS);
    NpQueryPipe(ClientHandle, STATUS_PIPE_DISCONNECTED);
    CheckServer(ServerHandle, FILE_PIPE_DISCONNECTED_STATE);

    /* Close client */
    if (ClientHandle)
        ObCloseHandle(ClientHandle, KernelMode);
    CheckServer(ServerHandle, FILE_PIPE_DISCONNECTED_STATE);

    /**************************************************************************/
    /* Listen once more */
    Okay = CheckListenPipe(&ListenContext, ServerHandle, 100);
    ok_bool_false(Okay, "CheckListenPipe returned");
    //blocks: CheckServer(ServerHandle, FILE_PIPE_LISTENING_STATE);

    /* Connect client */
    ClientHandle = NULL;
    Okay = CheckConnectPipe(&ConnectContext, PipePath, 100);
    ok_bool_true(Okay, "CheckConnectPipe returned");
    ok_eq_hex(ConnectContext.Connect.Status, STATUS_SUCCESS);
    if (NT_SUCCESS(ConnectContext.Connect.Status))
    {
        ClientHandle = ConnectContext.Connect.ClientHandle;
        CheckClient(ClientHandle, FILE_PIPE_CONNECTED_STATE);
    }
    Okay = WaitForWork(&ListenContext, 100);
    ok_bool_true(Okay, "WaitForWork returned");
    ok_eq_hex(ListenContext.Listen.Status, STATUS_SUCCESS);
    CheckServer(ServerHandle, FILE_PIPE_CONNECTED_STATE);

    /* Close server */
    Status = ObCloseHandle(ServerHandle, KernelMode);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckClient(ClientHandle, FILE_PIPE_CLOSING_STATE);

    /* Close client */
    if (ClientHandle)
        ObCloseHandle(ClientHandle, KernelMode);

    FinishWorkerThread(&ListenContext);
    FinishWorkerThread(&ConnectContext);
}

static KSTART_ROUTINE RunTest;
static
VOID
NTAPI
RunTest(
    IN PVOID Context)
{
    NTSTATUS Status;
    HANDLE ServerHandle;

    UNREFERENCED_PARAMETER(Context);

    ServerHandle = INVALID_HANDLE_VALUE;
    Status = NpCreatePipe(&ServerHandle,
                          DEVICE_NAMED_PIPE L"\\KmtestNpfsConnectTestPipe",
                          BYTE_STREAM, QUEUE, BYTE_STREAM, DUPLEX,
                          MAX_INSTANCES,
                          IN_QUOTA,
                          OUT_QUOTA);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok(ServerHandle != NULL && ServerHandle != INVALID_HANDLE_VALUE, "ServerHandle = %p\n", ServerHandle);
    if (!skip(NT_SUCCESS(Status) && ServerHandle != NULL && ServerHandle != INVALID_HANDLE_VALUE, "No pipe\n"))
    {
        CheckServer(ServerHandle, FILE_PIPE_LISTENING_STATE);
        TestConnect(ServerHandle, DEVICE_NAMED_PIPE L"\\KmtestNpfsConnectTestPipe");
    }
}

START_TEST(NpfsConnect)
{
    PKTHREAD Thread;

    Thread = KmtStartThread(RunTest, NULL);
    KmtFinishThread(Thread, NULL);
}
