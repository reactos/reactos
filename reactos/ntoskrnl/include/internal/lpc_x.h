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
                                  Executive,                \
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
                                  Executive,                \
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
PLPCP_MESSAGE
FORCEINLINE
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
