#pragma once

#include <typedefs.h>
#include <guiddef.h>
#include <pecoff.h>
#include <wine/unicode.h>

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <ctype.h>

typedef HANDLE HWND;

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))

#ifdef __i386__
#define CDECL __cdecl
#else
#define CDECL
#endif
typedef PVOID IUnknown, IDispatch, IRecordInfo;

// windef.h
#define MAX_PATH 260
#define CALLBACK
typedef int (*FARPROC)();

// bytesex.h
#define SWAPD(x) x
#define SWAPW(x) x

// Wine stuff
#define DECLSPEC_HIDDEN
#define WINE_DEFAULT_DEBUG_CHANNEL(x)
#define WINE_DECLARE_DEBUG_CHANNEL(x)
extern const char *wine_dbgstr_an( const char * s, int n );
extern const char *wine_dbgstr_wn( const WCHAR *s, int n );
extern const char *wine_dbg_sprintf( const char *format, ... );
static __inline const char *wine_dbgstr_longlong( ULONGLONG ll )
{
    if (/*sizeof(ll) > sizeof(unsigned long) &&*/ ll >> 32) /* ULONGLONG is always > long in ReactOS */
        return wine_dbg_sprintf( "%lx%08lx", (unsigned long)(ll >> 32), (unsigned long)ll );
    else return wine_dbg_sprintf( "%lx", (unsigned long)ll );
}
static __inline const char *debugstr_an( const char * s, int n ) { return wine_dbgstr_an( s, n ); }
static __inline const char *debugstr_wn( const WCHAR *s, int n ) { return wine_dbgstr_wn( s, n ); }
static __inline const char *debugstr_a( const char *s )  { return wine_dbgstr_an( s, -1 ); }
static __inline const char *debugstr_w( const WCHAR *s ) { return wine_dbgstr_wn( s, -1 ); }
static __inline const char *wine_dbgstr_w( const WCHAR *s ){return wine_dbgstr_wn( s, -1 );}

#if 0
#define WARN(fmt, ...) fprintf(stderr, "WARN %s: " fmt, __FUNCTION__, ##__VA_ARGS__)
#define FIXME(fmt, ...) fprintf(stderr, "FIXME %s: " fmt, __FUNCTION__, ##__VA_ARGS__)
#define TRACE(fmt, ...) fprintf(stderr, "TRACE %s: " fmt, __FUNCTION__, ##__VA_ARGS__)
#define ERR(fmt, ...) fprintf(stderr, "ERR %s: " fmt, __FUNCTION__, ##__VA_ARGS__)
#else
#define WARN(fmt, ...)
#define FIXME(fmt, ...)
#define TRACE(fmt, ...)
#define ERR(fmt, ...)
#endif

#define TRACE_ON(x) FALSE
#define TRACE_(x) TRACE
#define FIXME_(x) FIXME
const char *wine_dbg_sprintf( const char *format, ... );
#define CP_UNIXCP CP_ACP
#define __TRY if(1)
#define __EXCEPT_PAGE_FAULT else
#define __ENDTRY

// basetsd.h
typedef ULONG_PTR KAFFINITY;

// excpt.h
typedef enum _EXCEPTION_DISPOSITION
{
    ExceptionContinueExecution,
    ExceptionContinueSearch,
    ExceptionNestedException,
    ExceptionCollidedUnwind,
} EXCEPTION_DISPOSITION;

// winerror.h
#define ERROR_ACCESS_DENIED                                5
#define ERROR_INVALID_HANDLE                               6
#define ERROR_OUTOFMEMORY                                  14
#define ERROR_NOT_SUPPORTED                                50
#define ERROR_INVALID_PARAMETER                            87
#define ERROR_CALL_NOT_IMPLEMENTED                         120
#define ERROR_INVALID_NAME                                 123
#define ERROR_MOD_NOT_FOUND                                126
#define ERROR_NO_MORE_ITEMS                                259
#define ERROR_INVALID_ADDRESS                              487

// winnls.h
#define CP_ACP 0
#define MultiByteToWideChar __MultiByteToWideChar
#define WideCharToMultiByte __WideCharToMultiByte
INT __MultiByteToWideChar( UINT page, DWORD flags, LPCSTR src, INT srclen, LPWSTR dst, INT dstlen );
INT __WideCharToMultiByte( UINT page, DWORD flags, LPCWSTR src, INT srclen, LPSTR dst, INT dstlen, LPCSTR defchar, BOOL *used );

// #define strlenW(s) wcslen((s))
// #define strcpyW(d,s) wcscpy((d),(s))
// #define strchrW(s,c) wcschr((s),(c))
// #define strcatW(d,s) wcscat((d),(s))
// #define strncmpiW(s1,s2,n) _wcsnicmp((s1),(s2),(n))
// #define strcmpW(s1,s2) wcscmp((s1),(s2))
// #define strcmpiW(s1,s2) _wcsicmp((s1),(s2))
// #define tolowerW(n) towlower((n))
// #define toupperW(n) towupper((n))

