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
    CsrProcess->ReferenceCount++;

    /* Set the Parent Process */
    CsrThread->Process = CsrProcess;

    /* Return Thread */
    return CsrThread;
}

/*++
 * @name CsrLockedReferenceThread
 *
 * The CsrLockedReferenceThread refences a CSR Thread while the
 * Process Lock is already being held.
 *
 * @param CsrThread
 *        Pointer to the CSR Thread to be referenced.
 *
 * @return None.
 *
 * @remarks This routine will return with the Process Lock held.
 *
 *--*/
VOID
NTAPI
CsrLockedReferenceThread(IN PCSR_THREAD CsrThread)
{
    /* Increment the reference count */
    ++CsrThread->ReferenceCount;
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
    ASSERT(ProcessStructureListLocked());

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
            CsrThread->Process->Flags |= CsrProcessLastThreadTerminated;

            /* Reference the Process */
            CsrLockedDereferenceProcess(CsrThread->Process);
        }
    }

    /* Mark the thread for deletion */
    CsrThread->Flags |= CsrThreadInTermination;
}

/*++
 * @name CsrCreateRemoteThread
 * @implemented NT4
 *
 * The CsrCreateRemoteThread routine creates a CSR Thread object for
 * an NT Thread which is not part of the current NT Process.
 *
 * @param hThread
 *        Handle to an existing NT Thread to which to associate this
 *        CSR Thread.
 *
 * @param ClientId
 *        Pointer to the Client ID structure of the NT Thread to associate
 *        with this CSR Thread.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
 *         othwerwise.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrCreateRemoteThread(IN HANDLE hThread,
                      IN PCLIENT_ID ClientId)
{
    NTSTATUS Status;
    HANDLE ThreadHandle;
    PCSR_THREAD CsrThread;
    PCSR_PROCESS CsrProcess;
    KERNEL_USER_TIMES KernelTimes;
    DPRINT("CSRSRV: %s called\n", __FUNCTION__);

    /* Get the Thread Create Time */
    Status = NtQueryInformationThread(hThread,
                                      ThreadTimes,
                                      &KernelTimes,
                                      sizeof(KernelTimes),
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to query thread times: %lx\n", Status);
        return Status;
    }

    /* Lock the Owner Process */
    Status = CsrLockProcessByClientId(&ClientId->UniqueProcess, &CsrProcess);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("No known process for %lx\n", ClientId->UniqueProcess);
        return Status;
    }
    
    /* Make sure the thread didn't terminate */
    if (KernelTimes.ExitTime.QuadPart)
    {
        /* Unlock the process and return */
        CsrUnlockProcess(CsrProcess);
        DPRINT1("Dead thread: %I64x\n", KernelTimes.ExitTime.QuadPart);
        return STATUS_THREAD_IS_TERMINATING;
    }

    /* Allocate a CSR Thread Structure */
    CsrThread = CsrAllocateThread(CsrProcess);
    if (!CsrThread)
    {
        DPRINT1("CSRSRV:%s: out of memory!\n", __FUNCTION__);
        CsrUnlockProcess(CsrProcess);
        return STATUS_NO_MEMORY;
    }

    /* Duplicate the Thread Handle */
    Status = NtDuplicateObject(NtCurrentProcess(),
                               hThread,
                               NtCurrentProcess(),
                               &ThreadHandle,
                               0,
                               0,
                               DUPLICATE_SAME_ACCESS);
    /* Allow failure */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Thread duplication failed: %lx\n", Status);
        ThreadHandle = hThread;
    }

    /* Save the data we have */
    CsrThread->CreateTime = KernelTimes.CreateTime;
    CsrThread->ClientId = *ClientId;
    CsrThread->ThreadHandle = ThreadHandle;
    ProtectHandle(ThreadHandle);
    CsrThread->Flags = 0;

    /* Insert the Thread into the Process */
    CsrInsertThread(CsrProcess, CsrThread);

    /* Release the lock and return */
    CsrUnlockProcess(CsrProcess);
    return STATUS_SUCCESS;
}

VOID
NTAPI
CsrThreadRefcountZero(IN PCSR_THREAD CsrThread)
{
    PCSR_PROCESS CsrProcess = CsrThread->Process;
    NTSTATUS Status;
    ASSERT(ProcessStructureListLocked());

    /* Remove this thread */
    CsrRemoveThread(CsrThread);

    /* Release the Process Lock */
    CsrReleaseProcessLock();

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
    CsrDereferenceProcess(CsrProcess);
}

