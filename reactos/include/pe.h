#ifndef __INCLUDE_PE_H
#define __INCLUDE_PE_H

#define _ANONYMOUS_UNION __extension__
#define _ANONYMOUS_STRUCT __extension__

#ifndef NTAPI
#define NTAPI STDCALL
#endif

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES    16
#define IMAGE_SIZEOF_SHORT_NAME 8
#define IMAGE_SIZEOF_SYMBOL 18

#ifndef __USE_W32API

#define IMAGE_DOS_SIGNATURE     0x5a4d
#define IMAGE_OS2_SIGNATURE     0x454e
#define IMAGE_OS2_SIGNATURE_LE  0x454c
#define IMAGE_VXD_SIGNATURE     0x454c

#define IMAGE_FILE_RELOCS_STRIPPED           0x0001  // Relocation info stripped from file.
#define IMAGE_FILE_EXECUTABLE_IMAGE          0x0002  // File is executable  (i.e. no unresolved externel references).
#define IMAGE_FILE_LINE_NUMS_STRIPPED        0x0004  // Line nunbers stripped from file.
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED       0x0008  // Local symbols stripped from file.
#define IMAGE_FILE_BYTES_REVERSED_LO         0x0080  // Bytes of machine word are reversed.
#define IMAGE_FILE_32BIT_MACHINE             0x0100  // 32 bit word machine.
#define IMAGE_FILE_DEBUG_STRIPPED            0x0200  // Debugging info stripped from file in .DBG file
#define IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP   0x0400  // If Image is on removable media, copy and run from the swap file.
#define IMAGE_FILE_NET_RUN_FROM_SWAP         0x0800  // If Image is on Net, copy and run from the swap file.
#define IMAGE_FILE_SYSTEM                    0x1000  // System File.
#define IMAGE_FILE_DLL                       0x2000  // File is a DLL.
#define IMAGE_FILE_UP_SYSTEM_ONLY            0x4000  // File should only be run on a UP machine
#define IMAGE_FILE_BYTES_REVERSED_HI         0x8000  // Bytes of machine word are reversed.

