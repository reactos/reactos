
#ifndef __KDBGSYMS_H
#define __KDBGSYMS_H

#include <ddk/ntddk.h>

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

#endif

