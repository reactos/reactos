/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 SPI
 * FILE:        lib/mswsock/lib/init.c
 * PURPOSE:     DLL Initialization
 */

/* INCLUDES ******************************************************************/
#include "msafd.h"

/* DATA **********************************************************************/

SOCK_RW_LOCK SocketGlobalLock;

/* FUNCTIONS *****************************************************************/

VOID
WSPAPI
SockDestroySocket(PSOCKET_INFORMATION Socket)
{
    /* Dereference its helper DLL */
    SockDereferenceHelperDll(Socket->HelperData);

    /* Delete the lock */
    DeleteCriticalSection(&Socket->Lock);

    /* Free the socket */
    RtlFreeHeap(SockPrivateHeap, 0, Socket);
}

VOID
__inline
WSPAPI
SockDereferenceSocket(IN PSOCKET_INFORMATION Socket)
{
    /* Dereference and see if it's the last count */
    if (!InterlockedDecrement(&Socket->WshContext.RefCount))
    {
        /* Destroy the socket */
        SockDestroySocket(Socket);
    }
}

PSOCKET_INFORMATION
WSPAPI
SockImportHandle(IN SOCKET Handle)
{
    PWINSOCK_TEB_DATA ThreadData = NtCurrentTeb()->WinSockData;
    PWAH_HANDLE WahHandle;
    NTSTATUS Status;
    ULONG ContextSize;
    IO_STATUS_BLOCK IoStatusBlock;
    PSOCKET_INFORMATION ImportedSocket = NULL;
    UNICODE_STRING TransportName;

    /* Acquire the global lock */
    SockAcquireRwLockExclusive(&SocketGlobalLock);

    /* Make sure that the handle is still invalid */
    WahHandle = WahReferenceContextByHandle(SockContextTable, (HANDLE)Handle);
    if (WahHandle)
    {
        /* Some other thread imported it by now, release the lock and return */
        SockReleaseRwLockExclusive(&SocketGlobalLock);
        return (PSOCKET_INFORMATION)WahHandle;
    }

    /* Setup the NULL name for possible cleanup later */
    RtlInitUnicodeString(&TransportName, NULL);

    /* Call AFD to get the context size */
    Status = NtDeviceIoControlFile((HANDLE)Handle,
                                   ThreadData->EventHandle,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_AFD_GET_CONTEXT_SIZE,
                                   NULL,
                                   0,
                                   &ContextSize,
                                   sizeof(ContextSize));
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

    /* Make sure we didn't fail, and that this is a valid context */
    if (!NT_SUCCESS(Status) || (ContextSize < sizeof(SOCK_SHARED_INFO)))
    {
        /* Fail (the error handler will convert to Win32 Status) */
        goto error;
    }

error:
    /* Release the lock */
    SockReleaseRwLockExclusive(&SocketGlobalLock);

    return ImportedSocket;
}

INT
WSPAPI
SockSetInformation(IN PSOCKET_INFORMATION Socket, 
                   IN ULONG AfdInformationClass, 
                   IN PBOOLEAN Boolean OPTIONAL,
                   IN PULONG Ulong OPTIONAL, 
                   IN PLARGE_INTEGER LargeInteger OPTIONAL)
{
    IO_STATUS_BLOCK IoStatusBlock;
    AFD_INFO AfdInfo;
    NTSTATUS Status;
    PWINSOCK_TEB_DATA ThreadData = NtCurrentTeb()->WinSockData;

    /* Set Info Class */
    AfdInfo.InformationClass = AfdInformationClass;

    /* Set Information */
    if (Boolean) 
    {
        AfdInfo.Information.Boolean = *Boolean;
    }
    else if (Ulong)
    {
        AfdInfo.Information.Ulong = *Ulong;
    }
    else
    {
        AfdInfo.Information.LargeInteger = *LargeInteger;
    }

    /* Send IOCTL */
    Status = NtDeviceIoControlFile(Socket->WshContext.Handle,
                                   ThreadData->EventHandle,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_AFD_GET_INFO,
                                   &AfdInfo,
                                   sizeof(AfdInfo),
                                   NULL,
                                   0);
    /* Check if we have to wait */
    if (Status == STATUS_PENDING)
    {
        /* Wait for the operation to finish */
        SockWaitForSingleObject(ThreadData->EventHandle,
                                Socket->Handle,
                                NO_BLOCKING_HOOK,
                                NO_TIMEOUT);

        /* Get new status */
        Status = IoStatusBlock.Status;
    }

    /* Handle failure */
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        return NtStatusToSocketError(Status);
    }

    /* Return to caller */
    return NO_ERROR;
}