#define IMAGE_FILE_MACHINE_UNKNOWN           0
#define IMAGE_FILE_MACHINE_I386              0x14c   // Intel 386.
#define IMAGE_FILE_MACHINE_R3000             0x162   // MIPS little-endian, 0x160 big-endian
#define IMAGE_FILE_MACHINE_R4000             0x166   // MIPS little-endian
#define IMAGE_FILE_MACHINE_R10000            0x168   // MIPS little-endian
#define IMAGE_FILE_MACHINE_ALPHA             0x184   // Alpha_AXP
#define IMAGE_FILE_MACHINE_POWERPC           0x1F0   // IBM PowerPC Little-Endian

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
typedef struct _IMAGE_DATA_DIRECTORY {
	DWORD VirtualAddress;
	DWORD Size;
} IMAGE_DATA_DIRECTORY,*PIMAGE_DATA_DIRECTORY;
typedef struct _IMAGE_OPTIONAL_HEADER {
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
	DWORD Reserved1;
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
} IMAGE_OPTIONAL_HEADER,*PIMAGE_OPTIONAL_HEADER;
typedef struct _IMAGE_ROM_OPTIONAL_HEADER {
	WORD Magic;
	BYTE MajorLinkerVersion;
	BYTE MinorLinkerVersion;
	DWORD SizeOfCode;
	DWORD SizeOfInitializedData;
	DWORD SizeOfUninitializedData;
	DWORD AddressOfEntryPoint;
	DWORD BaseOfCode;
	DWORD BaseOfData;
	DWORD BaseOfBss;
	DWORD GprMask;
	DWORD CprMask[4];
	DWORD GpValue;
} IMAGE_ROM_OPTIONAL_HEADER,*PIMAGE_ROM_OPTIONAL_HEADER;
#pragma pack(pop)
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
typedef struct _IMAGE_OS2_HEADER {
	WORD ne_magic;
	CHAR ne_ver;
	CHAR ne_rev;
	WORD ne_enttab;
	WORD ne_cbenttab;
	LONG ne_crc;
	WORD ne_flags;
	WORD ne_autodata;
	WORD ne_heap;
	WORD ne_stack;
	LONG ne_csip;
	LONG ne_sssp;
	WORD ne_cseg;
	WORD ne_cmod;
	WORD ne_cbnrestab;
	WORD ne_segtab;
	WORD ne_rsrctab;
	WORD ne_restab;
	WORD ne_modtab;
	WORD ne_imptab;
	LONG ne_nrestab;
	WORD ne_cmovent;
	WORD ne_align;
	WORD ne_cres;
	BYTE ne_exetyp;
	BYTE ne_flagsothers;
	WORD ne_pretthunks;
	WORD ne_psegrefbytes;
	WORD ne_swaparea;
	WORD ne_expver;
} IMAGE_OS2_HEADER,*PIMAGE_OS2_HEADER;
#pragma pack(pop)
#pragma pack(push,4)
typedef struct _IMAGE_NT_HEADERS {
	DWORD Signature;
	IMAGE_FILE_HEADER FileHeader;
	IMAGE_OPTIONAL_HEADER OptionalHeader;
} IMAGE_NT_HEADERS,*PIMAGE_NT_HEADERS;
typedef struct _IMAGE_ROM_HEADERS {
	IMAGE_FILE_HEADER FileHeader;
	IMAGE_ROM_OPTIONAL_HEADER OptionalHeader;
} IMAGE_ROM_HEADERS,*PIMAGE_ROM_HEADERS;
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
#pragma pack(pop)
#pragma pack(push,2)
typedef struct _IMAGE_SYMBOL {
	union {
		BYTE ShortName[8];
		struct {
			DWORD Short;
			DWORD Long;
		} Name;
		PBYTE LongName[2];
	} N;
	DWORD Value;
	SHORT SectionNumber;
	WORD Type;
	BYTE StorageClass;
	BYTE NumberOfAuxSymbols;
} IMAGE_SYMBOL,*PIMAGE_SYMBOL;
typedef union _IMAGE_AUX_SYMBOL {
	struct {
		DWORD TagIndex;
		union {
			struct {
				WORD Linenumber;
				WORD Size;
			} LnSz;
			DWORD TotalSize;
		} Misc;
		union {
			struct {
				DWORD PointerToLinenumber;
				DWORD PointerToNextFunction;
			} Function;
			struct {
				WORD Dimension[4];
			} Array;
		} FcnAry;
		WORD TvIndex;
	} Sym;
	struct {
		BYTE Name[IMAGE_SIZEOF_SYMBOL];
	} File;
	struct {
		DWORD Length;
		WORD NumberOfRelocations;
		WORD NumberOfLinenumbers;
		DWORD CheckSum;
		SHORT Number;
		BYTE Selection;
	} Section;
} IMAGE_AUX_SYMBOL,*PIMAGE_AUX_SYMBOL;
typedef struct _IMAGE_COFF_SYMBOLS_HEADER {
	DWORD NumberOfSymbols;
	DWORD LvaToFirstSymbol;
	DWORD NumberOfLinenumbers;
	DWORD LvaToFirstLinenumber;
	DWORD RvaToFirstByteOfCode;
	DWORD RvaToLastByteOfCode;
	DWORD RvaToFirstByteOfData;
	DWORD RvaToLastByteOfData;
} IMAGE_COFF_SYMBOLS_HEADER,*PIMAGE_COFF_SYMBOLS_HEADER;
typedef struct _IMAGE_RELOCATION {
	_ANONYMOUS_UNION union {
		DWORD VirtualAddress;
		DWORD RelocCount;
	} DUMMYUNIONNAME;
	DWORD SymbolTableIndex;
	WORD Type;
} IMAGE_RELOCATION,*PIMAGE_RELOCATION;
#pragma pack(pop)
#pragma pack(push,4)
typedef struct _IMAGE_BASE_RELOCATION {
	DWORD VirtualAddress;
	DWORD SizeOfBlock;
} IMAGE_BASE_RELOCATION,*PIMAGE_BASE_RELOCATION;
#pragma pack(pop)
#pragma pack(push,2)
typedef struct _IMAGE_LINENUMBER {
	union {
		DWORD SymbolTableIndex;
		DWORD VirtualAddress;
	} Type;
	WORD Linenumber;
} IMAGE_LINENUMBER,*PIMAGE_LINENUMBER;
#pragma pack(pop)
#pragma pack(push,4)
typedef struct _IMAGE_ARCHIVE_MEMBER_HEADER {
	BYTE Name[16];
	BYTE Date[12];
	BYTE UserID[6];
	BYTE GroupID[6];
	BYTE Mode[8];
	BYTE Size[10];
	BYTE EndHeader[2];
} IMAGE_ARCHIVE_MEMBER_HEADER,*PIMAGE_ARCHIVE_MEMBER_HEADER;
typedef struct _IMAGE_EXPORT_DIRECTORY {
	DWORD Characteristics;
	DWORD TimeDateStamp;
	WORD MajorVersion;
	WORD MinorVersion;
	DWORD Name;
	DWORD Base;
	DWORD NumberOfFunctions;
	DWORD NumberOfNames;
	PDWORD *AddressOfFunctions;
	PDWORD *AddressOfNames;
	PWORD *AddressOfNameOrdinals;
} IMAGE_EXPORT_DIRECTORY,*PIMAGE_EXPORT_DIRECTORY;
typedef struct _IMAGE_IMPORT_BY_NAME {
	WORD Hint;
	BYTE Name[1];
} IMAGE_IMPORT_BY_NAME,*PIMAGE_IMPORT_BY_NAME;
typedef struct _IMAGE_THUNK_DATA {
	union {
		PBYTE ForwarderString;
		PDWORD Function;
		DWORD Ordinal;
		PIMAGE_IMPORT_BY_NAME AddressOfData;
	} u1;
} IMAGE_THUNK_DATA,*PIMAGE_THUNK_DATA;
typedef struct _IMAGE_IMPORT_DESCRIPTOR {
	_ANONYMOUS_UNION union {
		DWORD Characteristics;
		PIMAGE_THUNK_DATA OriginalFirstThunk;
	} DUMMYUNIONNAME;
	DWORD TimeDateStamp;
	DWORD ForwarderChain;
	DWORD Name;
	PIMAGE_THUNK_DATA FirstThunk;
} IMAGE_IMPORT_DESCRIPTOR,*PIMAGE_IMPORT_DESCRIPTOR;
typedef struct _IMAGE_BOUND_IMPORT_DESCRIPTOR {
	DWORD TimeDateStamp;
	WORD OffsetModuleName;
	WORD NumberOfModuleForwarderRefs;
} IMAGE_BOUND_IMPORT_DESCRIPTOR,*PIMAGE_BOUND_IMPORT_DESCRIPTOR;
typedef struct _IMAGE_BOUND_FORWARDER_REF {
	DWORD TimeDateStamp;
	WORD OffsetModuleName;
	WORD Reserved;
} IMAGE_BOUND_FORWARDER_REF,*PIMAGE_BOUND_FORWARDER_REF;
typedef void(NTAPI *PIMAGE_TLS_CALLBACK)(PVOID,DWORD,PVOID);
typedef struct _IMAGE_TLS_DIRECTORY {
	DWORD StartAddressOfRawData;
	DWORD EndAddressOfRawData;
	PDWORD AddressOfIndex;
	PIMAGE_TLS_CALLBACK *AddressOfCallBacks;
	DWORD SizeOfZeroFill;
	DWORD Characteristics;
} IMAGE_TLS_DIRECTORY,*PIMAGE_TLS_DIRECTORY;
typedef struct _IMAGE_RESOURCE_DIRECTORY {
	DWORD Characteristics;
	DWORD TimeDateStamp;
	WORD MajorVersion;
	WORD MinorVersion;
	WORD NumberOfNamedEntries;
	WORD NumberOfIdEntries;
} IMAGE_RESOURCE_DIRECTORY,*PIMAGE_RESOURCE_DIRECTORY;
/*_ANONYMOUS_STRUCT typedef struct _IMAGE_RESOURCE_DIRECTORY_ENTRY {
	_ANONYMOUS_UNION union {
		_ANONYMOUS_STRUCT struct {
			DWORD NameOffset:31;
			DWORD NameIsString:1;
		}DUMMYSTRUCTNAME;
		DWORD Name;
		WORD Id;
	} DUMMYUNIONNAME;
	_ANONYMOUS_UNION union {
		DWORD OffsetToData;
		_ANONYMOUS_STRUCT struct {
			DWORD OffsetToDirectory:31;
			DWORD DataIsDirectory:1;
		} DUMMYSTRUCTNAME2;
	} DUMMYUNIONNAME2;
} IMAGE_RESOURCE_DIRECTORY_ENTRY,*PIMAGE_RESOURCE_DIRECTORY_ENTRY;
*/
typedef struct _IMAGE_RESOURCE_DIRECTORY_STRING {
	WORD Length;
	CHAR NameString[1];
} IMAGE_RESOURCE_DIRECTORY_STRING,*PIMAGE_RESOURCE_DIRECTORY_STRING;
typedef struct _IMAGE_RESOURCE_DIR_STRING_U {
	WORD Length;
	WCHAR NameString[1];
} IMAGE_RESOURCE_DIR_STRING_U,*PIMAGE_RESOURCE_DIR_STRING_U;
typedef struct _IMAGE_RESOURCE_DATA_ENTRY {
	DWORD OffsetToData;
	DWORD Size;
	DWORD CodePage;
	DWORD Reserved;
} IMAGE_RESOURCE_DATA_ENTRY,*PIMAGE_RESOURCE_DATA_ENTRY;
typedef struct _IMAGE_LOAD_CONFIG_DIRECTORY {
	DWORD Characteristics;
	DWORD TimeDateStamp;
	WORD MajorVersion;
	WORD MinorVersion;
	DWORD GlobalFlagsClear;
	DWORD GlobalFlagsSet;
	DWORD CriticalSectionDefaultTimeout;
	DWORD DeCommitFreeBlockThreshold;
	DWORD DeCommitTotalFreeThreshold;
	PVOID LockPrefixTable;
	DWORD MaximumAllocationSize;
	DWORD VirtualMemoryThreshold;
	DWORD ProcessHeapFlags;
	DWORD Reserved[4];
} IMAGE_LOAD_CONFIG_DIRECTORY,*PIMAGE_LOAD_CONFIG_DIRECTORY;
typedef struct _IMAGE_RUNTIME_FUNCTION_ENTRY {
	DWORD BeginAddress;
	DWORD EndAddress;
	PVOID ExceptionHandler;
	PVOID HandlerData;
	DWORD PrologEndAddress;
} IMAGE_RUNTIME_FUNCTION_ENTRY,*PIMAGE_RUNTIME_FUNCTION_ENTRY;
typedef struct _IMAGE_DEBUG_DIRECTORY {
	DWORD Characteristics;
	DWORD TimeDateStamp;
	WORD MajorVersion;
	WORD MinorVersion;
	DWORD Type;
	DWORD SizeOfData;
	DWORD AddressOfRawData;
	DWORD PointerToRawData;
} IMAGE_DEBUG_DIRECTORY,*PIMAGE_DEBUG_DIRECTORY;
typedef struct _FPO_DATA {
	DWORD ulOffStart;
	DWORD cbProcSize;
	DWORD cdwLocals;
	WORD cdwParams;
	WORD cbProlog:8;
	WORD cbRegs:3;
	WORD fHasSEH:1;
	WORD fUseBP:1;
	WORD reserved:1;
	WORD cbFrame:2;
} FPO_DATA,*PFPO_DATA;
typedef struct _IMAGE_DEBUG_MISC {
	DWORD DataType;
	DWORD Length;
	BOOLEAN Unicode;
	BYTE Reserved[3];
	BYTE Data[1];
} IMAGE_DEBUG_MISC,*PIMAGE_DEBUG_MISC;
typedef struct _IMAGE_FUNCTION_ENTRY {
	DWORD StartingAddress;
	DWORD EndingAddress;
	DWORD EndOfPrologue;
} IMAGE_FUNCTION_ENTRY,*PIMAGE_FUNCTION_ENTRY;
typedef struct _IMAGE_SEPARATE_DEBUG_HEADER {
	WORD Signature;
	WORD Flags;
	WORD Machine;
	WORD Characteristics;
	DWORD TimeDateStamp;
	DWORD CheckSum;
	DWORD ImageBase;
	DWORD SizeOfImage;
	DWORD NumberOfSections;
	DWORD ExportedNamesSize;
	DWORD DebugDirectorySize;
	DWORD Reserved[3];
} IMAGE_SEPARATE_DEBUG_HEADER,*PIMAGE_SEPARATE_DEBUG_HEADER;
#pragma pack(pop)

