/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   querysec.c

Abstract:

    This module contains the routines which implement the
    NtQuerySection service.

Author:

    Lou Perazzoli (loup) 22-May-1989

Revision History:

--*/


#include "mi.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtQuerySection)
#endif


NTSTATUS
NtQuerySection(
    IN HANDLE SectionHandle,
    IN SECTION_INFORMATION_CLASS SectionInformationClass,
    OUT PVOID SectionInformation,
    IN ULONG SectionInformationLength,
    OUT PULONG ReturnLength OPTIONAL
     )

/*++

Routine Description:

   This function returns information about an opened section object.
   This function provides the capability to determine the base address,
   size, granted access, and allocation of an opened section object.

Arguments:

    SectionHandle - Supplies an open handle to a section object.

    SectionInformationClass - The section information class about
        which to retrieve information.

    SectionInformation - A pointer to a buffer that receives the
        specified information.  The format and content of the buffer
        depend on the specified section class.

       SectionInformation Format by Information Class:

       SectionBasicInformation - Data type is PSECTION_BASIC_INFORMATION.

           SECTION_BASIC_INFORMATION Structure

           PVOID BaseAddress - The base virtual address of the
               section if the section is based.

           LARGE_INTEGER MaximumSize - The maximum size of the section in
               bytes.

           ULONG AllocationAttributes - The allocation attributes
               flags.

               AllocationAttributes Flags

               SEC_BASED - The section is a based section.

               SEC_TILE - The section must be allocated in the first
                   512mb of the virtual address space.

               SEC_FILE - The section is backed by a data file.

               SEC_RESERVE - All pages of the section were initially
                   set to the reserved state.

               SEC_COMMIT - All pages of the section were initially
                   to the committed state.

               SEC_IMAGE - The section was mapped as an executable
                   image file.

        SECTION_IMAGE_INFORMATION

    SectionInformationLength - Specifies the length in bytes of the
        section information buffer.

    ReturnLength - An optional pointer which, if specified, receives
        the number of bytes placed in the section information buffer.


Return Value:

    Returns the status

    TBS


--*/

{
    PSECTION Section;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Get previous processor mode and probe output argument if necessary.
    //

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {

        //
        // Check arguments.
        //

        try {

            ProbeForWrite(SectionInformation,
                          SectionInformationLength,
                          sizeof(ULONG));

            if (ARGUMENT_PRESENT (ReturnLength)) {
                ProbeForWriteUlong(ReturnLength);
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {

            //
            // If an exception occurs during the probe or capture
            // of the initial values, then handle the exception and
            // return the exception code as the status value.
            //

            return GetExceptionCode();
        }
    }

    //
    // Check argument validity.
    //

    if ((SectionInformationClass != SectionBasicInformation) &&
        (SectionInformationClass != SectionImageInformation)) {
        return STATUS_INVALID_INFO_CLASS;
    }

    if (SectionInformationClass == SectionBasicInformation) {
        if (SectionInformationLength < (ULONG)sizeof(SECTION_BASIC_INFORMATION)) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }
    } else {
        if (SectionInformationLength < (ULONG)sizeof(SECTION_IMAGE_INFORMATION)) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }
    }

    //
    // Reference section object by handle for READ access, get the information
    // from the section object, dereference the section
    // object, fill in information structure, optionally return the length of
    // the information structure, and return service status.
    //

    Status = ObReferenceObjectByHandle(SectionHandle, SECTION_QUERY,
                                       MmSectionObjectType,
                                       PreviousMode, (PVOID *)&Section, NULL);

    if (NT_SUCCESS(Status)) {

        try {

            if (SectionInformationClass == SectionBasicInformation) {
                ((PSECTION_BASIC_INFORMATION)SectionInformation)->BaseAddress =
                                           (PVOID)Section->Address.StartingVpn;

                ((PSECTION_BASIC_INFORMATION)SectionInformation)->MaximumSize =
                                                 Section->SizeOfSection;

                ((PSECTION_BASIC_INFORMATION)SectionInformation)->AllocationAttributes =
                                                        0;

                if (Section->u.Flags.Image) {
                    ((PSECTION_BASIC_INFORMATION)SectionInformation)->AllocationAttributes =
                                                        SEC_IMAGE;
                }
                if (Section->u.Flags.Based) {
                    ((PSECTION_BASIC_INFORMATION)SectionInformation)->AllocationAttributes |=
                                                        SEC_BASED;
                }
                if (Section->u.Flags.File) {
                    ((PSECTION_BASIC_INFORMATION)SectionInformation)->AllocationAttributes |=
                                                        SEC_FILE;
                }
                if (Section->u.Flags.NoCache) {
                    ((PSECTION_BASIC_INFORMATION)SectionInformation)->AllocationAttributes |=
                                                        SEC_NOCACHE;
                }
                if (Section->u.Flags.Reserve) {
                    ((PSECTION_BASIC_INFORMATION)SectionInformation)->AllocationAttributes |=
                                                        SEC_RESERVE;
                }
                if (Section->u.Flags.Commit) {
                    ((PSECTION_BASIC_INFORMATION)SectionInformation)->AllocationAttributes |=
                                                        SEC_COMMIT;
                }
                if (Section->Segment->ControlArea->u.Flags.GlobalMemory) {
                    ((PSECTION_BASIC_INFORMATION)SectionInformation)->AllocationAttributes |=
                                                        SEC_GLOBAL;
                }

                if (ARGUMENT_PRESENT(ReturnLength)) {
                    *ReturnLength = sizeof(SECTION_BASIC_INFORMATION);
                }

            } else {

                if (Section->u.Flags.Image == 0) {
                    Status = STATUS_SECTION_NOT_IMAGE;
                }
                else {
                    *((PSECTION_IMAGE_INFORMATION)SectionInformation) =
                        *Section->Segment->ImageInformation;
    
                    if (ARGUMENT_PRESENT(ReturnLength)) {
                        *ReturnLength = sizeof(SECTION_IMAGE_INFORMATION);
                    }
                }
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {

        }

        ObDereferenceObject ((PVOID)Section);
    }
    return Status;
}
