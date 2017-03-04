/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/async.c
 * PURPOSE:     Async Block Object and Async Thread Management
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ws2_32.h>

/* DATA **********************************************************************/

BOOLEAN WsAsyncThreadInitialized;
LONG WsAsyncTaskHandle;
PLIST_ENTRY WsAsyncQueue;
CRITICAL_SECTION WsAsyncCritSect;
HANDLE WsAsyncEvent;
HANDLE WsAsyncCurrentTaskHandle;
HANDLE WsAsyncCancelledTaskHandle;
HINSTANCE WsAsyncDllHandle;

#define WsAsyncLock()       EnterCriticalSection(&WsAsyncCritSect)
#define WsAsyncUnlock()     LeaveCriticalSection(&WsAsyncCritSect)

/* FUNCTIONS *****************************************************************/

VOID
WSAAPI
WsAsyncGlobalInitialize(VOID)
{
    /* Initialize the async lock */
    InitializeCriticalSection(&WsAsyncCritSect);
}

VOID
WSAAPI
WsAsyncGlobalTerminate(VOID)
{
    /* Destroy the async lock */
    DeleteCriticalSection(&WsAsyncCritSect);
}


SIZE_T
WSAAPI
BytesInHostent(PHOSTENT Hostent)
{
    SIZE_T Bytes;
    INT i;

    /* Start with the static stuff */
    Bytes = sizeof(HOSTENT) + strlen(Hostent->h_name) + sizeof(CHAR);

    /* Add 2 pointers for the list-terminators */
    Bytes += 2 * sizeof(ULONG_PTR);

    /* Now loop for the aliases */
    for (i = 0; Hostent->h_aliases[i]; i++)
    {
        /* Add the alias size, plus the space its pointer takes */
        Bytes += strlen(Hostent->h_aliases[i]) + sizeof(CHAR) + sizeof(ULONG_PTR);
    }

    /* Now loop for the hostnames  */
    for (i = 0; Hostent->h_addr_list[i]; i++)
    {
        /* Add the alias size, plus the space its pointer takes */
        Bytes += Hostent->h_length + sizeof(ULONG_PTR);
    }

    /* Align to 8 bytes */
    return (Bytes + 7) & ~7;
}

SIZE_T
WSAAPI
BytesInServent(PSERVENT Servent)
{
    SIZE_T Bytes;
    INT i;

    /* Start with the static stuff */
    Bytes = sizeof(SERVENT) +
            strlen(Servent->s_name) + sizeof(CHAR) +
            strlen(Servent->s_proto) + sizeof(CHAR);

    /* Add 1 pointers for the list terminator */
    Bytes += sizeof(ULONG_PTR);

    /* Now loop for the aliases */
    for (i = 0; Servent->s_aliases[i]; i++)
    {
        /* Add the alias size, plus the space its pointer takes */
        Bytes += strlen(Servent->s_aliases[i]) + sizeof(CHAR) + sizeof(ULONG_PTR);
    }

    /* return */
    return Bytes;
}

SIZE_T
WSAAPI
BytesInProtoent(PPROTOENT Protoent)
{
    SIZE_T Bytes;
    INT i;

    /* Start with the static stuff */
    Bytes = sizeof(SERVENT) + strlen(Protoent->p_name) + sizeof(CHAR);

    /* Add 1 pointers for the list terminator */
    Bytes += sizeof(ULONG_PTR);

    /* Now loop for the aliases */
    for (i = 0; Protoent->p_aliases[i]; i++)
    {
        /* Add the alias size, plus the space its pointer takes */
        Bytes += strlen(Protoent->p_aliases[i]) + sizeof(CHAR) + sizeof(ULONG_PTR);
    }

    /* return */
    return Bytes;
}

