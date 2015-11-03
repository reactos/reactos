/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/dthread.c
 * PURPOSE:     Thread Object
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ws2_32.h>

/* FUNCTIONS *****************************************************************/

DWORD
WSAAPI
WsThreadDefaultBlockingHook(VOID)
{
    MSG Message;
    BOOL GotMessage = FALSE;
    
    /* Get the message */
    GotMessage = PeekMessage(&Message, NULL, 0, 0, PM_REMOVE);

    /* Check if we got one */
    if (GotMessage)
    {
        /* Process it */
        TranslateMessage(&Message);
        DispatchMessage(&Message);
    }

    /* return */
    return GotMessage;
}

BOOL
WSAAPI
WsThreadBlockingCallback(IN DWORD_PTR Context)
{
    PWSTHREAD Thread = TlsGetValue(TlsIndex);

    /* Set thread as blocking, set cancel callback and the clear cancel flag */
    Thread->Blocking = TRUE;
    Thread->CancelBlockingCall = (LPWSPCANCELBLOCKINGCALL)Context;
    Thread->Cancelled = FALSE;

    /* Call the blocking hook */
    while(Thread->BlockingHook());

    /* We're not blocking anymore */
    Thread->Blocking = FALSE;

    /* Return whether or not we were cancelled */
    return !Thread->Cancelled;
}

FARPROC
WSAAPI
WsThreadSetBlockingHook(IN PWSTHREAD Thread,
                        IN FARPROC BlockingHook)
{
    FARPROC OldHook = Thread->BlockingHook;

    /* Check if we're resetting to our default hook */
    if (BlockingHook == (FARPROC)WsThreadDefaultBlockingHook)
    {
        /* Clear out the blocking callback */
        Thread->BlockingCallback = NULL;
    }
    else
    {
        /* Set the blocking callback */
        Thread->BlockingCallback = WsThreadBlockingCallback;
    }

    /* Set the new blocking hook and return the previous */
    Thread->BlockingHook = BlockingHook;
    return OldHook;
}

DWORD
WSAAPI
WsThreadUnhookBlockingHook(IN PWSTHREAD Thread)
{
    /* Reset the hook to the default, and remove the callback */
    Thread->BlockingHook = (FARPROC)WsThreadDefaultBlockingHook;
    Thread->BlockingCallback = NULL;

    /* Return success */
    return ERROR_SUCCESS;
}

DWORD
WSAAPI
WsThreadCancelBlockingCall(IN PWSTHREAD Thread)
{
    INT ErrorCode, ReturnValue;

    /* Make sure that the Thread is really in a blocking call */
    if (!Thread->Blocking) return WSAEINVAL;

    /* Make sure we haven't already been cancelled */
    if (!Thread->Cancelled)
    {
        /* Call the cancel procedure */
        ReturnValue = Thread->CancelBlockingCall(&ErrorCode);
        if (ReturnValue != ERROR_SUCCESS) return ErrorCode;

        /* Set us as cancelled */
        Thread->Cancelled = TRUE;
    }

    /* Success */
    return ERROR_SUCCESS;
}

PWSPROTO_BUFFER
WSAAPI
WsThreadGetProtoBuffer(IN PWSTHREAD Thread)
{
    /* See if it already exists */
    if (!Thread->ProtocolInfo)
    {
        /* We don't have a buffer; allocate it */
        Thread->ProtocolInfo = HeapAlloc(WsSockHeap, 0, sizeof(WSPROTO_BUFFER));
    }

    /* Return it */
    return Thread->ProtocolInfo;
}

PWSTHREAD
WSAAPI
WsThreadAllocate(VOID)
{
    PWSTHREAD Thread;
    
    /* Allocate the object */
    Thread = HeapAlloc(WsSockHeap, HEAP_ZERO_MEMORY, sizeof(*Thread));

    /* Set non-zero data */
    Thread->BlockingHook = (FARPROC)WsThreadDefaultBlockingHook;

    /* Return it */
    return Thread;
}

DWORD
WSAAPI
WsThreadStartup(VOID)
{
    INT ErrorCode = WSASYSCALLFAILURE;
    
    /* Check if we have a valid TLS */
    if (TlsIndex != TLS_OUT_OF_INDEXES)
    {
        /* TLS was already OK */
        ErrorCode = ERROR_SUCCESS;
    }

    /* Return */
    return ErrorCode;
}

VOID
WSAAPI
WsThreadCleanup(VOID)
{
}

DWORD
WSAAPI
WsThreadInitialize(IN PWSTHREAD Thread,
                   IN PWSPROCESS Process)
{
    INT ErrorCode = WSASYSCALLFAILURE;
    
    /* Set the process */
    Thread->Process = Process;

    /* Get the helper device */
    if ((WsProcGetAsyncHelper(Process, &Thread->AsyncHelper)) == ERROR_SUCCESS)
    {
        /* Initialize a WAH Thread ID */
        if ((WahOpenCurrentThread(Thread->AsyncHelper,
                                  &Thread->WahThreadId)) == ERROR_SUCCESS)
        {
            /* Success */
            ErrorCode = ERROR_SUCCESS;
        }
    }

    /* Return */
    return ErrorCode;
}

