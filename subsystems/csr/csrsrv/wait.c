/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS CSR Sub System
 * FILE:            subsys/csr/csrsrv/wait.c
 * PURPOSE:         CSR Server DLL Wait Implementation
 * PROGRAMMERS:     Emanuele Aliberti
 *                  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include "srv.h"

#define NDEBUG
#include <debug.h>

/* DATA **********************************************************************/

RTL_CRITICAL_SECTION CsrWaitListsLock;

/* PRIVATE FUNCTIONS *********************************************************/

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
 * @return TRUE in case of success, FALSE othwerwise.
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
    if (!(WaitBlock = RtlAllocateHeap(CsrHeap, 0, Size)))
    {
        /* Fail */
        WaitApiMessage->Status = STATUS_NO_MEMORY;
        return FALSE;
    }

    /* Initialize it */
    WaitBlock->Size = Size;
    WaitBlock->WaitThread = CsrWaitThread;
    WaitBlock->WaitContext = WaitContext;
    WaitBlock->WaitFunction = WaitFunction;
    InitializeListHead(&WaitBlock->UserWaitList);
    InitializeListHead(&WaitBlock->WaitList);

    /* Copy the message */
    RtlMoveMemory(&WaitBlock->WaitApiMessage,
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
 *        Pointer to the CSR Wait Block
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
    if ((WaitBlock->WaitFunction)(WaitList,
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
                    (PPORT_MESSAGE)&WaitBlock->WaitApiMessage);

        /* Check if we should dereference the thread */
        if (DereferenceThread)
        {
            /* Remove it from the Wait List */
            if (WaitBlock->WaitList.Flink)
            {
                RemoveEntryList(&WaitBlock->WaitList);
            }

            /* Remove it from the User Wait List */
            if (WaitBlock->UserWaitList.Flink)
            {
                RemoveEntryList(&WaitBlock->UserWaitList);
            }

            /* Dereference teh thread */
            CsrDereferenceThread(WaitBlock->WaitThread);

            /* Free the wait block */
            RtlFreeHeap(CsrHeap, 0, WaitBlock);
        }
        else
        {
            /* The wait is complete, but the thread is being kept alive */
            WaitBlock->WaitFunction = NULL;
        }
    
        /* The wait suceeded*/
        return TRUE;
    }
    
    /* The wait failed */
    return FALSE;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name CsrCreateWait
 * @implemented NT4
 *
 * The CsrCreateWait routine creates a CSR Wait.
 *
 * @param WaitList
 *        Pointer to a list entry of the waits to associate.
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
 * @param UserWaitList
 *        Pointer to a list entry of the user-defined waits to associate.
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
              IN PVOID WaitContext,
              IN PLIST_ENTRY UserWaitList OPTIONAL)
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
    if (CsrWaitThread && (CsrWaitThread->Flags & CsrThreadTerminated))
    {
        /* Fail the wait */
        CsrWaitThread->WaitBlock = NULL;
        RtlFreeHeap(CsrHeap, 0, WaitBlock);
        CsrReleaseWaitLock();
        return FALSE;
    }

    /* Insert the wait in the queue */
    InsertTailList(WaitList, &WaitBlock->WaitList);

    /* Insert the User Wait too, if one was given */
    if (UserWaitList) InsertTailList(UserWaitList, &WaitBlock->UserWaitList);

    /* Return */
    CsrReleaseWaitLock();
    return TRUE;
}

