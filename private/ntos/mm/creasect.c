/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   creasect.c

Abstract:

    This module contains the routines which implement the
    NtCreateSection and NtOpenSection.

Author:

    Lou Perazzoli (loup) 22-May-1989
    Landy Wang (landyw) 02-Jun-1997

Revision History:

--*/

#include "mi.h"

ULONG MMCONTROL = 'aCmM';
ULONG MMTEMPORARY = 'xxmM';
ULONG MMSECT = 'tSmM';              // This is exported to special pool.

#define MM_SIZE_OF_LARGEST_IMAGE ((ULONG)0x77000000)

#define MM_MAXIMUM_IMAGE_HEADER (2 * PAGE_SIZE)

#define MM_ALLOCATION_FRAGMENT (64 * 1024)

//
// The maximum number of image object (object table entries) is
// the number which will fit into the MM_MAXIMUM_IMAGE_HEADER with
// the start of the PE image header in the last word of the first
//

#define MM_MAXIMUM_IMAGE_SECTIONS                       \
     ((MM_MAXIMUM_IMAGE_HEADER - (PAGE_SIZE + sizeof(IMAGE_NT_HEADERS))) /  \
            sizeof(IMAGE_SECTION_HEADER))

#if DBG
extern PEPROCESS MmWatchProcess;
VOID MmFooBar(VOID);
#endif // DBG

extern POBJECT_TYPE IoFileObjectType;

CCHAR MmImageProtectionArray[16] = {
                                    MM_NOACCESS,
                                    MM_EXECUTE,
                                    MM_READONLY,
                                    MM_EXECUTE_READ,
                                    MM_WRITECOPY,
                                    MM_EXECUTE_WRITECOPY,
                                    MM_WRITECOPY,
                                    MM_EXECUTE_WRITECOPY,
                                    MM_NOACCESS,
                                    MM_EXECUTE,
                                    MM_READONLY,
                                    MM_EXECUTE_READ,
                                    MM_READWRITE,
                                    MM_EXECUTE_READWRITE,
                                    MM_READWRITE,
                                    MM_EXECUTE_READWRITE };


CCHAR
MiGetImageProtection (
    IN ULONG SectionCharacteristics
    );

NTSTATUS
MiVerifyImageHeader (
    IN PIMAGE_NT_HEADERS NtHeader,
    IN PIMAGE_DOS_HEADER DosHeader,
    IN ULONG NtHeaderSize
    );

BOOLEAN
MiCheckDosCalls (
    IN PIMAGE_OS2_HEADER Os2Header,
    IN ULONG HeaderSize
    );

PCONTROL_AREA
MiFindImageSectionObject(
    IN PFILE_OBJECT File,
    IN PBOOLEAN GlobalNeeded
    );

VOID
MiInsertImageSectionObject(
    IN PFILE_OBJECT File,
    IN PCONTROL_AREA ControlArea
    );

VOID
MiFlushDataSection(
    IN PFILE_OBJECT File
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,MiCreateImageFileMap)
#pragma alloc_text(PAGE,NtCreateSection)
#pragma alloc_text(PAGE,NtOpenSection)
#pragma alloc_text(PAGE,MiGetImageProtection)
#pragma alloc_text(PAGE,MiVerifyImageHeader)
#pragma alloc_text(PAGE,MiCheckDosCalls)
#pragma alloc_text(PAGE,MiCreatePagingFileMap)
#pragma alloc_text(PAGE,MiCreateDataFileMap)

#pragma alloc_text(PAGEHYDRA,MiFindImageSectionObject)
#pragma alloc_text(PAGEHYDRA,MiInsertImageSectionObject)
#pragma alloc_text(PAGEHYDRA,MiRemoveImageSectionObject)
#pragma alloc_text(PAGEHYDRA,MiGetWritablePagesInSection)
#endif

#pragma pack (1)
typedef struct _PHARLAP_CONFIG {
    UCHAR  uchCopyRight[0x32];
    USHORT usType;
    USHORT usRsv1;
    USHORT usRsv2;
    USHORT usSign;
} CONFIGPHARLAP, *PCONFIGPHARLAP;
#pragma pack ()


NTSTATUS
NtCreateSection (
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER MaximumSize OPTIONAL,
    IN ULONG SectionPageProtection,
    IN ULONG AllocationAttributes,
    IN HANDLE FileHandle OPTIONAL
     )

/*++

Routine Description:

    This function creates a section object and opens a handle to the object
    with the specified desired access.

Arguments:

    SectionHandle - A pointer to a variable that will
        receive the section object handle value.

    DesiredAccess - The desired types of access for the section.

    DesiredAccess Flags

         EXECUTE - Execute access to the section is
              desired.

         READ - Read access to the section is desired.

         WRITE - Write access to the section is desired.


    ObjectAttributes - Supplies a pointer to an object attributes structure.

    MaximumSize - Supplies the maximum size of the section in bytes.
         This value is rounded up to the host page size and
         specifies the size of the section (page file
         backed section) or the maximum size to which a
         file can be extended or mapped (file backed
         section).

    SectionPageProtection - Supplies the protection to place on each page
         in the section.  One of PAGE_READ, PAGE_READWRITE, PAGE_EXECUTE,
         or PAGE_WRITECOPY and, optionally, PAGE_NOCACHE may be specified.

    AllocationAttributes - Supplies a set of flags that describe the
         allocation attributes of the section.

        AllocationAttributes Flags

        SEC_BASED - The section is a based section and will be
            allocated at the same virtual address in each process
            address space that receives the section.  This does not
            imply that addresses are reserved for based sections.
            Rather if the section cannot be mapped at the based address
            an error is returned.


        SEC_RESERVE - All pages of the section are set to the
            reserved state.

        SEC_COMMIT - All pages of the section are set to the commit
            state.

        SEC_IMAGE - The file specified by the file handle is an
                    executable image file.

        SEC_FILE - The file specified by the file handle is a mapped
                   file.  If a file handle is supplied and neither
                   SEC_IMAGE or SEC_FILE is supplied, SEC_FILE is
                   assumed.

        SEC_NO_CHANGE - Once the file is mapped, the protection cannot
                        be changed nor can the view be unmapped.  The
                        view is unmapped when the process is deleted.
                        Cannot be used with SEC_IMAGE.

    FileHandle - Supplies an optional handle of an open file object.
         If the value of this handle is null, then the
         section will be backed by a paging file. Otherwise
         the section is backed by the specified data file.

Return Value:

    Returns the status of the operation.

    TBS

--*/

{
    NTSTATUS Status;
    PVOID Section;
    HANDLE Handle;
    LARGE_INTEGER LargeSize;
    LARGE_INTEGER CapturedSize;
    ULONG RetryCount;

    if ((AllocationAttributes & ~(SEC_COMMIT | SEC_RESERVE | SEC_BASED |
            SEC_IMAGE | SEC_NOCACHE | SEC_NO_CHANGE)) != 0) {
        return STATUS_INVALID_PARAMETER_6;
    }

    if ((AllocationAttributes & (SEC_COMMIT | SEC_RESERVE | SEC_IMAGE)) == 0) {
        return STATUS_INVALID_PARAMETER_6;
    }

    if ((AllocationAttributes & SEC_IMAGE) &&
            (AllocationAttributes & (SEC_COMMIT | SEC_RESERVE |
                            SEC_NOCACHE | SEC_NO_CHANGE))) {

        return STATUS_INVALID_PARAMETER_6;
    }

    if ((AllocationAttributes & SEC_COMMIT) &&
            (AllocationAttributes & SEC_RESERVE)) {
        return STATUS_INVALID_PARAMETER_6;
    }

    //
    // Check the SectionProtection Flag.
    //

    if ((SectionPageProtection & PAGE_NOCACHE) ||
        (SectionPageProtection & PAGE_GUARD) ||
        (SectionPageProtection & PAGE_NOACCESS)) {

        //
        // No cache is only specified through SEC_NOCACHE option in the
        // allocation attributes.
        //

        return STATUS_INVALID_PAGE_PROTECTION;
    }


    if (KeGetPreviousMode() != KernelMode) {
        try {
            ProbeForWriteHandle(SectionHandle);

            if (ARGUMENT_PRESENT (MaximumSize)) {

                //
                // Note we only probe for byte alignment because prior to NT 5,
                // we never probed at all.  We don't want to break user apps
                // that had bad alignment if they worked before.
                //

                ProbeForRead(MaximumSize, sizeof(LARGE_INTEGER), sizeof(UCHAR));
                LargeSize = *MaximumSize;
            } else {
                ZERO_LARGE (LargeSize);
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
    } else {
        if (ARGUMENT_PRESENT (MaximumSize)) {
            LargeSize = *MaximumSize;
        } else {
            ZERO_LARGE (LargeSize);
        }
    }

    RetryCount = 0;

retry:

    CapturedSize = LargeSize;

    ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);
    Status = MmCreateSection ( &Section,
                               DesiredAccess,
                               ObjectAttributes,
                               &CapturedSize,
                               SectionPageProtection,
                               AllocationAttributes,
                               FileHandle,
                               NULL );


    ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);
    if (!NT_SUCCESS(Status)) {
        if ((Status == STATUS_FILE_LOCK_CONFLICT) &&
            (RetryCount < 3)) {

            //
            // The file system may have prevented this from working
            // due to log file flushing.  Delay and try again.
            //

            RetryCount += 1;

            KeDelayExecutionThread (KernelMode,
                                    FALSE,
                                    &MmHalfSecond);

            goto retry;

        }
        return Status;
    }

#if DBG
    if (MmDebug & MM_DBG_SECTIONS) {
        DbgPrint("inserting section %lx control %lx\n",Section,
                    ((PSECTION)Section)->Segment->ControlArea);
    }
#endif

    {
        PCONTROL_AREA ControlArea;
        ControlArea = ((PSECTION)Section)->Segment->ControlArea;
        if ((ControlArea != NULL) && (ControlArea->FilePointer != NULL)) {
            CcZeroEndOfLastPage (ControlArea->FilePointer);
        }
    }

    //
    // Note if the insertion fails, Ob will dereference the object for us.
    //

    Status = ObInsertObject (Section,
                             NULL,
                             DesiredAccess,
                             0,
                             (PVOID *)NULL,
                             &Handle);

    if (NT_SUCCESS(Status)) {
        try {
            *SectionHandle = Handle;
        } except (EXCEPTION_EXECUTE_HANDLER) {

            //
            // If the write attempt fails, then do not report an error.
            // When the caller attempts to access the handle value,
            // an access violation will occur.
            //
        }
    }

#if DBG
    if (MmDebug & MM_DBG_SHOW_NT_CALLS) {
        if ( !MmWatchProcess )
            DbgPrint("return creasect handle %lx status %lx\n",Handle, Status);
    }
#endif

    return Status;
}

NTSTATUS
MmCreateSection (
    OUT PVOID *SectionObject,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER InputMaximumSize,
    IN ULONG SectionPageProtection,
    IN ULONG AllocationAttributes,
    IN HANDLE FileHandle OPTIONAL,
    IN PFILE_OBJECT FileObject OPTIONAL
    )

/*++

Routine Description:

    This function creates a section object and opens a handle to the object
    with the specified desired access.

Arguments:

    Section - A pointer to a variable that will
              receive the section object address.

    DesiredAccess - The desired types of access for the section.

    DesiredAccess Flags

         EXECUTE - Execute access to the section is desired.

         READ - Read access to the section is desired.

         WRITE - Write access to the section is desired.

    ObjectAttributes - Supplies a pointer to an object attributes structure.

    InputMaximumSize - Supplies the maximum size of the section in bytes.
                       This value is rounded up to the host page size and
                       specifies the size of the section (page file
                       backed section) or the maximum size to which a
                       file can be extended or mapped (file backed
                       section).

    SectionPageProtection - Supplies the protection to place on each page
                            in the section.  One of PAGE_READ, PAGE_READWRITE,
                            PAGE_EXECUTE, or PAGE_WRITECOPY and, optionally,
                            PAGE_NOCACHE may be specified.

    AllocationAttributes - Supplies a set of flags that describe the
                           allocation attributes of the section.

        AllocationAttributes Flags

        SEC_BASED - The section is a based section and will be
                    allocated at the same virtual address in each process
                    address space that receives the section.  This does not
                    imply that addresses are reserved for based sections.
                    Rather if the section cannot be mapped at the based address
                    an error is returned.

        SEC_RESERVE - All pages of the section are set to the
                      reserved state.

        SEC_COMMIT - All pages of the section are set to the commit state.

        SEC_IMAGE - The file specified by the file handle is an
                    executable image file.

        SEC_FILE - The file specified by the file handle is a mapped
                   file.  If a file handle is supplied and neither
                   SEC_IMAGE or SEC_FILE is supplied, SEC_FILE is
                   assumed.

    FileHandle - Supplies an optional handle of an open file object.
                 If the value of this handle is null, then the
                 section will be backed by a paging file. Otherwise
                 the section is backed by the specified data file.

    FileObject - Supplies an optional pointer to the file object.  If this
                 value is NULL and the FileHandle is NULL, then there is
                 no file to map (image or mapped file).  If this value
                 is specified, then the File is to be mapped as a MAPPED FILE
                 and NO file size checking will be performed.

                 ONLY THE SYSTEM CACHE SHOULD PROVIDE A FILE OBJECT WITH THE
                 CALL!! as this is optimized to not check the size, only do
                 data mapping, no protection check, etc.

    Note - Only one of FileHandle or File should be specified!

Return Value:

    Returns the relevant NTSTATUS code.

--*/

