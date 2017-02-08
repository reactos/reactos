////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*

    Module Name:

        DLDetect.cpp

    Abstract:

        This file contains all source code related to DeadLock Detector.

    Environment:

        NT Kernel Mode

*/

#include "udffs.h"

/// define the file specific bug-check id
#define         UDF_BUG_CHECK_ID                UDF_FILE_DLD


/// Resource event (ExclusiveWaiters) 
#define RESOURCE_EVENT_TAG      'vEeR' 
/// Resource semaphore (SharedWaiters)
#define RESOURCE_SEMAFORE_TAG   'eSeR' 
/// Resource owner table (OwnerTable)
#define RESOURCE_TABLE_TAG      'aTeR' 

/// Maxmum recurse level  while exploring thread-resource aquisition graf
#define DLD_MAX_REC_LEVEL       40

/// Maximum supported number of threads (initialized by DLDInit())
ULONG MaxThreadCount = 0;

/// Waiters table
PTHREAD_STRUCT      DLDThreadTable;            
/// 4 sec
LARGE_INTEGER DLDpTimeout;                  
/// 8 sec
ULONG DLDpResourceTimeoutCount = 0x2;       

THREAD_REC_BLOCK DLDThreadAcquireChain[DLD_MAX_REC_LEVEL];

/// Initialize deadlock detector
VOID DLDInit(ULONG MaxThrdCount /// Maximum supported number of threads
) {
    if (KeNumberProcessors>1) {
        KdPrint(("Deadlock Detector is designed for uniprocessor machines only!\n"));
        BrutePoint();
    }
    DLDpTimeout.QuadPart = -40000000I64;

    MaxThreadCount = MaxThrdCount;
    DLDThreadTable = (PTHREAD_STRUCT) DLDAllocatePool(MaxThreadCount*sizeof(THREAD_STRUCT));
    RtlZeroMemory(DLDThreadTable, sizeof(THREAD_STRUCT)*MaxThreadCount);
}

VOID DLDFree(VOID) {

    DLDFreePool(DLDThreadTable);

}

PTHREAD_STRUCT DLDAllocFindThread(ULONG ThreadId) {
    ULONG i = 0;
    PTHREAD_STRUCT Temp = DLDThreadTable;
    ULONG FirstEmpty = -1;

    while (i<MaxThreadCount) {
        if (Temp->ThreadId == ThreadId) {
            return Temp;
        } else if (FirstEmpty == -1 && !Temp->ThreadId) {
            FirstEmpty = i;
        }
        Temp++;
        i++;
    }
    // Not found. Allocate new one.
    if (i == MaxThreadCount) {
        if (FirstEmpty == -1) {
            KdPrint(("Not enough table entries. Try to increase MaxThrdCount on next build"));
            BrutePoint();
        }
        i = FirstEmpty;
    }
    Temp = DLDThreadTable + i;

    RtlZeroMemory(Temp, sizeof(THREAD_STRUCT));
    Temp->ThreadId = ThreadId;

    return Temp;
}

PTHREAD_STRUCT DLDFindThread(ULONG ThreadId) {
    ULONG i = 0;
    PTHREAD_STRUCT Temp = DLDThreadTable;
    ULONG FirstEmpty = -1;

    
    while (i<MaxThreadCount) {
        if (Temp->ThreadId == ThreadId) {
            return Temp;
        } 
        Temp++;
        i++;
    }

    return NULL;
}

BOOLEAN DLDProcessResource(PERESOURCE Resource,       
                        PTHREAD_STRUCT ThrdStruct,
                        ULONG RecLevel);


