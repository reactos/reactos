/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/include/lpc_x.h
* PURPOSE:         Intenral Inlined Functions for Local Procedure Call
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

//
// Gets the message type, removing the kernel-mode flag
//
#define LpcpGetMessageType(x)                               \
    ((x)->u2.s2.Type &~ LPC_KERNELMODE_MESSAGE)

//
// Waits on an LPC semaphore for a receive operation
//
#define LpcpReceiveWait(s, w)                               \
{                                                           \
    LPCTRACE(LPC_REPLY_DEBUG, "Wait: %p\n", s);             \
    Status = KeWaitForSingleObject(s,                       \
                                   WrLpcReceive,            \
                                   w,                       \
                                   FALSE,                   \
                                   NULL);                   \
    LPCTRACE(LPC_REPLY_DEBUG, "Wait done: %lx\n", Status);  \
}

//
// Waits on an LPC semaphore for a reply operation
//
#define LpcpReplyWait(s, w)                                 \
{                                                           \
    LPCTRACE(LPC_SEND_DEBUG, "Wait: %p\n", s);              \
    Status = KeWaitForSingleObject(s,                       \
                                   WrLpcReply,              \
                                   w,                       \
                                   FALSE,                   \
                                   NULL);                   \
    LPCTRACE(LPC_SEND_DEBUG, "Wait done: %lx\n", Status);   \
    if (Status == STATUS_USER_APC)                          \
    {                                                       \
        /* We were preempted by an APC */                   \
        if (KeReadStateSemaphore(s))                        \
        {                                                   \
            /* It's still signaled, so wait on it */        \
            KeWaitForSingleObject(s,                        \
                                  WrExecutive,              \
                                  KernelMode,               \
                                  FALSE,                    \
                                  NULL);                    \
            Status = STATUS_SUCCESS;                        \
        }                                                   \
    }                                                       \
}

//
// Waits on an LPC semaphore for a connect operation
//
#define LpcpConnectWait(s, w)                               \
{                                                           \
    LPCTRACE(LPC_CONNECT_DEBUG, "Wait: %p\n", s);           \
    Status = KeWaitForSingleObject(s,                       \
                                   Executive,               \
                                   w,                       \
                                   FALSE,                   \
                                   NULL);                   \
    LPCTRACE(LPC_CONNECT_DEBUG, "Wait done: %lx\n", Status);\
    if (Status == STATUS_USER_APC)                          \
    {                                                       \
        /* We were preempted by an APC */                   \
        if (KeReadStateSemaphore(s))                        \
        {                                                   \
            /* It's still signaled, so wait on it */        \
            KeWaitForSingleObject(s,                        \
                                  WrExecutive,              \
                                  KernelMode,               \
                                  FALSE,                    \
                                  NULL);                    \
            Status = STATUS_SUCCESS;                        \
        }                                                   \
    }                                                       \
}

//
// Releases an LPC Semaphore to complete a wait
//
#define LpcpCompleteWait(s)                                 \
{                                                           \
    /* Release the semaphore */                             \
    LPCTRACE(LPC_SEND_DEBUG, "Release: %p\n", s);           \
    KeReleaseSemaphore(s, 1, 1, FALSE);                     \
}

//
// Allocates a new message
//
FORCEINLINE
PLPCP_MESSAGE
LpcpAllocateFromPortZone(VOID)
{
    PLPCP_MESSAGE Message;

    /* Allocate a message from the port zone while holding the lock */
    KeAcquireGuardedMutex(&LpcpLock);
    Message = ExAllocateFromPagedLookasideList(&LpcpMessagesLookaside);
    if (!Message)
    {
        /* Fail, and let caller cleanup */
        KeReleaseGuardedMutex(&LpcpLock);
        return NULL;
    }

    /* Initialize it */
    InitializeListHead(&Message->Entry);
    Message->RepliedToThread = NULL;
    Message->Request.u2.ZeroInit = 0;

    /* Release the lock */
    KeReleaseGuardedMutex(&LpcpLock);
    return Message;
}

//
// Get the LPC Message associated to the Thread
//
FORCEINLINE
PLPCP_MESSAGE
LpcpGetMessageFromThread(IN PETHREAD Thread)
{
    /* Check if the port flag is set */
    if (((ULONG_PTR)Thread->LpcReplyMessage) & LPCP_THREAD_FLAG_IS_PORT)
    {
        /* The pointer is actually a port, not a message, so return NULL */
        return NULL;
    }

    /* Otherwise, this is a message. Return the pointer */
    return (PVOID)((ULONG_PTR)Thread->LpcReplyMessage & ~LPCP_THREAD_FLAGS);
}

FORCEINLINE
PLPCP_PORT_OBJECT
LpcpGetPortFromThread(IN PETHREAD Thread)
{
    /* Check if the port flag is set */
    if (((ULONG_PTR)Thread->LpcReplyMessage) & LPCP_THREAD_FLAG_IS_PORT)
    {
        /* The pointer is actually a port, return it */
        return (PVOID)((ULONG_PTR)Thread->LpcWaitingOnPort &
                       ~LPCP_THREAD_FLAGS);
    }

    /* Otherwise, this is a message. There is nothing to return */
    return NULL;
}

FORCEINLINE
VOID
LpcpSetPortToThread(IN PETHREAD Thread,
                    IN PLPCP_PORT_OBJECT Port)
{
    /* Set the port object */
    Thread->LpcWaitingOnPort = (PVOID)(((ULONG_PTR)Port) |
                                       LPCP_THREAD_FLAG_IS_PORT);
}