/*++
 * @name CsrDestroyThread
 * @implemented NT4
 *
 * The CsrDestroyThread routine destroys the CSR Thread corresponding to
 * a given Thread ID.
 *
 * @param Cid
 *        Pointer to the Client ID Structure corresponding to the CSR
 *        Thread which is about to be destroyed.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_THREAD_IS_TERMINATING
 *         if the CSR Thread is already terminating.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrDestroyThread(IN PCLIENT_ID Cid)
{
    CLIENT_ID ClientId = *Cid;
    PCSR_THREAD CsrThread;
    PCSR_PROCESS CsrProcess;

    /* Acquire lock */
    CsrAcquireProcessLock();

    /* Find the thread */
    CsrThread = CsrLocateThreadByClientId(&CsrProcess,
                                          &ClientId);

    /* Make sure we got one back, and that it's not already gone */
    if (!CsrThread || CsrThread->Flags & CsrThreadTerminated)
    {
        /* Release the lock and return failure */
        CsrReleaseProcessLock();
        return STATUS_THREAD_IS_TERMINATING;
    }

    /* Set the terminated flag */
    CsrThread->Flags |= CsrThreadTerminated;

    /* Acquire the Wait Lock */
    CsrAcquireWaitLock();

    /* Do we have an active wait block? */
    if (CsrThread->WaitBlock)
    {
        /* Notify waiters of termination */
        CsrNotifyWaitBlock(CsrThread->WaitBlock,
                           NULL,
                           NULL,
                           NULL,
                           CsrProcessTerminating,
                           TRUE);
    }

    /* Release the Wait Lock */
    CsrReleaseWaitLock();

    /* Dereference the thread */
    CsrLockedDereferenceThread(CsrThread);

    /* Release the Process Lock and return success */
    CsrReleaseProcessLock();
    return STATUS_SUCCESS;
}

/*++
 * @name CsrLockedDereferenceThread
 *
 * The CsrLockedDereferenceThread derefences a CSR Thread while the
 * Process Lock is already being held.
 *
 * @param CsrThread
 *        Pointer to the CSR Thread to be dereferenced.
 *
 * @return None.
 *
 * @remarks This routine will return with the Process Lock held.
 *
 *--*/
VOID
NTAPI
CsrLockedDereferenceThread(IN PCSR_THREAD CsrThread)
{
    LONG LockCount;

    /* Decrease reference count */
    LockCount = --CsrThread->ReferenceCount;
    ASSERT(LockCount >= 0);
    if (!LockCount)
    {
        /* Call the generic cleanup code */
        CsrThreadRefcountZero(CsrThread);
        CsrAcquireProcessLock();
    }
}

NTSTATUS
NTAPI
CsrCreateThread(IN PCSR_PROCESS CsrProcess,
                IN HANDLE hThread,
                IN PCLIENT_ID ClientId)
{
    PCSR_THREAD CsrThread;
    PCSR_PROCESS CurrentProcess;
    PCSR_THREAD CurrentThread = NtCurrentTeb()->CsrClientThread;
    CLIENT_ID CurrentCid;
    KERNEL_USER_TIMES KernelTimes;

    /* Get the current thread and CID */
    CurrentCid = CurrentThread->ClientId;

    /* Acquire the Process Lock */
    CsrAcquireProcessLock();

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

/*++
 * @name CsrDereferenceThread
 * @implemented NT4
 *
 * The CsrDereferenceThread routine removes a reference from a CSR Thread.
 *
 * @param CsrThread
 *        Pointer to the CSR Thread to dereference.
 *
 * @return None.
 *
 * @remarks If the reference count has reached zero (ie: the CSR Thread has
 *          no more active references), it will be deleted.
 *
 *--*/
VOID
NTAPI
CsrDereferenceThread(IN PCSR_THREAD CsrThread)
{
    /* Acquire process lock */
    CsrAcquireProcessLock();

    /* Decrease reference count */
    ASSERT(CsrThread->ReferenceCount > 0);
    if (!(--CsrThread->ReferenceCount))
    {
        /* Call the generic cleanup code */
        CsrThreadRefcountZero(CsrThread);
    }
    else
    {
        /* Just release the lock */
        CsrReleaseProcessLock();
    }
}

/* EOF */
