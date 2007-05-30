/*
 * Declarations for DBGHELP
 *
 * Copyright (C) 2003 Eric Pouech
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_DBGHELP_H
#define __WINE_DBGHELP_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

typedef struct _LOADED_IMAGE
{
    LPSTR                       ModuleName;
    HANDLE                      hFile;
    PUCHAR                      MappedAddress;
    PIMAGE_NT_HEADERS           FileHeader;
    PIMAGE_SECTION_HEADER       LastRvaSection;
    ULONG                       NumberOfSections;
    PIMAGE_SECTION_HEADER       Sections;
    ULONG                       Characteristics;
    BOOLEAN                     fSystemImage;
    BOOLEAN                     fDOSImage;
    LIST_ENTRY                  Links;
    ULONG                       SizeOfImage;
} LOADED_IMAGE, *PLOADED_IMAGE;

/*************************
 *    IMAGEHLP equiv     *
 *************************/

typedef enum
{
    AddrMode1616,
    AddrMode1632,
    AddrModeReal,
    AddrModeFlat
} ADDRESS_MODE;

typedef struct _tagADDRESS
{
    DWORD                       Offset;
    WORD                        Segment;
    ADDRESS_MODE                Mode;
} ADDRESS, *LPADDRESS;

#define SYMF_OMAP_GENERATED   0x00000001
#define SYMF_OMAP_MODIFIED    0x00000002
#define SYMF_USER_GENERATED   0x00000004
#define SYMF_REGISTER         0x00000008
#define SYMF_REGREL           0x00000010
#define SYMF_FRAMEREL         0x00000020
#define SYMF_PARAMETER        0x00000040
#define SYMF_LOCAL            0x00000080
#define SYMF_CONSTANT         0x00000100
#define SYMF_EXPORT           0x00000200
#define SYMF_FORWARDER        0x00000400
#define SYMF_FUNCTION         0x00000800
#define SYMF_VIRTUAL          0x00001000
#define SYMF_THUNK            0x00002000
#define SYMF_TLSREL           0x00004000

typedef enum 
{
    SymNone = 0,
    SymCoff,
    SymCv,
    SymPdb,
    SymExport,
    SymDeferred,
    SymSym,
    SymDia,
    NumSymTypes
} SYM_TYPE;

typedef struct _IMAGEHLP_SYMBOL
{
    DWORD                       SizeOfStruct;
    DWORD                       Address;
    DWORD                       Size;
    DWORD                       Flags;
    DWORD                       MaxNameLength;
    CHAR                        Name[1];
} IMAGEHLP_SYMBOL, *PIMAGEHLP_SYMBOL;

typedef struct _IMAGEHLP_MODULE
{
    DWORD                       SizeOfStruct;
    DWORD                       BaseOfImage;
    DWORD                       ImageSize;
    DWORD                       TimeDateStamp;
    DWORD                       CheckSum;
    DWORD                       NumSyms;
    SYM_TYPE                    SymType;
    CHAR                        ModuleName[32];
    CHAR                        ImageName[256];
    CHAR                        LoadedImageName[256];
} IMAGEHLP_MODULE, *PIMAGEHLP_MODULE;

typedef struct _IMAGEHLP_MODULEW {
    DWORD                       SizeOfStruct;
    DWORD                       BaseOfImage;
    DWORD                       ImageSize;
    DWORD                       TimeDateStamp;
    DWORD                       CheckSum;
    DWORD                       NumSyms;
    SYM_TYPE                    SymType;
    WCHAR                       ModuleName[32];
    WCHAR                       ImageName[256];
    WCHAR                       LoadedImageName[256];
} IMAGEHLP_MODULEW, *PIMAGEHLP_MODULEW;

typedef struct _IMAGEHLP_LINE
{
    DWORD                       SizeOfStruct;
    PVOID                       Key;
    DWORD                       LineNumber;
    PCHAR                       FileName;
    DWORD                       Address;
} IMAGEHLP_LINE, *PIMAGEHLP_LINE;

typedef struct _SOURCEFILE
{
    DWORD                       ModBase;
    PCHAR                       FileName;
} SOURCEFILE, *PSOURCEFILE;

#define CBA_DEFERRED_SYMBOL_LOAD_START          0x00000001
#define CBA_DEFERRED_SYMBOL_LOAD_COMPLETE       0x00000002
#define CBA_DEFERRED_SYMBOL_LOAD_FAILURE        0x00000003
#define CBA_SYMBOLS_UNLOADED                    0x00000004
#define CBA_DUPLICATE_SYMBOL                    0x00000005
#define CBA_READ_MEMORY                         0x00000006
#define CBA_DEFERRED_SYMBOL_LOAD_CANCEL         0x00000007
#define CBA_SET_OPTIONS                         0x00000008
#define CBA_EVENT                               0x00000010
#define CBA_DEBUG_INFO                          0x10000000

typedef struct _IMAGEHLP_CBA_READ_MEMORY
{
    DWORD64   addr;
    PVOID     buf;
    DWORD     bytes;
    DWORD    *bytesread;
} IMAGEHLP_CBA_READ_MEMORY, *PIMAGEHLP_CBA_READ_MEMORY;

