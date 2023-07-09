/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    imagedir.c

Abstract:

    The module contains the code to translate an image directory type to
    the address of the data for that entry.

Author:

    Steve Wood (stevewo) 18-Aug-1989

Environment:

    User Mode or Kernel Mode

Revision History:

--*/

#include "ntrtlp.h"

PIMAGE_NT_HEADERS
RtlImageNtHeader (
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
#if defined (BLDR_KERNEL_RUNTIME) || defined(NTOS_KERNEL_RUNTIME)
    PIMAGE_NT_HEADERS NtHeaders = NULL;

    if (Base != NULL && Base != (PVOID)-1) {
        if (((PIMAGE_DOS_HEADER)Base)->e_magic == IMAGE_DOS_SIGNATURE) {
            NtHeaders = (PIMAGE_NT_HEADERS)((PCHAR)Base + ((PIMAGE_DOS_HEADER)Base)->e_lfanew);

#if defined(NTOS_KERNEL_RUNTIME)
            if (Base < MM_HIGHEST_USER_ADDRESS) {
                if ((PVOID)NtHeaders >= MM_HIGHEST_USER_ADDRESS) {
                    return NULL;
                }
                if ((PVOID)((PCHAR)NtHeaders + sizeof (IMAGE_NT_HEADERS)) >= MM_HIGHEST_USER_ADDRESS) {
                    return NULL;
                }
            }
#endif

            if (NtHeaders->Signature != IMAGE_NT_SIGNATURE) {
                NtHeaders = NULL;
            }
        }
    }

    return NtHeaders;
#else
    return RtlpImageNtHeader( Base );
#endif
}


PIMAGE_SECTION_HEADER
RtlSectionTableFromVirtualAddress (
    IN PIMAGE_NT_HEADERS NtHeaders,
    IN PVOID Base,
    IN ULONG Address
    )

/*++

Routine Description:

    This function locates a VirtualAddress within the image header
    of a file that is mapped as a file and returns a pointer to the
    section table entry for that virtual address

Arguments:

    NtHeaders - Supplies the pointer to the image or data file.

    Base - Supplies the base of the image or data file.

    Address - Supplies the virtual address to locate.

Return Value:

    NULL - The file does not contain data for the specified directory entry.

    NON-NULL - Returns the pointer of the section entry containing the data.

--*/

{
    ULONG i;
    PIMAGE_SECTION_HEADER NtSection;

    NtSection = IMAGE_FIRST_SECTION( NtHeaders );
    for (i=0; i<NtHeaders->FileHeader.NumberOfSections; i++) {
        if ((ULONG)Address >= NtSection->VirtualAddress &&
            (ULONG)Address < NtSection->VirtualAddress + NtSection->SizeOfRawData
           ) {
            return NtSection;
            }
        ++NtSection;
        }

    return NULL;
}


PVOID
RtlAddressInSectionTable (
    IN PIMAGE_NT_HEADERS NtHeaders,
    IN PVOID Base,
    IN ULONG Address
    )

/*++

Routine Description:

    This function locates a VirtualAddress within the image header
    of a file that is mapped as a file and returns the seek address
    of the data the Directory describes.

Arguments:

    NtHeaders - Supplies the pointer to the image or data file.

    Base - Supplies the base of the image or data file.

    Address - Supplies the virtual address to locate.

Return Value:

    NULL - The file does not contain data for the specified directory entry.

    NON-NULL - Returns the address of the raw data the directory describes.

--*/

{
    PIMAGE_SECTION_HEADER NtSection;

    NtSection = RtlSectionTableFromVirtualAddress( NtHeaders,
                                                   Base,
                                                   Address
                                                 );
    if (NtSection != NULL) {
        return( ((PCHAR)Base + ((ULONG_PTR)Address - NtSection->VirtualAddress) + NtSection->PointerToRawData) );
        }
    else {
        return( NULL );
        }
}


