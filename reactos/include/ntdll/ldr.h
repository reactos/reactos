typedef NTSTATUS (*PEPFUNC)(VOID);

typedef struct _DLL
{
   PIMAGE_NT_HEADERS Headers;
   PVOID BaseAddress;
   HANDLE SectionHandle;
   struct _DLL* Prev;
   struct _DLL* Next;
   UINT ReferenceCount;
} DLL, *PDLL;

#define RVA(m, b) ((ULONG)b + m)

extern DLL LdrDllListHead;

PEPFUNC LdrPEStartup(PVOID ImageBase, HANDLE SectionHandle);
NTSTATUS LdrMapSections(HANDLE ProcessHandle,
			PVOID ImageBase,
			HANDLE SectionHandle,
			PIMAGE_NT_HEADERS NTHeaders);
NTSTATUS LdrMapNTDllForProcess(HANDLE ProcessHandle,
			       PHANDLE NTDllSectionHandle);

NTSTATUS
LdrLoadDll (PDLL* Dll,PCHAR	Name);

NTSTATUS LdrUnloadDll(PDLL Dll);


PVOID
LdrGetExportByOrdinal (PDLL Module, ULONG Ordinal );

PVOID
LdrGetExportByName (PDLL Module, PUCHAR	SymbolName );
