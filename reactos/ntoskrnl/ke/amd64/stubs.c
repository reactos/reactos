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

VOID
FASTCALL
KeZeroPages(IN PVOID Address,
            IN ULONG Size)
{
    /* Not using XMMI in this routine */
    RtlZeroMemory(Address, Size);
}

VOID
FASTCALL
DECLSPEC_NORETURN
KiServiceExit(IN PKTRAP_FRAME TrapFrame,
              IN NTSTATUS Status)
{
    UNIMPLEMENTED;
}

VOID
FASTCALL
DECLSPEC_NORETURN
KiServiceExit2(IN PKTRAP_FRAME TrapFrame)
{
    UNIMPLEMENTED;
}

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

NTSTATUS
NTAPI
KiCallUserMode(
    IN PVOID *OutputBuffer,
    IN PULONG OutputLength)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

#undef ExQueryDepthSList
NTKERNELAPI
USHORT
ExQueryDepthSList(IN PSLIST_HEADER ListHead)
{
    return (USHORT)(ListHead->Alignment & 0xffff);
}


ULONG ProcessCount;

#ifdef _MSC_VER
void
__GSHandlerCheck()
{
}

int __security_cookie;

void
__security_check_cookie()
{
}

BOOLEAN CcPfEnablePrefetcher;

unsigned long __readfsdword(const unsigned long Offset)
{
    return 0;
}

void main()
{
}

#endif
