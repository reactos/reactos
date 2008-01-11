/* $Id$
*/

#define NDEBUG
#include <internal/debug.h>

#include <reactos/exeformat.h>

#ifndef __ELF_WORD_SIZE
#error __ELF_WORD_SIZE must be defined
#endif

#ifndef MAXULONG
#define MAXULONG ((ULONG)(~1))
#endif

#include <elf/elf.h>

/* TODO: Intsafe should be made into a library, as it's generally useful */
static __inline BOOLEAN Intsafe_CanAddULongPtr
(
 IN ULONG_PTR Addend1,
 IN ULONG_PTR Addend2
)
{
 return Addend1 <= (MAXULONG_PTR - Addend2);
}

#define Intsafe_CanAddSizeT Intsafe_CanAddULongPtr

static __inline BOOLEAN Intsafe_CanAddULong32
(
 IN ULONG Addend1,
 IN ULONG Addend2
)
{
 return Addend1 <= (MAXULONG - Addend2);
}

static __inline BOOLEAN Intsafe_AddULong32
(
 OUT PULONG32 Result,
 IN ULONG Addend1,
 IN ULONG Addend2
)
{
 if(!Intsafe_CanAddULong32(Addend1, Addend2))
  return FALSE;

 *Result = Addend1 + Addend2;
 return TRUE;
}

static __inline BOOLEAN Intsafe_CanAddULong64
(
 IN ULONG64 Addend1,
 IN ULONG64 Addend2
)
{
 return Addend1 <= (((ULONG64)-1) - Addend2);
}

static __inline BOOLEAN Intsafe_AddULong64
(
 OUT PULONG64 Result,
 IN ULONG64 Addend1,
 IN ULONG64 Addend2
)
{
 if(!Intsafe_CanAddULong64(Addend1, Addend2))
  return FALSE;

 *Result = Addend1 + Addend2;
 return TRUE;
}

static __inline BOOLEAN Intsafe_CanMulULong32
(
 IN ULONG Factor1,
 IN ULONG Factor2
)
{
 return Factor1 <= (MAXULONG / Factor2);
}

static __inline BOOLEAN Intsafe_MulULong32
(
 OUT PULONG32 Result,
 IN ULONG Factor1,
 IN ULONG Factor2
)
{
 if(!Intsafe_CanMulULong32(Factor1, Factor2))
  return FALSE;

 *Result = Factor1 * Factor2;
 return TRUE;
}

static __inline BOOLEAN Intsafe_CanOffsetPointer
(
 IN CONST VOID * Pointer,
 IN SIZE_T Offset
)
{
 /* FIXME: (PVOID)MAXULONG_PTR isn't necessarily a valid address */
 return Intsafe_CanAddULongPtr((ULONG_PTR)Pointer, Offset);
}

#if __ELF_WORD_SIZE == 32
#define ElfFmtpAddSize Intsafe_AddULong32
#define ElfFmtpReadAddr ElfFmtpReadULong
#define ElfFmtpReadOff  ElfFmtpReadULong
#define ElfFmtpSafeReadAddr ElfFmtpSafeReadULong
#define ElfFmtpSafeReadOff  ElfFmtpSafeReadULong
#define ElfFmtpSafeReadSize ElfFmtpSafeReadULong
#elif __ELF_WORD_SIZE == 64
#define ElfFmtpAddSize Intsafe_AddULong64
#define ElfFmtpReadAddr ElfFmtpReadULong64
#define ElfFmtpReadOff  ElfFmtpReadULong64
#define ElfFmtpSafeReadAddr ElfFmtpSafeReadULong64
#define ElfFmtpSafeReadOff  ElfFmtpSafeReadULong64
#define ElfFmtpSafeReadSize ElfFmtpSafeReadULong64
#endif

/* TODO: these are standard DDK/PSDK macros */
#define RtlRetrieveUlonglong(DST_, SRC_) \
 (RtlCopyMemory((DST_), (SRC_), sizeof(ULONG64)))

#ifndef RTL_FIELD_SIZE
#define RTL_FIELD_SIZE(TYPE_, FIELD_) (sizeof(((TYPE_ *)0)->FIELD_))
#endif

#ifndef RTL_SIZEOF_THROUGH_FIELD
#define RTL_SIZEOF_THROUGH_FIELD(TYPE_, FIELD_) \
 (FIELD_OFFSET(TYPE_, FIELD_) + RTL_FIELD_SIZE(TYPE_, FIELD_))
