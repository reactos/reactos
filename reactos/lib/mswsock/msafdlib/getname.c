/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 SPI
 * FILE:        lib/mswsock/lib/init.c
 * PURPOSE:     DLL Initialization
 */

/* INCLUDES ******************************************************************/
#include "msafd.h"

/* DATA **********************************************************************/

/* FUNCTIONS *****************************************************************/

INT
WSPAPI
WSPGetSockName(IN SOCKET Handle,
               OUT LPSOCKADDR Name,
               IN OUT LPINT NameLength,
               OUT LPINT lpErrno)
{
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG TdiAddressSize;
    INT ErrorCode;
    PTDI_ADDRESS_INFO TdiAddress = NULL;
    PSOCKET_INFORMATION Socket;
    NTSTATUS Status;
    PWINSOCK_TEB_DATA ThreadData;
    CHAR AddressBuffer[FIELD_OFFSET(TDI_ADDRESS_INFO, Address) +
                       MAX_TDI_ADDRESS_LENGTH];

    /* Enter prolog */
    ErrorCode = SockEnterApiFast(&ThreadData);
    if (ErrorCode != NO_ERROR)
    {
        /* Fail */
        *lpErrno = ErrorCode;
        return SOCKET_ERROR;
    }

    /* Get the socket structure */
    Socket = SockFindAndReferenceSocket(Handle, TRUE);
    if (!Socket)
    {
        /* Fail */
        ErrorCode = WSAENOTSOCK;
        goto error;
    }

    /* Lock the socket */
    EnterCriticalSection(&Socket->Lock);

    /* If the socket isn't bound, fail */
    if (Socket->SharedData.State == SocketOpen)
    {
        /* Fail */
        ErrorCode = WSAEINVAL;
        goto error;
    }

    /* Check how long the TDI Address is */
    TdiAddressSize = FIELD_OFFSET(TDI_ADDRESS_INFO, Address) + 
                     Socket->HelperData->MaxTDIAddressLength;

    /* See if it can fit in the stack */
    if (TdiAddressSize <= sizeof(AddressBuffer))
    {
        /* Use the stack */
        TdiAddress = (PVOID)AddressBuffer;
    }
    else
    {
        /* Allocate from heap */
        TdiAddress = SockAllocateHeapRoutine(SockPrivateHeap, 0, TdiAddressSize);
        if (!TdiAddress)
        {
            /* Fail */
            ErrorCode = WSAENOBUFS;
            goto error;
        }
    }

    /* Send IOCTL */
    Status = NtDeviceIoControlFile(Socket->WshContext.Handle,
                                   ThreadData->EventHandle,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_AFD_GET_SOCK_NAME,
                                   NULL,
                                   0,
                                   TdiAddress,
                                   TdiAddressSize);
    
    /* Check if it's pending */
    if (Status == STATUS_PENDING)
    {
        /* Wait for completion */
        LeaveCriticalSection(&Socket->Lock);
        SockWaitForSingleObject(ThreadData->EventHandle,
                                Handle,
                                NO_BLOCKING_HOOK,
                                NO_TIMEOUT);
        EnterCriticalSection(&Socket->Lock);

        /* Get new status */
        Status = IoStatusBlock.Status;
    }

    /* Check for error */
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        ErrorCode = NtStatusToSocketError(Status);
        goto error;
    }

    /* Convert to Sockaddr format */
    SockBuildSockaddr(Socket->LocalAddress,
                      &Socket->SharedData.SizeOfLocalAddress,
                      &TdiAddress->Address);

    /* Check for valid length */
    if (Socket->SharedData.SizeOfLocalAddress > *NameLength)
    {
        /* Fail */
        ErrorCode = WSAEFAULT;
        goto error;
    }

    /* Write the Address */
    RtlCopyMemory(Name,
                  Socket->LocalAddress,
                  Socket->SharedData.SizeOfLocalAddress);

    /* Return the Name Length */
    *NameLength = Socket->SharedData.SizeOfLocalAddress;

error:

    /* Check if we have a socket here */
    if (Socket)
    {
        /* Release the lock and dereference */
        LeaveCriticalSection(&Socket->Lock);
        SockDereferenceSocket(Socket);
    }

    /* Check if we should free the TDI address */
    if ((TdiAddress) && (TdiAddress != (PVOID)AddressBuffer))
    {
        /* Free the Buffer */
        RtlFreeHeap(SockPrivateHeap, 0, TdiAddress);
    }
            
    /* Check for error */
    if (ErrorCode != NO_ERROR)
    {
        /* Fail */
        *lpErrno = ErrorCode;
        return SOCKET_ERROR;
    }

    /* Return success */
    return NO_ERROR;
}

INT
WSPAPI
WSPGetPeerName(IN SOCKET Handle, 
               OUT LPSOCKADDR Name, 
               IN OUT LPINT NameLength, 
               OUT LPINT lpErrno)
{
    INT ErrorCode;
    PSOCKET_INFORMATION Socket;
    PWINSOCK_TEB_DATA ThreadData;

    /* Enter prolog */
    ErrorCode = SockEnterApiFast(&ThreadData);
    if (ErrorCode != NO_ERROR)
    {
        /* Fail */
        *lpErrno = ErrorCode;
        return SOCKET_ERROR;
    }

    /* Get the socket structure */
    Socket = SockFindAndReferenceSocket(Handle, TRUE);
    if (!Socket)
    {
        /* Fail */
        ErrorCode = WSAENOTSOCK;
        goto error;
    }

    /* Lock the socket */
    EnterCriticalSection(&Socket->Lock);

    /* If the socket isn't connected, then fail */
    if (!(SockIsSocketConnected(Socket)) && !(Socket->AsyncData))
    {
        /* Fail */
        ErrorCode = WSAENOTCONN;
        goto error;
    }

    /* Check for valid length */
    if (Socket->SharedData.SizeOfRemoteAddress > *NameLength)
    {
        /* Fail */
        ErrorCode = WSAEFAULT;
        goto error;
    }

    /* Write the Address */
    RtlCopyMemory(Name,
                  Socket->RemoteAddress,
                  Socket->SharedData.SizeOfRemoteAddress);

    /* Return the Name Length */
    *NameLength = Socket->SharedData.SizeOfRemoteAddress;

error:

    /* Check if we have a socket here */
    if (Socket)
    {
        /* Release the lock and dereference */
        LeaveCriticalSection(&Socket->Lock);
        SockDereferenceSocket(Socket);
    }

    /* Check for error */
    if (ErrorCode != NO_ERROR)
    {
        /* Fail */
        *lpErrno = ErrorCode;
        return SOCKET_ERROR;
    }

    /* Return success */
    return NO_ERROR;
}
