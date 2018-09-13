/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   mapview.c

Abstract:

    This module contains the routines which implement the
    NtMapViewOfSection service.

Author:

    Lou Perazzoli (loup) 22-May-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

ULONG MMPPTE_NAME = 'tPmM'; //MmPt
ULONG MMDB = 'bDmM';
extern ULONG MMVADKEY;

VOID
MiSetPageModified (
    IN PVOID Address
    );

extern LIST_ENTRY MmLoadedUserImageList;

#define X256MEG (256*1024*1024)

#if DBG
extern PEPROCESS MmWatchProcess;
#endif // DBG

#define ROUND_TO_PAGES64(Size)  (((UINT64)(Size) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

MMSESSION   MmSession;

NTSTATUS
MiMapViewOfImageSection (
    IN PCONTROL_AREA ControlArea,
    IN PEPROCESS Process,
    IN PVOID *CapturedBase,
    IN PLARGE_INTEGER SectionOffset,
    IN PSIZE_T CapturedViewSize,
    IN PSECTION Section,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG_PTR ZeroBits,
    IN SIZE_T ImageCommitment,
    OUT PBOOLEAN ReleasedWsMutex
    );

NTSTATUS
MiMapViewOfDataSection (
    IN PCONTROL_AREA ControlArea,
    IN PEPROCESS Process,
    IN PVOID *CapturedBase,
    IN PLARGE_INTEGER SectionOffset,
    IN PSIZE_T CapturedViewSize,
    IN PSECTION Section,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG ProtectionMask,
    IN SIZE_T CommitSize,
    IN ULONG_PTR ZeroBits,
    IN ULONG AllocationType,
    OUT PBOOLEAN ReleasedWsMutex
    );

VOID
MiRemoveMappedPtes (
    IN PVOID BaseAddress,
    IN ULONG NumberOfPtes,
    IN PCONTROL_AREA ControlArea,
    IN PMMSUPPORT WorkingSetInfo
    );

NTSTATUS
MiMapViewInSystemSpace (
    IN PVOID Section,
    IN PMMSESSION Session,
    OUT PVOID *MappedBase,
    IN OUT PSIZE_T ViewSize
    );

NTSTATUS
MiUnmapViewInSystemSpace (
    IN PMMSESSION Session,
    IN PVOID MappedBase
    );

BOOLEAN
MiFillSystemPageDirectory (
    PVOID Base,
    SIZE_T NumberOfBytes
    );

VOID
VadTreeWalk (
    PMMVAD Start
    );

#if DBG
VOID
MiDumpConflictingVad(
    IN PVOID StartingAddress,
    IN PVOID EndingAddress,
    IN PMMVAD Vad
    );


VOID
MiDumpConflictingVad(
    IN PVOID StartingAddress,
    IN PVOID EndingAddress,
    IN PMMVAD Vad
    )
{
    if (NtGlobalFlag & FLG_SHOW_LDR_SNAPS) {
        DbgPrint( "MM: [%p ... %p) conflicted with Vad %p\n",
                  StartingAddress, EndingAddress, Vad);
        if ((Vad->u.VadFlags.PrivateMemory == 1) ||
            (Vad->ControlArea == NULL)) {
            return;
        }
        if (Vad->ControlArea->u.Flags.Image)
            DbgPrint( "    conflict with %Z image at [%p .. %p)\n",
                      &Vad->ControlArea->FilePointer->FileName,
                      MI_VPN_TO_VA (Vad->StartingVpn),
                      MI_VPN_TO_VA_ENDING (Vad->EndingVpn)
                    );
        else
        if (Vad->ControlArea->u.Flags.File)
            DbgPrint( "    conflict with %Z file at [%p .. %p)\n",
                      &Vad->ControlArea->FilePointer->FileName,
                      MI_VPN_TO_VA (Vad->StartingVpn),
                      MI_VPN_TO_VA_ENDING (Vad->EndingVpn)
                    );
        else
            DbgPrint( "    conflict with section at [%p .. %p)\n",
                      MI_VPN_TO_VA (Vad->StartingVpn),
                      MI_VPN_TO_VA_ENDING (Vad->EndingVpn)
                    );
    }
}
#endif //DBG


ULONG
CacheImageSymbols(
    IN PVOID ImageBase
    );

PVOID
MiInsertInSystemSpace (
    IN PMMSESSION Session,
    IN ULONG SizeIn64k,
    IN PCONTROL_AREA ControlArea
    );

ULONG
MiRemoveFromSystemSpace (
    IN PMMSESSION Session,
    IN PVOID Base,
    OUT PCONTROL_AREA *ControlArea
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtMapViewOfSection)
#pragma alloc_text(PAGE,MmMapViewOfSection)
#pragma alloc_text(PAGE,MmSecureVirtualMemory)
#pragma alloc_text(PAGE,MmUnsecureVirtualMemory)
#pragma alloc_text(PAGE,CacheImageSymbols)
#pragma alloc_text(PAGE,NtAreMappedFilesTheSame)

#pragma alloc_text(PAGELK,MiMapViewOfPhysicalSection)
#pragma alloc_text(PAGELK,MiMapViewInSystemSpace)
#pragma alloc_text(PAGELK,MmMapViewInSystemSpace)
#pragma alloc_text(PAGELK,MmMapViewInSessionSpace)
#pragma alloc_text(PAGELK,MiUnmapViewInSystemSpace)
#pragma alloc_text(PAGELK,MmUnmapViewInSystemSpace)
#pragma alloc_text(PAGELK,MmUnmapViewInSessionSpace)
#pragma alloc_text(PAGELK,MiFillSystemPageDirectory)
#pragma alloc_text(PAGELK,MiInsertInSystemSpace)
#pragma alloc_text(PAGELK,MiRemoveFromSystemSpace)

#pragma alloc_text(PAGEHYDRA,MiInitializeSystemSpaceMap)
#pragma alloc_text(PAGEHYDRA, MiFreeSessionSpaceMap)
#endif


NTSTATUS
NtMapViewOfSection(
    IN HANDLE SectionHandle,
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG_PTR ZeroBits,
    IN SIZE_T CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
    IN OUT PSIZE_T ViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG Protect
    )

/*++

Routine Description:

    This function maps a view in the specified subject process to
    the section object.

Arguments:

    SectionHandle - Supplies an open handle to a section object.

    ProcessHandle - Supplies an open handle to a process object.

    BaseAddress - Supplies a pointer to a variable that will receive
                  the base address of the view. If the initial value
                  of this argument is not null, then the view will
                  be allocated starting at the specified virtual
                  address rounded down to the next 64kb address
                  boundary. If the initial value of this argument is
                  null, then the operating system will determine
                  where to allocate the view using the information
                  specified by the ZeroBits argument value and the
                  section allocation attributes (i.e. based and
                  tiled).
        
    ZeroBits - Supplies the number of high order address bits that
               must be zero in the base address of the section
               view. The value of this argument must be less than
               or equal to the maximum number of zero bits and is only
               used when memory management determines where to allocate
               the view (i.e. when BaseAddress is null).
        
               If ZeroBits is zero, then no zero bit constraints are applied.

               If ZeroBits is greater than 0 and less than 32, then it is
               the number of leading zero bits from bit 31.  Bits 63:32 are
               also required to be zero.  This retains compatibility
               with 32-bit systems.
                
               If ZeroBits is greater than 32, then it is considered as
               a mask and then number of leading zero are counted out
               in the mask.  This then becomes the zero bits argument.

    CommitSize - Supplies the size of the initially committed region
                 of the view in bytes. This value is rounded up to
                 the next host page size boundary.

    SectionOffset - Supplies the offset from the beginning of the
                    section to the view in bytes. This value is
                    rounded down to the next host page size boundary.

    ViewSize - Supplies a pointer to a variable that will receive
               the actual size in bytes of the view. If the value
               of this argument is zero, then a view of the
               section will be mapped starting at the specified
               section offset and continuing to the end of the
               section. Otherwise the initial value of this
               argument specifies the size of the view in bytes
               and is rounded up to the next host page size
               boundary.
        
    InheritDisposition - Supplies a value that specifies how the
                         view is to be shared by a child process created
                         with a create process operation.

        InheritDisposition Values

         ViewShare - Inherit view and share a single copy
              of the committed pages with a child process
              using the current protection value.

         ViewUnmap - Do not map the view into a child process.

    AllocationType - Supplies the type of allocation.

         MEM_TOP_DOWN
         MEM_DOS_LIM
         MEM_LARGE_PAGES
         MEM_RESERVE - for file mapped sections only.

    Protect - Supplies the protection desired for the region of
              initially committed pages.

        Protect Values


         PAGE_NOACCESS - No access to the committed region
              of pages is allowed. An attempt to read,
              write, or execute the committed region
              results in an access violation (i.e. a GP
              fault).

         PAGE_EXECUTE - Execute access to the committed
              region of pages is allowed. An attempt to
              read or write the committed region results in
              an access violation.

         PAGE_READONLY - Read only and execute access to the
              committed region of pages is allowed. An
              attempt to write the committed region results
              in an access violation.

         PAGE_READWRITE - Read, write, and execute access to
              the region of committed pages is allowed. If
              write access to the underlying section is
              allowed, then a single copy of the pages are
              shared. Otherwise the pages are shared read
              only/copy on write.

Return Value:

    Various NTSTATUS codes.

--*/

