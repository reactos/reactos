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

VOID
NTAPI
DbgkMapViewOfSection(
    IN HANDLE SectionHandle,
    IN PVOID BaseAddress,
    IN ULONG SectionOffset,
    IN ULONG_PTR ViewSize
);

VOID
NTAPI
DbgkUnMapViewOfSection(
    IN PVOID BaseAddress
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

HANDLE
NTAPI
DbgkpSectionToFileHandle(
    IN PVOID Section
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
