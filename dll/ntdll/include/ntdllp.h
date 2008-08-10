/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            dll/ntdll/include/ntdllp.h
 * PURPOSE:         Native Libary Internal Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

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
typedef BOOL
(NTAPI *PDLLMAIN_FUNC)(HANDLE hInst,
                         ULONG ul_reason_for_call,
                         LPVOID lpReserved);

#if defined(KDBG) || defined(DBG)
VOID
LdrpLoadUserModuleSymbols(PLDR_DATA_TABLE_ENTRY LdrModule);
#endif
extern HANDLE WindowsApiPort;

/* EOF */
