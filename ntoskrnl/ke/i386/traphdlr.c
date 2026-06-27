/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ke/i386/traphdlr.c
 * PURPOSE:         Kernel Trap Handlers
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

VOID __cdecl KiFastCallEntry(VOID);
VOID __cdecl KiFastCallEntryWithSingleStep(VOID);

extern PVOID KeUserPopEntrySListFault;
extern PVOID KeUserPopEntrySListResume;
extern PVOID FrRestore;
VOID FASTCALL Ke386LoadFpuState(IN PFX_SAVE_AREA SaveArea);

/* GLOBALS ********************************************************************/

UCHAR KiTrapPrefixTable[] =
{
    0xF2,                      /* REP                                  */
    0xF3,                      /* REP INS/OUTS                         */
    0x67,                      /* ADDR                                 */
    0xF0,                      /* LOCK                                 */
    0x66,                      /* OP                                   */
    0x2E,                      /* SEG                                  */
    0x3E,                      /* DS                                   */
    0x26,                      /* ES                                   */
    0x64,                      /* FS                                   */
    0x65,                      /* GS                                   */
    0x36,                      /* SS                                   */
};

UCHAR KiTrapIoTable[] =
{
    0xE4,                      /* IN                                   */
    0xE5,                      /* IN                                   */
    0xEC,                      /* IN                                   */
    0xED,                      /* IN                                   */
    0x6C,                      /* INS                                  */
    0x6D,                      /* INS                                  */
    0xE6,                      /* OUT                                  */
    0xE7,                      /* OUT                                  */
    0xEE,                      /* OUT                                  */
    0xEF,                      /* OUT                                  */
    0x6E,                      /* OUTS                                 */
    0x6F,                      /* OUTS                                 */
};

PFAST_SYSTEM_CALL_EXIT KiFastCallExitHandler;
#if DBG && defined(_M_IX86) && !defined(_WINKD_)
PKDBG_PRESERVICEHOOK KeWin32PreServiceHook = NULL;
PKDBG_POSTSERVICEHOOK KeWin32PostServiceHook = NULL;
#endif
#if DBG
BOOLEAN StopChecking = FALSE;
#endif
volatile KI_PAGE_FAULT_PROBE_SNAPSHOT KiPageFaultProbeSnapshot;
volatile KI_PAGE_FAULT_PROBE_SNAPSHOT KiPageFaultProbeSnapshotByCpu
    [KI_PAGE_FAULT_PROBE_MAXIMUM_PROCESSORS];
volatile KI_TRAP_EXIT_PROBE_SNAPSHOT KiTrapExitProbeSnapshot;
volatile KI_TRAP_EXIT_PROBE_SNAPSHOT KiTrapExitProbeSnapshotByCpu
    [KI_TRAP_EXIT_PROBE_MAXIMUM_PROCESSORS];
volatile ULONG KiTrapExitFinalSnapshotCount;
volatile KI_TRAP_EXIT_FINAL_SNAPSHOT KiTrapExitFinalSnapshotByCpu
    [KI_TRAP_EXIT_FINAL_SNAPSHOT_MAXIMUM_PROCESSORS];

#if DBG && defined(_M_IX86)
#define KI_RAW_COM1_BASE 0x3F8
#define KI_RAW_COM1_LINE_STATUS 5
#define KI_RAW_COM1_TRANSMIT_EMPTY 0x20

static
VOID
NTAPI
KiRawCom1WriteByte(
    _In_ UCHAR Character)
{
    ULONG SpinCount = 100000;

    while (SpinCount-- != 0)
    {
        if (READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)(KI_RAW_COM1_BASE +
                                                KI_RAW_COM1_LINE_STATUS)) &
            KI_RAW_COM1_TRANSMIT_EMPTY)
            break;
    }

    WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)KI_RAW_COM1_BASE, Character);
}

static
VOID
NTAPI
KiRawCom1WriteString(
    _In_z_ const CHAR *String)
{
    while (*String != ANSI_NULL)
    {
        if (*String == '\n')
            KiRawCom1WriteByte('\r');

        KiRawCom1WriteByte(*String++);
    }
}

static
ULONG
NTAPI
KiPackProcessImageNamePart(
    _In_opt_ PEPROCESS Process,
    _In_ ULONG Offset)
{
    ULONG Index;
    ULONG Part = 0;
    PUCHAR PartBytes = (PUCHAR)&Part;
    PUCHAR ImageName;

    if ((Process == NULL) || (Offset >= sizeof(Process->ImageFileName)))
        return 0;

    ImageName = (PUCHAR)Process->ImageFileName;
    for (Index = 0; Index < sizeof(Part); Index++)
    {
        if ((Offset + Index) >= sizeof(Process->ImageFileName))
            break;

        PartBytes[Index] = ImageName[Offset + Index];
    }

    return Part;
}

static
VOID
NTAPI
KiRawCom1WriteHex(
    _In_ ULONG_PTR Value)
{
    ULONG Index;

    for (Index = 0; Index < 8; Index++)
    {
        ULONG Nibble = (Value >> (28 - Index * 4)) & 0xF;

        KiRawCom1WriteByte((UCHAR)(Nibble < 10 ? ('0' + Nibble) :
                                               ('A' + Nibble - 10)));
    }
}

static
VOID
NTAPI
KiRawCom1WriteField(
    _In_z_ const CHAR *Name,
    _In_ ULONG_PTR Value)
{
    KiRawCom1WriteByte(' ');
    KiRawCom1WriteString(Name);
    KiRawCom1WriteByte('=');
    KiRawCom1WriteHex(Value);
}

static
VOID
NTAPI
KiRawCom1DumpFastFail(
    _In_ PKTRAP_FRAME TrapFrame)
{
    PKPCR Pcr = KeGetPcr();
    PKPRCB Prcb = KeGetCurrentPrcb();
    PKTHREAD Thread = KeGetCurrentThread();

    KiRawCom1WriteString("\nKiRawFastFail");
    KiRawCom1WriteField("cpu", KeGetCurrentProcessorNumber());
    KiRawCom1WriteField("irql", KeGetCurrentIrql());
    KiRawCom1WriteField("pcrirql", Pcr->Irql);
    KiRawCom1WriteField("code", TrapFrame->Ecx);
    KiRawCom1WriteField("tf", (ULONG_PTR)TrapFrame);
    KiRawCom1WriteField("eip", TrapFrame->Eip);
    KiRawCom1WriteField("cs", TrapFrame->SegCs);
    KiRawCom1WriteField("eflags", TrapFrame->EFlags);
    KiRawCom1WriteField("esp", TrapFrame->TempEsp != 0 ?
                               TrapFrame->TempEsp :
                               TrapFrame->HardwareEsp);
    KiRawCom1WriteField("tempesp", TrapFrame->TempEsp);
    KiRawCom1WriteField("hwdsp", TrapFrame->HardwareEsp);
    KiRawCom1WriteField("ebp", TrapFrame->Ebp);
    KiRawCom1WriteField("eax", TrapFrame->Eax);
    KiRawCom1WriteField("ebx", TrapFrame->Ebx);
    KiRawCom1WriteField("ecx", TrapFrame->Ecx);
    KiRawCom1WriteField("edx", TrapFrame->Edx);
    KiRawCom1WriteField("esi", TrapFrame->Esi);
    KiRawCom1WriteField("edi", TrapFrame->Edi);
    KiRawCom1WriteField("thread", (ULONG_PTR)Thread);
    KiRawCom1WriteField("state", Thread != NULL ? Thread->State : MAXULONG);
    KiRawCom1WriteField("next", Thread != NULL ? Thread->NextProcessor : MAXULONG);
    KiRawCom1WriteField("defer", Thread != NULL ? Thread->DeferredProcessor : MAXULONG);
    KiRawCom1WriteField("prio", Thread != NULL ? Thread->Priority : MAXULONG);
    KiRawCom1WriteField("aff", Thread != NULL ? Thread->Affinity : 0);
    KiRawCom1WriteField("wflink", Thread != NULL ? (ULONG_PTR)Thread->WaitListEntry.Flink : 0);
    KiRawCom1WriteField("wblink", Thread != NULL ? (ULONG_PTR)Thread->WaitListEntry.Blink : 0);
    KiRawCom1WriteField("preempt", Thread != NULL ? Thread->Preempted : 0);
    KiRawCom1WriteField("prq", Thread != NULL ? Thread->ProcessReadyQueue : 0);
    KiRawCom1WriteField("waitirql", Thread != NULL ? Thread->WaitIrql : MAXULONG);
    KiRawCom1WriteField("prcb", (ULONG_PTR)Prcb);
    KiRawCom1WriteField("pnum", Prcb != NULL ? Prcb->Number : MAXULONG);
    KiRawCom1WriteField("set", Prcb != NULL ? Prcb->SetMember : 0);
    KiRawCom1WriteField("ready", Prcb != NULL ? Prcb->ReadySummary : 0);
    KiRawCom1WriteField("cur", Prcb != NULL ? (ULONG_PTR)Prcb->CurrentThread : 0);
    KiRawCom1WriteField("nxt", Prcb != NULL ? (ULONG_PTR)Prcb->NextThread : 0);
    KiRawCom1WriteField("idle", Prcb != NULL ? (ULONG_PTR)Prcb->IdleThread : 0);
    KiRawCom1WriteByte('\n');
}

static
VOID
NTAPI
KiRawCom1DumpDebugService(
    _In_ ULONG Stage,
    _In_ PKTRAP_FRAME TrapFrame,
    _In_ ULONG_PTR Parameter1,
    _In_ ULONG_PTR Parameter2,
    _In_ ULONG_PTR Parameter3)
{
    KiRawCom1WriteString("\nKiDbgSvc");
    KiRawCom1WriteField("stage", Stage);
    KiRawCom1WriteField("cpu", KeGetCurrentProcessorNumber());
    KiRawCom1WriteField("irql", KeGetCurrentIrql());
    KiRawCom1WriteField("tf", (ULONG_PTR)TrapFrame);
    KiRawCom1WriteField("eip", TrapFrame->Eip);
    KiRawCom1WriteField("eflags", TrapFrame->EFlags);
    KiRawCom1WriteField("eax", TrapFrame->Eax);
    KiRawCom1WriteField("ecx", TrapFrame->Ecx);
    KiRawCom1WriteField("edx", TrapFrame->Edx);
    KiRawCom1WriteField("p1", Parameter1);
    KiRawCom1WriteField("p2", Parameter2);
    KiRawCom1WriteField("p3", Parameter3);
    KiRawCom1WriteByte('\n');
}

static
VOID
NTAPI
KiRawCom1DumpTrap0E(
    _In_ PKTRAP_FRAME TrapFrame,
    _In_ ULONG_PTR Cr2,
    _In_ ULONG_PTR PreviousTrapFrame)
{
    PKPCR Pcr = KeGetPcr();
    PKTHREAD Thread = KeGetCurrentThread();
    ULONG_PTR KernelStack = 0;
    ULONG_PTR InitialStack = 0;

    if (Thread != NULL)
    {
        KernelStack = (ULONG_PTR)Thread->KernelStack;
        InitialStack = (ULONG_PTR)Thread->InitialStack;
    }

    KiRawCom1WriteString("\nKiRawTrap0E");
    KiRawCom1WriteField("cpu", KeGetCurrentProcessorNumber());
    KiRawCom1WriteField("irql", KeGetCurrentIrql());
    KiRawCom1WriteField("pcrirql", Pcr->Irql);
    KiRawCom1WriteField("irr", Pcr->IRR);
    KiRawCom1WriteField("idr", Pcr->IDR);
    KiRawCom1WriteField("cr2", Cr2);
    KiRawCom1WriteField("tf", (ULONG_PTR)TrapFrame);
    KiRawCom1WriteField("prevtf", PreviousTrapFrame);
    KiRawCom1WriteField("thread", (ULONG_PTR)Thread);
    KiRawCom1WriteField("eip", TrapFrame->Eip);
    KiRawCom1WriteField("cs", TrapFrame->SegCs);
    KiRawCom1WriteField("eflags", TrapFrame->EFlags);
    KiRawCom1WriteField("live", __readeflags());
    KiRawCom1WriteField("err", TrapFrame->ErrCode);
    KiRawCom1WriteField("esp", TrapFrame->TempEsp != 0 ?
                               TrapFrame->TempEsp :
                               TrapFrame->HardwareEsp);
    KiRawCom1WriteField("tempesp", TrapFrame->TempEsp);
    KiRawCom1WriteField("hwdsp", TrapFrame->HardwareEsp);
    KiRawCom1WriteField("ebp", TrapFrame->Ebp);
    KiRawCom1WriteField("eax", TrapFrame->Eax);
    KiRawCom1WriteField("ebx", TrapFrame->Ebx);
    KiRawCom1WriteField("ecx", TrapFrame->Ecx);
    KiRawCom1WriteField("edx", TrapFrame->Edx);
    KiRawCom1WriteField("esi", TrapFrame->Esi);
    KiRawCom1WriteField("edi", TrapFrame->Edi);
    KiRawCom1WriteField("kstack", KernelStack);
    KiRawCom1WriteField("istack", InitialStack);
    KiRawCom1WriteField("pcr", (ULONG_PTR)Pcr->SelfPcr);
    KiRawCom1WriteField("prcb", (ULONG_PTR)Pcr->Prcb);
    KiRawCom1WriteByte('\n');
}
#endif


