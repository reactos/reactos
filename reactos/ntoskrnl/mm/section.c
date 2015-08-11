/*
 * Copyright (C) 1998-2005 ReactOS Team (and the authors from the programmers section)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/section.c
 * PURPOSE:         Implements section objects
 *
 * PROGRAMMERS:     Rex Jolliff
 *                  David Welch
 *                  Eric Kohl
 *                  Emanuele Aliberti
 *                  Eugene Ingerman
 *                  Casper Hornstrup
 *                  KJK::Hyperion
 *                  Guido de Jong
 *                  Ge van Geldorp
 *                  Royce Mitchell III
 *                  Filip Navara
 *                  Aleksey Bragin
 *                  Jason Filby
 *                  Thomas Weidenmueller
 *                  Gunnar Andre' Dalsnes
 *                  Mike Nordell
 *                  Alex Ionescu
 *                  Gregor Anich
 *                  Steven Edwards
 *                  Herve Poussineau
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <cache/newcc.h>
#include <cache/section/newmm.h>
#define NDEBUG
#include <debug.h>
#include <reactos/exeformat.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmCreatePhysicalMemorySection)
#pragma alloc_text(INIT, MmInitSectionImplementation)
#endif

#include "ARM3/miarm.h"

#undef MmSetPageEntrySectionSegment
#define MmSetPageEntrySectionSegment(S,O,E) do { \
        DPRINT("SetPageEntrySectionSegment(old,%p,%x,%x)\n",(S),(O)->LowPart,E); \
        _MmSetPageEntrySectionSegment((S),(O),(E),__FILE__,__LINE__);   \
	} while (0)

extern MMSESSION MmSession;

NTSTATUS
NTAPI
MiMapViewInSystemSpace(IN PVOID Section,
                       IN PVOID Session,
                       OUT PVOID *MappedBase,
                       IN OUT PSIZE_T ViewSize);

NTSTATUS
NTAPI
MmCreateArm3Section(OUT PVOID *SectionObject,
                    IN ACCESS_MASK DesiredAccess,
                    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
                    IN PLARGE_INTEGER InputMaximumSize,
                    IN ULONG SectionPageProtection,
                    IN ULONG AllocationAttributes,
                    IN HANDLE FileHandle OPTIONAL,
                    IN PFILE_OBJECT FileObject OPTIONAL);

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
                       IN ULONG Protect);

//
// PeFmtCreateSection depends on the following:
//
C_ASSERT(EXEFMT_LOAD_HEADER_SIZE >= sizeof(IMAGE_DOS_HEADER));
C_ASSERT(sizeof(IMAGE_NT_HEADERS32) <= sizeof(IMAGE_NT_HEADERS64));

C_ASSERT(TYPE_ALIGNMENT(IMAGE_NT_HEADERS32) == TYPE_ALIGNMENT(IMAGE_NT_HEADERS64));
C_ASSERT(RTL_SIZEOF_THROUGH_FIELD(IMAGE_NT_HEADERS32, FileHeader) == RTL_SIZEOF_THROUGH_FIELD(IMAGE_NT_HEADERS64, FileHeader));
C_ASSERT(FIELD_OFFSET(IMAGE_NT_HEADERS32, OptionalHeader) == FIELD_OFFSET(IMAGE_NT_HEADERS64, OptionalHeader));

C_ASSERT(PEFMT_FIELDS_EQUAL(IMAGE_OPTIONAL_HEADER32, IMAGE_OPTIONAL_HEADER64, Magic));
C_ASSERT(PEFMT_FIELDS_EQUAL(IMAGE_OPTIONAL_HEADER32, IMAGE_OPTIONAL_HEADER64, SectionAlignment));
C_ASSERT(PEFMT_FIELDS_EQUAL(IMAGE_OPTIONAL_HEADER32, IMAGE_OPTIONAL_HEADER64, FileAlignment));
C_ASSERT(PEFMT_FIELDS_EQUAL(IMAGE_OPTIONAL_HEADER32, IMAGE_OPTIONAL_HEADER64, Subsystem));
C_ASSERT(PEFMT_FIELDS_EQUAL(IMAGE_OPTIONAL_HEADER32, IMAGE_OPTIONAL_HEADER64, MinorSubsystemVersion));
C_ASSERT(PEFMT_FIELDS_EQUAL(IMAGE_OPTIONAL_HEADER32, IMAGE_OPTIONAL_HEADER64, MajorSubsystemVersion));
C_ASSERT(PEFMT_FIELDS_EQUAL(IMAGE_OPTIONAL_HEADER32, IMAGE_OPTIONAL_HEADER64, AddressOfEntryPoint));
C_ASSERT(PEFMT_FIELDS_EQUAL(IMAGE_OPTIONAL_HEADER32, IMAGE_OPTIONAL_HEADER64, SizeOfCode));
C_ASSERT(PEFMT_FIELDS_EQUAL(IMAGE_OPTIONAL_HEADER32, IMAGE_OPTIONAL_HEADER64, SizeOfHeaders));

/* TYPES *********************************************************************/

typedef struct
{
    PROS_SECTION_OBJECT Section;
    PMM_SECTION_SEGMENT Segment;
    LARGE_INTEGER Offset;
    BOOLEAN WasDirty;
    BOOLEAN Private;
    PEPROCESS CallingProcess;
    ULONG_PTR SectionEntry;
}
MM_SECTION_PAGEOUT_CONTEXT;

/* GLOBALS *******************************************************************/

POBJECT_TYPE MmSectionObjectType = NULL;

ULONG_PTR MmSubsectionBase;

static ULONG SectionCharacteristicsToProtect[16] =
{
    PAGE_NOACCESS,          /* 0 = NONE */
    PAGE_NOACCESS,          /* 1 = SHARED */
    PAGE_EXECUTE,           /* 2 = EXECUTABLE */
    PAGE_EXECUTE,           /* 3 = EXECUTABLE, SHARED */
    PAGE_READONLY,          /* 4 = READABLE */
    PAGE_READONLY,          /* 5 = READABLE, SHARED */
    PAGE_EXECUTE_READ,      /* 6 = READABLE, EXECUTABLE */
    PAGE_EXECUTE_READ,      /* 7 = READABLE, EXECUTABLE, SHARED */
    /*
     * FIXME? do we really need the WriteCopy field in segments? can't we use
     * PAGE_WRITECOPY here?
     */
    PAGE_READWRITE,         /* 8 = WRITABLE */
    PAGE_READWRITE,         /* 9 = WRITABLE, SHARED */
    PAGE_EXECUTE_READWRITE, /* 10 = WRITABLE, EXECUTABLE */
    PAGE_EXECUTE_READWRITE, /* 11 = WRITABLE, EXECUTABLE, SHARED */
    PAGE_READWRITE,         /* 12 = WRITABLE, READABLE */
    PAGE_READWRITE,         /* 13 = WRITABLE, READABLE, SHARED */
    PAGE_EXECUTE_READWRITE, /* 14 = WRITABLE, READABLE, EXECUTABLE */
    PAGE_EXECUTE_READWRITE, /* 15 = WRITABLE, READABLE, EXECUTABLE, SHARED */
};

extern ULONG MmMakeFileAccess [];
ACCESS_MASK NTAPI MiArm3GetCorrectFileAccessMask(IN ACCESS_MASK SectionPageProtection);
static GENERIC_MAPPING MmpSectionMapping =
{
    STANDARD_RIGHTS_READ | SECTION_MAP_READ | SECTION_QUERY,
    STANDARD_RIGHTS_WRITE | SECTION_MAP_WRITE,
    STANDARD_RIGHTS_EXECUTE | SECTION_MAP_EXECUTE,
    SECTION_ALL_ACCESS
};

static const INFORMATION_CLASS_INFO ExSectionInfoClass[] =
{
    ICI_SQ_SAME( sizeof(SECTION_BASIC_INFORMATION), sizeof(ULONG), ICIF_QUERY ), /* SectionBasicInformation */
    ICI_SQ_SAME( sizeof(SECTION_IMAGE_INFORMATION), sizeof(ULONG), ICIF_QUERY ), /* SectionImageInformation */
};

/* FUNCTIONS *****************************************************************/


/*
 References:
  [1] Microsoft Corporation, "Microsoft Portable Executable and Common Object
      File Format Specification", revision 6.0 (February 1999)
*/
NTSTATUS NTAPI PeFmtCreateSection(IN CONST VOID * FileHeader,
                                  IN SIZE_T FileHeaderSize,
                                  IN PVOID File,
                                  OUT PMM_IMAGE_SECTION_OBJECT ImageSectionObject,
                                  OUT PULONG Flags,
                                  IN PEXEFMT_CB_READ_FILE ReadFileCb,
                                  IN PEXEFMT_CB_ALLOCATE_SEGMENTS AllocateSegmentsCb)
{
    NTSTATUS nStatus;
    ULONG cbFileHeaderOffsetSize = 0;
    ULONG cbSectionHeadersOffset = 0;
    ULONG cbSectionHeadersSize;
    ULONG cbSectionHeadersOffsetSize = 0;
    ULONG cbOptHeaderSize;
    ULONG cbHeadersSize = 0;
    ULONG nSectionAlignment;
    ULONG nFileAlignment;
    ULONG_PTR ImageBase;
    const IMAGE_DOS_HEADER * pidhDosHeader;
    const IMAGE_NT_HEADERS32 * pinhNtHeader;
    const IMAGE_OPTIONAL_HEADER32 * piohOptHeader;
    const IMAGE_SECTION_HEADER * pishSectionHeaders;
    PMM_SECTION_SEGMENT pssSegments;
    LARGE_INTEGER lnOffset;
    PVOID pBuffer;
    SIZE_T nPrevVirtualEndOfSegment = 0;
    ULONG nFileSizeOfHeaders = 0;
    ULONG i;

    ASSERT(FileHeader);
    ASSERT(FileHeaderSize > 0);
    ASSERT(File);
    ASSERT(ImageSectionObject);
    ASSERT(ReadFileCb);
    ASSERT(AllocateSegmentsCb);

    ASSERT(Intsafe_CanOffsetPointer(FileHeader, FileHeaderSize));

    ASSERT(((UINT_PTR)FileHeader % TYPE_ALIGNMENT(IMAGE_DOS_HEADER)) == 0);

#define DIE(ARGS_) { DPRINT ARGS_; goto l_Return; }

    pBuffer = NULL;
    pidhDosHeader = FileHeader;

    /* DOS HEADER */
    nStatus = STATUS_ROS_EXEFMT_UNKNOWN_FORMAT;

    /* image too small to be an MZ executable */
    if(FileHeaderSize < sizeof(IMAGE_DOS_HEADER))
        DIE(("Too small to be an MZ executable, size is %lu\n", FileHeaderSize));

    /* no MZ signature */
    if(pidhDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        DIE(("No MZ signature found, e_magic is %hX\n", pidhDosHeader->e_magic));

    /* NT HEADER */
    nStatus = STATUS_INVALID_IMAGE_PROTECT;

    /* not a Windows executable */
    if(pidhDosHeader->e_lfanew <= 0)
        DIE(("Not a Windows executable, e_lfanew is %d\n", pidhDosHeader->e_lfanew));

    if(!Intsafe_AddULong32(&cbFileHeaderOffsetSize, pidhDosHeader->e_lfanew, RTL_SIZEOF_THROUGH_FIELD(IMAGE_NT_HEADERS32, FileHeader)))
        DIE(("The DOS stub is too large, e_lfanew is %X\n", pidhDosHeader->e_lfanew));

    if(FileHeaderSize < cbFileHeaderOffsetSize)
        pinhNtHeader = NULL;
    else
    {
        /*
         * we already know that Intsafe_CanOffsetPointer(FileHeader, FileHeaderSize),
         * and FileHeaderSize >= cbFileHeaderOffsetSize, so this holds true too
         */
        ASSERT(Intsafe_CanOffsetPointer(FileHeader, pidhDosHeader->e_lfanew));
        pinhNtHeader = (PVOID)((UINT_PTR)FileHeader + pidhDosHeader->e_lfanew);
    }

    /*
     * the buffer doesn't contain the NT file header, or the alignment is wrong: we
     * need to read the header from the file
     */
    if(FileHeaderSize < cbFileHeaderOffsetSize ||
            (UINT_PTR)pinhNtHeader % TYPE_ALIGNMENT(IMAGE_NT_HEADERS32) != 0)
    {
        ULONG cbNtHeaderSize;
        ULONG cbReadSize;
        PVOID pData;

l_ReadHeaderFromFile:
        cbNtHeaderSize = 0;
        lnOffset.QuadPart = pidhDosHeader->e_lfanew;

        /* read the header from the file */
        nStatus = ReadFileCb(File, &lnOffset, sizeof(IMAGE_NT_HEADERS64), &pData, &pBuffer, &cbReadSize);

        if(!NT_SUCCESS(nStatus))
        {
            NTSTATUS ReturnedStatus = nStatus;

            /* If it attempted to read past the end of the file, it means e_lfanew is invalid */
            if (ReturnedStatus == STATUS_END_OF_FILE) nStatus = STATUS_INVALID_IMAGE_PROTECT;

            DIE(("ReadFile failed, status %08X\n", ReturnedStatus));
        }

        ASSERT(pData);
        ASSERT(pBuffer);
        ASSERT(cbReadSize > 0);

        nStatus = STATUS_INVALID_IMAGE_FORMAT;

        /* the buffer doesn't contain the file header */
        if(cbReadSize < RTL_SIZEOF_THROUGH_FIELD(IMAGE_NT_HEADERS32, FileHeader))
            DIE(("The file doesn't contain the PE file header\n"));

        pinhNtHeader = pData;

        /* object still not aligned: copy it to the beginning of the buffer */
        if((UINT_PTR)pinhNtHeader % TYPE_ALIGNMENT(IMAGE_NT_HEADERS32) != 0)
        {
            ASSERT((UINT_PTR)pBuffer % TYPE_ALIGNMENT(IMAGE_NT_HEADERS32) == 0);
            RtlMoveMemory(pBuffer, pData, cbReadSize);
            pinhNtHeader = pBuffer;
        }

        /* invalid NT header */
        nStatus = STATUS_INVALID_IMAGE_PROTECT;

        if(pinhNtHeader->Signature != IMAGE_NT_SIGNATURE)
            DIE(("The file isn't a PE executable, Signature is %X\n", pinhNtHeader->Signature));

        nStatus = STATUS_INVALID_IMAGE_FORMAT;

        if(!Intsafe_AddULong32(&cbNtHeaderSize, pinhNtHeader->FileHeader.SizeOfOptionalHeader, FIELD_OFFSET(IMAGE_NT_HEADERS32, OptionalHeader)))
            DIE(("The full NT header is too large\n"));

        /* the buffer doesn't contain the whole NT header */
        if(cbReadSize < cbNtHeaderSize)
            DIE(("The file doesn't contain the full NT header\n"));
    }
    else
    {
        ULONG cbOptHeaderOffsetSize = 0;

        nStatus = STATUS_INVALID_IMAGE_PROTECT;

        /* don't trust an invalid NT header */
        if(pinhNtHeader->Signature != IMAGE_NT_SIGNATURE)
            DIE(("The file isn't a PE executable, Signature is %X\n", pinhNtHeader->Signature));

        if(!Intsafe_AddULong32(&cbOptHeaderOffsetSize, pidhDosHeader->e_lfanew, FIELD_OFFSET(IMAGE_NT_HEADERS32, OptionalHeader)))
            DIE(("The DOS stub is too large, e_lfanew is %X\n", pidhDosHeader->e_lfanew));

        nStatus = STATUS_INVALID_IMAGE_FORMAT;

        if(!Intsafe_AddULong32(&cbOptHeaderOffsetSize, cbOptHeaderOffsetSize, pinhNtHeader->FileHeader.SizeOfOptionalHeader))
            DIE(("The NT header is too large, SizeOfOptionalHeader is %X\n", pinhNtHeader->FileHeader.SizeOfOptionalHeader));

        /* the buffer doesn't contain the whole NT header: read it from the file */
        if(cbOptHeaderOffsetSize > FileHeaderSize)
            goto l_ReadHeaderFromFile;
    }

    /* read information from the NT header */
    piohOptHeader = &pinhNtHeader->OptionalHeader;
    cbOptHeaderSize = pinhNtHeader->FileHeader.SizeOfOptionalHeader;

    nStatus = STATUS_INVALID_IMAGE_FORMAT;

    if(!RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, Magic))
        DIE(("The optional header doesn't contain the Magic field, SizeOfOptionalHeader is %X\n", cbOptHeaderSize));

    /* ASSUME: RtlZeroMemory(ImageSectionObject, sizeof(*ImageSectionObject)); */

    switch(piohOptHeader->Magic)
    {
    case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
#ifdef _WIN64
    case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
#endif // _WIN64
        break;

    default:
        DIE(("Unrecognized optional header, Magic is %X\n", piohOptHeader->Magic));
    }

    if (RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, SectionAlignment) &&
            RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, FileAlignment))
    {
        /* See [1], section 3.4.2 */
        if(piohOptHeader->SectionAlignment < PAGE_SIZE)
        {
            if(piohOptHeader->FileAlignment != piohOptHeader->SectionAlignment)
                DIE(("Sections aren't page-aligned and the file alignment isn't the same\n"));
        }
        else if(piohOptHeader->SectionAlignment < piohOptHeader->FileAlignment)
            DIE(("The section alignment is smaller than the file alignment\n"));

        nSectionAlignment = piohOptHeader->SectionAlignment;
        nFileAlignment = piohOptHeader->FileAlignment;

        if(!IsPowerOf2(nSectionAlignment) || !IsPowerOf2(nFileAlignment))
            DIE(("The section alignment (%u) and file alignment (%u) aren't both powers of 2\n", nSectionAlignment, nFileAlignment));
    }
    else
    {
        nSectionAlignment = PAGE_SIZE;
        nFileAlignment = PAGE_SIZE;
    }

    ASSERT(IsPowerOf2(nSectionAlignment));
    ASSERT(IsPowerOf2(nFileAlignment));

    switch(piohOptHeader->Magic)
    {
    /* PE32 */
    case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
    {
        if (RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, ImageBase))
            ImageBase = piohOptHeader->ImageBase;

        if(RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, SizeOfImage))
            ImageSectionObject->ImageInformation.ImageFileSize = piohOptHeader->SizeOfImage;

        if(RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, SizeOfStackReserve))
            ImageSectionObject->ImageInformation.MaximumStackSize = piohOptHeader->SizeOfStackReserve;

        if(RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, SizeOfStackCommit))
            ImageSectionObject->ImageInformation.CommittedStackSize = piohOptHeader->SizeOfStackCommit;

        if (RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, Subsystem))
        {
            ImageSectionObject->ImageInformation.SubSystemType = piohOptHeader->Subsystem;

            if (RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, MinorSubsystemVersion) &&
                    RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, MajorSubsystemVersion))
            {
                ImageSectionObject->ImageInformation.SubSystemMinorVersion = piohOptHeader->MinorSubsystemVersion;
                ImageSectionObject->ImageInformation.SubSystemMajorVersion = piohOptHeader->MajorSubsystemVersion;
            }
        }

        if (RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, AddressOfEntryPoint))
        {
            ImageSectionObject->ImageInformation.TransferAddress = (PVOID) (ImageBase +
                    piohOptHeader->AddressOfEntryPoint);
        }

        if (RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, SizeOfCode))
            ImageSectionObject->ImageInformation.ImageContainsCode = piohOptHeader->SizeOfCode != 0;
        else
            ImageSectionObject->ImageInformation.ImageContainsCode = TRUE;

        if (RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, AddressOfEntryPoint))
        {
            if (piohOptHeader->AddressOfEntryPoint == 0)
            {
                ImageSectionObject->ImageInformation.ImageContainsCode = FALSE;
            }
        }

        if (RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, LoaderFlags))
            ImageSectionObject->ImageInformation.LoaderFlags = piohOptHeader->LoaderFlags;

        if (RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, DllCharacteristics))
        {
            ImageSectionObject->ImageInformation.DllCharacteristics = piohOptHeader->DllCharacteristics;

            /*
             * Since we don't really implement SxS yet and LD doesn't supoprt /ALLOWISOLATION:NO, hard-code
             * this flag here, which will prevent the loader and other code from doing any .manifest or SxS
             * magic to any binary.
             *
             * This will break applications that depend on SxS when running with real Windows Kernel32/SxS/etc
             * but honestly that's not tested. It will also break them when running no ReactOS once we implement
             * the SxS support -- at which point, duh, this should be removed.
             *
             * But right now, any app depending on SxS is already broken anyway, so this flag only helps.
             */
            ImageSectionObject->ImageInformation.DllCharacteristics |= IMAGE_DLLCHARACTERISTICS_NO_ISOLATION;
        }

        break;
    }