enum {
    sevInfo = 0,
    sevProblem,
    sevAttn,
    sevFatal,
    sevMax
};

typedef struct _IMAGEHLP_CBA_EVENT
{
    DWORD severity;
    DWORD code;
    PCHAR desc;
    PVOID object;
} IMAGEHLP_CBA_EVENT, *PIMAGEHLP_CBA_EVENT;

typedef struct _IMAGEHLP_DEFERRED_SYMBOL_LOAD
{
    DWORD                       SizeOfStruct;
    DWORD64                     BaseOfImage;
    DWORD                       CheckSum;
    DWORD                       TimeDateStamp;
    CHAR                        FileName[MAX_PATH];
    BOOLEAN                     Reparse;
} IMAGEHLP_DEFERRED_SYMBOL_LOAD, *PIMAGEHLP_DEFERRED_SYMBOL_LOAD;

typedef struct _IMAGEHLP_DUPLICATE_SYMBOL
{
    DWORD                       SizeOfStruct;
    DWORD                       NumberOfDups;
    PIMAGEHLP_SYMBOL            Symbol;
    DWORD                       SelectedSymbol;
} IMAGEHLP_DUPLICATE_SYMBOL, *PIMAGEHLP_DUPLICATE_SYMBOL;

#define SYMOPT_CASE_INSENSITIVE         0x00000001
#define SYMOPT_UNDNAME                  0x00000002
#define SYMOPT_DEFERRED_LOADS           0x00000004
#define SYMOPT_NO_CPP                   0x00000008
#define SYMOPT_LOAD_LINES               0x00000010
#define SYMOPT_OMAP_FIND_NEAREST        0x00000020
#define SYMOPT_LOAD_ANYTHING            0x00000040
#define SYMOPT_IGNORE_CVREC             0x00000080
#define SYMOPT_NO_UNQUALIFIED_LOADS     0x00000100
#define SYMOPT_FAIL_CRITICAL_ERRORS     0x00000200
#define SYMOPT_EXACT_SYMBOLS            0x00000400
#define SYMOPT_WILD_UNDERSCORE          0x00000800
#define SYMOPT_USE_DEFAULTS             0x00001000
#define SYMOPT_INCLUDE_32BIT_MODULES    0x00002000
#define SYMOPT_PUBLICS_ONLY             0x00004000
#define SYMOPT_NO_PUBLICS               0x00008000
#define SYMOPT_AUTO_PUBLICS             0x00010000
#define SYMOPT_NO_IMAGE_SEARCH          0x00020000
#define SYMOPT_SECURE                   0x00040000
#define SYMOPT_NO_PROMPTS               0x00080000

#define SYMOPT_DEBUG                    0x80000000

typedef struct _IMAGEHLP_STACK_FRAME
{
    ULONG64     InstructionOffset;
    ULONG64     ReturnOffset;
    ULONG64     FrameOffset;
    ULONG64     StackOffset;
    ULONG64     BackingStoreOffset;
    ULONG64     FuncTableEntry;
    ULONG64     Params[4];
    ULONG64     Reserved[5];
    BOOL        Virtual;
    ULONG       Reserved2;
} IMAGEHLP_STACK_FRAME, *PIMAGEHLP_STACK_FRAME;

typedef VOID IMAGEHLP_CONTEXT, *PIMAGEHLP_CONTEXT;

#define DBHHEADER_DEBUGDIRS     0x1
typedef struct _DBGHELP_MODLOAD_DATA
{
    DWORD               ssize;
    DWORD               ssig;
    PVOID               data;
    DWORD               size;
    DWORD               flags;
} MODLOAD_DATA, *PMODLOAD_DATA;

/*************************
 *       MiniDUMP        *
 *************************/

/* DebugHelp */

#define MINIDUMP_SIGNATURE 0x4D444D50 /* 'PMDM' */
#define MINIDUMP_VERSION   (42899)

typedef DWORD   RVA;
typedef ULONG64 RVA64;

typedef enum _MINIDUMP_TYPE 
{
    MiniDumpNormal         = 0x0000,
    MiniDumpWithDataSegs   = 0x0001,
    MiniDumpWithFullMemory = 0x0002,
    MiniDumpWithHandleData = 0x0004,
    MiniDumpFilterMemory   = 0x0008,
    MiniDumpScanMemory     = 0x0010
} MINIDUMP_TYPE;

typedef enum _MINIDUMP_CALLBACK_TYPE
{
    ModuleCallback,
    ThreadCallback,
    ThreadExCallback,
    IncludeThreadCallback,
    IncludeModuleCallback,
} MINIDUMP_CALLBACK_TYPE;

typedef struct _MINIDUMP_THREAD_CALLBACK
{
    ULONG                       ThreadId;
    HANDLE                      ThreadHandle;
    CONTEXT                     Context;
    ULONG                       SizeOfContext;
    ULONGLONG                   StackBase;
    ULONG64                     StackEnd;
} MINIDUMP_THREAD_CALLBACK, *PMINIDUMP_THREAD_CALLBACK;

