/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS CSR Sub System
 * FILE:            subsys/csr/csrsrv/procsup.c
 * PURPOSE:         CSR Process Management
 * PROGRAMMERS:     ReactOS Portable Systems Group
 *                  Alex Ionescu
 */
 
/* INCLUDES *******************************************************************/

#include <srv.h>

#define NDEBUG
#include <debug.h>

#define LOCK   RtlEnterCriticalSection(&ProcessDataLock)
#define UNLOCK RtlLeaveCriticalSection(&ProcessDataLock)
#define CsrHeap RtlGetProcessHeap()
#define CsrHashThread(t) \
    (HandleToUlong(t)&(256 - 1))

#define CsrAcquireProcessLock() LOCK
#define CsrReleaseProcessLock() UNLOCK

/* GLOBALS ********************************************************************/

LIST_ENTRY CsrThreadHashTable[256];
extern PCSRSS_PROCESS_DATA CsrRootProcess;
extern RTL_CRITICAL_SECTION ProcessDataLock;
extern PCSRSS_PROCESS_DATA ProcessData[256];

/* FUNCTIONS ******************************************************************/

PCSR_THREAD
NTAPI
CsrAllocateThread(IN PCSRSS_PROCESS_DATA CsrProcess)
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
CsrLocateThreadByClientId(OUT PCSRSS_PROCESS_DATA *Process OPTIONAL,
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
CsrLocateThreadInProcess(IN PCSRSS_PROCESS_DATA CsrProcess OPTIONAL,
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
CsrInsertThread(IN PCSRSS_PROCESS_DATA Process,
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
    /* Remove this thread */
    CsrRemoveThread(CsrThread);

    /* Release the Process Lock */
    //CsrReleaseProcessLock();

    /* Close the NT Thread Handle */
    if (CsrThread->ThreadHandle) NtClose(CsrThread->ThreadHandle);
    
    /* De-allocate the CSR Thread Object */
    CsrDeallocateThread(CsrThread);

    /* Remove a reference from the process */
    //CsrDereferenceProcess(CsrProcess);
}

NTSTATUS
NTAPI
CsrCreateThread(IN PCSRSS_PROCESS_DATA CsrProcess,
                IN HANDLE hThread,
                IN PCLIENT_ID ClientId)
{
    NTSTATUS Status;
    PCSR_THREAD CsrThread;
    //PCSRSS_PROCESS_DATA CurrentProcess;
    PCSR_THREAD CurrentThread = NtCurrentTeb()->CsrClientThread;
    CLIENT_ID CurrentCid;
    KERNEL_USER_TIMES KernelTimes;

//    DPRINT1("CSRSRV: %s called\n", __FUNCTION__);

    /* Get the current thread and CID */
    CurrentCid = CurrentThread->ClientId;
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
    Status = NtQueryInformationThread(hThread,
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
    if ((CsrThread = CsrAllocateThread(CsrRootProcess)))
    {
        /* Setup the Object */
//        DPRINT1("New CSR thread created: %lx PID/TID: %lx/%lx\n", CsrThread, ClientId->UniqueProcess, ClientId->UniqueThread);
        CsrThread->ThreadHandle = hThread;
        CsrThread->ClientId = *ClientId;
        CsrThread->Flags = ThreadFlags;

        /* Insert it into the Thread List */
        InsertTailList(&CsrRootProcess->ThreadList, &CsrThread->Link);

        /* Increment the thread count */
        CsrRootProcess->ThreadCount++;
    }

    /* Release the Process Lock and return */
    CsrReleaseProcessLock();
    return CsrThread;
}

/* EOF */
