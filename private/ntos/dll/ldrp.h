/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ldrp.h

Abstract:

    Private types... for executive portion of loader

Author:

    Mark Lucovsky (markl) 26-Mar-1990

Revision History:

--*/

#ifndef _LDRP_
#define _LDRP_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <string.h>
#define NOEXTAPI
#include "wdbgexts.h"
#include <ntdbg.h>

extern BOOLEAN LdrpImageHasTls;
extern UNICODE_STRING LdrpDefaultPath;
HANDLE LdrpKnownDllObjectDirectory;
#define LDRP_MAX_KNOWN_PATH 128
WCHAR LdrpKnownDllPathBuffer[LDRP_MAX_KNOWN_PATH];
UNICODE_STRING LdrpKnownDllPath;



#if defined(WX86)

extern BOOLEAN (*Wx86ProcessInit)(PVOID, BOOLEAN);

BOOLEAN
LdrpWx86DllMapNotify(
     PVOID DllBase,
     BOOLEAN Mapped
     );

PLDR_DATA_TABLE_ENTRY
LdrpWx86CheckForLoadedDll(
    IN PWSTR DllPath,
    IN PUNICODE_STRING DllName,
    IN BOOLEAN Wx86KnownDll,
    OUT PUNICODE_STRING FullDllName
    );


NTSTATUS
LdrpWx86MapDll(
    IN PWSTR DllPath OPTIONAL,
    IN PULONG DllCharacteristics OPTIONAL,
    IN BOOLEAN Wx86KnownDll,
    IN BOOLEAN StaticLink,
    OUT PUNICODE_STRING DllName,
    OUT PLDR_DATA_TABLE_ENTRY *pEntry,
    OUT SIZE_T *pViewSize,
    OUT HANDLE *pSection
    );

NTSTATUS
LdrpRunWx86DllEntryPoint(
    IN  PDLL_INIT_ROUTINE InitRoutine,
    OUT BOOLEAN *pInitStatus,
    IN  PVOID DllBase,
    IN  ULONG Reason,
    IN  PCONTEXT Context
    );

NTSTATUS
LdrpLoadWx86Dll(
    IN PCONTEXT Context
    );

NTSTATUS
LdrpInitWx86(
    IN PWX86TIB Wx86Tib,
    IN PCONTEXT Context,
    IN BOOLEAN NewThread
    );

VOID
LdrpWx86DllProcessDetach(
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    );

#define WX86PLUGIN_MAXPROVIDER 8                // maximum # of providers per plugin

NTSTATUS
Wx86IdentifyPlugin(
    IN PVOID ViewBase,
    IN PUNICODE_STRING FullDllName
    );

NTSTATUS
Wx86ThunkPluginExport(
    IN PVOID DllBase,
    IN PCHAR ExportName,
    IN ULONG Ordinal,
    IN PVOID ExportAddress,
    OUT PVOID *ExportThunk
    );

BOOLEAN
Wx86UnloadProviders(
    IN PVOID DllBase
    );
#endif

#if defined (_ALPHA_) || defined (BUILD_WOW6432)
NTSTATUS
LdrpWx86FormatVirtualImage(
    IN PIMAGE_NT_HEADERS32 NtHeaders,
    IN PVOID DllBase
    );

NTSTATUS
Wx86SetRelocatedSharedProtection (
    IN PVOID Base,
    IN BOOLEAN Reset
    );

ULONG
LdrpWx86RelocatedFixupDiff(
    IN PUCHAR ImageBase,
    IN ULONG  Offset
    );

BOOLEAN
LdrpWx86DllHasRelocatedSharedSection(
    IN PUCHAR ImageBase);


#endif

#if defined (BUILD_WOW6432)
#define NATIVE_PAGE_SIZE  0x2000
#define NATIVE_PAGE_SHIFT 13L
#define NATIVE_BYTES_TO_PAGES(Size)  ((ULONG)((ULONG_PTR)(Size) >> NATIVE_PAGE_SHIFT) + \
                                    (((ULONG)(Size) & (NATIVE_PAGE_SIZE - 1)) != 0))
#else
#define NATIVE_PAGE_SIZE  PAGE_SIZE
#define NATIVE_PAGE_SHIFT PAGE_SHIFT
#define NATIVE_BYTES_TO_PAGES(Size) BYTES_TO_PAGES(Size)
#endif


#define LDRP_HASH_TABLE_SIZE 32
#define LDRP_HASH_MASK       (LDRP_HASH_TABLE_SIZE-1)
#define LDRP_COMPUTE_HASH_INDEX(wch) ( (RtlUpcaseUnicodeChar((wch)) - (WCHAR)'A') & LDRP_HASH_MASK )
LIST_ENTRY LdrpHashTable[LDRP_HASH_TABLE_SIZE];


