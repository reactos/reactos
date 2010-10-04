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

#line 15 "ARM³::SECTION"
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
                                                       '  mM');
    ASSERT(Session->SystemSpaceBitMap);
    RtlInitializeBitMap(Session->SystemSpaceBitMap,
                        (PULONG)(Session->SystemSpaceBitMap + 1),
                        MmSystemViewSize / MI_SYSTEM_VIEW_BUCKET_SIZE);

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
                                                          '  mM');
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
            PageFrameIndex = MiRemoveZeroPage(MI_GET_NEXT_COLOR());
            ASSERT(PageFrameIndex);
            TempPde.u.Hard.PageFrameNumber = PageFrameIndex;

            /* Initialize its PFN entry, with the parent system page directory page table */
            MiInitializePfnForOtherProcess(PageFrameIndex,
                                           PointerPde,
                                           MmSystemPageDirectory[(PointerPde - MiAddressToPde(NULL)) / PDE_COUNT]);

            /* Make the system PDE entry valid */
            MI_WRITE_VALID_PTE(SystemMapPde, TempPde);
            
            /* The system PDE entry might be the PDE itself, so check for this */
            if (PointerPde->u.Hard.Valid == 0)
            {
                /* It's different, so make the real PDE valid too */
                MI_WRITE_VALID_PTE(PointerPde, TempPde);
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
    Buckets = *ViewSize / MI_SYSTEM_VIEW_BUCKET_SIZE;
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

    /* Create the PDEs needed for this mapping, and double-map them if needed */
    MiFillSystemPageDirectory(Base, Buckets * MI_SYSTEM_VIEW_BUCKET_SIZE);

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
MiCreatePagingFileMap(OUT PSEGMENT *Segment,
                      IN PSIZE_T MaximumSize,
                      IN ULONG ProtectionMask,
                      IN ULONG AllocationAttributes)
{
    SIZE_T SizeLimit;
    PFN_NUMBER PteCount;
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
    PteCount = (*MaximumSize + PAGE_SIZE - 1) >> PAGE_SHIFT;

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
    RtlFillMemoryUlong(PointerPte, PteCount * sizeof(MMPTE), TempPte.u.Long);
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
    PSEGMENT NewSegment;
    NTSTATUS Status;
    PCONTROL_AREA ControlArea;
    ULONG ProtectionMask;

    /* ARM3 does not yet support this */
    ASSERT(FileHandle == NULL);
    ASSERT(FileObject == NULL);
    ASSERT((AllocationAttributes & SEC_LARGE_PAGES) == 0);

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

/* SYSTEM CALLS ***************************************************************/

NTSTATUS
NTAPI
NtAreMappedFilesTheSame(IN PVOID File1MappedAsAnImage,
                        IN PVOID File2MappedAsFile)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
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
        DPRINT1("Bogus allocation attribute: %lx\n", AllocationAttributes);
        return STATUS_INVALID_PARAMETER_6;
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

    /* Check if this is an image for the current process */
    if ((Section->AllocationAttributes & SEC_IMAGE) &&
        (Process == PsGetCurrentProcess()) &&
        ((Status != STATUS_IMAGE_NOT_AT_BASE) ||
         (Status != STATUS_CONFLICTING_ADDRESSES)))
    {
        /* Notify the debugger */
        DbgkMapViewOfSection(Section,
                             SafeBaseAddress,
                             SafeSectionOffset.LowPart,
                             SafeViewSize);
    }
    
    /* Return data only on success */
    if (NT_SUCCESS(Status))
    {
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

/* PUBLIC FUNCTIONS ***********************************************************/

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

/* EOF */