/// TRUE Indicates deadlock
BOOLEAN DLDProcessThread(PTHREAD_STRUCT ThrdOwner,
                      PTHREAD_STRUCT ThrdStruct,
                      PERESOURCE Resource,
                      ULONG RecLevel) {

    if (ThrdOwner == ThrdStruct) {
        // ERESOURCE wait cycle. Deadlock detected.
        KdPrint(("DLD: *********DEADLOCK DETECTED*********\n"));
        KdPrint(("Thread %x holding resource %x\n",ThrdOwner->ThreadId,Resource));
        return TRUE;
    }

    for (int i=RecLevel+1;i<DLD_MAX_REC_LEVEL;i++) {
        if (DLDThreadAcquireChain[i].Thread->ThreadId == ThrdOwner->ThreadId) {
            // ERESOURCE wait cycle. Deadlock detected.
            KdPrint(("DLD: *********DEADLOCK DETECTED*********\n"));
            KdPrint(("Thread %x holding resource %x\n",ThrdOwner->ThreadId,Resource));
            for (int j=RecLevel+1;j<=i;j++) {
                KdPrint((" awaited by thread %x at (BugCheckId:%x:Line:%d) holding resource %x\n",
                DLDThreadAcquireChain[i].Thread->ThreadId,
                DLDThreadAcquireChain[i].Thread->BugCheckId, 
                DLDThreadAcquireChain[i].Thread->Line,
                Resource));
            }
            BrutePoint();
            return FALSE;
        }
    }
    DLDThreadAcquireChain[RecLevel].Thread          = ThrdOwner;
    DLDThreadAcquireChain[RecLevel].HoldingResource = Resource;

    // Find resource, awaited by thread
    if (ThrdOwner->WaitingResource) {
        if (DLDProcessResource(ThrdOwner->WaitingResource, ThrdStruct,RecLevel)) {
            KdPrint((" awaited by thread %x at (BugCheckId:%x:Line:%d) holding resource %x\n",
            ThrdOwner->ThreadId, 
            ThrdOwner->BugCheckId, 
            ThrdOwner->Line, 
            Resource));
            return TRUE;
        }
    }

    return FALSE;
}