// LDRP_BAD_DLL Sundown: sign-extended value.
#define LDRP_BAD_DLL LongToPtr(0xffbadd11)

LIST_ENTRY LdrpDefaultPathCache;
typedef struct _LDRP_PATH_CACHE {
    LIST_ENTRY Links;
    UNICODE_STRING Component;
    HANDLE Directory;
} LDRP_PATH_CACHE, *PLDRP_PATH_CACHE;


NTSTATUS
LdrpSnapIAT(
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry_Export,
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry_Import,
    IN PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor,
    IN BOOLEAN SnapForwardersOnly
    );

NTSTATUS
LdrpSnapLinksToDllHandle(
    IN PVOID DllHandle,
    IN ULONG NumberOfThunks,
    IN OUT PIMAGE_THUNK_DATA FirstThunk
    );

NTSTATUS
LdrpSnapThunk(
    IN PVOID DllBase,
    IN PVOID ImageBase,
    IN PIMAGE_THUNK_DATA OriginalThunk,
    IN OUT PIMAGE_THUNK_DATA Thunk,
    IN PIMAGE_EXPORT_DIRECTORY ExportDirectory,
    IN ULONG ExportSize,
    IN BOOLEAN StaticSnap,
    IN PSZ DllName OPTIONAL
    );

USHORT
LdrpNameToOrdinal(
    IN PSZ Name,
    IN ULONG NumberOfNames,
    IN PVOID DllBase,
    IN PULONG NameTableBase,
    IN PUSHORT NameOrdinalTableBase
    );

PLDR_DATA_TABLE_ENTRY
LdrpAllocateDataTableEntry(
    IN PVOID DllBase
    );

BOOLEAN
LdrpCheckForLoadedDll(
    IN PWSTR DllPath OPTIONAL,
    IN PUNICODE_STRING DllName,
    IN BOOLEAN StaticLink,
    IN BOOLEAN Wx86KnownDll,
    OUT PLDR_DATA_TABLE_ENTRY *LdrDataTableEntry
    );

BOOLEAN
LdrpCheckForLoadedDllHandle(
    IN PVOID DllHandle,
    OUT PLDR_DATA_TABLE_ENTRY *LdrDataTableEntry
    );

NTSTATUS
LdrpMapDll(
    IN PWSTR DllPath OPTIONAL,
    IN PWSTR DllName,
    IN PULONG DllCharacteristics OPTIONAL,
    IN BOOLEAN StaticLink,
    IN BOOLEAN Wx86KnownDll,
    OUT PLDR_DATA_TABLE_ENTRY *LdrDataTableEntry
    );

NTSTATUS
LdrpWalkImportDescriptor(
    IN PWSTR DllPath OPTIONAL,
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    );

NTSTATUS
LdrpRunInitializeRoutines(
    IN PCONTEXT Context OPTIONAL
    );

#define LdrpReferenceLoadedDll( lde ) LdrpUpdateLoadCount( lde, TRUE )
#define LdrpDereferenceLoadedDll( lde ) LdrpUpdateLoadCount( lde, FALSE )

VOID
LdrpUpdateLoadCount (
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry,
    IN BOOLEAN IncrementCount
    );

NTSTATUS
LdrpInitializeProcess(
    IN PCONTEXT Context OPTIONAL,
    IN PVOID SystemDllBase,
    IN PUNICODE_STRING UnicodeImageName
    );

