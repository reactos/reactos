/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for send/sendto
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>
#include <winsock2.h>

#define WIN32_NO_STATUS
#include <ndk/mmfuncs.h>
#include <ndk/rtlfuncs.h>

static
PVOID
AllocateReadOnly(
    _In_ SIZE_T SizeRequested)
{
    NTSTATUS Status;
    SIZE_T Size = PAGE_ROUND_UP(SizeRequested);
    PVOID VirtualMemory = NULL;

    Status = NtAllocateVirtualMemory(NtCurrentProcess(), &VirtualMemory, 0, &Size, MEM_COMMIT, PAGE_READONLY);
    if (!NT_SUCCESS(Status))
        return NULL;

    return VirtualMemory;
}

static
VOID
FreeReadOnly(
    _In_ PVOID VirtualMemory)
{
    NTSTATUS Status;
    SIZE_T Size = 0;

    Status = NtFreeVirtualMemory(NtCurrentProcess(), &VirtualMemory, &Size, MEM_RELEASE);
    ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
}

static
VOID
test_send(void)
{
    SOCKET sock;
    int ret;
    int error;
    PVOID buffer;
    ULONG bufferSize;
    struct sockaddr_in addr;

    bufferSize = 32;
    buffer = AllocateReadOnly(bufferSize);
    ok(buffer != NULL, "AllocateReadOnly failed\n");
    if (!buffer)
    {
        skip("No memory\n");
        return;
    }

    ret = send(0, NULL, 0, 0);
    error = WSAGetLastError();
    ok(ret == SOCKET_ERROR, "send returned %d\n", ret);
    ok(error == WSAENOTSOCK, "error = %d\n", error);

    ret = send(0, buffer, bufferSize, 0);
    error = WSAGetLastError();
    ok(ret == SOCKET_ERROR, "send returned %d\n", ret);
    ok(error == WSAENOTSOCK, "error = %d\n", error);

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(sock != INVALID_SOCKET, "socket failed\n");
    if (sock == INVALID_SOCKET)
    {
        skip("No socket\n");
        FreeReadOnly(buffer);
        return;
    }

    ret = send(sock, NULL, 0, 0);
    error = WSAGetLastError();
    ok(ret == SOCKET_ERROR, "send returned %d\n", ret);
    ok(error == WSAENOTCONN, "error = %d\n", error);

    ret = send(sock, buffer, bufferSize, 0);
    error = WSAGetLastError();
    ok(ret == SOCKET_ERROR, "send returned %d\n", ret);
    ok(error == WSAENOTCONN, "error = %d\n", error);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("8.8.8.8");
    addr.sin_port = htons(53);
    ret = connect(sock, (const struct sockaddr *)&addr, sizeof(addr));
    error = WSAGetLastError();
    ok(ret == 0, "connect returned %d\n", ret);
    ok(error == 0, "error = %d\n", error);

    ret = send(sock, NULL, 0, 0);
    error = WSAGetLastError();
    ok(ret == 0, "send returned %d\n", ret);
    ok(error == 0, "error = %d\n", error);

    ret = send(sock, buffer, bufferSize, 0);
    error = WSAGetLastError();
    ok(ret == bufferSize, "send returned %d\n", ret);
    ok(error == 0, "error = %d\n", error);

    closesocket(sock);

    FreeReadOnly(buffer);
}

static
VOID
test_sendto(void)
{
    SOCKET sock;
    int ret;
    int error;
    PVOID buffer;
    ULONG bufferSize;
    struct sockaddr_in addr;

    bufferSize = 32;
    buffer = AllocateReadOnly(bufferSize);
    ok(buffer != NULL, "AllocateReadOnly failed\n");
    if (!buffer)
    {
        skip("No memory\n");
        return;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("8.8.8.8");
    addr.sin_port = htons(53);

    ret = sendto(0, NULL, 0, 0, (const struct sockaddr *)&addr, sizeof(addr));
    error = WSAGetLastError();
    ok(ret == SOCKET_ERROR, "sendto returned %d\n", ret);
    ok(error == WSAENOTSOCK, "error = %d\n", error);

    ret = sendto(0, buffer, bufferSize, 0, (const struct sockaddr *)&addr, sizeof(addr));
    error = WSAGetLastError();
    ok(ret == SOCKET_ERROR, "sendto returned %d\n", ret);
    ok(error == WSAENOTSOCK, "error = %d\n", error);

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ok(sock != INVALID_SOCKET, "socket failed\n");
    if (sock == INVALID_SOCKET)
    {
        skip("No socket\n");
        FreeReadOnly(buffer);
        return;
    }

    ret = sendto(sock, NULL, 0, 0, (const struct sockaddr *)&addr, sizeof(addr));
    error = WSAGetLastError();
    ok(ret == 0, "sendto returned %d\n", ret);
    ok(error == 0, "error = %d\n", error);

    ret = sendto(sock, buffer, bufferSize, 0, (const struct sockaddr *)&addr, sizeof(addr));
    error = WSAGetLastError();
    ok(ret == bufferSize, "sendto returned %d\n", ret);
    ok(error == 0, "error = %d\n", error);

    closesocket(sock);

    FreeReadOnly(buffer);
}

START_TEST(send)
{
    int ret;
    WSADATA wsad;

    ret = WSAStartup(MAKEWORD(2, 2), &wsad);
    ok(ret == 0, "WSAStartup failed with %d\n", ret);
    test_send();
    test_sendto();
    WSACleanup();
}