/* TRAP EXIT CODE *************************************************************/

static
VOID
NTAPI
KiStorePageFaultProbe(
    _Out_ volatile KI_PAGE_FAULT_PROBE_SNAPSHOT *Snapshot,
    _In_ PKTRAP_FRAME TrapFrame,
    _In_ ULONG_PTR Cr2,
    _In_ ULONG Stage,
    _In_ ULONG_PTR PreviousTrapFrame,
    _In_ ULONG Count)
{
    PKPCR Pcr = KeGetPcr();
    PKTHREAD Thread = KeGetCurrentThread();
    ULONG_PTR KernelStack = 0;
    ULONG_PTR InitialStack = 0;

    if (Thread != NULL)
    {
        KernelStack = (ULONG_PTR)Thread->KernelStack;
        InitialStack = (ULONG_PTR)Thread->InitialStack;
    }

    Snapshot->Magic = KI_PAGE_FAULT_PROBE_MAGIC;
    Snapshot->Version = 1;
    Snapshot->Count = Count;
    Snapshot->Stage = Stage;
    Snapshot->Cpu = KeGetCurrentProcessorNumber();
    Snapshot->Irql = KeGetCurrentIrql();
    Snapshot->PcrIrql = Pcr->Irql;
    Snapshot->PcrIrr = Pcr->IRR;
    Snapshot->PcrIdr = Pcr->IDR;
    Snapshot->EFlags = TrapFrame->EFlags;
    Snapshot->CurrentEFlags = __readeflags();
    Snapshot->SegCs = TrapFrame->SegCs;
    Snapshot->SegFs = TrapFrame->SegFs;
    Snapshot->ErrCode = TrapFrame->ErrCode;
    Snapshot->Cr2 = Cr2;
    Snapshot->Eip = TrapFrame->Eip;
    Snapshot->HardwareEsp = TrapFrame->HardwareEsp;
    Snapshot->Ebp = TrapFrame->Ebp;
    Snapshot->Eax = TrapFrame->Eax;
    Snapshot->Edx = TrapFrame->Edx;
    Snapshot->TrapFrame = (ULONG_PTR)TrapFrame;
    Snapshot->PreviousTrapFrame = PreviousTrapFrame;
    Snapshot->Thread = (ULONG_PTR)Thread;
    Snapshot->KernelStack = KernelStack;
    Snapshot->InitialStack = InitialStack;
    Snapshot->PcrSelf = (ULONG_PTR)Pcr->SelfPcr;
    Snapshot->PcrPrcb = (ULONG_PTR)Pcr->Prcb;
    Snapshot->ExceptionList = (ULONG_PTR)TrapFrame->ExceptionList;
    Snapshot->PcrExceptionList = (ULONG_PTR)Pcr->NtTib.ExceptionList;
}

static
VOID
NTAPI
KiRecordPageFaultProbe(
    _In_ PKTRAP_FRAME TrapFrame,
    _In_ ULONG_PTR Cr2,
    _In_ ULONG Stage,
    _In_ ULONG_PTR PreviousTrapFrame)
{
    ULONG Cpu = KeGetCurrentProcessorNumber();
    ULONG Count = KiPageFaultProbeSnapshot.Count + 1;

    KiStorePageFaultProbe(&KiPageFaultProbeSnapshot,
                          TrapFrame,
                          Cr2,
                          Stage,
                          PreviousTrapFrame,
                          Count);
    if (Cpu < KI_PAGE_FAULT_PROBE_MAXIMUM_PROCESSORS)
    {
        KiStorePageFaultProbe(&KiPageFaultProbeSnapshotByCpu[Cpu],
                              TrapFrame,
                              Cr2,
                              Stage,
                              PreviousTrapFrame,
                              Count);
    }
}

#if DBG && defined(_M_IX86)
static
VOID
NTAPI
KiTracePageFaultProbe(
    _In_ PKTRAP_FRAME TrapFrame,
    _In_ ULONG_PTR Cr2,
    _In_ ULONG Stage,
    _In_ ULONG Event,
    _In_ ULONG_PTR Detail,
    _In_ ULONG_PTR PreviousTrapFrame)
{
    KiRecordPageFaultProbe(TrapFrame, Cr2, Stage, PreviousTrapFrame);
    KiI386BootTraceRecord(Event,
                          (ULONG_PTR)TrapFrame,
                          Cr2,
                          TrapFrame->Eip,
                          TrapFrame->ErrCode,
                          Detail,
                          PreviousTrapFrame);
}

static
VOID
NTAPI
KiTraceKernelTrapReturnProvenance(
    _In_ PKTRAP_FRAME TrapFrame)
{
    PKPCR Pcr;
    PKPRCB Prcb;
    PKTHREAD Thread;
    PKTRAP_FRAME FixedTrapFrame;
    ULONG_PTR KernelStack;
    ULONG_PTR InitialStack;
    ULONG_PTR StackLimit;
    ULONG_PTR TrapStack;

    if ((KeGetCurrentIrql() < DISPATCH_LEVEL) ||
        KiUserTrap(TrapFrame) ||
        (TrapFrame->EFlags & EFLAGS_V86_MASK))
    {
        return;
    }

    Pcr = KeGetPcr();
    Prcb = KeGetCurrentPrcb();
    Thread = KeGetCurrentThread();
    FixedTrapFrame = NULL;
    KernelStack = 0;
    InitialStack = 0;
    StackLimit = 0;
    TrapStack = KeGetTrapFrameStackRegister(TrapFrame);

    if (Thread != NULL)
    {
        FixedTrapFrame = KeGetTrapFrame(Thread);
        KernelStack = (ULONG_PTR)Thread->KernelStack;
        InitialStack = (ULONG_PTR)Thread->InitialStack;
        StackLimit = (ULONG_PTR)Thread->StackLimit;
    }

    KiI386BootTraceRecord(0xE170,
                          (ULONG_PTR)TrapFrame,
                          TrapFrame->Eip,
                          TrapFrame->SegCs,
                          TrapFrame->EFlags,
                          TrapFrame->HardwareEsp,
                          TrapStack);
    KiI386BootTraceRecord(0xE171,
                          (ULONG_PTR)Thread,
                          Thread != NULL ? (ULONG_PTR)Thread->TrapFrame : 0,
                          (ULONG_PTR)FixedTrapFrame,
                          KernelStack,
                          InitialStack,
                          StackLimit);
    KiI386BootTraceRecord(0xE172,
                          (ULONG_PTR)Prcb,
                          (ULONG_PTR)Prcb->CurrentThread,
                          (ULONG_PTR)Prcb->NextThread,
                          Prcb->SetMember,
                          Prcb->ReadySummary,
                          (ULONG_PTR)Pcr->NtTib.ExceptionList);
}
#endif

FORCEINLINE
BOOLEAN
KiVdmTrap(IN PKTRAP_FRAME TrapFrame)
{
    /* Either the V8086 flag is on, or this is user-mode with a VDM */
    return ((TrapFrame->EFlags & EFLAGS_V86_MASK) ||
            ((KiUserTrap(TrapFrame)) && (PsGetCurrentProcess()->VdmObjects)));
}

FORCEINLINE
BOOLEAN
KiV86Trap(IN PKTRAP_FRAME TrapFrame)
{
    /* Check if the V8086 flag is on */
    return ((TrapFrame->EFlags & EFLAGS_V86_MASK) != 0);
}

FORCEINLINE
BOOLEAN
KiIsFrameEdited(IN PKTRAP_FRAME TrapFrame)
{
    /* An edited frame changes esp. It is marked by clearing the bits
       defined by FRAME_EDITED in the SegCs field of the trap frame */
    return ((TrapFrame->SegCs & FRAME_EDITED) == 0);
}

FORCEINLINE
VOID
KiCommonExit(IN PKTRAP_FRAME TrapFrame, BOOLEAN SkipPreviousMode)
{
    /* Disable interrupts until we return */
    _disable();

#if DBG && defined(_M_IX86)
    KiI386BootTraceRecord(0xE120,
                          (ULONG_PTR)TrapFrame,
                          TrapFrame->Eip,
                          (ULONG_PTR)TrapFrame->ExceptionList,
                          (ULONG_PTR)KeGetPcr()->NtTib.ExceptionList,
                          SkipPreviousMode,
                          KeGetTrapFrameStackRegister(TrapFrame));
#endif

    /* Check for APC delivery */
    KiCheckForApcDelivery(TrapFrame);

    /* Restore the SEH handler chain */
    KeGetPcr()->NtTib.ExceptionList = TrapFrame->ExceptionList;

#if DBG && defined(_M_IX86)
    KiI386BootTraceRecord(0xE121,
                          (ULONG_PTR)TrapFrame,
                          TrapFrame->Eip,
                          (ULONG_PTR)TrapFrame->ExceptionList,
                          (ULONG_PTR)KeGetPcr()->NtTib.ExceptionList,
                          SkipPreviousMode,
                          KeGetTrapFrameStackRegister(TrapFrame));
#endif

    /* Check if there are active debug registers */
    if (__builtin_expect(TrapFrame->Dr7 & ~DR7_RESERVED_MASK, 0))
    {
        /* Check if the frame was from user mode or v86 mode */
        if (KiUserTrap(TrapFrame) ||
            (TrapFrame->EFlags & EFLAGS_V86_MASK))
        {
            /* Handle debug registers */
            KiHandleDebugRegistersOnTrapExit(TrapFrame);
        }
    }

    /* Debugging checks */
    KiExitTrapDebugChecks(TrapFrame, SkipPreviousMode);
}

