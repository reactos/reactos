/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            dll/ntdll/include/ntdllp.h
 * PURPOSE:         Native Libary Internal Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

extern BOOLEAN ShowSnaps;

typedef struct _LDRP_TLS_DATA
{
    LIST_ENTRY TlsLinks;
    IMAGE_TLS_DIRECTORY TlsDirectory;
} LDRP_TLS_DATA, *PLDRP_TLS_DATA;

typedef BOOL
(NTAPI *PDLLMAIN_FUNC)(HANDLE hInst,
                       ULONG ul_reason_for_call,
                       LPVOID lpReserved);


/* ldrinit.c */
NTSTATUS NTAPI LdrpInitializeTls(VOID);
NTSTATUS NTAPI LdrpAllocateTls(VOID);
VOID NTAPI LdrpFreeTls(VOID);
VOID NTAPI LdrpTlsCallback(PVOID BaseAddress, ULONG Reason);
BOOLEAN NTAPI LdrpCallDllEntry(PDLLMAIN_FUNC EntryPoint, PVOID BaseAddress, ULONG Reason, PVOID Context);


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
