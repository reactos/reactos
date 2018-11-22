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

/* A set of documentation macros (see also imagehlp.h) */
#ifndef __deref_out
# define __deref_out
#endif
#ifndef __deref_out_opt
# define __deref_out_opt
#endif
#ifndef __deref_opt_out
# define __deref_opt_out
#endif
#ifndef __in
# define __in
#endif
#ifndef __in_opt
# define __in_opt
#endif
#ifndef __in_bcount
# define __in_bcount(x)
#endif
#ifndef __in_bcount_opt
# define __in_bcount_opt(x)
#endif
#ifndef __in_ecount
# define __in_ecount(x)
#endif
#ifndef __inout
# define __inout
#endif
#ifndef __inout_opt
# define __inout_opt
#endif
#ifndef __inout_bcount
# define __inout_bcount(x)
#endif
#ifndef __inout_ecount
# define __inout_ecount(x)
#endif
#ifndef __out
# define __out
#endif
#ifndef __out_opt
# define __out_opt
#endif
#ifndef __out_bcount
# define __out_bcount(x)
#endif
#ifndef __out_bcount_opt
# define __out_bcount_opt(x)
#endif
#ifndef __out_ecount
# define __out_ecount(x)
#endif
#ifndef __out_ecount_opt
# define __out_ecount_opt(x)
#endif
#ifndef __out_xcount
# define __out_xcount(x)
#endif


#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#ifdef _WIN64
#ifndef _IMAGEHLP64
#define _IMAGEHLP64
#endif
#endif

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

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define ADDRESS ADDRESS64
#define LPADDRESS LPADDRESS64
#else
typedef struct _tagADDRESS
{
    DWORD                       Offset;
    WORD                        Segment;
    ADDRESS_MODE                Mode;
} ADDRESS, *LPADDRESS;
#endif

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

#ifdef _NO_CVCONST_H
enum SymTagEnum
{
    SymTagNull,
    SymTagExe,
    SymTagCompiland,
    SymTagCompilandDetails,
    SymTagCompilandEnv,
    SymTagFunction,
    SymTagBlock,
    SymTagData,
    SymTagAnnotation,
    SymTagLabel,
    SymTagPublicSymbol,
    SymTagUDT,
    SymTagEnum,
    SymTagFunctionType,
    SymTagPointerType,
    SymTagArrayType,
    SymTagBaseType,
    SymTagTypedef,
    SymTagBaseClass,
    SymTagFriend,
    SymTagFunctionArgType,
    SymTagFuncDebugStart,
    SymTagFuncDebugEnd,
    SymTagUsingNamespace,
    SymTagVTableShape,
    SymTagVTable,
    SymTagCustom,
    SymTagThunk,
    SymTagCustomType,
    SymTagManagedType,
    SymTagDimension,
    SymTagCallSite,
    SymTagInlineSite,
    SymTagBaseInterface,
    SymTagVectorType,
    SymTagMatrixType,
    SymTagHLSLType,
    SymTagCaller,
    SymTagCallee,
    SymTagExport,
    SymTagHeapAllocationSite,
    SymTagCoffGroup,
    SymTagMax
};
#endif // _NO_CVCONST_H

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define IMAGEHLP_SYMBOL IMAGEHLP_SYMBOL64
#define IMAGEHLP_SYMBOLW IMAGEHLP_SYMBOLW64
#define PIMAGEHLP_SYMBOL PIMAGEHLP_SYMBOL64
#define PIMAGEHLP_SYMBOLW PIMAGEHLP_SYMBOLW64
#else
typedef struct _IMAGEHLP_SYMBOL
{
    DWORD                       SizeOfStruct;
    DWORD                       Address;
    DWORD                       Size;
    DWORD                       Flags;
    DWORD                       MaxNameLength;
    CHAR                        Name[1];
} IMAGEHLP_SYMBOL, *PIMAGEHLP_SYMBOL;

typedef struct _IMAGEHLP_SYMBOLW
{
    DWORD                       SizeOfStruct;
    DWORD                       Address;
    DWORD                       Size;
    DWORD                       Flags;
    DWORD                       MaxNameLength;
    WCHAR                       Name[1];
} IMAGEHLP_SYMBOLW, *PIMAGEHLP_SYMBOLW;
#endif

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

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define IMAGEHLP_MODULE IMAGEHLP_MODULE64
#define PIMAGEHLP_MODULE PIMAGEHLP_MODULE64
#define IMAGEHLP_MODULEW IMAGEHLP_MODULEW64
#define PIMAGEHLP_MODULEW PIMAGEHLP_MODULEW64
#else
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
#endif

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

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define IMAGEHLP_LINE IMAGEHLP_LINE64
#define PIMAGEHLP_LINE PIMAGEHLP_LINE64
#define IMAGEHLP_LINEW IMAGEHLP_LINEW64
#define PIMAGEHLP_LINEW PIMAGEHLP_LINEW64
#else
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
#endif

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

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define IMAGEHLP_DEFERRED_SYMBOL_LOAD IMAGEHLP_DEFERRED_SYMBOL_LOAD64
#define PIMAGEHLP_DEFERRED_SYMBOL_LOAD PIMAGEHLP_DEFERRED_SYMBOL_LOAD64
#else
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
#endif

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

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define IMAGEHLP_DUPLICATE_SYMBOL IMAGEHLP_DUPLICATE_SYMBOL64
#define PIMAGEHLP_DUPLICATE_SYMBOL PIMAGEHLP_DUPLICATE_SYMBOL64
#else
typedef struct _IMAGEHLP_DUPLICATE_SYMBOL
{
    DWORD                       SizeOfStruct;
    DWORD                       NumberOfDups;
    PIMAGEHLP_SYMBOL            Symbol;
    DWORD                       SelectedSymbol;
} IMAGEHLP_DUPLICATE_SYMBOL, *PIMAGEHLP_DUPLICATE_SYMBOL;
#endif

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
#define SYMOPT_FLAT_DIRECTORY           0x00400000
#define SYMOPT_FAVOR_COMPRESSED         0x00800000
#define SYMOPT_ALLOW_ZERO_ADDRESS       0x01000000
#define SYMOPT_DISABLE_SYMSRV_AUTODETECT 0x02000000
#define SYMOPT_READONLY_CACHE           0x04000000
#define SYMOPT_SYMPATH_LAST             0x08000000
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