INT
WSPAPI
SockGetInformation(IN PSOCKET_INFORMATION Socket, 
                   IN ULONG AfdInformationClass, 
                   IN PVOID ExtraData OPTIONAL,
                   IN ULONG ExtraDataSize,
                   IN OUT PBOOLEAN Boolean OPTIONAL,
                   IN OUT PULONG Ulong OPTIONAL, 
                   IN OUT PLARGE_INTEGER LargeInteger OPTIONAL)
{
    ULONG InfoLength;
    IO_STATUS_BLOCK IoStatusBlock;
    PAFD_INFO AfdInfo;
    AFD_INFO InfoData;
    NTSTATUS Status;
    PWINSOCK_TEB_DATA ThreadData = NtCurrentTeb()->WinSockData;

    /* Check if extra data is there */
    if (ExtraData && ExtraDataSize)
    {
        /* Allocate space for it */
        InfoLength = sizeof(InfoData) + ExtraDataSize;
        AfdInfo = (PAFD_INFO)SockAllocateHeapRoutine(SockPrivateHeap,
                                                     0,
                                                     InfoLength);
        if (!AfdInfo) return WSAENOBUFS;

        /* Copy the extra data */
        RtlCopyMemory(AfdInfo + 1, ExtraData, ExtraDataSize);
    }
    else
    {
        /* Use local buffer */
        AfdInfo = &InfoData;
        InfoLength = sizeof(InfoData);
    }

    /* Set Info Class */
    AfdInfo->InformationClass = AfdInformationClass;

    /* Send IOCTL */
    Status = NtDeviceIoControlFile(Socket->WshContext.Handle,
                                   ThreadData->EventHandle,
                                   NULL,
                                   NULL,    
                                   &IoStatusBlock,
                                   IOCTL_AFD_GET_INFO,
                                   &InfoData,
                                   InfoLength,
                                   &InfoData,
                                   sizeof(InfoData));
    /* Check if we have to wait */
    if (Status == STATUS_PENDING)
    {
        /* Wait for the operation to finish */
        SockWaitForSingleObject(ThreadData->EventHandle,
                                Socket->Handle,
                                NO_BLOCKING_HOOK,
                                NO_TIMEOUT);

        /* Get new status */
        Status = IoStatusBlock.Status;
    }

    /* Handle failure */
    if (!NT_SUCCESS(Status))
    {
        /* Check if we have to free the data */
        if (AfdInfo != &InfoData) RtlFreeHeap(SockPrivateHeap, 0, AfdInfo);

        /* Fail */
        return NtStatusToSocketError(Status);
    }

    /* Return Information */
    if (Boolean) 
    {
        *Boolean = AfdInfo->Information.Boolean;
    }
    else if (Ulong)
    {
        *Ulong = AfdInfo->Information.Ulong;
    }
    else
    {
        *LargeInteger = AfdInfo->Information.LargeInteger;
    }

    /* Check if we have to free the data */
    if (AfdInfo != &InfoData) RtlFreeHeap(SockPrivateHeap, 0, AfdInfo);

    /* Return to caller */
    return NO_ERROR;
}

