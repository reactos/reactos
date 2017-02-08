/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/powerpc/stubs.c
 * PURPOSE:         VDM Support Services
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>
#include <ppcmmu/mmu.h>

NTSTATUS
NTAPI
NtVdmControl(IN ULONG ControlCode,
             IN PVOID ControlData)
{
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
Ke386CallBios(IN ULONG Int,
              OUT PCONTEXT Context)
{
    return STATUS_UNSUCCESSFUL;
}

VOID
NTAPI
KiUnexpectedInterrupt(VOID)
{
}

LONG NTAPI Exi386InterlockedDecrementLong(PLONG Addend)
{
    return _InterlockedDecrement(Addend);
}

LONG NTAPI Exi386InterlockedIncrementLong(PLONG Addend)
{
    return _InterlockedIncrement(Addend);
}

LONG NTAPI Exi386InterlockedExchangeUlong(PLONG Target, LONG Exch, LONG Compare)
{
    return _InterlockedCompareExchange(Target, Exch, Compare);
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
KeI386FlatToGdtSelector(IN ULONG Base,
                        IN USHORT Length,
                        IN USHORT Selector)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
KeI386ReleaseGdtSelectors(OUT PULONG SelArray,
                          IN ULONG NumOfSelectors)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
KeI386AllocateGdtSelectors(OUT PULONG SelArray,
                           IN ULONG NumOfSelectors)
{
    UNIMPLEMENTED;
    return 0;
}

VOID
NTAPI
KeDumpStackFrames(PULONG Frame)
{
}

LONG
NTAPI
Kei386EoiHelper() { return 0; }

NTSTATUS
NTAPI
KeUserModeCallback(IN ULONG RoutineIndex,
                   IN PVOID Argument,
                   IN ULONG ArgumentLength,
                   OUT PVOID *Result,
                   OUT PULONG ResultLength)
{
    return STATUS_UNSUCCESSFUL;
}

VOID
NTAPI
KiCoprocessorError() { }

VOID
NTAPI
KiDispatchInterrupt() { }

VOID
NTAPI
KiInitializeUserApc(IN PKEXCEPTION_FRAME ExceptionFrame,
                    IN PKTRAP_FRAME TrapFrame,
                    IN PKNORMAL_ROUTINE NormalRoutine,
                    IN PVOID NormalContext,
                    IN PVOID SystemArgument1,
                    IN PVOID SystemArgument2)
{
}

PVOID
NTAPI
KeSwitchKernelStack(PVOID StackBase, PVOID StackLimit)
{
    return NULL;
}

VOID
NTAPI
KiSwapProcess(struct _KPROCESS *NewProcess, struct _KPROCESS *OldProcess)
{
    PEPROCESS EProcess = (PEPROCESS)NewProcess;
    MmuSetVsid(0, 8, EProcess ? (ULONG)EProcess->UniqueProcessId : 0);
}

BOOLEAN
NTAPI
KiSwapContext(PKTHREAD CurrentThread, PKTHREAD NewThread)
{
    KeGetPcr()->Prcb->NextThread = NewThread;
    __asm__("mtdec %0" : : "r" (1));
    return TRUE;
}

VOID
NTAPI
KeI386VdmInitialize(VOID)
{
}

NTSYSAPI
NTSTATUS
NTAPI
NtCallbackReturn
( IN PVOID Result OPTIONAL, IN ULONG ResultLength, IN NTSTATUS Status )
{
    return STATUS_UNSUCCESSFUL;
}

NTSYSAPI
NTSTATUS
NTAPI
NtContinue
(IN PCONTEXT ThreadContext, IN BOOLEAN RaiseAlert)
{
    return STATUS_UNSUCCESSFUL;
}

NTSYSAPI
ULONG
NTAPI
NtGetTickCount() { return __rdtsc(); }

NTSTATUS
NTAPI
NtSetLdtEntries
(ULONG Selector1, LDT_ENTRY LdtEntry1, ULONG Selector2, LDT_ENTRY LdtEntry2)
{
    return STATUS_UNSUCCESSFUL;
}

NTSYSAPI
NTSTATUS
NTAPI
NtRaiseException
(IN PEXCEPTION_RECORD ExceptionRecord, IN PCONTEXT ThreadContext, IN BOOLEAN HandleException )
{
    return STATUS_UNSUCCESSFUL;
}

void _alldiv() { }

void _alldvrm() { }

void _allmul() { }

void _alloca_probe() { }

void _allrem() { }

void _allshl() { }

void _allshr() { }

void _aulldiv() { }

void _aulldvrm() { }

void _aullrem() { }

void _aullshr() { }

void _abnormal_termination() { }