SIZE_T
WSAAPI
CopyHostentToBuffer(IN PCHAR Buffer,
                    IN INT BufferLength,
                    IN PHOSTENT Hostent)
{
    SIZE_T BufferSize, CurrentSize, NameSize;
    PCHAR p = Buffer;
    DWORD Aliases = 0, Names = 0;
    DWORD i;
    PHOSTENT ReturnedHostent = (PHOSTENT)Buffer;

    /* Determine the buffer size required */
    BufferSize = BytesInHostent(Hostent);

    /* Check which size to use */
    if ((DWORD)BufferLength > BufferSize)
    {
        /* Zero the buffer */
        RtlZeroMemory(Buffer, BufferSize);
    }
    else
    {
        /* Zero the buffer */
        RtlZeroMemory(Buffer, BufferLength);
    }

    /* Start with the raw Hostent */
    CurrentSize = sizeof(HOSTENT);

    /* Return the size needed now */
    if (CurrentSize > (DWORD)BufferLength) return BufferSize;

    /* Copy the Hostent and initialize it */
    CopyMemory(p, Hostent, sizeof(HOSTENT));
    p = Buffer + CurrentSize;
    ReturnedHostent->h_name = NULL;
    ReturnedHostent->h_aliases = NULL;
    ReturnedHostent->h_addr_list = NULL;

    /* Find out how many aliases there are */
    while (Hostent->h_aliases[Aliases])
    {
        /* Increase the alias count */
        Aliases++;
    }

    /* Add the aliases to the size, and validate it */
    CurrentSize += (Aliases + 1) * sizeof(ULONG_PTR);
    if (CurrentSize > (DWORD)BufferLength)
    {
        /* Clear the aliases and return */
        Hostent->h_aliases = NULL;
        return BufferSize;
    }

    /* Write the aliases, update the pointer */
    ReturnedHostent->h_aliases = (PCHAR*)p;
    p = Buffer + CurrentSize;

    /* Find out how many names there are */
    while (Hostent->h_addr_list[Names])
    {
        /* Increase the alias count */
        Names++;
    }

    /* Add the names to the size, and validate it */
    CurrentSize += (Names + 1) * sizeof(ULONG_PTR);
    if (CurrentSize > (DWORD)BufferLength)
    {
        /* Clear the aliases and return */
        Hostent->h_addr_list = NULL;
        return BufferSize;
    }

    /* Write the names, update the pointer */
    ReturnedHostent->h_addr_list = (PCHAR*)p;
    p = Buffer + CurrentSize;

    /* Now add the names */
    for (i = 0; i < Names; i++)
    {
        /* Update size and validate */
        CurrentSize += Hostent->h_length;
        if (CurrentSize > (DWORD)BufferLength) return BufferSize;

        /* Write pointer and copy */
        ReturnedHostent->h_addr_list[i] = p;
        CopyMemory(p, Hostent->h_addr_list[i], Hostent->h_length);

        /* Update pointer */
        p = Buffer + CurrentSize;
    }

    /* Finalize the list */
    ReturnedHostent->h_addr_list[i] = NULL;

    /* Add the service name to the size, and validate it */
    NameSize = strlen(Hostent->h_name) + sizeof(CHAR);
    CurrentSize += NameSize;
    if (CurrentSize > (DWORD)BufferLength) return BufferSize;

    /* Write the service name and update the pointer */
    ReturnedHostent->h_name = p;
    CopyMemory(p, Hostent->h_name, NameSize);
    p = Buffer + CurrentSize;

    /* Now add the aliases */
    for (i = 0; i < Aliases; i++)
    {
        /* Update size and validate */
        NameSize = strlen(Hostent->h_aliases[i]) + sizeof(CHAR);
        CurrentSize += NameSize;
        if (CurrentSize > (DWORD)BufferLength) return BufferSize;

        /* Write pointer and copy */
        ReturnedHostent->h_aliases[i] = p;
        CopyMemory(p, Hostent->h_aliases[i], NameSize);

        /* Update pointer */
        p = Buffer + CurrentSize;
    }

    /* Finalize the list and return */
    ReturnedHostent->h_aliases[i] = NULL;
    return BufferSize;
}

