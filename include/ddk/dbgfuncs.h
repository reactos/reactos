#ifndef __INCLUDE_DDK_DBGFUNCS_H
#define __INCLUDE_DDK_DBGFUNCS_H
/* $Id$ */

VOID STDCALL DbgBreakPointWithStatus (ULONG Status);
VOID STDCALL DbgBreakPoint(VOID);
ULONG DbgPrint(PCH Format,...);
VOID STDCALL DbgPrompt (PCH OutputString, PCH InputString, USHORT InputSize);

ULONG
__cdecl
DbgPrintEx(
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCH Format,
    ...
    );

ULONG
__cdecl
DbgPrintReturnControlC(
    PCH Format,
    ...
    );

NTSTATUS
STDCALL
DbgQueryDebugFilterState(
    IN ULONG ComponentId,
    IN ULONG Level
    );

NTSTATUS
STDCALL
DbgSetDebugFilterState(
    IN ULONG ComponentId,
    IN ULONG Level,
    IN BOOLEAN State
    );

NTSTATUS
STDCALL
DbgLoadImageSymbols(
    IN PUNICODE_STRING Name,
    IN ULONG Base, 
    IN ULONG Unknown3
    );

#endif /* __INCLUDE_DDK_DBGFUNCS_H */
