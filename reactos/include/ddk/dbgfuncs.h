#ifndef __INCLUDE_DDK_DBGFUNCS_H
#define __INCLUDE_DDK_DBGFUNCS_H
/* $Id: dbgfuncs.h,v 1.8 2003/06/07 16:16:38 chorns Exp $ */

VOID STDCALL DbgBreakPointWithStatus (ULONG Status);
VOID STDCALL DbgBreakPoint(VOID);
ULONG DbgPrint(PCH Format,...);
VOID STDCALL DbgPrompt (PCH OutputString, PCH InputString, USHORT InputSize);

#endif /* __INCLUDE_DDK_DBGFUNCS_H */
