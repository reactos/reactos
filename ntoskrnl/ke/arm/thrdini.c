/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ke/arm/thrdini.c
 * PURPOSE:         Implements thread context setup and startup for ARM machines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

typedef struct _KSWITCHFRAME
{
    PVOID ExceptionList;
    BOOLEAN ApcBypassDisable;
    PVOID RetAddr;
} KSWITCHFRAME, *PKSWITCHFRAME;

typedef struct _KUINIT_FRAME
{
    KEXCEPTION_FRAME CtxSwitchFrame;
    KEXCEPTION_FRAME ExceptionFrame;
    KTRAP_FRAME TrapFrame;
} KUINIT_FRAME, *PKUINIT_FRAME;

typedef struct _KKINIT_FRAME
{
    KEXCEPTION_FRAME CtxSwitchFrame;
} KKINIT_FRAME, *PKKINIT_FRAME;

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
KiThreadStartup(VOID);

VOID
FASTCALL
KiSwitchThreads(
    IN PKTHREAD OldThread,
    IN PKTHREAD NewThread
);


/* FIXME: THIS IS TOTALLY BUSTED NOW */
VOID
NTAPI
KiInitializeContextThread(IN PKTHREAD Thread,
                          IN PKSYSTEM_ROUTINE SystemRoutine,
                          IN PKSTART_ROUTINE StartRoutine,
                          IN PVOID StartContext,
                          IN PCONTEXT ContextPointer)
{
    PKTRAP_FRAME TrapFrame;
    PKEXCEPTION_FRAME ExceptionFrame = NULL, CtxSwitchFrame;

    //
    // Check if this is a user thread
    //
    if (ContextPointer)
    {
        //
        // Setup the initial frame
        //
        PKUINIT_FRAME InitFrame;
        InitFrame = (PKUINIT_FRAME)((ULONG_PTR)Thread->InitialStack -
                                    sizeof(KUINIT_FRAME));

        //
        // Setup the Trap Frame and Exception frame
        //
        TrapFrame = &InitFrame->TrapFrame;
        ExceptionFrame = &InitFrame->ExceptionFrame;

        ///
        // Zero out the trap frame and exception frame
        //
        RtlZeroMemory(TrapFrame, sizeof(KTRAP_FRAME));
        RtlZeroMemory(ExceptionFrame, sizeof(KEXCEPTION_FRAME));

        //
        // Set up a trap frame from the context
        //
        KeContextToTrapFrame(ContextPointer,
                             ExceptionFrame,
                             TrapFrame,
                             ContextPointer->ContextFlags | CONTEXT_CONTROL,
                             UserMode);

        //
        // Set the previous mode as user
        //
        //TrapFrame->PreviousMode = UserMode;
        Thread->PreviousMode = UserMode;

        //
        // Clear the return address
        //
        ExceptionFrame->Return = 0;

        //
        // Context switch frame to setup below
        //
        CtxSwitchFrame = &InitFrame->CtxSwitchFrame;
    }
    else
    {
        //
        // Set up the Initial Frame for the system thread
        //
        PKKINIT_FRAME InitFrame;
        InitFrame = (PKKINIT_FRAME)((ULONG_PTR)Thread->InitialStack -
                                    sizeof(KKINIT_FRAME));

        //
        // Set the previous mode as kernel
        //
        Thread->PreviousMode = KernelMode;

        //
        // Context switch frame to setup below
        //
        CtxSwitchFrame = &InitFrame->CtxSwitchFrame;
    }

    //
    // Now setup the context switch frame
    //
    CtxSwitchFrame->Return = (ULONG)KiThreadStartup;
    CtxSwitchFrame->R11 = (ULONG)(ExceptionFrame ? ExceptionFrame : CtxSwitchFrame);

    //
    // Set the parameters
    //
    CtxSwitchFrame->R4 = (ULONG)ContextPointer;
    CtxSwitchFrame->R5 = (ULONG)StartContext;
    CtxSwitchFrame->R6 = (ULONG)StartRoutine;
    CtxSwitchFrame->R7 = (ULONG)SystemRoutine;

    //
    // Save back the new value of the kernel stack
    //
    Thread->KernelStack = (PVOID)CtxSwitchFrame;
}

