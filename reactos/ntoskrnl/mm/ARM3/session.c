/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/session.c
 * PURPOSE:         Session support routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include "../ARM3/miarm.h"

/* GLOBALS ********************************************************************/

PMM_SESSION_SPACE MmSessionSpace;
PFN_NUMBER MiSessionDataPages, MiSessionTagPages, MiSessionTagSizePages;
PFN_NUMBER MiSessionBigPoolPages, MiSessionCreateCharge;
KGUARDED_MUTEX MiSessionIdMutex;
LONG MmSessionDataPages;
PRTL_BITMAP MiSessionIdBitmap;
volatile LONG MiSessionLeaderExists;


/* PRIVATE FUNCTIONS **********************************************************/

LCID
NTAPI
MmGetSessionLocaleId(VOID)
{
    PEPROCESS Process;
    PAGED_CODE();

    //
    // Get the current process
    //
    Process = PsGetCurrentProcess();

    //
    // Check if it's the Session Leader
    //
    if (Process->Vm.Flags.SessionLeader)
    {
        //
        // Make sure it has a valid Session
        //
        if (Process->Session)
        {
            //
            // Get the Locale ID
            //
            return ((PMM_SESSION_SPACE)Process->Session)->LocaleId;
        }
    }

    //
    // Not a session leader, return the default
    //
    return PsDefaultThreadLocaleId;
}

VOID
NTAPI
MiInitializeSessionIds(VOID)
{
    ULONG Size, BitmapSize;
    PFN_NUMBER TotalPages;

    /* Setup the total number of data pages needed for the structure */
    TotalPages = MI_SESSION_DATA_PAGES_MAXIMUM;
    MiSessionDataPages = ROUND_TO_PAGES(sizeof(MM_SESSION_SPACE)) >> PAGE_SHIFT;
    ASSERT(MiSessionDataPages <= MI_SESSION_DATA_PAGES_MAXIMUM - 3);
    TotalPages -= MiSessionDataPages;

    /* Setup the number of pages needed for session pool tags */
    MiSessionTagSizePages = 2;
    MiSessionBigPoolPages = 1;
    MiSessionTagPages = MiSessionTagSizePages + MiSessionBigPoolPages;
    ASSERT(MiSessionTagPages <= TotalPages);
    ASSERT(MiSessionTagPages < MI_SESSION_TAG_PAGES_MAXIMUM);

    /* Total pages needed for a session (FIXME: Probably different on PAE/x64) */
    MiSessionCreateCharge = 1 + MiSessionDataPages + MiSessionTagPages;

    /* Initialize the lock */
    KeInitializeGuardedMutex(&MiSessionIdMutex);

    /* Allocate the bitmap */
    Size = MI_INITIAL_SESSION_IDS;
    BitmapSize = ((Size + 31) / 32) * sizeof(ULONG);
    MiSessionIdBitmap = ExAllocatePoolWithTag(PagedPool,
                                              sizeof(RTL_BITMAP) + BitmapSize,
                                              '  mM');
    if (MiSessionIdBitmap)
    {
        /* Free all the bits */
        RtlInitializeBitMap(MiSessionIdBitmap,
                            (PVOID)(MiSessionIdBitmap + 1),
                            Size);
        RtlClearAllBits(MiSessionIdBitmap);
    }
    else
    {
        /* Die if we couldn't allocate the bitmap */
        KeBugCheckEx(INSTALL_MORE_MEMORY,
                     MmNumberOfPhysicalPages,
                     MmLowestPhysicalPage,
                     MmHighestPhysicalPage,
                     0x200);
    }
}

VOID
NTAPI
MiSessionLeader(IN PEPROCESS Process)
{
    KIRQL OldIrql;

    /* Set the flag while under the expansion lock */
    OldIrql = KeAcquireQueuedSpinLock(LockQueueExpansionLock);
    Process->Vm.Flags.SessionLeader = TRUE;
    KeReleaseQueuedSpinLock(LockQueueExpansionLock, OldIrql);
}

ULONG
NTAPI
MmGetSessionId(IN PEPROCESS Process)
{
    PMM_SESSION_SPACE SessionGlobal;

    /* The session leader is always session zero */
    if (Process->Vm.Flags.SessionLeader == 1) return 0;

    /* Otherwise, get the session global, and read the session ID from it */
    SessionGlobal = (PMM_SESSION_SPACE)Process->Session;
    if (!SessionGlobal) return 0;
    return SessionGlobal->SessionId;
}

