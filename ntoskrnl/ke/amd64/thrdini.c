/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/amd64/thrdini.c
 * PURPOSE:         amd64 Thread Context Creation
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 *                  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

extern void KiInvalidSystemThreadStartupExit(void);
extern void KiUserThreadStartupExit(void);
extern void KiServiceExit3(void);

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
        RtlZeroMemory(&InitFrame->ExceptionFrame, sizeof(KEXCEPTION_FRAME));

        /* Set up a trap frame from the context. */
        KeContextToTrapFrame(Context,
                             &InitFrame->ExceptionFrame,
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

        /* KiThreadStartup returns to KiUserThreadStartupExit */
        StartFrame->Return = (ULONG64)KiUserThreadStartupExit;

        /* KiUserThreadStartupExit returns to KiServiceExit3 */
        InitFrame->ExceptionFrame.Return = (ULONG64)KiServiceExit3;
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

        /* This must never return! */
        StartFrame->Return = (ULONG64)KiInvalidSystemThreadStartupExit;
    }

    /* Set up the Context Switch Frame */
    CtxSwitchFrame->Return = (ULONG64)KiThreadStartup;
    CtxSwitchFrame->ApcBypass = TRUE;

    StartFrame->P1Home = (ULONG64)StartRoutine;
    StartFrame->P2Home = (ULONG64)StartContext;
    StartFrame->P3Home = 0;
    StartFrame->P4Home = (ULONG64)SystemRoutine;
    StartFrame->Reserved = 0;
}

BOOLEAN
KiSwapContextResume(
    _In_ BOOLEAN ApcBypass,
    _In_ PKTHREAD OldThread,
    _In_ PKTHREAD NewThread)
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

    /* Old thread os no longer busy */
    OldThread->SwapBusy = FALSE;

    /* Kernel APCs may be pending */
    if (NewThread->ApcState.KernelApcPending)
    {
        /* Are APCs enabled? */
        if ((NewThread->SpecialApcDisable == 0) &&
            (ApcBypass == 0))
        {
            /* Return TRUE to indicate that we want APCs to be delivered */
            return TRUE;
        }

        /* Request an APC interrupt to be delivered later */
        HalRequestSoftwareInterrupt(APC_LEVEL);
    }

    /* Return stating that no kernel APCs are pending*/
    return FALSE;
}

/* EOF */