{
    SECTION Section;
    PSECTION NewSection;
    PSEGMENT Segment;
    PSEGMENT NewSegment;
    KPROCESSOR_MODE PreviousMode;
    KIRQL OldIrql;
    NTSTATUS Status;
    PCONTROL_AREA ControlArea;
    PCONTROL_AREA NewControlArea;
    PCONTROL_AREA SegmentControlArea;
    ACCESS_MASK FileDesiredAccess;
    PFILE_OBJECT File;
    PEVENT_COUNTER Event;
    ULONG IgnoreFileSizing;
    ULONG ProtectionMask;
    ULONG ProtectMaskForAccess;
    ULONG FileAcquired;
    PEVENT_COUNTER SegmentEvent;
    BOOLEAN FileSizeChecked;
    LARGE_INTEGER TempSectionSize;
    UINT64 EndOfFile;
    ULONG IncrementedRefCount;
    ULONG ControlAreaSize;
    PUINT64 MaximumSize;
    PMMADDRESS_NODE *SectionBasedRoot;
    BOOLEAN GlobalNeeded;
    PFILE_OBJECT ChangeFileReference;
#if DBG
    PVOID PreviousSectionPointer;
#endif //DBG

    DesiredAccess;

    IgnoreFileSizing = FALSE;
    FileAcquired = FALSE;
    FileSizeChecked = FALSE;
    IncrementedRefCount = FALSE;
    ChangeFileReference = NULL;

    MaximumSize = (PUINT64) InputMaximumSize;

#if DBG
    if (MmDebug & MM_DBG_SHOW_NT_CALLS) {
        if ( !MmWatchProcess ) {
            DbgPrint("crea sect access mask %lx maxsize %I64X page prot %lx\n",
                DesiredAccess, *MaximumSize, SectionPageProtection);
            DbgPrint("     allocation attributes %lx file handle %lx\n",
                AllocationAttributes, FileHandle);
        }
    }
#endif

    //
    // Check allocation attributes flags.
    //

    File = (PFILE_OBJECT)NULL;

    ASSERT ((AllocationAttributes & ~(SEC_COMMIT | SEC_RESERVE | SEC_BASED |
            SEC_IMAGE | SEC_NOCACHE | SEC_NO_CHANGE)) == 0);

    ASSERT ((AllocationAttributes & (SEC_COMMIT | SEC_RESERVE | SEC_IMAGE)) != 0);

    ASSERT (!((AllocationAttributes & SEC_IMAGE) &&
            (AllocationAttributes & (SEC_COMMIT | SEC_RESERVE |
                            SEC_NOCACHE | SEC_NO_CHANGE))));

    ASSERT (!((AllocationAttributes & SEC_COMMIT) &&
            (AllocationAttributes & SEC_RESERVE)));

    ASSERT (!((SectionPageProtection & PAGE_NOCACHE) ||
        (SectionPageProtection & PAGE_GUARD) ||
        (SectionPageProtection & PAGE_NOACCESS)));

    if (AllocationAttributes & SEC_NOCACHE) {
        SectionPageProtection |= PAGE_NOCACHE;
    }

    //
    // Check the protection field.  This could raise an exception.
    //

    try {
        ProtectionMask = MiMakeProtectionMask (SectionPageProtection);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    ProtectMaskForAccess = ProtectionMask & 0x7;

    FileDesiredAccess = MmMakeFileAccess[ProtectMaskForAccess];

    //
    // Get previous processor mode and probe output arguments if necessary.
    //

    PreviousMode = KeGetPreviousMode();

    Section.InitialPageProtection = SectionPageProtection;
    Section.Segment = (PSEGMENT)NULL;

    if (ARGUMENT_PRESENT(FileHandle) || ARGUMENT_PRESENT(FileObject)) {

        if (ARGUMENT_PRESENT(FileObject)) {
            IgnoreFileSizing = TRUE;
            File = FileObject;

            //
            // Quick check to see if a control area already exists.
            //

            if (File->SectionObjectPointer->DataSectionObject) {

                LOCK_PFN (OldIrql);
                ControlArea =
                    (PCONTROL_AREA)(File->SectionObjectPointer->DataSectionObject);

                if ((ControlArea != NULL) &&
                    (!ControlArea->u.Flags.BeingDeleted) &&
                    (!ControlArea->u.Flags.BeingCreated)) {

                    //
                    // Control area exists and is not being deleted,
                    // reference it.
                    //

                    NewSegment = ControlArea->Segment;
                    if ((ControlArea->NumberOfSectionReferences == 0) &&
                        (ControlArea->NumberOfMappedViews == 0) &&
                        (ControlArea->ModifiedWriteCount == 0)) {

                        //
                        // Dereference the current file object and
                        // reference this one.
                        //

                        ChangeFileReference = ControlArea->FilePointer;
                        ControlArea->FilePointer = FileObject;

                        //
                        // This dereference is purposely at DPC_LEVEL
                        // so the object manager queues it to another
                        // thread thereby eliminating deadlocks with
                        // the redirector.
                        //

                        ObDereferenceObject (ChangeFileReference);
                    }
                    ControlArea->u.Flags.Accessed = 1;
                    ControlArea->NumberOfSectionReferences += 1;
                    if (ControlArea->DereferenceList.Flink != NULL) {

                        //
                        // Remove this from the list of unused segments.
                        //

                        RemoveEntryList (&ControlArea->DereferenceList);

                        MI_UNUSED_SEGMENTS_REMOVE_CHARGE (ControlArea);

                        ControlArea->DereferenceList.Flink = NULL;
                        ControlArea->DereferenceList.Blink = NULL;
                    }
                    UNLOCK_PFN (OldIrql);
                    IncrementedRefCount = TRUE;
                    Section.SizeOfSection.QuadPart = (LONGLONG)*MaximumSize;

                    goto ReferenceObject;
                }
                UNLOCK_PFN (OldIrql);
            }

            ObReferenceObject (FileObject);

        } else {

            //
            // Only one of FileHandle or FileObject should be supplied
            // if a FileObject is supplied, this must be from the
            // file system and therefore the file's size should not
            // be checked.
            //

            Status = ObReferenceObjectByHandle ( FileHandle,
                                                 FileDesiredAccess,
                                                 IoFileObjectType,
                                                 PreviousMode,
                                                 (PVOID *)&File,
                                                 NULL );
            if (!NT_SUCCESS(Status)) {
                return Status;
            }

            //
            // If this file doesn't have a section object pointer,
            // return an error.
            //

            if (File->SectionObjectPointer == NULL) {
                ObDereferenceObject (File);
                return STATUS_INVALID_FILE_FOR_SECTION;
            }
        }

        //
        // Check to see if the specified file already has a section.
        // If not, indicate in the file object's pointer to an FCB that
        // a section is being built.  This synchronizes segment creation
        // for the file.
        //

        if (MiHydra == TRUE && (AllocationAttributes & SEC_IMAGE)) {

            //
            // This control area is always just a place holder - the real one
            // is allocated in MiCreateImageFileMap and will be allocated
            // with the correct size and this one freed in a short while.
            //
            // This place holder must always be allocated as a large control
            // area so that it can be chained for the per-session case.
            //
            // This may need revisiting for wx86.
            //

            ControlAreaSize = (ULONG)sizeof(LARGE_CONTROL_AREA) +
                                                (ULONG)sizeof(SUBSECTION);
        }
        else {
            ControlAreaSize = (ULONG)sizeof(CONTROL_AREA) +
                                                (ULONG)sizeof(SUBSECTION);
        }

        NewControlArea = ExAllocatePoolWithTag (NonPagedPool,
                                                ControlAreaSize,
                                                MMCONTROL);

        if (NewControlArea == NULL) {
            ObDereferenceObject (File);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory (NewControlArea, ControlAreaSize);

        NewControlArea->NonPagedPoolUsage = EX_REAL_POOL_USAGE(ControlAreaSize);

        NewSegment = (PSEGMENT)NULL;

        //
        // We only need the file resource if the was a user request, i.e. not
        // a call from the cache manager or file system.
        //

        if (ARGUMENT_PRESENT(FileHandle)) {

            FsRtlAcquireFileExclusive (File);
            IoSetTopLevelIrp((PIRP)FSRTL_FSP_TOP_LEVEL_IRP);
            FileAcquired = TRUE;
        }

        LOCK_PFN (OldIrql);

        //
        // Allocate an event to wait on in case the segment is in the
        // process of being deleted.  This event cannot be allocated
        // with the PFN database locked as pool expansion would deadlock.
        //

ReallocateandcheckSegment:

        SegmentEvent = MiGetEventCounter();

        if (SegmentEvent == NULL) {
            UNLOCK_PFN (OldIrql);
            if (FileAcquired) {
                IoSetTopLevelIrp((PIRP)NULL);
                FsRtlReleaseFile (File);
            }
            ExFreePool (NewControlArea);
            ObDereferenceObject (File);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

RecheckSegment:

        if (AllocationAttributes & SEC_IMAGE) {

            if (MiHydra == TRUE) {
                ControlArea = MiFindImageSectionObject (File, &GlobalNeeded);
            }
            else {
                ControlArea =
                   (PCONTROL_AREA)(File->SectionObjectPointer->ImageSectionObject);
            }
        } else {
            ControlArea =
               (PCONTROL_AREA)(File->SectionObjectPointer->DataSectionObject);
        }

        if (ControlArea != NULL) {

            //
            // A segment already exists for this file.  Make sure that it
            // is not in the process of being deleted, or being created.
            //


            if ((ControlArea->u.Flags.BeingDeleted) ||
                (ControlArea->u.Flags.BeingCreated)) {

                //
                // The segment object is in the process of being deleted or
                // created.
                // Check to see if another thread is waiting for the deletion,
                // otherwise create an event object to wait upon.
                //

                if (ControlArea->WaitingForDeletion == NULL) {

                    //
                    // Initialize an event and put its address in the control area.
                    //

                    ControlArea->WaitingForDeletion = SegmentEvent;
                    Event = SegmentEvent;
                    SegmentEvent = NULL;
                } else {
                    Event = ControlArea->WaitingForDeletion;
                    Event->RefCount += 1;
                }

                //
                // Release the pfn lock, the file lock, and wait for the event.
                //

                UNLOCK_PFN (OldIrql);
                if (FileAcquired) {
                    IoSetTopLevelIrp((PIRP)NULL);
                    FsRtlReleaseFile (File);
                }

                KeWaitForSingleObject(&Event->Event,
                                      WrVirtualMemory,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER)NULL);

                if (FileAcquired) {
                    FsRtlAcquireFileExclusive (File);
                    IoSetTopLevelIrp((PIRP)FSRTL_FSP_TOP_LEVEL_IRP);
                }

                LOCK_PFN (OldIrql);
                MiFreeEventCounter (Event, TRUE);

                if (SegmentEvent == NULL) {

                    //
                    // The event was freed from pool, allocate another
                    // event in case we have to synchronize one more time.
                    //

                    goto ReallocateandcheckSegment;
                }
                goto RecheckSegment;

            } else {

                //
                // There is already a segment for this file, have
                // this section refer to that segment.
                // No need to reference the file object any more.
                //

                NewSegment = ControlArea->Segment;
                ControlArea->u.Flags.Accessed = 1;
                ControlArea->NumberOfSectionReferences += 1;
                if (ControlArea->DereferenceList.Flink != NULL) {

                    //
                    // Remove this from the list of unused segments.
                    //

                    RemoveEntryList (&ControlArea->DereferenceList);

                    MI_UNUSED_SEGMENTS_REMOVE_CHARGE (ControlArea);

                    ControlArea->DereferenceList.Flink = NULL;
                    ControlArea->DereferenceList.Blink = NULL;
                }
                IncrementedRefCount = TRUE;

                //
                // If this reference was not from the cache manager
                // up the count of user references.
                //

                if (IgnoreFileSizing == FALSE) {
                    ControlArea->NumberOfUserReferences += 1;
                }
            }
        } else {

            //
            // There is no segment associated with this file object.
            // Set the file object to refer to the new control area.
            //

            ControlArea = NewControlArea;
            ControlArea->u.Flags.BeingCreated = 1;

            if (AllocationAttributes & SEC_IMAGE) {
                if (MiHydra == TRUE) {
                    if (GlobalNeeded == TRUE) {
                        ControlArea->u.Flags.GlobalOnlyPerSession = 1;
                    }

                    MiInsertImageSectionObject (File, ControlArea);
                }
                else {
                    ((PCONTROL_AREA)((File->SectionObjectPointer->ImageSectionObject))) =
                                                                ControlArea;
                }
            } else {
#if DBG
                PreviousSectionPointer = File->SectionObjectPointer;
#endif //DBG
                ((PCONTROL_AREA)((File->SectionObjectPointer->DataSectionObject))) =
                                                                ControlArea;
            }
        }

        if (SegmentEvent != NULL) {
            MiFreeEventCounter (SegmentEvent, TRUE);
        }

        UNLOCK_PFN (OldIrql);
        
        if (NewSegment != (PSEGMENT)NULL) {

            //
            // Whether we created a segment or not, flush the data section
            // if there is one.
            //

            if (AllocationAttributes & SEC_IMAGE) {
                MiFlushDataSection (File);
            }

            //
            // A segment already exists for this file object.
            // Deallocate the new control area as it is no longer required.
            // Dereference the file object later when we're done with it.
            //

            ExFreePool (NewControlArea);

            //
            // The section is in paged pool, this can't be set until
            // the PFN mutex has been released.
            //

            if ((!IgnoreFileSizing) && (ControlArea->u.Flags.Image == 0)) {

                //
                // The file size in the segment may not match the current
                // file size, query the file system and get the file
                // size.
                //

                Status = FsRtlGetFileSize (File, (PLARGE_INTEGER)&EndOfFile );

                if (!NT_SUCCESS (Status)) {

                    if (FileAcquired) {
                        IoSetTopLevelIrp((PIRP)NULL);
                        FsRtlReleaseFile (File);
                        FileAcquired = FALSE;
                    }

                    ObDereferenceObject (File);
                    goto UnrefAndReturn;
                }

                if (EndOfFile == 0 && *MaximumSize == 0) {

                    //
                    // Can't map a zero length without specifying the maximum
                    // size as non-zero.
                    //

                    Status = STATUS_MAPPED_FILE_SIZE_ZERO;

                    if (FileAcquired) {
                        IoSetTopLevelIrp((PIRP)NULL);
                        FsRtlReleaseFile (File);
                        FileAcquired = FALSE;
                    }

                    ObDereferenceObject (File);
                    goto UnrefAndReturn;
                }
            } else {

                //
                // The size is okay in the segment.
                //

                EndOfFile = (UINT64) NewSegment->SizeOfSegment;
            }

            if (FileAcquired) {
                IoSetTopLevelIrp((PIRP)NULL);
                FsRtlReleaseFile (File);
                FileAcquired = FALSE;
            }

            ObDereferenceObject (File);

            if (*MaximumSize == 0) {

                Section.SizeOfSection.QuadPart = (LONGLONG)EndOfFile;
                FileSizeChecked = TRUE;

            } else if (EndOfFile >= *MaximumSize) {

                //
                // EndOfFile is greater than the MaximumSize,
                // use the specified maximum size.
                //

                Section.SizeOfSection.QuadPart = (LONGLONG)*MaximumSize;
                FileSizeChecked = TRUE;

            } else {

                //
                // Need to extend the section, make sure the file was
                // opened for write access.
                //

                if (((SectionPageProtection & PAGE_READWRITE) |
                    (SectionPageProtection & PAGE_EXECUTE_READWRITE)) == 0) {

                    Status = STATUS_SECTION_TOO_BIG;
                    goto UnrefAndReturn;
                }
                Section.SizeOfSection.QuadPart = (LONGLONG)*MaximumSize;
            }

        } else {

            //
            // The file does not have an associated segment, create a segment
            // object.
            //

            if (AllocationAttributes & SEC_IMAGE) {

                Status = MiCreateImageFileMap (File, &Segment);

            } else {

                Status = MiCreateDataFileMap (File,
                                              &Segment,
                                              MaximumSize,
                                              SectionPageProtection,
                                              AllocationAttributes,
                                              IgnoreFileSizing );
                ASSERT (PreviousSectionPointer == File->SectionObjectPointer);
            }

            if (!NT_SUCCESS(Status)) {

                //
                // Lock the PFN database and check to see if another thread has
                // tried to create a segment to the file object at the same
                // time.
                //

                LOCK_PFN (OldIrql);

                Event = ControlArea->WaitingForDeletion;
                ControlArea->WaitingForDeletion = NULL;
                ASSERT (ControlArea->u.Flags.FilePointerNull == 0);
                ControlArea->u.Flags.FilePointerNull = 1;

                if (AllocationAttributes & SEC_IMAGE) {
                    if (MiHydra == TRUE) {
                        MiRemoveImageSectionObject (File, ControlArea);
                    }
                    else {
                        (PCONTROL_AREA)((File->SectionObjectPointer->ImageSectionObject)) =
                                                                        NULL;
                    }
                } else {
                    (PCONTROL_AREA)((File->SectionObjectPointer->DataSectionObject)) =
                                                                        NULL;
                }
                ControlArea->u.Flags.BeingCreated = 0;

                UNLOCK_PFN (OldIrql);

                if (FileAcquired) {
                    IoSetTopLevelIrp((PIRP)NULL);
                    FsRtlReleaseFile (File);
                }

                ExFreePool (NewControlArea);

                ObDereferenceObject (File);

                if (Event != NULL) {

                    //
                    // Signal any waiters that the segment structure exists.
                    //

                    KeSetEvent (&Event->Event, 0, FALSE);
                }

                return Status;
            }

            //
            // If the size was specified as zero, set the section size
            // from the created segment size.  This solves problems with
            // race conditions when multiple sections
            // are created for the same mapped file with varying sizes.
            //

            if (*MaximumSize == 0) {
                Section.SizeOfSection.QuadPart = (LONGLONG)Segment->SizeOfSegment;
            } else {
                Section.SizeOfSection.QuadPart = (LONGLONG)*MaximumSize;
            }
        }

    } else {

        //
        // No file handle exists, this is a page file backed section.
        //

        if (AllocationAttributes & SEC_IMAGE) {
            return STATUS_INVALID_FILE_FOR_SECTION;
        }

        Status = MiCreatePagingFileMap ( &NewSegment,
                                         MaximumSize,
                                         ProtectionMask,
                                         AllocationAttributes);

        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        //
        // Set the section size from the created segment size.  This
        // solves problems with race conditions when multiple sections
        // are created for the same mapped file with varying sizes.
        //

        Section.SizeOfSection.QuadPart = (LONGLONG)NewSegment->SizeOfSegment;
        ControlArea = NewSegment->ControlArea;
    }

#if DBG
    if (MmDebug & MM_DBG_SECTIONS) {
        if (NewSegment == (PSEGMENT)NULL) {
            DbgPrint("inserting segment %lx control %lx\n",Segment,
                        Segment->ControlArea);
        } else {
            DbgPrint("inserting segment %lx control %lx\n",NewSegment,
                        NewSegment->ControlArea);
        }
    }
#endif


    if (NewSegment == (PSEGMENT)NULL) {
        NewSegment = Segment;

        //
        // Lock the PFN database and check to see if another thread has
        // tried to create a segment to the file object at the same time.
        //

        SegmentControlArea = Segment->ControlArea;

        ASSERT (File != NULL);

        LOCK_PFN (OldIrql);

        Event = ControlArea->WaitingForDeletion;
        ControlArea->WaitingForDeletion = NULL;

        if (AllocationAttributes & SEC_IMAGE) {

            //
            // Change the control area in the file object pointer.
            //

            if (MiHydra == TRUE) {
                MiRemoveImageSectionObject (File, NewControlArea);
                MiInsertImageSectionObject (File, SegmentControlArea);
            }
            else {
                ((PCONTROL_AREA)(File->SectionObjectPointer->ImageSectionObject)) =
                                                         SegmentControlArea;
            }

            ControlArea = SegmentControlArea;
        }

        ControlArea->u.Flags.BeingCreated = 0;

        UNLOCK_PFN (OldIrql);

        if (AllocationAttributes & SEC_IMAGE) {

            //
            // Deallocate the pool used for the original control area.
            //

            ExFreePool (NewControlArea);
        }

        if (Event != NULL) {

            //
            // Signal any waiters that the segment structure exists.
            //

            KeSetEvent (&Event->Event, 0, FALSE);
        }

        PERFINFO_SECTION_CREATE(ControlArea);
    }

    //
    // Being created has now been cleared allowing other threads
    // to reference the segment.  Release the resource on the file.
    //

    if (FileAcquired) {
        IoSetTopLevelIrp((PIRP)NULL);
        FsRtlReleaseFile (File);
        FileAcquired = FALSE;
    }

ReferenceObject:

    if (ChangeFileReference) {
        ObReferenceObject (FileObject);
    }

    //
    // Now that the segment object is created, make the section object
    // refer to the segment object.
    //

    Section.Segment = NewSegment;
    Section.u.LongFlags = ControlArea->u.LongFlags;

    //
    // Create the section object now.  The section object is created
    // now so that the error handling when the section object cannot
    // be created is simplified.
    //

    Status = ObCreateObject (PreviousMode,
                             MmSectionObjectType,
                             ObjectAttributes,
                             PreviousMode,
                             NULL,
                             sizeof(SECTION),
                             sizeof(SECTION) +
                                 NewSegment->TotalNumberOfPtes * sizeof(MMPTE),
                             sizeof(CONTROL_AREA) +
                                 NewSegment->ControlArea->NumberOfSubsections *
                                   sizeof(SUBSECTION),
                             (PVOID *)&NewSection);

    if (!NT_SUCCESS(Status)) {
        goto UnrefAndReturn;
    }

    RtlMoveMemory (NewSection, &Section, sizeof(SECTION));
    NewSection->Address.StartingVpn = 0;

    if (!IgnoreFileSizing) {

        //
        // Indicate that the cache manager is not the owner of this
        // section.
        //

        NewSection->u.Flags.UserReference = 1;

        if (AllocationAttributes & SEC_NO_CHANGE) {

            //
            // Indicate that once the section is mapped, no protection
            // changes or freeing the mapping is allowed.
            //

            NewSection->u.Flags.NoChange = 1;
        }

        if (((SectionPageProtection & PAGE_READWRITE) |
            (SectionPageProtection & PAGE_EXECUTE_READWRITE)) == 0) {

            //
            // This section does not support WRITE access, indicate
            // that changing the protection to WRITE results in COPY_ON_WRITE.
            //

            NewSection->u.Flags.CopyOnWrite = 1;
        }

        if (AllocationAttributes & SEC_BASED) {

            NewSection->u.Flags.Based = 1;

            SectionBasedRoot = &MmSectionBasedRoot;

            //
            // Get the allocation base mutex.
            //

            ExAcquireFastMutex (&MmSectionBasedMutex);

            //
            // This section is based at a unique address system wide.
            //

            try {
                NewSection->Address.StartingVpn = (ULONG_PTR)MiFindEmptySectionBaseDown (
                                                 NewSection->SizeOfSection.LowPart,
                                                 MmHighSectionBase);

            } except (EXCEPTION_EXECUTE_HANDLER) {
                ExReleaseFastMutex (&MmSectionBasedMutex);
                ObDereferenceObject (NewSection);
                return Status;
            }

            NewSection->Address.EndingVpn = NewSection->Address.StartingVpn +
                                  NewSection->SizeOfSection.LowPart - 1;

            MiInsertBasedSection (NewSection);
            ExReleaseFastMutex (&MmSectionBasedMutex);
        }
    }

    //
    // If the cache manager is creating the section, set the was
    // purged flag as the file size can change.
    //

    ControlArea->u.Flags.WasPurged |= IgnoreFileSizing;

    //
    // Check to see if the section is for a data file and the size
    // of the section is greater than the current size of the
    // segment.
    //

    if (((ControlArea->u.Flags.WasPurged == 1) && (!IgnoreFileSizing)) &&
                      (!FileSizeChecked)
                            ||
        ((UINT64)NewSection->SizeOfSection.QuadPart >
                                NewSection->Segment->SizeOfSegment)) {

        TempSectionSize = NewSection->SizeOfSection;

        NewSection->SizeOfSection.QuadPart = (LONGLONG)NewSection->Segment->SizeOfSegment;

        //
        // Even if the caller didn't specify extension rights, we enable it here
        // temporarily to make the section correct.  Use a temporary section
        // instead of temporarily editing the real section to avoid opening
        // a security window that other concurrent threads could exploit.
        //

        if (((NewSection->InitialPageProtection & PAGE_READWRITE) |
            (NewSection->InitialPageProtection & PAGE_EXECUTE_READWRITE)) == 0) {
                SECTION     WritableSection;

                *(PSECTION)&WritableSection = *NewSection;

                Status = MmExtendSection (&WritableSection,
                                          &TempSectionSize,
                                          IgnoreFileSizing);

                NewSection->SizeOfSection = WritableSection.SizeOfSection;
        }
        else {
            Status = MmExtendSection (NewSection,
                                      &TempSectionSize,
                                      IgnoreFileSizing);
        }

        if (!NT_SUCCESS(Status)) {
            ObDereferenceObject (NewSection);
            return Status;
        }
    }

    *SectionObject = (PVOID)NewSection;

    return Status;

UnrefAndReturn:

    //
    // Unreference the control area, if it was referenced and return
    // the error status.
    //

    if (FileAcquired) {
        IoSetTopLevelIrp((PIRP)NULL);
        FsRtlReleaseFile (File);
    }

    if (IncrementedRefCount) {
        LOCK_PFN (OldIrql);
        ControlArea->NumberOfSectionReferences -= 1;
        if (!IgnoreFileSizing) {
            ASSERT ((LONG)ControlArea->NumberOfUserReferences > 0);
            ControlArea->NumberOfUserReferences -= 1;
        }
        MiCheckControlArea (ControlArea, NULL, OldIrql);
    }
    return Status;
}

NTSTATUS
MiCreateImageFileMap (
    IN PFILE_OBJECT File,
    OUT PSEGMENT *Segment
    )

/*++

Routine Description:

    This function creates the necessary structures to allow the mapping
    of an image file.

    The image file is opened and verified for correctness, a segment
    object is created and initialized based on data in the image
    header.

Arguments:

    File - Supplies the file object for the image file.

    Segment - Returns the segment object.

Return Value:

    Returns the status value.

    TBS


--*/

{
    NTSTATUS Status;
    ULONG_PTR EndingAddress;
    ULONG NumberOfPtes;
    ULONG SizeOfSegment;
    ULONG SectionVirtualSize;
    ULONG i;
    ULONG j;
    PCONTROL_AREA ControlArea;
    PSUBSECTION Subsection;
    PMMPTE PointerPte;
    MMPTE TempPte;
    MMPTE TempPteDemandZero;
    PVOID Base;
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_FILE_HEADER FileHeader;
    ULONG SizeOfImage;
    ULONG SizeOfHeaders;
#if defined(_WIN64)
    PIMAGE_NT_HEADERS32 NtHeader32;
#endif
    PIMAGE_SECTION_HEADER SectionTableEntry;
    PSEGMENT NewSegment;
    ULONG SectorOffset;
    ULONG NumberOfSubsections;
    PFN_NUMBER PageFrameNumber;
    LARGE_INTEGER StartingOffset;
    PCHAR ExtendedHeader;
    PPFN_NUMBER Page;
    ULONG_PTR PreferredImageBase;
    ULONG_PTR NextVa;
    PKEVENT InPageEvent;
    PMDL Mdl;
    ULONG ImageFileSize;
    ULONG OffsetToSectionTable;
    ULONG ImageAlignment;
    ULONG RoundingAlignment;
    ULONG FileAlignment;
    BOOLEAN ImageCommit;
    BOOLEAN SectionCommit;
    IO_STATUS_BLOCK IoStatus;
    LARGE_INTEGER EndOfFile;
    ULONG NtHeaderSize;
    ULONG SubsectionsAllocated;
    PLARGE_CONTROL_AREA LargeControlArea;
    PSUBSECTION NewSubsection;
    ULONG OriginalProtection;
    ULONG LoaderFlags;

#if defined (_ALPHA_) || defined(_IA64_)
    //[barrybo] bugbug:  The image alignment code should be switched back
    //                   out on IA64 when EMNT supports 4k pages for WOW64.
    BOOLEAN InvalidAlignmentAllowed = FALSE;

    ULONG TempNumberOfSubsections;
    PIMAGE_SECTION_HEADER TempSectionTableEntry;
    ULONG AdditionalSubsections;
    ULONG AdditionalPtes;
    ULONG AdditionalBasePtes;
    ULONG NewSubsectionsAllocated; 
    PSEGMENT OldSegment;
    PMMPTE NewPointerPte;
    PMMPTE OldPointerPte;
    ULONG OrigNumberOfPtes;
    PCONTROL_AREA NewControlArea;

#endif

    ExtendedHeader = NULL;

    // *************************************************************
    // Create image file section.
    // *************************************************************

    PAGED_CODE();


    Status = FsRtlGetFileSize (File, &EndOfFile);

    if (Status == STATUS_FILE_IS_A_DIRECTORY) {

        //
        // Can't map a directory as a section. Return error.
        //

        return STATUS_INVALID_FILE_FOR_SECTION;
    }

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    if (EndOfFile.HighPart != 0) {

        //
        // File too big. Return error.
        //

        return STATUS_INVALID_FILE_FOR_SECTION;
    }

    //
    // Create a segment which maps an image file.
    // For now map a COFF image file with the subsections
    // containing the based address of the file.
    //

    //
    // Read in the file header.
    //

    InPageEvent = ExAllocatePoolWithTag (NonPagedPool,
                                  sizeof(KEVENT) + MmSizeOfMdl (
                                                      NULL,
                                                      MM_MAXIMUM_IMAGE_HEADER),
                                                      MMTEMPORARY);
    if (InPageEvent == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Mdl = (PMDL)(InPageEvent + 1);

    //
    // Create an event for the read operation.
    //

    KeInitializeEvent (InPageEvent, NotificationEvent, FALSE);

    //
    // Build an MDL for the operation.
    //

    MmCreateMdl( Mdl, NULL, PAGE_SIZE);
    Mdl->MdlFlags |= MDL_PAGES_LOCKED;

    PageFrameNumber = MiGetPageForHeader();

    Page = (PPFN_NUMBER)(Mdl + 1);
    *Page = PageFrameNumber;

    ZERO_LARGE (StartingOffset);

    CcZeroEndOfLastPage (File);

    //
    // Flush the data section if there is one.
    //

    MiFlushDataSection (File);

    Mdl->MdlFlags |= MDL_PAGES_LOCKED;
    Status = IoPageRead (File,
                         Mdl,
                         &StartingOffset,
                         InPageEvent,
                         &IoStatus
                        );

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject( InPageEvent,
                               WrPageIn,
                               KernelMode,
                               FALSE,
                               (PLARGE_INTEGER)NULL);
    }

    if (Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) {
        MmUnmapLockedPages (Mdl->MappedSystemVa, Mdl);
    }

    if ((!NT_SUCCESS(Status)) || (!NT_SUCCESS(IoStatus.Status))) {
        if (Status != STATUS_FILE_LOCK_CONFLICT) {
            Status = STATUS_INVALID_FILE_FOR_SECTION;
        }
        goto BadSection;
    }

    Base = MiMapImageHeaderInHyperSpace (PageFrameNumber);
    DosHeader = (PIMAGE_DOS_HEADER)Base;

    if (IoStatus.Information != PAGE_SIZE) {

        //
        // A full page was not read from the file, zero any remaining
        // bytes.
        //

        RtlZeroMemory ((PVOID)((PCHAR)Base + IoStatus.Information),
                       PAGE_SIZE - IoStatus.Information);
    }

    //
    // Check to determine if this is an NT image (PE format) or
    // a DOS image, Win-16 image, or OS/2 image.  If the image is
    // not NT format, return an error indicating which image it
    // appears to be.
    //

    if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE) {

        Status = STATUS_INVALID_IMAGE_NOT_MZ;
        goto NeImage;
    }

#ifndef i386
    if (((ULONG)DosHeader->e_lfanew & 3) != 0) {

        //
        // The image header is not aligned on a longword boundary.
        // Report this as an invalid protect mode image.
        //

        Status = STATUS_INVALID_IMAGE_PROTECT;
        goto NeImage;
    }
#endif

    if ((ULONG)DosHeader->e_lfanew > EndOfFile.LowPart) {
        Status = STATUS_INVALID_IMAGE_PROTECT;
        goto NeImage;
    }

    if (((ULONG)DosHeader->e_lfanew +
            sizeof(IMAGE_NT_HEADERS) +
            (16 * sizeof(IMAGE_SECTION_HEADER))) <= (ULONG)DosHeader->e_lfanew) {
        Status = STATUS_INVALID_IMAGE_PROTECT;
        goto NeImage;
    }

    if (((ULONG)DosHeader->e_lfanew +
            sizeof(IMAGE_NT_HEADERS) +
            (16 * sizeof(IMAGE_SECTION_HEADER))) > PAGE_SIZE) {

        //
        // The PE header is not within the page already read or the
        // objects are in another page.
        // Build another MDL and read an additional 8k.
        //

        ExtendedHeader = ExAllocatePoolWithTag (NonPagedPool,
                                         MM_MAXIMUM_IMAGE_HEADER,
                                         MMTEMPORARY);
        if (ExtendedHeader == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto NeImage;
        }

        //
        // Build an MDL for the operation.
        //

        MmCreateMdl( Mdl, ExtendedHeader, MM_MAXIMUM_IMAGE_HEADER);

        MmBuildMdlForNonPagedPool (Mdl);

        StartingOffset.LowPart = PtrToUlong(PAGE_ALIGN ((ULONG)DosHeader->e_lfanew));

        KeClearEvent (InPageEvent);
        Status = IoPageRead (File,
                             Mdl,
                             &StartingOffset,
                             InPageEvent,
                             &IoStatus
                            );

        if (Status == STATUS_PENDING) {
            KeWaitForSingleObject( InPageEvent,
                                   WrPageIn,
                                   KernelMode,
                                   FALSE,
                                   (PLARGE_INTEGER)NULL);
        }

        if (Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) {
            MmUnmapLockedPages (Mdl->MappedSystemVa, Mdl);
        }

        if ((!NT_SUCCESS(Status)) || (!NT_SUCCESS(IoStatus.Status))) {
            if (Status != STATUS_FILE_LOCK_CONFLICT) {
                Status = STATUS_INVALID_FILE_FOR_SECTION;
            }
            goto NeImage;
        }
        NtHeader = (PIMAGE_NT_HEADERS)((PCHAR)ExtendedHeader +
                          BYTE_OFFSET((ULONG)DosHeader->e_lfanew));
        NtHeaderSize = MM_MAXIMUM_IMAGE_HEADER -
                            (ULONG)(BYTE_OFFSET((ULONG)DosHeader->e_lfanew));

    } else {
        NtHeader = (PIMAGE_NT_HEADERS)((PCHAR)DosHeader +
                                          (ULONG)DosHeader->e_lfanew);
        NtHeaderSize = PAGE_SIZE - (ULONG)DosHeader->e_lfanew;
    }
    FileHeader = &NtHeader->FileHeader;

    //
    // Check to see if this is an NT image or a DOS or OS/2 image.
    //

    Status = MiVerifyImageHeader (NtHeader, DosHeader, NtHeaderSize);
    if (Status != STATUS_SUCCESS) {
        goto NeImage;
    }

#if defined(_WIN64)
    if (NtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        //
        // The image is 32-bit.  All code below this point must check
        // if NtHeader is NULL.  If it is, then the image is PE32 and
        // NtHeader32 must be used.
        //
        NtHeader32 = (PIMAGE_NT_HEADERS32)NtHeader;
        NtHeader = NULL;
    } else {
        NtHeader32 = NULL;
    }

    if (NtHeader) {
#endif
        ImageAlignment = NtHeader->OptionalHeader.SectionAlignment;
        FileAlignment = NtHeader->OptionalHeader.FileAlignment - 1;
        SizeOfImage = NtHeader->OptionalHeader.SizeOfImage;
        LoaderFlags = NtHeader->OptionalHeader.LoaderFlags;
#if defined (_WIN64)
    } else {
        ImageAlignment = NtHeader32->OptionalHeader.SectionAlignment;
        FileAlignment = NtHeader32->OptionalHeader.FileAlignment - 1;
        SizeOfImage = NtHeader32->OptionalHeader.SizeOfImage;
        LoaderFlags = NtHeader32->OptionalHeader.LoaderFlags;
    }
#endif

    RoundingAlignment = ImageAlignment;
    NumberOfSubsections = FileHeader->NumberOfSections;

    if (ImageAlignment < PAGE_SIZE) {

        //
        // The image alignment is less than the page size,
        // map the image with a single subsection.
        //

        ControlArea = ExAllocatePoolWithTag (NonPagedPool,
                      (ULONG)(sizeof(CONTROL_AREA) + (sizeof(SUBSECTION))),
                      MMCONTROL);
        SubsectionsAllocated = 1;
    } else {

        //
        // Allocate a control area and a subsection for each section
        // header plus one for the image header which has no section.
        //

        ControlArea = ExAllocatePoolWithTag(NonPagedPool,
                                            (ULONG)(sizeof(CONTROL_AREA) +
                                                    (sizeof(SUBSECTION) *
                                                (NumberOfSubsections + 1))),
                                            'iCmM');
        SubsectionsAllocated = NumberOfSubsections + 1;
    }

    if (ControlArea == NULL) {

        //
        // The requested pool could not be allocated.
        //

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto NeImage;
    }

    //
    // Zero the control area and the FIRST subsection.
    //

    RtlZeroMemory (ControlArea,
                   sizeof(CONTROL_AREA) + sizeof(SUBSECTION));

    ControlArea->NonPagedPoolUsage = EX_REAL_POOL_USAGE(sizeof(CONTROL_AREA) +
                                                        sizeof(SUBSECTION) *
                                                        SubsectionsAllocated);

    ASSERT (ControlArea->u.Flags.GlobalOnlyPerSession == 0);

    Subsection = (PSUBSECTION)(ControlArea + 1);

    NumberOfPtes = BYTES_TO_PAGES (SizeOfImage);

    if (NumberOfPtes == 0) {
        ExFreePool (ControlArea);
        Status = STATUS_INVALID_IMAGE_FORMAT;
        goto NeImage;
    }

#if defined (_ALPHA_) || defined(_IA64_)
   if (ImageAlignment < PAGE_SIZE && KeGetPreviousMode() != KernelMode &&
       (FileHeader->Machine < USER_SHARED_DATA->ImageNumberLow ||
        FileHeader->Machine > USER_SHARED_DATA->ImageNumberHigh)) {

        InvalidAlignmentAllowed = TRUE;  
    }
    OrigNumberOfPtes = NumberOfPtes;
#endif

    SizeOfSegment = sizeof(SEGMENT) + (sizeof(MMPTE) * (NumberOfPtes - 1)) +
                    sizeof(SECTION_IMAGE_INFORMATION);

    NewSegment = ExAllocatePoolWithTag (PagedPool,
                                        SizeOfSegment,
                                        MMSECT);

    if (NewSegment == NULL) {

        //
        // The requested pool could not be allocated.
        //

        ExFreePool (ControlArea);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto NeImage;
    }
    *Segment = NewSegment;
    RtlZeroMemory (NewSegment, sizeof(SEGMENT));

    //
    // Align the prototype PTEs on the proper boundary.
    //

    PointerPte = &NewSegment->ThePtes[0];
    i = (ULONG) (((ULONG_PTR)PointerPte >> PTE_SHIFT) &
                    ((MM_PROTO_PTE_ALIGNMENT / PAGE_SIZE) - 1));

    if (i != 0) {
        i = (MM_PROTO_PTE_ALIGNMENT / PAGE_SIZE) - i;
    }

    NewSegment->PrototypePte = &NewSegment->ThePtes[i];

    NewSegment->ControlArea = ControlArea;
    NewSegment->ImageInformation =
        (PSECTION_IMAGE_INFORMATION)((PCHAR)NewSegment + sizeof(SEGMENT) +
                                       (sizeof(MMPTE) * (NumberOfPtes - 1)));
    NewSegment->TotalNumberOfPtes = NumberOfPtes;
    NewSegment->NonExtendedPtes = NumberOfPtes;
    NewSegment->SizeOfSegment = (ULONG_PTR)NumberOfPtes * PAGE_SIZE;

    RtlZeroMemory (NewSegment->ImageInformation,
                   sizeof (SECTION_IMAGE_INFORMATION));


//
// This code is built twice on the Win64 build - once for PE32+ and once for
// PE32 images.
//
#define INIT_IMAGE_INFORMATION(OptHdr) {                        \
    NewSegment->ImageInformation->TransferAddress =             \
                    (PVOID)((ULONG_PTR)((OptHdr).ImageBase) +   \
                            (OptHdr).AddressOfEntryPoint);      \
    NewSegment->ImageInformation->MaximumStackSize =            \
                            (OptHdr).SizeOfStackReserve;        \
    NewSegment->ImageInformation->CommittedStackSize =          \
                            (OptHdr).SizeOfStackCommit;         \
    NewSegment->ImageInformation->SubSystemType =               \
                            (OptHdr).Subsystem;                 \
    NewSegment->ImageInformation->SubSystemMajorVersion = (USHORT)((OptHdr).MajorSubsystemVersion); \
    NewSegment->ImageInformation->SubSystemMinorVersion = (USHORT)((OptHdr).MinorSubsystemVersion); \
    NewSegment->ImageInformation->DllCharacteristics =          \
                            (OptHdr).DllCharacteristics;        \
    NewSegment->ImageInformation->ImageContainsCode =           \
                            (BOOLEAN)(((OptHdr).SizeOfCode != 0) || \
                                      ((OptHdr).AddressOfEntryPoint != 0)); \
    }

#if defined (_WIN64)
    if (NtHeader) {
#endif
        INIT_IMAGE_INFORMATION(NtHeader->OptionalHeader);
#if defined (_WIN64)
    } else {
        // The image is 32-bit so use the 32-bit header
        INIT_IMAGE_INFORMATION(NtHeader32->OptionalHeader);
    }
#endif
    #undef INIT_IMAGE_INFORMATION

    NewSegment->ImageInformation->ImageCharacteristics =
                            FileHeader->Characteristics;
    NewSegment->ImageInformation->Machine =
                            FileHeader->Machine;

    ControlArea->Segment = NewSegment;
    ControlArea->NumberOfSectionReferences = 1;
    ControlArea->NumberOfUserReferences = 1;
    ControlArea->u.Flags.BeingCreated = 1;
    ControlArea->PagedPoolUsage = EX_REAL_POOL_USAGE((sizeof(SEGMENT) + (NumberOfPtes * sizeof(MMPTE))));

    if (ImageAlignment < PAGE_SIZE) {

        //
        // Image alignment is less than a page, the number
        // of subsections is 1.
        //

        ControlArea->NumberOfSubsections = 1;
    } else {
        ControlArea->NumberOfSubsections = (USHORT)NumberOfSubsections;
    }

    ControlArea->u.Flags.Image = 1;
    ControlArea->u.Flags.File = 1;

    if ((FILE_FLOPPY_DISKETTE & File->DeviceObject->Characteristics) ||
        ((FileHeader->Characteristics &
                IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP) &&
         (FILE_REMOVABLE_MEDIA & File->DeviceObject->Characteristics)) ||
        ((FileHeader->Characteristics &
                IMAGE_FILE_NET_RUN_FROM_SWAP) &&
         (FILE_REMOTE_DEVICE & File->DeviceObject->Characteristics))) {

        //
        // This file resides on a floppy disk or a removable media or
        // network with flags set indicating it should be copied
        // to the paging file.
        //

        ControlArea->u.Flags.FloppyMedia = 1;
    }

    if (FILE_REMOTE_DEVICE & File->DeviceObject->Characteristics) {

        //
        // This file resides on a redirected drive.
        //

        ControlArea->u.Flags.Networked = 1;
    }

    ControlArea->FilePointer = File;

    //
    // Build the subsection and prototype PTEs for the image header.
    //

    Subsection->ControlArea = ControlArea;
#if defined(_WIN64)
    if (NtHeader) {
#endif
        NextVa = NtHeader->OptionalHeader.ImageBase;
#if defined(_WIN64)
    } else {
        NextVa = NtHeader32->OptionalHeader.ImageBase;
    }
#endif


    if ((NextVa & (X64K - 1)) != 0) {

        //
        // Image header is not aligned on a 64k boundary.
        //

        goto BadPeImageSegment;
    }

    NewSegment->BasedAddress = (PVOID)NextVa;
#if defined(_WIN64)
    if (NtHeader) {
#endif
        SizeOfHeaders = NtHeader->OptionalHeader.SizeOfHeaders;
#if defined(_WIN64)
    } else {
        SizeOfHeaders = NtHeader32->OptionalHeader.SizeOfHeaders;
    }
#endif

    if (SizeOfHeaders >= SizeOfImage) {
        goto BadPeImageSegment;
    }

    Subsection->PtesInSubsection = MI_ROUND_TO_SIZE (
                                       SizeOfHeaders,
                                       ImageAlignment
                                   ) >> PAGE_SHIFT;

    PointerPte = NewSegment->PrototypePte;
    Subsection->SubsectionBase = PointerPte;

    TempPte.u.Long = MiGetSubsectionAddressForPte (Subsection);
    TempPte.u.Soft.Prototype = 1;

    NewSegment->SegmentPteTemplate = TempPte;
    SectorOffset = 0;

    if (ImageAlignment < PAGE_SIZE) {

        //
        // Aligned on less than a page size boundary.
        // Build a single subsection to refer to the image.
        //

        PointerPte = NewSegment->PrototypePte;

        Subsection->PtesInSubsection = NumberOfPtes;

        Subsection->NumberOfFullSectors =
                            (ULONG)(EndOfFile.QuadPart >> MMSECTOR_SHIFT);

        ASSERT ((ULONG)(EndOfFile.HighPart & 0xFFFFF000) == 0);

        Subsection->u.SubsectionFlags.SectorEndOffset =
                              EndOfFile.LowPart & MMSECTOR_MASK;

        Subsection->u.SubsectionFlags.Protection = MM_EXECUTE_WRITECOPY;

        //
        // Set all the PTEs to the execute-read-write protection.
        // The section will control access to these and the segment
        // must provide a method to allow other users to map the file
        // for various protections.
        //

        TempPte.u.Soft.Protection = MM_EXECUTE_WRITECOPY;

        NewSegment->SegmentPteTemplate = TempPte;


#if defined (_ALPHA_) || defined(_IA64_)
        //
        // Invalid image alignments are supported for cross platform
        // emulation. Only alpha and IA64 require extra handling because page
        // size is larger than x86.
        //


        if (InvalidAlignmentAllowed) {

            TempPteDemandZero.u.Long = 0;
            TempPteDemandZero.u.Soft.Protection = MM_EXECUTE_WRITECOPY;
            SectorOffset = 0;

            for (i = 0; i < NumberOfPtes; i += 1) {

                //
                // Set prototype PTEs.
                //

                if (SectorOffset < EndOfFile.LowPart) {

                    //
                    // Data resides on the disk, refer to the control section.
                    //

                    MI_WRITE_INVALID_PTE (PointerPte, TempPte);

                } else {

                    //
                    // Data does not reside on the disk, use Demand zero pages.
                    //

                    MI_WRITE_INVALID_PTE (PointerPte, TempPteDemandZero);
                }

                SectorOffset += PAGE_SIZE;
                PointerPte += 1;
            }

        } else
#endif
           {

            for (i = 0; i < NumberOfPtes; i += 1) {

                //
                // Set all the prototype PTEs to refer to the control section.
                //

                MI_WRITE_INVALID_PTE (PointerPte, TempPte);
                PointerPte += 1;
            }
        }

        NewSegment->ImageCommitment = NumberOfPtes;


        //
        // Indicate alignment is less than a page.
        //

        TempPte.u.Long = 0;

    } else {

        //
        // Alignment is PAGE_SIZE or greater.
        //

        if (Subsection->PtesInSubsection > NumberOfPtes) {

            //
            // Inconsistent image, size does not agree with header.
            //

            goto BadPeImageSegment;
        }
        NumberOfPtes -= Subsection->PtesInSubsection;

        Subsection->NumberOfFullSectors =
            SizeOfHeaders >> MMSECTOR_SHIFT;

        Subsection->u.SubsectionFlags.SectorEndOffset =
            SizeOfHeaders & MMSECTOR_MASK;

        Subsection->u.SubsectionFlags.ReadOnly = 1;
        Subsection->u.SubsectionFlags.CopyOnWrite = 1;
        Subsection->u.SubsectionFlags.Protection = MM_READONLY;

        TempPte.u.Soft.Protection = MM_READONLY;
        NewSegment->SegmentPteTemplate = TempPte;

        for (i = 0; i < Subsection->PtesInSubsection; i += 1) {

            //
            // Set all the prototype PTEs to refer to the control section.
            //

            if (SectorOffset < SizeOfHeaders) {
                MI_WRITE_INVALID_PTE (PointerPte, TempPte);
            } else {
                MI_WRITE_INVALID_PTE (PointerPte, ZeroPte);
            }
            SectorOffset += PAGE_SIZE;
            PointerPte += 1;
            NextVa += PAGE_SIZE;
        }
    }

    //
    // Build the next subsections.
    //

#if defined(_WIN64)
    if (NtHeader) {
#endif
        PreferredImageBase = NtHeader->OptionalHeader.ImageBase;
#if defined(_WIN64)
    } else {
        PreferredImageBase = NtHeader32->OptionalHeader.ImageBase;
    }
#endif

    //
    // At this point the object table is read in (if it was not
    // already read in) and may displace the image header.
    //

    SectionTableEntry = NULL;
    OffsetToSectionTable = sizeof(ULONG) +
                              sizeof(IMAGE_FILE_HEADER) +
                              FileHeader->SizeOfOptionalHeader;

    if ((BYTE_OFFSET(NtHeader) + OffsetToSectionTable +
#if defined (_WIN64)
                BYTE_OFFSET(NtHeader32) +
#endif
                ((NumberOfSubsections + 1) *
                sizeof (IMAGE_SECTION_HEADER))) <= PAGE_SIZE) {

        //
        // Section tables are within the header which was read.
        //

#if defined(_WIN64)
        if (NtHeader32) {
            SectionTableEntry = (PIMAGE_SECTION_HEADER)((PCHAR)NtHeader32 +
                                    OffsetToSectionTable);
        } else
#endif
        {
            SectionTableEntry = (PIMAGE_SECTION_HEADER)((PCHAR)NtHeader +
                                    OffsetToSectionTable);
        }

    } else {

        //
        // Has an extended header been read in and are the object
        // tables resident?
        //

        if (ExtendedHeader != NULL) {

#if defined(_WIN64)
            if (NtHeader32) {
                SectionTableEntry = (PIMAGE_SECTION_HEADER)((PCHAR)NtHeader32 +
                                        OffsetToSectionTable);
            } else
#endif
            {
                SectionTableEntry = (PIMAGE_SECTION_HEADER)((PCHAR)NtHeader +
                                        OffsetToSectionTable);
            }

            //
            // Is the whole range of object tables mapped by the
            // extended header?
            //

            if ((((PCHAR)SectionTableEntry +
                 ((NumberOfSubsections + 1) *
                    sizeof (IMAGE_SECTION_HEADER))) -
                         (PCHAR)ExtendedHeader) >
                                            MM_MAXIMUM_IMAGE_HEADER) {
                SectionTableEntry = NULL;

            }
        }
    }

    if (SectionTableEntry == NULL) {

        //
        // The section table entries are not in the same
        // pages as the other data already read in.  Read in
        // the object table entries.
        //

        if (ExtendedHeader == NULL) {
            ExtendedHeader = ExAllocatePoolWithTag (NonPagedPool,
                                                    MM_MAXIMUM_IMAGE_HEADER,
                                                    MMTEMPORARY);
            if (ExtendedHeader == NULL) {
                ExFreePool (NewSegment);
                ExFreePool (ControlArea);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto NeImage;
            }

            //
            // Build an MDL for the operation.
            //

            MmCreateMdl( Mdl, ExtendedHeader, MM_MAXIMUM_IMAGE_HEADER);

            MmBuildMdlForNonPagedPool (Mdl);
        }

        StartingOffset.LowPart = PtrToUlong(PAGE_ALIGN (
                                    (ULONG)DosHeader->e_lfanew +
                                    OffsetToSectionTable));

        SectionTableEntry = (PIMAGE_SECTION_HEADER)((PCHAR)ExtendedHeader +
                                BYTE_OFFSET((ULONG)DosHeader->e_lfanew +
                                OffsetToSectionTable));

        KeClearEvent (InPageEvent);
        Status = IoPageRead (File,
                             Mdl,
                             &StartingOffset,
                             InPageEvent,
                             &IoStatus
                            );

        if (Status == STATUS_PENDING) {
            KeWaitForSingleObject( InPageEvent,
                                   WrPageIn,
                                   KernelMode,
                                   FALSE,
                                   (PLARGE_INTEGER)NULL);
        }

        if (Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) {
            MmUnmapLockedPages (Mdl->MappedSystemVa, Mdl);
        }

        if ((!NT_SUCCESS(Status)) || (!NT_SUCCESS(IoStatus.Status))) {
            if (Status != STATUS_FILE_LOCK_CONFLICT) {
                Status = STATUS_INVALID_FILE_FOR_SECTION;
            }
            ExFreePool (NewSegment);
            ExFreePool (ControlArea);
            goto NeImage;
        }

        //
        // From this point on NtHeader is only valid if it
        // was in the first 4k of the image, otherwise reading in
        // the object tables wiped it out.
        //

    }

    if (TempPte.u.Long == 0
#if defined (_ALPHA_) || defined(_IA64_)
        && (!InvalidAlignmentAllowed)
#endif
       ) {

        // The image header is no longer valid, TempPte is
        // used to indicate that this image alignment is
        // less than a PAGE_SIZE.

        //
        // Loop through all sections and make sure there is no
        // uninitialized data.
        //

        Status = STATUS_SUCCESS;

        while (NumberOfSubsections > 0) {
            if (SectionTableEntry->Misc.VirtualSize == 0) {
                SectionVirtualSize = SectionTableEntry->SizeOfRawData;
            } else {
                SectionVirtualSize = SectionTableEntry->Misc.VirtualSize;
            }

            //
            // If the raw pointer + raw size overflows a long word, return an error.
            //

            if (SectionTableEntry->PointerToRawData +
                        SectionTableEntry->SizeOfRawData <
                SectionTableEntry->PointerToRawData) {

                KdPrint(("MMCREASECT: invalid section/file size %Z\n",
                    &File->FileName));

                Status = STATUS_INVALID_IMAGE_FORMAT;
                break;
            }

            //
            // If the virtual size and address does not match the rawdata
            // and invalid alignments not allowed return an error.
            //

            if (((SectionTableEntry->PointerToRawData !=
                  SectionTableEntry->VirtualAddress))
                            ||
                   (SectionVirtualSize > SectionTableEntry->SizeOfRawData)) {

                KdPrint(("MMCREASECT: invalid BSS/Trailingzero %Z\n",
                        &File->FileName));

                Status = STATUS_INVALID_IMAGE_FORMAT;
                break;
            }


            SectionTableEntry += 1;
            NumberOfSubsections -= 1;
        }


        if (!NT_SUCCESS(Status)) {
            ExFreePool (NewSegment);
            ExFreePool (ControlArea);
            goto NeImage;
        }

        goto PeReturnSuccess;

    }
#if defined (_ALPHA_) || defined(_IA64_)

    else if (TempPte.u.Long == 0 && InvalidAlignmentAllowed) {

        TempNumberOfSubsections = NumberOfSubsections;
        TempSectionTableEntry = SectionTableEntry;
        
        //
        // The image header is no longer valid, TempPte is
        // used to indicate that this image alignment is
        // less than a PAGE_SIZE.
        //

        //
        // Loop through all sections and make sure there is no
        // uninitialized data.
        //
        // Also determine if there are shared data sections and
        // the number of extra ptes they require.
        //

        AdditionalSubsections = 0;
        AdditionalPtes = 0;
        AdditionalBasePtes = 0;
        RoundingAlignment = PAGE_SIZE;

        while (TempNumberOfSubsections > 0) {
            ULONG EndOfSection;
            ULONG ExtraPages;

            if (TempSectionTableEntry->Misc.VirtualSize == 0) {
                SectionVirtualSize = TempSectionTableEntry->SizeOfRawData;
            } else {
                SectionVirtualSize = TempSectionTableEntry->Misc.VirtualSize;
            }

            EndOfSection = TempSectionTableEntry->PointerToRawData +
                           TempSectionTableEntry->SizeOfRawData;
            
            //
            // If the raw pointer + raw size overflows a long word, return an error.
            //

            if (EndOfSection < TempSectionTableEntry->PointerToRawData) {

                 KdPrint(("MMCREASECT: invalid section/file size %Z\n",
                      &File->FileName));

                 Status = STATUS_INVALID_IMAGE_FORMAT;

                 ExFreePool (NewSegment);
                 ExFreePool (ControlArea);
                 goto NeImage;
            }

            //
            // If the section goes past SizeOfImage then allocate additional PTEs.
            // On x86 this is handled by the subsection mapping. On alpha this is handled
            // by the Wx86 loader extension after the image has been mapped in. But the
            // additional data must be in memory so it can be shuffled around later.
            //

            if ((EndOfSection <= EndOfFile.LowPart) &&
                (EndOfSection > SizeOfImage)) {

                //
                // Allocate enough PTEs to cover the end of this section.
                // Maximize with any other sections that extend beyond SizeOfImage
                //

                ExtraPages = MI_ROUND_TO_SIZE (EndOfSection - SizeOfImage, RoundingAlignment) >> PAGE_SHIFT;
                if (ExtraPages > AdditionalBasePtes) {
                    AdditionalBasePtes = ExtraPages;
                }
            }

            // Count number of shared data sections and additional ptes needed

            if ((TempSectionTableEntry->Characteristics & IMAGE_SCN_MEM_SHARED) &&
                (!(TempSectionTableEntry->Characteristics & IMAGE_SCN_MEM_EXECUTE) ||
                 (TempSectionTableEntry->Characteristics & IMAGE_SCN_MEM_WRITE))) {
                AdditionalPtes += 
                    MI_ROUND_TO_SIZE (SectionVirtualSize, RoundingAlignment) >>
                                                                PAGE_SHIFT;
                AdditionalSubsections += 1;
            }

            TempSectionTableEntry += 1;
            TempNumberOfSubsections -= 1;
        }

        if (AdditionalBasePtes == 0 && (AdditionalSubsections == 0 || AdditionalPtes == 0)) {
            // no shared data sections
            goto PeReturnSuccess;
        }

        //
        // There are additional Base PTEs or shared data sections.
        // For shared sections, allocate new PTEs for these sections
        // at the end of the image. The WX86 loader will change
        // fixups to point to the new pages.
        //
        // First reallocate the control area.
        //

        NewSubsectionsAllocated = SubsectionsAllocated +
                                                AdditionalSubsections;

        NewControlArea = ExAllocatePoolWithTag(NonPagedPool,
                                    (ULONG) (sizeof(CONTROL_AREA) +
                                            (sizeof(SUBSECTION) *
                                                NewSubsectionsAllocated)),
                                                'iCmM');
        if (NewControlArea == NULL) {
            ExFreePool (NewSegment);
            ExFreePool (ControlArea);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto NeImage;
        }

        //
        // Copy the old control area to the new one, modify some fields.
        //

        RtlMoveMemory (NewControlArea, ControlArea,
                        sizeof(CONTROL_AREA) +
                            sizeof(SUBSECTION) * SubsectionsAllocated);

        NewControlArea->NonPagedPoolUsage = 
                        EX_REAL_POOL_USAGE(sizeof(CONTROL_AREA) +
                                            (sizeof(SUBSECTION) *
                                                NewSubsectionsAllocated));

        NewControlArea->NumberOfSubsections = (USHORT) NewSubsectionsAllocated;

        //
        // Now allocate a new segment that has the newly calculated number
        // of PTEs, initialize it from the previously allocated new segment,
        // and overwrite the fields that should be changed.
        //

        OldSegment = NewSegment;

        
        OrigNumberOfPtes += AdditionalBasePtes;

        SizeOfSegment = sizeof(SEGMENT) + 
                     (sizeof(MMPTE) * (OrigNumberOfPtes + AdditionalPtes - 1)) +
                        sizeof(SECTION_IMAGE_INFORMATION);

        NewSegment = ExAllocatePoolWithTag (PagedPool,
                                            SizeOfSegment,
                                            MMSECT);

        if (NewSegment == NULL) {

            //
            // The requested pool could not be allocated.
            //

            ExFreePool (ControlArea);
            ExFreePool (NewControlArea);
            ExFreePool (OldSegment);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto NeImage;
        }

        *Segment = NewSegment;
        RtlMoveMemory (NewSegment, OldSegment, sizeof(SEGMENT));

        //
        // Align the prototype PTEs on the proper boundary.
        //

        NewPointerPte = &NewSegment->ThePtes[0];
        i = (ULONG) (((ULONG_PTR)NewPointerPte >> PTE_SHIFT) &
                        ((MM_PROTO_PTE_ALIGNMENT / PAGE_SIZE) - 1));

        if (i != 0) {
            i = (MM_PROTO_PTE_ALIGNMENT / PAGE_SIZE) - i;
        }

        NewSegment->PrototypePte = &NewSegment->ThePtes[i];
        if (i != 0) {
            RtlZeroMemory (&NewSegment->ThePtes[0], sizeof(MMPTE) * i);
        }
        PointerPte = NewSegment->PrototypePte + 
                                       (PointerPte - OldSegment->PrototypePte);

        NewSegment->ControlArea = NewControlArea;
        NewSegment->ImageInformation =
            (PSECTION_IMAGE_INFORMATION)((PCHAR)NewSegment + sizeof(SEGMENT) +
                     (sizeof(MMPTE) * (OrigNumberOfPtes + AdditionalPtes - 1)));
        NewSegment->TotalNumberOfPtes = OrigNumberOfPtes + AdditionalPtes;
        NewSegment->NonExtendedPtes = OrigNumberOfPtes + AdditionalPtes;
        NewSegment->SizeOfSegment = (ULONG_PTR)(OrigNumberOfPtes + AdditionalPtes) * PAGE_SIZE;

        RtlMoveMemory (NewSegment->ImageInformation,
                       OldSegment->ImageInformation,
                       sizeof (SECTION_IMAGE_INFORMATION));

        //
        // Now change the fields in the subsections to account for the new
        // control area and the new segment. Also change the PTEs in the 
        // newly allocated segment to point to the new subsections.
        //

        NewControlArea->Segment = NewSegment;
        NewControlArea->PagedPoolUsage = EX_REAL_POOL_USAGE((sizeof(SEGMENT) +
                        ((OrigNumberOfPtes + AdditionalPtes) * sizeof(MMPTE))));

        Subsection = (PSUBSECTION)(ControlArea + 1);
        NewSubsection = (PSUBSECTION)(NewControlArea + 1);
        NewSubsection->PtesInSubsection += AdditionalBasePtes;
        
        for (i = 0; i < SubsectionsAllocated; i += 1) {

            //
            // Note: SubsectionsAllocated is always 1 (for wx86), so this loop 
            // is executed only once.
            //

            NewSubsection->ControlArea = (PCONTROL_AREA) NewControlArea;
            
            NewSubsection->SubsectionBase = NewSegment->PrototypePte + 
                    (Subsection->SubsectionBase - OldSegment->PrototypePte);

            NewPointerPte = NewSegment->PrototypePte;
            OldPointerPte = OldSegment->PrototypePte;

            TempPte.u.Long = MiGetSubsectionAddressForPte (NewSubsection);
            TempPte.u.Soft.Prototype = 1;

            for (j = 0; j < OldSegment->TotalNumberOfPtes+AdditionalBasePtes; j += 1) {
                
                if ((OldPointerPte->u.Soft.Prototype == 1) &&
                    (MiGetSubsectionAddress (OldPointerPte) == Subsection)) {
                    OriginalProtection = MI_GET_PROTECTION_FROM_SOFT_PTE (OldPointerPte);
                    TempPte.u.Soft.Protection = OriginalProtection;
                    MI_WRITE_INVALID_PTE (NewPointerPte, TempPte);
                }
                else if (i == 0) {

                    //
                    // Since the outer for loop is executed only once, there 
                    // is no need for the i == 0 above, but it is safer to
                    // have it. If the code changes later and other sections
                    // are added, their PTEs will get initialized here as
                    // DemandZero and if they are not DemandZero, they will be
                    // overwritten in a later iteration of the outer loop. 
                    // For now, this else if clause will be executed only 
                    // for DemandZero PTEs.
                    //

                    OriginalProtection = MI_GET_PROTECTION_FROM_SOFT_PTE (OldPointerPte);
                    TempPteDemandZero.u.Long = 0;
                    TempPteDemandZero.u.Soft.Protection = OriginalProtection;
                    MI_WRITE_INVALID_PTE (NewPointerPte, TempPteDemandZero);
                }

                NewPointerPte += 1;
                // Stop incrementing the OldPointerPte at the last entry and use it
                // for the additional Base PTEs
                if (j < OldSegment->TotalNumberOfPtes-1) {
                    OldPointerPte += 1;
                }
            }

            Subsection += 1;
            NewSubsection += 1;
        }


        RtlZeroMemory (NewSubsection,
                            sizeof(SUBSECTION) * AdditionalSubsections);

        ExFreePool (OldSegment);
        ExFreePool (ControlArea);
        ControlArea = (PCONTROL_AREA) NewControlArea;

        //
        // Adjust some variables that are used below.
        // PointerPte has already been set above before OldSegment was freed.
        //

        SubsectionsAllocated = NewSubsectionsAllocated;
        Subsection = NewSubsection - 1; // Points to last used subsection.
        NumberOfPtes = AdditionalPtes;  // # PTEs that haven't yet been used in 
                                        // previous subsections.
        
        // Additional Base PTEs have been added. Only continue if there are
        // additional subsections to process
        if (AdditionalSubsections == 0 || AdditionalPtes == 0) {
            // no shared data sections
            goto PeReturnSuccess;
        }
    }
#endif

    while (NumberOfSubsections > 0) {

#if defined (_ALPHA_) || defined(_IA64_)
        if (!InvalidAlignmentAllowed || 
            ((SectionTableEntry->Characteristics & IMAGE_SCN_MEM_SHARED) &&
             (!(SectionTableEntry->Characteristics & IMAGE_SCN_MEM_EXECUTE) ||
              (SectionTableEntry->Characteristics & IMAGE_SCN_MEM_WRITE)))) {
#endif

        //
        // Handle case where virtual size is 0.
        //

        if (SectionTableEntry->Misc.VirtualSize == 0) {
            SectionVirtualSize = SectionTableEntry->SizeOfRawData;
        } else {
            SectionVirtualSize = SectionTableEntry->Misc.VirtualSize;
        }

        //
        // Fix for Borland linker problem.  The SizeOfRawData can
        // be a zero, but the PointerToRawData is not zero.
        // Set it to zero.
        //

        if (SectionTableEntry->SizeOfRawData == 0) {
            SectionTableEntry->PointerToRawData = 0;
        }

        //
        // If the section information wraps return an error.
        //

        if (SectionTableEntry->PointerToRawData +
                    SectionTableEntry->SizeOfRawData <
            SectionTableEntry->PointerToRawData) {

            goto BadPeImageSegment;
        }


        Subsection += 1;
        Subsection->ControlArea = ControlArea;
        Subsection->NextSubsection = (PSUBSECTION)NULL;
        Subsection->UnusedPtes = 0;
        
        if (((NextVa !=
                (PreferredImageBase + SectionTableEntry->VirtualAddress)) 

#if defined (_ALPHA_) || defined(_IA64_)
             && !InvalidAlignmentAllowed
#endif

            )  ||
            (SectionVirtualSize == 0)) {

            //
            // The specified virtual address does not align
            // with the next prototype PTE.
            //

            goto BadPeImageSegment;
        }

        Subsection->PtesInSubsection =
            MI_ROUND_TO_SIZE (SectionVirtualSize, RoundingAlignment) 
                                                                >> PAGE_SHIFT;

        if (Subsection->PtesInSubsection > NumberOfPtes) {

            //
            // Inconsistent image, size does not agree with object tables.
            //
            
            goto BadPeImageSegment;
        }
        NumberOfPtes -= Subsection->PtesInSubsection;

        Subsection->u.LongFlags = 0;
        Subsection->StartingSector =
                        SectionTableEntry->PointerToRawData >> MMSECTOR_SHIFT;

        //
        // Align ending sector on file align boundary.
        //

        EndingAddress = (SectionTableEntry->PointerToRawData +
                                     SectionTableEntry->SizeOfRawData +
                                     FileAlignment) & ~FileAlignment;
        
        Subsection->NumberOfFullSectors = (ULONG)
                         ((EndingAddress >> MMSECTOR_SHIFT) -
                         Subsection->StartingSector);

        Subsection->u.SubsectionFlags.SectorEndOffset =
                                    (ULONG) EndingAddress & MMSECTOR_MASK;

        Subsection->SubsectionBase = PointerPte;

        //
        // Build both a demand zero PTE and a PTE pointing to the
        // subsection.
        //
        TempPte.u.Long = 0;
        TempPteDemandZero.u.Long = 0;

        TempPte.u.Long = MiGetSubsectionAddressForPte (Subsection);
        TempPte.u.Soft.Prototype = 1;
        ImageFileSize = SectionTableEntry->PointerToRawData +
                                    SectionTableEntry->SizeOfRawData;
        TempPte.u.Soft.Protection =
                 MiGetImageProtection (SectionTableEntry->Characteristics);
        TempPteDemandZero.u.Soft.Protection = TempPte.u.Soft.Protection;

        if (SectionTableEntry->PointerToRawData == 0) {
            TempPte = TempPteDemandZero;
        }

        Subsection->u.SubsectionFlags.ReadOnly = 1;
        Subsection->u.SubsectionFlags.CopyOnWrite = 1;
        Subsection->u.SubsectionFlags.Protection = MI_GET_PROTECTION_FROM_SOFT_PTE (&TempPte);

        if (TempPte.u.Soft.Protection & MM_PROTECTION_WRITE_MASK) {
            if ((TempPte.u.Soft.Protection & MM_COPY_ON_WRITE_MASK)
                                            == MM_COPY_ON_WRITE_MASK) {

                //
                // This page is copy on write, charge ImageCommitment
                // for all pages in this subsection.
                //

                ImageCommit = TRUE;
            } else {

                //
                // This page is write shared, charge commitment when
                // the mapping completes.
                //

                SectionCommit = TRUE;
                Subsection->u.SubsectionFlags.GlobalMemory = 1;
                ControlArea->u.Flags.GlobalMemory = 1;
            }
        } else {

            //
            // Not writable, don't charge commitment at all.
            //

            ImageCommit = FALSE;
            SectionCommit = FALSE;
        }

        NewSegment->SegmentPteTemplate = TempPte;
        SectorOffset = 0;

        for (i = 0; i < Subsection->PtesInSubsection; i += 1) {

            //
            // Set all the prototype PTEs to refer to the control section.
            //

            if (SectorOffset < SectionVirtualSize) {

                //
                // Make PTE accessible.
                //

                if (SectionCommit) {
                    NewSegment->NumberOfCommittedPages += 1;
                }
                if (ImageCommit) {
                    NewSegment->ImageCommitment += 1;
                }

                if (SectorOffset < SectionTableEntry->SizeOfRawData) {

                    //
                    // Data resides on the disk, use the subsection format PTE.
                    //
                    MI_WRITE_INVALID_PTE (PointerPte, TempPte);
                } else {

                    //
                    // Demand zero pages.
                    //
                    MI_WRITE_INVALID_PTE (PointerPte, TempPteDemandZero);
                }
            } else {

                //
                // No access pages.
                //

                MI_WRITE_INVALID_PTE (PointerPte, ZeroPte);
            }
            SectorOffset += PAGE_SIZE;
            PointerPte += 1;
            NextVa += PAGE_SIZE;
        }

#if defined (_ALPHA_) || defined(_IA64_)
        }
#endif

        SectionTableEntry += 1;
        NumberOfSubsections -= 1;
    }

#if defined (_ALPHA_) || defined(_IA64_)
    if (!InvalidAlignmentAllowed) {
#endif

    //
    // Account for the number of subsections that really are mapped.
    //

    ASSERT (ImageAlignment >= PAGE_SIZE);
    ControlArea->NumberOfSubsections += 1;

    //
    // If the file size is not as big as the image claimed to be,
    // return an error.
    //

    if (ImageFileSize > EndOfFile.LowPart) {

        //
        // Invalid image size.
        //

        KdPrint(("MMCREASECT: invalid image size - file size %lx - image size %lx\n %Z\n",
            EndOfFile.LowPart, ImageFileSize, &File->FileName));
        goto BadPeImageSegment;
    }

    //
    // The total number of PTEs was decremented as sections were built,
    // make sure that there are less than 64k's worth at this point.
    //

    if (NumberOfPtes >= (ImageAlignment >> PAGE_SHIFT)) {

        //
        // Inconsistent image, size does not agree with object tables.
        //

        KdPrint(("MMCREASECT: invalid image - PTE left %lx\n image name %Z\n",
            NumberOfPtes, &File->FileName));

        goto BadPeImageSegment;
    }

    //
    // Set any remaining PTEs to no access.
    //

    while (NumberOfPtes != 0) {
        MI_WRITE_INVALID_PTE (PointerPte, ZeroPte);
        PointerPte += 1;
        NumberOfPtes -= 1;
    }

    //
    // Turn the image header page into a transition page within the
    // prototype PTEs.
    //

    if ((ExtendedHeader == NULL) && (SizeOfHeaders < PAGE_SIZE)) {

        //
        // Zero remaining portion of header.
        //

        RtlZeroMemory ((PVOID)((PCHAR)Base +
                       SizeOfHeaders),
                       PAGE_SIZE - SizeOfHeaders);
    }


#if defined (_ALPHA_) || defined(_IA64_)
    }
#endif

    if (NewSegment->NumberOfCommittedPages != 0) {

        //
        // Commit the pages for the image section.
        //

        if (MiChargeCommitment (NewSegment->NumberOfCommittedPages, NULL) == FALSE) {
            Status = STATUS_COMMITMENT_LIMIT;
            ExFreePool (NewSegment);
            ExFreePool (ControlArea);
            goto NeImage;
        }

        MM_TRACK_COMMIT (MM_DBG_COMMIT_IMAGE, NewSegment->NumberOfCommittedPages);
        Status = STATUS_SUCCESS;

        ExAcquireFastMutex (&MmSectionCommitMutex);
        MmSharedCommit += NewSegment->NumberOfCommittedPages;
        ExReleaseFastMutex (&MmSectionCommitMutex);
    }

PeReturnSuccess:

    //
    // If this image is global per session, then allocate a large control
    // area.  Note this doesn't need to be done for systemwide global control
    // areas or non-global control areas.
    //

    if ((MiHydra == TRUE) &&
        (ControlArea->u.Flags.GlobalMemory) &&
        ((LoaderFlags & IMAGE_LOADER_FLAGS_SYSTEM_GLOBAL) == 0)) {

        LargeControlArea = ExAllocatePoolWithTag(NonPagedPool,
                                            (ULONG)(sizeof(LARGE_CONTROL_AREA) +
                                                    (sizeof(SUBSECTION) *
                                                    SubsectionsAllocated)),
                                                    'iCmM');
        if (LargeControlArea == NULL) {

            //
            // The requested pool could not be allocated.
            //

            if (NewSegment->NumberOfCommittedPages != 0) {
                MiReturnCommitment (NewSegment->NumberOfCommittedPages);
                MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_IMAGE_NO_LARGE_CA, NewSegment->NumberOfCommittedPages);
                ExAcquireFastMutex (&MmSectionCommitMutex);
                MmSharedCommit -= NewSegment->NumberOfCommittedPages;
                ExReleaseFastMutex (&MmSectionCommitMutex);
            }

            ExFreePool (NewSegment);
            ExFreePool (ControlArea);


            Status = STATUS_INSUFFICIENT_RESOURCES;

            goto NeImage;
        }

        //
        // Copy the normal control area into our larger one, fix the linkages,
        // Fill in the additional fields in the new one and free the old one.
        //

        RtlMoveMemory (LargeControlArea, ControlArea, sizeof(CONTROL_AREA));
        LargeControlArea->NonPagedPoolUsage = EX_REAL_POOL_USAGE(
                                                    sizeof(LARGE_CONTROL_AREA) +
                                                    (sizeof(SUBSECTION) *
                                                    SubsectionsAllocated));

        ASSERT (ControlArea->u.Flags.GlobalOnlyPerSession == 0);

        Subsection = (PSUBSECTION)(ControlArea + 1);
        NewSubsection = (PSUBSECTION)(LargeControlArea + 1);

        for (i = 0; i < SubsectionsAllocated; i += 1) {
            RtlMoveMemory (NewSubsection, Subsection, sizeof(SUBSECTION));
            NewSubsection->ControlArea = (PCONTROL_AREA) LargeControlArea;

            PointerPte = NewSegment->PrototypePte;

            TempPte.u.Long = MiGetSubsectionAddressForPte (NewSubsection);
            TempPte.u.Soft.Prototype = 1;

            for (j = 0; j < NewSegment->TotalNumberOfPtes; j += 1) {

                if ((PointerPte->u.Soft.Prototype == 1) &&
                    (MiGetSubsectionAddress (PointerPte) == Subsection)) {
                        OriginalProtection = MI_GET_PROTECTION_FROM_SOFT_PTE (PointerPte);
                        TempPte.u.Soft.Protection = OriginalProtection;
                        MI_WRITE_INVALID_PTE (PointerPte, TempPte);
                }
                PointerPte += 1;
            }

            Subsection += 1;
            NewSubsection += 1;
        }

        NewSegment->ControlArea = (PCONTROL_AREA) LargeControlArea;

        LargeControlArea->u.Flags.GlobalOnlyPerSession = 1;

        LargeControlArea->SessionId = 0;
        InitializeListHead (&LargeControlArea->UserGlobalList);

        ExFreePool (ControlArea);

        ControlArea = (PCONTROL_AREA) LargeControlArea;
    }

    MiUnmapImageHeaderInHyperSpace ();

    //
    // Set the PFN database entry for this page to look like a transition
    // page.
    //

    PointerPte = NewSegment->PrototypePte;

    MiUpdateImageHeaderPage (PointerPte, PageFrameNumber, ControlArea);
    if (ExtendedHeader != NULL) {
        ExFreePool (ExtendedHeader);
    }
    ExFreePool (InPageEvent);

    return STATUS_SUCCESS;


    //
    // Error returns from image verification.
    //