#define IMAGE_ORDINAL(Ordinal) (Ordinal & 0xffff)

//
// Each directory contains the 32-bit Name of the entry and an offset,
// relative to the beginning of the resource directory of the data associated
// with this directory entry.  If the name of the entry is an actual text
// string instead of an integer Id, then the high order bit of the name field
// is set to one and the low order 31-bits are an offset, relative to the
// beginning of the resource directory of the string, which is of type
// IMAGE_RESOURCE_DIRECTORY_STRING.  Otherwise the high bit is clear and the
// low-order 16-bits are the integer Id that identify this resource directory
// entry. If the directory entry is yet another resource directory (i.e. a
// subdirectory), then the high order bit of the offset field will be
// set to indicate this.  Otherwise the high bit is clear and the offset
// field points to a resource data entry.
//
typedef struct _IMAGE_RESOURCE_DIRECTORY_ENTRY {
    DWORD    Name;
    DWORD    OffsetToData;
} IMAGE_RESOURCE_DIRECTORY_ENTRY, *PIMAGE_RESOURCE_DIRECTORY_ENTRY;
/*
typedef struct _IMAGE_RESOURCE_DIRECTORY {
    DWORD   Characteristics;
    DWORD   TimeDateStamp;
    WORD    MajorVersion;
    WORD    MinorVersion;
    WORD    NumberOfNamedEntries;
    WORD    NumberOfIdEntries;
    IMAGE_RESOURCE_DIRECTORY_ENTRY DirectoryEntries[0];
} IMAGE_RESOURCE_DIRECTORY, *PIMAGE_RESOURCE_DIRECTORY;
*/

#endif /* !__USE_W32API */

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))