DECLSPEC_NORETURN
VOID
KiIdleLoop(VOID)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    PKTHREAD OldThread, NewThread;

    /* Now loop forever */
    while (TRUE)
    {
        /* Start of the idle loop: disable interrupts */
        _enable();
        YieldProcessor();
        YieldProcessor();
        _disable();
    
        /* Check for pending timers, pending DPCs, or pending ready threads */
        if ((Prcb->DpcData[0].DpcQueueDepth) ||
            (Prcb->TimerRequest) ||
            (Prcb->DeferredReadyListHead.Next))
        {
            /* Quiesce the DPC software interrupt */
            HalClearSoftwareInterrupt(DISPATCH_LEVEL);

            /* Handle it */
            KiRetireDpcList(Prcb);
        }

        /* Check if a new thread is scheduled for execution */
        if (Prcb->NextThread)
        {
            /* Enable interrupts */
            _enable();

            /* Capture current thread data */
            OldThread = Prcb->CurrentThread;
            NewThread = Prcb->NextThread;

            /* Set new thread data */
            Prcb->NextThread = NULL;
            Prcb->CurrentThread = NewThread;

            /* The thread is now running */
            NewThread->State = Running;

            /* Switch away from the idle thread */
            KiSwapContext(APC_LEVEL, OldThread);
        }
        else
        {
            /* Continue staying idle. Note the HAL returns with interrupts on */
            Prcb->PowerState.IdleFunction(&Prcb->PowerState);
        }
    }
}

BOOLEAN
FASTCALL
KiSwapContextExit(IN PKTHREAD OldThread,
                  IN PKSWITCHFRAME SwitchFrame)
{
    PKIPCR Pcr = (PKIPCR)KeGetPcr();
    PKPROCESS OldProcess, NewProcess;
    PKTHREAD NewThread;
    ARM_TTB_REGISTER TtbRegister;

    /* We are on the new thread stack now */
    NewThread = Pcr->Prcb.CurrentThread;

    /* Now we are the new thread. Check if it's in a new process */
    OldProcess = OldThread->ApcState.Process;
    NewProcess = NewThread->ApcState.Process;
    if (OldProcess != NewProcess)
    {
        TtbRegister.AsUlong = NewProcess->DirectoryTableBase[0];
        ASSERT(TtbRegister.Reserved == 0);
        KeArmTranslationTableRegisterSet(TtbRegister);
    }

    /* Increase thread context switches */
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
            if (SwitchFrame->ApcBypassDisable) HalRequestSoftwareInterrupt(APC_LEVEL);
            return TRUE;
        }
    }

    /* Return */
    return FALSE;
}

VOID
FASTCALL
KiSwapContextEntry(IN PKSWITCHFRAME SwitchFrame,
                   IN ULONG_PTR OldThreadAndApcFlag)
{
    PKIPCR Pcr = (PKIPCR)KeGetPcr();
    PKTHREAD OldThread, NewThread;

    /* Save APC bypass disable */
    SwitchFrame->ApcBypassDisable = OldThreadAndApcFlag & 3;

    /* Increase context switch count and check if tracing is enabled */
    Pcr->Prcb.KeContextSwitches++;
#if 0
    if (Pcr->PerfGlobalGroupMask)
    {
        /* We don't support this yet on x86 either */
        DPRINT1("WMI Tracing not supported\n");
        ASSERT(FALSE);
    }
#endif // 0

    /* Get thread pointers */
    OldThread = (PKTHREAD)(OldThreadAndApcFlag & ~3);
    NewThread = Pcr->Prcb.CurrentThread;

    /* Get the old thread and set its kernel stack */
    OldThread->KernelStack = SwitchFrame;

    /* Do the switch */
    KiSwitchThreads(OldThread, NewThread->KernelStack);
}

VOID
NTAPI
KiDispatchInterrupt(VOID)
{
    PKIPCR Pcr = (PKIPCR)KeGetPcr();
    PKPRCB Prcb = &Pcr->Prcb;
    PKTHREAD NewThread, OldThread;

    /* Disable interrupts */
    _disable();

    /* Check for pending timers, pending DPCs, or pending ready threads */
    if ((Prcb->DpcData[0].DpcQueueDepth) ||
        (Prcb->TimerRequest) ||
        (Prcb->DeferredReadyListHead.Next))
    {
        /* Retire DPCs while under the DPC stack */
        //KiRetireDpcListInDpcStack(Prcb, Prcb->DpcStack);
        // FIXME!!! //
        KiRetireDpcList(Prcb);
    }

    /* Re-enable interrupts */
    _enable();

    /* Check for quantum end */
    if (Prcb->QuantumEnd)
    {
        /* Handle quantum end */
        Prcb->QuantumEnd = FALSE;
        KiQuantumEnd();
    }
    else if (Prcb->NextThread)
    {
        /* Capture current thread data */
        OldThread = Prcb->CurrentThread;
        NewThread = Prcb->NextThread;

        /* Set new thread data */
        Prcb->NextThread = NULL;
        Prcb->CurrentThread = NewThread;

        /* The thread is now running */
        NewThread->State = Running;
        OldThread->WaitReason = WrDispatchInt;

        /* Make the old thread ready */
        KxQueueReadyThread(OldThread, Prcb);

        /* Swap to the new thread */
        KiSwapContext(APC_LEVEL, OldThread);
    }
}

/* EOF */