typedef BOOL
(WINAPI* MINIDUMP_CALLBACK_ROUTINE)(
  _Inout_ PVOID,
  _In_ const PMINIDUMP_CALLBACK_INPUT,
  _Inout_ PMINIDUMP_CALLBACK_OUTPUT);

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

BOOL
WINAPI
MiniDumpWriteDump(
  _In_ HANDLE,
  _In_ DWORD,
  _In_ HANDLE,
  _In_ MINIDUMP_TYPE,
  _In_opt_ PMINIDUMP_EXCEPTION_INFORMATION,
  _In_opt_ PMINIDUMP_USER_STREAM_INFORMATION,
  _In_opt_ PMINIDUMP_CALLBACK_INFORMATION);

BOOL
WINAPI
MiniDumpReadDumpStream(
  _In_ PVOID,
  _In_ ULONG,
  _Outptr_result_maybenull_ PMINIDUMP_DIRECTORY*,
  _Outptr_result_maybenull_ PVOID*,
  _Out_opt_ ULONG*);

#include <poppack.h>

/*************************
 *    MODULE handling    *
 *************************/

/* flags for SymLoadModuleEx */
#define SLMFLAG_VIRTUAL         0x1
#define SLMFLAG_NO_SYMBOLS      0x4

typedef BOOL
(CALLBACK *PENUMLOADED_MODULES_CALLBACK64)(
  _In_ PCSTR,
  _In_ DWORD64,
  _In_ ULONG,
  _In_opt_ PVOID);

BOOL
WINAPI
EnumerateLoadedModules64(
  _In_ HANDLE,
  _In_ PENUMLOADED_MODULES_CALLBACK64,
  _In_opt_ PVOID);

typedef BOOL
(CALLBACK *PENUMLOADED_MODULES_CALLBACKW64)(
  _In_ PCWSTR,
  _In_ DWORD64,
  _In_ ULONG,
  _In_opt_ PVOID);

BOOL
WINAPI
EnumerateLoadedModulesW64(
  _In_ HANDLE,
  _In_ PENUMLOADED_MODULES_CALLBACKW64,
  _In_opt_ PVOID);

typedef BOOL
(CALLBACK *PSYM_ENUMMODULES_CALLBACK64)(
  _In_ PCSTR,
  _In_ DWORD64,
  _In_opt_ PVOID);

BOOL
WINAPI
SymEnumerateModules64(
  _In_ HANDLE,
  _In_ PSYM_ENUMMODULES_CALLBACK64,
  _In_opt_ PVOID);

typedef BOOL
(CALLBACK *PSYM_ENUMMODULES_CALLBACKW64)(
  _In_ PCWSTR,
  _In_ DWORD64,
  _In_opt_ PVOID);

BOOL
WINAPI
SymEnumerateModulesW64(
  _In_ HANDLE,
  _In_ PSYM_ENUMMODULES_CALLBACKW64,
  _In_opt_ PVOID);

BOOL
WINAPI
SymGetModuleInfo64(
  _In_ HANDLE,
  _In_ DWORD64,
  _Out_ PIMAGEHLP_MODULE64);

BOOL
WINAPI
SymGetModuleInfoW64(
  _In_ HANDLE,
  _In_ DWORD64,
  _Out_ PIMAGEHLP_MODULEW64);

DWORD64 WINAPI SymGetModuleBase64(_In_ HANDLE, _In_ DWORD64);

DWORD64
WINAPI
SymLoadModule64(
  _In_ HANDLE,
  _In_opt_ HANDLE,
  _In_opt_ PCSTR,
  _In_opt_ PCSTR,
  _In_ DWORD64,
  _In_ DWORD);

DWORD64
WINAPI
SymLoadModuleEx(
  _In_ HANDLE,
  _In_opt_ HANDLE,
  _In_opt_ PCSTR,
  _In_opt_ PCSTR,
  _In_ DWORD64,
  _In_ DWORD,
  _In_opt_ PMODLOAD_DATA,
  _In_opt_ DWORD);

DWORD64
WINAPI
SymLoadModuleExW(
  _In_ HANDLE,
  _In_opt_ HANDLE,
  _In_opt_ PCWSTR,
  _In_opt_ PCWSTR,
  _In_ DWORD64,
  _In_ DWORD,
  _In_opt_ PMODLOAD_DATA,
  _In_opt_ DWORD);

BOOL WINAPI SymUnloadModule64(_In_ HANDLE, _In_ DWORD64);

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
#define SYMFLAG_ILREL            0x00010000
#define SYMFLAG_METADATA         0x00020000
#define SYMFLAG_CLR_TOKEN        0x00040000
#define SYMFLAG_NULL             0x00080000
#define SYMFLAG_FUNC_NO_RETURN   0x00100000
#define SYMFLAG_SYNTHETIC_ZEROBASE 0x00200000
#define SYMFLAG_PUBLIC_CODE      0x00400000

#define MAX_SYM_NAME    2000

typedef struct _SYMBOL_INFO
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

BOOL
WINAPI
SymGetTypeInfo(
  _In_ HANDLE,
  _In_ DWORD64,
  _In_ ULONG,
  _In_ IMAGEHLP_SYMBOL_TYPE_INFO,
  _Out_ PVOID);

BOOL
WINAPI
SymGetTypeInfoEx(
  _In_ HANDLE,
  _In_ DWORD64,
  _Inout_ PIMAGEHLP_GET_TYPE_INFO_PARAMS);

typedef BOOL
(CALLBACK *PSYM_ENUMERATESYMBOLS_CALLBACK)(
  _In_ PSYMBOL_INFO,
  _In_ ULONG,
  _In_opt_ PVOID);

typedef BOOL
(CALLBACK *PSYM_ENUMERATESYMBOLS_CALLBACKW)(
  _In_ PSYMBOL_INFOW,
  _In_ ULONG,
  _In_opt_ PVOID);