ULONG
NTAPI
MmGetSessionIdEx(IN PEPROCESS Process)
{
    PMM_SESSION_SPACE SessionGlobal;

    /* The session leader is always session zero */
    if (Process->Vm.Flags.SessionLeader == 1) return 0;

    /* Otherwise, get the session global, and read the session ID from it */
    SessionGlobal = (PMM_SESSION_SPACE)Process->Session;
    if (!SessionGlobal) return -1;
    return SessionGlobal->SessionId;
}

VOID
NTAPI
MiReleaseProcessReferenceToSessionDataPage(IN PMM_SESSION_SPACE SessionGlobal)
{
    ULONG i, SessionId;
    PMMPTE PointerPte;
    PFN_NUMBER PageFrameIndex[MI_SESSION_DATA_PAGES_MAXIMUM];
    PMMPFN Pfn1;
    KIRQL OldIrql;

    /* Is there more than just this reference? If so, bail out */
    if (InterlockedDecrement(&SessionGlobal->ProcessReferenceToSession)) return;

    /* Get the session ID */
    SessionId = SessionGlobal->SessionId;
    DPRINT1("Last process in session %lu going down!!!\n", SessionId);

    /* Free the session page tables */
#ifndef _M_AMD64
    ExFreePoolWithTag(SessionGlobal->PageTables, 'tHmM');
#endif
    ASSERT(!MI_IS_PHYSICAL_ADDRESS(SessionGlobal));

    /* Capture the data page PFNs */
    PointerPte = MiAddressToPte(SessionGlobal);
    for (i = 0; i < MiSessionDataPages; i++)
    {
        PageFrameIndex[i] = PFN_FROM_PTE(PointerPte + i);
    }

    /* Release them */
    MiReleaseSystemPtes(PointerPte, MiSessionDataPages, SystemPteSpace);

    /* Mark them as deleted */
    for (i = 0; i < MiSessionDataPages; i++)
    {
        Pfn1 = MI_PFN_ELEMENT(PageFrameIndex[i]);
        MI_SET_PFN_DELETED(Pfn1);
    }

    /* Loop every data page and drop a reference count */
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
    for (i = 0; i < MiSessionDataPages; i++)
    {
        /* Sanity check that the page is correct, then decrement it */
        Pfn1 = MI_PFN_ELEMENT(PageFrameIndex[i]);
        ASSERT(Pfn1->u2.ShareCount == 1);
        ASSERT(Pfn1->u3.e2.ReferenceCount == 1);
        MiDecrementShareCount(Pfn1, PageFrameIndex[i]);
    }

    /* Done playing with pages, release the lock */
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);

    /* Decrement the number of data pages */
    InterlockedDecrement(&MmSessionDataPages);

    /* Free this session ID from the session bitmap */
    KeAcquireGuardedMutex(&MiSessionIdMutex);
    ASSERT(RtlCheckBit(MiSessionIdBitmap, SessionId));
    RtlClearBit(MiSessionIdBitmap, SessionId);
    KeReleaseGuardedMutex(&MiSessionIdMutex);
}

VOID
NTAPI
MiSessionRemoveProcess(VOID)
{
    PEPROCESS CurrentProcess = PsGetCurrentProcess();

    /* If the process isn't already in a session, or if it's the leader... */
    if (!(CurrentProcess->Flags & PSF_PROCESS_IN_SESSION_BIT) ||
        (CurrentProcess->Vm.Flags.SessionLeader))
    {
        /* Then there's nothing to do */
        return;
    }

    /* Sanity check */
    ASSERT(MmIsAddressValid(MmSessionSpace) == TRUE);

    /* Remove the process from the list ,and dereference the session */
    // DO NOT ENABLE THIS UNLESS YOU FIXED THE NP POOL CORRUPTION THAT IT CAUSES!!!
    //RemoveEntryList(&CurrentProcess->SessionProcessLinks);
    //MiDereferenceSession();
}

