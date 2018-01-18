/*
* PROJECT:     ReactOS ip helper library
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        dll/win32/iphlpapi/icmp.c
* PURPOSE:     Icmp APIs
* COPYRIGHT:   Copyright 2018 Ged Murphy <gedmurphy@reactos.org>
*
*/

#include "iphlpapi_private.h"
#include <tcpip_shared.h>

WINE_DEFAULT_DEBUG_CHANNEL(icmp);
//WINE_DECLARE_DEBUG_CHANNEL(winediag);

static
DWORD
IcmpParseResponse(
    PICMP_ECHO_REPLY ReplyBuffer,
    DWORD ReplySize,
    _Out_ LPDWORD NumReplies
);



/***********************************************************************
 *		IcmpCreateFile (IPHLPAPI.@)
 */
HANDLE WINAPI IcmpCreateFile(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING DeviceName;
    HANDLE FileHandle;
    NTSTATUS Status;
    DWORD Error;

    /* ICMP requests are handled by the Ip device */
    RtlInitUnicodeString(&DeviceName, L"\\Device\\Ip");
    InitializeObjectAttributes(&ObjectAttributes,
                               &DeviceName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Get a handle to the device */
    Status = NtCreateFile(&FileHandle,
                          GENERIC_EXECUTE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          0,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF,
                          0,
                          0,
                          0);
    if (!NT_SUCCESS(Status))
    {
        Error = RtlNtStatusToDosError(Status);
        SetLastError(Error);
        return INVALID_HANDLE_VALUE;
    }

    return FileHandle;
}


/***********************************************************************
 *		Icmp6CreateFile (IPHLPAPI.@)
 */
HANDLE WINAPI Icmp6CreateFile(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING DeviceName;
    HANDLE FileHandle;
    NTSTATUS Status;
    DWORD Error;

    /* ICMP requests are handled by the Ip device */
    RtlInitUnicodeString(&DeviceName, L"\\Device\\Ip6");
    InitializeObjectAttributes(&ObjectAttributes,
                               &DeviceName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Get a handle to the device */
    Status = NtCreateFile(&FileHandle,
                          GENERIC_EXECUTE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          0,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF,
                          0,
                          0,
                          0);
    if (!NT_SUCCESS(Status))
    {
        Error = RtlNtStatusToDosError(Status);
        SetLastError(Error);
        return INVALID_HANDLE_VALUE;
    }

    return FileHandle;
}



/***********************************************************************
 *		IcmpCloseHandle (IPHLPAPI.@)
 */
BOOL WINAPI IcmpCloseHandle(
    _In_ HANDLE IcmpHandle
    )
{
    NTSTATUS Status;
    DWORD Error;

    Status = NtClose(IcmpHandle);
    if (!NT_SUCCESS(Status))
    {
        Error = RtlNtStatusToDosError(Status);
        SetLastError(Error);
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *		IcmpSendEcho2 (IPHLPAPI.@)
 */
DWORD WINAPI IcmpSendEcho2(
    _In_ HANDLE IcmpHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _In_ IPAddr DestinationAddress,
    _In_reads_bytes_(RequestSize) LPVOID RequestData,
    _In_ WORD RequestSize,
    _In_opt_ PIP_OPTION_INFORMATION RequestOptions,
    _Out_writes_bytes_(ReplySize) LPVOID ReplyBuffer,
    _In_range_(>=, sizeof(ICMP_ECHO_REPLY) + RequestSize + 8) DWORD ReplySize,
    _In_ DWORD Timeout
    )
{
    PICMP_ECHO_DATA_V4 IcmpEchoData;
    IO_STATUS_BLOCK IoStatusBlock;
    DWORD BufferSize;
    HANDLE LocalEvent;
    BOOL CreatedEvent;
    DWORD NumReplies;
    DWORD Error;
    PBYTE Ptr;
    NTSTATUS Status;

    /* Initialize the variables */
    LocalEvent = NULL;
    CreatedEvent = FALSE;
    NumReplies = 0;

    /* Build up the required buffer size */
    BufferSize = sizeof(ICMP_ECHO_DATA_V4) + RequestSize;
    if (RequestOptions)
    {
        BufferSize += RequestOptions->OptionsSize;
    }

    /* And allocate it */
    IcmpEchoData = HeapAlloc(GetProcessHeap(), 0, BufferSize);
    if (!IcmpEchoData)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    /* Get the start of the dynamic data */
    Ptr = (PBYTE)(IcmpEchoData + 1);

    /* Fill out the struct */
    IcmpEchoData->DestinationAddress = DestinationAddress;
    IcmpEchoData->Timeout = Timeout;
    IcmpEchoData->Ttl = RequestOptions->Ttl;
    IcmpEchoData->Tos = RequestOptions->Tos;
    IcmpEchoData->Flags = RequestOptions->Flags;
    IcmpEchoData->OptionsSize = RequestOptions->OptionsSize;
    if (RequestOptions->OptionsSize)
    {
        IcmpEchoData->OptionsData = Ptr;
        CopyMemory(IcmpEchoData->OptionsData, RequestOptions->OptionsData, RequestOptions->OptionsSize);
        Ptr += RequestOptions->OptionsSize;
    }
    if (RequestSize)
    {
        IcmpEchoData->RequestData = Ptr;
        CopyMemory(IcmpEchoData->RequestData, RequestData, RequestSize);
    }

    /* If we have no event or APC, then we'll need an internal wait object */
    if ((Event == NULL) && (ApcRoutine == NULL))
    {
        LocalEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
        if (LocalEvent == NULL)
        {
            Error = GetLastError();
            goto Quit;
        }
        CreatedEvent = TRUE;
    }
    else
    {
        /* Just use whatever the caller gave us */
        LocalEvent = Event;
    }

    /* Forward the request to the IP device */
    Status = NtDeviceIoControlFile(IcmpHandle,
                                   LocalEvent,
                                   ApcRoutine,
                                   ApcContext,
                                   &IoStatusBlock,
                                   IOCTL_ECHO_REQUEST,
                                   IcmpEchoData,
                                   BufferSize,
                                   ReplyBuffer,
                                   ReplySize);
    if (Status == STATUS_PENDING)
    {
        /* Wait */
        NtWaitForSingleObject(Event, 0, 0);
        Status = IoStatusBlock.Status;
    }

    /* Check if we got a successful reply */
    if (NT_SUCCESS(Status))
    {
        /* Setup the response */
        Error = IcmpParseResponse(ReplyBuffer, ReplySize, &NumReplies);
    }
    else
    {
        Error = RtlNtStatusToDosError(Status);
    }

Quit:
    if (CreatedEvent && LocalEvent)
    {
        CloseHandle(LocalEvent);
    }
    if (IcmpEchoData)
    {
        HeapFree(GetProcessHeap(), 0, IcmpEchoData);
    }

    if (Error != ERROR_SUCCESS)
    {
        SetLastError(Error);
        NumReplies = 0;
    }

    return NumReplies;
}

/***********************************************************************
 *		IcmpSendEcho (IPHLPAPI.@)
 */
DWORD WINAPI IcmpSendEcho(
    _In_ HANDLE IcmpHandle,
    _In_ IPAddr DestinationAddress,
    _In_reads_bytes_(RequestSize) LPVOID RequestData,
    _In_ WORD RequestSize,
    _In_opt_ PIP_OPTION_INFORMATION RequestOptions,
    _Out_writes_bytes_(ReplySize) LPVOID ReplyBuffer,
    _In_range_(>= , sizeof(ICMP_ECHO_REPLY) + RequestSize + 8) DWORD ReplySize,
    _In_ DWORD Timeout
    )
{
    return IcmpSendEcho2(IcmpHandle,
                         NULL,
                         NULL,
                         NULL,
                         DestinationAddress,
                         RequestData,
                         RequestSize,
                         RequestOptions,
                         ReplyBuffer,
                         ReplySize,
                         Timeout);
}

/***********************************************************************
*		Icmp6SendEcho2 (IPHLPAPI.@)
*/
DWORD WINAPI Icmp6SendEcho2(
    _In_ HANDLE IcmpHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _In_ struct sockaddr_in6 *SourceAddress,
    _In_ struct sockaddr_in6 *DestinationAddress,
    _In_reads_bytes_(RequestSize) LPVOID RequestData,
    _In_ WORD RequestSize,
    _In_opt_ PIP_OPTION_INFORMATION RequestOptions,
    _Out_writes_bytes_(ReplySize) LPVOID ReplyBuffer,
    _In_range_(>=, sizeof(ICMPV6_ECHO_REPLY) + RequestSize + 8) DWORD ReplySize,
    _In_ DWORD Timeout
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


static
DWORD
IcmpParseResponse(
    _Out_writes_bytes_(ReplySize) PICMP_ECHO_REPLY ReplyBuffer,
    _In_range_(>= , sizeof(ICMPV6_ECHO_REPLY) + RequestSize + 8) DWORD ReplySize,
    _Out_ LPDWORD NumReplies
)
{
    /* Sanity check */
    if (!ReplyBuffer || !ReplySize)
    {
        return ERROR_INVALID_PARAMETER;
    }

    /* The number of replies received is held in the reserved param */
    *NumReplies = ReplyBuffer->Reserved;
    ReplyBuffer->Reserved = 0;

    //FIXME: Setup the options data

    return ReplyBuffer->Status;
}