/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS CSR Sub System
 * FILE:            subsystems/win32/csrss/csrsrv/thredsup.c
 * PURPOSE:         CSR Process Management
 * PROGRAMMERS:     ReactOS Portable Systems Group
 *                  Alex Ionescu
 */

/* INCLUDES *******************************************************************/

#include <srv.h>

#define NDEBUG
#include <debug.h>

#define CsrHashThread(t) \
    (HandleToUlong(t)&(256 - 1))

/* GLOBALS ********************************************************************/

LIST_ENTRY CsrThreadHashTable[256];

/* FUNCTIONS ******************************************************************/

/*++
 * @name ProtectHandle
 * @implemented NT5.2
 *
 * The ProtectHandle routine protects an object handle against closure.
 *
 * @return TRUE or FALSE.
 *
 * @remarks None.
 *
 *--*/
BOOLEAN
NTAPI
ProtectHandle(IN HANDLE ObjectHandle)
{
    NTSTATUS Status;
    OBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleInfo;

    /* Query current state */
    Status = NtQueryObject(ObjectHandle,
                           ObjectHandleFlagInformation,
                           &HandleInfo,
                           sizeof(HandleInfo),
                           NULL);
    if (NT_SUCCESS(Status))
    {
        /* Enable protect from close */
        HandleInfo.ProtectFromClose = TRUE;
        Status = NtSetInformationObject(ObjectHandle,
                                        ObjectHandleFlagInformation,
                                        &HandleInfo,
                                        sizeof(HandleInfo));
        if (NT_SUCCESS(Status)) return TRUE;
    }

    /* We failed to or set the state */
    return FALSE;
}

/*++
 * @name UnProtectHandle
 * @implemented NT5.2
 *
 * The UnProtectHandle routine unprotects an object handle against closure.
 *
 * @return TRUE or FALSE.
 *
 * @remarks None.
 *
 *--*/
BOOLEAN
NTAPI
UnProtectHandle(IN HANDLE ObjectHandle)
{
    NTSTATUS Status;
    OBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleInfo;

    /* Query current state */
    Status = NtQueryObject(ObjectHandle,
                           ObjectHandleFlagInformation,
                           &HandleInfo,
                           sizeof(HandleInfo),
                           NULL);
    if (NT_SUCCESS(Status))
    {
        /* Disable protect from close */
        HandleInfo.ProtectFromClose = FALSE;
        Status = NtSetInformationObject(ObjectHandle,
                                        ObjectHandleFlagInformation,
                                        &HandleInfo,
                                        sizeof(HandleInfo));
        if (NT_SUCCESS(Status)) return TRUE;
    }

    /* We failed to or set the state */
    return FALSE;
}

PCSR_THREAD
NTAPI
CsrAllocateThread(IN PCSR_PROCESS CsrProcess)
{
    PCSR_THREAD CsrThread;

    /* Allocate the structure */
    CsrThread = RtlAllocateHeap(CsrHeap, HEAP_ZERO_MEMORY, sizeof(CSR_THREAD));
    if (!CsrThread) return(NULL);

    /* Reference the Thread and Process */
    CsrThread->ReferenceCount++;
   // CsrProcess->ReferenceCount++;

    /* Set the Parent Process */
    CsrThread->Process = CsrProcess;

    /* Return Thread */
    return CsrThread;
}

PCSR_THREAD
NTAPI
CsrLocateThreadByClientId(OUT PCSR_PROCESS *Process OPTIONAL,
                          IN PCLIENT_ID ClientId)
{
    ULONG i;
    PLIST_ENTRY ListHead, NextEntry;
    PCSR_THREAD FoundThread;

    /* Hash the Thread */
    i = CsrHashThread(ClientId->UniqueThread);

    /* Set the list pointers */
    ListHead = &CsrThreadHashTable[i];
    NextEntry = ListHead->Flink;

    /* Star the loop */
    while (NextEntry != ListHead)
    {
        /* Get the thread */
        FoundThread = CONTAINING_RECORD(NextEntry, CSR_THREAD, HashLinks);

        /* Compare the CID */
        if (FoundThread->ClientId.UniqueThread == ClientId->UniqueThread)
        {
            /* Match found, return the process */
            *Process = FoundThread->Process;

            /* Return thread too */
//            DPRINT1("Found: %p %p\n", FoundThread, FoundThread->Process);
            return FoundThread;
        }

        /* Next */
        NextEntry = NextEntry->Flink;
    }

    /* Nothing found */
    return NULL;
}

