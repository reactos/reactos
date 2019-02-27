/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Test for AFD_INFO_RECEIVE_WINDOW_SIZE/AFD_INFO_SEND_WINDOW_SIZE
 * COPYRIGHT:   Copyright 2019 Pierre Schweitzer (pierre@reactos.org)
 */

#include "precomp.h"

static
void
TestTcp(void)
{
    NTSTATUS Status;
    HANDLE SocketHandle;
    struct sockaddr_in addr;
    ULONG OrigReceiveSize, OrigSendSize, ReceiveSize, SendSize;

    Status = AfdCreateSocket(&SocketHandle, AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(Status == STATUS_SUCCESS, "AfdCreateSocket failed with %lx\n", Status);

    Status = AfdGetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &OrigReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(OrigReceiveSize == 0x1000 || OrigReceiveSize == 0x2000, "Invalid size: %lu\n", OrigReceiveSize);
    Status = AfdGetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &OrigSendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(OrigSendSize == 0x1000 || OrigSendSize == 0x2000, "Invalid size: %lu\n", OrigSendSize);

    ReceiveSize = 0;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_INVALID_PARAMETER, "AfdSetInformation failed with %lx\n", Status);
    SendSize = 0;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_INVALID_PARAMETER, "AfdSetInformation failed with %lx\n", Status);

    ReceiveSize = (ULONG)-1L;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_INVALID_PARAMETER, "AfdSetInformation failed with %lx\n", Status);
    SendSize = (ULONG)-1L;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_INVALID_PARAMETER, "AfdSetInformation failed with %lx\n", Status);

    Status = AfdGetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(ReceiveSize == OrigReceiveSize, "Invalid size: %lu %lu\n", ReceiveSize, OrigReceiveSize);
    Status = AfdGetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(SendSize == OrigSendSize, "Invalid size: %lu %lu\n", SendSize, OrigSendSize);

    ReceiveSize = OrigReceiveSize;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_INVALID_PARAMETER, "AfdSetInformation failed with %lx\n", Status);
    SendSize = OrigSendSize;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_INVALID_PARAMETER, "AfdSetInformation failed with %lx\n", Status);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    addr.sin_port = htons(0);

    Status = AfdBind(SocketHandle, (const struct sockaddr *)&addr, sizeof(addr));
    ok(Status == STATUS_SUCCESS, "AfdBind failed with %lx\n", Status);

    Status = AfdGetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(ReceiveSize == OrigReceiveSize, "Invalid size: %lu %lu\n", ReceiveSize, OrigReceiveSize);
    Status = AfdGetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(SendSize == OrigSendSize, "Invalid size: %lu %lu\n", SendSize, OrigSendSize);

    ReceiveSize = 0;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_INVALID_PARAMETER, "AfdSetInformation failed with %lx\n", Status);
    SendSize = 0;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_INVALID_PARAMETER, "AfdSetInformation failed with %lx\n", Status);

    ReceiveSize = (ULONG)-1L;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_INVALID_PARAMETER, "AfdSetInformation failed with %lx\n", Status);
    SendSize = (ULONG)-1L;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_INVALID_PARAMETER, "AfdSetInformation failed with %lx\n", Status);

    Status = AfdGetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(ReceiveSize == OrigReceiveSize, "Invalid size: %lu %lu\n", ReceiveSize, OrigReceiveSize);
    Status = AfdGetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(SendSize == OrigSendSize, "Invalid size: %lu %lu\n", SendSize, OrigSendSize);

    ReceiveSize = OrigReceiveSize;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_INVALID_PARAMETER, "AfdSetInformation failed with %lx\n", Status);
    SendSize = OrigSendSize;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_INVALID_PARAMETER, "AfdSetInformation failed with %lx\n", Status);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("8.8.8.8");
    addr.sin_port = htons(53);

    Status = AfdConnect(SocketHandle, (const struct sockaddr *)&addr, sizeof(addr));
    ok(Status == STATUS_SUCCESS, "AfdConnect failed with %lx\n", Status);

    Status = AfdGetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(ReceiveSize == OrigReceiveSize, "Invalid size: %lu %lu\n", ReceiveSize, OrigReceiveSize);
    Status = AfdGetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(SendSize == OrigSendSize, "Invalid size: %lu %lu\n", SendSize, OrigSendSize);

    ReceiveSize = 0;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdSetInformation failed with %lx\n", Status);
    SendSize = 0;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdSetInformation failed with %lx\n", Status);

    Status = AfdGetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(ReceiveSize == OrigReceiveSize, "Invalid size: %lu %lu\n", ReceiveSize, OrigReceiveSize);
    Status = AfdGetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(SendSize == OrigSendSize, "Invalid size: %lu %lu\n", SendSize, OrigSendSize);

    ReceiveSize = (ULONG)-1L;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdSetInformation failed with %lx\n", Status);
    SendSize = (ULONG)-1L;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdSetInformation failed with %lx\n", Status);

    Status = AfdGetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(ReceiveSize == OrigReceiveSize, "Invalid size: %lu %lu\n", ReceiveSize, OrigReceiveSize);
    Status = AfdGetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(SendSize == OrigSendSize, "Invalid size: %lu %lu\n", SendSize, OrigSendSize);

    ReceiveSize = OrigReceiveSize + 1;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdSetInformation failed with %lx\n", Status);
    SendSize = OrigSendSize + 1;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdSetInformation failed with %lx\n", Status);

    Status = AfdGetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(ReceiveSize == OrigReceiveSize, "Invalid size: %lu %lu\n", ReceiveSize, OrigReceiveSize);
    Status = AfdGetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(SendSize == OrigSendSize, "Invalid size: %lu %lu\n", SendSize, OrigSendSize);

    ReceiveSize = OrigReceiveSize - 1;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdSetInformation failed with %lx\n", Status);
    SendSize = OrigSendSize - 1;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdSetInformation failed with %lx\n", Status);

    Status = AfdGetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(ReceiveSize == OrigReceiveSize, "Invalid size: %lu %lu\n", ReceiveSize, OrigReceiveSize);
    Status = AfdGetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(SendSize == OrigSendSize, "Invalid size: %lu %lu\n", SendSize, OrigSendSize);

    NtClose(SocketHandle);
}

