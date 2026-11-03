/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 trap, system service and interrupt dispatch
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* brk immediates, see sdk/lib/rtl/arm64/debug_asm.s */
#define KI_BRK_BREAKPOINT       0xF000
#define KI_BRK_ASSERTION        0xF001
#define KI_BRK_DEBUG_SERVICE    0xF002
#define KI_BRK_FASTFAIL         0xF003
#define KI_BRK_DIVIDE_BY_ZERO   0xF004

/* Data and instruction fault status codes, the low two bits are the level */
#define FSC_TYPE_MASK           0x3C
#define FSC_TRANSLATION         0x04
#define FSC_ACCESS_FLAG         0x08
#define FSC_PERMISSION          0x0C
#define FSC_LEVEL(Fsc)          ((Fsc) & 3)

/* Cache maintenance aborts report WnR set, CM tells them apart */
#define ESR_ISS_CM              0x100

#define MAX_SYSCALL_ARGUMENTS   16

typedef
NTSTATUS
(NTAPI *PKI_SYSTEM_SERVICE)(
    ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
    ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
    ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
    ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR);

VOID
NTAPI
KiArm64ExitToUser(
    _Inout_ PKTRAP_FRAME TrapFrame,
    _Inout_ PKEXCEPTION_FRAME ExceptionFrame);

NTSTATUS
NTAPI
PsConvertToGuiThread(VOID);

/* Every trap keeps its exception frame right below the floating point state */
#define KiArm64TrapExceptionFrame(TrapFrame) \
    ((PKEXCEPTION_FRAME)((ULONG_PTR)(TrapFrame) - sizeof(KARM64_VFP_STATE) - sizeof(KEXCEPTION_FRAME)))

/* FUNCTIONS *****************************************************************/

static
VOID
KiArm64DispatchTrapException(
    _Inout_ PKTRAP_FRAME TrapFrame,
    _Inout_ PKEXCEPTION_FRAME ExceptionFrame,
    _In_ NTSTATUS Code,
    _In_ ULONG ParameterCount,
    _In_ ULONG_PTR Parameter1,
    _In_ ULONG_PTR Parameter2,
    _In_ ULONG_PTR Parameter3)
{
    EXCEPTION_RECORD ExceptionRecord;

    RtlZeroMemory(&ExceptionRecord, sizeof(ExceptionRecord));
    ExceptionRecord.ExceptionCode = Code;
    ExceptionRecord.ExceptionAddress = (PVOID)TrapFrame->Pc;
    ExceptionRecord.NumberParameters = ParameterCount;
    ExceptionRecord.ExceptionInformation[0] = Parameter1;
    ExceptionRecord.ExceptionInformation[1] = Parameter2;
    ExceptionRecord.ExceptionInformation[2] = Parameter3;

    KiDispatchException(&ExceptionRecord,
                        ExceptionFrame,
                        TrapFrame,
                        KiGetPreviousMode(TrapFrame),
                        TRUE);
}

/**
 * @brief Handles the faults that only track page use.
 *
 * Writable is a software bit, a clean page is mapped read only and the first
 * write marks it dirty here. Pages with a clear access flag fault on their
 * first use. Neither needs the memory manager, so this works at any IRQL.
 */
static
BOOLEAN
KiArm64ResolveTrackingFault(
    _In_ PVOID Address,
    _In_ ULONG Fsc,
    _In_ BOOLEAN Write,
    _In_ BOOLEAN UserFault)
{
    MMPTE OldPte, NewPte;
    PMMPTE PointerPte;

    /* Only page level entries are ever left clean or unaccessed */
    if ((FSC_LEVEL(Fsc) != 3) || !MiIsPdeForAddressValid(Address))
        return FALSE;

    PointerPte = MiAddressToPte(Address);
    OldPte.u.Long = *(volatile ULONG_PTR *)&PointerPte->u.Long;
    if (!OldPte.u.Hard.Valid || (UserFault && !OldPte.u.Hard.Owner))
        return FALSE;

    NewPte = OldPte;
    switch (Fsc & FSC_TYPE_MASK)
    {
        case FSC_ACCESS_FLAG:
            NewPte.u.Hard.Accessed = 1;
            break;

        case FSC_PERMISSION:
            if (!Write || !OldPte.u.Hard.Writable || !OldPte.u.Hard.NotDirty)
                return FALSE;
            NewPte.u.Hard.Accessed = 1;
            NewPte.u.Hard.NotDirty = 0;
            break;

        default:
            return FALSE;
    }

    /* Losing the race means someone else changed the entry, just retry */
    InterlockedCompareExchange64((PLONG64)PointerPte, NewPte.u.Long, OldPte.u.Long);
    KeArm64FlushTbEntry(Address);
    return TRUE;
}