SIZE_T
WSAAPI
CopyServentToBuffer(IN PCHAR Buffer,
                    IN INT BufferLength,
                    IN PSERVENT Servent)
{
    SIZE_T BufferSize, CurrentSize, NameSize;
    PCHAR p = Buffer;
    DWORD Aliases = 0;
    DWORD i;
    PSERVENT ReturnedServent = (PSERVENT)Buffer;

    /* Determine the buffer size required */
    BufferSize = BytesInServent(Servent);

    /* Check which size to use */
    if ((DWORD)BufferLength > BufferSize)
    {
        /* Zero the buffer */
        ZeroMemory(Buffer, BufferSize);
    }
    else
    {
        /* Zero the buffer */
        ZeroMemory(Buffer, BufferLength);
    }

    /* Start with the raw servent */
    CurrentSize = sizeof(SERVENT);

    /* Return the size needed now */
    if (CurrentSize > (DWORD)BufferLength) return BufferSize;

    /* Copy the servent and initialize it */
    CopyMemory(p, Servent, sizeof(SERVENT));
    p = Buffer + CurrentSize;
    ReturnedServent->s_name = NULL;
    ReturnedServent->s_aliases = NULL;
    ReturnedServent->s_proto = NULL;

    /* Find out how many aliases there are */
    while (Servent->s_aliases[Aliases])
    {
        /* Increase the alias count */
        Aliases++;
    }

    /* Add the aliases to the size, and validate it */
    CurrentSize += (Aliases + 1) * sizeof(ULONG_PTR);
    if (CurrentSize > (DWORD)BufferLength)
    {
        /* Clear the aliases and return */
        Servent->s_aliases = NULL;
        return BufferSize;
    }

    /* Write the aliases, update the pointer */
    ReturnedServent->s_aliases = (PCHAR*)p;
    p = Buffer + CurrentSize;

    /* Add the service name to the size, and validate it */
    NameSize = strlen(Servent->s_name) + sizeof(CHAR);
    CurrentSize += NameSize;
    if (CurrentSize > (DWORD)BufferLength) return BufferSize;

    /* Write the service name and update the pointer */
    ReturnedServent->s_name = p;
    CopyMemory(p, Servent->s_name, NameSize);
    p = Buffer + CurrentSize;

    /* Now add the aliases */
    for (i = 0; i < Aliases; i++)
    {
        /* Update size and validate */
        NameSize = strlen(Servent->s_aliases[i]) + sizeof(CHAR);
        CurrentSize += NameSize;
        if (CurrentSize > (DWORD)BufferLength) return BufferSize;

        /* Write pointer and copy */
        ReturnedServent->s_aliases[i] = p;
        CopyMemory(p, Servent->s_aliases[i], NameSize);

        /* Update pointer */
        p = Buffer + CurrentSize;
    }

    /* Finalize the list and return */
    ReturnedServent->s_aliases[i] = NULL;
    return BufferSize;
}

SIZE_T
WSAAPI
CopyProtoentToBuffer(IN PCHAR Buffer,
                     IN INT BufferLength,
                     IN PPROTOENT Protoent)
{
    SIZE_T BufferSize, CurrentSize, NameSize;
    PCHAR p = Buffer;
    DWORD Aliases = 0;
    DWORD i;
    PPROTOENT ReturnedProtoent = (PPROTOENT)Buffer;

    /* Determine the buffer size required */
    BufferSize = BytesInProtoent(Protoent);

    /* Check which size to use */
    if ((DWORD)BufferLength > BufferSize)
    {
        /* Zero the buffer */
        ZeroMemory(Buffer, BufferSize);
    }
    else
    {
        /* Zero the buffer */
        ZeroMemory(Buffer, BufferLength);
    }

    /* Start with the raw servent */
    CurrentSize = sizeof(PROTOENT);

    /* Return the size needed now */
    if (CurrentSize > (DWORD)BufferLength) return BufferSize;

    /* Copy the servent and initialize it */
    CopyMemory(p, Protoent, sizeof(PROTOENT));
    p = Buffer + CurrentSize;
    ReturnedProtoent->p_name = NULL;
    ReturnedProtoent->p_aliases = NULL;

    /* Find out how many aliases there are */
    while (Protoent->p_aliases[Aliases])
    {
        /* Increase the alias count */
        Aliases++;
    }

    /* Add the aliases to the size, and validate it */
    CurrentSize += (Aliases + 1) * sizeof(ULONG_PTR);
    if (CurrentSize > (DWORD)BufferLength)
    {
        /* Clear the aliases and return */
        Protoent->p_aliases = NULL;
        return BufferSize;
    }

    /* Write the aliases, update the pointer */
    ReturnedProtoent->p_aliases = (PCHAR*)p;
    p = Buffer + CurrentSize;

    /* Add the service name to the size, and validate it */
    NameSize = strlen(Protoent->p_name) + sizeof(CHAR);
    CurrentSize += NameSize;
    if (CurrentSize > (DWORD)BufferLength) return BufferSize;

    /* Write the service name and update the pointer */
    ReturnedProtoent->p_name = p;
    CopyMemory(p, Protoent->p_name, NameSize);
    p = Buffer + CurrentSize;

    /* Now add the aliases */
    for (i = 0; i < Aliases; i++)
    {
        /* Update size and validate */
        NameSize = strlen(Protoent->p_aliases[i]) + sizeof(CHAR);
        CurrentSize += NameSize;
        if (CurrentSize > (DWORD)BufferLength) return BufferSize;

        /* Write pointer and copy */
        ReturnedProtoent->p_aliases[i] = p;
        CopyMemory(p, Protoent->p_aliases[i], NameSize);

        /* Update pointer */
        p = Buffer + CurrentSize;
    }

    /* Finalize the list and return */
    ReturnedProtoent->p_aliases[i] = NULL;
    return BufferSize;
}

