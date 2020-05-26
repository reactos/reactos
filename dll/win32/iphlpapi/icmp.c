/*
 * PROJECT:         ReactOS IP Helper API
 * LICENSE:         LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:         ICMP functions
 * COPYRIGHT:       2016 Tim Crawford (crawfxrd@gmail.com)
 *                  2019 Victor Perevertkin (victor.perevertkin@reactos.org)
 */

#include "iphlpapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(iphlpapi);

HANDLE
WINAPI
Icmp6CreateFile(void)
{
    HANDLE IcmpFile;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\Ip6");
    NTSTATUS Status;

    InitializeObjectAttributes(
        &ObjectAttributes,
        &DeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    Status = NtCreateFile(
        &IcmpFile,
        GENERIC_EXECUTE,
        &ObjectAttributes,
        &IoStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN_IF,
        0,
        NULL,
        0);

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return INVALID_HANDLE_VALUE;
    }

    return IcmpFile;
}

DWORD
WINAPI
Icmp6ParseReplies(
    _In_ LPVOID ReplyBuffer,
    _In_ DWORD  ReplySize)
{
    PICMPV6_ECHO_REPLY pEcho;

    if (ReplyBuffer == NULL || ReplySize == 0)
        return 0;

    pEcho = (PICMPV6_ECHO_REPLY)ReplyBuffer;

    // XXX: MSDN also says IP_TTL_EXPIRED_TRANSIT.
    if (pEcho->Status == IP_SUCCESS)
    {
        return 1;
    }

    SetLastError(pEcho->Status);
    return 0;
}

DWORD
WINAPI
Icmp6SendEcho2(
    _In_     HANDLE                 IcmpHandle,
    _In_opt_ HANDLE                 Event,
    _In_opt_ PIO_APC_ROUTINE        ApcRoutine,
    _In_opt_ PVOID                  ApcContext,
    _In_     struct sockaddr_in6    *SourceAddress,
    _In_     struct sockaddr_in6    *DestinationAddress,
    _In_     LPVOID                 RequestData,
    _In_     WORD                   RequestSize,
    _In_     PIP_OPTION_INFORMATION RequestOptions,
    _Out_    LPVOID                 ReplyBuffer,
    _In_     DWORD                  ReplySize,
    _In_     DWORD                  Timeout)
{
    HANDLE hEvent;
    PIO_STATUS_BLOCK IoStatusBlock;
    PVOID InputBuffer;
    ULONG InputBufferLength;
    //ULONG OutputBufferLength;
    PICMPV6_ECHO_REQUEST Request;
    NTSTATUS Status;

    InputBufferLength = sizeof(ICMPV6_ECHO_REQUEST) + RequestSize;

    if (ReplySize < sizeof(ICMPV6_ECHO_REPLY) + sizeof(IO_STATUS_BLOCK))
    {
        SetLastError(IP_BUF_TOO_SMALL);
        return 0;
    }

    // IO_STATUS_BLOCK will be stored inside ReplyBuffer (in the end)
    // that's because the function may return before device request ends
    IoStatusBlock = (PIO_STATUS_BLOCK)((PUCHAR)ReplyBuffer + ReplySize - sizeof(IO_STATUS_BLOCK));
    ReplySize -= sizeof(IO_STATUS_BLOCK);

    InputBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, InputBufferLength);
    if (InputBuffer == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    Request = (PICMPV6_ECHO_REQUEST)InputBuffer;

    Request->DestinationAddress.sin6_port = DestinationAddress->sin6_port;
    Request->DestinationAddress.sin6_flowinfo = DestinationAddress->sin6_flowinfo;
    CopyMemory(&(Request->DestinationAddress.sin6_addr), &(DestinationAddress->sin6_addr), sizeof(Request->DestinationAddress.sin6_addr));
    Request->DestinationAddress.sin6_scope_id = DestinationAddress->sin6_scope_id;

    Request->SourceAddress.sin6_port = SourceAddress->sin6_port;
    Request->SourceAddress.sin6_flowinfo = SourceAddress->sin6_flowinfo;
    CopyMemory(&(Request->SourceAddress.sin6_addr), &(SourceAddress->sin6_addr), sizeof(Request->SourceAddress.sin6_addr));
    Request->SourceAddress.sin6_scope_id = SourceAddress->sin6_scope_id;

    // XXX: What is this and why is it sometimes 0x72?
    Request->Unknown1 = 0x72;

    Request->Timeout = Timeout;
    Request->Ttl = RequestOptions->Ttl;
    Request->Flags = RequestOptions->Flags;

    if (RequestSize > 0)
    {
        CopyMemory((PBYTE)InputBuffer + sizeof(ICMPV6_ECHO_REQUEST), RequestData, RequestSize);
    }

    if (Event == NULL && ApcRoutine == NULL)
    {
        hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    }
    else
    {
        hEvent = Event;
    }

    Status = NtDeviceIoControlFile(
        IcmpHandle,
        hEvent,
        ApcRoutine,
        ApcContext,
        IoStatusBlock,
        IOCTL_ICMP_ECHO_REQUEST,
        InputBuffer,
        InputBufferLength,
        ReplyBuffer,
        ReplySize);         // TODO: Determine how Windows calculates OutputBufferLength.

    if (Event != NULL || ApcRoutine != NULL)
    {
        SetLastError(RtlNtStatusToDosError(Status));
        HeapFree(GetProcessHeap(), 0, InputBuffer);
        return 0;
    }

    if (Status == STATUS_PENDING)
    {
        Status = NtWaitForSingleObject(hEvent, FALSE, NULL);

        if (NT_SUCCESS(Status))
        {
            Status = IoStatusBlock->Status;
        }
    }

    CloseHandle(hEvent);
    HeapFree(GetProcessHeap(), 0, InputBuffer);

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return 0;
    }

    Status = ((PICMPV6_ECHO_REPLY)ReplyBuffer)->Status;
    if (Status != IP_SUCCESS)
    {
        SetLastError(Status);
        return 0;
    }

    return 1;
}

