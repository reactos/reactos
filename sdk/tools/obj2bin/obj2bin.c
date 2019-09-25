#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <typedefs.h>
#include <pecoff.h>


#if 0
#ifdef ASSERT
#undef ASSERT
#define ASSERT(x) \
    do { if (!(x)) { fprintf(stderr, "Assertion "#x" failed at %s:%d\n", __FILE__, __LINE__); exit(-1);} } while(0)
#endif
#endif


#ifndef max
#define max(a, b)   (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b)   (((a) < (b)) ? (a) : (b))
#endif


typedef unsigned __int64 ULONGLONG, *PULONGLONG;

typedef union _IMAGE_OPTIONAL_HEADER_PTR_3264
{
    PIMAGE_OPTIONAL_HEADER32 p32;
    PIMAGE_OPTIONAL_HEADER64 p64;
    PVOID pHdr;
} IMAGE_OPTIONAL_HEADER_PTR_3264, *PIMAGE_OPTIONAL_HEADER_PTR_3264;

#ifdef _PPC_
#define SWAPD(x) ((((x)&0xff)<<24)|(((x)&0xff00)<<8)|(((x)>>8)&0xff00)|(((x)>>24)&0xff))
#define SWAPW(x) ((((x)&0xff)<<8)|(((x)>>8)&0xff))
#define SWAPQ(x) ((SWAPD((x)&0xffffffff) << 32) | (SWAPD((x)>>32)))
#else
#define SWAPD(x) (x)
#define SWAPW(x) (x)
#define SWAPQ(x) (x)
#endif

#define ROUND_DOWN(n, align) \
    (((ULONG_PTR)(n)) & ~((align) - 1l))

#define ROUND_UP(n, align) \
    ROUND_DOWN(((ULONG_PTR)(n)) + (align) - 1, (align))

#define RVA(m, b) ((PVOID)((ULONG_PTR)(b) + (ULONG_PTR)(m)))

#define IMAGE_REL_BASED_ABSOLUTE        0
#define IMAGE_REL_BASED_HIGH            1
#define IMAGE_REL_BASED_LOW             2
#define IMAGE_REL_BASED_HIGHLOW         3
#define IMAGE_REL_BASED_HIGHADJ         4
#define IMAGE_REL_BASED_MIPS_JMPADDR    5
#define IMAGE_REL_BASED_MIPS_JMPADDR16  9
#define IMAGE_REL_BASED_IA64_IMM64      9
#define IMAGE_REL_BASED_DIR64           10

#define IMAGE_FILE_RELOCS_STRIPPED      0x0001
#define IMAGE_FILE_EXECUTABLE_IMAGE     0x0002


/* Relocation for OBJ files only */
static BOOLEAN
ObjRelocateSection(
    IN OUT PVOID pData,
    IN PIMAGE_SECTION_HEADER pSection,
    IN PIMAGE_SYMBOL pSymbols,
    IN ULONG iOffset)
{
    ULONG i, nOffset;
    PVOID pSectionData;
    PIMAGE_RELOCATION pReloc;
    PUSHORT p16;
    PULONG p32;

    /* Calculate pointers to section and relocation table */
    pSectionData = RVA(pData, pSection->PointerToRawData);

    /* Loop all relocations */
    pReloc = (PIMAGE_RELOCATION)RVA(pData, pSection->PointerToRelocations);
    for (i = 0; i < pSection->NumberOfRelocations; ++i, ++pReloc)
    {
        nOffset = pReloc->VirtualAddress - pSection->VirtualAddress;

        if (nOffset > pSection->SizeOfRawData) continue;

        switch (pReloc->Type)
        {
            /* case IMAGE_REL_BASED_ABSOLUTE: */
            case IMAGE_REL_I386_ABSOLUTE:
            // case IMAGE_REL_AMD64_ABSOLUTE:
            case 16: /* gas-type relocation */
                p16 = (PUSHORT)RVA(pSectionData, nOffset);
                *p16 += (USHORT)(pSymbols[pReloc->SymbolTableIndex].Value + iOffset);
                break;

            case IMAGE_REL_I386_REL16:
                p16 = (PUSHORT)RVA(pSectionData, nOffset);
                *p16 += (USHORT)(pSymbols[pReloc->SymbolTableIndex].Value + iOffset);
                break;

            case IMAGE_REL_I386_DIR32:
                p32 = (PULONG)RVA(pSectionData, nOffset);
                *p32 += (ULONG)(pSymbols[pReloc->SymbolTableIndex].Value + iOffset);
                break;

            default:
                fprintf(stderr, "Unknown relocation type %u, address 0x%x\n",
                        pReloc->Type, (ULONG_PTR)pReloc->VirtualAddress);
                return FALSE;
        }
    }

    return TRUE;
}

static BOOLEAN
ProcessOBJFile(
    IN OUT PVOID pData,
    IN PIMAGE_FILE_HEADER pFileHeader,
    IN PCSTR pszSectionName OPTIONAL,
    IN ULONG nBaseAddress,
    IN FILE* pDestFile)
{
    PIMAGE_SECTION_HEADER pSection;
    PIMAGE_SYMBOL pSymbols;
    ULONG i;

    /* Default to '.text' section if none has been specified by the user */
    if (!pszSectionName)
        pszSectionName = ".text";

    /* Calculate table pointers */
    pSymbols = RVA(pData, pFileHeader->PointerToSymbolTable);

    /* Loop all sections */
    pSection = RVA(pFileHeader + 1, pFileHeader->SizeOfOptionalHeader);
    for (i = 0; i < pFileHeader->NumberOfSections; ++i, ++pSection)
    {
        /* Check if this is the '.text' section */
        if ((strcmp((char*)pSection->Name, pszSectionName) == 0) &&
            (pSection->SizeOfRawData != 0))
        {
            if (!ObjRelocateSection(pData,
                                    pSection,
                                    pSymbols,
                                    nBaseAddress))
            {
                fprintf(stderr, "Failed to process relocations for section '%s'\n",
                        (char*)pSection->Name);
                return FALSE;
            }

            /* Write the section to the destination file */
            if (!fwrite(RVA(pData, pSection->PointerToRawData),
                        pSection->SizeOfRawData, 1, pDestFile))
            {
                fprintf(stderr, "Failed to write %u bytes to destination file\n",
                        (unsigned int)pSection->SizeOfRawData);
                return FALSE;
            }

            nBaseAddress += pSection->SizeOfRawData;
        }
    }

    return TRUE;
}


/* Relocation for PE images only */
static PIMAGE_BASE_RELOCATION
LdrProcessRelocationBlockLongLong(
    IN ULONG_PTR Address,
    IN ULONG Count,
    IN PUSHORT TypeOffset,
    IN LONGLONG Delta)
{
    SHORT Offset;
    USHORT Type;
    ULONG i;
    PUSHORT ShortPtr;
    PULONG LongPtr;
    PULONGLONG LongLongPtr;

    for (i = 0; i < Count; ++i, ++TypeOffset)
    {
        Offset = SWAPW(*TypeOffset) & 0xFFF;
        Type = SWAPW(*TypeOffset) >> 12;
        ShortPtr = (PUSHORT)RVA(Address, Offset);
        /*
        * Don't relocate within the relocation section itself.
        * GCC/LD generates sometimes relocation records for the relocation section.
        * This is a bug in GCC/LD.
        * Fix for it disabled, since it was only in ntoskrnl and not in ntdll
        */
        /*
        if ((ULONG_PTR)ShortPtr < (ULONG_PTR)RelocationDir ||
            (ULONG_PTR)ShortPtr >= (ULONG_PTR)RelocationEnd)
        {*/
        switch (Type)
        {
            /* case IMAGE_REL_BASED_SECTION: */
            /* case IMAGE_REL_BASED_REL32: */
        case IMAGE_REL_BASED_ABSOLUTE:
            break;

        case IMAGE_REL_BASED_HIGH:
            *ShortPtr = HIWORD(MAKELONG(0, *ShortPtr) + (Delta & 0xFFFFFFFF));
            break;

        case IMAGE_REL_BASED_LOW:
            *ShortPtr = SWAPW(*ShortPtr) + LOWORD(Delta & 0xFFFF);
            break;

        case IMAGE_REL_BASED_HIGHLOW:
            LongPtr = (PULONG)RVA(Address, Offset);
            *LongPtr = SWAPD(*LongPtr) + (Delta & 0xFFFFFFFF);
            break;

        case IMAGE_REL_BASED_DIR64:
            LongLongPtr = (/*PUINT64*/PULONGLONG)RVA(Address, Offset);
            *LongLongPtr = SWAPQ(*LongLongPtr) + Delta;
            break;

        case IMAGE_REL_BASED_HIGHADJ:
        case IMAGE_REL_BASED_MIPS_JMPADDR:
        default:
            fprintf(stderr,
                    "Unknown/unsupported fixup type %hu.\n"
                    "Address %p, Current %u, Count %u, *TypeOffset %x\n",
                    Type, (PVOID)Address, i, Count, SWAPW(*TypeOffset));
            return NULL;
        }
    }

    return (PIMAGE_BASE_RELOCATION)TypeOffset;
}

static ULONG
LdrRelocateImage(
    IN PVOID BaseAddress,
    IN PIMAGE_NT_HEADERS NtHeaders,
    IN LONGLONG AdditionalBias,
    IN ULONG Success,
    IN ULONG Conflict,
    IN ULONG Invalid)
{
    PIMAGE_DATA_DIRECTORY RelocationDDir;
    PIMAGE_BASE_RELOCATION RelocationDir, RelocationEnd;
    ULONG Count;
    ULONG_PTR Address;
    PUSHORT TypeOffset;
    LONGLONG Delta;

    if (SWAPW(NtHeaders->FileHeader.Characteristics) & IMAGE_FILE_RELOCS_STRIPPED)
    {
        return Conflict;
    }

    RelocationDDir = &NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];

    if (SWAPD(RelocationDDir->VirtualAddress) == 0 || SWAPD(RelocationDDir->Size) == 0)
    {
        return Success;
    }

    Delta = (ULONG_PTR)BaseAddress - SWAPD(NtHeaders->OptionalHeader.ImageBase) + AdditionalBias;
    RelocationDir = (PIMAGE_BASE_RELOCATION)((ULONG_PTR)BaseAddress + SWAPD(RelocationDDir->VirtualAddress));
    RelocationEnd = (PIMAGE_BASE_RELOCATION)((ULONG_PTR)RelocationDir + SWAPD(RelocationDDir->Size));

    while (RelocationDir < RelocationEnd &&
           SWAPW(RelocationDir->SizeOfBlock) > 0)
    {
        Count = (SWAPW(RelocationDir->SizeOfBlock) - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(USHORT);
        Address = (ULONG_PTR)RVA(BaseAddress, SWAPD(RelocationDir->VirtualAddress));
        TypeOffset = (PUSHORT)(RelocationDir + 1);

        RelocationDir = LdrProcessRelocationBlockLongLong(Address,
                                                          Count,
                                                          TypeOffset,
                                                          Delta);
        if (RelocationDir == NULL)
        {
            fprintf(stderr, "Error during call to LdrProcessRelocationBlockLongLong()!\n");
            return Invalid;
        }
    }

    return Success;
}

static BOOLEAN
ParsePEImage(
    IN PVOID pData,
    IN ULONG nFileSize,
    OUT PIMAGE_NT_HEADERS* pNtHeaders,
    OUT PIMAGE_FILE_HEADER* pFileHeader)
{
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pData;
    IMAGE_OPTIONAL_HEADER_PTR_3264 OptHeader;
    ULONG NtHeaderOffset;
    ULONG TotalHeadersSize = 0;

    /* Ensure it's a PE image */
    if (!(nFileSize >= sizeof(IMAGE_DOS_HEADER) && pDosHeader->e_magic == IMAGE_DOS_SIGNATURE))
    {
        /* Fail */
        fprintf(stderr, "Not a valid PE image!\n");
        return FALSE;
    }

    /* Get the offset to the NT headers */
    NtHeaderOffset = pDosHeader->e_lfanew;

    /* Make sure the file header fits into the size */
    TotalHeadersSize += NtHeaderOffset +
                        FIELD_OFFSET(IMAGE_NT_HEADERS, FileHeader) + (sizeof(((IMAGE_NT_HEADERS *)0)->FileHeader));
    if (TotalHeadersSize >= nFileSize)
    {
        /* Fail */
        fprintf(stderr, "NT headers beyond image size!\n");
        return FALSE;
    }

    /* Now get a pointer to the NT Headers */
    *pNtHeaders = (PIMAGE_NT_HEADERS)RVA(pData, NtHeaderOffset);

    /* Verify the PE Signature */
    if ((*pNtHeaders)->Signature != IMAGE_NT_SIGNATURE)
    {
        /* Fail */
        fprintf(stderr, "Invalid image NT signature!\n");
        return FALSE;
    }

    /* Ensure this is an executable image */
    if (((*pNtHeaders)->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) == 0)
    {
        /* Fail */
        fprintf(stderr, "Invalid executable image!\n");
        return FALSE;
    }

    /* Get the COFF header */
    *pFileHeader = &(*pNtHeaders)->FileHeader;

    /* Check for the presence of the optional header */
    if ((*pFileHeader)->SizeOfOptionalHeader == 0)
    {
        /* Fail */
        fprintf(stderr, "Unsupported PE image (no optional header)!\n");
        return FALSE;
    }

    /* Make sure the optional file header fits into the size */
    TotalHeadersSize += (*pFileHeader)->SizeOfOptionalHeader;
    if (TotalHeadersSize >= nFileSize)
    {
        /* Fail */
        fprintf(stderr, "NT optional header beyond image size!\n");
        return FALSE;
    }

    /* Retrieve the optional header and be sure that its size corresponds to its signature */
    OptHeader.pHdr = (PVOID)(*pFileHeader + 1);
    if (!((*pFileHeader)->SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER32) &&
          OptHeader.p32->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) &&
        !((*pFileHeader)->SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER64) &&
          OptHeader.p64->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) )
    {
        /* Fail */
        fprintf(stderr, "Invalid or unrecognized NT optional header!\n");
        return FALSE;
    }

    return TRUE;
}

static BOOLEAN
ProcessPEImage(
    IN OUT PVOID pData,
    IN PIMAGE_NT_HEADERS pNtHeaders,
    IN PIMAGE_FILE_HEADER pFileHeader,
    IN PCSTR pszSectionName OPTIONAL,
    IN ULONG nBaseAddress,
    IN FILE* pDestFile)
{
    PVOID pImage;
    IMAGE_OPTIONAL_HEADER_PTR_3264 OptHeader;
    PIMAGE_SECTION_HEADER pSection;
    ULONG SizeOfImage, SizeOfHeaders, SectionAlignment;
    ULONG SectionSize, RawSize;
    ULONG FirstSectionVA, EndOfImage;
    ULONG i;
    BOOLEAN Success = FALSE; // Assume failure.

    /* Be sure the user didn't pass inconsistent information */
    ASSERT(&pNtHeaders->FileHeader == pFileHeader);

    /* Retrieve the optional header, its validity has been checked in ParsePEImage() */
    OptHeader.pHdr = (PVOID)(pFileHeader + 1);

    /* Find the actual size of the image in memory, and the size of the headers in the file */
    if (OptHeader.p32->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        SizeOfImage = OptHeader.p32->SizeOfImage;
        SizeOfHeaders = OptHeader.p32->SizeOfHeaders;
        SectionAlignment = OptHeader.p32->SectionAlignment;
    }
    else if (OptHeader.p64->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        SizeOfImage = OptHeader.p64->SizeOfImage;
        SizeOfHeaders = OptHeader.p64->SizeOfHeaders;
        SectionAlignment = OptHeader.p64->SectionAlignment;
    }
    else
    {
        ASSERT(FALSE);
    }

    /*
     * If the user specified a section, search for it,
     * and bail out early if it doesn't exist.
     */
    if (pszSectionName)
    {
        pSection = RVA(pFileHeader + 1, pFileHeader->SizeOfOptionalHeader);
        for (i = 0; i < pFileHeader->NumberOfSections; ++i, ++pSection)
        {
            if (strcmp((char*)pSection->Name, pszSectionName) == 0)
                break; /* Found it */
        }
        if (i >= pFileHeader->NumberOfSections)
        {
            fprintf(stderr, "Section '%s' not found in the PE image.\n", pszSectionName);
            return FALSE;
        }
    }

    /* If there are actually no sections in this image (strange...) just return early */
    if (pFileHeader->NumberOfSections == 0)
    {
        fprintf(stderr, "This PE image does not have any sections!\n");
        return TRUE;
    }

    /* Allocate memory for the binary memory-mapped image */
    pImage = malloc(SizeOfImage);
    if (!pImage)
    {
        fprintf(stderr, "Failed to allocate %lu bytes\n", SizeOfImage);
        return FALSE;
    }

    /* Iterate through the sections and load them. Find also which section actually starts first. */
    pSection = RVA(pFileHeader + 1, pFileHeader->SizeOfOptionalHeader);
    FirstSectionVA = ULONG_MAX;
    EndOfImage = 0;
    for (i = 0; i < pFileHeader->NumberOfSections; ++i, ++pSection)
    {
        //
        // TODO: Skip the .reloc and .debug sections.
        // Partially done later in this loop.
        //

        /* Make sure that the section fits within the image */
        if ((pSection->VirtualAddress > SizeOfImage) ||
            (RVA(pImage, pSection->VirtualAddress) < pImage))
        {
            fprintf(stderr, "Section '%s' outside of the image.\n", (char*)pSection->Name);
            goto Quit;
        }

        /* Get the section virtual size and the raw size */
        SectionSize = pSection->Misc.VirtualSize;
        RawSize = pSection->SizeOfRawData;

        /* Handle a case when VirtualSize equals 0 */
        if (SectionSize == 0)
            SectionSize = RawSize;

        /* If PointerToRawData is 0, then force its size to be also 0 */
        if (pSection->PointerToRawData == 0)
            RawSize = 0;
        /* Truncate the loaded size to the VirtualSize extents */
        else if (RawSize > SectionSize)
            RawSize = SectionSize;

        //
        // Partial TODO (see at the beginning of the loop):
        // Still reserve the size of the .reloc section, but it will really be
        // skipped only if it is at either the beginning or the end of the image.
        //
        if (strcmp((char*)pSection->Name, ".reloc") == 0)
        {
            /* Keep its contents for now, but don't count it in the evaluation of the limits */
        }
        else
        {
            /* Find the start and end of what we should copy later */
            if (!pszSectionName)
            {
                /* Start at the very first section and end at the very last one */
                FirstSectionVA = min(FirstSectionVA, pSection->VirtualAddress);
                EndOfImage = max(EndOfImage, pSection->VirtualAddress + SectionSize);
            }
            else if (strcmp((char*)pSection->Name, pszSectionName) == 0)
            {
                /* Start and end only at the boundaries of this section */
                FirstSectionVA = pSection->VirtualAddress;
                EndOfImage = pSection->VirtualAddress + SectionSize;
            }
            EndOfImage = ROUND_UP(EndOfImage, SectionAlignment);
        }

        /* Actually read the section (if its size is not 0) */
        if (RawSize != 0)
        {
            /* Read this section from the file, size = SizeOfRawData */
            RtlCopyMemory(RVA(pImage, pSection->VirtualAddress),
                          RVA(pData, pSection->PointerToRawData),
                          RawSize);
        }

        /* Size of data is less than the virtual size - fill up the remainder with zeroes */
        if (RawSize < SectionSize)
        {
            RtlZeroMemory(RVA(pImage, pSection->VirtualAddress + RawSize),
                          SectionSize - RawSize);
        }
    }
    if ((FirstSectionVA == ULONG_MAX) || (EndOfImage == 0))
    {
        fprintf(stderr, "Failed to find any valid section?\n");
        goto Quit;
    }


    /* Relocate the image */
    if (!(BOOLEAN)LdrRelocateImage(pImage,
                                   pNtHeaders,
                                   (ULONG_PTR)nBaseAddress
                                        - (ULONG_PTR)RVA(pImage, FirstSectionVA),
                                   TRUE,
                                   TRUE, /* In case of conflict still return success */
                                   FALSE))
    {
        fprintf(stderr, "Failed to relocate image!\n");
        goto Quit;
    }

    /*
     * Write the selected section, or all of them, to the destination file.
     * We therefore exclude all PE headers as wanted (we create a flat "PE" binary).
     */
    SizeOfImage = EndOfImage - FirstSectionVA;
    if (!fwrite(RVA(pImage, FirstSectionVA), SizeOfImage, 1, pDestFile))
    {
        fprintf(stderr, "Failed to write %u bytes to destination file\n", SizeOfImage);
        goto Quit;
    }

    Success = TRUE;

Quit:
    free(pImage);
    return Success;
}


static void
Usage(void)
{
    printf("Converts a COFF object file or a relocatable PE image into a RAW binary file.\n"
           "Syntax: obj2bin <source file> <dest file> <base address> [section name]\n");
}

int main(int argc, char *argv[])
{
    int nErrorCode = 0;
    char *pszSourceFile;
    char *pszDestFile;
    char *pszSectionName;
    unsigned long nFileSize, nBaseAddress;
    FILE *pSourceFile, *pDestFile;
    PIMAGE_DOS_HEADER pDosHeader;
    PIMAGE_NT_HEADERS pNtHeaders;
    PIMAGE_FILE_HEADER pFileHeader;
    char *pData;

    if (!(argc == 4 || argc == 5) || (strcmp(argv[1], "--help") == 0))
    {
        Usage();
        return -1;
    }

    pszSourceFile = argv[1];
    pszDestFile = argv[2];
    nBaseAddress = strtol(argv[3], 0, 16);
    pszSectionName = ((argc == 5) ? argv[4] : NULL);

    pSourceFile = fopen(pszSourceFile, "rb");
    if (!pSourceFile)
    {
        fprintf(stderr, "Couldn't open source file '%s'\n", pszSourceFile);
        return -2;
    }

    /* Get file size */
    fseek(pSourceFile, 0, SEEK_END);
    nFileSize = ftell(pSourceFile);
    rewind(pSourceFile);

    /* Allocate memory for the file */
    pData = malloc(nFileSize);
    if (!pData)
    {
        fclose(pSourceFile);
        fprintf(stderr, "Failed to allocate %lu bytes\n", nFileSize);
        return -3;
    }

    /* Read the whole source file */
    if (!fread(pData, nFileSize, 1, pSourceFile))
    {
        free(pData);
        fclose(pSourceFile);
        fprintf(stderr, "Failed to read %lu bytes from source file\n", nFileSize);
        return -4;
    }

    /* Close the source file */
    fclose(pSourceFile);

    /* Open the destination file */
    pDestFile = fopen(pszDestFile, "wb");
    if (!pDestFile)
    {
        free(pData);
        fprintf(stderr, "Couldn't open destination file '%s'\n", pszDestFile);
        return -5;
    }

    /* Check whether this is a pure COFF file or a PE image */
    pDosHeader = (PIMAGE_DOS_HEADER)pData;
    if (nFileSize >= sizeof(IMAGE_DOS_HEADER) && pDosHeader->e_magic == IMAGE_DOS_SIGNATURE)
    {
        /* This is a PE image, parse it */
        if (!ParsePEImage(pData, nFileSize, &pNtHeaders, &pFileHeader))
        {
            fprintf(stderr, "ParsePEImage() failed.\n");
            nErrorCode = -6;
            goto Quit;
        }
    }
    else if (nFileSize >= sizeof(IMAGE_FILE_HEADER) /* == IMAGE_SIZEOF_FILE_HEADER */)
    {
        /* Get the COFF header */
        pNtHeaders = NULL;
        pFileHeader = (PIMAGE_FILE_HEADER)pData;
    }
    else
    {
        fprintf(stderr, "Unrecognized format!\n");
        nErrorCode = -6;
        goto Quit;
    }

    if (// pFileHeader->Machine != IMAGE_FILE_MACHINE_UNKNOWN &&
        pFileHeader->Machine != IMAGE_FILE_MACHINE_I386    &&
        pFileHeader->Machine != IMAGE_FILE_MACHINE_AMD64)
    {
        fprintf(stderr, "Unsupported machine type 0x%04x!\n", pFileHeader->Machine);
        nErrorCode = -7;
        goto Quit;
    }

    if (pNtHeaders == NULL)
    {
        /* OBJ file - Default to '.text' section if none has been specified by the user */
        if (!ProcessOBJFile(pData,
                            pFileHeader,
                            (pszSectionName ? pszSectionName : ".text"),
                            nBaseAddress,
                            pDestFile))
        {
            fprintf(stderr, "ProcessOBJFile() failed.\n");
            nErrorCode = -8;
            goto Quit;
        }
    }
    else
    {
        /* PE image - Either extract only the section specified by the user, or all of them */
        if (!ProcessPEImage(pData,
                            pNtHeaders,
                            pFileHeader,
                            pszSectionName,
                            nBaseAddress,
                            pDestFile))
        {
            fprintf(stderr, "ProcessPEImage() failed.\n");
            nErrorCode = -8;
            goto Quit;
        }
    }

Quit:
    fclose(pDestFile);
    free(pData);

    return nErrorCode;
}
