#define NTOS_MODE_KERNEL
#include <ntos.h>

NTSTATUS
LdrGetAddressInformation(IN PIMAGE_SYMBOL_INFO  SymbolInfo,
  IN ULONG_PTR  RelativeAddress,
  OUT PULONG LineNumber,
  OUT PCH FileName  OPTIONAL,
  OUT PCH FunctionName  OPTIONAL);

CHAR
KdbTryGetCharKeyboard(PULONG ScanCode);
ULONG
KdbTryGetCharSerial(VOID);
VOID
KdbInit2(VOID);
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

typedef struct _KDB_MODULE_INFO
{
    WCHAR              Name[256];
    ULONG_PTR          Base;
    ULONG              Size;
    PIMAGE_SYMBOL_INFO SymbolInfo;
} KDB_MODULE_INFO, *PKDB_MODULE_INFO;

BOOLEAN
KdbFindModuleByAddress(PVOID address, PKDB_MODULE_INFO info);
BOOLEAN
KdbFindModuleByName(LPCWSTR name, PKDB_MODULE_INFO info);
BOOLEAN
KdbFindModuleByIndex(INT i, PKDB_MODULE_INFO info);