/* A new entry can be missed by the walker until its store is visible */
static
BOOLEAN
KiArm64IsTranslationNowValid(
    _In_ PVOID Address)
{
    if (!MiIsPdeForAddressValid(Address))
        return FALSE;

    if (!MiAddressToPte(Address)->u.Hard.Valid)
        return FALSE;

    __dsb(_ARM64_BARRIER_ISH);
    __isb(_ARM64_BARRIER_SY);
    return TRUE;
}

static
VOID
KiArm64HandleAbort(
    _Inout_ PKTRAP_FRAME TrapFrame,
    _Inout_ PKEXCEPTION_FRAME ExceptionFrame,
    _In_ ULONG ExceptionClass)
{
    BOOLEAN Write, Execute, UserFault;
    ULONG Fsc, FaultCode;
    PVOID Address;
    NTSTATUS Status;

    Fsc = TrapFrame->Esr & ESR_ISS_FSC_MASK;
    Execute = (ExceptionClass == ESR_EC_IABT_LOWER) || (ExceptionClass == ESR_EC_IABT_CURRENT);
    Write = !Execute &&
            (TrapFrame->Esr & ESR_ISS_WNR) &&
            !(TrapFrame->Esr & ESR_ISS_CM);
    UserFault = KiUserTrap(TrapFrame);

    /* An instruction abort reports the faulting PC, not FAR */
    Address = Execute ? (PVOID)TrapFrame->Pc : (PVOID)TrapFrame->FaultAddress;

    switch (Fsc & FSC_TYPE_MASK)
    {
        case FSC_ACCESS_FLAG:
        case FSC_PERMISSION:
            if (KiArm64ResolveTrackingFault(Address, Fsc, Write, UserFault))
                return;
            break;

        case FSC_TRANSLATION:
            if (KiArm64IsTranslationNowValid(Address))
                return;
            break;

        default:
            /* Alignment, external and TLB conflict aborts are not paging faults */
            KiArm64DispatchTrapException(TrapFrame,
                                         ExceptionFrame,
                                         ((Fsc == 0x21) ? STATUS_DATATYPE_MISALIGNMENT : STATUS_ACCESS_VIOLATION),
                                         2,
                                         Write,
                                         (ULONG_PTR)Address,
                                         0);
            return;
    }

    /* Let the memory manager in with the interrupt state of the faulting code */
    if (!(TrapFrame->Spsr & ARM64_PSR_I))
        _enable();

    FaultCode = 0;
    if ((Fsc & FSC_TYPE_MASK) != FSC_TRANSLATION)
        FaultCode |= 0x1;
    if (Write)
        FaultCode |= 0x2;
    if (Execute)
        FaultCode |= 0x10;

    Status = MmAccessFault(FaultCode, Address, KiGetPreviousMode(TrapFrame), TrapFrame);
    if (NT_SUCCESS(Status))
        return;

    switch (Status)
    {
        case STATUS_ACCESS_VIOLATION:
        case STATUS_GUARD_PAGE_VIOLATION:
        case STATUS_STACK_OVERFLOW:
            KiArm64DispatchTrapException(TrapFrame,
                                         ExceptionFrame,
                                         Status,
                                         2,
                                         Execute ? EXCEPTION_EXECUTE_FAULT : Write,
                                         (ULONG_PTR)Address,
                                         0);
            break;

        default:
            KiArm64DispatchTrapException(TrapFrame,
                                         ExceptionFrame,
                                         STATUS_IN_PAGE_ERROR,
                                         3,
                                         Execute ? EXCEPTION_EXECUTE_FAULT : Write,
                                         (ULONG_PTR)Address,
                                         (ULONG_PTR)Status);
            break;
    }
}