DECLSPEC_NORETURN
VOID
FASTCALL
KiEoiHelper(IN PKTRAP_FRAME TrapFrame)
{
    /* Common trap exit code */
    KiCommonExit(TrapFrame, TRUE);

#if DBG && defined(_M_IX86)
    KiI386BootTraceRecord(0xE123,
                          (ULONG_PTR)TrapFrame,
                          TrapFrame->Eip,
                          TrapFrame->SegCs,
                          TrapFrame->EFlags,
                          (ULONG_PTR)TrapFrame->ExceptionList,
                          KeGetTrapFrameStackRegister(TrapFrame));
#endif

    /* Check if this was a V8086 trap */
    if (TrapFrame->EFlags & EFLAGS_V86_MASK)
    {
#if DBG && defined(_M_IX86)
        KiI386BootTraceRecord(0xE124,
                              (ULONG_PTR)TrapFrame,
                              TrapFrame->Eip,
                              TrapFrame->SegCs,
                              TrapFrame->EFlags,
                              (ULONG_PTR)TrapFrame->ExceptionList,
                              KeGetTrapFrameStackRegister(TrapFrame));
#endif
        KiTrapReturnNoSegments(TrapFrame);
    }

    /* Check for user mode exit */
    if (KiUserTrap(TrapFrame))
    {
#if DBG && defined(_M_IX86)
        KiI386BootTraceRecord(0xE125,
                              (ULONG_PTR)TrapFrame,
                              TrapFrame->Eip,
                              TrapFrame->SegCs,
                              TrapFrame->EFlags,
                              (ULONG_PTR)TrapFrame->ExceptionList,
                              KeGetTrapFrameStackRegister(TrapFrame));
#endif
        KiTrapReturn(TrapFrame);
    }

    /* Check for edited frame */
    if (KiIsFrameEdited(TrapFrame))
    {
#if DBG && defined(_M_IX86)
        KiI386BootTraceRecord(0xE126,
                              (ULONG_PTR)TrapFrame,
                              TrapFrame->Eip,
                              TrapFrame->SegCs,
                              TrapFrame->EFlags,
                              (ULONG_PTR)TrapFrame->ExceptionList,
                              KeGetTrapFrameStackRegister(TrapFrame));
#endif
        KiEditedTrapReturn(TrapFrame);
    }

    /* Check if we have single stepping enabled */
    if (TrapFrame->EFlags & EFLAGS_TF)
    {
#if DBG && defined(_M_IX86)
        KiI386BootTraceRecord(0xE127,
                              (ULONG_PTR)TrapFrame,
                              TrapFrame->Eip,
                              TrapFrame->SegCs,
                              TrapFrame->EFlags,
                              (ULONG_PTR)TrapFrame->ExceptionList,
                              KeGetTrapFrameStackRegister(TrapFrame));
#endif
        KiTrapReturnNoSegments(TrapFrame);
    }

#if DBG && defined(_M_IX86)
    KiI386BootTraceRecord(0xE122,
                          (ULONG_PTR)TrapFrame,
                          TrapFrame->Eip,
                          (ULONG_PTR)TrapFrame->ExceptionList,
                          (ULONG_PTR)KeGetPcr()->NtTib.ExceptionList,
                          TrapFrame->EFlags,
                          KeGetTrapFrameStackRegister(TrapFrame));
    KiTraceKernelTrapReturnProvenance(TrapFrame);
    KiI386BootTraceRecord(0xE128,
                          (ULONG_PTR)TrapFrame,
                          TrapFrame->Eip,
                          TrapFrame->SegCs,
                          TrapFrame->EFlags,
                          (ULONG_PTR)TrapFrame->ExceptionList,
                          KeGetTrapFrameStackRegister(TrapFrame));
#endif

    /* Exit the trap to kernel mode */
    KiTrapReturnNoSegmentsRet8(TrapFrame);
}

DECLSPEC_NORETURN
VOID
FASTCALL
KiServiceExit(IN PKTRAP_FRAME TrapFrame,
              IN NTSTATUS Status)
{
    ASSERT((TrapFrame->EFlags & EFLAGS_V86_MASK) == 0);
    ASSERT(!KiIsFrameEdited(TrapFrame));

    /* Copy the status into EAX */
    TrapFrame->Eax = Status;

    /* Common trap exit code */
    KiCommonExit(TrapFrame, FALSE);

#if DBG && defined(_M_IX86)
    KiI386BootTraceRecord(0xE140,
                          (ULONG_PTR)TrapFrame,
                          TrapFrame->Eip,
                          Status,
                          TrapFrame->PreviousPreviousMode,
                          TrapFrame->EFlags,
                          KeGetTrapFrameStackRegister(TrapFrame));
#endif

    /* Restore previous mode */
    KeGetCurrentThread()->PreviousMode = (CCHAR)TrapFrame->PreviousPreviousMode;

#if DBG && defined(_M_IX86)
    KiI386BootTraceRecord(0xE141,
                          (ULONG_PTR)TrapFrame,
                          TrapFrame->Eip,
                          TrapFrame->PreviousPreviousMode,
                          KeGetCurrentThread()->PreviousMode,
                          TrapFrame->EFlags,
                          KeGetTrapFrameStackRegister(TrapFrame));
#endif

    /* Check for user mode exit */
    if (KiUserTrap(TrapFrame))
    {
        /* Check if we were single stepping */
        if (TrapFrame->EFlags & EFLAGS_TF)
        {
#if DBG && defined(_M_IX86)
            KiI386BootTraceRecord(0xE142,
                                  (ULONG_PTR)TrapFrame,
                                  TrapFrame->Eip,
                                  TrapFrame->SegCs,
                                  TrapFrame->EFlags,
                                  KiFastCallExitHandler == KiSystemCallTrapReturn,
                                  KeGetTrapFrameStackRegister(TrapFrame));
#endif
            /* Must use the IRET handler */
            KiSystemCallTrapReturn(TrapFrame);
        }
        else
        {
#if DBG && defined(_M_IX86)
            KiI386BootTraceRecord(0xE143,
                                  (ULONG_PTR)TrapFrame,
                                  TrapFrame->Eip,
                                  TrapFrame->SegCs,
                                  TrapFrame->EFlags,
                                  (ULONG_PTR)KiFastCallExitHandler,
                                  KeGetTrapFrameStackRegister(TrapFrame));
#endif
            /* We can use the sysexit handler */
            KiFastCallExitHandler(TrapFrame);
            UNREACHABLE;
        }
    }

#if DBG && defined(_M_IX86)
    KiI386BootTraceRecord(0xE144,
                          (ULONG_PTR)TrapFrame,
                          TrapFrame->Eip,
                          TrapFrame->SegCs,
                          TrapFrame->EFlags,
                          (ULONG_PTR)TrapFrame->ExceptionList,
                          KeGetTrapFrameStackRegister(TrapFrame));
#endif

    /* Exit to kernel mode */
    KiSystemCallReturn(TrapFrame);
}

DECLSPEC_NORETURN
VOID
FASTCALL
KiServiceExit2(IN PKTRAP_FRAME TrapFrame)
{
    /* Common trap exit code */
    KiCommonExit(TrapFrame, FALSE);

#if DBG && defined(_M_IX86)
    KiI386BootTraceRecord(0xE150,
                          (ULONG_PTR)TrapFrame,
                          TrapFrame->Eip,
                          TrapFrame->SegCs,
                          TrapFrame->PreviousPreviousMode,
                          TrapFrame->EFlags,
                          KeGetTrapFrameStackRegister(TrapFrame));
#endif

    /* Restore previous mode */
    KeGetCurrentThread()->PreviousMode = (CCHAR)TrapFrame->PreviousPreviousMode;

#if DBG && defined(_M_IX86)
    KiI386BootTraceRecord(0xE151,
                          (ULONG_PTR)TrapFrame,
                          TrapFrame->Eip,
                          TrapFrame->PreviousPreviousMode,
                          KeGetCurrentThread()->PreviousMode,
                          TrapFrame->EFlags,
                          KeGetTrapFrameStackRegister(TrapFrame));
#endif

    /* Check if this was a V8086 trap */
    if (TrapFrame->EFlags & EFLAGS_V86_MASK)
    {
#if DBG && defined(_M_IX86)
        KiI386BootTraceRecord(0xE152,
                              (ULONG_PTR)TrapFrame,
                              TrapFrame->Eip,
                              TrapFrame->SegCs,
                              TrapFrame->EFlags,
                              (ULONG_PTR)TrapFrame->ExceptionList,
                              KeGetTrapFrameStackRegister(TrapFrame));
#endif
        KiTrapReturnNoSegments(TrapFrame);
    }

    /* Check for user mode exit */
    if (KiUserTrap(TrapFrame))
    {
#if DBG && defined(_M_IX86)
        PETHREAD CurrentThread = PsGetCurrentThread();
        PEPROCESS CurrentProcess = PsGetCurrentProcess();

        KiI386BootTraceRecord(0xE157,
                              (ULONG_PTR)TrapFrame,
                              (ULONG_PTR)CurrentThread,
                              (ULONG_PTR)CurrentProcess,
                              (ULONG_PTR)CurrentThread->Cid.UniqueThread,
                              (ULONG_PTR)CurrentThread->Cid.UniqueProcess,
                              (ULONG_PTR)CurrentThread->Tcb.ApcState.Process);
        KiI386BootTraceRecord(0xE158,
                              (ULONG_PTR)TrapFrame,
                              TrapFrame->Eip,
                              KiPackProcessImageNamePart(CurrentProcess, 0),
                              KiPackProcessImageNamePart(CurrentProcess, 4),
                              KiPackProcessImageNamePart(CurrentProcess, 8),
                              KiPackProcessImageNamePart(CurrentProcess, 12));
        KiI386BootTraceRecord(0xE153,
                              (ULONG_PTR)TrapFrame,
                              TrapFrame->Eip,
                              TrapFrame->SegCs,
                              TrapFrame->EFlags,
                              (ULONG_PTR)TrapFrame->ExceptionList,
                              KeGetTrapFrameStackRegister(TrapFrame));
        KiI386BootTraceRecord(0xE159,
                              (ULONG_PTR)TrapFrame,
                              TrapFrame->HardwareEsp,
                              TrapFrame->HardwareSegSs,
                              TrapFrame->SegDs,
                              TrapFrame->SegEs,
                              TrapFrame->SegFs);
#endif
        KiTrapReturn(TrapFrame);
    }

    /* Check for edited frame */
    if (KiIsFrameEdited(TrapFrame))
    {
#if DBG && defined(_M_IX86)
        KiI386BootTraceRecord(0xE154,
                              (ULONG_PTR)TrapFrame,
                              TrapFrame->Eip,
                              TrapFrame->SegCs,
                              TrapFrame->EFlags,
                              (ULONG_PTR)TrapFrame->ExceptionList,
                              KeGetTrapFrameStackRegister(TrapFrame));
#endif
        KiEditedTrapReturn(TrapFrame);
    }

    /* Check if we have single stepping enabled */
    if (TrapFrame->EFlags & EFLAGS_TF)
    {
#if DBG && defined(_M_IX86)
        KiI386BootTraceRecord(0xE155,
                              (ULONG_PTR)TrapFrame,
                              TrapFrame->Eip,
                              TrapFrame->SegCs,
                              TrapFrame->EFlags,
                              (ULONG_PTR)TrapFrame->ExceptionList,
                              KeGetTrapFrameStackRegister(TrapFrame));
#endif
        KiTrapReturnNoSegments(TrapFrame);
    }

#if DBG && defined(_M_IX86)
    KiI386BootTraceRecord(0xE156,
                          (ULONG_PTR)TrapFrame,
                          TrapFrame->Eip,
                          TrapFrame->SegCs,
                          TrapFrame->EFlags,
                          (ULONG_PTR)TrapFrame->ExceptionList,
                          KeGetTrapFrameStackRegister(TrapFrame));
#endif

    /* Exit the trap to kernel mode */
    KiTrapReturnNoSegmentsRet8(TrapFrame);
}


/* TRAP HANDLERS **************************************************************/

DECLSPEC_NORETURN
VOID
FASTCALL
KiDebugHandler(IN PKTRAP_FRAME TrapFrame,
               IN ULONG Parameter1,
               IN ULONG Parameter2,
               IN ULONG Parameter3)
{
    /* Check for VDM trap */
    ASSERT(KiVdmTrap(TrapFrame) == FALSE);

    /* Enable interrupts if the trap came from user-mode */
    if (KiUserTrap(TrapFrame)) _enable();

#if DBG && defined(_M_IX86)
    if (Parameter1 == BREAKPOINT_LOAD_SYMBOLS)
    {
        KiRawCom1DumpDebugService(0x2D10,
                                  TrapFrame,
                                  Parameter1,
                                  Parameter2,
                                  Parameter3);
    }
#endif

    /* Dispatch the exception. Fix EIP in case its a break breakpoint (sic) */
    KiDispatchExceptionFromTrapFrame(STATUS_BREAKPOINT,
                                     0,
                                     TrapFrame->Eip - (Parameter1 == BREAKPOINT_BREAK),
                                     3,
                                     Parameter1,
                                     Parameter2,
                                     Parameter3,
                                     TrapFrame);
}