#ifdef _WIN64
    /* PE64 */
    case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
    {
        const IMAGE_OPTIONAL_HEADER64 * pioh64OptHeader;

        pioh64OptHeader = (const IMAGE_OPTIONAL_HEADER64 *)piohOptHeader;

        if(RTL_CONTAINS_FIELD(pioh64OptHeader, cbOptHeaderSize, ImageBase))
        {
            ImageBase = pioh64OptHeader->ImageBase;
            if(pioh64OptHeader->ImageBase > MAXULONG_PTR)
                DIE(("ImageBase exceeds the address space\n"));
        }

        if(RTL_CONTAINS_FIELD(pioh64OptHeader, cbOptHeaderSize, SizeOfImage))
        {
            if(pioh64OptHeader->SizeOfImage > MAXULONG_PTR)
                DIE(("SizeOfImage exceeds the address space\n"));

            ImageSectionObject->ImageInformation.ImageFileSize = pioh64OptHeader->SizeOfImage;
        }

        if(RTL_CONTAINS_FIELD(pioh64OptHeader, cbOptHeaderSize, SizeOfStackReserve))
        {
            if(pioh64OptHeader->SizeOfStackReserve > MAXULONG_PTR)
                DIE(("SizeOfStackReserve exceeds the address space\n"));

            ImageSectionObject->ImageInformation.MaximumStackSize = (ULONG_PTR) pioh64OptHeader->SizeOfStackReserve;
        }

        if(RTL_CONTAINS_FIELD(pioh64OptHeader, cbOptHeaderSize, SizeOfStackCommit))
        {
            if(pioh64OptHeader->SizeOfStackCommit > MAXULONG_PTR)
                DIE(("SizeOfStackCommit exceeds the address space\n"));

            ImageSectionObject->ImageInformation.CommittedStackSize = (ULONG_PTR) pioh64OptHeader->SizeOfStackCommit;
        }

        if (RTL_CONTAINS_FIELD(pioh64OptHeader, cbOptHeaderSize, Subsystem))
        {
            ImageSectionObject->ImageInformation.SubSystemType = pioh64OptHeader->Subsystem;

            if (RTL_CONTAINS_FIELD(pioh64OptHeader, cbOptHeaderSize, MinorSubsystemVersion) &&
                    RTL_CONTAINS_FIELD(pioh64OptHeader, cbOptHeaderSize, MajorSubsystemVersion))
            {
                ImageSectionObject->ImageInformation.SubSystemMinorVersion = pioh64OptHeader->MinorSubsystemVersion;
                ImageSectionObject->ImageInformation.SubSystemMajorVersion = pioh64OptHeader->MajorSubsystemVersion;
            }
        }

        if (RTL_CONTAINS_FIELD(pioh64OptHeader, cbOptHeaderSize, AddressOfEntryPoint))
        {
            ImageSectionObject->ImageInformation.TransferAddress = (PVOID) (ImageBase +
                    pioh64OptHeader->AddressOfEntryPoint);
        }

        if (RTL_CONTAINS_FIELD(pioh64OptHeader, cbOptHeaderSize, SizeOfCode))
            ImageSectionObject->ImageInformation.ImageContainsCode = pioh64OptHeader->SizeOfCode != 0;
        else
            ImageSectionObject->ImageInformation.ImageContainsCode = TRUE;

        if (RTL_CONTAINS_FIELD(pioh64OptHeader, cbOptHeaderSize, AddressOfEntryPoint))
        {
            if (pioh64OptHeader->AddressOfEntryPoint == 0)
            {
                ImageSectionObject->ImageInformation.ImageContainsCode = FALSE;
            }
        }

        if (RTL_CONTAINS_FIELD(pioh64OptHeader, cbOptHeaderSize, LoaderFlags))
            ImageSectionObject->ImageInformation.LoaderFlags = pioh64OptHeader->LoaderFlags;

        if (RTL_CONTAINS_FIELD(pioh64OptHeader, cbOptHeaderSize, DllCharacteristics))
            ImageSectionObject->ImageInformation.DllCharacteristics = pioh64OptHeader->DllCharacteristics;

        break;
    }
#endif // _WIN64
    }

    /* [1], section 3.4.2 */
    if((ULONG_PTR)ImageBase % 0x10000)
        DIE(("ImageBase is not aligned on a 64KB boundary"));

    ImageSectionObject->ImageInformation.ImageCharacteristics = pinhNtHeader->FileHeader.Characteristics;
    ImageSectionObject->ImageInformation.Machine = pinhNtHeader->FileHeader.Machine;
    ImageSectionObject->ImageInformation.GpValue = 0;
    ImageSectionObject->ImageInformation.ZeroBits = 0;
    ImageSectionObject->BasedAddress = (PVOID)ImageBase;

    /* SECTION HEADERS */
    nStatus = STATUS_INVALID_IMAGE_FORMAT;

    /* see [1], section 3.3 */
    if(pinhNtHeader->FileHeader.NumberOfSections > 96)
        DIE(("Too many sections, NumberOfSections is %u\n", pinhNtHeader->FileHeader.NumberOfSections));

    /*
     * the additional segment is for the file's headers. They need to be present for
     * the benefit of the dynamic loader (to locate exports, defaults for thread
     * parameters, resources, etc.)
     */
    ImageSectionObject->NrSegments = pinhNtHeader->FileHeader.NumberOfSections + 1;

    /* file offset for the section headers */
    if(!Intsafe_AddULong32(&cbSectionHeadersOffset, pidhDosHeader->e_lfanew, FIELD_OFFSET(IMAGE_NT_HEADERS32, OptionalHeader)))
        DIE(("Offset overflow\n"));

    if(!Intsafe_AddULong32(&cbSectionHeadersOffset, cbSectionHeadersOffset, pinhNtHeader->FileHeader.SizeOfOptionalHeader))
        DIE(("Offset overflow\n"));

    /* size of the section headers */
    ASSERT(Intsafe_CanMulULong32(pinhNtHeader->FileHeader.NumberOfSections, sizeof(IMAGE_SECTION_HEADER)));
    cbSectionHeadersSize = pinhNtHeader->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);

    if(!Intsafe_AddULong32(&cbSectionHeadersOffsetSize, cbSectionHeadersOffset, cbSectionHeadersSize))
        DIE(("Section headers too large\n"));

    /* size of the executable's headers */
    if(RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, SizeOfHeaders))
    {
//        if(!IsAligned(piohOptHeader->SizeOfHeaders, nFileAlignment))
//            DIE(("SizeOfHeaders is not aligned\n"));

        if(cbSectionHeadersSize > piohOptHeader->SizeOfHeaders)
            DIE(("The section headers overflow SizeOfHeaders\n"));

        cbHeadersSize = piohOptHeader->SizeOfHeaders;
    }
    else if(!AlignUp(&cbHeadersSize, cbSectionHeadersOffsetSize, nFileAlignment))
        DIE(("Overflow aligning the size of headers\n"));

    if(pBuffer)
    {
        ExFreePool(pBuffer);
        pBuffer = NULL;
    }
    /* WARNING: pinhNtHeader IS NO LONGER USABLE */
    /* WARNING: piohOptHeader IS NO LONGER USABLE */
    /* WARNING: pioh64OptHeader IS NO LONGER USABLE */

    if(FileHeaderSize < cbSectionHeadersOffsetSize)
        pishSectionHeaders = NULL;
    else
    {
        /*
         * we already know that Intsafe_CanOffsetPointer(FileHeader, FileHeaderSize),
         * and FileHeaderSize >= cbSectionHeadersOffsetSize, so this holds true too
         */
        ASSERT(Intsafe_CanOffsetPointer(FileHeader, cbSectionHeadersOffset));
        pishSectionHeaders = (PVOID)((UINT_PTR)FileHeader + cbSectionHeadersOffset);
    }

    /*
     * the buffer doesn't contain the section headers, or the alignment is wrong:
     * read the headers from the file
     */
    if(FileHeaderSize < cbSectionHeadersOffsetSize ||
            (UINT_PTR)pishSectionHeaders % TYPE_ALIGNMENT(IMAGE_SECTION_HEADER) != 0)
    {
        PVOID pData;
        ULONG cbReadSize;

        lnOffset.QuadPart = cbSectionHeadersOffset;

        /* read the header from the file */
        nStatus = ReadFileCb(File, &lnOffset, cbSectionHeadersSize, &pData, &pBuffer, &cbReadSize);

        if(!NT_SUCCESS(nStatus))
            DIE(("ReadFile failed with status %08X\n", nStatus));

        ASSERT(pData);
        ASSERT(pBuffer);
        ASSERT(cbReadSize > 0);

        nStatus = STATUS_INVALID_IMAGE_FORMAT;

        /* the buffer doesn't contain all the section headers */
        if(cbReadSize < cbSectionHeadersSize)
            DIE(("The file doesn't contain all of the section headers\n"));

        pishSectionHeaders = pData;

        /* object still not aligned: copy it to the beginning of the buffer */
        if((UINT_PTR)pishSectionHeaders % TYPE_ALIGNMENT(IMAGE_SECTION_HEADER) != 0)
        {
            ASSERT((UINT_PTR)pBuffer % TYPE_ALIGNMENT(IMAGE_SECTION_HEADER) == 0);
            RtlMoveMemory(pBuffer, pData, cbReadSize);
            pishSectionHeaders = pBuffer;
        }
    }

    /* SEGMENTS */
    /* allocate the segments */
    nStatus = STATUS_INSUFFICIENT_RESOURCES;
    ImageSectionObject->Segments = AllocateSegmentsCb(ImageSectionObject->NrSegments);

    if(ImageSectionObject->Segments == NULL)
        DIE(("AllocateSegments failed\n"));

    /* initialize the headers segment */
    pssSegments = ImageSectionObject->Segments;

//  ASSERT(IsAligned(cbHeadersSize, nFileAlignment));

    if(!AlignUp(&nFileSizeOfHeaders, cbHeadersSize, nFileAlignment))
        DIE(("Cannot align the size of the section headers\n"));

    nPrevVirtualEndOfSegment = ALIGN_UP_BY(cbHeadersSize, nSectionAlignment);
    if (nPrevVirtualEndOfSegment < cbHeadersSize)
        DIE(("Cannot align the size of the section headers\n"));

    pssSegments[0].Image.FileOffset = 0;
    pssSegments[0].Protection = PAGE_READONLY;
    pssSegments[0].Length.QuadPart = nPrevVirtualEndOfSegment;
    pssSegments[0].RawLength.QuadPart = nFileSizeOfHeaders;
    pssSegments[0].Image.VirtualAddress = 0;
    pssSegments[0].Image.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA;
    pssSegments[0].WriteCopy = TRUE;

    /* skip the headers segment */
    ++ pssSegments;

    nStatus = STATUS_INVALID_IMAGE_FORMAT;

    /* convert the executable sections into segments. See also [1], section 4 */
    for(i = 0; i < ImageSectionObject->NrSegments - 1; ++ i)
    {
        ULONG nCharacteristics;

        /* validate the alignment */
        if(!IsAligned(pishSectionHeaders[i].VirtualAddress, nSectionAlignment))
            DIE(("Image.VirtualAddress[%u] is not aligned\n", i));

        /* sections must be contiguous, ordered by base address and non-overlapping */
        if(pishSectionHeaders[i].VirtualAddress != nPrevVirtualEndOfSegment)
            DIE(("Memory gap between section %u and the previous\n", i));

        /* ignore explicit BSS sections */
        if(pishSectionHeaders[i].SizeOfRawData != 0)
        {
            /* validate the alignment */
#if 0
            /* Yes, this should be a multiple of FileAlignment, but there's
             * stuff out there that isn't. We can cope with that
             */
            if(!IsAligned(pishSectionHeaders[i].SizeOfRawData, nFileAlignment))
                DIE(("SizeOfRawData[%u] is not aligned\n", i));
#endif

//            if(!IsAligned(pishSectionHeaders[i].PointerToRawData, nFileAlignment))
//                DIE(("PointerToRawData[%u] is not aligned\n", i));

            /* conversion */
            pssSegments[i].Image.FileOffset = pishSectionHeaders[i].PointerToRawData;
            pssSegments[i].RawLength.QuadPart = pishSectionHeaders[i].SizeOfRawData;
        }
        else
        {
            ASSERT(pssSegments[i].Image.FileOffset == 0);
            ASSERT(pssSegments[i].RawLength.QuadPart == 0);
        }

        ASSERT(Intsafe_CanAddLong64(pssSegments[i].Image.FileOffset, pssSegments[i].RawLength.QuadPart));

        nCharacteristics = pishSectionHeaders[i].Characteristics;

        /* no explicit protection */
        if((nCharacteristics & (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE)) == 0)
        {
            if(nCharacteristics & IMAGE_SCN_CNT_CODE)
                nCharacteristics |= IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;

            if(nCharacteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)
                nCharacteristics |= IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;

            if(nCharacteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
                nCharacteristics |= IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
        }

        /* see table above */
        pssSegments[i].Protection = SectionCharacteristicsToProtect[nCharacteristics >> 28];
        pssSegments[i].WriteCopy = !(nCharacteristics & IMAGE_SCN_MEM_SHARED);

        if(pishSectionHeaders[i].Misc.VirtualSize == 0 || pishSectionHeaders[i].Misc.VirtualSize < pishSectionHeaders[i].SizeOfRawData)
            pssSegments[i].Length.QuadPart = pishSectionHeaders[i].SizeOfRawData;
        else
            pssSegments[i].Length.QuadPart = pishSectionHeaders[i].Misc.VirtualSize;

        pssSegments[i].Length.LowPart = ALIGN_UP_BY(pssSegments[i].Length.LowPart, nSectionAlignment);
        /* FIXME: always false */
        if (pssSegments[i].Length.QuadPart < pssSegments[i].Length.QuadPart)
            DIE(("Cannot align the virtual size of section %u\n", i));

        if(pssSegments[i].Length.QuadPart == 0)
            DIE(("Virtual size of section %u is null\n", i));

        pssSegments[i].Image.VirtualAddress = pishSectionHeaders[i].VirtualAddress;
        pssSegments[i].Image.Characteristics = pishSectionHeaders[i].Characteristics;

        /* ensure the memory image is no larger than 4GB */
        nPrevVirtualEndOfSegment = (ULONG_PTR)(pssSegments[i].Image.VirtualAddress + pssSegments[i].Length.QuadPart);
        if (nPrevVirtualEndOfSegment < pssSegments[i].Image.VirtualAddress)
            DIE(("The image is too large\n"));
    }

    if(nSectionAlignment >= PAGE_SIZE)
        *Flags |= EXEFMT_LOAD_ASSUME_SEGMENTS_PAGE_ALIGNED;

    /* Success */
    nStatus = STATUS_SUCCESS;// STATUS_ROS_EXEFMT_LOADED_FORMAT | EXEFMT_LOADED_PE32;

l_Return:
    if(pBuffer)
        ExFreePool(pBuffer);

    return nStatus;
}

/*
 * FUNCTION:  Waits in kernel mode indefinitely for a file object lock.
 * ARGUMENTS: PFILE_OBJECT to wait for.
 * RETURNS:   Status of the wait.
 */
NTSTATUS
MmspWaitForFileLock(PFILE_OBJECT File)
{
    return STATUS_SUCCESS;
    //return KeWaitForSingleObject(&File->Lock, 0, KernelMode, FALSE, NULL);
}

VOID
NTAPI
MmFreeSectionSegments(PFILE_OBJECT FileObject)
{
    if (FileObject->SectionObjectPointer->ImageSectionObject != NULL)
    {
        PMM_IMAGE_SECTION_OBJECT ImageSectionObject;
        PMM_SECTION_SEGMENT SectionSegments;
        ULONG NrSegments;
        ULONG i;

        ImageSectionObject = (PMM_IMAGE_SECTION_OBJECT)FileObject->SectionObjectPointer->ImageSectionObject;
        NrSegments = ImageSectionObject->NrSegments;
        SectionSegments = ImageSectionObject->Segments;
        for (i = 0; i < NrSegments; i++)
        {
            if (SectionSegments[i].ReferenceCount != 0)
            {
                DPRINT1("Image segment %lu still referenced (was %lu)\n", i,
                        SectionSegments[i].ReferenceCount);
                KeBugCheck(MEMORY_MANAGEMENT);
            }
            MmFreePageTablesSectionSegment(&SectionSegments[i], NULL);
        }
        ExFreePool(ImageSectionObject->Segments);
        ExFreePool(ImageSectionObject);
        FileObject->SectionObjectPointer->ImageSectionObject = NULL;
    }
    if (FileObject->SectionObjectPointer->DataSectionObject != NULL)
    {
        PMM_SECTION_SEGMENT Segment;

        Segment = (PMM_SECTION_SEGMENT)FileObject->SectionObjectPointer->
                  DataSectionObject;

        if (Segment->ReferenceCount != 0)
        {
            DPRINT1("Data segment still referenced\n");
            KeBugCheck(MEMORY_MANAGEMENT);
        }
        MmFreePageTablesSectionSegment(Segment, NULL);
        ExFreePool(Segment);
        FileObject->SectionObjectPointer->DataSectionObject = NULL;
    }
}

VOID
NTAPI
MmSharePageEntrySectionSegment(PMM_SECTION_SEGMENT Segment,
                               PLARGE_INTEGER Offset)
{
    ULONG_PTR Entry;

    Entry = MmGetPageEntrySectionSegment(Segment, Offset);
    if (Entry == 0)
    {
        DPRINT1("Entry == 0 for MmSharePageEntrySectionSegment\n");
        KeBugCheck(MEMORY_MANAGEMENT);
    }
    if (SHARE_COUNT_FROM_SSE(Entry) == MAX_SHARE_COUNT)
    {
        DPRINT1("Maximum share count reached\n");
        KeBugCheck(MEMORY_MANAGEMENT);
    }
    if (IS_SWAP_FROM_SSE(Entry))
    {
        KeBugCheck(MEMORY_MANAGEMENT);
    }
    Entry = MAKE_SSE(PAGE_FROM_SSE(Entry), SHARE_COUNT_FROM_SSE(Entry) + 1);
    MmSetPageEntrySectionSegment(Segment, Offset, Entry);
}

BOOLEAN
NTAPI
MmUnsharePageEntrySectionSegment(PROS_SECTION_OBJECT Section,
                                 PMM_SECTION_SEGMENT Segment,
                                 PLARGE_INTEGER Offset,
                                 BOOLEAN Dirty,
                                 BOOLEAN PageOut,
                                 ULONG_PTR *InEntry)
{
    ULONG_PTR Entry = InEntry ? *InEntry : MmGetPageEntrySectionSegment(Segment, Offset);
    BOOLEAN IsDirectMapped = FALSE;

    if (Entry == 0)
    {
        DPRINT1("Entry == 0 for MmUnsharePageEntrySectionSegment\n");
        KeBugCheck(MEMORY_MANAGEMENT);
    }
    if (SHARE_COUNT_FROM_SSE(Entry) == 0)
    {
        DPRINT1("Zero share count for unshare (Seg %p Offset %x Page %x)\n", Segment, Offset->LowPart, PFN_FROM_SSE(Entry));
        KeBugCheck(MEMORY_MANAGEMENT);
    }
    if (IS_SWAP_FROM_SSE(Entry))
    {
        KeBugCheck(MEMORY_MANAGEMENT);
    }
    Entry = MAKE_SSE(PAGE_FROM_SSE(Entry), SHARE_COUNT_FROM_SSE(Entry) - 1);
    /*
     * If we reducing the share count of this entry to zero then set the entry
     * to zero and tell the cache the page is no longer mapped.
     */
    if (SHARE_COUNT_FROM_SSE(Entry) == 0)
    {
        PFILE_OBJECT FileObject;
        SWAPENTRY SavedSwapEntry;
        PFN_NUMBER Page;
#ifndef NEWCC
        PROS_SHARED_CACHE_MAP SharedCacheMap;
        BOOLEAN IsImageSection;
        LARGE_INTEGER FileOffset;

        FileOffset.QuadPart = Offset->QuadPart + Segment->Image.FileOffset;
        IsImageSection = Section->AllocationAttributes & SEC_IMAGE ? TRUE : FALSE;
#endif

        Page = PFN_FROM_SSE(Entry);
        FileObject = Section->FileObject;
        if (FileObject != NULL &&
                !(Segment->Image.Characteristics & IMAGE_SCN_MEM_SHARED))
        {

#ifndef NEWCC
            if ((FileOffset.QuadPart % PAGE_SIZE) == 0 &&
                    (Offset->QuadPart + PAGE_SIZE <= Segment->RawLength.QuadPart || !IsImageSection))
            {
                NTSTATUS Status;
                SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
                IsDirectMapped = TRUE;
#ifndef NEWCC
                Status = CcRosUnmapVacb(SharedCacheMap, FileOffset.QuadPart, Dirty);
#else
                Status = STATUS_SUCCESS;
#endif
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("CcRosUnmapVacb failed, status = %x\n", Status);
                    KeBugCheck(MEMORY_MANAGEMENT);
                }
            }
#endif
        }

        SavedSwapEntry = MmGetSavedSwapEntryPage(Page);
        if (SavedSwapEntry == 0)
        {
            if (!PageOut &&
                    ((Segment->Flags & MM_PAGEFILE_SEGMENT) ||
                     (Segment->Image.Characteristics & IMAGE_SCN_MEM_SHARED)))
            {
                /*
                 * FIXME:
                 *   Try to page out this page and set the swap entry
                 *   within the section segment. There exist no rmap entry
                 *   for this page. The pager thread can't page out a
                 *   page without a rmap entry.
                 */
                MmSetPageEntrySectionSegment(Segment, Offset, Entry);
                if (InEntry) *InEntry = Entry;
                MiSetPageEvent(NULL, NULL);
            }
            else
            {
                MmSetPageEntrySectionSegment(Segment, Offset, 0);
                if (InEntry) *InEntry = 0;
                MiSetPageEvent(NULL, NULL);
                if (!IsDirectMapped)
                {
                    MmReleasePageMemoryConsumer(MC_USER, Page);
                }
            }
        }
        else
        {
            if ((Segment->Flags & MM_PAGEFILE_SEGMENT) ||
                    (Segment->Image.Characteristics & IMAGE_SCN_MEM_SHARED))
            {
                if (!PageOut)
                {
                    if (Dirty)
                    {
                        /*
                         * FIXME:
                         *   We hold all locks. Nobody can do something with the current
                         *   process and the current segment (also not within an other process).
                         */
                        NTSTATUS Status;
                        Status = MmWriteToSwapPage(SavedSwapEntry, Page);
                        if (!NT_SUCCESS(Status))
                        {
                            DPRINT1("MM: Failed to write to swap page (Status was 0x%.8X)\n", Status);
                            KeBugCheck(MEMORY_MANAGEMENT);
                        }
                    }
                    MmSetPageEntrySectionSegment(Segment, Offset, MAKE_SWAP_SSE(SavedSwapEntry));
                    if (InEntry) *InEntry = MAKE_SWAP_SSE(SavedSwapEntry);
                    MmSetSavedSwapEntryPage(Page, 0);
                    MiSetPageEvent(NULL, NULL);
                }
                MmReleasePageMemoryConsumer(MC_USER, Page);
            }
            else
            {
                DPRINT1("Found a swapentry for a non private page in an image or data file sgment\n");
                KeBugCheck(MEMORY_MANAGEMENT);
            }
        }
    }
    else
    {
        if (InEntry)
            *InEntry = Entry;
        else
            MmSetPageEntrySectionSegment(Segment, Offset, Entry);
    }
    return(SHARE_COUNT_FROM_SSE(Entry) > 0);
}

