/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            dll/ntdll/include/ntdllp.h
 * PURPOSE:         Native Library Internal Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

#pragma once

#define LDR_HASH_TABLE_ENTRIES 32
#define LDR_GET_HASH_ENTRY(x) (RtlUpcaseUnicodeChar((x)) & (LDR_HASH_TABLE_ENTRIES - 1))

/* LdrpUpdateLoadCount2 flags */
#define LDRP_UPDATE_REFCOUNT   0x01
#define LDRP_UPDATE_DEREFCOUNT 0x02
#define LDRP_UPDATE_PIN        0x03

/* Loader flags */
#define IMAGE_LOADER_FLAGS_COMPLUS 0x00000001
#define IMAGE_LOADER_FLAGS_SYSTEM_GLOBAL 0x01000000

/* Page heap flags */
#define DPH_FLAG_DLL_NOTIFY 0x40

typedef struct _LDRP_TLS_DATA
{
    LIST_ENTRY TlsLinks;
    IMAGE_TLS_DIRECTORY TlsDirectory;
} LDRP_TLS_DATA, *PLDRP_TLS_DATA;

typedef
NTSTATUS
(NTAPI* PLDR_APP_COMPAT_DLL_REDIRECTION_CALLBACK_FUNCTION)(
    _In_ ULONG Flags,
    _In_ PCWSTR DllName,
    _In_ PCWSTR DllPath OPTIONAL,
    _Inout_opt_ PULONG DllCharacteristics,
    _In_ PVOID CallbackData,
    _Outptr_ PWSTR* EffectiveDllPath);

/* Global data */
extern RTL_CRITICAL_SECTION LdrpLoaderLock;
extern BOOLEAN LdrpInLdrInit;
extern PVOID LdrpHeap;
extern LIST_ENTRY LdrpHashTable[LDR_HASH_TABLE_ENTRIES];
extern BOOLEAN ShowSnaps;
extern UNICODE_STRING LdrpDefaultPath;
extern HANDLE LdrpKnownDllObjectDirectory;
extern ULONG LdrpNumberOfProcessors;
extern ULONG LdrpFatalHardErrorCount;
extern PUNICODE_STRING LdrpTopLevelDllBeingLoaded;
extern PLDR_DATA_TABLE_ENTRY LdrpCurrentDllInitializer;
extern UNICODE_STRING LdrApiDefaultExtension;
extern BOOLEAN LdrpLdrDatabaseIsSetup;
extern ULONG LdrpActiveUnloadCount;
extern BOOLEAN LdrpShutdownInProgress;
extern UNICODE_STRING LdrpKnownDllPath;
extern PLDR_DATA_TABLE_ENTRY LdrpGetModuleHandleCache, LdrpLoadedDllHandleCache;
extern BOOLEAN RtlpPageHeapEnabled;
extern ULONG RtlpDphGlobalFlags;
extern BOOLEAN g_ShimsEnabled;
extern PVOID g_pShimEngineModule;
extern PVOID g_pfnSE_DllLoaded;
extern PVOID g_pfnSE_DllUnloaded;
extern PVOID g_pfnSE_InstallBeforeInit;
extern PVOID g_pfnSE_InstallAfterInit;
extern PVOID g_pfnSE_ProcessDying;

/* ldrinit.c */
NTSTATUS NTAPI LdrpRunInitializeRoutines(IN PCONTEXT Context OPTIONAL);
VOID NTAPI LdrpInitializeThread(IN PCONTEXT Context);
NTSTATUS NTAPI LdrpInitializeTls(VOID);
NTSTATUS NTAPI LdrpAllocateTls(VOID);
VOID NTAPI LdrpFreeTls(VOID);
VOID NTAPI LdrpCallTlsInitializers(IN PLDR_DATA_TABLE_ENTRY LdrEntry, IN ULONG Reason);
BOOLEAN NTAPI LdrpCallInitRoutine(IN PDLL_INIT_ROUTINE EntryPoint, IN PVOID BaseAddress, IN ULONG Reason, IN PVOID Context);
NTSTATUS NTAPI LdrpInitializeProcess(IN PCONTEXT Context, IN PVOID SystemArgument1);
VOID NTAPI LdrpInitFailure(NTSTATUS Status);
VOID NTAPI LdrpValidateImageForMp(IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry);
VOID NTAPI LdrpEnsureLoaderLockIsHeld(VOID);

/* ldrpe.c */
NTSTATUS
NTAPI
LdrpSnapThunk(IN PVOID ExportBase,
              IN PVOID ImportBase,
              IN PIMAGE_THUNK_DATA OriginalThunk,
              IN OUT PIMAGE_THUNK_DATA Thunk,
              IN PIMAGE_EXPORT_DIRECTORY ExportEntry,
              IN ULONG ExportSize,
              IN BOOLEAN Static,
              IN LPSTR DllName);