INT
WSPAPI
SockSetHandleContext(IN PSOCKET_INFORMATION Socket)
{
	IO_STATUS_BLOCK IoStatusBlock;
	CHAR ContextData[256];
    PWINSOCK_TEB_DATA ThreadData = NtCurrentTeb()->WinSockData;
    PVOID Context;
    ULONG_PTR ContextPos;
    ULONG ContextLength, HelperContextLength;
    INT ErrorCode;
	NTSTATUS Status;

    /* Find out how big the helper DLL context is */
    ErrorCode = Socket->HelperData->WSHGetSocketInformation(Socket->HelperContext,
                                                            Socket->Handle,
                                                            Socket->TdiAddressHandle,
                                                            Socket->TdiConnectionHandle,
                                                            SOL_INTERNAL,
                                                            SO_CONTEXT,
                                                            NULL,
                                                            &HelperContextLength);

    /* Calculate the total space needed */
    ContextLength = sizeof(SOCK_SHARED_INFO) +
                    2 * Socket->HelperData->MaxWSAddressLength +
                    sizeof(ULONG) + HelperContextLength;

    /* See if our stack can hold it */
    if (ContextLength <= sizeof(ContextData))
    {
        /* Use our stack */
        Context = ContextData;
    }
    else
    {
        /* Allocate from heap */
        Context = SockAllocateHeapRoutine(SockPrivateHeap, 0, ContextLength);
        if (!Context) return WSAENOBUFS;
    }

	/* 
     * Create Context, this includes:
     * Shared Socket Data, Helper Context Length, Local and Remote Addresses
     * and finally the actual helper context.
     */
    ContextPos = (ULONG_PTR)Context;
    RtlCopyMemory((PVOID)ContextPos,
                  &Socket->SharedData,
                  sizeof(SOCK_SHARED_INFO));
    ContextPos += sizeof(SOCK_SHARED_INFO);
    *(PULONG)ContextPos = HelperContextLength;
    ContextPos += sizeof(ULONG);
    RtlCopyMemory((PVOID)ContextPos,
                  Socket->LocalAddress,
                  Socket->HelperData->MaxWSAddressLength);
    ContextPos += Socket->HelperData->MaxWSAddressLength;
    RtlCopyMemory((PVOID)ContextPos,
                  Socket->RemoteAddress,
                  Socket->HelperData->MaxWSAddressLength);
    ContextPos += Socket->HelperData->MaxWSAddressLength;

    /* Now get the helper context */
    ErrorCode = Socket->HelperData->WSHGetSocketInformation(Socket->HelperContext,
                                                            Socket->Handle,
                                                            Socket->TdiAddressHandle,
                                                            Socket->TdiConnectionHandle,
                                                            SOL_INTERNAL,
                                                            SO_CONTEXT,
                                                            (PVOID)ContextPos,
                                                            &HelperContextLength);
	/* Now give it to AFD  */
	Status = NtDeviceIoControlFile(Socket->WshContext.Handle,
                                   ThreadData->EventHandle,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_AFD_SET_CONTEXT,
                                   Context,
                                   ContextLength,
                                   NULL,
                                   0);
	/* Check if we need to wait */
	if (Status == STATUS_PENDING)
    {
		/* Wait for completion */
        SockWaitForSingleObject(ThreadData->EventHandle,
                                Socket->Handle,
                                NO_BLOCKING_HOOK,
                                NO_TIMEOUT);

        /* Get new status */
        Status = IoStatusBlock.Status;
	}

    /* Check if we need to free from heap */
    if (Context != ContextData) RtlFreeHeap(SockPrivateHeap, 0, Context);

    /* Check for error */
	if (!NT_SUCCESS(Status))
    {
        /* Convert and return error code */
        ErrorCode = NtStatusToSocketError(Status);
        return ErrorCode;
    }

    /* Return success */
    return NO_ERROR;
}