#endif

#ifndef RTL_CONTAINS_FIELD
#define RTL_CONTAINS_FIELD(P_, SIZE_, FIELD_) \
 ((ULONG_PTR)(P_) + (ULONG_PTR)(SIZE_) > (ULONG_PTR)&((P_)->FIELD_) + sizeof((P_)->FIELD_))
#endif

#define ELFFMT_FIELDS_EQUAL(TYPE1_, TYPE2_, FIELD_) \
 ( \
  (FIELD_OFFSET(TYPE1_, FIELD_) == FIELD_OFFSET(TYPE2_, FIELD_)) && \
  (RTL_FIELD_SIZE(TYPE1_, FIELD_) == RTL_FIELD_SIZE(TYPE2_, FIELD_)) \
 )

#define ELFFMT_MAKE_ULONG64(BYTE1_, BYTE2_, BYTE3_, BYTE4_, BYTE5_, BYTE6_, BYTE7_, BYTE8_) \
 ( \
  (((ULONG64)ELFFMT_MAKE_ULONG(BYTE1_, BYTE2_, BYTE3_, BYTE4_)) <<  0) | \
  (((ULONG64)ELFFMT_MAKE_ULONG(BYTE5_, BYTE6_, BYTE7_, BYTE8_)) << 32) \
 )

#define ELFFMT_MAKE_ULONG(BYTE1_, BYTE2_, BYTE3_, BYTE4_) \
 ( \
  (((ULONG)ELFFMT_MAKE_USHORT(BYTE1_, BYTE2_)) <<  0) | \
  (((ULONG)ELFFMT_MAKE_USHORT(BYTE3_, BYTE4_)) << 16) \
 )

#define ELFFMT_MAKE_USHORT(BYTE1_, BYTE2_) \
 ( \
  (((USHORT)(BYTE1_)) << 0) | \
  (((USHORT)(BYTE2_)) << 8) \
 )