PVOID
RtlpImageDirectoryEntryToData32 (
    IN PVOID Base,
    IN BOOLEAN MappedAsImage,
    IN USHORT DirectoryEntry,
    OUT PULONG Size,
    PIMAGE_NT_HEADERS32 NtHeaders
    )
{
    ULONG DirectoryAddress;

    if (DirectoryEntry >= NtHeaders->OptionalHeader.NumberOfRvaAndSizes) {
        return( NULL );
    }

    if (!(DirectoryAddress = NtHeaders->OptionalHeader.DataDirectory[ DirectoryEntry ].VirtualAddress)) {
        return( NULL );
    }

#if defined(NTOS_KERNEL_RUNTIME)
    if (Base < MM_HIGHEST_USER_ADDRESS) {
        if ((PVOID)((PCHAR)Base + DirectoryAddress) >= MM_HIGHEST_USER_ADDRESS) {
            return( NULL );
        }
    }
#endif

    *Size = NtHeaders->OptionalHeader.DataDirectory[ DirectoryEntry ].Size;
    if (MappedAsImage || DirectoryAddress < NtHeaders->OptionalHeader.SizeOfHeaders) {
        return( (PVOID)((PCHAR)Base + DirectoryAddress) );
    }

    return( RtlAddressInSectionTable((PIMAGE_NT_HEADERS)NtHeaders, Base, DirectoryAddress ));
}


PVOID
RtlpImageDirectoryEntryToData64 (
    IN PVOID Base,
    IN BOOLEAN MappedAsImage,
    IN USHORT DirectoryEntry,
    OUT PULONG Size,
    PIMAGE_NT_HEADERS64 NtHeaders
    )
{
    ULONG DirectoryAddress;

    if (DirectoryEntry >= NtHeaders->OptionalHeader.NumberOfRvaAndSizes) {
        return( NULL );
    }

    if (!(DirectoryAddress = NtHeaders->OptionalHeader.DataDirectory[ DirectoryEntry ].VirtualAddress)) {
        return( NULL );
    }

#if defined(NTOS_KERNEL_RUNTIME)
    if (Base < MM_HIGHEST_USER_ADDRESS) {
        if ((PVOID)((PCHAR)Base + DirectoryAddress) >= MM_HIGHEST_USER_ADDRESS) {
            return( NULL );
        }
    }
#endif

    *Size = NtHeaders->OptionalHeader.DataDirectory[ DirectoryEntry ].Size;
    if (MappedAsImage || DirectoryAddress < NtHeaders->OptionalHeader.SizeOfHeaders) {
        return( (PVOID)((PCHAR)Base + DirectoryAddress) );
    }

    return( RtlAddressInSectionTable((PIMAGE_NT_HEADERS)NtHeaders, Base, DirectoryAddress ));
}


PVOID
RtlImageDirectoryEntryToData (
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
    PIMAGE_NT_HEADERS NtHeaders;

    if ((ULONG_PTR)Base & 0x00000001) {
        Base = (PVOID)((ULONG_PTR)Base & ~0x00000001);
        MappedAsImage = FALSE;
        }

    NtHeaders = RtlImageNtHeader(Base);

    if (!NtHeaders)
        return NULL;

    if (NtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        return (RtlpImageDirectoryEntryToData32(Base,
                                                MappedAsImage,
                                                DirectoryEntry,
                                                Size,
                                                (PIMAGE_NT_HEADERS32)NtHeaders));
    } else if (NtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        return (RtlpImageDirectoryEntryToData64(Base,
                                                MappedAsImage,
                                                DirectoryEntry,
                                                Size,
                                                (PIMAGE_NT_HEADERS64)NtHeaders));
    } else {
        return (NULL);
    }
}


#if !defined(NTOS_KERNEL_RUNTIME) && !defined(BLDR_KERNEL_RUNTIME)

PIMAGE_SECTION_HEADER
RtlImageRvaToSection(
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

    Base - Supplies the base of the image or data file.  The image
        was mapped as a data file.

    Rva - Supplies the relative virtual address (RVA) to locate.

Return Value:

    NULL - The RVA was not found within any of the sections of the image.

    NON-NULL - Returns the pointer to the image section that contains
               the RVA

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
RtlImageRvaToVa(
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

    Base - Supplies the base of the image or data file.  The image
        was mapped as a data file.

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

    if (!ARGUMENT_PRESENT( LastRvaSection ) ||
        (NtSection = *LastRvaSection) == NULL ||
        Rva < NtSection->VirtualAddress ||
        Rva >= NtSection->VirtualAddress + NtSection->SizeOfRawData
       ) {
        NtSection = RtlImageRvaToSection( NtHeaders,
                                          Base,
                                          Rva
                                        );
        }

    if (NtSection != NULL) {
        if (LastRvaSection != NULL) {
            *LastRvaSection = NtSection;
            }

        return (PVOID)((PCHAR)Base +
                       (Rva - NtSection->VirtualAddress) +
                       NtSection->PointerToRawData
                      );
        }
    else {
        return NULL;
        }
}

#endif
