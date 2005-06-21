/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/ntdll/inc/ntdllp.h
 * PURPOSE:         Native Libary Internal Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* FIXME: Cleanup this mess */
//typedef NTSTATUS (STDCALL *PEPFUNC)(PPEB);
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
#if 0
typedef BOOL 
(STDCALL *PDLLMAIN_FUNC)(HANDLE hInst,
                         ULONG ul_reason_for_call,
                         LPVOID lpReserved);
#endif 
VOID
STDCALL
RtlpInitDeferedCriticalSection(
    VOID
);
#if defined(KDBG) || defined(DBG)
VOID
LdrpLoadUserModuleSymbols(PLDR_DATA_TABLE_ENTRY LdrModule);
#endif
extern HANDLE WindowsApiPort;

NTSTATUS
STDCALL
RtlpWaitForCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection
);

VOID
STDCALL
RtlpUnWaitCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection
);

VOID
STDCALL
RtlpCreateCriticalSectionSem(
    PRTL_CRITICAL_SECTION CriticalSection
);

VOID
STDCALL
RtlpInitDeferedCriticalSection(
    VOID
);

VOID
STDCALL
RtlpFreeDebugInfo(
    PRTL_CRITICAL_SECTION_DEBUG DebugInfo
);

PRTL_CRITICAL_SECTION_DEBUG
STDCALL
RtlpAllocateDebugInfo(
    VOID
);
/* EOF */
