#ifndef __INCLUDE_NTDLL_TRACE_H
#define __INCLUDE_NTDLL_TRACE_H

typedef struct _NTDLL_TRACE_TABLE
{
  CHAR Flags[4096];
} NTDLL_TABLE_TABLE, *PNTDLL_TABLE_TRACE;

#define TRACE_NTDLL    (1)
#define TRACE_KERNEL32 (2)
#define TRACE_CRTDLL   (3)

VOID
RtlPrintTrace(ULONG Flag, PCH Format, ...);

#define TPRINTF(F, X...) RtlPrintTrace(F, ## X)

#endif /* __INCLUDE_NTDLL_TRACE_H */
