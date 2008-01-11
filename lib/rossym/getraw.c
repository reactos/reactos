/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/rossym/getraw.c
 * PURPOSE:         Convert rossym info to raw external format
 *
 * PROGRAMMERS:     Ge van Geldorp (gvg@reactos.com)
 */

#define NTOSAPI
#include <ntddk.h>
#include <reactos/rossym.h>
#include "rossympriv.h"

#define NDEBUG
#include <debug.h>

ULONG
RosSymGetRawDataLength(PROSSYM_INFO RosSymInfo)
{
  return sizeof(ROSSYM_HEADER)
         + RosSymInfo->SymbolsCount * sizeof(ROSSYM_ENTRY)
         + RosSymInfo->StringsLength;
}

VOID
RosSymGetRawData(PROSSYM_INFO RosSymInfo, PVOID RawData)
{
  PROSSYM_HEADER RosSymHeader;

  RosSymHeader = (PROSSYM_HEADER) RawData;
  RosSymHeader->SymbolsOffset = sizeof(ROSSYM_HEADER);
  RosSymHeader->SymbolsLength = RosSymInfo->SymbolsCount * sizeof(ROSSYM_ENTRY);
  RosSymHeader->StringsOffset = RosSymHeader->SymbolsOffset + RosSymHeader->SymbolsLength;
  RosSymHeader->StringsLength = RosSymInfo->StringsLength;

  memcpy((char *) RawData + RosSymHeader->SymbolsOffset, RosSymInfo->Symbols,
         RosSymHeader->SymbolsLength);
  memcpy((char *) RawData + RosSymHeader->StringsOffset, RosSymInfo->Strings,
         RosSymHeader->StringsLength);
}

/* EOF */