DECLSPEC_NORETURN
VOID
FASTCALL
KiNpxHandler(IN PKTRAP_FRAME TrapFrame,
             IN PKTHREAD Thread,
             IN PFX_SAVE_AREA SaveArea)
{
    ULONG Cr0, Mask, Error, ErrorOffset, DataOffset;

    /* Check for VDM trap */
    ASSERT(KiVdmTrap(TrapFrame) == FALSE);

    /* Check for kernel trap */
    if (!KiUserTrap(TrapFrame))
    {
        /* Kernel might've tripped a delayed error */
        SaveArea->Cr0NpxState |= CR0_TS | CR0_MP;

        /* Only valid if it happened during a restore */
        if ((PVOID)TrapFrame->Eip == FrRestore)
        {
            /* It did, so just skip the instruction */
            TrapFrame->Eip += 3; /* Size of FRSTOR instruction */
            KiEoiHelper(TrapFrame);
        }
    }

    /* User or kernel trap -- check if we need to unload the current state */
    if (Thread->NpxState == NPX_STATE_LOADED)
    {
        /* Update CR0 */
        Cr0 = __readcr0();
        Cr0 &= ~(CR0_MP | CR0_EM | CR0_TS);
        __writecr0(Cr0);

        /* Save FPU state */
        Ke386SaveFpuState(SaveArea);

        /* Mark CR0 state dirty */
        Cr0 |= NPX_STATE_NOT_LOADED;
        Cr0 |= SaveArea->Cr0NpxState;
        Cr0 = KiNormalizeNpxCr0State(Cr0);
        __writecr0(Cr0);

        /* Update NPX state */
        Thread->NpxState = NPX_STATE_NOT_LOADED;
        KeGetCurrentPrcb()->NpxThread = NULL;
    }

    /* Clear the TS bit and re-enable interrupts */
    SaveArea->Cr0NpxState &= ~CR0_TS;
    _enable();

    /* Check if we should get the FN or FX error */
    if (KeI386FxsrPresent)
    {
        /* Get it from FX */
        Mask = SaveArea->U.FxArea.ControlWord;
        Error = SaveArea->U.FxArea.StatusWord;

        /* Get the FPU exception address too */
        ErrorOffset = SaveArea->U.FxArea.ErrorOffset;
        DataOffset = SaveArea->U.FxArea.DataOffset;
    }
    else
    {
        /* Get it from FN */
        Mask = SaveArea->U.FnArea.ControlWord;
        Error = SaveArea->U.FnArea.StatusWord;

        /* Get the FPU exception address too */
        ErrorOffset = SaveArea->U.FnArea.ErrorOffset;
        DataOffset = SaveArea->U.FnArea.DataOffset;
    }

    /* Get legal exceptions that software should handle */
    Mask &= (FSW_INVALID_OPERATION |
             FSW_DENORMAL |
             FSW_ZERO_DIVIDE |
             FSW_OVERFLOW |
             FSW_UNDERFLOW |
             FSW_PRECISION);
    Error &= ~Mask;

    /* Check for invalid operation */
    if (Error & FSW_INVALID_OPERATION)
    {
        /*
         * Now check if this is actually a Stack Fault. This is needed because
         * on x86 the Invalid Operation error is set for Stack Check faults as well.
         */
        if (Error & FSW_STACK_FAULT)
        {
            /* Issue stack check fault */
            KiDispatchException2Args(STATUS_FLOAT_STACK_CHECK,
                                     ErrorOffset,
                                     0,
                                     DataOffset,
                                     TrapFrame);
        }
        else
        {
            /* This is an invalid operation fault after all, so raise that instead */
            KiDispatchException1Args(STATUS_FLOAT_INVALID_OPERATION,
                                     ErrorOffset,
                                     0,
                                     TrapFrame);
        }
    }

    /* Check for divide by zero */
    if (Error & FSW_ZERO_DIVIDE)
    {
        /* Issue fault */
        KiDispatchException1Args(STATUS_FLOAT_DIVIDE_BY_ZERO,
                                 ErrorOffset,
                                 0,
                                 TrapFrame);
    }

    /* Check for denormal */
    if (Error & FSW_DENORMAL)
    {
        /* Issue fault */
        KiDispatchException1Args(STATUS_FLOAT_INVALID_OPERATION,
                                 ErrorOffset,
                                 0,
                                 TrapFrame);
    }

    /* Check for overflow */
    if (Error & FSW_OVERFLOW)
    {
        /* Issue fault */
        KiDispatchException1Args(STATUS_FLOAT_OVERFLOW,
                                 ErrorOffset,
                                 0,
                                 TrapFrame);
    }

    /* Check for underflow */
    if (Error & FSW_UNDERFLOW)
    {
        /* Issue fault */
        KiDispatchException1Args(STATUS_FLOAT_UNDERFLOW,
                                 ErrorOffset,
                                 0,
                                 TrapFrame);
    }

    /* Check for precision fault */
    if (Error & FSW_PRECISION)
    {
        /* Issue fault */
        KiDispatchException1Args(STATUS_FLOAT_INEXACT_RESULT,
                                 ErrorOffset,
                                 0,
                                 TrapFrame);
    }

    /* Unknown FPU fault */
    KeBugCheckWithTf(TRAP_CAUSE_UNKNOWN, 1, Error, 0, 0, TrapFrame);
}

DECLSPEC_NORETURN
VOID
FASTCALL
KiTrap00Handler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* Check for VDM trap */
    ASSERT(KiVdmTrap(TrapFrame) == FALSE);

    /*  Enable interrupts */
    _enable();

    /* Dispatch the exception */
    KiDispatchException0Args(STATUS_INTEGER_DIVIDE_BY_ZERO,
                             TrapFrame->Eip,
                             TrapFrame);
}

DECLSPEC_NORETURN
VOID
FASTCALL
KiTrap01Handler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* Check for VDM trap */
    ASSERT(KiVdmTrap(TrapFrame) == FALSE);

    /* Check if this was a single step after sysenter */
    if (TrapFrame->Eip == (ULONG)KiFastCallEntry)
    {
        /* Disable single stepping */
        TrapFrame->EFlags &= ~EFLAGS_TF;

        /* Re-enter at the alternative sysenter entry point */
        TrapFrame->Eip = (ULONG)KiFastCallEntryWithSingleStep;

        /* End this trap */
        KiEoiHelper(TrapFrame);
    }

    /* Enable interrupts if the trap came from user-mode */
    if (KiUserTrap(TrapFrame)) _enable();

    /*  Mask out trap flag and dispatch the exception */
    TrapFrame->EFlags &= ~EFLAGS_TF;
    KiDispatchException0Args(STATUS_SINGLE_STEP,
                             TrapFrame->Eip,
                             TrapFrame);
}

VOID
__cdecl
KiTrap02Handler(VOID)
{
    PKTSS Tss, NmiTss;
    PKTHREAD Thread;
    PKPROCESS Process;
    PKGDTENTRY TssGdt;
    KTRAP_FRAME TrapFrame;
    KIRQL OldIrql;

    /*
     * In some sort of strange recursion case, we might end up here with the IF
     * flag incorrectly on the interrupt frame -- during a normal NMI this would
     * normally already be set.
     *
     * For sanity's sake, make sure interrupts are disabled for sure.
     * NMIs will already be since the CPU does it for us.
     */
    _disable();

    /* Get the current TSS, thread, and process */
    Tss = KeGetPcr()->TSS;
    Thread = ((PKIPCR)KeGetPcr())->PrcbData.CurrentThread;
    Process = Thread->ApcState.Process;

    /* Save data usually not present in the TSS */
    Tss->CR3 = Process->DirectoryTableBase[0];
    Tss->IoMapBase = Process->IopmOffset;
    Tss->LDT = Process->LdtDescriptor.LimitLow ? KGDT_LDT : 0;

    /* Now get the base address of the NMI TSS */
    TssGdt = &((PKIPCR)KeGetPcr())->GDT[KGDT_NMI_TSS / sizeof(KGDTENTRY)];
    NmiTss = (PKTSS)(ULONG_PTR)(TssGdt->BaseLow |
                                TssGdt->HighWord.Bytes.BaseMid << 16 |
                                TssGdt->HighWord.Bytes.BaseHi << 24);

    /*
     * Switch to it and activate it, masking off the nested flag.
     *
     * Note that in reality, we are already on the NMI TSS -- we just
     * need to update the PCR to reflect this.
     */
    KeGetPcr()->TSS = NmiTss;
    __writeeflags(__readeflags() &~ EFLAGS_NESTED_TASK);
    TssGdt->HighWord.Bits.Dpl = 0;
    TssGdt->HighWord.Bits.Pres = 1;
    TssGdt->HighWord.Bits.Type = I386_TSS;

    /*
     * Now build the trap frame based on the original TSS.
     *
     * The CPU does a hardware "Context switch" / task switch of sorts
     * and so it takes care of saving our context in the normal TSS.
     *
     * We just have to go get the values...
     */
    RtlZeroMemory(&TrapFrame, sizeof(KTRAP_FRAME));
    TrapFrame.HardwareSegSs = Tss->Ss0;
    TrapFrame.HardwareEsp = Tss->Esp0;
    TrapFrame.EFlags = Tss->EFlags;
    TrapFrame.SegCs = Tss->Cs;
    TrapFrame.Eip = Tss->Eip;
    TrapFrame.Ebp = Tss->Ebp;
    TrapFrame.Ebx = Tss->Ebx;
    TrapFrame.Esi = Tss->Esi;
    TrapFrame.Edi = Tss->Edi;
    TrapFrame.SegFs = Tss->Fs;
    TrapFrame.ExceptionList = KeGetPcr()->NtTib.ExceptionList;
    TrapFrame.PreviousPreviousMode = (ULONG)-1;
    TrapFrame.Eax = Tss->Eax;
    TrapFrame.Ecx = Tss->Ecx;
    TrapFrame.Edx = Tss->Edx;
    TrapFrame.SegDs = Tss->Ds;
    TrapFrame.SegEs = Tss->Es;
    TrapFrame.SegGs = Tss->Gs;
    TrapFrame.DbgEip = Tss->Eip;
    TrapFrame.DbgEbp = Tss->Ebp;

    /* Store the trap frame in the KPRCB */
    KiSaveProcessorState(&TrapFrame, NULL);

    /*
     * Debugger freezes use APIC NMI delivery so CPUs at HIGH_LEVEL cannot mask
     * the stop request behind an ordinary IPI vector.
     */
    if (!KxHandleFreezeNmi() && !KiHandleNmi())
    {
        /*
         * They did not, so call the platform HAL routine to bugcheck the system
         *
         * Make sure the HAL believes it's running at HIGH IRQL... we can't use
         * the normal APIs here as playing with the IRQL could change the system
         * state.
         */
        OldIrql = KeGetPcr()->Irql;
        KeGetPcr()->Irql = HIGH_LEVEL;
        HalHandleNMI(NULL);
        KeGetPcr()->Irql = OldIrql;
    }

    /*
     * Although the CPU disabled NMIs, we just did a BIOS call, which could've
     * totally changed things.
     *
     * We have to make sure we're still in our original NMI -- a nested NMI
     * will point back to the NMI TSS, and in that case we're hosed.
     */
    if (KeGetPcr()->TSS->Backlink == KGDT_NMI_TSS)
    {
        /* Unhandled: crash the system */
        KiSystemFatalException(EXCEPTION_NMI, NULL);
    }

    /* Restore original TSS */
    KeGetPcr()->TSS = Tss;

    /* Set it back to busy */
    TssGdt->HighWord.Bits.Dpl = 0;
    TssGdt->HighWord.Bits.Pres = 1;
    TssGdt->HighWord.Bits.Type = I386_ACTIVE_TSS;

    /* Restore nested flag */
    __writeeflags(__readeflags() | EFLAGS_NESTED_TASK);

    /* Handled, return from interrupt */
}

DECLSPEC_NORETURN
VOID
FASTCALL
KiTrap03Handler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* Continue with the common handler */
    KiDebugHandler(TrapFrame, BREAKPOINT_BREAK, 0, 0);
}

