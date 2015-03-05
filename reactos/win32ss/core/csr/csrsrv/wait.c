/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Client/Server Runtime SubSystem
 * FILE:            subsystems/win32/csrsrv/wait.c
 * PURPOSE:         CSR Server DLL Wait Implementation
 * PROGRAMMERS:     Emanuele Aliberti
 *                  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *******************************************************************/

#include "srv.h"

#define NDEBUG
#include <debug.h>

/* DATA ***********************************************************************/

RTL_CRITICAL_SECTION CsrWaitListsLock;

/* PRIVATE FUNCTIONS **********************************************************/

/*++
 * @name CsrInitializeWait
 *
 * The CsrInitializeWait routine initializes a CSR Wait Object.
 *
 * @param WaitFunction
 *        Pointer to the function that will handle this wait.
 *
 * @param CsrWaitThread
 *        Pointer to the CSR Thread that will perform the wait.
 *
 * @param WaitApiMessage
 *        Pointer to the CSR API Message associated to this wait.
 *
 * @param WaitContext
 *        Pointer to a user-defined parameter associated to this wait.
 *
 * @param NewWaitBlock
 *        Pointed to the initialized CSR Wait Block for this wait.
 *
 * @return TRUE in case of success, FALSE otherwise.
 *
 * @remarks None.
 *
 *--*/
BOOLEAN
NTAPI
CsrInitializeWait(IN CSR_WAIT_FUNCTION WaitFunction,
                  IN PCSR_THREAD CsrWaitThread,
                  IN OUT PCSR_API_MESSAGE WaitApiMessage,
                  IN PVOID WaitContext,
                  OUT PCSR_WAIT_BLOCK *NewWaitBlock)
{
    ULONG Size;
    PCSR_WAIT_BLOCK WaitBlock;

    /* Calculate the size of the wait block */
    Size = sizeof(CSR_WAIT_BLOCK) -
           sizeof(WaitBlock->WaitApiMessage) +
           WaitApiMessage->Header.u1.s1.TotalLength;

    /* Allocate the Wait Block */
    WaitBlock = RtlAllocateHeap(CsrHeap, HEAP_ZERO_MEMORY, Size);
    if (!WaitBlock)
    {
        /* Fail */
        WaitApiMessage->Status = STATUS_NO_MEMORY;
        return FALSE;
    }

    /* Initialize it */
    WaitBlock->Size           = Size;
    WaitBlock->WaitThread     = CsrWaitThread;
    WaitBlock->WaitContext    = WaitContext;
    WaitBlock->WaitFunction   = WaitFunction;
    WaitBlock->WaitList.Flink = NULL;
    WaitBlock->WaitList.Blink = NULL;

    /* Copy the message */
    RtlCopyMemory(&WaitBlock->WaitApiMessage,
                  WaitApiMessage,
                  WaitApiMessage->Header.u1.s1.TotalLength);

    /* Return the block */
    *NewWaitBlock = WaitBlock;
    return TRUE;
}

/*++
 * @name CsrNotifyWaitBlock
 *
 * The CsrNotifyWaitBlock routine calls the wait function for a registered
 * CSR Wait Block, and replies to the attached CSR API Message, if any.
 *
 * @param WaitBlock
 *        Pointer to the CSR Wait Block.
 *
 * @param WaitList
 *        Pointer to the wait list for this wait.
 *
 * @param WaitArgument[1-2]
 *        User-defined values to pass to the wait function.
 *
 * @param WaitFlags
 *        Wait flags for this wait.
 *
 * @param DereferenceThread
 *        Specifies whether the CSR Thread should be dereferenced at the
 *        end of this wait.
 *
 * @return TRUE in case of success, FALSE otherwise.
 *
 * @remarks After a wait block is notified, the wait function becomes invalid.
 *
 *--*/
