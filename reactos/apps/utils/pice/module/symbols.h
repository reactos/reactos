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

typedef struct _LOCAL_VARIABLE
{
	char type_name[64];
	char name[64];
	ULONG value,offset,line;
    BOOLEAN bRegister;
}LOCAL_VARIABLE,*PLOCAL_VARIABLE;

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
ULONG FindFunctionInModuleByName(LPSTR szFunctionname,struct module* pMod);
PICE_SYMBOLFILE_HEADER* FindModuleSymbolsByModuleName(LPSTR modname);
BOOLEAN FindAddressForSourceLine(ULONG ulLineNumber,LPSTR pFilename,struct module* pMod,PULONG pValue);
ULONG ConvertDecimalToUlong(LPSTR p);
struct module* FindModuleFromAddress(ULONG addr);
PICE_SYMBOLFILE_HEADER* FindModuleSymbols(ULONG addr);
ULONG ListSymbolStartingAt(struct module* pMod,PICE_SYMBOLFILE_HEADER* pSymbols,ULONG index,LPSTR pOutput);
struct module* FindModuleByName(LPSTR modname);
void Evaluate(PICE_SYMBOLFILE_HEADER* pSymbols,LPSTR p);
LONG ExtractNumber(LPSTR p);
LPSTR ExtractTypeName(LPSTR p);

extern ULONG kernel_end;
extern PICE_SYMBOLFILE_HEADER* apSymbols[32];

extern struct module fake_kernel_module;
#define KERNEL_START (0xc0100000)
