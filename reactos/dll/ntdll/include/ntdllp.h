/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            dll/ntdll/include/ntdllp.h
 * PURPOSE:         Native Libary Internal Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#define LDR_HASH_TABLE_ENTRIES 32

typedef struct _LDRP_TLS_DATA
{
    LIST_ENTRY TlsLinks;
    IMAGE_TLS_DIRECTORY TlsDirectory;
} LDRP_TLS_DATA, *PLDRP_TLS_DATA;

typedef BOOL
(NTAPI *PDLLMAIN_FUNC)(HANDLE hInst,
                       ULONG ul_reason_for_call,
                       LPVOID lpReserved);

/* Global data */
extern RTL_CRITICAL_SECTION LdrpLoaderLock;
extern BOOLEAN LdrpInLdrInit;
extern LIST_ENTRY LdrpHashTable[LDR_HASH_TABLE_ENTRIES];
extern BOOLEAN ShowSnaps;

/* ldrinit.c */
NTSTATUS NTAPI LdrpRunInitializeRoutines(IN PCONTEXT Context OPTIONAL);
NTSTATUS NTAPI LdrpInitializeThread(IN PCONTEXT Context);
NTSTATUS NTAPI LdrpInitializeTls(VOID);
NTSTATUS NTAPI LdrpAllocateTls(VOID);
VOID NTAPI LdrpFreeTls(VOID);
VOID NTAPI LdrpTlsCallback(PVOID BaseAddress, ULONG Reason);
BOOLEAN NTAPI LdrpCallDllEntry(PDLLMAIN_FUNC EntryPoint, PVOID BaseAddress, ULONG Reason, PVOID Context);
NTSTATUS NTAPI LdrpInitializeProcess(PCONTEXT Context, PVOID SystemArgument1);
VOID NTAPI LdrpInitFailure(NTSTATUS Status);


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

ULONG NTAPI
LdrpClearLoadInProgress();

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


/* FIXME: Cleanup this mess */
typedef NTSTATUS (NTAPI *PEPFUNC)(PPEB);
NTSTATUS LdrMapSections(HANDLE ProcessHandle,
			PVOID ImageBase,
			HANDLE SectionHandle,
			PIMAGE_NT_HEADERS NTHeaders);
NTSTATUS LdrMapNTDllForProcess(HANDLE ProcessHandle,
			       PHANDLE NTDllSectionHandle);
BOOLEAN LdrMappedAsDataFile(PVOID *BaseAddress);
ULONG
LdrpGetResidentSize(PIMAGE_NT_HEADERS NTHeaders);
PEPFUNC LdrPEStartup (PVOID  ImageBase,
		      HANDLE SectionHandle,
		      PLDR_DATA_TABLE_ENTRY* Module,
		      PWSTR FullDosName);

extern HANDLE WindowsApiPort;

/* EOF */