PWSASYNCBLOCK
WSAAPI
WsAsyncAllocateBlock(IN SIZE_T ExtraLength)
{
    PWSASYNCBLOCK AsyncBlock;

    /* Add the size of the block */
    ExtraLength += sizeof(WSASYNCBLOCK);

    /* Allocate it */
    AsyncBlock = HeapAlloc(WsSockHeap, 0, ExtraLength);

    /* Get a handle to it */
    AsyncBlock->TaskHandle = UlongToPtr(InterlockedIncrement(&WsAsyncTaskHandle));

    /* Return it */
    return AsyncBlock;
}

BOOL
WINAPI
WsAsyncThreadBlockingHook(VOID)
{
    /* Check if this task is being cancelled */
    if (WsAsyncCurrentTaskHandle == WsAsyncCancelledTaskHandle)
    {
        /* Cancel the blocking call so we can get back */
        WSACancelBlockingCall();
    }

    /* Return to system */
    return FALSE;
}

VOID
WSAAPI
WsAsyncFreeBlock(IN PWSASYNCBLOCK AsyncBlock)
{
    /* Free it */
    HeapFree(WsSockHeap, 0, AsyncBlock);
}

VOID
WSAAPI
WsAsyncGetServ(IN HANDLE TaskHandle,
               IN DWORD Operation,
               IN HWND hWnd,
               IN UINT wMsg,
               IN CHAR FAR *ByWhat,
               IN CHAR FAR *Protocol,
               IN CHAR FAR *Buffer,
               IN INT BufferLength)
{
    PSERVENT Servent;
    SIZE_T BufferSize = 0;
    LPARAM lParam;
    INT ErrorCode = 0;

    /* Check the operation */
    if (Operation == WsAsyncGetServByName)
    {
        /* Call the API */
        Servent = getservbyname(ByWhat, Protocol);
    }
    else
    {
        /* Call the API */
        Servent = getservbyport(PtrToUlong(ByWhat), Protocol);
    }

    /* Make sure we got one */
    if (!Servent) ErrorCode = GetLastError();

    /* Acquire the lock */
    WsAsyncLock();

    /* Check if this task got cancelled */
    if (TaskHandle == WsAsyncCancelledTaskHandle)
    {
        /* Return */
        WsAsyncUnlock();
        return;
    }

    /* If we got a Servent back, copy it */
    if (Servent)
    {
        /* Copy it into the buffer */
        BufferSize = CopyServentToBuffer(Buffer, BufferLength, Servent);

        /* Check if we had enough space */
        if (BufferSize > (DWORD)BufferLength)
        {
            /* Not enough */
            ErrorCode = WSAENOBUFS;
        }
        else
        {
            /* Perfect */
            ErrorCode = NO_ERROR;
        }
    }

    /* Not processing anymore */
    WsAsyncCurrentTaskHandle = NULL;

    /* Release the lock */
    WsAsyncUnlock();

    /* Make the messed-up lParam reply */
    lParam = WSAMAKEASYNCREPLY(BufferSize, ErrorCode);

    /* Sent it through the Upcall API */
    WPUPostMessage(hWnd, wMsg, (WPARAM)TaskHandle, lParam);
}

