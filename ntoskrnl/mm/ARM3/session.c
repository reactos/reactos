/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/session.c
 * PURPOSE:         Session support routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 *                  Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include <mm/ARM3/miarm.h>

/* GLOBALS ********************************************************************/

PMM_SESSION_SPACE MmSessionSpace;
PFN_NUMBER MiSessionDataPages, MiSessionTagPages, MiSessionTagSizePages;
PFN_NUMBER MiSessionBigPoolPages, MiSessionCreateCharge;
KGUARDED_MUTEX MiSessionIdMutex;
LONG MmSessionDataPages;
PRTL_BITMAP MiSessionIdBitmap;
volatile LONG MiSessionLeaderExists;

LIST_ENTRY MiSessionWsList;
LIST_ENTRY MmWorkingSetExpansionHead;

KSPIN_LOCK MmExpansionLock;
PETHREAD MiExpansionLockOwner;


/* PRIVATE FUNCTIONS **********************************************************/

VOID
NTAPI
MiInitializeSessionWsSupport(VOID)
{
    /* Initialize the list heads */
    InitializeListHead(&MiSessionWsList);
    InitializeListHead(&MmWorkingSetExpansionHead);
}

BOOLEAN
NTAPI
MmIsSessionAddress(IN PVOID Address)
{
    /* Check if it is in range */
    return MI_IS_SESSION_ADDRESS(Address) ? TRUE : FALSE;
}

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
    // Check if it's NOT the Session Leader
    //
    if (!Process->Vm.Flags.SessionLeader)
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

_IRQL_requires_max_(APC_LEVEL)
VOID
NTAPI
MmSetSessionLocaleId(
    _In_ LCID LocaleId)
{
    PEPROCESS CurrentProcess;
    PAGED_CODE();

    /* Get the current process and check if it is in a session */
    CurrentProcess = PsGetCurrentProcess();
    if ((CurrentProcess->Vm.Flags.SessionLeader == 0) &&
        (CurrentProcess->Session != NULL))
    {
        /* Set the session locale Id */
        ((PMM_SESSION_SPACE)CurrentProcess->Session)->LocaleId = LocaleId;
    }
    else
    {
        /* Set the default locale */
        PsDefaultThreadLocaleId = LocaleId;
    }
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
                                              TAG_MM);
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
    OldIrql = MiAcquireExpansionLock();
    Process->Vm.Flags.SessionLeader = TRUE;
    MiReleaseExpansionLock(OldIrql);
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
MiDereferenceSessionFinal(VOID)
{
    PMM_SESSION_SPACE SessionGlobal;
    KIRQL OldIrql;

    /* Get the pointer to the global session address */
    SessionGlobal = MmSessionSpace->GlobalVirtualAddress;

    /* Acquire the expansion lock */
    OldIrql = MiAcquireExpansionLock();

    /* Set delete pending flag, so that processes can no longer attach to this
       session and the last process that detaches sets the AttachEvent */
    ASSERT(SessionGlobal->u.Flags.DeletePending == 0);
    SessionGlobal->u.Flags.DeletePending = 1;

    /* Check if we have any attached processes */
    if (SessionGlobal->AttachCount)
    {
        /* Initialize the event (it's not in use yet!) */
        KeInitializeEvent(&SessionGlobal->AttachEvent, NotificationEvent, FALSE);

        /* Release the expansion lock for the wait */
        MiReleaseExpansionLock(OldIrql);

        /* Wait for the event to be set due to the last process detach */
        KeWaitForSingleObject(&SessionGlobal->AttachEvent, WrVirtualMemory, 0, 0, 0);

        /* Reacquire the expansion lock */
        OldIrql = MiAcquireExpansionLock();

        /* Makes sure we still have the delete flag and no attached processes */
        ASSERT(MmSessionSpace->u.Flags.DeletePending == 1);
        ASSERT(MmSessionSpace->AttachCount == 0);
    }

    /* Check if the session is in the workingset expansion list */
    if (SessionGlobal->Vm.WorkingSetExpansionLinks.Flink != NULL)
    {
        /* Remove the session from the list and zero the list entry */
        RemoveEntryList(&SessionGlobal->Vm.WorkingSetExpansionLinks);
        SessionGlobal->Vm.WorkingSetExpansionLinks.Flink = 0;
    }

    /* Check if the session is in the workingset list */
    if (SessionGlobal->WsListEntry.Flink)
    {
        /* Remove the session from the list and zero the list entry */
        RemoveEntryList(&SessionGlobal->WsListEntry);
        SessionGlobal->WsListEntry.Flink = NULL;
    }

    /* Release the expansion lock */
    MiReleaseExpansionLock(OldIrql);

    /* Check for a win32k unload routine */
    if (SessionGlobal->Win32KDriverUnload)
    {
        /* Call it */
        SessionGlobal->Win32KDriverUnload(NULL);
    }
}