VOID
NTAPI
MiSessionAddProcess(IN PEPROCESS NewProcess)
{
    PMM_SESSION_SPACE SessionGlobal;

    /* The current process must already be in a session */
    if (!(PsGetCurrentProcess()->Flags & PSF_PROCESS_IN_SESSION_BIT)) return;

    /* Sanity check */
    ASSERT(MmIsAddressValid(MmSessionSpace) == TRUE);

    /* Get the global session */
    SessionGlobal = MmSessionSpace->GlobalVirtualAddress;

    /* Increment counters */
    InterlockedIncrement((PLONG)&SessionGlobal->ReferenceCount);
    InterlockedIncrement(&SessionGlobal->ResidentProcessCount);
    InterlockedIncrement(&SessionGlobal->ProcessReferenceToSession);

    /* Set the session pointer */
    ASSERT(NewProcess->Session == NULL);
    NewProcess->Session = SessionGlobal;

    /* Insert it into the process list */
    // DO NOT ENABLE THIS UNLESS YOU FIXED THE NP POOL CORRUPTION THAT IT CAUSES!!!
    //InsertTailList(&SessionGlobal->ProcessList, &NewProcess->SessionProcessLinks);

    /* Set the flag */
    PspSetProcessFlag(NewProcess, PSF_PROCESS_IN_SESSION_BIT);
}

