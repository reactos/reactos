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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_DBGHELP_H
#define __WINE_DBGHELP_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#define IMAGEAPI WINAPI
#define DBHLPAPI IMAGEAPI

typedef struct _LOADED_IMAGE
{
    PSTR                        ModuleName;
    HANDLE                      hFile;
    PUCHAR                      MappedAddress;
    PIMAGE_NT_HEADERS           FileHeader;
    PIMAGE_SECTION_HEADER       LastRvaSection;
    ULONG                       NumberOfSections;
    PIMAGE_SECTION_HEADER       Sections;
    ULONG                       Characteristics;
    BOOLEAN                     fSystemImage;
    BOOLEAN                     fDOSImage;
    BOOLEAN                     fReadOnly;
    UCHAR                       Version;
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

typedef struct _tagADDRESS64
{
    DWORD64                     Offset;
    WORD                        Segment;
    ADDRESS_MODE                Mode;
} ADDRESS64, *LPADDRESS64;

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
    SymVirtual,
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

typedef struct _IMAGEHLP_SYMBOL64
{
    DWORD                       SizeOfStruct;
    DWORD64                     Address;
    DWORD                       Size;
    DWORD                       Flags;
    DWORD                       MaxNameLength;
    CHAR                        Name[1];
} IMAGEHLP_SYMBOL64, *PIMAGEHLP_SYMBOL64;

typedef struct _IMAGEHLP_SYMBOLW64
{
    DWORD                       SizeOfStruct;
    DWORD64                     Address;
    DWORD                       Size;
    DWORD                       Flags;
    DWORD                       MaxNameLength;
    WCHAR                       Name[1];
} IMAGEHLP_SYMBOLW64, *PIMAGEHLP_SYMBOLW64;

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

typedef struct _IMAGEHLP_MODULEW
{
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

typedef struct _IMAGEHLP_MODULE64
{
    DWORD                       SizeOfStruct;
    DWORD64                     BaseOfImage;
    DWORD                       ImageSize;
    DWORD                       TimeDateStamp;
    DWORD                       CheckSum;
    DWORD                       NumSyms;
    SYM_TYPE                    SymType;
    CHAR                        ModuleName[32];
    CHAR                        ImageName[256];
    CHAR                        LoadedImageName[256];
    CHAR                        LoadedPdbName[256];
    DWORD                       CVSig;
    CHAR                        CVData[MAX_PATH*3];
    DWORD                       PdbSig;
    GUID                        PdbSig70;
    DWORD                       PdbAge;
    BOOL                        PdbUnmatched;
    BOOL                        DbgUnmatched;
    BOOL                        LineNumbers;
    BOOL                        GlobalSymbols;
    BOOL                        TypeInfo;
    BOOL                        SourceIndexed;
    BOOL                        Publics;
} IMAGEHLP_MODULE64, *PIMAGEHLP_MODULE64;

typedef struct _IMAGEHLP_MODULEW64
{
    DWORD                       SizeOfStruct;
    DWORD64                     BaseOfImage;
    DWORD                       ImageSize;
    DWORD                       TimeDateStamp;
    DWORD                       CheckSum;
    DWORD                       NumSyms;
    SYM_TYPE                    SymType;
    WCHAR                       ModuleName[32];
    WCHAR                       ImageName[256];
    WCHAR                       LoadedImageName[256];
    WCHAR                       LoadedPdbName[256];
    DWORD                       CVSig;
    WCHAR                       CVData[MAX_PATH*3];
    DWORD                       PdbSig;
    GUID                        PdbSig70;
    DWORD                       PdbAge;
    BOOL                        PdbUnmatched;
    BOOL                        DbgUnmatched;
    BOOL                        LineNumbers;
    BOOL                        GlobalSymbols;
    BOOL                        TypeInfo;
    BOOL                        SourceIndexed;
    BOOL                        Publics;
} IMAGEHLP_MODULEW64, *PIMAGEHLP_MODULEW64;

typedef struct _IMAGEHLP_LINE
{
    DWORD                       SizeOfStruct;
    PVOID                       Key;
    DWORD                       LineNumber;
    PCHAR                       FileName;
    DWORD                       Address;
} IMAGEHLP_LINE, *PIMAGEHLP_LINE;

typedef struct _IMAGEHLP_LINEW
{
    DWORD                       SizeOfStruct;
    PVOID                       Key;
    DWORD                       LineNumber;
    PWSTR                       FileName;
    DWORD                       Address;
} IMAGEHLP_LINEW, *PIMAGEHLP_LINEW;

typedef struct _IMAGEHLP_LINE64
{
    DWORD                       SizeOfStruct;
    PVOID                       Key;
    DWORD                       LineNumber;
    PCHAR                       FileName;
    DWORD64                     Address;
} IMAGEHLP_LINE64, *PIMAGEHLP_LINE64;

typedef struct _IMAGEHLP_LINEW64
{
    DWORD                       SizeOfStruct;
    PVOID                       Key;
    DWORD                       LineNumber;
    PWSTR                       FileName;
    DWORD64                     Address;
} IMAGEHLP_LINEW64, *PIMAGEHLP_LINEW64;

typedef struct _SOURCEFILE
{
    DWORD64                     ModBase;
    PCHAR                       FileName;
} SOURCEFILE, *PSOURCEFILE;

typedef struct _SOURCEFILEW
{
    DWORD64                     ModBase;
    PWSTR                       FileName;
} SOURCEFILEW, *PSOURCEFILEW;

#define CBA_DEFERRED_SYMBOL_LOAD_START          0x00000001
#define CBA_DEFERRED_SYMBOL_LOAD_COMPLETE       0x00000002
#define CBA_DEFERRED_SYMBOL_LOAD_FAILURE        0x00000003
#define CBA_SYMBOLS_UNLOADED                    0x00000004
#define CBA_DUPLICATE_SYMBOL                    0x00000005
#define CBA_READ_MEMORY                         0x00000006
#define CBA_DEFERRED_SYMBOL_LOAD_CANCEL         0x00000007
#define CBA_SET_OPTIONS                         0x00000008
#define CBA_EVENT                               0x00000010
#define CBA_DEFERRED_SYMBOL_LOAD_PARTIAL        0x00000020
#define CBA_DEBUG_INFO                          0x10000000

typedef struct _IMAGEHLP_CBA_READ_MEMORY
{
    DWORD64   addr;
    PVOID     buf;
    DWORD     bytes;
    DWORD    *bytesread;
} IMAGEHLP_CBA_READ_MEMORY, *PIMAGEHLP_CBA_READ_MEMORY;

enum
{
    sevInfo = 0,
    sevProblem,
    sevAttn,
    sevFatal,
    sevMax
};

#define EVENT_SRCSPEW_START 100
#define EVENT_SRCSPEW       100
#define EVENT_SRCSPEW_END   199

typedef struct _IMAGEHLP_CBA_EVENT
{
    DWORD       severity;
    DWORD       code;
    PCHAR       desc;
    PVOID       object;
} IMAGEHLP_CBA_EVENT, *PIMAGEHLP_CBA_EVENT;

typedef struct _IMAGEHLP_CBA_EVENTW
{
    DWORD       severity;
    DWORD       code;
    PCWSTR      desc;
    PVOID       object;
} IMAGEHLP_CBA_EVENTW, *PIMAGEHLP_CBA_EVENTW;

typedef struct _IMAGEHLP_DEFERRED_SYMBOL_LOAD
{
    DWORD                       SizeOfStruct;
    DWORD                       BaseOfImage;
    DWORD                       CheckSum;
    DWORD                       TimeDateStamp;
    CHAR                        FileName[MAX_PATH];
    BOOLEAN                     Reparse;
    HANDLE                      hFile;
} IMAGEHLP_DEFERRED_SYMBOL_LOAD, *PIMAGEHLP_DEFERRED_SYMBOL_LOAD;

typedef struct _IMAGEHLP_DEFERRED_SYMBOL_LOAD64
{
    DWORD                       SizeOfStruct;
    DWORD64                     BaseOfImage;
    DWORD                       CheckSum;
    DWORD                       TimeDateStamp;
    CHAR                        FileName[MAX_PATH];
    BOOLEAN                     Reparse;
    HANDLE                      hFile;
    DWORD                       Flags;
} IMAGEHLP_DEFERRED_SYMBOL_LOAD64, *PIMAGEHLP_DEFERRED_SYMBOL_LOAD64;

typedef struct _IMAGEHLP_DEFERRED_SYMBOL_LOADW64
{
    DWORD                       SizeOfStruct;
    DWORD64                     BaseOfImage;
    DWORD                       CheckSum;
    DWORD                       TimeDateStamp;
    WCHAR                       FileName[MAX_PATH + 1];
    BOOLEAN                     Reparse;
    HANDLE                      hFile;
    DWORD                       Flags;
} IMAGEHLP_DEFERRED_SYMBOL_LOADW64, *PIMAGEHLP_DEFERRED_SYMBOL_LOADW64;

typedef struct _IMAGEHLP_DUPLICATE_SYMBOL
{
    DWORD                       SizeOfStruct;
    DWORD                       NumberOfDups;
    PIMAGEHLP_SYMBOL            Symbol;
    DWORD                       SelectedSymbol;
} IMAGEHLP_DUPLICATE_SYMBOL, *PIMAGEHLP_DUPLICATE_SYMBOL;

typedef struct _IMAGEHLP_DUPLICATE_SYMBOL64
{
    DWORD                       SizeOfStruct;
    DWORD                       NumberOfDups;
    PIMAGEHLP_SYMBOL64          Symbol;
    DWORD                       SelectedSymbol;
} IMAGEHLP_DUPLICATE_SYMBOL64, *PIMAGEHLP_DUPLICATE_SYMBOL64;

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
/* latest SDK defines: */
#define SYMOPT_ALLOW_ABSOLUTE_SYMBOLS   0x00000800
#define SYMOPT_IGNORE_NT_SYMPATH        0x00001000

#define SYMOPT_INCLUDE_32BIT_MODULES    0x00002000
#define SYMOPT_PUBLICS_ONLY             0x00004000
#define SYMOPT_NO_PUBLICS               0x00008000
#define SYMOPT_AUTO_PUBLICS             0x00010000
#define SYMOPT_NO_IMAGE_SEARCH          0x00020000
#define SYMOPT_SECURE                   0x00040000
#define SYMOPT_NO_PROMPTS               0x00080000
#define SYMOPT_OVERWRITE                0x00100000
#define SYMOPT_IGNORE_IMAGEDIR          0x00200000

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

#include <pshpack4.h>
/* DebugHelp */

#define MINIDUMP_SIGNATURE 0x504D444D /* 'MDMP' */
#define MINIDUMP_VERSION   (42899)

typedef DWORD   RVA;
typedef ULONG64 RVA64;

typedef enum _MINIDUMP_TYPE
{
    MiniDumpNormal                              = 0x0000,
    MiniDumpWithDataSegs                        = 0x0001,
    MiniDumpWithFullMemory                      = 0x0002,
    MiniDumpWithHandleData                      = 0x0004,
    MiniDumpFilterMemory                        = 0x0008,
    MiniDumpScanMemory                          = 0x0010,
    MiniDumpWithUnloadedModules                 = 0x0020,
    MiniDumpWithIndirectlyReferencedMemory      = 0x0040,
    MiniDumpFilterModulePaths                   = 0x0080,
    MiniDumpWithProcessThreadData               = 0x0100,
    MiniDumpWithPrivateReadWriteMemory          = 0x0200,
    MiniDumpWithoutOptionalData                 = 0x0400,
    MiniDumpWithFullMemoryInfo                  = 0x0800,
    MiniDumpWithThreadInfo                      = 0x1000,
    MiniDumpWithCodeSegs                        = 0x2000
} MINIDUMP_TYPE;

typedef enum _MINIDUMP_CALLBACK_TYPE
{
    ModuleCallback,
    ThreadCallback,
    ThreadExCallback,
    IncludeThreadCallback,
    IncludeModuleCallback,
    MemoryCallback,
} MINIDUMP_CALLBACK_TYPE;

typedef struct _MINIDUMP_THREAD_CALLBACK
{
    ULONG                       ThreadId;
    HANDLE                      ThreadHandle;
    CONTEXT                     Context;
    ULONG                       SizeOfContext;
    ULONG64                     StackBase;
    ULONG64                     StackEnd;
} MINIDUMP_THREAD_CALLBACK, *PMINIDUMP_THREAD_CALLBACK;

typedef struct _MINIDUMP_THREAD_EX_CALLBACK
{
    ULONG                       ThreadId;
    HANDLE                      ThreadHandle;
    CONTEXT                     Context;
    ULONG                       SizeOfContext;
    ULONG64                     StackBase;
    ULONG64                     StackEnd;
    ULONG64                     BackingStoreBase;
    ULONG64                     BackingStoreEnd;
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
    ThreadWriteInstructionWindow = 0x0010,
    ThreadWriteThreadData        = 0x0020,
    ThreadWriteThreadInfo        = 0x0040
} THREAD_WRITE_FLAGS;

typedef struct _MINIDUMP_MODULE_CALLBACK
{
    PWCHAR                      FullPath;
    ULONG64                     BaseOfImage;
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
    ModuleReferencedByMemory = 0x0010,
    ModuleWriteTlsData       = 0x0020,
    ModuleWriteCodeSegs      = 0x0040,
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
    } DUMMYUNIONNAME;
} MINIDUMP_CALLBACK_INPUT, *PMINIDUMP_CALLBACK_INPUT;

typedef struct _MINIDUMP_CALLBACK_OUTPUT
{
    union
    {
        ULONG                           ModuleWriteFlags;
        ULONG                           ThreadWriteFlags;
        struct
        {
            ULONG64                     MemoryBase;
            ULONG                       MemorySize;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} MINIDUMP_CALLBACK_OUTPUT, *PMINIDUMP_CALLBACK_OUTPUT;

typedef BOOL (WINAPI* MINIDUMP_CALLBACK_ROUTINE)(PVOID, const PMINIDUMP_CALLBACK_INPUT, PMINIDUMP_CALLBACK_OUTPUT);

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

typedef struct _MINIDUMP_LOCATION_DESCRIPTOR64
{
    ULONG64                     DataSize;
    RVA64                       Rva;
} MINIDUMP_LOCATION_DESCRIPTOR64;

typedef struct _MINIDUMP_DIRECTORY 
{
    ULONG                       StreamType;
    MINIDUMP_LOCATION_DESCRIPTOR Location;
} MINIDUMP_DIRECTORY, *PMINIDUMP_DIRECTORY;

typedef struct _MINIDUMP_EXCEPTION
{
    ULONG                       ExceptionCode;
    ULONG                       ExceptionFlags;
    ULONG64                     ExceptionRecord;
    ULONG64                     ExceptionAddress;
    ULONG                       NumberParameters;
    ULONG                        __unusedAlignment;
    ULONG64                     ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
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
    } DUMMYUNIONNAME;
    ULONG64                     Flags;
} MINIDUMP_HEADER, *PMINIDUMP_HEADER;

typedef struct _MINIDUMP_MEMORY_DESCRIPTOR 
{
    ULONG64                     StartOfMemoryRange;
    MINIDUMP_LOCATION_DESCRIPTOR Memory;
} MINIDUMP_MEMORY_DESCRIPTOR, *PMINIDUMP_MEMORY_DESCRIPTOR;

typedef struct _MINIDUMP_MEMORY_LIST
{
    ULONG                       NumberOfMemoryRanges;
    MINIDUMP_MEMORY_DESCRIPTOR  MemoryRanges[1]; /* FIXME: 0-sized array not supported */
} MINIDUMP_MEMORY_LIST, *PMINIDUMP_MEMORY_LIST;

#define MINIDUMP_MISC1_PROCESS_ID       0x00000001
#define MINIDUMP_MISC1_PROCESS_TIMES    0x00000002

typedef struct _MINIDUMP_MISC_INFO
{
    ULONG                       SizeOfInfo;
    ULONG                       Flags1;
    ULONG                       ProcessId;
    ULONG                       ProcessCreateTime;
    ULONG                       ProcessUserTime;
    ULONG                       ProcessKernelTime;
} MINIDUMP_MISC_INFO, *PMINIDUMP_MISC_INFO;

typedef struct _MINIDUMP_MODULE
{
    ULONG64                     BaseOfImage;
    ULONG                       SizeOfImage;
    ULONG                       CheckSum;
    ULONG                       TimeDateStamp;
    RVA                         ModuleNameRva;
    VS_FIXEDFILEINFO            VersionInfo;
    MINIDUMP_LOCATION_DESCRIPTOR CvRecord;
    MINIDUMP_LOCATION_DESCRIPTOR MiscRecord;
    ULONG64                     Reserved0;
    ULONG64                     Reserved1;
} MINIDUMP_MODULE, *PMINIDUMP_MODULE;

typedef struct _MINIDUMP_MODULE_LIST
{
    ULONG                       NumberOfModules;
    MINIDUMP_MODULE             Modules[1]; /* FIXME: 0-sized array not supported */
} MINIDUMP_MODULE_LIST, *PMINIDUMP_MODULE_LIST;

typedef struct _MINIDUMP_STRING
{
    ULONG                       Length;
    WCHAR                       Buffer[1]; /* FIXME: O-sized array not supported */
} MINIDUMP_STRING, *PMINIDUMP_STRING;

typedef struct _MINIDUMP_SYSTEM_INFO
{
    USHORT                      ProcessorArchitecture;
    USHORT                      ProcessorLevel;
    USHORT                      ProcessorRevision;
    union
    {
        USHORT                          Reserved0;
        struct
        {
            UCHAR                       NumberOfProcessors;
            UCHAR                       ProductType;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;

    ULONG                       MajorVersion;
    ULONG                       MinorVersion;
    ULONG                       BuildNumber;
    ULONG                       PlatformId;

    RVA                         CSDVersionRva;
    union
    {
        ULONG                           Reserved1;
        struct
        {
            USHORT                      SuiteMask;
            USHORT                      Reserved2;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME1;
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
            ULONG64                     ProcessorFeatures[2];
        } OtherCpuInfo;
    } Cpu;

} MINIDUMP_SYSTEM_INFO, *PMINIDUMP_SYSTEM_INFO;

typedef struct _MINIDUMP_THREAD
{
    ULONG                       ThreadId;
    ULONG                       SuspendCount;
    ULONG                       PriorityClass;
    ULONG                       Priority;
    ULONG64                     Teb;
    MINIDUMP_MEMORY_DESCRIPTOR  Stack;
    MINIDUMP_LOCATION_DESCRIPTOR ThreadContext;
} MINIDUMP_THREAD, *PMINIDUMP_THREAD;

typedef struct _MINIDUMP_THREAD_LIST
{
    ULONG                       NumberOfThreads;
    MINIDUMP_THREAD             Threads[1]; /* FIXME: no support of 0 sized array */
} MINIDUMP_THREAD_LIST, *PMINIDUMP_THREAD_LIST;

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
    UnloadedModuleListStream    = 14,
    MiscInfoStream              = 15,
    MemoryInfoListStream        = 16,
    ThreadInfoListStream        = 17,

    LastReservedStream          = 0xffff
} MINIDUMP_STREAM_TYPE;

BOOL WINAPI MiniDumpWriteDump(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE,
                              const PMINIDUMP_EXCEPTION_INFORMATION,
                              const PMINIDUMP_USER_STREAM_INFORMATION,
                              const PMINIDUMP_CALLBACK_INFORMATION);
BOOL WINAPI MiniDumpReadDumpStream(PVOID, ULONG, PMINIDUMP_DIRECTORY*, PVOID*,
                                   ULONG*);

#include <poppack.h>

/*************************
 *    MODULE handling    *
 *************************/

/* flags for SymLoadModuleEx */
#define SLMFLAG_VIRTUAL         0x1
#define SLMFLAG_NO_SYMBOLS      0x4

typedef BOOL (CALLBACK *PENUMLOADED_MODULES_CALLBACK)(PCSTR, ULONG, ULONG, PVOID);
BOOL   WINAPI EnumerateLoadedModules(HANDLE, PENUMLOADED_MODULES_CALLBACK, PVOID);
typedef BOOL (CALLBACK *PENUMLOADED_MODULES_CALLBACK64)(PCSTR, DWORD64, ULONG, PVOID);
BOOL   WINAPI EnumerateLoadedModules64(HANDLE, PENUMLOADED_MODULES_CALLBACK64, PVOID);
typedef BOOL (CALLBACK *PENUMLOADED_MODULES_CALLBACKW64)(PCWSTR, DWORD64, ULONG, PVOID);
BOOL   WINAPI EnumerateLoadedModulesW64(HANDLE, PENUMLOADED_MODULES_CALLBACKW64, PVOID);
typedef BOOL (CALLBACK *PSYM_ENUMMODULES_CALLBACK)(PCSTR, ULONG, PVOID);
BOOL    WINAPI SymEnumerateModules(HANDLE, PSYM_ENUMMODULES_CALLBACK, PVOID);
typedef BOOL (CALLBACK *PSYM_ENUMMODULES_CALLBACK64)(PCSTR, DWORD64, PVOID);
BOOL    WINAPI SymEnumerateModules64(HANDLE, PSYM_ENUMMODULES_CALLBACK64, PVOID);
typedef BOOL (CALLBACK *PSYM_ENUMMODULES_CALLBACKW64)(PCWSTR, DWORD64, PVOID);
BOOL    WINAPI SymEnumerateModulesW64(HANDLE, PSYM_ENUMMODULES_CALLBACKW64, PVOID);
BOOL    WINAPI SymGetModuleInfo(HANDLE, DWORD, PIMAGEHLP_MODULE);
BOOL    WINAPI SymGetModuleInfoW(HANDLE, DWORD, PIMAGEHLP_MODULEW);
BOOL    WINAPI SymGetModuleInfo64(HANDLE, DWORD64, PIMAGEHLP_MODULE64);
BOOL    WINAPI SymGetModuleInfoW64(HANDLE, DWORD64, PIMAGEHLP_MODULEW64);
DWORD   WINAPI SymGetModuleBase(HANDLE, DWORD);
DWORD64 WINAPI SymGetModuleBase64(HANDLE, DWORD64);
DWORD   WINAPI SymLoadModule(HANDLE, HANDLE, PCSTR, PCSTR, DWORD, DWORD);
DWORD64 WINAPI SymLoadModule64(HANDLE, HANDLE, PCSTR, PCSTR, DWORD64, DWORD);
DWORD64 WINAPI SymLoadModuleEx(HANDLE, HANDLE, PCSTR, PCSTR, DWORD64, DWORD,
                               PMODLOAD_DATA, DWORD);
DWORD64 WINAPI SymLoadModuleExW(HANDLE, HANDLE, PCWSTR, PCWSTR, DWORD64, DWORD,
                                PMODLOAD_DATA, DWORD);
BOOL    WINAPI SymUnloadModule(HANDLE, DWORD);
BOOL    WINAPI SymUnloadModule64(HANDLE, DWORD64);

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
#define SYMFLAG_SLOT             0x00008000

#define MAX_SYM_NAME    2000

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

typedef struct _SYMBOL_INFOW
{
    ULONG       SizeOfStruct;
    ULONG       TypeIndex;
    ULONG64     Reserved[2];
    ULONG       Index;
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
    WCHAR       Name[1];
} SYMBOL_INFOW, *PSYMBOL_INFOW;

typedef struct _SYMBOL_INFO_PACKAGE
{
    SYMBOL_INFO si;
    CHAR        name[MAX_SYM_NAME+1];
} SYMBOL_INFO_PACKAGE, *PSYMBOL_INFO_PACKAGE;

typedef struct _SYMBOL_INFO_PACKAGEW
{
    SYMBOL_INFOW si;
    WCHAR        name[MAX_SYM_NAME+1];
} SYMBOL_INFO_PACKAGEW, *PSYMBOL_INFO_PACKAGEW;

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

#define IMAGEHLP_GET_TYPE_INFO_UNCACHED            0x00000001
#define IMAGEHLP_GET_TYPE_INFO_CHILDREN            0x00000002
typedef struct _IMAGEHLP_GET_TYPE_INFO_PARAMS
{
    ULONG       SizeOfStruct;
    ULONG       Flags;
    ULONG       NumIds;
    PULONG      TypeIds;
    ULONG64     TagFilter;
    ULONG       NumReqs;
    IMAGEHLP_SYMBOL_TYPE_INFO* ReqKinds;
    PULONG_PTR  ReqOffsets;
    PULONG      ReqSizes;
    ULONG_PTR   ReqStride;
    ULONG_PTR   BufferSize;
    PVOID       Buffer;
    ULONG       EntriesMatched;
    ULONG       EntriesFilled;
    ULONG64     TagsFound;
    ULONG64     AllReqsValid;
    ULONG       NumReqsValid;
    PULONG64    ReqsValid;
} IMAGEHLP_GET_TYPE_INFO_PARAMS, *PIMAGEHLP_GET_TYPE_INFO_PARAMS;

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

#define SYMSEARCH_MASKOBJS              0x01
#define SYMSEARCH_RECURSE               0x02
#define SYMSEARCH_GLOBALSONLY           0x04

BOOL WINAPI SymGetTypeInfo(HANDLE, DWORD64, ULONG, IMAGEHLP_SYMBOL_TYPE_INFO, PVOID);
BOOL WINAPI SymGetTypeInfoEx(HANDLE, DWORD64, PIMAGEHLP_GET_TYPE_INFO_PARAMS);
typedef BOOL (CALLBACK *PSYM_ENUMERATESYMBOLS_CALLBACK)(PSYMBOL_INFO, ULONG, PVOID);
typedef BOOL (CALLBACK *PSYM_ENUMERATESYMBOLS_CALLBACKW)(PSYMBOL_INFOW, ULONG, PVOID);
BOOL WINAPI SymEnumTypes(HANDLE, ULONG64, PSYM_ENUMERATESYMBOLS_CALLBACK, PVOID);
BOOL WINAPI SymEnumTypesW(HANDLE, ULONG64, PSYM_ENUMERATESYMBOLS_CALLBACKW, PVOID);
BOOL WINAPI SymFromAddr(HANDLE, DWORD64, DWORD64*, SYMBOL_INFO*);
BOOL WINAPI SymFromAddrW(HANDLE, DWORD64, DWORD64*, SYMBOL_INFOW*);
BOOL WINAPI SymFromToken(HANDLE, DWORD64, DWORD, PSYMBOL_INFO);
BOOL WINAPI SymFromTokenW(HANDLE, DWORD64, DWORD, PSYMBOL_INFOW);
BOOL WINAPI SymFromName(HANDLE, PCSTR, PSYMBOL_INFO);
BOOL WINAPI SymFromNameW(HANDLE, PCWSTR, PSYMBOL_INFOW);
BOOL WINAPI SymGetSymFromAddr(HANDLE, DWORD, PDWORD, PIMAGEHLP_SYMBOL);
BOOL WINAPI SymGetSymFromAddr64(HANDLE, DWORD64, PDWORD64, PIMAGEHLP_SYMBOL64);
BOOL WINAPI SymGetSymFromName(HANDLE, PCSTR, PIMAGEHLP_SYMBOL);
BOOL WINAPI SymGetSymFromName64(HANDLE, PCSTR, PIMAGEHLP_SYMBOL64);
BOOL WINAPI SymGetTypeFromName(HANDLE, ULONG64, PCSTR, PSYMBOL_INFO);
BOOL WINAPI SymGetTypeFromNameW(HANDLE, ULONG64, PCWSTR, PSYMBOL_INFOW);
BOOL WINAPI SymGetSymNext(HANDLE, PIMAGEHLP_SYMBOL);
BOOL WINAPI SymGetSymNext64(HANDLE, PIMAGEHLP_SYMBOL64);
BOOL WINAPI SymGetSymNextW64(HANDLE, PIMAGEHLP_SYMBOLW64);
BOOL WINAPI SymGetSymPrev(HANDLE, PIMAGEHLP_SYMBOL);
BOOL WINAPI SymGetSymPrev64(HANDLE, PIMAGEHLP_SYMBOL64);
BOOL WINAPI SymGetSymPrevW64(HANDLE, PIMAGEHLP_SYMBOLW64);
BOOL WINAPI SymEnumSym(HANDLE,ULONG64,PSYM_ENUMERATESYMBOLS_CALLBACK,PVOID);
BOOL WINAPI SymEnumSymbols(HANDLE, ULONG64, PCSTR, PSYM_ENUMERATESYMBOLS_CALLBACK,
                           PVOID);
BOOL WINAPI SymEnumSymbolsW(HANDLE, ULONG64, PCWSTR, PSYM_ENUMERATESYMBOLS_CALLBACKW,
                            PVOID);
typedef BOOL (CALLBACK *PSYM_ENUMSYMBOLS_CALLBACK)(PCSTR, ULONG, ULONG, PVOID);
typedef BOOL (CALLBACK *PSYM_ENUMSYMBOLS_CALLBACKW)(PCWSTR, ULONG, ULONG, PVOID);
BOOL WINAPI SymEnumerateSymbols(HANDLE, ULONG, PSYM_ENUMSYMBOLS_CALLBACK, PVOID);
BOOL WINAPI SymEnumerateSymbolsW(HANDLE, ULONG, PSYM_ENUMSYMBOLS_CALLBACKW, PVOID);
typedef BOOL (CALLBACK *PSYM_ENUMSYMBOLS_CALLBACK64)(PCSTR, DWORD64, ULONG, PVOID);
typedef BOOL (CALLBACK *PSYM_ENUMSYMBOLS_CALLBACK64W)(PCWSTR, DWORD64, ULONG, PVOID);
BOOL WINAPI SymEnumerateSymbols64(HANDLE, ULONG64, PSYM_ENUMSYMBOLS_CALLBACK64, PVOID);
BOOL WINAPI SymEnumerateSymbolsW64(HANDLE, ULONG64, PSYM_ENUMSYMBOLS_CALLBACK64W, PVOID);
BOOL WINAPI SymEnumSymbolsForAddr(HANDLE, DWORD64, PSYM_ENUMERATESYMBOLS_CALLBACK, PVOID);
BOOL WINAPI SymEnumSymbolsForAddrW(HANDLE, DWORD64, PSYM_ENUMERATESYMBOLS_CALLBACKW, PVOID);
typedef BOOL (CALLBACK *PSYMBOL_REGISTERED_CALLBACK)(HANDLE, ULONG, PVOID, PVOID);
BOOL WINAPI SymRegisterCallback(HANDLE, PSYMBOL_REGISTERED_CALLBACK, PVOID);
typedef BOOL (CALLBACK *PSYMBOL_REGISTERED_CALLBACK64)(HANDLE, ULONG, ULONG64, ULONG64);
BOOL WINAPI SymRegisterCallback64(HANDLE, PSYMBOL_REGISTERED_CALLBACK64, ULONG64);
BOOL WINAPI SymRegisterCallbackW64(HANDLE, PSYMBOL_REGISTERED_CALLBACK64, ULONG64);
BOOL WINAPI SymUnDName(PIMAGEHLP_SYMBOL, PSTR, DWORD);
BOOL WINAPI SymUnDName64(PIMAGEHLP_SYMBOL64, PSTR, DWORD);
BOOL WINAPI SymMatchString(PCSTR, PCSTR, BOOL);
BOOL WINAPI SymMatchStringA(PCSTR, PCSTR, BOOL);
BOOL WINAPI SymMatchStringW(PCWSTR, PCWSTR, BOOL);
BOOL WINAPI SymSearch(HANDLE, ULONG64, DWORD, DWORD, PCSTR, DWORD64, PSYM_ENUMERATESYMBOLS_CALLBACK, PVOID, DWORD);
BOOL WINAPI SymSearchW(HANDLE, ULONG64, DWORD, DWORD, PCWSTR, DWORD64, PSYM_ENUMERATESYMBOLS_CALLBACKW, PVOID, DWORD);
DWORD WINAPI UnDecorateSymbolName(PCSTR, PSTR, DWORD, DWORD);
DWORD WINAPI UnDecorateSymbolNameW(PCWSTR, PWSTR, DWORD, DWORD);
BOOL WINAPI SymGetScope(HANDLE, ULONG64, DWORD, PSYMBOL_INFO);
BOOL WINAPI SymGetScopeW(HANDLE, ULONG64, DWORD, PSYMBOL_INFOW);
BOOL WINAPI SymFromIndex(HANDLE, ULONG64, DWORD, PSYMBOL_INFO);
BOOL WINAPI SymFromIndexW(HANDLE, ULONG64, DWORD, PSYMBOL_INFOW);
BOOL WINAPI SymAddSymbol(HANDLE, ULONG64, PCSTR, DWORD64, DWORD, DWORD);
BOOL WINAPI SymAddSymbolW(HANDLE, ULONG64, PCWSTR, DWORD64, DWORD, DWORD);
BOOL WINAPI SymDeleteSymbol(HANDLE, ULONG64, PCSTR, DWORD64, DWORD);
BOOL WINAPI SymDeleteSymbolW(HANDLE, ULONG64, PCWSTR, DWORD64, DWORD);

/*************************
 *      Source Files     *
 *************************/
typedef BOOL (CALLBACK *PSYM_ENUMSOURCEFILES_CALLBACK)(PSOURCEFILE, PVOID);
typedef BOOL (CALLBACK *PSYM_ENUMSOURCEFILES_CALLBACKW)(PSOURCEFILEW, PVOID);

BOOL WINAPI SymEnumSourceFiles(HANDLE, ULONG64, PCSTR, PSYM_ENUMSOURCEFILES_CALLBACK,
                               PVOID);
BOOL WINAPI SymEnumSourceFilesW(HANDLE, ULONG64, PCWSTR, PSYM_ENUMSOURCEFILES_CALLBACKW, PVOID);
BOOL WINAPI SymGetLineFromAddr(HANDLE, DWORD, PDWORD, PIMAGEHLP_LINE);
BOOL WINAPI SymGetLineFromAddrW(HANDLE, DWORD, PDWORD, PIMAGEHLP_LINEW);
BOOL WINAPI SymGetLineFromAddr64(HANDLE, DWORD64, PDWORD, PIMAGEHLP_LINE64);
BOOL WINAPI SymGetLineFromAddrW64(HANDLE, DWORD64, PDWORD, PIMAGEHLP_LINEW64);
BOOL WINAPI SymGetLinePrev(HANDLE, PIMAGEHLP_LINE);
BOOL WINAPI SymGetLinePrevW(HANDLE, PIMAGEHLP_LINEW);
BOOL WINAPI SymGetLinePrev64(HANDLE, PIMAGEHLP_LINE64);
BOOL WINAPI SymGetLinePrevW64(HANDLE, PIMAGEHLP_LINEW64);
BOOL WINAPI SymGetLineNext(HANDLE, PIMAGEHLP_LINE);
BOOL WINAPI SymGetLineNextW(HANDLE, PIMAGEHLP_LINEW);
BOOL WINAPI SymGetLineNext64(HANDLE, PIMAGEHLP_LINE64);
BOOL WINAPI SymGetLineNextW64(HANDLE, PIMAGEHLP_LINEW64);
BOOL WINAPI SymGetLineFromName(HANDLE, PCSTR, PCSTR, DWORD, PLONG, PIMAGEHLP_LINE);
BOOL WINAPI SymGetLineFromName64(HANDLE, PCSTR, PCSTR, DWORD, PLONG, PIMAGEHLP_LINE64);
BOOL WINAPI SymGetLineFromNameW64(HANDLE, PCWSTR, PCWSTR, DWORD, PLONG, PIMAGEHLP_LINEW64);
ULONG WINAPI SymGetFileLineOffsets64(HANDLE, PCSTR, PCSTR, PDWORD64, ULONG);
BOOL WINAPI SymGetSourceFile(HANDLE, ULONG64, PCSTR, PCSTR, PSTR, DWORD);
BOOL WINAPI SymGetSourceFileW(HANDLE, ULONG64, PCWSTR, PCWSTR, PWSTR, DWORD);
BOOL WINAPI SymGetSourceFileToken(HANDLE, ULONG64, PCSTR, PVOID*, DWORD*);
BOOL WINAPI SymGetSourceFileTokenW(HANDLE, ULONG64, PCWSTR, PVOID*, DWORD*);
BOOL WINAPI SymGetSourceFileFromToken(HANDLE, PVOID, PCSTR, PSTR, DWORD);
BOOL WINAPI SymGetSourceFileFromTokenW(HANDLE, PVOID, PCWSTR, PWSTR, DWORD);
BOOL WINAPI SymGetSourceVarFromToken(HANDLE, PVOID, PCSTR, PCSTR, PSTR, DWORD);
BOOL WINAPI SymGetSourceVarFromTokenW(HANDLE, PVOID, PCWSTR, PCWSTR, PWSTR, DWORD);

typedef struct _SRCCODEINFO
{
    DWORD       SizeOfStruct;
    PVOID       Key;
    DWORD64     ModBase;
    CHAR        Obj[MAX_PATH+1];
    CHAR        FileName[MAX_PATH+1];
    DWORD       LineNumber;
    DWORD64     Address;
} SRCCODEINFO, *PSRCCODEINFO;

typedef struct _SRCCODEINFOW
{
    DWORD       SizeOfStruct;
    PVOID       Key;
    DWORD64     ModBase;
    WCHAR       Obj[MAX_PATH+1];
    WCHAR       FileName[MAX_PATH+1];
    DWORD       LineNumber;
    DWORD64     Address;
} SRCCODEINFOW, *PSRCCODEINFOW;

typedef BOOL (CALLBACK* PSYM_ENUMLINES_CALLBACK)(PSRCCODEINFO, PVOID);
typedef BOOL (CALLBACK* PSYM_ENUMLINES_CALLBACKW)(PSRCCODEINFOW, PVOID);
BOOL WINAPI SymEnumLines(HANDLE, ULONG64, PCSTR, PCSTR, PSYM_ENUMLINES_CALLBACK, PVOID);
BOOL WINAPI SymEnumLinesW(HANDLE, ULONG64, PCWSTR, PCWSTR, PSYM_ENUMLINES_CALLBACKW, PVOID);
BOOL WINAPI SymEnumSourceLines(HANDLE, ULONG64, PCSTR, PCSTR, DWORD, DWORD, PSYM_ENUMLINES_CALLBACK, PVOID);
BOOL WINAPI SymEnumSourceLinesW(HANDLE, ULONG64, PCWSTR, PCWSTR, DWORD, DWORD, PSYM_ENUMLINES_CALLBACKW, PVOID);

/*************************
 * File & image handling *
 *************************/
BOOL WINAPI SymInitialize(HANDLE, PCSTR, BOOL);
BOOL WINAPI SymInitializeW(HANDLE, PCWSTR, BOOL);
BOOL WINAPI SymCleanup(HANDLE);

HANDLE WINAPI FindDebugInfoFile(PCSTR, PCSTR, PSTR);
typedef BOOL (CALLBACK *PFIND_DEBUG_FILE_CALLBACK)(HANDLE, PCSTR, PVOID);
typedef BOOL (CALLBACK *PFIND_DEBUG_FILE_CALLBACKW)(HANDLE, PCWSTR, PVOID);
HANDLE WINAPI FindDebugInfoFileEx(PCSTR, PCSTR, PSTR, PFIND_DEBUG_FILE_CALLBACK, PVOID);
HANDLE WINAPI FindDebugInfoFileExW(PCWSTR, PCWSTR, PWSTR, PFIND_DEBUG_FILE_CALLBACKW, PVOID);
HANDLE WINAPI SymFindDebugInfoFile(HANDLE, PCSTR, PSTR, PFIND_DEBUG_FILE_CALLBACK, PVOID);
HANDLE WINAPI SymFindDebugInfoFileW(HANDLE, PCWSTR, PWSTR, PFIND_DEBUG_FILE_CALLBACKW, PVOID);
typedef BOOL (CALLBACK *PFINDFILEINPATHCALLBACK)(PCSTR, PVOID);
typedef BOOL (CALLBACK *PFINDFILEINPATHCALLBACKW)(PCWSTR, PVOID);
BOOL WINAPI FindFileInPath(HANDLE, LPSTR, LPSTR, PVOID, DWORD, DWORD, DWORD, LPSTR);
BOOL WINAPI SymFindFileInPath(HANDLE, PCSTR, PCSTR, PVOID, DWORD, DWORD, DWORD,
                              PSTR, PFINDFILEINPATHCALLBACK, PVOID);
BOOL WINAPI SymFindFileInPathW(HANDLE, PCWSTR, PCWSTR, PVOID, DWORD, DWORD, DWORD,
                              PWSTR, PFINDFILEINPATHCALLBACKW, PVOID);
HANDLE WINAPI FindExecutableImage(PCSTR, PCSTR, PSTR);
typedef BOOL (CALLBACK *PFIND_EXE_FILE_CALLBACK)(HANDLE, PCSTR, PVOID);
typedef BOOL (CALLBACK *PFIND_EXE_FILE_CALLBACKW)(HANDLE, PCWSTR, PVOID);
HANDLE WINAPI FindExecutableImageEx(PCSTR, PCSTR, PSTR, PFIND_EXE_FILE_CALLBACK, PVOID);
HANDLE WINAPI FindExecutableImageExW(PCWSTR, PCWSTR, PWSTR, PFIND_EXE_FILE_CALLBACKW, PVOID);
HANDLE WINAPI SymFindExecutableImage(HANDLE, PCSTR, PSTR, PFIND_EXE_FILE_CALLBACK, PVOID);
HANDLE WINAPI SymFindExecutableImageW(HANDLE, PCWSTR, PWSTR, PFIND_EXE_FILE_CALLBACKW, PVOID);
PIMAGE_NT_HEADERS WINAPI ImageNtHeader(PVOID);
PVOID WINAPI ImageDirectoryEntryToDataEx(PVOID, BOOLEAN, USHORT, PULONG,
                                         PIMAGE_SECTION_HEADER *);
PVOID WINAPI ImageDirectoryEntryToData(PVOID, BOOLEAN, USHORT, PULONG);
PIMAGE_SECTION_HEADER WINAPI ImageRvaToSection(PIMAGE_NT_HEADERS, PVOID, ULONG);
PVOID WINAPI ImageRvaToVa(PIMAGE_NT_HEADERS, PVOID, ULONG, PIMAGE_SECTION_HEADER*);
BOOL WINAPI SymGetSearchPath(HANDLE, PSTR, DWORD);
BOOL WINAPI SymGetSearchPathW(HANDLE, PWSTR, DWORD);
BOOL WINAPI SymSetSearchPath(HANDLE, PCSTR);
BOOL WINAPI SymSetSearchPathW(HANDLE, PCWSTR);
DWORD WINAPI GetTimestampForLoadedLibrary(HMODULE);
BOOL WINAPI MakeSureDirectoryPathExists(PCSTR);
BOOL WINAPI SearchTreeForFile(PCSTR, PCSTR, PSTR);
BOOL WINAPI SearchTreeForFileW(PCWSTR, PCWSTR, PWSTR);
typedef BOOL (CALLBACK *PENUMDIRTREE_CALLBACK)(PCSTR, PVOID);
typedef BOOL (CALLBACK *PENUMDIRTREE_CALLBACKW)(PCWSTR, PVOID);
BOOL WINAPI EnumDirTree(HANDLE, PCSTR, PCSTR, PSTR, PENUMDIRTREE_CALLBACK, PVOID);
BOOL WINAPI EnumDirTreeW(HANDLE, PCWSTR, PCWSTR, PWSTR, PENUMDIRTREE_CALLBACKW, PVOID);
BOOL WINAPI SymMatchFileName(PCSTR, PCSTR, PSTR*, PSTR*);
BOOL WINAPI SymMatchFileNameW(PCWSTR, PCWSTR, PWSTR*, PWSTR*);
PCHAR WINAPI SymSetHomeDirectory(HANDLE, PCSTR);
PWSTR WINAPI SymSetHomeDirectoryW(HANDLE, PCWSTR);
PCHAR WINAPI SymGetHomeDirectory(DWORD, PSTR, size_t);
PWSTR WINAPI SymGetHomeDirectoryW(DWORD, PWSTR, size_t);
#define hdBase  0
#define hdSym   1
#define hdSrc   2
#define hdMax   3

/*************************
 *   Context management  *
 *************************/
BOOL WINAPI SymSetContext(HANDLE, PIMAGEHLP_STACK_FRAME, PIMAGEHLP_CONTEXT);


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

typedef struct _KDHELP64
{
    DWORD64     Thread;
    DWORD       ThCallbackStack;
    DWORD       ThCallbackBStore;
    DWORD       NextCallback;
    DWORD       FramePointer;
    DWORD64     KiCallUserMode;
    DWORD64     KeUserCallbackDispatcher;
    DWORD64     SystemRangeStart;
    DWORD64     Reserved[8];
} KDHELP64, *PKDHELP64;

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
    ADDRESS     AddrBStore;
} STACKFRAME, *LPSTACKFRAME;

typedef struct _STACKFRAME64
{
    ADDRESS64   AddrPC;
    ADDRESS64   AddrReturn;
    ADDRESS64   AddrFrame;
    ADDRESS64   AddrStack;
    ADDRESS64   AddrBStore;
    PVOID       FuncTableEntry;
    DWORD64     Params[4];
    BOOL        Far;
    BOOL        Virtual;
    DWORD64     Reserved[3];
    KDHELP64    KdHelp;
} STACKFRAME64, *LPSTACKFRAME64;

typedef BOOL (CALLBACK *PREAD_PROCESS_MEMORY_ROUTINE)
    (HANDLE, DWORD, PVOID, DWORD, PDWORD);
typedef PVOID (CALLBACK *PFUNCTION_TABLE_ACCESS_ROUTINE)(HANDLE, DWORD);
typedef DWORD (CALLBACK *PGET_MODULE_BASE_ROUTINE)(HANDLE, DWORD);
typedef DWORD (CALLBACK *PTRANSLATE_ADDRESS_ROUTINE)(HANDLE, HANDLE, LPADDRESS);
BOOL WINAPI StackWalk(DWORD, HANDLE, HANDLE, LPSTACKFRAME, PVOID,
                      PREAD_PROCESS_MEMORY_ROUTINE,
                      PFUNCTION_TABLE_ACCESS_ROUTINE,
                      PGET_MODULE_BASE_ROUTINE,
                      PTRANSLATE_ADDRESS_ROUTINE);

typedef BOOL (CALLBACK *PREAD_PROCESS_MEMORY_ROUTINE64)
    (HANDLE, DWORD64, PVOID, DWORD, PDWORD);
typedef PVOID (CALLBACK *PFUNCTION_TABLE_ACCESS_ROUTINE64)(HANDLE, DWORD64);
typedef DWORD64 (CALLBACK *PGET_MODULE_BASE_ROUTINE64)(HANDLE, DWORD64);
typedef DWORD64 (CALLBACK *PTRANSLATE_ADDRESS_ROUTINE64)(HANDLE, HANDLE, LPADDRESS64);
BOOL WINAPI StackWalk64(DWORD, HANDLE, HANDLE, LPSTACKFRAME64, PVOID,
                        PREAD_PROCESS_MEMORY_ROUTINE64,
                        PFUNCTION_TABLE_ACCESS_ROUTINE64,
                        PGET_MODULE_BASE_ROUTINE64,
                        PTRANSLATE_ADDRESS_ROUTINE64);

PVOID WINAPI SymFunctionTableAccess(HANDLE, DWORD);
PVOID WINAPI SymFunctionTableAccess64(HANDLE, DWORD64);

typedef PVOID (CALLBACK *PSYMBOL_FUNCENTRY_CALLBACK)(HANDLE, DWORD, PVOID);
typedef PVOID (CALLBACK *PSYMBOL_FUNCENTRY_CALLBACK64)(HANDLE, ULONG64, ULONG64);

BOOL WINAPI SymRegisterFunctionEntryCallback(HANDLE, PSYMBOL_FUNCENTRY_CALLBACK, PVOID);
BOOL WINAPI SymRegisterFunctionEntryCallback64(HANDLE, PSYMBOL_FUNCENTRY_CALLBACK64, ULONG64);

/*************************
 * Version, global stuff *
 *************************/

#define API_VERSION_NUMBER 9

typedef struct API_VERSION
{
    USHORT  MajorVersion;
    USHORT  MinorVersion;
    USHORT  Revision;
    USHORT  Reserved;
} API_VERSION, *LPAPI_VERSION;

LPAPI_VERSION WINAPI ImagehlpApiVersion(void);
LPAPI_VERSION WINAPI ImagehlpApiVersionEx(LPAPI_VERSION);

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


PIMAGE_DEBUG_INFORMATION WINAPI MapDebugInformation(HANDLE, PCSTR, PCSTR, ULONG);

BOOL WINAPI UnmapDebugInformation(PIMAGE_DEBUG_INFORMATION);

DWORD   WINAPI  SymGetOptions(void);
DWORD   WINAPI  SymSetOptions(DWORD);

BOOL WINAPI SymSetParentWindow(HWND);

/*************************
 * Version, global stuff *
 *************************/

typedef BOOL     (WINAPI* PSYMBOLSERVERPROC)(PCSTR, PCSTR, PVOID, DWORD, DWORD, PSTR);
typedef BOOL     (WINAPI* PSYMBOLSERVERPROCA)(PCSTR, PCSTR, PVOID, DWORD, DWORD, PSTR);
typedef BOOL     (WINAPI* PSYMBOLSERVERPROCW)(PCWSTR, PCWSTR, PVOID, DWORD, DWORD, PWSTR);
typedef BOOL     (WINAPI* PSYMBOLSERVEROPENPROC)(void);
typedef BOOL     (WINAPI* PSYMBOLSERVERCLOSEPROC)(void);
typedef BOOL     (WINAPI* PSYMBOLSERVERSETOPTIONSPROC)(UINT_PTR, ULONG64);
typedef BOOL     (CALLBACK* PSYMBOLSERVERCALLBACKPROC)(UINT_PTR, ULONG64, ULONG64);
typedef UINT_PTR (WINAPI* PSYMBOLSERVERGETOPTIONSPROC)(void);
typedef BOOL     (WINAPI* PSYMBOLSERVERPINGPROC)(PCSTR);
typedef BOOL     (WINAPI* PSYMBOLSERVERPINGPROCA)(PCSTR);
typedef BOOL     (WINAPI* PSYMBOLSERVERPINGPROCW)(PCWSTR);

#define SSRVOPT_CALLBACK            0x0001
#define SSRVOPT_DWORD               0x0002
#define SSRVOPT_DWORDPTR            0x0004
#define SSRVOPT_GUIDPTR             0x0008
#define SSRVOPT_OLDGUIDPTR          0x0010
#define SSRVOPT_UNATTENDED          0x0020
#define SSRVOPT_NOCOPY              0x0040
#define SSRVOPT_PARENTWIN           0x0080
#define SSRVOPT_PARAMTYPE           0x0100
#define SSRVOPT_SECURE              0x0200
#define SSRVOPT_TRACE               0x0400
#define SSRVOPT_SETCONTEXT          0x0800
#define SSRVOPT_PROXY               0x1000
#define SSRVOPT_DOWNSTREAM_STORE    0x2000
#define SSRVOPT_RESET               ((ULONG_PTR)-1)

#define SSRVACTION_TRACE        1
#define SSRVACTION_QUERYCANCEL  2
#define SSRVACTION_EVENT        3

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif  /* __WINE_DBGHELP_H */