BOOLEAN MiIsPageFromCache(PMEMORY_AREA MemoryArea,
                          LONGLONG SegOffset)
{
#ifndef NEWCC
    if (!(MemoryArea->Data.SectionData.Segment->Image.Characteristics & IMAGE_SCN_MEM_SHARED))
    {
        PROS_SHARED_CACHE_MAP SharedCacheMap;
        PROS_VACB Vacb;
        SharedCacheMap = MemoryArea->Data.SectionData.Section->FileObject->SectionObjectPointer->SharedCacheMap;
        Vacb = CcRosLookupVacb(SharedCacheMap, SegOffset + MemoryArea->Data.SectionData.Segment->Image.FileOffset);
        if (Vacb)
        {
            CcRosReleaseVacb(SharedCacheMap, Vacb, Vacb->Valid, FALSE, TRUE);
            return TRUE;
        }
    }
#endif
    return FALSE;
}

NTSTATUS
NTAPI
MiCopyFromUserPage(PFN_NUMBER DestPage, PFN_NUMBER SrcPage)
{
    PEPROCESS Process;
    KIRQL Irql, Irql2;
    PVOID DestAddress, SrcAddress;

    Process = PsGetCurrentProcess();
    DestAddress = MiMapPageInHyperSpace(Process, DestPage, &Irql);
    SrcAddress = MiMapPageInHyperSpace(Process, SrcPage, &Irql2);
    if (DestAddress == NULL || SrcAddress == NULL)
    {
        return(STATUS_NO_MEMORY);
    }
    ASSERT((ULONG_PTR)DestAddress % PAGE_SIZE == 0);
    ASSERT((ULONG_PTR)SrcAddress % PAGE_SIZE == 0);
    RtlCopyMemory(DestAddress, SrcAddress, PAGE_SIZE);
    MiUnmapPageInHyperSpace(Process, SrcAddress, Irql2);
    MiUnmapPageInHyperSpace(Process, DestAddress, Irql);
    return(STATUS_SUCCESS);
}

#ifndef NEWCC
NTSTATUS
NTAPI
MiReadPage(PMEMORY_AREA MemoryArea,
           LONGLONG SegOffset,
           PPFN_NUMBER Page)
/*
 * FUNCTION: Read a page for a section backed memory area.
 * PARAMETERS:
 *       MemoryArea - Memory area to read the page for.
 *       Offset - Offset of the page to read.
 *       Page - Variable that receives a page contains the read data.
 */
{
    LONGLONG BaseOffset;
    LONGLONG FileOffset;
    PVOID BaseAddress;
    BOOLEAN UptoDate;
    PROS_VACB Vacb;
    PFILE_OBJECT FileObject;
    NTSTATUS Status;
    LONGLONG RawLength;
    PROS_SHARED_CACHE_MAP SharedCacheMap;
    BOOLEAN IsImageSection;
    LONGLONG Length;

    FileObject = MemoryArea->Data.SectionData.Section->FileObject;
    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
    RawLength = MemoryArea->Data.SectionData.Segment->RawLength.QuadPart;
    FileOffset = SegOffset + MemoryArea->Data.SectionData.Segment->Image.FileOffset;
    IsImageSection = MemoryArea->Data.SectionData.Section->AllocationAttributes & SEC_IMAGE ? TRUE : FALSE;

    ASSERT(SharedCacheMap);

    DPRINT("%S %I64x\n", FileObject->FileName.Buffer, FileOffset);

    /*
     * If the file system is letting us go directly to the cache and the
     * memory area was mapped at an offset in the file which is page aligned
     * then get the related VACB.
     */
    if (((FileOffset % PAGE_SIZE) == 0) &&
            ((SegOffset + PAGE_SIZE <= RawLength) || !IsImageSection) &&
            !(MemoryArea->Data.SectionData.Segment->Image.Characteristics & IMAGE_SCN_MEM_SHARED))
    {

        /*
         * Get the related VACB; we use a lower level interface than
         * filesystems do because it is safe for us to use an offset with an
         * alignment less than the file system block size.
         */
        Status = CcRosGetVacb(SharedCacheMap,
                              FileOffset,
                              &BaseOffset,
                              &BaseAddress,
                              &UptoDate,
                              &Vacb);
        if (!NT_SUCCESS(Status))
        {
            return(Status);
        }
        if (!UptoDate)
        {
            /*
             * If the VACB isn't up to date then call the file
             * system to read in the data.
             */
            Status = CcReadVirtualAddress(Vacb);
            if (!NT_SUCCESS(Status))
            {
                CcRosReleaseVacb(SharedCacheMap, Vacb, FALSE, FALSE, FALSE);
                return Status;
            }
        }

        /* Probe the page, since it's PDE might not be synced */
        (void)*((volatile char*)BaseAddress + FileOffset - BaseOffset);

        /*
         * Retrieve the page from the view that we actually want.
         */
        (*Page) = MmGetPhysicalAddress((char*)BaseAddress +
                                       FileOffset - BaseOffset).LowPart >> PAGE_SHIFT;

        CcRosReleaseVacb(SharedCacheMap, Vacb, TRUE, FALSE, TRUE);
    }
    else
    {
        PEPROCESS Process;
        KIRQL Irql;
        PVOID PageAddr;
        LONGLONG VacbOffset;

        /*
         * Allocate a page, this is rather complicated by the possibility
         * we might have to move other things out of memory
         */
        MI_SET_USAGE(MI_USAGE_SECTION);
        MI_SET_PROCESS2(PsGetCurrentProcess()->ImageFileName);
        Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, Page);
        if (!NT_SUCCESS(Status))
        {
            return(Status);
        }
        Status = CcRosGetVacb(SharedCacheMap,
                              FileOffset,
                              &BaseOffset,
                              &BaseAddress,
                              &UptoDate,
                              &Vacb);
        if (!NT_SUCCESS(Status))
        {
            return(Status);
        }
        if (!UptoDate)
        {
            /*
             * If the VACB isn't up to date then call the file
             * system to read in the data.
             */
            Status = CcReadVirtualAddress(Vacb);
            if (!NT_SUCCESS(Status))
            {
                CcRosReleaseVacb(SharedCacheMap, Vacb, FALSE, FALSE, FALSE);
                return Status;
            }
        }

        Process = PsGetCurrentProcess();
        PageAddr = MiMapPageInHyperSpace(Process, *Page, &Irql);
        VacbOffset = BaseOffset + VACB_MAPPING_GRANULARITY - FileOffset;
        Length = RawLength - SegOffset;
        if (Length <= VacbOffset && Length <= PAGE_SIZE)
        {
            memcpy(PageAddr, (char*)BaseAddress + FileOffset - BaseOffset, Length);
        }
        else if (VacbOffset >= PAGE_SIZE)
        {
            memcpy(PageAddr, (char*)BaseAddress + FileOffset - BaseOffset, PAGE_SIZE);
        }
        else
        {
            memcpy(PageAddr, (char*)BaseAddress + FileOffset - BaseOffset, VacbOffset);
            MiUnmapPageInHyperSpace(Process, PageAddr, Irql);
            CcRosReleaseVacb(SharedCacheMap, Vacb, TRUE, FALSE, FALSE);
            Status = CcRosGetVacb(SharedCacheMap,
                                  FileOffset + VacbOffset,
                                  &BaseOffset,
                                  &BaseAddress,
                                  &UptoDate,
                                  &Vacb);
            if (!NT_SUCCESS(Status))
            {
                return(Status);
            }
            if (!UptoDate)
            {
                /*
                 * If the VACB isn't up to date then call the file
                 * system to read in the data.
                 */
                Status = CcReadVirtualAddress(Vacb);
                if (!NT_SUCCESS(Status))
                {
                    CcRosReleaseVacb(SharedCacheMap, Vacb, FALSE, FALSE, FALSE);
                    return Status;
                }
            }
            PageAddr = MiMapPageInHyperSpace(Process, *Page, &Irql);
            if (Length < PAGE_SIZE)
            {
                memcpy((char*)PageAddr + VacbOffset, BaseAddress, Length - VacbOffset);
            }
            else
            {
                memcpy((char*)PageAddr + VacbOffset, BaseAddress, PAGE_SIZE - VacbOffset);
            }
        }
        MiUnmapPageInHyperSpace(Process, PageAddr, Irql);
        CcRosReleaseVacb(SharedCacheMap, Vacb, TRUE, FALSE, FALSE);
    }
    return(STATUS_SUCCESS);
}
#else
NTSTATUS
NTAPI
MiReadPage(PMEMORY_AREA MemoryArea,
           LONGLONG SegOffset,
           PPFN_NUMBER Page)
/*
 * FUNCTION: Read a page for a section backed memory area.
 * PARAMETERS:
 *       MemoryArea - Memory area to read the page for.
 *       Offset - Offset of the page to read.
 *       Page - Variable that receives a page contains the read data.
 */
{
    MM_REQUIRED_RESOURCES Resources;
    NTSTATUS Status;

    RtlZeroMemory(&Resources, sizeof(MM_REQUIRED_RESOURCES));

    Resources.Context = MemoryArea->Data.SectionData.Section->FileObject;
    Resources.FileOffset.QuadPart = SegOffset +
                                    MemoryArea->Data.SectionData.Segment->Image.FileOffset;
    Resources.Consumer = MC_USER;
    Resources.Amount = PAGE_SIZE;

    DPRINT("%S, offset 0x%x, len 0x%x, page 0x%x\n", ((PFILE_OBJECT)Resources.Context)->FileName.Buffer, Resources.FileOffset.LowPart, Resources.Amount, Resources.Page[0]);

    Status = MiReadFilePage(MmGetKernelAddressSpace(), MemoryArea, &Resources);
    *Page = Resources.Page[0];
    return Status;
}
#endif

NTSTATUS
NTAPI
MmNotPresentFaultSectionView(PMMSUPPORT AddressSpace,
                             MEMORY_AREA* MemoryArea,
                             PVOID Address,
                             BOOLEAN Locked)
{
    LARGE_INTEGER Offset;
    PFN_NUMBER Page;
    NTSTATUS Status;
    PROS_SECTION_OBJECT Section;
    PMM_SECTION_SEGMENT Segment;
    ULONG_PTR Entry;
    ULONG_PTR Entry1;
    ULONG Attributes;
    PMM_REGION Region;
    BOOLEAN HasSwapEntry;
    PVOID PAddress;
    PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);
    SWAPENTRY SwapEntry;

    /*
     * There is a window between taking the page fault and locking the
     * address space when another thread could load the page so we check
     * that.
     */
    if (MmIsPagePresent(Process, Address))
    {
        return(STATUS_SUCCESS);
    }

    if (MmIsDisabledPage(Process, Address))
    {
        return(STATUS_ACCESS_VIOLATION);
    }

    /*
     * Check for the virtual memory area being deleted.
     */
    if (MemoryArea->DeleteInProgress)
    {
        return(STATUS_UNSUCCESSFUL);
    }

    PAddress = MM_ROUND_DOWN(Address, PAGE_SIZE);
    Offset.QuadPart = (ULONG_PTR)PAddress - MA_GetStartingAddress(MemoryArea)
                      + MemoryArea->Data.SectionData.ViewOffset.QuadPart;

    Segment = MemoryArea->Data.SectionData.Segment;
    Section = MemoryArea->Data.SectionData.Section;
    Region = MmFindRegion((PVOID)MA_GetStartingAddress(MemoryArea),
                          &MemoryArea->Data.SectionData.RegionListHead,
                          Address, NULL);
    ASSERT(Region != NULL);
    /*
     * Lock the segment
     */
    MmLockSectionSegment(Segment);
    Entry = MmGetPageEntrySectionSegment(Segment, &Offset);
    /*
     * Check if this page needs to be mapped COW
     */
    if ((Segment->WriteCopy) &&
            (Region->Protect == PAGE_READWRITE ||
             Region->Protect == PAGE_EXECUTE_READWRITE))
    {
        Attributes = Region->Protect == PAGE_READWRITE ? PAGE_READONLY : PAGE_EXECUTE_READ;
    }
    else
    {
        Attributes = Region->Protect;
    }

    /*
     * Check if someone else is already handling this fault, if so wait
     * for them
     */
    if (Entry && IS_SWAP_FROM_SSE(Entry) && SWAPENTRY_FROM_SSE(Entry) == MM_WAIT_ENTRY)
    {
        MmUnlockSectionSegment(Segment);
        MmUnlockAddressSpace(AddressSpace);
        MiWaitForPageEvent(NULL, NULL);
        MmLockAddressSpace(AddressSpace);
        DPRINT("Address 0x%p\n", Address);
        return(STATUS_MM_RESTART_OPERATION);
    }

    HasSwapEntry = MmIsPageSwapEntry(Process, Address);

    if (HasSwapEntry)
    {
        SWAPENTRY DummyEntry;

        /*
         * Is it a wait entry?
         */
        MmGetPageFileMapping(Process, Address, &SwapEntry);

        if (SwapEntry == MM_WAIT_ENTRY)
        {
            MmUnlockSectionSegment(Segment);
            MmUnlockAddressSpace(AddressSpace);
            MiWaitForPageEvent(NULL, NULL);
            MmLockAddressSpace(AddressSpace);
            return STATUS_MM_RESTART_OPERATION;
        }

        /*
         * Must be private page we have swapped out.
         */

        /*
         * Sanity check
         */
        if (Segment->Flags & MM_PAGEFILE_SEGMENT)
        {
            DPRINT1("Found a swaped out private page in a pagefile section.\n");
            KeBugCheck(MEMORY_MANAGEMENT);
        }

        MmUnlockSectionSegment(Segment);
        MmDeletePageFileMapping(Process, Address, &SwapEntry);
        MmCreatePageFileMapping(Process, Address, MM_WAIT_ENTRY);

        MmUnlockAddressSpace(AddressSpace);
        MI_SET_USAGE(MI_USAGE_SECTION);
        if (Process) MI_SET_PROCESS2(Process->ImageFileName);
        if (!Process) MI_SET_PROCESS2("Kernel Section");
        Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &Page);
        if (!NT_SUCCESS(Status))
        {
            KeBugCheck(MEMORY_MANAGEMENT);
        }

        Status = MmReadFromSwapPage(SwapEntry, Page);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("MmReadFromSwapPage failed, status = %x\n", Status);
            KeBugCheck(MEMORY_MANAGEMENT);
        }
        MmLockAddressSpace(AddressSpace);
        MmDeletePageFileMapping(Process, PAddress, &DummyEntry);
        Status = MmCreateVirtualMapping(Process,
                                        PAddress,
                                        Region->Protect,
                                        &Page,
                                        1);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("MmCreateVirtualMapping failed, not out of memory\n");
            KeBugCheck(MEMORY_MANAGEMENT);
            return(Status);
        }

        /*
         * Store the swap entry for later use.
         */
        MmSetSavedSwapEntryPage(Page, SwapEntry);

        /*
         * Add the page to the process's working set
         */
        MmInsertRmap(Page, Process, Address);
        /*
         * Finish the operation
         */
        MiSetPageEvent(Process, Address);
        DPRINT("Address 0x%p\n", Address);
        return(STATUS_SUCCESS);
    }

    /*
     * Satisfying a page fault on a map of /Device/PhysicalMemory is easy
     */
    if (Section->AllocationAttributes & SEC_PHYSICALMEMORY)
    {
        MmUnlockSectionSegment(Segment);
        /*
         * Just map the desired physical page
         */
        Page = (PFN_NUMBER)(Offset.QuadPart >> PAGE_SHIFT);
        Status = MmCreateVirtualMappingUnsafe(Process,
                                              PAddress,
                                              Region->Protect,
                                              &Page,
                                              1);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("MmCreateVirtualMappingUnsafe failed, not out of memory\n");
            KeBugCheck(MEMORY_MANAGEMENT);
            return(Status);
        }

        /*
         * Cleanup and release locks
         */
        MiSetPageEvent(Process, Address);
        DPRINT("Address 0x%p\n", Address);
        return(STATUS_SUCCESS);
    }

    /*
     * Get the entry corresponding to the offset within the section
     */
    Entry = MmGetPageEntrySectionSegment(Segment, &Offset);

    if (Entry == 0)
    {
        SWAPENTRY FakeSwapEntry;

        /*
         * If the entry is zero (and it can't change because we have
         * locked the segment) then we need to load the page.
         */

        /*
         * Release all our locks and read in the page from disk
         */
        MmSetPageEntrySectionSegment(Segment, &Offset, MAKE_SWAP_SSE(MM_WAIT_ENTRY));
        MmUnlockSectionSegment(Segment);
        MmCreatePageFileMapping(Process, PAddress, MM_WAIT_ENTRY);
        MmUnlockAddressSpace(AddressSpace);

        if ((Segment->Flags & MM_PAGEFILE_SEGMENT) ||
                ((Offset.QuadPart >= (LONGLONG)PAGE_ROUND_UP(Segment->RawLength.QuadPart) &&
                  (Section->AllocationAttributes & SEC_IMAGE))))
        {
            MI_SET_USAGE(MI_USAGE_SECTION);
            if (Process) MI_SET_PROCESS2(Process->ImageFileName);
            if (!Process) MI_SET_PROCESS2("Kernel Section");
            Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &Page);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("MmRequestPageMemoryConsumer failed (Status %x)\n", Status);
            }

        }
        else
        {
            Status = MiReadPage(MemoryArea, Offset.QuadPart, &Page);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("MiReadPage failed (Status %x)\n", Status);
            }
        }
        if (!NT_SUCCESS(Status))
        {
            /*
             * FIXME: What do we know in this case?
             */
            /*
             * Cleanup and release locks
             */
            MmLockAddressSpace(AddressSpace);
            MiSetPageEvent(Process, Address);
            DPRINT("Address 0x%p\n", Address);
            return(Status);
        }

        /*
         * Mark the offset within the section as having valid, in-memory
         * data
         */
        MmLockAddressSpace(AddressSpace);
        MmLockSectionSegment(Segment);
        Entry = MAKE_SSE(Page << PAGE_SHIFT, 1);
        MmSetPageEntrySectionSegment(Segment, &Offset, Entry);
        MmUnlockSectionSegment(Segment);

        MmDeletePageFileMapping(Process, PAddress, &FakeSwapEntry);
        DPRINT("CreateVirtualMapping Page %x Process %p PAddress %p Attributes %x\n",
               Page, Process, PAddress, Attributes);
        Status = MmCreateVirtualMapping(Process,
                                        PAddress,
                                        Attributes,
                                        &Page,
                                        1);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Unable to create virtual mapping\n");
            KeBugCheck(MEMORY_MANAGEMENT);
        }
        ASSERT(MmIsPagePresent(Process, PAddress));
        MmInsertRmap(Page, Process, Address);

        MiSetPageEvent(Process, Address);
        DPRINT("Address 0x%p\n", Address);
        return(STATUS_SUCCESS);
    }
    else if (IS_SWAP_FROM_SSE(Entry))
    {
        SWAPENTRY SwapEntry;

        SwapEntry = SWAPENTRY_FROM_SSE(Entry);

        /*
        * Release all our locks and read in the page from disk
        */
        MmUnlockSectionSegment(Segment);

        MmUnlockAddressSpace(AddressSpace);
        MI_SET_USAGE(MI_USAGE_SECTION);
        if (Process) MI_SET_PROCESS2(Process->ImageFileName);
        if (!Process) MI_SET_PROCESS2("Kernel Section");
        Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &Page);
        if (!NT_SUCCESS(Status))
        {
            KeBugCheck(MEMORY_MANAGEMENT);
        }

        Status = MmReadFromSwapPage(SwapEntry, Page);
        if (!NT_SUCCESS(Status))
        {
            KeBugCheck(MEMORY_MANAGEMENT);
        }

        /*
         * Relock the address space and segment
         */
        MmLockAddressSpace(AddressSpace);
        MmLockSectionSegment(Segment);

        /*
         * Check the entry. No one should change the status of a page
         * that has a pending page-in.
         */
        Entry1 = MmGetPageEntrySectionSegment(Segment, &Offset);
        if (Entry != Entry1)
        {
            DPRINT1("Someone changed ppte entry while we slept (%x vs %x)\n", Entry, Entry1);
            KeBugCheck(MEMORY_MANAGEMENT);
        }

        /*
         * Mark the offset within the section as having valid, in-memory
         * data
         */
        Entry = MAKE_SSE(Page << PAGE_SHIFT, 1);
        MmSetPageEntrySectionSegment(Segment, &Offset, Entry);
        MmUnlockSectionSegment(Segment);

        /*
         * Save the swap entry.
         */
        MmSetSavedSwapEntryPage(Page, SwapEntry);
        Status = MmCreateVirtualMapping(Process,
                                        PAddress,
                                        Region->Protect,
                                        &Page,
                                        1);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Unable to create virtual mapping\n");
            KeBugCheck(MEMORY_MANAGEMENT);
        }
        MmInsertRmap(Page, Process, Address);
        MiSetPageEvent(Process, Address);
        DPRINT("Address 0x%p\n", Address);
        return(STATUS_SUCCESS);
    }
    else
    {
        /*
         * If the section offset is already in-memory and valid then just
         * take another reference to the page
         */

        Page = PFN_FROM_SSE(Entry);

        MmSharePageEntrySectionSegment(Segment, &Offset);
        MmUnlockSectionSegment(Segment);

        Status = MmCreateVirtualMapping(Process,
                                        PAddress,
                                        Attributes,
                                        &Page,
                                        1);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Unable to create virtual mapping\n");
            KeBugCheck(MEMORY_MANAGEMENT);
        }
        MmInsertRmap(Page, Process, Address);
        MiSetPageEvent(Process, Address);
        DPRINT("Address 0x%p\n", Address);
        return(STATUS_SUCCESS);
    }
}

