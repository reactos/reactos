
#ifndef __KDBGSYMS_H
#define __KDBGSYMS_H

#include <ddk/ntddk.h>

typedef struct _SYMBOL
{
  struct _SYMBOL *Next;
  /* Address relative to module base address */
  ULONG RelativeAddress;
  UNICODE_STRING Name;
} SYMBOL, *PSYMBOL;

typedef struct _SYMBOL_TABLE
{
  ULONG SymbolCount;
  PSYMBOL Symbols;
} SYMBOL_TABLE, *PSYMBOL_TABLE;

#endif

