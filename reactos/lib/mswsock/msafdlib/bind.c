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
WSPBind(SOCKET Handle, 
        const SOCKADDR *SocketAddress, 
        INT SocketAddressLength, 
        LPINT lpErrno)
{
    INT ErrorCode;
    IO_STATUS_BLOCK IoStatusBlock;
    PAFD_BIND_DATA BindData;
    PSOCKET_INFORMATION Socket;
    NTSTATUS Status;
    PTDI_ADDRESS_INFO TdiAddress = NULL;
    SOCKADDR_INFO SocketInfo;
    PWINSOCK_TEB_DATA ThreadData;
    CHAR AddressBuffer[FIELD_OFFSET(TDI_ADDRESS_INFO, Address) +
                       MAX_TDI_ADDRESS_LENGTH];
    ULONG BindDataLength, TdiAddressLength;

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

    /* If the socket is already bound, fail */
    if (Socket->SharedData.State != SocketOpen)
    {
        /* Fail */
        ErrorCode = WSAEINVAL;
        goto error;
    }

    /* Normalize address size */
    if (SocketAddressLength > Socket->HelperData->MaxWSAddressLength)
    {
        /* Don't go beyond the maximum */
        SocketAddressLength = Socket->HelperData->MaxWSAddressLength;
    }

    /* Get Address Information */
    ErrorCode = Socket->HelperData->WSHGetSockaddrType((PSOCKADDR)SocketAddress, 
                                                       SocketAddressLength, 
                                                       &SocketInfo);
    if (ErrorCode != NO_ERROR) goto error;

    /* Check how big the Bind and TDI Address Data will be */
    BindDataLength = Socket->HelperData->MaxTDIAddressLength + 
                     FIELD_OFFSET(AFD_BIND_DATA, Address);
    TdiAddressLength = Socket->HelperData->MaxTDIAddressLength + 
                       FIELD_OFFSET(TDI_ADDRESS_INFO, Address);

    /* Check if we can fit it in the stack */
    if ((TdiAddressLength <= sizeof(AddressBuffer)) && 
        (BindDataLength <= sizeof(AddressBuffer)))
    {
        /* Use the stack */
        TdiAddress = (PVOID)AddressBuffer;
        BindData = (PAFD_BIND_DATA)AddressBuffer;
    }
    else
    {
        /* Allocate from the heap */
        TdiAddress = SockAllocateHeapRoutine(SockPrivateHeap, 0, TdiAddressLength);
        if (!TdiAddress)
        {
            /* Fail */
            ErrorCode = WSAENOBUFS;
            goto error;
        }
        BindData = (PAFD_BIND_DATA)TdiAddress;
    }

    /* Set the Share Type */
    if (Socket->SharedData.ExclusiveAddressUse) 
    {
        BindData->ShareType = AFD_SHARE_EXCLUSIVE;
    } 
    else if (SocketInfo.EndpointInfo == SockaddrEndpointInfoWildcard) 
    {
        BindData->ShareType = AFD_SHARE_WILDCARD;
    } 
    else if (Socket->SharedData.ReuseAddresses) 
    {
        BindData->ShareType = AFD_SHARE_REUSE;
    } 
    else 
    {
        BindData->ShareType = AFD_SHARE_UNIQUE;
    }

    /* Build the TDI Address */
    ErrorCode = SockBuildTdiAddress(&BindData->Address,
                                    (PSOCKADDR)SocketAddress,
                                    SocketAddressLength);
    if (ErrorCode != NO_ERROR) goto error;

    /* Send IOCTL */
    Status = NtDeviceIoControlFile(Socket->WshContext.Handle,
                                   ThreadData->EventHandle,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_AFD_BIND,
                                   BindData,
                                   BindDataLength,
                                   TdiAddress,
                                   TdiAddressLength);

    /* Check if we need to wait */
    if (Status == STATUS_PENDING)
    {
        /* Wait for completion outside the lock */
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
    
    /* Save the TDI Address handle */
    Socket->TdiAddressHandle = (HANDLE)IoStatusBlock.Information;

    /* Notify the helper DLL */
    ErrorCode = SockNotifyHelperDll(Socket, WSH_NOTIFY_BIND);
    if (ErrorCode != NO_ERROR) goto error;

    /* Re-create Sockaddr format */
    ErrorCode = SockBuildSockaddr(Socket->LocalAddress,
                                  &SocketAddressLength,
                                  &TdiAddress->Address);
    if (ErrorCode != NO_ERROR) goto error;

    /* Set us as bound */
    Socket->SharedData.State = SocketBound;

    /* Send the new data to AFD */
    ErrorCode = SockSetHandleContext(Socket);
    if (ErrorCode != NO_ERROR) goto error;

    /* Update the window sizes */
    ErrorCode = SockUpdateWindowSizes(Socket, FALSE);
    if (ErrorCode != NO_ERROR) goto error;

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