BOOL
WINAPI
SymEnumTypes(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_ PSYM_ENUMERATESYMBOLS_CALLBACK,
  _In_opt_ PVOID);

BOOL
WINAPI
SymEnumTypesW(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_ PSYM_ENUMERATESYMBOLS_CALLBACKW,
  _In_opt_ PVOID);

BOOL
WINAPI
SymFromAddr(
  _In_ HANDLE,
  _In_ DWORD64,
  _Out_opt_ DWORD64*,
  _Inout_ SYMBOL_INFO*);

BOOL
WINAPI
SymFromAddrW(
  _In_ HANDLE,
  _In_ DWORD64,
  _Out_opt_ DWORD64*,
  _Inout_ SYMBOL_INFOW*);

BOOL
WINAPI
SymFromToken(
  _In_ HANDLE,
  _In_ DWORD64,
  _In_ DWORD,
  _Inout_ PSYMBOL_INFO);

BOOL
WINAPI
SymFromTokenW(
  _In_ HANDLE,
  _In_ DWORD64,
  _In_ DWORD,
  _Inout_ PSYMBOL_INFOW);

BOOL WINAPI SymFromName(_In_ HANDLE, _In_ PCSTR, _Inout_ PSYMBOL_INFO);
BOOL WINAPI SymFromNameW(_In_ HANDLE, _In_ PCWSTR, _Inout_ PSYMBOL_INFOW);

BOOL
WINAPI
SymGetSymFromAddr64(
  _In_ HANDLE,
  _In_ DWORD64,
  _Out_opt_ PDWORD64,
  _Inout_ PIMAGEHLP_SYMBOL64);

BOOL
WINAPI
SymGetSymFromName64(
  _In_ HANDLE,
  _In_ PCSTR,
  _Inout_ PIMAGEHLP_SYMBOL64);

BOOL
WINAPI
SymGetTypeFromName(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_ PCSTR,
  _Inout_ PSYMBOL_INFO);

BOOL
WINAPI
SymGetTypeFromNameW(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_ PCWSTR,
  _Inout_ PSYMBOL_INFOW);

BOOL WINAPI SymGetSymNext64(_In_ HANDLE, _Inout_ PIMAGEHLP_SYMBOL64);
BOOL WINAPI SymGetSymNextW64(_In_ HANDLE, _Inout_ PIMAGEHLP_SYMBOLW64);
BOOL WINAPI SymGetSymPrev64(_In_ HANDLE, _Inout_ PIMAGEHLP_SYMBOL64);
BOOL WINAPI SymGetSymPrevW64(_In_ HANDLE, _Inout_ PIMAGEHLP_SYMBOLW64);

BOOL
WINAPI
SymEnumSym(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_ PSYM_ENUMERATESYMBOLS_CALLBACK,
  _In_opt_ PVOID);

BOOL
WINAPI
SymEnumSymbols(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_opt_ PCSTR,
  _In_ PSYM_ENUMERATESYMBOLS_CALLBACK,
  _In_opt_ PVOID);

BOOL
WINAPI
SymEnumSymbolsW(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_opt_ PCWSTR,
  _In_ PSYM_ENUMERATESYMBOLS_CALLBACKW,
  _In_opt_ PVOID);

typedef BOOL
(CALLBACK *PSYM_ENUMSYMBOLS_CALLBACK64)(
  _In_ PCSTR,
  _In_ DWORD64,
  _In_ ULONG,
  _In_opt_ PVOID);

typedef BOOL
(CALLBACK *PSYM_ENUMSYMBOLS_CALLBACK64W)(
  _In_ PCWSTR,
  _In_ DWORD64,
  _In_ ULONG,
  _In_opt_ PVOID);

BOOL
WINAPI
SymEnumerateSymbols64(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_ PSYM_ENUMSYMBOLS_CALLBACK64,
  _In_opt_ PVOID);

BOOL
WINAPI
SymEnumerateSymbolsW64(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_ PSYM_ENUMSYMBOLS_CALLBACK64W,
  _In_opt_ PVOID);

BOOL
WINAPI
SymEnumSymbolsForAddr(
  _In_ HANDLE,
  _In_ DWORD64,
  _In_ PSYM_ENUMERATESYMBOLS_CALLBACK,
  _In_opt_ PVOID);

BOOL
WINAPI
SymEnumSymbolsForAddrW(
  _In_ HANDLE,
  _In_ DWORD64,
  _In_ PSYM_ENUMERATESYMBOLS_CALLBACKW,
  _In_opt_ PVOID);

typedef BOOL
(CALLBACK *PSYMBOL_REGISTERED_CALLBACK64)(
  _In_ HANDLE,
  _In_ ULONG,
  _In_opt_ ULONG64,
  _In_opt_ ULONG64);

BOOL
WINAPI
SymRegisterCallback64(
  _In_ HANDLE,
  _In_ PSYMBOL_REGISTERED_CALLBACK64,
  _In_ ULONG64);

BOOL
WINAPI
SymRegisterCallbackW64(
  _In_ HANDLE,
  _In_ PSYMBOL_REGISTERED_CALLBACK64,
  _In_ ULONG64);

BOOL
WINAPI
SymUnDName64(
  _In_ PIMAGEHLP_SYMBOL64,
  _Out_writes_(UnDecNameLength) PSTR,
  _In_ DWORD UnDecNameLength);

BOOL WINAPI SymMatchString(_In_ PCSTR, _In_ PCSTR, _In_ BOOL);
BOOL WINAPI SymMatchStringA(_In_ PCSTR, _In_ PCSTR, _In_ BOOL);
BOOL WINAPI SymMatchStringW(_In_ PCWSTR, _In_ PCWSTR, _In_ BOOL);

BOOL
WINAPI
SymSearch(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_opt_ DWORD,
  _In_opt_ DWORD,
  _In_opt_ PCSTR,
  _In_opt_ DWORD64,
  _In_ PSYM_ENUMERATESYMBOLS_CALLBACK,
  _In_opt_ PVOID,
  _In_ DWORD);