#define IMAGE_SECTION_CHAR_CODE          0x00000020
#define IMAGE_SECTION_CHAR_DATA          0x00000040
#define IMAGE_SECTION_CHAR_BSS           0x00000080
#define IMAGE_SECTION_CHAR_NON_CACHABLE  0x04000000
#define IMAGE_SECTION_CHAR_NON_PAGEABLE  0x08000000
#define IMAGE_SECTION_CHAR_SHARED        0x10000000
#define IMAGE_SECTION_CHAR_EXECUTABLE    0x20000000
#define IMAGE_SECTION_CHAR_READABLE      0x40000000
#define IMAGE_SECTION_CHAR_WRITABLE      0x80000000
#define IMAGE_SECTION_NOLOAD             0x00000002

#define IMAGE_DOS_MAGIC  0x5a4d
#define IMAGE_PE_MAGIC   0x00004550

#define IMAGE_NT_SIGNATURE      0x00004550


#define IMAGE_SIZEOF_FILE_HEADER             20


#define IMAGE_SUBSYSTEM_UNKNOWN		0
#define IMAGE_SUBSYSTEM_NATIVE		1
#define IMAGE_SUBSYSTEM_WINDOWS_GUI	2
#define IMAGE_SUBSYSTEM_WINDOWS_CUI	3
#define IMAGE_SUBSYSTEM_OS2_GUI		4
#define IMAGE_SUBSYSTEM_OS2_CUI		5
#define IMAGE_SUBSYSTEM_POSIX_GUI	6
#define IMAGE_SUBSYSTEM_POSIX_CUI	7
#define IMAGE_SUBSYSTEM_WINDOWS_CE_GUI	9



// Directory Entries