VOID
WSAAPI
WsAsyncGetProto(IN HANDLE TaskHandle,
                IN DWORD Operation,
                IN HWND hWnd,
                IN UINT wMsg,
                IN CHAR FAR *ByWhat,
                IN CHAR FAR *Buffer,
                IN INT BufferLength)
{
    PPROTOENT Protoent;
    SIZE_T BufferSize = 0;
    LPARAM lParam;
    INT ErrorCode = 0;

    /* Check the operation */
    if (Operation == WsAsyncGetProtoByName)
    {
        /* Call the API */
        Protoent = getprotobyname(ByWhat);
    }
    else
    {
        /* Call the API */
        Protoent = getprotobynumber(PtrToUlong(ByWhat));
    }

    /* Make sure we got one */
    if (!Protoent) ErrorCode = GetLastError();

    /* Acquire the lock */
    WsAsyncLock();

    /* Check if this task got cancelled */
    if (TaskHandle == WsAsyncCancelledTaskHandle)
    {
        /* Return */
        WsAsyncUnlock();
        return;
    }

    /* If we got a Servent back, copy it */
    if (Protoent)
    {
        /* Copy it into the buffer */
        BufferSize = CopyProtoentToBuffer(Buffer, BufferLength, Protoent);

        /* Check if we had enough space */
        if (BufferSize > (DWORD)BufferLength)
        {
            /* Not enough */
            ErrorCode = WSAENOBUFS;
        }
        else
        {
            /* Perfect */
            ErrorCode = NO_ERROR;
        }
    }

    /* Not processing anymore */
    WsAsyncCurrentTaskHandle = NULL;

    /* Release the lock */
    WsAsyncUnlock();

    /* Make the messed-up lParam reply */
    lParam = WSAMAKEASYNCREPLY(BufferSize, ErrorCode);

    /* Sent it through the Upcall API */
    WPUPostMessage(hWnd, wMsg, (WPARAM)TaskHandle, lParam);
}

VOID
WSAAPI
WsAsyncGetHost(IN HANDLE TaskHandle,
               IN DWORD Operation,
               IN HWND hWnd,
               IN UINT wMsg,
               IN CHAR FAR *ByWhat,
               IN INT Length,
               IN INT Type,
               IN CHAR FAR *Buffer,
               IN INT BufferLength)
{
    PHOSTENT Hostent;
    SIZE_T BufferSize = 0;
    LPARAM lParam;
    INT ErrorCode = 0;

    /* Check the operation */
    if (Operation == WsAsyncGetHostByAddr)
    {
        /* Call the API */
        Hostent = gethostbyaddr(ByWhat, Length, Type);
    }
    else
    {
        /* Call the API */
        Hostent = gethostbyname(ByWhat);
    }

    /* Make sure we got one */
    if (!Hostent) ErrorCode = GetLastError();

    /* Acquire the lock */
    WsAsyncLock();

    /* Check if this task got cancelled */
    if (TaskHandle == WsAsyncCancelledTaskHandle)
    {
        /* Return */
        WsAsyncUnlock();
        return;
    }

    /* If we got a Servent back, copy it */
    if (Hostent)
    {
        /* Copy it into the buffer */
        BufferSize = CopyHostentToBuffer(Buffer, BufferLength, Hostent);

        /* Check if we had enough space */
        if (BufferSize > (DWORD)BufferLength)
        {
            /* Not enough */
            ErrorCode = WSAENOBUFS;
        }
        else
        {
            /* Perfect */
            ErrorCode = NO_ERROR;
        }
    }

    /* Not processing anymore */
    WsAsyncCurrentTaskHandle = NULL;

    /* Release the lock */
    WsAsyncUnlock();

    /* Make the messed-up lParam reply */
    lParam = WSAMAKEASYNCREPLY(BufferSize, ErrorCode);

    /* Sent it through the Upcall API */
    WPUPostMessage(hWnd, wMsg, (WPARAM)TaskHandle, lParam);
}

