/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/thread.c
 * PURPOSE:         amd64 Thread Context Creation
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 *                  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

typedef struct _KUINIT_FRAME
{
    KSWITCH_FRAME CtxSwitchFrame;
    KSTART_FRAME StartFrame;
    KEXCEPTION_FRAME ExceptionFrame;
    KTRAP_FRAME TrapFrame;
    //FX_SAVE_AREA FxSaveArea;
} KUINIT_FRAME, *PKUINIT_FRAME;

typedef struct _KKINIT_FRAME
{
    KSWITCH_FRAME CtxSwitchFrame;
    KSTART_FRAME StartFrame;
    //FX_SAVE_AREA FxSaveArea;
} KKINIT_FRAME, *PKKINIT_FRAME;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KiInitializeContextThread(IN PKTHREAD Thread,
                           IN PKSYSTEM_ROUTINE SystemRoutine,
                           IN PKSTART_ROUTINE StartRoutine,
                           IN PVOID StartContext,
                           IN PCONTEXT Context)
{
    //PFX_SAVE_AREA FxSaveArea;
    //PFXSAVE_FORMAT FxSaveFormat;
    PKSTART_FRAME StartFrame;
    PKSWITCH_FRAME CtxSwitchFrame;
    PKTRAP_FRAME TrapFrame;
    ULONG ContextFlags;

    /* Check if this is a With-Context Thread */
    if (Context)
    {
        PKUINIT_FRAME InitFrame;

        /* Set up the Initial Frame */
        InitFrame = ((PKUINIT_FRAME)Thread->InitialStack) - 1;
        StartFrame = &InitFrame->StartFrame;
        CtxSwitchFrame = &InitFrame->CtxSwitchFrame;

        /* Save back the new value of the kernel stack. */
        Thread->KernelStack = (PVOID)InitFrame;

        /* Tell the thread it will run in User Mode */
        Thread->PreviousMode = UserMode;

        // FIXME Setup the Fx Area

        /* Set the Thread's NPX State */
        Thread->NpxState = 0xA;
        Thread->Header.NpxIrql = PASSIVE_LEVEL;

        /* Make sure, we have control registers, disable debug registers */
        ASSERT((Context->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL);
        ContextFlags = Context->ContextFlags & ~CONTEXT_DEBUG_REGISTERS;

        /* Setup the Trap Frame */
        TrapFrame = &InitFrame->TrapFrame;

        /* Zero out the trap frame */
        RtlZeroMemory(TrapFrame, sizeof(KTRAP_FRAME));

        /* Set up a trap frame from the context. */
        KeContextToTrapFrame(Context,
                             NULL,
                             TrapFrame,
                             CONTEXT_AMD64 | ContextFlags,
                             UserMode);

        /* Set SS, DS, ES's RPL Mask properly */
        TrapFrame->SegSs |= RPL_MASK;
        TrapFrame->SegDs |= RPL_MASK;
        TrapFrame->SegEs |= RPL_MASK;
        TrapFrame->Dr7 = 0;

        /* Set the previous mode as user */
        TrapFrame->PreviousMode = UserMode;

        /* Terminate the Exception Handler List */
        TrapFrame->ExceptionFrame = 0;

        /* We return to ... */
        StartFrame->Return = (ULONG64)KiServiceExit2;
    }
    else
    {
        PKKINIT_FRAME InitFrame;

        /* Set up the Initial Frame for the system thread */
        InitFrame = ((PKKINIT_FRAME)Thread->InitialStack) - 1;
        StartFrame = &InitFrame->StartFrame;
        CtxSwitchFrame = &InitFrame->CtxSwitchFrame;

        /* Save back the new value of the kernel stack. */
        Thread->KernelStack = (PVOID)InitFrame;

        /* Tell the thread it will run in Kernel Mode */
        Thread->PreviousMode = KernelMode;

        // FIXME Setup the Fx Area

        /* No NPX State */
        Thread->NpxState = 0xA;

        /* We have no return address! */
        StartFrame->Return = 0;
    }

    /* Set up the Context Switch Frame */
    CtxSwitchFrame->Return = (ULONG64)KiThreadStartup;
    CtxSwitchFrame->ApcBypass = FALSE;

    StartFrame->P1Home = (ULONG64)StartRoutine;
    StartFrame->P2Home = (ULONG64)StartContext;
    StartFrame->P3Home = 0;
    StartFrame->P4Home = (ULONG64)SystemRoutine;
    StartFrame->Reserved = 0;
}

BOOLEAN
KiSwapContextResume(
    IN PKTHREAD NewThread,
    IN PKTHREAD OldThread,
    IN BOOLEAN ApcBypass)
{
    PKIPCR Pcr = (PKIPCR)KeGetPcr();
    PKPROCESS OldProcess, NewProcess;

    /* Setup ring 0 stack pointer */
    Pcr->TssBase->Rsp0 = (ULONG64)NewThread->InitialStack; // FIXME: NPX save area?
    Pcr->Prcb.RspBase = Pcr->TssBase->Rsp0;

    /* Now we are the new thread. Check if it's in a new process */
    OldProcess = OldThread->ApcState.Process;
    NewProcess = NewThread->ApcState.Process;
    if (OldProcess != NewProcess)
    {
        /* Switch address space and flush TLB */
        __writecr3(NewProcess->DirectoryTableBase[0]);

        /* Set new TSS fields */
        //Pcr->TssBase->IoMapBase = NewProcess->IopmOffset;
    }

    /* Set TEB pointer and GS base */
    Pcr->NtTib.Self = (PVOID)NewThread->Teb;
    if (NewThread->Teb)
    {
       /* This will switch the usermode gs */
       __writemsr(MSR_GS_SWAP, (ULONG64)NewThread->Teb);
    }

    /* Increase context switch count */
    Pcr->ContextSwitches++;
    NewThread->ContextSwitches++;

    /* DPCs shouldn't be active */
    if (Pcr->Prcb.DpcRoutineActive)
    {
        /* Crash the machine */
        KeBugCheckEx(ATTEMPTED_SWITCH_FROM_DPC,
                     (ULONG_PTR)OldThread,
                     (ULONG_PTR)NewThread,
                     (ULONG_PTR)OldThread->InitialStack,
                     0);
    }

    /* Kernel APCs may be pending */
    if (NewThread->ApcState.KernelApcPending)
    {
        /* Are APCs enabled? */
        if (!NewThread->SpecialApcDisable)
        {
            /* Request APC delivery */
            if (!ApcBypass)
                HalRequestSoftwareInterrupt(APC_LEVEL);
            else
                return TRUE;
        }
    }

    /* Return stating that no kernel APCs are pending*/
    return FALSE;
}

/* EOF */