#define IMAGE_DIRECTORY_ENTRY_EXPORT         0   // Export Directory
#define IMAGE_DIRECTORY_ENTRY_IMPORT         1   // Import Directory
#define IMAGE_DIRECTORY_ENTRY_RESOURCE       2   // Resource Directory
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION      3   // Exception Directory
#define IMAGE_DIRECTORY_ENTRY_SECURITY       4   // Security Directory
#define IMAGE_DIRECTORY_ENTRY_BASERELOC      5   // Base Relocation Table
#define IMAGE_DIRECTORY_ENTRY_DEBUG          6   // Debug Directory
#define IMAGE_DIRECTORY_ENTRY_COPYRIGHT      7   // Description String
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR      8   // Machine Value (MIPS GP)
#define IMAGE_DIRECTORY_ENTRY_TLS            9   // TLS Directory
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG   10   // Load Configuration Directory
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT  11   // Bound Import Directory in headers
#define IMAGE_DIRECTORY_ENTRY_IAT           12   // Import Address
//
// Section header format.
//
#define IMAGE_SIZEOF_FILE_HEADER	20
#define IMAGE_FILE_MACHINE_UNKNOWN	0
#define IMAGE_NT_SIGNATURE 0x00004550
#define IMAGE_NT_OPTIONAL_HDR_MAGIC 0x10b
#define IMAGE_ROM_OPTIONAL_HDR_MAGIC 0x107
#define IMAGE_SEPARATE_DEBUG_SIGNATURE 0x4944
#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16
#define IMAGE_SIZEOF_ROM_OPTIONAL_HEADER 56
#define IMAGE_SIZEOF_STD_OPTIONAL_HEADER 28
#define IMAGE_SIZEOF_NT_OPTIONAL_HEADER 224
#define IMAGE_SIZEOF_SECTION_HEADER 40
#define IMAGE_SIZEOF_AUX_SYMBOL 18
#define IMAGE_SIZEOF_RELOCATION 10
#define IMAGE_SIZEOF_BASE_RELOCATION 8
#define IMAGE_SIZEOF_LINENUMBER 6
#define IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR 60
#define SIZEOF_RFPO_DATA 16
#define IMAGE_FIRST_SECTION(h) ((PIMAGE_SECTION_HEADER) ((DWORD)h+FIELD_OFFSET(IMAGE_NT_HEADERS,OptionalHeader)+((PIMAGE_NT_HEADERS)(h))->FileHeader.SizeOfOptionalHeader))
#define IMAGE_SCN_TYPE_NO_PAD 8
#define IMAGE_SCN_CNT_CODE 32
#define IMAGE_SCN_CNT_INITIALIZED_DATA 64
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA 128
#define IMAGE_SCN_LNK_OTHER 256
#define IMAGE_SCN_LNK_INFO 512
#define IMAGE_SCN_LNK_REMOVE 2048
#define IMAGE_SCN_LNK_COMDAT 4096
#define IMAGE_SCN_MEM_FARDATA 0x8000
#define IMAGE_SCN_MEM_PURGEABLE 0x20000
#define IMAGE_SCN_MEM_16BIT 0x20000
#define IMAGE_SCN_MEM_LOCKED  0x40000
#define IMAGE_SCN_MEM_PRELOAD 0x80000
#define IMAGE_SCN_ALIGN_1BYTES 0x100000
#define IMAGE_SCN_ALIGN_2BYTES 0x200000
#define IMAGE_SCN_ALIGN_4BYTES 0x300000
#define IMAGE_SCN_ALIGN_8BYTES 0x400000
#define IMAGE_SCN_ALIGN_16BYTES 0x500000
#define IMAGE_SCN_ALIGN_32BYTES 0x600000
#define IMAGE_SCN_ALIGN_64BYTES 0x700000
#define IMAGE_SCN_LNK_NRELOC_OVFL 0x1000000
#define IMAGE_SCN_MEM_DISCARDABLE 0x2000000
#define IMAGE_SCN_MEM_NOT_CACHED 0x4000000
#define IMAGE_SCN_MEM_NOT_PAGED 0x8000000
#define IMAGE_SCN_MEM_SHARED 0x10000000
#define IMAGE_SCN_MEM_EXECUTE 0x20000000
#define IMAGE_SCN_MEM_READ 0x40000000
#define IMAGE_SCN_MEM_WRITE 0x80000000
#define IMAGE_SYM_UNDEFINED	0
#define IMAGE_SYM_ABSOLUTE (-1)
#define IMAGE_SYM_DEBUG	(-2)
#define IMAGE_SYM_TYPE_NULL 0
#define IMAGE_SYM_TYPE_VOID 1
#define IMAGE_SYM_TYPE_CHAR 2
#define IMAGE_SYM_TYPE_SHORT 3
#define IMAGE_SYM_TYPE_INT 4
#define IMAGE_SYM_TYPE_LONG 5
#define IMAGE_SYM_TYPE_FLOAT 6
#define IMAGE_SYM_TYPE_DOUBLE 7
#define IMAGE_SYM_TYPE_STRUCT 8
#define IMAGE_SYM_TYPE_UNION 9
#define IMAGE_SYM_TYPE_ENUM 10
#define IMAGE_SYM_TYPE_MOE 11
#define IMAGE_SYM_TYPE_BYTE 12
#define IMAGE_SYM_TYPE_WORD 13
#define IMAGE_SYM_TYPE_UINT 14
#define IMAGE_SYM_TYPE_DWORD 15
#define IMAGE_SYM_TYPE_PCODE 32768
#define IMAGE_SYM_DTYPE_NULL 0
#define IMAGE_SYM_DTYPE_POINTER 1
#define IMAGE_SYM_DTYPE_FUNCTION 2
#define IMAGE_SYM_DTYPE_ARRAY 3
#define IMAGE_SYM_CLASS_END_OF_FUNCTION	(-1)
#define IMAGE_SYM_CLASS_NULL 0
#define IMAGE_SYM_CLASS_AUTOMATIC 1
#define IMAGE_SYM_CLASS_EXTERNAL 2
#define IMAGE_SYM_CLASS_STATIC 3
#define IMAGE_SYM_CLASS_REGISTER 4
#define IMAGE_SYM_CLASS_EXTERNAL_DEF 5
#define IMAGE_SYM_CLASS_LABEL 6
#define IMAGE_SYM_CLASS_UNDEFINED_LABEL 7
#define IMAGE_SYM_CLASS_MEMBER_OF_STRUCT 8
#define IMAGE_SYM_CLASS_ARGUMENT 9
#define IMAGE_SYM_CLASS_STRUCT_TAG 10
#define IMAGE_SYM_CLASS_MEMBER_OF_UNION 11
#define IMAGE_SYM_CLASS_UNION_TAG 12
#define IMAGE_SYM_CLASS_TYPE_DEFINITION 13
#define IMAGE_SYM_CLASS_UNDEFINED_STATIC 14
#define IMAGE_SYM_CLASS_ENUM_TAG 15
#define IMAGE_SYM_CLASS_MEMBER_OF_ENUM 16
#define IMAGE_SYM_CLASS_REGISTER_PARAM 17
#define IMAGE_SYM_CLASS_BIT_FIELD 18
#define IMAGE_SYM_CLASS_FAR_EXTERNAL 68
#define IMAGE_SYM_CLASS_BLOCK 100
#define IMAGE_SYM_CLASS_FUNCTION 101
#define IMAGE_SYM_CLASS_END_OF_STRUCT 102
#define IMAGE_SYM_CLASS_FILE 103
#define IMAGE_SYM_CLASS_SECTION 104
#define IMAGE_SYM_CLASS_WEAK_EXTERNAL 105
#define IMAGE_COMDAT_SELECT_NODUPLICATES 1
#define IMAGE_COMDAT_SELECT_ANY 2
#define IMAGE_COMDAT_SELECT_SAME_SIZE 3
#define IMAGE_COMDAT_SELECT_EXACT_MATCH 4
#define IMAGE_COMDAT_SELECT_ASSOCIATIVE 5
#define IMAGE_COMDAT_SELECT_LARGEST 6
#define IMAGE_COMDAT_SELECT_NEWEST 7
#define IMAGE_WEAK_EXTERN_SEARCH_NOLIBRARY 1
#define IMAGE_WEAK_EXTERN_SEARCH_LIBRARY 2
#define IMAGE_WEAK_EXTERN_SEARCH_ALIAS 3
#define IMAGE_REL_I386_ABSOLUTE 0
#define IMAGE_REL_I386_DIR16 1
#define IMAGE_REL_I386_REL16 2
#define IMAGE_REL_I386_DIR32 6
#define IMAGE_REL_I386_DIR32NB 7
#define IMAGE_REL_I386_SEG12 9
#define IMAGE_REL_I386_SECTION 10
#define IMAGE_REL_I386_SECREL 11
#define IMAGE_REL_I386_REL32 20
#define IMAGE_REL_MIPS_ABSOLUTE 0
#define IMAGE_REL_MIPS_REFHALF 1
#define IMAGE_REL_MIPS_REFWORD 2
#define IMAGE_REL_MIPS_JMPADDR 3
#define IMAGE_REL_MIPS_REFHI 4
#define IMAGE_REL_MIPS_REFLO 5
#define IMAGE_REL_MIPS_GPREL 6
#define IMAGE_REL_MIPS_LITERAL 7
#define IMAGE_REL_MIPS_SECTION 10
#define IMAGE_REL_MIPS_SECREL 11
#define IMAGE_REL_MIPS_SECRELLO 12
#define IMAGE_REL_MIPS_SECRELHI 13
#define IMAGE_REL_MIPS_REFWORDNB 34
#define IMAGE_REL_MIPS_PAIR 35
#define IMAGE_REL_ALPHA_ABSOLUTE 0
#define IMAGE_REL_ALPHA_REFLONG 1
#define IMAGE_REL_ALPHA_REFQUAD 2
#define IMAGE_REL_ALPHA_GPREL32 3
#define IMAGE_REL_ALPHA_LITERAL 4
#define IMAGE_REL_ALPHA_LITUSE 5
#define IMAGE_REL_ALPHA_GPDISP 6
#define IMAGE_REL_ALPHA_BRADDR 7
#define IMAGE_REL_ALPHA_HINT 8
#define IMAGE_REL_ALPHA_INLINE_REFLONG 9
#define IMAGE_REL_ALPHA_REFHI 10
#define IMAGE_REL_ALPHA_REFLO 11
#define IMAGE_REL_ALPHA_PAIR 12
#define IMAGE_REL_ALPHA_MATCH 13
#define IMAGE_REL_ALPHA_SECTION 14
#define IMAGE_REL_ALPHA_SECREL 15
#define IMAGE_REL_ALPHA_REFLONGNB 16
#define IMAGE_REL_ALPHA_SECRELLO 17
#define IMAGE_REL_ALPHA_SECRELHI 18
#define IMAGE_REL_PPC_ABSOLUTE 0
#define IMAGE_REL_PPC_ADDR64 1
#define IMAGE_REL_PPC_ADDR32 2
#define IMAGE_REL_PPC_ADDR24 3
#define IMAGE_REL_PPC_ADDR16 4
#define IMAGE_REL_PPC_ADDR14 5
#define IMAGE_REL_PPC_REL24 6
#define IMAGE_REL_PPC_REL14 7
#define IMAGE_REL_PPC_TOCREL16 8
#define IMAGE_REL_PPC_TOCREL14 9
#define IMAGE_REL_PPC_ADDR32NB 10
#define IMAGE_REL_PPC_SECREL 11
#define IMAGE_REL_PPC_SECTION 12
#define IMAGE_REL_PPC_IFGLUE 13
#define IMAGE_REL_PPC_IMGLUE 14
#define IMAGE_REL_PPC_SECREL16 15
#define IMAGE_REL_PPC_REFHI 16
#define IMAGE_REL_PPC_REFLO 17
#define IMAGE_REL_PPC_PAIR 18
#define IMAGE_REL_PPC_TYPEMASK 255
#define IMAGE_REL_PPC_NEG 256
#define IMAGE_REL_PPC_BRTAKEN 512
#define IMAGE_REL_PPC_BRNTAKEN 1024
#define IMAGE_REL_PPC_TOCDEFN 2048
#define IMAGE_REL_BASED_ABSOLUTE 0
#define IMAGE_REL_BASED_HIGH 1
#define IMAGE_REL_BASED_LOW 2
#define IMAGE_REL_BASED_HIGHLOW 3
#define IMAGE_REL_BASED_HIGHADJ 4
#define IMAGE_REL_BASED_MIPS_JMPADDR 5
#define IMAGE_ARCHIVE_START_SIZE 8
#define IMAGE_ARCHIVE_START "!<arch>\n"
#define IMAGE_ARCHIVE_END "`\n"
#define IMAGE_ARCHIVE_PAD "\n"
#define IMAGE_ARCHIVE_LINKER_MEMBER "/               "
#define IMAGE_ARCHIVE_LONGNAMES_MEMBER "//              "
#define IMAGE_ORDINAL_FLAG 0x80000000
#define IMAGE_SNAP_BY_ORDINAL(o) ((o&IMAGE_ORDINAL_FLAG)!=0)
#define IMAGE_RESOURCE_NAME_IS_STRING 0x80000000
#define IMAGE_RESOURCE_DATA_IS_DIRECTORY 0x80000000
#define IMAGE_DEBUG_TYPE_UNKNOWN 0
#define IMAGE_DEBUG_TYPE_COFF 1
#define IMAGE_DEBUG_TYPE_CODEVIEW 2
#define IMAGE_DEBUG_TYPE_FPO 3
#define IMAGE_DEBUG_TYPE_MISC 4
#define IMAGE_DEBUG_TYPE_EXCEPTION 5
#define IMAGE_DEBUG_TYPE_FIXUP 6
#define IMAGE_DEBUG_TYPE_OMAP_TO_SRC 7
#define IMAGE_DEBUG_TYPE_OMAP_FROM_SRC 8


