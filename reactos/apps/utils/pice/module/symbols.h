/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    symbols.h

Abstract:

    HEADER for symbols.c

Environment:

    LINUX 2.2.X
    Kernel mode only

Author:

    Klaus P. Gerlicher

Revision History:

    15-Nov-2000:    general cleanup of source files

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/

// constant defines
#define FIELD_OFFSET(Type,Field) (LONG)(&(((Type *)(0))->Field))
#define CONTAINING_RECORD(Address,Type,Field) (Type *)(((LONG)Address) - FIELD_OFFSET(Type,Field))

typedef struct _LOCAL_VARIABLE
{
	char type_name[64];
	char name[64];
	ULONG value,offset,line;
    BOOLEAN bRegister;
}LOCAL_VARIABLE,*PLOCAL_VARIABLE;


struct _DEBUG_MODULE_SYMBOL_
{
	ULONG value;
	char* name;
};

typedef struct _DEBUG_MODULE_
{
	struct _DEBUG_MODULE_ *next;
	ULONG size;
	PVOID BaseAddress;
	PVOID EntryPoint;
	WCHAR name[DEBUG_MODULE_NAME_LEN];
	struct _DEBUG_MODULE_SYMBOL_ syms;
}DEBUG_MODULE, *PDEBUG_MODULE;

BOOLEAN InitFakeKernelModule(void);
BOOLEAN LoadExports(void);
BOOLEAN SanityCheckExports(void);
void UnloadExports(void);
BOOLEAN ScanExports(const char *pFind,PULONG pValue);
BOOLEAN ScanExportsByAddress(LPSTR *pFind,ULONG ulValue);
PICE_SYMBOLFILE_HEADER* LoadSymbols(LPSTR filename);
BOOLEAN LoadSymbolsFromConfig(BOOLEAN bIgnoreBootParams);
void UnloadSymbols(void);
BOOLEAN ReloadSymbols(void);
LPSTR FindFunctionByAddress(ULONG ulValue,PULONG pulstart,PULONG pulend);
LPSTR FindSourceLineForAddress(ULONG addr,PULONG pulLineNumber,LPSTR* ppSrcStart,LPSTR* ppSrcEnd,LPSTR* ppFilename);
PLOCAL_VARIABLE FindLocalsByAddress(ULONG addr);
ULONG FindFunctionInModuleByName(LPSTR szFunctionname, PDEBUG_MODULE pMod);
PICE_SYMBOLFILE_HEADER* FindModuleSymbolsByModuleName(LPSTR modname);
BOOLEAN FindAddressForSourceLine(ULONG ulLineNumber,LPSTR pFilename, PDEBUG_MODULE pMod,PULONG pValue);
ULONG ConvertDecimalToUlong(LPSTR p);
PDEBUG_MODULE FindModuleFromAddress(ULONG addr);
PICE_SYMBOLFILE_HEADER* FindModuleSymbols(ULONG addr);
ULONG ListSymbolStartingAt(PDEBUG_MODULE pMod,PICE_SYMBOLFILE_HEADER* pSymbols,ULONG index,LPSTR pOutput);
PDEBUG_MODULE FindModuleByName(LPSTR modname);
void Evaluate(PICE_SYMBOLFILE_HEADER* pSymbols,LPSTR p);
LONG ExtractNumber(LPSTR p);
LPSTR ExtractTypeName(LPSTR p);
PDEBUG_MODULE IsModuleLoaded(LPSTR p);

//extern ULONG kernel_end;
extern PICE_SYMBOLFILE_HEADER* apSymbols[32];

//extern struct module fake_kernel_module;
#define KERNEL_START (0xc0000000)