NTSTATUS NTAPI
LdrpWalkImportDescriptor(IN LPWSTR DllPath OPTIONAL,
                         IN PLDR_DATA_TABLE_ENTRY LdrEntry);


/* ldrutils.c */
NTSTATUS NTAPI
LdrpGetProcedureAddress(IN PVOID BaseAddress,
                        IN PANSI_STRING Name,
                        IN ULONG Ordinal,
                        OUT PVOID *ProcedureAddress,
                        IN BOOLEAN ExecuteInit);

PLDR_DATA_TABLE_ENTRY NTAPI
LdrpAllocateDataTableEntry(IN PVOID BaseAddress);

VOID NTAPI
LdrpInsertMemoryTableEntry(IN PLDR_DATA_TABLE_ENTRY LdrEntry);

NTSTATUS NTAPI
LdrpLoadDll(IN BOOLEAN Redirected,
            IN PWSTR DllPath OPTIONAL,
            IN PULONG DllCharacteristics OPTIONAL,
            IN PUNICODE_STRING DllName,
            OUT PVOID *BaseAddress,
            IN BOOLEAN CallInit);

VOID NTAPI
LdrpUpdateLoadCount2(IN PLDR_DATA_TABLE_ENTRY LdrEntry,
                     IN ULONG Flags);

ULONG NTAPI
LdrpClearLoadInProgress(VOID);

NTSTATUS
NTAPI
LdrpSetProtection(PVOID ViewBase,
                  BOOLEAN Restore);

BOOLEAN
NTAPI
LdrpCheckForLoadedDllHandle(IN PVOID Base,
                            OUT PLDR_DATA_TABLE_ENTRY *LdrEntry);

BOOLEAN NTAPI
LdrpCheckForLoadedDll(IN PWSTR DllPath,
                      IN PUNICODE_STRING DllName,
                      IN BOOLEAN Flag,
                      IN BOOLEAN RedirectedDll,
                      OUT PLDR_DATA_TABLE_ENTRY *LdrEntry);

NTSTATUS NTAPI
LdrpMapDll(IN PWSTR SearchPath OPTIONAL,
           IN PWSTR DllPath2,
           IN PWSTR DllName OPTIONAL,
           IN PULONG DllCharacteristics,
           IN BOOLEAN Static,
           IN BOOLEAN Redirect,
           OUT PLDR_DATA_TABLE_ENTRY *DataTableEntry);

PVOID NTAPI
LdrpFetchAddressOfEntryPoint(PVOID ImageBase);

VOID NTAPI
LdrpFreeUnicodeString(PUNICODE_STRING String);

VOID NTAPI
LdrpRecordUnloadEvent(_In_ PLDR_DATA_TABLE_ENTRY LdrEntry);

VOID NTAPI
LdrpGetShimEngineInterface(VOID);

VOID
NTAPI
LdrpLoadShimEngine(IN PWSTR ImageName,
                   IN PUNICODE_STRING ProcessImage,
                   IN PVOID pShimData);

VOID NTAPI
LdrpUnloadShimEngine(VOID);

/* verifier.c */

NTSTATUS NTAPI
LdrpInitializeApplicationVerifierPackage(IN HANDLE KeyHandle,
                                         IN PPEB Peb,
                                         IN BOOLEAN SystemWide,
                                         IN BOOLEAN ReadAdvancedOptions);

NTSTATUS NTAPI
AVrfInitializeVerifier(VOID);

VOID NTAPI
AVrfDllLoadNotification(IN PLDR_DATA_TABLE_ENTRY LdrEntry);

VOID NTAPI
AVrfDllUnloadNotification(IN PLDR_DATA_TABLE_ENTRY LdrEntry);

VOID NTAPI
AVrfPageHeapDllNotification(IN PLDR_DATA_TABLE_ENTRY LdrEntry);


/* FIXME: Cleanup this mess */
typedef NTSTATUS (NTAPI *PEPFUNC)(PPEB);
NTSTATUS LdrMapSections(HANDLE ProcessHandle,
                        PVOID ImageBase,
                        HANDLE SectionHandle,
                        PIMAGE_NT_HEADERS NTHeaders);
NTSTATUS LdrMapNTDllForProcess(HANDLE ProcessHandle,
                               PHANDLE NTDllSectionHandle);
ULONG
LdrpGetResidentSize(PIMAGE_NT_HEADERS NTHeaders);

NTSTATUS
NTAPI
LdrpLoadImportModule(IN PWSTR DllPath OPTIONAL,
                     IN LPSTR ImportName,
                     OUT PLDR_DATA_TABLE_ENTRY *DataTableEntry,
                     OUT PBOOLEAN Existing);

VOID
NTAPI
LdrpFinalizeAndDeallocateDataTableEntry(IN PLDR_DATA_TABLE_ENTRY Entry);


/* path.c */
BOOLEAN
NTAPI
RtlDoesFileExists_UStr(
    IN PUNICODE_STRING FileName
);

/* EOF */