typedef struct _MINIDUMP_THREAD_EX_CALLBACK 
{
    ULONG                       ThreadId;
    HANDLE                      ThreadHandle;
    CONTEXT                     Context;
    ULONG                       SizeOfContext;
    ULONGLONG                   StackBase;
    ULONGLONG                   StackEnd;
    ULONGLONG                   BackingStoreBase;
    ULONGLONG                   BackingStoreEnd;
} MINIDUMP_THREAD_EX_CALLBACK, *PMINIDUMP_THREAD_EX_CALLBACK;

typedef struct _MINIDUMP_INCLUDE_THREAD_CALLBACK
{
    ULONG ThreadId;
} MINIDUMP_INCLUDE_THREAD_CALLBACK, *PMINIDUMP_INCLUDE_THREAD_CALLBACK;

typedef enum _THREAD_WRITE_FLAGS 
{
    ThreadWriteThread            = 0x0001,
    ThreadWriteStack             = 0x0002,
    ThreadWriteContext           = 0x0004,
    ThreadWriteBackingStore      = 0x0008,
    ThreadWriteInstructionWindow = 0x0010
} THREAD_WRITE_FLAGS;

typedef struct _MINIDUMP_MODULE_CALLBACK 
{
    PWCHAR                      FullPath;
    ULONGLONG                   BaseOfImage;
    ULONG                       SizeOfImage;
    ULONG                       CheckSum;
    ULONG                       TimeDateStamp;
    VS_FIXEDFILEINFO            VersionInfo;
    PVOID                       CvRecord;
    ULONG                       SizeOfCvRecord;
    PVOID                       MiscRecord;
    ULONG                       SizeOfMiscRecord;
} MINIDUMP_MODULE_CALLBACK, *PMINIDUMP_MODULE_CALLBACK;

typedef struct _MINIDUMP_INCLUDE_MODULE_CALLBACK 
{
    ULONG64 BaseOfImage;
} MINIDUMP_INCLUDE_MODULE_CALLBACK, *PMINIDUMP_INCLUDE_MODULE_CALLBACK;

typedef enum _MODULE_WRITE_FLAGS 
{
    ModuleWriteModule        = 0x0001,
    ModuleWriteDataSeg       = 0x0002,
    ModuleWriteMiscRecord    = 0x0004,
    ModuleWriteCvRecord      = 0x0008,
    ModuleReferencedByMemory = 0x0010
} MODULE_WRITE_FLAGS;

typedef struct _MINIDUMP_CALLBACK_INPUT 
{
    ULONG                       ProcessId;
    HANDLE                      ProcessHandle;
    ULONG                       CallbackType;
    union 
    {
        MINIDUMP_THREAD_CALLBACK        Thread;
        MINIDUMP_THREAD_EX_CALLBACK     ThreadEx;
        MINIDUMP_MODULE_CALLBACK        Module;
        MINIDUMP_INCLUDE_THREAD_CALLBACK IncludeThread;
        MINIDUMP_INCLUDE_MODULE_CALLBACK IncludeModule;
    } u;
} MINIDUMP_CALLBACK_INPUT, *PMINIDUMP_CALLBACK_INPUT;

typedef struct _MINIDUMP_CALLBACK_OUTPUT
{
    union 
    {
        ULONG                           ModuleWriteFlags;
        ULONG                           ThreadWriteFlags;
    } u;
} MINIDUMP_CALLBACK_OUTPUT, *PMINIDUMP_CALLBACK_OUTPUT;

typedef BOOL (WINAPI* MINIDUMP_CALLBACK_ROUTINE)(PVOID CallbackParam,
                                                 const PMINIDUMP_CALLBACK_INPUT CallbackInput,
                                                 PMINIDUMP_CALLBACK_OUTPUT CallbackOutput);

typedef struct _MINIDUMP_CALLBACK_INFORMATION 
{
    MINIDUMP_CALLBACK_ROUTINE   CallbackRoutine;
    void*                       CallbackParam;
} MINIDUMP_CALLBACK_INFORMATION, *PMINIDUMP_CALLBACK_INFORMATION;

typedef struct _MINIDUMP_LOCATION_DESCRIPTOR 
{
    ULONG                       DataSize;
    RVA                         Rva;
} MINIDUMP_LOCATION_DESCRIPTOR;

typedef struct _MINIDUMP_DIRECTORY 
{
    ULONG                       StreamType;
    MINIDUMP_LOCATION_DESCRIPTOR Location;
} MINIDUMP_DIRECTORY, *PMINIDUMP_DIRECTORY;

typedef struct _MINIDUMP_EXCEPTION
{
    ULONG                       ExceptionCode;
    ULONG                       ExceptionFlags;
    ULONGLONG                   ExceptionRecord;
    ULONGLONG                   ExceptionAddress;
    ULONG                       NumberParameters;
    ULONG                        __unusedAlignment;
    ULONGLONG                   ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} MINIDUMP_EXCEPTION, *PMINIDUMP_EXCEPTION;

