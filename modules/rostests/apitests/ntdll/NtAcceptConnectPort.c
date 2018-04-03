/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for NtAcceptConnectPort
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

#include <process.h>

#define TEST_CONNECTION_INFO_SIGNATURE1 0xaabb0123
#define TEST_CONNECTION_INFO_SIGNATURE2 0xaabb0124
typedef struct _TEST_CONNECTION_INFO
{
    ULONG Signature;
} TEST_CONNECTION_INFO, *PTEST_CONNECTION_INFO;

#define TEST_MESSAGE_MESSAGE 0x4455cdef
typedef struct _TEST_MESSAGE
{
    PORT_MESSAGE Header;
    ULONG Message;
} TEST_MESSAGE, *PTEST_MESSAGE;

static UNICODE_STRING PortName = RTL_CONSTANT_STRING(L"\\NtdllApitestNtAcceptConnectPortTestPort");
static UINT ServerThreadId;
static UINT ClientThreadId;
static UCHAR Context;

UINT
CALLBACK
ServerThread(
    _Inout_ PVOID Parameter)
{
    NTSTATUS Status;
    TEST_MESSAGE Message;
    HANDLE PortHandle;
    HANDLE ServerPortHandle = Parameter;

    /* Listen, but refuse the connection */
    RtlZeroMemory(&Message, sizeof(Message));
    Status = NtListenPort(ServerPortHandle,
                          &Message.Header);
    ok_hex(Status, STATUS_SUCCESS);

    ok(Message.Header.u1.s1.TotalLength == RTL_SIZEOF_THROUGH_FIELD(TEST_MESSAGE, Message),
       "TotalLength = %u, expected %lu\n",
       Message.Header.u1.s1.TotalLength, RTL_SIZEOF_THROUGH_FIELD(TEST_MESSAGE, Message));
    ok(Message.Header.u1.s1.DataLength == sizeof(TEST_CONNECTION_INFO),
       "DataLength = %u\n", Message.Header.u1.s1.DataLength);
    ok(Message.Header.u2.s2.Type == LPC_CONNECTION_REQUEST,
       "Type = %x\n", Message.Header.u2.s2.Type);
    ok(Message.Header.ClientId.UniqueProcess == UlongToHandle(GetCurrentProcessId()),
       "UniqueProcess = %p, expected %lx\n",
       Message.Header.ClientId.UniqueProcess, GetCurrentProcessId());
    ok(Message.Header.ClientId.UniqueThread == UlongToHandle(ClientThreadId),
       "UniqueThread = %p, expected %x\n",
       Message.Header.ClientId.UniqueThread, ClientThreadId);
    ok(Message.Message == TEST_CONNECTION_INFO_SIGNATURE1, "Message = %lx\n", Message.Message);

    PortHandle = (PVOID)(ULONG_PTR)0x55555555;
    Status = NtAcceptConnectPort(&PortHandle,
                                 &Context,
                                 &Message.Header,
                                 FALSE,
                                 NULL,
                                 NULL);
    ok_hex(Status, STATUS_SUCCESS);
    ok(PortHandle == (PVOID)(ULONG_PTR)0x55555555, "PortHandle = %p\n", PortHandle);

    /* Listen a second time, then accept */
    RtlZeroMemory(&Message, sizeof(Message));
    Status = NtListenPort(ServerPortHandle,
                          &Message.Header);
    ok_hex(Status, STATUS_SUCCESS);

    ok(Message.Header.u1.s1.TotalLength == RTL_SIZEOF_THROUGH_FIELD(TEST_MESSAGE, Message),
       "TotalLength = %u, expected %lu\n",
       Message.Header.u1.s1.TotalLength, RTL_SIZEOF_THROUGH_FIELD(TEST_MESSAGE, Message));
    ok(Message.Header.u1.s1.DataLength == sizeof(TEST_CONNECTION_INFO),
       "DataLength = %u\n", Message.Header.u1.s1.DataLength);
    ok(Message.Header.u2.s2.Type == LPC_CONNECTION_REQUEST,
       "Type = %x\n", Message.Header.u2.s2.Type);
    ok(Message.Header.ClientId.UniqueProcess == UlongToHandle(GetCurrentProcessId()),
       "UniqueProcess = %p, expected %lx\n",
       Message.Header.ClientId.UniqueProcess, GetCurrentProcessId());
    ok(Message.Header.ClientId.UniqueThread == UlongToHandle(ClientThreadId),
       "UniqueThread = %p, expected %x\n",
       Message.Header.ClientId.UniqueThread, ClientThreadId);
    ok(Message.Message == TEST_CONNECTION_INFO_SIGNATURE2, "Message = %lx\n", Message.Message);

    Status = NtAcceptConnectPort(&PortHandle,
                                 &Context,
                                 &Message.Header,
                                 TRUE,
                                 NULL,
                                 NULL);
    ok_hex(Status, STATUS_SUCCESS);

    Status = NtCompleteConnectPort(PortHandle);
    ok_hex(Status, STATUS_SUCCESS);

    RtlZeroMemory(&Message, sizeof(Message));
    Status = NtReplyWaitReceivePort(PortHandle,
                                    NULL,
                                    NULL,
                                    &Message.Header);
    ok_hex(Status, STATUS_SUCCESS);

    ok(Message.Header.u1.s1.TotalLength == sizeof(Message),
       "TotalLength = %u, expected %Iu\n",
       Message.Header.u1.s1.TotalLength, sizeof(Message));
    ok(Message.Header.u1.s1.DataLength == sizeof(Message.Message),
       "DataLength = %u\n", Message.Header.u1.s1.DataLength);
    ok(Message.Header.u2.s2.Type == LPC_DATAGRAM,
       "Type = %x\n", Message.Header.u2.s2.Type);
    ok(Message.Header.ClientId.UniqueProcess == UlongToHandle(GetCurrentProcessId()),
       "UniqueProcess = %p, expected %lx\n",
       Message.Header.ClientId.UniqueProcess, GetCurrentProcessId());
    ok(Message.Header.ClientId.UniqueThread == UlongToHandle(ClientThreadId),
       "UniqueThread = %p, expected %x\n",
       Message.Header.ClientId.UniqueThread, ClientThreadId);
    ok(Message.Message == TEST_MESSAGE_MESSAGE, "Message = %lx\n", Message.Message);

    Status = NtClose(PortHandle);
    ok_hex(Status, STATUS_SUCCESS);

    return 0;
}

