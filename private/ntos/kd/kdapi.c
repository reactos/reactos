/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    kdapi.c

Abstract:

    Implementation of Kernel Debugger portable remote APIs.

Author:

    Mark Lucovsky (markl) 31-Aug-1990

Revision History:

    John Vert (jvert) 28-May-1991

        Added APIs for reading and writing physical memory
        (KdpReadPhysicalMemory and KdpWritePhysicalMemory)

    Wesley Witt (wesw) 18-Aug-1993

        Added KdpGetVersion, KdpWriteBreakPointEx, & KdpRestoreBreakPointEx


--*/

#include "kdp.h"

#ifdef _GAMBIT_
#include "Ssc.h"
#endif // _GAMBIT_

#if ACCASM && !defined(_MSC_VER)
long asm(const char *,...);
#pragma intrinsic(asm)
#endif

LARGE_INTEGER KdpQueryPerformanceCounter (
    IN PKTRAP_FRAME TrapFrame
    );

extern LARGE_INTEGER Magic10000;
#define SHIFT10000   13
#define Convert100nsToMilliseconds(LARGE_INTEGER) (                         \
    RtlExtendedMagicDivide( (LARGE_INTEGER), Magic10000, SHIFT10000 )       \
    )

//
// Define forward referenced function prototypes.
//

VOID
KdpProcessInternalBreakpoint (
    ULONG BreakpointNumber
    );

NTSTATUS
KdQuerySpecialCalls (
    PDBGKD_MANIPULATE_STATE m,
    ULONG Length,
    PULONG RequiredLength
    );

VOID
KdSetSpecialCall (
    PDBGKD_MANIPULATE_STATE m,
    PCONTEXT ContextRecord
    );

VOID
KdClearSpecialCalls (
    VOID
    );

VOID
KdpGetVersion(
    IN PDBGKD_MANIPULATE_STATE m
    );

NTSTATUS
KdpNotSupported(
    IN PDBGKD_MANIPULATE_STATE m
    );

VOID
KdpCauseBugCheck(
    IN PDBGKD_MANIPULATE_STATE m
    );

