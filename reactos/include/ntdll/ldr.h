typedef NTSTATUS (*PEPFUNC)(VOID);

typedef struct _DLL
{
   PIMAGE_NT_HEADERS Headers;
   PVOID BaseAddress;
   HANDLE SectionHandle;
   struct _DLL* Prev;
   struct _DLL* Next;
} DLL, *PDLL;

#define RVA(m, b) ((ULONG)b + m)

extern DLL LdrDllListHead;

PEPFUNC LdrPEStartup(PVOID ImageBase, HANDLE SectionHandle);
