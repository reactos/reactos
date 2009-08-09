/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/powerpc/pfault.c
 * PURPOSE:         Paging file functions
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>
#include <ppcmmu/mmu.h>

/* EXTERNS *******************************************************************/

NTSTATUS
NTAPI
MmNotPresentFault(KPROCESSOR_MODE Mode,
                  ULONG_PTR Address,
                  BOOLEAN FromMdl);
extern ULONG KiKernelTrapHandler(PKTRAP_FRAME Tf, ULONG ExceptionNr, PVOID Cr2);

/* FUNCTIONS *****************************************************************/

VOID MmpPpcTrapFrameToTrapFrame(ppc_trap_frame_t *frame, PKTRAP_FRAME Tf)
{
    RtlCopyMemory(&Tf->Gpr0, frame->gpr, 12 * sizeof(ULONG));
    Tf->Lr = frame->lr;
    Tf->Cr = frame->cr;
    Tf->Ctr = frame->ctr;
    Tf->Xer = frame->xer;
    Tf->Iar = frame->srr0;
    Tf->Msr = frame->srr1 & 0xffff;
    Tf->Dr0 = frame->dar;
    Tf->Dr1 = frame->dsisr;
}

void CopyFrame(int *oldframe, int *ourframe)
{
    int i;

    for (i = 0; i < sizeof(ppc_trap_frame_t) / sizeof(int); i++)
    {
        ourframe[i] = GetPhys((int)&oldframe[i]);
    }
}

void KiPageFaultHandler(int trap, ppc_trap_frame_t *frame)
{
    NTSTATUS Status = STATUS_SUCCESS;
    KPROCESSOR_MODE Mode;
    EXCEPTION_RECORD Er;
    KTRAP_FRAME Tf;
    BOOLEAN AccessFault = !!(frame->dsisr & (1<<28));
    vaddr_t VirtualAddr;
    PVOID TrapInfo = NULL;

    /* get the faulting address */
    if (trap == 4) /* Instruction miss */
	VirtualAddr = frame->srr0;
    else /* Data miss */
	VirtualAddr = frame->dar;

    /* MSR_PR */
    Mode = frame->srr1 & 0x4000 ? UserMode : KernelMode;
    DPRINT("Page Fault at %08x\n", frame->srr0);

    /* handle the fault */
    if (AccessFault)
    {
	Status = MmAccessFault(Mode, (PVOID)VirtualAddr, FALSE, TrapInfo);
    }
    else
    {
	Status = MmNotPresentFault(Mode, VirtualAddr, FALSE);
    }

    if (NT_SUCCESS(Status))
    {
        MmuCallbackRet();
    }

    if (KeGetCurrentThread()->ApcState.UserApcPending)
    {
        KIRQL oldIrql;

        KeRaiseIrql(APC_LEVEL, &oldIrql);
        KiDeliverApc(UserMode, NULL, NULL);
        KeLowerIrql(oldIrql);
    }

    MmpPpcTrapFrameToTrapFrame(frame, &Tf);

    Er.ExceptionCode = STATUS_ACCESS_VIOLATION;
    Er.ExceptionFlags = 0;
    Er.ExceptionRecord = NULL;
    Er.ExceptionAddress = (PVOID)frame->srr0;
    Er.NumberParameters = 2;
    Er.ExceptionInformation[0] = AccessFault;
    Er.ExceptionInformation[1] = VirtualAddr;

    /* FIXME: Which exceptions are noncontinuable? */
    Er.ExceptionFlags = 0;

    KiDispatchException(&Er, 0, &Tf, Mode, TRUE);
    MmuCallbackRet();
}