BadPeImageSegment:

    ExFreePool (NewSegment);
    ExFreePool (ControlArea);

//BadPeImage:
    Status = STATUS_INVALID_IMAGE_FORMAT;

NeImage:
    MiUnmapImageHeaderInHyperSpace ();

BadSection:
    MiRemoveImageHeaderPage(PageFrameNumber);
    if (ExtendedHeader != NULL) {
        ExFreePool (ExtendedHeader);
    }
    ExFreePool (InPageEvent);
    return Status;
}


BOOLEAN
MiCheckDosCalls (
    IN PIMAGE_OS2_HEADER Os2Header,
    IN ULONG HeaderSize
    )

/*++

Routine Description:


Arguments:

Return Value:

    Returns the status value.

--*/

{
    PUCHAR ImportTable;
    UCHAR EntrySize;
    USHORT ModuleCount;
    USHORT ModuleSize;
    USHORT i;
    PUSHORT ModuleTable;

    PAGED_CODE();

    //
    // If there are no modules to check return immediately.
    //

    ModuleCount = Os2Header->ne_cmod;

    if (ModuleCount == 0) {
        return FALSE;
    }

    //
    // exe headers are notorious for having junk values for offsets
    // in the import table and module table.  We guard against this
    // via careful checking plus our exception handler.
    //

    try {

        //
        // Find out where the Module ref table is. Mod table has two byte
        // for each entry in import table. These two bytes tell the offset
        // in the import table for that entry.
        //

        ModuleTable = (PUSHORT)((PCHAR)Os2Header + (ULONG)Os2Header->ne_modtab);

        //
        // Make sure that the module table fits within the passed-in header.
        // Note that each module table entry is 2 bytes long.
        //

        if (((ULONG)Os2Header->ne_modtab + (ModuleCount * 2)) > HeaderSize) {
            return FALSE;
        }

        //
        // Now search individual entries for DOSCALLs.
        //

        for (i = 0; i < ModuleCount; i += 1) {

            ModuleSize = *((UNALIGNED USHORT *)ModuleTable);

            //
            // Import table has count byte followed by the string where count
            // is the string length.
            //

            ImportTable = (PUCHAR)((PCHAR)Os2Header +
                          (ULONG)Os2Header->ne_imptab + (ULONG)ModuleSize);

            //
            // Make sure the offset is within the valid range.
            //

            if (((ULONG)Os2Header->ne_imptab + (ULONG)ModuleSize)
                            >= HeaderSize) {
                return FALSE;
            }

            EntrySize = *ImportTable++;

            //
            // 0 is a bad size, bail out.
            //

            if (EntrySize == 0) {
                return FALSE;
            }

            //
            // Make sure the offset is within the valid range.
            // The sizeof(UCHAR) is included in the check because ImportTable
            // was incremented above and is used in the RtlEqualMemory
            // comparison below.
            //

            if (((ULONG)Os2Header->ne_imptab + (ULONG)ModuleSize +
                            (ULONG)EntrySize + sizeof(UCHAR)) > HeaderSize) {
                return FALSE;
            }

            //
            // If size matches compare DOSCALLS.
            //

            if (EntrySize == 8) {
                if (RtlEqualMemory (ImportTable, "DOSCALLS", 8) ) {
                    return TRUE;
                }
            }

            //
            // Move on to the next module table entry.  Each entry is 2 bytes.
            //

            ModuleTable = (PUSHORT)((PCHAR)ModuleTable + 2);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return FALSE;
    }

    return FALSE;
}



NTSTATUS
MiVerifyImageHeader (
    IN PIMAGE_NT_HEADERS NtHeader,
    IN PIMAGE_DOS_HEADER DosHeader,
    IN ULONG NtHeaderSize
    )

/*++

Routine Description:


Arguments:

Return Value:

    Returns the status value.

    TBS

--*/



{
    PCONFIGPHARLAP PharLapConfigured;
    PUCHAR         pb;
    LONG           pResTableAddress;

    PAGED_CODE();

    if (NtHeader->Signature != IMAGE_NT_SIGNATURE) {
        if ((USHORT)NtHeader->Signature == (USHORT)IMAGE_OS2_SIGNATURE) {

            //
            // Check to see if this is a win-16 image.
            //

            if ((!MiCheckDosCalls ((PIMAGE_OS2_HEADER)NtHeader, NtHeaderSize)) &&
                ((((PIMAGE_OS2_HEADER)NtHeader)->ne_exetyp == 2)
                                ||
                ((((PIMAGE_OS2_HEADER)NtHeader)->ne_exetyp == 0)  &&
                  (((((PIMAGE_OS2_HEADER)NtHeader)->ne_expver & 0xff00) ==
                        0x200)  ||
                ((((PIMAGE_OS2_HEADER)NtHeader)->ne_expver & 0xff00) ==
                        0x300))))) {

                //
                // This is a win-16 image.
                //

                return STATUS_INVALID_IMAGE_WIN_16;
            }

            // The following OS/2 headers types go to NTDOS
            //
            // - exetype == 5 means binary is for Dos 4.0.
            //                e.g Borland Dos extender type
            //
            // - OS/2 apps which have no import table entries
            //   cannot be meant for the OS/2 ss.
            //                e.g. QuickC for dos binaries
            //
            //  - "old" Borland Dosx BC++ 3.x, Paradox 4.x
            //     exe type == 1
            //     DosHeader->e_cs * 16 + DosHeader->e_ip + 0x200 - 10
            //     contains the string " mode EXE$"
            //     but import table is empty, so we don't make special check
            //

            if (((PIMAGE_OS2_HEADER)NtHeader)->ne_exetyp == 5  ||
                ((PIMAGE_OS2_HEADER)NtHeader)->ne_enttab ==
                  ((PIMAGE_OS2_HEADER)NtHeader)->ne_imptab  )
               {
                return STATUS_INVALID_IMAGE_PROTECT;
            }


            //
            // Borland Dosx types: exe type 1
            //
            //  - "new" Borland Dosx BP7.0
            //     exe type == 1
            //     DosHeader + 0x200 contains the string "16STUB"
            //     0x200 happens to be e_parhdr*16
            //

            if (((PIMAGE_OS2_HEADER)NtHeader)->ne_exetyp == 1 &&
                RtlEqualMemory((PUCHAR)DosHeader + 0x200, "16STUB", 6) )
               {
                return STATUS_INVALID_IMAGE_PROTECT;
            }

            //
            // Check for PharLap extended header which we run as a dos app.
            // The PharLap config block is pointed to by the SizeofHeader
            // field in the DosHdr.
            // The following algorithm for detecting a pharlap exe
            // was recommended by PharLap Software Inc.
            //

            PharLapConfigured =(PCONFIGPHARLAP) ((PCHAR)DosHeader +
                                      ((ULONG)DosHeader->e_cparhdr << 4));

            if ((PCHAR)PharLapConfigured <
                       (PCHAR)DosHeader + PAGE_SIZE - sizeof(CONFIGPHARLAP)) {
                if (RtlEqualMemory(&PharLapConfigured->uchCopyRight[0x18],
                                   "Phar Lap Software, Inc.", 24) &&
                    (PharLapConfigured->usSign == 0x4b50 ||  // stub loader type 2
                     PharLapConfigured->usSign == 0x4f50 ||  // bindable 286|DosExtender
                     PharLapConfigured->usSign == 0x5650  )) // bindable 286|DosExtender (Adv)
                  {
                    return STATUS_INVALID_IMAGE_PROTECT;
                }
            }



            //
            // Check for Rational extended header which we run as a dos app.
            // We look for the rational copyright at:
            //     wCopyRight = *(DosHeader->e_cparhdr*16 + 30h)
            //     pCopyRight = wCopyRight + DosHeader->e_cparhdr*16
            //     "Copyright (C) Rational Systems, Inc."
            //

            pb = ((PUCHAR)DosHeader + ((ULONG)DosHeader->e_cparhdr << 4));

            if ((ULONG_PTR)pb < (ULONG_PTR)DosHeader + PAGE_SIZE - 0x30 - sizeof(USHORT)) {
                pb += *(PUSHORT)(pb + 0x30);
                if ( (ULONG_PTR)pb < (ULONG_PTR)DosHeader + PAGE_SIZE - 36 &&
                     RtlEqualMemory(pb,
                                    "Copyright (C) Rational Systems, Inc.",
                                    36) )
                   {
                    return STATUS_INVALID_IMAGE_PROTECT;
                }
            }

            //
            // Check for lotus 123 family of applications. Starting
            // with 123 3.0 (till recently shipped 123 3.4), every
            // exe header is bound but is meant for DOS. This can
            // be checked via, a string signature in the extended
            // header. <len byte>"1-2-3 Preloader" is the string
            // at ne_nrestab offset.
            //

            pResTableAddress = ((PIMAGE_OS2_HEADER)NtHeader)->ne_nrestab;
            if (pResTableAddress > DosHeader->e_lfanew &&
                ((ULONG)((pResTableAddress+16) - DosHeader->e_lfanew) <
                            NtHeaderSize) &&
                RtlEqualMemory(
                    ((PUCHAR)NtHeader + 1 +
                             (ULONG)(pResTableAddress - DosHeader->e_lfanew)),
                    "1-2-3 Preloader",
                    15) ) {
                    return STATUS_INVALID_IMAGE_PROTECT;
            }

            return STATUS_INVALID_IMAGE_NE_FORMAT;
        }

        if ((USHORT)NtHeader->Signature == (USHORT)IMAGE_OS2_SIGNATURE_LE) {

            //
            // This is a LE (OS/2) image. We don't support it, so give it to
            // DOS subsystem. There are cases (Rbase.exe) which have a LE
            // header but actually it is suppose to run under DOS. When we
            // do support LE format, some work needs to be done here to
            // decide whether to give it to VDM or OS/2.
            //

            return STATUS_INVALID_IMAGE_PROTECT;
        }
        return STATUS_INVALID_IMAGE_PROTECT;
    }

    if ((NtHeader->FileHeader.Machine == 0) &&
        (NtHeader->FileHeader.SizeOfOptionalHeader == 0)) {

        //
        // This is a bogus DOS app which has a 32-bit portion
        // masquerading as a PE image.
        //

        return STATUS_INVALID_IMAGE_PROTECT;
    }

    if (!(NtHeader->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE)) {
        return STATUS_INVALID_IMAGE_FORMAT;
    }

#ifdef i386

    //
    // Make sure the image header is aligned on a Long word boundary.
    //

    if (((ULONG_PTR)NtHeader & 3) != 0) {
        return STATUS_INVALID_IMAGE_FORMAT;
    }
#endif

#define VALIDATE_NTHEADER(Hdr) {                                    \
    /* File alignment must be multiple of 512 and power of 2. */    \
    if (((((Hdr)->OptionalHeader).FileAlignment & 511) != 0) &&     \
        (((Hdr)->OptionalHeader).FileAlignment !=                   \
         ((Hdr)->OptionalHeader).SectionAlignment)) {               \
        return STATUS_INVALID_IMAGE_FORMAT;                         \
    }                                                               \
                                                                    \
    if (((Hdr)->OptionalHeader).FileAlignment == 0) {               \
        return STATUS_INVALID_IMAGE_FORMAT;                         \
    }                                                               \
                                                                    \
    if (((((Hdr)->OptionalHeader).FileAlignment - 1) &              \
          ((Hdr)->OptionalHeader).FileAlignment) != 0) {            \
        return STATUS_INVALID_IMAGE_FORMAT;                         \
    }                                                               \
                                                                    \
    if (((Hdr)->OptionalHeader).SectionAlignment < ((Hdr)->OptionalHeader).FileAlignment) { \
        return STATUS_INVALID_IMAGE_FORMAT;                         \
    }                                                               \
                                                                    \
    if (((Hdr)->OptionalHeader).SizeOfImage > MM_SIZE_OF_LARGEST_IMAGE) { \
        return STATUS_INVALID_IMAGE_FORMAT;                         \
    }                                                               \
                                                                    \
    if ((Hdr)->FileHeader.NumberOfSections > MM_MAXIMUM_IMAGE_SECTIONS) { \
        return STATUS_INVALID_IMAGE_FORMAT;                         \
    }                                                               \
}

    if (NtHeader->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC) {
        //
        // Image doesn't have the right magic value in its optional header
        //
#if defined (_WIN64)
        if (NtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
            //
            // PE32 image.  Validate it as such.
            //
            PIMAGE_NT_HEADERS32 NtHeader32 = (PIMAGE_NT_HEADERS32)NtHeader;

            VALIDATE_NTHEADER(NtHeader32);
            return STATUS_SUCCESS;
        }
#endif
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    VALIDATE_NTHEADER(NtHeader);
    #undef VALIDATE_NTHEADER

    return STATUS_SUCCESS;
}

NTSTATUS
MiCreateDataFileMap (
    IN PFILE_OBJECT File,
    OUT PSEGMENT *Segment,
    IN PUINT64 MaximumSize,
    IN ULONG SectionPageProtection,
    IN ULONG AllocationAttributes,
    IN ULONG IgnoreFileSizing
    )

/*++

Routine Description:

    This function creates the necessary structures to allow the mapping
    of a data file.

    The data file is accessed to verify desired access, a segment
    object is created and initialized.

Arguments:

    File - Supplies the file object for the image file.

    Segment - Returns the segment object.

    FileHandle - Supplies the handle to the image file.

    MaximumSize - Supplies the maximum size for the mapping.

    SectionPageProtection - Supplies the initial page protection.

    AllocationAttributes - Supplies the allocation attributes for the
                           mapping.

Return Value:

    Returns the status value.

    TBS


--*/

{

    NTSTATUS Status;
    ULONG NumberOfPtes;
    ULONG SizeOfSegment;
    ULONG j;
    ULONG Size;
    ULONG PartialSize;
    ULONG First;
    PCONTROL_AREA ControlArea;
    PSEGMENT NewSegment;
    PSUBSECTION Subsection;
    PSUBSECTION ExtendedSubsection;
    PMMPTE PointerPte;
    MMPTE TempPte;
    ULONG NumberOfPtesWithAlignment;
    UINT64 EndOfFile;
    UINT64 LastFileChunk;
    UINT64 FileOffset;
    UINT64 NumberOfPtesForEntireFile;
    ULONG ExtendedSubsections;
    PSUBSECTION FirstSubsection;
    PSUBSECTION Last;
    ULONG NumberOfNewSubsections;

    PAGED_CODE();

    ExtendedSubsections = 0;
    FirstSubsection = NULL;
    NumberOfNewSubsections = 0;

    // *************************************************************
    // Create mapped file section.
    // *************************************************************

    if (!IgnoreFileSizing) {

        Status = FsRtlGetFileSize (File, (PLARGE_INTEGER)&EndOfFile);

        if (Status == STATUS_FILE_IS_A_DIRECTORY) {

            //
            // Can't map a directory as a section. Return error.
            //

            return STATUS_INVALID_FILE_FOR_SECTION;
        }

        if (!NT_SUCCESS (Status)) {
            return Status;
        }

        if (EndOfFile == 0 && *MaximumSize == 0) {

            //
            // Can't map a zero length without specifying the maximum
            // size as non-zero.
            //

            return STATUS_MAPPED_FILE_SIZE_ZERO;
        }

        //
        // Make sure this file is big enough for the section.
        //

        if (*MaximumSize > EndOfFile) {

            //
            // If the maximum size is greater than the end-of-file,
            // and the user did not request page_write or page_execute_readwrite
            // to the section, reject the request.
            //

            if (((SectionPageProtection & PAGE_READWRITE) |
                (SectionPageProtection & PAGE_EXECUTE_READWRITE)) == 0) {

                return STATUS_SECTION_TOO_BIG;
            }

            //
            // Check to make sure that the allocation size large enough
            // to contain all the data, if not set a new allocation size.
            //

            EndOfFile = *MaximumSize;

            Status = FsRtlSetFileSize (File, (PLARGE_INTEGER)&EndOfFile);

            if (!NT_SUCCESS (Status)) {
                return Status;
            }
        }
    } else {

        //
        // Ignore the file size, this call is from the cache manager.
        //

        EndOfFile = *MaximumSize;
    }

    //
    // Calculate the number of prototype PTEs to build for this section.
    //

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

    NumberOfPtesForEntireFile = (EndOfFile + PAGE_SIZE - 1) >> PAGE_SHIFT;

    NumberOfPtes = (ULONG)NumberOfPtesForEntireFile;

    if (EndOfFile > MI_MAXIMUM_SECTION_SIZE) {
        return STATUS_SECTION_TOO_BIG;
    }

    if (NumberOfPtesForEntireFile > (UINT64)((MAXULONG_PTR / sizeof(MMPTE)) - sizeof (SEGMENT))) {
        return STATUS_SECTION_TOO_BIG;
    }

    if (NumberOfPtesForEntireFile > EndOfFile) {
        return STATUS_SECTION_TOO_BIG;
    }

    //
    // Calculate the number of PTEs to allocate to maintain the
    // desired alignment.  On x86 and R3000 no additional PTEs are
    // needed; on MIPS, the desired alignment is 64k to avoid cache
    // problems.
    //

    NumberOfPtesWithAlignment = (NumberOfPtes +
            ((MM_PROTO_PTE_ALIGNMENT >> PAGE_SHIFT) - 1)) &
            (~((MM_PROTO_PTE_ALIGNMENT >> PAGE_SHIFT) - 1));

    if (NumberOfPtesWithAlignment < NumberOfPtes) {
        return STATUS_SECTION_TOO_BIG;
    }

    SizeOfSegment = sizeof(SEGMENT) + sizeof(MMPTE) *
                                        (NumberOfPtesWithAlignment - 1);

    NewSegment = ExAllocatePoolWithTag (PagedPool,
                                        SizeOfSegment,
                                        MMSECT);
    if (NewSegment == NULL) {

        //
        // The requested pool could not be allocated.
        // Try to allocate the memory in smaller sizes.
        //

        if (SizeOfSegment < MM_ALLOCATION_FRAGMENT) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Size = MM_ALLOCATION_FRAGMENT;
        PartialSize = SizeOfSegment;

        do {

            if (PartialSize < MM_ALLOCATION_FRAGMENT) {
                PartialSize = (ULONG) ROUND_TO_PAGES (PartialSize);
                Size = PartialSize;
            }

            NewSegment = ExAllocatePoolWithTag (PagedPool,
                                                Size,
                                                MMSECT);
            ExtendedSubsection = ExAllocatePoolWithTag (NonPagedPool,
                                                   sizeof(SUBSECTION),
                                                   'bSmM');

            if ((NewSegment == NULL) || (ExtendedSubsection == NULL)) {
                if (NewSegment) {
                    ExFreePool (NewSegment);
                }
                if (ExtendedSubsection) {
                    ExFreePool (ExtendedSubsection);
                }

                //
                // Free all the previous allocations and return an error.
                //

                while (FirstSubsection != NULL) {
                    ExFreePool (FirstSubsection->SubsectionBase);
                    Last = FirstSubsection->NextSubsection;
                    ExFreePool (FirstSubsection);
                    FirstSubsection = Last;
                }
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            NumberOfNewSubsections += 1;
            RtlZeroMemory (ExtendedSubsection, sizeof(SUBSECTION));

            if (FirstSubsection == NULL) {
                FirstSubsection = ExtendedSubsection;
                Last = ExtendedSubsection;
                NumberOfNewSubsections = 0;
            } else {
                Last->NextSubsection = ExtendedSubsection;
            }

            ExtendedSubsection->PtesInSubsection = Size / sizeof(MMPTE);
            ExtendedSubsection->SubsectionBase = (PMMPTE)NewSegment;
            Last = ExtendedSubsection;
            PartialSize -= Size;
        } while (PartialSize != 0);

        //
        // Reset new segment and free the first subsection, as
        // the subsection after the control area will become the
        // first subsection.
        //

        NewSegment = (PSEGMENT)FirstSubsection->SubsectionBase;
    }

    *Segment = NewSegment;
    RtlZeroMemory (NewSegment, sizeof(SEGMENT));

    ControlArea =
              (PCONTROL_AREA)File->SectionObjectPointer->DataSectionObject;

    //
    // Control area and first subsection have been zeroed when allocated.
    //

    ControlArea->Segment = NewSegment;
    ControlArea->NumberOfSectionReferences = 1;

    if (IgnoreFileSizing == FALSE) {

        //
        // This reference is not from the cache manager.
        //

        ControlArea->NumberOfUserReferences = 1;
    }

    ControlArea->u.Flags.BeingCreated = 1;
    ControlArea->u.Flags.File = 1;

    if (FILE_REMOTE_DEVICE & File->DeviceObject->Characteristics) {

        //
        // This file resides on a redirected drive.
        //

        ControlArea->u.Flags.Networked = 1;
    }

    if (AllocationAttributes & SEC_NOCACHE) {
        ControlArea->u.Flags.NoCache = 1;
    }

    if (IgnoreFileSizing) {
        // Set the was purged flag to indicate that the
        // file size was not explicitly set.
        //

        ControlArea->u.Flags.WasPurged = 1;
    }

    ControlArea->NumberOfSubsections = (USHORT)(1 + NumberOfNewSubsections);
    ControlArea->FilePointer = File;

    ASSERT (ControlArea->u.Flags.GlobalOnlyPerSession == 0);

    Subsection = (PSUBSECTION)(ControlArea + 1);

    if (FirstSubsection) {

        Subsection->NextSubsection = FirstSubsection->NextSubsection;
        Subsection->PtesInSubsection = FirstSubsection->PtesInSubsection;
        ExFreePool (FirstSubsection);
#if DBG
        FirstSubsection = NULL;
#endif //DBG
    } else {
        ASSERT (Subsection->NextSubsection == NULL);
    }

    First = TRUE;

    FileOffset = 0;

    do {

        //
        // Loop through all the subsections and fill in the PTEs.
        //


        TempPte.u.Long = MiGetSubsectionAddressForPte (Subsection);
        TempPte.u.Soft.Prototype = 1;

        //
        // Set all the PTEs to the execute-read-write protection.
        // The section will control access to these and the segment
        // must provide a method to allow other users to map the file
        // for various protections.
        //

        TempPte.u.Soft.Protection = MM_EXECUTE_READWRITE;

        //
        // Align the prototype PTEs on the proper boundary.
        //

        if (First) {

            PointerPte = &NewSegment->ThePtes[0];
            j = (ULONG) (((ULONG_PTR)PointerPte >> PTE_SHIFT) &
                            ((MM_PROTO_PTE_ALIGNMENT / PAGE_SIZE) - 1));

            if (j != 0) {
                j = (MM_PROTO_PTE_ALIGNMENT / PAGE_SIZE) - j;
            }

            NewSegment->PrototypePte = &NewSegment->ThePtes[j];
            NewSegment->ControlArea = ControlArea;
            NewSegment->SizeOfSegment = EndOfFile;
            NewSegment->TotalNumberOfPtes = NumberOfPtes;
            NewSegment->SegmentPteTemplate = TempPte;
            PointerPte = NewSegment->PrototypePte;
            Subsection->SubsectionBase = PointerPte;
            ControlArea->PagedPoolUsage = EX_REAL_POOL_USAGE((sizeof(SEGMENT) + (NumberOfPtes * sizeof(MMPTE))));

            if (Subsection->NextSubsection != NULL) {

                //
                // Multiple segments and subsections.
                // Align first so it is a multiple of 64k sizes.
                //
                //

                NewSegment->NonExtendedPtes = (ULONG)
                  ((((Subsection->PtesInSubsection * sizeof(MMPTE)) -
                    ((PCHAR)NewSegment->PrototypePte - (PCHAR)NewSegment))
                        / sizeof(MMPTE)) &  ~((X64K >> PAGE_SHIFT) - 1));
            } else {
                NewSegment->NonExtendedPtes = NumberOfPtesWithAlignment;
            }
            Subsection->PtesInSubsection = NewSegment->NonExtendedPtes;

            First = FALSE;
        } else {
            PointerPte = (PMMPTE)Subsection->SubsectionBase;
        }

        Subsection->ControlArea = ControlArea;

        Mi4KStartForSubsection(&FileOffset, Subsection);

        Subsection->u.SubsectionFlags.Protection = MM_EXECUTE_READWRITE;

        if (Subsection->NextSubsection == NULL) {

            LastFileChunk = (EndOfFile >> MM4K_SHIFT) - FileOffset;

            //
            // Note this next line restricts the number of bytes mapped by
            // a single subsection to 16TB-4K.  multiple subsections can always
            // be chained together to support an overall file of size 16K TB.
            //

            Subsection->NumberOfFullSectors = (ULONG)LastFileChunk;

            Subsection->u.SubsectionFlags.SectorEndOffset =
                                 (ULONG) EndOfFile & MM4K_MASK;

            j = Subsection->PtesInSubsection;

            Subsection->PtesInSubsection = (ULONG)(
                NumberOfPtesForEntireFile -
                                (FileOffset >> (PAGE_SHIFT - MM4K_SHIFT)));

#if DBG
            MiSubsectionConsistent(Subsection);
#endif

            Subsection->UnusedPtes = j - Subsection->PtesInSubsection;
        } else {

            Subsection->NumberOfFullSectors =
                Subsection->PtesInSubsection << (PAGE_SHIFT - MM4K_SHIFT);
#if DBG
            MiSubsectionConsistent(Subsection);
#endif

        }

        MiFillMemoryPte (PointerPte,
                         (Subsection->PtesInSubsection +
                                Subsection->UnusedPtes) * sizeof(MMPTE),
                         TempPte.u.Long);

        FileOffset += Subsection->PtesInSubsection <<
                                        (PAGE_SHIFT - MM4K_SHIFT);
        Subsection = Subsection->NextSubsection;
    } while (Subsection != NULL);

    return STATUS_SUCCESS;
}

NTSTATUS
MiCreatePagingFileMap (
    OUT PSEGMENT *Segment,
    IN PUINT64 MaximumSize,
    IN ULONG ProtectionMask,
    IN ULONG AllocationAttributes
    )

/*++

Routine Description:

    This function creates the necessary structures to allow the mapping
    of a paging file.

Arguments:

    Segment - Returns the segment object.

    MaximumSize - Supplies the maximum size for the mapping.

    ProtectionMask - Supplies the initial page protection.

    AllocationAttributes - Supplies the allocation attributes for the
                           mapping.

Return Value:

    Returns the status value.

    TBS


--*/


{
    PFN_NUMBER NumberOfPtes;
    ULONG SizeOfSegment;
    ULONG i;
    PCONTROL_AREA ControlArea;
    PSEGMENT NewSegment;
    PMMPTE PointerPte;
    PSUBSECTION Subsection;
    MMPTE TempPte;
    LARGE_INTEGER SpecifiedSize;

    PAGED_CODE();

    //*******************************************************************
    // Create a section backed by paging file.
    //*******************************************************************

    if (*MaximumSize == 0) {
        return STATUS_INVALID_PARAMETER_4;
    }

    //
    // Limit page file backed sections to the pagefile maximum size.
    //

#if defined (_WIN64) || defined (_X86PAE_)
    SpecifiedSize.QuadPart = (ROUND_TO_PAGES (*MaximumSize)) >> PAGE_SHIFT;

    if (SpecifiedSize.HighPart != 0)
#else
    if (*MaximumSize > (((ULONGLONG) 4 * 1024 * 1024 * 1024)) - (1024 * 1024))
#endif
    {
        return STATUS_SECTION_TOO_BIG;
    }

    //
    // Create the segment object.
    //

    //
    // Calculate the number of prototype PTEs to build for this segment.
    //

    NumberOfPtes = BYTES_TO_PAGES (*MaximumSize);

    if (AllocationAttributes & SEC_COMMIT) {

        //
        // Commit the pages for the section.
        //

        if (MiChargeCommitment (NumberOfPtes, NULL) == FALSE) {
            return STATUS_COMMITMENT_LIMIT;
        }
        MM_TRACK_COMMIT (MM_DBG_COMMIT_PAGEFILE_BACKED_SHMEM, NumberOfPtes);
    }

    SizeOfSegment = sizeof(SEGMENT) + sizeof(MMPTE) * ((ULONG)NumberOfPtes - 1);

    NewSegment = ExAllocatePoolWithTag (PagedPool, SizeOfSegment, MMSECT);

    if (NewSegment == NULL) {

        //
        // The requested pool could not be allocated.
        //

        if (AllocationAttributes & SEC_COMMIT) {
            MiReturnCommitment (NumberOfPtes);
            MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_PAGEFILE_BACKED_SHMEM, NumberOfPtes);
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *Segment = NewSegment;

    ControlArea = ExAllocatePoolWithTag (NonPagedPool,
                                 (ULONG)sizeof(CONTROL_AREA) +
                                                (ULONG)sizeof(SUBSECTION),
                                         MMCONTROL);

    if (ControlArea == NULL) {

        //
        // The requested pool could not be allocated.
        //

        ExFreePool (NewSegment);

        if (AllocationAttributes & SEC_COMMIT) {
            MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_PAGEFILE_BACKED_SHMEM, NumberOfPtes);
            MiReturnCommitment (NumberOfPtes);
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Zero control area and first subsection.
    //

    RtlZeroMemory (ControlArea,
                   sizeof(CONTROL_AREA) + sizeof(SUBSECTION));

    ControlArea->NonPagedPoolUsage = EX_REAL_POOL_USAGE(sizeof(CONTROL_AREA) + sizeof(SUBSECTION));

    ControlArea->Segment = NewSegment;
    ControlArea->NumberOfSectionReferences = 1;
    ControlArea->NumberOfUserReferences = 1;
    ControlArea->NumberOfSubsections = 1;

    if (AllocationAttributes & SEC_BASED) {
        ControlArea->u.Flags.Based = 1;
    }

    if (AllocationAttributes & SEC_RESERVE) {
        ControlArea->u.Flags.Reserve = 1;
    }

    if (AllocationAttributes & SEC_COMMIT) {
        ControlArea->u.Flags.Commit = 1;
    }

    Subsection = (PSUBSECTION)(ControlArea + 1);

    Subsection->ControlArea = ControlArea;
    Subsection->PtesInSubsection = (ULONG)NumberOfPtes;
    Subsection->u.SubsectionFlags.Protection = ProtectionMask;

    //
    // Align the prototype PTEs on the proper boundary.
    //

    PointerPte = &NewSegment->ThePtes[0];
    i = (ULONG) (((ULONG_PTR)PointerPte >> PTE_SHIFT) &
                    ((MM_PROTO_PTE_ALIGNMENT / PAGE_SIZE) - 1));

    if (i != 0) {
        i = (MM_PROTO_PTE_ALIGNMENT / PAGE_SIZE) - i;
    }

    //
    // Zero the segment header.
    //

    RtlZeroMemory (NewSegment, sizeof(SEGMENT));

    NewSegment->PrototypePte = &NewSegment->ThePtes[i];

    NewSegment->ControlArea = ControlArea;

    //
    // As size is limited to 2gb, ignore the high part.
    //

    NewSegment->SizeOfSegment = (ULONG_PTR)NumberOfPtes * PAGE_SIZE;
    NewSegment->TotalNumberOfPtes = (ULONG)NumberOfPtes;
    NewSegment->NonExtendedPtes = (ULONG)NumberOfPtes;

    ControlArea->PagedPoolUsage = EX_REAL_POOL_USAGE((sizeof(SEGMENT) + (NumberOfPtes * sizeof(MMPTE))));

    PointerPte = NewSegment->PrototypePte;
    Subsection->SubsectionBase = PointerPte;
    TempPte = ZeroPte;

    if (AllocationAttributes & SEC_COMMIT) {
        TempPte.u.Soft.Protection = ProtectionMask;

        //
        // Record commitment charging.
        //

        NewSegment->NumberOfCommittedPages = NumberOfPtes;

        ExAcquireFastMutex (&MmSectionCommitMutex);
        MmSharedCommit += NewSegment->NumberOfCommittedPages;
        ExReleaseFastMutex (&MmSectionCommitMutex);
    }

    NewSegment->SegmentPteTemplate.u.Soft.Protection = ProtectionMask;

    for (i = 0; i < NumberOfPtes; i += 1) {

        //
        // Set all the prototype PTEs to either no access or demand zero
        // depending on the commit flag.
        //

        MI_WRITE_INVALID_PTE (PointerPte, TempPte);
        PointerPte += 1;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
NtOpenSection (
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )

/*++

Routine Description:

    This function opens a handle to a section object with the specified
    desired access.

Arguments:


    Sectionhandle - Supplies a pointer to a variable that will
                    receive the section object handle value.

    DesiredAccess - Supplies the desired types of access  for the
                    section.

    DesiredAccess Flags

         EXECUTE - Execute access to the section is desired.

         READ - Read access to the section is desired.

         WRITE - Write access to the section is desired.

    ObjectAttributes - Supplies a pointer to an object attributes structure.

Return Value:

    Returns the status

    TBS


--*/

{
    HANDLE Handle;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    PAGED_CODE();
    //
    // Get previous processor mode and probe output arguments if necessary.
    //

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {
        try {
            ProbeForWriteHandle(SectionHandle);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
    }

    //
    // Open handle to the section object with the specified desired
    // access.
    //

    Status = ObOpenObjectByName (ObjectAttributes,
                                 MmSectionObjectType,
                                 PreviousMode,
                                 NULL,
                                 DesiredAccess,
                                 NULL,
                                 &Handle
                                 );

    try {
        *SectionHandle = Handle;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return Status;
    }

    return Status;
}

CCHAR
MiGetImageProtection (
    IN ULONG SectionCharacteristics
    )

/*++

Routine Description:

    This function takes a section characteristic mask from the
    image and converts it to an PTE protection mask.

Arguments:

    SectionCharacteristics - Supplies the characteristics mask from the
                             image.

Return Value:

    Returns the protection mask for the PTE.

--*/

{
    ULONG Index;
    PAGED_CODE();

    Index = 0;
    if (SectionCharacteristics & IMAGE_SCN_MEM_EXECUTE) {
        Index |= 1;
    }
    if (SectionCharacteristics & IMAGE_SCN_MEM_READ) {
        Index |= 2;
    }
    if (SectionCharacteristics & IMAGE_SCN_MEM_WRITE) {
        Index |= 4;
    }
    if (SectionCharacteristics & IMAGE_SCN_MEM_SHARED) {
        Index |= 8;
    }

    return MmImageProtectionArray[Index];
}

PFN_NUMBER
MiGetPageForHeader (
    VOID
    )

/*++

Routine Description:

    This non-pagable function acquires the PFN lock, removes
    a page and updates the PFN database as though the page was
    ready to be deleted if the reference count is decremented.

Arguments:

    None.

Return Value:

    Returns the physical page frame number.

--*/

{
    KIRQL OldIrql;
    PFN_NUMBER PageFrameNumber;
    PMMPFN Pfn1;
    PEPROCESS Process;
    ULONG PageColor;

    Process = PsGetCurrentProcess();
    PageColor = MI_PAGE_COLOR_VA_PROCESS ((PVOID)X64K,
                                          &Process->NextPageColor);

    //
    // Lock the PFN database and get a page.
    //

    LOCK_PFN (OldIrql);

    MiEnsureAvailablePageOrWait (NULL, NULL);

    //
    // Remove page for 64k alignment.
    //

    PageFrameNumber = MiRemoveAnyPage (PageColor);

    //
    // Increment the reference count for the page so the
    // paging I/O will work, and so this page cannot be stolen from us.
    //

    Pfn1 = MI_PFN_ELEMENT (PageFrameNumber);
    Pfn1->u3.e2.ReferenceCount += 1;

#ifndef PFN_CONSISTENCY
    //
    // Don't need the PFN lock for the fields below...
    //
    UNLOCK_PFN (OldIrql);
#endif

    Pfn1->OriginalPte = ZeroPte;
    Pfn1->PteAddress = (PVOID) (ULONG_PTR)X64K;
    MI_SET_PFN_DELETED (Pfn1);

    CONSISTENCY_UNLOCK_PFN (OldIrql);

    return PageFrameNumber;
}

VOID
MiUpdateImageHeaderPage (
    IN PMMPTE PointerPte,
    IN PFN_NUMBER PageFrameNumber,
    IN PCONTROL_AREA ControlArea
    )

/*++

Routine Description:

    This non-pagable function acquires the PFN lock, and
    turns the specified prototype PTE into a transition PTE
    referring to the specified physical page.  It then
    decrements the reference count causing the page to
    be placed on the standby or modified list.

Arguments:

    PointerPte - Supplies the PTE to set into the transition state.

    PageFrameNumber - Supplies the physical page.

    ControlArea - Supplies the control area for the prototype PTEs.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);

    MiMakeSystemAddressValidPfn (PointerPte);

    MiInitializeTransitionPfn (PageFrameNumber, PointerPte, 0xFFFFFFFF);
    ControlArea->NumberOfPfnReferences += 1;

    //
    // Add the page to the standby list.
    //

    MiDecrementReferenceCount (PageFrameNumber);

    UNLOCK_PFN (OldIrql);
    return;
}

VOID
MiRemoveImageHeaderPage (
    IN PFN_NUMBER PageFrameNumber
    )

/*++

Routine Description:

    This non-pagable function acquires the PFN lock, and decrements
    the reference count thereby causing the physical page to
    be deleted.

Arguments:

    PageFrameNumber - Supplies the PFN to decrement.

Return Value:

    None.

--*/
{
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);
    MiDecrementReferenceCount (PageFrameNumber);
    UNLOCK_PFN (OldIrql);
    return;
}

PCONTROL_AREA
MiFindImageSectionObject(
    IN PFILE_OBJECT File,
    OUT PBOOLEAN GlobalNeeded
    )

/*++

Routine Description:

    This function searches the control area chains (if any) for an existing
    cache of the specified image file.  For non-global control areas, there is
    no chain and the control area is shared for all callers and sessions.
    Likewise for systemwide global control areas.

    However, for global PER-SESSION control areas, we must do the walk.

Arguments:

    File - Supplies the file object for the image file.

    GlobalNeeded - Supplies a pointer to store whether a global control area is
                   required as a placeholder.  This can only be set when there
                   is already some global control area in the list - ie: our
                   caller should only rely on this when this function returns
                   NULL so the caller knows what kind of control area to
                   insert.

Return Value:

    Returns the matching control area or NULL if one does not exist.

Environment:

    Must be holding the PFN lock.

--*/

{
    PCONTROL_AREA ControlArea;
    PLARGE_CONTROL_AREA LargeControlArea;
    PLIST_ENTRY Head, Next;
    ULONG SessionId;

    MM_PFN_LOCK_ASSERT();

    *GlobalNeeded = FALSE;

    //
    // Get first (if any) control area pointer.
    //

    ControlArea = (PCONTROL_AREA)(File->SectionObjectPointer->ImageSectionObject);

    //
    // If no control area, or the control area is not session global,
    // then our job is easy.  Note, however, that they each require different
    // return values as they represent different states.
    //

    if (ControlArea == NULL) {
        return NULL;
    }

    if (ControlArea->u.Flags.GlobalOnlyPerSession == 0) {
        return ControlArea;
    }

    LargeControlArea = (PLARGE_CONTROL_AREA) ControlArea;

    //
    // Get the current session ID and search for a matching control area.
    //

    SessionId = PsGetCurrentProcess()->SessionId;

    if (LargeControlArea->SessionId == SessionId) {
        return (PCONTROL_AREA) LargeControlArea;
    }

    //
    // Must search the control area list for a matching session ID.
    //

    Head = &LargeControlArea->UserGlobalList;

    for (Next = Head->Flink; Next != Head; Next = Next->Flink) {

        LargeControlArea = CONTAINING_RECORD (Next, LARGE_CONTROL_AREA, UserGlobalList);

        ASSERT (LargeControlArea->u.Flags.GlobalOnlyPerSession == 1);

        if (LargeControlArea->SessionId == SessionId) {
            return (PCONTROL_AREA) LargeControlArea;
        }
    }

    //
    // No match, so tell our caller to create a new global control area.
    //

    *GlobalNeeded = TRUE;

    return NULL;
}

VOID
MiInsertImageSectionObject(
    IN PFILE_OBJECT File,
    IN PCONTROL_AREA InputControlArea
    )

/*++

Routine Description:

    This function inserts the control area into the file's section object
    pointers.  For non-global control areas, there is no chain and the
    control area is shared for all callers and sessions.
    Likewise for systemwide global control areas.

    However, for global PER-SESSION control areas, we must do a list insertion.

Arguments:

    File - Supplies the file object for the image file.

    InputControlArea - Supplies the control area to insert.

Return Value:

    None.

Environment:

    Must be holding the PFN lock.

--*/

{
    PLIST_ENTRY Head;
    PLARGE_CONTROL_AREA ControlArea;
    PLARGE_CONTROL_AREA FirstControlArea;
#if DBG
    PLIST_ENTRY Next;
    PLARGE_CONTROL_AREA NextControlArea;
#endif

    MM_PFN_LOCK_ASSERT();

    ControlArea = (PLARGE_CONTROL_AREA) InputControlArea;

    //
    // If this is not a global-per-session control area or just a placeholder
    // control area (with no chain already in place) then just put it in.
    //

    FirstControlArea = (PLARGE_CONTROL_AREA)(File->SectionObjectPointer->ImageSectionObject);

    if (FirstControlArea == NULL) {
        if (ControlArea->u.Flags.GlobalOnlyPerSession == 0) {
            (PCONTROL_AREA)(File->SectionObjectPointer->ImageSectionObject) =
                                                    (PCONTROL_AREA) ControlArea;
            return;
        }
    }

    //
    // A per-session control area needs to be inserted...
    //

    ASSERT (ControlArea->u.Flags.GlobalOnlyPerSession == 1);

    ControlArea->SessionId = PsGetCurrentProcess()->SessionId;

    //
    // If the control area list is empty, just initialize links for this entry.
    //

    if (File->SectionObjectPointer->ImageSectionObject == NULL) {
        InitializeListHead (&ControlArea->UserGlobalList);
    }
    else {

        //
        // Insert new entry before the current first entry.  The control area
        // must be in the midst of creation/deletion or have a valid session
        // ID to be inserted.
        //

        ASSERT (ControlArea->u.Flags.BeingDeleted ||
                ControlArea->u.Flags.BeingCreated ||
                ControlArea->SessionId != (ULONG)-1);

        FirstControlArea = (PLARGE_CONTROL_AREA)(File->SectionObjectPointer->ImageSectionObject);

        Head = &FirstControlArea->UserGlobalList;

#if DBG
        //
        // Ensure no duplicate session IDs exist in the list.
        //

        for (Next = Head->Flink; Next != Head; Next = Next->Flink) {
            NextControlArea = CONTAINING_RECORD (Next, LARGE_CONTROL_AREA, UserGlobalList);
            ASSERT (NextControlArea->SessionId != (ULONG)-1 &&
                    NextControlArea->SessionId != ControlArea->SessionId);
        }
#endif

        InsertTailList (Head, &ControlArea->UserGlobalList);
    }

    //
    // Update first control area pointer.
    //

    (PCONTROL_AREA)(File->SectionObjectPointer->ImageSectionObject) =
                                                (PCONTROL_AREA) ControlArea;
}

VOID
MiRemoveImageSectionObject(
    IN PFILE_OBJECT File,
    IN PCONTROL_AREA InputControlArea
    )

/*++

Routine Description:

    This function searches the control area chains (if any) for an existing
    cache of the specified image file.  For non-global control areas, there is
    no chain and the control area is shared for all callers and sessions.
    Likewise for systemwide global control areas.

    However, for global PER-SESSION control areas, we must do the walk.

    Upon finding the specified control area, we unlink it.

Arguments:

    File - Supplies the file object for the image file.

    InputControlArea - Supplies the control area to remove.

Return Value:

    None.

Environment:

    Must be holding the PFN lock.

--*/

{
    PLIST_ENTRY Head, Next;
    PLARGE_CONTROL_AREA ControlArea;
    PLARGE_CONTROL_AREA FirstControlArea;
    PLARGE_CONTROL_AREA NextControlArea;

    MM_PFN_LOCK_ASSERT();

    ControlArea = (PLARGE_CONTROL_AREA) InputControlArea;

    FirstControlArea = (PLARGE_CONTROL_AREA)(File->SectionObjectPointer->ImageSectionObject);

    //
    // Get a pointer to the first control area.  If this is not a
    // global-per-session control area, then there is no list, so we're done.
    //

    if (FirstControlArea->u.Flags.GlobalOnlyPerSession == 0) {
        ASSERT (ControlArea->u.Flags.GlobalOnlyPerSession == 0);

        (PCONTROL_AREA)(File->SectionObjectPointer->ImageSectionObject) = NULL;
        return;
    }

    //
    // A list may exist.  Walk it as necessary and delete the requested entry.
    //

    if (FirstControlArea == ControlArea) {

        //
        // The first entry is the one to remove.  If it is the only entry
        // in the list, then the new first entry pointer will be NULL.
        // Otherwise, get a pointer to the next entry and unlink the current.
        //

        if (IsListEmpty (&FirstControlArea->UserGlobalList)) {
            NextControlArea = NULL;
        } else {
            Next = FirstControlArea->UserGlobalList.Flink;
            RemoveEntryList (&FirstControlArea->UserGlobalList);
            NextControlArea = CONTAINING_RECORD (Next,
                                                 LARGE_CONTROL_AREA,
                                                 UserGlobalList);

            ASSERT (NextControlArea->u.Flags.GlobalOnlyPerSession == 1);
        }

        (PCONTROL_AREA)(File->SectionObjectPointer->ImageSectionObject) =
                                            (PCONTROL_AREA) NextControlArea;
        return;
    }

    //
    // Remove the entry, note that the ImageSectionObject need not be updated
    // as the entry is not at the head.
    //

#if DBG
    Head = &FirstControlArea->UserGlobalList;

    for (Next = Head->Flink; Next != Head; Next = Next->Flink) {

        NextControlArea = CONTAINING_RECORD (Next,
                                             LARGE_CONTROL_AREA,
                                             UserGlobalList);

        ASSERT (NextControlArea->u.Flags.GlobalOnlyPerSession == 1);

        if (NextControlArea == ControlArea) {
            break;
        }
    }
    ASSERT (Next != Head);
#endif

    RemoveEntryList (&ControlArea->UserGlobalList);
}


NTSTATUS
MiGetWritablePagesInSection(
    IN PSECTION Section,
    OUT PULONG WritablePages
    )

/*++

Routine Description:

    This routine calculates the number of writable pages in the image.

Arguments:

    Section - Supplies the section for the image.

    WritablePages - Supplies a pointer to fill with the
                    number of writable pages.

Return Value:

    STATUS_SUCCESS if all went well, otherwise various NTSTATUS error codes.

Environment:

    Kernel mode, APC_LEVEL and below, MmSystemLoadLock held.

--*/

{
    NTSTATUS Status;
    PVOID ViewBase;
    SIZE_T ViewSize;
    ULONG PagesInSubsection;
    ULONG Protection;
    ULONG NumberOfSubsections;
    ULONG OffsetToSectionTable;
    ULONG SectionVirtualSize;
    PEPROCESS Process;
    LARGE_INTEGER SectionOffset;
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_SECTION_HEADER SectionTableEntry;

    PAGED_CODE();

    ViewBase = NULL;
    ViewSize = 0;
    SectionOffset.QuadPart = 0;

    *WritablePages = 0;

    Process = PsGetCurrentProcess();

    Status = MmMapViewOfSection (Section,
                                 Process,
                                 &ViewBase,
                                 0,
                                 0,
                                 &SectionOffset,
                                 &ViewSize,
                                 ViewUnmap,
                                 0,
                                 PAGE_EXECUTE);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // The DLL is mapped as a data file not as an image.  Pull apart the
    // executable header.  The security checks have already been
    // done as part of creating the section in the first place.
    //

    DosHeader = (PIMAGE_DOS_HEADER) ViewBase;

    ASSERT (DosHeader->e_magic == IMAGE_DOS_SIGNATURE);

#ifndef i386
    ASSERT (((ULONG)DosHeader->e_lfanew & 3) == 0);
#endif

    ASSERT ((ULONG)DosHeader->e_lfanew <= ViewSize);

    NtHeader = (PIMAGE_NT_HEADERS)((PCHAR)DosHeader +
                                      (ULONG)DosHeader->e_lfanew);

    OffsetToSectionTable = sizeof(ULONG) +
                              sizeof(IMAGE_FILE_HEADER) +
                              NtHeader->FileHeader.SizeOfOptionalHeader;

    SectionTableEntry = (PIMAGE_SECTION_HEADER)((PCHAR)NtHeader +
                                OffsetToSectionTable);

    NumberOfSubsections = NtHeader->FileHeader.NumberOfSections;

    while (NumberOfSubsections > 0) {

        Protection = MiGetImageProtection (SectionTableEntry->Characteristics);

        if (Protection & MM_PROTECTION_WRITE_MASK) {

            //
            // Handle the case where the virtual size is 0.
            //

            if (SectionTableEntry->Misc.VirtualSize == 0) {
                SectionVirtualSize = SectionTableEntry->SizeOfRawData;
            } else {
                SectionVirtualSize = SectionTableEntry->Misc.VirtualSize;
            }

            PagesInSubsection =
                MI_ROUND_TO_SIZE (SectionVirtualSize, PAGE_SIZE) >> PAGE_SHIFT;

            *WritablePages += PagesInSubsection;
        }

        SectionTableEntry += 1;
        NumberOfSubsections -= 1;
    }

    Status = MmUnmapViewOfSection (Process, ViewBase);

    ASSERT (NT_SUCCESS(Status));

    return Status;
}


VOID
MiFlushDataSection(
    IN PFILE_OBJECT File
    )

/*++

Routine Description:

    This routine flushes the data section if there is one.

Arguments:

    File - Supplies the file object.

Return Value:

    None.

Environment:

    Kernel mode, APC_LEVEL and below.

--*/

{
    KIRQL OldIrql;
    IO_STATUS_BLOCK IoStatus;
    PCONTROL_AREA ControlArea;

    LOCK_PFN (OldIrql);

    ControlArea = (PCONTROL_AREA) File->SectionObjectPointer->DataSectionObject;

    if (ControlArea) {
        if (ControlArea->NumberOfSystemCacheViews) {
            UNLOCK_PFN (OldIrql);
            CcFlushCache (File->SectionObjectPointer,
                          NULL,
                          0,
                          &IoStatus);

        } else {
            UNLOCK_PFN (OldIrql);
            MmFlushSection (File->SectionObjectPointer,
                            NULL,
                            0,
                            &IoStatus,
                            TRUE);
        }
    }
    else {
        UNLOCK_PFN (OldIrql);
    }
}
