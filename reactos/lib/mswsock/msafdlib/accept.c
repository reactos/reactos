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
SockCoreAccept(IN PSOCKET_INFORMATION Socket,
               IN PSOCKET_INFORMATION AcceptedSocket)
{
    INT ErrorCode, ReturnValue;
    BOOLEAN BlockMode = Socket->SharedData.NonBlocking;
    BOOLEAN Oob = Socket->SharedData.OobInline;
    ULONG HelperContextSize;
    PVOID HelperContext = NULL;
    HWND hWnd = 0;
    UINT wMsg = 0;
    HANDLE EventObject = NULL;
    ULONG AsyncEvents = 0, NetworkEvents = 0;
    CHAR HelperBuffer[256];

    /* Set the new state */
    AcceptedSocket->SharedData.State = SocketConnected;

    /* Copy some of the settings */
    AcceptedSocket->SharedData.LingerData = Socket->SharedData.LingerData;
    AcceptedSocket->SharedData.SizeOfRecvBuffer = Socket->SharedData.SizeOfRecvBuffer;
    AcceptedSocket->SharedData.SizeOfSendBuffer = Socket->SharedData.SizeOfSendBuffer;
    AcceptedSocket->SharedData.Broadcast = Socket->SharedData.Broadcast;
    AcceptedSocket->SharedData.Debug = Socket->SharedData.Debug;
    AcceptedSocket->SharedData.OobInline = Socket->SharedData.OobInline;
    AcceptedSocket->SharedData.ReuseAddresses = Socket->SharedData.ReuseAddresses;
    AcceptedSocket->SharedData.SendTimeout = Socket->SharedData.SendTimeout;
    AcceptedSocket->SharedData.RecvTimeout = Socket->SharedData.RecvTimeout;

    /* Check if the old socket had async select */
    if (Socket->SharedData.AsyncEvents)
    {
        /* Copy the data while we're still under the lock */
        AsyncEvents = Socket->SharedData.AsyncEvents;
        hWnd = Socket->SharedData.hWnd;
        wMsg = Socket->SharedData.wMsg;
    }
    else if (Socket->NetworkEvents)
    {
        /* Copy the data while we're still under the lock */
        NetworkEvents = Socket->NetworkEvents;
        EventObject = Socket->EventObject;
    }

    /* Check how much space is needed for the context */
    ReturnValue = Socket->HelperData->WSHGetSocketInformation(Socket->HelperContext,
                                                              Socket->Handle,
                                                              Socket->TdiAddressHandle,
                                                              Socket->TdiConnectionHandle,
                                                              SOL_INTERNAL,
                                                              SO_CONTEXT,
                                                              NULL,
                                                              &HelperContextSize);
    if (ReturnValue == NO_ERROR)
    {
        /* Check if our stack buffer is large enough to hold it */
        if (HelperContextSize <= sizeof(HelperBuffer))
        {
            /* Use it */
            HelperContext = (PVOID)HelperBuffer;
        }
        else
        {
            /* Allocate from the heap instead */
            HelperContext = SockAllocateHeapRoutine(SockPrivateHeap,
                                                    0,
                                                    HelperContextSize);
            if (!HelperContext)
            {
                /* Unlock the socket and fail */
                LeaveCriticalSection(&Socket->Lock);
                return WSAENOBUFS;
            }
        }

        /* Get the context */
        ReturnValue = Socket->HelperData->WSHGetSocketInformation(Socket->HelperContext,
                                                                  Socket->Handle,
                                                                  Socket->TdiAddressHandle,
                                                                  Socket->TdiConnectionHandle,
                                                                  SOL_INTERNAL,
                                                                  SO_CONTEXT,
                                                                  HelperContext,
                                                                  &HelperContextSize);
    }

    /* We're done with the old socket, so we can release the lock */
    LeaveCriticalSection(&Socket->Lock);

    /* Get the TDI Handles for the new socket */
    ErrorCode = SockGetTdiHandles(AcceptedSocket);
    
    /* Check if we have the handles and the context */
    if ((ErrorCode == NO_ERROR) && (ReturnValue == NO_ERROR))
    {
        /* Set the context */
        AcceptedSocket->HelperData->WSHGetSocketInformation(AcceptedSocket->HelperContext,
                                                            AcceptedSocket->Handle,
                                                            AcceptedSocket->TdiAddressHandle,
                                                            AcceptedSocket->TdiConnectionHandle,
                                                            SOL_INTERNAL,
                                                            SO_CONTEXT,
                                                            HelperContext,
                                                            &HelperContextSize);
    }

    /* Check if we should free from heap */
    if (HelperContext && (HelperContext != (PVOID)HelperBuffer))
    {
        /* Free it */
        RtlFreeHeap(SockPrivateHeap, 0, HelperContext);
    }

    /* Check if the old socket was non-blocking */
    if (BlockMode)
    {
        /* Set the new one like that too */
        ErrorCode = SockSetInformation(AcceptedSocket,
                                       AFD_INFO_BLOCKING_MODE,
                                       &BlockMode,
                                       NULL,
                                       NULL);
        if (ErrorCode != NO_ERROR) return ErrorCode;
    }

    /* Set it internally as well */
    AcceptedSocket->SharedData.NonBlocking = Socket->SharedData.NonBlocking;

    /* Check if inlined OOB was enabled */
    if (Oob)
    {
        /* Set the new one like that too */
        ErrorCode = SockSetInformation(AcceptedSocket,
                                       AFD_INFO_INLINING_MODE,
                                       &Oob,
                                       NULL,
                                       NULL);
        if (ErrorCode != NO_ERROR) return ErrorCode;
    }

    /* Set it internally as well */
    AcceptedSocket->SharedData.OobInline = Socket->SharedData.OobInline;

    /* Update the Window Sizes */
    ErrorCode = SockUpdateWindowSizes(AcceptedSocket, FALSE);
    if (ErrorCode != NO_ERROR) return ErrorCode;

    /* Check if async select was enabled */
    if (AsyncEvents)
    {
        /* Call WSPAsyncSelect on the accepted socket too */
        ErrorCode = SockAsyncSelectHelper(AcceptedSocket,
                                          hWnd,
                                          wMsg,
                                          AsyncEvents);
    }
    else if (NetworkEvents)
    {
        /* WSPEventSelect was enabled instead, call it on the new socket */
        ErrorCode = SockEventSelectHelper(AcceptedSocket,
                                          EventObject,
                                          NetworkEvents);
    }

    /* Check for failure */
    if (ErrorCode != NO_ERROR) return ErrorCode;

    /* Set the new context in AFD */
    ErrorCode = SockSetHandleContext(AcceptedSocket);
    if (ErrorCode != NO_ERROR) return ErrorCode;

    /* Return success*/
    return NO_ERROR;
}