DWORD
WINAPI
WsAsyncThread(IN PVOID ThreadContext)
{
    PWSASYNCCONTEXT Context = ThreadContext;
    PWSASYNCBLOCK AsyncBlock;
    PLIST_ENTRY Entry;
    HANDLE AsyncEvent = Context->AsyncEvent;
    PLIST_ENTRY ListHead = &Context->AsyncQueue;

    /* Set the blocking hook */
    WSASetBlockingHook((FARPROC)WsAsyncThreadBlockingHook);

    /* Loop */
    while (TRUE)
    {
        /* Wait for the event */
        WaitForSingleObject(AsyncEvent, INFINITE);

        /* Get the lock */
        WsAsyncLock();

        /* Process the queue */
        while (!IsListEmpty(ListHead))
        {
            /* Remove this entry and get the async block */
            Entry = RemoveHeadList(ListHead);
            AsyncBlock = CONTAINING_RECORD(Entry, WSASYNCBLOCK, AsyncQueue);

            /* Save the current task handle */
            WsAsyncCurrentTaskHandle = AsyncBlock->TaskHandle;

            /* Release the lock */
            WsAsyncUnlock();

            /* Check which operation to do */
            switch (AsyncBlock->Operation)
            {
                /* Get Host by Y */
                case WsAsyncGetHostByAddr: case WsAsyncGetHostByName:

                    /* Call the handler */
                    WsAsyncGetHost(AsyncBlock->TaskHandle,
                                   AsyncBlock->Operation,
                                   AsyncBlock->GetHost.hWnd,
                                   AsyncBlock->GetHost.wMsg,
                                   AsyncBlock->GetHost.ByWhat,
                                   AsyncBlock->GetHost.Length,
                                   AsyncBlock->GetHost.Type,
                                   AsyncBlock->GetHost.Buffer,
                                   AsyncBlock->GetHost.BufferLength);
                    break;

                /* Get Proto by Y */
                case WsAsyncGetProtoByNumber: case WsAsyncGetProtoByName:

                    /* Call the handler */
                    WsAsyncGetProto(AsyncBlock->TaskHandle,
                                    AsyncBlock->Operation,
                                    AsyncBlock->GetProto.hWnd,
                                    AsyncBlock->GetProto.wMsg,
                                    AsyncBlock->GetHost.ByWhat,
                                    AsyncBlock->GetProto.Buffer,
                                    AsyncBlock->GetProto.BufferLength);
                    break;

                /* Get Serv by Y */
                case WsAsyncGetServByPort: case WsAsyncGetServByName:

                    /* Call the handler */
                    WsAsyncGetServ(AsyncBlock->TaskHandle,
                                   AsyncBlock->Operation,
                                   AsyncBlock->GetServ.hWnd,
                                   AsyncBlock->GetServ.wMsg,
                                   AsyncBlock->GetServ.ByWhat,
                                   AsyncBlock->GetServ.Protocol,
                                   AsyncBlock->GetServ.Buffer,
                                   AsyncBlock->GetServ.BufferLength);
                    break;

                /* Termination */
                case WsAsyncTerminate:

                    /* Clean up the extra reference */
                    WSACleanup();

                    /* Free the context block */
                    WsAsyncFreeBlock(AsyncBlock);

                    /* Acquire the lock */
                    WsAsyncLock();

                    /* Loop the queue and flush it */
                    while (!IsListEmpty(ListHead))
                    {
                        Entry = RemoveHeadList(ListHead);
                        AsyncBlock = CONTAINING_RECORD(Entry,
                                                       WSASYNCBLOCK,
                                                       AsyncQueue);
                        WsAsyncFreeBlock(AsyncBlock);
                    }

                    /* Release lock */
                    WsAsyncUnlock();

                    /* Close the event, free the Context */
                    CloseHandle(AsyncEvent);
                    HeapFree(WsSockHeap, 0, Context);

                    /* Remove the extra DLL reference and kill us */
                    FreeLibraryAndExitThread(WsAsyncDllHandle, 0);

                default:
                    break;
            }

            /* Done processing */
            WsAsyncCurrentTaskHandle = NULL;

            /* Free this block, get lock and reloop */
            WsAsyncFreeBlock(AsyncBlock);
            WsAsyncLock();
        }

        /* Release the lock */
        WsAsyncUnlock();
    }
}

