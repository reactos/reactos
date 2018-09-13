/*++

Copyright (c) 1991-1995 Microsoft Corporation

Module Name:

    imagedir.c

Abstract:

    The module contains the code to translate an image directory type to
    the address of the data for that entry.

Environment:

    User Mode or Kernel Mode

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <private.h>

PVOID
ImageDirectoryEntryToData (
    IN PVOID Base,
    IN BOOLEAN MappedAsImage,
    IN USHORT DirectoryEntry,
    OUT PULONG Size
    );

PIMAGE_NT_HEADERS
ImageNtHeader (
    IN PVOID Base
    )

/*++

Routine Description:

    This function returns the address of the NT Header.

Arguments:

    Base - Supplies the base of the image.

Return Value:

    Returns the address of the NT Header.

--*/

{
    return RtlpImageNtHeader( Base );
}


PVOID
ImageDirectoryEntryToDataRom (
    IN PVOID Base,
    IN BOOLEAN MappedAsImage,
    IN USHORT DirectoryEntry,
    OUT PULONG Size,
    OUT PIMAGE_SECTION_HEADER *FoundSection OPTIONAL,
    IN PIMAGE_FILE_HEADER FileHeader,
    IN PIMAGE_ROM_OPTIONAL_HEADER OptionalHeader
    )
{
    ULONG i;
    PIMAGE_SECTION_HEADER NtSection;

    //
    // There's not much we can get from ROM images.  See if the info requested
    // is one of the known ones (debug/exception data)
    //

    NtSection = (PIMAGE_SECTION_HEADER)((ULONG_PTR)OptionalHeader +
                      FileHeader->SizeOfOptionalHeader);

    for (i = 0; i < FileHeader->NumberOfSections; i++, NtSection++) {

        if ( DirectoryEntry == IMAGE_DIRECTORY_ENTRY_DEBUG ) {
            if (!_stricmp((char *)NtSection->Name, ".rdata")) {
                PIMAGE_DEBUG_DIRECTORY DebugDirectory;
                *Size = 0;
                DebugDirectory = (PVOID)((ULONG_PTR)NtSection->PointerToRawData + (ULONG_PTR)Base);
                while (DebugDirectory->Type != 0) {
                    *Size += sizeof(IMAGE_DEBUG_DIRECTORY);
                    DebugDirectory++;
                }
                if (FoundSection) {
                    *FoundSection = NtSection;
                }
                return (PVOID)((ULONG_PTR)NtSection->PointerToRawData + (ULONG_PTR)Base);
            }
        } else
        if ( DirectoryEntry == IMAGE_DIRECTORY_ENTRY_EXCEPTION ) {
            if (!_stricmp((char *)NtSection->Name, ".pdata")) {
                if (FoundSection) {
                    *FoundSection = NtSection;
                }
                return (PVOID)((ULONG_PTR)NtSection->PointerToRawData + (ULONG_PTR)Base);
            }
        }
    }
    // Not one of the known sections.  Return error.
    *Size = 0;
    return( NULL );
}

PVOID
ImageDirectoryEntryToData64 (
    IN PVOID Base,
    IN BOOLEAN MappedAsImage,
    IN USHORT DirectoryEntry,
    OUT PULONG Size,
    OUT PIMAGE_SECTION_HEADER *FoundSection OPTIONAL,
    IN PIMAGE_FILE_HEADER FileHeader,
    IN PIMAGE_OPTIONAL_HEADER64 OptionalHeader
    )
{
    ULONG i;
    PIMAGE_SECTION_HEADER NtSection;
    ULONG DirectoryAddress;

    if (DirectoryEntry >= OptionalHeader->NumberOfRvaAndSizes) {
        *Size = 0;
        return( NULL );
    }

    if (!(DirectoryAddress = OptionalHeader->DataDirectory[ DirectoryEntry ].VirtualAddress)) {
        *Size = 0;
        return( NULL );
    }
    *Size = OptionalHeader->DataDirectory[ DirectoryEntry ].Size;
    if (MappedAsImage || DirectoryAddress < OptionalHeader->SizeOfHeaders) {
        if (FoundSection) {
            *FoundSection = NULL;
        }
        return( (PVOID)((ULONG_PTR)Base + DirectoryAddress) );
    }

    NtSection = (PIMAGE_SECTION_HEADER)((ULONG_PTR)OptionalHeader +
                        FileHeader->SizeOfOptionalHeader);

    for (i=0; i<FileHeader->NumberOfSections; i++) {
        if (DirectoryAddress >= NtSection->VirtualAddress &&
           DirectoryAddress < NtSection->VirtualAddress + NtSection->SizeOfRawData) {
            if (FoundSection) {
                *FoundSection = NtSection;
            }
            return( (PVOID)((ULONG_PTR)Base + (DirectoryAddress - NtSection->VirtualAddress) + NtSection->PointerToRawData) );
        }
        ++NtSection;
    }
    return( NULL );
}