DECLSPEC_NORETURN
VOID
FASTCALL
KiTrap04Handler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* Check for VDM trap */
    ASSERT(KiVdmTrap(TrapFrame) == FALSE);

     /* Enable interrupts */
    _enable();

    /* Dispatch the exception */
    KiDispatchException0Args(STATUS_INTEGER_OVERFLOW,
                             TrapFrame->Eip - 1,
                             TrapFrame);
}

DECLSPEC_NORETURN
VOID
FASTCALL
KiTrap05Handler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* Check for VDM trap */
    ASSERT(KiVdmTrap(TrapFrame) == FALSE);

    /* Check for kernel-mode fault */
    if (!KiUserTrap(TrapFrame)) KiSystemFatalException(EXCEPTION_BOUND_CHECK, TrapFrame);

    /* Enable interrupts */
    _enable();

    /* Dispatch the exception */
    KiDispatchException0Args(STATUS_ARRAY_BOUNDS_EXCEEDED,
                             TrapFrame->Eip,
                             TrapFrame);
}

DECLSPEC_NORETURN
VOID
FASTCALL
KiTrap06Handler(IN PKTRAP_FRAME TrapFrame)
{
    PUCHAR Instruction;
    ULONG i;
    KIRQL OldIrql;

    /* Check for V86 GPF */
    if (__builtin_expect(KiV86Trap(TrapFrame), 1))
    {
        /* Enter V86 trap */
        KiEnterV86Trap(TrapFrame);

        /* Must be a VDM process */
        if (__builtin_expect(!PsGetCurrentProcess()->VdmObjects, 0))
        {
            /* Enable interrupts */
            _enable();

            /* Setup illegal instruction fault */
            KiDispatchException0Args(STATUS_ILLEGAL_INSTRUCTION,
                                     TrapFrame->Eip,
                                     TrapFrame);
        }

        /* Go to APC level */
        KeRaiseIrql(APC_LEVEL, &OldIrql);
        _enable();

        /* Check for BOP */
        if (!VdmDispatchBop(TrapFrame))
        {
            /* Should only happen in VDM mode */
            UNIMPLEMENTED_FATAL();
        }

        /* Bring IRQL back */
        KeLowerIrql(OldIrql);
        _disable();

        /* Do a quick V86 exit if possible */
        KiExitV86Trap(TrapFrame);
    }

    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* Enable interrupts */
    Instruction = (PUCHAR)TrapFrame->Eip;
    _enable();

    /* Check for user trap */
    if (KiUserTrap(TrapFrame))
    {
        /* FIXME: Use SEH */

        /* Scan next 4 opcodes */
        for (i = 0; i < 4; i++)
        {
            /* Check for LOCK instruction */
            if (Instruction[i] == 0xF0)
            {
                /* Send invalid lock sequence exception */
                KiDispatchException0Args(STATUS_INVALID_LOCK_SEQUENCE,
                                         TrapFrame->Eip,
                                         TrapFrame);
            }
        }

        /* FIXME: SEH ends here */
    }

    /* Kernel-mode or user-mode fault (but not LOCK) */
    KiDispatchException0Args(STATUS_ILLEGAL_INSTRUCTION,
                             TrapFrame->Eip,
                             TrapFrame);

}

DECLSPEC_NORETURN
VOID
FASTCALL
KiTrap07Handler(IN PKTRAP_FRAME TrapFrame)
{
    PKTHREAD Thread, NpxThread;
    PFX_SAVE_AREA SaveArea, NpxSaveArea;
    ULONG Cr0;

    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* Try to handle NPX delay load */
    for (;;)
    {
        /* Get the current thread */
        Thread = KeGetCurrentThread();

        /* Get the NPX frame */
        SaveArea = KiGetThreadNpxArea(Thread);

        /* Check if emulation is enabled */
        if (SaveArea->Cr0NpxState & CR0_EM)
        {
            /* Not implemented */
            UNIMPLEMENTED_FATAL();
        }

        /* Save CR0 and check NPX state */
        Cr0 = __readcr0();
        if (Thread->NpxState != NPX_STATE_LOADED)
        {
            /* Update CR0 */
            Cr0 &= ~(CR0_MP | CR0_EM | CR0_TS);
            __writecr0(Cr0);

            /* Get the NPX thread */
            NpxThread = KeGetCurrentPrcb()->NpxThread;
            if (NpxThread)
            {
                /* Get the NPX frame */
                NpxSaveArea = KiGetThreadNpxArea(NpxThread);

                /* Save FPU state */
                Ke386SaveFpuState(NpxSaveArea);

                /* Update NPX state */
                NpxThread->NpxState = NPX_STATE_NOT_LOADED;
           }

            /* Load FPU state */
            Ke386LoadFpuState(SaveArea);

            /* Update NPX state */
            Thread->NpxState = NPX_STATE_LOADED;
            KeGetCurrentPrcb()->NpxThread = Thread;

            /* Enable interrupts */
            _enable();

            /* Check if CR0 needs to be reloaded due to context switch */
            if (!SaveArea->Cr0NpxState) KiEoiHelper(TrapFrame);

            /* Otherwise, we need to reload CR0, disable interrupts */
            _disable();

            /* Reload CR0 */
            Cr0 = __readcr0();
            SaveArea->Cr0NpxState = KiNormalizeNpxCr0State(SaveArea->Cr0NpxState);
            Cr0 |= SaveArea->Cr0NpxState;
            Cr0 = KiNormalizeNpxCr0State(Cr0);
            __writecr0(Cr0);

            /* Now restore interrupts and check for TS */
            _enable();
            if (Cr0 & CR0_TS) KiEoiHelper(TrapFrame);

            /* We're still here -- clear TS and try again */
            __writecr0(__readcr0() &~ CR0_TS);
            _disable();
        }
        else
        {
            /* This is an actual fault, not a lack of FPU state */
            break;
        }
    }

    /* TS should not be set */
    if (Cr0 & CR0_TS)
    {
        /*
         * If it's incorrectly set, then maybe the state is actually still valid
         * but we could have lost track of that due to a BIOS call.
         * Make sure MP is still set, which should verify the theory.
         */
        if (Cr0 & CR0_MP)
        {
            /* Indeed, the state is actually still valid, so clear TS */
            __writecr0(__readcr0() &~ CR0_TS);
            KiEoiHelper(TrapFrame);
        }

        /* Otherwise, something strange is going on */
        KeBugCheckWithTf(TRAP_CAUSE_UNKNOWN, 2, Cr0, 0, 0, TrapFrame);
    }

    /* It's not a delayed load, so process this trap as an NPX fault */
    KiNpxHandler(TrapFrame, Thread, SaveArea);
}

DECLSPEC_NORETURN
VOID
__cdecl
KiTrap08Handler(VOID)
{
    PKTSS Tss, DfTss;
    PKTHREAD Thread;
    PKPROCESS Process;
    PKGDTENTRY TssGdt;

    /* For sanity's sake, make sure interrupts are disabled */
    _disable();

    /* Get the current TSS, thread, and process */
    Tss = KeGetPcr()->TSS;
    Thread = ((PKIPCR)KeGetPcr())->PrcbData.CurrentThread;
    Process = Thread->ApcState.Process;

    /* Save data usually not present in the TSS */
    Tss->CR3 = Process->DirectoryTableBase[0];
    Tss->IoMapBase = Process->IopmOffset;
    Tss->LDT = Process->LdtDescriptor.LimitLow ? KGDT_LDT : 0;

    /* Now get the base address of the double-fault TSS */
    TssGdt = &((PKIPCR)KeGetPcr())->GDT[KGDT_DF_TSS / sizeof(KGDTENTRY)];
    DfTss  = (PKTSS)(ULONG_PTR)(TssGdt->BaseLow |
                                TssGdt->HighWord.Bytes.BaseMid << 16 |
                                TssGdt->HighWord.Bytes.BaseHi << 24);

    /*
     * Switch to it and activate it, masking off the nested flag.
     *
     * Note that in reality, we are already on the double-fault TSS
     * -- we just need to update the PCR to reflect this.
     */
    KeGetPcr()->TSS = DfTss;
    __writeeflags(__readeflags() &~ EFLAGS_NESTED_TASK);
    TssGdt->HighWord.Bits.Dpl = 0;
    TssGdt->HighWord.Bits.Pres = 1;
    // TssGdt->HighWord.Bits.Type &= ~0x2; /* I386_ACTIVE_TSS --> I386_TSS */
    TssGdt->HighWord.Bits.Type = I386_TSS; // Busy bit cleared in the TSS selector.

    /* Bugcheck the system */
    KeBugCheckWithTf(UNEXPECTED_KERNEL_MODE_TRAP,
                     EXCEPTION_DOUBLE_FAULT,
                     (ULONG_PTR)Tss,
                     0,
                     0,
                     NULL);
}

DECLSPEC_NORETURN
VOID
FASTCALL
KiTrap09Handler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    DPRINT1("KiTrap09Handler: cpu=%u eip=%p esp=%p eflags=%lx cr0=%lx cr4=%lx\n",
            KeGetCurrentProcessorNumber(),
            (PVOID)TrapFrame->Eip,
            (PVOID)KeGetTrapFrameStackRegister(TrapFrame),
            TrapFrame->EFlags,
            __readcr0(),
            __readcr4());

    /* Enable interrupts and kill the system */
    _enable();
    KiSystemFatalException(EXCEPTION_NPX_OVERRUN, TrapFrame);
}

DECLSPEC_NORETURN
VOID
FASTCALL
KiTrap0AHandler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* Check for VDM trap */
    ASSERT(KiVdmTrap(TrapFrame) == FALSE);

    /* Kill the system */
    KiSystemFatalException(EXCEPTION_INVALID_TSS, TrapFrame);
}

DECLSPEC_NORETURN
VOID
FASTCALL
KiTrap0BHandler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* FIXME: Kill the system */
    UNIMPLEMENTED;
    KiSystemFatalException(EXCEPTION_SEGMENT_NOT_PRESENT, TrapFrame);
}

DECLSPEC_NORETURN
VOID
FASTCALL
KiTrap0CHandler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* FIXME: Kill the system */
    UNIMPLEMENTED;
    KiSystemFatalException(EXCEPTION_STACK_FAULT, TrapFrame);
}

/* DECLSPEC_NORETURN VOID FASTCALL KiTrap0DHandler(IN PKTRAP_FRAME); */
DECLSPEC_NORETURN VOID FASTCALL KiTrap0EHandler(IN PKTRAP_FRAME);