static
VOID
KiArm64HandleBreak(
    _Inout_ PKTRAP_FRAME TrapFrame,
    _Inout_ PKEXCEPTION_FRAME ExceptionFrame)
{
    ULONG Immediate = TrapFrame->Esr & 0xFFFF;

    switch (Immediate)
    {
        case KI_BRK_BREAKPOINT:
            KiArm64DispatchTrapException(TrapFrame, ExceptionFrame, STATUS_BREAKPOINT, 1, BREAKPOINT_BREAK, 0, 0);
            break;

        case KI_BRK_ASSERTION:
            TrapFrame->Pc += sizeof(ULONG);
            KiArm64DispatchTrapException(TrapFrame, ExceptionFrame, STATUS_ASSERTION_FAILURE, 0, 0, 0, 0);
            break;

        case KI_BRK_DEBUG_SERVICE:
            /* The debugger steps over the breakpoint that follows the service one */
            TrapFrame->Pc += sizeof(ULONG);
            KiArm64DispatchTrapException(TrapFrame,
                                         ExceptionFrame,
                                         STATUS_BREAKPOINT,
                                         3,
                                         TrapFrame->X16,
                                         TrapFrame->X0,
                                         TrapFrame->X1);
            break;

        case KI_BRK_FASTFAIL:
            if (!KiUserTrap(TrapFrame))
                KeBugCheckEx(KERNEL_SECURITY_CHECK_FAILURE, TrapFrame->X0, (ULONG_PTR)TrapFrame, (ULONG_PTR)ExceptionFrame, 0);
            KiArm64DispatchTrapException(TrapFrame, ExceptionFrame, STATUS_STACK_BUFFER_OVERRUN, 1, TrapFrame->X0, 0, 0);
            break;

        case KI_BRK_DIVIDE_BY_ZERO:
            KiArm64DispatchTrapException(TrapFrame, ExceptionFrame, STATUS_INTEGER_DIVIDE_BY_ZERO, 0, 0, 0, 0);
            break;

        default:
            KiArm64DispatchTrapException(TrapFrame, ExceptionFrame, STATUS_BREAKPOINT, 1, BREAKPOINT_BREAK, 0, 0);
            break;
    }
}

/**
 * @brief Runs a system service, the trap frame moves when the thread becomes a GUI thread.
 */
static
PKTRAP_FRAME
KiArm64SystemService(
    _Inout_ PKTRAP_FRAME TrapFrame)
{
    PKSERVICE_TABLE_DESCRIPTOR DescriptorTable;
    ULONG_PTR Arguments[MAX_SYSCALL_ARGUMENTS];
    KPROCESSOR_MODE PreviousMode, OldMode;
    ULONG ServiceNumber, TableIndex, Count, i;
    PULONG_PTR StackArguments;
    PKTHREAD Thread;
    NTSTATUS Status;

    Thread = KeGetCurrentThread();
    KeGetCurrentPrcb()->KeSystemCalls++;

    PreviousMode = KiGetPreviousMode(TrapFrame);
    OldMode = Thread->PreviousMode;
    Thread->PreviousMode = PreviousMode;
    TrapFrame->PreviousMode = PreviousMode;

    TrapFrame->TrapFrame = (ULONG64)Thread->TrapFrame;
    Thread->TrapFrame = TrapFrame;

    if (!(TrapFrame->Spsr & ARM64_PSR_I))
        _enable();

    ServiceNumber = TrapFrame->Esr & 0xFFFF;
    TableIndex = (ServiceNumber >> TABLE_OFFSET_BITS) & ((1 << TABLE_NUMBER_BITS) - 1);
    ServiceNumber &= SERVICE_NUMBER_MASK;

    if ((TableIndex == WIN32K_SERVICE_INDEX) && (PreviousMode == UserMode))
    {
        ULONG GdiBatchCount;

        _SEH2_TRY
        {
            GdiBatchCount = NtCurrentTeb()->GdiBatchCount;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            GdiBatchCount = 0;
        }
        _SEH2_END;

        if (GdiBatchCount)
            KeGdiFlushUserBatch();
    }

    /* KiArm64HandleSynchronous already made the thread a GUI thread when it had to */
    DescriptorTable = &((PKSERVICE_TABLE_DESCRIPTOR)Thread->ServiceTable)[TableIndex];
    if (ServiceNumber >= DescriptorTable->Limit)
    {
        Status = STATUS_INVALID_SYSTEM_SERVICE;
        goto Exit;
    }

    /* The first eight arguments come in registers, the rest from the caller's stack */
    Count = DescriptorTable->Number[ServiceNumber] / sizeof(ULONG_PTR);
    ASSERT(Count <= MAX_SYSCALL_ARGUMENTS);

    for (i = 0; i < 8; i++)
        Arguments[i] = TrapFrame->X[i];
    for (; i < MAX_SYSCALL_ARGUMENTS; i++)
        Arguments[i] = 0;

    if (Count > 8)
    {
        StackArguments = (PULONG_PTR)TrapFrame->Sp;

        _SEH2_TRY
        {
            if (PreviousMode == UserMode)
                ProbeForRead(StackArguments, (Count - 8) * sizeof(ULONG_PTR), sizeof(ULONG_PTR));

            for (i = 8; i < Count; i++)
                Arguments[i] = StackArguments[i - 8];
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
            _SEH2_YIELD(goto Exit);
        }
        _SEH2_END;
    }

    Status = ((PKI_SYSTEM_SERVICE)DescriptorTable->Base[ServiceNumber])(
        Arguments[0], Arguments[1], Arguments[2], Arguments[3],
        Arguments[4], Arguments[5], Arguments[6], Arguments[7],
        Arguments[8], Arguments[9], Arguments[10], Arguments[11],
        Arguments[12], Arguments[13], Arguments[14], Arguments[15]);

Exit:
    TrapFrame->X0 = (ULONG_PTR)(LONG_PTR)Status;
    Thread->TrapFrame = KiGetLinkedTrapFrame(TrapFrame);
    Thread->PreviousMode = OldMode;
    return TrapFrame;
}