NTSTATUS
NTAPI
MmAccessFaultSectionView(PMMSUPPORT AddressSpace,
                         MEMORY_AREA* MemoryArea,
                         PVOID Address)
{
    PMM_SECTION_SEGMENT Segment;
    PROS_SECTION_OBJECT Section;
    PFN_NUMBER OldPage;
    PFN_NUMBER NewPage;
    NTSTATUS Status;
    PVOID PAddress;
    LARGE_INTEGER Offset;
    PMM_REGION Region;
    ULONG_PTR Entry;
    PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);
    SWAPENTRY SwapEntry;

    DPRINT("MmAccessFaultSectionView(%p, %p, %p)\n", AddressSpace, MemoryArea, Address);

    /*
     * Check if the page has already been set readwrite
     */
    if (MmGetPageProtect(Process, Address) & PAGE_READWRITE)
    {
        DPRINT("Address 0x%p\n", Address);
        return(STATUS_SUCCESS);
    }

    /*
     * Find the offset of the page
     */
    PAddress = MM_ROUND_DOWN(Address, PAGE_SIZE);
    Offset.QuadPart = (ULONG_PTR)PAddress - MA_GetStartingAddress(MemoryArea)
                      + MemoryArea->Data.SectionData.ViewOffset.QuadPart;

    Segment = MemoryArea->Data.SectionData.Segment;
    Section = MemoryArea->Data.SectionData.Section;
    Region = MmFindRegion((PVOID)MA_GetStartingAddress(MemoryArea),
                          &MemoryArea->Data.SectionData.RegionListHead,
                          Address, NULL);
    ASSERT(Region != NULL);
    /*
     * Lock the segment
     */
    MmLockSectionSegment(Segment);

    OldPage = MmGetPfnForProcess(Process, Address);
    Entry = MmGetPageEntrySectionSegment(Segment, &Offset);

    MmUnlockSectionSegment(Segment);

    /*
     * Check if we are doing COW
     */
    if (!((Segment->WriteCopy) &&
            (Region->Protect == PAGE_READWRITE ||
             Region->Protect == PAGE_EXECUTE_READWRITE)))
    {
        DPRINT("Address 0x%p\n", Address);
        return(STATUS_ACCESS_VIOLATION);
    }

    if (IS_SWAP_FROM_SSE(Entry) ||
            PFN_FROM_SSE(Entry) != OldPage)
    {
        /* This is a private page. We must only change the page protection. */
        MmSetPageProtect(Process, Address, Region->Protect);
        return(STATUS_SUCCESS);
    }

    if(OldPage == 0)
        DPRINT("OldPage == 0!\n");

    /*
     * Get or create a pageop
     */
    MmLockSectionSegment(Segment);
    Entry = MmGetPageEntrySectionSegment(Segment, &Offset);

    /*
     * Wait for any other operations to complete
     */
    if (Entry == SWAPENTRY_FROM_SSE(MM_WAIT_ENTRY))
    {
        MmUnlockSectionSegment(Segment);
        MmUnlockAddressSpace(AddressSpace);
        MiWaitForPageEvent(NULL, NULL);
        /*
         * Restart the operation
         */
        MmLockAddressSpace(AddressSpace);
        DPRINT("Address 0x%p\n", Address);
        return(STATUS_MM_RESTART_OPERATION);
    }

    MmDeleteRmap(OldPage, Process, PAddress);
    MmDeleteVirtualMapping(Process, PAddress, NULL, NULL);
    MmCreatePageFileMapping(Process, PAddress, MM_WAIT_ENTRY);

    /*
     * Release locks now we have the pageop
     */
    MmUnlockSectionSegment(Segment);
    MmUnlockAddressSpace(AddressSpace);

    /*
     * Allocate a page
     */
    MI_SET_USAGE(MI_USAGE_SECTION);
    if (Process) MI_SET_PROCESS2(Process->ImageFileName);
    if (!Process) MI_SET_PROCESS2("Kernel Section");
    Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &NewPage);
    if (!NT_SUCCESS(Status))
    {
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    /*
     * Copy the old page
     */
    MiCopyFromUserPage(NewPage, OldPage);

    MmLockAddressSpace(AddressSpace);

    /*
     * Set the PTE to point to the new page
     */
    MmDeletePageFileMapping(Process, PAddress, &SwapEntry);
    Status = MmCreateVirtualMapping(Process,
                                    PAddress,
                                    Region->Protect,
                                    &NewPage,
                                    1);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("MmCreateVirtualMapping failed, unable to create virtual mapping, not out of memory\n");
        KeBugCheck(MEMORY_MANAGEMENT);
        return(Status);
    }

    /*
     * Unshare the old page.
     */
    DPRINT("Swapping page (Old %x New %x)\n", OldPage, NewPage);
    MmInsertRmap(NewPage, Process, PAddress);
    MmLockSectionSegment(Segment);
    MmUnsharePageEntrySectionSegment(Section, Segment, &Offset, FALSE, FALSE, NULL);
    MmUnlockSectionSegment(Segment);

    MiSetPageEvent(Process, Address);
    DPRINT("Address 0x%p\n", Address);
    return(STATUS_SUCCESS);
}

VOID
MmPageOutDeleteMapping(PVOID Context, PEPROCESS Process, PVOID Address)
{
    MM_SECTION_PAGEOUT_CONTEXT* PageOutContext;
    BOOLEAN WasDirty;
    PFN_NUMBER Page = 0;

    PageOutContext = (MM_SECTION_PAGEOUT_CONTEXT*)Context;
    if (Process)
    {
        MmLockAddressSpace(&Process->Vm);
    }

    MmDeleteVirtualMapping(Process,
                           Address,
                           &WasDirty,
                           &Page);
    if (WasDirty)
    {
        PageOutContext->WasDirty = TRUE;
    }
    if (!PageOutContext->Private)
    {
        MmLockSectionSegment(PageOutContext->Segment);
        MmUnsharePageEntrySectionSegment((PROS_SECTION_OBJECT)PageOutContext->Section,
                                         PageOutContext->Segment,
                                         &PageOutContext->Offset,
                                         PageOutContext->WasDirty,
                                         TRUE,
                                         &PageOutContext->SectionEntry);
        MmUnlockSectionSegment(PageOutContext->Segment);
    }
    if (Process)
    {
        MmUnlockAddressSpace(&Process->Vm);
    }

    if (PageOutContext->Private)
    {
        MmReleasePageMemoryConsumer(MC_USER, Page);
    }
}

NTSTATUS
NTAPI
MmPageOutSectionView(PMMSUPPORT AddressSpace,
                     MEMORY_AREA* MemoryArea,
                     PVOID Address, ULONG_PTR Entry)
{
    PFN_NUMBER Page;
    MM_SECTION_PAGEOUT_CONTEXT Context;
    SWAPENTRY SwapEntry;
    NTSTATUS Status;
#ifndef NEWCC
    ULONGLONG FileOffset;
    PFILE_OBJECT FileObject;
    PROS_SHARED_CACHE_MAP SharedCacheMap = NULL;
    BOOLEAN IsImageSection;
#endif
    BOOLEAN DirectMapped;
    PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);
    KIRQL OldIrql;

    Address = (PVOID)PAGE_ROUND_DOWN(Address);

    /*
     * Get the segment and section.
     */
    Context.Segment = MemoryArea->Data.SectionData.Segment;
    Context.Section = MemoryArea->Data.SectionData.Section;
    Context.SectionEntry = Entry;
    Context.CallingProcess = Process;

    Context.Offset.QuadPart = (ULONG_PTR)Address - MA_GetStartingAddress(MemoryArea)
                              + MemoryArea->Data.SectionData.ViewOffset.QuadPart;

    DirectMapped = FALSE;

    MmLockSectionSegment(Context.Segment);

#ifndef NEWCC
    FileOffset = Context.Offset.QuadPart + Context.Segment->Image.FileOffset;
    IsImageSection = Context.Section->AllocationAttributes & SEC_IMAGE ? TRUE : FALSE;
    FileObject = Context.Section->FileObject;

    if (FileObject != NULL &&
            !(Context.Segment->Image.Characteristics & IMAGE_SCN_MEM_SHARED))
    {
        SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;

        /*
         * If the file system is letting us go directly to the cache and the
         * memory area was mapped at an offset in the file which is page aligned
         * then note this is a direct mapped page.
         */
        if ((FileOffset % PAGE_SIZE) == 0 &&
                (Context.Offset.QuadPart + PAGE_SIZE <= Context.Segment->RawLength.QuadPart || !IsImageSection))
        {
            DirectMapped = TRUE;
        }
    }
#endif


    /*
     * This should never happen since mappings of physical memory are never
     * placed in the rmap lists.
     */
    if (Context.Section->AllocationAttributes & SEC_PHYSICALMEMORY)
    {
        DPRINT1("Trying to page out from physical memory section address 0x%p "
                "process %p\n", Address,
                Process ? Process->UniqueProcessId : 0);
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    /*
     * Get the section segment entry and the physical address.
     */
    if (!MmIsPagePresent(Process, Address))
    {
        DPRINT1("Trying to page out not-present page at (%p,0x%p).\n",
                Process ? Process->UniqueProcessId : 0, Address);
        KeBugCheck(MEMORY_MANAGEMENT);
    }
    Page = MmGetPfnForProcess(Process, Address);
    SwapEntry = MmGetSavedSwapEntryPage(Page);

    /*
     * Check the reference count to ensure this page can be paged out
     */
    if (MmGetReferenceCountPage(Page) != 1)
    {
        DPRINT("Cannot page out locked section page: 0x%lu (RefCount: %lu)\n",
               Page, MmGetReferenceCountPage(Page));
        MmSetPageEntrySectionSegment(Context.Segment, &Context.Offset, Entry);
        MmUnlockSectionSegment(Context.Segment);
        return STATUS_UNSUCCESSFUL;
    }

    /*
     * Prepare the context structure for the rmap delete call.
     */
    MmUnlockSectionSegment(Context.Segment);
    Context.WasDirty = FALSE;
    if (Context.Segment->Image.Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA ||
            IS_SWAP_FROM_SSE(Entry) ||
            PFN_FROM_SSE(Entry) != Page)
    {
        Context.Private = TRUE;
    }
    else
    {
        Context.Private = FALSE;
    }

    /*
     * Take an additional reference to the page or the VACB.
     */
    if (DirectMapped && !Context.Private)
    {
        if(!MiIsPageFromCache(MemoryArea, Context.Offset.QuadPart))
        {
            DPRINT1("Direct mapped non private page is not associated with the cache.\n");
            KeBugCheck(MEMORY_MANAGEMENT);
        }
    }
    else
    {
        OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
        MmReferencePage(Page);
        KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
    }

    MmDeleteAllRmaps(Page, (PVOID)&Context, MmPageOutDeleteMapping);

    /* Since we passed in a surrogate, we'll get back the page entry
     * state in our context.  This is intended to make intermediate
     * decrements of share count not release the wait entry.
     */
    Entry = Context.SectionEntry;

    /*
     * If this wasn't a private page then we should have reduced the entry to
     * zero by deleting all the rmaps.
     */
    if (!Context.Private && Entry != 0)
    {
        if (!(Context.Segment->Flags & MM_PAGEFILE_SEGMENT) &&
                !(Context.Segment->Image.Characteristics & IMAGE_SCN_MEM_SHARED))
        {
            KeBugCheckEx(MEMORY_MANAGEMENT, Entry, (ULONG_PTR)Process, (ULONG_PTR)Address, 0);
        }
    }

    /*
     * If the page wasn't dirty then we can just free it as for a readonly page.
     * Since we unmapped all the mappings above we know it will not suddenly
     * become dirty.
     * If the page is from a pagefile section and has no swap entry,
     * we can't free the page at this point.
     */
    SwapEntry = MmGetSavedSwapEntryPage(Page);
    if (Context.Segment->Flags & MM_PAGEFILE_SEGMENT)
    {
        if (Context.Private)
        {
            DPRINT1("Found a %s private page (address %p) in a pagefile segment.\n",
                    Context.WasDirty ? "dirty" : "clean", Address);
            KeBugCheckEx(MEMORY_MANAGEMENT, SwapEntry, (ULONG_PTR)Process, (ULONG_PTR)Address, 0);
        }
        if (!Context.WasDirty && SwapEntry != 0)
        {
            MmSetSavedSwapEntryPage(Page, 0);
            MmLockSectionSegment(Context.Segment);
            MmSetPageEntrySectionSegment(Context.Segment, &Context.Offset, MAKE_SWAP_SSE(SwapEntry));
            MmUnlockSectionSegment(Context.Segment);
            MmReleasePageMemoryConsumer(MC_USER, Page);
            MiSetPageEvent(NULL, NULL);
            return(STATUS_SUCCESS);
        }
    }
    else if (Context.Segment->Image.Characteristics & IMAGE_SCN_MEM_SHARED)
    {
        if (Context.Private)
        {
            DPRINT1("Found a %s private page (address %p) in a shared section segment.\n",
                    Context.WasDirty ? "dirty" : "clean", Address);
            KeBugCheckEx(MEMORY_MANAGEMENT, Page, (ULONG_PTR)Process, (ULONG_PTR)Address, 0);
        }
        if (!Context.WasDirty || SwapEntry != 0)
        {
            MmSetSavedSwapEntryPage(Page, 0);
            if (SwapEntry != 0)
            {
                MmLockSectionSegment(Context.Segment);
                MmSetPageEntrySectionSegment(Context.Segment, &Context.Offset, MAKE_SWAP_SSE(SwapEntry));
                MmUnlockSectionSegment(Context.Segment);
            }
            MmReleasePageMemoryConsumer(MC_USER, Page);
            MiSetPageEvent(NULL, NULL);
            return(STATUS_SUCCESS);
        }
    }
    else if (!Context.Private && DirectMapped)
    {
        if (SwapEntry != 0)
        {
            DPRINT1("Found a swapentry for a non private and direct mapped page (address %p)\n",
                    Address);
            KeBugCheckEx(MEMORY_MANAGEMENT, STATUS_UNSUCCESSFUL, SwapEntry, (ULONG_PTR)Process, (ULONG_PTR)Address);
        }
#ifndef NEWCC
        Status = CcRosUnmapVacb(SharedCacheMap, FileOffset, FALSE);
#else
        Status = STATUS_SUCCESS;
#endif
#ifndef NEWCC
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("CcRosUnmapVacb failed, status = %x\n", Status);
            KeBugCheckEx(MEMORY_MANAGEMENT, Status, (ULONG_PTR)SharedCacheMap, (ULONG_PTR)FileOffset, (ULONG_PTR)Address);
        }
#endif
        MiSetPageEvent(NULL, NULL);
        return(STATUS_SUCCESS);
    }
    else if (!Context.WasDirty && !DirectMapped && !Context.Private)
    {
        if (SwapEntry != 0)
        {
            DPRINT1("Found a swap entry for a non dirty, non private and not direct mapped page (address %p)\n",
                    Address);
            KeBugCheckEx(MEMORY_MANAGEMENT, SwapEntry, Page, (ULONG_PTR)Process, (ULONG_PTR)Address);
        }
        MmReleasePageMemoryConsumer(MC_USER, Page);
        MiSetPageEvent(NULL, NULL);
        return(STATUS_SUCCESS);
    }
    else if (!Context.WasDirty && Context.Private && SwapEntry != 0)
    {
        DPRINT("Not dirty and private and not swapped (%p:%p)\n", Process, Address);
        MmSetSavedSwapEntryPage(Page, 0);
        MmLockAddressSpace(AddressSpace);
        Status = MmCreatePageFileMapping(Process,
                                         Address,
                                         SwapEntry);
        MmUnlockAddressSpace(AddressSpace);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Status %x Swapping out %p:%p\n", Status, Process, Address);
            KeBugCheckEx(MEMORY_MANAGEMENT, Status, (ULONG_PTR)Process, (ULONG_PTR)Address, SwapEntry);
        }
        MmReleasePageMemoryConsumer(MC_USER, Page);
        MiSetPageEvent(NULL, NULL);
        return(STATUS_SUCCESS);
    }

    /*
     * If necessary, allocate an entry in the paging file for this page
     */
    if (SwapEntry == 0)
    {
        SwapEntry = MmAllocSwapPage();
        if (SwapEntry == 0)
        {
            MmShowOutOfSpaceMessagePagingFile();
            MmLockAddressSpace(AddressSpace);
            /*
             * For private pages restore the old mappings.
             */
            if (Context.Private)
            {
                Status = MmCreateVirtualMapping(Process,
                                                Address,
                                                MemoryArea->Protect,
                                                &Page,
                                                1);
                MmSetDirtyPage(Process, Address);
                MmInsertRmap(Page,
                             Process,
                             Address);
            }
            else
            {
                ULONG_PTR OldEntry;
                /*
                 * For non-private pages if the page wasn't direct mapped then
                 * set it back into the section segment entry so we don't loose
                 * our copy. Otherwise it will be handled by the cache manager.
                 */
                Status = MmCreateVirtualMapping(Process,
                                                Address,
                                                MemoryArea->Protect,
                                                &Page,
                                                1);
                MmSetDirtyPage(Process, Address);
                MmInsertRmap(Page,
                             Process,
                             Address);
                // If we got here, the previous entry should have been a wait
                Entry = MAKE_SSE(Page << PAGE_SHIFT, 1);
                MmLockSectionSegment(Context.Segment);
                OldEntry = MmGetPageEntrySectionSegment(Context.Segment, &Context.Offset);
                ASSERT(OldEntry == 0 || OldEntry == MAKE_SWAP_SSE(MM_WAIT_ENTRY));
                MmSetPageEntrySectionSegment(Context.Segment, &Context.Offset, Entry);
                MmUnlockSectionSegment(Context.Segment);
            }
            MmUnlockAddressSpace(AddressSpace);
            MiSetPageEvent(NULL, NULL);
            return(STATUS_PAGEFILE_QUOTA);
        }
    }

    /*
     * Write the page to the pagefile
     */
    Status = MmWriteToSwapPage(SwapEntry, Page);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("MM: Failed to write to swap page (Status was 0x%.8X)\n",
                Status);
        /*
         * As above: undo our actions.
         * FIXME: Also free the swap page.
         */
        MmLockAddressSpace(AddressSpace);
        if (Context.Private)
        {
            Status = MmCreateVirtualMapping(Process,
                                            Address,
                                            MemoryArea->Protect,
                                            &Page,
                                            1);
            MmSetDirtyPage(Process, Address);
            MmInsertRmap(Page,
                         Process,
                         Address);
        }
        else
        {
            Status = MmCreateVirtualMapping(Process,
                                            Address,
                                            MemoryArea->Protect,
                                            &Page,
                                            1);
            MmSetDirtyPage(Process, Address);
            MmInsertRmap(Page,
                         Process,
                         Address);
            Entry = MAKE_SSE(Page << PAGE_SHIFT, 1);
            MmSetPageEntrySectionSegment(Context.Segment, &Context.Offset, Entry);
        }
        MmUnlockAddressSpace(AddressSpace);
        MiSetPageEvent(NULL, NULL);
        return(STATUS_UNSUCCESSFUL);
    }

    /*
     * Otherwise we have succeeded.
     */
    DPRINT("MM: Wrote section page 0x%.8X to swap!\n", Page << PAGE_SHIFT);
    MmSetSavedSwapEntryPage(Page, 0);
    if (Context.Segment->Flags & MM_PAGEFILE_SEGMENT ||
            Context.Segment->Image.Characteristics & IMAGE_SCN_MEM_SHARED)
    {
        MmLockSectionSegment(Context.Segment);
        MmSetPageEntrySectionSegment(Context.Segment, &Context.Offset, MAKE_SWAP_SSE(SwapEntry));
        MmUnlockSectionSegment(Context.Segment);
    }
    else
    {
        MmReleasePageMemoryConsumer(MC_USER, Page);
    }

    if (Context.Private)
    {
        MmLockAddressSpace(AddressSpace);
        MmLockSectionSegment(Context.Segment);
        Status = MmCreatePageFileMapping(Process,
                                         Address,
                                         SwapEntry);
        /* We had placed a wait entry upon entry ... replace it before leaving */
        MmSetPageEntrySectionSegment(Context.Segment, &Context.Offset, Entry);
        MmUnlockSectionSegment(Context.Segment);
        MmUnlockAddressSpace(AddressSpace);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Status %x Creating page file mapping for %p:%p\n", Status, Process, Address);
            KeBugCheckEx(MEMORY_MANAGEMENT, Status, (ULONG_PTR)Process, (ULONG_PTR)Address, SwapEntry);
        }
    }
    else
    {
        MmLockAddressSpace(AddressSpace);
        MmLockSectionSegment(Context.Segment);
        Entry = MAKE_SWAP_SSE(SwapEntry);
        /* We had placed a wait entry upon entry ... replace it before leaving */
        MmSetPageEntrySectionSegment(Context.Segment, &Context.Offset, Entry);
        MmUnlockSectionSegment(Context.Segment);
        MmUnlockAddressSpace(AddressSpace);
    }

    MiSetPageEvent(NULL, NULL);
    return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MmWritePageSectionView(PMMSUPPORT AddressSpace,
                       PMEMORY_AREA MemoryArea,
                       PVOID Address,
                       ULONG PageEntry)
{
    LARGE_INTEGER Offset;
    PROS_SECTION_OBJECT Section;
    PMM_SECTION_SEGMENT Segment;
    PFN_NUMBER Page;
    SWAPENTRY SwapEntry;
    ULONG_PTR Entry;
    BOOLEAN Private;
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
#ifndef NEWCC
    PROS_SHARED_CACHE_MAP SharedCacheMap = NULL;
#endif
    BOOLEAN DirectMapped;
    BOOLEAN IsImageSection;
    PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);

    Address = (PVOID)PAGE_ROUND_DOWN(Address);

    Offset.QuadPart = (ULONG_PTR)Address - MA_GetStartingAddress(MemoryArea)
                      + MemoryArea->Data.SectionData.ViewOffset.QuadPart;

    /*
     * Get the segment and section.
     */
    Segment = MemoryArea->Data.SectionData.Segment;
    Section = MemoryArea->Data.SectionData.Section;
    IsImageSection = Section->AllocationAttributes & SEC_IMAGE ? TRUE : FALSE;

    FileObject = Section->FileObject;
    DirectMapped = FALSE;
    if (FileObject != NULL &&
            !(Segment->Image.Characteristics & IMAGE_SCN_MEM_SHARED))
    {
#ifndef NEWCC
        SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
#endif

        /*
         * If the file system is letting us go directly to the cache and the
         * memory area was mapped at an offset in the file which is page aligned
         * then note this is a direct mapped page.
         */
        if (((Offset.QuadPart + Segment->Image.FileOffset) % PAGE_SIZE) == 0 &&
                (Offset.QuadPart + PAGE_SIZE <= Segment->RawLength.QuadPart || !IsImageSection))
        {
            DirectMapped = TRUE;
        }
    }

    /*
     * This should never happen since mappings of physical memory are never
     * placed in the rmap lists.
     */
    if (Section->AllocationAttributes & SEC_PHYSICALMEMORY)
    {
        DPRINT1("Trying to write back page from physical memory mapped at %p "
                "process %p\n", Address,
                Process ? Process->UniqueProcessId : 0);
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    /*
     * Get the section segment entry and the physical address.
     */
    Entry = MmGetPageEntrySectionSegment(Segment, &Offset);
    if (!MmIsPagePresent(Process, Address))
    {
        DPRINT1("Trying to page out not-present page at (%p,0x%p).\n",
                Process ? Process->UniqueProcessId : 0, Address);
        KeBugCheck(MEMORY_MANAGEMENT);
    }
    Page = MmGetPfnForProcess(Process, Address);
    SwapEntry = MmGetSavedSwapEntryPage(Page);

    /*
     * Check for a private (COWed) page.
     */
    if (Segment->Image.Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA ||
            IS_SWAP_FROM_SSE(Entry) ||
            PFN_FROM_SSE(Entry) != Page)
    {
        Private = TRUE;
    }
    else
    {
        Private = FALSE;
    }

    /*
     * Speculatively set all mappings of the page to clean.
     */
    MmSetCleanAllRmaps(Page);

    /*
     * If this page was direct mapped from the cache then the cache manager
     * will take care of writing it back to disk.
     */
    if (DirectMapped && !Private)
    {
        //LARGE_INTEGER SOffset;
        ASSERT(SwapEntry == 0);
        //SOffset.QuadPart = Offset.QuadPart + Segment->Image.FileOffset;
#ifndef NEWCC
        CcRosMarkDirtyVacb(SharedCacheMap, Offset.QuadPart);
#endif
        MmLockSectionSegment(Segment);
        MmSetPageEntrySectionSegment(Segment, &Offset, PageEntry);
        MmUnlockSectionSegment(Segment);
        MiSetPageEvent(NULL, NULL);
        return(STATUS_SUCCESS);
    }

    /*
     * If necessary, allocate an entry in the paging file for this page
     */
    if (SwapEntry == 0)
    {
        SwapEntry = MmAllocSwapPage();
        if (SwapEntry == 0)
        {
            MmSetDirtyAllRmaps(Page);
            MiSetPageEvent(NULL, NULL);
            return(STATUS_PAGEFILE_QUOTA);
        }
        MmSetSavedSwapEntryPage(Page, SwapEntry);
    }

    /*
     * Write the page to the pagefile
     */
    Status = MmWriteToSwapPage(SwapEntry, Page);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("MM: Failed to write to swap page (Status was 0x%.8X)\n",
                Status);
        MmSetDirtyAllRmaps(Page);
        MiSetPageEvent(NULL, NULL);
        return(STATUS_UNSUCCESSFUL);
    }

    /*
     * Otherwise we have succeeded.
     */
    DPRINT("MM: Wrote section page 0x%.8X to swap!\n", Page << PAGE_SHIFT);
    MiSetPageEvent(NULL, NULL);
    return(STATUS_SUCCESS);
}