PCSR_THREAD
NTAPI
CsrLocateThreadInProcess(IN PCSR_PROCESS CsrProcess OPTIONAL,
                         IN PCLIENT_ID Cid)
{
    PLIST_ENTRY ListHead, NextEntry;
    PCSR_THREAD FoundThread = NULL;

    /* Use the Root Process if none was specified */
    if (!CsrProcess) CsrProcess = CsrRootProcess;

    /* Save the List pointers */
//    DPRINT1("Searching in: %p %d\n", CsrProcess, CsrProcess->ThreadCount);
    ListHead = &CsrProcess->ThreadList;
    NextEntry = ListHead->Flink;

    /* Start the Loop */
    while (NextEntry != ListHead)
    {
        /* Get Thread Entry */
        FoundThread = CONTAINING_RECORD(NextEntry, CSR_THREAD, Link);

        /* Check for TID Match */
        if (FoundThread->ClientId.UniqueThread == Cid->UniqueThread) break;

        /* Next entry */
        NextEntry = NextEntry->Flink;
    }

    /* Return what we found */
//    DPRINT1("Found: %p\n", FoundThread);
    return FoundThread;
}

VOID
NTAPI
CsrInsertThread(IN PCSR_PROCESS Process,
                IN PCSR_THREAD Thread)
{
    ULONG i;

    /* Insert it into the Regular List */
    InsertTailList(&Process->ThreadList, &Thread->Link);

    /* Increase Thread Count */
    Process->ThreadCount++;

    /* Hash the Thread */
    i = CsrHashThread(Thread->ClientId.UniqueThread);
//    DPRINT1("TID %lx HASH: %lx\n", Thread->ClientId.UniqueThread, i);

    /* Insert it there too */
    InsertHeadList(&CsrThreadHashTable[i], &Thread->HashLinks);
}

VOID
NTAPI
CsrDeallocateThread(IN PCSR_THREAD CsrThread)
{
    /* Free the process object from the heap */
    RtlFreeHeap(CsrHeap, 0, CsrThread);
}

VOID
NTAPI
CsrRemoveThread(IN PCSR_THREAD CsrThread)
{
    /* Remove it from the List */
    RemoveEntryList(&CsrThread->Link);

    /* Decreate the thread count of the process */
    CsrThread->Process->ThreadCount--;

    /* Remove it from the Hash List as well */
    if (CsrThread->HashLinks.Flink) RemoveEntryList(&CsrThread->HashLinks);

    /* Check if this is the last Thread */
    if (!CsrThread->Process->ThreadCount)
    {
        /* Check if it's not already been marked for deletion */
        if (!(CsrThread->Process->Flags & CsrProcessLastThreadTerminated))
        {
            /* Let everyone know this process is about to lose the thread */
            //CsrThread->Process->Flags |= CsrProcessLastThreadTerminated;

            /* Reference the Process */
            //CsrLockedDereferenceProcess(CsrThread->Process);
        }
    }

    /* Mark the thread for deletion */
    CsrThread->Flags |= CsrThreadInTermination;
}

VOID
NTAPI
CsrThreadRefcountZero(IN PCSR_THREAD CsrThread)
{
    NTSTATUS Status;

    /* Remove this thread */
    CsrRemoveThread(CsrThread);

    /* Release the Process Lock */
    //CsrReleaseProcessLock();

    /* Close the NT Thread Handle */
    if (CsrThread->ThreadHandle)
    {
        UnProtectHandle(CsrThread->ThreadHandle);
        Status = NtClose(CsrThread->ThreadHandle);
        ASSERT(NT_SUCCESS(Status));
    }

    /* De-allocate the CSR Thread Object */
    CsrDeallocateThread(CsrThread);

    /* Remove a reference from the process */
    //CsrDereferenceProcess(CsrProcess);
}