/**
 * @brief Tells whether a system service needs the thread converted to a GUI thread first.
 *
 * The conversion moves the kernel stack, which only a caller without pointers
 * into the old stack in its registers survives. KiArm64SystemService uses
 * SEH, so its locals are addressed through a register the move does not fix.
 */
static
BOOLEAN
KiArm64ServiceNeedsGuiThread(
    _In_ PKTRAP_FRAME TrapFrame)
{
    PKTHREAD Thread = KeGetCurrentThread();
    ULONG ServiceNumber, TableIndex;

    ServiceNumber = TrapFrame->Esr & 0xFFFF;
    TableIndex = (ServiceNumber >> TABLE_OFFSET_BITS) & ((1 << TABLE_NUMBER_BITS) - 1);
    ServiceNumber &= SERVICE_NUMBER_MASK;

    return (TableIndex == WIN32K_SERVICE_INDEX) &&
           (Thread->ServiceTable == KeServiceDescriptorTable) &&
           (ServiceNumber >= KeServiceDescriptorTable[TableIndex].Limit);
}

/**
 * @brief Handles a synchronous exception and returns the trap frame to exit with.
 *
 * A system service can move the thread to a large stack, so the caller must
 * not keep any pointer into the stack across this call.
 */
static
PKTRAP_FRAME
KiArm64HandleSynchronous(
    _Inout_ PKTRAP_FRAME TrapFrame,
    _Inout_ PKEXCEPTION_FRAME ExceptionFrame)
{
    ULONG ExceptionClass = TrapFrame->Esr >> ESR_EC_SHIFT;
    PKTHREAD Thread = KeGetCurrentThread();
    PKTRAP_FRAME OldTrapFrame;
    KPROCESSOR_MODE OldMode;
    ULONG_PTR OldBase, OldLimit;
    NTSTATUS Status;

    if (ExceptionClass == ESR_EC_SVC64)
    {
        if (KiArm64ServiceNeedsGuiThread(TrapFrame))
        {
            /* The conversion calls back into user mode, so the frame is linked like for a service */
            OldTrapFrame = Thread->TrapFrame;
            OldMode = Thread->PreviousMode;
            Thread->TrapFrame = TrapFrame;
            Thread->PreviousMode = KiGetPreviousMode(TrapFrame);

            if (!(TrapFrame->Spsr & ARM64_PSR_I))
                _enable();

            OldBase = (ULONG_PTR)Thread->StackBase;
            OldLimit = Thread->StackLimit;
            Status = PsConvertToGuiThread();

            /* Everything on the old stack moved along, the stack switch fixed the current frame */
            TrapFrame = Thread->TrapFrame;
            if (((ULONG_PTR)OldTrapFrame >= OldLimit) && ((ULONG_PTR)OldTrapFrame < OldBase))
                OldTrapFrame = (PKTRAP_FRAME)((ULONG_PTR)OldTrapFrame + ((ULONG_PTR)Thread->StackBase - OldBase));

            Thread->TrapFrame = OldTrapFrame;
            Thread->PreviousMode = OldMode;

            if (!NT_SUCCESS(Status))
            {
                TrapFrame->X0 = (ULONG_PTR)(LONG_PTR)Status;
                return TrapFrame;
            }
        }

        return KiArm64SystemService(TrapFrame);
    }

    if ((ExceptionClass == ESR_EC_DABT_LOWER) || (ExceptionClass == ESR_EC_DABT_CURRENT) ||
        (ExceptionClass == ESR_EC_IABT_LOWER) || (ExceptionClass == ESR_EC_IABT_CURRENT))
    {
        KiArm64HandleAbort(TrapFrame, ExceptionFrame, ExceptionClass);
        return TrapFrame;
    }

    /* Everything else runs with the interrupt state of the faulting code */
    if (!(TrapFrame->Spsr & ARM64_PSR_I))
        _enable();

    switch (ExceptionClass)
    {
        case ESR_EC_BRK64:
            KiArm64HandleBreak(TrapFrame, ExceptionFrame);
            break;

        case ESR_EC_PC_ALIGN:
        case ESR_EC_SP_ALIGN:
            KiArm64DispatchTrapException(TrapFrame, ExceptionFrame, STATUS_DATATYPE_MISALIGNMENT, 0, 0, 0, 0);
            break;

        case ESR_EC_STEP_LOWER:
        case ESR_EC_STEP_CURRENT:
            KiArm64DispatchTrapException(TrapFrame, ExceptionFrame, STATUS_SINGLE_STEP, 0, 0, 0, 0);
            break;

        case ESR_EC_FP_EXCEPTION:
            KiArm64DispatchTrapException(TrapFrame, ExceptionFrame, STATUS_FLOAT_INVALID_OPERATION, 0, 0, 0, 0);
            break;

        case ESR_EC_SERROR:
            KeBugCheckEx(UNEXPECTED_KERNEL_MODE_TRAP, TrapFrame->Esr, TrapFrame->Pc, TrapFrame->FaultAddress, 0);

        default:
            KiArm64DispatchTrapException(TrapFrame, ExceptionFrame, STATUS_ILLEGAL_INSTRUCTION, 0, 0, 0, 0);
            break;
    }

    return TrapFrame;
}