BOOL
WINAPI
SymSearchW(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_opt_ DWORD,
  _In_opt_ DWORD,
  _In_opt_ PCWSTR,
  _In_opt_ DWORD64,
  _In_ PSYM_ENUMERATESYMBOLS_CALLBACKW,
  _In_opt_ PVOID,
  _In_ DWORD);

DWORD
WINAPI
UnDecorateSymbolName(
  _In_ PCSTR,
  _Out_writes_(maxStringLength) PSTR,
  _In_ DWORD maxStringLength,
  _In_ DWORD);

DWORD
WINAPI
UnDecorateSymbolNameW(
  _In_ PCWSTR,
  _Out_writes_(maxStringLength) PWSTR,
  _In_ DWORD maxStringLength,
  _In_ DWORD);

BOOL
WINAPI
SymGetScope(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_ DWORD,
  _Inout_ PSYMBOL_INFO);

BOOL
WINAPI
SymGetScopeW(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_ DWORD,
  _Inout_ PSYMBOL_INFOW);

BOOL
WINAPI
SymFromIndex(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_ DWORD,
  _Inout_ PSYMBOL_INFO);

BOOL
WINAPI
SymFromIndexW(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_ DWORD,
  _Inout_ PSYMBOL_INFOW);

BOOL
WINAPI
SymAddSymbol(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_ PCSTR,
  _In_ DWORD64,
  _In_ DWORD,
  _In_ DWORD);

BOOL
WINAPI
SymAddSymbolW(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_ PCWSTR,
  _In_ DWORD64,
  _In_ DWORD,
  _In_ DWORD);

BOOL
WINAPI
SymDeleteSymbol(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_opt_ PCSTR,
  _In_ DWORD64,
  _In_ DWORD);

BOOL
WINAPI
SymDeleteSymbolW(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_opt_ PCWSTR,
  _In_ DWORD64,
  _In_ DWORD);

/*************************
 *      Source Files     *
 *************************/

typedef BOOL
(CALLBACK *PSYM_ENUMSOURCEFILES_CALLBACK)(
  _In_ PSOURCEFILE,
  _In_opt_ PVOID);

typedef BOOL
(CALLBACK *PSYM_ENUMSOURCEFILES_CALLBACKW)(
  _In_ PSOURCEFILEW,
  _In_opt_ PVOID);

BOOL
WINAPI
SymEnumSourceFiles(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_opt_ PCSTR,
  _In_ PSYM_ENUMSOURCEFILES_CALLBACK,
  _In_opt_ PVOID);

BOOL
WINAPI
SymEnumSourceFilesW(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_opt_ PCWSTR,
  _In_ PSYM_ENUMSOURCEFILES_CALLBACKW,
  _In_opt_ PVOID);

BOOL
WINAPI
SymGetLineFromAddr64(
  _In_ HANDLE,
  _In_ DWORD64,
  _Out_ PDWORD,
  _Out_ PIMAGEHLP_LINE64);

BOOL
WINAPI
SymGetLineFromAddrW64(
  _In_ HANDLE,
  _In_ DWORD64,
  _Out_ PDWORD,
  _Out_ PIMAGEHLP_LINEW64);

BOOL WINAPI SymGetLinePrev64(_In_ HANDLE, _Inout_ PIMAGEHLP_LINE64);
BOOL WINAPI SymGetLinePrevW64(_In_ HANDLE, _Inout_ PIMAGEHLP_LINEW64);
BOOL WINAPI SymGetLineNext64(_In_ HANDLE, _Inout_ PIMAGEHLP_LINE64);
BOOL WINAPI SymGetLineNextW64(_In_ HANDLE, _Inout_ PIMAGEHLP_LINEW64);

BOOL
WINAPI
SymGetLineFromName64(
  _In_ HANDLE,
  _In_opt_ PCSTR,
  _In_opt_ PCSTR,
  _In_ DWORD,
  _Out_ PLONG,
  _Inout_ PIMAGEHLP_LINE64);

BOOL
WINAPI
SymGetLineFromNameW64(
  _In_ HANDLE,
  _In_opt_ PCWSTR,
  _In_opt_ PCWSTR,
  _In_ DWORD,
  _Out_ PLONG,
  _Inout_ PIMAGEHLP_LINEW64);

ULONG
WINAPI
SymGetFileLineOffsets64(
  _In_ HANDLE,
  _In_opt_ PCSTR,
  _In_ PCSTR,
  _Out_writes_(BufferLines) PDWORD64,
  _In_ ULONG BufferLines);

BOOL
WINAPI
SymGetSourceFile(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_opt_ PCSTR,
  _In_ PCSTR,
  _Out_writes_(Size) PSTR,
  _In_ DWORD Size);

BOOL
WINAPI
SymGetSourceFileW(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_opt_ PCWSTR,
  _In_ PCWSTR,
  _Out_writes_(Size) PWSTR,
  _In_ DWORD Size);

BOOL
WINAPI
SymGetSourceFileToken(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_ PCSTR,
  _Outptr_ PVOID*,
  _Out_ DWORD*);

BOOL
WINAPI
SymGetSourceFileTokenW(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_ PCWSTR,
  _Outptr_ PVOID*,
  _Out_ DWORD*);

BOOL
WINAPI
SymGetSourceFileFromToken(
  _In_ HANDLE,
  _In_ PVOID,
  _In_opt_ PCSTR,
  _Out_writes_(Size) PSTR,
  _In_ DWORD Size);

BOOL
WINAPI
SymGetSourceFileFromTokenW(
  _In_ HANDLE,
  _In_ PVOID,
  _In_opt_ PCWSTR,
  _Out_writes_(Size) PWSTR,
  _In_ DWORD Size);

BOOL
WINAPI
SymGetSourceVarFromToken(
  _In_ HANDLE,
  _In_ PVOID,
  _In_opt_ PCSTR,
  _In_ PCSTR,
  _Out_writes_(Size) PSTR,
  _In_ DWORD Size);