BOOLEAN
NTAPI
CsrNotifyWaitBlock(IN PCSR_WAIT_BLOCK WaitBlock,
                   IN PLIST_ENTRY WaitList,
                   IN PVOID WaitArgument1,
                   IN PVOID WaitArgument2,
                   IN ULONG WaitFlags,
                   IN BOOLEAN DereferenceThread)
{
    /* Call the wait function */
    if (WaitBlock->WaitFunction(WaitList,
                                WaitBlock->WaitThread,
                                &WaitBlock->WaitApiMessage,
                                WaitBlock->WaitContext,
                                WaitArgument1,
                                WaitArgument2,
                                WaitFlags))
    {
        /* The wait is done, clear the block */
        WaitBlock->WaitThread->WaitBlock = NULL;

        /* Check for captured arguments */
        if (WaitBlock->WaitApiMessage.CsrCaptureData)
        {
            /* Release them */
            CsrReleaseCapturedArguments(&WaitBlock->WaitApiMessage);
        }

        /* Reply to the port */
        NtReplyPort(WaitBlock->WaitThread->Process->ClientPort,
                    &WaitBlock->WaitApiMessage.Header);

        /* Check if we should dereference the thread */
        if (DereferenceThread)
        {
            /* Remove it from the Wait List */
            if (WaitBlock->WaitList.Flink)
            {
                RemoveEntryList(&WaitBlock->WaitList);
            }

            /* Dereference the thread */
            CsrDereferenceThread(WaitBlock->WaitThread);

            /* Free the wait block */
            RtlFreeHeap(CsrHeap, 0, WaitBlock);
        }
        else
        {
            /* The wait is complete, but the thread is being kept alive */
            WaitBlock->WaitFunction = NULL;
        }

        /* The wait succeeded */
        return TRUE;
    }

    /* The wait failed */
    return FALSE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*++
 * @name CsrCreateWait
 * @implemented NT4
 *
 * The CsrCreateWait routine creates a CSR Wait.
 *
 * @param WaitList
 *        Pointer to a wait list in which the wait will be added.
 *
 * @param WaitFunction
 *        Pointer to the function that will handle this wait.
 *
 * @param CsrWaitThread
 *        Pointer to the CSR Thread that will perform the wait.
 *
 * @param WaitApiMessage
 *        Pointer to the CSR API Message associated to this wait.
 *
 * @param WaitContext
 *        Pointer to a user-defined parameter associated to this wait.
 *
 * @return TRUE in case of success, FALSE otherwise.
 *
 * @remarks None.
 *
 *--*/
BOOLEAN
NTAPI
CsrCreateWait(IN PLIST_ENTRY WaitList,
              IN CSR_WAIT_FUNCTION WaitFunction,
              IN PCSR_THREAD CsrWaitThread,
              IN OUT PCSR_API_MESSAGE WaitApiMessage,
              IN PVOID WaitContext)
{
    PCSR_WAIT_BLOCK WaitBlock;

    /* Initialize the wait */
    if (!CsrInitializeWait(WaitFunction,
                           CsrWaitThread,
                           WaitApiMessage,
                           WaitContext,
                           &WaitBlock))
    {
        return FALSE;
    }

    /* Acquire the Wait Lock */
    CsrAcquireWaitLock();

    /* Make sure the thread wasn't destroyed */
    if (CsrWaitThread->Flags & CsrThreadTerminated)
    {
        /* Fail the wait */
        RtlFreeHeap(CsrHeap, 0, WaitBlock);
        CsrReleaseWaitLock();
        return FALSE;
    }

    /* Associate the newly created wait to the waiting thread */
    CsrWaitThread->WaitBlock = WaitBlock;

    /* Insert the wait in the queue */
    InsertTailList(WaitList, &WaitBlock->WaitList);

    /* Return */
    CsrReleaseWaitLock();
    return TRUE;
}

/*++
 * @name CsrDereferenceWait
 * @implemented NT4
 *
 * The CsrDereferenceWait routine dereferences a CSR Wait Block.
 *
 * @param WaitList
 *        Pointer to the Wait List associated to the wait.

 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
CsrDereferenceWait(IN PLIST_ENTRY WaitList)
{
    PLIST_ENTRY NextEntry;
    PCSR_WAIT_BLOCK WaitBlock;

    /* Acquire the Process and Wait Locks */
    CsrAcquireProcessLock();
    CsrAcquireWaitLock();

    /* Set the list pointers */
    NextEntry = WaitList->Flink;

    /* Start the loop */
    while (NextEntry != WaitList)
    {
        /* Get the wait block */
        WaitBlock = CONTAINING_RECORD(NextEntry, CSR_WAIT_BLOCK, WaitList);

        /* Move to the next entry */
        NextEntry = NextEntry->Flink;

        /* Check if there's no Wait Routine (satisfied wait) */
        if (WaitBlock->WaitFunction == NULL)
        {
            /* Remove it from the Wait List */
            if (WaitBlock->WaitList.Flink)
            {
                RemoveEntryList(&WaitBlock->WaitList);
            }

            /* Dereference the thread waiting on it */
            CsrDereferenceThread(WaitBlock->WaitThread);

            /* Free the block */
            RtlFreeHeap(CsrHeap, 0, WaitBlock);
        }
    }

    /* Release the locks */
    CsrReleaseWaitLock();
    CsrReleaseProcessLock();
}