INT
WSPAPI
SockIsAddressConsistentWithConstrainedGroup(IN PSOCKET_INFORMATION Socket,
                                            IN GROUP Group,
                                            IN PSOCKADDR SocketAddress,
                                            IN INT SocketAddressLength)
{
    PWINSOCK_TEB_DATA ThreadData = NtCurrentTeb()->WinSockData;
    INT ErrorCode;
    PAFD_VALIDATE_GROUP_DATA ValidateGroupData;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG ValidateGroupSize;
    CHAR ValidateBuffer[sizeof(AFD_VALIDATE_GROUP_DATA) + MAX_TDI_ADDRESS_LENGTH];

    /* Calculate the length of the buffer */
    ValidateGroupSize = sizeof(AFD_VALIDATE_GROUP_DATA) +
                        sizeof(TRANSPORT_ADDRESS) +
                        Socket->HelperData->MaxTDIAddressLength;

    /* Check if our stack buffer is large enough */
    if (ValidateGroupSize <= sizeof(ValidateBuffer))
    {
        /* Use the stack */
        ValidateGroupData = (PVOID)ValidateBuffer;
    }
    else
    {
        /* Allocate from heap */
        ValidateGroupData = SockAllocateHeapRoutine(SockPrivateHeap,
                                                    0,
                                                    ValidateGroupSize);
        if (!ValidateGroupData) return WSAENOBUFS;
    }

    /* Convert the address to TDI format */
    ErrorCode = SockBuildTdiAddress(&ValidateGroupData->Address,
                                    SocketAddress,
                                    SocketAddressLength);
    if (ErrorCode != NO_ERROR) return ErrorCode;

    /* Tell AFD which group to check, and let AFD validate it */
    ValidateGroupData->GroupId = Group;
    Status = NtDeviceIoControlFile(Socket->WshContext.Handle,
                                   ThreadData->EventHandle,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_AFD_VALIDATE_GROUP,
                                   ValidateGroupData,
                                   ValidateGroupSize,
                                   NULL,
                                   0);

    /* Check if we need to wait */
    if (Status == STATUS_PENDING)
    {
        /* Wait for completion */
        SockWaitForSingleObject(ThreadData->EventHandle,
                                Socket->Handle,
                                NO_BLOCKING_HOOK,
                                NO_TIMEOUT);

        /* Get new status */
        Status = IoStatusBlock.Status;
    }

    /* Check if we need to free the data from heap */
    if (ValidateGroupData != (PVOID)ValidateBuffer)
    {
        /* Free it */
        RtlFreeHeap(SockPrivateHeap, 0, ValidateGroupData);
    }

    /* Check for error */
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        return NtStatusToSocketError(Status);
    }

    /* Return success */
    return NO_ERROR;
}


INT
WSPAPI
SockGetTdiHandles(IN PSOCKET_INFORMATION Socket)
{
    AFD_TDI_HANDLE_DATA TdiHandleInfo;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG InfoType = 0;
    PWINSOCK_TEB_DATA ThreadData = NtCurrentTeb()->WinSockData;

    /* See which handle(s) we need */
    if (!Socket->TdiAddressHandle) InfoType |= AFD_ADDRESS_HANDLE;
    if (!Socket->TdiConnectionHandle) InfoType |= AFD_CONNECTION_HANDLE;

    /* Make sure we need one */
    if (!InfoType) return NO_ERROR;

    /* Call AFD */
    Status = NtDeviceIoControlFile(Socket->WshContext.Handle,
                                   ThreadData->EventHandle,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_AFD_GET_TDI_HANDLES,
                                   &InfoType,
                                   sizeof(InfoType),
                                   &TdiHandleInfo,
                                   sizeof(TdiHandleInfo));
    /* Check if we shoudl wait */
    if (Status == STATUS_PENDING)
    {
        /* Wait on it */
        SockWaitForSingleObject(ThreadData->EventHandle,
                                Socket->Handle,
                                NO_BLOCKING_HOOK,
                                NO_TIMEOUT);

        /* Update status */
        Status = IoStatusBlock.Status;
    }

    /* Check for success */
    if (!NT_SUCCESS(Status)) return NtStatusToSocketError(Status);

    /* Return handles */
    if (!Socket->TdiAddressHandle)
    {
        Socket->TdiAddressHandle = TdiHandleInfo.TdiAddressHandle;
    }
    if (!Socket->TdiConnectionHandle)
    {
        Socket->TdiConnectionHandle = TdiHandleInfo.TdiConnectionHandle;
    }

    /* Return */
    return NO_ERROR;
}