/// TRUE Indicates deadlock
BOOLEAN DLDProcessResource( PERESOURCE Resource,        // resource to process
                            PTHREAD_STRUCT ThrdStruct,  // thread structure of caller's thread
                            ULONG RecLevel)             // current recurse level
{
    if (RecLevel <= 0) {
        BrutePoint();
        return FALSE;
    }


    // If resource is free, just return. Not possible, but we must check.
    if (!Resource->ActiveCount) {
        return FALSE;
    }

    PTHREAD_STRUCT ThreadOwner;


    if (Resource->Flag & ResourceOwnedExclusive || (Resource->OwnerThreads[1].OwnerCount == 1)) {

        // If only one owner

        // Find thread owning this resource 
        if (Resource->Flag & ResourceOwnedExclusive) {
            ThreadOwner = DLDFindThread(Resource->OwnerThreads[0].OwnerThread);
        } else {
            ThreadOwner = DLDFindThread(Resource->OwnerThreads[1].OwnerThread);
        }

        BOOLEAN Result = FALSE;
        if (ThreadOwner) { 
            Result = DLDProcessThread(ThreadOwner, ThrdStruct, Resource,RecLevel-1);
        }
        return Result;
    } else {
        // Many owners
        int i;
        for (i=0; i<Resource->OwnerThreads[0].TableSize; i++) {
            if (Resource->OwnerTable[i].OwnerThread) {

                ThreadOwner = DLDFindThread(Resource->OwnerTable[i].OwnerThread);
                if (ThreadOwner && DLDProcessThread(ThreadOwner, ThrdStruct, Resource,RecLevel-1)) {

                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}



VOID DLDpWaitForResource(
    IN PERESOURCE Resource, 
    IN DISPATCHER_HEADER *DispatcherObject,
    IN PTHREAD_STRUCT ThrdStruct
    ) {
    KIRQL oldIrql;
    ULONG ResourceWaitCount = 0;

    Resource->ContentionCount++;

    while (KeWaitForSingleObject(DispatcherObject,Executive,KernelMode,FALSE,&DLDpTimeout) == STATUS_TIMEOUT) {
        KeAcquireSpinLock(&Resource->SpinLock, &oldIrql);
        if (++ResourceWaitCount>DLDpResourceTimeoutCount) {
            // May be deadlock?
            ResourceWaitCount = 0;

            if (DLDProcessResource(Resource, ThrdStruct,DLD_MAX_REC_LEVEL)) {
                KdPrint((" which thread %x has tried to acquire at (BugCheckId:%x:Line:%d)\n",
                ThrdStruct->ThreadId,
                ThrdStruct->BugCheckId,
                ThrdStruct->Line      
                ));
                BrutePoint();
            }
        } 
        // Priority boosts
        // .....
        // End of priority boosts
        KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
    }

}        




VOID DLDpAcquireResourceExclusiveLite(
    IN PERESOURCE Resource, 
    IN ERESOURCE_THREAD Thread,
    IN KIRQL oldIrql,
    IN ULONG BugCheckId,
    IN ULONG Line
    ) {
    KIRQL oldIrql2;

    if (!(Resource->ExclusiveWaiters)) {
        
        KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
        KeAcquireSpinLock(&Resource->SpinLock, &oldIrql2);

        // If ExclusiveWaiters Event not yet allocated allocate new one
        if (!(Resource->ExclusiveWaiters)) {
            Resource->ExclusiveWaiters = (PKEVENT)ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT),RESOURCE_EVENT_TAG);
            KeInitializeEvent(Resource->ExclusiveWaiters,SynchronizationEvent,FALSE);
        } 
        KeReleaseSpinLock(&Resource->SpinLock, oldIrql2);
        DLDAcquireExclusive(Resource,BugCheckId,Line);

    } else {
        Resource->NumberOfExclusiveWaiters++;

        PTHREAD_STRUCT ThrdStruct = DLDAllocFindThread(Thread);

        
        // Set WaitingResource for current thread
        ThrdStruct->WaitingResource = Resource;
        ThrdStruct->BugCheckId = BugCheckId;
        ThrdStruct->Line = Line;

        KeReleaseSpinLock(&Resource->SpinLock, oldIrql);

        DLDpWaitForResource(Resource,&(Resource->ExclusiveWaiters->Header),ThrdStruct);

        KeAcquireSpinLock(&Resource->SpinLock, &oldIrql);

        ThrdStruct->WaitingResource = NULL;
        ThrdStruct->ThreadId        = 0;
        ThrdStruct->BugCheckId      = 0;
        ThrdStruct->Line            = 0;
        Resource->OwnerThreads[0].OwnerThread = Thread;

        KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
    }
}




VOID DLDAcquireExclusive(PERESOURCE Resource,       
                         ULONG BugCheckId,
                         ULONG Line
) {

    KIRQL oldIrql;

    KeAcquireSpinLock(&Resource->SpinLock, &oldIrql);
            
    ERESOURCE_THREAD Thread = (ERESOURCE_THREAD)PsGetCurrentThread();

    if (!Resource->ActiveCount) goto SimpleAcquire;
    if ((Resource->Flag  & ResourceOwnedExclusive) && Resource->OwnerThreads[0].OwnerThread == Thread) {
        Resource->OwnerThreads[0].OwnerCount++;
        KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
        return;
    }

    DLDpAcquireResourceExclusiveLite(Resource, Thread, oldIrql,BugCheckId,Line); 
    return;

SimpleAcquire:

    Resource->Flag |= ResourceOwnedExclusive;
    Resource->ActiveCount = 1;
    Resource->OwnerThreads[0].OwnerThread = Thread;
    Resource->OwnerThreads[0].OwnerCount = 1;

    KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
}


POWNER_ENTRY DLDpFindCurrentThread(
    IN PERESOURCE Resource,
    IN ERESOURCE_THREAD Thread
    ) {
        
    if (Resource->OwnerThreads[0].OwnerThread == Thread) return &(Resource->OwnerThreads[0]);
    if (Resource->OwnerThreads[1].OwnerThread == Thread) return &(Resource->OwnerThreads[1]);

    POWNER_ENTRY LastEntry, CurrentEntry, FirstEmptyEntry = NULL;
    if (!(Resource->OwnerThreads[1].OwnerThread)) FirstEmptyEntry = &(Resource->OwnerThreads[1]);

    CurrentEntry = Resource->OwnerTable;
    LastEntry    = &(Resource->OwnerTable[Resource->OwnerThreads[0].TableSize]);

    while (CurrentEntry != LastEntry) {
        if (CurrentEntry->OwnerThread == Thread) {
            PCHAR CurrentThread = (PCHAR)PsGetCurrentThread();
            *((PULONG)(CurrentThread + 0x136)) = CurrentEntry - Resource->OwnerTable;
            return CurrentEntry;
        }
        if (!(CurrentEntry->OwnerThread)) {
            FirstEmptyEntry = CurrentEntry;
        }
        CurrentEntry++;
    }
    if (FirstEmptyEntry) {
        PCHAR CurrentThread = (PCHAR)PsGetCurrentThread();
        *((PULONG)(CurrentThread + 0x136)) = FirstEmptyEntry - Resource->OwnerTable;
        return FirstEmptyEntry;
    } else {
        // Grow OwnerTable

        
        USHORT OldSize = Resource->OwnerThreads[0].TableSize;
        USHORT NewSize = 3;
        if (OldSize) NewSize = OldSize + 4;
        POWNER_ENTRY NewEntry = (POWNER_ENTRY)ExAllocatePoolWithTag(NonPagedPool, sizeof(OWNER_ENTRY)*NewSize,RESOURCE_TABLE_TAG);
        RtlZeroMemory(NewEntry,sizeof(OWNER_ENTRY)*NewSize);
        if (Resource->OwnerTable) {
            RtlMoveMemory(NewEntry,Resource->OwnerTable,sizeof(OWNER_ENTRY)*OldSize);
            ExFreePool(Resource->OwnerTable);
        }
        Resource->OwnerTable = NewEntry;

        PCHAR CurrentThread = (PCHAR)PsGetCurrentThread();
        *((PULONG)(CurrentThread + 0x136)) = OldSize;
        Resource->OwnerThreads[0].TableSize = NewSize;

        return &(NewEntry[OldSize]);
    }
}


VOID DLDAcquireShared(PERESOURCE Resource,       
                      ULONG BugCheckId,
                      ULONG Line,
                      BOOLEAN WaitForExclusive)
{

    KIRQL oldIrql;

    ERESOURCE_THREAD Thread = (ERESOURCE_THREAD)PsGetCurrentThread();
    POWNER_ENTRY pOwnerEntry;

    KeAcquireSpinLock(&Resource->SpinLock, &oldIrql);

    if (!Resource->ActiveCount) {
        Resource->Flag &= ~ResourceOwnedExclusive;
        Resource->ActiveCount = 1;
        Resource->OwnerThreads[1].OwnerThread = Thread;
        Resource->OwnerThreads[1].OwnerCount = 1;
        KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
        return;
    }    

    if (Resource->Flag & ResourceOwnedExclusive ) {
        if (Resource->OwnerThreads[0].OwnerThread == Thread) {
            Resource->OwnerThreads[0].OwnerCount++;
            KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
            return;
        }

        pOwnerEntry = DLDpFindCurrentThread(Resource, 0);

    } else {
        // owned shared by some thread(s)

        pOwnerEntry = DLDpFindCurrentThread(Resource, Thread);

        if (!WaitForExclusive && pOwnerEntry->OwnerThread == Thread) {
            pOwnerEntry->OwnerCount++;
            KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
            return;
        }

        if (!(Resource->NumberOfExclusiveWaiters)) {

            pOwnerEntry->OwnerThread = Thread;
            pOwnerEntry->OwnerCount  = 1;
            Resource->ActiveCount++;        
            KeReleaseSpinLock(&Resource->SpinLock, oldIrql);

            return;
        }
    }

    if (!(Resource->SharedWaiters)) {
        Resource->SharedWaiters = (PKSEMAPHORE)ExAllocatePoolWithTag(NonPagedPool, sizeof(KSEMAPHORE),RESOURCE_SEMAFORE_TAG);
        KeInitializeSemaphore(Resource->SharedWaiters,0,0x7fffffff);
    }

    Resource->NumberOfSharedWaiters++;

    PTHREAD_STRUCT ThrdStruct = DLDAllocFindThread(Thread);


    // Set WaitingResource for current thread
    ThrdStruct->WaitingResource = Resource;
    ThrdStruct->BugCheckId = BugCheckId;
    ThrdStruct->Line = Line;
    
    KeReleaseSpinLock(&Resource->SpinLock, oldIrql);

    DLDpWaitForResource(Resource,&(Resource->SharedWaiters->Header),ThrdStruct);

    KeAcquireSpinLock(&Resource->SpinLock, &oldIrql);

    pOwnerEntry = DLDpFindCurrentThread(Resource, Thread);
    pOwnerEntry->OwnerThread    = Thread;
    pOwnerEntry->OwnerCount     = 1;

    ThrdStruct->WaitingResource = NULL;
    ThrdStruct->ThreadId        = 0;
    ThrdStruct->BugCheckId      = 0;
    ThrdStruct->Line            = 0;

    KeReleaseSpinLock(&Resource->SpinLock, oldIrql);

    return;

}