VOID
LdrpInitialize(
    IN PCONTEXT Context,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
LdrpInsertMemoryTableEntry(
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    );

BOOLEAN
LdrpResolveDllName(
    IN PWSTR DllPath OPTIONAL,
    IN PWSTR DllName,
    OUT PUNICODE_STRING FullDllName,
    OUT PUNICODE_STRING BaseDllName,
    OUT PHANDLE DllFile
    );

NTSTATUS
LdrpCreateDllSection(
    IN PUNICODE_STRING FullDllName,
    IN HANDLE DllFile,
    IN PUNICODE_STRING BaseName,
    IN PULONG DllCharacteristics OPTIONAL,
    OUT PHANDLE SectionHandle
    );

VOID
LdrpInitializePathCache(
    VOID
    );

PVOID
LdrpFetchAddressOfEntryPoint(
    IN PVOID Base
    );

BOOLEAN
xRtlDosPathNameToNtPathName(
    IN PSZ DosFileName,
    OUT PSTRING NtFileName,
    OUT PSZ *FilePart OPTIONAL,
    OUT PRTL_RELATIVE_NAME RelativeName OPTIONAL
    );

ULONG
xRtlDosSearchPath(
    PSZ lpPath,
    PSZ lpFileName,
    PSZ lpExtension,
    ULONG nBufferLength,
    PSZ lpBuffer,
    PSZ *lpFilePart OPTIONAL
    );

ULONG
xRtlGetFullPathName(
    PSZ lpFileName,
    ULONG nBufferLength,
    PSZ lpBuffer,
    PSZ *lpFilePart OPTIONAL
    );

PSZ
UnicodeToAnsii(
    IN PWSTR String
    );

HANDLE
LdrpCheckForKnownDll(
    IN PWSTR DllName,
    OUT PUNICODE_STRING FullDllName,
    OUT PUNICODE_STRING BaseDllName
    );

NTSTATUS
LdrpSetProtection(
    IN PVOID Base,
    IN BOOLEAN Reset,
    IN BOOLEAN StaticLink
    );

#if DBG
ULONG LdrpCompareCount;
ULONG LdrpSnapBypass;
ULONG LdrpNormalSnap;
ULONG LdrpSectionOpens;
ULONG LdrpSectionCreates;
ULONG LdrpSectionMaps;
ULONG LdrpSectionRelocates;
BOOLEAN LdrpDisplayLoadTime;
LARGE_INTEGER BeginTime, InitcTime, InitbTime, IniteTime, EndTime, ElapsedTime, Interval;

#endif // DBG

BOOLEAN ShowSnaps;
BOOLEAN RtlpTimoutDisable;
LARGE_INTEGER RtlpTimeout;
ULONG NtGlobalFlag;
LIST_ENTRY RtlCriticalSectionList;
RTL_CRITICAL_SECTION RtlCriticalSectionLock;
BOOLEAN LdrpShutdownInProgress;
extern BOOLEAN LdrpInLdrInit;
extern BOOLEAN LdrpLdrDatabaseIsSetup;
extern BOOLEAN LdrpVerifyDlls;
extern BOOLEAN LdrpShutdownInProgress;
extern BOOLEAN LdrpImageHasTls;
extern BOOLEAN LdrpVerifyDlls;

PLDR_DATA_TABLE_ENTRY LdrpImageEntry;
LIST_ENTRY LdrpUnloadHead;
BOOLEAN LdrpActiveUnloadCount;
PLDR_DATA_TABLE_ENTRY LdrpGetModuleHandleCache;
PLDR_DATA_TABLE_ENTRY LdrpLoadedDllHandleCache;
ULONG LdrpFatalHardErrorCount;
UNICODE_STRING LdrpDefaultPath;
RTL_CRITICAL_SECTION FastPebLock;
HANDLE LdrpShutdownThreadId;
PLDR_DATA_TABLE_ENTRY LdrpImageEntry;
ULONG LdrpNumberOfProcessors;



typedef struct _LDRP_TLS_ENTRY {
    LIST_ENTRY Links;
    IMAGE_TLS_DIRECTORY Tls;
} LDRP_TLS_ENTRY, *PLDRP_TLS_ENTRY;

LIST_ENTRY LdrpTlsList;
ULONG LdrpNumberOfTlsEntries;

NTSTATUS
LdrpInitializeTls(
        VOID
        );

NTSTATUS
LdrpAllocateTls(
        VOID
        );
VOID
LdrpFreeTls(
        VOID
        );

VOID
LdrpCallTlsInitializers(
    PVOID DllBase,
    ULONG Reason
    );

NTSTATUS
NTAPI
LdrpLoadDll(
    IN PWSTR DllPath OPTIONAL,
    IN PULONG DllCharacteristics OPTIONAL,
    IN PUNICODE_STRING DllName,
    OUT PVOID *DllHandle,
    IN BOOLEAN RunInitRoutines
    );

NTSTATUS
NTAPI
LdrpGetProcedureAddress(
    IN PVOID DllHandle,
    IN PANSI_STRING ProcedureName OPTIONAL,
    IN ULONG ProcedureNumber OPTIONAL,
    OUT PVOID *ProcedureAddress,
    IN BOOLEAN RunInitRoutines
    );

PLIST_ENTRY
RtlpLockProcessHeapsList( VOID );


VOID
RtlpUnlockProcessHeapsList( VOID );

BOOLEAN
RtlpSerializeHeap(
    IN PVOID HeapHandle
    );

ULONG NtdllBaseTag;

#define MAKE_TAG( t ) (RTL_HEAP_MAKE_TAG( NtdllBaseTag, t ))

#define CSR_TAG 0
#define LDR_TAG 1
#define CURDIR_TAG 2
#define TLS_TAG 3
#define DBG_TAG 4
#define SE_TAG 5
#define TEMP_TAG 6
#define ATOM_TAG 7

PVOID
LdrpDefineDllTag(
    PWSTR TagName,
    PUSHORT TagIndex
    );

#if defined(_X86_)
BOOLEAN
LdrpCallInitRoutine(
    IN PDLL_INIT_ROUTINE InitRoutine,
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    );
#else

#define LdrpCallInitRoutine(InitRoutine, DllHandle, Reason, Context)    \
    (InitRoutine)((DllHandle), (Reason), (Context))

#endif

#endif // _LDRP_