BOOL
WSPAPI
SockWaitForSingleObject(IN HANDLE Handle,
                        IN SOCKET SocketHandle,
                        IN DWORD BlockingFlags,
                        IN DWORD TimeoutFlags)
{
    LARGE_INTEGER Timeout, CurrentTime, DueTime;
    NTSTATUS Status;
    PSOCKET_INFORMATION Socket;
    BOOLEAN CallHook, UseTimeout;
    LPBLOCKINGCALLBACK BlockingHook;
    DWORD_PTR Context;
    INT ErrorCode;
    PWINSOCK_TEB_DATA ThreadData = NtCurrentTeb()->WinSockData;

    /* Start with a simple 0.5 second wait */
    Timeout.QuadPart = Int32x32To64(-10000, 500);
    Status = NtWaitForSingleObject(Handle, TRUE, &Timeout);
    if (Status == STATUS_SUCCESS) return TRUE;

    /* Check if our flags require the socket structure */
    if ((BlockingFlags == MAYBE_BLOCKING_HOOK) ||
        (BlockingFlags == ALWAYS_BLOCKING_HOOK) ||
        (TimeoutFlags == SEND_TIMEOUT) ||
        (TimeoutFlags == RECV_TIMEOUT))
    {
        /* Get the socket */
        Socket = SockFindAndReferenceSocket(SocketHandle, FALSE);
        if (!Socket)
        {
            /* We must be waiting on a non-socket for some reason? */
            NtWaitForSingleObject(Handle, TRUE, NULL);
            return TRUE;
        }
    }

    /* Check the blocking flags */
    if (BlockingFlags == ALWAYS_BLOCKING_HOOK)
    {
        /* Always call it */
        CallHook = TRUE;
    }
    else if (BlockingFlags == MAYBE_BLOCKING_HOOK)
    {
        /* Check if we have to call it */
        CallHook = !Socket->SharedData.NonBlocking;
    }
    else
    {
        /* Never call it*/
        CallHook = FALSE;
    }

    /* Check if we call it */
    if (CallHook)
    {
        /* Check if it actually exists */
        SockUpcallTable->lpWPUQueryBlockingCallback(Socket->SharedData.CatalogEntryId,
                                                    &BlockingHook,
                                                    &Context,
                                                    &ErrorCode);

        /* See if we'll call it */
        CallHook = (BlockingHook != NULL);
    }

    /* Now check the timeout flags */
    if (TimeoutFlags == NO_TIMEOUT)
    {
        /* None at all */
        UseTimeout = FALSE;
    }
    else if (TimeoutFlags == SEND_TIMEOUT)
    {
        /* See if there's a Send Timeout */
        if (Socket->SharedData.SendTimeout)
        {
            /* Use it */
            UseTimeout = TRUE;
            Timeout = RtlEnlargedIntegerMultiply(Socket->SharedData.SendTimeout,
                                                 10 * 1000);
        }
        else
        {
            /* There isn't any */
            UseTimeout = FALSE;
        }
    }
    else
    {
        /* See if there's a Receive Timeout */
        if (Socket->SharedData.RecvTimeout)
        {
            /* Use it */
            UseTimeout = TRUE;
            Timeout = RtlEnlargedIntegerMultiply(Socket->SharedData.RecvTimeout,
                                                 10 * 1000);
        }
        else
        {
            /* There isn't any */
            UseTimeout = FALSE;
        }
    }

    /* We don't need the socket anymore */
    if (Socket) SockDereferenceSocket(Socket);

    /* Check for timeout */
    if (UseTimeout)
    {
        /* Calculate the absolute time when the wait ends */
        Status = NtQuerySystemTime(&CurrentTime);
        DueTime.QuadPart = CurrentTime.QuadPart + Timeout.QuadPart;
    }
    else
    {
        /* Infinite wait */
        DueTime.LowPart = -1;
        DueTime.HighPart = 0x7FFFFFFF;
    }

    /* Check for blocking hook call */
    if (CallHook)
    {
        /* We're calling it, so we won't actually be waiting */
        Timeout.LowPart = -1;
        Timeout.HighPart = -1;
    }
    else
    {
        /* We'll be waiting till the Due Time */
        Timeout = DueTime;
    }

    /* Now write data to the TEB so we'll know what's going on */
    ThreadData->CancelIo = FALSE;
    ThreadData->SocketHandle = SocketHandle;

    /* Start wait loop */
    do
    {
        /* Call the hook */
        if (CallHook) (BlockingHook(Context));

        /* Check if we were cancelled */
        if (ThreadData->CancelIo)
        {
            /* Infinite timeout and wait for official cancel */
            Timeout.LowPart = -1;
            Timeout.HighPart = 0x7FFFFFFF;
        }
        else
        {
            /* Check if we're due */
            Status = NtQuerySystemTime(&CurrentTime);
            if (CurrentTime.QuadPart > DueTime.QuadPart)
            {
                /* We're out */
                Status = STATUS_TIMEOUT;
                break;
            }
        }

        /* Do the actual wait */
        Status = NtWaitForSingleObject(Handle, TRUE, &Timeout);
    } while ((Status == STATUS_USER_APC) ||
             (Status == STATUS_ALERTED) ||
             (Status == STATUS_TIMEOUT));

    /* Reset thread data */
    ThreadData->SocketHandle = INVALID_SOCKET;

    /* Return to caller */
    if (Status == STATUS_SUCCESS) return TRUE;
    return FALSE;    
}