typedef struct _MINIDUMP_EXCEPTION_INFORMATION
{
    DWORD                       ThreadId;
    PEXCEPTION_POINTERS         ExceptionPointers;
    BOOL                        ClientPointers;
} MINIDUMP_EXCEPTION_INFORMATION, *PMINIDUMP_EXCEPTION_INFORMATION;

typedef struct MINIDUMP_EXCEPTION_STREAM 
{
    ULONG                       ThreadId;
    ULONG                       __alignment;
    MINIDUMP_EXCEPTION          ExceptionRecord;
    MINIDUMP_LOCATION_DESCRIPTOR ThreadContext;
} MINIDUMP_EXCEPTION_STREAM, *PMINIDUMP_EXCEPTION_STREAM;

typedef struct _MINIDUMP_HEADER 
{
    DWORD                       Signature;
    DWORD                       Version;
    DWORD                       NumberOfStreams;
    RVA                         StreamDirectoryRva;
    DWORD                       CheckSum;
    union 
    {
        DWORD                           Reserved;
        DWORD                           TimeDateStamp;
    } u;
    ULONGLONG                   Flags;
} MINIDUMP_HEADER, *PMINIDUMP_HEADER;

typedef struct _MINIDUMP_MEMORY_DESCRIPTOR 
{
    ULONGLONG                   StartOfMemoryRange;
    MINIDUMP_LOCATION_DESCRIPTOR Memory;
} MINIDUMP_MEMORY_DESCRIPTOR, *PMINIDUMP_MEMORY_DESCRIPTOR;

typedef struct _MINIDUMP_MODULE
{
    ULONGLONG                   BaseOfImage;
    ULONG                       SizeOfImage;
    ULONG                       CheckSum;
    ULONG                       TimeDateStamp;
    RVA                         ModuleNameRva;
    VS_FIXEDFILEINFO            VersionInfo;
    MINIDUMP_LOCATION_DESCRIPTOR CvRecord;
    MINIDUMP_LOCATION_DESCRIPTOR MiscRecord;
    ULONGLONG                   Reserved0;
    ULONGLONG                   Reserved1;
} MINIDUMP_MODULE, *PMINIDUMP_MODULE;

typedef struct _MINIDUMP_MODULE_LIST 
{
    ULONG                       NumberOfModules;
    MINIDUMP_MODULE             Modules[1]; /* FIXME: 0-sized array not supported */
} MINIDUMP_MODULE_LIST, *PMINIDUMP_MODULE_LIST;

typedef struct _MINIDUMP_USER_STREAM
{
    ULONG                       Type;
    ULONG                       BufferSize;
    void*                       Buffer;
} MINIDUMP_USER_STREAM, *PMINIDUMP_USER_STREAM;

typedef struct _MINIDUMP_USER_STREAM_INFORMATION
{
    ULONG                       UserStreamCount;
    PMINIDUMP_USER_STREAM       UserStreamArray;
} MINIDUMP_USER_STREAM_INFORMATION, *PMINIDUMP_USER_STREAM_INFORMATION;

typedef enum _MINIDUMP_STREAM_TYPE
{
    UnusedStream                = 0,
    ReservedStream0             = 1,
    ReservedStream1             = 2,
    ThreadListStream            = 3,
    ModuleListStream            = 4,
    MemoryListStream            = 5,
    ExceptionStream             = 6,
    SystemInfoStream            = 7,
    ThreadExListStream          = 8,
    Memory64ListStream          = 9,
    CommentStreamA              = 10,
    CommentStreamW              = 11,
    HandleDataStream            = 12,
    FunctionTableStream         = 13,

    LastReservedStream          = 0xffff
} MINIDUMP_STREAM_TYPE;

typedef struct _MINIDUMP_SYSTEM_INFO
{
    USHORT                      ProcessorArchitecture;
    USHORT                      ProcessorLevel;
    USHORT                      ProcessorRevision;
    USHORT                      Reserved0;

    ULONG                       MajorVersion;
    ULONG                       MinorVersion;
    ULONG                       BuildNumber;
    ULONG                       PlatformId;

    RVA                         CSDVersionRva;
    ULONG                       Reserved1;
    union _CPU_INFORMATION 
    {
        struct 
        {
            ULONG                       VendorId[3];
            ULONG                       VersionInformation;
            ULONG                       FeatureInformation;
            ULONG                       AMDExtendedCpuFeatures;
        } X86CpuInfo;
        struct 
        {
            ULONGLONG                   ProcessorFeatures[2];
        } OtherCpuInfo;
    } Cpu;

} MINIDUMP_SYSTEM_INFO, *PMINIDUMP_SYSTEM_INFO;

typedef struct _MINIDUMP_THREAD
{
    ULONG                       ThreadId;
    ULONG                       SuspendCount;
    ULONG                       PriorityClass;
    ULONG                       Priority;
    ULONGLONG                   Teb;
    MINIDUMP_MEMORY_DESCRIPTOR  Stack;
    MINIDUMP_LOCATION_DESCRIPTOR ThreadContext;
} MINIDUMP_THREAD, *PMINIDUMP_THREAD;