static VOID
MmAlterViewAttributes(PMMSUPPORT AddressSpace,
                      PVOID BaseAddress,
                      SIZE_T RegionSize,
                      ULONG OldType,
                      ULONG OldProtect,
                      ULONG NewType,
                      ULONG NewProtect)
{
    PMEMORY_AREA MemoryArea;
    PMM_SECTION_SEGMENT Segment;
    BOOLEAN DoCOW = FALSE;
    ULONG i;
    PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);

    MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, BaseAddress);
    ASSERT(MemoryArea != NULL);
    Segment = MemoryArea->Data.SectionData.Segment;
    MmLockSectionSegment(Segment);

    if ((Segment->WriteCopy) &&
            (NewProtect == PAGE_READWRITE || NewProtect == PAGE_EXECUTE_READWRITE))
    {
        DoCOW = TRUE;
    }

    if (OldProtect != NewProtect)
    {
        for (i = 0; i < PAGE_ROUND_UP(RegionSize) / PAGE_SIZE; i++)
        {
            SWAPENTRY SwapEntry;
            PVOID Address = (char*)BaseAddress + (i * PAGE_SIZE);
            ULONG Protect = NewProtect;

            /* Wait for a wait entry to disappear */
            do
            {
                MmGetPageFileMapping(Process, Address, &SwapEntry);
                if (SwapEntry != MM_WAIT_ENTRY)
                    break;
                MiWaitForPageEvent(Process, Address);
            }
            while (TRUE);

            /*
             * If we doing COW for this segment then check if the page is
             * already private.
             */
            if (DoCOW && MmIsPagePresent(Process, Address))
            {
                LARGE_INTEGER Offset;
                ULONG_PTR Entry;
                PFN_NUMBER Page;

                Offset.QuadPart = (ULONG_PTR)Address - MA_GetStartingAddress(MemoryArea)
                                  + MemoryArea->Data.SectionData.ViewOffset.QuadPart;
                Entry = MmGetPageEntrySectionSegment(Segment, &Offset);
                /*
                 * An MM_WAIT_ENTRY is ok in this case...  It'll just count as
                 * IS_SWAP_FROM_SSE and we'll do the right thing.
                 */
                Page = MmGetPfnForProcess(Process, Address);

                Protect = PAGE_READONLY;
                if (Segment->Image.Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA ||
                        IS_SWAP_FROM_SSE(Entry) ||
                        PFN_FROM_SSE(Entry) != Page)
                {
                    Protect = NewProtect;
                }
            }

            if (MmIsPagePresent(Process, Address) || MmIsDisabledPage(Process, Address))
            {
                MmSetPageProtect(Process, Address,
                                 Protect);
            }
        }
    }

    MmUnlockSectionSegment(Segment);
}

NTSTATUS
NTAPI
MmProtectSectionView(PMMSUPPORT AddressSpace,
                     PMEMORY_AREA MemoryArea,
                     PVOID BaseAddress,
                     SIZE_T Length,
                     ULONG Protect,
                     PULONG OldProtect)
{
    PMM_REGION Region;
    NTSTATUS Status;
    ULONG_PTR MaxLength;

    MaxLength = MA_GetEndingAddress(MemoryArea) - (ULONG_PTR)BaseAddress;
    if (Length > MaxLength)
        Length = (ULONG)MaxLength;

    Region = MmFindRegion((PVOID)MA_GetStartingAddress(MemoryArea),
                          &MemoryArea->Data.SectionData.RegionListHead,
                          BaseAddress, NULL);
    ASSERT(Region != NULL);

    if ((MemoryArea->Flags & SEC_NO_CHANGE) &&
            Region->Protect != Protect)
    {
        return STATUS_INVALID_PAGE_PROTECTION;
    }

    *OldProtect = Region->Protect;
    Status = MmAlterRegion(AddressSpace, (PVOID)MA_GetStartingAddress(MemoryArea),
                           &MemoryArea->Data.SectionData.RegionListHead,
                           BaseAddress, Length, Region->Type, Protect,
                           MmAlterViewAttributes);

    return(Status);
}

NTSTATUS NTAPI
MmQuerySectionView(PMEMORY_AREA MemoryArea,
                   PVOID Address,
                   PMEMORY_BASIC_INFORMATION Info,
                   PSIZE_T ResultLength)
{
    PMM_REGION Region;
    PVOID RegionBaseAddress;
    PROS_SECTION_OBJECT Section;
    PMM_SECTION_SEGMENT Segment;

    Region = MmFindRegion((PVOID)MA_GetStartingAddress(MemoryArea),
                          &MemoryArea->Data.SectionData.RegionListHead,
                          Address, &RegionBaseAddress);
    if (Region == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Section = MemoryArea->Data.SectionData.Section;
    if (Section->AllocationAttributes & SEC_IMAGE)
    {
        Segment = MemoryArea->Data.SectionData.Segment;
        Info->AllocationBase = (PUCHAR)MA_GetStartingAddress(MemoryArea) - Segment->Image.VirtualAddress;
        Info->Type = MEM_IMAGE;
    }
    else
    {
        Info->AllocationBase = (PVOID)MA_GetStartingAddress(MemoryArea);
        Info->Type = MEM_MAPPED;
    }
    Info->BaseAddress = RegionBaseAddress;
    Info->AllocationProtect = MemoryArea->Protect;
    Info->RegionSize = Region->Length;
    Info->State = MEM_COMMIT;
    Info->Protect = Region->Protect;

    *ResultLength = sizeof(MEMORY_BASIC_INFORMATION);
    return(STATUS_SUCCESS);
}

VOID
NTAPI
MmpFreePageFileSegment(PMM_SECTION_SEGMENT Segment)
{
    ULONG Length;
    LARGE_INTEGER Offset;
    ULONG_PTR Entry;
    SWAPENTRY SavedSwapEntry;
    PFN_NUMBER Page;

    Page = 0;

    MmLockSectionSegment(Segment);

    Length = PAGE_ROUND_UP(Segment->Length.QuadPart);
    for (Offset.QuadPart = 0; Offset.QuadPart < Length; Offset.QuadPart += PAGE_SIZE)
    {
        Entry = MmGetPageEntrySectionSegment(Segment, &Offset);
        if (Entry)
        {
            MmSetPageEntrySectionSegment(Segment, &Offset, 0);
            if (IS_SWAP_FROM_SSE(Entry))
            {
                MmFreeSwapPage(SWAPENTRY_FROM_SSE(Entry));
            }
            else
            {
                Page = PFN_FROM_SSE(Entry);
                SavedSwapEntry = MmGetSavedSwapEntryPage(Page);
                if (SavedSwapEntry != 0)
                {
                    MmSetSavedSwapEntryPage(Page, 0);
                    MmFreeSwapPage(SavedSwapEntry);
                }
                MmReleasePageMemoryConsumer(MC_USER, Page);
            }
        }
    }

    MmUnlockSectionSegment(Segment);
}

VOID NTAPI
MmpDeleteSection(PVOID ObjectBody)
{
    PROS_SECTION_OBJECT Section = (PROS_SECTION_OBJECT)ObjectBody;

    /* Check if it's an ARM3, or ReactOS section */
    if (!MiIsRosSectionObject(Section))
    {
        MiDeleteARM3Section(ObjectBody);
        return;
    }

    DPRINT("MmpDeleteSection(ObjectBody %p)\n", ObjectBody);
    if (Section->AllocationAttributes & SEC_IMAGE)
    {
        ULONG i;
        ULONG NrSegments;
        ULONG RefCount;
        PMM_SECTION_SEGMENT SectionSegments;

        /*
         * NOTE: Section->ImageSection can be NULL for short time
         * during the section creating. If we fail for some reason
         * until the image section is properly initialized we shouldn't
         * process further here.
         */
        if (Section->ImageSection == NULL)
            return;

        SectionSegments = Section->ImageSection->Segments;
        NrSegments = Section->ImageSection->NrSegments;

        for (i = 0; i < NrSegments; i++)
        {
            if (SectionSegments[i].Image.Characteristics & IMAGE_SCN_MEM_SHARED)
            {
                MmLockSectionSegment(&SectionSegments[i]);
            }
            RefCount = InterlockedDecrementUL(&SectionSegments[i].ReferenceCount);
            if (SectionSegments[i].Image.Characteristics & IMAGE_SCN_MEM_SHARED)
            {
                MmUnlockSectionSegment(&SectionSegments[i]);
                if (RefCount == 0)
                {
                    MmpFreePageFileSegment(&SectionSegments[i]);
                }
            }
        }
    }
#ifdef NEWCC
    else if (Section->Segment && Section->Segment->Flags & MM_DATAFILE_SEGMENT)
    {
        ULONG RefCount = 0;
        PMM_SECTION_SEGMENT Segment = Section->Segment;

        if (Segment &&
                (RefCount = InterlockedDecrementUL(&Segment->ReferenceCount)) == 0)
        {
            DPRINT("Freeing section segment\n");
            Section->Segment = NULL;
            MmFinalizeSegment(Segment);
        }
        else
        {
            DPRINT("RefCount %d\n", RefCount);
        }
    }
#endif
    else
    {
        /*
         * NOTE: Section->Segment can be NULL for short time
         * during the section creating.
         */
        if (Section->Segment == NULL)
            return;

        if (Section->Segment->Flags & MM_PAGEFILE_SEGMENT)
        {
            MmpFreePageFileSegment(Section->Segment);
            MmFreePageTablesSectionSegment(Section->Segment, NULL);
            ExFreePool(Section->Segment);
            Section->Segment = NULL;
        }
        else
        {
            (void)InterlockedDecrementUL(&Section->Segment->ReferenceCount);
        }
    }
    if (Section->FileObject != NULL)
    {
#ifndef NEWCC
        CcRosDereferenceCache(Section->FileObject);
#endif
        ObDereferenceObject(Section->FileObject);
        Section->FileObject = NULL;
    }
}

VOID NTAPI
MmpCloseSection(IN PEPROCESS Process OPTIONAL,
                IN PVOID Object,
                IN ACCESS_MASK GrantedAccess,
                IN ULONG ProcessHandleCount,
                IN ULONG SystemHandleCount)
{
    DPRINT("MmpCloseSection(OB %p, HC %lu)\n", Object, ProcessHandleCount);
}

NTSTATUS
INIT_FUNCTION
NTAPI
MmCreatePhysicalMemorySection(VOID)
{
    PROS_SECTION_OBJECT PhysSection;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obj;
    UNICODE_STRING Name = RTL_CONSTANT_STRING(L"\\Device\\PhysicalMemory");
    LARGE_INTEGER SectionSize;
    HANDLE Handle;

    /*
     * Create the section mapping physical memory
     */
    SectionSize.QuadPart = 0xFFFFFFFF;
    InitializeObjectAttributes(&Obj,
                               &Name,
                               OBJ_PERMANENT,
                               NULL,
                               NULL);
    Status = MmCreateSection((PVOID)&PhysSection,
                             SECTION_ALL_ACCESS,
                             &Obj,
                             &SectionSize,
                             PAGE_EXECUTE_READWRITE,
                             SEC_PHYSICALMEMORY,
                             NULL,
                             NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create PhysicalMemory section\n");
        KeBugCheck(MEMORY_MANAGEMENT);
    }
    Status = ObInsertObject(PhysSection,
                            NULL,
                            SECTION_ALL_ACCESS,
                            0,
                            NULL,
                            &Handle);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(PhysSection);
    }
    ObCloseHandle(Handle, KernelMode);
    PhysSection->AllocationAttributes |= SEC_PHYSICALMEMORY;
    PhysSection->Segment->Flags &= ~MM_PAGEFILE_SEGMENT;

    return(STATUS_SUCCESS);
}

NTSTATUS
INIT_FUNCTION
NTAPI
MmInitSectionImplementation(VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;

    DPRINT("Creating Section Object Type\n");

    /* Initialize the section based root */
    ASSERT(MmSectionBasedRoot.NumberGenericTableElements == 0);
    MmSectionBasedRoot.BalancedRoot.u1.Parent = &MmSectionBasedRoot.BalancedRoot;

    /* Initialize the Section object type  */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Section");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultPagedPoolCharge = sizeof(ROS_SECTION_OBJECT);
    ObjectTypeInitializer.PoolType = PagedPool;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    ObjectTypeInitializer.GenericMapping = MmpSectionMapping;
    ObjectTypeInitializer.DeleteProcedure = MmpDeleteSection;
    ObjectTypeInitializer.CloseProcedure = MmpCloseSection;
    ObjectTypeInitializer.ValidAccessMask = SECTION_ALL_ACCESS;
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &MmSectionObjectType);

    MmCreatePhysicalMemorySection();

    return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MmCreatePageFileSection(PROS_SECTION_OBJECT *SectionObject,
                        ACCESS_MASK DesiredAccess,
                        POBJECT_ATTRIBUTES ObjectAttributes,
                        PLARGE_INTEGER UMaximumSize,
                        ULONG SectionPageProtection,
                        ULONG AllocationAttributes)
/*
 * Create a section which is backed by the pagefile
 */
{
    LARGE_INTEGER MaximumSize;
    PROS_SECTION_OBJECT Section;
    PMM_SECTION_SEGMENT Segment;
    NTSTATUS Status;

    if (UMaximumSize == NULL)
    {
        DPRINT1("MmCreatePageFileSection: (UMaximumSize == NULL)\n");
        return(STATUS_INVALID_PARAMETER);
    }
    MaximumSize = *UMaximumSize;

    /*
     * Create the section
     */
    Status = ObCreateObject(ExGetPreviousMode(),
                            MmSectionObjectType,
                            ObjectAttributes,
                            ExGetPreviousMode(),
                            NULL,
                            sizeof(ROS_SECTION_OBJECT),
                            0,
                            0,
                            (PVOID*)(PVOID)&Section);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("MmCreatePageFileSection: failed to create object (0x%lx)\n", Status);
        return(Status);
    }

    /*
     * Initialize it
     */
    RtlZeroMemory(Section, sizeof(ROS_SECTION_OBJECT));
    Section->Type = 'SC';
    Section->Size = 'TN';
    Section->SectionPageProtection = SectionPageProtection;
    Section->AllocationAttributes = AllocationAttributes;
    Section->MaximumSize = MaximumSize;
    Segment = ExAllocatePoolWithTag(NonPagedPool, sizeof(MM_SECTION_SEGMENT),
                                    TAG_MM_SECTION_SEGMENT);
    if (Segment == NULL)
    {
        ObDereferenceObject(Section);
        return(STATUS_NO_MEMORY);
    }
    RtlZeroMemory(Segment, sizeof(MM_SECTION_SEGMENT));
    Section->Segment = Segment;
    Segment->ReferenceCount = 1;
    ExInitializeFastMutex(&Segment->Lock);
    Segment->Image.FileOffset = 0;
    Segment->Protection = SectionPageProtection;
    Segment->RawLength.QuadPart = MaximumSize.u.LowPart;
    Segment->Length.QuadPart = PAGE_ROUND_UP(MaximumSize.u.LowPart);
    Segment->Flags = MM_PAGEFILE_SEGMENT;
    Segment->WriteCopy = FALSE;
    Segment->Image.VirtualAddress = 0;
    Segment->Image.Characteristics = 0;
    *SectionObject = Section;
    MiInitializeSectionPageTable(Segment);
    return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MmCreateDataFileSection(PROS_SECTION_OBJECT *SectionObject,
                        ACCESS_MASK DesiredAccess,
                        POBJECT_ATTRIBUTES ObjectAttributes,
                        PLARGE_INTEGER UMaximumSize,
                        ULONG SectionPageProtection,
                        ULONG AllocationAttributes,
                        HANDLE FileHandle)
