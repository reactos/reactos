/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

   extsect.c

Abstract:

    This module contains the routines which implement the
    NtExtendSection service.

Author:

    Lou Perazzoli (loup) 8-May-1990
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtExtendSection)
#pragma alloc_text(PAGE,MmExtendSection)
#endif

#if DBG
VOID
MiSubsectionConsistent(
    IN PSUBSECTION Subsection
    )
/*++

Routine Description:

    This function checks to ensure the subsection is consistent.

Arguments:

    Subsection - Supplies a pointer to the subsection to be checked.

Return Value:

    None.

--*/

{
    ULONG   Sectors;
    ULONG   FullPtes;

    //
    // Compare the disk sectors (4K units) to the PTE allocation
    //

    Sectors = Subsection->NumberOfFullSectors;
    if (Subsection->u.SubsectionFlags.SectorEndOffset) {
        Sectors += 1;
    }

    //
    // Calculate how many PTEs are needed to map this number of sectors.
    //

    FullPtes = Sectors >> (PAGE_SHIFT - MM4K_SHIFT);

    if (Sectors & ((1 << (PAGE_SHIFT - MM4K_SHIFT)) - 1)) {
        FullPtes += 1;
    }

    if (FullPtes != Subsection->PtesInSubsection) {
        DbgPrint("Mm: Subsection inconsistent (%x vs %x)\n",
            FullPtes,
            Subsection->PtesInSubsection);
        DbgBreakPoint();
    }
}
#endif


NTSTATUS
NtExtendSection(
    IN HANDLE SectionHandle,
    IN OUT PLARGE_INTEGER NewSectionSize
    )

/*++

Routine Description:

    This function extends the size of the specified section.  If
    the current size of the section is greater than or equal to the
    specified section size, the size is not updated.

Arguments:

    SectionHandle - Supplies an open handle to a section object.

    NewSectionSize - Supplies the new size for the section object.

Return Value:

    Returns the status

    TBS


--*/

{
    KPROCESSOR_MODE PreviousMode;
    PVOID Section;
    NTSTATUS Status;
    LARGE_INTEGER CapturedNewSectionSize;

    PAGED_CODE();

    //
    // Check to make sure the new section size is accessible.
    //

    PreviousMode = KeGetPreviousMode();

    if (PreviousMode != KernelMode) {

        try {

            ProbeForWrite (NewSectionSize,
                           sizeof(LARGE_INTEGER),
                           sizeof(ULONG ));

            CapturedNewSectionSize = *NewSectionSize;

        } except (EXCEPTION_EXECUTE_HANDLER) {

            //
            // If an exception occurs during the probe or capture
            // of the initial values, then handle the exception and
            // return the exception code as the status value.
            //

            return GetExceptionCode();
        }

    } else {

        CapturedNewSectionSize = *NewSectionSize;
    }

    //
    // Reference the section object.
    //

    Status = ObReferenceObjectByHandle ( SectionHandle,
                                         SECTION_EXTEND_SIZE,
                                         MmSectionObjectType,
                                         PreviousMode,
                                         (PVOID *)&Section,
                                         NULL );

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Make sure this section is backed by a file.
    //

    if (((PSECTION)Section)->Segment->ControlArea->FilePointer == NULL) {
        ObDereferenceObject (Section);
        return STATUS_SECTION_NOT_EXTENDED;
    }

    Status = MmExtendSection (Section, &CapturedNewSectionSize, FALSE);

    ObDereferenceObject (Section);

    //
    // Update the NewSectionSize field.
    //

    try {

        //
        // Return the captured section size.
        //

        *NewSectionSize = CapturedNewSectionSize;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        NOTHING;
    }

    return Status;
}

NTSTATUS
MmExtendSection (
    IN PVOID SectionToExtend,
    IN OUT PLARGE_INTEGER NewSectionSize,
    IN ULONG IgnoreFileSizeChecking
    )

