/*
 * PROJECT:     ReactOS Networking
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/iphlpapi/iphlpapi_reactos.c
 * PURPOSE:     DHCP helper functions for ReactOS
 * PROGRAMMERS: Pierre Schweitzer <pierre@reactos.org>
 */

#include "iphlpapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(iphlpapi);

DWORD TCPSendIoctl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer, PULONG pInBufferSize, LPVOID lpOutBuffer, PULONG pOutBufferSize)
{
    BOOL Hack = FALSE;
    HANDLE Event;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    /* FIXME: We don't have a global handle opened to \Device\Ip, so open one each time
     * we need. In a future, it would be cool, just to pass it to TCPSendIoctl using the first arg
     */
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        UNICODE_STRING DevName = RTL_CONSTANT_STRING(L"\\Device\\Ip");
        OBJECT_ATTRIBUTES ObjectAttributes;

        FIXME("Using the handle hack\n");
        Hack = TRUE;

        InitializeObjectAttributes(&ObjectAttributes,
                                   &DevName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        Status = NtCreateFile(&hDevice, GENERIC_EXECUTE, &ObjectAttributes,
                              &IoStatusBlock, 0, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN_IF,
                              0, NULL, 0);
        if (!NT_SUCCESS(Status))
        {
          return RtlNtStatusToDosError(Status);
        }
    }

    /* Sync event */
    Event = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (Event == NULL)
    {
        /* FIXME: See upper */
        if (Hack)
        {
            CloseHandle(hDevice);
        }
        return GetLastError();
    }

    /* Reinit, and call the networking stack */
    IoStatusBlock.Status = STATUS_SUCCESS;
    IoStatusBlock.Information = 0;
    Status = NtDeviceIoControlFile(hDevice, Event, NULL, NULL, &IoStatusBlock, dwIoControlCode, lpInBuffer, *pInBufferSize, lpOutBuffer, *pOutBufferSize);
    if (Status == STATUS_PENDING)
    {
        NtWaitForSingleObject(Event, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    /* Close & return size info */
    CloseHandle(Event);
    *pOutBufferSize = IoStatusBlock.Information;

    /* FIXME: See upper */
    if (Hack)
    {
        CloseHandle(hDevice);
    }

    /* Return result */
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    return ERROR_SUCCESS;
}