typedef struct _MINIDUMP_THREAD_LIST
{
    ULONG                       NumberOfThreads;
    MINIDUMP_THREAD             Threads[1]; /* FIXME: no support of 0 sized array */
} MINIDUMP_THREAD_LIST, *PMINIDUMP_THREAD_LIST;

BOOL WINAPI MiniDumpWriteDump(HANDLE,DWORD,HANDLE,MINIDUMP_TYPE,const PMINIDUMP_EXCEPTION_INFORMATION,
                              const PMINIDUMP_USER_STREAM_INFORMATION,const PMINIDUMP_CALLBACK_INFORMATION);
BOOL WINAPI MiniDumpReadDumpStream(PVOID,ULONG,PMINIDUMP_DIRECTORY*,PVOID*,ULONG*);


/*************************
 *    MODULE handling    *
 *************************/
typedef BOOL (CALLBACK *PENUMLOADED_MODULES_CALLBACK)(PSTR ModuleName, DWORD ModuleBase,
                                                      ULONG ModuleSize, PVOID UserContext);
BOOL   WINAPI EnumerateLoadedModules(HANDLE hProcess,
                                     PENUMLOADED_MODULES_CALLBACK EnumLoadedModulesCallback,
                                     PVOID UserContext);
typedef BOOL (CALLBACK *PSYM_ENUMMODULES_CALLBACK)(PSTR ModuleName, DWORD BaseOfDll,
                                                   PVOID UserContext);
BOOL    WINAPI SymEnumerateModules(HANDLE hProcess,
                                   PSYM_ENUMMODULES_CALLBACK EnumModulesCallback,
                                    PVOID UserContext);
BOOL    WINAPI SymGetModuleInfo(HANDLE hProcess, DWORD dwAddr, 
                                PIMAGEHLP_MODULE ModuleInfo);
BOOL    WINAPI SymGetModuleInfoW(HANDLE hProcess, DWORD dwAddr,
                                 PIMAGEHLP_MODULEW ModuleInfo);
DWORD   WINAPI SymGetModuleBase(HANDLE hProcess, DWORD dwAddr);
DWORD   WINAPI SymLoadModule(HANDLE hProcess, HANDLE hFile, PSTR ImageName,
                             PSTR ModuleName, DWORD BaseOfDll, DWORD SizeOfDll);
DWORD64 WINAPI SymLoadModuleEx(HANDLE hProcess, HANDLE hFile, PSTR ImageName,
                               PSTR ModuleName, DWORD64 BaseOfDll, DWORD DllSize,
                               PMODLOAD_DATA Data, DWORD Flags);
BOOL    WINAPI SymUnloadModule(HANDLE hProcess, DWORD BaseOfDll);

/*************************
 *    Symbol Handling    *
 *************************/

#define IMAGEHLP_SYMBOL_INFO_VALUEPRESENT          1
#define IMAGEHLP_SYMBOL_INFO_REGISTER              SYMF_REGISTER        /*  0x08 */
#define IMAGEHLP_SYMBOL_INFO_REGRELATIVE           SYMF_REGREL          /*  0x10 */
#define IMAGEHLP_SYMBOL_INFO_FRAMERELATIVE         SYMF_FRAMEREL        /*  0x20 */
#define IMAGEHLP_SYMBOL_INFO_PARAMETER             SYMF_PARAMETER       /*  0x40 */
#define IMAGEHLP_SYMBOL_INFO_LOCAL                 SYMF_LOCAL           /*  0x80 */
#define IMAGEHLP_SYMBOL_INFO_CONSTANT              SYMF_CONSTANT        /* 0x100 */
#define IMAGEHLP_SYMBOL_FUNCTION                   SYMF_FUNCTION        /* 0x800 */

#define SYMFLAG_VALUEPRESENT     0x00000001
#define SYMFLAG_REGISTER         0x00000008
#define SYMFLAG_REGREL           0x00000010
#define SYMFLAG_FRAMEREL         0x00000020
#define SYMFLAG_PARAMETER        0x00000040
#define SYMFLAG_LOCAL            0x00000080
#define SYMFLAG_CONSTANT         0x00000100
#define SYMFLAG_EXPORT           0x00000200
#define SYMFLAG_FORWARDER        0x00000400
#define SYMFLAG_FUNCTION         0x00000800
#define SYMFLAG_VIRTUAL          0x00001000
#define SYMFLAG_THUNK            0x00002000
#define SYMFLAG_TLSREL           0x00004000

typedef struct _SYMBOL_INFO
{
    ULONG       SizeOfStruct;
    ULONG       TypeIndex;
    ULONG64     Reserved[2];
    ULONG       info;   /* sdk states info, while MSDN says it's Index... */
    ULONG       Size;
    ULONG64     ModBase;
    ULONG       Flags;
    ULONG64     Value;
    ULONG64     Address;
    ULONG       Register;
    ULONG       Scope;
    ULONG       Tag;
    ULONG       NameLen;
    ULONG       MaxNameLen;
    CHAR        Name[1];
} SYMBOL_INFO, *PSYMBOL_INFO;

