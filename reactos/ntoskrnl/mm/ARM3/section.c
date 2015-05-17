/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/section.c
 * PURPOSE:         ARM Memory Manager Section Support
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include <mm/ARM3/miarm.h>

/* GLOBALS ********************************************************************/

ACCESS_MASK MmMakeSectionAccess[8] =
{
    SECTION_MAP_READ,
    SECTION_MAP_READ,
    SECTION_MAP_EXECUTE,
    SECTION_MAP_EXECUTE | SECTION_MAP_READ,
    SECTION_MAP_WRITE,
    SECTION_MAP_READ,
    SECTION_MAP_EXECUTE | SECTION_MAP_WRITE,
    SECTION_MAP_EXECUTE | SECTION_MAP_READ
};

ACCESS_MASK MmMakeFileAccess[8] =
{
    FILE_READ_DATA,
    FILE_READ_DATA,
    FILE_EXECUTE,
    FILE_EXECUTE | FILE_READ_DATA,
    FILE_WRITE_DATA | FILE_READ_DATA,
    FILE_READ_DATA,
    FILE_EXECUTE | FILE_WRITE_DATA | FILE_READ_DATA,
    FILE_EXECUTE | FILE_READ_DATA
};

CHAR MmUserProtectionToMask1[16] =
{
    0,
    MM_NOACCESS,
    MM_READONLY,
    (CHAR)MM_INVALID_PROTECTION,
    MM_READWRITE,
    (CHAR)MM_INVALID_PROTECTION,
    (CHAR)MM_INVALID_PROTECTION,
    (CHAR)MM_INVALID_PROTECTION,
    MM_WRITECOPY,
    (CHAR)MM_INVALID_PROTECTION,
    (CHAR)MM_INVALID_PROTECTION,
    (CHAR)MM_INVALID_PROTECTION,
    (CHAR)MM_INVALID_PROTECTION,
    (CHAR)MM_INVALID_PROTECTION,
    (CHAR)MM_INVALID_PROTECTION,
    (CHAR)MM_INVALID_PROTECTION
};

CHAR MmUserProtectionToMask2[16] =
{
    0,
    MM_EXECUTE,
    MM_EXECUTE_READ,
    (CHAR)MM_INVALID_PROTECTION,
    MM_EXECUTE_READWRITE,
    (CHAR)MM_INVALID_PROTECTION,
    (CHAR)MM_INVALID_PROTECTION,
    (CHAR)MM_INVALID_PROTECTION,
    MM_EXECUTE_WRITECOPY,
    (CHAR)MM_INVALID_PROTECTION,
    (CHAR)MM_INVALID_PROTECTION,
    (CHAR)MM_INVALID_PROTECTION,
    (CHAR)MM_INVALID_PROTECTION,
    (CHAR)MM_INVALID_PROTECTION,
    (CHAR)MM_INVALID_PROTECTION,
    (CHAR)MM_INVALID_PROTECTION
};

ULONG MmCompatibleProtectionMask[8] =
{
    PAGE_NOACCESS,

    PAGE_NOACCESS | PAGE_READONLY | PAGE_WRITECOPY,

    PAGE_NOACCESS | PAGE_EXECUTE,

    PAGE_NOACCESS | PAGE_READONLY | PAGE_WRITECOPY | PAGE_EXECUTE |
    PAGE_EXECUTE_READ,

    PAGE_NOACCESS | PAGE_READONLY | PAGE_WRITECOPY | PAGE_READWRITE,

    PAGE_NOACCESS | PAGE_READONLY | PAGE_WRITECOPY,

    PAGE_NOACCESS | PAGE_READONLY | PAGE_WRITECOPY | PAGE_READWRITE |
    PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE |
    PAGE_EXECUTE_WRITECOPY,

    PAGE_NOACCESS | PAGE_READONLY | PAGE_WRITECOPY | PAGE_EXECUTE |
    PAGE_EXECUTE_READ | PAGE_EXECUTE_WRITECOPY
};

MMSESSION MmSession;
KGUARDED_MUTEX MmSectionCommitMutex;
MM_AVL_TABLE MmSectionBasedRoot;
KGUARDED_MUTEX MmSectionBasedMutex;
PVOID MmHighSectionBase;

/* PRIVATE FUNCTIONS **********************************************************/

BOOLEAN
NTAPI
MiIsProtectionCompatible(IN ULONG SectionPageProtection,
                         IN ULONG NewSectionPageProtection)
{
    ULONG ProtectionMask, CompatibleMask;

    /* Calculate the protection mask and make sure it's valid */
    ProtectionMask = MiMakeProtectionMask(SectionPageProtection);
    if (ProtectionMask == MM_INVALID_PROTECTION)
    {
        DPRINT1("Invalid protection mask\n");
        return FALSE;
    }

    /* Calculate the compatible mask */
    CompatibleMask = MmCompatibleProtectionMask[ProtectionMask & 0x7] |
                     PAGE_GUARD | PAGE_NOCACHE | PAGE_WRITECOMBINE;

    /* See if the mapping protection is compatible with the create protection */
    return ((CompatibleMask | NewSectionPageProtection) == CompatibleMask);
}

ACCESS_MASK
NTAPI
MiArm3GetCorrectFileAccessMask(IN ACCESS_MASK SectionPageProtection)
{
    ULONG ProtectionMask;

    /* Calculate the protection mask and make sure it's valid */
    ProtectionMask = MiMakeProtectionMask(SectionPageProtection);
    if (ProtectionMask == MM_INVALID_PROTECTION)
    {
        DPRINT1("Invalid protection mask\n");
        return STATUS_INVALID_PAGE_PROTECTION;
    }

    /* Now convert it to the required file access */
    return MmMakeFileAccess[ProtectionMask & 0x7];
}

ULONG
NTAPI
MiMakeProtectionMask(IN ULONG Protect)
{
    ULONG Mask1, Mask2, ProtectMask;

    /* PAGE_EXECUTE_WRITECOMBINE is theoretically the maximum */
    if (Protect >= (PAGE_WRITECOMBINE * 2)) return MM_INVALID_PROTECTION;

    /*
     * Windows API protection mask can be understood as two bitfields, differing
     * by whether or not execute rights are being requested
     */
    Mask1 = Protect & 0xF;
    Mask2 = (Protect >> 4) & 0xF;

    /* Check which field is there */
    if (!Mask1)
    {
        /* Mask2 must be there, use it to determine the PTE protection */
        if (!Mask2) return MM_INVALID_PROTECTION;
        ProtectMask = MmUserProtectionToMask2[Mask2];
    }
    else
    {
        /* Mask2 should not be there, use Mask1 to determine the PTE mask */
        if (Mask2) return MM_INVALID_PROTECTION;
        ProtectMask = MmUserProtectionToMask1[Mask1];
    }

    /* Make sure the final mask is a valid one */
    if (ProtectMask == MM_INVALID_PROTECTION) return MM_INVALID_PROTECTION;

    /* Check for PAGE_GUARD option */
    if (Protect & PAGE_GUARD)
    {
        /* It's not valid on no-access, nocache, or writecombine pages */
        if ((ProtectMask == MM_NOACCESS) ||
            (Protect & (PAGE_NOCACHE | PAGE_WRITECOMBINE)))
        {
            /* Fail such requests */
            return MM_INVALID_PROTECTION;
        }

        /* This actually turns on guard page in this scenario! */
        ProtectMask |= MM_GUARDPAGE;
    }

    /* Check for nocache option */
    if (Protect & PAGE_NOCACHE)
    {
        /* The earlier check should've eliminated this possibility */
        ASSERT((Protect & PAGE_GUARD) == 0);

        /* Check for no-access page or write combine page */
        if ((ProtectMask == MM_NOACCESS) || (Protect & PAGE_WRITECOMBINE))
        {
            /* Such a request is invalid */
            return MM_INVALID_PROTECTION;
        }

        /* Add the PTE flag */
        ProtectMask |= MM_NOCACHE;
    }

    /* Check for write combine option */
    if (Protect & PAGE_WRITECOMBINE)
    {
        /* The two earlier scenarios should've caught this */
        ASSERT((Protect & (PAGE_GUARD | PAGE_NOACCESS)) == 0);

        /* Don't allow on no-access pages */
        if (ProtectMask == MM_NOACCESS) return MM_INVALID_PROTECTION;

        /* This actually turns on write-combine in this scenario! */
        ProtectMask |= MM_NOACCESS;
    }

    /* Return the final MM PTE protection mask */
    return ProtectMask;
}

BOOLEAN
NTAPI
MiInitializeSystemSpaceMap(IN PMMSESSION InputSession OPTIONAL)
{
    SIZE_T AllocSize, BitmapSize, Size;
    PVOID ViewStart;
    PMMSESSION Session;

    /* Check if this a session or system space */
    if (InputSession)
    {
        /* Use the input session */
        Session = InputSession;
        ViewStart = MiSessionViewStart;
        Size = MmSessionViewSize;
    }
    else
    {
        /* Use the system space "session" */
        Session = &MmSession;
        ViewStart = MiSystemViewStart;
        Size = MmSystemViewSize;
    }

    /* Initialize the system space lock */
    Session->SystemSpaceViewLockPointer = &Session->SystemSpaceViewLock;
    KeInitializeGuardedMutex(Session->SystemSpaceViewLockPointer);

    /* Set the start address */
    Session->SystemSpaceViewStart = ViewStart;

    /* Create a bitmap to describe system space */
    BitmapSize = sizeof(RTL_BITMAP) + ((((Size / MI_SYSTEM_VIEW_BUCKET_SIZE) + 31) / 32) * sizeof(ULONG));
    Session->SystemSpaceBitMap = ExAllocatePoolWithTag(NonPagedPool,
                                                       BitmapSize,
                                                       TAG_MM);
    ASSERT(Session->SystemSpaceBitMap);
    RtlInitializeBitMap(Session->SystemSpaceBitMap,
                        (PULONG)(Session->SystemSpaceBitMap + 1),
                        (ULONG)(Size / MI_SYSTEM_VIEW_BUCKET_SIZE));

    /* Set system space fully empty to begin with */
    RtlClearAllBits(Session->SystemSpaceBitMap);

    /* Set default hash flags */
    Session->SystemSpaceHashSize = 31;
    Session->SystemSpaceHashKey = Session->SystemSpaceHashSize - 1;
    Session->SystemSpaceHashEntries = 0;

    /* Calculate how much space for the hash views we'll need */
    AllocSize = sizeof(MMVIEW) * Session->SystemSpaceHashSize;
    ASSERT(AllocSize < PAGE_SIZE);

    /* Allocate and zero the view table */
    Session->SystemSpaceViewTable = ExAllocatePoolWithTag(Session == &MmSession ?
                                                          NonPagedPool :
                                                          PagedPool,
                                                          AllocSize,
                                                          TAG_MM);
    ASSERT(Session->SystemSpaceViewTable != NULL);
    RtlZeroMemory(Session->SystemSpaceViewTable, AllocSize);

    /* Success */
    return TRUE;
}

PVOID
NTAPI
MiInsertInSystemSpace(IN PMMSESSION Session,
                      IN ULONG Buckets,
                      IN PCONTROL_AREA ControlArea)
{
    PVOID Base;
    ULONG Entry, Hash, i, HashSize;
    PMMVIEW OldTable;
    PAGED_CODE();

    /* Stay within 4GB */
    ASSERT(Buckets < MI_SYSTEM_VIEW_BUCKET_SIZE);

    /* Lock system space */
    KeAcquireGuardedMutex(Session->SystemSpaceViewLockPointer);

    /* Check if we're going to exhaust hash entries */
    if ((Session->SystemSpaceHashEntries + 8) > Session->SystemSpaceHashSize)
    {
        /* Double the hash size */
        HashSize = Session->SystemSpaceHashSize * 2;

        /* Save the old table and allocate a new one */
        OldTable = Session->SystemSpaceViewTable;
        Session->SystemSpaceViewTable = ExAllocatePoolWithTag(Session ==
                                                              &MmSession ?
                                                              NonPagedPool :
                                                              PagedPool,
                                                              HashSize *
                                                              sizeof(MMVIEW),
                                                              TAG_MM);
        if (!Session->SystemSpaceViewTable)
        {
            /* Failed to allocate a new table, keep the old one for now */
            Session->SystemSpaceViewTable = OldTable;
        }
        else
        {
            /* Clear the new table and set the new ahsh and key */
            RtlZeroMemory(Session->SystemSpaceViewTable, HashSize * sizeof(MMVIEW));
            Session->SystemSpaceHashSize = HashSize;
            Session->SystemSpaceHashKey = Session->SystemSpaceHashSize - 1;

            /* Loop the old table */
            for (i = 0; i < Session->SystemSpaceHashSize / 2; i++)
            {
                /* Check if the entry was valid */
                if (OldTable[i].Entry)
                {
                    /* Re-hash the old entry and search for space in the new table */
                    Hash = (OldTable[i].Entry >> 16) % Session->SystemSpaceHashKey;
                    while (Session->SystemSpaceViewTable[Hash].Entry)
                    {
                        /* Loop back at the beginning if we had an overflow */
                        if (++Hash >= Session->SystemSpaceHashSize) Hash = 0;
                    }

                    /* Write the old entry in the new table */
                    Session->SystemSpaceViewTable[Hash] = OldTable[i];
                }
            }

            /* Free the old table */
            ExFreePool(OldTable);
        }
    }

    /* Check if we ran out */
    if (Session->SystemSpaceHashEntries == Session->SystemSpaceHashSize)
    {
        DPRINT1("Ran out of system view hash entries\n");
        KeReleaseGuardedMutex(Session->SystemSpaceViewLockPointer);
        return NULL;
    }

    /* Find space where to map this view */
    i = RtlFindClearBitsAndSet(Session->SystemSpaceBitMap, Buckets, 0);
    if (i == 0xFFFFFFFF)
    {
        /* Out of space, fail */
        Session->BitmapFailures++;
        DPRINT1("Out of system view space\n");
        KeReleaseGuardedMutex(Session->SystemSpaceViewLockPointer);
        return NULL;
    }

    /* Compute the base address */
    Base = (PVOID)((ULONG_PTR)Session->SystemSpaceViewStart + (i * MI_SYSTEM_VIEW_BUCKET_SIZE));

    /* Get the hash entry for this allocation */
    Entry = ((ULONG_PTR)Base & ~(MI_SYSTEM_VIEW_BUCKET_SIZE - 1)) + Buckets;
    Hash = (Entry >> 16) % Session->SystemSpaceHashKey;

    /* Loop hash entries until a free one is found */
    while (Session->SystemSpaceViewTable[Hash].Entry)
    {
        /* Unless we overflow, in which case loop back at hash o */
        if (++Hash >= Session->SystemSpaceHashSize) Hash = 0;
    }

    /* Add this entry into the hash table */
    Session->SystemSpaceViewTable[Hash].Entry = Entry;
    Session->SystemSpaceViewTable[Hash].ControlArea = ControlArea;

    /* Hash entry found, increment total and return the base address */
    Session->SystemSpaceHashEntries++;
    KeReleaseGuardedMutex(Session->SystemSpaceViewLockPointer);
    return Base;
}

