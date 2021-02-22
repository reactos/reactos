#pragma once

#define IMAGE_NT_OPTIONAL_HDR32_MAGIC 0x010b
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC 0x020b

#define IMAGE_DOS_MAGIC 0x5a4d
#define IMAGE_PE_MAGIC 0x00004550
#define IMAGE_SIZEOF_SHORT_NAME 8

#define IMAGE_FILE_LINE_NUMS_STRIPPED   0x0004
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED  0x0008
#define IMAGE_FILE_DEBUG_STRIPPED   0x0200

#define IMAGE_FILE_MACHINE_I386 0x14c
#define IMAGE_FILE_MACHINE_AMD64 0x8664
#define IMAGE_FILE_MACHINE_IA64 0x0200

#define IMAGE_DIRECTORY_ENTRY_BASERELOC	5

#define IMAGE_SCN_TYPE_NOLOAD     0x00000002
#define IMAGE_SCN_TYPE_NO_PAD     0x00000008
#define IMAGE_SCN_CNT_CODE        0x00000020
#define IMAGE_SCN_CNT_INITIALIZED_DATA    0x00000040
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA  0x00000080
#define IMAGE_SCN_LNK_OTHER       0x00000100
#define IMAGE_SCN_LNK_INFO        0x00000200
#define IMAGE_SCN_LNK_REMOVE      0x00000800
#define IMAGE_SCN_NO_DEFER_SPEC_EXC 0x00004000
#define IMAGE_SCN_GPREL           0x00008000
#define IMAGE_SCN_MEM_PURGEABLE   0x00020000
#define IMAGE_SCN_MEM_LOCKED      0x00040000
#define IMAGE_SCN_MEM_PRELOAD     0x00080000
#define IMAGE_SCN_LNK_NRELOC_OVFL 0x01000000
#define IMAGE_SCN_MEM_DISCARDABLE 0x02000000
#define IMAGE_SCN_MEM_NOT_CACHED  0x04000000
#define IMAGE_SCN_MEM_NOT_PAGED   0x08000000
#define IMAGE_SCN_MEM_SHARED      0x10000000
#define IMAGE_SCN_MEM_EXECUTE     0x20000000
#define IMAGE_SCN_MEM_READ        0x40000000
#define IMAGE_SCN_MEM_WRITE       0x80000000

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

#define IMAGE_REL_I386_ABSOLUTE 0x0001
#define IMAGE_REL_I386_REL16    0x0002
#define IMAGE_REL_I386_DIR32    0x0006

#pragma pack(push,2)
typedef struct _IMAGE_DOS_HEADER {
  WORD e_magic;
  WORD e_cblp;
  WORD e_cp;
  WORD e_crlc;
  WORD e_cparhdr;
  WORD e_minalloc;
  WORD e_maxalloc;
  WORD e_ss;
  WORD e_sp;
  WORD e_csum;
  WORD e_ip;
  WORD e_cs;
  WORD e_lfarlc;
  WORD e_ovno;
  WORD e_res[4];
  WORD e_oemid;
  WORD e_oeminfo;
  WORD e_res2[10];
  LONG e_lfanew;
} IMAGE_DOS_HEADER,*PIMAGE_DOS_HEADER;
#pragma pack(pop)