// winnt.h
#define IMAGE_FILE_MACHINE_ARMNT      0x1c4
#define IMAGE_FILE_MACHINE_POWERPC    0x1f0
#define IMAGE_FILE_MACHINE_ARM64      0x1c5
#define DLL_PROCESS_DETACH	0
#define DLL_PROCESS_ATTACH	1
#define DLL_THREAD_ATTACH	2
#define DLL_THREAD_DETACH	3
#define HEAP_ZERO_MEMORY 8
#define GENERIC_READ	0x80000000
#define FILE_SHARE_READ			0x00000001
#define FILE_ATTRIBUTE_NORMAL			0x00000080
#define PAGE_READONLY	0x0002
#define SECTION_MAP_READ 4
#define IMAGE_DEBUG_TYPE_UNKNOWN 0
#define IMAGE_DEBUG_TYPE_COFF 1
#define IMAGE_DEBUG_TYPE_CODEVIEW 2
#define IMAGE_DEBUG_TYPE_FPO 3
#define IMAGE_DEBUG_TYPE_MISC 4
#define IMAGE_DEBUG_TYPE_EXCEPTION 5
#define IMAGE_DEBUG_TYPE_FIXUP 6
#define IMAGE_DEBUG_TYPE_OMAP_TO_SRC 7
#define IMAGE_DEBUG_TYPE_OMAP_FROM_SRC 8
#define IMAGE_SYM_CLASS_EXTERNAL 2
#define IMAGE_SYM_CLASS_FILE 103
#define IMAGE_DIRECTORY_ENTRY_EXPORT	0
#define IMAGE_DIRECTORY_ENTRY_DEBUG	6
#define IMAGE_DEBUG_MISC_EXENAME 1
#define IMAGE_SEPARATE_DEBUG_SIGNATURE 0x4944
typedef struct _IMAGE_EXPORT_DIRECTORY {
  DWORD Characteristics;
  DWORD TimeDateStamp;
  WORD MajorVersion;
  WORD MinorVersion;
  DWORD Name;
  DWORD Base;
  DWORD NumberOfFunctions;
  DWORD NumberOfNames;
  DWORD AddressOfFunctions;
  DWORD AddressOfNames;
  DWORD AddressOfNameOrdinals;
} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;
typedef struct _IMAGE_DEBUG_MISC {
  DWORD DataType;
  DWORD Length;
  BOOLEAN Unicode;
  BYTE Reserved[3];
  BYTE Data[1];
} IMAGE_DEBUG_MISC, *PIMAGE_DEBUG_MISC;
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
  DWORD SectionAlignment;
  DWORD Reserved[2];
} IMAGE_SEPARATE_DEBUG_HEADER, *PIMAGE_SEPARATE_DEBUG_HEADER;
typedef struct _SECURITY_ATTRIBUTES {
	DWORD nLength;
	LPVOID lpSecurityDescriptor;
	BOOL bInheritHandle;
} SECURITY_ATTRIBUTES,*PSECURITY_ATTRIBUTES,*LPSECURITY_ATTRIBUTES;
typedef struct _IMAGE_DEBUG_DIRECTORY {
  DWORD Characteristics;
  DWORD TimeDateStamp;
  WORD MajorVersion;
  WORD MinorVersion;
  DWORD Type;
  DWORD SizeOfData;
  DWORD AddressOfRawData;
  DWORD PointerToRawData;
} IMAGE_DEBUG_DIRECTORY, *PIMAGE_DEBUG_DIRECTORY;
#define EXCEPTION_MAXIMUM_PARAMETERS   15
typedef struct _EXCEPTION_RECORD {
  DWORD ExceptionCode;
  DWORD ExceptionFlags;
  struct _EXCEPTION_RECORD *ExceptionRecord;
  PVOID ExceptionAddress;
  DWORD NumberParameters;
  ULONG_PTR ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD, *PEXCEPTION_RECORD;
#if defined(_X86_)
#define SIZE_OF_80387_REGISTERS	80
#define CONTEXT_i386	0x10000
#define CONTEXT_i486	0x10000
#define CONTEXT_CONTROL	(CONTEXT_i386|0x00000001L)
#define CONTEXT_INTEGER	(CONTEXT_i386|0x00000002L)
#define CONTEXT_SEGMENTS	(CONTEXT_i386|0x00000004L)
#define CONTEXT_FLOATING_POINT	(CONTEXT_i386|0x00000008L)
#define CONTEXT_DEBUG_REGISTERS	(CONTEXT_i386|0x00000010L)
#define CONTEXT_EXTENDED_REGISTERS (CONTEXT_i386|0x00000020L)
#define CONTEXT_FULL	(CONTEXT_CONTROL|CONTEXT_INTEGER|CONTEXT_SEGMENTS)
#define MAXIMUM_SUPPORTED_EXTENSION  512

#define EXCEPTION_READ_FAULT    0
#define EXCEPTION_WRITE_FAULT   1
#define EXCEPTION_EXECUTE_FAULT 8

typedef struct _FLOATING_SAVE_AREA {
  DWORD ControlWord;
  DWORD StatusWord;
  DWORD TagWord;
  DWORD ErrorOffset;
  DWORD ErrorSelector;
  DWORD DataOffset;
  DWORD DataSelector;
  BYTE RegisterArea[80];
  DWORD Cr0NpxState;
} FLOATING_SAVE_AREA, *PFLOATING_SAVE_AREA;

typedef struct _CONTEXT {
  DWORD ContextFlags;
  DWORD Dr0;
  DWORD Dr1;
  DWORD Dr2;
  DWORD Dr3;
  DWORD Dr6;
  DWORD Dr7;
  FLOATING_SAVE_AREA FloatSave;
  DWORD SegGs;
  DWORD SegFs;
  DWORD SegEs;
  DWORD SegDs;
  DWORD Edi;
  DWORD Esi;
  DWORD Ebx;
  DWORD Edx;
  DWORD Ecx;
  DWORD Eax;
  DWORD Ebp;
  DWORD Eip;
  DWORD SegCs;
  DWORD EFlags;
  DWORD Esp;
  DWORD SegSs;
  BYTE ExtendedRegisters[MAXIMUM_SUPPORTED_EXTENSION];
} CONTEXT, *PCONTEXT;

#else /* ARM? */

/* The following flags control the contents of the CONTEXT structure. */

#define CONTEXT_ARM    0x0200000
#define CONTEXT_CONTROL         (CONTEXT_ARM | 0x00000001)
#define CONTEXT_INTEGER         (CONTEXT_ARM | 0x00000002)
#define CONTEXT_FLOATING_POINT  (CONTEXT_ARM | 0x00000004)
#define CONTEXT_DEBUG_REGISTERS (CONTEXT_ARM | 0x00000008)

#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_INTEGER)

#define EXCEPTION_READ_FAULT    0
#define EXCEPTION_WRITE_FAULT   1
#define EXCEPTION_EXECUTE_FAULT 8

typedef struct _RUNTIME_FUNCTION
{
    DWORD BeginAddress;
    union {
        DWORD UnwindData;
        struct {
            DWORD Flag : 2;
            DWORD FunctionLength : 11;
            DWORD Ret : 2;
            DWORD H : 1;
            DWORD Reg : 3;
            DWORD R : 1;
            DWORD L : 1;
            DWORD C : 1;
            DWORD StackAdjust : 10;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} RUNTIME_FUNCTION, *PRUNTIME_FUNCTION;

#define UNWIND_HISTORY_TABLE_SIZE 12
typedef struct _UNWIND_HISTORY_TABLE_ENTRY
{
    DWORD ImageBase;
    PRUNTIME_FUNCTION FunctionEntry;
} UNWIND_HISTORY_TABLE_ENTRY, *PUNWIND_HISTORY_TABLE_ENTRY;

typedef struct _UNWIND_HISTORY_TABLE
{
    DWORD Count;
    BYTE  LocalHint;
    BYTE  GlobalHint;
    BYTE  Search;
    BYTE  Once;
    DWORD LowAddress;
    DWORD HighAddress;
    UNWIND_HISTORY_TABLE_ENTRY Entry[UNWIND_HISTORY_TABLE_SIZE];
} UNWIND_HISTORY_TABLE, *PUNWIND_HISTORY_TABLE;

typedef struct _CONTEXT {
        /* The flags values within this flag control the contents of
           a CONTEXT record.

           If the context record is used as an input parameter, then
           for each portion of the context record controlled by a flag
           whose value is set, it is assumed that that portion of the
           context record contains valid context. If the context record
           is being used to modify a thread's context, then only that
           portion of the threads context will be modified.

           If the context record is used as an IN OUT parameter to capture
           the context of a thread, then only those portions of the thread's
           context corresponding to set flags will be returned.

           The context record is never used as an OUT only parameter. */

        ULONG ContextFlags;

        /* This section is specified/returned if the ContextFlags word contains
           the flag CONTEXT_INTEGER. */
        ULONG R0;
        ULONG R1;
        ULONG R2;
        ULONG R3;
        ULONG R4;
        ULONG R5;
        ULONG R6;
        ULONG R7;
        ULONG R8;
        ULONG R9;
        ULONG R10;
        ULONG Fp;
        ULONG Ip;

        /* These are selected by CONTEXT_CONTROL */
        ULONG Sp;
        ULONG Lr;
        ULONG Pc;
        ULONG Cpsr;
} CONTEXT;

BOOLEAN CDECL            RtlAddFunctionTable(RUNTIME_FUNCTION*,DWORD,DWORD);
BOOLEAN CDECL            RtlDeleteFunctionTable(RUNTIME_FUNCTION*);
PRUNTIME_FUNCTION WINAPI RtlLookupFunctionEntry(ULONG_PTR,DWORD*,UNWIND_HISTORY_TABLE*);
#endif

typedef
EXCEPTION_DISPOSITION
NTAPI
EXCEPTION_ROUTINE(
  struct _EXCEPTION_RECORD *ExceptionRecord,
  PVOID EstablisherFrame,
  struct _CONTEXT *ContextRecord,
  PVOID DispatcherContext);
typedef EXCEPTION_ROUTINE *PEXCEPTION_ROUTINE;
typedef struct _NT_TIB {
  struct _EXCEPTION_REGISTRATION_RECORD *ExceptionList;
  PVOID StackBase;
  PVOID StackLimit;
  PVOID SubSystemTib;
  union {
    PVOID FiberData;
    DWORD Version;
  } DUMMYUNIONNAME;
  PVOID ArbitraryUserPointer;
  struct _NT_TIB *Self;
} NT_TIB,*PNT_TIB;

// rtltypes.h
typedef struct _EXCEPTION_REGISTRATION_RECORD
{
    struct _EXCEPTION_REGISTRATION_RECORD *Next;
    PEXCEPTION_ROUTINE Handler;
} EXCEPTION_REGISTRATION_RECORD, *PEXCEPTION_REGISTRATION_RECORD;

// winbase.h
#define INVALID_HANDLE_VALUE (HANDLE)(-1)
#define HeapAlloc __HeapAlloc
#define HeapReAlloc __HeapReAlloc
#define HeapFree(x,y,z) free(z) 
#define GetProcessHeap() 1
#define GetProcessId(x) 8
#define lstrcpynW __lstrcpynW
#define CloseHandle __CloseHandle
#define CreateFileA(a,b,c,d,e,f,g) fopen(a, "rb")
#define CreateFileW __CreateFileW
#define CreateFileMappingW(a,b,c,d,e,f) a
#define MapViewOfFile __MapViewOfFile
#define UnmapViewOfFile __UnmapViewOfFile
#define LoadLibraryW(x) 0
#define FreeLibrary(x) 0
#define lstrcpyW strcpyW // Forward this to wine unicode inline function
#define lstrlenW strlenW // ditto
#define lstrcpynA __lstrcpynA
#define SetLastError(x)
#define GetProcAddress(x,y) 0
#define GetEnvironmentVariableA(x, y, z) 0
#define GetEnvironmentVariableW(x, y, z) 0
#define GetCurrentDirectoryW(x, y) 0
#define GetFileSizeEx __GetFileSizeEx
#define ReadProcessMemory(a,b,c,d,e) 0

void* __HeapAlloc(int heap, int flags, size_t size);
void* __HeapReAlloc(int heap, DWORD d2, void *slab, SIZE_T newsize);
WCHAR* __lstrcpynW(WCHAR* lpString1, const WCHAR* lpString2, int iMaxLength);
BOOL __CloseHandle(HANDLE handle);
HANDLE __CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
void* __MapViewOfFile(HANDLE file,DWORD d1,DWORD d2,DWORD d3,SIZE_T s);
BOOL __UnmapViewOfFile(const void*);
LPSTR __lstrcpynA(LPSTR,LPCSTR,int);
BOOL __GetFileSizeEx(HANDLE,PLARGE_INTEGER);
#define OPEN_EXISTING	3
#define FILE_MAP_READ SECTION_MAP_READ
typedef struct _LDT_ENTRY {
	WORD LimitLow;
	WORD BaseLow;
	union {
		struct {
			BYTE BaseMid;
			BYTE Flags1;
			BYTE Flags2;
			BYTE BaseHi;
		} Bytes;
		struct {
			DWORD BaseMid:8;
			DWORD Type:5;
			DWORD Dpl:2;
			DWORD Pres:1;
			DWORD LimitHi:4;
			DWORD Sys:1;
			DWORD Reserved_0:1;
			DWORD Default_Big:1;
			DWORD Granularity:1;
			DWORD BaseHi:8;
		} Bits;
	} HighWord;
} LDT_ENTRY,*PLDT_ENTRY,*LPLDT_ENTRY;

// umtypes.h
typedef LONG KPRIORITY;

// winternl.h
#define RtlImageNtHeader __RtlImageNtHeader
#define RtlImageRvaToVa __RtlImageRvaToVa
#define RtlImageRvaToSection __RtlImageRvaToSection
#define RtlImageDirectoryEntryToData __RtlImageDirectoryEntryToData

#ifdef _MSC_VER
#define RtlUlongByteSwap(_x) _byteswap_ulong((_x))
#else
#define RtlUlongByteSwap(_x) __builtin_bswap32((_x))
#endif

PIMAGE_NT_HEADERS __RtlImageNtHeader(void *data);
PVOID __RtlImageRvaToVa (const IMAGE_NT_HEADERS* NtHeader, PVOID BaseAddress, ULONG Rva, PIMAGE_SECTION_HEADER *SectionHeader);
PVOID __RtlImageDirectoryEntryToData(PVOID BaseAddress, BOOLEAN MappedAsImage, USHORT Directory, PULONG Size);

typedef struct _CLIENT_ID
{
   HANDLE UniqueProcess;
   HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;
#define GDI_BATCH_BUFFER_SIZE 0x136
typedef struct _GDI_TEB_BATCH
{
    ULONG Offset;
    HANDLE HDC;
    ULONG Buffer[GDI_BATCH_BUFFER_SIZE];
} GDI_TEB_BATCH, *PGDI_TEB_BATCH;
typedef struct _TEB
{
    NT_TIB          Tib;                        /* 000 */
    PVOID           EnvironmentPointer;         /* 01c */
    CLIENT_ID       ClientId;                   /* 020 */
    PVOID           ActiveRpcHandle;            /* 028 */
    PVOID           ThreadLocalStoragePointer;  /* 02c */
    PVOID           Peb;                        /* 030 */
    ULONG           LastErrorValue;             /* 034 */
    ULONG           CountOfOwnedCriticalSections;/* 038 */
    PVOID           CsrClientThread;            /* 03c */
    PVOID           Win32ThreadInfo;            /* 040 */
    ULONG           Win32ClientInfo[31];        /* 044 used for user32 private data in Wine */
    PVOID           WOW32Reserved;              /* 0c0 */
    ULONG           CurrentLocale;              /* 0c4 */
    ULONG           FpSoftwareStatusRegister;   /* 0c8 */
    PVOID           SystemReserved1[54];        /* 0cc used for kernel32 private data in Wine */
    PVOID           Spare1;                     /* 1a4 */
    LONG            ExceptionCode;              /* 1a8 */
    PVOID     ActivationContextStackPointer;            /* 1a8/02c8 */
    BYTE            SpareBytes1[36];            /* 1ac */
    PVOID           SystemReserved2[10];        /* 1d4 used for ntdll private data in Wine */
    GDI_TEB_BATCH   GdiTebBatch;                /* 1fc */
    ULONG           gdiRgn;                     /* 6dc */
    ULONG           gdiPen;                     /* 6e0 */
    ULONG           gdiBrush;                   /* 6e4 */
    CLIENT_ID       RealClientId;               /* 6e8 */
    HANDLE          GdiCachedProcessHandle;     /* 6f0 */
    ULONG           GdiClientPID;               /* 6f4 */
    ULONG           GdiClientTID;               /* 6f8 */
    PVOID           GdiThreadLocaleInfo;        /* 6fc */
    PVOID           UserReserved[5];            /* 700 */
    PVOID           glDispatchTable[280];        /* 714 */
    ULONG           glReserved1[26];            /* b74 */
    PVOID           glReserved2;                /* bdc */
    PVOID           glSectionInfo;              /* be0 */
    PVOID           glSection;                  /* be4 */
    PVOID           glTable;                    /* be8 */
    PVOID           glCurrentRC;                /* bec */
    PVOID           glContext;                  /* bf0 */
    ULONG           LastStatusValue;            /* bf4 */
    UNICODE_STRING  StaticUnicodeString;        /* bf8 used by advapi32 */
    WCHAR           StaticUnicodeBuffer[261];   /* c00 used by advapi32 */
    PVOID           DeallocationStack;          /* e0c */
    PVOID           TlsSlots[64];               /* e10 */
    LIST_ENTRY      TlsLinks;                   /* f10 */
    PVOID           Vdm;                        /* f18 */
    PVOID           ReservedForNtRpc;           /* f1c */
    PVOID           DbgSsReserved[2];           /* f20 */
    ULONG           HardErrorDisabled;          /* f28 */
    PVOID           Instrumentation[16];        /* f2c */
    PVOID           WinSockData;                /* f6c */
    ULONG           GdiBatchCount;              /* f70 */
    ULONG           Spare2;                     /* f74 */
    ULONG           Spare3;                     /* f78 */
    ULONG           Spare4;                     /* f7c */
    PVOID           ReservedForOle;             /* f80 */
    ULONG           WaitingOnLoaderLock;        /* f84 */
    PVOID           Reserved5[3];               /* f88 */
    PVOID          *TlsExpansionSlots;          /* f94 */
} TEB, *PTEB;


// winver.h
typedef struct tagVS_FIXEDFILEINFO {
	DWORD dwSignature;
	DWORD dwStrucVersion;
	DWORD dwFileVersionMS;
	DWORD dwFileVersionLS;
	DWORD dwProductVersionMS;
	DWORD dwProductVersionLS;
	DWORD dwFileFlagsMask;
	DWORD dwFileFlags;
	DWORD dwFileOS;
	DWORD dwFileType;
	DWORD dwFileSubtype;
	DWORD dwFileDateMS;
	DWORD dwFileDateLS;
} VS_FIXEDFILEINFO;


// psapi.h
typedef struct _MODULEINFO {
	LPVOID lpBaseOfDll;
	DWORD SizeOfImage;
	LPVOID EntryPoint;
} MODULEINFO,*LPMODULEINFO;
#define GetModuleFileNameExW(w, x, y, z) 0

// pstypes.h
typedef struct _THREAD_BASIC_INFORMATION
{
    NTSTATUS ExitStatus;
    PVOID TebBaseAddress;
    CLIENT_ID ClientId;
    KAFFINITY AffinityMask;
    KPRIORITY Priority;
    KPRIORITY BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;
typedef enum _THREADINFOCLASS {
  ThreadBasicInformation,
  ThreadTimes,
  ThreadPriority,
  ThreadBasePriority,
  ThreadAffinityMask,
  ThreadImpersonationToken,
  ThreadDescriptorTableEntry,
  ThreadEnableAlignmentFaultFixup,
  ThreadEventPair_Reusable,
  ThreadQuerySetWin32StartAddress,
  ThreadZeroTlsCell,
  ThreadPerformanceCount,
  ThreadAmILastThread,
  ThreadIdealProcessor,
  ThreadPriorityBoost,
  ThreadSetTlsArrayAddress,
  ThreadIsIoPending,
  ThreadHideFromDebugger,
  ThreadBreakOnTermination,
  ThreadSwitchLegacyState,
  ThreadIsTerminated,
  ThreadLastSystemCall,
  ThreadIoPriority,
  ThreadCycleTime,
  ThreadPagePriority,
  ThreadActualBasePriority,
  ThreadTebInformation,
  ThreadCSwitchMon,
  ThreadCSwitchPmu,
  ThreadWow64Context,
  ThreadGroupInformation,
  ThreadUmsInformation,
  ThreadCounterProfiling,
  ThreadIdealProcessorEx,
  MaxThreadInfoClass
} THREADINFOCLASS;


// dbghelp.h
typedef VOID IMAGEHLP_CONTEXT, *PIMAGEHLP_CONTEXT;
#define MAX_SYM_NAME    2000
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
#define SYMOPT_CASE_INSENSITIVE         0x00000001
#define SYMOPT_UNDNAME                  0x00000002
#define SYMOPT_DEFERRED_LOADS           0x00000004
#define SYMOPT_LOAD_LINES               0x00000010
#define SYMOPT_LOAD_ANYTHING            0x00000040
#define SYMOPT_PUBLICS_ONLY             0x00004000
#define SYMOPT_NO_PUBLICS               0x00008000
#define SYMOPT_AUTO_PUBLICS             0x00010000
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
#define UNDNAME_COMPLETE                 (0x0000)
#define UNDNAME_NAME_ONLY                (0x1000)
typedef struct _TI_FINDCHILDREN_PARAMS 
{
    ULONG Count;
    ULONG Start;
    ULONG ChildId[1];
} TI_FINDCHILDREN_PARAMS;
#define SYMSEARCH_GLOBALSONLY           0x04
/* flags for SymLoadModuleEx */
#define SLMFLAG_VIRTUAL         0x1
#define SLMFLAG_NO_SYMBOLS      0x4
typedef struct _DBGHELP_MODLOAD_DATA
{
    DWORD               ssize;
    DWORD               ssig;
    PVOID               data;
    DWORD               size;
    DWORD               flags;
} MODLOAD_DATA, *PMODLOAD_DATA;
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
typedef struct _IMAGEHLP_LINE64
{
    DWORD                       SizeOfStruct;
    PVOID                       Key;
    DWORD                       LineNumber;
    PCHAR                       FileName;
    DWORD64                     Address;
} IMAGEHLP_LINE64, *PIMAGEHLP_LINE64;
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
typedef BOOL (CALLBACK* PSYM_ENUMLINES_CALLBACK)(PSRCCODEINFO, PVOID);
typedef BOOL (CALLBACK *PSYM_ENUMERATESYMBOLS_CALLBACK)(PSYMBOL_INFO, ULONG, PVOID);
BOOL WINAPI SymInitialize(HANDLE hProcess, PCSTR UserSearchPath, BOOL fInvadeProcess);
BOOL WINAPI SymCleanup(HANDLE hProcess);
BOOL WINAPI SymAddSymbolW(HANDLE hProcess, ULONG64 BaseOfDll, PCWSTR name, DWORD64 addr, DWORD size, DWORD flags);
BOOL  WINAPI SymGetModuleInfoW64(HANDLE hProcess, DWORD64 dwAddr, PIMAGEHLP_MODULEW64 ModuleInfo);
BOOL WINAPI SymMatchStringW(PCWSTR string, PCWSTR re, BOOL _case);
DWORD WINAPI SymLoadModule(HANDLE hProcess, HANDLE hFile, PCSTR ImageName,
                           PCSTR ModuleName, DWORD BaseOfDll, DWORD SizeOfDll);
DWORD64 WINAPI SymLoadModuleEx(HANDLE, HANDLE, PCSTR, PCSTR, DWORD64, DWORD,
                               PMODLOAD_DATA, DWORD);
DWORD64 WINAPI SymLoadModuleExW(HANDLE, HANDLE, PCWSTR, PCWSTR, DWORD64, DWORD,
                                PMODLOAD_DATA, DWORD);
DWORD64 WINAPI SymGetModuleBase64(HANDLE, DWORD64);
BOOL WINAPI SymUnloadModule(HANDLE hProcess, DWORD BaseOfDll);
PVOID   WINAPI SymFunctionTableAccess(HANDLE, DWORD);
PVOID WINAPI SymFunctionTableAccess64(HANDLE, DWORD64);
BOOL WINAPI SymFromAddr(HANDLE hProcess, DWORD64 Address, DWORD64* Displacement, PSYMBOL_INFO Symbol);
BOOL WINAPI SymEnumLines(HANDLE hProcess, ULONG64 base, PCSTR compiland, PCSTR srcfile, PSYM_ENUMLINES_CALLBACK cb, PVOID user);
DWORD WINAPI SymSetOptions(DWORD opts);
BOOL WINAPI SymGetLineFromAddr64(HANDLE hProcess, DWORD64 dwAddr, PDWORD pdwDisplacement, PIMAGEHLP_LINE64 Line);
typedef BOOL (CALLBACK *PFIND_EXE_FILE_CALLBACKW)(HANDLE, PCWSTR, PVOID);
#define FindExecutableImageExW __FindExecutableImageExW
HANDLE __FindExecutableImageExW(PCWSTR, PCWSTR, PWSTR, PFIND_EXE_FILE_CALLBACKW, PVOID);
DWORD WINAPI UnDecorateSymbolName(PCSTR, PSTR, DWORD, DWORD);
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
typedef enum
{
    AddrMode1616,
    AddrMode1632,
    AddrModeReal,
    AddrModeFlat
} ADDRESS_MODE;
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
typedef struct _tagADDRESS64
{
    DWORD64                     Offset;
    WORD                        Segment;
    ADDRESS_MODE                Mode;
} ADDRESS64, *LPADDRESS64;
typedef BOOL (CALLBACK *PENUMDIRTREE_CALLBACKW)(PCWSTR, PVOID);
typedef PVOID (CALLBACK *PFUNCTION_TABLE_ACCESS_ROUTINE64)(HANDLE, DWORD64);
typedef DWORD64 (CALLBACK *PGET_MODULE_BASE_ROUTINE64)(HANDLE, DWORD64);
typedef DWORD64 (CALLBACK *PTRANSLATE_ADDRESS_ROUTINE64)(HANDLE, HANDLE, LPADDRESS64);
typedef BOOL (CALLBACK *PSYMBOL_REGISTERED_CALLBACK64)(HANDLE, ULONG, ULONG64, ULONG64);
typedef BOOL (CALLBACK *PREAD_PROCESS_MEMORY_ROUTINE64)(HANDLE, DWORD64, PVOID, DWORD, PDWORD);
typedef BOOL (CALLBACK *PSYM_ENUMMODULES_CALLBACK64)(PCSTR, DWORD64, PVOID);
typedef BOOL (CALLBACK *PSYM_ENUMMODULES_CALLBACKW64)(PCWSTR, DWORD64, PVOID);
typedef BOOL (CALLBACK *PENUMLOADED_MODULES_CALLBACK64)(PCSTR, DWORD64, ULONG, PVOID);
typedef BOOL (CALLBACK *PENUMLOADED_MODULES_CALLBACKW64)(PCWSTR, DWORD64, ULONG, PVOID);
typedef BOOL (CALLBACK *PSYM_ENUMSYMBOLS_CALLBACK64)(PCSTR, DWORD64, ULONG, PVOID);
typedef BOOL (CALLBACK *PSYM_ENUMSYMBOLS_CALLBACK64W)(PCWSTR, DWORD64, ULONG, PVOID);

BOOL WINAPI SymEnumerateModulesW64(HANDLE hProcess, PSYM_ENUMMODULES_CALLBACKW64 EnumModulesCallback, PVOID UserContext);

typedef struct _tagADDRESS
{
    DWORD                       Offset;
    WORD                        Segment;
    ADDRESS_MODE                Mode;
} ADDRESS, *LPADDRESS;

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
typedef BOOL  (CALLBACK *PENUMLOADED_MODULES_CALLBACK)(PCSTR, ULONG, ULONG, PVOID);
typedef BOOL  (CALLBACK *PSYMBOL_REGISTERED_CALLBACK)(HANDLE, ULONG, PVOID, PVOID);
typedef BOOL  (CALLBACK *PREAD_PROCESS_MEMORY_ROUTINE)(HANDLE, DWORD, PVOID, DWORD, PDWORD);
typedef DWORD (CALLBACK *PTRANSLATE_ADDRESS_ROUTINE)(HANDLE, HANDLE, LPADDRESS);
typedef PVOID (CALLBACK *PFUNCTION_TABLE_ACCESS_ROUTINE)(HANDLE, DWORD);
typedef DWORD (CALLBACK *PGET_MODULE_BASE_ROUTINE)(HANDLE, DWORD);
typedef BOOL  (CALLBACK *PSYM_ENUMMODULES_CALLBACK)(PCSTR, ULONG, PVOID);
typedef BOOL  (CALLBACK *PSYM_ENUMSYMBOLS_CALLBACK)(PCSTR, ULONG, ULONG, PVOID);
typedef BOOL  (CALLBACK *PSYM_ENUMSYMBOLS_CALLBACKW)(PCWSTR, ULONG, ULONG, PVOID);

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
typedef struct _MINIDUMP_INCLUDE_THREAD_CALLBACK
{
    ULONG ThreadId;
} MINIDUMP_INCLUDE_THREAD_CALLBACK, *PMINIDUMP_INCLUDE_THREAD_CALLBACK;
typedef struct _MINIDUMP_INCLUDE_MODULE_CALLBACK 
{
    ULONG64 BaseOfImage;
} MINIDUMP_INCLUDE_MODULE_CALLBACK, *PMINIDUMP_INCLUDE_MODULE_CALLBACK;
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
typedef BOOL (CALLBACK *PSYM_ENUMSOURCEFILES_CALLBACK)(PSOURCEFILE, PVOID);
typedef BOOL (CALLBACK *PSYM_ENUMSOURCEFILES_CALLBACKW)(PSOURCEFILEW, PVOID);
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
typedef BOOL (CALLBACK* PSYM_ENUMLINES_CALLBACKW)(PSRCCODEINFOW, PVOID);
typedef BOOL (CALLBACK *PSYM_ENUMERATESYMBOLS_CALLBACKW)(PSYMBOL_INFOW, ULONG, PVOID);
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
typedef struct _IMAGEHLP_LINEW64
{
    DWORD                       SizeOfStruct;
    PVOID                       Key;
    DWORD                       LineNumber;
    PWSTR                       FileName;
    DWORD64                     Address;
} IMAGEHLP_LINEW64, *PIMAGEHLP_LINEW64;
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
typedef struct API_VERSION
{
    USHORT  MajorVersion;
    USHORT  MinorVersion;
    USHORT  Revision;
    USHORT  Reserved;
} API_VERSION, *LPAPI_VERSION;

// cvconst.h
/* symbols & types enumeration */
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
   SymTagMax
};

enum BasicType
{
    btNoType = 0,
    btVoid = 1,
    btChar = 2,
    btWChar = 3,
    btInt = 6,
    btUInt = 7,
    btFloat = 8,
    btBCD = 9,
    btBool = 10,
    btLong = 13,
    btULong = 14,
    btCurrency = 25,
    btDate = 26,
    btVariant = 27,
    btComplex = 28,
    btBit = 29,
    btBSTR = 30,
    btHresult = 31,
};

/* kind of UDT */
enum UdtKind
{
    UdtStruct,
    UdtClass,
    UdtUnion
};

/* kind of SymTagData */
enum DataKind
{
    DataIsUnknown,
    DataIsLocal,
    DataIsStaticLocal,
    DataIsParam,
    DataIsObjectPtr,
    DataIsFileStatic,
    DataIsGlobal,
    DataIsMember,
    DataIsStaticMember,
    DataIsConstant
};

/* values for registers (on different CPUs) */
enum CV_HREG_e
{
    /* those values are common to all supported CPUs (and CPU independent) */
    CV_ALLREG_ERR       = 30000,
    CV_ALLREG_TEB       = 30001,
    CV_ALLREG_TIMER     = 30002,
    CV_ALLREG_EFAD1     = 30003,
    CV_ALLREG_EFAD2     = 30004,
    CV_ALLREG_EFAD3     = 30005,
    CV_ALLREG_VFRAME    = 30006,
    CV_ALLREG_HANDLE    = 30007,
    CV_ALLREG_PARAMS    = 30008,
    CV_ALLREG_LOCALS    = 30009,
    CV_ALLREG_TID       = 30010,
    CV_ALLREG_ENV       = 30011,
    CV_ALLREG_CMDLN     = 30012,

    /* Intel x86 CPU */
    CV_REG_NONE         = 0,
    CV_REG_AL           = 1,
    CV_REG_CL           = 2,
    CV_REG_DL           = 3,
    CV_REG_BL           = 4,
    CV_REG_AH           = 5,
    CV_REG_CH           = 6,
    CV_REG_DH           = 7,
    CV_REG_BH           = 8,
    CV_REG_AX           = 9,
    CV_REG_CX           = 10,
    CV_REG_DX           = 11,
    CV_REG_BX           = 12,
    CV_REG_SP           = 13,
    CV_REG_BP           = 14,
    CV_REG_SI           = 15,
    CV_REG_DI           = 16,
    CV_REG_EAX          = 17,
    CV_REG_ECX          = 18,
    CV_REG_EDX          = 19,
    CV_REG_EBX          = 20,
    CV_REG_ESP          = 21,
    CV_REG_EBP          = 22,
    CV_REG_ESI          = 23,
    CV_REG_EDI          = 24,
    CV_REG_ES           = 25,
    CV_REG_CS           = 26,
    CV_REG_SS           = 27,
    CV_REG_DS           = 28,
    CV_REG_FS           = 29,
    CV_REG_GS           = 30,
    CV_REG_IP           = 31,
    CV_REG_FLAGS        = 32,
    CV_REG_EIP          = 33,
    CV_REG_EFLAGS       = 34,

    /* <pcode> */
    CV_REG_TEMP         = 40,
    CV_REG_TEMPH        = 41,
    CV_REG_QUOTE        = 42,
    CV_REG_PCDR3        = 43,   /* this includes PCDR4 to PCDR7 */
    CV_REG_CR0          = 80,   /* this includes CR1 to CR4 */
    CV_REG_DR0          = 90,   /* this includes DR1 to DR7 */
    /* </pcode> */

    CV_REG_GDTR         = 110,
    CV_REG_GDTL         = 111,
    CV_REG_IDTR         = 112,
    CV_REG_IDTL         = 113,
    CV_REG_LDTR         = 114,
    CV_REG_TR           = 115,

    CV_REG_PSEUDO1      = 116, /* this includes Pseudo02 to Pseudo09 */
    CV_REG_ST0          = 128, /* this includes ST1 to ST7 */
    CV_REG_CTRL         = 136,
    CV_REG_STAT         = 137,
    CV_REG_TAG          = 138,
    CV_REG_FPIP         = 139,
    CV_REG_FPCS         = 140,
    CV_REG_FPDO         = 141,
    CV_REG_FPDS         = 142,
    CV_REG_ISEM         = 143,
    CV_REG_FPEIP        = 144,
    CV_REG_FPEDO        = 145,
    CV_REG_MM0          = 146, /* this includes MM1 to MM7 */
    CV_REG_XMM0         = 154, /* this includes XMM1 to XMM7 */
    CV_REG_XMM00        = 162,
    CV_REG_XMM0L        = 194, /* this includes XMM1L to XMM7L */
    CV_REG_XMM0H        = 202, /* this includes XMM1H to XMM7H */
    CV_REG_MXCSR        = 211,
    CV_REG_EDXEAX       = 212,
    CV_REG_EMM0L        = 220,
    CV_REG_EMM0H        = 228,
    CV_REG_MM00         = 236,
    CV_REG_MM01         = 237,
    CV_REG_MM10         = 238,
    CV_REG_MM11         = 239,
    CV_REG_MM20         = 240,
    CV_REG_MM21         = 241,
    CV_REG_MM30         = 242,
    CV_REG_MM31         = 243,
    CV_REG_MM40         = 244,
    CV_REG_MM41         = 245,
    CV_REG_MM50         = 246,
    CV_REG_MM51         = 247,
    CV_REG_MM60         = 248,
    CV_REG_MM61         = 249,
    CV_REG_MM70         = 250,
    CV_REG_MM71         = 251,

    CV_REG_YMM0         = 252, /* this includes YMM1 to YMM7 */
    CV_REG_YMM0H        = 260, /* this includes YMM1H to YMM7H */
    CV_REG_YMM0I0       = 268, /* this includes YMM0I1 to YMM0I3 */
    CV_REG_YMM1I0       = 272, /* this includes YMM1I1 to YMM1I3 */
    CV_REG_YMM2I0       = 276, /* this includes YMM2I1 to YMM2I3 */
    CV_REG_YMM3I0       = 280, /* this includes YMM3I1 to YMM3I3 */
    CV_REG_YMM4I0       = 284, /* this includes YMM4I1 to YMM4I3 */
    CV_REG_YMM5I0       = 288, /* this includes YMM5I1 to YMM5I3 */
    CV_REG_YMM6I0       = 292, /* this includes YMM6I1 to YMM6I3 */
    CV_REG_YMM7I0       = 296, /* this includes YMM7I1 to YMM7I3 */
    CV_REG_YMM0F0       = 300, /* this includes YMM0F1 to YMM0F7 */
    CV_REG_YMM1F0       = 308, /* this includes YMM1F1 to YMM1F7 */
    CV_REG_YMM2F0       = 316, /* this includes YMM2F1 to YMM2F7 */
    CV_REG_YMM3F0       = 324, /* this includes YMM3F1 to YMM3F7 */
    CV_REG_YMM4F0       = 332, /* this includes YMM4F1 to YMM4F7 */
    CV_REG_YMM5F0       = 340, /* this includes YMM5F1 to YMM5F7 */
    CV_REG_YMM6F0       = 348, /* this includes YMM6F1 to YMM6F7 */
    CV_REG_YMM7F0       = 356, /* this includes YMM7F1 to YMM7F7 */
    CV_REG_YMM0D0       = 364, /* this includes YMM0D1 to YMM0D3 */
    CV_REG_YMM1D0       = 368, /* this includes YMM1D1 to YMM1D3 */
    CV_REG_YMM2D0       = 372, /* this includes YMM2D1 to YMM2D3 */
    CV_REG_YMM3D0       = 376, /* this includes YMM3D1 to YMM3D3 */
    CV_REG_YMM4D0       = 380, /* this includes YMM4D1 to YMM4D3 */
    CV_REG_YMM5D0       = 384, /* this includes YMM5D1 to YMM5D3 */
    CV_REG_YMM6D0       = 388, /* this includes YMM6D1 to YMM6D3 */
    CV_REG_YMM7D0       = 392, /* this includes YMM7D1 to YMM7D3 */

    /* Motorola 68K CPU */
    CV_R68_D0           = 0, /* this includes D1 to D7 too */
    CV_R68_A0           = 8, /* this includes A1 to A7 too */
    CV_R68_CCR          = 16,
    CV_R68_SR           = 17,
    CV_R68_USP          = 18,
    CV_R68_MSP          = 19,
    CV_R68_SFC          = 20,
    CV_R68_DFC          = 21,
    CV_R68_CACR         = 22,
    CV_R68_VBR          = 23,
    CV_R68_CAAR         = 24,
    CV_R68_ISP          = 25,
    CV_R68_PC           = 26,
    CV_R68_FPCR         = 28,
    CV_R68_FPSR         = 29,
    CV_R68_FPIAR        = 30,
    CV_R68_FP0          = 32, /* this includes FP1 to FP7 */
    CV_R68_MMUSR030     = 41,
    CV_R68_MMUSR        = 42,
    CV_R68_URP          = 43,
    CV_R68_DTT0         = 44,
    CV_R68_DTT1         = 45,
    CV_R68_ITT0         = 46,
    CV_R68_ITT1         = 47,
    CV_R68_PSR          = 51,
    CV_R68_PCSR         = 52,
    CV_R68_VAL          = 53,
    CV_R68_CRP          = 54,
    CV_R68_SRP          = 55,
    CV_R68_DRP          = 56,
    CV_R68_TC           = 57,
    CV_R68_AC           = 58,
    CV_R68_SCC          = 59,
    CV_R68_CAL          = 60,
    CV_R68_TT0          = 61,
    CV_R68_TT1          = 62,
    CV_R68_BAD0         = 64, /* this includes BAD1 to BAD7 */
    CV_R68_BAC0         = 72, /* this includes BAC1 to BAC7 */

    /* MIPS 4000 CPU */
    CV_M4_NOREG         = CV_REG_NONE,
    CV_M4_IntZERO       = 10,
    CV_M4_IntAT         = 11,
    CV_M4_IntV0         = 12,
    CV_M4_IntV1         = 13,
    CV_M4_IntA0         = 14, /* this includes IntA1 to IntA3 */
    CV_M4_IntT0         = 18, /* this includes IntT1 to IntT7 */
    CV_M4_IntS0         = 26, /* this includes IntS1 to IntS7 */
    CV_M4_IntT8         = 34,
    CV_M4_IntT9         = 35,
    CV_M4_IntKT0        = 36,
    CV_M4_IntKT1        = 37,
    CV_M4_IntGP         = 38,
    CV_M4_IntSP         = 39,
    CV_M4_IntS8         = 40,
    CV_M4_IntRA         = 41,
    CV_M4_IntLO         = 42,
    CV_M4_IntHI         = 43,
    CV_M4_Fir           = 50,
    CV_M4_Psr           = 51,
    CV_M4_FltF0         = 60, /* this includes FltF1 to Flt31 */
    CV_M4_FltFsr        = 92,

    /* Alpha AXP CPU */
    CV_ALPHA_NOREG      = CV_REG_NONE,
    CV_ALPHA_FltF0      = 10, /* this includes FltF1 to FltF31 */
    CV_ALPHA_IntV0      = 42,
    CV_ALPHA_IntT0      = 43, /* this includes T1 to T7 */
    CV_ALPHA_IntS0      = 51, /* this includes S1 to S5 */
    CV_ALPHA_IntFP      = 57,
    CV_ALPHA_IntA0      = 58, /* this includes A1 to A5 */
    CV_ALPHA_IntT8      = 64,
    CV_ALPHA_IntT9      = 65,
    CV_ALPHA_IntT10     = 66,
    CV_ALPHA_IntT11     = 67,
    CV_ALPHA_IntRA      = 68,
    CV_ALPHA_IntT12     = 69,
    CV_ALPHA_IntAT      = 70,
    CV_ALPHA_IntGP      = 71,
    CV_ALPHA_IntSP      = 72,
    CV_ALPHA_IntZERO    = 73,
    CV_ALPHA_Fpcr       = 74,
    CV_ALPHA_Fir        = 75,
    CV_ALPHA_Psr        = 76,
    CV_ALPHA_FltFsr     = 77,
    CV_ALPHA_SoftFpcr   = 78,

    /* Motorola & IBM PowerPC CPU */
    CV_PPC_GPR0         = 1, /* this includes GPR1 to GPR31 */
    CV_PPC_CR           = 33,
    CV_PPC_CR0          = 34, /* this includes CR1 to CR7 */
    CV_PPC_FPR0         = 42, /* this includes FPR1 to FPR31 */

    CV_PPC_FPSCR        = 74,
    CV_PPC_MSR          = 75,
    CV_PPC_SR0          = 76, /* this includes SR1 to SR15 */
    CV_PPC_PC           = 99,
    CV_PPC_MQ           = 100,
    CV_PPC_XER          = 101,
    CV_PPC_RTCU         = 104,
    CV_PPC_RTCL         = 105,
    CV_PPC_LR           = 108,
    CV_PPC_CTR          = 109,
    CV_PPC_COMPARE      = 110,
    CV_PPC_COUNT        = 111,
    CV_PPC_DSISR        = 118,
    CV_PPC_DAR          = 119,
    CV_PPC_DEC          = 122,
    CV_PPC_SDR1         = 125,
    CV_PPC_SRR0         = 126,
    CV_PPC_SRR1         = 127,
    CV_PPC_SPRG0        = 372, /* this includes SPRG1 to SPRG3 */
    CV_PPC_ASR          = 280,
    CV_PPC_EAR          = 382,
    CV_PPC_PVR          = 287,
    CV_PPC_BAT0U        = 628,
    CV_PPC_BAT0L        = 629,
    CV_PPC_BAT1U        = 630,
    CV_PPC_BAT1L        = 631,
    CV_PPC_BAT2U        = 632,
    CV_PPC_BAT2L        = 633,
    CV_PPC_BAT3U        = 634,
    CV_PPC_BAT3L        = 635,
    CV_PPC_DBAT0U       = 636,
    CV_PPC_DBAT0L       = 637,
    CV_PPC_DBAT1U       = 638,
    CV_PPC_DBAT1L       = 639,
    CV_PPC_DBAT2U       = 640,
    CV_PPC_DBAT2L       = 641,
    CV_PPC_DBAT3U       = 642,
    CV_PPC_DBAT3L       = 643,
    CV_PPC_PMR0         = 1044, /* this includes PMR1 to PMR15 */
    CV_PPC_DMISS        = 1076,
    CV_PPC_DCMP         = 1077,
    CV_PPC_HASH1        = 1078,
    CV_PPC_HASH2        = 1079,
    CV_PPC_IMISS        = 1080,
    CV_PPC_ICMP         = 1081,
    CV_PPC_RPA          = 1082,
    CV_PPC_HID0         = 1108, /* this includes HID1 to HID15 */

    /* Java */
    CV_JAVA_PC          = 1,

    /* Hitachi SH3 CPU */
    CV_SH3_NOREG        = CV_REG_NONE,
    CV_SH3_IntR0        = 10, /* this include R1 to R13 */
    CV_SH3_IntFp        = 24,
    CV_SH3_IntSp        = 25,
    CV_SH3_Gbr          = 38,
    CV_SH3_Pr           = 39,
    CV_SH3_Mach         = 40,
    CV_SH3_Macl         = 41,
    CV_SH3_Pc           = 50,
    CV_SH3_Sr           = 51,
    CV_SH3_BarA         = 60,
    CV_SH3_BasrA        = 61,
    CV_SH3_BamrA        = 62,
    CV_SH3_BbrA         = 63,
    CV_SH3_BarB         = 64,
    CV_SH3_BasrB        = 65,
    CV_SH3_BamrB        = 66,
    CV_SH3_BbrB         = 67,
    CV_SH3_BdrB         = 68,
    CV_SH3_BdmrB        = 69,
    CV_SH3_Brcr         = 70,
    CV_SH_Fpscr         = 75,
    CV_SH_Fpul          = 76,
    CV_SH_FpR0          = 80, /* this includes FpR1 to FpR15 */
    CV_SH_XFpR0         = 96, /* this includes XFpR1 to XXFpR15 */

    /* ARM CPU */
    CV_ARM_NOREG        = CV_REG_NONE,
    CV_ARM_R0           = 10, /* this includes R1 to R12 */
    CV_ARM_SP           = 23,
    CV_ARM_LR           = 24,
    CV_ARM_PC           = 25,
    CV_ARM_CPSR         = 26,
    CV_ARM_ACC0         = 27,
    CV_ARM_FPSCR        = 40,
    CV_ARM_FPEXC        = 41,
    CV_ARM_FS0          = 50, /* this includes FS1 to FS31 */
    CV_ARM_FPEXTRA0     = 90, /* this includes FPEXTRA1 to FPEXTRA7 */
    CV_ARM_WR0          = 128, /* this includes WR1 to WR15 */
    CV_ARM_WCID         = 144,
    CV_ARM_WCON         = 145,
    CV_ARM_WCSSF        = 146,
    CV_ARM_WCASF        = 147,
    CV_ARM_WC4          = 148,
    CV_ARM_WC5          = 149,
    CV_ARM_WC6          = 150,
    CV_ARM_WC7          = 151,
    CV_ARM_WCGR0        = 152, /* this includes WCGR1 to WCGR3 */
    CV_ARM_WC12         = 156,
    CV_ARM_WC13         = 157,
    CV_ARM_WC14         = 158,
    CV_ARM_WC15         = 159,
    CV_ARM_FS32         = 200, /* this includes FS33 to FS63 */
    CV_ARM_ND0          = 300, /* this includes ND1 to ND31 */
    CV_ARM_NQ0          = 400, /* this includes NQ1 to NQ15 */

    /* Intel IA64 CPU */
    CV_IA64_NOREG       = CV_REG_NONE,
    CV_IA64_Br0         = 512, /* this includes Br1 to Br7 */
    CV_IA64_P0          = 704, /* this includes P1 to P63 */
    CV_IA64_Preds       = 768,
    CV_IA64_IntH0       = 832, /* this includes H1 to H15 */
    CV_IA64_Ip          = 1016,
    CV_IA64_Umask       = 1017,
    CV_IA64_Cfm         = 1018,
    CV_IA64_Psr         = 1019,
    CV_IA64_Nats        = 1020,
    CV_IA64_Nats2       = 1021,
    CV_IA64_Nats3       = 1022,
    CV_IA64_IntR0       = 1024, /* this includes R1 to R127 */
    CV_IA64_FltF0       = 2048, /* this includes FltF1 to FltF127 */
    /* some IA64 registers missing */

    /* TriCore CPU */
    CV_TRI_NOREG        = CV_REG_NONE,
    CV_TRI_D0           = 10, /* includes D1 to D15 */
    CV_TRI_A0           = 26, /* includes A1 to A15 */
    CV_TRI_E0           = 42,
    CV_TRI_E2           = 43,
    CV_TRI_E4           = 44,
    CV_TRI_E6           = 45,
    CV_TRI_E8           = 46,
    CV_TRI_E10          = 47,
    CV_TRI_E12          = 48,
    CV_TRI_E14          = 49,
    CV_TRI_EA0          = 50,
    CV_TRI_EA2          = 51,
    CV_TRI_EA4          = 52,
    CV_TRI_EA6          = 53,
    CV_TRI_EA8          = 54,
    CV_TRI_EA10         = 55,
    CV_TRI_EA12         = 56,
    CV_TRI_EA14         = 57,
    CV_TRI_PSW          = 58,
    CV_TRI_PCXI         = 59,
    CV_TRI_PC           = 60,
    CV_TRI_FCX          = 61,
    CV_TRI_LCX          = 62,
    CV_TRI_ISP          = 63,
    CV_TRI_ICR          = 64,
    CV_TRI_BIV          = 65,
    CV_TRI_BTV          = 66,
    CV_TRI_SYSCON       = 67,
    CV_TRI_DPRx_0       = 68, /* includes DPRx_1 to DPRx_3 */
    CV_TRI_CPRx_0       = 68, /* includes CPRx_1 to CPRx_3 */
    CV_TRI_DPMx_0       = 68, /* includes DPMx_1 to DPMx_3 */
    CV_TRI_CPMx_0       = 68, /* includes CPMx_1 to CPMx_3 */
    CV_TRI_DBGSSR       = 72,
    CV_TRI_EXEVT        = 73,
    CV_TRI_SWEVT        = 74,
    CV_TRI_CREVT        = 75,
    CV_TRI_TRnEVT       = 76,
    CV_TRI_MMUCON       = 77,
    CV_TRI_ASI          = 78,
    CV_TRI_TVA          = 79,
    CV_TRI_TPA          = 80,
    CV_TRI_TPX          = 81,
    CV_TRI_TFA          = 82,

    /* AM33 (and the likes) CPU */
    CV_AM33_NOREG       = CV_REG_NONE,
    CV_AM33_E0          = 10, /* this includes E1 to E7 */
    CV_AM33_A0          = 20, /* this includes A1 to A3 */
    CV_AM33_D0          = 30, /* this includes D1 to D3 */
    CV_AM33_FS0         = 40, /* this includes FS1 to FS31 */
    CV_AM33_SP          = 80,
    CV_AM33_PC          = 81,
    CV_AM33_MDR         = 82,
    CV_AM33_MDRQ        = 83,
    CV_AM33_MCRH        = 84,
    CV_AM33_MCRL        = 85,
    CV_AM33_MCVF        = 86,
    CV_AM33_EPSW        = 87,
    CV_AM33_FPCR        = 88,
    CV_AM33_LIR         = 89,
    CV_AM33_LAR         = 90,

    /* Mitsubishi M32R CPU */
    CV_M32R_NOREG       = CV_REG_NONE,
    CV_M32R_R0          = 10, /* this includes R1 to R11 */
    CV_M32R_R12         = 22,
    CV_M32R_R13         = 23,
    CV_M32R_R14         = 24,
    CV_M32R_R15         = 25,
    CV_M32R_PSW         = 26,
    CV_M32R_CBR         = 27,
    CV_M32R_SPI         = 28,
    CV_M32R_SPU         = 29,
    CV_M32R_SPO         = 30,
    CV_M32R_BPC         = 31,
    CV_M32R_ACHI        = 32,
    CV_M32R_ACLO        = 33,
    CV_M32R_PC          = 34,

    /* AMD/Intel x86_64 CPU */
    CV_AMD64_NONE       = CV_REG_NONE,
    CV_AMD64_AL         = CV_REG_AL,
    CV_AMD64_CL         = CV_REG_CL,
    CV_AMD64_DL         = CV_REG_DL,
    CV_AMD64_BL         = CV_REG_BL,
    CV_AMD64_AH         = CV_REG_AH,
    CV_AMD64_CH         = CV_REG_CH,
    CV_AMD64_DH         = CV_REG_DH,
    CV_AMD64_BH         = CV_REG_BH,
    CV_AMD64_AX         = CV_REG_AX,
    CV_AMD64_CX         = CV_REG_CX,
    CV_AMD64_DX         = CV_REG_DX,
    CV_AMD64_BX         = CV_REG_BX,
    CV_AMD64_SP         = CV_REG_SP,
    CV_AMD64_BP         = CV_REG_BP,
    CV_AMD64_SI         = CV_REG_SI,
    CV_AMD64_DI         = CV_REG_DI,
    CV_AMD64_EAX        = CV_REG_EAX,
    CV_AMD64_ECX        = CV_REG_ECX,
    CV_AMD64_EDX        = CV_REG_EDX,
    CV_AMD64_EBX        = CV_REG_EBX,
    CV_AMD64_ESP        = CV_REG_ESP,
    CV_AMD64_EBP        = CV_REG_EBP,
    CV_AMD64_ESI        = CV_REG_ESI,
    CV_AMD64_EDI        = CV_REG_EDI,
    CV_AMD64_ES         = CV_REG_ES,
    CV_AMD64_CS         = CV_REG_CS,
    CV_AMD64_SS         = CV_REG_SS,
    CV_AMD64_DS         = CV_REG_DS,
    CV_AMD64_FS         = CV_REG_FS,
    CV_AMD64_GS         = CV_REG_GS,
    CV_AMD64_FLAGS      = CV_REG_FLAGS,
    CV_AMD64_RIP        = CV_REG_EIP,
    CV_AMD64_EFLAGS     = CV_REG_EFLAGS,

    /* <pcode> */
    CV_AMD64_TEMP       = CV_REG_TEMP,
    CV_AMD64_TEMPH      = CV_REG_TEMPH,
    CV_AMD64_QUOTE      = CV_REG_QUOTE,
    CV_AMD64_PCDR3      = CV_REG_PCDR3, /* this includes PCDR4 to PCDR7 */
    CV_AMD64_CR0        = CV_REG_CR0,   /* this includes CR1 to CR4 */
    CV_AMD64_DR0        = CV_REG_DR0,   /* this includes DR1 to DR7 */
    /* </pcode> */

    CV_AMD64_GDTR       = CV_REG_GDTR,
    CV_AMD64_GDTL       = CV_REG_GDTL,
    CV_AMD64_IDTR       = CV_REG_IDTR,
    CV_AMD64_IDTL       = CV_REG_IDTL,
    CV_AMD64_LDTR       = CV_REG_LDTR,
    CV_AMD64_TR         = CV_REG_TR,

    CV_AMD64_PSEUDO1    = CV_REG_PSEUDO1, /* this includes Pseudo02 to Pseudo09 */
    CV_AMD64_ST0        = CV_REG_ST0,     /* this includes ST1 to ST7 */
    CV_AMD64_CTRL       = CV_REG_CTRL,
    CV_AMD64_STAT       = CV_REG_STAT,
    CV_AMD64_TAG        = CV_REG_TAG,
    CV_AMD64_FPIP       = CV_REG_FPIP,
    CV_AMD64_FPCS       = CV_REG_FPCS,
    CV_AMD64_FPDO       = CV_REG_FPDO,
    CV_AMD64_FPDS       = CV_REG_FPDS,
    CV_AMD64_ISEM       = CV_REG_ISEM,
    CV_AMD64_FPEIP      = CV_REG_FPEIP,
    CV_AMD64_FPEDO      = CV_REG_FPEDO,
    CV_AMD64_MM0        = CV_REG_MM0,     /* this includes MM1 to MM7 */
    CV_AMD64_XMM0       = CV_REG_XMM0,    /* this includes XMM1 to XMM7 */
    CV_AMD64_XMM00      = CV_REG_XMM00,
    CV_AMD64_XMM0L      = CV_REG_XMM0L,   /* this includes XMM1L to XMM7L */
    CV_AMD64_XMM0H      = CV_REG_XMM0H,   /* this includes XMM1H to XMM7H */
    CV_AMD64_MXCSR      = CV_REG_MXCSR,
    CV_AMD64_EDXEAX     = CV_REG_EDXEAX,
    CV_AMD64_EMM0L      = CV_REG_EMM0L,
    CV_AMD64_EMM0H      = CV_REG_EMM0H,
    CV_AMD64_MM00       = CV_REG_MM00,
    CV_AMD64_MM01       = CV_REG_MM01,
    CV_AMD64_MM10       = CV_REG_MM10,
    CV_AMD64_MM11       = CV_REG_MM11,
    CV_AMD64_MM20       = CV_REG_MM20,
    CV_AMD64_MM21       = CV_REG_MM21,
    CV_AMD64_MM30       = CV_REG_MM30,
    CV_AMD64_MM31       = CV_REG_MM31,
    CV_AMD64_MM40       = CV_REG_MM40,
    CV_AMD64_MM41       = CV_REG_MM41,
    CV_AMD64_MM50       = CV_REG_MM50,
    CV_AMD64_MM51       = CV_REG_MM51,
    CV_AMD64_MM60       = CV_REG_MM60,
    CV_AMD64_MM61       = CV_REG_MM61,
    CV_AMD64_MM70       = CV_REG_MM70,
    CV_AMD64_MM71       = CV_REG_MM71,

    CV_AMD64_XMM8       = 252,           /* this includes XMM9 to XMM15 */

    CV_AMD64_RAX        = 328,
    CV_AMD64_RBX        = 329,
    CV_AMD64_RCX        = 330,
    CV_AMD64_RDX        = 331,
    CV_AMD64_RSI        = 332,
    CV_AMD64_RDI        = 333,
    CV_AMD64_RBP        = 334,
    CV_AMD64_RSP        = 335,

    CV_AMD64_R8         = 336,
    CV_AMD64_R9         = 337,
    CV_AMD64_R10        = 338,
    CV_AMD64_R11        = 339,
    CV_AMD64_R12        = 340,
    CV_AMD64_R13        = 341,
    CV_AMD64_R14        = 342,
    CV_AMD64_R15        = 343,

    /* Wine extension */
    CV_ARM64_NOREG        = CV_REG_NONE,
    CV_ARM64_X0           = 10, /* this includes X0 to X30 */
    CV_ARM64_SP           = 41,
    CV_ARM64_PC           = 42,
    CV_ARM64_PSTATE       = 43,
};

typedef enum
{
   THUNK_ORDINAL_NOTYPE,
   THUNK_ORDINAL_ADJUSTOR,
   THUNK_ORDINAL_VCALL,
   THUNK_ORDINAL_PCODE,
   THUNK_ORDINAL_LOAD 
} THUNK_ORDINAL;

typedef enum CV_call_e
{
    CV_CALL_NEAR_C,
    CV_CALL_FAR_C,
    CV_CALL_NEAR_PASCAL,
    CV_CALL_FAR_PASCAL,
    CV_CALL_NEAR_FAST,
    CV_CALL_FAR_FAST,
    CV_CALL_SKIPPED,
    CV_CALL_NEAR_STD,
    CV_CALL_FAR_STD,
    CV_CALL_NEAR_SYS,
    CV_CALL_FAR_SYS,
    CV_CALL_THISCALL,
    CV_CALL_MIPSCALL,
    CV_CALL_GENERIC,
    CV_CALL_ALPHACALL,
    CV_CALL_PPCCALL,
    CV_CALL_SHCALL,
    CV_CALL_ARMCALL,
    CV_CALL_AM33CALL,
    CV_CALL_TRICALL,
    CV_CALL_SH5CALL,
    CV_CALL_M32RCALL,
    CV_CALL_RESERVED,
} CV_call_e;


// wtypes.h
typedef LONG SCODE;
typedef double DATE;
typedef unsigned short VARTYPE;
typedef union tagCY {
    struct {
#ifdef WORDS_BIGENDIAN
        LONG  Hi;
        ULONG Lo;
#else
        ULONG Lo;
        LONG  Hi;
#endif
    } DUMMYSTRUCTNAME;
    LONGLONG int64;
} CY;
typedef struct tagDEC {
  USHORT wReserved;
  union {
    struct {
      BYTE scale;
      BYTE sign;
    } DUMMYSTRUCTNAME;
    USHORT signscale;
  } DUMMYUNIONNAME;
  ULONG Hi32;
  union {
    struct {
#ifdef WORDS_BIGENDIAN
      ULONG Mid32;
      ULONG Lo32;
#else
      ULONG Lo32;
      ULONG Mid32;
#endif
    } DUMMYSTRUCTNAME1;
    ULONGLONG Lo64;
  } DUMMYUNIONNAME1;
} DECIMAL;
typedef short VARIANT_BOOL;
typedef VARIANT_BOOL _VARIANT_BOOL;
typedef WCHAR OLECHAR;
typedef OLECHAR *BSTR;
enum VARENUM {
    VT_EMPTY = 0,
    VT_NULL = 1,
    VT_I2 = 2,
    VT_I4 = 3,
    VT_R4 = 4,
    VT_R8 = 5,
    VT_CY = 6,
    VT_DATE = 7,
    VT_BSTR = 8,
    VT_DISPATCH = 9,
    VT_ERROR = 10,
    VT_BOOL = 11,
    VT_VARIANT = 12,
    VT_UNKNOWN = 13,
    VT_DECIMAL = 14,
    VT_I1 = 16,
    VT_UI1 = 17,
    VT_UI2 = 18,
    VT_UI4 = 19,
    VT_I8 = 20,
    VT_UI8 = 21,
    VT_INT = 22,
    VT_UINT = 23,
    VT_VOID = 24,
    VT_HRESULT = 25,
    VT_PTR = 26,
    VT_SAFEARRAY = 27,
    VT_CARRAY = 28,
    VT_USERDEFINED = 29,
    VT_LPSTR = 30,
    VT_LPWSTR = 31,
    VT_RECORD = 36,
    VT_INT_PTR = 37,
    VT_UINT_PTR = 38,
    VT_FILETIME = 64,
    VT_BLOB = 65,
    VT_STREAM = 66,
    VT_STORAGE = 67,
    VT_STREAMED_OBJECT = 68,
    VT_STORED_OBJECT = 69,
    VT_BLOB_OBJECT = 70,
    VT_CF = 71,
    VT_CLSID = 72,
    VT_VERSIONED_STREAM = 73,
    VT_BSTR_BLOB = 0xfff,
    VT_VECTOR = 0x1000,
    VT_ARRAY = 0x2000,
    VT_BYREF = 0x4000,
    VT_RESERVED = 0x8000,
    VT_ILLEGAL = 0xffff,
    VT_ILLEGALMASKED = 0xfff,
    VT_TYPEMASK = 0xfff
};

// oaidl.h
typedef struct tagSAFEARRAYBOUND {
    ULONG cElements;
    LONG lLbound;
} SAFEARRAYBOUND;
typedef struct tagSAFEARRAY {
    USHORT cDims;
    USHORT fFeatures;
    ULONG cbElements;
    ULONG cLocks;
    PVOID pvData;
    SAFEARRAYBOUND rgsabound[1];
} SAFEARRAY;
typedef SAFEARRAY *LPSAFEARRAY;

#if (__STDC__ && !defined(_FORCENAMELESSUNION)) || defined(NONAMELESSUNION)
#define __VARIANT_NAME_1 n1
#define __VARIANT_NAME_2 n2
#define __VARIANT_NAME_3 n3
#define __VARIANT_NAME_4 brecVal
#else
#define __tagVARIANT
#define __VARIANT_NAME_1
#define __VARIANT_NAME_2
#define __VARIANT_NAME_3
#define __tagBRECORD
#define __VARIANT_NAME_4
#endif
typedef struct tagVARIANT VARIANT;
struct tagVARIANT {
    union {
        struct __tagVARIANT {
            VARTYPE vt;
            WORD wReserved1;
            WORD wReserved2;
            WORD wReserved3;
            union {
                signed char cVal;
                USHORT uiVal;
                ULONG ulVal;
                INT intVal;
                UINT uintVal;
                BYTE bVal;
                SHORT iVal;
                LONG lVal;
                FLOAT fltVal;
                DOUBLE dblVal;
                VARIANT_BOOL boolVal;
                SCODE scode;
                DATE date;
                BSTR bstrVal;
                CY cyVal;
                IUnknown *punkVal;
                IDispatch *pdispVal;
                SAFEARRAY *parray;
                LONGLONG llVal;
                ULONGLONG ullVal;
                signed char *pcVal;
                USHORT *puiVal;
                ULONG *pulVal;
                INT *pintVal;
                UINT *puintVal;
                BYTE *pbVal;
                SHORT *piVal;
                LONG *plVal;
                FLOAT *pfltVal;
                DOUBLE *pdblVal;
                VARIANT_BOOL *pboolVal;
                SCODE *pscode;
                DATE *pdate;
                BSTR *pbstrVal;
                VARIANT *pvarVal;
                PVOID byref;
                CY *pcyVal;
                DECIMAL *pdecVal;
                IUnknown **ppunkVal;
                IDispatch **ppdispVal;
                SAFEARRAY **pparray;
                LONGLONG *pllVal;
                ULONGLONG *pullVal;
                struct __tagBRECORD {
                    PVOID pvRecord;
                    IRecordInfo *pRecInfo;
                } __VARIANT_NAME_4;
            } __VARIANT_NAME_3;
        } __VARIANT_NAME_2;
        DECIMAL decVal;
    } __VARIANT_NAME_1;
};

typedef VARIANT *LPVARIANT;
typedef VARIANT VARIANTARG;
typedef VARIANTARG *LPVARIANTARG;

// wine/windef16.h
typedef DWORD SEGPTR;

// wine/winbase16.h
typedef struct _STACK32FRAME
{
    DWORD   restore_addr;   /* 00 return address for restoring code selector */
    DWORD   codeselector;   /* 04 code selector to restore */
    EXCEPTION_REGISTRATION_RECORD frame;  /* 08 Exception frame */
    SEGPTR  frame16;        /* 10 16-bit frame from last CallFrom16() */
    DWORD   edi;            /* 14 saved registers */
    DWORD   esi;            /* 18 */
    DWORD   ebx;            /* 1c */
    DWORD   ebp;            /* 20 saved 32-bit frame pointer */
    DWORD   retaddr;        /* 24 return address */
    DWORD   target;         /* 28 target address / CONTEXT86 pointer */
    DWORD   nb_args;        /* 2c number of 16-bit argument bytes */
} STACK32FRAME;

/* 16-bit stack layout after __wine_call_from_16() */
typedef struct _STACK16FRAME
{
    STACK32FRAME *frame32;        /* 00 32-bit frame from last CallTo16() */
    DWORD         edx;            /* 04 saved registers */
    DWORD         ecx;            /* 08 */
    DWORD         ebp;            /* 0c */
    WORD          ds;             /* 10 */
    WORD          es;             /* 12 */
    WORD          fs;             /* 14 */
    WORD          gs;             /* 16 */
    DWORD         callfrom_ip;    /* 18 callfrom tail IP */
    DWORD         module_cs;      /* 1c module code segment */
    DWORD         relay;          /* 20 relay function address */
    WORD          entry_ip;       /* 22 entry point IP */
    DWORD         entry_point;    /* 26 API entry point to call, reused as mutex count */
    WORD          bp;             /* 2a 16-bit stack frame chain */
    WORD          ip;             /* 2c return address */
    WORD          cs;             /* 2e */
} STACK16FRAME;