/*++

Routine Description:

    This function extends the size of the specified section.  If
    the current size of the section is greater than or equal to the
    specified section size, the size is not updated.

Arguments:

    Section - Supplies a pointer to a referenced section object.

    NewSectionSize - Supplies the new size for the section object.

    IgnoreFileSizeChecking -  Supplies the value TRUE is file size
                              checking should be ignored (i.e., it
                              is being called from a file system which
                              has already done the checks).  FALSE
                              if the checks still need to be made.

Return Value:

    Returns the status

    TBS


--*/

{
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE ExtendedPtes;
    MMPTE TempPte;
    PCONTROL_AREA ControlArea;
    PSECTION Section;
    PSUBSECTION LastSubsection;
    PSUBSECTION ExtendedSubsection;
    ULONG RequiredPtes;
    ULONG NumberOfPtes;
    ULONG PtesUsed;
    ULONG AllocationSize;
    UINT64 EndOfFile;
    UINT64 NumberOfPtesForEntireFile;
    NTSTATUS Status;
    LARGE_INTEGER NumberOf4KsForEntireFile;
    LARGE_INTEGER Starting4K;
    LARGE_INTEGER Last4KChunk;

    PAGED_CODE();

    Section = (PSECTION)SectionToExtend;

    //
    // Make sure the section is really extendable - physical and
    // image sections are not.
    //

    ControlArea = Section->Segment->ControlArea;

    if ((ControlArea->u.Flags.PhysicalMemory || ControlArea->u.Flags.Image) ||
         (ControlArea->FilePointer == NULL)) {
        return STATUS_SECTION_NOT_EXTENDED;
    }

    //
    // Acquire the section extension mutex, this blocks other threads from
    // updating the size at the same time.
    //

    KeEnterCriticalRegion ();
    ExAcquireResourceExclusive (&MmSectionExtendResource, TRUE);

    //
    // Each subsection is limited to 16TB - 64K because the NumberOfFullSectors
    // and various other fields in the subsection are ULONGs.  For NT64, the
    // allocation could be split into multiple subsections as needed to
    // conform to this limit - this is not worth doing for NT32 unless
    // sparse prototype PTE allocations are supported.
    //

    // This must be a multiple of the size of prototype pte allocation so any
    // given prototype pte allocation will have the same subsection for all
    // PTEs.
    //
    // The total section size is limited to 16PB - 4K because of the
    // StartingSector4132 field in each subsection.
    //

    NumberOfPtesForEntireFile = (NewSectionSize->QuadPart + PAGE_SIZE - 1) >> PAGE_SHIFT;

    NumberOfPtes = (ULONG)NumberOfPtesForEntireFile;

    if (NewSectionSize->QuadPart > MI_MAXIMUM_SECTION_SIZE) {
        Status = STATUS_SECTION_TOO_BIG;
        goto ReleaseAndReturn;
    }

    if (NumberOfPtesForEntireFile > (UINT64)((MAXULONG_PTR / sizeof(MMPTE)) - sizeof (SEGMENT))) {
        Status = STATUS_SECTION_TOO_BIG;
        goto ReleaseAndReturn;
    }

    if (NumberOfPtesForEntireFile > (UINT64)NewSectionSize->QuadPart) {
        Status = STATUS_SECTION_TOO_BIG;
        goto ReleaseAndReturn;
    }

    if (ControlArea->u.Flags.WasPurged == 0) {

        if ((UINT64)NewSectionSize->QuadPart <= (UINT64)Section->SizeOfSection.QuadPart) {
            *NewSectionSize = Section->SizeOfSection;
            goto ReleaseAndReturnSuccess;
        }
    }

    //
    // If a file handle was specified, set the allocation size of the file.
    //

    if (IgnoreFileSizeChecking == FALSE) {

        //
        // Release the resource so we don't deadlock with the file
        // system trying to extend this section at the same time.
        //

        ExReleaseResource (&MmSectionExtendResource);

        //
        // Get a different resource to single thread query/set operations.
        //

        ExAcquireResourceExclusive (&MmSectionExtendSetResource, TRUE);

        //
        // Query the file size to see if this file really needs extending.
        //
        // If the specified size is less than the current size, return
        // the current size.
        //

        Status = FsRtlGetFileSize (ControlArea->FilePointer,
                                   (PLARGE_INTEGER)&EndOfFile);

        if (!NT_SUCCESS (Status)) {
            ExReleaseResource (&MmSectionExtendSetResource);
            KeLeaveCriticalRegion ();
            return Status;
        }

        if ((UINT64)NewSectionSize->QuadPart > EndOfFile) {

            //
            // Don't allow section extension unless the section was originally
            // created with write access.  The check couldn't be done at create
            // time without breaking existing binaries, so the caller gets the
            // error at this point instead.
            //

            if (((Section->InitialPageProtection & PAGE_READWRITE) |
                (Section->InitialPageProtection & PAGE_EXECUTE_READWRITE)) == 0) {
#if DBG
                    DbgPrint("Section extension failed %x\n", Section);
#endif
                    ExReleaseResource (&MmSectionExtendSetResource);
                    KeLeaveCriticalRegion ();
                    return STATUS_SECTION_NOT_EXTENDED;
            }

            //
            // Current file is smaller, attempt to set a new end of file.
            //

            EndOfFile = *(PUINT64)NewSectionSize;

            Status = FsRtlSetFileSize (ControlArea->FilePointer,
                                       (PLARGE_INTEGER)&EndOfFile);

            if (!NT_SUCCESS (Status)) {
                ExReleaseResource (&MmSectionExtendSetResource);
                KeLeaveCriticalRegion ();
                return Status;
            }
        }

        if (ControlArea->Segment->ExtendInfo) {
            ExAcquireFastMutex (&MmSectionBasedMutex);
            if (ControlArea->Segment->ExtendInfo) {
                ControlArea->Segment->ExtendInfo->CommittedSize = EndOfFile;
            }
            ExReleaseFastMutex (&MmSectionBasedMutex);
        }

        //
        // Release the query/set resource and reacquire the extend section
        // resource.
        //

        ExReleaseResource (&MmSectionExtendSetResource);
        ExAcquireResourceExclusive (&MmSectionExtendResource, TRUE);
    }

    //
    // Find the last subsection.
    //

    ASSERT (ControlArea->u.Flags.GlobalOnlyPerSession == 0);

    LastSubsection = (PSUBSECTION)(ControlArea + 1);

    while (LastSubsection->NextSubsection != NULL ) {
        ASSERT (LastSubsection->UnusedPtes == 0);
        LastSubsection = LastSubsection->NextSubsection;
    }

#if DBG
    MiSubsectionConsistent(LastSubsection);
#endif

    //
    // Does the structure need extending?
    //

    if (NumberOfPtes <= Section->Segment->TotalNumberOfPtes) {

        //
        // The segment is already large enough, just update
        // the section size and return.
        //

        Section->SizeOfSection = *NewSectionSize;
        if (Section->Segment->SizeOfSegment < (UINT64)NewSectionSize->QuadPart) {
            //
            // Only update if it is really bigger.
            //

            Section->Segment->SizeOfSegment = *(PUINT64)NewSectionSize;

            Mi4KStartFromSubsection(&Starting4K, LastSubsection);

            Last4KChunk.QuadPart = (NewSectionSize->QuadPart >> MM4K_SHIFT) - Starting4K.QuadPart;

            ASSERT (Last4KChunk.HighPart == 0);

            LastSubsection->NumberOfFullSectors = Last4KChunk.LowPart;
            LastSubsection->u.SubsectionFlags.SectorEndOffset =
                                        NewSectionSize->LowPart & MM4K_MASK;
#if DBG
            MiSubsectionConsistent(LastSubsection);
#endif
        }
        goto ReleaseAndReturnSuccess;
    }

    //
    // Add new structures to the section - locate the last subsection
    // and add there.
    //

    RequiredPtes = NumberOfPtes - Section->Segment->TotalNumberOfPtes;
    PtesUsed = 0;

    if (RequiredPtes < LastSubsection->UnusedPtes) {

        //
        // There are ample PTEs to extend the section already allocated.
        //

        PtesUsed = RequiredPtes;
        RequiredPtes = 0;

    } else {
        PtesUsed = LastSubsection->UnusedPtes;
        RequiredPtes -= PtesUsed;
    }

    LastSubsection->PtesInSubsection += PtesUsed;
    LastSubsection->UnusedPtes -= PtesUsed;
    ControlArea->Segment->SizeOfSegment += (ULONG_PTR)PtesUsed * PAGE_SIZE;
    ControlArea->Segment->TotalNumberOfPtes += PtesUsed;

    if (RequiredPtes == 0) {

        //
        // There is no extension necessary, update the high VBN.
        //

        Mi4KStartFromSubsection(&Starting4K, LastSubsection);

        Last4KChunk.QuadPart = (NewSectionSize->QuadPart >> MM4K_SHIFT) - Starting4K.QuadPart;

        ASSERT (Last4KChunk.HighPart == 0);

        LastSubsection->NumberOfFullSectors = Last4KChunk.LowPart;

        LastSubsection->u.SubsectionFlags.SectorEndOffset =
                                    NewSectionSize->LowPart & MM4K_MASK;
#if DBG
        MiSubsectionConsistent(LastSubsection);
#endif
    } else {

        //
        // An extension is required.  Allocate paged pool
        // and populate it with prototype PTEs.
        //

        AllocationSize = (ULONG) ROUND_TO_PAGES (RequiredPtes * sizeof(MMPTE));

        ExtendedPtes = (PMMPTE)ExAllocatePoolWithTag (PagedPool,
                                                      AllocationSize,
                                                      'ppmM');

        if (ExtendedPtes == NULL) {

            //
            // The required pool could not be allocated.  Reset
            // the subsection and control area fields to their
            // original values.
            //

            LastSubsection->PtesInSubsection -= PtesUsed;
            LastSubsection->UnusedPtes += PtesUsed;
            ControlArea->Segment->TotalNumberOfPtes -= PtesUsed;
            ControlArea->Segment->SizeOfSegment -= ((ULONG_PTR)PtesUsed * PAGE_SIZE);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ReleaseAndReturn;
        }

        //
        // Allocate an extended subsection descriptor.
        //

        ExtendedSubsection = (PSUBSECTION)ExAllocatePoolWithTag (NonPagedPool,
                                                     sizeof(SUBSECTION),
                                                     'bSmM'
                                                     );
        if (ExtendedSubsection == NULL) {

            //
            // The required pool could not be allocated.  Reset
            // the subsection and control area fields to their
            // original values.
            //

            LastSubsection->PtesInSubsection -= PtesUsed;
            LastSubsection->UnusedPtes += PtesUsed;
            ControlArea->Segment->TotalNumberOfPtes -= PtesUsed;
            ControlArea->Segment->SizeOfSegment -= ((ULONG_PTR)PtesUsed * PAGE_SIZE);
            ExFreePool (ExtendedPtes);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ReleaseAndReturn;
        }

        ControlArea->NonPagedPoolUsage += EX_REAL_POOL_USAGE(sizeof(SUBSECTION));
        ControlArea->PagedPoolUsage += AllocationSize;
    
        ASSERT (ControlArea->DereferenceList.Flink == NULL);

        NumberOf4KsForEntireFile.QuadPart =
            ControlArea->Segment->SizeOfSegment >> MM4K_SHIFT;

        Mi4KStartFromSubsection(&Starting4K, LastSubsection);

        Last4KChunk.QuadPart = NumberOf4KsForEntireFile.QuadPart -
                                   Starting4K.QuadPart;

        if (LastSubsection->u.SubsectionFlags.SectorEndOffset) {
            Last4KChunk.QuadPart += 1;
        }

        ASSERT(Last4KChunk.HighPart == 0);

        LastSubsection->NumberOfFullSectors = Last4KChunk.LowPart;
        LastSubsection->u.SubsectionFlags.SectorEndOffset = 0;

        //
        // If the number of sectors doesn't completely fill the PTEs (this can
        // only happen when the page size is not MM4K), then fill it now.
        //

        if (LastSubsection->NumberOfFullSectors & ((1 << (PAGE_SHIFT - MM4K_SHIFT)) - 1)) {
            LastSubsection->NumberOfFullSectors += 1;
        }

#if DBG
        MiSubsectionConsistent(LastSubsection);
#endif

        ExtendedSubsection->u.LongFlags = 0;
        ExtendedSubsection->NextSubsection = NULL;
        ExtendedSubsection->UnusedPtes = (AllocationSize / sizeof(MMPTE)) -
                                                    RequiredPtes;

        ExtendedSubsection->ControlArea = ControlArea;
        ExtendedSubsection->PtesInSubsection = RequiredPtes;

        Starting4K.QuadPart += LastSubsection->NumberOfFullSectors;

        Mi4KStartForSubsection(&Starting4K, ExtendedSubsection);

        Last4KChunk.QuadPart =
            (NewSectionSize->QuadPart >> MM4K_SHIFT) - Starting4K.QuadPart;

        ASSERT(Last4KChunk.HighPart == 0);

        ExtendedSubsection->NumberOfFullSectors = Last4KChunk.LowPart;
        ExtendedSubsection->u.SubsectionFlags.SectorEndOffset =
                                    NewSectionSize->LowPart & MM4K_MASK;

#if DBG
        MiSubsectionConsistent(ExtendedSubsection);
#endif

        ExtendedSubsection->SubsectionBase = ExtendedPtes;

        PointerPte = ExtendedPtes;
        LastPte = ExtendedPtes + (AllocationSize / sizeof(MMPTE));

        if (ControlArea->FilePointer != NULL) {
            TempPte.u.Long = MiGetSubsectionAddressForPte(ExtendedSubsection);
        }
#if DBG
        else {
            DbgPrint("MM: Extend with no control area file pointer %x %x\n",
                ExtendedSubsection, ControlArea);
            DbgBreakPoint();
        }
#endif

        TempPte.u.Soft.Protection = ControlArea->Segment->SegmentPteTemplate.u.Soft.Protection;
        TempPte.u.Soft.Prototype = 1;
        ExtendedSubsection->u.SubsectionFlags.Protection =
            MI_GET_PROTECTION_FROM_SOFT_PTE(&TempPte);

        while (PointerPte < LastPte) {
            MI_WRITE_INVALID_PTE (PointerPte, TempPte);
            PointerPte += 1;
        }

        //
        // Link this into the list.
        //

        LastSubsection->NextSubsection = ExtendedSubsection;
        ControlArea->Segment->TotalNumberOfPtes += RequiredPtes;

#if defined(_ALPHA_) && !defined(NT_UP)
        //
        // A memory barrier is required here to synchronize with
        // NtMapViewOfSection, which validates the specified offset against
        // the section object without holding lock synchronization.
        // This memory barrier forces the subsection chaining to be correct
        // before increasing the size in the section object.
        //
        __MB();
#endif

    }

    ControlArea->Segment->SizeOfSegment = *(PUINT64)NewSectionSize;
    Section->SizeOfSection = *NewSectionSize;

ReleaseAndReturnSuccess:

    Status = STATUS_SUCCESS;

ReleaseAndReturn:

    ExReleaseResource (&MmSectionExtendResource);
    KeLeaveCriticalRegion ();

    return Status;
}