PSOCKET_INFORMATION
WSPAPI
SockFindAndReferenceSocket(IN SOCKET Handle,
                           IN BOOLEAN Import)
{
    PWAH_HANDLE WahHandle;

    /* Get it from our table and return it */
    WahHandle = WahReferenceContextByHandle(SockContextTable, (HANDLE)Handle);
    if (WahHandle) return (PSOCKET_INFORMATION)WahHandle;

    /* Couldn't find it, shoudl we import it? */
    if (Import) return SockImportHandle(Handle);

    /* Nothing found */
    return NULL;
}

INT
WSPAPI
SockBuildTdiAddress(OUT PTRANSPORT_ADDRESS TdiAddress,
                    IN PSOCKADDR Sockaddr,
                    IN INT SockaddrLength)
{
    /* Setup the TDI Address */
    TdiAddress->TAAddressCount = 1;
    TdiAddress->Address[0].AddressLength = (USHORT)SockaddrLength -
                                                   sizeof(Sockaddr->sa_family);

    /* Copy it */
    RtlCopyMemory(&TdiAddress->Address[0].AddressType, Sockaddr, SockaddrLength);

    /* Return */
    return NO_ERROR;
}

INT
WSPAPI
SockBuildSockaddr(OUT PSOCKADDR Sockaddr,
                  OUT PINT SockaddrLength,
                  IN PTRANSPORT_ADDRESS TdiAddress)
{
    /* Calculate the length it will take */
    *SockaddrLength = TdiAddress->Address[0].AddressLength +
                      sizeof(Sockaddr->sa_family);

    /* Copy it */
    RtlCopyMemory(Sockaddr, &TdiAddress->Address[0].AddressType, *SockaddrLength);

    /* Return */
    return NO_ERROR;
}

BOOLEAN
WSPAPI
SockIsSocketConnected(IN PSOCKET_INFORMATION Socket)
{
    LARGE_INTEGER Timeout;
    PVOID Context;
    PVOID AsyncCallback;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    /* Check if there is an async connect in progress, but still unprocessed */
    while ((Socket->AsyncData) &&
           (Socket->AsyncData->IoStatusBlock.Status != STATUS_PENDING))
    {
        /* The socket will be locked, release it */
        LeaveCriticalSection(&Socket->Lock);

        /* Setup the timeout and wait on completion */
        Timeout.QuadPart = 0;
        Status = NtRemoveIoCompletion(SockAsyncQueuePort,
                                      &AsyncCallback,
                                      &Context,
                                      &IoStatusBlock,
                                      &Timeout);

        /* Check for success */
        if (Status == STATUS_SUCCESS)
        {
            /* Check if we're supposed to terminate */
            if (AsyncCallback != (PVOID)-1)
            {
                /* Handle the Async */
                SockHandleAsyncIndication(AsyncCallback, Context, &IoStatusBlock);
            }
            else
            {
                /* Terminate it */
                Status = NtSetIoCompletion(SockAsyncQueuePort,
                                           (PVOID)-1,
                                           (PVOID)-1,
                                           0,
                                           0);

                /* Acquire the lock and break out */
                EnterCriticalSection(&Socket->Lock);
                break;
            }
        }

        /* Acquire the socket lock again */
        EnterCriticalSection(&Socket->Lock);
    }

    /* Check if it's already connected */
    if (Socket->SharedData.State == SocketConnected) return TRUE;
    return FALSE;
}

VOID
WSPAPI
SockCancelIo(IN SOCKET Handle)
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    /* Cancel the I/O */
    Status = NtCancelIoFile((HANDLE)Handle, &IoStatusBlock);
}