NTSTATUS
NTAPI
MiSessionInitializeWorkingSetList(VOID)
{
    KIRQL OldIrql;
    PMMPTE PointerPte, PointerPde;
    MMPTE TempPte;
    ULONG Color, Index;
    PFN_NUMBER PageFrameIndex;
    PMM_SESSION_SPACE SessionGlobal;
    BOOLEAN AllocatedPageTable;
    PMMWSL WorkingSetList;

    /* Get pointers to session global and the session working set list */
    SessionGlobal = MmSessionSpace->GlobalVirtualAddress;
    WorkingSetList = (PMMWSL)MiSessionSpaceWs;

    /* Fill out the two pointers */
    MmSessionSpace->Vm.VmWorkingSetList = WorkingSetList;
    MmSessionSpace->Wsle = (PMMWSLE)WorkingSetList->UsedPageTableEntries;

    /* Get the PDE for the working set, and check if it's already allocated */
    PointerPde = MiAddressToPde(WorkingSetList);
    if (PointerPde->u.Hard.Valid == 1)
    {
        /* Nope, we'll have to do it */
        ASSERT(PointerPde->u.Hard.Global == 0);
        AllocatedPageTable = FALSE;
    }
    else
    {
        /* Yep, that makes our job easier */
        AllocatedPageTable = TRUE;
    }

    /* Get the PTE for the working set */
    PointerPte = MiAddressToPte(WorkingSetList);

    /* Initialize the working set lock, and lock the PFN database */
    ExInitializePushLock(&SessionGlobal->Vm.WorkingSetMutex);
    //MmLockPageableSectionByHandle(ExPageLockHandle);
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

    /* Check if we need a page table */
    if (AllocatedPageTable == TRUE)
    {
        /* Get a zeroed colored zero page */
        Color = MI_GET_NEXT_COLOR();
        PageFrameIndex = MiRemoveZeroPageSafe(Color);
        if (!PageFrameIndex)
        {
            /* No zero pages, grab a free one */
            PageFrameIndex = MiRemoveAnyPage(Color);

            /* Zero it outside the PFN lock */
            KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
            MiZeroPhysicalPage(PageFrameIndex);
            OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
        }

        /* Write a valid PDE for it */
        TempPte.u.Long = ValidKernelPdeLocal.u.Long;
        TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
        MI_WRITE_VALID_PTE(PointerPde, TempPte);

        /* Add this into the list */
        Index = ((ULONG_PTR)WorkingSetList - (ULONG_PTR)MmSessionBase) >> 22;
#ifndef _M_AMD64
        MmSessionSpace->PageTables[Index] = TempPte;
#endif
        /* Initialize the page directory page, and now zero the working set list itself */
        MiInitializePfnForOtherProcess(PageFrameIndex,
                                       PointerPde,
                                       MmSessionSpace->SessionPageDirectoryIndex);
        KeZeroPages(PointerPte, PAGE_SIZE);
    }

    /* Get a zeroed colored zero page */
    Color = MI_GET_NEXT_COLOR();
    PageFrameIndex = MiRemoveZeroPageSafe(Color);
    if (!PageFrameIndex)
    {
        /* No zero pages, grab a free one */
        PageFrameIndex = MiRemoveAnyPage(Color);

        /* Zero it outside the PFN lock */
        KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
        MiZeroPhysicalPage(PageFrameIndex);
        OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
    }

    /* Write a valid PTE for it */
    TempPte.u.Long = ValidKernelPteLocal.u.Long;
    TempPte.u.Hard.Dirty = TRUE;
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;

    /* Initialize the working set list page */
    MiInitializePfnAndMakePteValid(PageFrameIndex, PointerPte, TempPte);

    /* Now we can release the PFN database lock */
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);

    /* Fill out the working set structure */
    MmSessionSpace->Vm.Flags.SessionSpace = 1;
    MmSessionSpace->Vm.MinimumWorkingSetSize = 20;
    MmSessionSpace->Vm.MaximumWorkingSetSize = 384;
    WorkingSetList->LastEntry = 20;
    WorkingSetList->HashTable = NULL;
    WorkingSetList->HashTableSize = 0;
    WorkingSetList->Wsle = MmSessionSpace->Wsle;

    /* FIXME: Handle list insertions */
    ASSERT(SessionGlobal->WsListEntry.Flink == NULL);
    ASSERT(SessionGlobal->WsListEntry.Blink == NULL);
    ASSERT(SessionGlobal->Vm.WorkingSetExpansionLinks.Flink == NULL);
    ASSERT(SessionGlobal->Vm.WorkingSetExpansionLinks.Blink == NULL);

    /* All done, return */
    //MmUnlockPageableImageSection(ExPageLockHandle);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MiSessionCreateInternal(OUT PULONG SessionId)
{
    PEPROCESS Process = PsGetCurrentProcess();
    ULONG NewFlags, Flags, Size, i, Color;
    KIRQL OldIrql;
    PMMPTE PointerPte, PageTables, SessionPte;
    PMMPDE PointerPde;
    PMM_SESSION_SPACE SessionGlobal;
    MMPTE TempPte;
    NTSTATUS Status;
    BOOLEAN Result;
    PFN_NUMBER SessionPageDirIndex;
    PFN_NUMBER TagPage[MI_SESSION_TAG_PAGES_MAXIMUM];
    PFN_NUMBER DataPage[MI_SESSION_DATA_PAGES_MAXIMUM];

    /* This should not exist yet */
    ASSERT(MmIsAddressValid(MmSessionSpace) == FALSE);

    /* Loop so we can set the session-is-creating flag */
    Flags = Process->Flags;
    while (TRUE)
    {
        /* Check if it's already set */
        if (Flags & PSF_SESSION_CREATION_UNDERWAY_BIT)
        {
            /* Bail out */
            DPRINT1("Lost session race\n");
            return STATUS_ALREADY_COMMITTED;
        }

        /* Now try to set it */
        NewFlags = InterlockedCompareExchange((PLONG)&Process->Flags,
                                              Flags | PSF_SESSION_CREATION_UNDERWAY_BIT,
                                              Flags);
        if (NewFlags == Flags) break;

        /* It changed, try again */
        Flags = NewFlags;
    }

    /* Now we should own the flag */
    ASSERT(Process->Flags & PSF_SESSION_CREATION_UNDERWAY_BIT);

    /*
     * Session space covers everything from 0xA0000000 to 0xC0000000.
     * Allocate enough page tables to describe the entire region
     */
    Size = (0x20000000 / PDE_MAPPED_VA) * sizeof(MMPTE);
    PageTables = ExAllocatePoolWithTag(NonPagedPool, Size, 'tHmM');
    ASSERT(PageTables != NULL);
    RtlZeroMemory(PageTables, Size);

    /* Lock the session ID creation mutex */
    KeAcquireGuardedMutex(&MiSessionIdMutex);

    /* Allocate a new Session ID */
    *SessionId = RtlFindClearBitsAndSet(MiSessionIdBitmap, 1, 0);
    if (*SessionId == 0xFFFFFFFF)
    {
        /* We ran out of session IDs, we should expand */
        DPRINT1("Too many sessions created. Expansion not yet supported\n");
        ExFreePoolWithTag(PageTables, 'tHmM');
        return STATUS_NO_MEMORY;
    }

    /* Unlock the session ID creation mutex */
    KeReleaseGuardedMutex(&MiSessionIdMutex);

    /* Reserve the global PTEs */
    SessionPte = MiReserveSystemPtes(MiSessionDataPages, SystemPteSpace);
    ASSERT(SessionPte != NULL);

    /* Acquire the PFN lock while we set everything up */
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

    /* Loop the global PTEs */
    TempPte.u.Long = ValidKernelPte.u.Long;
    for (i = 0; i < MiSessionDataPages; i++)
    {
        /* Get a zeroed colored zero page */
        Color = MI_GET_NEXT_COLOR();
        DataPage[i] = MiRemoveZeroPageSafe(Color);
        if (!DataPage[i])
        {
            /* No zero pages, grab a free one */
            DataPage[i] = MiRemoveAnyPage(Color);

            /* Zero it outside the PFN lock */
            KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
            MiZeroPhysicalPage(DataPage[i]);
            OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
        }

        /* Fill the PTE out */
        TempPte.u.Hard.PageFrameNumber = DataPage[i];
        MI_WRITE_VALID_PTE(SessionPte + i, TempPte);
    }

    /* Set the pointer to global space */
    SessionGlobal = MiPteToAddress(SessionPte);

    /* Get a zeroed colored zero page */
    Color = MI_GET_NEXT_COLOR();
    SessionPageDirIndex = MiRemoveZeroPageSafe(Color);
    if (!SessionPageDirIndex)
    {
        /* No zero pages, grab a free one */
        SessionPageDirIndex = MiRemoveAnyPage(Color);

        /* Zero it outside the PFN lock */
        KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
        MiZeroPhysicalPage(SessionPageDirIndex);
        OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
    }

    /* Fill the PTE out */
    TempPte.u.Long = ValidKernelPdeLocal.u.Long;
    TempPte.u.Hard.PageFrameNumber = SessionPageDirIndex;

    /* Setup, allocate, fill out the MmSessionSpace PTE */
    PointerPde = MiAddressToPde(MmSessionSpace);
    ASSERT(PointerPde->u.Long == 0);
    MI_WRITE_VALID_PTE(PointerPde, TempPte);
    MiInitializePfnForOtherProcess(SessionPageDirIndex,
                                   PointerPde,
                                   SessionPageDirIndex);
    ASSERT(MI_PFN_ELEMENT(SessionPageDirIndex)->u1.WsIndex == 0);

     /* Loop all the local PTEs for it */
    TempPte.u.Long = ValidKernelPteLocal.u.Long;
    PointerPte = MiAddressToPte(MmSessionSpace);
    for (i = 0; i < MiSessionDataPages; i++)
    {
        /* And fill them out */
        TempPte.u.Hard.PageFrameNumber = DataPage[i];
        MiInitializePfnAndMakePteValid(DataPage[i], PointerPte + i, TempPte);
        ASSERT(MI_PFN_ELEMENT(DataPage[i])->u1.WsIndex == 0);
    }

     /* Finally loop all of the session pool tag pages */
    for (i = 0; i < MiSessionTagPages; i++)
    {
        /* Grab a zeroed colored page */
        Color = MI_GET_NEXT_COLOR();
        TagPage[i] = MiRemoveZeroPageSafe(Color);
        if (!TagPage[i])
        {
            /* No zero pages, grab a free one */
            TagPage[i] = MiRemoveAnyPage(Color);

            /* Zero it outside the PFN lock */
            KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
            MiZeroPhysicalPage(TagPage[i]);
            OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
        }

        /* Fill the PTE out */
        TempPte.u.Hard.PageFrameNumber = TagPage[i];
        MiInitializePfnAndMakePteValid(TagPage[i],
                                       PointerPte + MiSessionDataPages + i,
                                       TempPte);
    }

    /* PTEs have been setup, release the PFN lock */
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);

    /* Fill out the session space structure now */
    MmSessionSpace->GlobalVirtualAddress = SessionGlobal;
    MmSessionSpace->ReferenceCount = 1;
    MmSessionSpace->ResidentProcessCount = 1;
    MmSessionSpace->u.LongFlags = 0;
    MmSessionSpace->SessionId = *SessionId;
    MmSessionSpace->LocaleId = PsDefaultSystemLocaleId;
    MmSessionSpace->SessionPageDirectoryIndex = SessionPageDirIndex;
    MmSessionSpace->Color = Color;
    MmSessionSpace->NonPageablePages = MiSessionCreateCharge;
    MmSessionSpace->CommittedPages = MiSessionCreateCharge;
