/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: pe.c,v 1.1.2.2 2004/12/09 19:31:26 hyperion Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/pe.c
 * PURPOSE:         Loader for PE executables
 * PROGRAMMER:      KJK::Hyperion <hackbunny@reactos.com>
 * UPDATE HISTORY:
 *                  2004-12-06 Created
 *                  2004-12-09 Compiles
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

#include <reactos/exeformat.h>

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
 PAGE_WRITECOPY,         /* 8 = WRITABLE */
 PAGE_READWRITE,         /* 9 = WRITABLE, SHARED */
 PAGE_EXECUTE_WRITECOPY, /* 10 = WRITABLE, EXECUTABLE */
 PAGE_EXECUTE_READWRITE, /* 11 = WRITABLE, EXECUTABLE, SHARED */
 PAGE_WRITECOPY,         /* 12 = WRITABLE, READABLE */
 PAGE_READWRITE,         /* 13 = WRITABLE, READABLE, SHARED */
 PAGE_EXECUTE_WRITECOPY, /* 14 = WRITABLE, READABLE, EXECUTABLE */
 PAGE_EXECUTE_READWRITE, /* 15 = WRITABLE, READABLE, EXECUTABLE, SHARED */
};

/* TODO: Intsafe should be made into a library, as it's generally useful */
BOOLEAN FASTCALL Intsafe_CanAddULongPtr
(
 IN ULONG_PTR Addend1,
 IN ULONG_PTR Addend2
)
{
 return Addend1 <= (MAXULONG_PTR - Addend2);
}

BOOLEAN FASTCALL Intsafe_AddULongPtr
(
 OUT PULONG_PTR Result,
 IN ULONG_PTR Addend1,
 IN ULONG_PTR Addend2
)
{
 if(!Intsafe_CanAddULongPtr(Addend1, Addend2))
  return FALSE;

 *Result = Addend1 + Addend2;
 return TRUE;
}

BOOLEAN FASTCALL Intsafe_CanMulULongPtr
(
 IN ULONG_PTR Factor1,
 IN ULONG_PTR Factor2
)
{
 return Factor1 <= (MAXULONG_PTR / Factor2);
}

BOOLEAN FASTCALL Intsafe_MulULongPtr
(
 OUT PULONG_PTR Result,
 IN ULONG_PTR Factor1,
 IN ULONG_PTR Factor2
)
{
 if(!Intsafe_CanMulULongPtr(Factor1, Factor2))
  return FALSE;

 *Result = Factor1 * Factor2;
 return TRUE;
}

BOOLEAN FASTCALL Intsafe_CanAddULong32
(
 IN ULONG Addend1,
 IN ULONG Addend2
)
{
 return Addend1 <= (MAXULONG - Addend2);
}

BOOLEAN FASTCALL Intsafe_AddULong32
(
 OUT PULONG Result,
 IN ULONG Addend1,
 IN ULONG Addend2
)
{
 if(!Intsafe_CanAddULong32(Addend1, Addend2))
  return FALSE;

 *Result = Addend1 + Addend2;
 return TRUE;
}

BOOLEAN FASTCALL Intsafe_CanMulULong32
(
 IN ULONG Factor1,
 IN ULONG Factor2
)
{
 return Factor1 <= (MAXULONG / Factor2);
}

BOOLEAN FASTCALL Intsafe_MulULong32
(
 OUT PULONG Result,
 IN ULONG Factor1,
 IN ULONG Factor2
)
{
 if(!Intsafe_CanMulULong32(Factor1, Factor2))
  return FALSE;

 *Result = Factor1 * Factor2;
 return TRUE;
}

BOOLEAN FASTCALL Intsafe_CanOffsetPointer
(
 IN CONST VOID * Pointer,
 IN SIZE_T Offset
)
{
 /* FIXME: (PVOID)MAXULONG_PTR isn't necessarily a valid address */
 return Intsafe_CanAddULongPtr((ULONG_PTR)Pointer, Offset);
}

/* TODO: these are standard DDK/PSDK macros */
#ifndef RTL_FIELD_SIZE
#define RTL_FIELD_SIZE(TYPE_, FIELD_) (sizeof(((TYPE_ *)0)->FIELD_))
#endif

#ifndef RTL_SIZEOF_THROUGH_FIELD
#define RTL_SIZEOF_THROUGH_FIELD(TYPE_, FIELD_) \
 (FIELD_OFFSET(TYPE_, FIELD_) + RTL_FIELD_SIZE(TYPE_, FIELD_))
