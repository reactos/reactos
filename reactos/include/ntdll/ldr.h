#ifndef __NTOSKRNL_INCLUDE_INTERNAL_LDR_H
#define __NTOSKRNL_INCLUDE_INTERNAL_LDR_H

#include <roscfg.h>
#ifndef _NTNDK_
#include <napi/teb.h>
#endif
#include <reactos/rossym.h>

typedef NTSTATUS (STDCALL *PEPFUNC)(PPEB);

/* Type for a DLL's entry point */
typedef BOOL 
(STDCALL *PDLLMAIN_FUNC)(HANDLE hInst,
                         ULONG ul_reason_for_call,
                         LPVOID lpReserved);

#if defined(__USE_W32API) || defined(__NTDLL__)
/*
 * Fu***ng headers hell made me do this...i'm sick of it
 */

typedef struct _LOCK_INFORMATION
{
  ULONG LockCount;
  DEBUG_LOCK_INFORMATION LockEntry[1];
} LOCK_INFORMATION, *PLOCK_INFORMATION;

typedef struct _HEAP_INFORMATION
{
  ULONG HeapCount;
  DEBUG_HEAP_INFORMATION HeapEntry[1];
} HEAP_INFORMATION, *PHEAP_INFORMATION;

typedef struct _MODULE_INFORMATION
{
  ULONG ModuleCount;
  DEBUG_MODULE_INFORMATION ModuleEntry[1];
} MODULE_INFORMATION, *PMODULE_INFORMATION;

NTSTATUS STDCALL
LdrQueryProcessModuleInformation(IN PMODULE_INFORMATION ModuleInformation OPTIONAL,
				 IN ULONG Size OPTIONAL,
				 OUT PULONG ReturnedSize);

#endif /* __USE_W32API */

/* Module flags */
#define IMAGE_DLL		0x00000004
#define LOAD_IN_PROGRESS	0x00001000
#define UNLOAD_IN_PROGRESS	0x00002000
#define ENTRY_PROCESSED		0x00004000
#define DONT_CALL_FOR_THREAD	0x00040000
#define PROCESS_ATTACH_CALLED	0x00080000
#define IMAGE_NOT_AT_BASE	0x00200000

typedef struct _LDR_MODULE
{
   LIST_ENTRY     InLoadOrderModuleList;
   LIST_ENTRY     InMemoryOrderModuleList;		/* not used */
   LIST_ENTRY     InInitializationOrderModuleList;	/* not used */
   PVOID          BaseAddress;
   ULONG          EntryPoint;
   ULONG          ResidentSize;
   UNICODE_STRING FullDllName;
   UNICODE_STRING BaseDllName;
   ULONG          Flags;
   SHORT          LoadCount;
   SHORT          TlsIndex;
   HANDLE         SectionHandle;
   ULONG          CheckSum;
   ULONG          TimeDateStamp;
#if defined(DBG) || defined(KDBG)
   PROSSYM_INFO   RosSymInfo;
#endif /* KDBG */
} LDR_MODULE, *PLDR_MODULE;

typedef struct _LDR_SYMBOL_INFO {
  PLDR_MODULE ModuleObject;
  ULONG_PTR ImageBase;
  PVOID SymbolsBuffer;
  ULONG SymbolsBufferLength;
  PVOID SymbolStringsBuffer;
  ULONG SymbolStringsBufferLength;
} LDR_SYMBOL_INFO, *PLDR_SYMBOL_INFO;


#define RVA(m, b) ((ULONG)b + m)

#if defined(KDBG) || defined(DBG)

VOID
LdrpLoadUserModuleSymbols(PLDR_MODULE LdrModule);

#endif

ULONG
LdrpGetResidentSize(PIMAGE_NT_HEADERS NTHeaders);

PEPFUNC LdrPEStartup (PVOID  ImageBase,
		      HANDLE SectionHandle,
		      PLDR_MODULE* Module,
		      PWSTR FullDosName);
NTSTATUS LdrMapSections(HANDLE ProcessHandle,
			PVOID ImageBase,
			HANDLE SectionHandle,
			PIMAGE_NT_HEADERS NTHeaders);
NTSTATUS LdrMapNTDllForProcess(HANDLE ProcessHandle,
			       PHANDLE NTDllSectionHandle);
BOOLEAN LdrMappedAsDataFile(PVOID *BaseAddress);

NTSTATUS STDCALL
LdrDisableThreadCalloutsForDll(IN PVOID BaseAddress);

NTSTATUS STDCALL
LdrGetDllHandle(IN PWCHAR Path OPTIONAL,
		IN ULONG Unknown2,
		IN PUNICODE_STRING DllName,
		OUT PVOID *BaseAddress);

NTSTATUS STDCALL
LdrFindEntryForAddress(IN PVOID Address,
		       OUT PLDR_MODULE *Module);

NTSTATUS STDCALL
LdrGetProcedureAddress(IN PVOID BaseAddress,
		       IN PANSI_STRING Name,
		       IN ULONG Ordinal,
		       OUT PVOID *ProcedureAddress);

VOID STDCALL
LdrInitializeThunk(ULONG Unknown1,
		   ULONG Unknown2,
		   ULONG Unknown3,
		   ULONG Unknown4);

NTSTATUS STDCALL
LdrLoadDll(IN PWSTR SearchPath OPTIONAL,
	   IN ULONG LoadFlags,
	   IN PUNICODE_STRING Name,
	   OUT PVOID *BaseAddress OPTIONAL);

PIMAGE_BASE_RELOCATION STDCALL
LdrProcessRelocationBlock(IN PVOID Address,
			  IN USHORT Count,
			  IN PUSHORT TypeOffset,
			  IN ULONG_PTR Delta);

NTSTATUS STDCALL
LdrQueryImageFileExecutionOptions (IN PUNICODE_STRING SubKey,
				   IN PCWSTR ValueName,
				   IN ULONG ValueSize,
				   OUT PVOID Buffer,
				   IN ULONG BufferSize,
				   OUT PULONG RetunedLength OPTIONAL);

NTSTATUS STDCALL
LdrShutdownProcess(VOID);

NTSTATUS STDCALL
LdrShutdownThread(VOID);

NTSTATUS STDCALL
LdrUnloadDll(IN PVOID BaseAddress);

NTSTATUS STDCALL
LdrVerifyImageMatchesChecksum (IN HANDLE FileHandle,
			       ULONG Unknown1,
			       ULONG Unknown2,
			       ULONG Unknown3);

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_LDR_H */

/* EOF */