PVOID
ImageDirectoryEntryToData32 (
    IN PVOID Base,
    IN BOOLEAN MappedAsImage,
    IN USHORT DirectoryEntry,
    OUT PULONG Size,
    OUT PIMAGE_SECTION_HEADER *FoundSection OPTIONAL,
    IN PIMAGE_FILE_HEADER FileHeader,
    IN PIMAGE_OPTIONAL_HEADER32 OptionalHeader
    )
{
    ULONG i;
    PIMAGE_SECTION_HEADER NtSection;
    ULONG DirectoryAddress;

    if (DirectoryEntry >= OptionalHeader->NumberOfRvaAndSizes) {
        *Size = 0;
        return( NULL );
    }

    if (!(DirectoryAddress = OptionalHeader->DataDirectory[ DirectoryEntry ].VirtualAddress)) {
        *Size = 0;
        return( NULL );
    }
    *Size = OptionalHeader->DataDirectory[ DirectoryEntry ].Size;
    if (MappedAsImage || DirectoryAddress < OptionalHeader->SizeOfHeaders) {
        if (FoundSection) {
            *FoundSection = NULL;
        }
        return( (PVOID)((ULONG_PTR)Base + DirectoryAddress) );
    }

    NtSection = (PIMAGE_SECTION_HEADER)((ULONG_PTR)OptionalHeader +
                        FileHeader->SizeOfOptionalHeader);

    for (i=0; i<FileHeader->NumberOfSections; i++) {
        if (DirectoryAddress >= NtSection->VirtualAddress &&
           DirectoryAddress < NtSection->VirtualAddress + NtSection->SizeOfRawData) {
            if (FoundSection) {
                *FoundSection = NtSection;
            }
            return( (PVOID)((ULONG_PTR)Base + (DirectoryAddress - NtSection->VirtualAddress) + NtSection->PointerToRawData) );
        }
        ++NtSection;
    }
    return( NULL );
}

PVOID
ImageDirectoryEntryToDataEx (
    IN PVOID Base,
    IN BOOLEAN MappedAsImage,
    IN USHORT DirectoryEntry,
    OUT PULONG Size,
    OUT PIMAGE_SECTION_HEADER *FoundSection OPTIONAL
    )

/*++

Routine Description:

    This function locates a Directory Entry within the image header
    and returns either the virtual address or seek address of the
    data the Directory describes.  It may optionally return the
    section header, if any, for the found data.

Arguments:

    Base - Supplies the base of the image or data file.

    MappedAsImage - FALSE if the file is mapped as a data file.
                  - TRUE if the file is mapped as an image.

    DirectoryEntry - Supplies the directory entry to locate.

    Size - Return the size of the directory.

    FoundSection - Returns the section header, if any, for the data

Return Value:

    NULL - The file does not contain data for the specified directory entry.

    NON-NULL - Returns the address of the raw data the directory describes.

--*/

{
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_FILE_HEADER FileHeader;
    PIMAGE_OPTIONAL_HEADER OptionalHeader;

    if ((ULONG_PTR)Base & 0x00000001) {
        Base = (PVOID)((ULONG_PTR)Base & ~0x1);
        MappedAsImage = FALSE;
        }

    NtHeader = ImageNtHeader(Base);

    if (NtHeader) {
        FileHeader = &NtHeader->FileHeader;
        OptionalHeader = &NtHeader->OptionalHeader;
    } else {
        // Handle case where Image passed in doesn't have a dos stub (ROM images for instance);
        FileHeader = (PIMAGE_FILE_HEADER)Base;
        OptionalHeader = (PIMAGE_OPTIONAL_HEADER) ((ULONG_PTR)Base + IMAGE_SIZEOF_FILE_HEADER);
    }

    if (OptionalHeader->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        return (ImageDirectoryEntryToData32 ( Base,
                                              MappedAsImage,
                                              DirectoryEntry,
                                              Size,
                                              FoundSection,
                                              FileHeader,
                                              (PIMAGE_OPTIONAL_HEADER32)OptionalHeader));
    } else if (OptionalHeader->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        return (ImageDirectoryEntryToData64 ( Base,
                                               MappedAsImage,
                                               DirectoryEntry,
                                               Size,
                                               FoundSection,
                                               FileHeader,
                                               (PIMAGE_OPTIONAL_HEADER64)OptionalHeader));
    } else if (OptionalHeader->Magic == IMAGE_ROM_OPTIONAL_HDR_MAGIC) {
        return (ImageDirectoryEntryToDataRom ( Base,
                                               MappedAsImage,
                                               DirectoryEntry,
                                               Size,
                                               FoundSection,
                                               FileHeader,
                                               (PIMAGE_ROM_OPTIONAL_HEADER)OptionalHeader));
    } else
        return NULL;
}


