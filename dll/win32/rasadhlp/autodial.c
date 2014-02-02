/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 SPI
 * FILE:        lib/mswsock/lib/init.c
 * PURPOSE:     DLL Initialization
 */

#include "precomp.h"

#include <ndk/iofuncs.h>
#include <ndk/rtltypes.h>

/* FUNCTIONS *****************************************************************/

BOOLEAN
WINAPI
AcsHlpSendCommand(IN PAUTODIAL_COMMAND Command)
{
    UNICODE_STRING DriverName = RTL_CONSTANT_STRING(L"\\Device\\RasAcd");
    NTSTATUS Status;
    HANDLE DriverHandle;
    HANDLE EventHandle = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;

    /* Initialize the object attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               &DriverName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Open a handle to it */
    Status = NtCreateFile(&DriverHandle,
                          FILE_READ_DATA | FILE_WRITE_DATA,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF,
                          0,
                          NULL,
                          0);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Create an event */
    EventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!EventHandle)
    {
        /* Event failed, fail us */
        CloseHandle(DriverHandle);
        return FALSE;
    }

    /* Connect to the driver */
    Status = NtDeviceIoControlFile(DriverHandle,
                                   EventHandle,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_ACD_CONNECT_ADDRESS,
                                   Command,
                                   sizeof(AUTODIAL_COMMAND),
                                   NULL,
                                   0);

    /* Check if we need to wait */
    if (Status == STATUS_PENDING)
    {
        /* Wait for the driver */
        Status = WaitForSingleObject(EventHandle, INFINITE);

        /* Update status */
        Status = IoStatusBlock.Status;
    }

    /* Close handles and return */
    CloseHandle(EventHandle);
    CloseHandle(DriverHandle);
    return NT_SUCCESS(Status);
}

/*
 * @implemented
 */
BOOLEAN
WINAPI
AcsHlpAttemptConnection(IN PAUTODIAL_ADDR ConnectionAddress)
{
    AUTODIAL_COMMAND Command;

    /* Clear the command packet */
    RtlZeroMemory(&Command, sizeof(AUTODIAL_COMMAND));

    /* Copy the address into the command packet */
    RtlCopyMemory(&Command.Address, ConnectionAddress, sizeof(AUTODIAL_ADDR));

    /* Send it to the driver */
    return AcsHlpSendCommand(&Command);
}

/*
 * @implemented
 */
BOOLEAN
WINAPI
AcsHlpNoteNewConnection(IN PAUTODIAL_ADDR ConnectionAddress,
                        IN PAUTODIAL_CONN Connection)
{
    AUTODIAL_COMMAND Command;

    /* Copy the address into the command packet */
    RtlCopyMemory(&Command.Address, ConnectionAddress, sizeof(AUTODIAL_ADDR));

    /* Set the New Connection flag and copy the connection data */
    Command.NewConnection = TRUE;
    RtlCopyMemory(&Command.Connection, Connection, sizeof(AUTODIAL_CONN));

    /* Send it to the driver */
    return AcsHlpSendCommand(&Command);
}
