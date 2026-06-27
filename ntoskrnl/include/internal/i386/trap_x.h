/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/include/internal/i386/trap_x.h
 * PURPOSE:         Internal Inlined Functions for the Trap Handling Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#pragma once

#if defined(_MSC_VER)
#define               __builtin_expect(a,b) (a)
#endif

//
// Helper Code
//
FORCEINLINE
BOOLEAN
KiUserTrap(IN PKTRAP_FRAME TrapFrame)
{
    /* Anything else but Ring 0 is Ring 3 */
    return !!(TrapFrame->SegCs & MODE_MASK);
}

//
// Debug Macros
//
FORCEINLINE
VOID
KiDumpTrapFrame(IN PKTRAP_FRAME TrapFrame)
{
    /* Dump the whole thing */
    DbgPrint("DbgEbp: %x\n", TrapFrame->DbgEbp);
    DbgPrint("DbgEip: %x\n", TrapFrame->DbgEip);
    DbgPrint("DbgArgMark: %x\n", TrapFrame->DbgArgMark);
    DbgPrint("DbgArgPointer: %x\n", TrapFrame->DbgArgPointer);
    DbgPrint("TempSegCs: %x\n", TrapFrame->TempSegCs);
    DbgPrint("TempEsp: %x\n", TrapFrame->TempEsp);
    DbgPrint("Dr0: %x\n", TrapFrame->Dr0);
    DbgPrint("Dr1: %x\n", TrapFrame->Dr1);
    DbgPrint("Dr2: %x\n", TrapFrame->Dr2);
    DbgPrint("Dr3: %x\n", TrapFrame->Dr3);
    DbgPrint("Dr6: %x\n", TrapFrame->Dr6);
    DbgPrint("Dr7: %x\n", TrapFrame->Dr7);
    DbgPrint("SegGs: %x\n", TrapFrame->SegGs);
    DbgPrint("SegEs: %x\n", TrapFrame->SegEs);
    DbgPrint("SegDs: %x\n", TrapFrame->SegDs);
    DbgPrint("Edx: %x\n", TrapFrame->Edx);
    DbgPrint("Ecx: %x\n", TrapFrame->Ecx);
    DbgPrint("Eax: %x\n", TrapFrame->Eax);
    DbgPrint("PreviousPreviousMode: %x\n", TrapFrame->PreviousPreviousMode);
    DbgPrint("ExceptionList: %p\n", TrapFrame->ExceptionList);
    DbgPrint("SegFs: %x\n", TrapFrame->SegFs);
    DbgPrint("Edi: %x\n", TrapFrame->Edi);
    DbgPrint("Esi: %x\n", TrapFrame->Esi);
    DbgPrint("Ebx: %x\n", TrapFrame->Ebx);
    DbgPrint("Ebp: %x\n", TrapFrame->Ebp);
    DbgPrint("ErrCode: %x\n", TrapFrame->ErrCode);
    DbgPrint("Eip: %x\n", TrapFrame->Eip);
    DbgPrint("SegCs: %x\n", TrapFrame->SegCs);
    DbgPrint("EFlags: %x\n", TrapFrame->EFlags);
    DbgPrint("HardwareEsp: %x\n", TrapFrame->HardwareEsp);
    DbgPrint("HardwareSegSs: %x\n", TrapFrame->HardwareSegSs);
    DbgPrint("V86Es: %x\n", TrapFrame->V86Es);
    DbgPrint("V86Ds: %x\n", TrapFrame->V86Ds);
    DbgPrint("V86Fs: %x\n", TrapFrame->V86Fs);
    DbgPrint("V86Gs: %x\n", TrapFrame->V86Gs);
}

#if DBG
FORCEINLINE
VOID
KiFillTrapFrameDebug(IN PKTRAP_FRAME TrapFrame)
{
    /* Set the debug information */
    TrapFrame->DbgArgPointer = TrapFrame->Edx;
    TrapFrame->DbgArgMark = 0xBADB0D00;
    TrapFrame->DbgEip = TrapFrame->Eip;
    TrapFrame->DbgEbp = TrapFrame->Ebp;
    TrapFrame->PreviousPreviousMode = (ULONG)-1;
}

#define DR7_RESERVED_READ_AS_1 0x400