/*++
 * @name CsrMoveSatisfiedWait
 * @implemented NT5
 *
 * The CsrMoveSatisfiedWait routine moves satisfied waits from
 * a wait list to another list.
 *
 * @param DestinationList
 *        Pointer to a list in which the satisfied waits will be added.
 *
 * @param WaitList
 *        Pointer to the wait list whose satisfied wait blocks
 *        will be moved away.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
CsrMoveSatisfiedWait(IN PLIST_ENTRY DestinationList,
                     IN PLIST_ENTRY WaitList)
{
    PLIST_ENTRY NextEntry;
    PCSR_WAIT_BLOCK WaitBlock;

    /* Acquire the Wait Lock */
    CsrAcquireWaitLock();

    /* Set the List pointers */
    NextEntry = WaitList->Flink;

    /* Start looping */
    while (NextEntry != WaitList)
    {
        /* Get the Wait block */
        WaitBlock = CONTAINING_RECORD(NextEntry, CSR_WAIT_BLOCK, WaitList);

        /* Go to the next entry */
        NextEntry = NextEntry->Flink;

        /* Check if there's no Wait Routine (satisfied wait) */
        if (WaitBlock->WaitFunction == NULL)
        {
            /* Remove it from the Wait Block Queue */
            RemoveEntryList(&WaitBlock->WaitList);

            /* Insert the wait into the destination list */
            InsertTailList(DestinationList, &WaitBlock->WaitList);
        }
    }

    /* Release the wait lock */
    CsrReleaseWaitLock();
}

/*++
 * @name CsrNotifyWait
 * @implemented NT4
 *
 * The CsrNotifyWait routine notifies CSR Wait Blocks.
 *
 * @param WaitList
 *        Pointer to the wait list whose wait blocks will be notified.
 *
 * @param NotifyAll
 *        Whether or not we must notify all the waits.
 *
 * @param WaitArgument[1-2]
 *        User-defined argument to pass on to the wait function.
 *
 * @return TRUE in case of success, FALSE otherwise.
 *
 * @remarks None.
 *
 *--*/
BOOLEAN
NTAPI
CsrNotifyWait(IN PLIST_ENTRY WaitList,
              IN BOOLEAN NotifyAll,
              IN PVOID WaitArgument1,
              IN PVOID WaitArgument2)
{
    PLIST_ENTRY NextEntry;
    PCSR_WAIT_BLOCK WaitBlock;
    BOOLEAN NotifySuccess = FALSE;

    /* Acquire the Wait Lock */
    CsrAcquireWaitLock();

    /* Set the List pointers */
    NextEntry = WaitList->Flink;

    /* Start looping */
    while (NextEntry != WaitList)
    {
        /* Get the Wait block */
        WaitBlock = CONTAINING_RECORD(NextEntry, CSR_WAIT_BLOCK, WaitList);

        /* Go to the next entry */
        NextEntry = NextEntry->Flink;

        /* Check if there is a Wait Routine */
        if (WaitBlock->WaitFunction != NULL)
        {
            /* Notify the Waiter */
            NotifySuccess |= CsrNotifyWaitBlock(WaitBlock,
                                                WaitList,
                                                WaitArgument1,
                                                WaitArgument2,
                                                0,
                                                FALSE);

            /*
             * We've already done a wait, so leave unless
             * we want to notify all the waits...
             */
            if (!NotifyAll) break;
        }
    }

    /* Release the wait lock and return */
    CsrReleaseWaitLock();
    return NotifySuccess;
}

/* EOF */
