/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/sectopm.c
 * PURPOSE:         ARM Memory Manager Section Support
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include "../ARM3/miarm.h"

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

MMSESSION MmSession;

/* PRIVATE FUNCTIONS **********************************************************/

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
        ProtectMask |= MM_DECOMMIT;
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
MiInitializeSystemSpaceMap(IN PVOID InputSession OPTIONAL)
{
    SIZE_T AllocSize, BitmapSize;
    PMMSESSION Session;

    /* For now, always use the global session */
    ASSERT(InputSession == NULL);
    Session = &MmSession;

    /* Initialize the system space lock */
    Session->SystemSpaceViewLockPointer = &Session->SystemSpaceViewLock;
    KeInitializeGuardedMutex(Session->SystemSpaceViewLockPointer);

    /* Set the start address */
    Session->SystemSpaceViewStart = MiSystemViewStart;

    /* Create a bitmap to describe system space */
    BitmapSize = sizeof(RTL_BITMAP) + ((((MmSystemViewSize / MI_SYSTEM_VIEW_BUCKET_SIZE) + 31) / 32) * sizeof(ULONG));
    Session->SystemSpaceBitMap = ExAllocatePoolWithTag(NonPagedPool,
                                                       BitmapSize,
                                                       TAG_MM);
    ASSERT(Session->SystemSpaceBitMap);
    RtlInitializeBitMap(Session->SystemSpaceBitMap,
                        (PULONG)(Session->SystemSpaceBitMap + 1),
                        (ULONG)(MmSystemViewSize / MI_SYSTEM_VIEW_BUCKET_SIZE));

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
    Session->SystemSpaceViewTable = ExAllocatePoolWithTag(NonPagedPool,
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
    ULONG Entry, Hash, i;
    PAGED_CODE();

    /* Only global mappings supported for now */
    ASSERT(Session == &MmSession);

    /* Stay within 4GB and don't go past the number of hash entries available */
    ASSERT(Buckets < MI_SYSTEM_VIEW_BUCKET_SIZE);
    ASSERT(Session->SystemSpaceHashEntries < Session->SystemSpaceHashSize);

    /* Find space where to map this view */
    i = RtlFindClearBitsAndSet(Session->SystemSpaceBitMap, Buckets, 0);
    ASSERT(i != 0xFFFFFFFF);
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
            UNIMPLEMENTED;
            while (TRUE);
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

#if (_MI_PAGING_LEVELS == 2)
VOID
NTAPI
MiFillSystemPageDirectory(IN PVOID Base,
                          IN SIZE_T NumberOfBytes)
{
    PMMPDE PointerPde, LastPde, SystemMapPde;
    MMPDE TempPde;
    PFN_NUMBER PageFrameIndex;
    KIRQL OldIrql;
    PAGED_CODE();

    /* Find the PDEs needed for this mapping */
    PointerPde = MiAddressToPde(Base);
    LastPde = MiAddressToPde((PVOID)((ULONG_PTR)Base + NumberOfBytes - 1));

    /* Find the system double-mapped PDE that describes this mapping */
    SystemMapPde = &MmSystemPagePtes[((ULONG_PTR)PointerPde & (SYSTEM_PD_SIZE - 1)) / sizeof(MMPTE)];

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

            /* Initialize its PFN entry, with the parent system page directory page table */
            MiInitializePfnForOtherProcess(PageFrameIndex,
                                           (PMMPTE)PointerPde,
                                           MmSystemPageDirectory[(PointerPde - MiAddressToPde(NULL)) / PDE_COUNT]);

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
#endif

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
    ASSERT(!(ControlArea->u.Flags.Image) & !(ControlArea->u.Flags.File));
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
        ASSERT(TempPte.u.Soft.Prototype == 1);

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
MiRemoveMappedView(IN PEPROCESS CurrentProcess,
                   IN PMMVAD Vad)
{
    KIRQL OldIrql;
    PCONTROL_AREA ControlArea;

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
    MiUnlockProcessWorkingSet(CurrentProcess, PsGetCurrentThread());

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

    /* Only global mappings for now */
    ASSERT(Session == &MmSession);

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
        /* We should probably fail. FIXME TODO */
        UNIMPLEMENTED;
        while (TRUE);
    }

    /* Get the number of 64K buckets required for this mapping */
    Buckets = (ULONG)(*ViewSize / MI_SYSTEM_VIEW_BUCKET_SIZE);
    if (*ViewSize & (MI_SYSTEM_VIEW_BUCKET_SIZE - 1)) Buckets++;

    /* Check if the view is more than 4GB large */
    if (Buckets >= MI_SYSTEM_VIEW_BUCKET_SIZE)
    {
        /* We should probably fail */
        UNIMPLEMENTED;
        while (TRUE);
    }

    /* Insert this view into system space and get a base address for it */
    Base = MiInsertInSystemSpace(Session, Buckets, ControlArea);
    ASSERT(Base);

#if (_MI_PAGING_LEVELS == 2)
    /* Create the PDEs needed for this mapping, and double-map them if needed */
    MiFillSystemPageDirectory(Base, Buckets * MI_SYSTEM_VIEW_BUCKET_SIZE);
#endif

    /* Create the actual prototype PTEs for this mapping */
    Status = MiAddMappedPtes(MiAddressToPte(Base),
                             BYTES_TO_PAGES(*ViewSize),
                             ControlArea);
    ASSERT(NT_SUCCESS(Status));

    /* Return the base adress of the mapping and success */
    *MappedBase = Base;
    return STATUS_SUCCESS;
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
    PMMVAD Vad;
    PETHREAD Thread = PsGetCurrentThread();
    ULONG_PTR StartAddress, EndingAddress;
    PSUBSECTION Subsection;
    PSEGMENT Segment;
    PFN_NUMBER PteOffset;
    NTSTATUS Status;

    /* Get the segment and subection for this section */
    Segment = ControlArea->Segment;
    Subsection = (PSUBSECTION)(ControlArea + 1);

    /* Non-pagefile-backed sections not supported */
    ASSERT(ControlArea->u.Flags.GlobalOnlyPerSession == 0);
    ASSERT(ControlArea->u.Flags.Rom == 0);
    ASSERT(ControlArea->FilePointer == NULL);
    ASSERT(Segment->SegmentFlags.TotalNumberOfPtes4132 == 0);

    /* Based sections not supported */
    ASSERT(Section->Address.StartingVpn == 0);

    /* These flags/parameters are not supported */
    ASSERT((AllocationType & MEM_DOS_LIM) == 0);
    ASSERT((AllocationType & MEM_RESERVE) == 0);
    ASSERT(Process->VmTopDown == 0);
    ASSERT(Section->u.Flags.CopyOnWrite == FALSE);
    ASSERT(ZeroBits == 0);

    /* First, increase the map count. No purging is supported yet */
    Status = MiCheckPurgeAndUpMapCount(ControlArea, FALSE);
    ASSERT(NT_SUCCESS(Status));

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

    /* We must be dealing with a 64KB aligned offset */
    ASSERT((SectionOffset->LowPart & ((ULONG)_64K - 1)) == 0);

    /* It's illegal to try to map more than 2GB */
    if (*ViewSize >= 0x80000000) return STATUS_INVALID_VIEW_SIZE;

    /* Within this section, figure out which PTEs will describe the view */
    PteOffset = (PFN_NUMBER)(SectionOffset->QuadPart >> PAGE_SHIFT);

    /* The offset must be in this segment's PTE chunk and it must be valid */
    ASSERT(PteOffset < Segment->TotalNumberOfPtes);
    ASSERT(((SectionOffset->QuadPart + *ViewSize + PAGE_SIZE - 1) >> PAGE_SHIFT) >= PteOffset);

    /* In ARM3, only one subsection is used for now. It must contain these PTEs */
    ASSERT(PteOffset < Subsection->PtesInSubsection);
    ASSERT(Subsection->SubsectionBase != NULL);

    /* In ARM3, only MEM_COMMIT is supported for now. The PTEs must've been committed */
    ASSERT(Segment->NumberOfCommittedPages >= Segment->TotalNumberOfPtes);

    /* Did the caller specify an address? */
    if (!(*BaseAddress))
    {
        /* Which way should we search? */
        if (AllocationType & MEM_TOP_DOWN)
        {
            /* No, find an address top-down */
            Status = MiFindEmptyAddressRangeDownTree(*ViewSize,
                                                     (ULONG_PTR)MM_HIGHEST_VAD_ADDRESS,
                                                     _64K,
                                                     &Process->VadRoot,
                                                     &StartAddress,
                                                     (PMMADDRESS_NODE*)&Process->VadFreeHint);
            ASSERT(NT_SUCCESS(Status));
        }
        else
        {
            /* No, find an address bottom-up */
            Status = MiFindEmptyAddressRangeInTree(*ViewSize,
                                                   _64K,
                                                   &Process->VadRoot,
                                                   (PMMADDRESS_NODE*)&Process->VadFreeHint,
                                                   &StartAddress);
            ASSERT(NT_SUCCESS(Status));
        }
    }
    else
    {
        /* This (rather easy) code path is not yet implemented */
        UNIMPLEMENTED;
        while (TRUE);
    }

    /* Get the ending address, which is the last piece we need for the VAD */
    EndingAddress = (StartAddress + *ViewSize - 1) | (PAGE_SIZE - 1);

    /* A VAD can now be allocated. Do so and zero it out */
    Vad = ExAllocatePoolWithTag(NonPagedPool, sizeof(MMVAD), 'ldaV');
    ASSERT(Vad);
    RtlZeroMemory(Vad, sizeof(MMVAD));

    /* Write all the data required in the VAD for handling a fault */
    Vad->StartingVpn = StartAddress >> PAGE_SHIFT;
    Vad->EndingVpn = EndingAddress >> PAGE_SHIFT;
    Vad->ControlArea = ControlArea;
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
    PteOffset += (Vad->EndingVpn - Vad->StartingVpn);
    Vad->LastContiguousPte = &Subsection->SubsectionBase[PteOffset];

    /* Make sure the last PTE is valid and still within the subsection */
    ASSERT(PteOffset < Subsection->PtesInSubsection);
    ASSERT(Vad->FirstPrototypePte <= Vad->LastContiguousPte);

    /* FIXME: Should setup VAD bitmap */
    Status = STATUS_SUCCESS;

    /* Pretend as if we own the working set */
    MiLockProcessWorkingSet(Process, Thread);

    /* Insert the VAD */
    MiInsertVad(Vad, Process);

    /* Release the working set */
    MiUnlockProcessWorkingSet(Process, Thread);

    /* Windows stores this for accounting purposes, do so as well */
    if (!Segment->u2.FirstMappedVa) Segment->u2.FirstMappedVa = (PVOID)StartAddress;

    /* Finally, let the caller know where, and for what size, the view was mapped */
    *ViewSize = (ULONG_PTR)EndingAddress - (ULONG_PTR)StartAddress + 1;
    *BaseAddress = (PVOID)StartAddress;
    return STATUS_SUCCESS;
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

PFILE_OBJECT
NTAPI
MmGetFileObjectForSection(IN PVOID SectionObject)
{
    PSECTION_OBJECT Section;
    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
    ASSERT(SectionObject != NULL);

    /* Check if it's an ARM3, or ReactOS section */
    if ((ULONG_PTR)SectionObject & 1)
    {
        /* Return the file pointer stored in the control area */
        Section = (PVOID)((ULONG_PTR)SectionObject & ~1);
        return Section->Segment->ControlArea->FilePointer;
    }

    /* Return the file object */
    return ((PROS_SECTION_OBJECT)SectionObject)->FileObject;
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
    if ((ULONG_PTR)Section & 1)
    {
        /* Check ARM3 Section flag */
        if (((PSECTION)((ULONG_PTR)Section & ~1))->u.Flags.Image == 0)
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
   PVOID Section;
   PMEMORY_AREA MemoryArea;
   POBJECT_NAME_INFORMATION ModuleNameInformation;
   PVOID AddressSpace;
   NTSTATUS Status;
   PFILE_OBJECT FileObject = NULL;
   PMMVAD Vad;
   PCONTROL_AREA ControlArea;

   /* Lock address space */
   AddressSpace = MmGetCurrentAddressSpace();
   MmLockAddressSpace(AddressSpace);

   /* Locate the memory area for the process by address */
   MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, Address);
   if (!MemoryArea)
   {
       /* Fail, the address does not exist */
InvalidAddress:
       DPRINT1("Invalid address\n");
       MmUnlockAddressSpace(AddressSpace);
       return STATUS_INVALID_ADDRESS;
   }

   /* Check if it's a section view (RosMm section) or ARM3 section */
   if (MemoryArea->Type == MEMORY_AREA_SECTION_VIEW)
   {
      /* Get the section pointer to the SECTION_OBJECT */
      Section = MemoryArea->Data.SectionData.Section;

      /* Unlock address space */
      MmUnlockAddressSpace(AddressSpace);

      /* Get the filename of the section */
      Status = MmGetFileNameForSection(Section, &ModuleNameInformation);
   }
   else if (MemoryArea->Type == MEMORY_AREA_OWNED_BY_ARM3)
   {
       /* Get the VAD */
       Vad = MiLocateAddress(Address);
       if (!Vad) goto InvalidAddress;

       /* Make sure it's not a VM VAD */
       if (Vad->u.VadFlags.PrivateMemory == 1)
       {
NotSection:
           DPRINT1("Address is not a section\n");
           MmUnlockAddressSpace(AddressSpace);
           return STATUS_SECTION_NOT_IMAGE;
       }

       /* Get the control area */
       ControlArea = Vad->ControlArea;
       if (!(ControlArea) || !(ControlArea->u.Flags.Image)) goto NotSection;

       /* Get the file object */
       FileObject = ControlArea->FilePointer;
       ASSERT(FileObject != NULL);
       ObReferenceObject(FileObject);

       /* Unlock address space */
       MmUnlockAddressSpace(AddressSpace);

       /* Get the filename of the file object */
       Status = MmGetFileNameForFileObject(FileObject, &ModuleNameInformation);

       /* Dereference it */
       ObDereferenceObject(FileObject);
   }
   else
   {
       /* Trying to access virtual memory or something */
       goto InvalidAddress;
   }

   /* Check if we were able to get the file object name */
   if (NT_SUCCESS(Status))
   {
        /* Init modulename */
       RtlCreateUnicodeString(ModuleName,
                              ModuleNameInformation->Name.Buffer);

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
    PSEGMENT NewSegment;
    NTSTATUS Status;
    PCONTROL_AREA ControlArea;
    ULONG ProtectionMask;

    /* ARM3 does not yet support this */
    ASSERT(FileHandle == NULL);
    ASSERT(FileObject == NULL);
    ASSERT((AllocationAttributes & SEC_LARGE_PAGES) == 0);
    ASSERT((AllocationAttributes & SEC_BASED) == 0);

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

    /* A handle must be supplied with SEC_IMAGE, and this is the no-handle path */
    if (AllocationAttributes & SEC_IMAGE) return STATUS_INVALID_FILE_FOR_SECTION;

    /* So this must be a pagefile-backed section, create the mappings needed */
    Status = MiCreatePagingFileMap(&NewSegment,
                                   (PSIZE_T)InputMaximumSize,
                                   ProtectionMask,
                                   AllocationAttributes);
    ASSERT(NT_SUCCESS(Status));

    /* Set the initial section object data */
    Section.InitialPageProtection = SectionPageProtection;
    Section.Segment = NULL;
    Section.SizeOfSection.QuadPart = NewSegment->SizeOfSegment;
    Section.Segment = NewSegment;

    /* THe mapping created a control area and segment, save the flags */
    ControlArea = NewSegment->ControlArea;
    Section.u.LongFlags = ControlArea->u.LongFlags;

    /* ARM3 cannot support these right now, make sure they're not being set */
    ASSERT(ControlArea->u.Flags.Image == 0);
    ASSERT(ControlArea->FilePointer == NULL);
    ASSERT(ControlArea->u.Flags.GlobalOnlyPerSession == 0);
    ASSERT(ControlArea->u.Flags.Rom == 0);
    ASSERT(ControlArea->u.Flags.WasPurged == 0);

    /* A pagefile-backed mapping only has one subsection, and this is all ARM3 supports */
    Subsection = (PSUBSECTION)(ControlArea + 1);
    ASSERT(Subsection->NextSubsection == NULL);

    /* Create the actual section object, with enough space for the prototype PTEs */
    Status = ObCreateObject(ExGetPreviousMode(),
                            MmSectionObjectType,
                            ObjectAttributes,
                            ExGetPreviousMode(),
                            NULL,
                            sizeof(SECTION),
                            sizeof(SECTION) +
                            NewSegment->TotalNumberOfPtes * sizeof(MMPTE),
                            sizeof(CONTROL_AREA) + sizeof(SUBSECTION),
                            (PVOID*)&NewSection);
    ASSERT(NT_SUCCESS(Status));

    /* Now copy the local section object from the stack into this new object */
    RtlCopyMemory(NewSection, &Section, sizeof(SECTION));
    NewSection->Address.StartingVpn = 0;

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
    ASSERT((AllocationType & MEM_RESERVE) == 0);
    ASSERT(ControlArea->u.Flags.PhysicalMemory == 0);


#if 0
    /* FIXME: Check if the mapping protection is compatible with the create */
    if (!MiIsProtectionCompatible(Section->InitialPageProtection, Protect))
    {
        DPRINT1("Mapping protection is incompatible\n");
        return STATUS_SECTION_PROTECTION;
    }
#endif

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

    /* Lock the address space and make sure the process is alive */
    MmLockAddressSpace(&Process->Vm);
    if (!Process->VmDeleted)
    {
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
    }
    else
    {
        /* The process is being terminated, fail */
        DPRINT1("The process is dying\n");
        Status = STATUS_PROCESS_IS_TERMINATING;
    }

    /* Unlock the address space and detatch if needed, then return status */
    MmUnlockAddressSpace(&Process->Vm);
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
 * @unimplemented
 */
NTSTATUS
NTAPI
MmMapViewInSessionSpace(IN PVOID Section,
                        OUT PVOID *MappedBase,
                        IN OUT PSIZE_T ViewSize)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
MmUnmapViewInSessionSpace(IN PVOID MappedBase)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* SYSTEM CALLS ***************************************************************/

NTSTATUS
NTAPI
NtAreMappedFilesTheSame(IN PVOID File1MappedAsAnImage,
                        IN PVOID File2MappedAsFile)
{
    PVOID AddressSpace;
    PMEMORY_AREA MemoryArea1, MemoryArea2;
    PROS_SECTION_OBJECT Section1, Section2;
    
    /* Lock address space */
    AddressSpace = MmGetCurrentAddressSpace();
    MmLockAddressSpace(AddressSpace);
    
    /* Locate the memory area for the process by address */
    MemoryArea1 = MmLocateMemoryAreaByAddress(AddressSpace, File1MappedAsAnImage);
    if (!MemoryArea1)
    {
        /* Fail, the address does not exist */
        MmUnlockAddressSpace(AddressSpace);
        return STATUS_INVALID_ADDRESS;
    }
    
    /* Check if it's a section view (RosMm section) or ARM3 section */
    if (MemoryArea1->Type != MEMORY_AREA_SECTION_VIEW)
    {
        /* Fail, the address is not a section */
        MmUnlockAddressSpace(AddressSpace);
        return STATUS_CONFLICTING_ADDRESSES;
    }
    
    /* Get the section pointer to the SECTION_OBJECT */
    Section1 = MemoryArea1->Data.SectionData.Section;
    if (Section1->FileObject == NULL)
    {
        MmUnlockAddressSpace(AddressSpace);
        return STATUS_CONFLICTING_ADDRESSES; 
    }
    
    /* Locate the memory area for the process by address */
    MemoryArea2 = MmLocateMemoryAreaByAddress(AddressSpace, File2MappedAsFile);
    if (!MemoryArea2)
    {
        /* Fail, the address does not exist */
        MmUnlockAddressSpace(AddressSpace);
        return STATUS_INVALID_ADDRESS;
    }
    
    /* Check if it's a section view (RosMm section) or ARM3 section */
    if (MemoryArea2->Type != MEMORY_AREA_SECTION_VIEW)
    {
        /* Fail, the address is not a section */
        MmUnlockAddressSpace(AddressSpace);
        return STATUS_CONFLICTING_ADDRESSES;
    }
    
    /* Get the section pointer to the SECTION_OBJECT */
    Section2 = MemoryArea2->Data.SectionData.Section;
    if (Section2->FileObject == NULL)
    {
        MmUnlockAddressSpace(AddressSpace);
        return STATUS_CONFLICTING_ADDRESSES; 
    }
    
    /* The shared cache map seems to be the same if both of these are equal */
    if (Section1->FileObject->SectionObjectPointer->SharedCacheMap ==
        Section2->FileObject->SectionObjectPointer->SharedCacheMap)
    {
        MmUnlockAddressSpace(AddressSpace);
        return STATUS_SUCCESS; 
    }
    
    /* Unlock address space */
    MmUnlockAddressSpace(AddressSpace);
    return STATUS_NOT_SAME_DEVICE;
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
    Status = MmUnmapViewOfSection(Process, BaseAddress);

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