NTSTATUS
NTAPI
MiAddMappedPtes(IN PMMPTE FirstPte,
                IN PFN_NUMBER PteCount,
                IN PCONTROL_AREA ControlArea)
{
    MMPTE TempPte;
    PMMPTE PointerPte, ProtoPte, LastProtoPte, LastPte;
    PSUBSECTION Subsection;

    /* ARM3 doesn't support this yet */
    ASSERT(ControlArea->u.Flags.GlobalOnlyPerSession == 0);
    ASSERT(ControlArea->u.Flags.Rom == 0);
    ASSERT(ControlArea->FilePointer == NULL);

    /* Sanity checks */
    ASSERT(PteCount != 0);
    ASSERT(ControlArea->NumberOfMappedViews >= 1);
    ASSERT(ControlArea->NumberOfUserReferences >= 1);
    ASSERT(ControlArea->NumberOfSectionReferences != 0);
    ASSERT(ControlArea->u.Flags.BeingCreated == 0);
    ASSERT(ControlArea->u.Flags.BeingDeleted == 0);
    ASSERT(ControlArea->u.Flags.BeingPurged == 0);

    /* Get the PTEs for the actual mapping */
    PointerPte = FirstPte;
    LastPte = FirstPte + PteCount;

    /* Get the prototype PTEs that desribe the section mapping in the subsection */
    Subsection = (PSUBSECTION)(ControlArea + 1);
    ProtoPte = Subsection->SubsectionBase;
    LastProtoPte = &Subsection->SubsectionBase[Subsection->PtesInSubsection];

    /* Loop the PTEs for the mapping */
    while (PointerPte < LastPte)
    {
        /* We may have run out of prototype PTEs in this subsection */
        if (ProtoPte >= LastProtoPte)
        {
            /* But we don't handle this yet */
            ASSERT(FALSE);
        }

        /* The PTE should be completely clear */
        ASSERT(PointerPte->u.Long == 0);

        /* Build the prototype PTE and write it */
        MI_MAKE_PROTOTYPE_PTE(&TempPte, ProtoPte);
        MI_WRITE_INVALID_PTE(PointerPte, TempPte);

        /* Keep going */
        PointerPte++;
        ProtoPte++;
    }

    /* No failure path */
    return STATUS_SUCCESS;
}

VOID
NTAPI
MiFillSystemPageDirectory(IN PVOID Base,
                          IN SIZE_T NumberOfBytes)
{
    PMMPDE PointerPde, LastPde, SystemMapPde;
    MMPDE TempPde;
    PFN_NUMBER PageFrameIndex, ParentPage;
    KIRQL OldIrql;
    PAGED_CODE();

    /* Find the PDEs needed for this mapping */
    PointerPde = MiAddressToPde(Base);
    LastPde = MiAddressToPde((PVOID)((ULONG_PTR)Base + NumberOfBytes - 1));

#if (_MI_PAGING_LEVELS == 2)
    /* Find the system double-mapped PDE that describes this mapping */
    SystemMapPde = &MmSystemPagePtes[((ULONG_PTR)PointerPde & (SYSTEM_PD_SIZE - 1)) / sizeof(MMPTE)];
#else
    /* We don't have a double mapping */
    SystemMapPde = PointerPde;
#endif

    /* Use the PDE template and loop the PDEs */
    TempPde = ValidKernelPde;
    while (PointerPde <= LastPde)
    {
        /* Lock the PFN database */
        OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

        /* Check if we don't already have this PDE mapped */
        if (SystemMapPde->u.Hard.Valid == 0)
        {
            /* Grab a page for it */
            MI_SET_USAGE(MI_USAGE_PAGE_TABLE);
            MI_SET_PROCESS2(PsGetCurrentProcess()->ImageFileName);
            PageFrameIndex = MiRemoveZeroPage(MI_GET_NEXT_COLOR());
            ASSERT(PageFrameIndex);
            TempPde.u.Hard.PageFrameNumber = PageFrameIndex;

#if (_MI_PAGING_LEVELS == 2)
            ParentPage = MmSystemPageDirectory[(PointerPde - MiAddressToPde(NULL)) / PDE_COUNT];
#else
            ParentPage = MiPdeToPpe(PointerPde)->u.Hard.PageFrameNumber;
#endif
            /* Initialize its PFN entry, with the parent system page directory page table */
            MiInitializePfnForOtherProcess(PageFrameIndex,
                                           (PMMPTE)PointerPde,
                                           ParentPage);

            /* Make the system PDE entry valid */
            MI_WRITE_VALID_PDE(SystemMapPde, TempPde);

            /* The system PDE entry might be the PDE itself, so check for this */
            if (PointerPde->u.Hard.Valid == 0)
            {
                /* It's different, so make the real PDE valid too */
                MI_WRITE_VALID_PDE(PointerPde, TempPde);
            }
        }

        /* Release the lock and keep going with the next PDE */
        KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
        SystemMapPde++;
        PointerPde++;
    }
}

NTSTATUS
NTAPI
MiCheckPurgeAndUpMapCount(IN PCONTROL_AREA ControlArea,
                          IN BOOLEAN FailIfSystemViews)
{
    KIRQL OldIrql;

    /* Flag not yet supported */
    ASSERT(FailIfSystemViews == FALSE);

    /* Lock the PFN database */
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

    /* State not yet supported */
    ASSERT(ControlArea->u.Flags.BeingPurged == 0);

    /* Increase the reference counts */
    ControlArea->NumberOfMappedViews++;
    ControlArea->NumberOfUserReferences++;
    ASSERT(ControlArea->NumberOfSectionReferences != 0);

    /* Release the PFN lock and return success */
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
    return STATUS_SUCCESS;
}

PSUBSECTION
NTAPI
MiLocateSubsection(IN PMMVAD Vad,
                   IN ULONG_PTR Vpn)
{
    PSUBSECTION Subsection;
    PCONTROL_AREA ControlArea;
    ULONG_PTR PteOffset;

    /* Get the control area */
    ControlArea = Vad->ControlArea;
    ASSERT(ControlArea->u.Flags.Rom == 0);
    ASSERT(ControlArea->u.Flags.Image == 0);
    ASSERT(ControlArea->u.Flags.GlobalOnlyPerSession == 0);

    /* Get the subsection */
    Subsection = (PSUBSECTION)(ControlArea + 1);

    /* We only support single-subsection segments */
    ASSERT(Subsection->SubsectionBase != NULL);
    ASSERT(Vad->FirstPrototypePte >= Subsection->SubsectionBase);
    ASSERT(Vad->FirstPrototypePte < &Subsection->SubsectionBase[Subsection->PtesInSubsection]);

    /* Compute the PTE offset */
    PteOffset = Vpn - Vad->StartingVpn;
    PteOffset += Vad->FirstPrototypePte - Subsection->SubsectionBase;

    /* Again, we only support single-subsection segments */
    ASSERT(PteOffset < 0xF0000000);
    ASSERT(PteOffset < Subsection->PtesInSubsection);

    /* Return the subsection */
    return Subsection;
}

VOID
NTAPI
MiSegmentDelete(IN PSEGMENT Segment)
{
    PCONTROL_AREA ControlArea;
    SEGMENT_FLAGS SegmentFlags;
    PSUBSECTION Subsection;
    PMMPTE PointerPte, LastPte, PteForProto;
    PMMPFN Pfn1;
    PFN_NUMBER PageFrameIndex;
    MMPTE TempPte;
    KIRQL OldIrql;

    /* Capture data */
    SegmentFlags = Segment->SegmentFlags;
    ControlArea = Segment->ControlArea;

    /* Make sure control area is on the right delete path */
    ASSERT(ControlArea->u.Flags.BeingDeleted == 1);
    ASSERT(ControlArea->WritableUserReferences == 0);

    /* These things are not supported yet */
    ASSERT(ControlArea->DereferenceList.Flink == NULL);
    ASSERT(!(ControlArea->u.Flags.Image) && !(ControlArea->u.Flags.File));
    ASSERT(ControlArea->u.Flags.GlobalOnlyPerSession == 0);
    ASSERT(ControlArea->u.Flags.Rom == 0);

    /* Get the subsection and PTEs for this segment */
    Subsection = (PSUBSECTION)(ControlArea + 1);
    PointerPte = Subsection->SubsectionBase;
    LastPte = PointerPte + Segment->NonExtendedPtes;

    /* Lock the PFN database */
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

    /* Check if the master PTE is invalid */
    PteForProto = MiAddressToPte(PointerPte);
    if (!PteForProto->u.Hard.Valid)
    {
        /* Fault it in */
        MiMakeSystemAddressValidPfn(PointerPte, OldIrql);
    }

    /* Loop all the segment PTEs */
    while (PointerPte < LastPte)
    {
        /* Check if it's time to switch master PTEs if we passed a PDE boundary */
        if (!((ULONG_PTR)PointerPte & (PD_SIZE - 1)) &&
            (PointerPte != Subsection->SubsectionBase))
        {
            /* Check if the master PTE is invalid */
            PteForProto = MiAddressToPte(PointerPte);
            if (!PteForProto->u.Hard.Valid)
            {
                /* Fault it in */
                MiMakeSystemAddressValidPfn(PointerPte, OldIrql);
            }
        }

        /* This should be a prototype PTE */
        TempPte = *PointerPte;
        ASSERT(SegmentFlags.LargePages == 0);
        ASSERT(TempPte.u.Hard.Valid == 0);

        /* See if we should clean things up */
        if (!(ControlArea->u.Flags.Image) && !(ControlArea->u.Flags.File))
        {
            /*
             * This is a section backed by the pagefile. Now that it doesn't exist anymore,
             * we can give everything back to the system.
             */
            ASSERT(TempPte.u.Soft.Prototype == 0);

            if (TempPte.u.Soft.Transition == 1)
            {
                /* We can give the page back for other use */
                DPRINT("Releasing page for transition PTE %p\n", PointerPte);
                PageFrameIndex = PFN_FROM_PTE(&TempPte);
                Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);

                /* As this is a paged-backed section, nobody should reference it anymore (no cache or whatever) */
                ASSERT(Pfn1->u3.ReferenceCount == 0);

                /* And it should be in standby or modified list */
                ASSERT((Pfn1->u3.e1.PageLocation == ModifiedPageList) || (Pfn1->u3.e1.PageLocation == StandbyPageList));

                /* Unlink it and put it back in free list */
                MiUnlinkPageFromList(Pfn1);

                /* Temporarily mark this as active and make it free again */
                Pfn1->u3.e1.PageLocation = ActiveAndValid;
                MI_SET_PFN_DELETED(Pfn1);

                MiInsertPageInFreeList(PageFrameIndex);
            }
            else if (TempPte.u.Soft.PageFileHigh != 0)
            {
                /* Should not happen for now */
                ASSERT(FALSE);
            }
        }
        else
        {
            /* unsupported for now */
            ASSERT(FALSE);

            /* File-backed section must have prototype PTEs */
            ASSERT(TempPte.u.Soft.Prototype == 1);
        }

        /* Zero the PTE and keep going */
        PointerPte->u.Long = 0;
        PointerPte++;
    }

    /* Release the PFN lock */
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);

    /* Free the structures */
    ExFreePool(ControlArea);
    ExFreePool(Segment);
}

VOID
NTAPI
MiCheckControlArea(IN PCONTROL_AREA ControlArea,
                   IN KIRQL OldIrql)
{
    BOOLEAN DeleteSegment = FALSE;
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    /* Check if this is the last reference or view */
    if (!(ControlArea->NumberOfMappedViews) &&
        !(ControlArea->NumberOfSectionReferences))
    {
        /* There should be no more user references either */
        ASSERT(ControlArea->NumberOfUserReferences == 0);

        /* Not yet supported */
        ASSERT(ControlArea->FilePointer == NULL);

        /* The control area is being destroyed */
        ControlArea->u.Flags.BeingDeleted = TRUE;
        DeleteSegment = TRUE;
    }

    /* Release the PFN lock */
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);

    /* Delete the segment if needed */
    if (DeleteSegment)
    {
        /* No more user write references at all */
        ASSERT(ControlArea->WritableUserReferences == 0);
        MiSegmentDelete(ControlArea->Segment);
    }
}

VOID
NTAPI
MiDereferenceControlArea(IN PCONTROL_AREA ControlArea)
{
    KIRQL OldIrql;

    /* Lock the PFN database */
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

    /* Drop reference counts */
    ControlArea->NumberOfMappedViews--;
    ControlArea->NumberOfUserReferences--;

    /* Check if it's time to delete the CA. This releases the lock */
    MiCheckControlArea(ControlArea, OldIrql);
}

VOID
NTAPI
MiRemoveMappedView(IN PEPROCESS CurrentProcess,
                   IN PMMVAD Vad)
{
    KIRQL OldIrql;
    PCONTROL_AREA ControlArea;
    PETHREAD CurrentThread = PsGetCurrentThread();

    /* Get the control area */
    ControlArea = Vad->ControlArea;

    /* We only support non-extendable, non-image, pagefile-backed regular sections */
    ASSERT(Vad->u.VadFlags.VadType == VadNone);
    ASSERT(Vad->u2.VadFlags2.ExtendableFile == FALSE);
    ASSERT(ControlArea);
    ASSERT(ControlArea->FilePointer == NULL);

    /* Delete the actual virtual memory pages */
    MiDeleteVirtualAddresses(Vad->StartingVpn << PAGE_SHIFT,
                             (Vad->EndingVpn << PAGE_SHIFT) | (PAGE_SIZE - 1),
                             Vad);

    /* Release the working set */
    MiUnlockProcessWorkingSetUnsafe(CurrentProcess, CurrentThread);

    /* Lock the PFN database */
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

    /* Remove references */
    ControlArea->NumberOfMappedViews--;
    ControlArea->NumberOfUserReferences--;

    /* Check if it should be destroyed */
    MiCheckControlArea(ControlArea, OldIrql);
}