BOOL
WSAAPI
WsAsyncCheckAndInitThread(VOID)
{
    HANDLE ThreadHandle;
    DWORD Tid;
    PWSASYNCCONTEXT Context = NULL;
    WSADATA WsaData;

    /* Make sure we're not initialized */
    if (WsAsyncThreadInitialized) return TRUE;

    /* Acquire the lock */
    WsAsyncLock();

    /* Make sure we're not initialized */
    if (!WsAsyncThreadInitialized)
    {
        /* Initialize Thread Context */
        Context = HeapAlloc(WsSockHeap, 0, sizeof(*Context));
        if (!Context)
            goto Exit;

        /* Initialize the Queue and event */
        WsAsyncQueue = &Context->AsyncQueue;
        InitializeListHead(WsAsyncQueue);
        Context->AsyncEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        WsAsyncEvent = Context->AsyncEvent;

        /* Prevent us from ever being killed while running */
        if (WSAStartup(MAKEWORD(2,2), &WsaData) != ERROR_SUCCESS)
            goto Fail;

        /* Create the thread */
        ThreadHandle = CreateThread(NULL,
                                    0,
                                    WsAsyncThread,
                                    Context,
                                    0,
                                    &Tid);
        if (ThreadHandle == NULL)
        {
            /* Cleanup and fail */
            WSACleanup();
            goto Fail;
        }

        /* Close the handle and set init */
        CloseHandle(ThreadHandle);
        WsAsyncThreadInitialized = TRUE;
    }

Exit:
    /* Release the lock */
    WsAsyncUnlock();
    return WsAsyncThreadInitialized;

Fail:
    /* Close the event, free the Context */
    if (Context->AsyncEvent)
        CloseHandle(Context->AsyncEvent);
    HeapFree(WsSockHeap, 0, Context);

    /* Bail out */
    goto Exit;
}

VOID
WSAAPI
WsAsyncTerminateThread(VOID)
{
    PWSASYNCBLOCK AsyncBlock;

    /* Make sure we're initialized */
    if (!WsAsyncThreadInitialized) return;

    /* Allocate a block */
    AsyncBlock = WsAsyncAllocateBlock(0);

    /* Initialize it for termination */
    AsyncBlock->Operation = WsAsyncTerminate;

    /* Queue the request and return */
    WsAsyncQueueRequest(AsyncBlock);
    WsAsyncThreadInitialized = FALSE;
}

VOID
WSAAPI
WsAsyncQueueRequest(IN PWSASYNCBLOCK AsyncBlock)
{
    /* Get the lock */
    WsAsyncLock();

    /* Insert it into the queue */
    InsertTailList(WsAsyncQueue, &AsyncBlock->AsyncQueue);

    /* Wake up the thread */
    SetEvent(WsAsyncEvent);

    /* Release lock and return */
    WsAsyncUnlock();
}

INT
WSAAPI
WsAsyncCancelRequest(IN HANDLE TaskHandle)
{
    PLIST_ENTRY Entry;
    PWSASYNCBLOCK AsyncBlock;

    /* Make sure we're initialized */
    if (!WsAsyncThreadInitialized) return WSAEINVAL;

    /* Acquire the lock */
    WsAsyncLock();

    /* Check if we're cancelling the current task */
    if (TaskHandle == WsAsyncCurrentTaskHandle)
    {
        /* Mark us as cancelled, the async thread will see this later */
        WsAsyncCancelledTaskHandle = TaskHandle;

        /* Release lock and return */
        WsAsyncUnlock();
        return NO_ERROR;
    }

    /* Loop the queue */
    Entry = WsAsyncQueue->Flink;
    while (Entry != WsAsyncQueue)
    {
        /* Get the Async Block */
        AsyncBlock = CONTAINING_RECORD(Entry, WSASYNCBLOCK, AsyncQueue);

        /* Check if this is the one */
        if (TaskHandle == AsyncBlock->TaskHandle)
        {
            /* It is, remove it */
            RemoveEntryList(Entry);

            /* Release the lock, free the block, and return */
            WsAsyncUnlock();
            WsAsyncFreeBlock(AsyncBlock);
            return NO_ERROR;
        }

        /* Move to the next entry */
        Entry = Entry->Flink;
    }

    /* Nothing found, fail */
    WsAsyncUnlock();
    return WSAEINVAL;
}
