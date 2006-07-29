#ifndef __NTOSKRNL_INCLUDE_INTERNAL_DBGK_H
#define __NTOSKRNL_INCLUDE_INTERNAL_DBGK_H

VOID
STDCALL
DbgkCreateThread(PVOID StartAddress);

VOID
NTAPI
DbgkExitProcess(IN NTSTATUS ExitStatus);

VOID
NTAPI
DbgkExitThread(IN NTSTATUS ExitStatus);

VOID
NTAPI
DbgkCopyProcessDebugPort(
    IN PEPROCESS Process,
    IN PEPROCESS Parent
);

extern POBJECT_TYPE DbgkDebugObjectType;
#endif

/* EOF */