typedef enum _IMAGEHLP_SYMBOL_TYPE_INFO 
{
    TI_GET_SYMTAG,
    TI_GET_SYMNAME,
    TI_GET_LENGTH,
    TI_GET_TYPE,
    TI_GET_TYPEID,
    TI_GET_BASETYPE,
    TI_GET_ARRAYINDEXTYPEID,
    TI_FINDCHILDREN,
    TI_GET_DATAKIND,
    TI_GET_ADDRESSOFFSET,
    TI_GET_OFFSET,
    TI_GET_VALUE,
    TI_GET_COUNT,
    TI_GET_CHILDRENCOUNT,
    TI_GET_BITPOSITION,
    TI_GET_VIRTUALBASECLASS,
    TI_GET_VIRTUALTABLESHAPEID,
    TI_GET_VIRTUALBASEPOINTEROFFSET,
    TI_GET_CLASSPARENTID,
    TI_GET_NESTED,
    TI_GET_SYMINDEX,
    TI_GET_LEXICALPARENT,
    TI_GET_ADDRESS,
    TI_GET_THISADJUST,
    TI_GET_UDTKIND,
    TI_IS_EQUIV_TO,
    TI_GET_CALLING_CONVENTION,
} IMAGEHLP_SYMBOL_TYPE_INFO;

typedef struct _TI_FINDCHILDREN_PARAMS 
{
    ULONG Count;
    ULONG Start;
    ULONG ChildId[1];
} TI_FINDCHILDREN_PARAMS;

#define UNDNAME_COMPLETE                 (0x0000)
#define UNDNAME_NO_LEADING_UNDERSCORES   (0x0001)
#define UNDNAME_NO_MS_KEYWORDS           (0x0002)
#define UNDNAME_NO_FUNCTION_RETURNS      (0x0004)
#define UNDNAME_NO_ALLOCATION_MODEL      (0x0008)
#define UNDNAME_NO_ALLOCATION_LANGUAGE   (0x0010)
#define UNDNAME_NO_MS_THISTYPE           (0x0020)
#define UNDNAME_NO_CV_THISTYPE           (0x0040)
#define UNDNAME_NO_THISTYPE              (0x0060)
#define UNDNAME_NO_ACCESS_SPECIFIERS     (0x0080)
#define UNDNAME_NO_THROW_SIGNATURES      (0x0100)
#define UNDNAME_NO_MEMBER_TYPE           (0x0200)
#define UNDNAME_NO_RETURN_UDT_MODEL      (0x0400)
#define UNDNAME_32_BIT_DECODE            (0x0800)
#define UNDNAME_NAME_ONLY                (0x1000)
#define UNDNAME_NO_ARGUMENTS             (0x2000)
#define UNDNAME_NO_SPECIAL_SYMS          (0x4000)

BOOL WINAPI SymGetTypeInfo(HANDLE hProcess, DWORD64 ModBase, ULONG TypeId,
                           IMAGEHLP_SYMBOL_TYPE_INFO GetType, PVOID);
typedef BOOL (CALLBACK *PSYM_ENUMERATESYMBOLS_CALLBACK)(PSYMBOL_INFO pSymInfo,
                                                        ULONG SymbolSize, PVOID UserContext);
BOOL WINAPI SymEnumTypes(HANDLE hProcess, ULONG64 BaseOfDll,
                         PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
                         PVOID UserContext);
BOOL WINAPI SymFromAddr(HANDLE hProcess, DWORD64 addr, DWORD64* displacement, 
                        SYMBOL_INFO* sym_info);
BOOL WINAPI SymFromName(HANDLE hProcess, LPSTR Name, PSYMBOL_INFO Symbol);
BOOL WINAPI SymGetSymFromAddr(HANDLE,DWORD,PDWORD,PIMAGEHLP_SYMBOL);
BOOL WINAPI SymGetSymFromName(HANDLE,PSTR,PIMAGEHLP_SYMBOL);
BOOL WINAPI SymGetTypeFromName(HANDLE hProcess, ULONG64 BaseOfDll, LPSTR Name,
                               PSYMBOL_INFO Symbol);
BOOL WINAPI SymGetSymNext(HANDLE,PIMAGEHLP_SYMBOL);
BOOL WINAPI SymGetSymPrev(HANDLE,PIMAGEHLP_SYMBOL);
BOOL WINAPI SymEnumSymbols(HANDLE hProcess, ULONG64 BaseOfDll, PCSTR Mask,
                           PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
                           PVOID UserContext);
typedef BOOL (CALLBACK *PSYM_ENUMSYMBOLS_CALLBACK)(PSTR SymbolName, DWORD SymbolAddress,
                                                   ULONG SymbolSize, PVOID UserContext);
BOOL WINAPI SymEnumerateSymbols(HANDLE hProcess, DWORD BaseOfDll,
                                PSYM_ENUMSYMBOLS_CALLBACK EnumSymbolsCallback,
                                PVOID UserContext);
typedef BOOL (CALLBACK *PSYMBOL_REGISTERED_CALLBACK)(HANDLE hProcess, ULONG ActionCode,
                                                     PVOID CallbackData, PVOID UserContext);
