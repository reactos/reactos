#include <ntos/kdbgsyms.h>
#include "../ntoskrnl/include/internal/config.h"

typedef NTSTATUS STDCALL (*PEPFUNC)(PPEB);

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
  SYMBOL_TABLE    Symbols;
#endif /* KDBG */
} LDR_MODULE, *PLDR_MODULE;


#define RVA(m, b) ((ULONG)b + m)


PEPFUNC LdrPEStartup(PVOID ImageBase, HANDLE SectionHandle);
NTSTATUS LdrMapSections(HANDLE ProcessHandle,
			PVOID ImageBase,
			HANDLE SectionHandle,
			PIMAGE_NT_HEADERS NTHeaders);
NTSTATUS LdrMapNTDllForProcess(HANDLE ProcessHandle,
			       PHANDLE NTDllSectionHandle);



NTSTATUS STDCALL
LdrDisableThreadCalloutsForDll (IN PVOID BaseAddress);

NTSTATUS STDCALL
LdrGetDllHandle (IN ULONG Unknown1,
                 IN ULONG Unknown2,
                 IN PUNICODE_STRING DllName,
                 OUT PVOID *BaseAddress);

NTSTATUS STDCALL
LdrGetProcedureAddress (IN PVOID BaseAddress,
                        IN PANSI_STRING Name,
                        IN ULONG Ordinal,
                        OUT PVOID *ProcedureAddress);

VOID STDCALL
LdrInitializeThunk (ULONG Unknown1,
                    ULONG Unknown2,
                    ULONG Unknown3,
                    ULONG Unknown4);

NTSTATUS STDCALL
LdrLoadDll (IN PWSTR SearchPath OPTIONAL,
            IN ULONG LoadFlags,
            IN PUNICODE_STRING Name,
            OUT PVOID *BaseAddress OPTIONAL);

NTSTATUS STDCALL
LdrShutdownProcess (VOID);

NTSTATUS STDCALL
LdrShutdownThread (VOID);

NTSTATUS STDCALL
LdrUnloadDll (IN PVOID BaseAddress);

VOID LdrLoadModuleSymbols(PLDR_MODULE ModuleObject);

/* EOF */