#define CheckDr(DrNumner, ExpectedValue) \
    { \
        ULONG DrValue = __readdr(DrNumner); \
        if (DrValue != (ExpectedValue)) \
        { \
            DbgPrint("Dr%ld: expected %.8lx, got %.8lx\n", \
                    DrNumner, ExpectedValue, DrValue); \
            __debugbreak(); \
        } \
    }

extern BOOLEAN StopChecking;

#if DBG && defined(_M_IX86) && !defined(_NTHAL_)
VOID
NTAPI
KiI386BootTraceRecord(
    _In_ ULONG Event,
    _In_ ULONG_PTR Arg0,
    _In_ ULONG_PTR Arg1,
    _In_ ULONG_PTR Arg2,
    _In_ ULONG_PTR Arg3,
    _In_ ULONG_PTR Arg4,
    _In_ ULONG_PTR Arg5);
#endif

#ifndef _NTHAL_
#define KI_TRAP_EXIT_PROBE_MAGIC 0x5058544B
#define KI_TRAP_EXIT_PROBE_MAXIMUM_PROCESSORS 32

typedef struct _KI_TRAP_EXIT_PROBE_SNAPSHOT
{
    ULONG Magic;
    ULONG Version;
    ULONG Count;
    ULONG Reason;
    ULONG Cpu;
    ULONG Irql;
    ULONG PcrIrql;
    ULONG PcrIrr;
    ULONG PcrIdr;
    ULONG SkipPreviousMode;
    ULONG EFlags;
    ULONG CurrentEFlags;
    ULONG SegCs;
    ULONG SegFs;
    ULONG ErrCode;
    ULONG Dr7;
    ULONG DbgArgMark;
    ULONG PreviousPreviousMode;
    ULONG_PTR TrapFrame;
    ULONG_PTR Thread;
    ULONG_PTR PcrExceptionList;
    ULONG_PTR TrapExceptionList;
    ULONG_PTR FsExceptionList;
    ULONG_PTR Eip;
    ULONG_PTR LinkedTrapFrame;
    ULONG_PTR StackPointer;
    ULONG_PTR TempEsp;
    ULONG_PTR HardwareEsp;
    ULONG_PTR Ebp;
    ULONG_PTR Eax;
    ULONG_PTR Ebx;
    ULONG_PTR Ecx;
    ULONG_PTR Edx;
    ULONG_PTR Esi;
    ULONG_PTR Edi;
    ULONG_PTR KernelStack;
    ULONG_PTR InitialStack;
    ULONG_PTR StackLimit;
} KI_TRAP_EXIT_PROBE_SNAPSHOT, *PKI_TRAP_EXIT_PROBE_SNAPSHOT;

extern volatile KI_TRAP_EXIT_PROBE_SNAPSHOT KiTrapExitProbeSnapshot;
extern volatile KI_TRAP_EXIT_PROBE_SNAPSHOT KiTrapExitProbeSnapshotByCpu
    [KI_TRAP_EXIT_PROBE_MAXIMUM_PROCESSORS];

#define KI_TRAP_EXIT_FINAL_SNAPSHOT_MAGIC 0x4658544B
#define KI_TRAP_EXIT_FINAL_SNAPSHOT_MAXIMUM_PROCESSORS 32

typedef struct _KI_TRAP_EXIT_FINAL_SNAPSHOT
{
    ULONG Magic;
    ULONG Version;
    ULONG Sequence;
    ULONG Stage;
    ULONG Cpu;
    ULONG Flags;
    ULONG OffsetEsp;
    ULONG_PTR TrapFrame;
    ULONG_PTR Eip;
    ULONG_PTR SegCs;
    ULONG_PTR EFlags;
    ULONG_PTR HardwareEsp;
    ULONG_PTR HardwareSegSs;
    ULONG_PTR SegDs;
    ULONG_PTR SegEs;
    ULONG_PTR SegFs;
    ULONG_PTR SegGs;
    ULONG_PTR Eax;
    ULONG_PTR Ecx;
    ULONG_PTR Edx;
    ULONG_PTR LiveEsp;
    UCHAR Gdtr[6];
    UCHAR Idtr[6];
    ULONG_PTR Cr0;
    ULONG_PTR Cr2;
    ULONG_PTR Cr3;
    ULONG_PTR Cr4;
    ULONG_PTR Tr;
    ULONG_PTR GdtCsLow;
    ULONG_PTR GdtCsHigh;
    ULONG_PTR GdtSsLow;
    ULONG_PTR GdtSsHigh;
    ULONG_PTR GdtFsLow;
    ULONG_PTR GdtFsHigh;
    ULONG_PTR GdtTrLow;
    ULONG_PTR GdtTrHigh;
    ULONG_PTR TssBase;
    ULONG_PTR TssEsp0;
} KI_TRAP_EXIT_FINAL_SNAPSHOT, *PKI_TRAP_EXIT_FINAL_SNAPSHOT;

