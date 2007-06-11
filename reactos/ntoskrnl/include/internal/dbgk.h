/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/include/dbgk.h
 * PURPOSE:         Internal header for the User-Mode Debugging Backend
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

//
// Define this if you want debugging support
//
#define _DBGK_DEBUG_                                    0x01

//
// These define the Debug Masks Supported
//
#define DBGK_THREAD_DEBUG                               0x01
#define DBGK_PROCESS_DEBUG                              0x02
#define DBGK_OBJECT_DEBUG                               0x04
#define DBGK_MESSAGE_DEBUG                              0x08
#define DBGK_EXCEPTION_DEBUG                            0x10

//
// Debug/Tracing support
//
#if _DBGK_DEBUG_
#ifdef NEW_DEBUG_SYSTEM_IMPLEMENTED // enable when Debug Filters are implemented
#define DBGKTRACE(x, ...)                                   \
    {                                                       \
        DbgPrintEx("%s [%.16s] - ",                         \
                   __FUNCTION__,                            \
                   PsGetCurrentProcess()->ImageFileName);   \
        DbgPrintEx(__VA_ARGS__);                            \
    }
#else
#define DBGKTRACE(x, ...)                                   \
    if (x & DbgkpTraceLevel)                                \
    {                                                       \
        DbgPrint("%s [%.16s] - ",                           \
                 __FUNCTION__,                              \
                 PsGetCurrentProcess()->ImageFileName);     \
        DbgPrint(__VA_ARGS__);                              \
    }
#endif
#else
#define DBGKTRACE(x, ...) DPRINT(__VA_ARGS__);
#endif

VOID
INIT_FUNCTION
NTAPI
DbgkInitialize(
    VOID
);

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
    IN PVOID Section,
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

/* EOF */
