#ifndef __KDBGSYMS_H
#define __KDBGSYMS_H

#include <ddk/ntddk.h>

typedef struct _IMAGE_SYMBOL_INFO
{
  ULONG_PTR ImageBase;
  ULONG_PTR ImageSize;
  PVOID FileBuffer;
  PVOID SymbolsBase;
  ULONG SymbolsLength;
  PVOID SymbolStringsBase;
  ULONG SymbolStringsLength;
} IMAGE_SYMBOL_INFO, *PIMAGE_SYMBOL_INFO;

#endif /* __KDBGSYMS_H */