PMMPTE
FASTCALL
MiGetProtoPteAddressExtended (
    IN PMMVAD Vad,
    IN ULONG_PTR Vpn
    )

/*++

Routine Description:

    This function calculates the address of the prototype PTE
    for the corresponding virtual address.

Arguments:


    Vad - Supplies a pointer to the virtual address desciptor which
          encompasses the virtual address.

    Vpn - Supplies the virtual page number to locate a prototype PTE
                     for.

Return Value:

    The corresponding prototype PTE address.

--*/

{
    PSUBSECTION Subsection;
    PCONTROL_AREA ControlArea;
    ULONG PteOffset;

    ControlArea = Vad->ControlArea;

    if (ControlArea->u.Flags.GlobalOnlyPerSession == 0) {
        Subsection = (PSUBSECTION)(ControlArea + 1);
    }
    else {
        Subsection = (PSUBSECTION)((PLARGE_CONTROL_AREA)ControlArea + 1);
    }

    //
    // Locate the subsection which contains the First Prototype PTE
    // for this VAD.
    //

    while ((Vad->FirstPrototypePte < Subsection->SubsectionBase) ||
           (Vad->FirstPrototypePte >=
               &Subsection->SubsectionBase[Subsection->PtesInSubsection])) {

        //
        // Get the next subsection.
        //

        Subsection = Subsection->NextSubsection;
        if (Subsection == NULL) {
            return NULL;
        }
    }

    //
    // How many PTEs beyond this subsection must we go?
    //

    PteOffset = (ULONG) (((Vpn - Vad->StartingVpn) +
                 (ULONG)(Vad->FirstPrototypePte - Subsection->SubsectionBase)) -
                 Subsection->PtesInSubsection);

// DbgPrint("map extended subsection offset = %lx\n",PteOffset);

    ASSERT (PteOffset < 0xF0000000);

    PteOffset += Subsection->PtesInSubsection;

    //
    // Locate the subsection which contains the prototype PTEs.
    //

    while (PteOffset >= Subsection->PtesInSubsection) {
        PteOffset -= Subsection->PtesInSubsection;
        Subsection = Subsection->NextSubsection;
        if (Subsection == NULL) {
            return NULL;
        }
    }

    //
    // The PTEs are in this subsection.
    //

    ASSERT (PteOffset < Subsection->PtesInSubsection);

    return &Subsection->SubsectionBase[PteOffset];

}