{
    PSECTION Section;
    PEPROCESS Process;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    PVOID CapturedBase;
    SIZE_T CapturedViewSize;
    LARGE_INTEGER TempViewSize;
    LARGE_INTEGER CapturedOffset;
    ULONGLONG HighestPhysicalAddressInPfnDatabase;
    ACCESS_MASK DesiredSectionAccess;
    ULONG ProtectMaskForAccess;
    BOOLEAN WriteCombined;

    PAGED_CODE();

    //
    // Check the zero bits argument for correctness.
    //

#if defined (_WIN64)

    if (ZeroBits >= 32) {

        //
        // ZeroBits is a mask instead of a count.  Translate it to a count now.
        //

        ZeroBits = 64 - RtlFindMostSignificantBit (ZeroBits);        
    }
    else if (ZeroBits) {
        ZeroBits += 32;
    }

#endif

    if (ZeroBits > MM_MAXIMUM_ZERO_BITS) {
        return STATUS_INVALID_PARAMETER_4;
    }

    //
    // Check the inherit disposition flags.
    //

    if ((InheritDisposition > ViewUnmap) ||
        (InheritDisposition < ViewShare)) {
        return STATUS_INVALID_PARAMETER_8;
    }

    //
    // Check the allocation type field.
    //

#ifdef i386

    //
    // Only allow DOS_LIM support for i386.  The MEM_DOS_LIM flag allows
    // map views of data sections to be done on 4k boundaries rather
    // than 64k boundaries.
    //

    if ((AllocationType & ~(MEM_TOP_DOWN | MEM_LARGE_PAGES | MEM_DOS_LIM |
           SEC_NO_CHANGE | MEM_RESERVE)) != 0) {
        return STATUS_INVALID_PARAMETER_9;
    }
#else
    if ((AllocationType & ~(MEM_TOP_DOWN | MEM_LARGE_PAGES |
           SEC_NO_CHANGE | MEM_RESERVE)) != 0) {
        return STATUS_INVALID_PARAMETER_9;
    }

#endif //i386

    //
    // Check the protection field.  This could raise an exception.
    //

    if (Protect & PAGE_WRITECOMBINE) {
        Protect &= ~PAGE_WRITECOMBINE;
        WriteCombined = TRUE;
    }
    else {
        WriteCombined = FALSE;
    }

    try {
        ProtectMaskForAccess = MiMakeProtectionMask (Protect) & 0x7;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    DesiredSectionAccess = MmMakeSectionAccess[ProtectMaskForAccess];

    PreviousMode = KeGetPreviousMode();

    //
    // Establish an exception handler, probe the specified addresses
    // for write access and capture the initial values.
    //

    try {
        if (PreviousMode != KernelMode) {
            ProbeForWritePointer ((PULONG)BaseAddress);
            ProbeForWriteUlong_ptr (ViewSize);

        }

        if (ARGUMENT_PRESENT (SectionOffset)) {
            if (PreviousMode != KernelMode) {
                ProbeForWrite (SectionOffset,
                               sizeof(LARGE_INTEGER),
                               sizeof(ULONG_PTR));
            }
            CapturedOffset = *SectionOffset;
        } else {
            ZERO_LARGE (CapturedOffset);
        }

        //
        // Capture the base address.
        //

        CapturedBase = *BaseAddress;

        //
        // Capture the region size.
        //

        CapturedViewSize = *ViewSize;

    } except (ExSystemExceptionFilter()) {

        //
        // If an exception occurs during the probe or capture
        // of the initial values, then handle the exception and
        // return the exception code as the status value.
        //

        return GetExceptionCode();
    }

#if DBG
    if (MmDebug & MM_DBG_SHOW_NT_CALLS) {
        if ( !MmWatchProcess ) {
            DbgPrint("mapview process handle %lx section %lx base address %p zero bits %lx\n",
                ProcessHandle, SectionHandle, CapturedBase, ZeroBits);
            DbgPrint("    view size %p offset %p commitsize %p  protect %lx\n",
                CapturedViewSize, CapturedOffset, CommitSize, Protect);
            DbgPrint("    Inheritdisp %lx  Allocation type %lx\n",
                InheritDisposition, AllocationType);
        }
    }
#endif

    //
    // Make sure the specified starting and ending addresses are
    // within the user part of the virtual address space.
    //

    if (CapturedBase > MM_HIGHEST_VAD_ADDRESS) {

        //
        // Invalid base address.
        //

        return STATUS_INVALID_PARAMETER_3;
    }

    if (((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS - (ULONG_PTR)CapturedBase) <
                                                        CapturedViewSize) {

        //
        // Invalid region size;
        //

        return STATUS_INVALID_PARAMETER_3;

    }

    if (((ULONG_PTR)CapturedBase + CapturedViewSize) > ((ULONG_PTR)MM_USER_ADDRESS_RANGE_LIMIT >> ZeroBits)) {

        //
        // Desired Base and zero_bits conflict.
        //

        return STATUS_INVALID_PARAMETER_4;
    }

    Status = ObReferenceObjectByHandle ( ProcessHandle,
                                         PROCESS_VM_OPERATION,
                                         PsProcessType,
                                         PreviousMode,
                                         (PVOID *)&Process,
                                         NULL );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Reference the section object, if a view is mapped to the section
    // object, the object is not dereferenced as the virtual address
    // descriptor contains a pointer to the section object.
    //

    Status = ObReferenceObjectByHandle ( SectionHandle,
                                         DesiredSectionAccess,
                                         MmSectionObjectType,
                                         PreviousMode,
                                         (PVOID *)&Section,
                                         NULL );

    if (!NT_SUCCESS(Status)) {
        goto ErrorReturn1;
    }

    if (Section->u.Flags.Image == 0) {

        //
        // This is not an image section, make sure the section page
        // protection is compatible with the specified page protection.
        //

        if (!MiIsProtectionCompatible (Section->InitialPageProtection,
                                       Protect)) {
            Status = STATUS_SECTION_PROTECTION;
            goto ErrorReturn;
        }
    }

    //
    // Check to see if this the section backs physical memory, if
    // so DON'T align the offset on a 64K boundary, just a 4k boundary.
    //

    if (Section->Segment->ControlArea->u.Flags.PhysicalMemory) {
        HighestPhysicalAddressInPfnDatabase = (ULONGLONG)MmHighestPhysicalPage << PAGE_SHIFT;
        CapturedOffset.LowPart = CapturedOffset.LowPart & ~(PAGE_SIZE - 1);

        //
        // No usermode mappings past the end of the PFN database are allowed.
        // Address wrap is checked in the common path.
        //

        if (PreviousMode != KernelMode) {

            if ((ULONGLONG)(CapturedOffset.QuadPart + CapturedViewSize) > HighestPhysicalAddressInPfnDatabase) {
                Status = STATUS_INVALID_PARAMETER_6;
                goto ErrorReturn;
            }
        }

    } else {

        //
        // Make sure alignments are correct for specified address
        // and offset into the file.
        //

        if ((AllocationType & MEM_DOS_LIM) == 0) {
            if (((ULONG_PTR)CapturedBase & (X64K - 1)) != 0) {
                Status = STATUS_MAPPED_ALIGNMENT;
                goto ErrorReturn;
            }

            if ((ARGUMENT_PRESENT (SectionOffset)) &&
                ((CapturedOffset.LowPart & (X64K - 1)) != 0)) {
                Status = STATUS_MAPPED_ALIGNMENT;
                goto ErrorReturn;
            }
        }
    }

    //
    // Check to make sure the view size plus the offset is less
    // than the size of the section.
    //

    if ((ULONGLONG) (CapturedOffset.QuadPart + CapturedViewSize) <
        (ULONGLONG)CapturedOffset.QuadPart) {

        Status = STATUS_INVALID_VIEW_SIZE;
        goto ErrorReturn;
    }

    if (((ULONGLONG) (CapturedOffset.QuadPart + CapturedViewSize) >
                 (ULONGLONG)Section->SizeOfSection.QuadPart) &&
        ((AllocationType & MEM_RESERVE) == 0)) {

        Status = STATUS_INVALID_VIEW_SIZE;
        goto ErrorReturn;
    }

    if (CapturedViewSize == 0) {

        //
        // Set the view size to be size of the section less the offset.
        //

        TempViewSize.QuadPart = Section->SizeOfSection.QuadPart -
                                                CapturedOffset.QuadPart;

        CapturedViewSize = (SIZE_T)TempViewSize.QuadPart;

        if (

#if !defined(_WIN64)

            (TempViewSize.HighPart != 0) ||

#endif

            (((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS - (ULONG_PTR)CapturedBase) <
                                                        CapturedViewSize)) {

            //
            // Invalid region size;
            //

            Status = STATUS_INVALID_VIEW_SIZE;
            goto ErrorReturn;
        }
    }

    //
    // Check commit size.
    //

    if ((CommitSize > CapturedViewSize) &&
        ((AllocationType & MEM_RESERVE) == 0)) {
        Status = STATUS_INVALID_PARAMETER_5;
        goto ErrorReturn;
    }

    if (WriteCombined == TRUE) {
        Protect |= PAGE_WRITECOMBINE;
    }

    Status = MmMapViewOfSection ( (PVOID)Section,
                                  Process,
                                  &CapturedBase,
                                  ZeroBits,
                                  CommitSize,
                                  &CapturedOffset,
                                  &CapturedViewSize,
                                  InheritDisposition,
                                  AllocationType,
                                  Protect);

    if (!NT_SUCCESS(Status) ) {
        if ( (Section->Segment->ControlArea->u.Flags.Image) &&
             Process == PsGetCurrentProcess() ) {
            if (Status == STATUS_CONFLICTING_ADDRESSES ) {
                DbgkMapViewOfSection(
                    SectionHandle,
                    CapturedBase,
                    CapturedOffset.LowPart,
                    CapturedViewSize
                    );
            }
        }
        goto ErrorReturn;
    }

    //
    // Anytime the current process maps an image file,
    // a potential debug event occurs. DbgkMapViewOfSection
    // handles these events.
    //

    if ( (Section->Segment->ControlArea->u.Flags.Image) &&
         Process == PsGetCurrentProcess() ) {
        if (Status != STATUS_IMAGE_NOT_AT_BASE ) {
            DbgkMapViewOfSection(
                SectionHandle,
                CapturedBase,
                CapturedOffset.LowPart,
                CapturedViewSize
                );
        }
    }

    //
    // Establish an exception handler and write the size and base
    // address.
    //

    try {

        *ViewSize = CapturedViewSize;
        *BaseAddress = CapturedBase;

        if (ARGUMENT_PRESENT(SectionOffset)) {
            *SectionOffset = CapturedOffset;
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        goto ErrorReturn;
    }

#if 0 // test code...
    if ((Status == STATUS_SUCCESS) &&
        (Section->u.Flags.Image == 0)) {

        PVOID Base;
        ULONG Size = 0;
        NTSTATUS Status;

        Status = MmMapViewInSystemSpace ((PVOID)Section,
                                        &Base,
                                        &Size);
        if (Status == STATUS_SUCCESS) {
            MmUnmapViewInSystemSpace (Base);
        }
    }
#endif //0

    {
ErrorReturn:
        ObDereferenceObject (Section);
ErrorReturn1:
        ObDereferenceObject (Process);
        return Status;
    }
}

NTSTATUS
MmMapViewOfSection(
    IN PVOID SectionToMap,
    IN PEPROCESS Process,
    IN OUT PVOID *CapturedBase,
    IN ULONG_PTR ZeroBits,
    IN SIZE_T CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset,
    IN OUT PSIZE_T CapturedViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG Protect
    )

/*++

Routine Description:

    This function maps a view in the specified subject process to
    the section object.

    This function is a kernel mode interface to allow LPC to map
    a section given the section pointer to map.

    This routine assumes all arguments have been probed and captured.

    ********************************************************************
    ********************************************************************
    ********************************************************************

    NOTE:

    CapturedViewSize, SectionOffset, and CapturedBase must be
    captured in non-paged system space (i.e., kernel stack).

    ********************************************************************
    ********************************************************************
    ********************************************************************

Arguments:

    SectionToMap - Supplies a pointer to the section object.

    Process - Supplies a pointer to the process object.

    BaseAddress - Supplies a pointer to a variable that will receive
         the base address of the view. If the initial value
         of this argument is not null, then the view will
         be allocated starting at the specified virtual
         address rounded down to the next 64kb address
         boundary. If the initial value of this argument is
         null, then the operating system will determine
         where to allocate the view using the information
         specified by the ZeroBits argument value and the
         section allocation attributes (i.e. based and
         tiled).

    ZeroBits - Supplies the number of high order address bits that
         must be zero in the base address of the section
         view. The value of this argument must be less than
         21 and is only used when the operating system
         determines where to allocate the view (i.e. when
         BaseAddress is null).

    CommitSize - Supplies the size of the initially committed region
         of the view in bytes. This value is rounded up to
         the next host page size boundary.

    SectionOffset - Supplies the offset from the beginning of the
         section to the view in bytes. This value is
         rounded down to the next host page size boundary.

    ViewSize - Supplies a pointer to a variable that will receive
         the actual size in bytes of the view. If the value
         of this argument is zero, then a view of the
         section will be mapped starting at the specified
         section offset and continuing to the end of the
         section. Otherwise the initial value of this
         argument specifies the size of the view in bytes
         and is rounded up to the next host page size
         boundary.

    InheritDisposition - Supplies a value that specifies how the
         view is to be shared by a child process created
         with a create process operation.

    AllocationType - Supplies the type of allocation.

    Protect - Supplies the protection desired for the region of
         initially committed pages.

Return Value:

    Returns the status

    TBS


--*/
{
    BOOLEAN Attached;
    PSECTION Section;
    PCONTROL_AREA ControlArea;
    ULONG ProtectionMask;
    NTSTATUS status;
    BOOLEAN ReleasedWsMutex;
    BOOLEAN WriteCombined;
    SIZE_T ImageCommitment;

    PAGED_CODE();

    Attached = FALSE;
    ReleasedWsMutex = TRUE;

    Section = (PSECTION)SectionToMap;

    //
    // Check to make sure the section is not smaller than the view size.
    //

    if ((LONGLONG)*CapturedViewSize > Section->SizeOfSection.QuadPart) {
        if ((AllocationType & MEM_RESERVE) == 0) {
            return STATUS_INVALID_VIEW_SIZE;
        }
    }

    if (AllocationType & MEM_RESERVE) {
        if (((Section->InitialPageProtection & PAGE_READWRITE) |
            (Section->InitialPageProtection & PAGE_EXECUTE_READWRITE)) == 0) {

            return STATUS_SECTION_PROTECTION;
        }
    }

    if (Section->u.Flags.NoCache) {
        Protect |= PAGE_NOCACHE;
    }

    //
    // Note that write combining is only relevant to physical memory sections
    // because they are never trimmed - the write combining bits in a PTE entry
    // are not preserved across trims.
    //

    if (Protect & PAGE_WRITECOMBINE) {
        Protect &= ~PAGE_WRITECOMBINE;
        WriteCombined = TRUE;
    }
    else {
        WriteCombined = FALSE;
    }

    //
    // Check the protection field.  This could raise an exception.
    //

    try {
        ProtectionMask = MiMakeProtectionMask (Protect);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    ControlArea = Section->Segment->ControlArea;
    ImageCommitment = Section->Segment->ImageCommitment;

    //
    // If the specified process is not the current process, attach
    // to the specified process.
    //

    if (PsGetCurrentProcess() != Process) {
        KeAttachProcess (&Process->Pcb);
        Attached = TRUE;
    }

    //
    // Get the address creation mutex to block multiple threads
    // creating or deleting address space at the same time.
    //

    LOCK_ADDRESS_SPACE (Process);

    //
    // Make sure the address space was not deleted, if so, return an error.
    //

    if (Process->AddressSpaceDeleted != 0) {
        status = STATUS_PROCESS_IS_TERMINATING;
        goto ErrorReturn;
    }

    //
    // Map the view base on the type.
    //

    ReleasedWsMutex = FALSE;

    if (ControlArea->u.Flags.PhysicalMemory) {

        MmLockPagableSectionByHandle(ExPageLockHandle);
        status = MiMapViewOfPhysicalSection (ControlArea,
                                             Process,
                                             CapturedBase,
                                             SectionOffset,
                                             CapturedViewSize,
                                             ProtectionMask,
                                             ZeroBits,
                                             AllocationType,
                                             WriteCombined,
                                             &ReleasedWsMutex);
        MmUnlockPagableImageSection(ExPageLockHandle);

    } else if (ControlArea->u.Flags.Image) {
        if (AllocationType & MEM_RESERVE) {
            status = STATUS_INVALID_PARAMETER_9;
        }
        else if (WriteCombined == TRUE) {
            status = STATUS_INVALID_PARAMETER_10;
        } else {

            status = MiMapViewOfImageSection (ControlArea,
                                              Process,
                                              CapturedBase,
                                              SectionOffset,
                                              CapturedViewSize,
                                              Section,
                                              InheritDisposition,
                                              ZeroBits,
                                              ImageCommitment,
                                              &ReleasedWsMutex);
        }

    } else {

        //
        // Not an image section, therefore it is a data section.
        //

        if (WriteCombined == TRUE) {
            status = STATUS_INVALID_PARAMETER_10;
        }
        else {
            status = MiMapViewOfDataSection (ControlArea,
                                         Process,
                                         CapturedBase,
                                         SectionOffset,
                                         CapturedViewSize,
                                         Section,
                                         InheritDisposition,
                                         ProtectionMask,
                                         CommitSize,
                                         ZeroBits,
                                         AllocationType,
                                         &ReleasedWsMutex
                                        );
        }
    }

ErrorReturn:
    if (!ReleasedWsMutex) {
        UNLOCK_WS_AND_ADDRESS_SPACE (Process);
    }
    else {
        UNLOCK_ADDRESS_SPACE (Process);
    }

    if (Attached) {
        KeDetachProcess();
    }

    return status;
}

#ifndef _ALPHA_

NTSTATUS
MiMapViewOfPhysicalSection (
    IN PCONTROL_AREA ControlArea,
    IN PEPROCESS Process,
    IN PVOID *CapturedBase,
    IN PLARGE_INTEGER SectionOffset,
    IN PSIZE_T CapturedViewSize,
    IN ULONG ProtectionMask,
    IN ULONG_PTR ZeroBits,
    IN ULONG AllocationType,
    IN BOOLEAN WriteCombined,
    OUT PBOOLEAN ReleasedWsMutex
    )

/*++

Routine Description:

    This routine maps the specified physical section into the
    specified process's address space.

Arguments:

    see MmMapViewOfSection above...

    ControlArea - Supplies the control area for the section.

    Process - Supplies the process pointer which is receiving the section.

    ProtectionMask - Supplies the initial page protection-mask.

    ReleasedWsMutex - Supplies FALSE. If the working set mutex is
                      not held when returning this must be set to TRUE
                      so the caller will not release the unheld mutex.

                      Note if this routine acquires the working set mutex
                      it is always done unsafely as the address space mutex
                      must be held on entry regardless.

Return Value:

    Status of the map view operation.

Environment:

    Kernel Mode, address creation mutex held.

--*/

{
    PMMVAD Vad;
    PVOID StartingAddress;
    PVOID EndingAddress;
    KIRQL OldIrql;
    KIRQL OldIrql2;
    PMMPTE PointerPpe;
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    MMPTE TempPte;
    PMMPFN Pfn2;
    SIZE_T PhysicalViewSize;
    ULONG_PTR Alignment;
    PVOID UsedPageTableHandle;
    PVOID UsedPageDirectoryHandle;
    PMI_PHYSICAL_VIEW PhysicalView;
#ifdef LARGE_PAGES
    ULONG size;
    PMMPTE protoPte;
    ULONG pageSize;
    PSUBSECTION Subsection;
    ULONG EmPageSize;
#endif //LARGE_PAGES

    //
    // Physical memory section.
    //

    //
    // If running on an R4000 and MEM_LARGE_PAGES is specified,
    // set up the PTEs as a series of  pointers to the same
    // prototype PTE.  This prototype PTE will reference a subsection
    // that indicates large pages should be used.
    //
    // The R4000 supports pages of 4k, 16k, 64k, etc (powers of 4).
    // Since the TB supports 2 entries, sizes of 8k, 32k etc can
    // be mapped by 2 LargePages in a single TB entry.  These 2 entries
    // are maintained in the subsection structure pointed to by the
    // prototype PTE.
    //

    if (AllocationType & MEM_RESERVE) {
        *ReleasedWsMutex = TRUE;
        return STATUS_INVALID_PARAMETER_9;
    }
    Alignment = X64K;
    LOCK_WS_UNSAFE (Process);

#ifdef LARGE_PAGES
    if (AllocationType & MEM_LARGE_PAGES) {

        //
        // Determine the page size and the required alignment.
        //

        if ((SectionOffset->LowPart & (X64K - 1)) != 0) {
            return STATUS_INVALID_PARAMETER_9;
        }

        size = (*CapturedViewSize - 1) >> (PAGE_SHIFT + 1);
        pageSize = PAGE_SIZE;

        while (size != 0) {
            size = size >> 2;
            pageSize = pageSize << 2;
        }

        Alignment = pageSize << 1;
        if (Alignment < MM_VA_MAPPED_BY_PDE) {
            Alignment = MM_VA_MAPPED_BY_PDE;
        }

#if defined(_IA64_)

        //
        // Convert pageSize to the EM page-size field format.
        //

        EmPageSize = 0;
        size = pageSize - 1 ;

        while (size) {
            size = size >> 1;
            EmPageSize += 1;
        }

        if (*CapturedViewSize > pageSize) {

            if (MmPageSizeInfo & (pageSize << 1)) {

                //
                // if larger page size is supported in the implementation
                //

                pageSize = pageSize << 1;
                EmPageSize += 1;

            }
            else {

                EmPageSize = EmPageSize | pageSize;

            }
        }

        pageSize = EmPageSize;
#endif

    }
#endif //LARGE_PAGES

    if (*CapturedBase == NULL) {

        //
        // Attempt to locate address space.  This could raise an
        // exception.
        //

        try {

            //
            // Find a starting address on a 64k boundary.
            //
#ifdef i386
            ASSERT (SectionOffset->HighPart == 0);
#endif

#ifdef LARGE_PAGES
            if (AllocationType & MEM_LARGE_PAGES) {
                PhysicalViewSize = Alignment;
            } else {
#endif //LARGE_PAGES

                PhysicalViewSize = *CapturedViewSize +
                                       (SectionOffset->LowPart & (X64K - 1));
#ifdef LARGE_PAGES
            }
#endif //LARGE_PAGES

            StartingAddress = MiFindEmptyAddressRange (PhysicalViewSize,
                                                       Alignment,
                                                       (ULONG)ZeroBits);

        } except (EXCEPTION_EXECUTE_HANDLER) {

            return GetExceptionCode();
        }

        EndingAddress = (PVOID)(((ULONG_PTR)StartingAddress +
                                PhysicalViewSize - 1L) | (PAGE_SIZE - 1L));
        StartingAddress = (PVOID)((ULONG_PTR)StartingAddress +
                                     (SectionOffset->LowPart & (X64K - 1)));

        if (ZeroBits > 0) {
            if (EndingAddress > (PVOID)((ULONG_PTR)MM_USER_ADDRESS_RANGE_LIMIT >> ZeroBits)) {
                return STATUS_NO_MEMORY;
            }
        }

    } else {

        //
        // Check to make sure the specified base address to ending address
        // is currently unused.
        //

        StartingAddress = (PVOID)((ULONG_PTR)MI_64K_ALIGN(*CapturedBase) +
                                    (SectionOffset->LowPart & (X64K - 1)));
        EndingAddress = (PVOID)(((ULONG_PTR)StartingAddress +
                                *CapturedViewSize - 1L) | (PAGE_SIZE - 1L));

#ifdef LARGE_PAGES
        if (AllocationType & MEM_LARGE_PAGES) {
            if (((ULONG_PTR)StartingAddress & (Alignment - 1)) != 0) {
                return STATUS_CONFLICTING_ADDRESSES;
            }
            EndingAddress = (PVOID)((PCHAR)StartingAddress + Alignment);
        }
#endif //LARGE_PAGES

        Vad = MiCheckForConflictingVad (StartingAddress, EndingAddress);

        if (Vad != (PMMVAD)NULL) {
#if DBG
            MiDumpConflictingVad (StartingAddress, EndingAddress, Vad);
#endif

            return STATUS_CONFLICTING_ADDRESSES;
        }
    }

    //
    // An unoccupied address range has been found, build the virtual
    // address descriptor to describe this range.
    //

#ifdef LARGE_PAGES
    if (AllocationType & MEM_LARGE_PAGES) {
        //
        // Allocate a subsection and 4 prototype PTEs to hold
        // the information for the large pages.
        //

        Subsection = ExAllocatePoolWithTag (NonPagedPool,
                                     sizeof(SUBSECTION) + (4 * sizeof(MMPTE)),
                                     MMPPTE_NAME);
        if (Subsection == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
#endif //LARGE_PAGES

    //
    // Establish an exception handler and attempt to allocate
    // the pool and charge quota.  Note that the InsertVad routine
    // will also charge quota which could raise an exception.
    //

    try  {

        PhysicalView = (PMI_PHYSICAL_VIEW)ExAllocatePoolWithTag (NonPagedPool,
                                                                 sizeof(MI_PHYSICAL_VIEW),
                                                                 MI_PHYSICAL_VIEW_KEY);
        if (PhysicalView == NULL) {
            ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
        }

        Vad = (PMMVAD)ExAllocatePoolWithTag (NonPagedPool,
                                             sizeof(MMVAD),
                                             MMVADKEY);
        if (Vad == NULL) {
            ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
        }

        PhysicalView->Vad = Vad;
        PhysicalView->StartVa = StartingAddress;
        PhysicalView->EndVa = EndingAddress;

        RtlZeroMemory (Vad, sizeof(MMVAD));
        Vad->StartingVpn = MI_VA_TO_VPN (StartingAddress);
        Vad->EndingVpn = MI_VA_TO_VPN (EndingAddress);
        Vad->ControlArea = ControlArea;
        Vad->u2.VadFlags2.Inherit = MM_VIEW_UNMAP;
        Vad->u.VadFlags.PhysicalMapping = 1;
        Vad->u.VadFlags.Protection = ProtectionMask;

        //
        // Set the last contiguous PTE field in the Vad to the page frame
        // number of the starting physical page.
        //

        Vad->LastContiguousPte = (PMMPTE)(ULONG)(
                            SectionOffset->QuadPart >> PAGE_SHIFT);
#ifdef LARGE_PAGES
    if (AllocationType & MEM_LARGE_PAGES) {
        Vad->u.VadFlags.LargePages = 1;
        Vad->FirstPrototypePte = (PMMPTE)Subsection;
    } else {
#endif //LARGE_PAGES
        // Vad->u.VadFlags.LargePages = 0;
        Vad->FirstPrototypePte = Vad->LastContiguousPte;
#ifdef LARGE_PAGES
    }
#endif //LARGE_PAGES

        //
        // Insert the VAD.  This could get an exception.
        //

        ASSERT (Vad->FirstPrototypePte <= Vad->LastContiguousPte);
        MiInsertVad (Vad);

    } except (EXCEPTION_EXECUTE_HANDLER) {

        if (PhysicalView != NULL) {
            ExFreePool (PhysicalView);
        }

        if (Vad != (PMMVAD)NULL) {

            //
            // The pool allocation succeeded, but the quota charge
            // in InsertVad failed, deallocate the pool and return
            // an error.
            //

            ExFreePool (Vad);
#ifdef LARGE_PAGES
    if (AllocationType & MEM_LARGE_PAGES) {
            ExFreePool (Subsection);
    }
#endif //LARGE_PAGES
            return GetExceptionCode();
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Increment the count of the number of views for the
    // section object.  This requires the PFN lock to be held.
    //

    LOCK_AWE (Process, OldIrql);
    LOCK_PFN2 (OldIrql2);

    InsertHeadList (&Process->PhysicalVadList, &PhysicalView->ListEntry);

    ControlArea->NumberOfMappedViews += 1;
    ControlArea->NumberOfUserReferences += 1;

    ASSERT (ControlArea->NumberOfSectionReferences != 0);

    UNLOCK_PFN2 (OldIrql2);
    UNLOCK_AWE (Process, OldIrql);

    //
    // Build the PTEs in the address space.
    //

    PointerPpe = MiGetPpeAddress (StartingAddress);
    PointerPde = MiGetPdeAddress (StartingAddress);
    PointerPte = MiGetPteAddress (StartingAddress);
    LastPte = MiGetPteAddress (EndingAddress);

    MI_MAKE_VALID_PTE (TempPte,
                       (ULONG_PTR)Vad->LastContiguousPte,
                       ProtectionMask,
                       PointerPte);

    if (WriteCombined == TRUE) {
        MI_SET_PTE_WRITE_COMBINE (TempPte);
    }

    if (TempPte.u.Hard.Write) {
        MI_SET_PTE_DIRTY (TempPte);
    }

#if defined(_IA64_)
    if (MI_IS_CACHING_DISABLED(&TempPte) || (WriteCombined == TRUE)) {
        KeFlushEntireTb(FALSE, TRUE);
    }
#endif

#ifdef LARGE_PAGES
    if (AllocationType & MEM_LARGE_PAGES) {
        Subsection->StartingSector = pageSize;
        Subsection->EndingSector = (ULONG_PTR)StartingAddress;
        Subsection->u.LongFlags = 0;
        Subsection->u.SubsectionFlags.LargePages = 1;
        protoPte = (PMMPTE)(Subsection + 1);

        //
        // Build the first 2 PTEs as entries for the TLB to
        // map the specified physical address.
        //

        *protoPte = TempPte;
        protoPte += 1;

        if (*CapturedViewSize > pageSize) {
            *protoPte = TempPte;
            protoPte->u.Hard.PageFrameNumber += (pageSize >> PAGE_SHIFT);
        } else {
            *protoPte = ZeroPte;
        }
        protoPte += 1;

        //
        // Build the first prototype PTE as a paging file format PTE
        // referring to the subsection.
        //

        protoPte->u.Long = MiGetSubsectionAddressForPte (Subsection);
        protoPte->u.Soft.Prototype = 1;
        protoPte->u.Soft.Protection = ProtectionMask;

        //
        // Set the PTE up for all the user's PTE entries, proto pte
        // format pointing to the 3rd prototype PTE.
        //

        TempPte.u.Long = MiProtoAddressForPte (protoPte);
    }

    if (!(AllocationType & MEM_LARGE_PAGES)) {
#endif //LARGE_PAGES

#if defined (_WIN64)
        MiMakePpeExistAndMakeValid (PointerPpe, Process, FALSE);
        if (PointerPde->u.Long == 0) {
            UsedPageDirectoryHandle = MI_GET_USED_PTES_HANDLE (PointerPte);
            MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageDirectoryHandle);
        }
#endif

        MiMakePdeExistAndMakeValid (PointerPde, Process, FALSE);

        Pfn2 = MI_PFN_ELEMENT (PointerPde->u.Hard.PageFrameNumber);

        UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (StartingAddress);

        while (PointerPte <= LastPte) {

            if (MiIsPteOnPdeBoundary (PointerPte)) {

                PointerPde = MiGetPteAddress (PointerPte);

                if (MiIsPteOnPpeBoundary (PointerPte)) {
                    PointerPpe = MiGetPteAddress (PointerPde);
                    MiMakePpeExistAndMakeValid (PointerPpe, Process, FALSE);
                }

#if defined (_WIN64)
                if (PointerPde->u.Long == 0) {
                    UsedPageDirectoryHandle = MI_GET_USED_PTES_HANDLE (PointerPte);
                    MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageDirectoryHandle);
                }
#endif

                MiMakePdeExistAndMakeValid (PointerPde, Process, FALSE);
                Pfn2 = MI_PFN_ELEMENT (PointerPde->u.Hard.PageFrameNumber);
                UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (MiGetVirtualAddressMappedByPte (PointerPte));
            }

            ASSERT (PointerPte->u.Long == 0);

            MI_WRITE_VALID_PTE (PointerPte, TempPte);

            CONSISTENCY_LOCK_PFN (OldIrql);

            Pfn2->u2.ShareCount += 1;

            CONSISTENCY_UNLOCK_PFN (OldIrql);

            //
            // Increment the count of non-zero page table entries for this
            // page table and the number of private pages for the process.
            //

            MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableHandle);

            PointerPte += 1;
            TempPte.u.Hard.PageFrameNumber += 1;
        }
#ifdef LARGE_PAGES
    }
#endif //LARGE_PAGES

#if defined(i386)
    //
    // If write combined was specified then flush all caches and TBs.
    //

    if (WriteCombined == TRUE && MiWriteCombiningPtes == TRUE) {
        KeFlushEntireTb (FALSE, TRUE);
        KeInvalidateAllCaches (TRUE);
    }
#endif

#if defined(_IA64_)
    if (MI_IS_CACHING_DISABLED(&TempPte) || (WriteCombined == TRUE)) {
        MiSweepCacheMachineDependent(StartingAddress, 
                                     (PCHAR)EndingAddress - (PCHAR)StartingAddress + 1L,
                                     MmWriteCombined);
    }
#endif

    UNLOCK_WS_UNSAFE (Process);
    *ReleasedWsMutex = TRUE;

    //
    // Update the current virtual size in the process header.
    //

    *CapturedViewSize = (PCHAR)EndingAddress - (PCHAR)StartingAddress + 1L;
    Process->VirtualSize += *CapturedViewSize;

    if (Process->VirtualSize > Process->PeakVirtualSize) {
        Process->PeakVirtualSize = Process->VirtualSize;
    }

    *CapturedBase = StartingAddress;

    return STATUS_SUCCESS;
}

#endif //!_ALPHA_


NTSTATUS
MiMapViewOfImageSection (
    IN PCONTROL_AREA ControlArea,
    IN PEPROCESS Process,
    IN PVOID *CapturedBase,
    IN PLARGE_INTEGER SectionOffset,
    IN PSIZE_T CapturedViewSize,
    IN PSECTION Section,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG_PTR ZeroBits,
    IN SIZE_T ImageCommitment,
    IN OUT PBOOLEAN ReleasedWsMutex
    )

/*++

Routine Description:

    This routine maps the specified Image section into the
    specified process's address space.

Arguments:

    see MmMapViewOfSection above...

    ControlArea - Supplies the control area for the section.

    Process - Supplies the process pointer which is receiving the section.

    ReleasedWsMutex - Supplies FALSE. If the working set mutex is
                      not held when returning this must be set to TRUE
                      so the caller will not release the unheld mutex.

                      Note if this routine acquires the working set mutex
                      it is always done unsafely as the address space mutex
                      must be held on entry regardless.

Return Value:

    Status of the map view operation.

Environment:

    Kernel Mode, address creation mutex held.

--*/

{
    PMMVAD Vad;
    PVOID StartingAddress;
    PVOID EndingAddress;
    BOOLEAN Attached;
    KIRQL OldIrql;
    PSUBSECTION Subsection;
    ULONG PteOffset;
    NTSTATUS ReturnedStatus;
    PMMPTE ProtoPte;
    PVOID BasedAddress;
    SIZE_T NeededViewSize;
#if defined(_ALPHA_)
    ULONG_PTR Starting2gb;
    ULONG_PTR Ending2gb;
    ULONG_PTR BoundaryMask;
    LOGICAL AlignmentOk;
#endif
#if defined(_MIALT4K_)
    PMMPTE PointerAltPte;
    PMMPTE ProtoAltPte;
    MMPTE TempAltPte;
    PIMAGE_NT_HEADERS NtHeaders;
    ULONG ImageAlignment;
#endif

    Attached = FALSE;

    //
    // Image file.
    //
    // Locate the first subsection (text) and create a virtual
    // address descriptor to map the entire image here.
    //

    if (ControlArea->u.Flags.GlobalOnlyPerSession == 0) {
        Subsection = (PSUBSECTION)(ControlArea + 1);
    }
    else {
        Subsection = (PSUBSECTION)((PLARGE_CONTROL_AREA)ControlArea + 1);
    }

    if (ControlArea->u.Flags.ImageMappedInSystemSpace) {

        if (KeGetPreviousMode() != KernelMode) {

            //
            // Mapping in system space as a driver, hence copy on write does
            // not work.  Don't allow user processes to map the image.
            //

            *ReleasedWsMutex = TRUE;
            return STATUS_CONFLICTING_ADDRESSES;
        }
    }

    //
    // Check to see if a purge operation is in progress and if so, wait
    // for the purge to complete.  In addition, up the count of mapped
    // views for this control area.
    //

    if (MiCheckPurgeAndUpMapCount (ControlArea) == FALSE) {
        *ReleasedWsMutex = TRUE;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Capture the based address to the stack, to prevent page faults.
    //

    BasedAddress = ControlArea->Segment->BasedAddress;

    if (*CapturedViewSize == 0) {
        *CapturedViewSize =
            (ULONG_PTR)(Section->SizeOfSection.QuadPart - SectionOffset->QuadPart);
    }

    LOCK_WS_UNSAFE (Process);

    ReturnedStatus = STATUS_SUCCESS;

    //
    // Determine if a specific base was specified.
    //

    if (*CapturedBase != NULL) {

        //
        // Captured base is not NULL.
        //
        // Check to make sure the specified address range is currently unused
        // and within the user address space.
        //

        Vad = (PMMVAD)1;
        StartingAddress = MI_64K_ALIGN(*CapturedBase);
        EndingAddress = (PVOID)(((ULONG_PTR)StartingAddress +
                                       *CapturedViewSize - 1) | (PAGE_SIZE - 1));

        if ((StartingAddress <=  MM_HIGHEST_VAD_ADDRESS) &&
            (((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS + 1) -
                                (ULONG_PTR)StartingAddress >= *CapturedViewSize) &&

            (EndingAddress <= MM_HIGHEST_VAD_ADDRESS)) {

            Vad = MiCheckForConflictingVad (StartingAddress, EndingAddress);
        }

        if (Vad != NULL) {
#if DBG
            if (Vad != (PMMVAD)1) {
                MiDumpConflictingVad (StartingAddress, EndingAddress, Vad);
            }
#endif

            LOCK_PFN (OldIrql);
            ControlArea->NumberOfMappedViews -= 1;
            ControlArea->NumberOfUserReferences -= 1;

            //
            // Check to see if the control area (segment) should be deleted.
            // This routine releases the PFN lock.
            //
        
            MiCheckControlArea (ControlArea, Process, OldIrql);
            return STATUS_CONFLICTING_ADDRESSES;
        }

        //
        // A conflicting VAD was not found and the specified address range is
        // within the user address space. If the image will not reside at its
        // base address, then set a special return status.
        //

        if (((ULONG_PTR)StartingAddress +
            (ULONG_PTR)MI_64K_ALIGN(SectionOffset->LowPart)) != (ULONG_PTR)BasedAddress) {
            ReturnedStatus = STATUS_IMAGE_NOT_AT_BASE;
        }

    } else {

        //
        // Captured base is NULL.
        //
        // If the captured view size is greater than the largest size that
        // can fit in the user address space, then it is not possible to map
        // the image.
        //

        if ((PVOID)*CapturedViewSize > MM_HIGHEST_VAD_ADDRESS) {
            LOCK_PFN (OldIrql);
            ControlArea->NumberOfMappedViews -= 1;
            ControlArea->NumberOfUserReferences -= 1;

            //
            // Check to see if the control area (segment) should be deleted.
            // This routine releases the PFN lock.
            //
        
            MiCheckControlArea (ControlArea, Process, OldIrql);
            return STATUS_NO_MEMORY;
        }

        //
        // Check to make sure the specified address range is currently unused
        // and within the user address space.
        //

        StartingAddress = (PVOID)((ULONG_PTR)BasedAddress +
                                    (ULONG_PTR)MI_64K_ALIGN(SectionOffset->LowPart));

        EndingAddress = (PVOID)(((ULONG_PTR)StartingAddress +
                                    *CapturedViewSize - 1) | (PAGE_SIZE - 1));

        Vad = (PMMVAD)1;
        NeededViewSize = *CapturedViewSize;

#if defined(_ALPHA_)

#if defined(_AXP64_)
#define BOUNDARY ((LONG_PTR)_2gb)
#else
#define BOUNDARY ((ULONG_PTR)_2gb)
#endif

        //
        // Alpha images cannot span 2GB boundaries with the current compiler
        // generated branch instructions.  If a spanning image is detected, it
        // must be relocated.
        //

        BoundaryMask = (ULONG_PTR)~(BOUNDARY-1);

        Starting2gb = (ULONG_PTR)StartingAddress & BoundaryMask;
        Ending2gb = (ULONG_PTR)EndingAddress & BoundaryMask;

        AlignmentOk = TRUE;

        if (Starting2gb != Ending2gb) {
            AlignmentOk = FALSE;
            if (Starting2gb + 1 == Ending2gb) {
                if (((ULONG_PTR)EndingAddress & (BOUNDARY-1)) == 0) {
                    AlignmentOk = TRUE;
                }
            }
        }

        if (AlignmentOk == FALSE) {

            //
            // Find twice the required size so that it can be placed correctly.
            //

            NeededViewSize = *CapturedViewSize * 2 + X64K;
        }
        else
#endif

        if ((StartingAddress >= MM_LOWEST_USER_ADDRESS) &&
            (StartingAddress <= MM_HIGHEST_VAD_ADDRESS) &&
            (((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS + 1) -
                                (ULONG_PTR)StartingAddress >= *CapturedViewSize) &&

            (EndingAddress <= MM_HIGHEST_VAD_ADDRESS)) {

            Vad = MiCheckForConflictingVad (StartingAddress, EndingAddress);
        }

        //
        // If the VAD address is not NULL, then a conflict was discovered.
        // Attempt to select another address range in which to map the image.
        //

        if (Vad != (PMMVAD)NULL) {

            //
            // The image could not be mapped at its natural base address
            // try to find another place to map it.
            //
#if DBG
            if (Vad != (PMMVAD)1) {
                MiDumpConflictingVad (StartingAddress, EndingAddress, Vad);
            }
#endif

            //
            // If the system has been biased to an alternate base address to
            // allow 3gb of user address space, then make sure the high order
            // address bit is zero.
            //

#if defined(_X86_)

            if ((MmVirtualBias != 0) &&
                (ZeroBits == 0)) {
                ZeroBits = 1;
            }
#endif

            ReturnedStatus = STATUS_IMAGE_NOT_AT_BASE;
            try {

                //
                // Find a starting address on a 64k boundary.
                //

                StartingAddress = MiFindEmptyAddressRange (NeededViewSize,
                                                           X64K,
                                                           (ULONG)ZeroBits);


            } except (EXCEPTION_EXECUTE_HANDLER) {
                LOCK_PFN (OldIrql);
                ControlArea->NumberOfMappedViews -= 1;
                ControlArea->NumberOfUserReferences -= 1;

                //
                // Check to see if the control area (segment) should be deleted.
                // This routine releases the PFN lock.
                //
            
                MiCheckControlArea (ControlArea, Process, OldIrql);

                return GetExceptionCode();
            }

            EndingAddress = (PVOID)(((ULONG_PTR)StartingAddress +
                                        *CapturedViewSize - 1) | (PAGE_SIZE - 1));

#if defined(_ALPHA_)

            if (AlignmentOk == FALSE) {
                Starting2gb = (ULONG_PTR)StartingAddress & ~(BOUNDARY-1);
                Ending2gb = (ULONG_PTR)EndingAddress & ~(BOUNDARY-1);
        
                if (Starting2gb != Ending2gb) {
    
                    //
                    // Not in the same 2gb.  Up the start to a 2gb boundary.
                    //
    
                    StartingAddress = (PVOID)Ending2gb;
                    EndingAddress = (PVOID)(((ULONG_PTR)StartingAddress +
                                        *CapturedViewSize - 1) | (PAGE_SIZE - 1));
                }
            }
#endif

        }
    }

    //
    // Allocate and initialize a VAD for the specified address range.
    //

    Vad = ExAllocatePoolWithTag (NonPagedPool, sizeof(MMVAD), MMVADKEY);
    try  {
        if (Vad == NULL) {
            ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
        }

        RtlZeroMemory (Vad, sizeof(MMVAD));
        Vad->StartingVpn = MI_VA_TO_VPN (StartingAddress);
        Vad->EndingVpn = MI_VA_TO_VPN (EndingAddress);
        Vad->u2.VadFlags2.Inherit = (InheritDisposition == ViewShare);
        Vad->u.VadFlags.ImageMap = 1;

        //
        // Set the protection in the VAD as EXECUTE_WRITE_COPY.
        //

        Vad->u.VadFlags.Protection = MM_EXECUTE_WRITECOPY;
        Vad->ControlArea = ControlArea;

        //
        // Set the first prototype PTE field in the Vad.
        //

        SectionOffset->LowPart = SectionOffset->LowPart & ~(X64K - 1);
        PteOffset = (ULONG)(SectionOffset->QuadPart >> PAGE_SHIFT);

        Vad->FirstPrototypePte = &Subsection->SubsectionBase[PteOffset];
        Vad->LastContiguousPte = MM_ALLOCATION_FILLS_VAD;

        //
        // NOTE: the full commitment is charged even if a partial map of an
        // image is being done.  This saves from having to run through the
        // entire image (via prototype PTEs) and calculate the charge on
        // a per page basis for the partial map.
        //

        Vad->u.VadFlags.CommitCharge = (SIZE_T)ImageCommitment; // ****** temp

        ASSERT (Vad->FirstPrototypePte <= Vad->LastContiguousPte);

        MiInsertVad (Vad);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        LOCK_PFN (OldIrql);
        ControlArea->NumberOfMappedViews -= 1;
        ControlArea->NumberOfUserReferences -= 1;

        //
        // Check to see if the control area (segment) should be deleted.
        // This routine releases the PFN lock.
        //
    
        MiCheckControlArea (ControlArea, Process, OldIrql);

        if (Vad != (PMMVAD)NULL) {

            //
            // The pool allocation succeeded, but the quota charge
            // in InsertVad failed, deallocate the pool and return
            // an error.
            //

            ExFreePool (Vad);
            return GetExceptionCode();
        }

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *CapturedViewSize = (PCHAR)EndingAddress - (PCHAR)StartingAddress + 1L;
    *CapturedBase = StartingAddress;

#if DBG
    if (MmDebug & MM_DBG_WALK_VAD_TREE) {
        DbgPrint("mapped image section vads\n");
        VadTreeWalk(Process->VadRoot);
    }
#endif

    //
    // Update the current virtual size in the process header.
    //

    Process->VirtualSize += *CapturedViewSize;

    if (Process->VirtualSize > Process->PeakVirtualSize) {
        Process->PeakVirtualSize = Process->VirtualSize;
    }

    if (ControlArea->u.Flags.FloppyMedia) {

        *ReleasedWsMutex = TRUE;
        UNLOCK_WS_UNSAFE (Process);

        //
        // The image resides on a floppy disk, in-page all
        // pages from the floppy and mark them as modified so
        // they migrate to the paging file rather than reread
        // them from the floppy disk which may have been removed.
        //

        ProtoPte = Vad->FirstPrototypePte;

        //
        // This could get an in-page error from the floppy.
        //

        while (StartingAddress < EndingAddress) {

            //
            // If the prototype PTE is valid, transition or
            // in prototype PTE format, bring the page into
            // memory and set the modified bit.
            //

            if ((ProtoPte->u.Hard.Valid == 1) ||
                (ProtoPte->u.Soft.Prototype == 1) ||
                (ProtoPte->u.Soft.Transition == 1)) {

                try {

                    MiSetPageModified (StartingAddress);

                } except (EXCEPTION_EXECUTE_HANDLER) {

                    //
                    // An in page error must have occurred touching the image,
                    // ignore the error and continue to the next page.
                    //

                    NOTHING;
                }
            }
            ProtoPte += 1;
            StartingAddress = (PVOID)((PCHAR)StartingAddress + PAGE_SIZE);
        }
    }

    if (!*ReleasedWsMutex) {
        *ReleasedWsMutex = TRUE;
        UNLOCK_WS_UNSAFE (Process);
    }

    if (NT_SUCCESS(ReturnedStatus)) {


        //
        // Check to see if this image is for the architecture of the current
        // machine.
        //

        if (ControlArea->Segment->ImageInformation->ImageContainsCode &&
            ((ControlArea->Segment->ImageInformation->Machine <
                                          USER_SHARED_DATA->ImageNumberLow) ||
             (ControlArea->Segment->ImageInformation->Machine >
                                          USER_SHARED_DATA->ImageNumberHigh)
            )
           ) {
#if defined (_WIN64)

            //
            // If this is a wow64 process, allow i386 images
            //

            if (!Process->Wow64Process ||
                ControlArea->Segment->ImageInformation->Machine != IMAGE_FILE_MACHINE_I386) {
                return STATUS_IMAGE_MACHINE_TYPE_MISMATCH;
            }
#else   //!_WIN64
            return STATUS_IMAGE_MACHINE_TYPE_MISMATCH;
#endif
        }

        StartingAddress = MI_VPN_TO_VA (Vad->StartingVpn);

        if (PsImageNotifyEnabled) {

            IMAGE_INFO ImageInfo;

            if ( (StartingAddress < MmHighestUserAddress) &&
                 Process->UniqueProcessId &&
                 Process != PsInitialSystemProcess ) {

                ImageInfo.Properties = 0;
                ImageInfo.ImageAddressingMode = IMAGE_ADDRESSING_MODE_32BIT;
                ImageInfo.ImageBase = StartingAddress;
                ImageInfo.ImageSize = *CapturedViewSize;
                ImageInfo.ImageSelector = 0;
                ImageInfo.ImageSectionNumber = 0;
                PsCallImageNotifyRoutines(
                            (PUNICODE_STRING) &ControlArea->FilePointer->FileName,
                            Process->UniqueProcessId,
                            &ImageInfo
                            );
            }
        }

        if ((NtGlobalFlag & FLG_ENABLE_KDEBUG_SYMBOL_LOAD) &&
            (ControlArea->u.Flags.Image) &&
            (ReturnedStatus != STATUS_IMAGE_NOT_AT_BASE)) {
            if (ControlArea->u.Flags.DebugSymbolsLoaded == 0) {
                if (CacheImageSymbols (StartingAddress)) {

                    //
                    //  TEMP TEMP TEMP rip out when debugger converted
                    //

                    PUNICODE_STRING FileName;
                    ANSI_STRING AnsiName;
                    NTSTATUS Status;

                    LOCK_PFN (OldIrql);
                    ControlArea->u.Flags.DebugSymbolsLoaded = 1;
                    UNLOCK_PFN (OldIrql);

                    FileName = (PUNICODE_STRING)&ControlArea->FilePointer->FileName;
                    if (FileName->Length != 0 && (NtGlobalFlag & FLG_ENABLE_KDEBUG_SYMBOL_LOAD)) {
                        PLIST_ENTRY Head, Next;
                        PLDR_DATA_TABLE_ENTRY Entry;

                        KeEnterCriticalRegion();
                        ExAcquireResourceExclusive (&PsLoadedModuleResource, TRUE);
                        Head = &MmLoadedUserImageList;
                        Next = Head->Flink;
                        while (Next != Head) {
                            Entry = CONTAINING_RECORD( Next,
                                                       LDR_DATA_TABLE_ENTRY,
                                                       InLoadOrderLinks
                                                     );
                            if (Entry->DllBase == StartingAddress) {
                                Entry->LoadCount += 1;
                                break;
                            }
                            Next = Next->Flink;
                        }

                        if (Next == Head) {
                            Entry = ExAllocatePoolWithTag( NonPagedPool,
                                                    sizeof( *Entry ) +
                                                        FileName->Length +
                                                        sizeof( UNICODE_NULL ),
                                                        MMDB
                                                  );
                            if (Entry != NULL) {
                                PIMAGE_NT_HEADERS NtHeaders;

                                RtlZeroMemory (Entry, sizeof(*Entry));

                                try {
                                    NtHeaders = RtlImageNtHeader (StartingAddress);
                                    if (NtHeaders != NULL) {
#if defined(_WIN64)
                                        if (NtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
                                            Entry->SizeOfImage = NtHeaders->OptionalHeader.SizeOfImage;
                                            Entry->CheckSum = NtHeaders->OptionalHeader.CheckSum;
                                        }
                                        else {
                                            PIMAGE_NT_HEADERS32 NtHeaders32 = (PIMAGE_NT_HEADERS32)NtHeaders;

                                            Entry->SizeOfImage = NtHeaders32->OptionalHeader.SizeOfImage;
                                            Entry->CheckSum = NtHeaders32->OptionalHeader.CheckSum;
                                        }
#else
                                        Entry->SizeOfImage = NtHeaders->OptionalHeader.SizeOfImage;
                                        Entry->CheckSum = NtHeaders->OptionalHeader.CheckSum;
#endif
                                    }
                                } except (EXCEPTION_EXECUTE_HANDLER) {
                                    NOTHING;
                                }

                                Entry->DllBase = StartingAddress;
                                Entry->FullDllName.Buffer = (PWSTR)(Entry+1);
                                Entry->FullDllName.Length = FileName->Length;
                                Entry->FullDllName.MaximumLength = (USHORT)
                                    (Entry->FullDllName.Length + sizeof( UNICODE_NULL ));
                                RtlMoveMemory( Entry->FullDllName.Buffer,
                                               FileName->Buffer,
                                               FileName->Length
                                             );
                                Entry->FullDllName.Buffer[ Entry->FullDllName.Length / sizeof( WCHAR )] = UNICODE_NULL;
                                Entry->LoadCount = 1;
                                InsertTailList( &MmLoadedUserImageList,
                                                &Entry->InLoadOrderLinks
                                              );
                                InitializeListHead( &Entry->InInitializationOrderLinks );
                                InitializeListHead( &Entry->InMemoryOrderLinks );
                            }
                        }

                        ExReleaseResource (&PsLoadedModuleResource);
                        KeLeaveCriticalRegion();
                    }

                    Status = RtlUnicodeStringToAnsiString( &AnsiName,
                                                           FileName,
                                                           TRUE );

                    if (NT_SUCCESS( Status)) {
                        DbgLoadImageSymbols( &AnsiName,
                                             StartingAddress,
                                             (ULONG_PTR)Process
                                           );
                        RtlFreeAnsiString( &AnsiName );
                    }
                }
            }
        }
    }

#if defined(_MIALT4K_)

    if (Process->Wow64Process != NULL) {

        MiProtectImageFileFor4kPage(StartingAddress,
                                    *CapturedViewSize,
                                    Vad->FirstPrototypePte,
                                    Process);
    }

#endif

    return ReturnedStatus;

}

NTSTATUS
MiMapViewOfDataSection (
    IN PCONTROL_AREA ControlArea,
    IN PEPROCESS Process,
    IN PVOID *CapturedBase,
    IN PLARGE_INTEGER SectionOffset,
    IN PSIZE_T CapturedViewSize,
    IN PSECTION Section,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG ProtectionMask,
    IN SIZE_T CommitSize,
    IN ULONG_PTR ZeroBits,
    IN ULONG AllocationType,
    IN PBOOLEAN ReleasedWsMutex
    )

/*++

Routine Description:

    This routine maps the specified physical section into the
    specified process's address space.

Arguments:

    see MmMapViewOfSection above...

    ControlArea - Supplies the control area for the section.

    Process - Supplies the process pointer which is receiving the section.

    ProtectionMask - Supplies the initial page protection-mask.

    ReleasedWsMutex - Supplies FALSE. If the working set mutex is
                      not held when returning this must be set to TRUE
                      so the caller will not release the unheld mutex.

                      Note if this routine acquires the working set mutex
                      it is always done unsafely as the address space mutex
                      must be held on entry regardless.

Return Value:

    Status of the map view operation.

Environment:

    Kernel Mode, address creation mutex held.

--*/

{
    PMMVAD Vad;
    PVOID StartingAddress;
    PVOID EndingAddress;
    BOOLEAN Attached;
    KIRQL OldIrql;
    PSUBSECTION Subsection;
    ULONG PteOffset;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    MMPTE TempPte;
    ULONG_PTR Alignment;
    SIZE_T QuotaCharge;
    BOOLEAN ChargedQuota;
    PMMPTE TheFirstPrototypePte;
    PVOID CapturedStartingVa;
    ULONG CapturedCopyOnWrite;
#if defined(_MIALT4K_)
    SIZE_T ViewSizeFor4k;
#endif

    Attached = FALSE;
    QuotaCharge = 0;
    ChargedQuota = FALSE;

    if ((AllocationType & MEM_RESERVE) &&
        (ControlArea->FilePointer == NULL)) {
        *ReleasedWsMutex = TRUE;
        return STATUS_INVALID_PARAMETER_9;
    }

    //
    // Check to see if there is a purge operation ongoing for
    // this segment.
    //

    if ((AllocationType & MEM_DOS_LIM) != 0) {
        if ((*CapturedBase == NULL) ||
            (AllocationType & MEM_RESERVE)) {

            //
            // If MEM_DOS_LIM is specified, the address to map the
            // view MUST be specified as well.
            //

            *ReleasedWsMutex = TRUE;
            return STATUS_INVALID_PARAMETER_3;
        }
        Alignment = PAGE_SIZE;
    } else {
       Alignment = X64K;
    }

    //
    // Check to see if a purge operation is in progress and if so, wait
    // for the purge to complete.  In addition, up the count of mapped
    // views for this control area.
    //

    if (MiCheckPurgeAndUpMapCount (ControlArea) == FALSE) {
        *ReleasedWsMutex = TRUE;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (*CapturedViewSize == 0) {

        SectionOffset->LowPart = SectionOffset->LowPart & ~((ULONG)Alignment - 1);
        *CapturedViewSize = (ULONG_PTR)(Section->SizeOfSection.QuadPart -
                                    SectionOffset->QuadPart);
    } else {
        *CapturedViewSize += SectionOffset->LowPart & (Alignment - 1);
        SectionOffset->LowPart = SectionOffset->LowPart & ~((ULONG)Alignment - 1);
    }

    if ((LONG_PTR)*CapturedViewSize <= 0) {

        //
        // Section offset or view size past size of section.
        //

        LOCK_PFN (OldIrql);
        ControlArea->NumberOfMappedViews -= 1;
        ControlArea->NumberOfUserReferences -= 1;

        //
        // Check to see if the control area (segment) should be deleted.
        // This routine releases the PFN lock.
        //
    
        MiCheckControlArea (ControlArea, NULL, OldIrql);

        *ReleasedWsMutex = TRUE;
        return STATUS_INVALID_VIEW_SIZE;
    }

    //
    // Calculate the first prototype PTE field in the Vad.
    //

    ASSERT (ControlArea->u.Flags.GlobalOnlyPerSession == 0);

    Subsection = (PSUBSECTION)(ControlArea + 1);

    SectionOffset->LowPart = SectionOffset->LowPart & ~((ULONG)Alignment - 1);
    PteOffset = (ULONG)(SectionOffset->QuadPart >> PAGE_SHIFT);

    //
    // Make sure the PTEs are not in the extended part of the
    // segment.
    //

    if (PteOffset >= ControlArea->Segment->TotalNumberOfPtes) {
        LOCK_PFN (OldIrql);
        ControlArea->NumberOfMappedViews -= 1;
        ControlArea->NumberOfUserReferences -= 1;

        //
        // Check to see if the control area (segment) should be deleted.
        // This routine releases the PFN lock.
        //
    
        MiCheckControlArea (ControlArea, NULL, OldIrql);

        *ReleasedWsMutex = TRUE;
        return STATUS_INVALID_VIEW_SIZE;
    }

    while (PteOffset >= Subsection->PtesInSubsection) {
        PteOffset -= Subsection->PtesInSubsection;
        Subsection = Subsection->NextSubsection;
        ASSERT (Subsection != NULL);
    }

    TheFirstPrototypePte = &Subsection->SubsectionBase[PteOffset];

    //
    // Calculate the quota for the specified pages.
    //

    if ((ControlArea->FilePointer == NULL) &&
        (CommitSize != 0) &&
        (ControlArea->Segment->NumberOfCommittedPages <
                ControlArea->Segment->TotalNumberOfPtes)) {


        ExAcquireFastMutexUnsafe (&MmSectionCommitMutex);

        PointerPte = TheFirstPrototypePte;
        LastPte = PointerPte + BYTES_TO_PAGES(CommitSize);

        while (PointerPte < LastPte) {
            if (PointerPte->u.Long == 0) {
                QuotaCharge += 1;
            }
            PointerPte += 1;
        }
        ExReleaseFastMutexUnsafe (&MmSectionCommitMutex);
    }

    CapturedStartingVa = (PVOID)Section->Address.StartingVpn;
    CapturedCopyOnWrite = Section->u.Flags.CopyOnWrite;
    LOCK_WS_UNSAFE (Process);

    if ((*CapturedBase == NULL) && (CapturedStartingVa == NULL)) {

        //
        // The section is not based, find an empty range.
        // This could raise an exception.

        try {

            //
            // Find a starting address on a 64k boundary.
            //

            if ( AllocationType & MEM_TOP_DOWN ) {
                StartingAddress = MiFindEmptyAddressRangeDown (
                                    *CapturedViewSize,
                                    (PVOID)((PCHAR)MM_HIGHEST_VAD_ADDRESS + 1),
                                    Alignment
                                    );
            } else {
                StartingAddress = MiFindEmptyAddressRange (*CapturedViewSize,
                                                           Alignment,
                                                           (ULONG)ZeroBits);
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {

            LOCK_PFN (OldIrql);
            ControlArea->NumberOfMappedViews -= 1;
            ControlArea->NumberOfUserReferences -= 1;

            //
            // Check to see if the control area (segment) should be deleted.
            // This routine releases the PFN lock.
            //
        
            MiCheckControlArea (ControlArea, Process, OldIrql);

            return GetExceptionCode();
        }

        EndingAddress = (PVOID)(((ULONG_PTR)StartingAddress +
                                    *CapturedViewSize - 1L) | (PAGE_SIZE - 1L));

        if (ZeroBits > 0) {
            if (EndingAddress > (PVOID)((ULONG_PTR)MM_USER_ADDRESS_RANGE_LIMIT >> ZeroBits)) {
                LOCK_PFN (OldIrql);
                ControlArea->NumberOfMappedViews -= 1;
                ControlArea->NumberOfUserReferences -= 1;

                //
                // Check to see if the control area (segment) should be deleted.
                // This routine releases the PFN lock.
                //
            
                MiCheckControlArea (ControlArea, Process, OldIrql);

                return STATUS_NO_MEMORY;
            }
        }

    } else {

        if (*CapturedBase == NULL) {

            //
            // The section is based.
            //

            StartingAddress = (PVOID)((PCHAR)CapturedStartingVa +
                                                     SectionOffset->LowPart);
        } else {

            StartingAddress = MI_ALIGN_TO_SIZE (*CapturedBase, Alignment);

        }

        //
        // Check to make sure the specified base address to ending address
        // is currently unused.
        //

        EndingAddress = (PVOID)(((ULONG_PTR)StartingAddress +
                                   *CapturedViewSize - 1L) | (PAGE_SIZE - 1L));

        Vad = MiCheckForConflictingVad (StartingAddress, EndingAddress);
        if (Vad != (PMMVAD)NULL) {
#if DBG
                MiDumpConflictingVad (StartingAddress, EndingAddress, Vad);
#endif

            LOCK_PFN (OldIrql);
            ControlArea->NumberOfMappedViews -= 1;
            ControlArea->NumberOfUserReferences -= 1;

            //
            // Check to see if the control area (segment) should be deleted.
            // This routine releases the PFN lock.
            //
        
            MiCheckControlArea (ControlArea, Process, OldIrql);

            return STATUS_CONFLICTING_ADDRESSES;
        }
    }

    //
    // An unoccupied address range has been found, build the virtual
    // address descriptor to describe this range.
    //

    try  {

        Vad = ExAllocatePoolWithTag (NonPagedPool,
                                     sizeof(MMVAD),
                                     MMVADKEY);
        if (Vad == NULL) {
            ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
        }
        RtlZeroMemory (Vad, sizeof(MMVAD));

        Vad->StartingVpn = MI_VA_TO_VPN (StartingAddress);
        Vad->EndingVpn = MI_VA_TO_VPN (EndingAddress);
        Vad->FirstPrototypePte = TheFirstPrototypePte;

        //
        // Set the protection in the PTE template field of the VAD.
        //

        Vad->ControlArea = ControlArea;

        Vad->u2.VadFlags2.Inherit = (InheritDisposition == ViewShare);
        Vad->u.VadFlags.Protection = ProtectionMask;
        Vad->u2.VadFlags2.CopyOnWrite = CapturedCopyOnWrite;

        //
        // Note that for MEM_DOS_LIM significance is lost here, but those
        // files are not mapped MEM_RESERVE.
        //

        Vad->u2.VadFlags2.FileOffset = (ULONG)(SectionOffset->QuadPart >> 16);

        if ((AllocationType & SEC_NO_CHANGE) || (Section->u.Flags.NoChange)) {
            Vad->u.VadFlags.NoChange = 1;
            Vad->u2.VadFlags2.SecNoChange = 1;
        }
        if (AllocationType & MEM_RESERVE) {
            PMMEXTEND_INFO ExtendInfo;

            ExAcquireFastMutexUnsafe (&MmSectionBasedMutex);
            ExtendInfo = ControlArea->Segment->ExtendInfo;
            if (ExtendInfo) {
                ExtendInfo->ReferenceCount += 1;
                if (ExtendInfo->CommittedSize < (UINT64)Section->SizeOfSection.QuadPart) {
                    ExtendInfo->CommittedSize = (UINT64)Section->SizeOfSection.QuadPart;
                }
            } else {

                ExtendInfo = ExAllocatePoolWithTag (NonPagedPool,
                                                    sizeof(MMEXTEND_INFO),
                                                    'xCmM');
                if (ExtendInfo == NULL) {
                    ExReleaseFastMutexUnsafe (&MmSectionBasedMutex);
                    ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
                }
                ExtendInfo->ReferenceCount = 1;
                ExtendInfo->CommittedSize = ControlArea->Segment->SizeOfSegment;
                if (ExtendInfo->CommittedSize < (UINT64)Section->SizeOfSection.QuadPart) {
                    ExtendInfo->CommittedSize = (UINT64)Section->SizeOfSection.QuadPart;
                }
                ControlArea->Segment->ExtendInfo = ExtendInfo;
            }
            ExReleaseFastMutexUnsafe (&MmSectionBasedMutex);
            Vad->u2.VadFlags2.ExtendableFile = 1;
            ASSERT (Vad->u4.ExtendedInfo == NULL);
            Vad->u4.ExtendedInfo = ExtendInfo;
        }

        //
        // If the page protection is write-copy or execute-write-copy
        // charge for each page in the view as it may become private.
        //

        if (MI_IS_PTE_PROTECTION_COPY_WRITE(ProtectionMask)) {
            Vad->u.VadFlags.CommitCharge = (BYTES_TO_PAGES ((PCHAR) EndingAddress -
                               (PCHAR) StartingAddress));
        }

        //
        // If this is a page file backed section, charge the process's page
        // file quota as if all the pages have been committed.  This solves
        // the problem when other processes commit all the pages and leave
        // only one process around who may not have been charged the proper
        // quota.  This is solved by charging everyone the maximum quota.
        //
//
// commented out for commitment charging.
//

#if 0
        if (ControlArea->FilePointer == NULL) {

            //
            // This is a page file backed section.  Charge for all the pages.
            //

            Vad->CommitCharge += (BYTES_TO_PAGES ((PCHAR)EndingAddress -
                               (PCHAR)StartingAddress));
        }
#endif


        PteOffset += (ULONG)(Vad->EndingVpn - Vad->StartingVpn);

        if (PteOffset < Subsection->PtesInSubsection ) {
            Vad->LastContiguousPte = &Subsection->SubsectionBase[PteOffset];

        } else {
            Vad->LastContiguousPte = &Subsection->SubsectionBase[
                                        (Subsection->PtesInSubsection - 1) +
                                        Subsection->UnusedPtes];
        }

        if (QuotaCharge != 0) {
            if (MiChargeCommitment (QuotaCharge, Process) == FALSE) {
                ExRaiseStatus (STATUS_COMMITMENT_LIMIT);
            }
            ChargedQuota = TRUE;
            MM_TRACK_COMMIT (MM_DBG_COMMIT_MAPVIEW_DATA, QuotaCharge);
        }

        ASSERT (Vad->FirstPrototypePte <= Vad->LastContiguousPte);
        MiInsertVad (Vad);

    } except (EXCEPTION_EXECUTE_HANDLER) {

        LOCK_PFN (OldIrql);
        ControlArea->NumberOfMappedViews -= 1;
        ControlArea->NumberOfUserReferences -= 1;

        //
        // Check to see if the control area (segment) should be deleted.
        // This routine releases the PFN lock.
        //
    
        MiCheckControlArea (ControlArea, Process, OldIrql);

        if (Vad != (PMMVAD)NULL) {

            //
            // The pool allocation succeeded, but the quota charge
            // in InsertVad failed, deallocate the pool and return
            // an error.
            //

            ExFreePool (Vad);
            if (ChargedQuota) {
                MiReturnCommitment (QuotaCharge);
                MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_MAPVIEW_DATA, QuotaCharge);
            }
            return GetExceptionCode();
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *ReleasedWsMutex = TRUE;

#if DBG
    if (!(AllocationType & MEM_RESERVE)) {
        ASSERT(((ULONG_PTR)EndingAddress - (ULONG_PTR)StartingAddress) <=
                ROUND_TO_PAGES64(Section->Segment->SizeOfSegment));
    }
#endif //DBG

    UNLOCK_WS_UNSAFE (Process);

    //
    // If a commit size was specified, make sure those pages are committed.
    //

    if (QuotaCharge != 0) {

        ExAcquireFastMutexUnsafe (&MmSectionCommitMutex);

        PointerPte = Vad->FirstPrototypePte;
        LastPte = PointerPte + BYTES_TO_PAGES(CommitSize);
        TempPte = ControlArea->Segment->SegmentPteTemplate;

        while (PointerPte < LastPte) {

            if (PointerPte->u.Long == 0) {

                MI_WRITE_INVALID_PTE (PointerPte, TempPte);
            }
            PointerPte += 1;
        }

        ControlArea->Segment->NumberOfCommittedPages += QuotaCharge;

        ASSERT (ControlArea->Segment->NumberOfCommittedPages <=
                ControlArea->Segment->TotalNumberOfPtes);
        MmSharedCommit += QuotaCharge;

        ExReleaseFastMutexUnsafe (&MmSectionCommitMutex);
    }

#if defined(_MIALT4K_)

    if (Process->Wow64Process != NULL) {


        EndingAddress = (PVOID)(((ULONG_PTR)StartingAddress +
                                 *CapturedViewSize - 1L) | (PAGE_4K - 1L));

        ViewSizeFor4k = (PCHAR)EndingAddress - (PCHAR)StartingAddress + 1L;

        MiProtectMapFileFor4kPage (StartingAddress,
                                   ViewSizeFor4k,
                                   ProtectionMask, 
                                   Vad->FirstPrototypePte,
                                   Process);
    }
#endif

    //
    // Update the current virtual size in the process header.
    //

    *CapturedViewSize = (PCHAR)EndingAddress - (PCHAR)StartingAddress + 1L;
    Process->VirtualSize += *CapturedViewSize;

    if (Process->VirtualSize > Process->PeakVirtualSize) {
        Process->PeakVirtualSize = Process->VirtualSize;
    }

    *CapturedBase = StartingAddress;

    return STATUS_SUCCESS;
}

LOGICAL
MiCheckPurgeAndUpMapCount (
    IN PCONTROL_AREA ControlArea
    )

/*++

Routine Description:

    This routine synchronizes with any on going purge operations
    on the same segment (identified via the control area).  If
    another purge operation is occurring, the function blocks until
    it is completed.

    When this function returns the MappedView and the NumberOfUserReferences
    count for the control area will be incremented thereby referencing
    the control area.

Arguments:

    ControlArea - Supplies the control area for the segment to be purged.

Return Value:

    TRUE if the synchronization was successful.
    FALSE if the synchronization did not occur due to low resources, etc.

Environment:

    Kernel Mode.

--*/

{
    KIRQL OldIrql;
    PEVENT_COUNTER PurgedEvent;
    PEVENT_COUNTER WaitEvent;
    ULONG OldRef;

    PurgedEvent = NULL;
    OldRef = 1;

    LOCK_PFN (OldIrql);

    while (ControlArea->u.Flags.BeingPurged != 0) {

        //
        // A purge operation is in progress.
        //

        if (PurgedEvent == NULL) {

            //
            // Release the locks and allocate pool for the event.
            //

            PurgedEvent = MiGetEventCounter ();
            if (PurgedEvent == NULL) {
                UNLOCK_PFN (OldIrql);
                return FALSE;
            }

            continue;
        }

        if (ControlArea->WaitingForDeletion == NULL) {
            ControlArea->WaitingForDeletion = PurgedEvent;
            WaitEvent = PurgedEvent;
            PurgedEvent = NULL;
        } else {
            WaitEvent = ControlArea->WaitingForDeletion;
            WaitEvent->RefCount += 1;
        }

        //
        // Release the pfn lock and wait for the event.
        //

        KeEnterCriticalRegion();
        UNLOCK_PFN_AND_THEN_WAIT(OldIrql);

        KeWaitForSingleObject(&WaitEvent->Event,
                              WrVirtualMemory,
                              KernelMode,
                              FALSE,
                              (PLARGE_INTEGER)NULL);
        LOCK_PFN (OldIrql);
        KeLeaveCriticalRegion();
        MiFreeEventCounter (WaitEvent, FALSE);
    }

    //
    // Indicate another file is mapped for the segment.
    //

    ControlArea->NumberOfMappedViews += 1;
    ControlArea->NumberOfUserReferences += 1;
    ControlArea->u.Flags.HadUserReference = 1;
    ASSERT (ControlArea->NumberOfSectionReferences != 0);

    if (PurgedEvent != NULL) {
        MiFreeEventCounter (PurgedEvent, TRUE);
    }
    UNLOCK_PFN (OldIrql);

    return TRUE;
}

typedef struct _NTSYM {
    struct _NTSYM *Next;
    PVOID SymbolTable;
    ULONG NumberOfSymbols;
    PVOID StringTable;
    USHORT Flags;
    USHORT EntrySize;
    ULONG MinimumVa;
    ULONG MaximumVa;
    PCHAR MapName;
    ULONG MapNameLen;
} NTSYM, *PNTSYM;

ULONG
CacheImageSymbols(
    IN PVOID ImageBase
    )
{
    PIMAGE_DEBUG_DIRECTORY DebugDirectory;
    ULONG DebugSize;

    PAGED_CODE();

    try {
        DebugDirectory = (PIMAGE_DEBUG_DIRECTORY)
        RtlImageDirectoryEntryToData( ImageBase,
                                      TRUE,
                                      IMAGE_DIRECTORY_ENTRY_DEBUG,
                                      &DebugSize
                                    );
        if (!DebugDirectory) {
            return FALSE;
        }

        //
        // If using remote KD, ImageBase is what it wants to see.
        //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        return FALSE;
    }

    return TRUE;
}


NTSYSAPI
NTSTATUS
NTAPI
NtAreMappedFilesTheSame (
    IN PVOID File1MappedAsAnImage,
    IN PVOID File2MappedAsFile
    )

/*++

Routine Description:

    This routine compares the two files mapped at the specified
    addresses to see if they are both the same file.

Arguments:

    File1MappedAsAnImage - Supplies an address within the first file which
        is mapped as an image file.

    File2MappedAsFile - Supplies an address within the second file which
        is mapped as either an image file or a data file.

Return Value:


    STATUS_SUCCESS is returned if the two files are the same.

    STATUS_NOT_SAME_DEVICE is returned if the files are different.

    Other status values can be returned if the addresses are not mapped as
    files, etc.

Environment:

    User mode callable system service.

--*/

{
    PMMVAD FoundVad1;
    PMMVAD FoundVad2;
    NTSTATUS Status;

    LOCK_ADDRESS_SPACE (PsGetCurrentProcess());
    FoundVad1 = MiLocateAddress (File1MappedAsAnImage);
    FoundVad2 = MiLocateAddress (File2MappedAsFile);

    if ((FoundVad1 == NULL) ||
        (FoundVad2 == NULL)) {

        //
        // No virtual address is allocated at the specified base address,
        // return an error.
        //

        Status = STATUS_INVALID_ADDRESS;
        goto ErrorReturn;
    }

    //
    // Check file names.
    //

    if ((FoundVad1->u.VadFlags.PrivateMemory == 1) ||
        (FoundVad2->u.VadFlags.PrivateMemory == 1)) {
        Status = STATUS_CONFLICTING_ADDRESSES;
        goto ErrorReturn;
    }

    if ((FoundVad1->ControlArea == NULL) ||
        (FoundVad2->ControlArea == NULL)) {
        Status = STATUS_CONFLICTING_ADDRESSES;
        goto ErrorReturn;
    }

    if ((FoundVad1->ControlArea->FilePointer == NULL) ||
        (FoundVad2->ControlArea->FilePointer == NULL)) {
        Status = STATUS_CONFLICTING_ADDRESSES;
        goto ErrorReturn;
    }

    Status = STATUS_NOT_SAME_DEVICE;

    if ((PVOID)FoundVad1->ControlArea ==
            FoundVad2->ControlArea->FilePointer->SectionObjectPointer->ImageSectionObject) {
        Status = STATUS_SUCCESS;
    }

ErrorReturn:

    UNLOCK_ADDRESS_SPACE (PsGetCurrentProcess());
    return Status;
}



VOID
MiSetPageModified (
    IN PVOID Address
    )

/*++

Routine Description:

    This routine sets the modified bit in the PFN database for the
    pages that correspond to the specified address range.

    Note that the dirty bit in the PTE is cleared by this operation.

Arguments:

    Address - Supplies the address of the start of the range.  This
              range must reside within the system cache.

Return Value:

    None.

Environment:

    Kernel mode.  APC_LEVEL and below for pagable addresses,
                  DISPATCH_LEVEL and below for non-pagable addresses.

--*/

{
    volatile PMMPTE PointerPpe;
    volatile PMMPTE PointerPde;
    volatile PMMPTE PointerPte;
    PMMPFN Pfn1;
    MMPTE PteContents;
    KIRQL OldIrql;

    //
    // Loop on the copy on write case until the page is only
    // writable.
    //

    PointerPte = MiGetPteAddress (Address);
    PointerPde = MiGetPdeAddress (Address);
    PointerPpe = MiGetPpeAddress (Address);

    *(volatile CCHAR *)Address;

    LOCK_PFN (OldIrql);

#if defined (_WIN64)
    while ((PointerPpe->u.Hard.Valid == 0) ||
           (PointerPde->u.Hard.Valid == 0) ||
           (PointerPte->u.Hard.Valid == 0))
#else
    while ((PointerPde->u.Hard.Valid == 0) ||
           (PointerPte->u.Hard.Valid == 0))
#endif
    {

        //
        // Page is no longer valid.
        //

        UNLOCK_PFN (OldIrql);
        *(volatile CCHAR *)Address;
        LOCK_PFN (OldIrql);
    }

    PteContents = *PointerPte;

    Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);
    Pfn1->u3.e1.Modified = 1;

    if ((Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
                 (Pfn1->u3.e1.WriteInProgress == 0)) {
        MiReleasePageFileSpace (Pfn1->OriginalPte);
        Pfn1->OriginalPte.u.Soft.PageFileHigh = 0;
    }

#ifdef NT_UP
    if (MI_IS_PTE_DIRTY (PteContents)) {
#endif //NT_UP
        MI_SET_PTE_CLEAN (PteContents);

        //
        // Clear the dirty bit in the PTE so new writes can be tracked.
        //

        (VOID)KeFlushSingleTb (Address,
                               FALSE,
                               TRUE,
                               (PHARDWARE_PTE)PointerPte,
                               PteContents.u.Flush);
#ifdef NT_UP
    }
#endif //NT_UP

    UNLOCK_PFN (OldIrql);
    return;
}


NTSTATUS
MmMapViewInSystemSpace (
    IN PVOID Section,
    OUT PVOID *MappedBase,
    IN OUT PSIZE_T ViewSize
    )

/*++

Routine Description:

    This routine maps the specified section into the system's address space.

Arguments:

    Section - Supplies a pointer to the section to map.

    *MappedBase - Returns the address where the section was mapped.

    ViewSize - Supplies the size of the view to map.  If this
               is specified as zero, the whole section is mapped.
               Returns the actual size mapped.

Return Value:

    Status of the map view operation.

Environment:

    Kernel Mode, IRQL of dispatch level.

--*/

{
    PMMSESSION  Session;

    PAGED_CODE();

    Session = &MmSession;

    return MiMapViewInSystemSpace (Section,
                                   Session,
                                   MappedBase,
                                   ViewSize);
}


NTSTATUS
MmMapViewInSessionSpace (
    IN PVOID Section,
    OUT PVOID *MappedBase,
    IN OUT PSIZE_T ViewSize
    )

/*++

Routine Description:

    This routine maps the specified section into the current process's
    session address space.

Arguments:

    Section - Supplies a pointer to the section to map.

    *MappedBase - Returns the address where the section was mapped.

    ViewSize - Supplies the size of the view to map.  If this
               is specified as zero, the whole section is mapped.
               Returns the actual size mapped.

Return Value:

    Status of the map view operation.

Environment:

    Kernel Mode, IRQL of dispatch level.

--*/

{
    PMMSESSION  Session;

    PAGED_CODE();

    if (MiHydra == TRUE) {
        if (PsGetCurrentProcess()->Vm.u.Flags.ProcessInSession == 0) {
            return STATUS_NOT_MAPPED_VIEW;
        }
        ASSERT (MmIsAddressValid(MmSessionSpace) == TRUE);
        Session = &MmSessionSpace->Session;
    }
    else {
        Session = &MmSession;
    }

    return MiMapViewInSystemSpace (Section,
                                   Session,
                                   MappedBase,
                                   ViewSize);
}


NTSTATUS
MiMapViewInSystemSpace (
    IN PVOID Section,
    IN PMMSESSION Session,
    OUT PVOID *MappedBase,
    IN OUT PSIZE_T ViewSize
    )

/*++

Routine Description:

    This routine maps the specified section into the system's address space.

Arguments:

    Section - Supplies a pointer to the section to map.

    Session - Supplies the session data structure for this view.

    *MappedBase - Returns the address where the section was mapped.

    ViewSize - Supplies the size of the view to map.  If this
               is specified as zero, the whole section is mapped.
               Returns the actual size mapped.

    Protect - Supplies the protection for the view.  Must be
              either PAGE_READWRITE or PAGE_READONLY.

Return Value:

    Status of the map view operation.

Environment:

    Kernel Mode, IRQL of APC_LEVEL or below.

--*/

{
    PVOID Base;
    KIRQL OldIrql;
    PSUBSECTION Subsection;
    PCONTROL_AREA ControlArea;
    PCONTROL_AREA NewControlArea;
    ULONG StartBit;
    ULONG SizeIn64k;
    ULONG NewSizeIn64k;
    PMMPTE BasePte;
    ULONG NumberOfPtes;
    ULONG NumberOfBytes;
    BOOLEAN status;
    KIRQL WsIrql;

    PAGED_CODE();

    //
    // Check to see if a purge operation is in progress and if so, wait
    // for the purge to complete.  In addition, up the count of mapped
    // views for this control area.
    //

    ControlArea = ((PSECTION)Section)->Segment->ControlArea;

    if (ControlArea->u.Flags.GlobalOnlyPerSession == 0) {
        Subsection = (PSUBSECTION)(ControlArea + 1);
    }
    else {
        Subsection = (PSUBSECTION)((PLARGE_CONTROL_AREA)ControlArea + 1);
    }

    MmLockPagableSectionByHandle(ExPageLockHandle);

    if (MiCheckPurgeAndUpMapCount (ControlArea) == FALSE) {
        MmUnlockPagableImageSection(ExPageLockHandle);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (*ViewSize == 0) {

        *ViewSize = ((PSECTION)Section)->SizeOfSection.LowPart;

    } else if (*ViewSize > ((PSECTION)Section)->SizeOfSection.LowPart) {

        //
        // Section offset or view size past size of section.
        //

        LOCK_PFN (OldIrql);
        ControlArea->NumberOfMappedViews -= 1;
        ControlArea->NumberOfUserReferences -= 1;

        //
        // Check to see if the control area (segment) should be deleted.
        // This routine releases the PFN lock.
        //
    
        MiCheckControlArea (ControlArea, NULL, OldIrql);

        MmUnlockPagableImageSection(ExPageLockHandle);
        return STATUS_INVALID_VIEW_SIZE;
    }

    //
    // Calculate the first prototype PTE field in the Vad.
    //

    SizeIn64k = (ULONG)((*ViewSize / X64K) + ((*ViewSize & (X64K - 1)) != 0));

    Base = MiInsertInSystemSpace (Session, SizeIn64k, ControlArea);

    if (Base == NULL) {
        LOCK_PFN (OldIrql);
        ControlArea->NumberOfMappedViews -= 1;
        ControlArea->NumberOfUserReferences -= 1;

        //
        // Check to see if the control area (segment) should be deleted.
        // This routine releases the PFN lock.
        //
    
        MiCheckControlArea (ControlArea, NULL, OldIrql);

        MmUnlockPagableImageSection(ExPageLockHandle);
        return STATUS_NO_MEMORY;
    }

    NumberOfBytes = SizeIn64k * X64K;

    if (Session == &MmSession) {
        status = MiFillSystemPageDirectory (Base, NumberOfBytes);
    }
    else {
        LOCK_SESSION_SPACE_WS (WsIrql);
        if (NT_SUCCESS(MiSessionCommitPageTables (Base,
                (PVOID)((ULONG_PTR)Base + NumberOfBytes - 1)))) {
                    status = TRUE;
        }
        else {
                    status = FALSE;
        }
    }

    if (status == FALSE) {
        if (Session != &MmSession) {
            UNLOCK_SESSION_SPACE_WS (WsIrql);
        }

        LOCK_PFN (OldIrql);
        ControlArea->NumberOfMappedViews -= 1;
        ControlArea->NumberOfUserReferences -= 1;

        //
        // Check to see if the control area (segment) should be deleted.
        // This routine releases the PFN lock.
        //
    
        MiCheckControlArea (ControlArea, NULL, OldIrql);

        StartBit = (ULONG) (((ULONG_PTR)Base - (ULONG_PTR)Session->SystemSpaceViewStart) >> 16);

        LOCK_SYSTEM_VIEW_SPACE (Session);

        NewSizeIn64k = MiRemoveFromSystemSpace (Session, Base, &NewControlArea);

        ASSERT (ControlArea == NewControlArea);
        ASSERT (SizeIn64k == NewSizeIn64k);

        RtlClearBits (Session->SystemSpaceBitMap, StartBit, SizeIn64k);

        UNLOCK_SYSTEM_VIEW_SPACE (Session);

        MmUnlockPagableImageSection(ExPageLockHandle);
        return STATUS_NO_MEMORY;
    }

    //
    // Setup PTEs to point to prototype PTEs.
    //

    if (((PSECTION)Section)->u.Flags.Image) {

#if DBG
        //
        // The only reason this ASSERT isn't done for Hydra is because
        // the session space working set lock is currently held so faults
        // are not allowed.
        //

        if (Session == &MmSession) {
            ASSERT (((PSECTION)Section)->Segment->ControlArea == ControlArea);
        }
#endif

        LOCK_PFN (OldIrql);
        ControlArea->u.Flags.ImageMappedInSystemSpace = 1;
        UNLOCK_PFN (OldIrql);
    }

    BasePte = MiGetPteAddress (Base);
    NumberOfPtes = BYTES_TO_PAGES (*ViewSize);

    MiAddMappedPtes (BasePte,
                     NumberOfPtes,
                     ControlArea);

    if (Session != &MmSession) {
        UNLOCK_SESSION_SPACE_WS (WsIrql);
    }

    *MappedBase = Base;

    MmUnlockPagableImageSection(ExPageLockHandle);
    return STATUS_SUCCESS;
}

BOOLEAN
MiFillSystemPageDirectory (
    PVOID Base,
    SIZE_T NumberOfBytes
    )

/*++

Routine Description:

    This routine allocates page tables and fills the system page directory
    entries for the specified virtual address range.

Arguments:

    Base - Supplies the virtual address of the view.

    NumberOfBytes - Supplies the number of bytes the view spans.

Return Value:

    TRUE on success, FALSE on failure.

Environment:

    Kernel Mode, IRQL of dispatch level.

--*/

{
    PMMPTE FirstPde;
    PMMPTE LastPde;
    PMMPTE FirstSystemPde;
    MMPTE TempPte;
    PFN_NUMBER PageFrameIndex;
    KIRQL OldIrql;
    ULONG i;

    PAGED_CODE();

    //
    // CODE IS ALREADY LOCKED BY CALLER.
    //

    FirstPde = MiGetPdeAddress (Base);
    LastPde = MiGetPdeAddress ((PVOID)(((PCHAR)Base) + NumberOfBytes - 1));

#if defined (_WIN64)
    FirstSystemPde = FirstPde;
#else
#if !defined (_X86PAE_)
    FirstSystemPde = &MmSystemPagePtes[((ULONG_PTR)FirstPde &
                     ((PDE_PER_PAGE * sizeof(MMPTE)) - 1)) / sizeof(MMPTE) ];
#else
    FirstSystemPde = &MmSystemPagePtes[((ULONG_PTR)FirstPde &
                     (PD_PER_SYSTEM * (PDE_PER_PAGE * sizeof(MMPTE)) - 1)) / sizeof(MMPTE) ];
#endif
#endif

    do {
        if (FirstSystemPde->u.Hard.Valid == 0) {

            //
            // No page table page exists, get a page and map it in.
            //

            TempPte = ValidKernelPde;

            LOCK_PFN (OldIrql);

            if (((volatile MMPTE *)FirstSystemPde)->u.Hard.Valid == 0) {

                if (MiEnsureAvailablePageOrWait (NULL, FirstPde)) {

                    //
                    // PFN_LOCK was dropped, redo this loop as another process
                    // could have made this PDE valid.
                    //

                    UNLOCK_PFN (OldIrql);
                    continue;
                }

                MiChargeCommitmentCantExpand (1, TRUE);
                MM_TRACK_COMMIT (MM_DBG_COMMIT_FILL_SYSTEM_DIRECTORY, 1);
                PageFrameIndex = MiRemoveAnyPage (
                                    MI_GET_PAGE_COLOR_FROM_PTE (FirstSystemPde));
                TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
                MI_WRITE_VALID_PTE (FirstSystemPde, TempPte);
                MI_WRITE_VALID_PTE (FirstPde, TempPte);

#if defined (_WIN64)
                MiInitializePfn (PageFrameIndex,
                                 FirstPde,
                                 1);
#else

#if !defined (_X86PAE_)
                MiInitializePfnForOtherProcess (PageFrameIndex,
                                                FirstPde,
                                                MmSystemPageDirectory);
#else
                i = (FirstPde - MiGetPdeAddress(0)) / PDE_PER_PAGE;
                MiInitializePfnForOtherProcess (PageFrameIndex,
                                                FirstPde,
                                                MmSystemPageDirectory[i]);
#endif

#endif

                MiFillMemoryPte (MiGetVirtualAddressMappedByPte (FirstPde),
                                 PAGE_SIZE,
                                 MM_ZERO_KERNEL_PTE);
            }
            UNLOCK_PFN (OldIrql);
        }

        FirstSystemPde += 1;
        FirstPde += 1;
    } while (FirstPde <= LastPde  );

    return TRUE;
}

NTSTATUS
MmUnmapViewInSystemSpace (
    IN PVOID MappedBase
    )

/*++

Routine Description:

    This routine unmaps the specified section from the system's address space.

Arguments:

    MappedBase - Supplies the address of the view to unmap.

Return Value:

    Status of the map view operation.

Environment:

    Kernel Mode, IRQL of dispatch level.

--*/

{
    PAGED_CODE();

    return MiUnmapViewInSystemSpace (&MmSession, MappedBase);
}

NTSTATUS
MmUnmapViewInSessionSpace (
    IN PVOID MappedBase
    )

/*++

Routine Description:

    This routine unmaps the specified section from the system's address space.

Arguments:

    MappedBase - Supplies the address of the view to unmap.

Return Value:

    Status of the map view operation.

Environment:

    Kernel Mode, IRQL of dispatch level.

--*/

{
    PMMSESSION Session;

    PAGED_CODE();

    if (MiHydra == TRUE) {
        if (PsGetCurrentProcess()->Vm.u.Flags.ProcessInSession == 0) {
            return STATUS_NOT_MAPPED_VIEW;
        }
        ASSERT (MmIsAddressValid(MmSessionSpace) == TRUE);
        Session = &MmSessionSpace->Session;
    }
    else {
        Session = &MmSession;
    }

    return MiUnmapViewInSystemSpace (Session, MappedBase);
}

NTSTATUS
MiUnmapViewInSystemSpace (
    IN PMMSESSION Session,
    IN PVOID MappedBase
    )

/*++

Routine Description:

    This routine unmaps the specified section from the system's address space.

Arguments:

    Session - Supplies the session data structure for this view.

    MappedBase - Supplies the address of the view to unmap.

Return Value:

    Status of the map view operation.

Environment:

    Kernel Mode, IRQL of dispatch level.

--*/

{
    ULONG StartBit;
    ULONG Size;
    PCONTROL_AREA ControlArea;
    PMMSUPPORT Ws;
    KIRQL WsIrql;

    PAGED_CODE();

    MmLockPagableSectionByHandle(ExPageLockHandle);
    StartBit =  (ULONG) (((ULONG_PTR)MappedBase - (ULONG_PTR)Session->SystemSpaceViewStart) >> 16);

    LOCK_SYSTEM_VIEW_SPACE (Session);

    Size = MiRemoveFromSystemSpace (Session, MappedBase, &ControlArea);

    RtlClearBits (Session->SystemSpaceBitMap, StartBit, Size);

    //
    // Zero PTEs.
    //

    Size = Size * (X64K >> PAGE_SHIFT);

    if (Session == &MmSession) {
        Ws = &MmSystemCacheWs;
    }
    else {
        Ws = &MmSessionSpace->Vm;
        LOCK_SESSION_SPACE_WS(WsIrql);
    }

    MiRemoveMappedPtes (MappedBase, Size, ControlArea, Ws);

    if (Session != &MmSession) {
        UNLOCK_SESSION_SPACE_WS(WsIrql);
    }

    UNLOCK_SYSTEM_VIEW_SPACE (Session);

    MmUnlockPagableImageSection(ExPageLockHandle);

    return STATUS_SUCCESS;
}


PVOID
MiInsertInSystemSpace (
    IN PMMSESSION Session,
    IN ULONG SizeIn64k,
    IN PCONTROL_AREA ControlArea
    )

/*++

Routine Description:

    This routine creates a view in system space for the specified control
    area (file mapping).

Arguments:

    SizeIn64k - Supplies the size of the view to be created.

    ControlArea - Supplies a pointer to the control area for this view.

Return Value:

    Base address where the view was mapped, NULL if the view could not be
    mapped.

Environment:

    Kernel Mode.

--*/

{

    PVOID Base;
    ULONG_PTR Entry;
    ULONG Hash;
    ULONG i;
    ULONG AllocSize;
    PMMVIEW OldTable;
    ULONG StartBit;
    ULONG NewHashSize;

    PAGED_CODE();

    //
    // CODE IS ALREADY LOCKED BY CALLER.
    //

    LOCK_SYSTEM_VIEW_SPACE (Session);

    if (Session->SystemSpaceHashEntries + 8 > Session->SystemSpaceHashSize) {

        //
        // Less than 8 free slots, reallocate and rehash.
        //

        NewHashSize = Session->SystemSpaceHashSize << 1;

        AllocSize = sizeof(MMVIEW) * NewHashSize;
        ASSERT (AllocSize < PAGE_SIZE);

        OldTable = Session->SystemSpaceViewTable;

        Session->SystemSpaceViewTable = ExAllocatePoolWithTag (PagedPool,
                                                               AllocSize,
                                                               '  mM');

        if (Session->SystemSpaceViewTable == NULL) {
            Session->SystemSpaceViewTable = OldTable;
        }
        else {
            RtlZeroMemory (Session->SystemSpaceViewTable, AllocSize);

            Session->SystemSpaceHashSize = NewHashSize;
            Session->SystemSpaceHashKey = Session->SystemSpaceHashSize - 1;

            for (i = 0; i < (Session->SystemSpaceHashSize / 2); i += 1) {
                if (OldTable[i].Entry != 0) {
                    Hash = (ULONG) ((OldTable[i].Entry >> 16) % Session->SystemSpaceHashKey);

                    while (Session->SystemSpaceViewTable[Hash].Entry != 0) {
                        Hash += 1;
                        if (Hash >= Session->SystemSpaceHashSize) {
                            Hash = 0;
                        }
                    }
                    Session->SystemSpaceViewTable[Hash] = OldTable[i];
                }
            }
            ExFreePool (OldTable);
        }
    }

    if (Session->SystemSpaceHashEntries == Session->SystemSpaceHashSize) {

        //
        // There are no free hash slots to place a new entry into even
        // though there may still be unused virtual address space.
        //

        UNLOCK_SYSTEM_VIEW_SPACE (Session);
        return NULL;
    }

    StartBit = RtlFindClearBitsAndSet (Session->SystemSpaceBitMap,
                                       SizeIn64k,
                                       0);

    if (StartBit == 0xFFFFFFFF) {
        UNLOCK_SYSTEM_VIEW_SPACE (Session);
        return NULL;
    }

    Base = (PVOID)((PCHAR)Session->SystemSpaceViewStart + (StartBit * X64K));

    Entry = (ULONG_PTR) MI_64K_ALIGN(Base) + SizeIn64k;

    Hash = (ULONG) ((Entry >> 16) % Session->SystemSpaceHashKey);

    while (Session->SystemSpaceViewTable[Hash].Entry != 0) {
        Hash += 1;
        if (Hash >= Session->SystemSpaceHashSize) {
            Hash = 0;
        }
    }

    Session->SystemSpaceHashEntries += 1;

    Session->SystemSpaceViewTable[Hash].Entry = Entry;
    Session->SystemSpaceViewTable[Hash].ControlArea = ControlArea;

    UNLOCK_SYSTEM_VIEW_SPACE (Session);
    return Base;
}


ULONG
MiRemoveFromSystemSpace (
    IN PMMSESSION Session,
    IN PVOID Base,
    OUT PCONTROL_AREA *ControlArea
    )

/*++

Routine Description:

    This routine looks up the specified view in the system space hash
    table and unmaps the view from system space and the table.

Arguments:

    Session - Supplies the session data structure for this view.

    Base - Supplies the base address for the view.  If this address is
           NOT found in the hash table, the system bugchecks.

    ControlArea - Returns the control area corresponding to the base
                  address.

Return Value:

    Size of the view divided by 64k.

Environment:

    Kernel Mode, system view hash table locked.

--*/

{
    ULONG_PTR Base16;
    ULONG Hash;
    ULONG Size;
    ULONG count;

    PAGED_CODE();

    count = 0;

    //
    // CODE IS ALREADY LOCKED BY CALLER.
    //

    Base16 = (ULONG_PTR)Base >> 16;
    Hash = (ULONG)(Base16 % Session->SystemSpaceHashKey);

    while ((Session->SystemSpaceViewTable[Hash].Entry >> 16) != Base16) {
        Hash += 1;
        if (Hash >= Session->SystemSpaceHashSize) {
            Hash = 0;
            count += 1;
            if (count == 2) {
                KeBugCheckEx (DRIVER_UNMAPPING_INVALID_VIEW,
                              (ULONG_PTR)Base,
                              MiHydra,
                              0,
                              0);
            }
        }
    }

    Session->SystemSpaceHashEntries -= 1;
    Size = (ULONG) (Session->SystemSpaceViewTable[Hash].Entry & 0xFFFF);
    Session->SystemSpaceViewTable[Hash].Entry = 0;
    *ControlArea = Session->SystemSpaceViewTable[Hash].ControlArea;
    return Size;
}


BOOLEAN
MiInitializeSystemSpaceMap (
    PVOID   InputSession OPTIONAL
    )

/*++

Routine Description:

    This routine initializes the tables for mapping views into system space.
    Views are kept in a multiple of 64k bytes in a growable hashed table.

Arguments:

    NULL if this is the initial system session (non-Hydra), a valid session
    pointer (the pointer must be in global space, not session space) for
    Hydra session initialization.

Return Value:

    TRUE on success, FALSE on failure.

Environment:

    Kernel Mode, initialization.

--*/

{
    ULONG AllocSize;
    ULONG Size;
    PCHAR ViewStart;
    PMMSESSION Session;

    if (ARGUMENT_PRESENT (InputSession)) {
        Session = (PMMSESSION)InputSession;
        ViewStart = (PCHAR)MI_SESSION_VIEW_START;
        Size = MI_SESSION_VIEW_SIZE;
    }
    else {
        Session = &MmSession;
        ViewStart = (PCHAR)MiSystemViewStart;
        if (MiHydra == TRUE) {
            Size = MM_SYSTEM_VIEW_SIZE_IF_HYDRA;
        }
        else {
            Size = MM_SYSTEM_VIEW_SIZE;
        }
    }

    //
    // We are passed a system global address for the address of the session.
    // Save a global pointer to the mutex below because multiple sessions will
    // generally give us a session-space (not a global space) pointer to the
    // MMSESSION in subsequent calls.  We need the global pointer for the mutex
    // field for the kernel primitives to work properly.
    //

    Session->SystemSpaceViewLockPointer = &Session->SystemSpaceViewLock;
    ExInitializeFastMutex(Session->SystemSpaceViewLockPointer);

    //
    // If the kernel image has not been biased to allow for 3gb of user space,
    // then the system space view starts at the defined place. Otherwise, it
    // starts 16mb above the kernel image.
    //

    Session->SystemSpaceViewStart = ViewStart;

    MiCreateBitMap (&Session->SystemSpaceBitMap, Size / X64K, NonPagedPool);
    if (Session->SystemSpaceBitMap == NULL) {
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_NONPAGED_POOL);
        return FALSE;
    }

    RtlClearAllBits (Session->SystemSpaceBitMap);

    //
    // Build the view table.
    //

    Session->SystemSpaceHashSize = 31;
    Session->SystemSpaceHashKey = Session->SystemSpaceHashSize - 1;
    Session->SystemSpaceHashEntries = 0;

    AllocSize = sizeof(MMVIEW) * Session->SystemSpaceHashSize;
    ASSERT (AllocSize < PAGE_SIZE);

    Session->SystemSpaceViewTable = ExAllocatePoolWithTag (PagedPool,
                                                           AllocSize,
                                                           '  mM');

    if (Session->SystemSpaceViewTable == NULL) {
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_SESSION_PAGED_POOL);
        MiRemoveBitMap (&Session->SystemSpaceBitMap);
        return FALSE;
    }

    RtlZeroMemory (Session->SystemSpaceViewTable, AllocSize);

    return TRUE;
}


VOID
MiFreeSessionSpaceMap (
    VOID
    )

/*++

Routine Description:

    This routine frees the tables used for mapping session views.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel Mode.  The caller must be in the correct session context.

--*/

{
    PMMSESSION Session;

    PAGED_CODE();

    Session = &MmSessionSpace->Session;

    //
    // Check for leaks of objects in the view table.
    //

    LOCK_SYSTEM_VIEW_SPACE (Session);

    if (Session->SystemSpaceViewTable && Session->SystemSpaceHashEntries) {

        KeBugCheckEx (SESSION_HAS_VALID_VIEWS_ON_EXIT,
                      (ULONG_PTR)MmSessionSpace->SessionId,
                      Session->SystemSpaceHashEntries,
                      (ULONG_PTR)&Session->SystemSpaceViewTable[0],
                      Session->SystemSpaceHashSize);

#if 0
        ULONG Index;

        for (Index = 0; Index < Session->SystemSpaceHashSize; Index += 1) {

            PMMVIEW Table;
            PVOID Base;

            Table = &Session->SystemSpaceViewTable[Index];

            if (Table->Entry) {

#if DBG
                DbgPrint ("MM: MiFreeSessionSpaceMap: view entry %d leak: ControlArea %p, Addr %p, Size %d\n",
                    Index,
                    Table->ControlArea,
                    Table->Entry & ~0xFFFF,
                    Table->Entry & 0x0000FFFF
                );
#endif

                Base = (PVOID)(Table->Entry & ~0xFFFF);

                //
                // MiUnmapViewInSystemSpace locks the ViewLock.
                //

                UNLOCK_SYSTEM_VIEW_SPACE(Session);

                MiUnmapViewInSystemSpace (Session, Base);

                LOCK_SYSTEM_VIEW_SPACE (Session);

                //
                // The view table may have been deleted while we let go of
                // the lock.
                //

                if (Session->SystemSpaceViewTable == NULL) {
                    break;
                }
            }
        }
#endif

    }

    UNLOCK_SYSTEM_VIEW_SPACE (Session);

    if (Session->SystemSpaceViewTable) {
        ExFreePool (Session->SystemSpaceViewTable);
        Session->SystemSpaceViewTable = NULL;
    }

    if (Session->SystemSpaceBitMap) {
        MiRemoveBitMap (&Session->SystemSpaceBitMap);
    }
}


HANDLE
MmSecureVirtualMemory (
    IN PVOID Address,
    IN SIZE_T Size,
    IN ULONG ProbeMode
    )

/*++

Routine Description:

    This routine probes the requested address range and protects
    the specified address range from having its protection made
    more restricted and being deleted.

    MmUnsecureVirtualMemory is used to allow the range to return
    to a normal state.

Arguments:

    Address - Supplies the base address to probe and secure.

    Size - Supplies the size of the range to secure.

    ProbeMode - Supplies one of PAGE_READONLY or PAGE_READWRITE.

Return Value:

    Returns a handle to be used to unsecure the range.
    If the range could not be locked because of protection
    problems or noncommitted memory, the value (HANDLE)0
    is returned.

Environment:

    Kernel Mode.

--*/

{
    ULONG_PTR EndAddress;
    PVOID StartAddress;
    CHAR Temp;
    ULONG Probe;
    HANDLE Handle;
    PMMVAD Vad;
    PMMVAD NewVad;
    PMMSECURE_ENTRY Secure;
    PEPROCESS Process;
    PMMPTE PointerPpe;
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    MMLOCK_CONFLICT Conflict;
    ULONG Waited;

    PAGED_CODE();

    if ((ULONG_PTR)Address + Size > (ULONG_PTR)MM_HIGHEST_USER_ADDRESS || (ULONG_PTR)Address + Size <= (ULONG_PTR)Address) {
        return (HANDLE)0;
    }

    Handle = (HANDLE)0;

    Probe = (ProbeMode == PAGE_READONLY);

    Process = PsGetCurrentProcess();
    StartAddress = Address;

    LOCK_ADDRESS_SPACE (Process);

    //
    // Check for a private committed VAD first instead of probing to avoid all
    // the page faults and zeroing.  If we find one, then we run the PTEs
    // instead.
    //

    if (Size >= 64 * 1024) {
        EndAddress = (ULONG_PTR)StartAddress + Size - 1;
        Vad = MiLocateAddress (StartAddress);

        if (Vad == NULL) {
            goto Return1;
        }

        if (Vad->u.VadFlags.UserPhysicalPages == 1) {
            goto Return1;
        }

        if (Vad->u.VadFlags.MemCommit == 0) {
            goto LongWay;
        }

        if (Vad->u.VadFlags.PrivateMemory == 0) {
            goto LongWay;
        }

        if (Vad->u.VadFlags.PhysicalMapping == 1) {
            goto LongWay;
        }

        ASSERT (Vad->u.VadFlags.Protection);

        if ((MI_VA_TO_VPN (StartAddress) < Vad->StartingVpn) ||
            (MI_VA_TO_VPN (EndAddress) > Vad->EndingVpn)) {
            goto Return1;
        }

        if (Vad->u.VadFlags.Protection == MM_NOACCESS) {
            goto LongWay;
        }

        if (ProbeMode == PAGE_READONLY) {
            if (Vad->u.VadFlags.Protection > MM_EXECUTE_WRITECOPY) {
                goto LongWay;
            }
        }
        else {
            if (Vad->u.VadFlags.Protection != MM_READWRITE &&
                Vad->u.VadFlags.Protection != MM_EXECUTE_READWRITE) {
                    goto LongWay;
            }
        }

        //
        // Check individual page permissions.
        //

        PointerPde = MiGetPdeAddress (StartAddress);
        PointerPpe = MiGetPteAddress (PointerPde);
        PointerPte = MiGetPteAddress (StartAddress);
        LastPte = MiGetPteAddress (EndAddress);

        LOCK_WS_UNSAFE (Process);

        do {

            while (MiDoesPpeExistAndMakeValid (PointerPpe,
                                               Process,
                                               FALSE,
                                               &Waited) == FALSE) {
                //
                // Page directory parent entry is empty, go to the next one.
                //

                PointerPpe += 1;
                PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
                if (PointerPte > LastPte) {
                    UNLOCK_WS_UNSAFE (Process);
                    goto EditVad;
                }
            }

            Waited = 0;

            while (MiDoesPdeExistAndMakeValid (PointerPde,
                                               Process,
                                               FALSE,
                                               &Waited) == FALSE) {
                //
                // This page directory entry is empty, go to the next one.
                //

                PointerPde += 1;
                PointerPpe = MiGetPteAddress (PointerPde);
                PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
                if (PointerPte > LastPte) {
                    UNLOCK_WS_UNSAFE (Process);
                    goto EditVad;
                }
#if defined (_WIN64)
                if (MiIsPteOnPdeBoundary (PointerPde)) {
                    Waited = 1;
                    break;
                }
#endif
            }

        } while (Waited != 0);

        while (PointerPte <= LastPte) {

            if (MiIsPteOnPdeBoundary (PointerPte)) {

                PointerPde = MiGetPteAddress (PointerPte);
                PointerPpe = MiGetPteAddress (PointerPde);

                do {

                    while (MiDoesPpeExistAndMakeValid (PointerPpe,
                                                       Process,
                                                       FALSE,
                                                       &Waited) == FALSE) {
                        //
                        // Page directory parent entry is empty, go to the next one.
                        //

                        PointerPpe += 1;
                        PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                        PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);

                        if (PointerPte > LastPte) {
                            UNLOCK_WS_UNSAFE (Process);
                            goto EditVad;
                        }
                    }

                    Waited = 0;

                    while (MiDoesPdeExistAndMakeValid (PointerPde,
                                                       Process,
                                                       FALSE,
                                                       &Waited) == FALSE) {
                        //
                        // This page directory entry is empty, go to the next one.
                        //

                        PointerPde += 1;
                        PointerPpe = MiGetPteAddress (PointerPde);
                        PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
                        if (PointerPte > LastPte) {
                            UNLOCK_WS_UNSAFE (Process);
                            goto EditVad;
                        }
#if defined (_WIN64)
                        if (MiIsPteOnPdeBoundary (PointerPde)) {
                            Waited = 1;
                            break;
                        }
#endif
                    }

                } while (Waited != 0);
            }
            if (PointerPte->u.Long) {
                UNLOCK_WS_UNSAFE (Process);
                goto LongWay;
            }
            PointerPte += 1;
        }
        UNLOCK_WS_UNSAFE (Process);
    }
    else {
LongWay:

        MiInsertConflictInList (&Conflict);

        try {

            if (ProbeMode == PAGE_READONLY) {

                EndAddress = (ULONG_PTR)Address + Size - 1;
                EndAddress = (EndAddress & ~(PAGE_SIZE - 1)) + PAGE_SIZE;

                do {
                    Temp = *(volatile CHAR *)Address;
                    Address = (PVOID)(((ULONG_PTR)Address & ~(PAGE_SIZE - 1)) + PAGE_SIZE);
                } while ((ULONG_PTR)Address != EndAddress);
            } else {
                ProbeForWrite (Address, (ULONG)Size, 1); // ****** temp ******
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
            MiRemoveConflictFromList (&Conflict);
            goto Return1;
        }

        MiRemoveConflictFromList (&Conflict);

        //
        // Locate VAD and add in secure descriptor.
        //

        EndAddress = (ULONG_PTR)StartAddress + Size - 1;
        Vad = MiLocateAddress (StartAddress);

        if (Vad == NULL) {
            goto Return1;
        }

        if (Vad->u.VadFlags.UserPhysicalPages == 1) {
            goto Return1;
        }

        if ((MI_VA_TO_VPN (StartAddress) < Vad->StartingVpn) ||
            (MI_VA_TO_VPN (EndAddress) > Vad->EndingVpn)) {

            //
            // Not within the section virtual address descriptor,
            // return an error.
            //

            goto Return1;
        }
    }

EditVad:

    //
    // If this is a short VAD, it needs to be reallocated as a large
    // VAD.
    //

    if ((Vad->u.VadFlags.PrivateMemory) && (!Vad->u.VadFlags.NoChange)) {

        NewVad = ExAllocatePoolWithTag (NonPagedPool,
                                        sizeof(MMVAD),
                                        MMVADKEY);
        if (NewVad == NULL) {
            goto Return1;
        }

        RtlZeroMemory (NewVad, sizeof(MMVAD));
        RtlCopyMemory (NewVad, Vad, sizeof(MMVAD_SHORT));
        NewVad->u.VadFlags.NoChange = 1;
        NewVad->u2.VadFlags2.OneSecured = 1;
        NewVad->u2.VadFlags2.StoredInVad = 1;
        NewVad->u2.VadFlags2.ReadOnly = Probe;
        NewVad->u3.Secured.StartVpn = (ULONG_PTR)StartAddress;
        NewVad->u3.Secured.EndVpn = EndAddress;

        //
        // Replace the current VAD with this expanded VAD.
        //

        LOCK_WS_UNSAFE (Process);
        if (Vad->Parent) {
            if (Vad->Parent->RightChild == Vad) {
                Vad->Parent->RightChild = NewVad;
            } else {
                ASSERT (Vad->Parent->LeftChild == Vad);
                Vad->Parent->LeftChild = NewVad;
            }
        } else {
            Process->VadRoot = NewVad;
        }
        if (Vad->LeftChild) {
            Vad->LeftChild->Parent = NewVad;
        }
        if (Vad->RightChild) {
            Vad->RightChild->Parent = NewVad;
        }
        if (Process->VadHint == Vad) {
            Process->VadHint = NewVad;
        }
        if (Process->VadFreeHint == Vad) {
            Process->VadFreeHint = NewVad;
        }

        if ((Vad->u.VadFlags.PhysicalMapping == 1) ||
            (Vad->u.VadFlags.WriteWatch == 1)) {

            MiPhysicalViewAdjuster (Process, Vad, NewVad);
        }

        UNLOCK_WS_UNSAFE (Process);
        ExFreePool (Vad);
        Handle = (HANDLE)&NewVad->u2.LongFlags2;
        goto Return1;
    }

    //
    // This is already a large VAD, add the secure entry.
    //

    if (Vad->u2.VadFlags2.OneSecured) {

        //
        // This VAD already is secured.  Move the info out of the
        // block into pool.
        //

        Secure = ExAllocatePoolWithTag (NonPagedPool,
                                        sizeof (MMSECURE_ENTRY),
                                        'eSmM');
        if (Secure == NULL) {
            goto Return1;
        }

        ASSERT (Vad->u.VadFlags.NoChange == 1);
        Vad->u2.VadFlags2.OneSecured = 0;
        Vad->u2.VadFlags2.MultipleSecured = 1;
        Secure->u2.LongFlags2 = (ULONG) Vad->u.LongFlags;
        Secure->u2.VadFlags2.StoredInVad = 0;
        Secure->StartVpn = Vad->u3.Secured.StartVpn;
        Secure->EndVpn = Vad->u3.Secured.EndVpn;

        InitializeListHead (&Vad->u3.List);
        InsertTailList (&Vad->u3.List,
                        &Secure->List);
    }

    if (Vad->u2.VadFlags2.MultipleSecured) {

        //
        // This VAD already has a secured element in its list, allocate and
        // add in the new secured element.
        //

        Secure = ExAllocatePoolWithTag (NonPagedPool,
                                        sizeof (MMSECURE_ENTRY),
                                        'eSmM');
        if (Secure == NULL) {
            goto Return1;
        }

        Secure->u2.LongFlags2 = 0;
        Secure->u2.VadFlags2.ReadOnly = Probe;
        Secure->StartVpn = (ULONG_PTR)StartAddress;
        Secure->EndVpn = EndAddress;

        InsertTailList (&Vad->u3.List,
                        &Secure->List);
        Handle = (HANDLE)Secure;

    } else {

        //
        // This list does not have a secure element.  Put it in the VAD.
        //

        Vad->u.VadFlags.NoChange = 1;
        Vad->u2.VadFlags2.OneSecured = 1;
        Vad->u2.VadFlags2.StoredInVad = 1;
        Vad->u2.VadFlags2.ReadOnly = Probe;
        Vad->u3.Secured.StartVpn = (ULONG_PTR)StartAddress;
        Vad->u3.Secured.EndVpn = EndAddress;
        Handle = (HANDLE)&Vad->u2.LongFlags2;
    }

Return1:
    UNLOCK_ADDRESS_SPACE (Process);
    return Handle;
}


VOID
MmUnsecureVirtualMemory (
    IN HANDLE SecureHandle
    )

/*++

Routine Description:

    This routine unsecures memory previous secured via a call to
    MmSecureVirtualMemory.

Arguments:

    SecureHandle - Supplies the handle returned in MmSecureVirtualMemory.

Return Value:

    None.

Environment:

    Kernel Mode.

--*/

{
    PMMSECURE_ENTRY Secure;
    PEPROCESS Process;
    PMMVAD Vad;

    PAGED_CODE();

    Secure = (PMMSECURE_ENTRY)SecureHandle;
    Process = PsGetCurrentProcess ();
    LOCK_ADDRESS_SPACE (Process);

    if (Secure->u2.VadFlags2.StoredInVad) {
        Vad = CONTAINING_RECORD( Secure,
                                 MMVAD,
                                 u2.LongFlags2);
    } else {
        Vad = MiLocateAddress ((PVOID)Secure->StartVpn);
    }

    ASSERT (Vad);
    ASSERT (Vad->u.VadFlags.NoChange == 1);

    if (Vad->u2.VadFlags2.OneSecured) {
        ASSERT (Secure == (PMMSECURE_ENTRY)&Vad->u2.LongFlags2);
        Vad->u2.VadFlags2.OneSecured = 0;
        ASSERT (Vad->u2.VadFlags2.MultipleSecured == 0);
        if (Vad->u2.VadFlags2.SecNoChange == 0) {

            //
            // No more secure entries in this list, remove the state.
            //

            Vad->u.VadFlags.NoChange = 0;
        }
    } else {
        ASSERT (Vad->u2.VadFlags2.MultipleSecured == 1);

        if (Secure == (PMMSECURE_ENTRY)&Vad->u2.LongFlags2) {

            //
            // This was a single block that got converted into a list.
            // Reset the entry.
            //

            Secure = CONTAINING_RECORD (Vad->u3.List.Flink,
                                        MMSECURE_ENTRY,
                                        List);
        }
        RemoveEntryList (&Secure->List);
        ExFreePool (Secure);
        if (IsListEmpty (&Vad->u3.List)) {

            //
            // No more secure entries, reset the state.
            //

            Vad->u2.VadFlags2.MultipleSecured = 0;

            if ((Vad->u2.VadFlags2.SecNoChange == 0) &&
               (Vad->u.VadFlags.PrivateMemory == 0)) {

                //
                // No more secure entries in this list, remove the state
                // if and only if this VAD is not private.  If this VAD
                // is private, removing the state NoChange flag indicates
                // that this is a short VAD which it no longer is.
                //

                Vad->u.VadFlags.NoChange = 0;
            }
        }
    }

    UNLOCK_ADDRESS_SPACE (Process);
    return;
}