static
VOID
KiArm64DpcInterrupt(VOID)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    PKTHREAD NewThread, OldThread;

    if ((Prcb->DpcData[0].DpcQueueDepth) ||
        (Prcb->TimerRequest) ||
        (Prcb->DeferredReadyListHead.Next))
    {
        _disable();
        KiRetireDpcList(Prcb);
        _enable();
    }

    if (Prcb->QuantumEnd)
    {
        Prcb->QuantumEnd = FALSE;
        KiQuantumEnd();
    }
    else if (Prcb->NextThread)
    {
        KiAcquirePrcbLock(Prcb);

        OldThread = Prcb->CurrentThread;
        NewThread = Prcb->NextThread;
        Prcb->NextThread = NULL;
        Prcb->CurrentThread = NewThread;
        NewThread->State = Running;
        OldThread->WaitReason = WrDispatchInt;

        KxQueueReadyThread(OldThread, Prcb);
        KiSwapContext(APC_LEVEL, OldThread);
    }
}

/* Software interrupts and devices all come through the GIC */
static
VOID
KiArm64InterruptDispatch(
    _Inout_ PKTRAP_FRAME TrapFrame,
    _Inout_ PKEXCEPTION_FRAME ExceptionFrame)
{
    PKIPCR Pcr = (PKIPCR)KeGetPcr();
    PKINTERRUPT Interrupt = NULL;
    PLIST_ENTRY Entry;
    KIRQL OldIrql, Irql;
    CHAR PreviousMode;
    ULONG Vector;

    Vector = HalGetInterruptSource();
    if (Vector >= 1020)
        return;

    Pcr->Prcb.InterruptCount++;

    if ((Vector == APC_LEVEL) || (Vector == DISPATCH_LEVEL) || (Vector == IPI_LEVEL))
    {
        Irql = (KIRQL)Vector;
    }
    else
    {
        if (Vector < RTL_NUMBER_OF(Pcr->Idt))
            Interrupt = Pcr->Idt[Vector];

        if (!Interrupt)
        {
            HalBeginSystemInterrupt(HIGH_LEVEL, Vector, &OldIrql);
            HalEndSystemInterrupt(OldIrql, TrapFrame);
            DPRINT1("Unexpected interrupt %lu\n", Vector);
            return;
        }

        Irql = Interrupt->Irql;
    }

    if (!HalBeginSystemInterrupt(Irql, Vector, &OldIrql))
        return;

    /* Interrupt frames carry the IRQL they interrupted, the clock reads it */
    PreviousMode = TrapFrame->PreviousMode;
    TrapFrame->PreviousIrql = OldIrql;
    _enable();

    switch (Vector)
    {
        case APC_LEVEL:
            KiDeliverApc(KernelMode, NULL, TrapFrame);
            break;

        case DISPATCH_LEVEL:
            KiArm64DpcInterrupt();
            break;

        case IPI_LEVEL:
            if (!KiProcessorFreezeHandler(TrapFrame, ExceptionFrame))
                KiIpiServiceRoutine(TrapFrame, ExceptionFrame);
            break;

        default:
            /* The first object heads the list of the ones sharing the vector */
            Entry = &Interrupt->InterruptListEntry;
            do
            {
                Interrupt = CONTAINING_RECORD(Entry, KINTERRUPT, InterruptListEntry);
                Interrupt->TrapFrame = TrapFrame;
                KeAcquireSpinLockAtDpcLevel(Interrupt->ActualLock);
                Interrupt->ServiceRoutine(Interrupt, Interrupt->ServiceContext);
                KeReleaseSpinLockFromDpcLevel(Interrupt->ActualLock);
                Entry = Entry->Flink;
            } while (Entry != &((PKINTERRUPT)Pcr->Idt[Vector])->InterruptListEntry);
            break;
    }

    _disable();
    HalEndSystemInterrupt(OldIrql, TrapFrame);
    TrapFrame->PreviousMode = PreviousMode;
}