/*
 * Create a section backed by a data file
 */
{
    PROS_SECTION_OBJECT Section;
    NTSTATUS Status;
    LARGE_INTEGER MaximumSize;
    PFILE_OBJECT FileObject;
    PMM_SECTION_SEGMENT Segment;
    ULONG FileAccess;
    IO_STATUS_BLOCK Iosb;
    LARGE_INTEGER Offset;
    CHAR Buffer;
    FILE_STANDARD_INFORMATION FileInfo;
    ULONG Length;

    /*
     * Create the section
     */
    Status = ObCreateObject(ExGetPreviousMode(),
                            MmSectionObjectType,
                            ObjectAttributes,
                            ExGetPreviousMode(),
                            NULL,
                            sizeof(ROS_SECTION_OBJECT),
                            0,
                            0,
                            (PVOID*)&Section);
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }
    /*
     * Initialize it
     */
    RtlZeroMemory(Section, sizeof(ROS_SECTION_OBJECT));
    Section->Type = 'SC';
    Section->Size = 'TN';
    Section->SectionPageProtection = SectionPageProtection;
    Section->AllocationAttributes = AllocationAttributes;

    /*
     * Reference the file handle
     */
    FileAccess = MiArm3GetCorrectFileAccessMask(SectionPageProtection);
    Status = ObReferenceObjectByHandle(FileHandle,
                                       FileAccess,
                                       IoFileObjectType,
                                       ExGetPreviousMode(),
                                       (PVOID*)(PVOID)&FileObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(Section);
        return(Status);
    }

    /*
     * FIXME: This is propably not entirely correct. We can't look into
     * the standard FCB header because it might not be initialized yet
     * (as in case of the EXT2FS driver by Manoj Paul Joseph where the
     * standard file information is filled on first request).
     */
    Status = IoQueryFileInformation(FileObject,
                                    FileStandardInformation,
                                    sizeof(FILE_STANDARD_INFORMATION),
                                    &FileInfo,
                                    &Length);
    Iosb.Information = Length;
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(Section);
        ObDereferenceObject(FileObject);
        return Status;
    }

    /*
     * FIXME: Revise this once a locking order for file size changes is
     * decided
     */
    if ((UMaximumSize != NULL) && (UMaximumSize->QuadPart != 0))
    {
        MaximumSize = *UMaximumSize;
    }
    else
    {
        MaximumSize = FileInfo.EndOfFile;
        /* Mapping zero-sized files isn't allowed. */
        if (MaximumSize.QuadPart == 0)
        {
            ObDereferenceObject(Section);
            ObDereferenceObject(FileObject);
            return STATUS_FILE_INVALID;
        }
    }

    if (MaximumSize.QuadPart > FileInfo.EndOfFile.QuadPart)
    {
        Status = IoSetInformation(FileObject,
                                  FileAllocationInformation,
                                  sizeof(LARGE_INTEGER),
                                  &MaximumSize);
        if (!NT_SUCCESS(Status))
        {
            ObDereferenceObject(Section);
            ObDereferenceObject(FileObject);
            return(STATUS_SECTION_NOT_EXTENDED);
        }
    }

    if (FileObject->SectionObjectPointer == NULL ||
            FileObject->SectionObjectPointer->SharedCacheMap == NULL)
    {
        /*
         * Read a bit so caching is initiated for the file object.
         * This is only needed because MiReadPage currently cannot
         * handle non-cached streams.
         */
        Offset.QuadPart = 0;
        Status = ZwReadFile(FileHandle,
                            NULL,
                            NULL,
                            NULL,
                            &Iosb,
                            &Buffer,
                            sizeof (Buffer),
                            &Offset,
                            0);
        if (!NT_SUCCESS(Status) && (Status != STATUS_END_OF_FILE))
        {
            ObDereferenceObject(Section);
            ObDereferenceObject(FileObject);
            return(Status);
        }
        if (FileObject->SectionObjectPointer == NULL ||
                FileObject->SectionObjectPointer->SharedCacheMap == NULL)
        {
            /* FIXME: handle this situation */
            ObDereferenceObject(Section);
            ObDereferenceObject(FileObject);
            return STATUS_INVALID_PARAMETER;
        }
    }

    /*
     * Lock the file
     */
    Status = MmspWaitForFileLock(FileObject);
    if (Status != STATUS_SUCCESS)
    {
        ObDereferenceObject(Section);
        ObDereferenceObject(FileObject);
        return(Status);
    }

    /*
     * If this file hasn't been mapped as a data file before then allocate a
     * section segment to describe the data file mapping
     */
    if (FileObject->SectionObjectPointer->DataSectionObject == NULL)
    {
        Segment = ExAllocatePoolWithTag(NonPagedPool, sizeof(MM_SECTION_SEGMENT),
                                        TAG_MM_SECTION_SEGMENT);
        if (Segment == NULL)
        {
            //KeSetEvent((PVOID)&FileObject->Lock, IO_NO_INCREMENT, FALSE);
            ObDereferenceObject(Section);
            ObDereferenceObject(FileObject);
            return(STATUS_NO_MEMORY);
        }
        Section->Segment = Segment;
        Segment->ReferenceCount = 1;
        ExInitializeFastMutex(&Segment->Lock);
        /*
         * Set the lock before assigning the segment to the file object
         */
        ExAcquireFastMutex(&Segment->Lock);
        FileObject->SectionObjectPointer->DataSectionObject = (PVOID)Segment;

        Segment->Image.FileOffset = 0;
        Segment->Protection = SectionPageProtection;
        Segment->Flags = MM_DATAFILE_SEGMENT;
        Segment->Image.Characteristics = 0;
        Segment->WriteCopy = (SectionPageProtection & (PAGE_WRITECOPY | PAGE_EXECUTE_WRITECOPY));
        if (AllocationAttributes & SEC_RESERVE)
        {
            Segment->Length.QuadPart = Segment->RawLength.QuadPart = 0;
        }
        else
        {
            Segment->RawLength.QuadPart = MaximumSize.QuadPart;
            Segment->Length.QuadPart = PAGE_ROUND_UP(Segment->RawLength.QuadPart);
        }
        Segment->Image.VirtualAddress = 0;
        Segment->Locked = TRUE;
        MiInitializeSectionPageTable(Segment);
    }
    else
    {
        /*
         * If the file is already mapped as a data file then we may need
         * to extend it
         */
        Segment =
            (PMM_SECTION_SEGMENT)FileObject->SectionObjectPointer->
            DataSectionObject;
        Section->Segment = Segment;
        (void)InterlockedIncrementUL(&Segment->ReferenceCount);
        MmLockSectionSegment(Segment);

        if (MaximumSize.QuadPart > Segment->RawLength.QuadPart &&
                !(AllocationAttributes & SEC_RESERVE))
        {
            Segment->RawLength.QuadPart = MaximumSize.QuadPart;
            Segment->Length.QuadPart = PAGE_ROUND_UP(Segment->RawLength.QuadPart);
        }
    }
    MmUnlockSectionSegment(Segment);
    Section->FileObject = FileObject;
    Section->MaximumSize = MaximumSize;
#ifndef NEWCC
    CcRosReferenceCache(FileObject);
#endif
    //KeSetEvent((PVOID)&FileObject->Lock, IO_NO_INCREMENT, FALSE);
    *SectionObject = Section;
    return(STATUS_SUCCESS);
}

/*
 TODO: not that great (declaring loaders statically, having to declare all of
 them, having to keep them extern, etc.), will fix in the future
*/
extern NTSTATUS NTAPI PeFmtCreateSection
(
    IN CONST VOID * FileHeader,
    IN SIZE_T FileHeaderSize,
    IN PVOID File,
    OUT PMM_IMAGE_SECTION_OBJECT ImageSectionObject,
    OUT PULONG Flags,
    IN PEXEFMT_CB_READ_FILE ReadFileCb,
    IN PEXEFMT_CB_ALLOCATE_SEGMENTS AllocateSegmentsCb
);

extern NTSTATUS NTAPI ElfFmtCreateSection
(
    IN CONST VOID * FileHeader,
    IN SIZE_T FileHeaderSize,
    IN PVOID File,
    OUT PMM_IMAGE_SECTION_OBJECT ImageSectionObject,
    OUT PULONG Flags,
    IN PEXEFMT_CB_READ_FILE ReadFileCb,
    IN PEXEFMT_CB_ALLOCATE_SEGMENTS AllocateSegmentsCb
);

/* TODO: this is a standard DDK/PSDK macro */
#ifndef RTL_NUMBER_OF
#define RTL_NUMBER_OF(ARR_) (sizeof(ARR_) / sizeof((ARR_)[0]))
#endif

static PEXEFMT_LOADER ExeFmtpLoaders[] =
{
    PeFmtCreateSection,
#ifdef __ELF
    ElfFmtCreateSection
#endif
};

static
PMM_SECTION_SEGMENT
NTAPI
ExeFmtpAllocateSegments(IN ULONG NrSegments)
{
    SIZE_T SizeOfSegments;
    PMM_SECTION_SEGMENT Segments;

    /* TODO: check for integer overflow */
    SizeOfSegments = sizeof(MM_SECTION_SEGMENT) * NrSegments;

    Segments = ExAllocatePoolWithTag(NonPagedPool,
                                     SizeOfSegments,
                                     TAG_MM_SECTION_SEGMENT);

    if(Segments)
        RtlZeroMemory(Segments, SizeOfSegments);

    return Segments;
}