NTSTATUS
NTAPI
CsrCreateThread(IN PCSR_PROCESS CsrProcess,
                IN HANDLE hThread,
                IN PCLIENT_ID ClientId)
{
    PCSR_THREAD CsrThread;
    //PCSR_PROCESS CurrentProcess;
    //PCSR_THREAD CurrentThread = NtCurrentTeb()->CsrClientThread;
    //CLIENT_ID CurrentCid;
    KERNEL_USER_TIMES KernelTimes;

//    DPRINT1("CSRSRV: %s called\n", __FUNCTION__);

    /* Get the current thread and CID */
    //CurrentCid = CurrentThread->ClientId;
//    DPRINT1("CALLER PID/TID: %lx/%lx\n", CurrentCid.UniqueProcess, CurrentCid.UniqueThread);

    /* Acquire the Process Lock */
    CsrAcquireProcessLock();
#if 0
    /* Get the current Process and make sure the Thread is valid with this CID */
    CurrentThread = CsrLocateThreadByClientId(&CurrentProcess,
                                              &CurrentCid);

    /* Something is wrong if we get an empty thread back */
    if (!CurrentThread)
    {
        DPRINT1("CSRSRV:%s: invalid thread!\n", __FUNCTION__);
        CsrReleaseProcessLock();
        return STATUS_THREAD_IS_TERMINATING;
    }
#endif
    /* Get the Thread Create Time */
    NtQueryInformationThread(hThread,
                             ThreadTimes,
                             (PVOID)&KernelTimes,
                             sizeof(KernelTimes),
                             NULL);

    /* Allocate a CSR Thread Structure */
    if (!(CsrThread = CsrAllocateThread(CsrProcess)))
    {
        DPRINT1("CSRSRV:%s: out of memory!\n", __FUNCTION__);
        CsrReleaseProcessLock();
        return STATUS_NO_MEMORY;
    }

    /* Save the data we have */
    CsrThread->CreateTime = KernelTimes.CreateTime;
    CsrThread->ClientId = *ClientId;
    CsrThread->ThreadHandle = hThread;
    CsrThread->Flags = 0;

    /* Insert the Thread into the Process */
    CsrInsertThread(CsrProcess, CsrThread);

    /* Release the lock and return */
    CsrReleaseProcessLock();
    return STATUS_SUCCESS;
}

/*++
 * @name CsrAddStaticServerThread
 * @implemented NT4
 *
 * The CsrAddStaticServerThread routine adds a new CSR Thread to the
 * CSR Server Process (CsrRootProcess).
 *
 * @param hThread
 *        Handle to an existing NT Thread to which to associate this
 *        CSR Thread.
 *
 * @param ClientId
 *        Pointer to the Client ID structure of the NT Thread to associate
 *        with this CSR Thread.
 *
 * @param ThreadFlags
 *        Initial CSR Thread Flags to associate to this CSR Thread. Usually
 *        CsrThreadIsServerThread.
 *
 * @return Pointer to the newly allocated CSR Thread.
 *
 * @remarks None.
 *
 *--*/
PCSR_THREAD
NTAPI
CsrAddStaticServerThread(IN HANDLE hThread,
                         IN PCLIENT_ID ClientId,
                         IN ULONG ThreadFlags)
{
    PCSR_THREAD CsrThread;

    /* Get the Lock */
    CsrAcquireProcessLock();

    /* Allocate the Server Thread */
    CsrThread = CsrAllocateThread(CsrRootProcess);
    if (CsrThread)
    {
        /* Setup the Object */
        CsrThread->ThreadHandle = hThread;
        ProtectHandle(hThread);
        CsrThread->ClientId = *ClientId;
        CsrThread->Flags = ThreadFlags;

        /* Insert it into the Thread List */
        InsertTailList(&CsrRootProcess->ThreadList, &CsrThread->Link);

        /* Increment the thread count */
        CsrRootProcess->ThreadCount++;
    }
    else
    {
        DPRINT1("CsrAddStaticServerThread: alloc failed for thread 0x%x\n", hThread);
    }

    /* Release the Process Lock and return */
    CsrReleaseProcessLock();
    return CsrThread;
}

/* EOF */
