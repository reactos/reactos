/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/rossym/frommem.c
 * PURPOSE:         Creating rossym info from an in-memory image
 *
 * PROGRAMMERS:     Ge van Geldorp (gvg@reactos.com)
 */

#define NTOSAPI
#include <ntddk.h>
#include <reactos/rossym.h>
#include "rossympriv.h"

#define NDEBUG
#include <debug.h>

BOOLEAN
RosSymCreateFromRaw(PVOID RawData, ULONG_PTR DataSize, PROSSYM_INFO *RosSymInfo)
{
  PROSSYM_HEADER RosSymHeader;

  RosSymHeader = (PROSSYM_HEADER) RawData;
  if (RosSymHeader->SymbolsOffset < sizeof(ROSSYM_HEADER)
      || RosSymHeader->StringsOffset < RosSymHeader->SymbolsOffset + RosSymHeader->SymbolsLength
      || DataSize < RosSymHeader->StringsOffset + RosSymHeader->StringsLength
      || 0 != (RosSymHeader->SymbolsLength % sizeof(ROSSYM_ENTRY)))
    {
      DPRINT1("Invalid ROSSYM_HEADER\n");
      return FALSE;
    }

  /* Copy */
  *RosSymInfo = RosSymAllocMem(sizeof(ROSSYM_INFO) + RosSymHeader->SymbolsLength
                               + RosSymHeader->StringsLength + 1);
  if (NULL == *RosSymInfo)
    {
      DPRINT1("Failed to allocate memory for rossym\n");
      return FALSE;
    }
  (*RosSymInfo)->Symbols = (PROSSYM_ENTRY)((char *) *RosSymInfo + sizeof(ROSSYM_INFO));
  (*RosSymInfo)->SymbolsCount = RosSymHeader->SymbolsLength / sizeof(ROSSYM_ENTRY);
  (*RosSymInfo)->Strings = (PCHAR) *RosSymInfo + sizeof(ROSSYM_INFO) + RosSymHeader->SymbolsLength;
  (*RosSymInfo)->StringsLength = RosSymHeader->StringsLength;
  memcpy((*RosSymInfo)->Symbols, (char *) RosSymHeader + RosSymHeader->SymbolsOffset,
         RosSymHeader->SymbolsLength);
  memcpy((*RosSymInfo)->Strings, (char *) RosSymHeader + RosSymHeader->StringsOffset,
         RosSymHeader->StringsLength);
  /* Make sure the last string is null terminated, we allocated an extra byte for that */
  (*RosSymInfo)->Strings[(*RosSymInfo)->StringsLength] = '\0';

  return TRUE;
}

/* EOF */