NTSTATUS
NTAPI
MiUnmapViewOfSection(IN PEPROCESS Process,
                     IN PVOID BaseAddress,
                     IN ULONG Flags)
{
    PMEMORY_AREA MemoryArea;
    BOOLEAN Attached = FALSE;
    KAPC_STATE ApcState;
    PMMVAD Vad;
    PVOID DbgBase = NULL;
    SIZE_T RegionSize;
    NTSTATUS Status;
    PETHREAD CurrentThread = PsGetCurrentThread();
    PEPROCESS CurrentProcess = PsGetCurrentProcess();
    PAGED_CODE();

    /* Check for Mm Region */
    MemoryArea = MmLocateMemoryAreaByAddress(&Process->Vm, BaseAddress);
    if ((MemoryArea) && (MemoryArea->Type != MEMORY_AREA_OWNED_BY_ARM3))
    {
        /* Call Mm API */
        return MiRosUnmapViewOfSection(Process, BaseAddress, Flags);
    }

    /* Check if we should attach to the process */
    if (CurrentProcess != Process)
    {
        /* The process is different, do an attach */
        KeStackAttachProcess(&Process->Pcb, &ApcState);
        Attached = TRUE;
    }

    /* Check if we need to lock the address space */
    if (!Flags) MmLockAddressSpace(&Process->Vm);

    /* Check if the process is already daed */
    if (Process->VmDeleted)
    {
        /* Fail the call */
        DPRINT1("Process died!\n");
        if (!Flags) MmUnlockAddressSpace(&Process->Vm);
        Status = STATUS_PROCESS_IS_TERMINATING;
        goto Quickie;
    }

    /* Find the VAD for the address and make sure it's a section VAD */
    Vad = MiLocateAddress(BaseAddress);
    if (!(Vad) || (Vad->u.VadFlags.PrivateMemory))
    {
        /* Couldn't find it, or invalid VAD, fail */
        DPRINT1("No VAD or invalid VAD\n");
        if (!Flags) MmUnlockAddressSpace(&Process->Vm);
        Status = STATUS_NOT_MAPPED_VIEW;
        goto Quickie;
    }

    /* We should be attached */
    ASSERT(Process == PsGetCurrentProcess());

    /* We need the base address for the debugger message on image-backed VADs */
    if (Vad->u.VadFlags.VadType == VadImageMap)
    {
        DbgBase = (PVOID)(Vad->StartingVpn >> PAGE_SHIFT);
    }

    /* Compute the size of the VAD region */
    RegionSize = PAGE_SIZE + ((Vad->EndingVpn - Vad->StartingVpn) << PAGE_SHIFT);

    /* For SEC_NO_CHANGE sections, we need some extra checks */
    if (Vad->u.VadFlags.NoChange == 1)
    {
        /* Are we allowed to mess with this VAD? */
        Status = MiCheckSecuredVad(Vad,
                                   (PVOID)(Vad->StartingVpn >> PAGE_SHIFT),
                                   RegionSize,
                                   MM_DELETE_CHECK);
        if (!NT_SUCCESS(Status))
        {
            /* We failed */
            DPRINT1("Trying to unmap protected VAD!\n");
            if (!Flags) MmUnlockAddressSpace(&Process->Vm);
            goto Quickie;
        }
    }

    /* Not currently supported */
    ASSERT(Vad->u.VadFlags.VadType != VadRotatePhysical);

    /* FIXME: Remove VAD charges */

    /* Lock the working set */
    MiLockProcessWorkingSetUnsafe(Process, CurrentThread);

    /* Remove the VAD */
    ASSERT(Process->VadRoot.NumberGenericTableElements >= 1);
    MiRemoveNode((PMMADDRESS_NODE)Vad, &Process->VadRoot);

    /* Remove the PTEs for this view, which also releases the working set lock */
    MiRemoveMappedView(Process, Vad);

    /* FIXME: Remove commitment */

    /* Update performance counter and release the lock */
    Process->VirtualSize -= RegionSize;
    if (!Flags) MmUnlockAddressSpace(&Process->Vm);

    /* Destroy the VAD and return success */
    ExFreePool(Vad);
    Status = STATUS_SUCCESS;

    /* Failure and success case -- send debugger message, detach, and return */
Quickie:
    if (DbgBase) DbgkUnMapViewOfSection(DbgBase);
    if (Attached) KeUnstackDetachProcess(&ApcState);
    return Status;
}

NTSTATUS
NTAPI
MiSessionCommitPageTables(IN PVOID StartVa,
                          IN PVOID EndVa)
{
    KIRQL OldIrql;
    ULONG Color, Index;
    PMMPDE StartPde, EndPde;
    MMPDE TempPde = ValidKernelPdeLocal;
    PMMPFN Pfn1;
    PFN_NUMBER PageCount = 0, ActualPages = 0, PageFrameNumber;

    /* Windows sanity checks */
    ASSERT(StartVa >= (PVOID)MmSessionBase);
    ASSERT(EndVa < (PVOID)MiSessionSpaceEnd);
    ASSERT(PAGE_ALIGN(EndVa) == EndVa);

    /* Get the start and end PDE, then loop each one */
    StartPde = MiAddressToPde(StartVa);
    EndPde = MiAddressToPde((PVOID)((ULONG_PTR)EndVa - 1));
    Index = ((ULONG_PTR)StartVa - (ULONG_PTR)MmSessionBase) >> 22;
    while (StartPde <= EndPde)
    {
#ifndef _M_AMD64
        /* If we don't already have a page table for it, increment count */
        if (MmSessionSpace->PageTables[Index].u.Long == 0) PageCount++;
#endif
        /* Move to the next one */
        StartPde++;
        Index++;
    }

    /* If there's no page tables to create, bail out */
    if (PageCount == 0) return STATUS_SUCCESS;

    /* Reset the start PDE and index */
    StartPde = MiAddressToPde(StartVa);
    Index = ((ULONG_PTR)StartVa - (ULONG_PTR)MmSessionBase) >> 22;

    /* Loop each PDE while holding the working set lock */
//  MiLockWorkingSet(PsGetCurrentThread(),
//                   &MmSessionSpace->GlobalVirtualAddress->Vm);
#ifdef _M_AMD64
_WARN("MiSessionCommitPageTables halfplemented for amd64")
    DBG_UNREFERENCED_LOCAL_VARIABLE(OldIrql);
    DBG_UNREFERENCED_LOCAL_VARIABLE(Color);
    DBG_UNREFERENCED_LOCAL_VARIABLE(TempPde);
    DBG_UNREFERENCED_LOCAL_VARIABLE(Pfn1);
    DBG_UNREFERENCED_LOCAL_VARIABLE(PageFrameNumber);
    ASSERT(FALSE);
#else
    while (StartPde <= EndPde)
    {
        /* Check if we already have a page table */
        if (MmSessionSpace->PageTables[Index].u.Long == 0)
        {
            /* We don't, so the PDE shouldn't be ready yet */
            ASSERT(StartPde->u.Hard.Valid == 0);

            /* ReactOS check to avoid MiEnsureAvailablePageOrWait */
            ASSERT(MmAvailablePages >= 32);

            /* Acquire the PFN lock and grab a zero page */
            OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
            Color = (++MmSessionSpace->Color) & MmSecondaryColorMask;
            PageFrameNumber = MiRemoveZeroPage(Color);
            TempPde.u.Hard.PageFrameNumber = PageFrameNumber;
            MI_WRITE_VALID_PDE(StartPde, TempPde);

            /* Write the page table in session space structure */
            ASSERT(MmSessionSpace->PageTables[Index].u.Long == 0);
            MmSessionSpace->PageTables[Index] = TempPde;

            /* Initialize the PFN */
            MiInitializePfnForOtherProcess(PageFrameNumber,
                                           StartPde,
                                           MmSessionSpace->SessionPageDirectoryIndex);

            /* And now release the lock */
            KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);

            /* Get the PFN entry and make sure there's no event for it */
            Pfn1 = MI_PFN_ELEMENT(PageFrameNumber);
            ASSERT(Pfn1->u1.Event == NULL);

            /* Increment the number of pages */
            ActualPages++;
        }

        /* Move to the next PDE */
        StartPde++;
        Index++;
    }
#endif

    /* Make sure we didn't do more pages than expected */
    ASSERT(ActualPages <= PageCount);

    /* Release the working set lock */
//  MiUnlockWorkingSet(PsGetCurrentThread(),
//                     &MmSessionSpace->GlobalVirtualAddress->Vm);


    /* If we did at least one page... */
    if (ActualPages)
    {
        /* Update the performance counters! */
        InterlockedExchangeAddSizeT(&MmSessionSpace->NonPageablePages, ActualPages);
        InterlockedExchangeAddSizeT(&MmSessionSpace->CommittedPages, ActualPages);
    }

    /* Return status */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MiMapViewInSystemSpace(IN PVOID Section,
                       IN PMMSESSION Session,
                       OUT PVOID *MappedBase,
                       IN OUT PSIZE_T ViewSize)
{
    PVOID Base;
    PCONTROL_AREA ControlArea;
    ULONG Buckets, SectionSize;
    NTSTATUS Status;
    PAGED_CODE();

    /* Get the control area, check for any flags ARM3 doesn't yet support */
    ControlArea = ((PSECTION)Section)->Segment->ControlArea;
    ASSERT(ControlArea->u.Flags.Image == 0);
    ASSERT(ControlArea->FilePointer == NULL);
    ASSERT(ControlArea->u.Flags.GlobalOnlyPerSession == 0);
    ASSERT(ControlArea->u.Flags.Rom == 0);
    ASSERT(ControlArea->u.Flags.WasPurged == 0);

    /* Increase the reference and map count on the control area, no purges yet */
    Status = MiCheckPurgeAndUpMapCount(ControlArea, FALSE);
    ASSERT(NT_SUCCESS(Status));

    /* Get the section size at creation time */
    SectionSize = ((PSECTION)Section)->SizeOfSection.LowPart;

    /* If the caller didn't specify a view size, assume the whole section */
    if (!(*ViewSize)) *ViewSize = SectionSize;

    /* Check if the caller wanted a larger section than the view */
    if (*ViewSize > SectionSize)
    {
        /* Fail */
        DPRINT1("View is too large\n");
        MiDereferenceControlArea(ControlArea);
        return STATUS_INVALID_VIEW_SIZE;
    }

    /* Get the number of 64K buckets required for this mapping */
    Buckets = (ULONG)(*ViewSize / MI_SYSTEM_VIEW_BUCKET_SIZE);
    if (*ViewSize & (MI_SYSTEM_VIEW_BUCKET_SIZE - 1)) Buckets++;

    /* Check if the view is more than 4GB large */
    if (Buckets >= MI_SYSTEM_VIEW_BUCKET_SIZE)
    {
        /* Fail */
        DPRINT1("View is too large\n");
        MiDereferenceControlArea(ControlArea);
        return STATUS_INVALID_VIEW_SIZE;
    }

    /* Insert this view into system space and get a base address for it */
    Base = MiInsertInSystemSpace(Session, Buckets, ControlArea);
    if (!Base)
    {
        /* Fail */
        DPRINT1("Out of system space\n");
        MiDereferenceControlArea(ControlArea);
        return STATUS_NO_MEMORY;
    }

    /* What's the underlying session? */
    if (Session == &MmSession)
    {
        /* Create the PDEs needed for this mapping, and double-map them if needed */
        MiFillSystemPageDirectory(Base, Buckets * MI_SYSTEM_VIEW_BUCKET_SIZE);
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* Create the PDEs needed for this mapping */
        Status = MiSessionCommitPageTables(Base,
                                           (PVOID)((ULONG_PTR)Base +
                                           Buckets * MI_SYSTEM_VIEW_BUCKET_SIZE));
        NT_ASSERT(NT_SUCCESS(Status));
    }

    /* Create the actual prototype PTEs for this mapping */
    Status = MiAddMappedPtes(MiAddressToPte(Base),
                             BYTES_TO_PAGES(*ViewSize),
                             ControlArea);
    ASSERT(NT_SUCCESS(Status));

    /* Return the base adress of the mapping and success */
    *MappedBase = Base;
    return STATUS_SUCCESS;
}

VOID
NTAPI
MiSetControlAreaSymbolsLoaded(IN PCONTROL_AREA ControlArea)
{
    KIRQL OldIrql;

    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
    ControlArea->u.Flags.DebugSymbolsLoaded |= 1;

    ASSERT(OldIrql <= APC_LEVEL);
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
}

VOID
NTAPI
MiLoadUserSymbols(IN PCONTROL_AREA ControlArea,
                  IN PVOID BaseAddress,
                  IN PEPROCESS Process)
{
    NTSTATUS Status;
    ANSI_STRING FileNameA;
    PLIST_ENTRY NextEntry;
    PUNICODE_STRING FileName;
    PIMAGE_NT_HEADERS NtHeaders;
    PLDR_DATA_TABLE_ENTRY LdrEntry;

    FileName = &ControlArea->FilePointer->FileName;
    if (FileName->Length == 0)
    {
        return;
    }

    /* Acquire module list lock */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&PsLoadedModuleResource, TRUE);

    /* Browse list to try to find current module */
    for (NextEntry = MmLoadedUserImageList.Flink;
         NextEntry != &MmLoadedUserImageList;
         NextEntry = NextEntry->Flink)
    {
        /* Get the entry */
        LdrEntry = CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InLoadOrderLinks);

        /* If already in the list, increase load count */
        if (LdrEntry->DllBase == BaseAddress)
        {
            ++LdrEntry->LoadCount;
            break;
        }
    }

    /* Not in the list, we'll add it */
    if (NextEntry == &MmLoadedUserImageList)
    {
        /* Allocate our element, taking to the name string and its null char */
        LdrEntry = ExAllocatePoolWithTag(NonPagedPool, FileName->Length + sizeof(UNICODE_NULL) + sizeof(*LdrEntry), 'bDmM');
        if (LdrEntry)
        {
            memset(LdrEntry, 0, FileName->Length + sizeof(UNICODE_NULL) + sizeof(*LdrEntry));

            _SEH2_TRY
            {
                /* Get image checksum and size */
                NtHeaders = RtlImageNtHeader(BaseAddress);
                if (NtHeaders)
                {
                    LdrEntry->SizeOfImage = NtHeaders->OptionalHeader.SizeOfImage;
                    LdrEntry->CheckSum = NtHeaders->OptionalHeader.CheckSum;
                }
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                ExFreePoolWithTag(LdrEntry, 'bDmM');
                _SEH2_YIELD(return);
            }
            _SEH2_END;

            /* Fill all the details */
            LdrEntry->DllBase = BaseAddress;
            LdrEntry->FullDllName.Buffer = (PVOID)((ULONG_PTR)LdrEntry + sizeof(*LdrEntry));
            LdrEntry->FullDllName.Length = FileName->Length;
            LdrEntry->FullDllName.MaximumLength = FileName->Length + sizeof(UNICODE_NULL);
            memcpy(LdrEntry->FullDllName.Buffer, FileName->Buffer, FileName->Length);
            LdrEntry->FullDllName.Buffer[LdrEntry->FullDllName.Length / sizeof(WCHAR)] = UNICODE_NULL;
            LdrEntry->LoadCount = 1;

            /* Insert! */
            InsertHeadList(&MmLoadedUserImageList, &LdrEntry->InLoadOrderLinks);
        }
    }

    /* Release locks */
    ExReleaseResourceLite(&PsLoadedModuleResource);
    KeLeaveCriticalRegion();

    /* Load symbols */
    Status = RtlUnicodeStringToAnsiString(&FileNameA, FileName, TRUE);
    if (NT_SUCCESS(Status))
    {
        DbgLoadImageSymbols(&FileNameA, BaseAddress, (ULONG_PTR)Process->UniqueProcessId);
        RtlFreeAnsiString(&FileNameA);
    }
}