#ifndef _M_AMD64
    MmSessionSpace->PageTables = PageTables;
    MmSessionSpace->PageTables[PointerPde - MiAddressToPde(MmSessionBase)] = *PointerPde;
#endif
    InitializeListHead(&MmSessionSpace->ImageList);
    DPRINT1("Session %lu is ready to go: 0x%p 0x%p, %lx 0x%p\n",
            *SessionId, MmSessionSpace, SessionGlobal, SessionPageDirIndex, PageTables);

    /* Initialize session pool */
    //Status = MiInitializeSessionPool();
    Status = STATUS_SUCCESS;
    ASSERT(NT_SUCCESS(Status) == TRUE);

    /* Initialize system space */
    Result = MiInitializeSystemSpaceMap(&SessionGlobal->Session);
    ASSERT(Result == TRUE);

    /* Initialize the process list, make sure the workign set list is empty */
    ASSERT(SessionGlobal->WsListEntry.Flink == NULL);
    ASSERT(SessionGlobal->WsListEntry.Blink == NULL);
    InitializeListHead(&SessionGlobal->ProcessList);

    /* We're done, clear the flag */
    ASSERT(Process->Flags & PSF_SESSION_CREATION_UNDERWAY_BIT);
    PspClearProcessFlag(Process, PSF_SESSION_CREATION_UNDERWAY_BIT);

    /* Insert the process into the session  */
    ASSERT(Process->Session == NULL);
    ASSERT(SessionGlobal->ProcessReferenceToSession == 0);
    SessionGlobal->ProcessReferenceToSession = 1;

    /* We're done */
    InterlockedIncrement(&MmSessionDataPages);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MmSessionCreate(OUT PULONG SessionId)
{
    PEPROCESS Process = PsGetCurrentProcess();
    ULONG SessionLeaderExists;
    NTSTATUS Status;

    /* Fail if the process is already in a session */
    if (Process->Flags & PSF_PROCESS_IN_SESSION_BIT)
    {
        DPRINT1("Process already in session\n");
        return STATUS_ALREADY_COMMITTED;
    }

    /* Check if the process is already the session leader */
    if (!Process->Vm.Flags.SessionLeader)
    {
        /* Atomically set it as the leader */
        SessionLeaderExists = InterlockedCompareExchange(&MiSessionLeaderExists, 1, 0);
        if (SessionLeaderExists)
        {
            DPRINT1("Session leader race\n");
            return STATUS_INVALID_SYSTEM_SERVICE;
        }

        /* Do the work required to upgrade him */
        MiSessionLeader(Process);
    }

    /* Create the session */
    KeEnterCriticalRegion();
    Status = MiSessionCreateInternal(SessionId);
    if (!NT_SUCCESS(Status))
    {
        KeLeaveCriticalRegion();
        return Status;
    }

    /* Set up the session working set */
    Status = MiSessionInitializeWorkingSetList();
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        //MiDereferenceSession();
        ASSERT(FALSE);
        KeLeaveCriticalRegion();
        return Status;
    }

    /* All done */
    KeLeaveCriticalRegion();

    /* Set and assert the flags, and return */
    MmSessionSpace->u.Flags.Initialized = 1;
    PspSetProcessFlag(Process, PSF_PROCESS_IN_SESSION_BIT);
    ASSERT(MiSessionLeaderExists == 1);
    return Status;
}

NTSTATUS
NTAPI
MmSessionDelete(IN ULONG SessionId)
{
    PEPROCESS Process = PsGetCurrentProcess();

    /* Process must be in a session */
    if (!(Process->Flags & PSF_PROCESS_IN_SESSION_BIT))
    {
        DPRINT1("Not in a session!\n");
        return STATUS_UNABLE_TO_FREE_VM;
    }

    /* It must be the session leader */
    if (!Process->Vm.Flags.SessionLeader)
    {
        DPRINT1("Not a session leader!\n");
        return STATUS_UNABLE_TO_FREE_VM;
    }

    /* Remove one reference count */
    KeEnterCriticalRegion();
    /* FIXME: Do it */
    KeLeaveCriticalRegion();

    /* All done */
    return STATUS_SUCCESS;
}
