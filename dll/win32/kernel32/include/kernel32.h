#pragma once

//
// Kernel32 Filter IDs
//
#define kernel32file            200
#define kernel32ver             201
#define actctx                  202
#define resource                203
#define kernel32session         204
#define comm                    205
#define profile                 206
#define nls                     207


#if DBG
#define DEBUG_CHANNEL(ch) static ULONG gDebugChannel = ch;
#else
#define DEBUG_CHANNEL(ch)
#endif

#define TRACE(fmt, ...)         TRACE__(gDebugChannel, fmt, ##__VA_ARGS__)
#define WARN(fmt, ...)          WARN__(gDebugChannel, fmt, ##__VA_ARGS__)
#define FIXME(fmt, ...)         WARN__(gDebugChannel, fmt,## __VA_ARGS__)
#define ERR(fmt, ...)           ERR__(gDebugChannel, fmt, ##__VA_ARGS__)

#define STUB \
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED); \
  DPRINT1("%s() is UNIMPLEMENTED!\n", __FUNCTION__)

#define debugstr_a
#define debugstr_w
#define debugstr_wn
#define wine_dbgstr_w
#define debugstr_guid

#include "wine/unicode.h"
#include "baseheap.h"

#define MAGIC(c1,c2,c3,c4)  ((c1) + ((c2)<<8) + ((c3)<<16) + ((c4)<<24))

#define MAGIC_HEAP      MAGIC( 'H','E','A','P' )

#define ROUNDUP(a,b)    ((((a)+(b)-1)/(b))*(b))
#define ROUNDDOWN(a,b)  (((a)/(b))*(b))

#define ROUND_DOWN(n, align) \
    (((ULONG)n) & ~((align) - 1l))

#define ROUND_UP(n, align) \
    ROUND_DOWN(((ULONG)n) + (align) - 1, (align))

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

#define __TRY _SEH2_TRY
#define __EXCEPT_PAGE_FAULT _SEH2_EXCEPT(_SEH2_GetExceptionCode() == STATUS_ACCESS_VIOLATION)
#define __ENDTRY _SEH2_END

/* Undocumented CreateProcess flag */
#define STARTF_SHELLPRIVATE         0x400

typedef struct _CODEPAGE_ENTRY
{
    LIST_ENTRY Entry;
    UINT CodePage;
    HANDLE SectionHandle;
    PBYTE SectionMapping;
    CPTABLEINFO CodePageTable;
} CODEPAGE_ENTRY, *PCODEPAGE_ENTRY;

typedef struct tagLOADPARMS32
{
    LPSTR lpEnvAddress;
    LPSTR lpCmdLine;
    WORD  wMagicValue;
    WORD  wCmdShow;
    DWORD dwReserved;
} LOADPARMS32;

typedef enum _BASE_SEARCH_PATH_TYPE
{
    BaseSearchPathInvalid,
    BaseSearchPathDll,
    BaseSearchPathApp,
    BaseSearchPathDefault,
    BaseSearchPathEnv,
    BaseSearchPathCurrent,
    BaseSearchPathMax
} BASE_SEARCH_PATH_TYPE, *PBASE_SEARCH_PATH_TYPE;

typedef enum _BASE_CURRENT_DIR_PLACEMENT
{
    BaseCurrentDirPlacementInvalid = -1,
    BaseCurrentDirPlacementDefault,
    BaseCurrentDirPlacementSafe,
    BaseCurrentDirPlacementMax
} BASE_CURRENT_DIR_PLACEMENT;

typedef struct _BASEP_ACTCTX_BLOCK
{
    ULONG Flags;
    PVOID ActivationContext;
    PVOID CompletionContext;
    LPOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine;
} BASEP_ACTCTX_BLOCK, *PBASEP_ACTCTX_BLOCK;

#define BASEP_GET_MODULE_HANDLE_EX_PARAMETER_VALIDATION_ERROR    1
#define BASEP_GET_MODULE_HANDLE_EX_PARAMETER_VALIDATION_SUCCESS  2
#define BASEP_GET_MODULE_HANDLE_EX_PARAMETER_VALIDATION_CONTINUE 3

extern PBASE_STATIC_SERVER_DATA BaseStaticServerData;