extern volatile ULONG KiTrapExitFinalSnapshotCount;
extern volatile KI_TRAP_EXIT_FINAL_SNAPSHOT KiTrapExitFinalSnapshotByCpu
    [KI_TRAP_EXIT_FINAL_SNAPSHOT_MAXIMUM_PROCESSORS];
#endif

#define KI_TRAP_RAW_COM1_BASE 0x3F8
#define KI_TRAP_RAW_COM1_LINE_STATUS 5
#define KI_TRAP_RAW_COM1_TRANSMIT_EMPTY 0x20

FORCEINLINE
VOID
KiTrapRawCom1WriteByte(
    _In_ UCHAR Character)
{
    ULONG SpinCount = 100000;

    while (SpinCount-- != 0)
    {
        if (READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)(KI_TRAP_RAW_COM1_BASE +
                                                KI_TRAP_RAW_COM1_LINE_STATUS)) &
            KI_TRAP_RAW_COM1_TRANSMIT_EMPTY)
        {
            break;
        }
    }

    WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)KI_TRAP_RAW_COM1_BASE, Character);
}

FORCEINLINE
VOID
KiTrapRawCom1WriteString(
    _In_z_ const CHAR *String)
{
    while (*String != ANSI_NULL)
    {
        if (*String == '\n')
            KiTrapRawCom1WriteByte('\r');

        KiTrapRawCom1WriteByte(*String++);
    }
}

FORCEINLINE
VOID
KiTrapRawCom1WriteHex(
    _In_ ULONG_PTR Value)
{
    ULONG Index;

    for (Index = 0; Index < 8; Index++)
    {
        ULONG Nibble = (Value >> (28 - Index * 4)) & 0xF;

        KiTrapRawCom1WriteByte((UCHAR)(Nibble < 10 ? ('0' + Nibble) :
                                                   ('A' + Nibble - 10)));
    }
}

FORCEINLINE
VOID
KiTrapRawCom1WriteField(
    _In_z_ const CHAR *Name,
    _In_ ULONG_PTR Value)
{
    KiTrapRawCom1WriteByte(' ');
    KiTrapRawCom1WriteString(Name);
    KiTrapRawCom1WriteByte('=');
    KiTrapRawCom1WriteHex(Value);
}

