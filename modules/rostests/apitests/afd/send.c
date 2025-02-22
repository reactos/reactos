/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Test for IOCTL_AFD_SEND/IOCTL_AFD_SEND_DATAGRAM
 * COPYRIGHT:   Copyright 2015 Thomas Faber (thomas.faber@reactos.org)
 */

#include "precomp.h"

static
void
TestSend(void)
{
    NTSTATUS Status;
    HANDLE SocketHandle;
    CHAR Buffer[32];
    struct sockaddr_in addr;

    RtlZeroMemory(Buffer, sizeof(Buffer));

    Status = AfdCreateSocket(&SocketHandle, AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(Status == STATUS_SUCCESS, "AfdCreateSocket failed with %lx\n", Status);

    Status = AfdSend(SocketHandle, NULL, 0);
    ok(Status == STATUS_INVALID_CONNECTION, "AfdSend failed with %lx\n", Status);

    Status = AfdSend(SocketHandle, Buffer, sizeof(Buffer));
    ok(Status == STATUS_INVALID_CONNECTION, "AfdSend failed with %lx\n", Status);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    addr.sin_port = htons(0);

    Status = AfdBind(SocketHandle, (const struct sockaddr *)&addr, sizeof(addr));
    ok(Status == STATUS_SUCCESS, "AfdBind failed with %lx\n", Status);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("8.8.8.8");
    addr.sin_port = htons(53);

    Status = AfdConnect(SocketHandle, (const struct sockaddr *)&addr, sizeof(addr));
    ok(Status == STATUS_SUCCESS, "AfdConnect failed with %lx\n", Status);

    Status = AfdSend(SocketHandle, NULL, 0);
    ok(Status == STATUS_SUCCESS, "AfdSend failed with %lx\n", Status);

    Status = AfdSend(SocketHandle, Buffer, sizeof(Buffer));
    ok(Status == STATUS_SUCCESS, "AfdSend failed with %lx\n", Status);

    NtClose(SocketHandle);
}

static
void
TestSendTo(void)
{
    NTSTATUS Status;
    HANDLE SocketHandle;
    CHAR Buffer[32];
    struct sockaddr_in addr;

    RtlZeroMemory(Buffer, sizeof(Buffer));

    Status = AfdCreateSocket(&SocketHandle, AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ok(Status == STATUS_SUCCESS, "AfdCreateSocket failed with %lx\n", Status);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    addr.sin_port = htons(0);

    Status = AfdBind(SocketHandle, (const struct sockaddr *)&addr, sizeof(addr));
    ok(Status == STATUS_SUCCESS, "AfdBind failed with %lx\n", Status);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("8.8.8.8");
    addr.sin_port = htons(53);

    Status = AfdSendTo(SocketHandle, NULL, 0, (const struct sockaddr *)&addr, sizeof(addr));
    ok(Status == STATUS_SUCCESS, "AfdSendTo failed with %lx\n", Status);

    Status = AfdSendTo(SocketHandle, Buffer, sizeof(Buffer), (const struct sockaddr *)&addr, sizeof(addr));
    ok(Status == STATUS_SUCCESS, "AfdSendTo failed with %lx\n", Status);

    NtClose(SocketHandle);
}

START_TEST(send)
{
    TestSend();
    TestSendTo();
}
