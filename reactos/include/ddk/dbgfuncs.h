#ifndef __INCLUDE_DDK_DBGFUNCS_H
#define __INCLUDE_DDK_DBGFUNCS_H
/* $Id: dbgfuncs.h,v 1.5 2000/05/25 15:49:50 ekohl Exp $ */

#define DBG_STATUS_CONTROL_C       1
#define DBG_STATUS_SYSRQ           2
#define DBG_STATUS_BUGCHECK_FIRST  3
#define DBG_STATUS_BUGCHECK_SECOND 4
#define DBG_STATUS_FATAL           5
VOID STDCALL DbgBreakPointWithStatus (ULONG Status);
VOID STDCALL DbgBreakPoint(VOID);
ULONG DbgPrint(PCH Format,...);
VOID STDCALL DbgPrompt (PCH OutputString, PCH InputString, USHORT InputSize);


#define DBG_GET_SHOW_FACILITY 0x0001
#define DBG_GET_SHOW_SEVERITY 0x0002
#define DBG_GET_SHOW_ERRCODE  0x0004
#define DBG_GET_SHOW_ERRTEXT  0x0008
VOID DbgGetErrorText(NTSTATUS ErrorCode, PUNICODE_STRING ErrorText, ULONG Flags);
VOID DbgPrintErrorMessage(NTSTATUS ErrorCode);

#endif /* __INCLUDE_DDK_DBGFUNCS_H */