typedef
DWORD
(*WaitForInputIdleType)(
    HANDLE hProcess,
    DWORD dwMilliseconds);

extern WaitForInputIdleType UserWaitForInputIdleRoutine;

/* Flags for PrivCopyFileExW && BasepCopyFileExW */
#define BASEP_COPY_METADATA         0x10
#define BASEP_COPY_SACL             0x20
#define BASEP_COPY_OWNER_AND_GROUP  0x40
#define BASEP_COPY_DIRECTORY        0x80
#define BASEP_COPY_BACKUP_SEMANTICS 0x100
#define BASEP_COPY_REPLACE          0x200
#define BASEP_COPY_SKIP_DACL        0x400
#define BASEP_COPY_PUBLIC_MASK      0xF
#define BASEP_COPY_BASEP_MASK       0xFFFFFFF0

/* Flags for PrivMoveFileIdentityW */
#define PRIV_DELETE_ON_SUCCESS      0x1
#define PRIV_ALLOW_NON_TRACKABLE    0x2

/* GLOBAL VARIABLES **********************************************************/

extern BOOL bIsFileApiAnsi;
extern HMODULE hCurrentModule;

extern RTL_CRITICAL_SECTION BaseDllDirectoryLock;

extern UNICODE_STRING BaseDllDirectory;
extern UNICODE_STRING BaseDefaultPath;
extern UNICODE_STRING BaseDefaultPathAppend;
extern PLDR_DATA_TABLE_ENTRY BasepExeLdrEntry;

extern LPTOP_LEVEL_EXCEPTION_FILTER GlobalTopLevelExceptionFilter;

extern SYSTEM_BASIC_INFORMATION BaseCachedSysInfo;

extern BOOLEAN BaseRunningInServerProcess;

/* FUNCTION PROTOTYPES *******************************************************/

VOID
NTAPI
BaseDllInitializeMemoryManager(VOID);

PTEB GetTeb(VOID);

PWCHAR FilenameA2W(LPCSTR NameA, BOOL alloc);
DWORD FilenameW2A_N(LPSTR dest, INT destlen, LPCWSTR src, INT srclen);

DWORD FilenameW2A_FitOrFail(LPSTR  DestA, INT destLen, LPCWSTR SourceW, INT sourceLen);
DWORD FilenameU2A_FitOrFail(LPSTR  DestA, INT destLen, PUNICODE_STRING SourceU);

#define HeapAlloc RtlAllocateHeap
#define HeapReAlloc RtlReAllocateHeap
#define HeapFree RtlFreeHeap
#define _lread(a, b, c)  (long)(_hread(a, b, (long)c))

PLARGE_INTEGER
WINAPI
BaseFormatTimeOut(OUT PLARGE_INTEGER Timeout,
                  IN DWORD dwMilliseconds);

POBJECT_ATTRIBUTES
WINAPI
BaseFormatObjectAttributes(OUT POBJECT_ATTRIBUTES ObjectAttributes,
                             IN PSECURITY_ATTRIBUTES SecurityAttributes OPTIONAL,
                             IN PUNICODE_STRING ObjectName);

NTSTATUS
WINAPI
BaseCreateStack(
    _In_ HANDLE hProcess,
    _In_opt_ SIZE_T StackCommit,
    _In_opt_ SIZE_T StackReserve,
    _Out_ PINITIAL_TEB InitialTeb);

VOID
WINAPI
BaseFreeThreadStack(
    _In_ HANDLE hProcess,
    _In_ PINITIAL_TEB InitialTeb);

VOID
WINAPI
BaseInitializeContext(IN PCONTEXT Context,
                       IN PVOID Parameter,
                       IN PVOID StartAddress,
                       IN PVOID StackAddress,
                       IN ULONG ContextType);

VOID
WINAPI
BaseThreadStartupThunk(VOID);

VOID
WINAPI
BaseProcessStartThunk(VOID);

VOID
NTAPI
BasepFreeActivationContextActivationBlock(
    IN PBASEP_ACTCTX_BLOCK ActivationBlock
);