#endif

#ifndef RTL_CONTAINS_FIELD
#define RTL_CONTAINS_FIELD(P_, SIZE_, FIELD_) \
 ((((char *)(P_)) + (SIZE_)) > (((char *)(&((P_)->FIELD_))) + sizeof((P_)->FIELD_)))
#endif

BOOLEAN FASTCALL IsPowerOf2(IN ULONG Number)
{
 if(Number == 0)
  return FALSE;

 while((Number % 2) == 0)
  Number /= 2;

 return Number == 1;
}

ULONG FASTCALL GetExcess(IN ULONG Address, IN ULONG Alignment)
{
 ASSERT(IsPowerOf2(Alignment));
 return Address & ~(Alignment - 1);
}

BOOLEAN FASTCALL IsAligned(IN ULONG Address, IN ULONG Alignment)
{
 return GetExcess(Address, Alignment) == 0;
}

BOOLEAN FASTCALL AlignUp
(
 OUT PULONG AlignedAddress,
 IN ULONG Address,
 IN ULONG Alignment
)
{
 return Intsafe_AddULong32
 (
  AlignedAddress,
  Address,
  Alignment - GetExcess(Address, Alignment)
 );
}

/*
 References:
  [1] Microsoft Corporation, "Microsoft Portable Executable and Common Object
      File Format Specification", revision 6.0 (February 1999)
*/
NTSTATUS NTAPI PeFmtCreateSection
(
 IN CONST VOID * FileHeader,
 IN SIZE_T FileHeaderSize,
 IN PFILE_OBJECT File,
 OUT PMM_IMAGE_SECTION_OBJECT ImageSectionObject,
 OUT PULONG Flags,
 IN PEXEFMT_LOADER_READ_FILE ReadFileFunc,
 IN PEXEFMT_LOADER_ALLOCATE_SEGMENTS AllocateSegmentsFunc
)
{
 NTSTATUS nStatus;
 ULONG cbFileHeaderOffsetSize;
 ULONG cbSectionHeadersOffset;
 ULONG cbSectionHeadersSize;
 ULONG cbSectionHeadersOffsetSize;
 ULONG cbOptHeaderSize;
 ULONG cbHeadersSize;
 ULONG nSectionAlignment;
 ULONG nFileAlignment;
 const IMAGE_DOS_HEADER * pidhDosHeader;
 const IMAGE_NT_HEADERS32 * pinhNtHeader;
 const IMAGE_OPTIONAL_HEADER32 * piohOptHeader;
 const IMAGE_SECTION_HEADER * pishSectionHeaders;
 PMM_SECTION_SEGMENT pssSegments;
 LARGE_INTEGER lnOffset;
 PVOID pBuffer;
 ULONG nPrevVirtualEndOfSegment;
 ULONG nPrevFileEndOfSegment;
 ULONG i;

 ASSERT(FileHeader);
 ASSERT(FileHeaderSize > 0);
 ASSERT(File);
 ASSERT(ImageSectionObject);
 ASSERT(ReadFileFunc);
 ASSERT(AllocateSegmentsFunc);
 
 ASSERT(Intsafe_CanOffsetPointer(FileHeader, FileHeaderSize));

 ASSERT(FileHeaderSize >= sizeof(IMAGE_DOS_HEADER));
 ASSERT(((UINT_PTR)FileHeader % TYPE_ALIGNMENT(IMAGE_DOS_HEADER)) == 0);

 pBuffer = NULL;
 pidhDosHeader = FileHeader;

 /* DOS HEADER */
 nStatus = STATUS_ROS_EXEFMT_UNKNOWN_FORMAT;

 /* no MZ signature */
 if(pidhDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
  goto l_Return;

 /* not a Windows executable */
 if(pidhDosHeader->e_lfanew <= 0)
  goto l_Return;

 /* NT HEADER */
 nStatus = STATUS_INVALID_IMAGE_FORMAT;

 if(!Intsafe_AddULong32(&cbFileHeaderOffsetSize, pidhDosHeader->e_lfanew, RTL_SIZEOF_THROUGH_FIELD(IMAGE_NT_HEADERS32, FileHeader)))
  goto l_Return;

 if(FileHeaderSize < cbFileHeaderOffsetSize)
  pinhNtHeader = NULL;
 else
 {
  /*
   we already know that Intsafe_CanOffsetPointer(FileHeader, FileHeaderSize),
   and FileHeaderSize >= cbFileHeaderOffsetSize, so this holds true too
  */
  ASSERT(Intsafe_CanOffsetPointer(FileHeader, pidhDosHeader->e_lfanew));
  pinhNtHeader = (PVOID)((UINT_PTR)FileHeader + pidhDosHeader->e_lfanew);
 }

 ASSERT(sizeof(IMAGE_NT_HEADERS32) <= sizeof(IMAGE_NT_HEADERS64));
 ASSERT(TYPE_ALIGNMENT(IMAGE_NT_HEADERS32) == TYPE_ALIGNMENT(IMAGE_NT_HEADERS64));
 ASSERT(RTL_SIZEOF_THROUGH_FIELD(IMAGE_NT_HEADERS32, FileHeader) == RTL_SIZEOF_THROUGH_FIELD(IMAGE_NT_HEADERS64, FileHeader));
 ASSERT(FIELD_OFFSET(IMAGE_NT_HEADERS32, OptionalHeader) == FIELD_OFFSET(IMAGE_NT_HEADERS64, OptionalHeader));

 /*
  the buffer doesn't contain the NT file header, or the alignment is wrong: we
  need to read the header from the file
 */
 if
 (
  FileHeaderSize < cbFileHeaderOffsetSize ||
  (UINT_PTR)pinhNtHeader % TYPE_ALIGNMENT(IMAGE_NT_HEADERS32) != 0
 )
 {
  SIZE_T cbNtHeaderSize;
  SIZE_T cbReadSize;

l_ReadHeaderFromFile:

  nStatus = STATUS_INSUFFICIENT_RESOURCES;

  /* allocate the buffer. We use the largest size (IMAGE_NT_HEADERS64) */
  pBuffer = ExAllocatePoolWithTag
  (
   NonPagedPool,
   sizeof(IMAGE_NT_HEADERS64),
   TAG('P', 'e', 'F', 'm')
  );

  if(pBuffer == NULL)
   goto l_Return;

  lnOffset.QuadPart = pidhDosHeader->e_lfanew;

  /* read the header from the file */
  nStatus = ReadFileFunc
  (
   File,
   pBuffer,
   sizeof(IMAGE_NT_HEADERS64),
   &lnOffset,
   &cbReadSize
  );

  if(!NT_SUCCESS(nStatus))
   goto l_Return;

  nStatus = STATUS_UNSUCCESSFUL;

  if(cbReadSize == 0)
   goto l_Return;

  nStatus = STATUS_INVALID_IMAGE_FORMAT;

  /* the buffer doesn't contain the file header */
  if(cbReadSize < RTL_SIZEOF_THROUGH_FIELD(IMAGE_NT_HEADERS32, FileHeader))
   goto l_Return;

  pinhNtHeader = pBuffer;

  /* invalid NT header */
  if(pinhNtHeader->Signature != IMAGE_NT_SIGNATURE)
   goto l_Return;

  if(!Intsafe_AddULong32(&cbNtHeaderSize, pinhNtHeader->FileHeader.SizeOfOptionalHeader, FIELD_OFFSET(IMAGE_NT_HEADERS32, OptionalHeader)))
   goto l_Return;

  nStatus = STATUS_UNSUCCESSFUL;

  /* the buffer doesn't contain the whole NT header */
  if(cbReadSize < cbNtHeaderSize)
   goto l_Return;
 }
 else
 {
  SIZE_T cbOptHeaderOffsetSize;

  nStatus = STATUS_INVALID_IMAGE_FORMAT;

  /* don't trust an invalid NT header */
  if(pinhNtHeader->Signature != IMAGE_NT_SIGNATURE)
   goto l_Return;
  
  if(!Intsafe_AddULong32(&cbOptHeaderOffsetSize, pidhDosHeader->e_lfanew, FIELD_OFFSET(IMAGE_NT_HEADERS32, OptionalHeader)))
   goto l_Return;

  if(!Intsafe_AddULong32(&cbOptHeaderOffsetSize, cbOptHeaderOffsetSize, pinhNtHeader->FileHeader.SizeOfOptionalHeader))
   goto l_Return;

  /* the buffer doesn't contain the whole NT header: read it from the file */
  if(cbOptHeaderOffsetSize > FileHeaderSize)
   goto l_ReadHeaderFromFile;
 }

 /* read information from the NT header */
 piohOptHeader = &pinhNtHeader->OptionalHeader;
 cbOptHeaderSize = pinhNtHeader->FileHeader.SizeOfOptionalHeader;

 nStatus = STATUS_INVALID_IMAGE_FORMAT;

 if(!RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, Magic))
  goto l_Return;

 /* ASSUME: RtlZeroMemory(ImageSectionObject, sizeof(*ImageSectionObject)); */

 switch(piohOptHeader->Magic)
 {
  case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
  case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
   break;
  
  default:
   goto l_Return;
 }

 if
 (
  RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, SectionAlignment) &&
  RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, FileAlignment)
 )
 {
  /* See [1], section 3.4.2 */
  if(piohOptHeader->SectionAlignment < PAGE_SIZE)
  {
   if(piohOptHeader->FileAlignment != piohOptHeader->SectionAlignment)
    goto l_Return;
  }
  else if(piohOptHeader->SectionAlignment < piohOptHeader->FileAlignment)
   goto l_Return;

  nSectionAlignment = piohOptHeader->SectionAlignment;
  nFileAlignment = piohOptHeader->FileAlignment;

  if(!IsPowerOf2(nSectionAlignment) || !IsPowerOf2(nFileAlignment))
   goto l_Return;
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
   if(cbOptHeaderSize > sizeof(IMAGE_OPTIONAL_HEADER32))
    goto l_Return;

   if(RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, ImageBase))
    ImageSectionObject->ImageBase = piohOptHeader->ImageBase;

   if(RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, SizeOfStackReserve))
    ImageSectionObject->StackReserve = piohOptHeader->SizeOfStackReserve;

   if(RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, SizeOfStackCommit))
    ImageSectionObject->StackCommit = piohOptHeader->SizeOfStackCommit;

   break;
  }

  /* PE32+ */
  case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
  {
   PIMAGE_OPTIONAL_HEADER64 pioh64OptHeader;

   if(cbOptHeaderSize > sizeof(IMAGE_OPTIONAL_HEADER64))
    goto l_Return;

   pioh64OptHeader = (PIMAGE_OPTIONAL_HEADER64)piohOptHeader;

   if(RTL_CONTAINS_FIELD(pioh64OptHeader, cbOptHeaderSize, ImageBase))
   {
    if(pioh64OptHeader->ImageBase > MAXULONG_PTR)
     goto l_Return;

    ImageSectionObject->ImageBase = pioh64OptHeader->ImageBase;
   }

   if(RTL_CONTAINS_FIELD(pioh64OptHeader, cbOptHeaderSize, SizeOfStackReserve))
   {
    if(pioh64OptHeader->SizeOfStackReserve > MAXULONG_PTR)
     goto l_Return;

    ImageSectionObject->StackReserve = pioh64OptHeader->SizeOfStackReserve;
   }

   if(RTL_CONTAINS_FIELD(pioh64OptHeader, cbOptHeaderSize, SizeOfStackCommit))
   {
    if(pioh64OptHeader->SizeOfStackCommit > MAXULONG_PTR)
     goto l_Return;

    ImageSectionObject->StackCommit = pioh64OptHeader->SizeOfStackCommit;
   }

   break;
  }
 }