DECLSPEC_NORETURN
VOID
FASTCALL
KiTrap0DHandler(IN PKTRAP_FRAME TrapFrame)
{
    ULONG i, j, Iopl;
    BOOLEAN Privileged = FALSE;
    PUCHAR Instructions;
    UCHAR Instruction = 0;
    KIRQL OldIrql;

    /* Check for V86 GPF */
    if (__builtin_expect(KiV86Trap(TrapFrame), 1))
    {
        /* Enter V86 trap */
        KiEnterV86Trap(TrapFrame);

        /* Must be a VDM process */
        if (__builtin_expect(!PsGetCurrentProcess()->VdmObjects, 0))
        {
            /* Enable interrupts */
            _enable();

            /* Setup illegal instruction fault */
            KiDispatchException0Args(STATUS_ILLEGAL_INSTRUCTION,
                                     TrapFrame->Eip,
                                     TrapFrame);
        }

        /* Go to APC level */
        KeRaiseIrql(APC_LEVEL, &OldIrql);
        _enable();

        /* Handle the V86 opcode */
        if (__builtin_expect(Ki386HandleOpcodeV86(TrapFrame) == 0xFF, 0))
        {
            /* Should only happen in VDM mode */
            UNIMPLEMENTED_FATAL();
        }

        /* Bring IRQL back */
        KeLowerIrql(OldIrql);
        _disable();

        /* Do a quick V86 exit if possible */
        KiExitV86Trap(TrapFrame);
    }

    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* Check for user-mode GPF */
    if (KiUserTrap(TrapFrame))
    {
        /* Should not be VDM */
        ASSERT(KiVdmTrap(TrapFrame) == FALSE);

        /* Enable interrupts and check error code */
        _enable();
        if (!TrapFrame->ErrCode)
        {
            /* FIXME: Use SEH */
            Instructions = (PUCHAR)TrapFrame->Eip;

            /* Scan next 15 bytes */
            for (i = 0; i < 15; i++)
            {
                /* Skip prefix instructions */
                for (j = 0; j < sizeof(KiTrapPrefixTable); j++)
                {
                    /* Is this a prefix instruction? */
                    if (Instructions[i] == KiTrapPrefixTable[j])
                    {
                        /* Stop looking */
                        break;
                    }
                }

                /* Is this NOT any prefix instruction? */
                if (j == sizeof(KiTrapPrefixTable))
                {
                    /* We can go ahead and handle the fault now */
                    Instruction = Instructions[i];
                    break;
                }
            }

            /* If all we found was prefixes, then this instruction is too long */
            if (i == 15)
            {
                /* Setup illegal instruction fault */
                KiDispatchException0Args(STATUS_ILLEGAL_INSTRUCTION,
                                         TrapFrame->Eip,
                                         TrapFrame);
            }

            /* Check for privileged instructions */
            DPRINT("Instruction (%lu) at fault: %lx %lx %lx %lx\n",
                    i,
                    Instructions[i],
                    Instructions[i + 1],
                    Instructions[i + 2],
                    Instructions[i + 3]);
            if (Instruction == 0xF4)                            // HLT
            {
                /* HLT is privileged */
                Privileged = TRUE;
            }
            else if (Instruction == 0x0F)
            {
                /* Test if it's any of the privileged two-byte opcodes */
                if (((Instructions[i + 1] == 0x00) &&              // LLDT or LTR
                     (((Instructions[i + 2] & 0x38) == 0x10) ||        // LLDT
                      (Instructions[i + 2] == 0x18))) ||               // LTR
                    ((Instructions[i + 1] == 0x01) &&              // LGDT or LIDT or LMSW
                     (((Instructions[i + 2] & 0x38) == 0x10) ||        // LGDT
                      (Instructions[i + 2] == 0x18) ||                 // LIDT
                      (Instructions[i + 2] == 0x30))) ||               // LMSW
                    (Instructions[i + 1] == 0x08) ||               // INVD
                    (Instructions[i + 1] == 0x09) ||               // WBINVD
                    (Instructions[i + 1] == 0x35) ||               // SYSEXIT
                    (Instructions[i + 1] == 0x21) ||               // MOV DR, XXX
                    (Instructions[i + 1] == 0x06) ||               // CLTS
                    (Instructions[i + 1] == 0x20) ||               // MOV CR, XXX
                    (Instructions[i + 1] == 0x22) ||               // MOV XXX, CR
                    (Instructions[i + 1] == 0x23) ||               // MOV YYY, DR
                    (Instructions[i + 1] == 0x30) ||               // WRMSR
                    (Instructions[i + 1] == 0x33))                 // RDPMC
                    // INVLPG, INVLPGA, SYSRET
                {
                    /* These are all privileged */
                    Privileged = TRUE;
                }
            }
            else
            {
                /* Get the IOPL and compare with the RPL mask */
                Iopl = (TrapFrame->EFlags & EFLAGS_IOPL) >> 12;
                if ((TrapFrame->SegCs & RPL_MASK) > Iopl)
                {
                    /* I/O privilege error -- check for known instructions */
                    if ((Instruction == 0xFA) || (Instruction == 0xFB)) // CLI or STI
                    {
                        /* These are privileged */
                        Privileged = TRUE;
                    }
                    else
                    {
                        /* Last hope: an IN/OUT instruction */
                        for (j = 0; j < sizeof(KiTrapIoTable); j++)
                        {
                            /* Is this an I/O instruction? */
                            if (Instruction == KiTrapIoTable[j])
                            {
                                /* Then it's privileged */
                                Privileged = TRUE;
                                break;
                            }
                        }
                    }
                }
            }

            /* So now... was the instruction privileged or not? */
            if (Privileged)
            {
                /* Whew! We have a privileged instruction, so dispatch the fault */
                KiDispatchException0Args(STATUS_PRIVILEGED_INSTRUCTION,
                                         TrapFrame->Eip,
                                         TrapFrame);
            }
        }

        /* If we got here, send an access violation */
        KiDispatchException2Args(STATUS_ACCESS_VIOLATION,
                                 TrapFrame->Eip,
                                 0,
                                 0xFFFFFFFF,
                                 TrapFrame);
    }

    /*
     * Check for a fault during checking of the user instruction.
     *
     * Note that the SEH handler will catch invalid EIP, but we could be dealing
     * with an invalid CS, which will generate another GPF instead.
     *
     */
    if ((PVOID)TrapFrame->Eip >= (PVOID)KiTrap0DHandler &&
        (PVOID)TrapFrame->Eip <  (PVOID)KiTrap0EHandler)
    {
        /* Not implemented */
        UNIMPLEMENTED_FATAL();
    }

    /*
     * NOTE: The ASM trap exit code would restore segment registers by doing
     * a POP <SEG>, which could cause an invalid segment if someone had messed
     * with the segment values.
     *
     * Another case is a bogus SS, which would hit a GPF when doing the iret.
     * This could only be done through a buggy or malicious driver, or perhaps
     * the kernel debugger.
     *
     * The kernel normally restores the "true" segment if this happens.
     *
     * However, since we're restoring in C, not ASM, we can't detect
     * POP <SEG> since the actual instructions will be different.
     *
     * A better technique would be to check the EIP and somehow edit the
     * trap frame before restarting the instruction -- but we would need to
     * know the extract instruction that was used first.
     *
     * We could force a special instrinsic to use stack instructions, or write
     * a simple instruction length checker.
     *
     * Nevertheless, this is a lot of work for the purpose of avoiding a crash
     * when the user is purposedly trying to create one from kernel-mode, so
     * we should probably table this for now since it's not a "real" issue.
     */

    /*
     * NOTE2: Another scenario is the IRET during a V8086 restore (BIOS Call)
     * which will cause a GPF since the trap frame is a total mess (on purpose)
     * as built in KiEnterV86Mode.
     *
     * The idea is to scan for IRET, scan for the known EIP address, validate CS
     * and then manually issue a jump to the V8086 return EIP.
     */
    Instructions = (PUCHAR)TrapFrame->Eip;
    if (Instructions[0] == 0xCF)
    {
        /*
         * Some evil shit is going on here -- this is not the SS:ESP you're
         * looking for! Instead, this is actually CS:EIP you're looking at!
         * Why? Because part of the trap frame actually corresponds to the IRET
         * stack during the trap exit!
         */
        if ((TrapFrame->HardwareEsp == (ULONG)Ki386BiosCallReturnAddress) &&
            (TrapFrame->HardwareSegSs == (KGDT_R0_CODE | RPL_MASK)))
        {
            /* Exit the V86 trap! */
            Ki386BiosCallReturnAddress(TrapFrame);
        }
        else
        {
            /* Otherwise, this is another kind of IRET fault */
            UNIMPLEMENTED_FATAL();
        }
    }

     /* So since we're not dealing with the above case, check for RDMSR/WRMSR */
    if ((Instructions[0] == 0xF) &&            // 2-byte opcode
        ((Instructions[1] == 0x32) ||        // RDMSR
         (Instructions[1] == 0x30)))         // WRMSR
    {
        /* Unknown CPU MSR, so raise an access violation */
        KiDispatchException0Args(STATUS_ACCESS_VIOLATION,
                                 TrapFrame->Eip,
                                 TrapFrame);
    }

    /* Check for lazy segment load */
    if (TrapFrame->SegDs != (KGDT_R3_DATA | RPL_MASK))
    {
        /* Fix it */
        TrapFrame->SegDs = (KGDT_R3_DATA | RPL_MASK);
    }
    else if (TrapFrame->SegEs != (KGDT_R3_DATA | RPL_MASK))
    {
        /* Fix it */
        TrapFrame->SegEs = (KGDT_R3_DATA | RPL_MASK);
    }
    else
    {
        /* Whatever it is, we can't handle it */
        KiSystemFatalException(EXCEPTION_GP_FAULT, TrapFrame);
    }

    /* Return to where we came from */
    KiTrapReturn(TrapFrame);
}

BOOLEAN
FASTCALL
KiCheckForSListFault(PKTRAP_FRAME TrapFrame)
{
    /* Explanation: An S-List fault can occur due to a race condition between 2
       threads simultaneously trying to pop an element from the S-List. After
       thread 1 has read the pointer to the top element on the S-List it is
       preempted and thread 2 calls InterlockedPopEntrySlist on the same S-List,
       removing the top element and freeing it's memory. After that thread 1
       resumes and tries to read the address of the Next pointer from the top
       element, which it assumes will be the next top element.
       But since that memory has been freed, we get a page fault. To handle this
       race condition, we let thread 1 repeat the operation.
       We do NOT invoke the page fault handler in this case, since we do not
       want to trigger any side effects, like paging or a guard page fault.

       Sequence of operations:

           Thread 1 : mov eax, [ebp] <= eax now points to the first element
           Thread 1 : mov edx, [ebp + 4] <= edx is loaded with Depth and Sequence
            *** preempted ***
           Thread 2 : calls InterlockedPopEntrySlist, changing the top element
           Thread 2 : frees the memory of the element that was popped
            *** preempted ***
           Thread 1 : checks if eax is NULL
           Thread 1 : InterlockedPopEntrySListFault: mov ebx, [eax] <= faults

        To be sure that we are dealing with exactly the case described above, we
        check whether the ListHeader has changed. If Thread 2 only popped one
        entry, the Next field in the S-List-header has changed.
        If after thread 1 has faulted, thread 2 allocates a new element, by
        chance getting the same address as the previously freed element and
        pushes it on the list again, we will see the same top element, but the
        Sequence member of the S-List header has changed. Therefore we check
        both fields to make sure we catch any concurrent modification of the
        S-List-header.
    */
    if ((TrapFrame->Eip == (ULONG_PTR)ExpInterlockedPopEntrySListFault) ||
        (TrapFrame->Eip == (ULONG_PTR)KeUserPopEntrySListFault))
    {
        ULARGE_INTEGER SListHeader;
        PVOID ResumeAddress;

        PUCHAR Instruction = (PUCHAR)TrapFrame->Eip;

        /* Sanity check that the assembly is correct:
           mov ebx, [eax], followed by cmpxchg8b [ebp].
           SMP builds lock-prefix the cmpxchg8b instruction. */
        ASSERT((Instruction[0] == 0x8B) &&
               (Instruction[1] == 0x18));

        if (Instruction[2] == 0xF0)
        {
            ASSERT((Instruction[3] == 0x0F) &&
                   (Instruction[4] == 0xC7) &&
                   (Instruction[5] == 0x4D) &&
                   (Instruction[6] == 0x00));
        }
        else
        {
            ASSERT((Instruction[2] == 0x0F) &&
                   (Instruction[3] == 0xC7) &&
                   (Instruction[4] == 0x4D) &&
                   (Instruction[5] == 0x00));
        }

        /* Check if this is a user fault */
        if (TrapFrame->Eip == (ULONG_PTR)KeUserPopEntrySListFault)
        {
            /* EBP points to the S-List-header. Copy it inside SEH, to protect
               against a bogus pointer from user mode */
            _SEH2_TRY
            {
                ProbeForRead((PVOID)TrapFrame->Ebp,
                             sizeof(ULARGE_INTEGER),
                             TYPE_ALIGNMENT(SLIST_HEADER));
                SListHeader = *(PULARGE_INTEGER)TrapFrame->Ebp;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* The S-List pointer is not valid! */
                return FALSE;
            }
            _SEH2_END;
            ResumeAddress = KeUserPopEntrySListResume;
        }
        else
        {
            SListHeader = *(PULARGE_INTEGER)TrapFrame->Ebp;
            ResumeAddress = ExpInterlockedPopEntrySListResume;
        }

        /* Check if either the Next pointer or the Sequence member in the
           S-List-header has changed. If any of these has changed, we restart
           the operation. Otherwise we only have a bogus pointer and let the
           page fault handler deal with it. */
        if ((SListHeader.LowPart != TrapFrame->Eax) ||
            (SListHeader.HighPart != TrapFrame->Edx))
        {
            DPRINT1("*** Got an S-List-Fault ***\n");
            KeGetCurrentThread()->SListFaultCount++;

            /* Restart the operation */
            TrapFrame->Eip = (ULONG_PTR)ResumeAddress;

            return TRUE;
        }
    }

    return FALSE;
}

