typedef NTSTATUS (*PEPFUNC)(PPEB);

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



NTSTATUS STDCALL
LdrDisableThreadCalloutsForDll (IN PVOID BaseAddress,
                                IN BOOLEAN Disable);

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
LdrUnloadDll (IN PVOID BaseAddress);

/* EOF */
