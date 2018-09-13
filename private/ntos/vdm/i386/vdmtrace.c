/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vdmtrace.c

Abstract:

    This module contains the support maintaining the VDM trace log.

Author:

    Neil Sandlin (neilsa) 15-Sep-1996

Revision History:

--*/


#include "vdmp.h"

VOID
VdmTraceEvent(
    USHORT Type,
    USHORT wData,
    USHORT lData,
    PKTRAP_FRAME TrapFrame
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, VdmTraceEvent)
#endif

VOID
VdmTraceEvent(
    USHORT Type,
    USHORT wData,
    USHORT lData,
    PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:



Arguments:

Return Value:

    None

--*/
{
    PVDM_TIB VdmTib;
    NTSTATUS Status = STATUS_SUCCESS;
    KIRQL   OldIrql;
    PVDM_TRACEENTRY pEntry;
    PVDM_TRACEINFO pInfo;
    LARGE_INTEGER CurTime, DiffTime;


    PAGED_CODE();
#if 0
	// This code represents a security problem.  Since it is only used
	// on special occasions, it won't be built into the standard build.
	// Individuals wishing to use it can build themselves a kernel with
	// it in.
#if 0
    //
    // Raise Irql to APC level...
    //
    KeRaiseIrql(APC_LEVEL, &OldIrql);

    //
    // VdmTib is in user mode memory
    //
    try {
#endif
        if ((*(ULONG *)(FIXED_NTVDMSTATE_LINEAR) & VDM_TRACE_HISTORY)) {

            //
            // Get a pointer to the VdmTib
            //
            VdmTib = NtCurrentTeb()->Vdm;

            if (VdmTib->TraceInfo.pTraceTable) {
           
                pEntry = &VdmTib->TraceInfo.pTraceTable[VdmTib->TraceInfo.CurrentEntry];
               
                pEntry->Type = Type;
                pEntry->wData = wData;
                pEntry->lData = lData;

                switch (VdmTib->TraceInfo.Flags & VDMTI_TIMER_MODE) {
                case VDMTI_TIMER_TICK:
                    CurTime.LowPart = NtGetTickCount();
                    pEntry->Time = CurTime.LowPart - VdmTib->TraceInfo.TimeStamp.LowPart;
                    VdmTib->TraceInfo.TimeStamp.LowPart = CurTime.LowPart;
                    break;

                case VDMTI_TIMER_PERFCTR:
                    pEntry->Time = 0;
                    break;

                case VDMTI_TIMER_STAT:
                    pEntry->Time = 0;
                    break;

                }
               
                pEntry->eax = TrapFrame->Eax;
                pEntry->ebx = TrapFrame->Ebx;
                pEntry->ecx = TrapFrame->Ecx;
                pEntry->edx = TrapFrame->Edx;
                pEntry->esi = TrapFrame->Esi;
                pEntry->edi = TrapFrame->Edi;
                pEntry->ebp = TrapFrame->Ebp;
                pEntry->esp = TrapFrame->HardwareEsp;
                pEntry->eip = TrapFrame->Eip;
                pEntry->eflags = TrapFrame->EFlags;
               
                pEntry->cs = (USHORT) TrapFrame->SegCs;
                pEntry->ds = (USHORT) TrapFrame->SegDs;
                pEntry->es = (USHORT) TrapFrame->SegEs;
                pEntry->fs = (USHORT) TrapFrame->SegFs;
                pEntry->gs = (USHORT) TrapFrame->SegGs;
                pEntry->ss = (USHORT) TrapFrame->HardwareSegSs;
               
                if (++VdmTib->TraceInfo.CurrentEntry >=
                   (VdmTib->TraceInfo.NumPages*4096/sizeof(VDM_TRACEENTRY))) {
                    VdmTib->TraceInfo.CurrentEntry = 0;
                }
            }
        }

#if 0
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    KeLowerIrql(OldIrql);
#endif
#endif
}