NTSTATUS
NTAPI
MiMapViewOfDataSection(IN PCONTROL_AREA ControlArea,
                       IN PEPROCESS Process,
                       IN PVOID *BaseAddress,
                       IN PLARGE_INTEGER SectionOffset,
                       IN PSIZE_T ViewSize,
                       IN PSECTION Section,
                       IN SECTION_INHERIT InheritDisposition,
                       IN ULONG ProtectionMask,
                       IN SIZE_T CommitSize,
                       IN ULONG_PTR ZeroBits,
                       IN ULONG AllocationType)
{
    PMMVAD_LONG Vad;
    ULONG_PTR StartAddress;
    ULONG_PTR ViewSizeInPages;
    PSUBSECTION Subsection;
    PSEGMENT Segment;
    PFN_NUMBER PteOffset;
    NTSTATUS Status;
    ULONG QuotaCharge = 0, QuotaExcess = 0;
    PMMPTE PointerPte, LastPte;
    MMPTE TempPte;
    DPRINT("Mapping ARM3 data section\n");

    /* Get the segment for this section */
    Segment = ControlArea->Segment;

    /* One can only reserve a file-based mapping, not shared memory! */
    if ((AllocationType & MEM_RESERVE) && !(ControlArea->FilePointer))
    {
        return STATUS_INVALID_PARAMETER_9;
    }

    /* This flag determines alignment, but ARM3 does not yet support it */
    ASSERT((AllocationType & MEM_DOS_LIM) == 0);

    /* First, increase the map count. No purging is supported yet */
    Status = MiCheckPurgeAndUpMapCount(ControlArea, FALSE);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check if the caller specified the view size */
    if (!(*ViewSize))
    {
        /* The caller did not, so pick a 64K aligned view size based on the offset */
        SectionOffset->LowPart &= ~(_64K - 1);
        *ViewSize = (SIZE_T)(Section->SizeOfSection.QuadPart - SectionOffset->QuadPart);
    }
    else
    {
        /* A size was specified, align it to a 64K boundary */
        *ViewSize += SectionOffset->LowPart & (_64K - 1);

        /* Align the offset as well to make this an aligned map */
        SectionOffset->LowPart &= ~((ULONG)_64K - 1);
    }

    /* We must be dealing with a 64KB aligned offset. This is a Windows ASSERT */
    ASSERT((SectionOffset->LowPart & ((ULONG)_64K - 1)) == 0);

    /* It's illegal to try to map more than overflows a LONG_PTR */
    if (*ViewSize >= MAXLONG_PTR)
    {
        MiDereferenceControlArea(ControlArea);
        return STATUS_INVALID_VIEW_SIZE;
    }

    /* Windows ASSERTs for this flag */
    ASSERT(ControlArea->u.Flags.GlobalOnlyPerSession == 0);

    /* Get the subsection. We don't support LARGE_CONTROL_AREA in ARM3 */
    ASSERT(ControlArea->u.Flags.Rom == 0);
    Subsection = (PSUBSECTION)(ControlArea + 1);

    /* Sections with extended segments are not supported in ARM3 */
    ASSERT(Segment->SegmentFlags.TotalNumberOfPtes4132 == 0);

    /* Within this section, figure out which PTEs will describe the view */
    PteOffset = (PFN_NUMBER)(SectionOffset->QuadPart >> PAGE_SHIFT);

    /* The offset must be in this segment's PTE chunk and it must be valid. Windows ASSERTs */
    ASSERT(PteOffset < Segment->TotalNumberOfPtes);
    ASSERT(((SectionOffset->QuadPart + *ViewSize + PAGE_SIZE - 1) >> PAGE_SHIFT) >= PteOffset);

    /* In ARM3, only one subsection is used for now. It must contain these PTEs */
    ASSERT(PteOffset < Subsection->PtesInSubsection);

    /* In ARM3, only page-file backed sections (shared memory) are supported now */
    ASSERT(ControlArea->FilePointer == NULL);

    /* Windows ASSERTs for this too -- there must be a subsection base address */
    ASSERT(Subsection->SubsectionBase != NULL);

    /* Compute how much commit space the segment will take */
    if ((CommitSize) && (Segment->NumberOfCommittedPages < Segment->TotalNumberOfPtes))
    {
        /* Charge for the maximum pages */
        QuotaCharge = BYTES_TO_PAGES(CommitSize);
    }

    /* ARM3 does not currently support large pages */
    ASSERT(Segment->SegmentFlags.LargePages == 0);

    /* Calculate how many pages the region spans */
    ViewSizeInPages = BYTES_TO_PAGES(*ViewSize);

    /* A VAD can now be allocated. Do so and zero it out */
    /* FIXME: we are allocating a LONG VAD for ReactOS compatibility only */
    ASSERT((AllocationType & MEM_RESERVE) == 0); /* ARM3 does not support this */
    Vad = ExAllocatePoolWithTag(NonPagedPool, sizeof(MMVAD_LONG), 'ldaV');
    if (!Vad)
    {
        MiDereferenceControlArea(ControlArea);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Vad, sizeof(MMVAD_LONG));
    Vad->u4.Banked = (PVOID)0xDEADBABE;

    /* Write all the data required in the VAD for handling a fault */
    Vad->ControlArea = ControlArea;
    Vad->u.VadFlags.CommitCharge = 0;
    Vad->u.VadFlags.Protection = ProtectionMask;
    Vad->u2.VadFlags2.FileOffset = (ULONG)(SectionOffset->QuadPart >> 16);
    Vad->u2.VadFlags2.Inherit = (InheritDisposition == ViewShare);
    if ((AllocationType & SEC_NO_CHANGE) || (Section->u.Flags.NoChange))
    {
        /* This isn't really implemented yet, but handle setting the flag */
        Vad->u.VadFlags.NoChange = 1;
        Vad->u2.VadFlags2.SecNoChange = 1;
    }

    /* Finally, write down the first and last prototype PTE */
    Vad->FirstPrototypePte = &Subsection->SubsectionBase[PteOffset];
    PteOffset += ViewSizeInPages - 1;
    ASSERT(PteOffset < Subsection->PtesInSubsection);
    Vad->LastContiguousPte = &Subsection->SubsectionBase[PteOffset];

    /* Make sure the prototype PTE ranges make sense, this is a Windows ASSERT */
    ASSERT(Vad->FirstPrototypePte <= Vad->LastContiguousPte);

    /* FIXME: Should setup VAD bitmap */
    Status = STATUS_SUCCESS;

    /* Check if anything was committed */
    if (QuotaCharge)
    {
        /* Set the start and end PTE addresses, and pick the template PTE */
        PointerPte = Vad->FirstPrototypePte;
        LastPte = PointerPte + BYTES_TO_PAGES(CommitSize);
        TempPte = Segment->SegmentPteTemplate;

        /* Acquire the commit lock and loop all prototype PTEs to be committed */
        KeAcquireGuardedMutex(&MmSectionCommitMutex);
        while (PointerPte < LastPte)
        {
            /* Make sure the PTE is already invalid */
            if (PointerPte->u.Long == 0)
            {
                /* And write the invalid PTE */
                MI_WRITE_INVALID_PTE(PointerPte, TempPte);
            }
            else
            {
                /* The PTE is valid, so skip it */
                QuotaExcess++;
            }

            /* Move to the next PTE */
            PointerPte++;
        }

        /* Now check how many pages exactly we committed, and update accounting */
        ASSERT(QuotaCharge >= QuotaExcess);
        QuotaCharge -= QuotaExcess;
        Segment->NumberOfCommittedPages += QuotaCharge;
        ASSERT(Segment->NumberOfCommittedPages <= Segment->TotalNumberOfPtes);

        /* Now that we're done, release the lock */
        KeReleaseGuardedMutex(&MmSectionCommitMutex);
    }

    /* Is it SEC_BASED, or did the caller manually specify an address? */
    if (*BaseAddress != NULL)
    {
        /* Just align what the caller gave us */
        StartAddress = ROUND_UP((ULONG_PTR)*BaseAddress, _64K);
    }
    else if (Section->Address.StartingVpn != 0)
    {
        /* It is a SEC_BASED mapping, use the address that was generated */
        StartAddress = Section->Address.StartingVpn + SectionOffset->LowPart;
    }
    else
    {
        StartAddress = 0;
    }

    /* Insert the VAD */
    Status = MiInsertVadEx((PMMVAD)Vad,
                           &StartAddress,
                           ViewSizeInPages * PAGE_SIZE,
                           MAXULONG_PTR >> ZeroBits,
                           MM_VIRTMEM_GRANULARITY,
                           AllocationType);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Windows stores this for accounting purposes, do so as well */
    if (!Segment->u2.FirstMappedVa) Segment->u2.FirstMappedVa = (PVOID)StartAddress;

    /* Finally, let the caller know where, and for what size, the view was mapped */
    *ViewSize = ViewSizeInPages * PAGE_SIZE;
    *BaseAddress = (PVOID)StartAddress;
    DPRINT("Start and region: 0x%p, 0x%p\n", *BaseAddress, *ViewSize);
    return STATUS_SUCCESS;
}

VOID
NTAPI
MiSubsectionConsistent(IN PSUBSECTION Subsection)
{
    /* ReactOS only supports systems with 4K pages and 4K sectors */
    ASSERT(Subsection->u.SubsectionFlags.SectorEndOffset == 0);

    /* Therefore, then number of PTEs should be equal to the number of sectors */
    if (Subsection->NumberOfFullSectors != Subsection->PtesInSubsection)
    {
        /* Break and warn if this is inconsistent */
        DPRINT1("Mm: Subsection inconsistent (%x vs %x)\n",
                Subsection->NumberOfFullSectors, Subsection->PtesInSubsection);
        DbgBreakPoint();
    }
}