VOID
WSPAPI
SockIoCompletion(IN PVOID ApcContext,
                 IN PIO_STATUS_BLOCK IoStatusBlock,
                 DWORD Reserved)
{
    LPWSAOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine = ApcContext;
    INT ErrorCode;
    DWORD BytesSent;
    DWORD Flags = 0;
    LPWSAOVERLAPPED lpOverlapped;
    PWINSOCK_TEB_DATA ThreadData = NtCurrentTeb()->WinSockData;

    /* Check if this was an error */
    if (NT_ERROR(IoStatusBlock->Status))
    {
        /* Check if it was anything but a simple cancel */
        if (IoStatusBlock->Status != STATUS_CANCELLED)
        {
            /* Convert it */
            ErrorCode = NtStatusToSocketError(IoStatusBlock->Status);
        }
        else
        {
            /* Use the right error */
            ErrorCode = WSA_OPERATION_ABORTED;
        }

        /* Either ways, nothing was done */
        BytesSent = 0;
    }
    else
    {
        /* No error and check how many bytes were sent */
        ErrorCode = NO_ERROR;
        BytesSent = PtrToUlong(IoStatusBlock->Information);

        /* Check the status */
        if (IoStatusBlock->Status == STATUS_BUFFER_OVERFLOW)
        {
            /* This was an error */
            ErrorCode = WSAEMSGSIZE;
        }
        else if (IoStatusBlock->Status == STATUS_RECEIVE_PARTIAL)
        {
            /* Partial receive */
            Flags = MSG_PARTIAL;
        }
        else if (IoStatusBlock->Status == STATUS_RECEIVE_EXPEDITED)
        {
            /* OOB receive */
            Flags = MSG_OOB;
        }
        else if (IoStatusBlock->Status == STATUS_RECEIVE_PARTIAL_EXPEDITED)
        {
            /* Partial OOB receive */
            Flags = MSG_OOB | MSG_PARTIAL;
        }
    }

    /* Get the overlapped structure */
    lpOverlapped = CONTAINING_RECORD(IoStatusBlock, WSAOVERLAPPED, Internal);

    /* Call it */
    CompletionRoutine(ErrorCode, BytesSent, lpOverlapped, Flags);

    /* Decrease pending APCs */
    ThreadData->PendingAPCs--;
    InterlockedDecrement(&SockProcessPendingAPCCount);
}

VOID
WSPAPI
SockpWaitForReaderCount(IN PSOCK_RW_LOCK Lock)
{
    NTSTATUS Status;
    LARGE_INTEGER Timeout;

    /* Switch threads to see if the lock gets released that way */
    Timeout.QuadPart = 0;
    NtDelayExecution(FALSE, &Timeout);
    if (Lock->ReaderCount == -2) return;

    /* Either the thread isn't executing (priority inversion) or it's a hog */
    if (!Lock->WriterWaitEvent)
    {
        /* We don't have an event to wait on yet, allocate it */
        Status = NtCreateEvent(&Lock->WriterWaitEvent,
                               EVENT_ALL_ACCESS,
                               NULL,
                               SynchronizationEvent,
                               FALSE);
        if (!NT_SUCCESS(Status))
        {
            /* We can't get an event, do a manual loop */
            Timeout.QuadPart = Int32x32To64(1000, -100);
            while (Lock->ReaderCount != -2) NtDelayExecution(FALSE, &Timeout);
        }
    }

    /* We have en event, now increment the reader count to signal them */
    if (InterlockedIncrement(&Lock->ReaderCount) != -1)
    {
        /* Wait for them to signal us */
        NtWaitForSingleObject(&Lock->WriterWaitEvent, FALSE, NULL);
    }

    /* Finally it's free */
    Lock->ReaderCount = -2;
}