BOOL
WINAPI
IcmpCloseHandle(
    _In_ HANDLE IcmpHandle)
{
    NTSTATUS Status;

    Status = NtClose(IcmpHandle);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

HANDLE
WINAPI
IcmpCreateFile(void)
{
    HANDLE IcmpFile;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\Ip");
    NTSTATUS Status;

    InitializeObjectAttributes(
        &ObjectAttributes,
        &DeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    Status = NtCreateFile(
        &IcmpFile,
        GENERIC_EXECUTE,
        &ObjectAttributes,
        &IoStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN_IF,
        0,
        NULL,
        0);

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return INVALID_HANDLE_VALUE;
    }

    return IcmpFile;
}

DWORD
WINAPI
IcmpParseReplies(
    _In_ LPVOID ReplyBuffer,
    _In_ DWORD  ReplySize)
{
    PICMP_ECHO_REPLY pEcho;
    DWORD nReplies;

    if (ReplyBuffer == NULL || ReplySize == 0)
        return 0;

    // TODO: Handle ReplyBuffer having more than 1 ICMP_ECHO_REPLY.

    pEcho = (PICMP_ECHO_REPLY)ReplyBuffer;

    if (pEcho->Reserved == 0)
    {
        SetLastError(pEcho->Status);
    }

    nReplies = pEcho->Reserved;
    pEcho->Reserved = 0;

    return nReplies;
}

DWORD
WINAPI
IcmpSendEcho2(
    _In_     HANDLE                 IcmpHandle,
    _In_opt_ HANDLE                 Event,
    _In_opt_ PIO_APC_ROUTINE        ApcRoutine,
    _In_opt_ PVOID                  ApcContext,
    _In_     IPAddr                 DestinationAddress,
    _In_     LPVOID                 RequestData,
    _In_     WORD                   RequestSize,
    _In_opt_ PIP_OPTION_INFORMATION RequestOptions,
    _Out_    LPVOID                 ReplyBuffer,
    _In_     DWORD                  ReplySize,
    _In_     DWORD                  Timeout)
{
    HANDLE hEvent;
    PIO_STATUS_BLOCK IoStatusBlock;
    PVOID InputBuffer;
    PICMP_ECHO_REQUEST Request;
    DWORD nReplies;
    NTSTATUS Status;

    if (ReplySize < sizeof(ICMP_ECHO_REPLY) + sizeof(IO_STATUS_BLOCK))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
    }

    if (ReplySize < RequestSize + sizeof(ICMP_ECHO_REPLY))
    {
        SetLastError(IP_GENERAL_FAILURE);
        return 0;
    }

    // IO_STATUS_BLOCK will be stored inside ReplyBuffer (in the end)
    // that's because the function may return before device request ends
    IoStatusBlock = (PIO_STATUS_BLOCK)((PUCHAR)ReplyBuffer + ReplySize - sizeof(IO_STATUS_BLOCK));
    ReplySize -= sizeof(IO_STATUS_BLOCK);

    InputBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ReplySize);
    if (InputBuffer == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    Request = (PICMP_ECHO_REQUEST)InputBuffer;
    Request->Address = DestinationAddress;
    Request->Timeout = Timeout;
    Request->OptionsOffset = sizeof(ICMP_ECHO_REQUEST);
    Request->DataOffset = sizeof(ICMP_ECHO_REQUEST);

    if (RequestOptions != NULL)
    {
        Request->HasOptions = TRUE;
        Request->Ttl = RequestOptions->Ttl;
        Request->Tos = RequestOptions->Tos;
        Request->Flags = RequestOptions->Flags;

        if (RequestOptions->OptionsSize > 0)
        {
            Request->OptionsSize = RequestOptions->OptionsSize;
            Request->DataOffset += Request->OptionsSize;

            CopyMemory(
                (PUCHAR)InputBuffer + sizeof(ICMP_ECHO_REQUEST),
                RequestOptions->OptionsData,
                Request->OptionsSize);
        }
    }

    if (RequestSize > 0)
    {
        Request->DataSize = RequestSize;
        CopyMemory((PUCHAR)InputBuffer + Request->DataOffset, RequestData, RequestSize);
    }

    if (Event == NULL && ApcRoutine == NULL)
    {
        hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    }
    else
    {
        hEvent = Event;
    }

    Status = NtDeviceIoControlFile(
        IcmpHandle,
        hEvent,
        ApcRoutine,
        ApcContext,
        IoStatusBlock,
        IOCTL_ICMP_ECHO_REQUEST,
        InputBuffer,
        ReplySize,
        ReplyBuffer,
        ReplySize);         // TODO: Determine how Windows calculates OutputBufferLength.

    // If called asynchronously, return for the caller to handle.
    if (Event != NULL || ApcRoutine != NULL)
    {
        SetLastError(RtlNtStatusToDosError(Status));
        HeapFree(GetProcessHeap(), 0, InputBuffer);
        return 0;
    }

    // Otherwise handle it like IcmpSendEcho.
    if (Status == STATUS_PENDING)
    {
        Status = NtWaitForSingleObject(hEvent, FALSE, NULL);

        if (NT_SUCCESS(Status))
        {
            Status = IoStatusBlock->Status;
        }
    }

    CloseHandle(hEvent);
    HeapFree(GetProcessHeap(), 0, InputBuffer);

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return 0;
    }

    Status = ((PICMP_ECHO_REPLY)ReplyBuffer)->Status;
    if (Status != IP_SUCCESS)
    {
        SetLastError(Status);
    }

    nReplies = ((PICMP_ECHO_REPLY)ReplyBuffer)->Reserved;
    ((PICMP_ECHO_REPLY)ReplyBuffer)->Reserved = 0;

    return nReplies;
}