#ifndef _NTHAL_
FORCEINLINE
VOID
KiTrapStoreExitProbeSnapshot(
    _Out_ volatile KI_TRAP_EXIT_PROBE_SNAPSHOT *Snapshot,
    _In_ ULONG Reason,
    _In_ PKTRAP_FRAME TrapFrame,
    _In_ BOOLEAN SkipPreviousMode,
    _In_ ULONG Count)
{
    PKPCR Pcr = KeGetPcr();
    PKTHREAD Thread = KeGetCurrentThread();

    Snapshot->Magic = KI_TRAP_EXIT_PROBE_MAGIC;
    Snapshot->Version = 1;
    Snapshot->Count = Count;
    Snapshot->Reason = Reason;
    Snapshot->Cpu = KeGetCurrentProcessorNumber();
    Snapshot->Irql = KeGetCurrentIrql();
    Snapshot->PcrIrql = Pcr->Irql;
    Snapshot->PcrIrr = Pcr->IRR;
    Snapshot->PcrIdr = Pcr->IDR;
    Snapshot->SkipPreviousMode = SkipPreviousMode;
    Snapshot->EFlags = TrapFrame->EFlags;
    Snapshot->CurrentEFlags = __readeflags();
    Snapshot->SegCs = TrapFrame->SegCs;
    Snapshot->SegFs = TrapFrame->SegFs;
    Snapshot->ErrCode = TrapFrame->ErrCode;
    Snapshot->Dr7 = TrapFrame->Dr7;
    Snapshot->DbgArgMark = TrapFrame->DbgArgMark;
    Snapshot->PreviousPreviousMode = TrapFrame->PreviousPreviousMode;
    Snapshot->TrapFrame = (ULONG_PTR)TrapFrame;
    Snapshot->Thread = (ULONG_PTR)Thread;
    Snapshot->PcrExceptionList = (ULONG_PTR)Pcr->NtTib.ExceptionList;
    Snapshot->TrapExceptionList = (ULONG_PTR)TrapFrame->ExceptionList;
    Snapshot->FsExceptionList =
        __readfsdword(FIELD_OFFSET(KPCR, NtTib.ExceptionList));
    Snapshot->Eip = TrapFrame->Eip;
    Snapshot->LinkedTrapFrame = TrapFrame->Edx;
    Snapshot->StackPointer = KeGetTrapFrameStackRegister(TrapFrame);
    Snapshot->TempEsp = TrapFrame->TempEsp;
    Snapshot->HardwareEsp = TrapFrame->HardwareEsp;
    Snapshot->Ebp = TrapFrame->Ebp;
    Snapshot->Eax = TrapFrame->Eax;
    Snapshot->Ebx = TrapFrame->Ebx;
    Snapshot->Ecx = TrapFrame->Ecx;
    Snapshot->Edx = TrapFrame->Edx;
    Snapshot->Esi = TrapFrame->Esi;
    Snapshot->Edi = TrapFrame->Edi;
    Snapshot->KernelStack = (ULONG_PTR)Thread->KernelStack;
    Snapshot->InitialStack = (ULONG_PTR)Thread->InitialStack;
    Snapshot->StackLimit = (ULONG_PTR)Thread->StackLimit;
}
#endif

FORCEINLINE
VOID
KiTrapRawCom1DumpExit(
    _In_ ULONG Reason,
    _In_ PKTRAP_FRAME TrapFrame,
    _In_ BOOLEAN SkipPreviousMode)
{
    PKPCR Pcr = KeGetPcr();
    PKTHREAD Thread = KeGetCurrentThread();
    ULONG Cpu = KeGetCurrentProcessorNumber();
#ifndef _NTHAL_
    ULONG Count = KiTrapExitProbeSnapshot.Count + 1;

    /* Persistent state survives concurrent serial writes from other processors. */
    KiTrapStoreExitProbeSnapshot(&KiTrapExitProbeSnapshot,
                                 Reason,
                                 TrapFrame,
                                 SkipPreviousMode,
                                 Count);
    if (Cpu < KI_TRAP_EXIT_PROBE_MAXIMUM_PROCESSORS)
    {
        KiTrapStoreExitProbeSnapshot(&KiTrapExitProbeSnapshotByCpu[Cpu],
                                     Reason,
                                     TrapFrame,
                                     SkipPreviousMode,
                                     Count);
    }
#endif

    /* Raw serial output preserves early evidence when KDBG formatting faults. */
    KiTrapRawCom1WriteString("\nKiRawTrapExit");
    KiTrapRawCom1WriteField("reason", Reason);
    KiTrapRawCom1WriteField("cpu", Cpu);
    KiTrapRawCom1WriteField("irql", KeGetCurrentIrql());
    KiTrapRawCom1WriteField("pcrirql", Pcr->Irql);
    KiTrapRawCom1WriteField("irr", Pcr->IRR);
    KiTrapRawCom1WriteField("idr", Pcr->IDR);
    KiTrapRawCom1WriteField("skip", SkipPreviousMode);
    KiTrapRawCom1WriteField("tf", (ULONG_PTR)TrapFrame);
    KiTrapRawCom1WriteField("thread", (ULONG_PTR)Thread);
    KiTrapRawCom1WriteField("pcrex", (ULONG_PTR)Pcr->NtTib.ExceptionList);
    KiTrapRawCom1WriteField("tfex", (ULONG_PTR)TrapFrame->ExceptionList);
    KiTrapRawCom1WriteField("fs0", __readfsdword(FIELD_OFFSET(KPCR, NtTib.ExceptionList)));
    KiTrapRawCom1WriteField("eip", TrapFrame->Eip);
    KiTrapRawCom1WriteField("cs", TrapFrame->SegCs);
    KiTrapRawCom1WriteField("eflags", TrapFrame->EFlags);
    KiTrapRawCom1WriteField("live", __readeflags());
    KiTrapRawCom1WriteField("err", TrapFrame->ErrCode);
    KiTrapRawCom1WriteField("fs", TrapFrame->SegFs);
    KiTrapRawCom1WriteField("dr7", TrapFrame->Dr7);
    KiTrapRawCom1WriteField("dbg", TrapFrame->DbgArgMark);
    KiTrapRawCom1WriteField("prev", TrapFrame->PreviousPreviousMode);
    KiTrapRawCom1WriteField("edx", TrapFrame->Edx);
    KiTrapRawCom1WriteField("esp", KeGetTrapFrameStackRegister(TrapFrame));
    KiTrapRawCom1WriteField("tempesp", TrapFrame->TempEsp);
    KiTrapRawCom1WriteField("hwesp", TrapFrame->HardwareEsp);
    KiTrapRawCom1WriteField("ebp", TrapFrame->Ebp);
    KiTrapRawCom1WriteField("eax", TrapFrame->Eax);
    KiTrapRawCom1WriteField("ebx", TrapFrame->Ebx);
    KiTrapRawCom1WriteField("ecx", TrapFrame->Ecx);
    KiTrapRawCom1WriteField("esi", TrapFrame->Esi);
    KiTrapRawCom1WriteField("edi", TrapFrame->Edi);
    KiTrapRawCom1WriteField("kstack", (ULONG_PTR)Thread->KernelStack);
    KiTrapRawCom1WriteField("istack", (ULONG_PTR)Thread->InitialStack);
    KiTrapRawCom1WriteField("slimit", (ULONG_PTR)Thread->StackLimit);
    KiTrapRawCom1WriteByte('\n');
}