VOID
NTAPI
MiDereferenceSession(VOID)
{
    PMM_SESSION_SPACE SessionGlobal;
    PEPROCESS Process;
    ULONG ReferenceCount, SessionId;

    /* Sanity checks */
    ASSERT(PsGetCurrentProcess()->ProcessInSession ||
           ((MmSessionSpace->u.Flags.Initialized == 0) &&
            (PsGetCurrentProcess()->Vm.Flags.SessionLeader == 1) &&
            (MmSessionSpace->ReferenceCount == 1)));

    /* The session bit must be set */
    SessionId = MmSessionSpace->SessionId;
    ASSERT(RtlCheckBit(MiSessionIdBitmap, SessionId));

    /* Get the current process */
    Process = PsGetCurrentProcess();

    /* Decrement the process count */
    InterlockedDecrement(&MmSessionSpace->ResidentProcessCount);

    /* Decrement the reference count and check if was the last reference */
    ReferenceCount = InterlockedDecrement(&MmSessionSpace->ReferenceCount);
    if (ReferenceCount == 0)
    {
        /* No more references left, kill the session completely */
        MiDereferenceSessionFinal();
    }

    /* Check if tis is the session leader or the last process in the session */
    if ((Process->Vm.Flags.SessionLeader) || (ReferenceCount == 0))
    {
        /* Get the global session address before we kill the session mapping */
        SessionGlobal = MmSessionSpace->GlobalVirtualAddress;

        /* Delete all session PDEs and flush the TB */
        RtlZeroMemory(MiAddressToPde(MmSessionBase),
                      BYTES_TO_PAGES(MmSessionSize) * sizeof(MMPDE));
        KeFlushEntireTb(FALSE, FALSE);

        /* Is this the session leader? */
        if (Process->Vm.Flags.SessionLeader)
        {
            /* Clean up the references here. */
            ASSERT(Process->Session == NULL);
            MiReleaseProcessReferenceToSessionDataPage(SessionGlobal);
        }
    }

    /* Reset the current process' session flag */
    RtlInterlockedClearBits(&Process->Flags, PSF_PROCESS_IN_SESSION_BIT);
}

VOID
NTAPI
MiSessionRemoveProcess(VOID)
{
    PEPROCESS CurrentProcess = PsGetCurrentProcess();
    KIRQL OldIrql;

    /* If the process isn't already in a session, or if it's the leader... */
    if (!(CurrentProcess->Flags & PSF_PROCESS_IN_SESSION_BIT) ||
        (CurrentProcess->Vm.Flags.SessionLeader))
    {
        /* Then there's nothing to do */
        return;
    }

    /* Sanity check */
    ASSERT(MmIsAddressValid(MmSessionSpace) == TRUE);

    /* Acquire the expansion lock while touching the session */
    OldIrql = MiAcquireExpansionLock();

    /* Remove the process from the list */
    RemoveEntryList(&CurrentProcess->SessionProcessLinks);

    /* Release the lock again */
    MiReleaseExpansionLock(OldIrql);

    /* Dereference the session */
    MiDereferenceSession();
}

VOID
NTAPI
MiSessionAddProcess(IN PEPROCESS NewProcess)
{
    PMM_SESSION_SPACE SessionGlobal;
    KIRQL OldIrql;

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

    /* Acquire the expansion lock while touching the session */
    OldIrql = MiAcquireExpansionLock();

    /* Insert it into the process list */
    InsertTailList(&SessionGlobal->ProcessList, &NewProcess->SessionProcessLinks);

    /* Release the lock again */
    MiReleaseExpansionLock(OldIrql);

    /* Set the flag */
    PspSetProcessFlag(NewProcess, PSF_PROCESS_IN_SESSION_BIT);
}