BOOL WINAPI SymRegisterCallback(HANDLE hProcess,
                                PSYMBOL_REGISTERED_CALLBACK CallbackFunction,
                                PVOID UserContext);
BOOL WINAPI SymUnDName(PIMAGEHLP_SYMBOL,PSTR,DWORD);
DWORD WINAPI UnDecorateSymbolName(LPCSTR DecoratedName, LPSTR UnDecoratedName,
                                  DWORD UndecoratedLength, DWORD Flags);

/*************************
 *      Source Files     *
 *************************/
typedef BOOL (CALLBACK *PSYM_ENUMSOURCFILES_CALLBACK)(PSOURCEFILE pSourceFile,
                                                      PVOID UserContext);

BOOL WINAPI SymEnumSourceFiles(HANDLE hProcess, ULONG64 ModBase, LPSTR Mask,
                               PSYM_ENUMSOURCFILES_CALLBACK cbSrcFiles,
                               PVOID UserContext);
BOOL WINAPI SymGetLineFromAddr(HANDLE hProcess, DWORD dwAddr, 
                               PDWORD pdwDisplacement, PIMAGEHLP_LINE Line);
BOOL WINAPI SymGetLinePrev(HANDLE hProcess, PIMAGEHLP_LINE Line);
BOOL WINAPI SymGetLineNext(HANDLE hProcess, PIMAGEHLP_LINE Line);

/*************************
 * File & image handling *
 *************************/
BOOL WINAPI SymInitialize(HANDLE hProcess, PSTR UserSearchPath, BOOL fInvadeProcess);
BOOL WINAPI SymCleanup(HANDLE hProcess);

HANDLE WINAPI FindDebugInfoFile(PSTR FileName, PSTR SymbolPath, PSTR DebugFilePath);
typedef BOOL (CALLBACK *PFIND_DEBUG_FILE_CALLBACK)(HANDLE FileHandle, PSTR FileName,
                                                   PVOID CallerData);
HANDLE WINAPI FindDebugInfoFileEx(PSTR FileName, PSTR SymbolPath, PSTR DebugFilePath,
                                  PFIND_DEBUG_FILE_CALLBACK Callback, PVOID CallerData);
typedef BOOL (CALLBACK *PFINDFILEINPATHCALLBACK)(PSTR filename, PVOID context);

BOOL WINAPI SymFindFileInPath(HANDLE hProcess, LPSTR searchPath, LPSTR FileName,
                              PVOID id, DWORD two, DWORD three, DWORD flags,
                              LPSTR FilePath, PFINDFILEINPATHCALLBACK callback,
                              PVOID context);
HANDLE WINAPI FindExecutableImage(PSTR FileName, PSTR SymbolPath, PSTR ImageFilePath);
typedef BOOL (CALLBACK *PFIND_EXE_FILE_CALLBACK)(HANDLE FileHandle, PSTR FileName,
                                                 PVOID CallerData);
HANDLE WINAPI FindExecutableImageEx(PSTR FileName, PSTR SymbolPath, PSTR ImageFilePath,
                                    PFIND_EXE_FILE_CALLBACK Callback, PVOID CallerData);
PIMAGE_NT_HEADERS WINAPI ImageNtHeader(PVOID Base);

PVOID WINAPI ImageDirectoryEntryToDataEx(PVOID Base, BOOLEAN MappedAsImage,
                                         USHORT DirectoryEntry, PULONG Size,
                                         PIMAGE_SECTION_HEADER *FoundHeader);
PVOID WINAPI ImageDirectoryEntryToData(PVOID Base, BOOLEAN MappedAsImage,
                                       USHORT DirectoryEntry, PULONG Size);
PIMAGE_SECTION_HEADER WINAPI ImageRvaToSection(PIMAGE_NT_HEADERS NtHeaders,
                                               PVOID Base, ULONG Rva);
PVOID WINAPI ImageRvaToVa(PIMAGE_NT_HEADERS NtHeaders, PVOID Base,
                          ULONG Rva, OUT PIMAGE_SECTION_HEADER *LastRvaSection);
BOOL WINAPI SymGetSearchPath(HANDLE,PSTR,DWORD);
BOOL WINAPI SymSetSearchPath(HANDLE,PSTR);
DWORD WINAPI GetTimestampForLoadedLibrary(HMODULE);
BOOL WINAPI MakeSureDirectoryPathExists(PCSTR);
BOOL WINAPI SearchTreeForFile(PSTR,PSTR,PSTR);
typedef BOOL (CALLBACK *PENUMDIRTREE_CALLBACK)(LPCSTR path, PVOID user);
BOOL WINAPI EnumDirTree(HANDLE hProcess, PCSTR root, PCSTR file,
                        LPSTR buffer, PENUMDIRTREE_CALLBACK cb, void* user);
BOOL WINAPI SymMatchFileName(LPSTR file, LPSTR match, LPSTR* filestop, LPSTR* matchstop);

/*************************
 *   Context management  *
 *************************/