static
NTSTATUS
NTAPI
ExeFmtpReadFile(IN PVOID File,
                IN PLARGE_INTEGER Offset,
                IN ULONG Length,
                OUT PVOID * Data,
                OUT PVOID * AllocBase,
                OUT PULONG ReadSize)
{
    NTSTATUS Status;
    LARGE_INTEGER FileOffset;
    ULONG AdjustOffset;
    ULONG OffsetAdjustment;
    ULONG BufferSize;
    ULONG UsedSize;
    PVOID Buffer;
    PFILE_OBJECT FileObject = File;
    IO_STATUS_BLOCK Iosb;

    ASSERT_IRQL_LESS(DISPATCH_LEVEL);

    if(Length == 0)
    {
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    FileOffset = *Offset;

    /* Negative/special offset: it cannot be used in this context */
    if(FileOffset.u.HighPart < 0)
    {
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    AdjustOffset = PAGE_ROUND_DOWN(FileOffset.u.LowPart);
    OffsetAdjustment = FileOffset.u.LowPart - AdjustOffset;
    FileOffset.u.LowPart = AdjustOffset;

    BufferSize = Length + OffsetAdjustment;
    BufferSize = PAGE_ROUND_UP(BufferSize);

    /* Flush data since we're about to perform a non-cached read */
    CcFlushCache(FileObject->SectionObjectPointer,
                 &FileOffset,
                 BufferSize,
                 &Iosb);

    /*
     * It's ok to use paged pool, because this is a temporary buffer only used in
     * the loading of executables. The assumption is that MmCreateSection is
     * always called at low IRQLs and that these buffers don't survive a brief
     * initialization phase
     */
    Buffer = ExAllocatePoolWithTag(PagedPool,
                                   BufferSize,
                                   'rXmM');
    if (!Buffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    UsedSize = 0;

    Status = MiSimpleRead(FileObject, &FileOffset, Buffer, BufferSize, TRUE, &Iosb);

    UsedSize = (ULONG)Iosb.Information;

    if(NT_SUCCESS(Status) && UsedSize < OffsetAdjustment)
    {
        Status = STATUS_IN_PAGE_ERROR;
        ASSERT(!NT_SUCCESS(Status));
    }

    if(NT_SUCCESS(Status))
    {
        *Data = (PVOID)((ULONG_PTR)Buffer + OffsetAdjustment);
        *AllocBase = Buffer;
        *ReadSize = UsedSize - OffsetAdjustment;
    }
    else
    {
        ExFreePoolWithTag(Buffer, 'rXmM');
    }

    return Status;
}

#ifdef NASSERT
# define MmspAssertSegmentsSorted(OBJ_) ((void)0)
# define MmspAssertSegmentsNoOverlap(OBJ_) ((void)0)
# define MmspAssertSegmentsPageAligned(OBJ_) ((void)0)
#else
static
VOID
NTAPI
MmspAssertSegmentsSorted(IN PMM_IMAGE_SECTION_OBJECT ImageSectionObject)
{
    ULONG i;

    for( i = 1; i < ImageSectionObject->NrSegments; ++ i )
    {
        ASSERT(ImageSectionObject->Segments[i].Image.VirtualAddress >=
               ImageSectionObject->Segments[i - 1].Image.VirtualAddress);
    }
}

static
VOID
NTAPI
MmspAssertSegmentsNoOverlap(IN PMM_IMAGE_SECTION_OBJECT ImageSectionObject)
{
    ULONG i;

    MmspAssertSegmentsSorted(ImageSectionObject);

    for( i = 0; i < ImageSectionObject->NrSegments; ++ i )
    {
        ASSERT(ImageSectionObject->Segments[i].Length.QuadPart > 0);

        if(i > 0)
        {
            ASSERT(ImageSectionObject->Segments[i].Image.VirtualAddress >=
                   (ImageSectionObject->Segments[i - 1].Image.VirtualAddress +
                    ImageSectionObject->Segments[i - 1].Length.QuadPart));
        }
    }
}

static
VOID
NTAPI
MmspAssertSegmentsPageAligned(IN PMM_IMAGE_SECTION_OBJECT ImageSectionObject)
{
    ULONG i;

    for( i = 0; i < ImageSectionObject->NrSegments; ++ i )
    {
        ASSERT((ImageSectionObject->Segments[i].Image.VirtualAddress % PAGE_SIZE) == 0);
        ASSERT((ImageSectionObject->Segments[i].Length.QuadPart % PAGE_SIZE) == 0);
    }
}
#endif

static
int
__cdecl
MmspCompareSegments(const void * x,
                    const void * y)
{
    const MM_SECTION_SEGMENT *Segment1 = (const MM_SECTION_SEGMENT *)x;
    const MM_SECTION_SEGMENT *Segment2 = (const MM_SECTION_SEGMENT *)y;

    return
        (Segment1->Image.VirtualAddress - Segment2->Image.VirtualAddress) >>
        ((sizeof(ULONG_PTR) - sizeof(int)) * 8);
}

/*
 * Ensures an image section's segments are sorted in memory
 */
static
VOID
NTAPI
MmspSortSegments(IN OUT PMM_IMAGE_SECTION_OBJECT ImageSectionObject,
                 IN ULONG Flags)
{
    if (Flags & EXEFMT_LOAD_ASSUME_SEGMENTS_SORTED)
    {
        MmspAssertSegmentsSorted(ImageSectionObject);
    }
    else
    {
        qsort(ImageSectionObject->Segments,
              ImageSectionObject->NrSegments,
              sizeof(ImageSectionObject->Segments[0]),
              MmspCompareSegments);
    }
}


/*
 * Ensures an image section's segments don't overlap in memory and don't have
 * gaps and don't have a null size. We let them map to overlapping file regions,
 * though - that's not necessarily an error
 */
static
BOOLEAN
NTAPI
MmspCheckSegmentBounds
(
    IN OUT PMM_IMAGE_SECTION_OBJECT ImageSectionObject,
    IN ULONG Flags
)
{
    ULONG i;

    if (Flags & EXEFMT_LOAD_ASSUME_SEGMENTS_NO_OVERLAP)
    {
        MmspAssertSegmentsNoOverlap(ImageSectionObject);
        return TRUE;
    }

    ASSERT(ImageSectionObject->NrSegments >= 1);

    for ( i = 0; i < ImageSectionObject->NrSegments; ++ i )
    {
        if(ImageSectionObject->Segments[i].Length.QuadPart == 0)
        {
            return FALSE;
        }

        if(i > 0)
        {
            /*
             * TODO: relax the limitation on gaps. For example, gaps smaller than a
             * page could be OK (Windows seems to be OK with them), and larger gaps
             * could lead to image sections spanning several discontiguous regions
             * (NtMapViewOfSection could then refuse to map them, and they could
             * e.g. only be allowed as parameters to NtCreateProcess, like on UNIX)
             */
            if ((ImageSectionObject->Segments[i - 1].Image.VirtualAddress +
                    ImageSectionObject->Segments[i - 1].Length.QuadPart) !=
                    ImageSectionObject->Segments[i].Image.VirtualAddress)
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}

/*
 * Merges and pads an image section's segments until they all are page-aligned
 * and have a size that is a multiple of the page size
 */
static
BOOLEAN
NTAPI
MmspPageAlignSegments
(
    IN OUT PMM_IMAGE_SECTION_OBJECT ImageSectionObject,
    IN ULONG Flags
)
{
    ULONG i;
    ULONG LastSegment;
    PMM_SECTION_SEGMENT EffectiveSegment;

    if (Flags & EXEFMT_LOAD_ASSUME_SEGMENTS_PAGE_ALIGNED)
    {
        MmspAssertSegmentsPageAligned(ImageSectionObject);
        return TRUE;
    }

    LastSegment = 0;
    EffectiveSegment = &ImageSectionObject->Segments[LastSegment];

    for ( i = 0; i < ImageSectionObject->NrSegments; ++ i )
    {
        /*
         * The first segment requires special handling
         */
        if (i == 0)
        {
            ULONG_PTR VirtualAddress;
            ULONG_PTR VirtualOffset;

            VirtualAddress = EffectiveSegment->Image.VirtualAddress;

            /* Round down the virtual address to the nearest page */
            EffectiveSegment->Image.VirtualAddress = PAGE_ROUND_DOWN(VirtualAddress);

            /* Round up the virtual size to the nearest page */
            EffectiveSegment->Length.QuadPart = PAGE_ROUND_UP(VirtualAddress + EffectiveSegment->Length.QuadPart) -
                                                EffectiveSegment->Image.VirtualAddress;

            /* Adjust the raw address and size */
            VirtualOffset = VirtualAddress - EffectiveSegment->Image.VirtualAddress;

            if (EffectiveSegment->Image.FileOffset < VirtualOffset)
            {
                return FALSE;
            }

            /*
             * Garbage in, garbage out: unaligned base addresses make the file
             * offset point in curious and odd places, but that's what we were
             * asked for
             */
            EffectiveSegment->Image.FileOffset -= VirtualOffset;
            EffectiveSegment->RawLength.QuadPart += VirtualOffset;
        }
        else
        {
            PMM_SECTION_SEGMENT Segment = &ImageSectionObject->Segments[i];
            ULONG_PTR EndOfEffectiveSegment;

            EndOfEffectiveSegment = (ULONG_PTR)(EffectiveSegment->Image.VirtualAddress + EffectiveSegment->Length.QuadPart);
            ASSERT((EndOfEffectiveSegment % PAGE_SIZE) == 0);

            /*
             * The current segment begins exactly where the current effective
             * segment ended, therefore beginning a new effective segment
             */
            if (EndOfEffectiveSegment == Segment->Image.VirtualAddress)
            {
                LastSegment ++;
                ASSERT(LastSegment <= i);
                ASSERT(LastSegment < ImageSectionObject->NrSegments);

                EffectiveSegment = &ImageSectionObject->Segments[LastSegment];

                if (LastSegment != i)
                {
                    /*
                     * Copy the current segment. If necessary, the effective segment
                     * will be expanded later
                     */
                    *EffectiveSegment = *Segment;
                }

                /*
                 * Page-align the virtual size. We know for sure the virtual address
                 * already is
                 */
                ASSERT((EffectiveSegment->Image.VirtualAddress % PAGE_SIZE) == 0);
                EffectiveSegment->Length.QuadPart = PAGE_ROUND_UP(EffectiveSegment->Length.QuadPart);
            }
            /*
             * The current segment is still part of the current effective segment:
             * extend the effective segment to reflect this
             */
            else if (EndOfEffectiveSegment > Segment->Image.VirtualAddress)
            {
                static const ULONG FlagsToProtection[16] =
                {
                    PAGE_NOACCESS,
                    PAGE_READONLY,
                    PAGE_READWRITE,
                    PAGE_READWRITE,
                    PAGE_EXECUTE_READ,
                    PAGE_EXECUTE_READ,
                    PAGE_EXECUTE_READWRITE,
                    PAGE_EXECUTE_READWRITE,
                    PAGE_WRITECOPY,
                    PAGE_WRITECOPY,
                    PAGE_WRITECOPY,
                    PAGE_WRITECOPY,
                    PAGE_EXECUTE_WRITECOPY,
                    PAGE_EXECUTE_WRITECOPY,
                    PAGE_EXECUTE_WRITECOPY,
                    PAGE_EXECUTE_WRITECOPY
                };

                unsigned ProtectionFlags;

                /*
                 * Extend the file size
                 */

                /* Unaligned segments must be contiguous within the file */
                if (Segment->Image.FileOffset != (EffectiveSegment->Image.FileOffset +
                                                  EffectiveSegment->RawLength.QuadPart))
                {
                    return FALSE;
                }

                EffectiveSegment->RawLength.QuadPart += Segment->RawLength.QuadPart;

                /*
                 * Extend the virtual size
                 */
                ASSERT(PAGE_ROUND_UP(Segment->Image.VirtualAddress + Segment->Length.QuadPart) >= EndOfEffectiveSegment);

                EffectiveSegment->Length.QuadPart = PAGE_ROUND_UP(Segment->Image.VirtualAddress + Segment->Length.QuadPart) -
                                                    EffectiveSegment->Image.VirtualAddress;

                /*
                 * Merge the protection
                 */
                EffectiveSegment->Protection |= Segment->Protection;

                /* Clean up redundance */
                ProtectionFlags = 0;

                if(EffectiveSegment->Protection & PAGE_IS_READABLE)
                    ProtectionFlags |= 1 << 0;

                if(EffectiveSegment->Protection & PAGE_IS_WRITABLE)
                    ProtectionFlags |= 1 << 1;

                if(EffectiveSegment->Protection & PAGE_IS_EXECUTABLE)
                    ProtectionFlags |= 1 << 2;

                if(EffectiveSegment->Protection & PAGE_IS_WRITECOPY)
                    ProtectionFlags |= 1 << 3;

                ASSERT(ProtectionFlags < 16);
                EffectiveSegment->Protection = FlagsToProtection[ProtectionFlags];

                /* If a segment was required to be shared and cannot, fail */
                if(!(Segment->Protection & PAGE_IS_WRITECOPY) &&
                        EffectiveSegment->Protection & PAGE_IS_WRITECOPY)
                {
                    return FALSE;
                }
            }
            /*
             * We assume no holes between segments at this point
             */
            else
            {
                KeBugCheck(MEMORY_MANAGEMENT);
            }
        }
    }
    ImageSectionObject->NrSegments = LastSegment + 1;

    return TRUE;
}

NTSTATUS
ExeFmtpCreateImageSection(PFILE_OBJECT FileObject,
                          PMM_IMAGE_SECTION_OBJECT ImageSectionObject)
{
    LARGE_INTEGER Offset;
    PVOID FileHeader;
    PVOID FileHeaderBuffer;
    ULONG FileHeaderSize;
    ULONG Flags;
    ULONG OldNrSegments;
    NTSTATUS Status;
    ULONG i;

    /*
     * Read the beginning of the file (2 pages). Should be enough to contain
     * all (or most) of the headers
     */
    Offset.QuadPart = 0;

    Status = ExeFmtpReadFile (FileObject,
                              &Offset,
                              PAGE_SIZE * 2,
                              &FileHeader,
                              &FileHeaderBuffer,
                              &FileHeaderSize);

    if (!NT_SUCCESS(Status))
        return Status;

    if (FileHeaderSize == 0)
    {
        ExFreePool(FileHeaderBuffer);
        return STATUS_UNSUCCESSFUL;
    }

    /*
     * Look for a loader that can handle this executable
     */
    for (i = 0; i < RTL_NUMBER_OF(ExeFmtpLoaders); ++ i)
    {
        RtlZeroMemory(ImageSectionObject, sizeof(*ImageSectionObject));
        Flags = 0;

        Status = ExeFmtpLoaders[i](FileHeader,
                                   FileHeaderSize,
                                   FileObject,
                                   ImageSectionObject,
                                   &Flags,
                                   ExeFmtpReadFile,
                                   ExeFmtpAllocateSegments);

        if (!NT_SUCCESS(Status))
        {
            if (ImageSectionObject->Segments)
            {
                ExFreePool(ImageSectionObject->Segments);
                ImageSectionObject->Segments = NULL;
            }
        }

        if (Status != STATUS_ROS_EXEFMT_UNKNOWN_FORMAT)
            break;
    }

    ExFreePoolWithTag(FileHeaderBuffer, 'rXmM');

    /*
     * No loader handled the format
     */
    if (Status == STATUS_ROS_EXEFMT_UNKNOWN_FORMAT)
    {
        Status = STATUS_INVALID_IMAGE_NOT_MZ;
        ASSERT(!NT_SUCCESS(Status));
    }

    if (!NT_SUCCESS(Status))
        return Status;

    ASSERT(ImageSectionObject->Segments != NULL);

    /*
     * Some defaults
     */
    /* FIXME? are these values platform-dependent? */
    if (ImageSectionObject->ImageInformation.MaximumStackSize == 0)
        ImageSectionObject->ImageInformation.MaximumStackSize = 0x40000;

    if(ImageSectionObject->ImageInformation.CommittedStackSize == 0)
        ImageSectionObject->ImageInformation.CommittedStackSize = 0x1000;

    if(ImageSectionObject->BasedAddress == NULL)
    {
        if(ImageSectionObject->ImageInformation.ImageCharacteristics & IMAGE_FILE_DLL)
            ImageSectionObject->BasedAddress = (PVOID)0x10000000;
        else
            ImageSectionObject->BasedAddress = (PVOID)0x00400000;
    }

    /*
     * And now the fun part: fixing the segments
     */

    /* Sort them by virtual address */
    MmspSortSegments(ImageSectionObject, Flags);

    /* Ensure they don't overlap in memory */
    if (!MmspCheckSegmentBounds(ImageSectionObject, Flags))
        return STATUS_INVALID_IMAGE_FORMAT;

    /* Ensure they are aligned */
    OldNrSegments = ImageSectionObject->NrSegments;

    if (!MmspPageAlignSegments(ImageSectionObject, Flags))
        return STATUS_INVALID_IMAGE_FORMAT;

    /* Trim them if the alignment phase merged some of them */
    if (ImageSectionObject->NrSegments < OldNrSegments)
    {
        PMM_SECTION_SEGMENT Segments;
        SIZE_T SizeOfSegments;

        SizeOfSegments = sizeof(MM_SECTION_SEGMENT) * ImageSectionObject->NrSegments;

        Segments = ExAllocatePoolWithTag(PagedPool,
                                         SizeOfSegments,
                                         TAG_MM_SECTION_SEGMENT);

        if (Segments == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        RtlCopyMemory(Segments, ImageSectionObject->Segments, SizeOfSegments);
        ExFreePool(ImageSectionObject->Segments);
        ImageSectionObject->Segments = Segments;
    }

    /* And finish their initialization */
    for ( i = 0; i < ImageSectionObject->NrSegments; ++ i )
    {
        ExInitializeFastMutex(&ImageSectionObject->Segments[i].Lock);
        ImageSectionObject->Segments[i].ReferenceCount = 1;
        MiInitializeSectionPageTable(&ImageSectionObject->Segments[i]);
    }

    ASSERT(NT_SUCCESS(Status));
    return Status;
}

NTSTATUS
MmCreateImageSection(PROS_SECTION_OBJECT *SectionObject,
                     ACCESS_MASK DesiredAccess,
                     POBJECT_ATTRIBUTES ObjectAttributes,
                     PLARGE_INTEGER UMaximumSize,
                     ULONG SectionPageProtection,
                     ULONG AllocationAttributes,
                     PFILE_OBJECT FileObject)
{
    PROS_SECTION_OBJECT Section;
    NTSTATUS Status;
    PMM_SECTION_SEGMENT SectionSegments;
    PMM_IMAGE_SECTION_OBJECT ImageSectionObject;
    ULONG i;

    if (FileObject == NULL)
        return STATUS_INVALID_FILE_FOR_SECTION;

    /*
     * Create the section
     */
    Status = ObCreateObject (ExGetPreviousMode(),
                             MmSectionObjectType,
                             ObjectAttributes,
                             ExGetPreviousMode(),
                             NULL,
                             sizeof(ROS_SECTION_OBJECT),
                             0,
                             0,
                             (PVOID*)(PVOID)&Section);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(FileObject);
        return(Status);
    }

    /*
     * Initialize it
     */
    RtlZeroMemory(Section, sizeof(ROS_SECTION_OBJECT));
    Section->Type = 'SC';
    Section->Size = 'TN';
    Section->SectionPageProtection = SectionPageProtection;
    Section->AllocationAttributes = AllocationAttributes;

#ifndef NEWCC
    /*
     * Initialized caching for this file object if previously caching
     * was initialized for the same on disk file
     */
    Status = CcTryToInitializeFileCache(FileObject);
#else
    Status = STATUS_SUCCESS;
#endif

    if (!NT_SUCCESS(Status) || FileObject->SectionObjectPointer->ImageSectionObject == NULL)
    {
        NTSTATUS StatusExeFmt;

        ImageSectionObject = ExAllocatePoolWithTag(PagedPool, sizeof(MM_IMAGE_SECTION_OBJECT), TAG_MM_SECTION_SEGMENT);
        if (ImageSectionObject == NULL)
        {
            ObDereferenceObject(FileObject);
            ObDereferenceObject(Section);
            return(STATUS_NO_MEMORY);
        }

        RtlZeroMemory(ImageSectionObject, sizeof(MM_IMAGE_SECTION_OBJECT));

        StatusExeFmt = ExeFmtpCreateImageSection(FileObject, ImageSectionObject);

        if (!NT_SUCCESS(StatusExeFmt))
        {
            if(ImageSectionObject->Segments != NULL)
                ExFreePool(ImageSectionObject->Segments);

            ExFreePoolWithTag(ImageSectionObject, TAG_MM_SECTION_SEGMENT);
            ObDereferenceObject(Section);
            ObDereferenceObject(FileObject);
            return(StatusExeFmt);
        }

        Section->ImageSection = ImageSectionObject;
        ASSERT(ImageSectionObject->Segments);

        /*
         * Lock the file
         */
        Status = MmspWaitForFileLock(FileObject);
        if (!NT_SUCCESS(Status))
        {
            ExFreePool(ImageSectionObject->Segments);
            ExFreePool(ImageSectionObject);
            ObDereferenceObject(Section);
            ObDereferenceObject(FileObject);
            return(Status);
        }

        if (NULL != InterlockedCompareExchangePointer(&FileObject->SectionObjectPointer->ImageSectionObject,
                ImageSectionObject, NULL))
        {
            /*
             * An other thread has initialized the same image in the background
             */
            ExFreePool(ImageSectionObject->Segments);
            ExFreePool(ImageSectionObject);
            ImageSectionObject = FileObject->SectionObjectPointer->ImageSectionObject;
            Section->ImageSection = ImageSectionObject;
            SectionSegments = ImageSectionObject->Segments;

            for (i = 0; i < ImageSectionObject->NrSegments; i++)
            {
                (void)InterlockedIncrementUL(&SectionSegments[i].ReferenceCount);
            }
        }

        Status = StatusExeFmt;
    }
    else
    {
        /*
         * Lock the file
         */
        Status = MmspWaitForFileLock(FileObject);
        if (Status != STATUS_SUCCESS)
        {
            ObDereferenceObject(Section);
            ObDereferenceObject(FileObject);
            return(Status);
        }

        ImageSectionObject = FileObject->SectionObjectPointer->ImageSectionObject;
        Section->ImageSection = ImageSectionObject;
        SectionSegments = ImageSectionObject->Segments;

        /*
         * Otherwise just reference all the section segments
         */
        for (i = 0; i < ImageSectionObject->NrSegments; i++)
        {
            (void)InterlockedIncrementUL(&SectionSegments[i].ReferenceCount);
        }

        Status = STATUS_SUCCESS;
    }
    Section->FileObject = FileObject;
#ifndef NEWCC
    CcRosReferenceCache(FileObject);
#endif
    //KeSetEvent((PVOID)&FileObject->Lock, IO_NO_INCREMENT, FALSE);
    *SectionObject = Section;
    return(Status);
}



static NTSTATUS
MmMapViewOfSegment(PMMSUPPORT AddressSpace,
                   PROS_SECTION_OBJECT Section,
                   PMM_SECTION_SEGMENT Segment,
                   PVOID* BaseAddress,
                   SIZE_T ViewSize,
                   ULONG Protect,
                   ULONG ViewOffset,
                   ULONG AllocationType)
{
    PMEMORY_AREA MArea;
    NTSTATUS Status;
    ULONG Granularity;

    if (Segment->WriteCopy)
    {
        /* We have to do this because the not present fault
         * and access fault handlers depend on the protection
         * that should be granted AFTER the COW fault takes
         * place to be in Region->Protect. The not present fault
         * handler changes this to the correct protection for COW when
         * mapping the pages into the process's address space. If a COW
         * fault takes place, the access fault handler sets the page protection
         * to these values for the newly copied pages
         */
        if (Protect == PAGE_WRITECOPY)
            Protect = PAGE_READWRITE;
        else if (Protect == PAGE_EXECUTE_WRITECOPY)
            Protect = PAGE_EXECUTE_READWRITE;
    }

    if (*BaseAddress == NULL)
        Granularity = MM_ALLOCATION_GRANULARITY;
    else
        Granularity = PAGE_SIZE;

#ifdef NEWCC
    if (Segment->Flags & MM_DATAFILE_SEGMENT)
    {
        LARGE_INTEGER FileOffset;
        FileOffset.QuadPart = ViewOffset;
        ObReferenceObject(Section);
        return _MiMapViewOfSegment(AddressSpace, Segment, BaseAddress, ViewSize, Protect, &FileOffset, AllocationType, __FILE__, __LINE__);
    }
#endif
    Status = MmCreateMemoryArea(AddressSpace,
                                MEMORY_AREA_SECTION_VIEW,
                                BaseAddress,
                                ViewSize,
                                Protect,
                                &MArea,
                                AllocationType,
                                Granularity);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Mapping between 0x%p and 0x%p failed (%X).\n",
                (*BaseAddress), (char*)(*BaseAddress) + ViewSize, Status);
        return(Status);
    }

    ObReferenceObject((PVOID)Section);

    MArea->Data.SectionData.Segment = Segment;
    MArea->Data.SectionData.Section = Section;
    MArea->Data.SectionData.ViewOffset.QuadPart = ViewOffset;
    if (Section->AllocationAttributes & SEC_IMAGE)
    {
        MArea->VadNode.u.VadFlags.VadType = VadImageMap;
    }

    MmInitializeRegion(&MArea->Data.SectionData.RegionListHead,
                       ViewSize, 0, Protect);

    return(STATUS_SUCCESS);
}


static VOID
MmFreeSectionPage(PVOID Context, MEMORY_AREA* MemoryArea, PVOID Address,
                  PFN_NUMBER Page, SWAPENTRY SwapEntry, BOOLEAN Dirty)
{
    ULONG_PTR Entry;
#ifndef NEWCC
    PFILE_OBJECT FileObject;
    PROS_SHARED_CACHE_MAP SharedCacheMap;
#endif
    LARGE_INTEGER Offset;
    SWAPENTRY SavedSwapEntry;
    PROS_SECTION_OBJECT Section;
    PMM_SECTION_SEGMENT Segment;
    PMMSUPPORT AddressSpace;
    PEPROCESS Process;

    AddressSpace = (PMMSUPPORT)Context;
    Process = MmGetAddressSpaceOwner(AddressSpace);

    Address = (PVOID)PAGE_ROUND_DOWN(Address);

    Offset.QuadPart = ((ULONG_PTR)Address - MA_GetStartingAddress(MemoryArea)) +
                      MemoryArea->Data.SectionData.ViewOffset.QuadPart;

    Section = MemoryArea->Data.SectionData.Section;
    Segment = MemoryArea->Data.SectionData.Segment;

    Entry = MmGetPageEntrySectionSegment(Segment, &Offset);
    while (Entry && IS_SWAP_FROM_SSE(Entry) && SWAPENTRY_FROM_SSE(Entry) == MM_WAIT_ENTRY)
    {
        MmUnlockSectionSegment(Segment);
        MmUnlockAddressSpace(AddressSpace);

        MiWaitForPageEvent(NULL, NULL);

        MmLockAddressSpace(AddressSpace);
        MmLockSectionSegment(Segment);
        Entry = MmGetPageEntrySectionSegment(Segment, &Offset);
    }

    /*
     * For a dirty, datafile, non-private page mark it as dirty in the
     * cache manager.
     */
    if (Segment->Flags & MM_DATAFILE_SEGMENT)
    {
        if (Page == PFN_FROM_SSE(Entry) && Dirty)
        {
#ifndef NEWCC
            FileObject = MemoryArea->Data.SectionData.Section->FileObject;
            SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
            CcRosMarkDirtyVacb(SharedCacheMap, Offset.QuadPart + Segment->Image.FileOffset);
#endif
            ASSERT(SwapEntry == 0);
        }
    }

    if (SwapEntry != 0)
    {
        /*
         * Sanity check
         */
        if (Segment->Flags & MM_PAGEFILE_SEGMENT)
        {
            DPRINT1("Found a swap entry for a page in a pagefile section.\n");
            KeBugCheck(MEMORY_MANAGEMENT);
        }
        MmFreeSwapPage(SwapEntry);
    }
    else if (Page != 0)
    {
        if (IS_SWAP_FROM_SSE(Entry) ||
                Page != PFN_FROM_SSE(Entry))
        {
            /*
             * Sanity check
             */
            if (Segment->Flags & MM_PAGEFILE_SEGMENT)
            {
                DPRINT1("Found a private page in a pagefile section.\n");
                KeBugCheck(MEMORY_MANAGEMENT);
            }
            /*
             * Just dereference private pages
             */
            SavedSwapEntry = MmGetSavedSwapEntryPage(Page);
            if (SavedSwapEntry != 0)
            {
                MmFreeSwapPage(SavedSwapEntry);
                MmSetSavedSwapEntryPage(Page, 0);
            }
            MmDeleteRmap(Page, Process, Address);
            MmReleasePageMemoryConsumer(MC_USER, Page);
        }
        else
        {
            MmDeleteRmap(Page, Process, Address);
            MmUnsharePageEntrySectionSegment(Section, Segment, &Offset, Dirty, FALSE, NULL);
        }
    }
}

static NTSTATUS
MmUnmapViewOfSegment(PMMSUPPORT AddressSpace,
                     PVOID BaseAddress)
{
    NTSTATUS Status;
    PMEMORY_AREA MemoryArea;
    PROS_SECTION_OBJECT Section;
    PMM_SECTION_SEGMENT Segment;
    PLIST_ENTRY CurrentEntry;
    PMM_REGION CurrentRegion;
    PLIST_ENTRY RegionListHead;

    MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace,
                 BaseAddress);
    if (MemoryArea == NULL)
    {
        return(STATUS_UNSUCCESSFUL);
    }

    Section = MemoryArea->Data.SectionData.Section;
    Segment = MemoryArea->Data.SectionData.Segment;

#ifdef NEWCC
    if (Segment->Flags & MM_DATAFILE_SEGMENT)
    {
        MmUnlockAddressSpace(AddressSpace);
        Status = MmUnmapViewOfCacheSegment(AddressSpace, BaseAddress);
        MmLockAddressSpace(AddressSpace);

        return Status;
    }
#endif

    MemoryArea->DeleteInProgress = TRUE;

    MmLockSectionSegment(Segment);

    RegionListHead = &MemoryArea->Data.SectionData.RegionListHead;
    while (!IsListEmpty(RegionListHead))
    {
        CurrentEntry = RemoveHeadList(RegionListHead);
        CurrentRegion = CONTAINING_RECORD(CurrentEntry, MM_REGION, RegionListEntry);
        ExFreePoolWithTag(CurrentRegion, TAG_MM_REGION);
    }

    if (Section->AllocationAttributes & SEC_PHYSICALMEMORY)
    {
        Status = MmFreeMemoryArea(AddressSpace,
                                  MemoryArea,
                                  NULL,
                                  NULL);
    }
    else
    {
        Status = MmFreeMemoryArea(AddressSpace,
                                  MemoryArea,
                                  MmFreeSectionPage,
                                  AddressSpace);
    }
    MmUnlockSectionSegment(Segment);
    ObDereferenceObject(Section);
    return(Status);
}

NTSTATUS
NTAPI
MiRosUnmapViewOfSection(IN PEPROCESS Process,
                        IN PVOID BaseAddress,
                        IN ULONG Flags)
{
    NTSTATUS Status;
    PMEMORY_AREA MemoryArea;
    PMMSUPPORT AddressSpace;
    PROS_SECTION_OBJECT Section;
    PVOID ImageBaseAddress = 0;

    DPRINT("Opening memory area Process %p BaseAddress %p\n",
           Process, BaseAddress);

    ASSERT(Process);

    AddressSpace = Process ? &Process->Vm : MmGetKernelAddressSpace();

    MmLockAddressSpace(AddressSpace);
    MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace,
                 BaseAddress);
    if (MemoryArea == NULL ||
            ((MemoryArea->Type != MEMORY_AREA_SECTION_VIEW) &&
             (MemoryArea->Type != MEMORY_AREA_CACHE)) ||
            MemoryArea->DeleteInProgress)
    {
        if (MemoryArea) NT_ASSERT(MemoryArea->Type != MEMORY_AREA_OWNED_BY_ARM3);
        MmUnlockAddressSpace(AddressSpace);
        return STATUS_NOT_MAPPED_VIEW;
    }

    Section = MemoryArea->Data.SectionData.Section;

    if ((Section != NULL) && (Section->AllocationAttributes & SEC_IMAGE))
    {
        ULONG i;
        ULONG NrSegments;
        PMM_IMAGE_SECTION_OBJECT ImageSectionObject;
        PMM_SECTION_SEGMENT SectionSegments;
        PMM_SECTION_SEGMENT Segment;

        Segment = MemoryArea->Data.SectionData.Segment;
        ImageSectionObject = Section->ImageSection;
        SectionSegments = ImageSectionObject->Segments;
        NrSegments = ImageSectionObject->NrSegments;

        MemoryArea->DeleteInProgress = TRUE;

        /* Search for the current segment within the section segments
         * and calculate the image base address */
        for (i = 0; i < NrSegments; i++)
        {
            if (Segment == &SectionSegments[i])
            {
                ImageBaseAddress = (char*)BaseAddress - (ULONG_PTR)SectionSegments[i].Image.VirtualAddress;
                break;
            }
        }
        if (i >= NrSegments)
        {
            KeBugCheck(MEMORY_MANAGEMENT);
        }

        for (i = 0; i < NrSegments; i++)
        {
            PVOID SBaseAddress = (PVOID)
                                 ((char*)ImageBaseAddress + (ULONG_PTR)SectionSegments[i].Image.VirtualAddress);

            Status = MmUnmapViewOfSegment(AddressSpace, SBaseAddress);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("MmUnmapViewOfSegment failed for %p (Process %p) with %lx\n",
                        SBaseAddress, Process, Status);
                NT_ASSERT(NT_SUCCESS(Status));
            }
        }
    }
    else
    {
        Status = MmUnmapViewOfSegment(AddressSpace, BaseAddress);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("MmUnmapViewOfSegment failed for %p (Process %p) with %lx\n",
                    BaseAddress, Process, Status);
            NT_ASSERT(NT_SUCCESS(Status));
        }
    }

    MmUnlockAddressSpace(AddressSpace);

    /* Notify debugger */
    if (ImageBaseAddress) DbgkUnMapViewOfSection(ImageBaseAddress);

    return(STATUS_SUCCESS);
}




/**
 * Queries the information of a section object.
 *
 * @param SectionHandle
 *        Handle to the section object. It must be opened with SECTION_QUERY
 *        access.
 * @param SectionInformationClass
 *        Index to a certain information structure. Can be either
 *        SectionBasicInformation or SectionImageInformation. The latter
 *        is valid only for sections that were created with the SEC_IMAGE
 *        flag.
 * @param SectionInformation
 *        Caller supplies storage for resulting information.
 * @param Length
 *        Size of the supplied storage.
 * @param ResultLength
 *        Data written.
 *
 * @return Status.
 *
 * @implemented
 */