BOOL
WINAPI
SymGetSourceVarFromTokenW(
  _In_ HANDLE,
  _In_ PVOID,
  _In_opt_ PCWSTR,
  _In_ PCWSTR,
  _Out_writes_(Size) PWSTR,
  _In_ DWORD Size);

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

typedef BOOL
(CALLBACK* PSYM_ENUMLINES_CALLBACK)(
  _In_ PSRCCODEINFO,
  _In_opt_ PVOID);

typedef BOOL
(CALLBACK* PSYM_ENUMLINES_CALLBACKW)(
  _In_ PSRCCODEINFOW,
  _In_opt_ PVOID);

BOOL
WINAPI
SymEnumLines(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_opt_ PCSTR,
  _In_opt_ PCSTR,
  _In_ PSYM_ENUMLINES_CALLBACK,
  _In_opt_ PVOID);

BOOL
WINAPI
SymEnumLinesW(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_opt_ PCWSTR,
  _In_opt_ PCWSTR,
  _In_ PSYM_ENUMLINES_CALLBACKW,
  _In_opt_ PVOID);

BOOL
WINAPI
SymEnumSourceLines(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_opt_ PCSTR,
  _In_opt_ PCSTR,
  _In_opt_ DWORD,
  _In_ DWORD,
  _In_ PSYM_ENUMLINES_CALLBACK,
  _In_opt_ PVOID);

BOOL
WINAPI
SymEnumSourceLinesW(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_opt_ PCWSTR,
  _In_opt_ PCWSTR,
  _In_opt_ DWORD,
  _In_ DWORD,
  _In_ PSYM_ENUMLINES_CALLBACKW,
  _In_opt_ PVOID);

/*************************
 * File & image handling *
 *************************/

BOOL WINAPI SymInitialize(_In_ HANDLE, _In_opt_ PCSTR, _In_ BOOL);
BOOL WINAPI SymInitializeW(_In_ HANDLE, _In_opt_ PCWSTR, _In_ BOOL);
BOOL WINAPI SymCleanup(_In_ HANDLE);

HANDLE
WINAPI
FindDebugInfoFile(
  _In_ PCSTR,
  _In_ PCSTR,
  _Out_writes_(MAX_PATH + 1) PSTR);

typedef BOOL
(CALLBACK *PFIND_DEBUG_FILE_CALLBACK)(
  _In_ HANDLE,
  _In_ PCSTR,
  _In_ PVOID);

typedef BOOL
(CALLBACK *PFIND_DEBUG_FILE_CALLBACKW)(
  _In_ HANDLE,
  _In_ PCWSTR,
  _In_ PVOID);

HANDLE
WINAPI
FindDebugInfoFileEx(
  _In_ PCSTR,
  _In_ PCSTR,
  _Out_writes_(MAX_PATH + 1) PSTR,
  _In_opt_ PFIND_DEBUG_FILE_CALLBACK,
  _In_opt_ PVOID);

HANDLE
WINAPI
FindDebugInfoFileExW(
  _In_ PCWSTR,
  _In_ PCWSTR,
  _Out_writes_(MAX_PATH + 1) PWSTR,
  _In_opt_ PFIND_DEBUG_FILE_CALLBACKW,
  _In_opt_ PVOID);

HANDLE
WINAPI
SymFindDebugInfoFile(
  _In_ HANDLE,
  _In_ PCSTR,
  _Out_writes_(MAX_PATH + 1) PSTR,
  _In_opt_ PFIND_DEBUG_FILE_CALLBACK,
  _In_opt_ PVOID);

HANDLE
WINAPI
SymFindDebugInfoFileW(
  _In_ HANDLE,
  _In_ PCWSTR,
  _Out_writes_(MAX_PATH + 1) PWSTR,
  _In_opt_ PFIND_DEBUG_FILE_CALLBACKW,
  _In_opt_ PVOID);

typedef BOOL
(CALLBACK *PFINDFILEINPATHCALLBACK)(
  _In_ PCSTR,
  _In_ PVOID);

typedef BOOL
(CALLBACK *PFINDFILEINPATHCALLBACKW)(
  _In_ PCWSTR,
  _In_ PVOID);

BOOL WINAPI FindFileInPath(HANDLE, PCSTR, PCSTR, PVOID, DWORD, DWORD, DWORD,
                           PSTR, PFINDFILEINPATHCALLBACK, PVOID);

BOOL
WINAPI
SymFindFileInPath(
  _In_ HANDLE,
  _In_opt_ PCSTR,
  _In_ PCSTR,
  _In_opt_ PVOID,
  _In_ DWORD,
  _In_ DWORD,
  _In_ DWORD,
  _Out_writes_(MAX_PATH + 1) PSTR,
  _In_opt_ PFINDFILEINPATHCALLBACK,
  _In_opt_ PVOID);

BOOL
WINAPI
SymFindFileInPathW(
  _In_ HANDLE,
  _In_opt_ PCWSTR,
  _In_ PCWSTR,
  _In_opt_ PVOID,
  _In_ DWORD,
  _In_ DWORD,
  _In_ DWORD,
  _Out_writes_(MAX_PATH + 1) PWSTR,
  _In_opt_ PFINDFILEINPATHCALLBACKW,
  _In_opt_ PVOID);

HANDLE
WINAPI
FindExecutableImage(
  _In_ PCSTR,
  _In_ PCSTR,
  _Out_writes_(MAX_PATH + 1) PSTR);

typedef BOOL
(CALLBACK *PFIND_EXE_FILE_CALLBACK)(
  _In_ HANDLE,
  _In_ PCSTR,
  _In_opt_ PVOID);

typedef BOOL
(CALLBACK *PFIND_EXE_FILE_CALLBACKW)(
  _In_ HANDLE,
  _In_ PCWSTR,
  _In_opt_ PVOID);

HANDLE
WINAPI
FindExecutableImageEx(
  _In_ PCSTR,
  _In_ PCSTR,
  _Out_writes_(MAX_PATH + 1) PSTR,
  _In_opt_ PFIND_EXE_FILE_CALLBACK,
  _In_opt_ PVOID);