#define IMAGE_SIZEOF_SHORT_NAME              8

#define IMAGE_SIZEOF_SECTION_HEADER          40

#define IMAGE_SECTION_CODE (0x20)
#define IMAGE_SECTION_INITIALIZED_DATA (0x40)
#define IMAGE_SECTION_UNINITIALIZED_DATA (0x80)

//
// Import Format
//

#define IMAGE_ORDINAL_FLAG 0x80000000


// Predefined resource types ... there may be some more, but I don't have
//                               the information yet.  .....sang cho.....

#define    RT_NEWRESOURCE   0x2000
#define    RT_ERROR         0x7fff
#define    NEWBITMAP        (RT_BITMAP|RT_NEWRESOURCE)
#define    NEWMENU          (RT_MENU|RT_NEWRESOURCE)
#define    NEWDIALOG        (RT_DIALOG|RT_NEWRESOURCE)


//
// Resource Format.
//

//
// Resource directory consists of two counts, following by a variable length
// array of directory entries.  The first count is the number of entries at
// beginning of the array that have actual names associated with each entry.
// The entries are in ascending order, case insensitive strings.  The second
// count is the number of entries that immediately follow the named entries.
// This second count identifies the number of entries that have 16-bit integer
// Ids as their name.  These entries are also sorted in ascending order.
//
// This structure allows fast lookup by either name or number, but for any
// given resource entry only one form of lookup is supported, not both.
// This is consistant with the syntax of the .RC file and the .RES file.
//


#define IMAGE_RESOURCE_NAME_IS_STRING        0x80000000
#define IMAGE_RESOURCE_DATA_IS_DIRECTORY     0x80000000



//
// For resource directory entries that have actual string names, the Name
// field of the directory entry points to an object of the following type.
// All of these string objects are stored together after the last resource
// directory entry and before the first resource data object.  This minimizes
// the impact of these variable length objects on the alignment of the fixed
// size directory entry objects.
//
/* defined above from mingw. ei
typedef struct _IMAGE_RESOURCE_DIRECTORY_STRING {
    WORD    Length;
    CHAR    NameString[ 1 ];
} IMAGE_RESOURCE_DIRECTORY_STRING, *PIMAGE_RESOURCE_DIRECTORY_STRING;


typedef struct _IMAGE_RESOURCE_DIR_STRING_U {
    WORD    Length;
    WCHAR   NameString[ 1 ];
} IMAGE_RESOURCE_DIR_STRING_U, *PIMAGE_RESOURCE_DIR_STRING_U;
*/

//
// Each resource data entry describes a leaf node in the resource directory
// tree.  It contains an offset, relative to the beginning of the resource
// directory of the data for the resource, a size field that gives the number
// of bytes of data at that offset, a CodePage that should be used when
// decoding code point values within the resource data.  Typically for new
// applications the code page would be the unicode code page.
//
/* ei
typedef struct _IMAGE_RESOURCE_DATA_ENTRY {
    DWORD   OffsetToData;
    DWORD   Size;
    DWORD   CodePage;
    DWORD   Reserved;
} IMAGE_RESOURCE_DATA_ENTRY, *PIMAGE_RESOURCE_DATA_ENTRY;
*/

