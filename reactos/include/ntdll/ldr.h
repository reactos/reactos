#ifndef __NTOSKRNL_INCLUDE_INTERNAL_LDR_H
#define __NTOSKRNL_INCLUDE_INTERNAL_LDR_H

#include <ntos/kdbgsyms.h>
#include <roscfg.h>
#include <napi/teb.h>

typedef NTSTATUS STDCALL (*PEPFUNC)(PPEB);

/* Type for a DLL's entry point */
typedef BOOL STDCALL
(* PDLLMAIN_FUNC)(HANDLE hInst,
		  ULONG ul_reason_for_call,
		  LPVOID lpReserved);

typedef struct _LDR_MODULE
{
   LIST_ENTRY     InLoadOrderModuleList;
   LIST_ENTRY     InMemoryOrderModuleList;		// not used
   LIST_ENTRY     InInitializationOrderModuleList;	// not used
   PVOID          BaseAddress;
   ULONG          EntryPoint;
   ULONG          SizeOfImage;
   UNICODE_STRING FullDllName;
   UNICODE_STRING BaseDllName;
   ULONG          Flags;
   SHORT          LoadCount;
   SHORT          TlsIndex;
   HANDLE         SectionHandle;
   ULONG          CheckSum;
   ULONG          TimeDateStamp;
#ifdef KDBG
  IMAGE_SYMBOL_INFO SymbolInfo;
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


typedef struct _MODULE_ENTRY
{
  ULONG Unknown0;
  ULONG Unknown1;
  PVOID BaseAddress;
  ULONG SizeOfImage;
  ULONG Flags;
  USHORT Unknown2;
  USHORT Unknown3;
  SHORT LoadCount;
  USHORT PathLength;
  CHAR ModuleName[256];
} MODULE_ENTRY, *PMODULE_ENTRY;

typedef struct _MODULE_INFORMATION
{
  ULONG ModuleCount;
  MODULE_ENTRY ModuleEntry[1];
} MODULE_INFORMATION, *PMODULE_INFORMATION;

#ifdef KDBG

VOID
LdrpLoadUserModuleSymbols(PLDR_MODULE LdrModule);

#endif

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


NTSTATUS STDCALL
LdrDisableThreadCalloutsForDll(IN PVOID BaseAddress);

NTSTATUS STDCALL
LdrGetDllHandle(IN ULONG Unknown1,
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

NTSTATUS STDCALL
LdrQueryProcessModuleInformation(IN PMODULE_INFORMATION ModuleInformation OPTIONAL,
				 IN ULONG Size OPTIONAL,
				 OUT PULONG ReturnedSize);

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