FORCEINLINE
VOID
KiExitTrapDebugChecks(IN PKTRAP_FRAME TrapFrame,
                      IN BOOLEAN SkipPreviousMode)
{
    /* Don't check recursively */
    if (StopChecking) return;
    StopChecking = TRUE;

    /* Make sure interrupts are disabled */
    if (__readeflags() & EFLAGS_INTERRUPT_MASK)
    {
        KiTrapRawCom1DumpExit(1, TrapFrame, SkipPreviousMode);
        DbgPrint("Exiting with interrupts enabled: %lx\n", __readeflags());
        __debugbreak();
    }

    /* Make sure this is a real trap frame */
    if (TrapFrame->DbgArgMark != 0xBADB0D00)
    {
        KiTrapRawCom1DumpExit(2, TrapFrame, SkipPreviousMode);
        DbgPrint("Exiting with an invalid trap frame? (No MAGIC in trap frame)\n");
        KiDumpTrapFrame(TrapFrame);
        __debugbreak();
    }

    /* Make sure we're not in user-mode or something */
    if (Ke386GetFs() != KGDT_R0_PCR)
    {
        KiTrapRawCom1DumpExit(3, TrapFrame, SkipPreviousMode);
        DbgPrint("Exiting with an invalid FS: %lx\n", Ke386GetFs());
        __debugbreak();
    }

    /* Make sure we have a valid SEH chain */
    if (KeGetPcr()->NtTib.ExceptionList == 0)
    {
        KiTrapRawCom1DumpExit(4, TrapFrame, SkipPreviousMode);
        DbgPrint("Exiting with NULL exception chain: %p\n", KeGetPcr()->NtTib.ExceptionList);
        __debugbreak();
    }

    /* Make sure we're restoring a valid SEH chain */
    if (TrapFrame->ExceptionList == 0)
    {
        KiTrapRawCom1DumpExit(5, TrapFrame, SkipPreviousMode);
        DbgPrint("Entered a trap with a NULL exception chain: %p\n", TrapFrame->ExceptionList);
        __debugbreak();
    }

    /* If we're ignoring previous mode, make sure caller doesn't actually want it */
    if (SkipPreviousMode && (TrapFrame->PreviousPreviousMode != (ULONG)-1))
    {
        KiTrapRawCom1DumpExit(6, TrapFrame, SkipPreviousMode);
        DbgPrint("Exiting a trap witout restoring previous mode, yet previous mode seems valid: %lx\n", TrapFrame->PreviousPreviousMode);
        __debugbreak();
    }

    /* FIXME: KDBG messes around with these improperly */
#if !defined(KDBG)
    /* Check DR values */
    if (KiUserTrap(TrapFrame))
    {
        /* Check for active debugging */
        if (KeGetCurrentThread()->Header.DebugActive)
        {
            if ((TrapFrame->Dr7 & ~DR7_RESERVED_MASK) == 0) __debugbreak();

            CheckDr(0, TrapFrame->Dr0);
            CheckDr(1, TrapFrame->Dr1);
            CheckDr(2, TrapFrame->Dr2);
            CheckDr(3, TrapFrame->Dr3);
            CheckDr(7, TrapFrame->Dr7 | DR7_RESERVED_READ_AS_1);
        }
    }
    else
    {
        PKPRCB Prcb = KeGetCurrentPrcb();
        CheckDr(0, Prcb->ProcessorState.SpecialRegisters.KernelDr0);
        CheckDr(1, Prcb->ProcessorState.SpecialRegisters.KernelDr1);
        CheckDr(2, Prcb->ProcessorState.SpecialRegisters.KernelDr2);
        CheckDr(3, Prcb->ProcessorState.SpecialRegisters.KernelDr3);
        // CheckDr(7, Prcb->ProcessorState.SpecialRegisters.KernelDr7); // Disabled, see CORE-10165 for more details.
    }
#endif

    StopChecking = FALSE;
}