DECLSPEC_NORETURN
VOID
FASTCALL
KiTrap0EHandler(IN PKTRAP_FRAME TrapFrame)
{
    PKTHREAD Thread;
    PKTRAP_FRAME CurrentTrapFrame;
    ULONG_PTR Cr2;
    ULONG_PTR PreviousTrapFrame;
    NTSTATUS Status;

    Thread = KeGetCurrentThread();
    CurrentTrapFrame = Thread != NULL ? KeGetTrapFrame(Thread) : NULL;
    PreviousTrapFrame = (ULONG_PTR)CurrentTrapFrame;

    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* Check if this is the base frame */
    if ((CurrentTrapFrame != NULL) && (CurrentTrapFrame != TrapFrame))
    {
        /* It isn't, check if this is a second nested frame */
        if (((ULONG_PTR)CurrentTrapFrame - (ULONG_PTR)TrapFrame) <=
            FIELD_OFFSET(KTRAP_FRAME, EFlags))
        {
            /* The stack is somewhere in between frames, we need to fix it */
            UNIMPLEMENTED_FATAL();
        }
    }

    /* Save CR2 */
    Cr2 = __readcr2();

#if DBG && defined(_M_IX86)
    KiTracePageFaultProbe(TrapFrame,
                          Cr2,
                          2,
                          0xE133,
                          TrapFrame->EFlags,
                          PreviousTrapFrame);
#endif

    /* Enable interrupts */
    _enable();

    /* Check if we came in with interrupts disabled */
    if (!(TrapFrame->EFlags & EFLAGS_INTERRUPT_MASK))
    {
#if DBG && defined(_M_IX86)
        PETHREAD CurrentThread = PsGetCurrentThread();
        PEPROCESS CurrentProcess = CurrentThread != NULL ?
                                   CurrentThread->ThreadsProcess : NULL;

        KiI386BootTraceRecord(0xE130,
                              (ULONG_PTR)TrapFrame,
                              Cr2,
                              TrapFrame->Eip,
                              (ULONG_PTR)TrapFrame->ExceptionList,
                              PreviousTrapFrame,
                              TrapFrame->EFlags);
        KiI386BootTraceRecord(0xE131,
                              (ULONG_PTR)TrapFrame,
                              (ULONG_PTR)CurrentThread,
                              (ULONG_PTR)CurrentProcess,
                              CurrentThread != NULL ?
                                  (ULONG_PTR)CurrentThread->Cid.UniqueThread :
                                  0,
                              CurrentThread != NULL ?
                                  (ULONG_PTR)CurrentThread->Cid.UniqueProcess :
                                  0,
                              (ULONG_PTR)Thread);
        KiI386BootTraceRecord(0xE132,
                              (ULONG_PTR)TrapFrame,
                              TrapFrame->Eip,
                              KiPackProcessImageNamePart(CurrentProcess, 0),
                              KiPackProcessImageNamePart(CurrentProcess, 4),
                              KiPackProcessImageNamePart(CurrentProcess, 8),
                              KiPackProcessImageNamePart(CurrentProcess, 12));
#endif
        KiRecordPageFaultProbe(TrapFrame, Cr2, 1, PreviousTrapFrame);
#if DBG && defined(_M_IX86)
        KiRawCom1DumpTrap0E(TrapFrame, Cr2, PreviousTrapFrame);
#endif

        /* This is completely illegal, bugcheck the system */
        KeBugCheckWithTf(IRQL_NOT_LESS_OR_EQUAL,
                         Cr2,
                         (ULONG_PTR)-1,
                         TrapFrame->ErrCode,
                         TrapFrame->Eip,
                         TrapFrame);
    }

    /* Check for S-List fault */
    if (KiCheckForSListFault(TrapFrame))
    {
        /* Continue execution */
        KiEoiHelper(TrapFrame);
    }

    /* Call the access fault handler */
#if DBG && defined(_M_IX86)
    KiTracePageFaultProbe(TrapFrame,
                          Cr2,
                          3,
                          0xE134,
                          KiUserTrap(TrapFrame),
                          PreviousTrapFrame);
#endif
    Status = MmAccessFault(TrapFrame->ErrCode,
                           (PVOID)Cr2,
                           KiUserTrap(TrapFrame),
                           TrapFrame);
#if DBG && defined(_M_IX86)
    KiTracePageFaultProbe(TrapFrame,
                          Cr2,
                          4,
                          0xE135,
                          Status,
                          PreviousTrapFrame);
    {
        PKPRCB CurrentPrcbProbe = KeGetCurrentPrcb();

        KiI386BootTraceRecord(0xE13A,
                              (ULONG_PTR)TrapFrame,
                              (ULONG_PTR)Thread,
                              (ULONG_PTR)KeGetCurrentThread(),
                              (ULONG_PTR)CurrentPrcbProbe->CurrentThread,
                              (ULONG_PTR)CurrentPrcbProbe->NextThread,
                              (ULONG_PTR)KeGetPcr()->NtTib.ExceptionList);
    }
#endif
    if (NT_SUCCESS(Status))
    {
#if DBG && defined(_M_IX86)
        KiTracePageFaultProbe(TrapFrame,
                              Cr2,
                              5,
                              0xE136,
                              Status,
                              PreviousTrapFrame);
        {
            PKPRCB CurrentPrcbProbe = KeGetCurrentPrcb();

            KiI386BootTraceRecord(0xE13B,
                                  (ULONG_PTR)TrapFrame,
                                  (ULONG_PTR)Thread,
                                  (ULONG_PTR)KeGetCurrentThread(),
                                  (ULONG_PTR)CurrentPrcbProbe->CurrentThread,
                                  (ULONG_PTR)CurrentPrcbProbe->NextThread,
                                  (ULONG_PTR)KeGetPcr()->NtTib.ExceptionList);
        }
#endif
        /* Check whether the kernel debugger has owed breakpoints to be inserted */
        KdSetOwedBreakpoints();
        /* We succeeded, return */
        KiEoiHelper(TrapFrame);
    }

    /* Check for syscall fault */
#if 0
    if ((TrapFrame->Eip == (ULONG_PTR)CopyParams) ||
        (TrapFrame->Eip == (ULONG_PTR)ReadBatch))
    {
        /* Not yet implemented */
        UNIMPLEMENTED_FATAL();
    }
#endif

    /* Check for VDM trap */
    if (KiVdmTrap(TrapFrame))
    {
        DPRINT1("VDM PAGE FAULT at %lx:%lx for address %lx\n",
                TrapFrame->SegCs, TrapFrame->Eip, Cr2);
        if (VdmDispatchPageFault(TrapFrame))
        {
            /* Return and end VDM execution */
            DPRINT1("VDM page fault with status 0x%lx resolved\n", Status);
            KiEoiHelper(TrapFrame);
        }
        DPRINT1("VDM page fault with status 0x%lx NOT resolved\n", Status);
    }

    /* Either kernel or user trap (non VDM) so dispatch exception */
    if (Status == STATUS_ACCESS_VIOLATION)
    {
#if DBG && defined(_M_IX86)
        KiTracePageFaultProbe(TrapFrame,
                              Cr2,
                              6,
                              0xE137,
                              Status,
                              PreviousTrapFrame);
#endif
        /* This status code is repurposed so we can recognize it later */
        KiDispatchException2Args(KI_EXCEPTION_ACCESS_VIOLATION,
                                 TrapFrame->Eip,
                                 MI_IS_WRITE_ACCESS(TrapFrame->ErrCode),
                                 Cr2,
                                 TrapFrame);
    }
    else if ((Status == STATUS_GUARD_PAGE_VIOLATION) ||
             (Status == STATUS_STACK_OVERFLOW))
    {
#if DBG && defined(_M_IX86)
        KiTracePageFaultProbe(TrapFrame,
                              Cr2,
                              7,
                              0xE138,
                              Status,
                              PreviousTrapFrame);
#endif
        /* These faults only have two parameters */
        KiDispatchException2Args(Status,
                                 TrapFrame->Eip,
                                 MI_IS_WRITE_ACCESS(TrapFrame->ErrCode),
                                 Cr2,
                                 TrapFrame);
    }

    /* Only other choice is an in-page error, with 3 parameters */
#if DBG && defined(_M_IX86)
    KiTracePageFaultProbe(TrapFrame,
                          Cr2,
                          8,
                          0xE139,
                          Status,
                          PreviousTrapFrame);
#endif
    KiDispatchExceptionFromTrapFrame(STATUS_IN_PAGE_ERROR,
                                     0,
                                     TrapFrame->Eip,
                                     3,
                                     MI_IS_WRITE_ACCESS(TrapFrame->ErrCode),
                                     Cr2,
                                     Status,
                                     TrapFrame);
}

DECLSPEC_NORETURN
VOID
FASTCALL
KiTrap0FHandler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* FIXME: Kill the system */
    UNIMPLEMENTED;
    KiSystemFatalException(EXCEPTION_RESERVED_TRAP, TrapFrame);
}

DECLSPEC_NORETURN
VOID
FASTCALL
KiTrap10Handler(IN PKTRAP_FRAME TrapFrame)
{
    PKTHREAD Thread;
    PFX_SAVE_AREA SaveArea;

    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* Check if this is the NPX thrad */
    Thread = KeGetCurrentThread();
    SaveArea = KiGetThreadNpxArea(Thread);
    if (Thread != KeGetCurrentPrcb()->NpxThread)
    {
        /* It isn't, enable interrupts and set delayed error */
        _enable();
        SaveArea->Cr0NpxState |= CR0_TS | CR0_MP;

        /* End trap */
        KiEoiHelper(TrapFrame);
    }

    /* Otherwise, proceed with NPX fault handling */
    KiNpxHandler(TrapFrame, Thread, SaveArea);
}

DECLSPEC_NORETURN
VOID
FASTCALL
KiTrap11Handler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* Enable interrupts and kill the system */
    _enable();
    KiSystemFatalException(EXCEPTION_ALIGNMENT_CHECK, TrapFrame);
}

DECLSPEC_NORETURN
VOID
FASTCALL
KiTrap13Handler(IN PKTRAP_FRAME TrapFrame)
{
    PKTHREAD Thread;
    PFX_SAVE_AREA SaveArea;
    ULONG Cr0, MxCsrMask, Error;

    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* Check if this is the NPX thrad */
    Thread = KeGetCurrentThread();
    if (Thread != KeGetCurrentPrcb()->NpxThread)
    {
        /* It isn't, kill the system */
        KeBugCheckWithTf(TRAP_CAUSE_UNKNOWN, 13, (ULONG_PTR)Thread, 0, 0, TrapFrame);
    }

    /* Get the NPX frame */
    SaveArea = KiGetThreadNpxArea(Thread);

    /* Check for VDM trap */
    ASSERT(KiVdmTrap(TrapFrame) == FALSE);

    /* Check for user trap */
    if (!KiUserTrap(TrapFrame))
    {
        /* Kernel should not fault on XMMI */
        KeBugCheckWithTf(TRAP_CAUSE_UNKNOWN, 13, 0, 0, 2, TrapFrame);
    }

    /* Update CR0 */
    Cr0 = __readcr0();
    Cr0 &= ~(CR0_MP | CR0_EM | CR0_TS);
    __writecr0(Cr0);

    /* Save FPU state */
    Ke386SaveFpuState(SaveArea);

    /* Mark CR0 state dirty */
    Cr0 |= NPX_STATE_NOT_LOADED;
    Cr0 |= SaveArea->Cr0NpxState;
    Cr0 = KiNormalizeNpxCr0State(Cr0);
     __writecr0(Cr0);

    /* Update NPX state */
    Thread->NpxState = NPX_STATE_NOT_LOADED;
    KeGetCurrentPrcb()->NpxThread = NULL;

    /* Clear the TS bit and re-enable interrupts */
    SaveArea->Cr0NpxState &= ~CR0_TS;
    _enable();

    /* Now look at MxCsr to get the mask of errors we should care about */
    MxCsrMask = ~((USHORT)SaveArea->U.FxArea.MXCsr >> 7);

    /* Get legal exceptions that software should handle */
    Error = (USHORT)SaveArea->U.FxArea.MXCsr & (FSW_INVALID_OPERATION |
                                                FSW_DENORMAL |
                                                FSW_ZERO_DIVIDE |
                                                FSW_OVERFLOW |
                                                FSW_UNDERFLOW |
                                                FSW_PRECISION);
    Error &= MxCsrMask;

    /* Now handle any of those legal errors */
    if (Error & (FSW_INVALID_OPERATION |
                 FSW_DENORMAL |
                 FSW_ZERO_DIVIDE |
                 FSW_OVERFLOW |
                 FSW_UNDERFLOW |
                 FSW_PRECISION))
    {
        /* By issuing an exception */
        KiDispatchException1Args(STATUS_FLOAT_MULTIPLE_TRAPS,
                                 TrapFrame->Eip,
                                 0,
                                 TrapFrame);
    }

    /* Unknown XMMI fault */
    KeBugCheckWithTf(TRAP_CAUSE_UNKNOWN, 13, 0, 0, 1, TrapFrame);
}