DWORD
WINAPI
IcmpSendEcho(
    _In_     HANDLE                 IcmpHandle,
    _In_     IPAddr                 DestinationAddress,
    _In_     LPVOID                 RequestData,
    _In_     WORD                   RequestSize,
    _In_opt_ PIP_OPTION_INFORMATION RequestOptions,
    _Out_    LPVOID                 ReplyBuffer,
    _In_     DWORD                  ReplySize,
    _In_     DWORD                  Timeout)
{
    HANDLE hEvent;
    IO_STATUS_BLOCK IoStatusBlock;
    PVOID InputBuffer;
    ULONG InputBufferLength;
    PICMP_ECHO_REQUEST Request;
    DWORD nReplies;
    NTSTATUS Status;

    if (Timeout == 0 || Timeout == (DWORD)-1)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (ReplySize < sizeof(ICMP_ECHO_REPLY))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
    }

    if (ReplySize < RequestSize + sizeof(ICMP_ECHO_REPLY))
    {
        SetLastError(IP_GENERAL_FAILURE);
        return 0;
    }

    InputBufferLength = sizeof(ICMP_ECHO_REQUEST) + RequestSize;
    if (RequestOptions != NULL)
        InputBufferLength += RequestOptions->OptionsSize;

    if (InputBufferLength < ReplySize)
        InputBufferLength = ReplySize;

    InputBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, InputBufferLength);
    if (InputBuffer == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    Request = (PICMP_ECHO_REQUEST)InputBuffer;
    Request->Address = DestinationAddress;
    Request->Timeout = Timeout;
    Request->OptionsOffset = sizeof(ICMP_ECHO_REQUEST);
    Request->DataOffset = sizeof(ICMP_ECHO_REQUEST);

    if (RequestOptions != NULL)
    {
        Request->HasOptions = TRUE;
        Request->Ttl = RequestOptions->Ttl;
        Request->Tos = RequestOptions->Tos;
        Request->Flags = RequestOptions->Flags;

        if (RequestOptions->OptionsSize > 0)
        {
            Request->OptionsSize = RequestOptions->OptionsSize;
            Request->DataOffset += Request->OptionsSize;

            CopyMemory(
                (PUCHAR)InputBuffer + sizeof(ICMP_ECHO_REQUEST),
                RequestOptions->OptionsData,
                Request->OptionsSize);
        }
    }

    if (RequestSize > 0)
    {
        Request->DataSize = RequestSize;
        CopyMemory((PUCHAR)InputBuffer + Request->DataOffset, RequestData, RequestSize);
    }

    hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (hEvent == NULL)
    {
        HeapFree(GetProcessHeap(), 0, InputBuffer);
        return 0;
    }

    Status = NtDeviceIoControlFile(
        IcmpHandle,
        hEvent,
        NULL,
        NULL,
        &IoStatusBlock,
        IOCTL_ICMP_ECHO_REQUEST,
        InputBuffer,
        InputBufferLength,
        ReplyBuffer,
        ReplySize);

    if (Status == STATUS_PENDING)
    {
        Status = NtWaitForSingleObject(hEvent, FALSE, NULL);

        if (NT_SUCCESS(Status))
        {
            Status = IoStatusBlock.Status;
        }
    }

    CloseHandle(hEvent);
    HeapFree(GetProcessHeap(), 0, InputBuffer);

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return 0;
    }

    Status = ((PICMP_ECHO_REPLY)ReplyBuffer)->Status;
    if (Status != IP_SUCCESS)
    {
        SetLastError(Status);
    }

    nReplies = ((PICMP_ECHO_REPLY)ReplyBuffer)->Reserved;
    ((PICMP_ECHO_REPLY)ReplyBuffer)->Reserved = 0;

    return nReplies;
}