/*++
 * @name CsrDereferenceWait
 * @implemented NT4
 *
 * The CsrDereferenceWait routine derefences a CSR Wait Block.
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
    PLIST_ENTRY ListHead, NextEntry;
    PCSR_WAIT_BLOCK WaitBlock;

    /* Acquire the Process and Wait Locks */
    CsrAcquireProcessLock();
    CsrAcquireWaitLock();

    /* Set the list pointers */
    ListHead = WaitList;
    NextEntry = ListHead->Flink;

    /* Start the loop */
    while (NextEntry != ListHead)
    {
        /* Get the wait block */
        WaitBlock = CONTAINING_RECORD(NextEntry, CSR_WAIT_BLOCK, WaitList);

        /* Move to the next entry */
        NextEntry = NextEntry->Flink;

        /* Check if there's no Wait Routine */
        if (!WaitBlock->WaitFunction)
        {
            /* Remove it from the Wait List */
            if (WaitBlock->WaitList.Flink)
            {
                RemoveEntryList(&WaitBlock->WaitList);
            }

            /* Remove it from the User Wait List */
            if (WaitBlock->UserWaitList.Flink)
            {
                RemoveEntryList(&WaitBlock->UserWaitList);
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
 * The CsrMoveSatisfiedWait routine moves satisfied waits from a wait list
 * to another list entry.
 *
 * @param NewEntry
 *        Pointer to a list entry where the satisfied waits will be added.
 *
 * @param WaitList
 *        Pointer to a list entry to analyze for satisfied waits.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
CsrMoveSatisfiedWait(IN PLIST_ENTRY NewEntry,
                     IN PLIST_ENTRY WaitList)
{
    PLIST_ENTRY ListHead, NextEntry;
    PCSR_WAIT_BLOCK WaitBlock;

    /* Acquire the Wait Lock */
    CsrAcquireWaitLock();

    /* Set the List pointers */
    ListHead = WaitList;
    NextEntry = ListHead->Flink;

    /* Start looping */
    while (NextEntry != ListHead)
    {
        /* Get the Wait block */
        WaitBlock = CONTAINING_RECORD(NextEntry, CSR_WAIT_BLOCK, WaitList);

        /* Go to the next entry */
        NextEntry = NextEntry->Flink;

        /* Check if there is a Wait Callback */
        if (WaitBlock->WaitFunction)
        {
            /* Remove it from the Wait Block Queue */
            RemoveEntryList(&WaitBlock->WaitList);

            /* Insert the new entry */
            InsertTailList(&WaitBlock->WaitList, NewEntry);
        }
    }

    /* Release the wait lock */
    CsrReleaseWaitLock();
}

/*++
 * @name CsrNotifyWait
 * @implemented NT4
 *
 * The CsrNotifyWait notifies a CSR Wait Block.
 *
 * @param WaitList
 *        Pointer to the list entry for this wait.
 *
 * @param WaitType
 *        Type of the wait to perform, either WaitAny or WaitAll.
 *
 * @param WaitArgument[1-2]
 *        User-defined argument to pass on to the wait function.
 *
 * @return TRUE in case of success, FALSE othwerwise.
 *
 * @remarks None.
 *
 *--*/
BOOLEAN
NTAPI
CsrNotifyWait(IN PLIST_ENTRY WaitList,
              IN ULONG WaitType,
              IN PVOID WaitArgument1,
              IN PVOID WaitArgument2)
{
    PLIST_ENTRY ListHead, NextEntry;
    PCSR_WAIT_BLOCK WaitBlock;
    BOOLEAN NotifySuccess = FALSE;

    /* Acquire the Wait Lock */
    CsrAcquireWaitLock();

    /* Set the List pointers */
    ListHead = WaitList;
    NextEntry = ListHead->Flink;

    /* Start looping */
    while (NextEntry != ListHead)
    {
        /* Get the Wait block */
        WaitBlock = CONTAINING_RECORD(NextEntry, CSR_WAIT_BLOCK, WaitList);

        /* Go to the next entry */
        NextEntry = NextEntry->Flink;

        /* Check if there is a Wait Callback */
        if (WaitBlock->WaitFunction)
        {
            /* Notify the Waiter */
            NotifySuccess |= CsrNotifyWaitBlock(WaitBlock,
                                                WaitList,
                                                WaitArgument1,
                                                WaitArgument2,
                                                0,
                                                FALSE);
            
            /* We've already done a wait, so leave unless this is a Wait All */
            if (WaitType != WaitAll) break;
        }
    }

    /* Release the wait lock and return */
    CsrReleaseWaitLock();
    return NotifySuccess;
}

/* EOF */