NTSTATUS
WSPAPI
SockInitializeRwLockAndSpinCount(IN PSOCK_RW_LOCK Lock,
                                 IN ULONG SpinCount)
{
    NTSTATUS Status;

    /* check if this is a special event create request */
    if (SpinCount & 0x80000000)
    {
        /* Create the event */
        Status = NtCreateEvent(&Lock->WriterWaitEvent,
                               EVENT_ALL_ACCESS,
                               NULL,
                               SynchronizationEvent,
                               FALSE);
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Initialize the lock */
    Status = RtlInitializeCriticalSectionAndSpinCount(&Lock->Lock, SpinCount);
    if (NT_SUCCESS(Status))
    {
        /* Initialize our structure */
        Lock->ReaderCount = 0;
    }
    else if (Lock->WriterWaitEvent)
    {
        /* We failed, close the event if we had one */
        NtClose(Lock->WriterWaitEvent);
        Lock->WriterWaitEvent = NULL;
    }

    /* Return status */
    return Status;
}

VOID
WSPAPI
SockAcquireRwLockExclusive(IN PSOCK_RW_LOCK Lock)
{
    LONG Count, NewCount;
    ULONG_PTR SpinCount;

    /* Acquire the lock */
    RtlEnterCriticalSection(&Lock->Lock);

    /* Check for ReaderCount */
    if (Lock->ReaderCount >= 0)
    {
        /* Loop while trying to change the count */
        do
        {
            /* Get the reader count */
            Count = Lock->ReaderCount;
            
            /* Modify the count so ReaderCount know that a writer is waiting */
            NewCount = -Count - 2;
        } while (InterlockedCompareExchange(&Lock->ReaderCount,
                                            NewCount,
                                            Count) != Count);

        /* Check if some ReaderCount are still active */
        if (NewCount != -2)
        {
            /* Get the spincount of the CS */
            SpinCount = Lock->Lock.SpinCount;

            /* Loop until they are done */
            while (Lock->ReaderCount != -2)
            {
                /* Check if the CS has a spin count */
                if (SpinCount)
                {
                    /* Spin on it */
                    SpinCount--;
                }
                else
                {
                    /* Do a full wait for ReaderCount */
                    SockpWaitForReaderCount(Lock);
                    break;
                }
            }
        }
    }
    else
    {
        /* Acquiring it again, decrement the count to handle this */
        Lock->ReaderCount--;
    }
}

VOID
WSPAPI
SockAcquireRwLockShared(IN PSOCK_RW_LOCK Lock)
{
    BOOL GotLock = FALSE;
    LONG Count, NewCount;

    /* Start acquire loop */
    do
    {
        /* Get the current count */
        Count = Lock->ReaderCount;

        /* Check if a writer is active */
        if (Count < 0)
        {
            /* Acquire the lock (this will wait for the writer) */
            EnterCriticalSection(&Lock->Lock);
            GotLock = TRUE;

            /* Get the counter again */
            Count = Lock->ReaderCount;
            if (Count < 0)
            {
                /* It's still below 0, so this is a recursive acquire */
                NewCount = Count - 1;
            }
            else
            {
                /* Increase the count since the writer has finished */
                NewCount = Count + 1;
            }
        }
        else
        {
            /* No writers are active, increase count */
            NewCount = Count + 1;
        }

        /* Update the count */
        Lock->ReaderCount = InterlockedCompareExchange(&Lock->ReaderCount,
                                                   NewCount,
                                                   Count);

        /* Check if we got the lock */
        if (GotLock)
        {
            /* Release it */
            LeaveCriticalSection(&Lock->Lock);
            GotLock = FALSE;
        }
    } while (NewCount != Count);
}

VOID
WSPAPI
SockReleaseRwLockExclusive(IN PSOCK_RW_LOCK Lock)
{
    /* Increase the reader count and check if it's a recursive acquire */
    if (++Lock->ReaderCount == -1)
    {
        /* This release is the final one, so unhack the reader count */
        Lock->ReaderCount = 0;
    }

    /* Leave the RTL CS */
    RtlLeaveCriticalSection(&Lock->Lock);
}

VOID
WSPAPI
SockReleaseRwLockShared(IN PSOCK_RW_LOCK Lock)
{
    LONG NewCount, Count = Lock->ReaderCount;

    /* Start release loop */
    while (TRUE)
    {
        /* Check if writers are using the lock */
        if (Count > 0)
        {
            /* Lock is free, decrement the count */
            NewCount = Count - 1;
        }
        else
        {
            /* Lock is busy, increment the count */
            NewCount = Count + 1;
        }

        /* Update the count */
        if (InterlockedCompareExchange(&Lock->ReaderCount, NewCount, Count) == Count)
        {
            /* Count changed sucesfully, was this the last reader? */
            if (NewCount == -1)
            {
                /* It was, we need to tell the writer about it */
                NtSetEvent(Lock->WriterWaitEvent, NULL);
            }
            break;
        }
    }
}

NTSTATUS
WSPAPI
SockDeleteRwLock(IN PSOCK_RW_LOCK Lock)
{
    /* Check if there's an event */
    if (Lock->WriterWaitEvent)
    {
        /* Close it */
        NtClose(Lock->WriterWaitEvent);
        Lock->WriterWaitEvent = NULL;
    }

    /* Free the Crtitical Section */
    return RtlDeleteCriticalSection(&Lock->Lock);
}