UINT
CALLBACK
ClientThread(
    _Inout_ PVOID Parameter)
{
    NTSTATUS Status;
    HANDLE PortHandle;
    TEST_CONNECTION_INFO ConnectInfo;
    ULONG ConnectInfoLength;
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    TEST_MESSAGE Message;

    SecurityQos.Length = sizeof(SecurityQos);
    SecurityQos.ImpersonationLevel = SecurityIdentification;
    SecurityQos.EffectiveOnly = TRUE;
    SecurityQos.ContextTrackingMode = SECURITY_STATIC_TRACKING;

    /* Attempt to connect -- will be rejected */
    ConnectInfo.Signature = TEST_CONNECTION_INFO_SIGNATURE1;
    ConnectInfoLength = sizeof(ConnectInfo);
    PortHandle = (PVOID)(ULONG_PTR)0x55555555;
    Status = NtConnectPort(&PortHandle,
                           &PortName,
                           &SecurityQos,
                           NULL,
                           NULL,
                           NULL,
                           &ConnectInfo,
                           &ConnectInfoLength);
    ok_hex(Status, STATUS_PORT_CONNECTION_REFUSED);
    ok(PortHandle == (PVOID)(ULONG_PTR)0x55555555, "PortHandle = %p\n", PortHandle);

    /* Try again, this time it will be accepted */
    ConnectInfo.Signature = TEST_CONNECTION_INFO_SIGNATURE2;
    ConnectInfoLength = sizeof(ConnectInfo);
    Status = NtConnectPort(&PortHandle,
                           &PortName,
                           &SecurityQos,
                           NULL,
                           NULL,
                           NULL,
                           &ConnectInfo,
                           &ConnectInfoLength);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to connect\n");
        return 0;
    }

    RtlZeroMemory(&Message, sizeof(Message));
    Message.Header.u1.s1.TotalLength = sizeof(Message);
    Message.Header.u1.s1.DataLength = sizeof(Message.Message);
    Message.Message = TEST_MESSAGE_MESSAGE;
    Status = NtRequestPort(PortHandle,
                           &Message.Header);
    ok_hex(Status, STATUS_SUCCESS);

    Status = NtClose(PortHandle);
    ok_hex(Status, STATUS_SUCCESS);

    return 0;
}

START_TEST(NtAcceptConnectPort)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE PortHandle;
    HANDLE ThreadHandles[2];

    InitializeObjectAttributes(&ObjectAttributes,
                               &PortName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtCreatePort(&PortHandle,
                          &ObjectAttributes,
                          sizeof(TEST_CONNECTION_INFO),
                          sizeof(TEST_MESSAGE),
                          2 * sizeof(TEST_MESSAGE));
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create port\n");
        return;
    }

    ThreadHandles[0] = (HANDLE)_beginthreadex(NULL,
                                              0,
                                              ServerThread,
                                              PortHandle,
                                              0,
                                              &ServerThreadId);
    ok(ThreadHandles[0] != NULL, "_beginthreadex failed\n");

    ThreadHandles[1] = (HANDLE)_beginthreadex(NULL,
                                              0,
                                              ClientThread,
                                              PortHandle,
                                              0,
                                              &ClientThreadId);
    ok(ThreadHandles[1] != NULL, "_beginthreadex failed\n");

    Status = NtWaitForMultipleObjects(RTL_NUMBER_OF(ThreadHandles),
                                      ThreadHandles,
                                      WaitAll,
                                      FALSE,
                                      NULL);
    ok_hex(Status, STATUS_SUCCESS);

    Status = NtClose(ThreadHandles[0]);
    ok_hex(Status, STATUS_SUCCESS);
    Status = NtClose(ThreadHandles[1]);
    ok_hex(Status, STATUS_SUCCESS);

    Status = NtClose(PortHandle);
    ok_hex(Status, STATUS_SUCCESS);
}