HANDLE
WINAPI
FindExecutableImageExW(
  _In_ PCWSTR,
  _In_ PCWSTR,
  _Out_writes_(MAX_PATH + 1) PWSTR,
  _In_opt_ PFIND_EXE_FILE_CALLBACKW,
  _In_opt_ PVOID);

HANDLE
WINAPI
SymFindExecutableImage(
  _In_ HANDLE,
  _In_ PCSTR,
  _Out_writes_(MAX_PATH + 1) PSTR,
  _In_ PFIND_EXE_FILE_CALLBACK,
  _In_ PVOID);

HANDLE
WINAPI
SymFindExecutableImageW(
  _In_ HANDLE,
  _In_ PCWSTR,
  _Out_writes_(MAX_PATH + 1) PWSTR,
  _In_ PFIND_EXE_FILE_CALLBACKW,
  _In_ PVOID);

PIMAGE_NT_HEADERS WINAPI ImageNtHeader(_In_ PVOID);

PVOID
WINAPI
ImageDirectoryEntryToDataEx(
  _In_ PVOID,
  _In_ BOOLEAN,
  _In_ USHORT,
  _Out_ PULONG,
  _Out_opt_ PIMAGE_SECTION_HEADER *);

PVOID
WINAPI
ImageDirectoryEntryToData(
  _In_ PVOID,
  _In_ BOOLEAN,
  _In_ USHORT,
  _Out_ PULONG);

PIMAGE_SECTION_HEADER
WINAPI
ImageRvaToSection(
  _In_ PIMAGE_NT_HEADERS,
  _In_ PVOID,
  _In_ ULONG);

PVOID
WINAPI
ImageRvaToVa(
  _In_ PIMAGE_NT_HEADERS,
  _In_ PVOID,
  _In_ ULONG,
  _In_opt_ PIMAGE_SECTION_HEADER*);

BOOL
WINAPI
SymGetSearchPath(
  _In_ HANDLE,
  _Out_writes_(SearchPathLength) PSTR,
  _In_ DWORD SearchPathLength);

BOOL
WINAPI
SymGetSearchPathW(
  _In_ HANDLE,
  _Out_writes_(SearchPathLength) PWSTR,
  _In_ DWORD SearchPathLength);

BOOL WINAPI SymSetSearchPath(_In_ HANDLE, _In_opt_ PCSTR);
BOOL WINAPI SymSetSearchPathW(_In_ HANDLE, _In_opt_ PCWSTR);
DWORD WINAPI GetTimestampForLoadedLibrary(_In_ HMODULE);
BOOL WINAPI MakeSureDirectoryPathExists(_In_ PCSTR);

BOOL
WINAPI
SearchTreeForFile(
  _In_ PCSTR,
  _In_ PCSTR,
  _Out_writes_(MAX_PATH + 1) PSTR);

BOOL
WINAPI
SearchTreeForFileW(
  _In_ PCWSTR,
  _In_ PCWSTR,
  _Out_writes_(MAX_PATH + 1) PWSTR);

typedef BOOL
(CALLBACK *PENUMDIRTREE_CALLBACK)(
  _In_ PCSTR,
  _In_opt_ PVOID);

typedef BOOL
(CALLBACK *PENUMDIRTREE_CALLBACKW)(
  _In_ PCWSTR,
  _In_opt_ PVOID);

BOOL
WINAPI
EnumDirTree(
  _In_opt_ HANDLE,
  _In_ PCSTR,
  _In_ PCSTR,
  _Out_writes_opt_(MAX_PATH + 1) PSTR,
  _In_opt_ PENUMDIRTREE_CALLBACK,
  _In_opt_ PVOID);

BOOL
WINAPI
EnumDirTreeW(
  _In_opt_ HANDLE,
  _In_ PCWSTR,
  _In_ PCWSTR,
  _Out_writes_opt_(MAX_PATH + 1) PWSTR,
  _In_opt_ PENUMDIRTREE_CALLBACKW,
  _In_opt_ PVOID);

BOOL
WINAPI
SymMatchFileName(
  _In_ PCSTR,
  _In_ PCSTR,
  _Outptr_opt_ PSTR*,
  _Outptr_opt_ PSTR*);

BOOL
WINAPI
SymMatchFileNameW(
  _In_ PCWSTR,
  _In_ PCWSTR,
  _Outptr_opt_ PWSTR*,
  _Outptr_opt_ PWSTR*);

PCHAR WINAPI SymSetHomeDirectory(_In_opt_ HANDLE, _In_opt_ PCSTR);
PWSTR WINAPI SymSetHomeDirectoryW(_In_opt_ HANDLE, _In_opt_ PCWSTR);

PCHAR
WINAPI
SymGetHomeDirectory(
  _In_ DWORD,
  _Out_writes_(size) PSTR,
  _In_ size_t size);

PWSTR
WINAPI
SymGetHomeDirectoryW(
  _In_ DWORD,
  _Out_writes_(size) PWSTR,
  _In_ size_t size);

#define hdBase  0
#define hdSym   1
#define hdSrc   2
#define hdMax   3

/*************************
 *   Context management  *
 *************************/

BOOL
WINAPI
SymSetContext(
  _In_ HANDLE,
  _In_ PIMAGEHLP_STACK_FRAME,
  _In_opt_ PIMAGEHLP_CONTEXT);


/*************************
 *    Stack management   *
 *************************/

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define KDHELP KDHELP64
#define PKDHELP PKDHELP64
#else
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
#endif

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

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define STACKFRAME STACKFRAME64
#define LPSTACKFRAME LPSTACKFRAME64
#else
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
#endif

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

typedef BOOL
(CALLBACK *PREAD_PROCESS_MEMORY_ROUTINE64)(
  _In_ HANDLE,
  _In_ DWORD64,
  _Out_writes_bytes_(nSize) PVOID,
  _In_ DWORD nSize,
  _Out_ PDWORD);

typedef PVOID
(CALLBACK *PFUNCTION_TABLE_ACCESS_ROUTINE64)(
  _In_ HANDLE,
  _In_ DWORD64);

typedef DWORD64
(CALLBACK *PGET_MODULE_BASE_ROUTINE64)(
  _In_ HANDLE,
  _In_ DWORD64);