//  Menu Resources	 ... added by .....sang cho....

// Menu resources are composed of a menu header followed by a sequential list
// of menu items. There are two types of menu items: pop-ups and normal menu
// itmes. The MENUITEM SEPARATOR is a special case of a normal menu item with
// an empty name, zero ID, and zero flags.

typedef struct _IMAGE_MENU_HEADER{
	WORD   wVersion;	// Currently zero
	WORD   cbHeaderSize;	// Also zero
} IMAGE_MENU_HEADER, *PIMAGE_MENU_HEADER;

typedef struct _IMAGE_POPUP_MENU_ITEM{
	WORD   fItemFlags;
	WCHAR  szItemText[1];
} IMAGE_POPUP_MENU_ITEM, *PIMAGE_POPUP_MENU_ITEM;

typedef struct _IMAGE_NORMAL_MENU_ITEM{
	WORD   fItemFlags;
	WORD   wMenuID;
	WCHAR  szItemText[1];
} IMAGE_NORMAL_MENU_ITEM, *PIMAGE_NORMAL_MENU_ITEM;

#define MI_GRAYED       0x0001 // GRAYED keyword
#define MI_INACTIVE     0x0002 // INACTIVE keyword
#define MI_BITMAP       0x0004 // BITMAP keyword
#define MI_OWNERDRAW    0x0100 // OWNERDRAW keyword
#define MI_CHECKED      0x0008 // CHECKED keyword
#define MI_POPUP        0x0010 // used internally
#define MI_MENUBARBREAK 0x0020 // MENUBARBREAK keyword
#define MI_MENUBREAK    0x0040 // MENUBREAK keyword
#define MI_ENDMENU      0x0080 // used internally

// Dialog Box Resources	.................. added by sang cho.

// A dialog box is contained in a single resource and has a header and
// a portion repeated for each control in the dialog box.
// The item DWORD IStyle is a standard window style composed of flags found
// in WINDOWS.H.
// The default style for a dialog box is:
// WS_POPUP | WS_BORDER | WS_SYSMENU
//
// The itme marked "Name or Ordinal" are :
// If the first word is an 0xffff, the next two bytes contain an ordinal ID.
// Otherwise, the first one or more WORDS contain a double-null-terminated string.
// An empty string is represented by a single WORD zero in the first location.
//
// The WORD wPointSize and WCHAR szFontName entries are present if the FONT
// statement was included for the dialog box. This can be detected by checking
// the entry IStyle. If IStyle & DS_SETFONT ( which is 0x40), then these
// entries will be present.

typedef struct _IMAGE_DIALOG_BOX_HEADER1{
	DWORD  IStyle;
	DWORD  IExtendedStyle;    // New for Windows NT
	WORD   nControls;         // Number of Controls
	WORD   x;
	WORD   y;
	WORD   cx;
	WORD   cy;
//	N_OR_O MenuName;         // Name or Ordinal ID
//	N_OR_O ClassName;		 // Name or Ordinal ID
//	WCHAR  szCaption[];
//	WORD   wPointSize;       // Only here if FONT set for dialog
//	WCHAR  szFontName[];     // This too
} IMAGE_DIALOG_HEADER, *PIMAGE_DIALOG_HEADER;

typedef union _NAME_OR_ORDINAL{    // Name or Ordinal ID
	struct _ORD_ID{
	    WORD   flgId;
	    WORD   Id;
	} ORD_ID;
	WCHAR  szName[1];
} NAME_OR_ORDINAL, *PNAME_OR_ORDINAL;

// The data for each control starts on a DWORD boundary (which may require
// some padding from the previous control), and its format is as follows:

typedef struct _IMAGE_CONTROL_DATA{
	DWORD   IStyle;
	DWORD   IExtendedStyle;
	WORD    x;
	WORD    y;
	WORD    cx;
	WORD    cy;
	WORD    wId;
//  N_OR_O  ClassId;
//  N_OR_O  Text;
//  WORD    nExtraStuff;
} IMAGE_CONTROL_DATA, *PIMAGE_CONTROL_DATA;

#define BUTTON       0x80
#define EDIT         0x81
//#define STATIC       0x82
#define LISTBOX      0x83
#define SCROLLBAR    0x84
#define COMBOBOX     0x85

// The various statements used in a dialog script are all mapped to these
// classes along with certain modifying styles. The values for these styles
// can be found in WINDOWS.H. All dialog controls have the default styles
// of WS_CHILD and WS_VISIBLE. A list of the default styles used follows:
//
// Statement           Default Class         Default Styles
// CONTROL             None                  WS_CHILD|WS_VISIBLE
// LTEXT               STATIC                ES_LEFT
// RTEXT               STATIC                ES_RIGHT
// CTEXT               STATIC                ES_CENTER
// LISTBOX             LISTBOX               WS_BORDER|LBS_NOTIFY
// CHECKBOX            BUTTON                BS_CHECKBOX|WS_TABSTOP
// PUSHBUTTON          BUTTON                BS_PUSHBUTTON|WS_TABSTOP
// GROUPBOX            BUTTON                BS_GROUPBOX
// DEFPUSHBUTTON       BUTTON                BS_DFPUSHBUTTON|WS_TABSTOP
// RADIOBUTTON         BUTTON                BS_RADIOBUTTON
// AUTOCHECKBOX        BUTTON                BS_AUTOCHECKBOX
// AUTO3STATE          BUTTON                BS_AUTO3STATE
// AUTORADIOBUTTON     BUTTON                BS_AUTORADIOBUTTON
// PUSHBOX             BUTTON                BS_PUSHBOX
// STATE3              BUTTON                BS_3STATE
// EDITTEXT            EDIT                  ES_LEFT|WS_BORDER|WS_TABSTOP
// COMBOBOX            COMBOBOX              None
// ICON                STATIC                SS_ICON
// SCROLLBAR           SCROLLBAR             None
///