static
void
TestUdp(void)
{
    NTSTATUS Status;
    HANDLE SocketHandle;
    struct sockaddr_in addr;
    ULONG OrigReceiveSize, OrigSendSize, ReceiveSize, SendSize;

    Status = AfdCreateSocket(&SocketHandle, AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ok(Status == STATUS_SUCCESS, "AfdCreateSocket failed with %lx\n", Status);

    Status = AfdGetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &OrigReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(OrigReceiveSize == 0x1000 || OrigReceiveSize == 0x2000, "Invalid size: %lu\n", OrigReceiveSize);
    Status = AfdGetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &OrigSendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(OrigSendSize == 0x1000 || OrigSendSize == 0x2000, "Invalid size: %lu\n", OrigSendSize);

    ReceiveSize = 0;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdSetInformation failed with %lx\n", Status);
    SendSize = 0;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdSetInformation failed with %lx\n", Status);

    Status = AfdGetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(ReceiveSize == OrigReceiveSize, "Invalid size: %lu %lu\n", ReceiveSize, OrigReceiveSize);
    Status = AfdGetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(SendSize == OrigSendSize, "Invalid size: %lu %lu\n", SendSize, OrigSendSize);

    ReceiveSize = (ULONG)-1L;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdSetInformation failed with %lx\n", Status);
    SendSize = (ULONG)-1L;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdSetInformation failed with %lx\n", Status);

    Status = AfdGetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(ReceiveSize == OrigReceiveSize, "Invalid size: %lu %lu\n", ReceiveSize, OrigReceiveSize);
    Status = AfdGetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(SendSize == OrigSendSize, "Invalid size: %lu %lu\n", SendSize, OrigSendSize);

    ReceiveSize = OrigReceiveSize;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdSetInformation failed with %lx\n", Status);
    SendSize = OrigSendSize;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdSetInformation failed with %lx\n", Status);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    addr.sin_port = htons(0);

    Status = AfdBind(SocketHandle, (const struct sockaddr *)&addr, sizeof(addr));
    ok(Status == STATUS_SUCCESS, "AfdBind failed with %lx\n", Status);

    Status = AfdGetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(ReceiveSize == OrigReceiveSize, "Invalid size: %lu %lu\n", ReceiveSize, OrigReceiveSize);
    Status = AfdGetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(SendSize == OrigSendSize, "Invalid size: %lu %lu\n", SendSize, OrigSendSize);

    ReceiveSize = 0;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdSetInformation failed with %lx\n", Status);
    SendSize = 0;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdSetInformation failed with %lx\n", Status);

    Status = AfdGetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(ReceiveSize == OrigReceiveSize, "Invalid size: %lu %lu\n", ReceiveSize, OrigReceiveSize);
    Status = AfdGetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(SendSize == OrigSendSize, "Invalid size: %lu %lu\n", SendSize, OrigSendSize);

    ReceiveSize = (ULONG)-1L;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdSetInformation failed with %lx\n", Status);
    SendSize = (ULONG)-1L;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdSetInformation failed with %lx\n", Status);

    Status = AfdGetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(ReceiveSize == OrigReceiveSize, "Invalid size: %lu %lu\n", ReceiveSize, OrigReceiveSize);
    Status = AfdGetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(SendSize == OrigSendSize, "Invalid size: %lu %lu\n", SendSize, OrigSendSize);

    ReceiveSize = OrigReceiveSize + 1;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdSetInformation failed with %lx\n", Status);
    SendSize = OrigSendSize + 1;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdSetInformation failed with %lx\n", Status);

    Status = AfdGetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(ReceiveSize == OrigReceiveSize, "Invalid size: %lu %lu\n", ReceiveSize, OrigReceiveSize);
    Status = AfdGetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(SendSize == OrigSendSize, "Invalid size: %lu %lu\n", SendSize, OrigSendSize);

    ReceiveSize = OrigReceiveSize - 1;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdSetInformation failed with %lx\n", Status);
    SendSize = OrigSendSize - 1;
    Status = AfdSetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdSetInformation failed with %lx\n", Status);

    Status = AfdGetInformation(SocketHandle, AFD_INFO_RECEIVE_WINDOW_SIZE, NULL, &ReceiveSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(ReceiveSize == OrigReceiveSize, "Invalid size: %lu %lu\n", ReceiveSize, OrigReceiveSize);
    Status = AfdGetInformation(SocketHandle, AFD_INFO_SEND_WINDOW_SIZE, NULL, &SendSize, NULL);
    ok(Status == STATUS_SUCCESS, "AfdGetInformation failed with %lx\n", Status);
    ok(SendSize == OrigSendSize, "Invalid size: %lu %lu\n", SendSize, OrigSendSize);

    NtClose(SocketHandle);
}

START_TEST(windowsize)
{
    TestTcp();
    TestUdp();
}