NTSTATUS
KdpWriteBreakPointEx(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpRestoreBreakPointEx(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpSearchMemory(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

#if i386
VOID
InternalBreakpointCheck (
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    );

VOID
KdSetInternalBreakpoint (
    IN PDBGKD_MANIPULATE_STATE m
    );

NTSTATUS
KdGetTraceInformation(
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength
    );

VOID
KdGetInternalBreakpoint(
    IN PDBGKD_MANIPULATE_STATE m
    );

VOID
KdGetInternalBreakpoint(
    IN PDBGKD_MANIPULATE_STATE m
    );

long
SymNumFor(
    ULONG_PTR pc
    );

void PotentialNewSymbol (ULONG_PTR pc);

void DumpTraceData(PSTRING MessageData);

BOOLEAN
TraceDataRecordCallInfo(
    ULONG InstructionsTraced,
    LONG CallLevelChange,
    ULONG_PTR pc
    );

BOOLEAN
SkippingWhichBP (
    PVOID thread,
    PULONG BPNum
    );

BOOLEAN
KdpCheckTracePoint(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PCONTEXT ContextRecord
    );

ULONG_PTR
KdpGetReturnAddress(
    IN PCONTEXT ContextRecord
    );

ULONG_PTR
KdpGetCallNextOffset (
    ULONG_PTR Pc,
    IN PCONTEXT ContextRecord
    );

LONG
KdpLevelChange (
    ULONG_PTR Pc,
    PCONTEXT ContextRecord,
    IN OUT PBOOLEAN SpecialCall
    );

#endif // i386

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEKD, KdEnterDebugger)
#pragma alloc_text(PAGEKD, KdExitDebugger)
#pragma alloc_text(PAGEKD, KdpTimeSlipDpcRoutine)
#pragma alloc_text(PAGEKD, KdpTimeSlipWork)
#pragma alloc_text(PAGEKD, KdpSendWaitContinue)
#pragma alloc_text(PAGEKD, KdpReadVirtualMemory)
#pragma alloc_text(PAGEKD, KdpReadVirtualMemory64)
#pragma alloc_text(PAGEKD, KdpWriteVirtualMemory)
#pragma alloc_text(PAGEKD, KdpWriteVirtualMemory64)
#pragma alloc_text(PAGEKD, KdpGetContext)
#pragma alloc_text(PAGEKD, KdpSetContext)
#pragma alloc_text(PAGEKD, KdpWriteBreakpoint)
#pragma alloc_text(PAGEKD, KdpRestoreBreakpoint)
#pragma alloc_text(PAGEKD, KdpReportExceptionStateChange)
#pragma alloc_text(PAGEKD, KdpReportLoadSymbolsStateChange)
#pragma alloc_text(PAGEKD, KdpReadPhysicalMemory)
#pragma alloc_text(PAGEKD, KdpWritePhysicalMemory)
#pragma alloc_text(PAGEKD, KdpGetVersion)
#pragma alloc_text(PAGEKD, KdpNotSupported)
#pragma alloc_text(PAGEKD, KdpCauseBugCheck)
#pragma alloc_text(PAGEKD, KdpWriteBreakPointEx)
#pragma alloc_text(PAGEKD, KdpRestoreBreakPointEx)
#pragma alloc_text(PAGEKD, KdpSearchMemory)
#if DBG
#pragma alloc_text(PAGEKD, KdpDprintf)
#endif
#if i386
#pragma alloc_text(PAGEKD, InternalBreakpointCheck)
#pragma alloc_text(PAGEKD, KdSetInternalBreakpoint)
#pragma alloc_text(PAGEKD, KdGetTraceInformation)
#pragma alloc_text(PAGEKD, KdGetInternalBreakpoint)
#pragma alloc_text(PAGEKD, SymNumFor)
#pragma alloc_text(PAGEKD, PotentialNewSymbol)
#pragma alloc_text(PAGEKD, DumpTraceData)
#pragma alloc_text(PAGEKD, TraceDataRecordCallInfo)
#pragma alloc_text(PAGEKD, SkippingWhichBP)
#pragma alloc_text(PAGEKD, KdQuerySpecialCalls)
#pragma alloc_text(PAGEKD, KdSetSpecialCall)
#pragma alloc_text(PAGEKD, KdClearSpecialCalls)
#pragma alloc_text(PAGEKD, KdpCheckTracePoint)
#pragma alloc_text(PAGEKD, KdpProcessInternalBreakpoint)
#endif // i386
#endif // ALLOC_PRAGMA


//
// This variable has a count for each time KdDisableDebugger has been called.
//
LONG KdDisableCount = 0 ;
BOOLEAN KdPreviouslyEnabled ;


#if DBG
VOID
KdpDprintf(
    IN PCHAR f,
    ...
    )
/*++

Routine Description:

    Printf routine for the debugger that is safer than DbgPrint.  Calls
    the packet driver instead of reentering the debugger.

Arguments:

    f - Supplies printf format

Return Value:

    None

--*/
{
    char    buf[100];
    STRING  Output;
    va_list mark;

    va_start(mark, f);
    _vsnprintf(buf, 100, f, mark);
    va_end(mark);

#ifdef _GAMBIT_
/*  Uncomment out to display DPRINT() output to GAMBIT console
    if (f) {

        PHYSICAL_ADDRESS pStringBuffer;

        // KdpDprintf() is unsafe to use while other KD command is in progress.
        // Send all DPRINT() output to Gambit console instead.

       pStringBuffer = MmGetPhysicalAddress (&buf);
       if (pStringBuffer.QuadPart != 0ULL) {
           SscDisplayString(pStringBuffer);
       }
    }
*/
#else
    Output.Buffer = buf;
    Output.Length = strlen(Output.Buffer);
    KdpPrintString(&Output);
#endif // _GAMBIT_
}
#endif // DBG


BOOLEAN
KdEnterDebugger(
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    )

/*++

Routine Description:

    This function is used to enter the kernel debugger. Its purpose
    is to freeze all other processors and aqcuire the kernel debugger
    comm port.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame that describes the
        trap.

    ExceptionFrame - Supplies a pointer to an exception frame that
        describes the trap.

Return Value:

    Returns the previous interrupt enable.

--*/

{

    BOOLEAN Enable;
    TIME_FIELDS TimeFields;
#if DBG
    extern ULONG KiFreezeFlag;
#endif

    //
    // HACKHACK - do some crude timer support
    //            but not if called from KdSetOwedBreakpoints()
    //

    if (TrapFrame) {
        KdTimerStop = KdpQueryPerformanceCounter (TrapFrame);
        KdTimerDifference.QuadPart = KdTimerStop.QuadPart - KdTimerStart.QuadPart;
    } else {
        KdTimerStop.QuadPart = 0;
    }

    //
    // Freeze all other processors, raise IRQL to HIGH_LEVEL, and save debug
    // port state.  We lock the port so that KdPollBreakin and a debugger
    // operation don't interfere with each other.
    //

    Enable = KeFreezeExecution(TrapFrame, ExceptionFrame);
    KdpPortLocked = KiTryToAcquireSpinLock(&KdpDebuggerLock);
    KdPortSave();
    KdEnteredDebugger = TRUE;

#if DBG

    if ((KiFreezeFlag & FREEZE_BACKUP) != 0) {
        DPRINT(("FreezeLock was jammed!  Backup SpinLock was used!\n"));
    }

    if ((KiFreezeFlag & FREEZE_SKIPPED_PROCESSOR) != 0) {
        DPRINT(("Some processors not frozen in debugger!\n"));
    }

    if (KdpPortLocked == FALSE) {
        DPRINT(("Port lock was not acquired!\n"));
    }

#endif

    return Enable;
}

VOID
KdExitDebugger(
    IN BOOLEAN Enable
    )

/*++

Routine Description:

    This function is used to exit the kernel debugger. It is the reverse
    of KdEnterDebugger.

Arguments:

    Enable - Supplies the previous interrupt enable which is to be restored.

Return Value:

    None.

--*/

{
    ULONG ElapsedTime;
    ULARGE_INTEGER TimeDifference;
    TIME_FIELDS TimeFields;
    ULONG Pending;

    //
    // restore stuff and exit
    //

    KdPortRestore();
    if (KdpPortLocked) {
        KdpPortUnlock();
    }

    KeThawExecution(Enable);

    //
    // Do some crude timer support.  If KdEnterDebugger didn't
    // Query the performance counter, then don't do it here either.
    //

    if (KdTimerStop.QuadPart == 0) {
        KdTimerStart = KdTimerStop;
    } else {
        KdTimerStart = KeQueryPerformanceCounter(NULL);
    }

    //
    // Process a time slip
    //

    if (!PoHiberInProgress) {

        Pending = InterlockedIncrement(&KdpTimeSlipPending);

        //
        // If there's wasn't a time slip pending, queue the DPC to handle it
        //

        if (Pending == 1) {
            InterlockedIncrement(&KdpTimeSlipPending);
            KeInsertQueueDpc(&KdpTimeSlipDpc, NULL, NULL);
        }
    }

    return;
}


VOID
KdUpdateTimeSlipEvent(
    PVOID Event
    )

/*++

Routine Description:

    Update the reference to an event object which will be signalled when
    the debugger has caused the system clock to skew.

Arguments:

    Event - Supplies a pointer to an event object

Return Value:

    None

--*/

{
    KIRQL OldIrql;

    KeAcquireSpinLock(&KdpTimeSlipEventLock, &OldIrql);

    //
    // Dereference the old event and forget about it.
    // Remember the new event if there is one.
    //

    if (KdpTimeSlipEvent != NULL) {
        ObDereferenceObject(KdpTimeSlipEvent);
    }

    KdpTimeSlipEvent = Event;

    KeReleaseSpinLock(&KdpTimeSlipEventLock, OldIrql);
}

VOID
KdpTimeSlipDpcRoutine (
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    )
{
    LONG OldCount, NewCount, j;

    //
    // Reset pending count.  If the current count is 1, then clear
    // the pending count.  if the current count is greater then 1,
    // then set to one and update the time now.
    //

    j = KdpTimeSlipPending;
    do {
        OldCount = j;
        NewCount = OldCount > 1 ? 1 : 0;

        j = InterlockedCompareExchange(&KdpTimeSlipPending, NewCount, OldCount);

    } while (j != OldCount);

    //
    // If new count is non-zero, then process a time slip now
    //

    if (NewCount) {
        ExQueueWorkItem(&KdpTimeSlipWorkItem, DelayedWorkQueue);
    }
}

VOID
KdpTimeSlipWork (
    IN PVOID Context
    )
{
    KIRQL               OldIrql;
    LARGE_INTEGER       DueTime;

    //
    // Update time from the real time clock
    //

    ExAcquireTimeRefreshLock();
    ExUpdateSystemTimeFromCmos (FALSE, 0);
    ExReleaseTimeRefreshLock();

    //
    // If there's a time service installed, signal it's time slip event
    //

    KeAcquireSpinLock(&KdpTimeSlipEventLock, &OldIrql);
    if (KdpTimeSlipEvent) {
        KeSetEvent (KdpTimeSlipEvent, 0, FALSE);
    }
    KeReleaseSpinLock(&KdpTimeSlipEventLock, OldIrql);

    //
    // Insert a forced delay between time slip operations
    //

    DueTime.QuadPart = -1800000000;
    KeSetTimer (&KdpTimeSlipTimer, DueTime, &KdpTimeSlipDpc);
}

#if i386
VOID
InternalBreakpointCheck (
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    )
{
    LARGE_INTEGER dueTime;
    ULONG i;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    dueTime.LowPart = (ULONG)(-1 * 10 * 1000 * 1000);
    dueTime.HighPart = -1;

    KeSetTimer(
        &InternalBreakpointTimer,
        dueTime,
        &InternalBreakpointCheckDpc
        );

    for ( i = 0 ; i < KdpNumInternalBreakpoints; i++ ) {
        if ( !(KdpInternalBPs[i].Flags & DBGKD_INTERNAL_BP_FLAG_INVALID) &&
             (KdpInternalBPs[i].Flags & DBGKD_INTERNAL_BP_FLAG_COUNTONLY) ) {

            PDBGKD_INTERNAL_BREAKPOINT b = KdpInternalBPs + i;
            ULONG callsThisPeriod;

            callsThisPeriod = b->Calls - b->CallsLastCheck;
            if ( callsThisPeriod > b->MaxCallsPerPeriod ) {
                b->MaxCallsPerPeriod = callsThisPeriod;
            }
            b->CallsLastCheck = b->Calls;
        }
    }

    return;

} // InternalBreakpointCheck


VOID
KdSetInternalBreakpoint (
    IN PDBGKD_MANIPULATE_STATE m
    )

/*++

Routine Description:

    This function sets an internal breakpoint.  "Internal breakpoint"
    means one in which control is not returned to the kernel debugger at
    all, but rather just update internal counting routines and resume.

Arguments:

    m - Supplies the state manipulation message.

Return Value:

    None.
--*/

{
    ULONG i;
    PDBGKD_INTERNAL_BREAKPOINT bp = NULL;
    ULONG savedFlags;

    for ( i = 0 ; i < KdpNumInternalBreakpoints; i++ ) {
        if ( KdpInternalBPs[i].Addr ==
                            m->u.SetInternalBreakpoint.BreakpointAddress ) {
            bp = &KdpInternalBPs[i];
            break;
        }
    }

    if ( !bp ) {
        for ( i = 0; i < KdpNumInternalBreakpoints; i++ ) {
            if ( KdpInternalBPs[i].Flags & DBGKD_INTERNAL_BP_FLAG_INVALID ) {
                bp = &KdpInternalBPs[i];
                break;
            }
        }
    }

    if ( !bp ) {
        if ( KdpNumInternalBreakpoints >= DBGKD_MAX_INTERNAL_BREAKPOINTS ) {
            return; // no space.  Probably should report error.
        }
        bp = &KdpInternalBPs[KdpNumInternalBreakpoints++];
        bp->Flags |= DBGKD_INTERNAL_BP_FLAG_INVALID; // force initialization
    }

    if ( bp->Flags & DBGKD_INTERNAL_BP_FLAG_INVALID ) {
        if ( m->u.SetInternalBreakpoint.Flags &
                                        DBGKD_INTERNAL_BP_FLAG_INVALID ) {
            return; // tried clearing a non-existant BP.  Ignore the request
        }
        bp->Calls = bp->MaxInstructions = bp->TotalInstructions = 0;
        bp->CallsLastCheck = bp->MaxCallsPerPeriod = 0;
        bp->MinInstructions = 0xffffffff;
        bp->Handle = 0;
        bp->Thread = 0;
    }

    savedFlags = bp->Flags;
    bp->Flags = m->u.SetInternalBreakpoint.Flags; // this could possibly invalidate the BP
    bp->Addr = m->u.SetInternalBreakpoint.BreakpointAddress;

    if ( bp->Flags & (DBGKD_INTERNAL_BP_FLAG_INVALID |
                      DBGKD_INTERNAL_BP_FLAG_SUSPENDED) ) {

        if ( (bp->Flags & DBGKD_INTERNAL_BP_FLAG_INVALID) &&
             (bp->Thread != 0) ) {
            // The breakpoint is active; defer its deletion
            bp->Flags &= ~DBGKD_INTERNAL_BP_FLAG_INVALID;
            bp->Flags |= DBGKD_INTERNAL_BP_FLAG_DYING;
        }

        // This is really a CLEAR bp request.

        if ( bp->Handle != 0 ) {
            KdpDeleteBreakpoint( bp->Handle );
        }
        bp->Handle = 0;

        return;
    }

    // now set the real breakpoint and remember its handle.

    if ( savedFlags & (DBGKD_INTERNAL_BP_FLAG_INVALID |
                       DBGKD_INTERNAL_BP_FLAG_SUSPENDED) ) {
        // breakpoint was invalid; activate it now
        bp->Handle = KdpAddBreakpoint( (PVOID)bp->Addr );
    }

    if ( BreakpointsSuspended ) {
        KdpSuspendBreakpoint( bp->Handle );
    }

} // KdSetInternalBreakpoint

NTSTATUS
KdGetTraceInformation(
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength
    )

/*++

Routine Description:

    This function gets data about an internal breakpoint and returns it
    in a buffer provided for it.  It is designed to be called from
    NTQuerySystemInformation.  It is morally equivalent to GetInternalBP
    except that it communicates locally, and returns all the breakpoints
    at once.

Arguments:

    SystemInforamtion - the buffer into which to write the result.
    SystemInformationLength - the maximum length to write
    RetrunLength - How much data was really written

Return Value:

    None.

--*/

{
    ULONG numEntries = 0;
    ULONG i = 0;
    PDBGKD_GET_INTERNAL_BREAKPOINT outPtr;

    for ( i = 0; i < KdpNumInternalBreakpoints; i++ ) {
        if ( !(KdpInternalBPs[i].Flags & DBGKD_INTERNAL_BP_FLAG_INVALID) ) {
            numEntries++;
        }
    }

    *ReturnLength = numEntries * sizeof(DBGKD_GET_INTERNAL_BREAKPOINT);
    if ( *ReturnLength > SystemInformationLength ) {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    //
    // We've got enough space.  Copy it in.
    //

    outPtr = (PDBGKD_GET_INTERNAL_BREAKPOINT)SystemInformation;
    for ( i = 0; i < KdpNumInternalBreakpoints; i++ ) {
        if ( !(KdpInternalBPs[i].Flags & DBGKD_INTERNAL_BP_FLAG_INVALID) ) {
            outPtr->BreakpointAddress = KdpInternalBPs[i].Addr;
            outPtr->Flags = KdpInternalBPs[i].Flags;
            outPtr->Calls = KdpInternalBPs[i].Calls;
            outPtr->MaxCallsPerPeriod = KdpInternalBPs[i].MaxCallsPerPeriod;
            outPtr->MinInstructions = KdpInternalBPs[i].MinInstructions;
            outPtr->MaxInstructions = KdpInternalBPs[i].MaxInstructions;
            outPtr->TotalInstructions = KdpInternalBPs[i].TotalInstructions;
            outPtr++;
        }
    }

    return STATUS_SUCCESS;

} // KdGetTraceInformation

VOID
KdGetInternalBreakpoint(
    IN PDBGKD_MANIPULATE_STATE m
    )

/*++

Routine Description:

    This function gets data about an internal breakpoint and returns it
    to the calling debugger.

Arguments:

    m - Supplies the state manipulation message.

Return Value:

    None.

--*/

{
    ULONG i;
    PDBGKD_INTERNAL_BREAKPOINT bp = NULL;
    STRING messageHeader;

    messageHeader.Length = sizeof(*m);
    messageHeader.Buffer = (PCHAR)m;

    for ( i = 0; i < KdpNumInternalBreakpoints; i++ ) {
        if ( !(KdpInternalBPs[i].Flags & (DBGKD_INTERNAL_BP_FLAG_INVALID |
                                          DBGKD_INTERNAL_BP_FLAG_SUSPENDED)) &&
             (KdpInternalBPs[i].Addr ==
                        m->u.GetInternalBreakpoint.BreakpointAddress) ) {
            bp = &KdpInternalBPs[i];
            break;
        }
    }

    if ( !bp ) {
        m->u.GetInternalBreakpoint.Flags = DBGKD_INTERNAL_BP_FLAG_INVALID;
        m->u.GetInternalBreakpoint.Calls = 0;
        m->u.GetInternalBreakpoint.MaxCallsPerPeriod = 0;
        m->u.GetInternalBreakpoint.MinInstructions = 0;
        m->u.GetInternalBreakpoint.MaxInstructions = 0;
        m->u.GetInternalBreakpoint.TotalInstructions = 0;
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
    } else {
        m->u.GetInternalBreakpoint.Flags = bp->Flags;
        m->u.GetInternalBreakpoint.Calls = bp->Calls;
        m->u.GetInternalBreakpoint.MaxCallsPerPeriod = bp->MaxCallsPerPeriod;
        m->u.GetInternalBreakpoint.MinInstructions = bp->MinInstructions;
        m->u.GetInternalBreakpoint.MaxInstructions = bp->MaxInstructions;
        m->u.GetInternalBreakpoint.TotalInstructions = bp->TotalInstructions;
        m->ReturnStatus = STATUS_SUCCESS;
    }

    m->ApiNumber = DbgKdGetInternalBreakPointApi;

    KdpSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                    &messageHeader,
                    NULL
                    );

    return;

} // KdGetInternalBreakpoint
#endif // i386

KCONTINUE_STATUS
KdpSendWaitContinue (
    IN ULONG OutPacketType,
    IN PSTRING OutMessageHeader,
    IN PSTRING OutMessageData OPTIONAL,
    IN OUT PCONTEXT ContextRecord
    )

/*++

Routine Description:

    This function sends a packet, and then waits for a continue message.
    BreakIns received while waiting will always cause a resend of the
    packet originally sent out.  While waiting, manipulate messages
    will be serviced.

    A resend always resends the original event sent to the debugger,
    not the last response to some debugger command.

Arguments:

    OutPacketType - Supplies the type of packet to send.

    OutMessageHeader - Supplies a pointer to a string descriptor that describes
        the message information.

    OutMessageData - Supplies a pointer to a string descriptor that describes
        the optional message data.

    ContextRecord - Exception context

Return Value:

    A value of TRUE is returned if the continue message indicates
    success, Otherwise, a value of FALSE is returned.

--*/

{

    ULONG Length;
    STRING MessageData;
    STRING MessageHeader;
    DBGKD_MANIPULATE_STATE ManipulateState;
    ULONG ReturnCode;
    NTSTATUS Status;
    KCONTINUE_STATUS ContinueStatus;

    //
    // Loop servicing state manipulation message until a continue message
    // is received.
    //

    MessageHeader.MaximumLength = sizeof(DBGKD_MANIPULATE_STATE);
    MessageHeader.Buffer = (PCHAR)&ManipulateState;
    MessageData.MaximumLength = KDP_MESSAGE_BUFFER_SIZE;
    MessageData.Buffer = (PCHAR)KdpMessageBuffer;

ResendPacket:

    //
    // Send event notification packet to debugger on host.  Come back
    // here any time we see a breakin sequence.
    //

    KdpSendPacket(
                  OutPacketType,
                  OutMessageHeader,
                  OutMessageData
                  );

    //
    // After sending packet, if there is no response from debugger
    // AND the packet is for reporting symbol (un)load, the debugger
    // will be declared to be not present.  Note If the packet is for
    // reporting exception, the KdpSendPacket will never stop.
    //

    if (KdDebuggerNotPresent) {
        return ContinueSuccess;
    }

    while (TRUE) {

        //
        // Wait for State Manipulate Packet without timeout.
        //

        do {

            ReturnCode = KdpReceivePacket(
                            PACKET_TYPE_KD_STATE_MANIPULATE,
                            &MessageHeader,
                            &MessageData,
                            &Length
                            );
            if (ReturnCode == (USHORT)KDP_PACKET_RESEND) {
                goto ResendPacket;
            }
        } while (ReturnCode == KDP_PACKET_TIMEOUT);

        //
        // Switch on the return message API number.
        //

        switch (ManipulateState.ApiNumber) {

        case DbgKdReadVirtualMemoryApi:
            KdpReadVirtualMemory(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdReadVirtualMemory64Api:
            KdpReadVirtualMemory64(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdWriteVirtualMemoryApi:
            KdpWriteVirtualMemory(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdWriteVirtualMemory64Api:
            KdpWriteVirtualMemory64(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdReadPhysicalMemoryApi:
            KdpReadPhysicalMemory(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdWritePhysicalMemoryApi:
            KdpWritePhysicalMemory(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdGetContextApi:
            KdpGetContext(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdSetContextApi:
            KdpSetContext(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdWriteBreakPointApi:
            KdpWriteBreakpoint(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdRestoreBreakPointApi:
            KdpRestoreBreakpoint(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdReadControlSpaceApi:
            KdpReadControlSpace(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdWriteControlSpaceApi:
            KdpWriteControlSpace(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdReadIoSpaceApi:
            KdpReadIoSpace(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdWriteIoSpaceApi:
            KdpWriteIoSpace(&ManipulateState,&MessageData,ContextRecord);
            break;

#ifdef _ALPHA_

        case DbgKdReadIoSpaceExtendedApi:
            KdpReadIoSpaceExtended(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdWriteIoSpaceExtendedApi:
            KdpWriteIoSpaceExtended(&ManipulateState,&MessageData,ContextRecord);
            break;

#endif // _ALPHA_

        case DbgKdContinueApi:
            if (NT_SUCCESS(ManipulateState.u.Continue.ContinueStatus) != FALSE) {
                return ContinueSuccess;
            } else {
                return ContinueError;
            }
            break;

        case DbgKdContinueApi2:
            if (NT_SUCCESS(ManipulateState.u.Continue2.ContinueStatus) != FALSE) {
                KdpGetStateChange(&ManipulateState,ContextRecord);
                return ContinueSuccess;
            } else {
                return ContinueError;
            }
            break;

        case DbgKdRebootApi:
            KdpReboot();
            break;

#if i386
        case DbgKdReadMachineSpecificRegister:
            KdpReadMachineSpecificRegister(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdWriteMachineSpecificRegister:
            KdpWriteMachineSpecificRegister(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdSetSpecialCallApi:
            KdSetSpecialCall(&ManipulateState,ContextRecord);
            break;

        case DbgKdClearSpecialCallsApi:
            KdClearSpecialCalls();
            break;

        case DbgKdSetInternalBreakPointApi:
            KdSetInternalBreakpoint(&ManipulateState);
            break;

        case DbgKdGetInternalBreakPointApi:
            KdGetInternalBreakpoint(&ManipulateState);
            break;
#endif

        case DbgKdGetVersionApi:
            KdpGetVersion(&ManipulateState);
            break;

        case DbgKdCauseBugCheckApi:
            KdpCauseBugCheck(&ManipulateState);
            break;

        case DbgKdPageInApi:
            KdpNotSupported(&ManipulateState);
            break;

        case DbgKdWriteBreakPointExApi:
            Status = KdpWriteBreakPointEx(&ManipulateState,
                                          &MessageData,
                                          ContextRecord);
            if (Status) {
                ManipulateState.ApiNumber = DbgKdContinueApi;
                ManipulateState.u.Continue.ContinueStatus = Status;
                return ContinueError;
            }
            break;

        case DbgKdRestoreBreakPointExApi:
            KdpRestoreBreakPointEx(&ManipulateState,&MessageData,ContextRecord);
            break;

        case DbgKdSwitchProcessor:
            KdPortRestore ();
            ContinueStatus = KeSwitchFrozenProcessor(ManipulateState.Processor);
            KdPortSave ();
            return ContinueStatus;

        case DbgKdSearchMemoryApi:
            KdpSearchMemory(&ManipulateState, &MessageData, ContextRecord);
            break;

            //
            // Invalid message.
            //

        default:
            MessageData.Length = 0;
            ManipulateState.ReturnStatus = STATUS_UNSUCCESSFUL;
            KdpSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &MessageHeader, &MessageData);
            break;
        }

#ifdef _ALPHA_

        //
        //jnfix
        // this is embarrasing, we have an icache coherency problem that
        // the following imb fixes, later we must track this down to the
        // exact offending API but for now this statement allows the stub
        // work to appropriately for Alpha.
        //

#if defined(_MSC_VER)
        __PAL_IMB();
#else
        asm( "call_pal 0x86" );   // x86 = imb
#endif

#endif

    }
}

VOID
KdpReadVirtualMemory(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response to a read virtual memory 32-bit
    state manipulation message. Its function is to read virtual memory
    and return.

Arguments:

    m - Supplies a pointer to the state manipulation message.

    AdditionalData - Supplies a pointer to a descriptor for the data to read.

    Context - Supplies a pointer to the current context.

Return Value:

    None.

--*/

{

    ULONG Length;
    STRING MessageHeader;

    //
    // Trim the transfer count to fit in a single message.
    //

    Length = m->u.ReadMemory.TransferCount;
    if (Length > (PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE))) {
        Length = PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE);
    }

    //
    // Move the data to the destination buffer.
    //

    AdditionalData->Length = (USHORT)KdpMoveMemory(AdditionalData->Buffer,
                                                   m->u.ReadMemory.TargetBaseAddress,
                                                   Length);

    //
    // If all the data is read, then return a success status. Otherwise,
    // return unsuccessful.
    //

    m->ReturnStatus = STATUS_SUCCESS;
    if (Length != AdditionalData->Length) {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
    }

    //
    // Set the actual number of bytes read, initialize the message header,
    // and send the reply packet to the host debugger.
    //

    m->u.ReadMemory.ActualBytesRead = AdditionalData->Length;
    MessageHeader.Length = sizeof(DBGKD_MANIPULATE_STATE);
    MessageHeader.Buffer = (PCHAR)m;
    KdpSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                  &MessageHeader,
                  AdditionalData);

    return;
}

VOID
KdpReadVirtualMemory64(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response of a read virtual memory 64-bit
    state manipulation message. Its function is to read virtual memory
    and return.

Arguments:

    m - Supplies a pointer to a state manipulation message.

    AdditionalData - Supplies a pointer to descriptor for the data to read.

    Context - Supplies a pointer to the current context.

Return Value:

    None.

--*/

{

    UCHAR * POINTER_64 Address;
    PUCHAR Destination;
    ULONG Length;
    STRING MessageHeader;
    UCHAR * POINTER_64 Source;

    //
    // Trim the transfer count to fit in a single message.
    //

    Length = m->u.ReadMemory64.TransferCount;
    if (Length > (PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE))) {
        Length = PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE);
    }

    //
    // Move the data to the destination buffer.
    //

    AdditionalData->Length = (USHORT)Length;

#if defined(_MIPS_) || defined(_ALPHA_)

    Destination = AdditionalData->Buffer;
    Source = (UCHAR * POINTER_64)m->u.ReadMemory64.TargetBaseAddress;
    while (Length > 0) {
        if ((Address = MmDbgReadCheck64(Source)) == NULL64) {
            break;
        }

        *Destination++ = *Address;
        Source += 1;
        Length -= 1;
    }

#endif

    //
    // If all the data is read, then return a success status. Otherwise,
    // return an unsuccessful status.
    //

    m->ReturnStatus = STATUS_SUCCESS;
    if (Length != 0) {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
    }

    //
    // Set the actual number of bytes read, initialize the message header,
    // and send the reply packet to the host debugger.
    //

    m->u.ReadMemory64.ActualBytesRead = AdditionalData->Length - Length;
    MessageHeader.Length = sizeof(DBGKD_MANIPULATE_STATE);
    MessageHeader.Buffer = (PCHAR)m;
    KdpSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                  &MessageHeader,
                  AdditionalData);

    return;
}

VOID
KdpWriteVirtualMemory(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response of a write virtual memory 32-bit
    state manipulation message. Its function is to write virtual memory
    and return.

Arguments:

    m - Supplies a pointer to the state manipulation message.

    AdditionalData - Supplies a pointer to a descriptor for the data to write.

    Context - Supplies a pointer to the current context.

Return Value:

    None.

--*/

{

    ULONG Length;
    STRING MessageHeader;

    //
    // Move the data to the destination buffer.
    //

    Length = KdpMoveMemory(m->u.WriteMemory.TargetBaseAddress,
                           AdditionalData->Buffer,
                           AdditionalData->Length);

    //
    // If all the data is written, then return a success status. Otherwise,
    // return an unsuccessful status.
    //

    m->ReturnStatus = STATUS_SUCCESS;
    if (Length != AdditionalData->Length) {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
    }

    //
    // Set the actual number of bytes written, initialize the message header,
    // and send the reply packet to the host debugger.
    //

    m->u.WriteMemory.ActualBytesWritten = Length;
    MessageHeader.Length = sizeof(DBGKD_MANIPULATE_STATE);
    MessageHeader.Buffer = (PCHAR)m;
    KdpSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                  &MessageHeader,
                  NULL);

    return;
}

VOID
KdpWriteVirtualMemory64(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response of a write virtual memory 64-bit
    state manipulation message. Its function is to write virtual memory
    and return.

Arguments:

    m - Supplies a pointer to the state manipulation message.

    AdditionalData - Supplies a pointer to a descriptor for the data to write.

    Context - Supplies a pointer to the current context.

Return Value:

    None.

--*/

{

    UCHAR * POINTER_64 Address;
    UCHAR * POINTER_64 Destination;
    ULONG Length;
    STRING MessageHeader;
    PUCHAR Source;
    ULONG_PTR Opaque;

    //
    // Move the data to the destination buffer.
    //

    Length = AdditionalData->Length;

#if defined(_MIPS_) || defined(_ALPHA_)

    Destination = (UCHAR * POINTER_64)m->u.WriteMemory64.TargetBaseAddress;
    Source = AdditionalData->Buffer;
    while (Length > 0) {
        if ((Address = MmDbgWriteCheck64(Destination)) == NULL64) {
            break;
        }

        *Address = *Source++;
        Destination += 1;
        Length -= 1;
    }

#endif

    //
    // If all the data is written, then return a success status. Otherwise,
    // return an unsuccessful status.
    //

    m->ReturnStatus = STATUS_SUCCESS;
    if (Length != 0) {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
    }

    //
    // Set the actual number of bytes written, initialize the message header,
    // and send the reply packet to the host debugger.
    //

    m->u.WriteMemory64.ActualBytesWritten = AdditionalData->Length - Length;
    MessageHeader.Length = sizeof(DBGKD_MANIPULATE_STATE);
    MessageHeader.Buffer = (PCHAR)m;
    KdpSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                  &MessageHeader,
                  NULL);

    return;
}


VOID
KdpGetContext(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response of a get context state
    manipulation message.  Its function is to return the current
    context.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    PDBGKD_GET_CONTEXT a = &m->u.GetContext;
    STRING MessageHeader;

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    ASSERT(AdditionalData->Length == 0);

    if (m->Processor >= (USHORT)KeNumberProcessors) {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
    } else {
        m->ReturnStatus = STATUS_SUCCESS;
        AdditionalData->Length = sizeof(CONTEXT);
        if (m->Processor == (USHORT)KeGetCurrentPrcb()->Number) {
            KdpQuickMoveMemory(AdditionalData->Buffer, (PCHAR)Context, sizeof(CONTEXT));
        } else {
            KdpQuickMoveMemory(AdditionalData->Buffer,
                          (PCHAR)&KiProcessorBlock[m->Processor]->ProcessorState.ContextFrame,
                          sizeof(CONTEXT)
                         );
        }
    }

    KdpSendPacket(
                  PACKET_TYPE_KD_STATE_MANIPULATE,
                  &MessageHeader,
                  AdditionalData
                  );
}

VOID
KdpSetContext(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response of a set context state
    manipulation message.  Its function is set the current
    context.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    PDBGKD_SET_CONTEXT a = &m->u.SetContext;
    STRING MessageHeader;

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    ASSERT(AdditionalData->Length == sizeof(CONTEXT));

    if (m->Processor >= (USHORT)KeNumberProcessors) {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
    } else {
        m->ReturnStatus = STATUS_SUCCESS;
        if (m->Processor == (USHORT)KeGetCurrentPrcb()->Number) {
            KdpQuickMoveMemory((PCHAR)Context, AdditionalData->Buffer, sizeof(CONTEXT));
        } else {
            KdpQuickMoveMemory((PCHAR)&KiProcessorBlock[m->Processor]->ProcessorState.ContextFrame,
                          AdditionalData->Buffer,
                          sizeof(CONTEXT)
                         );
        }
    }

    KdpSendPacket(
                  PACKET_TYPE_KD_STATE_MANIPULATE,
                  &MessageHeader,
                  NULL
                  );
}

VOID
KdpWriteBreakpoint(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response of a write breakpoint state
    manipulation message.  Its function is to write a breakpoint
    and return a handle to the breakpoint.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    PDBGKD_WRITE_BREAKPOINT a = &m->u.WriteBreakPoint;
    STRING MessageHeader;

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    ASSERT(AdditionalData->Length == 0);

    a->BreakPointHandle = KdpAddBreakpoint(a->BreakPointAddress);
    if (a->BreakPointHandle != 0) {
        m->ReturnStatus = STATUS_SUCCESS;
    } else {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
    }
    KdpSendPacket(
                  PACKET_TYPE_KD_STATE_MANIPULATE,
                  &MessageHeader,
                  NULL
                  );
    UNREFERENCED_PARAMETER(Context);
}

VOID
KdpRestoreBreakpoint(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response of a restore breakpoint state
    manipulation message.  Its function is to restore a breakpoint
    using the specified handle.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    PDBGKD_RESTORE_BREAKPOINT a = &m->u.RestoreBreakPoint;
    STRING MessageHeader;

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    ASSERT(AdditionalData->Length == 0);
    if (KdpDeleteBreakpoint(a->BreakPointHandle)) {
        m->ReturnStatus = STATUS_SUCCESS;
    } else {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
    }
    KdpSendPacket(
                  PACKET_TYPE_KD_STATE_MANIPULATE,
                  &MessageHeader,
                  NULL
                  );
    UNREFERENCED_PARAMETER(Context);
}

#if i386
long
SymNumFor(
    ULONG pc
    )
{
    ULONG index;

    for (index = 0; index < NumTraceDataSyms; index++) {
        if ((TraceDataSyms[index].SymMin <= pc) &&
            (TraceDataSyms[index].SymMax > pc)) return(index);
    }
    return(-1);
}

BOOLEAN TraceDataBufferFilled = FALSE;

void PotentialNewSymbol (ULONG pc)
{
    if (!TraceDataBufferFilled &&
        -1 != SymNumFor(pc)) {     // we've already seen this one
        return;
    }

    TraceDataBufferFilled = FALSE;

    // OK, we've got to start up a TraceDataRecord
    TraceDataBuffer[TraceDataBufferPosition].s.LevelChange = 0;

    if (-1 != SymNumFor(pc)) {
        int sym = SymNumFor(pc);
        TraceDataBuffer[TraceDataBufferPosition].s.SymbolNumber = (UCHAR) sym;
        KdpCurrentSymbolStart = TraceDataSyms[sym].SymMin;
        KdpCurrentSymbolEnd = TraceDataSyms[sym].SymMax;

        return;  // we've already seen this one
    }

    TraceDataSyms[NextTraceDataSym].SymMin = KdpCurrentSymbolStart;
    TraceDataSyms[NextTraceDataSym].SymMax = KdpCurrentSymbolEnd;

    TraceDataBuffer[TraceDataBufferPosition].s.SymbolNumber = NextTraceDataSym;

    // Bump the "next" pointer, wrapping if necessary.  Also bump the
    // "valid" pointer if we need to.
    NextTraceDataSym = (NextTraceDataSym + 1) % 256;
    if (NumTraceDataSyms < NextTraceDataSym) {
        NumTraceDataSyms = NextTraceDataSym;
    }

}

void DumpTraceData(PSTRING MessageData)
{

 TraceDataBuffer[0].LongNumber = TraceDataBufferPosition;
 MessageData->Length = (USHORT)(sizeof(TraceDataBuffer[0]) * TraceDataBufferPosition);
 MessageData->Buffer = (PVOID)TraceDataBuffer;
 TraceDataBufferPosition = 1;
}

BOOLEAN
TraceDataRecordCallInfo(
    ULONG InstructionsTraced,
    LONG CallLevelChange,
    ULONG pc
    )
{
    // We've just exited a symbol scope.  The InstructionsTraced number goes
    // with the old scope, the CallLevelChange goes with the new, and the
    // pc fills in the symbol for the new TraceData record.

    long SymNum = SymNumFor(pc);

    if (KdpNextCallLevelChange != 0) {
        TraceDataBuffer[TraceDataBufferPosition].s.LevelChange =
                                                (char) KdpNextCallLevelChange;
        KdpNextCallLevelChange = 0;
    }


    if (InstructionsTraced >= TRACE_DATA_INSTRUCTIONS_BIG) {
       TraceDataBuffer[TraceDataBufferPosition].s.Instructions =
           TRACE_DATA_INSTRUCTIONS_BIG;
       TraceDataBuffer[TraceDataBufferPosition+1].LongNumber =
           InstructionsTraced;
       TraceDataBufferPosition += 2;
    } else {
       TraceDataBuffer[TraceDataBufferPosition].s.Instructions =
           (unsigned short)InstructionsTraced;
       TraceDataBufferPosition++;
    }

    if ((TraceDataBufferPosition + 2 >= TRACE_DATA_BUFFER_MAX_SIZE) ||
        (-1 == SymNum)) {
        if (TraceDataBufferPosition +2 >= TRACE_DATA_BUFFER_MAX_SIZE) {
            TraceDataBufferFilled = TRUE;
        }
       KdpNextCallLevelChange = CallLevelChange;
       return FALSE;
    }

    TraceDataBuffer[TraceDataBufferPosition].s.LevelChange =(char)CallLevelChange;
    TraceDataBuffer[TraceDataBufferPosition].s.SymbolNumber = (UCHAR) SymNum;
    KdpCurrentSymbolStart = TraceDataSyms[SymNum].SymMin;
    KdpCurrentSymbolEnd = TraceDataSyms[SymNum].SymMax;

    return TRUE;
}

BOOLEAN
SkippingWhichBP (
    PVOID thread,
    PULONG BPNum
    )

/*
 * Return TRUE iff the pc corresponds to an internal breakpoint
 * that has just been replaced for execution.  If TRUE, then return
 * the breakpoint number in BPNum.
 */

{
    ULONG index;

    if (!IntBPsSkipping) return FALSE;

    for (index = 0; index < KdpNumInternalBreakpoints; index++) {
        if (!(KdpInternalBPs[index].Flags & DBGKD_INTERNAL_BP_FLAG_INVALID) &&
            (KdpInternalBPs[index].Thread == thread)) {
            *BPNum = index;
            return TRUE;
        }
    }
    return FALSE; // didn't match any
}


NTSTATUS
KdQuerySpecialCalls (
    IN PDBGKD_MANIPULATE_STATE m,
    ULONG Length,
    PULONG RequiredLength
    )
{
    *RequiredLength = sizeof(DBGKD_MANIPULATE_STATE) +
                        (sizeof(ULONG) * KdNumberOfSpecialCalls);

    if ( Length < *RequiredLength ) {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    m->u.QuerySpecialCalls.NumberOfSpecialCalls = KdNumberOfSpecialCalls;
        RtlCopyMemory(
        m + 1,
        KdSpecialCalls,
        sizeof(ULONG) * KdNumberOfSpecialCalls
        );

    return STATUS_SUCCESS;

} // KdQuerySpecialCalls


VOID
KdSetSpecialCall (
    IN PDBGKD_MANIPULATE_STATE m,
    IN PCONTEXT ContextRecord
    )

/*++

Routine Description:

    This function sets the addresses of the "special" call addresses
    that the watchtrace facility pushes back to the kernel debugger
    rather than stepping through.

Arguments:

    m - Supplies the state manipulation message.

Return Value:

    None.
--*/

{
    if ( KdNumberOfSpecialCalls >= DBGKD_MAX_SPECIAL_CALLS ) {
        return; // too bad
    }

    KdSpecialCalls[KdNumberOfSpecialCalls++] = m->u.SetSpecialCall.SpecialCall;

    NextTraceDataSym = 0;
    NumTraceDataSyms = 0;
    KdpNextCallLevelChange = 0;
    if (ContextRecord && !InstrCountInternal) {
        InitialSP = ContextRecord->Esp;
    }

} // KdSetSpecialCall


VOID
KdClearSpecialCalls (
    VOID
    )

/*++

Routine Description:

    This function clears the addresses of the "special" call addresses
    that the watchtrace facility pushes back to the kernel debugger
    rather than stepping through.

Arguments:

    None.

Return Value:

    None.

--*/

{
    KdNumberOfSpecialCalls = 0;
    return;

} // KdClearSpecialCalls


BOOLEAN
KdpCheckTracePoint(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PCONTEXT ContextRecord
    )
{
    ULONG pc = (ULONG)CONTEXT_TO_PROGRAM_COUNTER(ContextRecord);
    LONG BpNum;
    ULONG SkippedBPNum;
    BOOLEAN AfterSC = FALSE;

    if (ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP) {
        if (WatchStepOverSuspended) {
            //
            //  For background, see the comment below where WSOThread is
            //  wrong.  We've now stepped over the breakpoint in the non-traced
            //  thread, and need to replace it and restart the non-traced
            //  thread at full speed.
            //

            WatchStepOverHandle = KdpAddBreakpoint((PVOID)WatchStepOverBreakAddr);
            WatchStepOverSuspended = FALSE;
            ContextRecord->EFlags &= ~0x100L; /* clear trace flag */
            return TRUE; // resume non-traced thread at full speed
        }

        if ((!SymbolRecorded) && (KdpCurrentSymbolStart != 0) && (KdpCurrentSymbolEnd != 0)) {
            //
            //  We need to use oldpc here, because this may have been
            //  a 1 instruction call.  We've ALREADY executed the instruction
            //  that the new symbol is for, and if the pc has moved out of
            //  range, we might screw up.  Hence, use the pc from when
            //  SymbolRecorded was set.  Yuck.
            //

            PotentialNewSymbol(oldpc);
            SymbolRecorded = TRUE;
        }

        if (!InstrCountInternal &&
            SkippingWhichBP((PVOID)KeGetCurrentThread(),&SkippedBPNum)) {

            //
            //  We just single-stepped over a temporarily removed internal
            //  breakpoint.
            //  If it's a COUNTONLY breakpoint:
            //      Put the breakpoint instruction back and resume
            //      regular execution.
            //

            if (KdpInternalBPs[SkippedBPNum].Flags &
                DBGKD_INTERNAL_BP_FLAG_COUNTONLY) {

                IntBPsSkipping --;

                KdpRestoreAllBreakpoints();

                ContextRecord->EFlags &= ~0x100L;  // Clear trace flag
                KdpInternalBPs[SkippedBPNum].Thread = 0;

                if (KdpInternalBPs[SkippedBPNum].Flags &
                        DBGKD_INTERNAL_BP_FLAG_DYING) {
                    KdpDeleteBreakpoint(KdpInternalBPs[SkippedBPNum].Handle);
                    KdpInternalBPs[SkippedBPNum].Flags |=
                            DBGKD_INTERNAL_BP_FLAG_INVALID; // bye, bye
                }

                return TRUE;
            }

            //
            //  If it's not:
            //      set up like it's a ww, by setting Begin and KdpCurrentSymbolEnd
            //      and bop off into single step land.  We probably ought to
            //      disable all breakpoints here, too, so that we don't do
            //      anything foul like trying two non-COUNTONLY's at the
            //      same time or something...
            //

            KdpCurrentSymbolEnd = 0;
            KdpCurrentSymbolStart = KdpInternalBPs[SkippedBPNum].ReturnAddress;

            ContextRecord->EFlags |= 0x100L; /* Trace on. */
            InitialSP = ContextRecord->Esp;

            InstructionsTraced = 1;  /* Count the initial call instruction. */
            InstrCountInternal = TRUE;
        }

    } /* if single step */
    else if (ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) {
        if (WatchStepOver && pc == WatchStepOverBreakAddr) {
            //
            //  This is a breakpoint after completion of a "special call"
            //

            if ((WSOThread != (PVOID)KeGetCurrentThread()) ||
                (WSOEsp + 0x20 < ContextRecord->Esp) ||
                (ContextRecord->Esp + 0x20 < WSOEsp)) {
                //
                //  Here's the story up to this point: the traced thread
                //  cruised along until it it a special call.  The tracer
                //  placed a breakpoint on the instruction immediately after
                //  the special call returns and restarted the traced thread
                //  at full speed.  Then, some *other* thread hit the
                //  breakpoint.  So, to correct for this, we're going to
                //  remove the breakpoint, single step the non-traced
                //  thread one instruction, replace the breakpoint,
                //  restart the non-traced thread at full speed, and wait
                //  for the traced thread to get to this breakpoint, just
                //  like we were when this happened.  The assumption
                //  here is that the traced thread won't hit the breakpoint
                //  while it's removed, which I believe to be true, because
                //  I don't think a context switch can occur during a single
                //  step operation.
                //
                //  For extra added fun, it's possible to execute interrupt
                //  routines IN THE SAME THREAD!!!  That's why we need to keep
                //  the stack pointer as well as the thread address: the APC
                //  code can result in pushing on the stack and doing a call
                //  that's really part on an interrupt service routine in the
                //  context of the current thread.  Lovely, isn't it?
                //

                WatchStepOverSuspended = TRUE;
                KdpDeleteBreakpoint(WatchStepOverHandle);
                ContextRecord->EFlags |= 0x100L; // Set trace flag
                return TRUE; // single step "non-traced" thread
            }

            //
            //  we're in the thread we started in; resume in single-step mode
            //  to continue the trace.
            //

            WatchStepOver = FALSE;
            KdpDeleteBreakpoint(WatchStepOverHandle);
            ContextRecord->EFlags |= 0x100L; // back to single step mode
            AfterSC = TRUE; // put us into the regular watchStep code

        } else {

            for ( BpNum = 0; BpNum < (LONG) KdpNumInternalBreakpoints; BpNum++ ) {
                if ( !(KdpInternalBPs[BpNum].Flags &
                       (DBGKD_INTERNAL_BP_FLAG_INVALID |
                        DBGKD_INTERNAL_BP_FLAG_SUSPENDED) ) &&
                     (KdpInternalBPs[BpNum].Addr == pc) ) {
                    break;
                }
            }

            if ( BpNum < (LONG) KdpNumInternalBreakpoints ) {

                //
                //  This is an internal monitoring breakpoint.
                //  Restore the instruction and start in single-step
                //  mode so that we can retore the breakpoint once the
                //  instruction executes, or continue stepping if this isn't
                //  a COUNTONLY breakpoint.
                //

                KdpProcessInternalBreakpoint( BpNum );
                KdpInternalBPs[BpNum].Thread = (PVOID)KeGetCurrentThread();
                IntBPsSkipping ++;

                KdpSuspendAllBreakpoints();

                ContextRecord->EFlags |= 0x100L;  // Set trace flag
                if (!(KdpInternalBPs[BpNum].Flags &
                        DBGKD_INTERNAL_BP_FLAG_COUNTONLY)) {
                    KdpInternalBPs[BpNum].ReturnAddress =
                                    KdpGetReturnAddress( ContextRecord );
                }
                return TRUE;
            }
        }
    } /* if breakpoint */

//  if (AfterSC) {
//      DPRINT(( "1: KdpCurrentSymbolStart %x  KdpCurrentSymbolEnd %x\n", KdpCurrentSymbolStart, KdpCurrentSymbolEnd ));
//  }

    if ((AfterSC || ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP) &&
        KdpCurrentSymbolStart != 0 &&
        ((KdpCurrentSymbolEnd == 0 && ContextRecord->Esp <= InitialSP) ||
         (KdpCurrentSymbolStart <= pc && pc < KdpCurrentSymbolEnd))) {
        ULONG lc;
        BOOLEAN IsSpecialCall;

        //
        //  We've taken a step trace, but are still executing in the current
        //  function.  Remember that we executed an instruction and see if the
        //  instruction changes the call level.
        //

        lc = KdpLevelChange( pc, ContextRecord, &IsSpecialCall );
        InstructionsTraced++;
        CallLevelChange += lc;

        //
        //  See if instruction is a transfer to a special routine, one that we
        //  cannot trace through since it may swap contexts
        //

        if (IsSpecialCall) {

//  DPRINT( ("2: pc=%x, level change %d\n", pc, lc) );

            //
            //  We are about to transfer to a special call routine.  Since we
            //  cannot trace through this routine, we execute it atomically by
            //  setting a breakpoint at the next logical offset.
            //
            //  Note in the case of an indirect jump to a special call routine, the
            //  level change will be -1 and the next offset will be the ULONG that's
            //  on the top of the stack.
            //
            //  However, we've already adjusted the level based on this
            //  instruction.  We need to undo this except for the magic -1 call.
            //

            if (lc != -1) {
                CallLevelChange -= lc;
            }

            //
            //  Set up for stepping over a procedure
            //

            WatchStepOver = TRUE;
            WatchStepOverBreakAddr = KdpGetCallNextOffset( pc, ContextRecord );
            WSOThread = (PVOID)KeGetCurrentThread( );
            WSOEsp = ContextRecord->Esp;

            //
            //  Establish the breakpoint
            //

            WatchStepOverHandle = KdpAddBreakpoint( (PVOID)WatchStepOverBreakAddr );


            //
            //  Note that we are continuing rather than tracing and rely on hitting
            //  the breakpoint in the current thread context to resume the watch
            //  action.
            //

            ContextRecord->EFlags &= ~0x100L;
            return TRUE;
        }

        //
        //  Resume execution with the trace flag set.  Avoid going over the wire to
        //  the remote debugger.
        //

        ContextRecord->EFlags |= 0x100L;  // Set trace flag

        return TRUE;
    }

    if ((AfterSC || (ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP)) &&
        (KdpCurrentSymbolStart != 0)) {
        //
        // We're WatchTracing, but have just changed symbol range.
        // Fill in the call record and return to the debugger if
        // either we're full or the pc is outside of the known
        // symbol scopes.  Otherwise, resume stepping.
        //
        int lc;
        BOOLEAN IsSpecialCall;

        InstructionsTraced++; // don't forget to count the call/ret instruction.

//  if (AfterSC) {
//      DPRINT(( "3: InstrCountInternal: %x\n", InstrCountInternal ));
//  }

        if (InstrCountInternal) {

            // We've just finished processing a non-COUNTONLY breakpoint.
            // Record the appropriate data and resume full speed execution.

            SkippingWhichBP((PVOID)KeGetCurrentThread(),&SkippedBPNum);

            KdpInternalBPs[SkippedBPNum].Calls++;


            if (KdpInternalBPs[SkippedBPNum].MinInstructions > InstructionsTraced) {
                KdpInternalBPs[SkippedBPNum].MinInstructions = InstructionsTraced;
            }
            if (KdpInternalBPs[SkippedBPNum].MaxInstructions < InstructionsTraced) {
                KdpInternalBPs[SkippedBPNum].MaxInstructions = InstructionsTraced;
            }
            KdpInternalBPs[SkippedBPNum].TotalInstructions += InstructionsTraced;

            KdpInternalBPs[SkippedBPNum].Thread = 0;

            IntBPsSkipping--;
            InstrCountInternal = FALSE;
            KdpCurrentSymbolStart = 0;
            KdpRestoreAllBreakpoints();

            if (KdpInternalBPs[SkippedBPNum].Flags &
                    DBGKD_INTERNAL_BP_FLAG_DYING) {
                KdpDeleteBreakpoint(KdpInternalBPs[SkippedBPNum].Handle);
                KdpInternalBPs[SkippedBPNum].Flags |=
                        DBGKD_INTERNAL_BP_FLAG_INVALID; // bye, bye
            }

            ContextRecord->EFlags &= ~0x100L; // clear trace flag
            return TRUE; // Back to normal execution.
        }

        if (TraceDataRecordCallInfo( InstructionsTraced, CallLevelChange, pc)) {

            //
            //  Everything was cool internally.  We can keep executing without
            //  going back to the remote debugger.
            //
            //  We have to compute lc after calling
            //  TraceDataRecordCallInfo, because LevelChange relies on
            //  KdpCurrentSymbolStart and KdpCurrentSymbolEnd corresponding to
            //  the pc.
            //

            lc = KdpLevelChange( pc, ContextRecord, &IsSpecialCall );
            InstructionsTraced = 0;
            CallLevelChange = lc;

            //
            //  See if instruction is a transfer to a special routine, one that we
            //  cannot trace through since it may swap contexts
            //

            if (IsSpecialCall) {

//  DPRINT(( "4: pc=%x, level change %d\n", pc, lc));

                //
                //  We are about to transfer to a special call routine.  Since we
                //  cannot trace through this routine, we execute it atomically by
                //  setting a breakpoint at the next logical offset.
                //
                //  Note in the case of an indirect jump to a special call routine, the
                //  level change will be -1 and the next offset will be the ULONG that's
                //  on the top of the stack.
                //
                //  However, we've already adjusted the level based on this
                //  instruction.  We need to undo this except for the magic -1 call.
                //

                if (lc != -1) {
                    CallLevelChange -= lc;
                }

                //
                //  Set up for stepping over a procedure
                //

                WatchStepOver = TRUE;
                WSOThread = (PVOID)KeGetCurrentThread();

                //
                //  Establish the breakpoint
                //

                WatchStepOverHandle =
                    KdpAddBreakpoint( (PVOID)KdpGetCallNextOffset( pc, ContextRecord ));

                //
                //  Resume execution with the trace flag set.  Avoid going over the wire to
                //  the remote debugger.
                //

                ContextRecord->EFlags &= ~0x100L;
                return TRUE;
            }

            ContextRecord->EFlags |= 0x100L; // Set trace flag
            return TRUE; // Off we go
        }

        lc = KdpLevelChange( pc, ContextRecord, &IsSpecialCall );
        InstructionsTraced = 0;
        CallLevelChange = lc;

        // We need to go back to the remote debugger.  Just fall through.

        if ((lc != 0) && IsSpecialCall) {
            // We're hosed
            DPRINT(( "Special call on first entry to symbol scope @ %x\n", pc ));
        }
    }

    SymbolRecorded = FALSE;
    oldpc = pc;

    return FALSE;
}
#endif // i386

BOOLEAN
KdpSwitchProcessor (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PCONTEXT ContextRecord,
    IN BOOLEAN SecondChance
    )
{
    BOOLEAN Status;

    //
    // Save port state
    //

    KdPortSave ();

    //
    // Process state change for this processor
    //

    Status = KdpReportExceptionStateChange (
                ExceptionRecord,
                ContextRecord,
                SecondChance
                );

    //
    // Restore port state and return status
    //

    KdPortRestore ();
    return Status;
}

BOOLEAN
KdpReportExceptionStateChange (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PCONTEXT ContextRecord,
    IN BOOLEAN SecondChance
    )

/*++

Routine Description:

    This routine sends an exception state change packet to the kernel
    debugger and waits for a manipulate state message.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    ContextRecord - Supplies a pointer to a context record.

    SecondChance - Supplies a boolean value that determines whether this is
        the first or second chance for the exception.

Return Value:

    A value of TRUE is returned if the exception is handled. Otherwise, a
    value of FALSE is returned.

--*/

{
    STRING MessageData;
    STRING MessageHeader;
    DBGKD_WAIT_STATE_CHANGE WaitStateChange;
    KCONTINUE_STATUS Status;

#if i386
    if (KdpCheckTracePoint(ExceptionRecord,ContextRecord)) return TRUE;
#endif

    do {

        //
        // Construct the wait state change message and message descriptor.
        //

        KdpSetStateChange(&WaitStateChange,
                            ExceptionRecord,
                            ContextRecord,
                            SecondChance
                            );

        MessageHeader.Length = sizeof(DBGKD_WAIT_STATE_CHANGE);
        MessageHeader.Buffer = (PCHAR)&WaitStateChange;

#if i386
        //
        // Construct the wait state change data and data descriptor.
        //

        DumpTraceData(&MessageData);
#else
        MessageData.Length = 0;
#endif

        //
        // Send packet to the kernel debugger on the host machine,
        // wait for answer.
        //

        Status = KdpSendWaitContinue(
                    PACKET_TYPE_KD_STATE_CHANGE,
                    &MessageHeader,
                    &MessageData,
                    ContextRecord
                    );

    } while (Status == ContinueProcessorReselected) ;

    return (BOOLEAN) Status;
}


BOOLEAN
KdpReportLoadSymbolsStateChange (
    IN PSTRING PathName,
    IN PKD_SYMBOLS_INFO SymbolInfo,
    IN BOOLEAN UnloadSymbols,
    IN OUT PCONTEXT ContextRecord
    )

/*++

Routine Description:

    This routine sends a load symbols state change packet to the kernel
    debugger and waits for a manipulate state message.

Arguments:

    PathName - Supplies a pointer to the pathname of the image whose
        symbols are to be loaded.

    BaseOfDll - Supplies the base address where the image was loaded.

    ProcessId - Unique 32-bit identifier for process that is using
        the symbols.  -1 for system process.

    CheckSum - Unique 32-bit identifier from image header.

    UnloadSymbol - TRUE if the symbols that were previously loaded for
        the named image are to be unloaded from the debugger.

Return Value:

    A value of TRUE is returned if the exception is handled. Otherwise, a
    value of FALSE is returned.

--*/

{

    PSTRING AdditionalData;
    STRING MessageData;
    STRING MessageHeader;
    DBGKD_WAIT_STATE_CHANGE WaitStateChange;
    KCONTINUE_STATUS Status;

    do {
        //
        // Construct the wait state change message and message descriptor.
        //

        WaitStateChange.NewState = DbgKdLoadSymbolsStateChange;
        WaitStateChange.ProcessorLevel = KeProcessorLevel;
        WaitStateChange.Processor = (USHORT)KeGetCurrentPrcb()->Number;
        WaitStateChange.NumberProcessors = (ULONG)KeNumberProcessors;
        WaitStateChange.Thread = (PVOID)KeGetCurrentThread();
        WaitStateChange.ProgramCounter = (PVOID)CONTEXT_TO_PROGRAM_COUNTER(ContextRecord);
        KdpSetLoadState(&WaitStateChange, ContextRecord);
        WaitStateChange.u.LoadSymbols.UnloadSymbols = UnloadSymbols;
        WaitStateChange.u.LoadSymbols.BaseOfDll = SymbolInfo->BaseOfDll;
        WaitStateChange.u.LoadSymbols.ProcessId = (ULONG)SymbolInfo->ProcessId;
        WaitStateChange.u.LoadSymbols.CheckSum = SymbolInfo->CheckSum;
        WaitStateChange.u.LoadSymbols.SizeOfImage = SymbolInfo->SizeOfImage;
        if (ARGUMENT_PRESENT( PathName )) {
            WaitStateChange.u.LoadSymbols.PathNameLength =
                KdpMoveMemory(
                    (PCHAR)KdpPathBuffer,
                    (PCHAR)PathName->Buffer,
                    PathName->Length
                    ) + 1;

            MessageData.Buffer = KdpPathBuffer;
            MessageData.Length = (USHORT)WaitStateChange.u.LoadSymbols.PathNameLength;
            MessageData.Buffer[MessageData.Length-1] = '\0';
            AdditionalData = &MessageData;
        } else {
            WaitStateChange.u.LoadSymbols.PathNameLength = 0;
            AdditionalData = NULL;
        }

        MessageHeader.Length = sizeof(DBGKD_WAIT_STATE_CHANGE);
        MessageHeader.Buffer = (PCHAR)&WaitStateChange;

        //
        // Send packet to the kernel debugger on the host machine, wait
        // for the reply.
        //

        Status = KdpSendWaitContinue(
                    PACKET_TYPE_KD_STATE_CHANGE,
                    &MessageHeader,
                    AdditionalData,
                    ContextRecord
                    );

    } while (Status == ContinueProcessorReselected);

    return (BOOLEAN) Status;
}


VOID
KdpReadPhysicalMemory(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response to a read physical memory
    state manipulation message. Its function is to read physical memory
    and return.

    N.B. This is now more dangerous than ever:  if the modified physical
    memory is mapped to a virtual page which is protected as readonly text,
    the memory manager will eventually bugcheck when it discovers that
    the page has been modified.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    PDBGKD_READ_MEMORY a = &m->u.ReadMemory;
    ULONG Length;
    STRING MessageHeader;
    PVOID VirtualAddress;
    PHYSICAL_ADDRESS Source;
    PUCHAR Destination;
    USHORT NumberBytes;
    USHORT BytesLeft;

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    //
    // make sure that nothing but a read memory message was transmitted
    //

    ASSERT(AdditionalData->Length == 0);

    //
    // Trim transfer count to fit in a single message
    //

    if (a->TransferCount > (PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE))) {
        Length = PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE);
    } else {
        Length = a->TransferCount;
    }

    //
    // Since the MmDbgTranslatePhysicalAddress only maps in one physical
    // page at a time, we need to break the memory move up into smaller
    // moves which don't cross page boundaries.  There are two cases we
    // need to deal with.  The area to be moved may start and end on the
    // same page, or it may start and end on different pages (with an
    // arbitrary number of pages in between)
    //
    Source.QuadPart = (ULONG_PTR)a->TargetBaseAddress;
    Destination = AdditionalData->Buffer;
    BytesLeft = (USHORT)Length;
    if(PAGE_ALIGN((PUCHAR)a->TargetBaseAddress) ==
       PAGE_ALIGN((PUCHAR)(a->TargetBaseAddress)+Length)) {
        //
        // Memory move starts and ends on the same page.
        //
        VirtualAddress=MmDbgTranslatePhysicalAddress(Source);
        if (VirtualAddress == NULL) {
            AdditionalData->Length = 0;
        } else {
            AdditionalData->Length = (USHORT)KdpMoveMemory(
                                                Destination,
                                                VirtualAddress,
                                                BytesLeft
                                                );
            BytesLeft -= AdditionalData->Length;
        }
    } else {
        //
        // Memory move spans page boundaries
        //
        VirtualAddress=MmDbgTranslatePhysicalAddress(Source);
        if (VirtualAddress == NULL) {
            AdditionalData->Length = 0;
        } else {
            NumberBytes = (USHORT)(PAGE_SIZE - BYTE_OFFSET(VirtualAddress));
            AdditionalData->Length = (USHORT)KdpMoveMemory(
                                            Destination,
                                            VirtualAddress,
                                            NumberBytes
                                            );
            Source.LowPart += NumberBytes;
            Destination += NumberBytes;
            BytesLeft -= NumberBytes;
            while(BytesLeft > 0) {
                //
                // Transfer a full page or the last bit,
                // whichever is smaller.
                //
                VirtualAddress = MmDbgTranslatePhysicalAddress(Source);
                if (VirtualAddress == NULL) {
                    break;
                } else {
                    NumberBytes = (USHORT) ((PAGE_SIZE < BytesLeft) ? PAGE_SIZE : BytesLeft);
                    AdditionalData->Length += (USHORT)KdpMoveMemory(
                                                    Destination,
                                                    VirtualAddress,
                                                    NumberBytes
                                                    );
                    Source.LowPart += NumberBytes;
                    Destination += NumberBytes;
                    BytesLeft -= NumberBytes;
                }
            }
        }
    }

    if (Length == AdditionalData->Length) {
        m->ReturnStatus = STATUS_SUCCESS;
    } else {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
    }
    a->ActualBytesRead = AdditionalData->Length;

    KdpSendPacket(
                  PACKET_TYPE_KD_STATE_MANIPULATE,
                  &MessageHeader,
                  AdditionalData
                  );
    UNREFERENCED_PARAMETER(Context);
}


VOID
KdpWritePhysicalMemory(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response to a write physical memory
    state manipulation message. Its function is to write physical memory
    and return.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    PDBGKD_WRITE_MEMORY a = &m->u.WriteMemory;
    ULONG Length;
    STRING MessageHeader;
    PVOID VirtualAddress;
    PHYSICAL_ADDRESS Destination;
    PUCHAR Source;
    USHORT NumberBytes;
    USHORT BytesLeft;

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;


    //
    // Since the MmDbgTranslatePhysicalAddress only maps in one physical
    // page at a time, we need to break the memory move up into smaller
    // moves which don't cross page boundaries.  There are two cases we
    // need to deal with.  The area to be moved may start and end on the
    // same page, or it may start and end on different pages (with an
    // arbitrary number of pages in between)
    //
    Destination.QuadPart = (ULONG_PTR)a->TargetBaseAddress;
    Source = AdditionalData->Buffer;
    BytesLeft = (USHORT) a->TransferCount;
    if(PAGE_ALIGN((PUCHAR)Destination.LowPart) ==
       PAGE_ALIGN((PUCHAR)(Destination.LowPart)+BytesLeft)) {
        //
        // Memory move starts and ends on the same page.
        //
        VirtualAddress=MmDbgTranslatePhysicalAddress(Destination);
        Length = (USHORT)KdpMoveMemory(
                                       VirtualAddress,
                                       Source,
                                       BytesLeft
                                      );
        BytesLeft -= (USHORT) Length;
    } else {
        //
        // Memory move spans page boundaries
        //
        VirtualAddress=MmDbgTranslatePhysicalAddress(Destination);
        NumberBytes = (USHORT) (PAGE_SIZE - BYTE_OFFSET(VirtualAddress));
        Length = (USHORT)KdpMoveMemory(
                                       VirtualAddress,
                                       Source,
                                       NumberBytes
                                      );
        Source += NumberBytes;
        Destination.LowPart += NumberBytes;
        BytesLeft -= NumberBytes;
        while(BytesLeft > 0) {
            //
            // Transfer a full page or the last bit,
            // whichever is smaller.
            //
            VirtualAddress = MmDbgTranslatePhysicalAddress(Destination);
            NumberBytes = (USHORT) ((PAGE_SIZE < BytesLeft) ? PAGE_SIZE : BytesLeft);
            Length += (USHORT)KdpMoveMemory(
                                            VirtualAddress,
                                            Source,
                                            NumberBytes
                                           );
            Source += NumberBytes;
            Destination.LowPart += NumberBytes;
            BytesLeft -= NumberBytes;
        }
    }


    if (Length == AdditionalData->Length) {
        m->ReturnStatus = STATUS_SUCCESS;
    } else {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
    }

    a->ActualBytesWritten = Length;

    KdpSendPacket(
                  PACKET_TYPE_KD_STATE_MANIPULATE,
                  &MessageHeader,
                  NULL
                  );
    UNREFERENCED_PARAMETER(Context);
}


#if i386
VOID
KdpProcessInternalBreakpoint (
    ULONG BreakpointNumber
    )
{
    static BOOLEAN timerStarted = FALSE;
    LARGE_INTEGER dueTime;

    if ( !KdpInternalBPs[BreakpointNumber].Flags &
          DBGKD_INTERNAL_BP_FLAG_COUNTONLY ) {
        return;     // We only deal with COUNTONLY breakpoints
    }

    //
    // We've hit a real internal breakpoint; make sure the timeout is
    // kicked off.
    //

    if ( !timerStarted ) { // ok, maybe there's a prettier way to do this.
        dueTime.LowPart = (ULONG)(-1 * 10 * 1000 * 1000);
        dueTime.HighPart = -1;
        KeInitializeDpc(
            &InternalBreakpointCheckDpc,
            &InternalBreakpointCheck,
            NULL
            );
        KeInitializeTimer( &InternalBreakpointTimer );
        KeSetTimer(
            &InternalBreakpointTimer,
            dueTime,
            &InternalBreakpointCheckDpc
            );
        timerStarted = TRUE;
    }

    KdpInternalBPs[BreakpointNumber].Calls++;

} // KdpProcessInternalBreakpoint
#endif


VOID
KdpGetVersion(
    IN PDBGKD_MANIPULATE_STATE m
    )

/*++

Routine Description:

    This function returns to the caller a general information packet
    that contains useful information to a debugger.  This packet is also
    used for a debugger to determine if the writebreakpointex and
    readbreakpointex apis are available.

Arguments:

    m - Supplies the state manipulation message.

Return Value:

    None.

--*/

{
    STRING                   messageHeader;


    messageHeader.Length = sizeof(*m);
    messageHeader.Buffer = (PCHAR)m;

    RtlZeroMemory(&m->u.GetVersion, sizeof(m->u.GetVersion));
    //
    // the current build number
    //
    m->u.GetVersion.MinorVersion = (short)NtBuildNumber;
    m->u.GetVersion.MajorVersion = (short)((NtBuildNumber >> 28) & 0xFFFFFFF);

    //
    // kd protocol version number.  this should be incremented if the
    // protocol changes.
    //
    m->u.GetVersion.ProtocolVersion = 4;
    m->u.GetVersion.Flags = DBGKD_VERS_FLAG_DATA;

#if !defined(NT_UP)
    m->u.GetVersion.Flags |= DBGKD_VERS_FLAG_MP;
#endif

#if defined(_M_IX86)
    m->u.GetVersion.MachineType = IMAGE_FILE_MACHINE_I386;
#elif defined(_M_MRX000)
    m->u.GetVersion.MachineType = IMAGE_FILE_MACHINE_R4000;
#elif defined(_M_ALPHA)
    m->u.GetVersion.MachineType = IMAGE_FILE_MACHINE_ALPHA;
#if defined (_AXP64_)
    m->u.GetVersion.Flags |= DBGKD_VERS_FLAG_PTR64;
#endif
#elif defined(_M_PPC)
    m->u.GetVersion.MachineType = IMAGE_FILE_MACHINE_POWERPC;
#elif defined(_M_IA64)
    m->u.GetVersion.MachineType = IMAGE_FILE_MACHINE_IA64;
    m->u.GetVersion.Flags |= DBGKD_VERS_FLAG_PTR64;
#else
#error( "unknown target machine" );
#endif

    //
    // address of the loader table
    //
    m->u.GetVersion.PsLoadedModuleList = (ULONG_PTR)&PsLoadedModuleList;

    //
    // If the debugger is being initialized during boot, PsNtosImageBase
    // and PsLoadedModuleList are not yet valid.  KdInitSystem got
    // the image base from the loader block.
    // On the other hand, if the debugger was initialized by a bugcheck,
    // it didn't get a loader block to look at, but the system was
    // running so the other variables are valid.
    //
    if (KdpNtosImageBase) {
        m->u.GetVersion.KernBase = (ULONG_PTR)KdpNtosImageBase;
    } else {
        m->u.GetVersion.KernBase = (ULONG_PTR)PsNtosImageBase;
    }

    //
    // These fields are obsolete with the introduction of
    // KdDebuggerDataBlock.  They are being kept for a while
    // so people can still use their 4.0 debuggers.
    //
    m->u.GetVersion.ThCallbackStack = FIELD_OFFSET(KTHREAD, CallbackStack);
    m->u.GetVersion.KiCallUserMode = (ULONG_PTR)KiCallUserMode;
    m->u.GetVersion.KeUserCallbackDispatcher = (ULONG_PTR) KeUserCallbackDispatcher;
    m->u.GetVersion.NextCallback = FIELD_OFFSET(KCALLOUT_FRAME, CbStk);
#if defined(_X86_)
    m->u.GetVersion.FramePointer = FIELD_OFFSET(KCALLOUT_FRAME, Ebp);
#endif
    m->u.GetVersion.BreakpointWithStatus = (ULONG_PTR)RtlpBreakWithStatusInstruction;





    m->u.GetVersion.DebuggerDataList = (ULONG_PTR)&KdpDebuggerDataListHead;

    //
    // the usual stuff
    //
    m->ReturnStatus = STATUS_SUCCESS;
    m->ApiNumber = DbgKdGetVersionApi;

    KdpSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                  &messageHeader,
                  NULL
                 );

    return;
} // KdGetVersion


NTSTATUS
KdpNotSupported(
    IN PDBGKD_MANIPULATE_STATE m
    )
/*++

Routine Description:

    This routine returns STATUS_UNSUCCESSFUL to the debugger

Arguments:

    m - Supplies a DBGKD_MANIPULATE_STATE struct to answer with

Return Value:

    0, to indicate that the system should not continue

--*/
{

    STRING          MessageHeader;
    //
    // setup packet
    //
    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;
    m->ReturnStatus = STATUS_UNSUCCESSFUL;

    //
    // send back our response
    //
    KdpSendPacket(
        PACKET_TYPE_KD_STATE_MANIPULATE,
        &MessageHeader,
        NULL
        );

    //
    // return the caller's continue status value.  if this is a non-zero
    // value the system is continued using this value as the continuestatus.
    //

    return 0;

} // KdpNotSupported


VOID
KdpCauseBugCheck(
    IN PDBGKD_MANIPULATE_STATE m
    )

/*++

Routine Description:

    This routine causes a bugcheck.  It is used for testing the debugger.

Arguments:

    m - Supplies the state manipulation message.

Return Value:

    None.

--*/

{

    KeBugCheckEx( *(PULONG)&m->u, 0, 0, 0, 0 );

} // KdCauseBugCheck


NTSTATUS
KdpWriteBreakPointEx(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response of a write breakpoint state 'ex'
    manipulation message.  Its function is to clear breakpoints, write
    new breakpoints, and continue the target system.  The clearing of
    breakpoints is conditional based on the presence of breakpoint handles.
    The setting of breakpoints is conditional based on the presence of
    valid, non-zero, addresses.  The continueing of the target system
    is conditional based on a non-zero continuestatus.

    This api allows a debugger to clear breakpoints, add new breakpoint,
    and continue the target system all in one api packet.  This reduces the
    amount of traffic across the wire and greatly improves source stepping.


Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    PDBGKD_BREAKPOINTEX       a = &m->u.BreakPointEx;
    PDBGKD_WRITE_BREAKPOINT   b;
    STRING                    MessageHeader;
    ULONG                     i;
    DBGKD_WRITE_BREAKPOINT    BpBuf[BREAKPOINT_TABLE_SIZE];


    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    //
    // verify that the packet size is correct
    //
    if (AdditionalData->Length !=
                         a->BreakPointCount*sizeof(DBGKD_WRITE_BREAKPOINT)) {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
        KdpSendPacket(
                      PACKET_TYPE_KD_STATE_MANIPULATE,
                      &MessageHeader,
                      AdditionalData
                      );
        return m->ReturnStatus;
    }

    KdpMoveMemory((PUCHAR)BpBuf,
                  AdditionalData->Buffer,
                  a->BreakPointCount*sizeof(DBGKD_WRITE_BREAKPOINT));

    //
    // assume success
    //
    m->ReturnStatus = STATUS_SUCCESS;

    //
    // loop thru the breakpoint handles passed in from the debugger and
    // clear any breakpoint that has a non-zero handle
    //
    b = BpBuf;
    for (i=0; i<a->BreakPointCount; i++,b++) {
        if (b->BreakPointHandle) {
            if (!KdpDeleteBreakpoint(b->BreakPointHandle)) {
                m->ReturnStatus = STATUS_UNSUCCESSFUL;
            }
            b->BreakPointHandle = 0;
        }
    }

    //
    // loop thru the breakpoint addesses passed in from the debugger and
    // add any new breakpoints that have a non-zero address
    //
    b = BpBuf;
    for (i=0; i<a->BreakPointCount; i++,b++) {
        if (b->BreakPointAddress) {
            b->BreakPointHandle = KdpAddBreakpoint( b->BreakPointAddress );
            if (!b->BreakPointHandle) {
                m->ReturnStatus = STATUS_UNSUCCESSFUL;
            }
        }
    }

    //
    // send back our response
    //

    KdpMoveMemory(AdditionalData->Buffer,
                  (PUCHAR)BpBuf,
                  a->BreakPointCount*sizeof(DBGKD_WRITE_BREAKPOINT));

    KdpSendPacket(
                  PACKET_TYPE_KD_STATE_MANIPULATE,
                  &MessageHeader,
                  AdditionalData
                  );

    //
    // return the caller's continue status value.  if this is a non-zero
    // value the system is continued using this value as the continuestatus.
    //
    return a->ContinueStatus;
}


VOID
KdpRestoreBreakPointEx(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response of a restore breakpoint state 'ex'
    manipulation message.  Its function is to clear a list of breakpoints.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    PDBGKD_BREAKPOINTEX         a = &m->u.BreakPointEx;
    PDBGKD_RESTORE_BREAKPOINT   b;
    STRING                      MessageHeader;
    ULONG                       i;
    DBGKD_RESTORE_BREAKPOINT    BpBuf[BREAKPOINT_TABLE_SIZE];


    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    //
    // verify that the packet size is correct
    //
    if (AdditionalData->Length !=
                       a->BreakPointCount*sizeof(DBGKD_RESTORE_BREAKPOINT)) {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
        KdpSendPacket(
                      PACKET_TYPE_KD_STATE_MANIPULATE,
                      &MessageHeader,
                      AdditionalData
                      );
        return;
    }

    KdpMoveMemory((PUCHAR)BpBuf,
                  AdditionalData->Buffer,
                  a->BreakPointCount*sizeof(DBGKD_RESTORE_BREAKPOINT));

    //
    // assume success
    //
    m->ReturnStatus = STATUS_SUCCESS;

    //
    // loop thru the breakpoint handles passed in from the debugger and
    // clear any breakpoint that has a non-zero handle
    //
    b = BpBuf;
    for (i=0; i<a->BreakPointCount; i++,b++) {
        if (!KdpDeleteBreakpoint(b->BreakPointHandle)) {
            m->ReturnStatus = STATUS_UNSUCCESSFUL;
        }
    }

    //
    // send back our response
    //
    KdpSendPacket(
                  PACKET_TYPE_KD_STATE_MANIPULATE,
                  &MessageHeader,
                  AdditionalData
                  );
}

VOID
KdDisableDebugger(
    VOID
    )

/*++

Routine Description:

    This function is called to disable the debugger.

Arguments:

    None.

Return Value:

    None.

--*/

{
    KIRQL oldIrql ;

    KeRaiseIrql(DISPATCH_LEVEL, &oldIrql) ;
    KdpPortLock();

    if (!KdDisableCount) {

        KdPreviouslyEnabled = KdDebuggerEnabled && (!KdPitchDebugger) ;
        if (KdDebuggerEnabled) {

            KdpSuspendAllBreakpoints() ;
            KiDebugRoutine = KdpStub;
            KdDebuggerEnabled = FALSE ;
        }
    }
    KdDisableCount++ ;
    KdpPortUnlock();
    KeLowerIrql(oldIrql);
}

VOID
KdEnableDebugger(
   VOID
   )
/*++

Routine Description:

    This function is called to reenable the debugger after a call to
    KdDisableDebugger.

Arguments:

    None.

Return Value:

    None.

--*/
{
    KIRQL oldIrql ;

    KeRaiseIrql(DISPATCH_LEVEL, &oldIrql) ;
    KdpPortLock();

    ASSERT(KdDisableCount > 0) ;
    KdDisableCount-- ;

    if (!KdDisableCount) {
        if (KdPreviouslyEnabled) {

            //
            // Ugly HACKHACK - Make sure the timers aren't reset.
            //
            PoHiberInProgress = TRUE ;
            KdInitSystem(NULL, FALSE) ;
            KdpRestoreAllBreakpoints();
            PoHiberInProgress = FALSE ;
        }
    }
    KdpPortUnlock();
    KeLowerIrql(oldIrql);
}


VOID
KdpSearchMemory(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function implements a memory pattern searcher.  This will
    find an instance of a pattern that begins in the range
    SearchAddress..SearchAddress+SearchLength.  The pattern may
    end outside of the range.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies the pattern to search for

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    PUCHAR Pattern = AdditionalData->Buffer;
    ULONG_PTR StartAddress = (ULONG_PTR)m->u.SearchMemory.SearchAddress;
    ULONG_PTR EndAddress = (ULONG_PTR)(StartAddress + m->u.SearchMemory.SearchLength);
    ULONG PatternLength = m->u.SearchMemory.PatternLength;

    STRING MessageHeader;
    ULONG MaskIndex;
    PUCHAR PatternTail;
    PUCHAR DataTail;
    ULONG TailLength;
    ULONG Data;
    ULONG FirstWordPattern[4];
    ULONG FirstWordMask[4];


    //
    // On failure, return STATUS_NO_MORE_ENTRIES.  DON'T RETURN
    // STATUS_UNSUCCESSFUL!  That return status indicates that the
    // operation is not supported, and the debugger will fall back
    // to a debugger-side search.
    //

    m->ReturnStatus = STATUS_NO_MORE_ENTRIES;

    //
    // Do a fast search for the beginning of the pattern
    //

    if (PatternLength > 3) {
        FirstWordMask[0] = 0xffffffff;
    } else {
        FirstWordMask[0] = 0xffffffff >> (8*(4-PatternLength));
    }

    FirstWordMask[1] = FirstWordMask[0] << 8;
    FirstWordMask[2] = FirstWordMask[1] << 8;
    FirstWordMask[3] = FirstWordMask[2] << 8;

    FirstWordPattern[0] = 0;
    KdpQuickMoveMemory((PVOID)FirstWordPattern,
                       Pattern,
                       PatternLength < 5 ? PatternLength : 4);

    FirstWordPattern[1] = FirstWordPattern[0] << 8;
    FirstWordPattern[2] = FirstWordPattern[1] << 8;
    FirstWordPattern[3] = FirstWordPattern[2] << 8;


/*
{
    int i;
    for (i = 0; i < (int)PatternLength; i++) {
        KdpDprintf("%08x: %02x\n", &Pattern[i], Pattern[i]);
    }
    for (i = 0; i < 4; i++) {
        KdpDprintf("%d: %08x %08x\n", i, FirstWordPattern[i], FirstWordMask[i]);
    }
}
*/



    //
    // Get starting mask
    //

    MaskIndex = StartAddress & 3;
    StartAddress = StartAddress & ~3;

    //
    // check that the starting page is available
    //

    if (MmDbgReadCheck((PVOID)StartAddress) == NULL) {
        StartAddress = (StartAddress + PAGE_SIZE) & ~(PAGE_SIZE-1);
        MaskIndex = 0;
    }

    while (StartAddress < EndAddress) {

        //
        // check when starting a new page
        //
        if ((StartAddress & (PAGE_SIZE-1)) == 0) {
            if (MmDbgReadCheck((PVOID)StartAddress) == NULL) {
                StartAddress = StartAddress + PAGE_SIZE;
                continue;
            }
        }

        //
        // search for a match in each of the 4 starting positions
        //

        Data = *(ULONG*)StartAddress;
//KdpDprintf("\n%08x: %08x ", StartAddress, Data);

        for ( ; MaskIndex < 4; MaskIndex++) {
//KdpDprintf(" %d", MaskIndex);

            if ( (Data & FirstWordMask[MaskIndex]) == FirstWordPattern[MaskIndex]) {

                //
                // first word matched
                //

                if ( (4-MaskIndex) >= PatternLength ) {

                    //
                    // string is all in this word; good match
                    //
//KdpDprintf(" %d hit, complete\n", MaskIndex);

                    m->u.SearchMemory.FoundAddress = StartAddress + MaskIndex;
                    m->ReturnStatus = STATUS_SUCCESS;
                    goto done;

                } else {

                    //
                    // string is longer; see if tail matches
                    //
//KdpDprintf(" %d hit, check tail\n", MaskIndex);

                    PatternTail = Pattern + 4 - MaskIndex;
                    DataTail = (PUCHAR)StartAddress + 4;
                    TailLength = PatternLength - 4 + MaskIndex;

//KdpDprintf("Pattern == %08x\n", Pattern);
//KdpDprintf("PatternTail == %08x\n", PatternTail);
//KdpDprintf("DataTail == %08x\n", DataTail);

                    while (TailLength) {
                        if ( ((ULONG_PTR)DataTail & (PAGE_SIZE-1)) == 0 &&
                             MmDbgReadCheck(DataTail) == FALSE) {
//KdpDprintf("Tail failed: page not present at %08x\n", DataTail);
                            break;
                        } else
{
//KdpDprintf("D: %02x  P: %02x\n", *DataTail, *PatternTail);

                        if (*DataTail != *PatternTail) {
//KdpDprintf("Tail failed at %08x\n", DataTail);
                            break;
                        } else {
                            DataTail++;
                            PatternTail++;
                            TailLength--;
                        }
}
                    }

                    if (TailLength == 0) {

                        //
                        // A winner
                        //

                        m->u.SearchMemory.FoundAddress = StartAddress + MaskIndex;
                        m->ReturnStatus = STATUS_SUCCESS;
                        goto done;

                    }
                }
            }
        }

        StartAddress += 4;
        MaskIndex = 0;
    }

done:
//KdpDprintf("\n");
    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    KdpSendPacket(
        PACKET_TYPE_KD_STATE_MANIPULATE,
        &MessageHeader,
        NULL
        );

}
