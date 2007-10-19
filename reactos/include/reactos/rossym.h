/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            include/reactos/rossym.h
 * PURPOSE:         Handling of rossym symbol info
 *
 * PROGRAMMERS:     Ge van Geldorp (gvg@reactos.com)
 */

#ifndef REACTOS_ROSSYM_H_INCLUDED
#define REACTOS_ROSSYM_H_INCLUDED

#define ROSSYM_SECTION_NAME ".rossym"

typedef struct _ROSSYM_HEADER {
  unsigned long SymbolsOffset;
  unsigned long SymbolsLength;
  unsigned long StringsOffset;
  unsigned long StringsLength;
} ROSSYM_HEADER, *PROSSYM_HEADER;

typedef struct _ROSSYM_ENTRY {
  ULONG_PTR Address;
  ULONG FunctionOffset;
  ULONG FileOffset;
  ULONG SourceLine;
} ROSSYM_ENTRY, *PROSSYM_ENTRY;

typedef struct _ROSSYM_CALLBACKS {
  PVOID (*AllocMemProc)(ULONG_PTR Size);
  VOID (*FreeMemProc)(PVOID Area);
  BOOLEAN (*ReadFileProc)(PVOID FileContext, PVOID Buffer, ULONG Size);
  BOOLEAN (*SeekFileProc)(PVOID FileContext, ULONG_PTR Position);
} ROSSYM_CALLBACKS, *PROSSYM_CALLBACKS;

typedef struct _ROSSYM_INFO *PROSSYM_INFO;

VOID RosSymInit(PROSSYM_CALLBACKS Callbacks);
VOID RosSymInitKernelMode(VOID);
VOID RosSymInitUserMode(VOID);

BOOLEAN RosSymCreateFromRaw(PVOID RawData, ULONG_PTR DataSize,
                            PROSSYM_INFO *RosSymInfo);
BOOLEAN RosSymCreateFromMem(PVOID ImageStart, ULONG_PTR ImageSize,
                            PROSSYM_INFO *RosSymInfo);
BOOLEAN RosSymCreateFromFile(PVOID FileContext, PROSSYM_INFO *RosSymInfo);
ULONG RosSymGetRawDataLength(PROSSYM_INFO RosSymInfo);
VOID RosSymGetRawData(PROSSYM_INFO RosSymInfo, PVOID RawData);
BOOLEAN RosSymGetAddressInformation(PROSSYM_INFO RosSymInfo,
                                    ULONG_PTR RelativeAddress,
                                    ULONG *LineNumber,
                                    char *FileName,
                                    char *FunctionName);
VOID RosSymDelete(PROSSYM_INFO RosSymInfo);

#endif /* REACTOS_ROSSYM_H_INCLUDED */

/* EOF */