static __inline ULONG64 ElfFmtpReadULong64
(
 IN ULONG64 Input,
 IN ULONG DataType
)
{
 PUCHAR p;

 if(DataType == ELF_TARG_DATA)
  return Input;

 p = (PUCHAR)&Input;

 switch(DataType)
 {
  case ELFDATA2LSB: return ELFFMT_MAKE_ULONG64(p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
  case ELFDATA2MSB: return ELFFMT_MAKE_ULONG64(p[7], p[6], p[5], p[4], p[3], p[2], p[1], p[0]);
 }

 ASSERT(FALSE);
 return (ULONG64)-1;
}

static __inline ULONG ElfFmtpReadULong
(
 IN ULONG Input,
 IN ULONG DataType
)
{
 PUCHAR p;

 if(DataType == ELF_TARG_DATA)
  return Input;

 p = (PUCHAR)&Input;

 switch(DataType)
 {
  case ELFDATA2LSB: return ELFFMT_MAKE_ULONG(p[0], p[1], p[2], p[3]);
  case ELFDATA2MSB: return ELFFMT_MAKE_ULONG(p[3], p[2], p[1], p[0]);
 }

 ASSERT(FALSE);
 return (ULONG)-1;
}

static __inline USHORT ElfFmtpReadUShort
(
 IN USHORT Input,
 IN ULONG DataType
)
{
 PUCHAR p;

 if(DataType == ELF_TARG_DATA)
  return Input;

 p = (PUCHAR)&Input;

 switch(DataType)
 {
  case ELFDATA2LSB: return ELFFMT_MAKE_USHORT(p[0], p[1]);
  case ELFDATA2MSB: return ELFFMT_MAKE_USHORT(p[1], p[0]);
 }

 ASSERT(FALSE);
 return (USHORT)-1;
}

static __inline ULONG64 ElfFmtpSafeReadULong64
(
 IN CONST ULONG64 * Input,
 IN ULONG DataType
)
{
 PUCHAR p;
 ULONG64 nSafeInput;

 RtlRetrieveUlonglong(&nSafeInput, Input);

 p = (PUCHAR)&nSafeInput;

 switch(DataType)
 {
  case ELFDATA2LSB: return ELFFMT_MAKE_ULONG64(p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
  case ELFDATA2MSB: return ELFFMT_MAKE_ULONG64(p[7], p[6], p[5], p[4], p[3], p[2], p[1], p[0]);
 }

 ASSERT(FALSE);
 return (ULONG64)-1;
}

static __inline ULONG ElfFmtpSafeReadULong
(
 IN CONST ULONG32 * Input,
 IN ULONG DataType
)
{
 PUCHAR p;
 ULONG nSafeInput;
 union
 {
     CONST ULONG32 *ConstInput;
     ULONG32 *Input;
 }pInput = {Input};

 RtlRetrieveUlong(&nSafeInput, pInput.Input);

 if(DataType == ELF_TARG_DATA)
  return nSafeInput;

 p = (PUCHAR)&nSafeInput;

 switch(DataType)
 {
  case ELFDATA2LSB: return ELFFMT_MAKE_ULONG(p[0], p[1], p[2], p[3]);
  case ELFDATA2MSB: return ELFFMT_MAKE_ULONG(p[3], p[2], p[1], p[0]);
 }

 ASSERT(FALSE);
 return (ULONG)-1;
}

static __inline BOOLEAN ElfFmtpIsPowerOf2(IN Elf_Addr Number)
{
 if(Number == 0)
  return FALSE;

 return (Number & (Number - 1)) == 0;
}

static __inline Elf_Addr ElfFmtpModPow2
(
 IN Elf_Addr Address,
 IN Elf_Addr Alignment
)
{
 ASSERT(sizeof(Elf_Addr) == sizeof(Elf_Size));
 ASSERT(sizeof(Elf_Addr) == sizeof(Elf_Off));
 ASSERT(ElfFmtpIsPowerOf2(Alignment));
 return Address & (Alignment - 1);
}

static __inline Elf_Addr ElfFmtpAlignDown
(
 IN Elf_Addr Address,
 IN Elf_Addr Alignment
)
{
 ASSERT(sizeof(Elf_Addr) == sizeof(Elf_Size));
 ASSERT(sizeof(Elf_Addr) == sizeof(Elf_Off));
 ASSERT(ElfFmtpIsPowerOf2(Alignment));
 return Address & ~(Alignment - 1);
}

static __inline BOOLEAN ElfFmtpAlignUp
(
 OUT Elf_Addr * AlignedAddress,
 IN Elf_Addr Address,
 IN Elf_Addr Alignment
)
{
 Elf_Addr nExcess = ElfFmtpModPow2(Address, Alignment);

 if(nExcess == 0)
 {
  *AlignedAddress = Address;
  return nExcess == 0;
 }
 else
  return ElfFmtpAddSize(AlignedAddress, Address, Alignment - nExcess);
}

/*
 References:
  [1] Tool Interface Standards (TIS) Committee, "Executable and Linking Format
      (ELF) Specification", Version 1.2
*/
NTSTATUS NTAPI
#if __ELF_WORD_SIZE == 32
Elf32FmtCreateSection
#elif __ELF_WORD_SIZE == 64
Elf64FmtCreateSection
#endif
(
 IN CONST VOID * FileHeader,
 IN SIZE_T FileHeaderSize,
 IN PVOID File,
 OUT PMM_IMAGE_SECTION_OBJECT ImageSectionObject,
 OUT PULONG Flags,
 IN PEXEFMT_CB_READ_FILE ReadFileCb,
 IN PEXEFMT_CB_ALLOCATE_SEGMENTS AllocateSegmentsCb
)
{
 NTSTATUS nStatus;
 const Elf_Ehdr * pehHeader;
 const Elf_Phdr * pphPHdrs;
 BOOLEAN fPageAligned;
 ULONG nData;
 ULONG nPHdrCount;
 ULONG cbPHdrSize;
 Elf_Off cbPHdrOffset;
 PVOID pBuffer;
 PMM_SECTION_SEGMENT pssSegments;
 Elf_Addr nImageBase = 0;
 Elf_Addr nEntryPoint;
 ULONG32 nPrevVirtualEndOfSegment = 0;
 ULONG i;
 ULONG j;

 (void)Intsafe_AddULong64;
 (void)Intsafe_MulULong32;
 (void)ElfFmtpReadULong64;
 (void)ElfFmtpSafeReadULong64;
 (void)ElfFmtpReadULong;

#define DIE(ARGS_) { DPRINT ARGS_; goto l_Return; }

 pBuffer = NULL;

 nStatus = STATUS_INVALID_IMAGE_FORMAT;

 /* Ensure the file contains the full header */
 /*
  EXEFMT_LOAD_HEADER_SIZE is 8KB: enough to contain an ELF header (at least in
  all the classes defined as of December 2004). If FileHeaderSize is less than
  sizeof(Elf_Ehdr), it means the file itself is small enough not to contain a
  full ELF header
 */
 ASSERT(sizeof(Elf_Ehdr) <= EXEFMT_LOAD_HEADER_SIZE);

 if(FileHeaderSize < sizeof(Elf_Ehdr))
  DIE(("The file is truncated, doesn't contain the full header\n"));

 pehHeader = FileHeader;
 ASSERT(((ULONG_PTR)pehHeader % TYPE_ALIGNMENT(Elf_Ehdr)) == 0);

 nData = pehHeader->e_ident[EI_DATA];

 /* Validate the header */
 if(ElfFmtpReadUShort(pehHeader->e_ehsize, nData) < sizeof(Elf_Ehdr))
  DIE(("Inconsistent value for e_ehsize\n"));

 /* Calculate size and offset of the program headers */
 cbPHdrSize = ElfFmtpReadUShort(pehHeader->e_phentsize, nData);

 if(cbPHdrSize != sizeof(Elf_Phdr))
  DIE(("Inconsistent value for e_phentsize\n"));

 /* MAXUSHORT * MAXUSHORT < MAXULONG */
 nPHdrCount = ElfFmtpReadUShort(pehHeader->e_phnum, nData);
 ASSERT(Intsafe_CanMulULong32(cbPHdrSize, nPHdrCount));
 cbPHdrSize *= nPHdrCount;

 cbPHdrOffset = ElfFmtpReadOff(pehHeader->e_phoff, nData);

 /* The initial header doesn't contain the program headers */
 if(cbPHdrOffset > FileHeaderSize || cbPHdrSize > (FileHeaderSize - cbPHdrOffset))
 {
  NTSTATUS nReadStatus;
  LARGE_INTEGER lnOffset;
  PVOID pData;
  ULONG cbReadSize;

  /* Will worry about this when ELF128 comes */
  ASSERT(sizeof(cbPHdrOffset) <= sizeof(lnOffset.QuadPart));

  lnOffset.QuadPart = (LONG64)cbPHdrOffset;

  /*
   We can't support executable files larger than 8 Exabytes - it's a limitation
   of the I/O system (only 63-bit offsets are supported). Quote:

    [...] the total amount of printed material in the world is estimated to be
    around a fifth of an exabyte. [...] [Source: Wikipedia]
  */
  if(lnOffset.u.HighPart < 0)
   DIE(("The program header is too far into the file\n"));

  nReadStatus = ReadFileCb
  (
   File,
   &lnOffset,
   cbPHdrSize,
   &pData,
   &pBuffer,
   &cbReadSize
  );

  if(!NT_SUCCESS(nReadStatus))
  {
   nStatus = nReadStatus;
   DIE(("ReadFile failed, status %08X\n", nStatus));
  }

  ASSERT(pData);
  ASSERT(pBuffer);
  ASSERT(Intsafe_CanOffsetPointer(pData, cbReadSize));

  if(cbReadSize < cbPHdrSize)
   DIE(("The file didn't contain the program headers\n"));

  /* Force the buffer to be aligned */
  if((ULONG_PTR)pData % TYPE_ALIGNMENT(Elf_Phdr))
  {
   ASSERT(((ULONG_PTR)pBuffer % TYPE_ALIGNMENT(Elf_Phdr)) == 0);
   RtlMoveMemory(pBuffer, pData, cbPHdrSize);
   pphPHdrs = pBuffer;
  }
  else
   pphPHdrs = pData;
 }
 else
 {
  ASSERT(Intsafe_CanAddSizeT(cbPHdrOffset, 0));
  ASSERT(Intsafe_CanOffsetPointer(FileHeader, cbPHdrOffset));
  pphPHdrs = (PVOID)((ULONG_PTR)FileHeader + (ULONG_PTR)cbPHdrOffset);
 }

 /* Allocate the segments */
 pssSegments = AllocateSegmentsCb(nPHdrCount);

 if(pssSegments == NULL)
 {
  nStatus = STATUS_INSUFFICIENT_RESOURCES;
  DIE(("Out of memory\n"));
 }

 ImageSectionObject->Segments = pssSegments;

 fPageAligned = TRUE;

 /* Fill in the segments */
 for(i = 0, j = 0; i < nPHdrCount; ++ i)
 {
  switch(ElfFmtpSafeReadULong(&pphPHdrs[i].p_type, nData))
  {
   case PT_LOAD:
   {
    static const ULONG ProgramHeaderFlagsToProtect[8] =
    {
     PAGE_NOACCESS,          /* 0 */
     PAGE_EXECUTE_READ,      /* PF_X */
     PAGE_READWRITE,         /* PF_W */
     PAGE_EXECUTE_READWRITE, /* PF_X | PF_W */
     PAGE_READONLY,          /* PF_R */
     PAGE_EXECUTE_READ,      /* PF_X | PF_R */
     PAGE_READWRITE,         /* PF_W | PF_R */
     PAGE_EXECUTE_READWRITE  /* PF_X | PF_W | PF_R */
    };

    Elf_Size nAlignment;
    Elf_Off nFileOffset;
    Elf_Addr nVirtualAddr;
    Elf_Size nAdj;
    Elf_Size nVirtualSize = 0;
    Elf_Size nFileSize = 0;

    ASSERT(j <= nPHdrCount);

    /* Retrieve and validate the segment alignment */
    nAlignment = ElfFmtpSafeReadSize(&pphPHdrs[i].p_align, nData);

    if(nAlignment == 0)
     nAlignment = 1;
    else if(!ElfFmtpIsPowerOf2(nAlignment))
     DIE(("Alignment of loadable segment isn't a power of 2\n"));

    if(nAlignment < PAGE_SIZE)
     fPageAligned = FALSE;

    /* Retrieve the addresses and calculate the adjustment */
    nFileOffset = ElfFmtpSafeReadOff(&pphPHdrs[i].p_offset, nData);
    nVirtualAddr = ElfFmtpSafeReadAddr(&pphPHdrs[i].p_vaddr, nData);

    nAdj = ElfFmtpModPow2(nFileOffset, nAlignment);

    if(nAdj != ElfFmtpModPow2(nVirtualAddr, nAlignment))
     DIE(("File and memory address of loadable segment not congruent modulo alignment\n"));

    /* Retrieve, adjust and align the file size and memory size */
    if(!ElfFmtpAddSize(&nFileSize, ElfFmtpSafeReadSize(&pphPHdrs[i].p_filesz, nData), nAdj))
     DIE(("Can't adjust the file size of loadable segment\n"));

    if(!ElfFmtpAddSize(&nVirtualSize, ElfFmtpSafeReadSize(&pphPHdrs[i].p_memsz, nData), nAdj))
     DIE(("Can't adjust the memory size of lodable segment\n"));

    if(!ElfFmtpAlignUp(&nVirtualSize, nVirtualSize, nAlignment))
     DIE(("Can't align the memory size of lodable segment\n"));

    if(nFileSize > nVirtualSize)
     nFileSize = nVirtualSize;

    if(nVirtualSize > MAXULONG)
     DIE(("Virtual image larger than 4GB\n"));

    ASSERT(nFileSize <= MAXULONG);

    pssSegments[j].Length = (ULONG)(nVirtualSize & 0xFFFFFFFF);
    pssSegments[j].RawLength = (ULONG)(nFileSize & 0xFFFFFFFF);

    /* File offset */
    nFileOffset = ElfFmtpAlignDown(nFileOffset, nAlignment);

#if __ELF_WORD_SIZE >= 64
    ASSERT(sizeof(nFileOffset) == sizeof(LONG64));

    if(((LONG64)nFileOffset) < 0)
     DIE(("File offset of loadable segment is too large\n"));
#endif

    pssSegments[j].FileOffset = (LONG64)nFileOffset;

    /* Virtual address */
    nVirtualAddr = ElfFmtpAlignDown(nVirtualAddr, nAlignment);

    if(j == 0)
    {
     /* First segment: its address is the base address of the image */
     nImageBase = nVirtualAddr;
     pssSegments[j].VirtualAddress = 0;

     /* Several places make this assumption */
     if(pssSegments[j].FileOffset != 0)
      DIE(("First loadable segment doesn't contain the ELF header\n"));
    }
    else
    {
     Elf_Size nVirtualOffset;

     /* Other segment: store the offset from the base address */
     if(nVirtualAddr <= nImageBase)
      DIE(("Loadable segments are not sorted\n"));

     nVirtualOffset = nVirtualAddr - nImageBase;

     if(nVirtualOffset > MAXULONG)
      DIE(("Virtual image larger than 4GB\n"));

     pssSegments[j].VirtualAddress = (ULONG)(nVirtualOffset & 0xFFFFFFFF);

     if(pssSegments[j].VirtualAddress != nPrevVirtualEndOfSegment)
      DIE(("Loadable segments are not sorted and contiguous\n"));
    }

    /* Memory protection */
    pssSegments[j].Protection = ProgramHeaderFlagsToProtect
    [
     ElfFmtpSafeReadULong(&pphPHdrs[i].p_flags, nData) & (PF_R | PF_W | PF_X)
    ];

    /* Characteristics */
    /*
     TODO: need to add support for the shared, non-pageable, non-cacheable and
     discardable attributes. This involves extensions to the ELF format, so it's
     nothing to be taken lightly
    */
    if(pssSegments[j].Protection & PAGE_IS_EXECUTABLE)
    {
     ImageSectionObject->Executable = TRUE;
     pssSegments[j].Characteristics = IMAGE_SCN_CNT_CODE;
    }
    else if(pssSegments[j].RawLength == 0)
     pssSegments[j].Characteristics = IMAGE_SCN_CNT_UNINITIALIZED_DATA;
    else
     pssSegments[j].Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA;

    /*
     FIXME: see the TODO above. This is the safest way to load ELF drivers, for
     now, if a bit wasteful of memory
    */
    pssSegments[j].Characteristics |= IMAGE_SCN_MEM_NOT_PAGED;

    /* Copy-on-write */
    pssSegments[j].WriteCopy = TRUE;

    if(!Intsafe_AddULong32(&nPrevVirtualEndOfSegment, pssSegments[j].VirtualAddress, pssSegments[j].Length))
     DIE(("Virtual image larger than 4GB\n"));

    ++ j;
    break;
   }
  }
 }

 if(j == 0)
  DIE(("No loadable segments\n"));

 ImageSectionObject->NrSegments = j;

 *Flags =
  EXEFMT_LOAD_ASSUME_SEGMENTS_SORTED |
  EXEFMT_LOAD_ASSUME_SEGMENTS_NO_OVERLAP;

 if(fPageAligned)
  *Flags |= EXEFMT_LOAD_ASSUME_SEGMENTS_PAGE_ALIGNED;

 nEntryPoint = ElfFmtpReadAddr(pehHeader->e_entry, nData);

 if(nEntryPoint < nImageBase || nEntryPoint - nImageBase > nPrevVirtualEndOfSegment)
  DIE(("Entry point not within the virtual image\n"));

 ASSERT(nEntryPoint >= nImageBase);
 ASSERT((nEntryPoint - nImageBase) <= MAXULONG);
 ImageSectionObject->EntryPoint = nEntryPoint - nImageBase;

 /* TODO: support Wine executables and read these values from nt_headers */
 ImageSectionObject->ImageCharacteristics |=
  IMAGE_FILE_EXECUTABLE_IMAGE |
  IMAGE_FILE_LINE_NUMS_STRIPPED |
  IMAGE_FILE_LOCAL_SYMS_STRIPPED |
  (nImageBase > MAXULONG ? IMAGE_FILE_LARGE_ADDRESS_AWARE : 0) |
  IMAGE_FILE_DEBUG_STRIPPED;

 if(nData == ELFDATA2LSB)
  ImageSectionObject->ImageCharacteristics |= IMAGE_FILE_BYTES_REVERSED_LO;
 else if(nData == ELFDATA2MSB)
  ImageSectionObject->ImageCharacteristics |= IMAGE_FILE_BYTES_REVERSED_HI;

 /* Base address outside the possible address space */
 if(nImageBase > MAXULONG_PTR)
  ImageSectionObject->ImageBase = EXEFMT_LOAD_BASE_NONE;
 /* Position-independent image, base address doesn't matter */
 else if(nImageBase == 0)
  ImageSectionObject->ImageBase = EXEFMT_LOAD_BASE_ANY;
 /* Use the specified base address */
 else
  ImageSectionObject->ImageBase = (ULONG_PTR)nImageBase;

 /* safest bet */
 ImageSectionObject->Subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
 ImageSectionObject->MinorSubsystemVersion = 0;
 ImageSectionObject->MajorSubsystemVersion = 4;

 /* Success, at last */
 nStatus = STATUS_SUCCESS;

l_Return:
 if(pBuffer)
  ExFreePool(pBuffer);

 return nStatus;
}

/* EOF */