PVOID
ImageDirectoryEntryToData (
    IN PVOID Base,
    IN BOOLEAN MappedAsImage,
    IN USHORT DirectoryEntry,
    OUT PULONG Size
    )

/*++

Routine Description:

    This function locates a Directory Entry within the image header
    and returns either the virtual address or seek address of the
    data the Directory describes.

    This just calls ImageDirectoryToDataEx without a FoundSection arg.

Arguments:

    Base - Supplies the base of the image or data file.

    MappedAsImage - FALSE if the file is mapped as a data file.
                  - TRUE if the file is mapped as an image.

    DirectoryEntry - Supplies the directory entry to locate.

    Size - Return the size of the directory.

Return Value:

    NULL - The file does not contain data for the specified directory entry.

    NON-NULL - Returns the address of the raw data the directory describes.

--*/

{
    return ImageDirectoryEntryToDataEx(Base, MappedAsImage, DirectoryEntry, Size, NULL);
}


PIMAGE_SECTION_HEADER
ImageRvaToSection(
    IN PIMAGE_NT_HEADERS NtHeaders,
    IN PVOID Base,
    IN ULONG Rva
    )

/*++

Routine Description:

    This function locates an RVA within the image header of a file
    that is mapped as a file and returns a pointer to the section
    table entry for that virtual address

Arguments:

    NtHeaders - Supplies the pointer to the image or data file.

    Base - Supplies the base of the image or data file.

    Rva - Supplies the relative virtual address (RVA) to locate.

Return Value:

    NULL - The file does not contain data for the specified directory entry.

    NON-NULL - Returns the pointer of the section entry containing the data.

--*/

{
    ULONG i;
    PIMAGE_SECTION_HEADER NtSection;

    NtSection = IMAGE_FIRST_SECTION( NtHeaders );
    for (i=0; i<NtHeaders->FileHeader.NumberOfSections; i++) {
        if (Rva >= NtSection->VirtualAddress &&
            Rva < NtSection->VirtualAddress + NtSection->SizeOfRawData
           ) {
            return NtSection;
            }
        ++NtSection;
        }

    return NULL;
}


PVOID
ImageRvaToVa(
    IN PIMAGE_NT_HEADERS NtHeaders,
    IN PVOID Base,
    IN ULONG Rva,
    IN OUT PIMAGE_SECTION_HEADER *LastRvaSection OPTIONAL
    )

/*++

Routine Description:

    This function locates an RVA within the image header of a file that
    is mapped as a file and returns the virtual addrees of the
    corresponding byte in the file.


Arguments:

    NtHeaders - Supplies the pointer to the image or data file.

    Base - Supplies the base of the image or data file.

    Rva - Supplies the relative virtual address (RVA) to locate.

    LastRvaSection - Optional parameter that if specified, points
        to a variable that contains the last section value used for
        the specified image to translate and RVA to a VA.

Return Value:

    NULL - The file does not contain the specified RVA

    NON-NULL - Returns the virtual addrees in the mapped file.

--*/

{
    PIMAGE_SECTION_HEADER NtSection;

    if (LastRvaSection == NULL ||
        (NtSection = *LastRvaSection) == NULL ||
        NtSection == NULL ||
        Rva < NtSection->VirtualAddress ||
        Rva >= NtSection->VirtualAddress + NtSection->SizeOfRawData
       ) {
        NtSection = ImageRvaToSection( NtHeaders,
                                       Base,
                                       Rva
                                     );
        }

    if (NtSection != NULL) {
        if (LastRvaSection != NULL) {
            *LastRvaSection = NtSection;
            }

        return (PVOID)((ULONG_PTR)Base +
                       (Rva - NtSection->VirtualAddress) +
                       NtSection->PointerToRawData
                      );
        }
    else {
        return NULL;
        }
}