#else
#define KiExitTrapDebugChecks(x, y)
#define KiFillTrapFrameDebug(x)
#endif

FORCEINLINE
VOID
KiExitSystemCallDebugChecks(IN ULONG SystemCall,
                            IN PKTRAP_FRAME TrapFrame)
{
    KIRQL OldIrql;

    /* Check if this was a user call */
    if (KiUserTrap(TrapFrame))
    {
        /* Make sure we are not returning with elevated IRQL */
        OldIrql = KeGetCurrentIrql();
        if (OldIrql != PASSIVE_LEVEL)
        {
            /* Forcibly put us in a sane state */
            KeGetPcr()->Irql = PASSIVE_LEVEL;
            _disable();

            /* Fail */
            KeBugCheckEx(IRQL_GT_ZERO_AT_SYSTEM_SERVICE,
                         SystemCall,
                         OldIrql,
                         0,
                         0);
        }

        /* Make sure we're not attached and that APCs are not disabled */
        if ((KeGetCurrentThread()->ApcStateIndex != OriginalApcEnvironment) ||
            (KeGetCurrentThread()->CombinedApcDisable != 0))
        {
            /* Fail */
            KeBugCheckEx(APC_INDEX_MISMATCH,
                         SystemCall,
                         KeGetCurrentThread()->ApcStateIndex,
                         KeGetCurrentThread()->CombinedApcDisable,
                         0);
        }
    }
}

//
// Generic Exit Routine
//
DECLSPEC_NORETURN VOID FASTCALL KiSystemCallReturn(IN PKTRAP_FRAME TrapFrame);
DECLSPEC_NORETURN VOID FASTCALL KiSystemCallSysExitReturn(IN PKTRAP_FRAME TrapFrame);
DECLSPEC_NORETURN VOID FASTCALL KiSystemCallTrapReturn(IN PKTRAP_FRAME TrapFrame);
DECLSPEC_NORETURN VOID FASTCALL KiEditedTrapReturn(IN PKTRAP_FRAME TrapFrame);
DECLSPEC_NORETURN VOID FASTCALL KiTrapReturn(IN PKTRAP_FRAME TrapFrame);
DECLSPEC_NORETURN VOID FASTCALL KiTrapReturnNoSegments(IN PKTRAP_FRAME TrapFrame);
DECLSPEC_NORETURN VOID FASTCALL KiTrapReturnNoSegmentsRet8(IN PKTRAP_FRAME TrapFrame);

typedef
VOID
(FASTCALL *PFAST_SYSTEM_CALL_EXIT)(
    IN PKTRAP_FRAME TrapFrame
);

extern PFAST_SYSTEM_CALL_EXIT KiFastCallExitHandler;