NTSTATUS
NTAPI
BasepAllocateActivationContextActivationBlock(
    IN DWORD Flags,
    IN PVOID CompletionRoutine,
    IN PVOID CompletionContext,
    OUT PBASEP_ACTCTX_BLOCK *ActivationBlock
);

NTSTATUS
NTAPI
BasepProbeForDllManifest(
    IN PVOID DllHandle,
    IN PCWSTR FullDllName,
    OUT PVOID *ActCtx
);

DECLSPEC_NORETURN
VOID
WINAPI
BaseThreadStartup(
    _In_ LPTHREAD_START_ROUTINE lpStartAddress,
    _In_ LPVOID lpParameter);

DECLSPEC_NORETURN
VOID
WINAPI
BaseFiberStartup(VOID);

typedef DWORD (WINAPI *PPROCESS_START_ROUTINE)(VOID);

DECLSPEC_NORETURN
VOID
WINAPI
BaseProcessStartup(
    _In_ PPROCESS_START_ROUTINE lpStartAddress);

PVOID
WINAPI
BasepIsRealtimeAllowed(IN BOOLEAN Keep);

VOID
WINAPI
BasepAnsiStringToHeapUnicodeString(IN LPCSTR AnsiString,
                                   OUT LPWSTR* UnicodeString);

PUNICODE_STRING
WINAPI
Basep8BitStringToStaticUnicodeString(IN LPCSTR AnsiString);

BOOLEAN
WINAPI
Basep8BitStringToDynamicUnicodeString(OUT PUNICODE_STRING UnicodeString,
                                      IN LPCSTR String);

typedef NTSTATUS (NTAPI *PRTL_CONVERT_STRING)(IN PUNICODE_STRING UnicodeString,
                                              IN PANSI_STRING AnsiString,
                                              IN BOOLEAN AllocateMemory);

typedef ULONG (NTAPI *PRTL_COUNT_STRING)(IN PUNICODE_STRING UnicodeString);

typedef NTSTATUS (NTAPI *PRTL_CONVERT_STRINGA)(IN PANSI_STRING AnsiString,
                                              IN PCUNICODE_STRING UnicodeString,
                                              IN BOOLEAN AllocateMemory);

typedef ULONG (NTAPI *PRTL_COUNT_STRINGA)(IN PANSI_STRING UnicodeString);

ULONG
NTAPI
BasepUnicodeStringToAnsiSize(IN PUNICODE_STRING String);

ULONG
NTAPI
BasepAnsiStringToUnicodeSize(IN PANSI_STRING String);

extern PRTL_CONVERT_STRING Basep8BitStringToUnicodeString;
extern PRTL_CONVERT_STRINGA BasepUnicodeStringTo8BitString;
extern PRTL_COUNT_STRING BasepUnicodeStringTo8BitSize;
extern PRTL_COUNT_STRINGA Basep8BitStringToUnicodeSize;

extern UNICODE_STRING BaseWindowsDirectory, BaseWindowsSystemDirectory;
extern HANDLE BaseNamedObjectDirectory;

HANDLE
WINAPI
BaseGetNamedObjectDirectory(VOID);

NTSTATUS
WINAPI
BasepMapFile(IN LPCWSTR lpApplicationName,
             OUT PHANDLE hSection,
             IN PUNICODE_STRING ApplicationName);

PCODEPAGE_ENTRY FASTCALL
IntGetCodePageEntry(UINT CodePage);

LPWSTR
WINAPI
BaseComputeProcessDllPath(
    IN LPWSTR FullPath,
    IN PVOID Environment
);

LPWSTR
WINAPI
BaseComputeProcessExePath(
    IN LPWSTR FullPath
);

ULONG
WINAPI
BaseIsDosApplication(
    IN PUNICODE_STRING PathName,
    IN NTSTATUS Status
);

NTSTATUS
WINAPI
BasepCheckBadapp(
    IN HANDLE FileHandle,
    IN PWCHAR ApplicationName,
    IN PWCHAR Environment,
    IN USHORT ExeType,
    IN PVOID* SdbQueryAppCompatData,
    IN PULONG SdbQueryAppCompatDataSize,
    IN PVOID* SxsData,
    IN PULONG SxsDataSize,
    OUT PULONG FusionFlags
);