typedef DWORD64
(CALLBACK *PTRANSLATE_ADDRESS_ROUTINE64)(
  _In_ HANDLE,
  _In_ HANDLE,
  _In_ LPADDRESS64);

BOOL
WINAPI
StackWalk64(
  _In_ DWORD,
  _In_ HANDLE,
  _In_ HANDLE,
  _Inout_ LPSTACKFRAME64,
  _Inout_ PVOID,
  _In_opt_ PREAD_PROCESS_MEMORY_ROUTINE64,
  _In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE64,
  _In_opt_ PGET_MODULE_BASE_ROUTINE64,
  _In_opt_ PTRANSLATE_ADDRESS_ROUTINE64);

PVOID WINAPI SymFunctionTableAccess64(_In_ HANDLE, _In_ DWORD64);

typedef PVOID
(CALLBACK *PSYMBOL_FUNCENTRY_CALLBACK64)(
  _In_ HANDLE,
  _In_ ULONG64,
  _In_ ULONG64);

BOOL
WINAPI
SymRegisterFunctionEntryCallback64(
  _In_ HANDLE,
  _In_ PSYMBOL_FUNCENTRY_CALLBACK64,
  _In_ ULONG64);

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
LPAPI_VERSION WINAPI ImagehlpApiVersionEx(_In_ LPAPI_VERSION);

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


PIMAGE_DEBUG_INFORMATION
WINAPI
MapDebugInformation(
  _In_opt_ HANDLE,
  _In_ PCSTR,
  _In_opt_ PCSTR,
  _In_ ULONG);

BOOL WINAPI UnmapDebugInformation(_Out_ PIMAGE_DEBUG_INFORMATION);

DWORD WINAPI SymGetOptions(void);
DWORD WINAPI SymSetOptions(_In_ DWORD);

BOOL WINAPI SymSetParentWindow(_In_ HWND);

BOOL
IMAGEAPI
SymSrvIsStore(
  _In_opt_ HANDLE hProcess,
  _In_ PCSTR path);

BOOL
IMAGEAPI
SymSrvIsStoreW(
    _In_opt_ HANDLE hProcess,
    _In_ PCWSTR path);

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

/* 32-bit functions */

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)

#define PENUMLOADED_MODULES_CALLBACK PENUMLOADED_MODULES_CALLBACK64
#define PFUNCTION_TABLE_ACCESS_ROUTINE PFUNCTION_TABLE_ACCESS_ROUTINE64
#define PGET_MODULE_BASE_ROUTINE PGET_MODULE_BASE_ROUTINE64
#define PREAD_PROCESS_MEMORY_ROUTINE PREAD_PROCESS_MEMORY_ROUTINE64
#define PSYMBOL_FUNCENTRY_CALLBACK PSYMBOL_FUNCENTRY_CALLBACK64
#define PSYMBOL_REGISTERED_CALLBACK PSYMBOL_REGISTERED_CALLBACK64
#define PSYM_ENUMMODULES_CALLBACK PSYM_ENUMMODULES_CALLBACK64
#define PSYM_ENUMSYMBOLS_CALLBACK PSYM_ENUMSYMBOLS_CALLBACK64
#define PSYM_ENUMSYMBOLS_CALLBACKW PSYM_ENUMSYMBOLS_CALLBACKW64
#define PTRANSLATE_ADDRESS_ROUTINE PTRANSLATE_ADDRESS_ROUTINE64

#define EnumerateLoadedModules EnumerateLoadedModules64
#define StackWalk StackWalk64
#define SymEnumerateModules SymEnumerateModules64
#define SymEnumerateSymbols SymEnumerateSymbols64
#define SymEnumerateSymbolsW SymEnumerateSymbolsW64
#define SymFunctionTableAccess SymFunctionTableAccess64
#define SymGetLineFromAddr SymGetLineFromAddr64
#define SymGetLineFromAddrW SymGetLineFromAddrW64
#define SymGetLineFromName SymGetLineFromName64
#define SymGetLineNext SymGetLineNext64
#define SymGetLineNextW SymGetLineNextW64
#define SymGetLinePrev SymGetLinePrev64
#define SymGetLinePrevW SymGetLinePrevW64
#define SymGetModuleBase SymGetModuleBase64
#define SymGetModuleInfo SymGetModuleInfo64
#define SymGetModuleInfoW SymGetModuleInfoW64
#define SymGetSymFromAddr SymGetSymFromAddr64
#define SymGetSymFromName SymGetSymFromName64
#define SymGetSymNext SymGetSymNext64
#define SymGetSymNextW SymGetSymNextW64
#define SymGetSymPrev SymGetSymPrev64
#define SymGetSymPrevW SymGetSymPrevW64
#define SymLoadModule SymLoadModule64
#define SymRegisterCallback SymRegisterCallback64
#define SymRegisterFunctionEntryCallback SymRegisterFunctionEntryCallback64
#define SymUnDName SymUnDName64
#define SymUnloadModule SymUnloadModule64

#else

typedef BOOL
(CALLBACK *PENUMLOADED_MODULES_CALLBACK)(
  _In_ PCSTR,
  _In_ ULONG,
  _In_ ULONG,
  _In_opt_ PVOID);

typedef PVOID
(CALLBACK *PFUNCTION_TABLE_ACCESS_ROUTINE)(
  _In_ HANDLE,
  _In_ DWORD);

typedef DWORD
(CALLBACK *PGET_MODULE_BASE_ROUTINE)(
  _In_ HANDLE,
  _In_ DWORD);

typedef BOOL
(CALLBACK *PREAD_PROCESS_MEMORY_ROUTINE)(
  _In_ HANDLE,
  _In_ DWORD,
  _Out_writes_bytes_(nSize) PVOID,
  _In_ DWORD nSize,
  _Out_ PDWORD);

typedef BOOL
(CALLBACK *PSYM_ENUMMODULES_CALLBACK)(
  _In_ PCSTR,
  _In_ ULONG,
  _In_opt_ PVOID);