//
// Save user mode debug registers and restore kernel values
//
FORCEINLINE
VOID
KiHandleDebugRegistersOnTrapEntry(
    IN PKTRAP_FRAME TrapFrame)
{
    PKPRCB Prcb = KeGetCurrentPrcb();

    /* Save all debug registers in the trap frame */
    TrapFrame->Dr0 = __readdr(0);
    TrapFrame->Dr1 = __readdr(1);
    TrapFrame->Dr2 = __readdr(2);
    TrapFrame->Dr3 = __readdr(3);
    TrapFrame->Dr6 = __readdr(6);
    TrapFrame->Dr7 = __readdr(7);

    /* Disable all active debugging */
    __writedr(7, 0);

    /* Restore kernel values */
    __writedr(0, Prcb->ProcessorState.SpecialRegisters.KernelDr0);
    __writedr(1, Prcb->ProcessorState.SpecialRegisters.KernelDr1);
    __writedr(2, Prcb->ProcessorState.SpecialRegisters.KernelDr2);
    __writedr(3, Prcb->ProcessorState.SpecialRegisters.KernelDr3);
    __writedr(6, Prcb->ProcessorState.SpecialRegisters.KernelDr6);
    __writedr(7, Prcb->ProcessorState.SpecialRegisters.KernelDr7);
}

FORCEINLINE
VOID
KiHandleDebugRegistersOnTrapExit(
    PKTRAP_FRAME TrapFrame)
{
    /* Disable all active debugging */
    __writedr(7, 0);

    /* Load all debug registers from the trap frame */
    __writedr(0, TrapFrame->Dr0);
    __writedr(1, TrapFrame->Dr1);
    __writedr(2, TrapFrame->Dr2);
    __writedr(3, TrapFrame->Dr3);
    __writedr(6, TrapFrame->Dr6);
    __writedr(7, TrapFrame->Dr7);
}

//
// Virtual 8086 Mode Optimized Trap Exit
//
FORCEINLINE
DECLSPEC_NORETURN
VOID
KiExitV86Trap(IN PKTRAP_FRAME TrapFrame)
{
    PKTHREAD Thread;
    KIRQL OldIrql;

    /* Get the thread */
    Thread = KeGetCurrentThread();
    while (TRUE)
    {
        /* Return if this isn't V86 mode anymore */
        if (!(TrapFrame->EFlags & EFLAGS_V86_MASK)) KiEoiHelper(TrapFrame);

        /* Turn off the alerted state for kernel mode */
        Thread->Alerted[KernelMode] = FALSE;

        /* Are there pending user APCs? */
        if (__builtin_expect(!Thread->ApcState.UserApcPending, 1)) break;

        /* Raise to APC level and enable interrupts */
        OldIrql = KfRaiseIrql(APC_LEVEL);
        _enable();

        /* Deliver APCs */
        KiDeliverApc(UserMode, NULL, TrapFrame);

        /* Restore IRQL and disable interrupts once again */
        KfLowerIrql(OldIrql);
        _disable();
    }

    /* If we got here, we're still in a valid V8086 context, so quit it */
    if (__builtin_expect(TrapFrame->Dr7 & ~DR7_RESERVED_MASK, 0))
    {
        /* Restore debug registers from the trap frame */
        KiHandleDebugRegistersOnTrapExit(TrapFrame);
    }

    /* Return from interrupt */
    KiTrapReturnNoSegments(TrapFrame);
}

//
// Virtual 8086 Mode Optimized Trap Entry
//
FORCEINLINE
VOID
KiEnterV86Trap(IN PKTRAP_FRAME TrapFrame)
{
    PVOID ExceptionList;

    /* Check exception list */
    ExceptionList = KeGetPcr()->NtTib.ExceptionList;
    ASSERTMSG("V86 trap handler must not register an SEH frame\n",
              ExceptionList == TrapFrame->ExceptionList);

    /* Save DR7 and check for debugging */
    TrapFrame->Dr7 = __readdr(7);
    if (__builtin_expect(TrapFrame->Dr7 & ~DR7_RESERVED_MASK, 0))
    {
        /* Handle debug registers */
        KiHandleDebugRegistersOnTrapEntry(TrapFrame);
    }
}