/**
 * @brief Delivers user APCs and checks the IRQL before a return to user mode.
 *
 * Returns with interrupts off, the trap exit restores the user state.
 */
VOID
NTAPI
KiArm64ExitToUser(
    _Inout_ PKTRAP_FRAME TrapFrame,
    _Inout_ PKEXCEPTION_FRAME ExceptionFrame)
{
    PKTHREAD Thread = KeGetCurrentThread();
    KIRQL OldIrql;

    for (;;)
    {
        _disable();
        if (!Thread->ApcState.UserApcPending)
            break;

        OldIrql = KfRaiseIrql(APC_LEVEL);
        _enable();
        KiDeliverApc(UserMode, ExceptionFrame, TrapFrame);
        _disable();
        KeLowerIrql(OldIrql);
    }

    if (KeGetCurrentIrql() != PASSIVE_LEVEL)
    {
        KeBugCheckEx(IRQL_GT_ZERO_AT_SYSTEM_SERVICE,
                     TrapFrame->Esr & 0xFFFF,
                     KeGetCurrentIrql(),
                     0,
                     0);
    }
}

/**
 * @brief C side of every trap, called by KiArm64TrapSave.
 */
VOID
NTAPI
KiArm64TrapDispatch(
    _Inout_ PKTRAP_FRAME TrapFrame,
    _Inout_ PKEXCEPTION_FRAME ExceptionFrame,
    _In_ ULONG TrapType)
{
    switch (TrapType)
    {
        case KI_TRAP_SYNCHRONOUS:
            TrapFrame = KiArm64HandleSynchronous(TrapFrame, ExceptionFrame);
            ExceptionFrame = KiArm64TrapExceptionFrame(TrapFrame);
            break;

        case KI_TRAP_IRQ:
        case KI_TRAP_FIQ:
            KiArm64InterruptDispatch(TrapFrame, ExceptionFrame);
            break;

        default:
            KeBugCheckEx(UNEXPECTED_KERNEL_MODE_TRAP, TrapType, TrapFrame->Esr, TrapFrame->Pc, (ULONG_PTR)TrapFrame);
    }

    if (KiUserTrap(TrapFrame))
        KiArm64ExitToUser(TrapFrame, ExceptionFrame);

    _disable();
}
