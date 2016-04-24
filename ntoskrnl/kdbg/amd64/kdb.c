/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/kdbg/amd64/kdb.c
 * PURPOSE:         Kernel Debugger
 * PROGRAMMERS:
 */


/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

ULONG
NTAPI
KiEspFromTrapFrame(IN PKTRAP_FRAME TrapFrame)
{
    return TrapFrame->Rsp;
}

VOID
NTAPI
KiEspToTrapFrame(IN PKTRAP_FRAME TrapFrame,
                 IN ULONG_PTR Esp)
{
    KIRQL OldIrql;
    ULONG Previous;

    /* Raise to APC_LEVEL if needed */
    OldIrql = KeGetCurrentIrql();
    if (OldIrql < APC_LEVEL) KeRaiseIrql(APC_LEVEL, &OldIrql);

    /* Get the old ESP */
    Previous = KiEspFromTrapFrame(TrapFrame);

    /* Check if this is user-mode  */
    if ((TrapFrame->SegCs & MODE_MASK))
    {
        /* Write it directly */
        TrapFrame->Rsp = Esp;
    }
    else
    {
        /* Don't allow ESP to be lowered, this is illegal */
        if (Esp < Previous) KeBugCheckEx(SET_OF_INVALID_CONTEXT,
                                         Esp,
                                         Previous,
                                         (ULONG_PTR)TrapFrame,
                                         0);

        /* Create an edit frame, check if it was alrady */
        if (!(TrapFrame->SegCs & FRAME_EDITED))
        {
            /* Update the value */
            TrapFrame->Rsp = Esp;
        }
        else
        {
            /* Check if ESP changed */
            if (Previous != Esp)
            {
                /* Save CS */
                TrapFrame->SegCs &= ~FRAME_EDITED;

                /* Save ESP */
                TrapFrame->Rsp = Esp;
            }
        }
    }

    /* Restore IRQL */
    if (OldIrql < APC_LEVEL) KeLowerIrql(OldIrql);

}

ULONG
NTAPI
KiSsFromTrapFrame(IN PKTRAP_FRAME TrapFrame)
{
    if (TrapFrame->SegCs & MODE_MASK)
    {
        /* User mode, return the User SS */
        return TrapFrame->SegSs | RPL_MASK;
    }
    else
    {
        /* Kernel mode */
        return KGDT64_SYS_TSS;
    }
}

VOID
NTAPI
KiSsToTrapFrame(IN PKTRAP_FRAME TrapFrame,
                IN ULONG Ss)
{
        /* Remove the high-bits */
    Ss &= 0xFFFF;

    if (TrapFrame->SegCs & MODE_MASK)
    {
        /* Usermode, save the User SS */
        TrapFrame->SegSs = Ss | RPL_MASK;
    }

}



