/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         stubs
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <debug.h>

BOOLEAN
NTAPI
KeConnectInterrupt(IN PKINTERRUPT Interrupt)
{
    UNIMPLEMENTED;
    return FALSE;
}

PVOID
NTAPI
KeSwitchKernelStack(PVOID StackBase, PVOID StackLimit)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOLEAN
NTAPI
KeSynchronizeExecution(
    IN OUT PKINTERRUPT Interrupt,
    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    IN PVOID SynchronizeContext)
{
    UNIMPLEMENTED;
    return FALSE;
}

VOID
NTAPI
KeUpdateRunTime(IN PKTRAP_FRAME TrapFrame,
                IN KIRQL Irql)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
KeUpdateSystemTime(IN PKTRAP_FRAME TrapFrame,
                   IN KIRQL Irql,
                   IN ULONG Increment)
{
    UNIMPLEMENTED;
}


NTSTATUS
NTAPI
KeUserModeCallback(IN ULONG RoutineIndex,
                   IN PVOID Argument,
                   IN ULONG ArgumentLength,
                   OUT PVOID *Result,
                   OUT PULONG ResultLength)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

ULONG
NTAPI
KiComputeTimerTableIndex(LONGLONG Timer)
{
    UNIMPLEMENTED;
    return 0;
}

VOID
KiIdleLoop()
{
    UNIMPLEMENTED;
    for(;;);
}

VOID
NTAPI
KiInitializeUserApc(IN PKEXCEPTION_FRAME ExceptionFrame,
                    IN PKTRAP_FRAME TrapFrame,
                    IN PKNORMAL_ROUTINE NormalRoutine,
                    IN PVOID NormalContext,
                    IN PVOID SystemArgument1,
                    IN PVOID SystemArgument2)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
KiSwapProcess(IN PKPROCESS NewProcess,
              IN PKPROCESS OldProcess)
{
    UNIMPLEMENTED;
}

VOID
KiSystemService(IN PKTHREAD Thread,
                IN PKTRAP_FRAME TrapFrame,
                IN ULONG Instruction)
{
    UNIMPLEMENTED;
}

NTSYSAPI
NTSTATUS
NTAPI
NtCallbackReturn
( IN PVOID Result OPTIONAL, IN ULONG ResultLength, IN NTSTATUS Status )
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSYSAPI
NTSTATUS
NTAPI
NtContinue(
    IN PCONTEXT ThreadContext, IN BOOLEAN RaiseAlert)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSYSAPI
NTSTATUS
NTAPI
NtRaiseException
(IN PEXCEPTION_RECORD ExceptionRecord, IN PCONTEXT ThreadContext, IN BOOLEAN HandleException )
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
NtSetLdtEntries
(ULONG Selector1, LDT_ENTRY LdtEntry1, ULONG Selector2, LDT_ENTRY LdtEntry2)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
NtVdmControl(IN ULONG ControlCode,
             IN PVOID ControlData)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTKERNELAPI
PSLIST_ENTRY
ExpInterlockedFlushSList(
    PSLIST_HEADER ListHead)
{
    UNIMPLEMENTED;
    return NULL;
}
