#define NTOS_MODE_KERNEL
#include <ntos.h>

typedef struct _KDB_MODULE_INFO
{
    WCHAR              Name[256];
    ULONG_PTR          Base;
    ULONG              Size;
    PIMAGE_SYMBOL_INFO SymbolInfo;
} KDB_MODULE_INFO, *PKDB_MODULE_INFO;

/* from kdb_symbols.c */

BOOLEAN
KdbpSymFindModuleByAddress(IN PVOID Address,
                           OUT PKDB_MODULE_INFO pInfo);

BOOLEAN
KdbpSymFindModuleByName(IN LPCWSTR Name,
                        OUT PKDB_MODULE_INFO pInfo);

BOOLEAN
KdbpSymFindModuleByIndex(IN INT Index,
                         OUT PKDB_MODULE_INFO pInfo);

BOOLEAN 
KdbSymPrintAddress(IN PVOID Address);

NTSTATUS
KdbSymGetAddressInformation(IN PIMAGE_SYMBOL_INFO  SymbolInfo,
                            IN ULONG_PTR  RelativeAddress,
                            OUT PULONG LineNumber  OPTIONAL,
                            OUT PCH FileName  OPTIONAL,
                            OUT PCH FunctionName  OPTIONAL);

BOOLEAN
KdbpSymGetSourceAddress(IN PIMAGE_SYMBOL_INFO SymbolInfo,
                        IN PCHAR FileName,
                        IN ULONG LineNumber  OPTIONAL,
                        IN PCHAR FuncName  OPTIONAL,
                        OUT PVOID *Address);

/* other functions */
/*NTSTATUS
KdbSafeReadMemory(PVOID dst, PVOID src, INT size);
NTSTATUS
KdbSafeWriteMemory(PVOID dst, PVOID src, INT size);*/
#define KdbpSafeReadMemory(dst, src, size) MmSafeCopyFromUser(dst, src, size)
#define KdbpSafeWriteMemory(dst, src, size) MmSafeCopyToUser(dst, src, size)
CHAR
KdbTryGetCharKeyboard(PULONG ScanCode);
ULONG
KdbTryGetCharSerial(VOID);
VOID
KdbEnter(VOID);
VOID
DbgRDebugInit(VOID);
VOID
DbgShowFiles(VOID);
VOID
DbgEnableFile(PCH Filename);
VOID
DbgDisableFile(PCH Filename);
VOID
KdbInitProfiling();
VOID
KdbInitProfiling2();
VOID
KdbDisableProfiling();
VOID
KdbEnableProfiling();
VOID
KdbProfileInterrupt(ULONG_PTR Eip);

struct KDB_BPINFO {
    DWORD Addr;
    DWORD Type;
    DWORD Size;
    DWORD Enabled;
};
