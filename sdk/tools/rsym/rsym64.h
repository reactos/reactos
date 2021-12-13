#pragma once

//C_ASSERT(sizeof(ULONG) == 4);
typedef unsigned char UBYTE;
#if defined(_MSC_VER) || defined(__MINGW32__)
typedef unsigned __int64 ULONG64;
#else
#include <stdint.h>
typedef uint64_t ULONG64;
#endif


#define IMAGE_FILE_MACHINE_I386 0x14c
#define IMAGE_FILE_MACHINE_AMD64 0x8664
#define IMAGE_FILE_MACHINE_ARM64 0xaa64
#define IMAGE_DIRECTORY_ENTRY_IMPORT 1
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION 3

#define UWOP_PUSH_NONVOL 0
#define UWOP_ALLOC_LARGE 1
#define UWOP_ALLOC_SMALL 2
#define UWOP_SET_FPREG 3
#define UWOP_SAVE_NONVOL 4
#define UWOP_SAVE_NONVOL_FAR 5
#define UWOP_SAVE_XMM 6
#define UWOP_SAVE_XMM_FAR 7
#define UWOP_SAVE_XMM128 8
#define UWOP_SAVE_XMM128_FAR 9
#define UWOP_PUSH_MACHFRAME 10

#define REG_RAX 0
#define REG_RCX 1
#define REG_RDX 2
#define REG_RBX 3
#define REG_RSP 4
#define REG_RBP 5
#define REG_RSI 6
#define REG_RDI 7
#define REG_R8 8
#define REG_R9 9
#define REG_R10 10
#define REG_R11 11
#define REG_R12 12
#define REG_R13 13
#define REG_R14 14
#define REG_R15 15

#define REG_XMM0 0
#define REG_XMM1 1
#define REG_XMM2 2
#define REG_XMM3 3
#define REG_XMM4 4
#define REG_XMM5 5
#define REG_XMM6 6
#define REG_XMM7 7
#define REG_XMM8 8
#define REG_XMM9 9
#define REG_XMM10 10
#define REG_XMM11 11
#define REG_XMM12 12
#define REG_XMM13 13
#define REG_XMM14 14
#define REG_XMM15 15


typedef struct _IMAGE_IMPORT_DESCRIPTOR
{
    union {
        DWORD   Characteristics;
        DWORD   OriginalFirstThunk;
    };
    DWORD   TimeDateStamp;
    DWORD   ForwarderChain;
    DWORD   Name;
    DWORD   FirstThunk;
} IMAGE_IMPORT_DESCRIPTOR, *PIMAGE_IMPORT_DESCRIPTOR;

typedef struct _IMAGE_THUNK_DATA64
{
    union {
        ULONGLONG ForwarderString;
        ULONGLONG Function;
        ULONGLONG Ordinal;
        ULONGLONG AddressOfData;
    } u1;
} IMAGE_THUNK_DATA64, *PIMAGE_THUNK_DATA64;

typedef struct _RUNTIME_FUNCTION
{
    ULONG FunctionStart;
    ULONG FunctionEnd;
    ULONG UnwindInfo;
} RUNTIME_FUNCTION, *PRUNTIME_FUNCTION;

typedef union _UNWIND_CODE
{
    struct
    {
        UBYTE CodeOffset;
        UBYTE UnwindOp:4;
        UBYTE OpInfo:4;
    };
    USHORT FrameOffset;
} UNWIND_CODE, *PUNWIND_CODE;

enum
{
    UNW_FLAG_EHANDLER  = 0x01,
    UNW_FLAG_UHANDLER  = 0x02,
    UNW_FLAG_CHAININFO = 0x03,
};

typedef struct _UNWIND_INFO
{
    UBYTE Version:3;
    UBYTE Flags:5;
    UBYTE SizeOfProlog;
    UBYTE CountOfCodes;
    UBYTE FrameRegister:4;
    UBYTE FrameOffset:4;
    UNWIND_CODE UnwindCode[1];
/*    union {
        OPTIONAL ULONG ExceptionHandler;
        OPTIONAL ULONG FunctionEntry;
    };
    OPTIONAL ULONG ExceptionData[];
*/
} UNWIND_INFO, *PUNWIND_INFO;

typedef struct _C_SCOPE_TABLE_ENTRY
{
    ULONG Begin;
    ULONG End;
    ULONG Handler;
    ULONG Target;
} C_SCOPE_TABLE_ENTRY, *PC_SCOPE_TABLE_ENTRY;

typedef struct _C_SCOPE_TABLE
{
    ULONG NumEntries;
    C_SCOPE_TABLE_ENTRY Entry[1];
} C_SCOPE_TABLE, *PC_SCOPE_TABLE;


typedef struct
{
    IMAGE_SECTION_HEADER *psh;
    char *pName;
    void *p;
    ULONG idx;
} SECTION;

typedef struct
{
    char* FilePtr;
    size_t cbInFileSize;
    size_t cbNewFileSize;

    /* PE data pointers */
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_FILE_HEADER FileHeader;
    PIMAGE_OPTIONAL_HEADER64 OptionalHeader;
    PIMAGE_SECTION_HEADER SectionHeaders;
    PIMAGE_SECTION_HEADER NewSectionHeaders;
    ULONG NewSectionHeaderSize;
    PIMAGE_BASE_RELOCATION Relocations;
    void *Symbols;
    char *Strings;
    ULONG64 ImageBase;
    ULONG HeaderSize;
    char *UseSection;

    /* Sections */
    ULONG AllSections;
    ULONG UsedSections;

    SECTION eh_frame;
    SECTION pdata;
    SECTION xdata;

    char *AlignBuf;

    ULONG cFuncs;
    ULONG cUWOP;
    ULONG cScopes;

} FILE_INFO, *PFILE_INFO;