#pragma pack(push,4)
typedef struct _IMAGE_FILE_HEADER {
	WORD Machine;
	WORD NumberOfSections;
	DWORD TimeDateStamp;
	DWORD PointerToSymbolTable;
	DWORD NumberOfSymbols;
	WORD SizeOfOptionalHeader;
	WORD Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;
#pragma pack(pop)

typedef struct _IMAGE_DATA_DIRECTORY {
  DWORD VirtualAddress;
  DWORD Size;
} IMAGE_DATA_DIRECTORY,*PIMAGE_DATA_DIRECTORY;

typedef struct _IMAGE_OPTIONAL_HEADER32 {
	WORD Magic;
	BYTE MajorLinkerVersion;
	BYTE MinorLinkerVersion;
	DWORD SizeOfCode;
	DWORD SizeOfInitializedData;
	DWORD SizeOfUninitializedData;
	DWORD AddressOfEntryPoint;
	DWORD BaseOfCode;
	DWORD BaseOfData;
	DWORD ImageBase;
	DWORD SectionAlignment;
	DWORD FileAlignment;
	WORD MajorOperatingSystemVersion;
	WORD MinorOperatingSystemVersion;
	WORD MajorImageVersion;
	WORD MinorImageVersion;
	WORD MajorSubsystemVersion;
	WORD MinorSubsystemVersion;
	DWORD Win32VersionValue;
	DWORD SizeOfImage;
	DWORD SizeOfHeaders;
	DWORD CheckSum;
	WORD Subsystem;
	WORD DllCharacteristics;
	DWORD SizeOfStackReserve;
	DWORD SizeOfStackCommit;
	DWORD SizeOfHeapReserve;
	DWORD SizeOfHeapCommit;
	DWORD LoaderFlags;
	DWORD NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32,*PIMAGE_OPTIONAL_HEADER32;

typedef struct _IMAGE_OPTIONAL_HEADER64 {
	WORD Magic;
	BYTE MajorLinkerVersion;
	BYTE MinorLinkerVersion;
	DWORD SizeOfCode;
	DWORD SizeOfInitializedData;
	DWORD SizeOfUninitializedData;
	DWORD AddressOfEntryPoint;
	DWORD BaseOfCode;
	ULONGLONG ImageBase;
	DWORD SectionAlignment;
	DWORD FileAlignment;
	WORD MajorOperatingSystemVersion;
	WORD MinorOperatingSystemVersion;
	WORD MajorImageVersion;
	WORD MinorImageVersion;
	WORD MajorSubsystemVersion;
	WORD MinorSubsystemVersion;
	DWORD Win32VersionValue;
	DWORD SizeOfImage;
	DWORD SizeOfHeaders;
	DWORD CheckSum;
	WORD Subsystem;
	WORD DllCharacteristics;
	ULONGLONG SizeOfStackReserve;
	ULONGLONG SizeOfStackCommit;
	ULONGLONG SizeOfHeapReserve;
	ULONGLONG SizeOfHeapCommit;
	DWORD LoaderFlags;
	DWORD NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER64,*PIMAGE_OPTIONAL_HEADER64;

#ifdef _TARGET_PE64
typedef IMAGE_OPTIONAL_HEADER64 IMAGE_OPTIONAL_HEADER;
typedef PIMAGE_OPTIONAL_HEADER64 PIMAGE_OPTIONAL_HEADER;
#else
typedef IMAGE_OPTIONAL_HEADER32 IMAGE_OPTIONAL_HEADER;
typedef PIMAGE_OPTIONAL_HEADER32 PIMAGE_OPTIONAL_HEADER;
#endif

typedef struct _IMAGE_NT_HEADERS32
{
  DWORD Signature;
  IMAGE_FILE_HEADER FileHeader;
  IMAGE_OPTIONAL_HEADER32 OptionalHeader;
} IMAGE_NT_HEADERS32, *PIMAGE_NT_HEADERS32;

typedef struct _IMAGE_NT_HEADERS64
{
  DWORD Signature;
  IMAGE_FILE_HEADER FileHeader;
  IMAGE_OPTIONAL_HEADER32 OptionalHeader;
} IMAGE_NT_HEADERS64, *PIMAGE_NT_HEADERS64;

#ifdef _TARGET_PE64
typedef IMAGE_NT_HEADERS64 IMAGE_NT_HEADERS;
typedef PIMAGE_NT_HEADERS64 PIMAGE_NT_HEADERS;
#else
typedef IMAGE_NT_HEADERS32 IMAGE_NT_HEADERS;
typedef PIMAGE_NT_HEADERS32 PIMAGE_NT_HEADERS;
#endif

typedef struct _IMAGE_SECTION_HEADER {
  BYTE Name[IMAGE_SIZEOF_SHORT_NAME];
  union {
    DWORD PhysicalAddress;
    DWORD VirtualSize;
  } Misc;
  DWORD VirtualAddress;
  DWORD SizeOfRawData;
  DWORD PointerToRawData;
  DWORD PointerToRelocations;
  DWORD PointerToLinenumbers;
  WORD NumberOfRelocations;
  WORD NumberOfLinenumbers;
  DWORD Characteristics;
} IMAGE_SECTION_HEADER,*PIMAGE_SECTION_HEADER;

#pragma pack(push,4)
typedef struct _IMAGE_BASE_RELOCATION {
	DWORD VirtualAddress;
	DWORD SizeOfBlock;
	// Followed by: WORD TypeOffset[ANYSIZE_ARRAY];
} IMAGE_BASE_RELOCATION,*PIMAGE_BASE_RELOCATION;
#pragma pack(pop)

#ifndef UNALIGNED
#define UNALIGNED
#endif

#pragma pack(push,2)
typedef struct _IMAGE_RELOCATION {
  union {
    DWORD VirtualAddress;
    DWORD RelocCount;
  };
  DWORD SymbolTableIndex;
  WORD Type;
} IMAGE_RELOCATION;
typedef struct _IMAGE_RELOCATION UNALIGNED *PIMAGE_RELOCATION;
#pragma pack(pop)

#pragma pack(push,2)
typedef struct _IMAGE_SYMBOL {
  union {
    BYTE ShortName[8];
    struct {
      DWORD Short;
      DWORD Long;
    } Name;
    DWORD LongName[2];
  } N;
  DWORD Value;
  SHORT SectionNumber;
  WORD Type;
  BYTE StorageClass;
  BYTE NumberOfAuxSymbols;
} IMAGE_SYMBOL;
typedef struct _IMAGE_SYMBOL UNALIGNED *PIMAGE_SYMBOL;
#pragma pack(pop)

#define IMAGE_DOS_SIGNATURE 0x5A4D
#define IMAGE_OS2_SIGNATURE 0x454E
#define IMAGE_OS2_SIGNATURE_LE 0x454C
#define IMAGE_VXD_SIGNATURE 0x454C
#define IMAGE_NT_SIGNATURE 0x00004550

#define IMAGE_FIRST_SECTION(h) ((PIMAGE_SECTION_HEADER) ((ULONG_PTR)h+FIELD_OFFSET(IMAGE_NT_HEADERS,OptionalHeader)+((PIMAGE_NT_HEADERS)(h))->FileHeader.SizeOfOptionalHeader))