NTSTATUS
NTAPI
MiSessionInitializeWorkingSetList(VOID)
{
    KIRQL OldIrql;
    PMMPTE PointerPte;
    PMMPDE PointerPde;
    MMPTE TempPte;
    MMPDE TempPde;
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
#ifndef _M_ARM
        ASSERT(PointerPde->u.Hard.Global == 0);
#endif
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
    if (AllocatedPageTable != FALSE)
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
        TempPde = ValidKernelPdeLocal;
        TempPde.u.Hard.PageFrameNumber = PageFrameIndex;
        MI_WRITE_VALID_PDE(PointerPde, TempPde);

        /* Add this into the list */
        Index = ((ULONG_PTR)WorkingSetList - (ULONG_PTR)MmSessionBase) >> 22;
#ifndef _M_AMD64
        MmSessionSpace->PageTables[Index] = TempPde;
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
    TempPte = ValidKernelPteLocal;
    MI_MAKE_DIRTY_PAGE(&TempPte);
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

    /* Acquire the expansion lock while touching the session */
    OldIrql = MiAcquireExpansionLock();

    /* Handle list insertions */
    ASSERT(SessionGlobal->WsListEntry.Flink == NULL);
    ASSERT(SessionGlobal->WsListEntry.Blink == NULL);
    InsertTailList(&MiSessionWsList, &SessionGlobal->WsListEntry);

    ASSERT(SessionGlobal->Vm.WorkingSetExpansionLinks.Flink == NULL);
    ASSERT(SessionGlobal->Vm.WorkingSetExpansionLinks.Blink == NULL);
    InsertTailList(&MmWorkingSetExpansionHead,
                   &SessionGlobal->Vm.WorkingSetExpansionLinks);

    /* Release the lock again */
    MiReleaseExpansionLock(OldIrql);

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
    PMMPTE PointerPte, SessionPte;
    PMMPDE PointerPde, PageTables;
    PMM_SESSION_SPACE SessionGlobal;
    MMPTE TempPte;
    MMPDE TempPde;
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
    TempPte = ValidKernelPte;
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
    TempPde = ValidKernelPdeLocal;
    TempPde.u.Hard.PageFrameNumber = SessionPageDirIndex;

    /* Setup, allocate, fill out the MmSessionSpace PTE */
    PointerPde = MiAddressToPde(MmSessionSpace);
    ASSERT(PointerPde->u.Long == 0);
    MI_WRITE_VALID_PDE(PointerPde, TempPde);
    MiInitializePfnForOtherProcess(SessionPageDirIndex,
                                   PointerPde,
                                   SessionPageDirIndex);
    ASSERT(MI_PFN_ELEMENT(SessionPageDirIndex)->u1.WsIndex == 0);

     /* Loop all the local PTEs for it */
    TempPte = ValidKernelPteLocal;
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

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NTAPI
MmAttachSession(
    _Inout_ PVOID SessionEntry,
    _Out_ PKAPC_STATE ApcState)
{
    PEPROCESS EntryProcess;
    PMM_SESSION_SPACE EntrySession, CurrentSession;
    PEPROCESS CurrentProcess;
    KIRQL OldIrql;

    /* The parameter is the actual process! */
    EntryProcess = SessionEntry;
    ASSERT(EntryProcess != NULL);

    /* Sanity checks */
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    ASSERT(EntryProcess->Vm.Flags.SessionLeader == 0);

    /* Get the session from the process that was passed in */
    EntrySession = EntryProcess->Session;
    ASSERT(EntrySession != NULL);

    /* Get the current process and it's session */
    CurrentProcess = PsGetCurrentProcess();
    CurrentSession = CurrentProcess->Session;

    /* Acquire the expansion lock while touching the session */
    OldIrql = MiAcquireExpansionLock();

    /* Check if the session is about to be deleted */
    if (EntrySession->u.Flags.DeletePending)
    {
        /* We cannot attach to it, so unlock and fail */
        MiReleaseExpansionLock(OldIrql);
        return STATUS_PROCESS_IS_TERMINATING;
    }

    /* Count the number of attaches */
    EntrySession->AttachCount++;

    /* we can release the lock again */
    MiReleaseExpansionLock(OldIrql);

    /* Check if we are not the session leader and we are in a session */
    if (!CurrentProcess->Vm.Flags.SessionLeader && (CurrentSession != NULL))
    {
        /* Are we already in the right session? */
        if (CurrentSession == EntrySession)
        {
            /* We are, so "attach" to the current process */
            EntryProcess = CurrentProcess;
        }
        else
        {
            /* We are not, the session id should better not match! */
            ASSERT(CurrentSession->SessionId != EntrySession->SessionId);
        }
    }

    /* Now attach to the process that we have */
    KeStackAttachProcess(&EntryProcess->Pcb, ApcState);

    /* Success! */
    return STATUS_SUCCESS;
}

_IRQL_requires_max_(APC_LEVEL)
VOID
NTAPI
MmDetachSession(
    _Inout_ PVOID SessionEntry,
    _In_ PKAPC_STATE ApcState)
{
    PEPROCESS EntryProcess;
    PMM_SESSION_SPACE EntrySession;
    KIRQL OldIrql;
    BOOLEAN DeletePending;

    /* The parameter is the actual process! */
    EntryProcess = SessionEntry;
    ASSERT(EntryProcess != NULL);

    /* Sanity checks */
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    ASSERT(EntryProcess->Vm.Flags.SessionLeader == 0);

    /* Get the session from the process that was passed in */
    EntrySession = EntryProcess->Session;
    ASSERT(EntrySession != NULL);

    /* Acquire the expansion lock while touching the session */
    OldIrql = MiAcquireExpansionLock();

    /* Make sure we have at least one attach and decrement the count */
    ASSERT(EntrySession->AttachCount >= 1);
    EntrySession->AttachCount--;

    /* Remember if a delete is pending and we were the last one attached */
    DeletePending = EntrySession->u.Flags.DeletePending &&
                    (EntrySession->AttachCount == 0);

    /* Release the lock again */
    MiReleaseExpansionLock(OldIrql);

    /* Detach from the process */
    KeUnstackDetachProcess(ApcState);

    /* Check if we need to set the attach event */
    if (DeletePending)
        KeSetEvent(&EntrySession->AttachEvent, IO_NO_INCREMENT, FALSE);
}

VOID
NTAPI
MmQuitNextSession(
    _Inout_ PVOID SessionEntry)
{
    PEPROCESS EntryProcess;

    /* The parameter is the actual process! */
    EntryProcess = SessionEntry;
    ASSERT(EntryProcess != NULL);

    /* Sanity checks */
    ASSERT(KeGetCurrentIrql () <= APC_LEVEL);
    ASSERT(EntryProcess->Vm.Flags.SessionLeader == 0);
    ASSERT(EntryProcess->Session != NULL);

    /* Get rid of the reference we took */
    ObDereferenceObject(EntryProcess);
}

PVOID
NTAPI
MmGetSessionById(
    _In_ ULONG SessionId)
{
    PLIST_ENTRY ListEntry;
    PMM_SESSION_SPACE Session;
    PEPROCESS Process = NULL;
    KIRQL OldIrql;

    /* Acquire the expansion lock while touching the session */
    OldIrql = MiAcquireExpansionLock();

    /* Loop all entries in the session ws list */
    ListEntry = MiSessionWsList.Flink;
    while (ListEntry != &MiSessionWsList)
    {
        Session = CONTAINING_RECORD(ListEntry, MM_SESSION_SPACE, WsListEntry);

        /* Check if this is the session we are looking for */
        if (Session->SessionId == SessionId)
        {
            /* Check if we also have a process in the process list */
            if (!IsListEmpty(&Session->ProcessList))
            {
                Process = CONTAINING_RECORD(Session->ProcessList.Flink,
                                            EPROCESS,
                                            SessionProcessLinks);

                /* Reference the process */
                ObReferenceObject(Process);
                break;
            }
        }
    }

    /* Release the lock again */
    MiReleaseExpansionLock(OldIrql);

    return Process;
}