BOOLEAN
WINAPI
IsShimInfrastructureDisabled(
    VOID
);

VOID
WINAPI
InitCommandLines(VOID);

DWORD
WINAPI
BaseSetLastNTError(IN NTSTATUS Status);

VOID
NTAPI
BasepLocateExeLdrEntry(IN PLDR_DATA_TABLE_ENTRY Entry,
                       IN PVOID Context,
                       OUT BOOLEAN *StopEnumeration);

typedef NTSTATUS
(NTAPI *PBASEP_APPCERT_PLUGIN_FUNC)(
    IN LPWSTR ApplicationName,
    IN ULONG CertFlag
);

typedef NTSTATUS
(NTAPI *PBASEP_APPCERT_EMBEDDED_FUNC)(
    IN LPWSTR ApplicationName
);

typedef NTSTATUS
(NTAPI *PSAFER_REPLACE_PROCESS_THREAD_TOKENS)(
    IN HANDLE Token,
    IN HANDLE Process,
    IN HANDLE Thread
);

typedef struct _BASEP_APPCERT_ENTRY
{
    LIST_ENTRY Entry;
    UNICODE_STRING Name;
    PBASEP_APPCERT_PLUGIN_FUNC fPluginCertFunc;
} BASEP_APPCERT_ENTRY, *PBASEP_APPCERT_ENTRY;

typedef struct _BASE_MSG_SXS_HANDLES
{
    HANDLE File;
    HANDLE Process;
    HANDLE Section;
    LARGE_INTEGER ViewBase;
} BASE_MSG_SXS_HANDLES, *PBASE_MSG_SXS_HANDLES;

typedef struct _SXS_WIN32_NT_PATH_PAIR
{
    PUNICODE_STRING Win32;
    PUNICODE_STRING Nt;
} SXS_WIN32_NT_PATH_PAIR, *PSXS_WIN32_NT_PATH_PAIR;

typedef struct _SXS_OVERRIDE_MANIFEST
{
    PCWCH Name;
    PVOID Address;
    ULONG Size;
} SXS_OVERRIDE_MANIFEST, *PSXS_OVERRIDE_MANIFEST;

NTSTATUS
NTAPI
BasepConfigureAppCertDlls(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
);

extern LIST_ENTRY BasepAppCertDllsList;
extern RTL_CRITICAL_SECTION gcsAppCert;

VOID
WINAPI
BaseMarkFileForDelete(
    IN HANDLE FileHandle,
    IN ULONG FileAttributes
);

BOOL
BasepCopyFileExW(
    IN LPCWSTR lpExistingFileName,
    IN LPCWSTR lpNewFileName,
    IN LPPROGRESS_ROUTINE lpProgressRoutine OPTIONAL,
    IN LPVOID lpData OPTIONAL,
    IN LPBOOL pbCancel OPTIONAL,
    IN DWORD dwCopyFlags,
    IN DWORD dwBasepFlags,
    OUT LPHANDLE lpExistingHandle,
    OUT LPHANDLE lpNewHandle
);

BOOL
BasepGetVolumeNameForVolumeMountPoint(
    IN LPCWSTR lpszMountPoint,
    OUT LPWSTR lpszVolumeName,
    IN DWORD cchBufferLength,
    OUT LPBOOL IsAMountPoint
);

BOOL
BasepGetVolumeNameFromReparsePoint(
    IN LPCWSTR lpszMountPoint,
    OUT LPWSTR lpszVolumeName,
    IN DWORD cchBufferLength,
    OUT LPBOOL IsAMountPoint
);

BOOL
IsThisARootDirectory(
    IN HANDLE VolumeHandle,
    IN PUNICODE_STRING NtPathName
);

/* FIXME: This is EXPORTED! It should go in an external kernel32.h header */
VOID
WINAPI
BasepFreeAppCompatData(
    IN PVOID AppCompatData,
    IN PVOID AppCompatSxsData
);

NTSTATUS
WINAPI
BasepCheckWinSaferRestrictions(
    IN HANDLE UserToken,
    IN LPWSTR ApplicationName,
    IN HANDLE FileHandle,
    OUT PBOOLEAN InJob,
    OUT PHANDLE NewToken,
    OUT PHANDLE JobHandle
);