//
// Interrupt Trap Entry
//
FORCEINLINE
VOID
KiEnterInterruptTrap(IN PKTRAP_FRAME TrapFrame)
{
    PVOID ExceptionList;

    /* Check exception list and terminate it */
    ExceptionList = KeGetPcr()->NtTib.ExceptionList;
#if DBG && defined(_M_IX86) && !defined(_NTHAL_)
    KiI386BootTraceRecord(0xE110,
                          (ULONG_PTR)TrapFrame,
                          TrapFrame->Eip,
                          (ULONG_PTR)TrapFrame->ExceptionList,
                          (ULONG_PTR)ExceptionList,
                          TrapFrame->EFlags,
                          (ULONG_PTR)KeGetCurrentThread());
#endif
#if DBG
    if (ExceptionList != TrapFrame->ExceptionList)
    {
        DbgPrint("KiEnterInterruptTrap mismatch: cpu=%u fsEx=%p pcrEx=%p "
                 "tfEx=%p eip=%lx cs=%lx eflags=%lx fs=%lx irql=%u\n",
                 KeGetCurrentProcessorNumber(),
                 (PVOID)__readfsdword(FIELD_OFFSET(KPCR, NtTib.ExceptionList)),
                 ExceptionList,
                 TrapFrame->ExceptionList,
                 TrapFrame->Eip,
                 TrapFrame->SegCs,
                 TrapFrame->EFlags,
                 TrapFrame->SegFs,
                 KeGetCurrentIrql());
    }
#endif
    ASSERTMSG("Interrupt handler must not register an SEH frame\n",
              ExceptionList == TrapFrame->ExceptionList);
    KeGetPcr()->NtTib.ExceptionList = EXCEPTION_CHAIN_END;

    /* Default to debugging disabled */
    TrapFrame->Dr7 = 0;

    /* Check if the frame was from user mode or v86 mode */
    if (KiUserTrap(TrapFrame) ||
        (TrapFrame->EFlags & EFLAGS_V86_MASK))
    {
        /* Check for active debugging */
        if (KeGetCurrentThread()->Header.DebugActive & 0xFF)
        {
            /* Handle debug registers */
            KiHandleDebugRegistersOnTrapEntry(TrapFrame);
        }
    }

    /* Set debug header */
    KiFillTrapFrameDebug(TrapFrame);

#if DBG && defined(_M_IX86) && !defined(_NTHAL_)
    KiI386BootTraceRecord(0xE111,
                          (ULONG_PTR)TrapFrame,
                          TrapFrame->Eip,
                          (ULONG_PTR)TrapFrame->ExceptionList,
                          (ULONG_PTR)KeGetPcr()->NtTib.ExceptionList,
                          TrapFrame->EFlags,
                          (ULONG_PTR)KeGetCurrentThread());
#endif
}

//
// Generic Trap Entry
//
FORCEINLINE
VOID
KiEnterTrap(IN PKTRAP_FRAME TrapFrame)
{
    PVOID ExceptionList;

    /* Check exception list */
    ExceptionList = KeGetPcr()->NtTib.ExceptionList;
#if DBG && defined(_M_IX86) && !defined(_NTHAL_)
    KiI386BootTraceRecord(0xE100,
                          (ULONG_PTR)TrapFrame,
                          TrapFrame->Eip,
                          (ULONG_PTR)TrapFrame->ExceptionList,
                          (ULONG_PTR)ExceptionList,
                          TrapFrame->EFlags,
                          (ULONG_PTR)KeGetCurrentThread());
#endif
    ASSERTMSG("Trap handler must not register an SEH frame\n",
              ExceptionList == TrapFrame->ExceptionList);

    /* Default to debugging disabled */
    TrapFrame->Dr7 = 0;

    /* Check if the frame was from user mode or v86 mode */
    if (KiUserTrap(TrapFrame) ||
        (TrapFrame->EFlags & EFLAGS_V86_MASK))
    {
        /* Check for active debugging */
        if (KeGetCurrentThread()->Header.DebugActive & 0xFF)
        {
            /* Handle debug registers */
            KiHandleDebugRegistersOnTrapEntry(TrapFrame);
        }
    }

    /* Set debug header */
    KiFillTrapFrameDebug(TrapFrame);

#if DBG && defined(_M_IX86) && !defined(_NTHAL_)
    KiI386BootTraceRecord(0xE101,
                          (ULONG_PTR)TrapFrame,
                          TrapFrame->Eip,
                          (ULONG_PTR)TrapFrame->ExceptionList,
                          (ULONG_PTR)KeGetPcr()->NtTib.ExceptionList,
                          TrapFrame->EFlags,
                          (ULONG_PTR)KeGetCurrentThread());
#endif
}