#define IMAGE_DEBUG_TYPE_UNKNOWN          0
#define IMAGE_DEBUG_TYPE_COFF             1
#define IMAGE_DEBUG_TYPE_CODEVIEW         2
#define IMAGE_DEBUG_TYPE_FPO              3
#define IMAGE_DEBUG_TYPE_MISC             4
#define IMAGE_DEBUG_TYPE_EXCEPTION        5
#define IMAGE_DEBUG_TYPE_FIXUP            6
#define IMAGE_DEBUG_TYPE_OMAP_TO_SRC      7
#define IMAGE_DEBUG_TYPE_OMAP_FROM_SRC    8



//
// Debugging information can be stripped from an image file and placed
// in a separate .DBG file, whose file name part is the same as the
// image file name part (e.g. symbols for CMD.EXE could be stripped
// and placed in CMD.DBG).  This is indicated by the IMAGE_FILE_DEBUG_STRIPPED
// flag in the Characteristics field of the file header.  The beginning of
// the .DBG file contains the following structure which captures certain
// information from the image file.  This allows a debug to proceed even if
// the original image file is not accessable.  This header is followed by
// zero of more IMAGE_SECTION_HEADER structures, followed by zero or more
// IMAGE_DEBUG_DIRECTORY structures.  The latter structures and those in
// the image file contain file offsets relative to the beginning of the
// .DBG file.
//
// If symbols have been stripped from an image, the IMAGE_DEBUG_MISC structure
// is left in the image file, but not mapped.  This allows a debugger to
// compute the name of the .DBG file, from the name of the image in the
// IMAGE_DEBUG_MISC structure.
//
/*  ei
typedef struct _IMAGE_SEPARATE_DEBUG_HEADER {
    WORD        Signature;
    WORD        Flags;
    WORD        Machine;
    WORD        Characteristics;
    DWORD       TimeDateStamp;
    DWORD       CheckSum;
    DWORD       ImageBase;
    DWORD       SizeOfImage;
    DWORD       NumberOfSections;
    DWORD       ExportedNamesSize;
    DWORD       DebugDirectorySize;
    DWORD       SectionAlignment;
    DWORD       Reserved[2];
} IMAGE_SEPARATE_DEBUG_HEADER, *PIMAGE_SEPARATE_DEBUG_HEADER;
*/
#define IMAGE_SEPARATE_DEBUG_SIGNATURE  0x4944

#define IMAGE_SEPARATE_DEBUG_FLAGS_MASK 0x8000
#define IMAGE_SEPARATE_DEBUG_MISMATCH   0x8000  // when DBG was updated, the
                                                // old checksum didn't match.

//
// End Image Format
//

#define SIZE_OF_NT_SIGNATURE	sizeof (DWORD)
#define MAXRESOURCENAME 	13

/* global macros to define header offsets into file */
/* offset to PE file signature				       */
#define NTSIGNATURE(a) ((LPVOID)((BYTE *)a		     +	\
			((PIMAGE_DOS_HEADER)a)->e_lfanew))

/* DOS header identifies the NT PEFile signature dword
   the PEFILE header exists just after that dword	       */
#define PEFHDROFFSET(a) ((LPVOID)((BYTE *)a		     +	\
			 ((PIMAGE_DOS_HEADER)a)->e_lfanew    +	\
			 SIZE_OF_NT_SIGNATURE))

/* PE optional header is immediately after PEFile header       */
#define OPTHDROFFSET(a) ((LPVOID)((BYTE *)a		     +	\
			 ((PIMAGE_DOS_HEADER)a)->e_lfanew    +	\
			 SIZE_OF_NT_SIGNATURE		     +	\
			 sizeof (IMAGE_FILE_HEADER)))

/* section headers are immediately after PE optional header    */
#define SECHDROFFSET(a) ((LPVOID)((BYTE *)a		     +	\
			 ((PIMAGE_DOS_HEADER)a)->e_lfanew    +	\
			 SIZE_OF_NT_SIGNATURE		     +	\
			 sizeof (IMAGE_FILE_HEADER)	     +	\
			 sizeof (IMAGE_OPTIONAL_HEADER)))

//#define FIELD_OFFSET(type, field)    ((LONG)(LONG_PTR)&(((type *)0)->field))
/* defined above ei
#define IMAGE_FIRST_SECTION( ntheader ) ((PIMAGE_SECTION_HEADER)        \
    ((ULONG_PTR)ntheader +                                              \
     FIELD_OFFSET( IMAGE_NT_HEADERS, OptionalHeader ) +                 \
     ((PIMAGE_NT_HEADERS)(ntheader))->FileHeader.SizeOfOptionalHeader   \
    ))
*/

#define MakePtr( cast, ptr, addValue ) (cast)( (DWORD)(ptr) + (DWORD)(addValue))

typedef struct _IMAGE_IMPORT_MODULE_DIRECTORY
{
  DWORD    dwRVAFunctionNameList;
  DWORD    dwUseless1;
  DWORD    dwUseless2;
  DWORD    dwRVAModuleName;
  DWORD    dwRVAFunctionAddressList;
} IMAGE_IMPORT_MODULE_DIRECTORY, *PIMAGE_IMPORT_MODULE_DIRECTORY;

typedef struct _RELOCATION_DIRECTORY
{
    DWORD  VirtualAddress; /* adresse virtuelle du bloc ou se font les relocations */
    DWORD   SizeOfBlock;    // taille de cette structure + des structures
			// relocation_entry qui suivent (ces dernieres sont
			// donc au nombre de (SizeOfBlock-8)/2
} RELOCATION_DIRECTORY, *PRELOCATION_DIRECTORY;

typedef struct _RELOCATION_ENTRY
{
    WORD    TypeOffset;
	//	(TypeOffset >> 12) est le type
	//	(TypeOffset&0xfff) est l'offset dans le bloc
} RELOCATION_ENTRY, *PRELOCATION_ENTRY;

#define	TYPE_RELOC_ABSOLUTE	0
#define	TYPE_RELOC_HIGH		1
#define	TYPE_RELOC_LOW		2
#define	TYPE_RELOC_HIGHLOW	3
#define	TYPE_RELOC_HIGHADJ	4
#define	TYPE_RELOC_MIPS_JMPADDR	5

#endif /* __INCLUDE_PE_H */