PSUBSECTION
FASTCALL
MiLocateSubsection (
    IN PMMVAD Vad,
    IN ULONG_PTR Vpn
    )

/*++

Routine Description:

    This function calculates the address of the subsection
    for the corresponding virtual address.

    This function only works for mapped files NOT mapped images.

Arguments:


    Vad - Supplies a pointer to the virtual address desciptor which
          encompasses the virtual address.

    Vpn - Supplies the virtual page number to locate a prototype PTE
                     for.

Return Value:

    The corresponding prototype subsection.

--*/

{
    PSUBSECTION Subsection;
    PCONTROL_AREA ControlArea;
    ULONG PteOffset;

    ControlArea = Vad->ControlArea;

    if (ControlArea->u.Flags.GlobalOnlyPerSession == 0) {
        Subsection = (PSUBSECTION)(ControlArea + 1);
    }
    else {
        Subsection = (PSUBSECTION)((PLARGE_CONTROL_AREA)ControlArea + 1);
    }

    Subsection = (PSUBSECTION)(ControlArea + 1);

#if 0
    if (Subsection->NextSubsection == NULL) {

        //
        // There is only one subsection, don't look any further.
        //

        return Subsection;
    }
#endif //0

    if (ControlArea->u.Flags.Image) {

        //
        // There is only one subsection, don't look any further.
        //

        return Subsection;
    }

    //
    // Locate the subsection which contains the First Prototype PTE
    // for this VAD.
    //

    while ((Vad->FirstPrototypePte < Subsection->SubsectionBase) ||
           (Vad->FirstPrototypePte >=
               &Subsection->SubsectionBase[Subsection->PtesInSubsection])) {

        //
        // Get the next subsection.
        //

        Subsection = Subsection->NextSubsection;
        if (Subsection == NULL) {
            return NULL;
        }
    }

    //
    // How many PTEs beyond this subsection must we go?
    //

    PteOffset = (ULONG)((Vpn - Vad->StartingVpn) +
         (ULONG)(Vad->FirstPrototypePte - Subsection->SubsectionBase));

    ASSERT (PteOffset < 0xF0000000);

    //
    // Locate the subsection which contains the prototype PTEs.
    //

    while (PteOffset >= Subsection->PtesInSubsection) {
        PteOffset -= Subsection->PtesInSubsection;
        Subsection = Subsection->NextSubsection;
        if (Subsection == NULL) {
            return NULL;
        }
    }

    //
    // The PTEs are in this subsection.
    //

    return Subsection;
}