/* SOFTWARE SERVICES **********************************************************/

VOID
FASTCALL
KiRaiseSecurityCheckFailureHandler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* Decrement EIP to point to the INT29 instruction (2 bytes, not 1 like INT3) */
    TrapFrame->Eip -= 2;

#if DBG && defined(_M_IX86)
    KiRawCom1DumpFastFail(TrapFrame);
#endif

    /* Check if this is a user trap */
    if (KiUserTrap(TrapFrame))
    {
        /* Dispatch exception to user mode */
        KiDispatchExceptionFromTrapFrame(STATUS_STACK_BUFFER_OVERRUN,
                                         EXCEPTION_NONCONTINUABLE,
                                         TrapFrame->Eip,
                                         1,
                                         TrapFrame->Ecx,
                                         0,
                                         0,
                                         TrapFrame);
    }
    else
    {
        EXCEPTION_RECORD ExceptionRecord;

        /* Bugcheck the system */
        ExceptionRecord.ExceptionCode = STATUS_STACK_BUFFER_OVERRUN;
        ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
        ExceptionRecord.ExceptionRecord = NULL;
        ExceptionRecord.ExceptionAddress = (PVOID)TrapFrame->Eip;
        ExceptionRecord.NumberParameters = 1;
        ExceptionRecord.ExceptionInformation[0] = TrapFrame->Ecx;

        KeBugCheckWithTf(KERNEL_SECURITY_CHECK_FAILURE,
                         TrapFrame->Ecx,
                         (ULONG_PTR)TrapFrame,
                         (ULONG_PTR)&ExceptionRecord,
                         0,
                         TrapFrame);
    }
}

VOID
FASTCALL
KiGetTickCountHandler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /*
     * Just fail the request
     */
    DbgPrint("INT 0x2A attempted, returning 0 tick count\n");
    TrapFrame->Eax = 0;

    /* Exit the trap */
    KiEoiHelper(TrapFrame);
}

VOID
FASTCALL
KiCallbackReturnHandler(IN PKTRAP_FRAME TrapFrame)
{
    PKTHREAD Thread;
    NTSTATUS Status;

    /* Save the SEH chain, NtCallbackReturn will restore this */
    TrapFrame->ExceptionList = KeGetPcr()->NtTib.ExceptionList;

    /* Set thread fields */
    Thread = KeGetCurrentThread();
    Thread->TrapFrame = TrapFrame;
    Thread->PreviousMode = KiUserTrap(TrapFrame);
    ASSERT(Thread->PreviousMode != KernelMode);

    /* Pass the register parameters to NtCallbackReturn.
       Result pointer is in ecx, result length in edx, status in eax */
    Status = NtCallbackReturn((PVOID)TrapFrame->Ecx,
                              TrapFrame->Edx,
                              TrapFrame->Eax);

    /* If we got here, something went wrong. Return an error to the caller */
    KiServiceExit(TrapFrame, Status);
}

DECLSPEC_NORETURN
VOID
FASTCALL
KiRaiseAssertionHandler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* Decrement EIP to point to the INT2C instruction (2 bytes, not 1 like INT3) */
    TrapFrame->Eip -= 2;

    /* Dispatch the exception */
    KiDispatchException0Args(STATUS_ASSERTION_FAILURE,
                             TrapFrame->Eip,
                             TrapFrame);
}

DECLSPEC_NORETURN
VOID
FASTCALL
KiDebugServiceHandler(IN PKTRAP_FRAME TrapFrame)
{
#if DBG && defined(_M_IX86)
    if (TrapFrame->Eax == BREAKPOINT_LOAD_SYMBOLS)
    {
        KiRawCom1DumpDebugService(0x2D01,
                                  TrapFrame,
                                  TrapFrame->Eax,
                                  TrapFrame->Ecx,
                                  TrapFrame->Edx);
    }
#endif

    /* Save trap frame */
    KiEnterTrap(TrapFrame);

#if DBG && defined(_M_IX86)
    if (TrapFrame->Eax == BREAKPOINT_LOAD_SYMBOLS)
    {
        KiRawCom1DumpDebugService(0x2D02,
                                  TrapFrame,
                                  TrapFrame->Eax,
                                  TrapFrame->Ecx,
                                  TrapFrame->Edx);
    }
#endif

    /* Increment EIP to skip the INT3 instruction */
    TrapFrame->Eip++;

#if DBG && defined(_M_IX86)
    if (TrapFrame->Eax == BREAKPOINT_LOAD_SYMBOLS)
    {
        KiRawCom1DumpDebugService(0x2D03,
                                  TrapFrame,
                                  TrapFrame->Eax,
                                  TrapFrame->Ecx,
                                  TrapFrame->Edx);
    }
#endif

    /* Continue with the common handler */
    KiDebugHandler(TrapFrame, TrapFrame->Eax, TrapFrame->Ecx, TrapFrame->Edx);
}


FORCEINLINE
VOID
KiDbgPreServiceHook(ULONG SystemCallNumber, PULONG_PTR Arguments)
{
#if DBG && !defined(_WINKD_)
    if (SystemCallNumber >= 0x1000 && KeWin32PreServiceHook)
        KeWin32PreServiceHook(SystemCallNumber, Arguments);
#endif
}

FORCEINLINE
ULONG_PTR
KiDbgPostServiceHook(ULONG SystemCallNumber, ULONG_PTR Result)
{
#if DBG && !defined(_WINKD_)
    if (SystemCallNumber >= 0x1000 && KeWin32PostServiceHook)
        return KeWin32PostServiceHook(SystemCallNumber, Result);
#endif
    return Result;
}

DECLSPEC_NORETURN
VOID
FASTCALL
KiSystemServiceHandler(IN PKTRAP_FRAME TrapFrame,
                       IN PVOID Arguments)
{
    PKTHREAD Thread;
    PKSERVICE_TABLE_DESCRIPTOR DescriptorTable;
    ULONG Id, Offset, StackBytes;
    NTSTATUS Status;
    PVOID Handler;
    ULONG SystemCallNumber = TrapFrame->Eax;

    /* Get the current thread */
    Thread = KeGetCurrentThread();

    /* Set debug header */
    KiFillTrapFrameDebug(TrapFrame);

    /* Chain trap frames */
    TrapFrame->Edx = (ULONG_PTR)Thread->TrapFrame;

    /* No error code */
    TrapFrame->ErrCode = 0;

    /* Save previous mode */
    TrapFrame->PreviousPreviousMode = Thread->PreviousMode;

    /* Save the SEH chain and terminate it for now */
    TrapFrame->ExceptionList = KeGetPcr()->NtTib.ExceptionList;
    KeGetPcr()->NtTib.ExceptionList = EXCEPTION_CHAIN_END;

    /* Default to debugging disabled */
    TrapFrame->Dr7 = 0;

    /* Check if the frame was from user mode */
    if (KiUserTrap(TrapFrame))
    {
        /* Check for active debugging */
        if (KeGetCurrentThread()->Header.DebugActive & 0xFF)
        {
            /* Handle debug registers */
            KiHandleDebugRegistersOnTrapEntry(TrapFrame);
        }
    }

    /* Set thread fields */
    Thread->TrapFrame = TrapFrame;
    Thread->PreviousMode = KiUserTrap(TrapFrame);

    /* Enable interrupts */
    _enable();

    /* Decode the system call number */
    Offset = (SystemCallNumber >> SERVICE_TABLE_SHIFT) & SERVICE_TABLE_MASK;
    Id = SystemCallNumber & SERVICE_NUMBER_MASK;

    /* Get descriptor table */
    DescriptorTable = (PVOID)((ULONG_PTR)Thread->ServiceTable + Offset);

    /* Validate the system call number */
    if (__builtin_expect(Id >= DescriptorTable->Limit, 0))
    {
        /* Check if this is a GUI call */
        if (!(Offset & SERVICE_TABLE_TEST))
        {
            /* Fail the call */
            Status = STATUS_INVALID_SYSTEM_SERVICE;
            goto ExitCall;
        }

        /* Convert us to a GUI thread -- must wrap in ASM to get new EBP */
        Status = KiConvertToGuiThread();

        /* Reload trap frame and descriptor table pointer from new stack */
        TrapFrame = *(volatile PVOID*)&Thread->TrapFrame;
        DescriptorTable = (PVOID)(*(volatile ULONG_PTR*)&Thread->ServiceTable + Offset);

        if (!NT_SUCCESS(Status))
        {
            /* Set the last error and fail */
            goto ExitCall;
        }

        /* Validate the system call number again */
        if (Id >= DescriptorTable->Limit)
        {
            /* Fail the call */
            Status = STATUS_INVALID_SYSTEM_SERVICE;
            goto ExitCall;
        }
    }

    /* Check if this is a GUI call */
    if (__builtin_expect(Offset & SERVICE_TABLE_TEST, 0))
    {
        /* Get the batch count and flush if necessary */
        if (NtCurrentTeb()->GdiBatchCount) KeGdiFlushUserBatch();
    }

    /* Increase system call count */
    KeGetCurrentPrcb()->KeSystemCalls++;

    /* FIXME: Increase individual counts on debug systems */
    //KiIncreaseSystemCallCount(DescriptorTable, Id);

    /* Get stack bytes */
    StackBytes = DescriptorTable->Number[Id];

    /* Probe caller stack */
    if (__builtin_expect((Arguments < (PVOID)MmUserProbeAddress) && !(KiUserTrap(TrapFrame)), 0))
    {
        /* Access violation */
        UNIMPLEMENTED_FATAL();
    }

    /* Call pre-service debug hook */
    KiDbgPreServiceHook(SystemCallNumber, Arguments);

    /* Get the handler and make the system call */
    Handler = (PVOID)DescriptorTable->Base[Id];
    Status = KiSystemCallTrampoline(Handler, Arguments, StackBytes);

    /* Call post-service debug hook */
    Status = KiDbgPostServiceHook(SystemCallNumber, Status);

    /* Make sure we're exiting correctly */
    KiExitSystemCallDebugChecks(Id, TrapFrame);

    /* Restore the old trap frame */
ExitCall:
    Thread->TrapFrame = (PKTRAP_FRAME)TrapFrame->Edx;

    /* Exit from system call */
    KiServiceExit(TrapFrame, Status);
}

VOID
FASTCALL
KiCheckForSListAddress(IN PKTRAP_FRAME TrapFrame)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
VOID
NTAPI
Kei386EoiHelper(VOID)
{
    /* We should never see this call happening */
    KeBugCheck(MISMATCHED_HAL);
}

/* EOF */
