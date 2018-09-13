/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

   super.c

Abstract:

    This module contains the routines which implement the SuperSection
    object.

Author:

    Lou Perazzoli (loup) 4-Apr-92

Revision History:

--*/

#include "mi.h"
#include "zwapi.h"


VOID
MiSuperSectionDelete (
    PVOID Object
    );

BOOLEAN
MiSuperSectionInitialization (
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,MiSuperSectionInitialization)
#endif

#define MM_MAX_SUPERSECTION_COUNT (32)

POBJECT_TYPE MmSuperSectionObjectType;

#define STATUS_TOO_MANY_SECTIONS ((NTSTATUS)0xC0033333)
#define STATUS_INCOMPLETE_MAP    ((NTSTATUS)0xC0033334)


extern GENERIC_MAPPING MiSectionMapping;

typedef struct _MMSUPER_SECTION {
    ULONG NumberOfSections;
    PSECTION SectionPointers[1];
} MMSUPER_SECTION, *PMMSUPER_SECTION;


NTSTATUS
NtCreateSuperSection (
    OUT PHANDLE SuperSectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN ULONG Count,
    IN HANDLE SectionHandles[]
    )

/*++

Routine Description:

    This routine creates a super section object.  A supersection
    object consists a group of sections that are mapped as images.

Arguments:

    SuperSectionHandle - Returns a handle to the created supersection.

    DesiredAccess - Supplies the desired access for the super section.

    ObjectAttributes - Supplies the object attributes for the super
                       section.

    Count - Supplies the number of sections contained in the section
            handle array.

    SectionHandles[] - Supplies the section handles to place into
                       the supersection.


Return Value:

    Returns the status

    TBS

--*/

{
    NTSTATUS Status;
    PSECTION Section;
    PCONTROL_AREA ControlArea;
    HANDLE CapturedHandle;
    HANDLE CapturedHandles[MM_MAX_SUPERSECTION_COUNT];
    PMMSUPER_SECTION SuperSection;
    KPROCESSOR_MODE PreviousMode;
    ULONG RefCount;
    ULONG i;
    KIRQL OldIrql;

    if (Count > MM_MAX_SUPERSECTION_COUNT) {
        return STATUS_TOO_MANY_SECTIONS;
    }

    try {

        if (PreviousMode != KernelMode) {
            ProbeForWriteHandle (SuperSectionHandle);
        }

        i= 0;
        do {
            CapturedHandles[i] = SectionHandles[i];
            i += 1;
        } while (i < Count);

    } except (ExSystemExceptionFilter()) {

        //
        // If an exception occurs during the probe or capture
        // of the initial values, then handle the exception and
        // return the exception code as the status value.
        //

        return GetExceptionCode();
    }

    Status = ObCreateObject (PreviousMode,
                             MmSuperSectionObjectType,
                             ObjectAttributes,
                             PreviousMode,
                             NULL,
                             sizeof(MMSUPER_SECTION) +
                                       (sizeof(PSECTION) * (Count - 1)),
                             sizeof(MMSUPER_SECTION) +
                                       (sizeof(PSECTION) * (Count - 1)),
                             0,
                             (PVOID *)&SuperSection);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    SuperSection->NumberOfSections = Count;

    i = 0;
    RefCount = 0;
    do {

        //
        // Get a referenced pointer to the specified objects with
        // the desired access.
        //

        Status = ObReferenceObjectByHandle(CapturedHandles[i],
                                           DesiredAccess,
                                           MmSectionObjectType,
                                           PreviousMode,
                                           (PVOID *)&Section,
                                           NULL);

        if (NT_SUCCESS(Status) != FALSE) {
            if (Section->u.Flags.Image == 0) {

                //
                // This is not an image section, return an error.
                //

                Status = STATUS_SECTION_NOT_IMAGE;
                goto ServiceFailed;
            }
            RefCount += 1;
            SuperSection->SectionPointers[i] = Section;
        } else {
            goto ServiceFailed;
        }

        i += 1;
    } while (i < Count);

    i= 0;
    do {

        //
        // For each section increment the number of section references
        // count.
        //

        ControlArea = SuperSection->SectionPointers[i]->Segment->ControlArea;

        LOCK_PFN (OldIrql);
        ControlArea->NumberOfSectionReferences += 1;
        ControlArea->NumberOfUserReferences += 1;
        UNLOCK_PFN (OldIrql);
        i++;

    } while (i < Count);


    Status = ObInsertObject (SuperSection,
                             NULL,
                             DesiredAccess,
                             0,
                             (PVOID *)NULL,
                             &CapturedHandle);

    try {
        *SuperSectionHandle = CapturedHandle;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return Status;
    }
    return Status;

ServiceFailed:
    while (RefCount > 0) {
        RefCount -= 1;
        ObDereferenceObject(SuperSection->SectionPointers[RefCount]);
    }

    //
    // Delete the supersection object as it was never inserted into
    // a handle table.
    //

    ObDereferenceObject (SuperSection);
    return Status;
}