VOID
WSAAPI
WsThreadDelete(IN PWSTHREAD Thread)
{
    /* Remove the blocking hook */
    Thread->BlockingHook = NULL;

    /* Free our buffers */
    if (Thread->Hostent) HeapFree(WsSockHeap, 0, Thread->Hostent);
    if (Thread->Servent) HeapFree(WsSockHeap, 0, Thread->Servent);
    if (Thread->ProtocolInfo) HeapFree(WsSockHeap, 0, Thread->ProtocolInfo);

    /* Clear the TLS */
    TlsSetValue(TlsIndex, NULL);

    /* Close the WAH Handle */
    WahCloseThread(Thread->AsyncHelper, &Thread->WahThreadId);

    /* Unlink the process and free us */
    Thread->Process = NULL;
    HeapFree(WsSockHeap, 0, Thread);
}

VOID
WSAAPI
WsThreadDestroyCurrentThread(VOID)
{
    PWSTHREAD Thread;

    /* Make sure we have TLS */
    if (TlsIndex != TLS_OUT_OF_INDEXES)
    {
        /* Get the thread */
        if ((Thread = TlsGetValue(TlsIndex)))
        {
            /* Delete it */
            WsThreadDelete(Thread);
			TlsSetValue(TlsIndex, 0);
        }
    }
}

DWORD
WSAAPI
WsThreadCreate(IN PWSPROCESS Process,
               IN PWSTHREAD *CurrentThread)
{
    PWSTHREAD Thread = NULL;
    INT ErrorCode = WSASYSCALLFAILURE;
    
    /* Make sure we have TLS */
    if (TlsIndex != TLS_OUT_OF_INDEXES)
    {
        /* Allocate the thread */
        if ((Thread = WsThreadAllocate()))
        {
            /* Initialize it */
            if (WsThreadInitialize(Thread, Process) == ERROR_SUCCESS)
            {
                /* Set the TLS */
                if (TlsSetValue(TlsIndex, Thread))
                {
                    /* Return it and success */
                    *CurrentThread = Thread;
                    ErrorCode = ERROR_SUCCESS;
                }
            }

            /* Check for any failures */
            if (ErrorCode != ERROR_SUCCESS) WsThreadDelete(Thread);
        }
    }

    /* Return */
    return ErrorCode;
}

DWORD
WSAAPI
WsThreadGetCurrentThread(IN PWSPROCESS Process,
                         IN PWSTHREAD *Thread)
{
    /* Get the thread */
    if ((*Thread = TlsGetValue(TlsIndex)))
    {
        /* Success */
        return ERROR_SUCCESS;
    }
    else
    {
        /* We failed, initialize it */
        return WsThreadCreate(Process, Thread);
    }
}

LPWSATHREADID
WSAAPI
WsThreadGetThreadId(IN PWSPROCESS Process)
{
    PWSTHREAD Thread;

    /* Get the thread */
    if ((Thread = TlsGetValue(TlsIndex)))
    {
        /* Return the ID */
        return &Thread->WahThreadId;
    }
    else
    {
        /* Not a valid thread */
        return NULL;
    }
}

PHOSTENT
WSAAPI
WsThreadBlobToHostent(IN PWSTHREAD Thread,
                      IN LPBLOB Blob)
{
    /* Check if our buffer is too small */
    if (Thread->HostentSize < Blob->cbSize)
    {
        /* Delete the current buffer and allocate a new one */
        HeapFree(WsSockHeap, 0, Thread->Hostent);
        Thread->Hostent = HeapAlloc(WsSockHeap, 0, Blob->cbSize);

        /* Set the new size */
        Thread->HostentSize = Blob->cbSize;
    }

    /* Do we have a buffer? */
    if (Thread->Hostent)
    {
        /* Copy the data inside */
        RtlMoveMemory(Thread->Hostent, Blob->pBlobData, Blob->cbSize);
    }
    else
    {
        /* No buffer space! */
        Thread->HostentSize = 0;
        SetLastError(WSA_NOT_ENOUGH_MEMORY);
    }

    /* Return the buffer */
    return (PHOSTENT)Thread->Hostent;
}

PSERVENT
WSAAPI
WsThreadBlobToServent(IN PWSTHREAD Thread,
                      IN LPBLOB Blob)
{
    /* Check if our buffer is too small */
    if (Thread->ServentSize < Blob->cbSize)
    {
        /* Delete the current buffer and allocate a new one */
        HeapFree(WsSockHeap, 0, Thread->Servent);
        Thread->Servent = HeapAlloc(WsSockHeap, 0, Blob->cbSize);

        /* Set the new size */
        Thread->ServentSize = Blob->cbSize;
    }

    /* Do we have a buffer? */
    if (Thread->Servent)
    {
        /* Copy the data inside */
        RtlMoveMemory(Thread->Servent, Blob->pBlobData, Blob->cbSize);
    }
    else
    {
        /* No buffer space! */
        Thread->ServentSize = 0;
        SetLastError(WSA_NOT_ENOUGH_MEMORY);
    }

    /* Return the buffer */
    return (PSERVENT)Thread->Servent;
}