#if 0
 /* some defaults */
 if(ImageSectionObject->StackReserve == 0)
  ImageSectionObject->StackReserve = 0x40000;

 if(ImageSectionObject->StackCommit == 0)
  ImageSectionObject->StackCommit = 0x1000;

 if(ImageSectionObject->ImageBase == NULL)
 {
  if(pinhNtHeader->FileHeader.Characteristics & IMAGE_FILE_DLL)
   ImageSectionObject->ImageBase = (PVOID)0x10000000;
  else
   ImageSectionObject->ImageBase = (PVOID)0x00400000;
 }
#endif

 /* [1], section 3.4.2 */
 if((ULONG_PTR)ImageSectionObject->ImageBase % 0x10000)
  goto l_Return;

 /*
  ASSUME: all the fields used here have the same offset and size in both
  IMAGE_OPTIONAL_HEADER32 and IMAGE_OPTIONAL_HEADER64
 */
 if(RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, Subsystem))
 {
  ImageSectionObject->Subsystem = piohOptHeader->Subsystem;

  if
  (
   RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, MinorSubsystemVersion) &&
   RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, MajorSubsystemVersion)
  )
  {
   ImageSectionObject->MinorSubsystemVersion = piohOptHeader->MinorSubsystemVersion;
   ImageSectionObject->MajorSubsystemVersion = piohOptHeader->MajorSubsystemVersion;
  }
 }

 if(RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, AddressOfEntryPoint))
  ImageSectionObject->EntryPoint = piohOptHeader->AddressOfEntryPoint;

 if(RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, SizeOfCode))
  ImageSectionObject->Executable = piohOptHeader->SizeOfCode != 0;
 else
  ImageSectionObject->Executable = TRUE;

 ImageSectionObject->ImageCharacteristics = pinhNtHeader->FileHeader.Characteristics;
 ImageSectionObject->Machine = pinhNtHeader->FileHeader.Machine;

 /* SECTION HEADERS */
 nStatus = STATUS_INVALID_IMAGE_FORMAT;

 /* see [1], section 3.3 */
 if(pinhNtHeader->FileHeader.NumberOfSections > 96)
  goto l_Return;

 /*
  the additional segment is for the file's headers. They need to be present for
  the benefit of the dynamic loader (to locate exports, defaults for thread
  parameters, resources, etc.)
 */
 ImageSectionObject->NrSegments = pinhNtHeader->FileHeader.NumberOfSections + 1;

 /* file offset for the section headers */
 if(!Intsafe_AddULong32(&cbSectionHeadersOffset, pidhDosHeader->e_lfanew, FIELD_OFFSET(IMAGE_NT_HEADERS32, OptionalHeader)))
  goto l_Return;

 if(!Intsafe_AddULong32(&cbSectionHeadersOffset, cbSectionHeadersOffset, pinhNtHeader->FileHeader.SizeOfOptionalHeader))
  goto l_Return;

 /* size of the section headers */
 ASSERT(Intsafe_CanMulULong32(pinhNtHeader->FileHeader.NumberOfSections, sizeof(IMAGE_SECTION_HEADER)));
 cbSectionHeadersSize = pinhNtHeader->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);

 if(!Intsafe_AddULong32(&cbSectionHeadersOffsetSize, cbSectionHeadersOffset, cbSectionHeadersSize))
  goto l_Return;

 /* size of the executable's headers */
 if(RTL_CONTAINS_FIELD(piohOptHeader, cbOptHeaderSize, SizeOfHeaders))
 {
  if(!IsAligned(piohOptHeader->SizeOfHeaders, nFileAlignment))
   goto l_Return;

  if(cbSectionHeadersSize > piohOptHeader->SizeOfHeaders)
   goto l_Return;

  cbHeadersSize = piohOptHeader->SizeOfHeaders;
 }
 else if(!AlignUp(&cbHeadersSize, cbSectionHeadersOffsetSize, nFileAlignment))
  goto l_Return;

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
   we already know that Intsafe_CanOffsetPointer(FileHeader, FileHeaderSize),
   and FileHeaderSize >= cbSectionHeadersOffsetSize, so this holds true too
  */
  ASSERT(Intsafe_CanOffsetPointer(FileHeader, cbSectionHeadersOffset));
  pishSectionHeaders = (PVOID)((UINT_PTR)FileHeader + cbSectionHeadersOffset);
 }

 /*
  the buffer doesn't contain the section headers, or the alignment is wrong:
  read the headers from the file
 */
 if
 (
  FileHeaderSize < cbSectionHeadersOffsetSize ||
  (UINT_PTR)pishSectionHeaders % TYPE_ALIGNMENT(IMAGE_SECTION_HEADER) != 0
 )
 {
  SIZE_T cbReadSize;

  nStatus = STATUS_INSUFFICIENT_RESOURCES;

  /* allocate the buffer */
  pBuffer = ExAllocatePoolWithTag
  (
   NonPagedPool,
   cbSectionHeadersSize,
   TAG('P', 'e', 'F', 'm')
  );

  if(pBuffer == NULL)
   goto l_Return;

  lnOffset.QuadPart = cbSectionHeadersOffset;

  /* read the header from the file */
  nStatus = ReadFileFunc
  (
   File,
   pBuffer,
   cbSectionHeadersSize,
   &lnOffset,
   &cbReadSize
  );

  if(!NT_SUCCESS(nStatus))
   goto l_Return;

  nStatus = STATUS_UNSUCCESSFUL;

  /* the buffer doesn't contain all the section headers */
  if(cbReadSize < cbSectionHeadersSize)
   goto l_Return;
 }

 /* SEGMENTS */
 /* allocate the segments */
 nStatus = STATUS_INSUFFICIENT_RESOURCES;
 ImageSectionObject->Segments = AllocateSegmentsFunc(ImageSectionObject->NrSegments);

 if(ImageSectionObject->Segments == NULL)
  goto l_Return;

 /* initialize the headers segment */
 pssSegments = ImageSectionObject->Segments;

 ASSERT(IsAligned(cbHeadersSize, nFileAlignment));

 if(!AlignUp(&nPrevVirtualEndOfSegment, cbHeadersSize, nSectionAlignment))
  goto l_Return;

 pssSegments[0].FileOffset = 0;
 pssSegments[0].Protection = PAGE_READONLY;
 pssSegments[0].Length = nPrevVirtualEndOfSegment;
 pssSegments[0].RawLength = nPrevFileEndOfSegment;
 pssSegments[0].VirtualAddress = 0;
 pssSegments[0].Characteristics = 0;

 /* skip the headers segment */
 ++ pssSegments;

 nStatus = STATUS_INVALID_IMAGE_FORMAT;

 /* convert the executable sections into segments. See also [1], section 4 */
 for(i = 0; i < ImageSectionObject->NrSegments - 1; ++ i)
 {
  ULONG nCharacteristics;

  /* validate the alignment */
  if(!IsAligned(pishSectionHeaders[i].SizeOfRawData, nFileAlignment))
   goto l_Return;

  if(!IsAligned(pishSectionHeaders[i].PointerToRawData, nFileAlignment))
   goto l_Return;

  if(!IsAligned(pishSectionHeaders[i].Misc.VirtualSize, nSectionAlignment))
   goto l_Return;

  if(!IsAligned(pishSectionHeaders[i].VirtualAddress, nSectionAlignment))
   goto l_Return;

  /* sections must be contiguous, ordered by base address and non-overlapping */
  if(pishSectionHeaders[i].PointerToRawData != nPrevFileEndOfSegment)
   goto l_Return;

  if(pishSectionHeaders[i].VirtualAddress != nPrevVirtualEndOfSegment)
   goto l_Return;

  /* conversion */
  pssSegments[i].FileOffset = pishSectionHeaders[i].PointerToRawData;

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

  pssSegments[i].RawLength = pishSectionHeaders[i].SizeOfRawData;

  if(pishSectionHeaders[i].Misc.VirtualSize != 0)
   pssSegments[i].Length = pishSectionHeaders[i].Misc.VirtualSize;
  else
   pssSegments[i].Length = pishSectionHeaders[i].SizeOfRawData;

  /* ExInitializeFastMutex(&pssSegments[i].Lock); */
  /* pssSegments[i].ReferenceCount = 1; */
  /* RtlZeroMemory(&pssSegments[i].PageDirectory, sizeof(pssSegments[i].PageDirectory)); */
  pssSegments[i].VirtualAddress = pishSectionHeaders[i].VirtualAddress;
  pssSegments[i].Characteristics = pishSectionHeaders[i].Characteristics;

  /* ensure the image is no larger than 4GB */
  if(!Intsafe_AddULong32(&nPrevFileEndOfSegment, pssSegments[i].FileOffset, pssSegments[i].RawLength))
   goto l_Return;

  if(!Intsafe_AddULong32(&nPrevVirtualEndOfSegment, pssSegments[i].VirtualAddress, pssSegments[i].Length))
   goto l_Return;
 }

 /* spare our caller some work in validating the segments */
 *Flags |=
  EXEFMT_LOAD_ASSUME_SEGMENTS_SORTED |
  EXEFMT_LOAD_ASSUME_SEGMENTS_NO_OVERLAP;

 if(nSectionAlignment >= PAGE_SIZE)
  *Flags |= EXEFMT_LOAD_ASSUME_SEGMENTS_PAGE_ALIGNED;

 /* Success */
 nStatus = STATUS_ROS_EXEFMT_LOADED_FORMAT & EXEFMT_LOADED_PE32;

l_Return:
 if(pBuffer)
  ExFreePool(pBuffer);

 return nStatus;
}

/* EOF */