typedef BOOL
(CALLBACK *PSYM_ENUMSYMBOLS_CALLBACK)(
  _In_ PCSTR,
  _In_ ULONG,
  _In_ ULONG,
  _In_opt_ PVOID);

typedef BOOL
(CALLBACK *PSYM_ENUMSYMBOLS_CALLBACKW)(
  _In_ PCWSTR,
  _In_ ULONG,
  _In_ ULONG,
  _In_opt_ PVOID);

typedef BOOL
(CALLBACK *PSYMBOL_REGISTERED_CALLBACK)(
  _In_ HANDLE,
  _In_ ULONG,
  _In_opt_ PVOID,
  _In_opt_ PVOID);

typedef PVOID
(CALLBACK *PSYMBOL_FUNCENTRY_CALLBACK)(
  _In_ HANDLE,
  _In_ DWORD,
  _In_opt_ PVOID);

typedef DWORD
(CALLBACK *PTRANSLATE_ADDRESS_ROUTINE)(
  _In_ HANDLE,
  _In_ HANDLE,
  _Out_ LPADDRESS);

BOOL
WINAPI
EnumerateLoadedModules(
  _In_ HANDLE,
  _In_ PENUMLOADED_MODULES_CALLBACK,
  _In_opt_ PVOID);

BOOL
WINAPI
StackWalk(
  _In_ DWORD,
  _In_ HANDLE,
  _In_ HANDLE,
  _Inout_ LPSTACKFRAME,
  _Inout_ PVOID,
  _In_opt_ PREAD_PROCESS_MEMORY_ROUTINE,
  _In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE,
  _In_opt_ PGET_MODULE_BASE_ROUTINE,
  _In_opt_ PTRANSLATE_ADDRESS_ROUTINE);

BOOL
WINAPI
SymEnumerateModules(
  _In_ HANDLE,
  _In_ PSYM_ENUMMODULES_CALLBACK,
  _In_opt_ PVOID);

BOOL
WINAPI
SymEnumerateSymbols(
  _In_ HANDLE,
  _In_ ULONG,
  _In_ PSYM_ENUMSYMBOLS_CALLBACK,
  _In_opt_ PVOID);

BOOL
WINAPI
SymEnumerateSymbolsW(
  _In_ HANDLE,
  _In_ ULONG,
  _In_ PSYM_ENUMSYMBOLS_CALLBACKW,
  _In_opt_ PVOID);

PVOID WINAPI SymFunctionTableAccess(_In_ HANDLE, _In_ DWORD);

BOOL
WINAPI
SymGetLineFromAddr(
  _In_ HANDLE,
  _In_ DWORD,
  _Out_ PDWORD,
  _Out_ PIMAGEHLP_LINE);

BOOL
WINAPI
SymGetLineFromAddrW(
  _In_ HANDLE,
  _In_ DWORD,
  _Out_ PDWORD,
  _Out_ PIMAGEHLP_LINEW);

BOOL
WINAPI
SymGetLineFromName(
  _In_ HANDLE,
  _In_opt_ PCSTR,
  _In_opt_ PCSTR,
  _In_ DWORD,
  _Out_ PLONG,
  _Inout_ PIMAGEHLP_LINE);

BOOL WINAPI SymGetLineNext(_In_ HANDLE, _Inout_ PIMAGEHLP_LINE);
BOOL WINAPI SymGetLineNextW(_In_ HANDLE, _Inout_ PIMAGEHLP_LINEW);
BOOL WINAPI SymGetLinePrev(_In_ HANDLE, _Inout_ PIMAGEHLP_LINE);
BOOL WINAPI SymGetLinePrevW(_In_ HANDLE, _Inout_ PIMAGEHLP_LINEW);
DWORD WINAPI SymGetModuleBase(_In_ HANDLE, _In_ DWORD);

BOOL
WINAPI
SymGetModuleInfo(
  _In_ HANDLE,
  _In_ DWORD,
  _Out_ PIMAGEHLP_MODULE);

BOOL
WINAPI
SymGetModuleInfoW(
  _In_ HANDLE,
  _In_ DWORD,
  _Out_ PIMAGEHLP_MODULEW);

BOOL
WINAPI
SymGetSymFromAddr(
  _In_ HANDLE,
  _In_ DWORD,
  _Out_opt_ PDWORD,
  _Inout_ PIMAGEHLP_SYMBOL);

BOOL
WINAPI
SymGetSymFromName(
  _In_ HANDLE,
  _In_ PCSTR,
  _Inout_ PIMAGEHLP_SYMBOL);

BOOL WINAPI SymGetSymNext(_In_ HANDLE, _Inout_ PIMAGEHLP_SYMBOL);
BOOL WINAPI SymGetSymNextW(_In_ HANDLE, _Inout_ PIMAGEHLP_SYMBOLW);
BOOL WINAPI SymGetSymPrev(_In_ HANDLE, _Inout_ PIMAGEHLP_SYMBOL);
BOOL WINAPI SymGetSymPrevW(_In_ HANDLE, _Inout_ PIMAGEHLP_SYMBOLW);

DWORD
WINAPI
SymLoadModule(
  _In_ HANDLE,
  _In_opt_ HANDLE,
  _In_opt_ PCSTR,
  _In_opt_ PCSTR,
  _In_ DWORD,
  _In_ DWORD);

BOOL
WINAPI
SymRegisterCallback(
  _In_ HANDLE,
  _In_ PSYMBOL_REGISTERED_CALLBACK,
  _In_opt_ PVOID);

BOOL
WINAPI
SymRegisterFunctionEntryCallback(
  _In_ HANDLE,
  _In_ PSYMBOL_FUNCENTRY_CALLBACK,
  _In_opt_ PVOID);

BOOL WINAPI SymRefreshModuleList(_In_ HANDLE);

BOOL
WINAPI
SymUnDName(
  _In_ PIMAGEHLP_SYMBOL,
  _Out_writes_(UnDecNameLength) PSTR,
  _In_ DWORD UnDecNameLength);

BOOL WINAPI SymUnloadModule(_In_ HANDLE, _In_ DWORD);

#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif  /* __WINE_DBGHELP_H */
