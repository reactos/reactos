#ifndef __NTOSKRNL_INCLUDE_INTERNAL_DBGK_H
#define __NTOSKRNL_INCLUDE_INTERNAL_DBGK_H

VOID
NTAPI
DbgkCreateThread(
    IN PVOID StartAddress
);

VOID
NTAPI
DbgkExitProcess(
    IN NTSTATUS ExitStatus
);

VOID
NTAPI
DbgkExitThread(
    IN NTSTATUS ExitStatus
);

BOOLEAN
NTAPI
DbgkpSuspendProcess(
    VOID
);

VOID
NTAPI
DbgkpResumeProcess(
    VOID
);

NTSTATUS
NTAPI
DbgkpSendApiMessage(
    IN OUT PDBGKM_MSG ApiMsg,
    IN ULONG Flags
);

VOID
NTAPI
DbgkCopyProcessDebugPort(
    IN PEPROCESS Process,
    IN PEPROCESS Parent
);

BOOLEAN
NTAPI
DbgkForwardException(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN BOOLEAN DebugPort,
    IN BOOLEAN SecondChance
);

NTSTATUS
NTAPI
DbgkClearProcessDebugObject(
    IN PEPROCESS Process,
    IN PDEBUG_OBJECT SourceDebugObject
);

extern POBJECT_TYPE DbgkDebugObjectType;
#endif

/* EOF */