NTSTATUS
NTAPI
MiCreateDataFileMap(IN PFILE_OBJECT File,
                    OUT PSEGMENT *Segment,
                    IN PSIZE_T MaximumSize,
                    IN ULONG SectionPageProtection,
                    IN ULONG AllocationAttributes,
                    IN ULONG IgnoreFileSizing)
{
    /* Not yet implemented */
    ASSERT(FALSE);
    *Segment = NULL;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
MiCreatePagingFileMap(OUT PSEGMENT *Segment,
                      IN PSIZE_T MaximumSize,
                      IN ULONG ProtectionMask,
                      IN ULONG AllocationAttributes)
{
    SIZE_T SizeLimit;
    PFN_COUNT PteCount;
    PMMPTE PointerPte;
    MMPTE TempPte;
    PCONTROL_AREA ControlArea;
    PSEGMENT NewSegment;
    PSUBSECTION Subsection;
    PAGED_CODE();

    /* No large pages in ARM3 yet */
    ASSERT((AllocationAttributes & SEC_LARGE_PAGES) == 0);

    /* Pagefile-backed sections need a known size */
    if (!(*MaximumSize)) return STATUS_INVALID_PARAMETER_4;

    /* Calculate the maximum size possible, given the Prototype PTEs we'll need */
    SizeLimit = MAXULONG_PTR - sizeof(SEGMENT);
    SizeLimit /= sizeof(MMPTE);
    SizeLimit <<= PAGE_SHIFT;

    /* Fail if this size is too big */
    if (*MaximumSize > SizeLimit) return STATUS_SECTION_TOO_BIG;

    /* Calculate how many Prototype PTEs will be needed */
    PteCount = (PFN_COUNT)((*MaximumSize + PAGE_SIZE - 1) >> PAGE_SHIFT);

    /* For commited memory, we must have a valid protection mask */
    if (AllocationAttributes & SEC_COMMIT) ASSERT(ProtectionMask != 0);

    /* The segment contains all the Prototype PTEs, allocate it in paged pool */
    NewSegment = ExAllocatePoolWithTag(PagedPool,
                                       sizeof(SEGMENT) +
                                       sizeof(MMPTE) * (PteCount - 1),
                                       'tSmM');
    ASSERT(NewSegment);
    *Segment = NewSegment;

    /* Now allocate the control area, which has the subsection structure */
    ControlArea = ExAllocatePoolWithTag(NonPagedPool,
                                        sizeof(CONTROL_AREA) + sizeof(SUBSECTION),
                                        'tCmM');
    ASSERT(ControlArea);

    /* And zero it out, filling the basic segmnet pointer and reference fields */
    RtlZeroMemory(ControlArea, sizeof(CONTROL_AREA) + sizeof(SUBSECTION));
    ControlArea->Segment = NewSegment;
    ControlArea->NumberOfSectionReferences = 1;
    ControlArea->NumberOfUserReferences = 1;

    /* Convert allocation attributes to control area flags */
    if (AllocationAttributes & SEC_BASED) ControlArea->u.Flags.Based = 1;
    if (AllocationAttributes & SEC_RESERVE) ControlArea->u.Flags.Reserve = 1;
    if (AllocationAttributes & SEC_COMMIT) ControlArea->u.Flags.Commit = 1;

    /* The subsection follows, write the mask, PTE count and point back to the CA */
    Subsection = (PSUBSECTION)(ControlArea + 1);
    Subsection->ControlArea = ControlArea;
    Subsection->PtesInSubsection = PteCount;
    Subsection->u.SubsectionFlags.Protection = ProtectionMask;

    /* Zero out the segment's prototype PTEs, and link it with the control area */
    PointerPte = &NewSegment->ThePtes[0];
    RtlZeroMemory(NewSegment, sizeof(SEGMENT));
    NewSegment->PrototypePte = PointerPte;
    NewSegment->ControlArea = ControlArea;

    /* Save some extra accounting data for the segment as well */
    NewSegment->u1.CreatingProcess = PsGetCurrentProcess();
    NewSegment->SizeOfSegment = PteCount * PAGE_SIZE;
    NewSegment->TotalNumberOfPtes = PteCount;
    NewSegment->NonExtendedPtes = PteCount;

    /* The subsection's base address is the first Prototype PTE in the segment */
    Subsection->SubsectionBase = PointerPte;

    /* Start with an empty PTE, unless this is a commit operation */
    TempPte.u.Long = 0;
    if (AllocationAttributes & SEC_COMMIT)
    {
        /* In which case, write down the protection mask in the Prototype PTEs */
        TempPte.u.Soft.Protection = ProtectionMask;

        /* For accounting, also mark these pages as being committed */
        NewSegment->NumberOfCommittedPages = PteCount;
    }

    /* The template PTE itself for the segment should also have the mask set */
    NewSegment->SegmentPteTemplate.u.Soft.Protection = ProtectionMask;

    /* Write out the prototype PTEs, for now they're simply demand zero */
#ifdef _WIN64
    RtlFillMemoryUlonglong(PointerPte, PteCount * sizeof(MMPTE), TempPte.u.Long);
#else
    RtlFillMemoryUlong(PointerPte, PteCount * sizeof(MMPTE), TempPte.u.Long);
#endif
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MiGetFileObjectForSectionAddress(
    IN PVOID Address,
    OUT PFILE_OBJECT *FileObject)
{
    PMMVAD Vad;
    PCONTROL_AREA ControlArea;

    /* Get the VAD */
    Vad = MiLocateAddress(Address);
    if (Vad == NULL)
    {
        /* Fail, the address does not exist */
        DPRINT1("Invalid address\n");
        return STATUS_INVALID_ADDRESS;
    }

    /* Check if this is a RosMm memory area */
    if (Vad->u.VadFlags.Spare != 0)
    {
        PMEMORY_AREA MemoryArea = (PMEMORY_AREA)Vad;
        PROS_SECTION_OBJECT Section;

        /* Check if it's a section view (RosMm section) */
        if (MemoryArea->Type == MEMORY_AREA_SECTION_VIEW)
        {
            /* Get the section pointer to the SECTION_OBJECT */
            Section = MemoryArea->Data.SectionData.Section;
            *FileObject = Section->FileObject;
        }
        else
        {
            ASSERT(MemoryArea->Type == MEMORY_AREA_CACHE);
            DPRINT1("Address is a cache section!\n");
            return STATUS_SECTION_NOT_IMAGE;
        }
    }
    else
    {
        /* Make sure it's not a VM VAD */
        if (Vad->u.VadFlags.PrivateMemory == 1)
        {
            DPRINT1("Address is not a section\n");
            return STATUS_SECTION_NOT_IMAGE;
        }

        /* Get the control area */
        ControlArea = Vad->ControlArea;
        if (!(ControlArea) || !(ControlArea->u.Flags.Image))
        {
            DPRINT1("Address is not a section\n");
            return STATUS_SECTION_NOT_IMAGE;
        }

        /* Get the file object */
        *FileObject = ControlArea->FilePointer;
    }

    /* Return success */
    return STATUS_SUCCESS;
}

PFILE_OBJECT
NTAPI
MmGetFileObjectForSection(IN PVOID SectionObject)
{
    PSECTION_OBJECT Section;
    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
    ASSERT(SectionObject != NULL);

    /* Check if it's an ARM3, or ReactOS section */
    if (MiIsRosSectionObject(SectionObject) == FALSE)
    {
        /* Return the file pointer stored in the control area */
        Section = SectionObject;
        return Section->Segment->ControlArea->FilePointer;
    }

    /* Return the file object */
    return ((PROS_SECTION_OBJECT)SectionObject)->FileObject;
}

VOID
NTAPI
MmGetImageInformation (OUT PSECTION_IMAGE_INFORMATION ImageInformation)
{
    PSECTION_OBJECT SectionObject;

    /* Get the section object of this process*/
    SectionObject = PsGetCurrentProcess()->SectionObject;
    ASSERT(SectionObject != NULL);
    ASSERT(MiIsRosSectionObject(SectionObject) == TRUE);

    /* Return the image information */
    *ImageInformation = ((PROS_SECTION_OBJECT)SectionObject)->ImageSection->ImageInformation;
}

NTSTATUS
NTAPI
MmGetFileNameForFileObject(IN PFILE_OBJECT FileObject,
                           OUT POBJECT_NAME_INFORMATION *ModuleName)
{
    POBJECT_NAME_INFORMATION ObjectNameInfo;
    NTSTATUS Status;
    ULONG ReturnLength;

    /* Allocate memory for our structure */
    ObjectNameInfo = ExAllocatePoolWithTag(PagedPool, 1024, TAG_MM);
    if (!ObjectNameInfo) return STATUS_NO_MEMORY;

    /* Query the name */
    Status = ObQueryNameString(FileObject,
                               ObjectNameInfo,
                               1024,
                               &ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        /* Failed, free memory */
        DPRINT1("Name query failed\n");
        ExFreePoolWithTag(ObjectNameInfo, TAG_MM);
        *ModuleName = NULL;
        return Status;
    }

    /* Success */
    *ModuleName = ObjectNameInfo;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MmGetFileNameForSection(IN PVOID Section,
                        OUT POBJECT_NAME_INFORMATION *ModuleName)
{
    PFILE_OBJECT FileObject;

    /* Make sure it's an image section */
    if (MiIsRosSectionObject(Section) == FALSE)
    {
        /* Check ARM3 Section flag */
        if (((PSECTION)Section)->u.Flags.Image == 0)
        {
            /* It's not, fail */
            DPRINT1("Not an image section\n");
            return STATUS_SECTION_NOT_IMAGE;
        }
    }
    else if (!(((PROS_SECTION_OBJECT)Section)->AllocationAttributes & SEC_IMAGE))
    {
        /* It's not, fail */
        DPRINT1("Not an image section\n");
        return STATUS_SECTION_NOT_IMAGE;
    }

    /* Get the file object */
    FileObject = MmGetFileObjectForSection(Section);
    return MmGetFileNameForFileObject(FileObject, ModuleName);
}

NTSTATUS
NTAPI
MmGetFileNameForAddress(IN PVOID Address,
                        OUT PUNICODE_STRING ModuleName)
{
    POBJECT_NAME_INFORMATION ModuleNameInformation;
    PVOID AddressSpace;
    NTSTATUS Status;
    PFILE_OBJECT FileObject = NULL;

    /* Lock address space */
    AddressSpace = MmGetCurrentAddressSpace();
    MmLockAddressSpace(AddressSpace);

    /* Get the file object pointer for the address */
    Status = MiGetFileObjectForSectionAddress(Address, &FileObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get file object for Address %p\n", Address);
        MmUnlockAddressSpace(AddressSpace);
        return Status;
    }

    /* Reference the file object */
    ObReferenceObject(FileObject);

    /* Unlock address space */
    MmUnlockAddressSpace(AddressSpace);

    /* Get the filename of the file object */
    Status = MmGetFileNameForFileObject(FileObject, &ModuleNameInformation);

    /* Dereference the file object */
    ObDereferenceObject(FileObject);

    /* Check if we were able to get the file object name */
    if (NT_SUCCESS(Status))
    {
        /* Init modulename */
        RtlCreateUnicodeString(ModuleName, ModuleNameInformation->Name.Buffer);

        /* Free temp taged buffer from MmGetFileNameForFileObject() */
        ExFreePoolWithTag(ModuleNameInformation, TAG_MM);
        DPRINT("Found ModuleName %S by address %p\n", ModuleName->Buffer, Address);
    }

   /* Return status */
   return Status;
}

NTSTATUS
NTAPI
MiQueryMemorySectionName(IN HANDLE ProcessHandle,
                         IN PVOID BaseAddress,
                         OUT PVOID MemoryInformation,
                         IN SIZE_T MemoryInformationLength,
                         OUT PSIZE_T ReturnLength)
{
    PEPROCESS Process;
    NTSTATUS Status;
    WCHAR ModuleFileNameBuffer[MAX_PATH] = {0};
    UNICODE_STRING ModuleFileName;
    PMEMORY_SECTION_NAME SectionName = NULL;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();

    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_QUERY_INFORMATION,
                                       NULL,
                                       PreviousMode,
                                       (PVOID*)(&Process),
                                       NULL);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("MiQueryMemorySectionName: ObReferenceObjectByHandle returned %x\n",Status);
        return Status;
    }

    RtlInitEmptyUnicodeString(&ModuleFileName, ModuleFileNameBuffer, sizeof(ModuleFileNameBuffer));
    Status = MmGetFileNameForAddress(BaseAddress, &ModuleFileName);

    if (NT_SUCCESS(Status))
    {
        SectionName = MemoryInformation;
        if (PreviousMode != KernelMode)
        {
            _SEH2_TRY
            {
                RtlInitUnicodeString(&SectionName->SectionFileName, SectionName->NameBuffer);
                SectionName->SectionFileName.MaximumLength = (USHORT)MemoryInformationLength;
                RtlCopyUnicodeString(&SectionName->SectionFileName, &ModuleFileName);

                if (ReturnLength) *ReturnLength = ModuleFileName.Length;

            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
        }
        else
        {
            RtlInitUnicodeString(&SectionName->SectionFileName, SectionName->NameBuffer);
            SectionName->SectionFileName.MaximumLength = (USHORT)MemoryInformationLength;
            RtlCopyUnicodeString(&SectionName->SectionFileName, &ModuleFileName);

            if (ReturnLength) *ReturnLength = ModuleFileName.Length;

        }
    }
    ObDereferenceObject(Process);
    return Status;
}

VOID
NTAPI
MiFlushTbAndCapture(IN PMMVAD FoundVad,
                    IN PMMPTE PointerPte,
                    IN ULONG ProtectionMask,
                    IN PMMPFN Pfn1,
                    IN BOOLEAN CaptureDirtyBit)
{
    MMPTE TempPte, PreviousPte;
    KIRQL OldIrql;
    BOOLEAN RebuildPte = FALSE;

    //
    // User for sanity checking later on
    //
    PreviousPte = *PointerPte;

    //
    // Build the PTE and acquire the PFN lock
    //
    MI_MAKE_HARDWARE_PTE_USER(&TempPte,
                              PointerPte,
                              ProtectionMask,
                              PreviousPte.u.Hard.PageFrameNumber);
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

    //
    // We don't support I/O mappings in this path yet
    //
    ASSERT(Pfn1 != NULL);
    ASSERT(Pfn1->u3.e1.CacheAttribute != MiWriteCombined);

    //
    // Make sure new protection mask doesn't get in conflict and fix it if it does
    //
    if (Pfn1->u3.e1.CacheAttribute == MiCached)
    {
        //
        // This is a cached PFN
        //
        if (ProtectionMask & (MM_NOCACHE | MM_NOACCESS))
        {
            RebuildPte = TRUE;
            ProtectionMask &= ~(MM_NOCACHE | MM_NOACCESS);
        }
    }
    else if (Pfn1->u3.e1.CacheAttribute == MiNonCached)
    {
        //
        // This is a non-cached PFN
        //
        if ((ProtectionMask & (MM_NOCACHE | MM_NOACCESS)) != MM_NOCACHE)
        {
            RebuildPte = TRUE;
            ProtectionMask &= ~MM_NOACCESS;
            ProtectionMask |= MM_NOCACHE;
        }
    }

    if (RebuildPte)
    {
        MI_MAKE_HARDWARE_PTE_USER(&TempPte,
                                  PointerPte,
                                  ProtectionMask,
                                  PreviousPte.u.Hard.PageFrameNumber);
    }

    //
    // Write the new PTE, making sure we are only changing the bits
    //
    MI_UPDATE_VALID_PTE(PointerPte, TempPte);

    //
    // Flush the TLB
    //
    ASSERT(PreviousPte.u.Hard.Valid == 1);
    KeFlushCurrentTb();
    ASSERT(PreviousPte.u.Hard.Valid == 1);

    //
    // Windows updates the relevant PFN1 information, we currently don't.
    //
    if (CaptureDirtyBit) DPRINT1("Warning, not handling dirty bit\n");

    //
    // Not supported in ARM3
    //
    ASSERT(FoundVad->u.VadFlags.VadType != VadWriteWatch);

    //
    // Release the PFN lock, we are done
    //
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
}

//
// NOTE: This function gets a lot more complicated if we want Copy-on-Write support
//
NTSTATUS
NTAPI
MiSetProtectionOnSection(IN PEPROCESS Process,
                         IN PMMVAD FoundVad,
                         IN PVOID StartingAddress,
                         IN PVOID EndingAddress,
                         IN ULONG NewProtect,
                         OUT PULONG CapturedOldProtect,
                         IN ULONG DontCharge,
                         OUT PULONG Locked)
{
    PMMPTE PointerPte, LastPte;
    MMPTE TempPte, PteContents;
    PMMPDE PointerPde;
    PMMPFN Pfn1;
    ULONG ProtectionMask, QuotaCharge = 0;
    PETHREAD Thread = PsGetCurrentThread();
    PAGED_CODE();

    //
    // Tell caller nothing is being locked
    //
    *Locked = FALSE;

    //
    // This function should only be used for section VADs. Windows ASSERT */
    //
    ASSERT(FoundVad->u.VadFlags.PrivateMemory == 0);

    //
    // We don't support these features in ARM3
    //
    ASSERT(FoundVad->u.VadFlags.VadType != VadImageMap);
    ASSERT(FoundVad->u2.VadFlags2.CopyOnWrite == 0);

    //
    // Convert and validate the protection mask
    //
    ProtectionMask = MiMakeProtectionMask(NewProtect);
    if (ProtectionMask == MM_INVALID_PROTECTION)
    {
        DPRINT1("Invalid section protect\n");
        return STATUS_INVALID_PAGE_PROTECTION;
    }

    //
    // Get the PTE and PDE for the address, as well as the final PTE
    //
    MiLockProcessWorkingSetUnsafe(Process, Thread);
    PointerPde = MiAddressToPde(StartingAddress);
    PointerPte = MiAddressToPte(StartingAddress);
    LastPte = MiAddressToPte(EndingAddress);

    //
    // Make the PDE valid, and check the status of the first PTE
    //
    MiMakePdeExistAndMakeValid(PointerPde, Process, MM_NOIRQL);
    if (PointerPte->u.Long)
    {
        //
        // Not supported in ARM3
        //
        ASSERT(FoundVad->u.VadFlags.VadType != VadRotatePhysical);

        //
        // Capture the page protection and make the PDE valid
        //
        *CapturedOldProtect = MiGetPageProtection(PointerPte);
        MiMakePdeExistAndMakeValid(PointerPde, Process, MM_NOIRQL);
    }
    else
    {
        //
        // Only pagefile-backed section VADs are supported for now
        //
        ASSERT(FoundVad->u.VadFlags.VadType != VadImageMap);

        //
        // Grab the old protection from the VAD itself
        //
        *CapturedOldProtect = MmProtectToValue[FoundVad->u.VadFlags.Protection];
    }

    //
    // Loop all the PTEs now
    //
    MiMakePdeExistAndMakeValid(PointerPde, Process, MM_NOIRQL);
    while (PointerPte <= LastPte)
    {
        //
        // Check if we've crossed a PDE boundary and make the new PDE valid too
        //
        if ((((ULONG_PTR)PointerPte) & (SYSTEM_PD_SIZE - 1)) == 0)
        {
            PointerPde = MiPteToPde(PointerPte);
            MiMakePdeExistAndMakeValid(PointerPde, Process, MM_NOIRQL);
        }

        //
        // Capture the PTE and see what we're dealing with
        //
        PteContents = *PointerPte;
        if (PteContents.u.Long == 0)
        {
            //
            // This used to be a zero PTE and it no longer is, so we must add a
            // reference to the pagetable.
            //
            MiIncrementPageTableReferences(MiPteToAddress(PointerPte));

            //
            // Create the demand-zero prototype PTE
            //
            TempPte = PrototypePte;
            TempPte.u.Soft.Protection = ProtectionMask;
            MI_WRITE_INVALID_PTE(PointerPte, TempPte);
        }
        else if (PteContents.u.Hard.Valid == 1)
        {
            //
            // Get the PFN entry
            //
            Pfn1 = MiGetPfnEntry(PFN_FROM_PTE(&PteContents));

            //
            // We don't support these yet
            //
            ASSERT((NewProtect & (PAGE_NOACCESS | PAGE_GUARD)) == 0);
            ASSERT(Pfn1->u3.e1.PrototypePte == 0);

            //
            // Write the protection mask and write it with a TLB flush
            //
            Pfn1->OriginalPte.u.Soft.Protection = ProtectionMask;
            MiFlushTbAndCapture(FoundVad,
                                PointerPte,
                                ProtectionMask,
                                Pfn1,
                                TRUE);
        }
        else
        {
            //
            // We don't support these cases yet
            //
            ASSERT(PteContents.u.Soft.Prototype == 0);
            ASSERT(PteContents.u.Soft.Transition == 0);

            //
            // The PTE is already demand-zero, just update the protection mask
            //
            PointerPte->u.Soft.Protection = ProtectionMask;
        }

        PointerPte++;
    }

    //
    // Unlock the working set and update quota charges if needed, then return
    //
    MiUnlockProcessWorkingSetUnsafe(Process, Thread);
    if ((QuotaCharge > 0) && (!DontCharge))
    {
        FoundVad->u.VadFlags.CommitCharge -= QuotaCharge;
        Process->CommitCharge -= QuotaCharge;
    }
    return STATUS_SUCCESS;
}

VOID
NTAPI
MiRemoveMappedPtes(IN PVOID BaseAddress,
                   IN ULONG NumberOfPtes,
                   IN PCONTROL_AREA ControlArea,
                   IN PMMSUPPORT Ws)
{
    PMMPTE PointerPte, ProtoPte;//, FirstPte;
    PMMPDE PointerPde, SystemMapPde;
    PMMPFN Pfn1, Pfn2;
    MMPTE PteContents;
    KIRQL OldIrql;
    DPRINT("Removing mapped view at: 0x%p\n", BaseAddress);

    ASSERT(Ws == NULL);

    /* Get the PTE and loop each one */
    PointerPte = MiAddressToPte(BaseAddress);
    //FirstPte = PointerPte;
    while (NumberOfPtes)
    {
        /* Check if the PTE is already valid */
        PteContents = *PointerPte;
        if (PteContents.u.Hard.Valid == 1)
        {
            /* Get the PFN entry */
            Pfn1 = MiGetPfnEntry(PFN_FROM_PTE(&PteContents));

            /* Get the PTE */
            PointerPde = MiPteToPde(PointerPte);

            /* Lock the PFN database and make sure this isn't a mapped file */
            OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
            ASSERT(((Pfn1->u3.e1.PrototypePte) && (Pfn1->OriginalPte.u.Soft.Prototype)) == 0);

            /* Mark the page as modified accordingly */
            if (MI_IS_PAGE_DIRTY(&PteContents))
                Pfn1->u3.e1.Modified = 1;

            /* Was the PDE invalid */
            if (PointerPde->u.Long == 0)
            {
#if (_MI_PAGING_LEVELS == 2)
                /* Find the system double-mapped PDE that describes this mapping */
                SystemMapPde = &MmSystemPagePtes[((ULONG_PTR)PointerPde & (SYSTEM_PD_SIZE - 1)) / sizeof(MMPTE)];

                /* Make it valid */
                ASSERT(SystemMapPde->u.Hard.Valid == 1);
                MI_WRITE_VALID_PDE(PointerPde, *SystemMapPde);
#else
                DBG_UNREFERENCED_LOCAL_VARIABLE(SystemMapPde);
                ASSERT(FALSE);
#endif
            }

            /* Dereference the PDE and the PTE */
            Pfn2 = MiGetPfnEntry(PFN_FROM_PTE(PointerPde));
            MiDecrementShareCount(Pfn2, PFN_FROM_PTE(PointerPde));
            DBG_UNREFERENCED_LOCAL_VARIABLE(Pfn2);
            MiDecrementShareCount(Pfn1, PFN_FROM_PTE(&PteContents));

            /* Release the PFN lock */
            KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
        }
        else
        {
            /* Windows ASSERT */
            ASSERT((PteContents.u.Long == 0) || (PteContents.u.Soft.Prototype == 1));

            /* Check if this is a prototype pointer PTE */
            if (PteContents.u.Soft.Prototype == 1)
            {
                /* Get the prototype PTE */
                ProtoPte = MiProtoPteToPte(&PteContents);

                /* We don't support anything else atm */
                ASSERT(ProtoPte->u.Long == 0);
            }
        }

        /* Make the PTE into a zero PTE */
        PointerPte->u.Long = 0;

        /* Move to the next PTE */
        PointerPte++;
        NumberOfPtes--;
    }

    /* Flush the TLB */
    KeFlushCurrentTb();

    /* Acquire the PFN lock */
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

    /* Decrement the accounting counters */
    ControlArea->NumberOfUserReferences--;
    ControlArea->NumberOfMappedViews--;

    /* Check if we should destroy the CA and release the lock */
    MiCheckControlArea(ControlArea, OldIrql);
}

ULONG
NTAPI
MiRemoveFromSystemSpace(IN PMMSESSION Session,
                        IN PVOID Base,
                        OUT PCONTROL_AREA *ControlArea)
{
    ULONG Hash, Size, Count = 0;
    ULONG_PTR Entry;
    PAGED_CODE();

    /* Compute the hash for this entry and loop trying to find it */
    Entry = (ULONG_PTR)Base >> 16;
    Hash = Entry % Session->SystemSpaceHashKey;
    while ((Session->SystemSpaceViewTable[Hash].Entry >> 16) != Entry)
    {
        /* Check if we overflew past the end of the hash table */
        if (++Hash >= Session->SystemSpaceHashSize)
        {
            /* Reset the hash to zero and keep searching from the bottom */
            Hash = 0;
            if (++Count == 2)
            {
                /* But if we overflew twice, then this is not a real mapping */
                KeBugCheckEx(DRIVER_UNMAPPING_INVALID_VIEW,
                             (ULONG_PTR)Base,
                             1,
                             0,
                             0);
            }
        }
    }

    /* One less entry */
    Session->SystemSpaceHashEntries--;

    /* Extract the size and clear the entry */
    Size = Session->SystemSpaceViewTable[Hash].Entry & 0xFFFF;
    Session->SystemSpaceViewTable[Hash].Entry = 0;

    /* Return the control area and the size */
    *ControlArea = Session->SystemSpaceViewTable[Hash].ControlArea;
    return Size;
}

NTSTATUS
NTAPI
MiUnmapViewInSystemSpace(IN PMMSESSION Session,
                         IN PVOID MappedBase)
{
    ULONG Size;
    PCONTROL_AREA ControlArea;
    PAGED_CODE();

    /* Remove this mapping */
    KeAcquireGuardedMutex(Session->SystemSpaceViewLockPointer);
    Size = MiRemoveFromSystemSpace(Session, MappedBase, &ControlArea);

    /* Clear the bits for this mapping */
    RtlClearBits(Session->SystemSpaceBitMap,
                 (ULONG)(((ULONG_PTR)MappedBase - (ULONG_PTR)Session->SystemSpaceViewStart) >> 16),
                 Size);

    /* Convert the size from a bit size into the actual size */
    Size = Size * (_64K >> PAGE_SHIFT);

    /* Remove the PTEs now */
    MiRemoveMappedPtes(MappedBase, Size, ControlArea, NULL);
    KeReleaseGuardedMutex(Session->SystemSpaceViewLockPointer);

    /* Return success */
    return STATUS_SUCCESS;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
MmCreateArm3Section(OUT PVOID *SectionObject,
                    IN ACCESS_MASK DesiredAccess,
                    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
                    IN PLARGE_INTEGER InputMaximumSize,
                    IN ULONG SectionPageProtection,
                    IN ULONG AllocationAttributes,
                    IN HANDLE FileHandle OPTIONAL,
                    IN PFILE_OBJECT FileObject OPTIONAL)
{
    SECTION Section;
    PSECTION NewSection;
    PSUBSECTION Subsection;
    PSEGMENT NewSegment, Segment;
    NTSTATUS Status;
    PCONTROL_AREA ControlArea;
    ULONG ProtectionMask, ControlAreaSize, Size, NonPagedCharge, PagedCharge;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    BOOLEAN FileLock = FALSE, KernelCall = FALSE;
    KIRQL OldIrql;
    PFILE_OBJECT File;
    PVOID PreviousSectionPointer;

    /* Make the same sanity checks that the Nt interface should've validated */
    ASSERT((AllocationAttributes & ~(SEC_COMMIT | SEC_RESERVE | SEC_BASED |
                                     SEC_LARGE_PAGES | SEC_IMAGE | SEC_NOCACHE |
                                     SEC_NO_CHANGE)) == 0);
    ASSERT((AllocationAttributes & (SEC_COMMIT | SEC_RESERVE | SEC_IMAGE)) != 0);
    ASSERT(!((AllocationAttributes & SEC_IMAGE) &&
             (AllocationAttributes & (SEC_COMMIT | SEC_RESERVE |
                                      SEC_NOCACHE | SEC_NO_CHANGE))));
    ASSERT(!((AllocationAttributes & SEC_COMMIT) && (AllocationAttributes & SEC_RESERVE)));
    ASSERT(!((SectionPageProtection & PAGE_NOCACHE) ||
             (SectionPageProtection & PAGE_WRITECOMBINE) ||
             (SectionPageProtection & PAGE_GUARD) ||
             (SectionPageProtection & PAGE_NOACCESS)));

    /* Convert section flag to page flag */
    if (AllocationAttributes & SEC_NOCACHE) SectionPageProtection |= PAGE_NOCACHE;

    /* Check to make sure the protection is correct. Nt* does this already */
    ProtectionMask = MiMakeProtectionMask(SectionPageProtection);
    if (ProtectionMask == MM_INVALID_PROTECTION) return STATUS_INVALID_PAGE_PROTECTION;

    /* Check if this is going to be a data or image backed file section */
    if ((FileHandle) || (FileObject))
    {
        /* These cannot be mapped with large pages */
        if (AllocationAttributes & SEC_LARGE_PAGES) return STATUS_INVALID_PARAMETER_6;

        /* For now, only support the mechanism through a file handle */
        ASSERT(FileObject == NULL);

        /* Reference the file handle to get the object */
        Status = ObReferenceObjectByHandle(FileHandle,
                                           MmMakeFileAccess[ProtectionMask],
                                           IoFileObjectType,
                                           PreviousMode,
                                           (PVOID*)&File,
                                           NULL);
        if (!NT_SUCCESS(Status)) return Status;

        /* Make sure Cc has been doing its job */
        if (!File->SectionObjectPointer)
        {
            /* This is not a valid file system-based file, fail */
            ObDereferenceObject(File);
            return STATUS_INVALID_FILE_FOR_SECTION;
        }

        /* Image-file backed sections are not yet supported */
        ASSERT((AllocationAttributes & SEC_IMAGE) == 0);

        /* Compute the size of the control area, and allocate it */
        ControlAreaSize = sizeof(CONTROL_AREA) + sizeof(MSUBSECTION);
        ControlArea = ExAllocatePoolWithTag(NonPagedPool, ControlAreaSize, 'aCmM');
        if (!ControlArea)
        {
            ObDereferenceObject(File);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Zero it out */
        RtlZeroMemory(ControlArea, ControlAreaSize);

        /* Did we get a handle, or an object? */
        if (FileHandle)
        {
            /* We got a file handle so we have to lock down the file */
#if 0
            Status = FsRtlAcquireToCreateMappedSection(File, SectionPageProtection);
            if (!NT_SUCCESS(Status))
            {
                ExFreePool(ControlArea);
                ObDereferenceObject(File);
                return Status;
            }
#else
            /* ReactOS doesn't support this API yet, so do nothing */
            Status = STATUS_SUCCESS;
#endif
            /* Update the top-level IRP so that drivers know what's happening */
            IoSetTopLevelIrp((PIRP)FSRTL_FSP_TOP_LEVEL_IRP);
            FileLock = TRUE;
        }

        /* Lock the PFN database while we play with the section pointers */
        OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

        /* Image-file backed sections are not yet supported */
        ASSERT((AllocationAttributes & SEC_IMAGE) == 0);

        /* There should not already be a control area for this file */
        ASSERT(File->SectionObjectPointer->DataSectionObject == NULL);
        NewSegment = NULL;

        /* Write down that this CA is being created, and set it */
        ControlArea->u.Flags.BeingCreated = TRUE;
        PreviousSectionPointer = File->SectionObjectPointer;
        File->SectionObjectPointer->DataSectionObject = ControlArea;

        /* We can release the PFN lock now */
        KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);

        /* We don't support previously-mapped file */
        ASSERT(NewSegment == NULL);

        /* Image-file backed sections are not yet supported */
        ASSERT((AllocationAttributes & SEC_IMAGE) == 0);

        /* So we always create a data file map */
        Status = MiCreateDataFileMap(File,
                                     &Segment,
                                     (PSIZE_T)InputMaximumSize,
                                     SectionPageProtection,
                                     AllocationAttributes,
                                     KernelCall);
        ASSERT(PreviousSectionPointer == File->SectionObjectPointer);
        ASSERT(NT_SUCCESS(Status));

        /* Check if a maximum size was specified */
        if (!InputMaximumSize->QuadPart)
        {
            /* Nope, use the segment size */
            Section.SizeOfSection.QuadPart = (LONGLONG)Segment->SizeOfSegment;
        }
        else
        {
            /* Yep, use the entered size */
            Section.SizeOfSection.QuadPart = InputMaximumSize->QuadPart;
        }
    }
    else
    {
        /* A handle must be supplied with SEC_IMAGE, as this is the no-handle path */
        if (AllocationAttributes & SEC_IMAGE) return STATUS_INVALID_FILE_FOR_SECTION;

        /* Not yet supported */
        ASSERT((AllocationAttributes & SEC_LARGE_PAGES) == 0);

        /* So this must be a pagefile-backed section, create the mappings needed */
        Status = MiCreatePagingFileMap(&NewSegment,
                                       (PSIZE_T)InputMaximumSize,
                                       ProtectionMask,
                                       AllocationAttributes);
        if (!NT_SUCCESS(Status)) return Status;

        /* Set the size here, and read the control area */
        Section.SizeOfSection.QuadPart = NewSegment->SizeOfSegment;
        ControlArea = NewSegment->ControlArea;
    }

    /* Did we already have a segment? */
    if (!NewSegment)
    {
        /* This must be the file path and we created a segment */
        NewSegment = Segment;
        ASSERT(File != NULL);

        /* Acquire the PFN lock while we set control area flags */
        OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

        /* We don't support this race condition yet, so assume no waiters */
        ASSERT(ControlArea->WaitingForDeletion == NULL);
        ControlArea->WaitingForDeletion = NULL;

        /* Image-file backed sections are not yet supported, nor ROM images */
        ASSERT((AllocationAttributes & SEC_IMAGE) == 0);
        ASSERT(Segment->ControlArea->u.Flags.Rom == 0);

        /* Take off the being created flag, and then release the lock */
        ControlArea->u.Flags.BeingCreated = FALSE;
        KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
    }

    /* Check if we locked the file earlier */
    if (FileLock)
    {
        /* Reset the top-level IRP and release the lock */
        IoSetTopLevelIrp(NULL);
        //FsRtlReleaseFile(File);
        FileLock = FALSE;
    }

    /* Set the initial section object data */
    Section.InitialPageProtection = SectionPageProtection;

    /* The mapping created a control area and segment, save the flags */
    Section.Segment = NewSegment;
    Section.u.LongFlags = ControlArea->u.LongFlags;

    /* Check if this is a user-mode read-write non-image file mapping */
    if (!(FileObject) &&
        (SectionPageProtection & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE)) &&
        (ControlArea->u.Flags.Image == 0) &&
        (ControlArea->FilePointer != NULL))
    {
        /* Add a reference and set the flag */
        Section.u.Flags.UserWritable = 1;
        InterlockedIncrement((PLONG)&ControlArea->WritableUserReferences);
    }

    /* Check for image mappings or page file mappings */
    if ((ControlArea->u.Flags.Image == 1) || !(ControlArea->FilePointer))
    {
        /* Charge the segment size, and allocate a subsection */
        PagedCharge = sizeof(SECTION) + NewSegment->TotalNumberOfPtes * sizeof(MMPTE);
        Size = sizeof(SUBSECTION);
    }
    else
    {
        /* Charge nothing, and allocate a mapped subsection */
        PagedCharge = 0;
        Size = sizeof(MSUBSECTION);
    }

    /* Check if this is a normal CA */
    ASSERT(ControlArea->u.Flags.GlobalOnlyPerSession == 0);
    ASSERT(ControlArea->u.Flags.Rom == 0);

    /* Charge only a CA, and the subsection is right after */
    NonPagedCharge = sizeof(CONTROL_AREA);
    Subsection = (PSUBSECTION)(ControlArea + 1);

    /* We only support single-subsection mappings */
    NonPagedCharge += Size;
    ASSERT(Subsection->NextSubsection == NULL);

    /* Create the actual section object, with enough space for the prototype PTEs */
    Status = ObCreateObject(PreviousMode,
                            MmSectionObjectType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(SECTION),
                            PagedCharge,
                            NonPagedCharge,
                            (PVOID*)&NewSection);
    ASSERT(NT_SUCCESS(Status));

    /* Now copy the local section object from the stack into this new object */
    RtlCopyMemory(NewSection, &Section, sizeof(SECTION));
    NewSection->Address.StartingVpn = 0;

    /* For now, only user calls are supported */
    ASSERT(KernelCall == FALSE);
    NewSection->u.Flags.UserReference = TRUE;

    /* Migrate the attribute into a flag */
    if (AllocationAttributes & SEC_NO_CHANGE) NewSection->u.Flags.NoChange = TRUE;

    /* If R/W access is not requested, this might eventually become a CoW mapping */
    if (!(SectionPageProtection & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE)))
    {
        NewSection->u.Flags.CopyOnWrite = TRUE;
    }

    /* Is this a "based" allocation, in which all mappings are identical? */
    if (AllocationAttributes & SEC_BASED)
    {
        /* Convert the flag, and make sure the section isn't too big */
        NewSection->u.Flags.Based = TRUE;
        if ((ULONGLONG)NewSection->SizeOfSection.QuadPart >
            (ULONG_PTR)MmHighSectionBase)
        {
            DPRINT1("BASED section is too large\n");
            ObDereferenceObject(NewSection);
            return STATUS_NO_MEMORY;
        }

        /* Lock the VAD tree during the search */
        KeAcquireGuardedMutex(&MmSectionBasedMutex);

        /* Find an address top-down */
        Status = MiFindEmptyAddressRangeDownBasedTree(NewSection->SizeOfSection.LowPart,
                                                      (ULONG_PTR)MmHighSectionBase,
                                                      _64K,
                                                      &MmSectionBasedRoot,
                                                      &NewSection->Address.StartingVpn);
        ASSERT(NT_SUCCESS(Status));

        /* Compute the ending address and insert it into the VAD tree */
        NewSection->Address.EndingVpn = NewSection->Address.StartingVpn +
                                        NewSection->SizeOfSection.LowPart -
                                        1;
        MiInsertBasedSection(NewSection);

        /* Finally release the lock */
        KeReleaseGuardedMutex(&MmSectionBasedMutex);
    }

    /* Write down if this was a kernel call */
    ControlArea->u.Flags.WasPurged |= KernelCall;
    ASSERT(ControlArea->u.Flags.WasPurged == FALSE);

    /* Make sure the segment and the section are the same size, or the section is smaller */
    ASSERT((ULONG64)NewSection->SizeOfSection.QuadPart <= NewSection->Segment->SizeOfSegment);

    /* Return the object and the creation status */
    *SectionObject = (PVOID)NewSection;
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
MmMapViewOfArm3Section(IN PVOID SectionObject,
                       IN PEPROCESS Process,
                       IN OUT PVOID *BaseAddress,
                       IN ULONG_PTR ZeroBits,
                       IN SIZE_T CommitSize,
                       IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
                       IN OUT PSIZE_T ViewSize,
                       IN SECTION_INHERIT InheritDisposition,
                       IN ULONG AllocationType,
                       IN ULONG Protect)
{
    KAPC_STATE ApcState;
    BOOLEAN Attached = FALSE;
    PSECTION Section;
    PCONTROL_AREA ControlArea;
    ULONG ProtectionMask;
    NTSTATUS Status;
    PAGED_CODE();

    /* Get the segment and control area */
    Section = (PSECTION)SectionObject;
    ControlArea = Section->Segment->ControlArea;

    /* These flags/states are not yet supported by ARM3 */
    ASSERT(Section->u.Flags.Image == 0);
    ASSERT(Section->u.Flags.NoCache == 0);
    ASSERT(Section->u.Flags.WriteCombined == 0);
    ASSERT(ControlArea->u.Flags.PhysicalMemory == 0);

    /* FIXME */
    if ((AllocationType & MEM_RESERVE) != 0)
    {
        DPRINT1("MmMapViewOfArm3Section called with MEM_RESERVE, this is not implemented yet!!!\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    /* Check if the mapping protection is compatible with the create */
    if (!MiIsProtectionCompatible(Section->InitialPageProtection, Protect))
    {
        DPRINT1("Mapping protection is incompatible\n");
        return STATUS_SECTION_PROTECTION;
    }

    /* Check if the offset and size would cause an overflow */
    if (((ULONG64)SectionOffset->QuadPart + *ViewSize) <
         (ULONG64)SectionOffset->QuadPart)
    {
        DPRINT1("Section offset overflows\n");
        return STATUS_INVALID_VIEW_SIZE;
    }

    /* Check if the offset and size are bigger than the section itself */
    if (((ULONG64)SectionOffset->QuadPart + *ViewSize) >
         (ULONG64)Section->SizeOfSection.QuadPart)
    {
        DPRINT1("Section offset is larger than section\n");
        return STATUS_INVALID_VIEW_SIZE;
    }

    /* Check if the caller did not specify a view size */
    if (!(*ViewSize))
    {
        /* Compute it for the caller */
        *ViewSize = (SIZE_T)(Section->SizeOfSection.QuadPart - SectionOffset->QuadPart);

        /* Check if it's larger than 4GB or overflows into kernel-mode */
        if ((*ViewSize > 0xFFFFFFFF) ||
            (((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS - (ULONG_PTR)*BaseAddress) < *ViewSize))
        {
            DPRINT1("Section view won't fit\n");
            return STATUS_INVALID_VIEW_SIZE;
        }
    }

    /* Check if the commit size is larger than the view size */
    if (CommitSize > *ViewSize)
    {
        DPRINT1("Attempting to commit more than the view itself\n");
        return STATUS_INVALID_PARAMETER_5;
    }

    /* Check if the view size is larger than the section */
    if (*ViewSize > (ULONG64)Section->SizeOfSection.QuadPart)
    {
        DPRINT1("The view is larger than the section\n");
        return STATUS_INVALID_VIEW_SIZE;
    }

    /* Compute and validate the protection mask */
    ProtectionMask = MiMakeProtectionMask(Protect);
    if (ProtectionMask == MM_INVALID_PROTECTION)
    {
        DPRINT1("The protection is invalid\n");
        return STATUS_INVALID_PAGE_PROTECTION;
    }

    /* We only handle pagefile-backed sections, which cannot be writecombined */
    if (Protect & PAGE_WRITECOMBINE)
    {
        DPRINT1("Cannot write combine a pagefile-backed section\n");
        return STATUS_INVALID_PARAMETER_10;
    }

    /* Start by attaching to the current process if needed */
    if (PsGetCurrentProcess() != Process)
    {
        KeStackAttachProcess(&Process->Pcb, &ApcState);
        Attached = TRUE;
    }

    /* Do the actual mapping */
    Status = MiMapViewOfDataSection(ControlArea,
                                    Process,
                                    BaseAddress,
                                    SectionOffset,
                                    ViewSize,
                                    Section,
                                    InheritDisposition,
                                    ProtectionMask,
                                    CommitSize,
                                    ZeroBits,
                                    AllocationType);

    /* Detatch if needed, then return status */
    if (Attached) KeUnstackDetachProcess(&ApcState);
    return Status;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
MmDisableModifiedWriteOfSection(IN PSECTION_OBJECT_POINTERS SectionObjectPointer)
{
   UNIMPLEMENTED;
   return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
MmForceSectionClosed(IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
                     IN BOOLEAN DelayClose)
{
   UNIMPLEMENTED;
   return FALSE;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
MmMapViewInSessionSpace(IN PVOID Section,
                        OUT PVOID *MappedBase,
                        IN OUT PSIZE_T ViewSize)
{
    PAGED_CODE();

    // HACK
    if (MiIsRosSectionObject(Section))
    {
        return MmMapViewInSystemSpace(Section, MappedBase, ViewSize);
    }

    /* Process must be in a session */
    if (PsGetCurrentProcess()->ProcessInSession == FALSE)
    {
        DPRINT1("Process is not in session\n");
        return STATUS_NOT_MAPPED_VIEW;
    }

    /* Use the system space API, but with the session view instead */
    ASSERT(MmIsAddressValid(MmSessionSpace) == TRUE);
    return MiMapViewInSystemSpace(Section,
                                  &MmSessionSpace->Session,
                                  MappedBase,
                                  ViewSize);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
MmUnmapViewInSessionSpace(IN PVOID MappedBase)
{
    PAGED_CODE();

    // HACK
    if (!MI_IS_SESSION_ADDRESS(MappedBase))
    {
        return MmUnmapViewInSystemSpace(MappedBase);
    }

    /* Process must be in a session */
    if (PsGetCurrentProcess()->ProcessInSession == FALSE)
    {
        DPRINT1("Proess is not in session\n");
        return STATUS_NOT_MAPPED_VIEW;
    }

    /* Use the system space API, but with the session view instead */
    ASSERT(MmIsAddressValid(MmSessionSpace) == TRUE);
    return MiUnmapViewInSystemSpace(&MmSessionSpace->Session,
                                    MappedBase);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
MmUnmapViewOfSection(IN PEPROCESS Process,
                     IN PVOID BaseAddress)
{
    return MiUnmapViewOfSection(Process, BaseAddress, 0);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
MmUnmapViewInSystemSpace(IN PVOID MappedBase)
{
    PMEMORY_AREA MemoryArea;
    PAGED_CODE();

    /* Was this mapped by RosMm? */
    MemoryArea = MmLocateMemoryAreaByAddress(MmGetKernelAddressSpace(), MappedBase);
    if ((MemoryArea) && (MemoryArea->Type != MEMORY_AREA_OWNED_BY_ARM3))
    {
        return MiRosUnmapViewInSystemSpace(MappedBase);
    }

    /* It was not, call the ARM3 routine */
    return MiUnmapViewInSystemSpace(&MmSession, MappedBase);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
MmCommitSessionMappedView(IN PVOID MappedBase,
                          IN SIZE_T ViewSize)
{
    ULONG_PTR StartAddress, EndingAddress, Base;
    ULONG Hash, Count = 0, Size, QuotaCharge;
    PMMSESSION Session;
    PMMPTE LastProtoPte, PointerPte, ProtoPte;
    PCONTROL_AREA ControlArea;
    PSEGMENT Segment;
    PSUBSECTION Subsection;
    MMPTE TempPte;
    PAGED_CODE();

    /* Make sure the base isn't past the session view range */
    if ((MappedBase < MiSessionViewStart) ||
        (MappedBase >= (PVOID)((ULONG_PTR)MiSessionViewStart + MmSessionViewSize)))
    {
        DPRINT1("Base outside of valid range\n");
        return STATUS_INVALID_PARAMETER_1;
    }

    /* Make sure the size isn't past the session view range */
    if (((ULONG_PTR)MiSessionViewStart + MmSessionViewSize -
        (ULONG_PTR)MappedBase) < ViewSize)
    {
        DPRINT1("Size outside of valid range\n");
        return STATUS_INVALID_PARAMETER_2;
    }

    /* Sanity check */
    ASSERT(ViewSize != 0);

    /* Process must be in a session */
    if (PsGetCurrentProcess()->ProcessInSession == FALSE)
    {
        DPRINT1("Process is not in session\n");
        return STATUS_NOT_MAPPED_VIEW;
    }

    /* Compute the correctly aligned base and end addresses */
    StartAddress = (ULONG_PTR)PAGE_ALIGN(MappedBase);
    EndingAddress = ((ULONG_PTR)MappedBase + ViewSize - 1) | (PAGE_SIZE - 1);

    /* Sanity check and grab the session */
    ASSERT(MmIsAddressValid(MmSessionSpace) == TRUE);
    Session = &MmSessionSpace->Session;

    /* Get the hash entry for this allocation */
    Hash = (StartAddress >> 16) % Session->SystemSpaceHashKey;

    /* Lock system space */
    KeAcquireGuardedMutex(Session->SystemSpaceViewLockPointer);

    /* Loop twice so we can try rolling over if needed */
    while (TRUE)
    {
        /* Extract the size and base addresses from the entry */
        Base = Session->SystemSpaceViewTable[Hash].Entry & ~0xFFFF;
        Size = Session->SystemSpaceViewTable[Hash].Entry & 0xFFFF;

        /* Convert the size to bucket chunks */
        Size *= MI_SYSTEM_VIEW_BUCKET_SIZE;

        /* Bail out if this entry fits in here */
        if ((StartAddress >= Base) && (EndingAddress < (Base + Size))) break;

        /* Check if we overflew past the end of the hash table */
        if (++Hash >= Session->SystemSpaceHashSize)
        {
            /* Reset the hash to zero and keep searching from the bottom */
            Hash = 0;
            if (++Count == 2)
            {
                /* But if we overflew twice, then this is not a real mapping */
                KeBugCheckEx(DRIVER_UNMAPPING_INVALID_VIEW,
                             Base,
                             2,
                             0,
                             0);
            }
        }
    }

    /* Make sure the view being mapped is not file-based */
    ControlArea = Session->SystemSpaceViewTable[Hash].ControlArea;
    if (ControlArea->FilePointer != NULL)
    {
        /* It is, so we have to bail out */
        DPRINT1("Only page-filed backed sections can be commited\n");
        KeReleaseGuardedMutex(Session->SystemSpaceViewLockPointer);
        return STATUS_ALREADY_COMMITTED;
    }

    /* Get the subsection. We don't support LARGE_CONTROL_AREA in ARM3 */
    ASSERT(ControlArea->u.Flags.GlobalOnlyPerSession == 0);
    ASSERT(ControlArea->u.Flags.Rom == 0);
    Subsection = (PSUBSECTION)(ControlArea + 1);

    /* Get the start and end PTEs -- make sure the end PTE isn't past the end */
    ProtoPte = Subsection->SubsectionBase + ((StartAddress - Base) >> PAGE_SHIFT);
    QuotaCharge = MiAddressToPte(EndingAddress) - MiAddressToPte(StartAddress) + 1;
    LastProtoPte = ProtoPte + QuotaCharge;
    if (LastProtoPte >= Subsection->SubsectionBase + Subsection->PtesInSubsection)
    {
        DPRINT1("PTE is out of bounds\n");
        KeReleaseGuardedMutex(Session->SystemSpaceViewLockPointer);
        return STATUS_INVALID_PARAMETER_2;
    }

    /* Acquire the commit lock and count all the non-committed PTEs */
    KeAcquireGuardedMutexUnsafe(&MmSectionCommitMutex);
    PointerPte = ProtoPte;
    while (PointerPte < LastProtoPte)
    {
        if (PointerPte->u.Long) QuotaCharge--;
        PointerPte++;
    }

    /* Was everything committed already? */
    if (!QuotaCharge)
    {
        /* Nothing to do! */
        KeReleaseGuardedMutexUnsafe(&MmSectionCommitMutex);
        KeReleaseGuardedMutex(Session->SystemSpaceViewLockPointer);
        return STATUS_SUCCESS;
    }

    /* Pick the segment and template PTE */
    Segment = ControlArea->Segment;
    TempPte = Segment->SegmentPteTemplate;
    ASSERT(TempPte.u.Long != 0);

    /* Loop all prototype PTEs to be committed */
    PointerPte = ProtoPte;
    while (PointerPte < LastProtoPte)
    {
        /* Make sure the PTE is already invalid */
        if (PointerPte->u.Long == 0)
        {
            /* And write the invalid PTE */
            MI_WRITE_INVALID_PTE(PointerPte, TempPte);
        }

        /* Move to the next PTE */
        PointerPte++;
    }

    /* Check if we had at least one page charged */
    if (QuotaCharge)
    {
        /* Update the accounting data */
        Segment->NumberOfCommittedPages += QuotaCharge;
        InterlockedExchangeAddSizeT(&MmSharedCommit, QuotaCharge);
    }

    /* Release all */
    KeReleaseGuardedMutexUnsafe(&MmSectionCommitMutex);
    KeReleaseGuardedMutex(Session->SystemSpaceViewLockPointer);
    return STATUS_SUCCESS;
}

VOID
NTAPI
MiDeleteARM3Section(PVOID ObjectBody)
{
    PSECTION SectionObject;
    PCONTROL_AREA ControlArea;
    KIRQL OldIrql;

    SectionObject = (PSECTION)ObjectBody;

    /* Lock the PFN database */
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

    ASSERT(SectionObject->Segment);
    ASSERT(SectionObject->Segment->ControlArea);

    ControlArea = SectionObject->Segment->ControlArea;

    /* Dereference */
    ControlArea->NumberOfSectionReferences--;
    ControlArea->NumberOfUserReferences--;

    ASSERT(ControlArea->u.Flags.BeingDeleted == 0);

    /* Check it. It will delete it if there is no more reference to it */
    MiCheckControlArea(ControlArea, OldIrql);
}

/* SYSTEM CALLS ***************************************************************/

NTSTATUS
NTAPI
NtAreMappedFilesTheSame(IN PVOID File1MappedAsAnImage,
                        IN PVOID File2MappedAsFile)
{
    PVOID AddressSpace;
    PFILE_OBJECT FileObject1, FileObject2;
    NTSTATUS Status;

    /* Lock address space */
    AddressSpace = MmGetCurrentAddressSpace();
    MmLockAddressSpace(AddressSpace);

    /* Get the file object pointer for address 1 */
    Status = MiGetFileObjectForSectionAddress(File1MappedAsAnImage, &FileObject1);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get file object for Address %p\n", File1MappedAsAnImage);
        MmUnlockAddressSpace(AddressSpace);
        return Status;
    }

    /* Get the file object pointer for address 2 */
    Status = MiGetFileObjectForSectionAddress(File2MappedAsFile, &FileObject2);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get file object for Address %p\n", File1MappedAsAnImage);
        MmUnlockAddressSpace(AddressSpace);
        return Status;
    }

    /* SectionObjectPointer is equal if the files are equal */
    if (FileObject1->SectionObjectPointer != FileObject2->SectionObjectPointer)
    {
        MmUnlockAddressSpace(AddressSpace);
        return STATUS_NOT_SAME_DEVICE;
    }

    /* Unlock address space */
    MmUnlockAddressSpace(AddressSpace);
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtCreateSection(OUT PHANDLE SectionHandle,
                IN ACCESS_MASK DesiredAccess,
                IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
                IN PLARGE_INTEGER MaximumSize OPTIONAL,
                IN ULONG SectionPageProtection OPTIONAL,
                IN ULONG AllocationAttributes,
                IN HANDLE FileHandle OPTIONAL)
{
    LARGE_INTEGER SafeMaximumSize;
    PVOID SectionObject;
    HANDLE Handle;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    PAGED_CODE();

    /* Check for non-existing flags */
    if ((AllocationAttributes & ~(SEC_COMMIT | SEC_RESERVE | SEC_BASED |
                                  SEC_LARGE_PAGES | SEC_IMAGE | SEC_NOCACHE |
                                  SEC_NO_CHANGE)))
    {
        if (!(AllocationAttributes & 1))
        {
            DPRINT1("Bogus allocation attribute: %lx\n", AllocationAttributes);
            return STATUS_INVALID_PARAMETER_6;
        }
    }

    /* Check for no allocation type */
    if (!(AllocationAttributes & (SEC_COMMIT | SEC_RESERVE | SEC_IMAGE)))
    {
        DPRINT1("Missing allocation type in allocation attributes\n");
        return STATUS_INVALID_PARAMETER_6;
    }

    /* Check for image allocation with invalid attributes */
    if ((AllocationAttributes & SEC_IMAGE) &&
        (AllocationAttributes & (SEC_COMMIT | SEC_RESERVE | SEC_LARGE_PAGES |
                                 SEC_NOCACHE | SEC_NO_CHANGE)))
    {
        DPRINT1("Image allocation with invalid attributes\n");
        return STATUS_INVALID_PARAMETER_6;
    }

    /* Check for allocation type is both commit and reserve */
    if ((AllocationAttributes & SEC_COMMIT) && (AllocationAttributes & SEC_RESERVE))
    {
        DPRINT1("Commit and reserve in the same time\n");
        return STATUS_INVALID_PARAMETER_6;
    }

    /* Now check for valid protection */
    if ((SectionPageProtection & PAGE_NOCACHE) ||
        (SectionPageProtection & PAGE_WRITECOMBINE) ||
        (SectionPageProtection & PAGE_GUARD) ||
        (SectionPageProtection & PAGE_NOACCESS))
    {
        DPRINT1("Sections don't support these protections\n");
        return STATUS_INVALID_PAGE_PROTECTION;
    }

    /* Use a maximum size of zero, if none was specified */
    SafeMaximumSize.QuadPart = 0;

    /* Check for user-mode caller */
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH */
        _SEH2_TRY
        {
            /* Safely check user-mode parameters */
            if (MaximumSize) SafeMaximumSize = ProbeForReadLargeInteger(MaximumSize);
            MaximumSize = &SafeMaximumSize;
            ProbeForWriteHandle(SectionHandle);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else if (!MaximumSize) MaximumSize = &SafeMaximumSize;

    /* Check that MaximumSize is valid if backed by paging file */
    if ((!FileHandle) && (!MaximumSize->QuadPart))
        return STATUS_INVALID_PARAMETER_4;

    /* Create the section */
    Status = MmCreateSection(&SectionObject,
                             DesiredAccess,
                             ObjectAttributes,
                             MaximumSize,
                             SectionPageProtection,
                             AllocationAttributes,
                             FileHandle,
                             NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* FIXME: Should zero last page for a file mapping */

    /* Now insert the object */
    Status = ObInsertObject(SectionObject,
                            NULL,
                            DesiredAccess,
                            0,
                            NULL,
                            &Handle);
    if (NT_SUCCESS(Status))
    {
        /* Enter SEH */
        _SEH2_TRY
        {
            /* Return the handle safely */
            *SectionHandle = Handle;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Nothing here */
        }
        _SEH2_END;
    }

    /* Return the status */
    return Status;
}

NTSTATUS
NTAPI
NtOpenSection(OUT PHANDLE SectionHandle,
              IN ACCESS_MASK DesiredAccess,
              IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    HANDLE Handle;
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PAGED_CODE();

    /* Check for user-mode caller */
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH */
        _SEH2_TRY
        {
            /* Safely check user-mode parameters */
            ProbeForWriteHandle(SectionHandle);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Try opening the object */
    Status = ObOpenObjectByName(ObjectAttributes,
                                MmSectionObjectType,
                                PreviousMode,
                                NULL,
                                DesiredAccess,
                                NULL,
                                &Handle);

    /* Enter SEH */
    _SEH2_TRY
    {
        /* Return the handle safely */
        *SectionHandle = Handle;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Nothing here */
    }
    _SEH2_END;

    /* Return the status */
    return Status;
}

NTSTATUS
NTAPI
NtMapViewOfSection(IN HANDLE SectionHandle,
                   IN HANDLE ProcessHandle,
                   IN OUT PVOID* BaseAddress,
                   IN ULONG_PTR ZeroBits,
                   IN SIZE_T CommitSize,
                   IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
                   IN OUT PSIZE_T ViewSize,
                   IN SECTION_INHERIT InheritDisposition,
                   IN ULONG AllocationType,
                   IN ULONG Protect)
{
    PVOID SafeBaseAddress;
    LARGE_INTEGER SafeSectionOffset;
    SIZE_T SafeViewSize;
    PROS_SECTION_OBJECT Section;
    PEPROCESS Process;
    NTSTATUS Status;
    ACCESS_MASK DesiredAccess;
    ULONG ProtectionMask;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();

    /* Check for invalid zero bits */
    if (ZeroBits > 21) // per-arch?
    {
        DPRINT1("Invalid zero bits\n");
        return STATUS_INVALID_PARAMETER_4;
    }

    /* Check for invalid inherit disposition */
    if ((InheritDisposition > ViewUnmap) || (InheritDisposition < ViewShare))
    {
        DPRINT1("Invalid inherit disposition\n");
        return STATUS_INVALID_PARAMETER_8;
    }

    /* Allow only valid allocation types */
    if ((AllocationType & ~(MEM_TOP_DOWN | MEM_LARGE_PAGES | MEM_DOS_LIM |
                            SEC_NO_CHANGE | MEM_RESERVE)))
    {
        DPRINT1("Invalid allocation type\n");
        return STATUS_INVALID_PARAMETER_9;
    }

    /* Convert the protection mask, and validate it */
    ProtectionMask = MiMakeProtectionMask(Protect);
    if (ProtectionMask == MM_INVALID_PROTECTION)
    {
        DPRINT1("Invalid page protection\n");
        return STATUS_INVALID_PAGE_PROTECTION;
    }

    /* Check for non-allocation-granularity-aligned BaseAddress */
    if (BaseAddress && (*BaseAddress != ALIGN_DOWN_POINTER_BY(*BaseAddress, MM_VIRTMEM_GRANULARITY)))
    {
       DPRINT("BaseAddress is not at 64-kilobyte address boundary.");
       return STATUS_MAPPED_ALIGNMENT;
    }

    /* Now convert the protection mask into desired section access mask */
    DesiredAccess = MmMakeSectionAccess[ProtectionMask & 0x7];

    /* Assume no section offset */
    SafeSectionOffset.QuadPart = 0;

    /* Enter SEH */
    _SEH2_TRY
    {
        /* Check for unsafe parameters */
        if (PreviousMode != KernelMode)
        {
            /* Probe the parameters */
            ProbeForWritePointer(BaseAddress);
            ProbeForWriteSize_t(ViewSize);
        }

        /* Check if a section offset was given */
        if (SectionOffset)
        {
            /* Check for unsafe parameters and capture section offset */
            if (PreviousMode != KernelMode) ProbeForWriteLargeInteger(SectionOffset);
            SafeSectionOffset = *SectionOffset;
        }

        /* Capture the other parameters */
        SafeBaseAddress = *BaseAddress;
        SafeViewSize = *ViewSize;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Return the exception code */
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Check for kernel-mode address */
    if (SafeBaseAddress > MM_HIGHEST_VAD_ADDRESS)
    {
        DPRINT1("Kernel base not allowed\n");
        return STATUS_INVALID_PARAMETER_3;
    }

    /* Check for range entering kernel-mode */
    if (((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS - (ULONG_PTR)SafeBaseAddress) < SafeViewSize)
    {
        DPRINT1("Overflowing into kernel base not allowed\n");
        return STATUS_INVALID_PARAMETER_3;
    }

    /* Check for invalid zero bits */
    if (((ULONG_PTR)SafeBaseAddress + SafeViewSize) > (0xFFFFFFFF >> ZeroBits)) // arch?
    {
        DPRINT1("Invalid zero bits\n");
        return STATUS_INVALID_PARAMETER_4;
    }

    /* Reference the process */
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_VM_OPERATION,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID*)&Process,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Reference the section */
    Status = ObReferenceObjectByHandle(SectionHandle,
                                       DesiredAccess,
                                       MmSectionObjectType,
                                       PreviousMode,
                                       (PVOID*)&Section,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(Process);
        return Status;
    }

    /* Now do the actual mapping */
    Status = MmMapViewOfSection(Section,
                                Process,
                                &SafeBaseAddress,
                                ZeroBits,
                                CommitSize,
                                &SafeSectionOffset,
                                &SafeViewSize,
                                InheritDisposition,
                                AllocationType,
                                Protect);

    /* Return data only on success */
    if (NT_SUCCESS(Status))
    {
        /* Check if this is an image for the current process */
        if ((Section->AllocationAttributes & SEC_IMAGE) &&
            (Process == PsGetCurrentProcess()) &&
            (Status != STATUS_IMAGE_NOT_AT_BASE))
        {
            /* Notify the debugger */
            DbgkMapViewOfSection(Section,
                                 SafeBaseAddress,
                                 SafeSectionOffset.LowPart,
                                 SafeViewSize);
        }

        /* Enter SEH */
        _SEH2_TRY
        {
            /* Return parameters to user */
            *BaseAddress = SafeBaseAddress;
            *ViewSize = SafeViewSize;
            if (SectionOffset) *SectionOffset = SafeSectionOffset;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Nothing to do */
        }
        _SEH2_END;
    }

    /* Dereference all objects and return status */
    ObDereferenceObject(Section);
    ObDereferenceObject(Process);
    return Status;
}

NTSTATUS
NTAPI
NtUnmapViewOfSection(IN HANDLE ProcessHandle,
                     IN PVOID BaseAddress)
{
    PEPROCESS Process;
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();

    /* Don't allowing mapping kernel views */
    if ((PreviousMode == UserMode) && (BaseAddress > MM_HIGHEST_USER_ADDRESS))
    {
        DPRINT1("Trying to unmap a kernel view\n");
        return STATUS_NOT_MAPPED_VIEW;
    }

    /* Reference the process */
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_VM_OPERATION,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID*)&Process,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Unmap the view */
    Status = MiUnmapViewOfSection(Process, BaseAddress, 0);

    /* Dereference the process and return status */
    ObDereferenceObject(Process);
    return Status;
}

NTSTATUS
NTAPI
NtExtendSection(IN HANDLE SectionHandle,
                IN OUT PLARGE_INTEGER NewMaximumSize)
{
    LARGE_INTEGER SafeNewMaximumSize;
    PROS_SECTION_OBJECT Section;
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();

    /* Check for user-mode parameters */
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH */
        _SEH2_TRY
        {
            /* Probe and capture the maximum size, it's both read and write */
            ProbeForWriteLargeInteger(NewMaximumSize);
            SafeNewMaximumSize = *NewMaximumSize;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        /* Just read the size directly */
        SafeNewMaximumSize = *NewMaximumSize;
    }

    /* Reference the section */
    Status = ObReferenceObjectByHandle(SectionHandle,
                                       SECTION_EXTEND_SIZE,
                                       MmSectionObjectType,
                                       PreviousMode,
                                       (PVOID*)&Section,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Really this should go in MmExtendSection */
    if (!(Section->AllocationAttributes & SEC_FILE))
    {
        DPRINT1("Not extending a file\n");
        ObDereferenceObject(Section);
        return STATUS_SECTION_NOT_EXTENDED;
    }

    /* FIXME: Do the work */

    /* Dereference the section */
    ObDereferenceObject(Section);

    /* Enter SEH */
    _SEH2_TRY
    {
        /* Write back the new size */
        *NewMaximumSize = SafeNewMaximumSize;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Nothing to do */
    }
    _SEH2_END;

    /* Return the status */
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
