#include <ntoskrnl.h>

#define ST_FILENAME	0x00
#define ST_FUNCTION	0x01
#define ST_LINENUMBER	0x02

typedef struct _SYMBOL
{
  struct _SYMBOL *Next;
  /* Address relative to module base address */
  ULONG RelativeAddress;
  ULONG SymbolType;
  ANSI_STRING Name;
  ULONG LineNumber;
} SYMBOL, *PSYMBOL;

typedef struct _SYMBOL_TABLE
{
  ULONG SymbolCount;
  PSYMBOL Symbols;
} SYMBOL_TABLE, *PSYMBOL_TABLE;

typedef struct _IMAGE_SYMBOL_INFO
{
  SYMBOL_TABLE FileNameSymbols;
  SYMBOL_TABLE FunctionSymbols;
  SYMBOL_TABLE LineNumberSymbols;
  ULONG_PTR ImageBase;
  ULONG_PTR ImageSize;
  PVOID FileBuffer;
  PVOID SymbolsBase;
  ULONG SymbolsLength;
  PVOID SymbolStringsBase;
  ULONG SymbolStringsLength;
} IMAGE_SYMBOL_INFO, *PIMAGE_SYMBOL_INFO;

#define AreSymbolsParsed(si)((si)->FileNameSymbols.Symbols \
	|| (si)->FunctionSymbols.Symbols \
	|| (si)->LineNumberSymbols.Symbols)
	
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