SOCKET
WSPAPI 
WSPAccept(SOCKET Handle, 
          SOCKADDR FAR * SocketAddress, 
          LPINT SocketAddressLength, 
          LPCONDITIONPROC lpfnCondition, 
          DWORD_PTR dwCallbackData, 
          LPINT lpErrno)
{
    INT ErrorCode, ReturnValue;
    PSOCKET_INFORMATION Socket, AcceptedSocket = NULL;
    PWINSOCK_TEB_DATA ThreadData;
    CHAR AfdAcceptBuffer[32];
    PAFD_RECEIVED_ACCEPT_DATA ReceivedAcceptData = NULL;
    ULONG ReceiveBufferSize;
    FD_SET ReadFds;
    TIMEVAL Timeout;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG AddressBufferSize;
    CHAR AddressBuffer[sizeof(SOCKADDR)];
    PVOID SockAddress;
    ULONG ConnectDataSize;
    PVOID ConnectData = NULL;
    AFD_PENDING_ACCEPT_DATA PendingAcceptData;
    ULONG AddressSize;
    PVOID CalleeDataBuffer = NULL;
    WSABUF CallerId, CalleeId, CallerData, CalleeData;
    GROUP GroupId;
    LPQOS Qos = NULL, GroupQos = NULL;
    BOOLEAN ValidGroup = TRUE;
    AFD_DEFER_ACCEPT_DATA DeferData;
    ULONG BytesReturned;
    SOCKET AcceptedHandle = INVALID_SOCKET;
    AFD_ACCEPT_DATA AcceptData;

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

    /* Invalid for datagram sockets */
    if (MSAFD_IS_DGRAM_SOCK(Socket))
    {
        /* Fail */
        ErrorCode = WSAEOPNOTSUPP;
        goto error;
    }

    /* Only valid if the socket is listening */
    if (!Socket->SharedData.Listening)
    {
        /* Fail */
        ErrorCode = WSAEINVAL;
        goto error;
    }

    /* Validate address length */
    if (SocketAddressLength &&
        (Socket->HelperData->MinWSAddressLength > *SocketAddressLength))
    {
        /* Fail */
        ErrorCode = WSAEINVAL;
        goto error;
    }

    /* Calculate how much space we'll need for the Receive Buffer */
    ReceiveBufferSize = sizeof(AFD_RECEIVED_ACCEPT_DATA) + 
                        sizeof(TRANSPORT_ADDRESS) +
                        Socket->HelperData->MaxTDIAddressLength;

    /* Check if our stack buffer is large enough */
    if (ReceiveBufferSize <= sizeof(AfdAcceptBuffer))
    {
        /* Use the stack */
        ReceivedAcceptData = (PVOID)AfdAcceptBuffer;
    }
    else
    {
        /* Allocate from heap */
        ReceivedAcceptData = SockAllocateHeapRoutine(SockPrivateHeap,
                                                     0,
                                                     ReceiveBufferSize);
        if (!ReceivedAcceptData)
        {
            /* Fail */
            ErrorCode = WSAENOBUFS;
            goto error;
        }
    }

    /* If this is non-blocking, make sure there's something for us to accept */
    if (Socket->SharedData.NonBlocking)
    {
        /* Set up a nonblocking select */
        FD_ZERO(&ReadFds);
        FD_SET(Handle, &ReadFds);
        Timeout.tv_sec = 0;
        Timeout.tv_usec = 0;

        /* See if there's any data */
        ReturnValue = WSPSelect(1,
                                &ReadFds,
                                NULL,
                                NULL,
                                &Timeout,
                                lpErrno);
        if (ReturnValue == SOCKET_ERROR)
        {
            /* Fail */
            ErrorCode = *lpErrno;
            goto error;
        }
     
        /* Make sure we got a read back */
        if (!FD_ISSET(Handle, &ReadFds)) 
        {
            /* Fail */
            ErrorCode = WSAEWOULDBLOCK;
            goto error;
        }
    }

    /* Send IOCTL */
    Status = NtDeviceIoControlFile(Socket->WshContext.Handle,
                                   ThreadData->EventHandle,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_AFD_WAIT_FOR_LISTEN,
                                   NULL,
                                   0,
                                   ReceivedAcceptData,
                                   ReceiveBufferSize);
    /* Check if we need to wait */
    if (Status == STATUS_PENDING)
    {
        /* Wait for completion outside the lock */
        LeaveCriticalSection(&Socket->Lock);
        SockWaitForSingleObject(ThreadData->EventHandle,
                                Handle,
                                MAYBE_BLOCKING_HOOK,
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

    /* Check if we got a condition callback */
    if (lpfnCondition) 
    {
        /* Find out how much space we'll need for the address */
        AddressBufferSize = Socket->HelperData->MaxWSAddressLength;

        /* Check if our local buffer is enough */
        if (AddressBufferSize <= sizeof(AddressBuffer))
        {
            /* It is, use the stack */
            SockAddress = (PVOID)AddressBuffer;
        }
        else
        {
            /* Allocate from heap */
            SockAddress = SockAllocateHeapRoutine(SockPrivateHeap,
                                                  0,
                                                  AddressBufferSize);
            if (!SockAddress)
            {
                /* Fail */
                ErrorCode = WSAENOBUFS;
                goto error;
            }
        }

        /* Assume no connect data */
        ConnectDataSize = 0;

        /* Make sure we support connect data */
        if ((Socket->SharedData.ServiceFlags1 & XP1_CONNECT_DATA)) 
        {
            /* Find out how much data is pending */
            PendingAcceptData.SequenceNumber = ReceivedAcceptData->SequenceNumber;
            PendingAcceptData.ReturnSize = TRUE;

            /* Send IOCTL */
            Status = NtDeviceIoControlFile(Socket->WshContext.Handle,
                                           ThreadData->EventHandle,
                                           NULL,
                                           NULL,
                                           &IoStatusBlock,
                                           IOCTL_AFD_GET_PENDING_CONNECT_DATA,
                                           &PendingAcceptData,
                                           sizeof(PendingAcceptData),
                                           &PendingAcceptData,
                                           sizeof(PendingAcceptData));
            /* Check if we need to wait */
            if (Status == STATUS_PENDING)
            {
                /* Wait for completion */
                SockWaitForSingleObject(ThreadData->EventHandle,
                                        Handle,
                                        NO_BLOCKING_HOOK,
                                        NO_TIMEOUT);
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

            /* How much data to allocate */
            ConnectDataSize = PtrToUlong(IoStatusBlock.Information);
            if (ConnectDataSize)
            {
                /* Allocate needed space */
                ConnectData = SockAllocateHeapRoutine(SockPrivateHeap,
                                                      0,
                                                      ConnectDataSize);
                if (!ConnectData)
                {
                    /* Fail */
                    ErrorCode = WSAENOBUFS;
                    goto error;
                }

                /* Setup the structure to actually get the data now */
                PendingAcceptData.SequenceNumber = ReceivedAcceptData->SequenceNumber;
                PendingAcceptData.ReturnSize = FALSE;

                /* Send IOCTL */
                Status = NtDeviceIoControlFile(Socket->WshContext.Handle,
                                               ThreadData->EventHandle,
                                               NULL,
                                               NULL,
                                               &IoStatusBlock,
                                               IOCTL_AFD_GET_PENDING_CONNECT_DATA,
                                               &PendingAcceptData,
                                               sizeof(PendingAcceptData),
                                               ConnectData,
                                               ConnectDataSize);
                /* Check if we need to wait */
                if (Status == STATUS_PENDING)
                {
                    /* Wait for completion */
                    SockWaitForSingleObject(ThreadData->EventHandle,
                                            Handle,
                                            NO_BLOCKING_HOOK,
                                            NO_TIMEOUT);
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
            }
        }

        if ((Socket->SharedData.ServiceFlags1 & XP1_QOS_SUPPORTED)) 
        {
            /* Set the accept data */
            AcceptData.SequenceNumber = ReceivedAcceptData->SequenceNumber;
            AcceptData.ListenHandle = Socket->WshContext.Handle;

            /* Save it in the TEB */
            ThreadData->AcceptData = &AcceptData;

            /* Send the IOCTL to get QOS Size */
            BytesReturned = 0;
            ReturnValue = WSPIoctl(Handle,
                                   SIO_GET_QOS,
                                   NULL,
                                   0,
                                   NULL,
                                   0,
                                   &BytesReturned,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &ErrorCode);

            /* Check if it failed (it should) */
            if (ReturnValue == SOCKET_ERROR)
            {
                /* Check if it failed because it had no buffer (it should) */
                if (ErrorCode == WSAEFAULT)
                {
                    /* Make sure it told us how many bytes it needed */
                    if (BytesReturned)
                    {
                        /* Allocate memory for it */
                        Qos = SockAllocateHeapRoutine(SockPrivateHeap,
                                                      0,
                                                      BytesReturned);
                        if (!Qos)
                        {
                            /* Fail */
                            ErrorCode = WSAENOBUFS;
                            goto error;
                        }

                        /* Save the accept data and set the QoS */
                        ThreadData->AcceptData = &AcceptData;
                        ReturnValue = WSPIoctl(Handle,
                                               SIO_GET_QOS,
                                               NULL,
                                               0,
                                               Qos,
                                               BytesReturned,
                                               &BytesReturned,
                                               NULL,
                                               NULL,
                                               NULL,
                                               &ErrorCode);
                    }
                }
                else
                {
                    /* We got some other weird, error, fail. */
                    goto error;
                }
            }
        
            /* Save the accept in the TEB */
            ThreadData->AcceptData = &AcceptData;

            /* Send the IOCTL to get Group QOS Size */
            BytesReturned = 0;
            ReturnValue = WSPIoctl(Handle,
                                   SIO_GET_GROUP_QOS,
                                   NULL,
                                   0,
                                   NULL,
                                   0,
                                   &BytesReturned,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &ErrorCode);

            /* Check if it failed (it should) */
            if (ReturnValue == SOCKET_ERROR)
            {
                /* Check if it failed because it had no buffer (it should) */
                if (ErrorCode == WSAEFAULT)
                {
                    /* Make sure it told us how many bytes it needed */
                    if (BytesReturned)
                    {
                        /* Allocate memory for it */
                        GroupQos = SockAllocateHeapRoutine(SockPrivateHeap,
                                                           0,
                                                           BytesReturned);
                        if (!GroupQos)
                        {
                            /* Fail */
                            ErrorCode = WSAENOBUFS;
                            goto error;
                        }

                        /* Save the accept data and set the QoS */
                        ThreadData->AcceptData = &AcceptData;
                        ReturnValue = WSPIoctl(Handle,
                                               SIO_GET_QOS,
                                               NULL,
                                               0,
                                               GroupQos,
                                               BytesReturned,
                                               &BytesReturned,
                                               NULL,
                                               NULL,
                                               NULL,
                                               &ErrorCode);
                    }
                }
                else
                {
                    /* We got some other weird, error, fail. */
                    goto error;
                }
            }
        }
        
        /* Build Callee ID */
        CalleeId.buf = (PVOID)Socket->LocalAddress;
        CalleeId.len = Socket->SharedData.SizeOfLocalAddress;

        /* Set up Address in SOCKADDR Format */
        SockBuildSockaddr((PSOCKADDR)SockAddress,
                          &AddressSize,
                          &ReceivedAcceptData->Address);

        /* Build Caller ID */
        CallerId.buf = (PVOID)SockAddress;
        CallerId.len = AddressSize;

        /* Build Caller Data */
        CallerData.buf = ConnectData;
        CallerData.len = ConnectDataSize;

        /* Check if socket supports Conditional Accept */
        if (Socket->SharedData.UseDelayedAcceptance) 
        {
            /* Allocate Buffer for Callee Data */
            CalleeDataBuffer = SockAllocateHeapRoutine(SockPrivateHeap, 0, 4096);
            if (CalleeDataBuffer)
            {
                /* Fill the structure */
                CalleeData.buf = CalleeDataBuffer;
                CalleeData.len = 4096;
            }
            else
            {
                /* Don't fail, just don't use this... */
                CalleeData.len = 0;
            }
        } 
        else 
        {
            /* Nothing */
            CalleeData.buf = NULL;
            CalleeData.len = 0;
        }
    
        /* Call the Condition Function */
        ReturnValue = (lpfnCondition)(&CallerId,
                                      !CallerData.buf ? NULL : & CallerData,
                                      NULL,
                                      NULL,
                                      &CalleeId,
                                      !CalleeData.buf ? NULL: & CalleeData,
                                      &GroupId,
                                      dwCallbackData);

        if ((ReturnValue == CF_ACCEPT) &&
            (GroupId) &&
            (GroupId != SG_UNCONSTRAINED_GROUP) &&
            (GroupId != SG_CONSTRAINED_GROUP))
        {
            /* Check for validity */
            ErrorCode = SockIsAddressConsistentWithConstrainedGroup(Socket,
                                                                    GroupId,
                                                                    SockAddress,
                                                                    AddressSize);
            ValidGroup = (ErrorCode == NO_ERROR);
        }

        /* Check if the address was from the heap */
        if (SockAddress != AddressBuffer)
        {
            /* Free it */
            RtlFreeHeap(SockPrivateHeap, 0, SockAddress);
        }

        /* Check if it was accepted */
        if (ReturnValue == CF_ACCEPT)
        {
            /* Check if the group is invalid, however */
            if (!ValidGroup) goto error;

            /* Now check if QOS is supported */
            if ((Socket->SharedData.ServiceFlags1 & XP1_QOS_SUPPORTED)) 
            {
                /* Check if we had Qos */
                if (Qos)
                {
                    /* Set the accept data */
                    AcceptData.SequenceNumber = ReceivedAcceptData->SequenceNumber;
                    AcceptData.ListenHandle = Socket->WshContext.Handle;

                    /* Save it in the TEB */
                    ThreadData->AcceptData = &AcceptData;

                    /* Send the IOCTL */
                    BytesReturned = 0;
                    ReturnValue = WSPIoctl(Handle,
                                           SIO_SET_QOS,
                                           Qos,
                                           sizeof(*Qos),
                                           NULL,
                                           0,
                                           &BytesReturned,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &ErrorCode);
                    if (ReturnValue == SOCKET_ERROR) goto error;
                }

                /* Check if we had Group Qos */
                if (GroupQos)
                {
                    /* Set the accept data */
                    AcceptData.SequenceNumber = ReceivedAcceptData->SequenceNumber;
                    AcceptData.ListenHandle = Socket->WshContext.Handle;

                    /* Save it in the TEB */
                    ThreadData->AcceptData = &AcceptData;

                    /* Send the IOCTL */
                    ReturnValue = WSPIoctl(Handle,
                                           SIO_SET_GROUP_QOS,
                                           GroupQos,
                                           sizeof(*GroupQos),
                                           NULL,
                                           0,
                                           &BytesReturned,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &ErrorCode);
                    if (ReturnValue == SOCKET_ERROR) goto error;
                }
            }

            /* Check if delayed acceptance is used and we have callee data */
            if ((Socket->HelperData->UseDelayedAcceptance) && (CalleeData.len))
            {
                /* Save the accept data in the TEB */
                ThreadData->AcceptData = &AcceptData;

                /* Set the connect data */
                ErrorCode = SockGetConnectData(Socket,
                                               IOCTL_AFD_SET_CONNECT_DATA,
                                               CalleeData.buf,
                                               CalleeData.len,
                                               NULL);
                if (ErrorCode == SOCKET_ERROR) goto error;
            }
        } 
        else 
        {
            /* Callback rejected. Build Defer Structure */
            DeferData.SequenceNumber = ReceivedAcceptData->SequenceNumber;
            DeferData.RejectConnection = (ReturnValue == CF_REJECT);

            /* Send IOCTL */
            Status = NtDeviceIoControlFile(Socket->WshContext.Handle,
                                           ThreadData->EventHandle,
                                           NULL,
                                           NULL,
                                           &IoStatusBlock,
                                           IOCTL_AFD_DEFER_ACCEPT,
                                           &DeferData,
                                           sizeof(DeferData),
                                           NULL,
                                           0);
            /* Check for error */
            if (!NT_SUCCESS(Status))
            {
                /* Fail */
                ErrorCode = NtStatusToSocketError(Status);
                goto error;
            }

            if (ReturnValue == CF_REJECT) 
            {
                /* The connection was refused */
                ErrorCode = WSAECONNREFUSED;
            } 
            else 
            {
                /* The connection was deferred */
                ErrorCode = WSATRY_AGAIN;
            }

            /* Fail */
            goto error;
        }
    }

    /* Create a new Socket */
    ErrorCode = SockSocket(Socket->SharedData.AddressFamily,
                           Socket->SharedData.SocketType,
                           Socket->SharedData.Protocol,
                           &Socket->ProviderId,
                           GroupId,
                           Socket->SharedData.CreateFlags,
                           Socket->SharedData.ProviderFlags,
                           Socket->SharedData.ServiceFlags1,
                           Socket->SharedData.CatalogEntryId,
                           &AcceptedSocket);
    if (ErrorCode != NO_ERROR)
    {
        /* Fail */
        goto error;
    }

    /* Set up the Accept Structure */
    AcceptData.ListenHandle = AcceptedSocket->WshContext.Handle;
    AcceptData.SequenceNumber = ReceivedAcceptData->SequenceNumber;

    /* Build the socket address */
    SockBuildSockaddr(AcceptedSocket->RemoteAddress,
                      &AcceptedSocket->SharedData.SizeOfRemoteAddress,
                      &ReceivedAcceptData->Address);

    /* Copy the local address */
    RtlCopyMemory(AcceptedSocket->LocalAddress,
                  Socket->LocalAddress,
                  Socket->SharedData.SizeOfLocalAddress);
    AcceptedSocket->SharedData.SizeOfLocalAddress = Socket->SharedData.SizeOfLocalAddress;

    /* We can release the accepted socket's lock now */
    LeaveCriticalSection(&AcceptedSocket->Lock);

    /* Send IOCTL to Accept */
    AcceptData.UseSAN = SockSanEnabled;
    Status = NtDeviceIoControlFile(Socket->WshContext.Handle,
                                   ThreadData->EventHandle,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_AFD_ACCEPT,
                                   &AcceptData,
                                   sizeof(AcceptData),
                                   NULL,
                                   0);
    /* Check if we need to wait */
    if (Status == STATUS_PENDING)
    {
        /* Wait for completion outside the lock */
        LeaveCriticalSection(&Socket->Lock);
        SockWaitForSingleObject(ThreadData->EventHandle,
                                Handle,
                                MAYBE_BLOCKING_HOOK,
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

    /* Notify the helper DLL */
    ErrorCode = SockNotifyHelperDll(AcceptedSocket, WSH_NOTIFY_ACCEPT);
    if (ErrorCode != NO_ERROR) goto error;

    /* If the caller sent a socket address pointer and length */
    if (SocketAddress && SocketAddressLength)
    {
        /* Return the address in its buffer */
        ErrorCode = SockBuildSockaddr(SocketAddress,
                                      SocketAddressLength,
                                      &ReceivedAcceptData->Address);
        if (ErrorCode != NO_ERROR) goto error;
    }

    /* Check if async select was active */
    if (SockAsyncSelectCalled)
    {
        /* Re-enable the regular accept event */
        SockReenableAsyncSelectEvent(Socket, FD_ACCEPT);
    }

    /* Finally, do the internal core accept code */
    ErrorCode = SockCoreAccept(Socket, AcceptedSocket);
    if (ErrorCode != NO_ERROR) goto error;

    /* Call WPU to tell it about the new handle */
    AcceptedHandle = SockUpcallTable->lpWPUModifyIFSHandle(AcceptedSocket->SharedData.CatalogEntryId,
                                                           (SOCKET)AcceptedSocket->WshContext.Handle,
                                                           &ErrorCode);

    /* Dereference the socket and clear its pointer for error code logic */
    SockDereferenceSocket(Socket);
    Socket = NULL;

error:

    /* Check if we have a socket here */
    if (Socket)
    {
        /* Check if async select was active */
        if (SockAsyncSelectCalled)
        {
            /* Re-enable the regular accept event */
            SockReenableAsyncSelectEvent(Socket, FD_ACCEPT);
        }

        /* Unlock and dereference it */
        LeaveCriticalSection(&Socket->Lock);
        SockDereferenceSocket(Socket);
    }

    /* Check if we got the accepted socket */
    if (AcceptedSocket)
    {
        /* Check if the accepted socket also has a handle */
        if (ErrorCode == NO_ERROR)
        {
            /* Close the socket */
            SockCloseSocket(AcceptedSocket);
        }

        /* Dereference it */
        SockDereferenceSocket(AcceptedSocket);
    }

    /* Check if the accept buffer was from the heap */
    if (ReceivedAcceptData && (ReceivedAcceptData != (PVOID)AfdAcceptBuffer))
    {
        /* Free it */
        RtlFreeHeap(SockPrivateHeap, 0, ReceivedAcceptData);
    }

    /* Check if we have a connect data buffer */
    if (ConnectData)
    {
        /* Free it */
        RtlFreeHeap(SockPrivateHeap, 0, ConnectData);
    }

    /* Check if we have a callee data buffer */
    if (CalleeDataBuffer)
    {
        /* Free it */
        RtlFreeHeap(SockPrivateHeap, 0, CalleeDataBuffer);
    }

    /* Check if we have allocated QOS structures */
    if (Qos)
    {
        /* Free it */
        RtlFreeHeap(SockPrivateHeap, 0, Qos);
    }
    if (GroupQos)
    {
        /* Free it */
        RtlFreeHeap(SockPrivateHeap, 0, GroupQos);
    }   

     /* Check for error */
    if (ErrorCode != NO_ERROR)
    {
        /* Fail */
        *lpErrno = ErrorCode;
        return INVALID_SOCKET;
    }

    /* Return the new handle */
    return AcceptedHandle;
}