NTSTATUS NTAPI
NtQuerySection(IN HANDLE SectionHandle,
               IN SECTION_INFORMATION_CLASS SectionInformationClass,
               OUT PVOID SectionInformation,
               IN SIZE_T SectionInformationLength,
               OUT PSIZE_T ResultLength  OPTIONAL)
{
    PROS_SECTION_OBJECT Section;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    Status = DefaultQueryInfoBufferCheck(SectionInformationClass,
                                         ExSectionInfoClass,
                                         sizeof(ExSectionInfoClass) / sizeof(ExSectionInfoClass[0]),
                                         SectionInformation,
                                         (ULONG)SectionInformationLength,
                                         NULL,
                                         ResultLength,
                                         PreviousMode);

    if(!NT_SUCCESS(Status))
    {
        DPRINT1("NtQuerySection() failed, Status: 0x%x\n", Status);
        return Status;
    }

    Status = ObReferenceObjectByHandle(SectionHandle,
                                       SECTION_QUERY,
                                       MmSectionObjectType,
                                       PreviousMode,
                                       (PVOID*)(PVOID)&Section,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        switch (SectionInformationClass)
        {
        case SectionBasicInformation:
        {
            PSECTION_BASIC_INFORMATION Sbi = (PSECTION_BASIC_INFORMATION)SectionInformation;

            _SEH2_TRY
            {
                Sbi->Attributes = Section->AllocationAttributes;
                if (Section->AllocationAttributes & SEC_IMAGE)
                {
                    Sbi->BaseAddress = 0;
                    Sbi->Size.QuadPart = 0;
                }
                else
                {
                    Sbi->BaseAddress = (PVOID)Section->Segment->Image.VirtualAddress;
                    Sbi->Size.QuadPart = Section->Segment->Length.QuadPart;
                }

                if (ResultLength != NULL)
                {
                    *ResultLength = sizeof(SECTION_BASIC_INFORMATION);
                }
                Status = STATUS_SUCCESS;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            break;
        }

        case SectionImageInformation:
        {
            PSECTION_IMAGE_INFORMATION Sii = (PSECTION_IMAGE_INFORMATION)SectionInformation;

            _SEH2_TRY
            {
                if (Section->AllocationAttributes & SEC_IMAGE)
                {
                    PMM_IMAGE_SECTION_OBJECT ImageSectionObject;
                    ImageSectionObject = Section->ImageSection;

                    *Sii = ImageSectionObject->ImageInformation;
                }

                if (ResultLength != NULL)
                {
                    *ResultLength = sizeof(SECTION_IMAGE_INFORMATION);
                }
                Status = STATUS_SUCCESS;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            break;
        }
        }

        ObDereferenceObject(Section);
    }

    return(Status);
}

/**********************************************************************
 * NAME       EXPORTED
 * MmMapViewOfSection
 *
 * DESCRIPTION
 * Maps a view of a section into the virtual address space of a
 * process.
 *
 * ARGUMENTS
 * Section
 *  Pointer to the section object.
 *
 * ProcessHandle
 *  Pointer to the process.
 *
 * BaseAddress
 *  Desired base address (or NULL) on entry;
 *  Actual base address of the view on exit.
 *
 * ZeroBits
 *  Number of high order address bits that must be zero.
 *
 * CommitSize
 *  Size in bytes of the initially committed section of
 *  the view.
 *
 * SectionOffset
 *  Offset in bytes from the beginning of the section
 *  to the beginning of the view.
 *
 * ViewSize
 *  Desired length of map (or zero to map all) on entry
 *  Actual length mapped on exit.
 *
 * InheritDisposition
 *  Specified how the view is to be shared with
 *  child processes.
 *
 * AllocationType
 *  Type of allocation for the pages.
 *
 * Protect
 *  Protection for the committed region of the view.
 *
 * RETURN VALUE
 * Status.
 *
 * @implemented
 */
NTSTATUS NTAPI
MmMapViewOfSection(IN PVOID SectionObject,
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
    PROS_SECTION_OBJECT Section;
    PMMSUPPORT AddressSpace;
    ULONG ViewOffset;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN NotAtBase = FALSE;

    if (MiIsRosSectionObject(SectionObject) == FALSE)
    {
        DPRINT("Mapping ARM3 section into %s\n", Process->ImageFileName);
        return MmMapViewOfArm3Section(SectionObject,
                                      Process,
                                      BaseAddress,
                                      ZeroBits,
                                      CommitSize,
                                      SectionOffset,
                                      ViewSize,
                                      InheritDisposition,
                                      AllocationType,
                                      Protect);
    }

    ASSERT(Process);

    if (!Protect || Protect & ~PAGE_FLAGS_VALID_FOR_SECTION)
    {
        return STATUS_INVALID_PAGE_PROTECTION;
    }

    /* FIXME: We should keep this, but it would break code checking equality */
    Protect &= ~PAGE_NOCACHE;

    Section = (PROS_SECTION_OBJECT)SectionObject;
    AddressSpace = &Process->Vm;

    AllocationType |= (Section->AllocationAttributes & SEC_NO_CHANGE);

    MmLockAddressSpace(AddressSpace);

    if (Section->AllocationAttributes & SEC_IMAGE)
    {
        ULONG i;
        ULONG NrSegments;
        ULONG_PTR ImageBase;
        SIZE_T ImageSize;
        PMM_IMAGE_SECTION_OBJECT ImageSectionObject;
        PMM_SECTION_SEGMENT SectionSegments;

        ImageSectionObject = Section->ImageSection;
        SectionSegments = ImageSectionObject->Segments;
        NrSegments = ImageSectionObject->NrSegments;

        ImageBase = (ULONG_PTR)*BaseAddress;
        if (ImageBase == 0)
        {
            ImageBase = (ULONG_PTR)ImageSectionObject->BasedAddress;
        }

        ImageSize = 0;
        for (i = 0; i < NrSegments; i++)
        {
            ULONG_PTR MaxExtent;
            MaxExtent = (ULONG_PTR)(SectionSegments[i].Image.VirtualAddress +
                                    SectionSegments[i].Length.QuadPart);
            ImageSize = max(ImageSize, MaxExtent);
        }

        ImageSectionObject->ImageInformation.ImageFileSize = (ULONG)ImageSize;

        /* Check for an illegal base address */
        if (((ImageBase + ImageSize) > (ULONG_PTR)MmHighestUserAddress) ||
                ((ImageBase + ImageSize) < ImageSize))
        {
            NT_ASSERT(*BaseAddress == NULL);
            ImageBase = ALIGN_DOWN_BY((ULONG_PTR)MmHighestUserAddress - ImageSize,
                                      MM_VIRTMEM_GRANULARITY);
            NotAtBase = TRUE;
        }
        else if (ImageBase != ALIGN_DOWN_BY(ImageBase, MM_VIRTMEM_GRANULARITY))
        {
            NT_ASSERT(*BaseAddress == NULL);
            ImageBase = ALIGN_DOWN_BY(ImageBase, MM_VIRTMEM_GRANULARITY);
            NotAtBase = TRUE;
        }

        /* Check there is enough space to map the section at that point. */
        if (MmLocateMemoryAreaByRegion(AddressSpace, (PVOID)ImageBase,
                                       PAGE_ROUND_UP(ImageSize)) != NULL)
        {
            /* Fail if the user requested a fixed base address. */
            if ((*BaseAddress) != NULL)
            {
                MmUnlockAddressSpace(AddressSpace);
                return(STATUS_CONFLICTING_ADDRESSES);
            }
            /* Otherwise find a gap to map the image. */
            ImageBase = (ULONG_PTR)MmFindGap(AddressSpace, PAGE_ROUND_UP(ImageSize), MM_VIRTMEM_GRANULARITY, FALSE);
            if (ImageBase == 0)
            {
                MmUnlockAddressSpace(AddressSpace);
                return(STATUS_CONFLICTING_ADDRESSES);
            }
            /* Remember that we loaded image at a different base address */
            NotAtBase = TRUE;
        }

        for (i = 0; i < NrSegments; i++)
        {
            PVOID SBaseAddress = (PVOID)
                                 ((char*)ImageBase + (ULONG_PTR)SectionSegments[i].Image.VirtualAddress);
            MmLockSectionSegment(&SectionSegments[i]);
            Status = MmMapViewOfSegment(AddressSpace,
                                        Section,
                                        &SectionSegments[i],
                                        &SBaseAddress,
                                        SectionSegments[i].Length.LowPart,
                                        SectionSegments[i].Protection,
                                        0,
                                        0);
            MmUnlockSectionSegment(&SectionSegments[i]);
            if (!NT_SUCCESS(Status))
            {
                MmUnlockAddressSpace(AddressSpace);
                return(Status);
            }
        }

        *BaseAddress = (PVOID)ImageBase;
        *ViewSize = ImageSize;
    }
    else
    {
        /* check for write access */
        if ((Protect & (PAGE_READWRITE|PAGE_EXECUTE_READWRITE)) &&
                !(Section->SectionPageProtection & (PAGE_READWRITE|PAGE_EXECUTE_READWRITE)))
        {
            MmUnlockAddressSpace(AddressSpace);
            return STATUS_SECTION_PROTECTION;
        }
        /* check for read access */
        if ((Protect & (PAGE_READONLY|PAGE_WRITECOPY|PAGE_EXECUTE_READ|PAGE_EXECUTE_WRITECOPY)) &&
                !(Section->SectionPageProtection & (PAGE_READONLY|PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))
        {
            MmUnlockAddressSpace(AddressSpace);
            return STATUS_SECTION_PROTECTION;
        }
        /* check for execute access */
        if ((Protect & (PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)) &&
                !(Section->SectionPageProtection & (PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))
        {
            MmUnlockAddressSpace(AddressSpace);
            return STATUS_SECTION_PROTECTION;
        }

        if (SectionOffset == NULL)
        {
            ViewOffset = 0;
        }
        else
        {
            ViewOffset = SectionOffset->u.LowPart;
        }

        if ((ViewOffset % PAGE_SIZE) != 0)
        {
            MmUnlockAddressSpace(AddressSpace);
            return(STATUS_MAPPED_ALIGNMENT);
        }

        if ((*ViewSize) == 0)
        {
            (*ViewSize) = Section->MaximumSize.u.LowPart - ViewOffset;
        }
        else if (((*ViewSize)+ViewOffset) > Section->MaximumSize.u.LowPart)
        {
            (*ViewSize) = Section->MaximumSize.u.LowPart - ViewOffset;
        }

        *ViewSize = PAGE_ROUND_UP(*ViewSize);

        MmLockSectionSegment(Section->Segment);
        Status = MmMapViewOfSegment(AddressSpace,
                                    Section,
                                    Section->Segment,
                                    BaseAddress,
                                    *ViewSize,
                                    Protect,
                                    ViewOffset,
                                    AllocationType & (MEM_TOP_DOWN|SEC_NO_CHANGE));
        MmUnlockSectionSegment(Section->Segment);
        if (!NT_SUCCESS(Status))
        {
            MmUnlockAddressSpace(AddressSpace);
            return(Status);
        }
    }

    MmUnlockAddressSpace(AddressSpace);
    NT_ASSERT(*BaseAddress == ALIGN_DOWN_POINTER_BY(*BaseAddress, MM_VIRTMEM_GRANULARITY));

    if (NotAtBase)
        Status = STATUS_IMAGE_NOT_AT_BASE;
    else
        Status = STATUS_SUCCESS;

    return Status;
}

/*
 * @unimplemented
 */
BOOLEAN NTAPI
MmCanFileBeTruncated (IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
                      IN PLARGE_INTEGER   NewFileSize)
{
    /* Check whether an ImageSectionObject exists */
    if (SectionObjectPointer->ImageSectionObject != NULL)
    {
        DPRINT1("ERROR: File can't be truncated because it has an image section\n");
        return FALSE;
    }

    if (SectionObjectPointer->DataSectionObject != NULL)
    {
        PMM_SECTION_SEGMENT Segment;

        Segment = (PMM_SECTION_SEGMENT)SectionObjectPointer->
                  DataSectionObject;

        if (Segment->ReferenceCount != 0)
        {
#ifdef NEWCC
            CC_FILE_SIZES FileSizes;
            CcpLock();
            if (SectionObjectPointer->SharedCacheMap && (Segment->ReferenceCount > CcpCountCacheSections((PNOCC_CACHE_MAP)SectionObjectPointer->SharedCacheMap)))
            {
                CcpUnlock();
                /* Check size of file */
                if (SectionObjectPointer->SharedCacheMap)
                {
                    if (!CcGetFileSizes(Segment->FileObject, &FileSizes))
                    {
                        return FALSE;
                    }

                    if (NewFileSize->QuadPart <= FileSizes.FileSize.QuadPart)
                    {
                        return FALSE;
                    }
                }
            }
            else
                CcpUnlock();
#else
            /* Check size of file */
            if (SectionObjectPointer->SharedCacheMap)
            {
                PROS_SHARED_CACHE_MAP SharedCacheMap = SectionObjectPointer->SharedCacheMap;
                if (NewFileSize->QuadPart <= SharedCacheMap->FileSize.QuadPart)
                {
                    return FALSE;
                }
            }
#endif
        }
        else
        {
            /* Something must gone wrong
             * how can we have a Section but no
             * reference? */
            DPRINT("ERROR: DataSectionObject without reference!\n");
        }
    }

    DPRINT("FIXME: didn't check for outstanding write probes\n");

    return TRUE;
}




/*
 * @implemented
 */
BOOLEAN NTAPI
MmFlushImageSection (IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
                     IN MMFLUSH_TYPE   FlushType)
{
    BOOLEAN Result = TRUE;
#ifdef NEWCC
    PMM_SECTION_SEGMENT Segment;
#endif

    switch(FlushType)
    {
    case MmFlushForDelete:
        if (SectionObjectPointer->ImageSectionObject ||
                SectionObjectPointer->DataSectionObject)
        {
            return FALSE;
        }
#ifndef NEWCC
        CcRosRemoveIfClosed(SectionObjectPointer);
#endif
        return TRUE;
    case MmFlushForWrite:
    {
        DPRINT("MmFlushImageSection(%d)\n", FlushType);
#ifdef NEWCC
        Segment = (PMM_SECTION_SEGMENT)SectionObjectPointer->DataSectionObject;
#endif

        if (SectionObjectPointer->ImageSectionObject)
        {
            DPRINT1("SectionObject has ImageSection\n");
            return FALSE;
        }

#ifdef NEWCC
        CcpLock();
        Result = !SectionObjectPointer->SharedCacheMap || (Segment->ReferenceCount == CcpCountCacheSections((PNOCC_CACHE_MAP)SectionObjectPointer->SharedCacheMap));
        CcpUnlock();
        DPRINT("Result %d\n", Result);
#endif
        return Result;
    }
    }
    return FALSE;
}

/*
 * @implemented
 */
NTSTATUS NTAPI
MmMapViewInSystemSpace (IN PVOID SectionObject,
                        OUT PVOID * MappedBase,
                        IN OUT PSIZE_T ViewSize)
{
    PROS_SECTION_OBJECT Section;
    PMMSUPPORT AddressSpace;
    NTSTATUS Status;
    PAGED_CODE();

    if (MiIsRosSectionObject(SectionObject) == FALSE)
    {
        return MiMapViewInSystemSpace(SectionObject,
                                      &MmSession,
                                      MappedBase,
                                      ViewSize);
    }

    DPRINT("MmMapViewInSystemSpace() called\n");

    Section = (PROS_SECTION_OBJECT)SectionObject;
    AddressSpace = MmGetKernelAddressSpace();

    MmLockAddressSpace(AddressSpace);


    if ((*ViewSize) == 0)
    {
        (*ViewSize) = Section->MaximumSize.u.LowPart;
    }
    else if ((*ViewSize) > Section->MaximumSize.u.LowPart)
    {
        (*ViewSize) = Section->MaximumSize.u.LowPart;
    }

    MmLockSectionSegment(Section->Segment);


    Status = MmMapViewOfSegment(AddressSpace,
                                Section,
                                Section->Segment,
                                MappedBase,
                                *ViewSize,
                                PAGE_READWRITE,
                                0,
                                0);

    MmUnlockSectionSegment(Section->Segment);
    MmUnlockAddressSpace(AddressSpace);

    return Status;
}

NTSTATUS
NTAPI
MiRosUnmapViewInSystemSpace(IN PVOID MappedBase)
{
    PMMSUPPORT AddressSpace;
    NTSTATUS Status;

    DPRINT("MmUnmapViewInSystemSpace() called\n");

    AddressSpace = MmGetKernelAddressSpace();

    MmLockAddressSpace(AddressSpace);

    Status = MmUnmapViewOfSegment(AddressSpace, MappedBase);

    MmUnlockAddressSpace(AddressSpace);

    return Status;
}

/**********************************************************************
 * NAME       EXPORTED
 *  MmCreateSection@
 *
 * DESCRIPTION
 *  Creates a section object.
 *
 * ARGUMENTS
 * SectionObject (OUT)
 *  Caller supplied storage for the resulting pointer
 *  to a SECTION_OBJECT instance;
 *
 * DesiredAccess
 *  Specifies the desired access to the section can be a
 *  combination of:
 *   STANDARD_RIGHTS_REQUIRED |
 *   SECTION_QUERY   |
 *   SECTION_MAP_WRITE  |
 *   SECTION_MAP_READ  |
 *   SECTION_MAP_EXECUTE
 *
 * ObjectAttributes [OPTIONAL]
 *  Initialized attributes for the object can be used
 *  to create a named section;
 *
 * MaximumSize
 *  Maximizes the size of the memory section. Must be
 *  non-NULL for a page-file backed section.
 *  If value specified for a mapped file and the file is
 *  not large enough, file will be extended.
 *
 * SectionPageProtection
 *  Can be a combination of:
 *   PAGE_READONLY |
 *   PAGE_READWRITE |
 *   PAGE_WRITEONLY |
 *   PAGE_WRITECOPY
 *
 * AllocationAttributes
 *  Can be a combination of:
 *   SEC_IMAGE |
 *   SEC_RESERVE
 *
 * FileHandle
 *  Handle to a file to create a section mapped to a file
 *  instead of a memory backed section;
 *
 * File
 *  Unknown.
 *
 * RETURN VALUE
 *  Status.
 *
 * @implemented
 */
NTSTATUS NTAPI
MmCreateSection (OUT PVOID  * Section,
                 IN ACCESS_MASK  DesiredAccess,
                 IN POBJECT_ATTRIBUTES ObjectAttributes     OPTIONAL,
                 IN PLARGE_INTEGER  MaximumSize,
                 IN ULONG   SectionPageProtection,
                 IN ULONG   AllocationAttributes,
                 IN HANDLE   FileHandle   OPTIONAL,
                 IN PFILE_OBJECT  FileObject  OPTIONAL)
{
    NTSTATUS Status;
    ULONG Protection;
    PROS_SECTION_OBJECT *SectionObject = (PROS_SECTION_OBJECT *)Section;

    /* Check if an ARM3 section is being created instead */
    if (!(AllocationAttributes & (SEC_IMAGE | SEC_PHYSICALMEMORY)))
    {
        if (!(FileObject) && !(FileHandle))
        {
            return MmCreateArm3Section(Section,
                                       DesiredAccess,
                                       ObjectAttributes,
                                       MaximumSize,
                                       SectionPageProtection,
                                       AllocationAttributes &~ 1,
                                       FileHandle,
                                       FileObject);
        }
    }

    /* Convert section flag to page flag */
    if (AllocationAttributes & SEC_NOCACHE) SectionPageProtection |= PAGE_NOCACHE;

    /* Check to make sure the protection is correct. Nt* does this already */
    Protection = MiMakeProtectionMask(SectionPageProtection);
    if (Protection == MM_INVALID_PROTECTION)
    {
        DPRINT1("Page protection is invalid\n");
        return STATUS_INVALID_PAGE_PROTECTION;
    }

    /* Check if this is going to be a data or image backed file section */
    if ((FileHandle) || (FileObject))
    {
        /* These cannot be mapped with large pages */
        if (AllocationAttributes & SEC_LARGE_PAGES)
        {
            DPRINT1("Large pages cannot be used with an image mapping\n");
            return STATUS_INVALID_PARAMETER_6;
        }

        /* Did the caller pass an object? */
        if (FileObject)
        {
            /* Reference the object directly */
            ObReferenceObject(FileObject);
        }
        else
        {
            /* Reference the file handle to get the object */
            Status = ObReferenceObjectByHandle(FileHandle,
                                               MmMakeFileAccess[Protection],
                                               IoFileObjectType,
                                               ExGetPreviousMode(),
                                               (PVOID*)&FileObject,
                                               NULL);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to get a handle to the FO: %lx\n", Status);
                return Status;
            }
        }
    }
    else
    {
        /* A handle must be supplied with SEC_IMAGE, as this is the no-handle path */
        if (AllocationAttributes & SEC_IMAGE) return STATUS_INVALID_FILE_FOR_SECTION;
    }

#ifndef NEWCC // A hack for initializing caching.
    // This is needed only in the old case.
    if (FileHandle)
    {
        IO_STATUS_BLOCK Iosb;
        NTSTATUS Status;
        CHAR Buffer;
        LARGE_INTEGER ByteOffset;
        ByteOffset.QuadPart = 0;
        Status = ZwReadFile(FileHandle,
                            NULL,
                            NULL,
                            NULL,
                            &Iosb,
                            &Buffer,
                            sizeof(Buffer),
                            &ByteOffset,
                            NULL);
        if (!NT_SUCCESS(Status) && Status != STATUS_END_OF_FILE)
        {
            DPRINT1("CC failure: %lx\n", Status);
            return Status;
        }
        // Caching is initialized...
    }
#endif

    if (AllocationAttributes & SEC_IMAGE)
    {
        Status = MmCreateImageSection(SectionObject,
                                      DesiredAccess,
                                      ObjectAttributes,
                                      MaximumSize,
                                      SectionPageProtection,
                                      AllocationAttributes,
                                      FileObject);
    }
#ifndef NEWCC
    else if (FileHandle != NULL)
    {
        Status =  MmCreateDataFileSection(SectionObject,
                                          DesiredAccess,
                                          ObjectAttributes,
                                          MaximumSize,
                                          SectionPageProtection,
                                          AllocationAttributes,
                                          FileHandle);
        if (FileObject)
            ObDereferenceObject(FileObject);
    }
#else
    else if (FileHandle != NULL || FileObject != NULL)
    {
        Status = MmCreateCacheSection(SectionObject,
                                      DesiredAccess,
                                      ObjectAttributes,
                                      MaximumSize,
                                      SectionPageProtection,
                                      AllocationAttributes,
                                      FileObject);
    }
#endif
    else
    {
        if ((AllocationAttributes & SEC_PHYSICALMEMORY) == 0)
        {
            DPRINT1("Invalid path: %lx %p %p\n", AllocationAttributes, FileObject, FileHandle);
        }
//        ASSERT(AllocationAttributes & SEC_PHYSICALMEMORY);
        Status = MmCreatePageFileSection(SectionObject,
                                         DesiredAccess,
                                         ObjectAttributes,
                                         MaximumSize,
                                         SectionPageProtection,
                                         AllocationAttributes);
    }

    return Status;
}

/* EOF */