NTSTATUS
NtOpenSuperSection (
    OUT PHANDLE SuperSectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    )

/*++

Routine Description:

    This routine opens a super section object.  A supersection
    object consists a group of sections that are mapped as images.

Arguments:

    SuperSectionHandle - Returns a handle to the created supersection.

    DesiredAccess - Supplies the desired access for the super section.

    ObjectAttributes - Supplies the object attributes for the super
                       section.


Return Value:

    Returns the status

    TBS

--*/

{
    HANDLE Handle;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    //
    // Get previous processor mode and probe output arguments if necessary.
    //

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {
        try {
            ProbeForWriteHandle(SuperSectionHandle);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
    }

    //
    // Open handle to the super section object with the specified desired
    // access.
    //

    Status = ObOpenObjectByName (ObjectAttributes,
                                 MmSuperSectionObjectType,
                                 PreviousMode,
                                 NULL,
                                 DesiredAccess,
                                 NULL,
                                 &Handle
                                 );

    try {
        *SuperSectionHandle = Handle;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return Status;
    }

    return Status;
}


NTSTATUS
NtMapViewOfSuperSection (
    IN HANDLE SuperSectionHandle,
    IN HANDLE ProcessHandle,
    IN OUT PULONG Count,
    OUT PVOID BaseAddress[],
    OUT ULONG ViewSize[],
    IN SECTION_INHERIT InheritDisposition,
    )

/*++

Routine Description:

    This routine maps into the specified process a view of each
    section contained within the supersection.

Arguments:

    SuperSectionHandle - Supplies a handle to the supersection.

    ProcessHandle - Supplies a handle to the process in which to
                    map the supersection's sections.

    Count - Supplies the number of elements in the BaseAddress and
            ViewSize arrays, returns the number of views actually
            mapped.


    BaseAddresses[] - Returns the base address of each view that was mapped.

    ViewSize[] - Returns the view size of each view that was mapped.

    InheritDisposition - Supplies the inherit disposition to be applied
                         to each section which is contained in the
                         super section.

Return Value:

    Returns the status

    TBS

--*/

{
    NTSTATUS Status;
    PVOID CapturedBases[MM_MAX_SUPERSECTION_COUNT];
    ULONG CapturedViews[MM_MAX_SUPERSECTION_COUNT];
    PMMSUPER_SECTION SuperSection;
    KPROCESSOR_MODE PreviousMode;
    ULONG i;
    ULONG CapturedCount;
    ULONG NumberMapped;
    PEPROCESS Process;
    LARGE_INTEGER LargeZero = {0,0};


    PreviousMode = KeGetPreviousMode();

    try {
        ProbeForWriteUlong (Count);
        CapturedCount = *Count;

        if (PreviousMode != KernelMode) {
            ProbeForWrite (BaseAddress,
                           sizeof(PVOID) * CapturedCount,
                           sizeof(PVOID));
            ProbeForWrite (ViewSize,
                           sizeof(ULONG) * CapturedCount,
                           sizeof(ULONG));
        }

    } except (ExSystemExceptionFilter()) {

        //
        // If an exception occurs during the probe or capture
        // of the initial values, then handle the exception and
        // return the exception code as the status value.
        //

        return GetExceptionCode();
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
    // Reference the supersection object.
    //

    Status = ObReferenceObjectByHandle ( SuperSectionHandle,
                                         SECTION_MAP_EXECUTE,
                                         MmSuperSectionObjectType,
                                         PreviousMode,
                                         (PVOID *)&SuperSection,
                                         NULL );

    if (!NT_SUCCESS(Status)) {
        ObDereferenceObject (Process);
        return Status;
    }

    if (CapturedCount < SuperSection->NumberOfSections) {
        ObDereferenceObject (Process);
        ObDereferenceObject (SuperSection);
        return STATUS_BUFFER_TOO_SMALL;
    }

    NumberMapped = 0;
    do {

        //
        // For each section within the supersection, map a view in
        // the specified process.
        //

        Status = MmMapViewOfSection (SuperSection->SectionPointers[i],
                                     Process,
                                     &CapturedBases[i],
                                     0,
                                     0,
                                     &LargeZero,
                                     &CapturedViews[i],
                                     InheritDisposition,
                                     0,
                                     PAGE_EXECUTE);

        if (NT_SUCCESS (Status) == FALSE) {
            Status = STATUS_INCOMPLETE_MAP;
            break;
        }
        NumberMapped++;
    } while (NumberMapped < SuperSection->NumberOfSections);

    //
    // Dereference the supersection and the process.
    //

    ObDereferenceObject (SuperSection);
    ObDereferenceObject (Process);

    try {
        *Count = NumberMapped;
        i = 0;

        do {

            //
            // Store the captured view base and sizes for each section
            // that was mapped.
            //

            BaseAddress[i] = CapturedBases[i];
            ViewSize[i] = CapturedViews[i];

            i++;
        } while (i < NumberMapped);

    } except (ExSystemExceptionFilter()) {
        NOTHING;
    }

    return(Status);
}



#if 0

NTSTATUS
NtQuerySuperSection (
    IN HANDLE SuperSectionHandle,
    IN SUPERSECTION_INFORMATION_CLASS SectionInformationClass,
    OUT PVOID SuperSectionInformation,
    IN ULONG SuperSectionInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )

Routine Description:

   This function returns information about an opened supersection object.
   This function provides the capability to determine the basic section
   information about each section in the supersection or the image
   information about each section in the supersection.

Arguments:

    SuperSectionHandle - Supplies an open handle to a section object.

    SectionInformationClass - The section information class about
        which to retrieve information.

    SuperSectionInformation - A pointer to a buffer that receives the
        specified information.  The format and content of the buffer
        depend on the specified section class.


    SuperSectionInformationLength - Specifies the length in bytes of the
        section information buffer.

    ReturnLength - An optional pointer which, if specified, receives
        the number of bytes placed in the section information buffer.


Return Value:

    Returns the status

    TBS


--*/


#endif //0

VOID
MiSuperSectionDelete (
    PVOID Object
    )

/*++

Routine Description:


    This routine is called by the object management procedures whenever
    the last reference to a super section object has been removed.
    This routine dereferences the associated segment objects.

Arguments:

    Object - a pointer to the body of the supersection object.

Return Value:

    None.

--*/

{
    PMMSUPER_SECTION SuperSection;
    PCONTROL_AREA ControlArea;
    KIRQL OldIrql;
    ULONG i = 0;

    SuperSection = (PMMSUPER_SECTION)Object;

    do {

        //
        // For each section increment the number of section references
        // count.
        //

        ControlArea = SuperSection->SectionPointers[i]->Segment->ControlArea;

        LOCK_PFN (OldIrql);
        ControlArea->NumberOfSectionReferences -= 1;
        ControlArea->NumberOfUserReferences -= 1;
        UNLOCK_PFN (OldIrql);
        ObDereferenceObject (SuperSection->SectionPointers[i]);
        i++;

    } while (i < SuperSection->NumberOfSections);

    return;
}

BOOLEAN
MiSuperSectionInitialization (
    )

/*++

Routine Description:

    This function creates the section object type descriptor at system
    initialization and stores the address of the object type descriptor
    in global storage.

Arguments:

    None.

Return Value:

    TRUE - Initialization was successful.

    FALSE - Initialization Failed.



--*/

{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING TypeName;

    //
    // Initialize the common fields of the Object Type Initializer record
    //

    RtlZeroMemory( &ObjectTypeInitializer, sizeof( ObjectTypeInitializer ) );
    ObjectTypeInitializer.Length = sizeof( ObjectTypeInitializer );
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    ObjectTypeInitializer.GenericMapping = MiSectionMapping;
    ObjectTypeInitializer.PoolType = PagedPool;

    //
    // Initialize string descriptor.
    //

    RtlInitUnicodeString (&TypeName, L"SuperSection");

    //
    // Create the section object type descriptor
    //

    ObjectTypeInitializer.ValidAccessMask = SECTION_ALL_ACCESS;
    ObjectTypeInitializer.DeleteProcedure = MiSuperSectionDelete;
    ObjectTypeInitializer.GenericMapping = MiSectionMapping;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    if ( !NT_SUCCESS(ObCreateObjectType(&TypeName,
                                     &ObjectTypeInitializer,
                                     (PSECURITY_DESCRIPTOR) NULL,
                                     &MmSuperSectionObjectType
                                     )) ) {
        return FALSE;
    }

    return TRUE;

}