BOOL WINAPI SymSetContext(HANDLE hProcess, PIMAGEHLP_STACK_FRAME StackFrame,
                          PIMAGEHLP_CONTEXT Context);


/*************************
 *    Stack management   *
 *************************/

typedef struct _KDHELP
{
    DWORD       Thread;
    DWORD       ThCallbackStack;
    DWORD       NextCallback;
    DWORD       FramePointer;
    DWORD       KiCallUserMode;
    DWORD       KeUserCallbackDispatcher;
    DWORD       SystemRangeStart;
} KDHELP, *PKDHELP;

typedef struct _STACKFRAME
{
    ADDRESS     AddrPC;
    ADDRESS     AddrReturn;
    ADDRESS     AddrFrame;
    ADDRESS     AddrStack;
    PVOID       FuncTableEntry;
    DWORD       Params[4];
    BOOL        Far;
    BOOL        Virtual;
    DWORD       Reserved[3];
    KDHELP      KdHelp;
} STACKFRAME, *LPSTACKFRAME;

typedef BOOL (CALLBACK *PREAD_PROCESS_MEMORY_ROUTINE)
    (HANDLE  hProcess, LPCVOID lpBaseAddress, PVOID lpBuffer,
     DWORD nSize, PDWORD lpNumberOfBytesRead);

typedef PVOID (CALLBACK *PFUNCTION_TABLE_ACCESS_ROUTINE)
    (HANDLE hProcess, DWORD AddrBase);

typedef DWORD (CALLBACK *PGET_MODULE_BASE_ROUTINE)
    (HANDLE hProcess, DWORD ReturnAddress);

typedef DWORD (CALLBACK *PTRANSLATE_ADDRESS_ROUTINE)
    (HANDLE hProcess, HANDLE hThread, LPADDRESS lpaddr);

BOOL WINAPI StackWalk(DWORD MachineType, HANDLE hProcess, HANDLE hThread,
                      LPSTACKFRAME StackFrame, PVOID ContextRecord,
                      PREAD_PROCESS_MEMORY_ROUTINE ReadMemoryRoutine,
                      PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
                      PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine,
                      PTRANSLATE_ADDRESS_ROUTINE TranslateAddress);

PVOID WINAPI SymFunctionTableAccess(HANDLE hProcess, DWORD AddrBase);

/*************************
 * Version, global stuff *
 *************************/

typedef struct API_VERSION
{
    USHORT  MajorVersion;
    USHORT  MinorVersion;
    USHORT  Revision;
    USHORT  Reserved;
} API_VERSION, *LPAPI_VERSION;

LPAPI_VERSION WINAPI ImagehlpApiVersion(void);
LPAPI_VERSION WINAPI ImagehlpApiVersionEx(LPAPI_VERSION AppVersion);

typedef struct _IMAGE_DEBUG_INFORMATION
{
    LIST_ENTRY                  List;
    DWORD                       ReservedSize;
    PVOID                       ReservedMappedBase;
    USHORT                      ReservedMachine;
    USHORT                      ReservedCharacteristics;
    DWORD                       ReservedCheckSum;
    DWORD                       ImageBase;
    DWORD                       SizeOfImage;
    DWORD                       ReservedNumberOfSections;
    PIMAGE_SECTION_HEADER       ReservedSections;
    DWORD                       ReservedExportedNamesSize;
    PSTR                        ReservedExportedNames;
    DWORD                       ReservedNumberOfFunctionTableEntries;
    PIMAGE_FUNCTION_ENTRY       ReservedFunctionTableEntries;
    DWORD                       ReservedLowestFunctionStartingAddress;
    DWORD                       ReservedHighestFunctionEndingAddress;
    DWORD                       ReservedNumberOfFpoTableEntries;
    PFPO_DATA                   ReservedFpoTableEntries;
    DWORD                       SizeOfCoffSymbols;
    PIMAGE_COFF_SYMBOLS_HEADER  CoffSymbols;
    DWORD                       ReservedSizeOfCodeViewSymbols;
    PVOID                       ReservedCodeViewSymbols;
    PSTR                        ImageFilePath;
    PSTR                        ImageFileName;
    PSTR                        ReservedDebugFilePath;
    DWORD                       ReservedTimeDateStamp;
    BOOL                        ReservedRomImage;
    PIMAGE_DEBUG_DIRECTORY      ReservedDebugDirectory;
    DWORD                       ReservedNumberOfDebugDirectories;
    DWORD                       ReservedOriginalFunctionTableBaseAddress;
    DWORD                       Reserved[ 2 ];
} IMAGE_DEBUG_INFORMATION, *PIMAGE_DEBUG_INFORMATION;


PIMAGE_DEBUG_INFORMATION WINAPI MapDebugInformation(HANDLE FileHandle, PSTR FileName,
                                                    PSTR SymbolPath, DWORD ImageBase);

BOOL WINAPI UnmapDebugInformation(PIMAGE_DEBUG_INFORMATION DebugInfo);

DWORD   WINAPI  SymGetOptions(void);
DWORD   WINAPI  SymSetOptions(DWORD);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif  /* __WINE_DBGHELP_H */
