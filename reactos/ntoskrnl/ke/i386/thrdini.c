/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/thrdini.c
 * PURPOSE:         i386 Thread Context Creation
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

typedef struct _KSWITCHFRAME
{
    PVOID ExceptionList;
    BOOLEAN ApcBypassDisable;
    PVOID RetAddr;
} KSWITCHFRAME, *PKSWITCHFRAME;

typedef struct _KSTART_FRAME
{
    PKSYSTEM_ROUTINE SystemRoutine;
    PKSTART_ROUTINE StartRoutine;
    PVOID StartContext;
    BOOLEAN UserThread;
} KSTART_FRAME, *PKSTART_FRAME;

typedef struct _KUINIT_FRAME
{
    KSWITCHFRAME CtxSwitchFrame;
    KSTART_FRAME StartFrame;
    KTRAP_FRAME TrapFrame;
    FX_SAVE_AREA FxSaveArea;
} KUINIT_FRAME, *PKUINIT_FRAME;

typedef struct _KKINIT_FRAME
{
    KSWITCHFRAME CtxSwitchFrame;
    KSTART_FRAME StartFrame;
    FX_SAVE_AREA FxSaveArea;
} KKINIT_FRAME, *PKKINIT_FRAME;

VOID
FASTCALL
KiSwitchThreads(
    IN PKTHREAD OldThread,
    IN PKTHREAD NewThread
);

VOID
FASTCALL
KiRetireDpcListInDpcStack(
    IN PKPRCB Prcb,
    IN PVOID DpcStack
);

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KiThreadStartup(VOID)
{
    PKTRAP_FRAME TrapFrame;
    PKSTART_FRAME StartFrame;
    PKUINIT_FRAME InitFrame;

    /* Get the start and trap frames */
    InitFrame = KeGetCurrentThread()->KernelStack;
    StartFrame = &InitFrame->StartFrame;
    TrapFrame = &InitFrame->TrapFrame;

    /* Lower to APC level */
    KfLowerIrql(APC_LEVEL);

    /* Call the system routine */
    StartFrame->SystemRoutine(StartFrame->StartRoutine, StartFrame->StartContext);

    /* If we returned, we better be a user thread */
    if (!StartFrame->UserThread) DbgBreakPoint();

    /* Exit to user-mode */
    KiServiceExit2(TrapFrame);
}

VOID
NTAPI
KiInitializeContextThread(IN PKTHREAD Thread,
                          IN PKSYSTEM_ROUTINE SystemRoutine,
                          IN PKSTART_ROUTINE StartRoutine,
                          IN PVOID StartContext,
                          IN PCONTEXT ContextPointer)
{
    PFX_SAVE_AREA FxSaveArea;
    PFXSAVE_FORMAT FxSaveFormat;
    PKSTART_FRAME StartFrame;
    PKSWITCHFRAME CtxSwitchFrame;
    PKTRAP_FRAME TrapFrame;
    CONTEXT LocalContext;
    PCONTEXT Context = NULL;
    ULONG ContextFlags;

    /* Check if this is a With-Context Thread */
    if (ContextPointer)
    {
        /* Set up the Initial Frame */
        PKUINIT_FRAME InitFrame;
        InitFrame = (PKUINIT_FRAME)((ULONG_PTR)Thread->InitialStack -
                                    sizeof(KUINIT_FRAME));

        /* Copy over the context we got */
        RtlCopyMemory(&LocalContext, ContextPointer, sizeof(CONTEXT));
        Context = &LocalContext;
        ContextFlags = CONTEXT_CONTROL;

        /* Zero out the trap frame and save area */
        RtlZeroMemory(&InitFrame->TrapFrame,
                      KTRAP_FRAME_LENGTH + sizeof(FX_SAVE_AREA));

        /* Setup the Fx Area */
        FxSaveArea = &InitFrame->FxSaveArea;

        /* Check if we support FXsr */
        if (KeI386FxsrPresent)
        {
            /* Get the FX Save Format Area */
            FxSaveFormat = (PFXSAVE_FORMAT)Context->ExtendedRegisters;

            /* Set an initial state */
            FxSaveFormat->ControlWord = 0x27F;
            FxSaveFormat->StatusWord = 0;
            FxSaveFormat->TagWord = 0;
            FxSaveFormat->ErrorOffset = 0;
            FxSaveFormat->ErrorSelector = 0;
            FxSaveFormat->DataOffset = 0;
            FxSaveFormat->DataSelector = 0;
            FxSaveFormat->MXCsr = 0x1F80;
        }
        else
        {
            /* Setup the regular save area */
            Context->FloatSave.ControlWord = 0x27F;
            Context->FloatSave.StatusWord = 0;
            Context->FloatSave.TagWord = -1;
            Context->FloatSave.ErrorOffset = 0;
            Context->FloatSave.ErrorSelector = 0;
            Context->FloatSave.DataOffset =0;
            Context->FloatSave.DataSelector = 0;
        }

        /* Check if the CPU has NPX */
        if (KeI386NpxPresent)
        {
            /* Set an intial NPX State */
            Context->FloatSave.Cr0NpxState = 0;
            FxSaveArea->Cr0NpxState = 0;
            FxSaveArea->NpxSavedCpu = 0;

            /* Now set the context flags depending on XMM support */
            ContextFlags |= (KeI386FxsrPresent) ? CONTEXT_EXTENDED_REGISTERS :
                                                  CONTEXT_FLOATING_POINT;

            /* Set the Thread's NPX State */
            Thread->NpxState = NPX_STATE_NOT_LOADED;
            Thread->Header.NpxIrql = PASSIVE_LEVEL;
        }
        else
        {
            /* We'll use emulation */
            FxSaveArea->Cr0NpxState = CR0_EM;
            Thread->NpxState = NPX_STATE_NOT_LOADED &~ CR0_MP;
        }

        /* Disable any debug registers */
        Context->ContextFlags &= ~CONTEXT_DEBUG_REGISTERS;

        /* Setup the Trap Frame */
        TrapFrame = &InitFrame->TrapFrame;

        /* Set up a trap frame from the context. */
        KeContextToTrapFrame(Context,
                             NULL,
                             TrapFrame,
                             Context->ContextFlags | ContextFlags,
                             UserMode);

        /* Set SS, DS, ES's RPL Mask properly */
        TrapFrame->HardwareSegSs |= RPL_MASK;
        TrapFrame->SegDs |= RPL_MASK;
        TrapFrame->SegEs |= RPL_MASK;
        TrapFrame->Dr7 = 0;

        /* Set the debug mark */
        TrapFrame->DbgArgMark = 0xBADB0D00;

        /* Set the previous mode as user */
        TrapFrame->PreviousPreviousMode = UserMode;

        /* Terminate the Exception Handler List */
        TrapFrame->ExceptionList = EXCEPTION_CHAIN_END;

        /* Setup the Stack for KiThreadStartup and Context Switching */
        StartFrame = &InitFrame->StartFrame;
        CtxSwitchFrame = &InitFrame->CtxSwitchFrame;

        /* Tell the thread it will run in User Mode */
        Thread->PreviousMode = UserMode;

        /* Tell KiThreadStartup of that too */
        StartFrame->UserThread = TRUE;
    }
    else
    {
        /* Set up the Initial Frame for the system thread */
        PKKINIT_FRAME InitFrame;
        InitFrame = (PKKINIT_FRAME)((ULONG_PTR)Thread->InitialStack -
                                    sizeof(KKINIT_FRAME));

        /* Setup the Fx Area */
        FxSaveArea = &InitFrame->FxSaveArea;
        RtlZeroMemory(FxSaveArea, sizeof(FX_SAVE_AREA));

        /* Check if we have Fxsr support */
        if (KeI386FxsrPresent)
        {
            /* Set the stub FX area */
            FxSaveArea->U.FxArea.ControlWord = 0x27F;
            FxSaveArea->U.FxArea.MXCsr = 0x1F80;
        }
        else
        {
            /* Set the stub FN area */
            FxSaveArea->U.FnArea.ControlWord = 0x27F;
            FxSaveArea->U.FnArea.TagWord = -1;
        }

        /* No NPX State */
        Thread->NpxState = NPX_STATE_NOT_LOADED;

        /* Setup the Stack for KiThreadStartup and Context Switching */
        StartFrame = &InitFrame->StartFrame;
        CtxSwitchFrame = &InitFrame->CtxSwitchFrame;

        /* Tell the thread it will run in Kernel Mode */
        Thread->PreviousMode = KernelMode;

        /* Tell KiThreadStartup of that too */
        StartFrame->UserThread = FALSE;
    }

    /* Now setup the remaining data for KiThreadStartup */
    StartFrame->StartContext = StartContext;
    StartFrame->StartRoutine = StartRoutine;
    StartFrame->SystemRoutine = SystemRoutine;

    /* And set up the Context Switch Frame */
    CtxSwitchFrame->RetAddr = KiThreadStartup;
    CtxSwitchFrame->ApcBypassDisable = TRUE;
    CtxSwitchFrame->ExceptionList = EXCEPTION_CHAIN_END;

    /* Save back the new value of the kernel stack. */
    Thread->KernelStack = (PVOID)CtxSwitchFrame;
}

VOID
FASTCALL
KiIdleLoop(VOID)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    PKTHREAD OldThread, NewThread;

    /* Initialize the idle loop: disable interrupts */
    _enable();
    YieldProcessor();
    YieldProcessor();
    _disable();

    /* Now loop forever */
    while (TRUE)
    {
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
            /* Enable interupts */
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

            /* We are back in the idle thread -- disable interrupts again */
            _enable();
            YieldProcessor();
            YieldProcessor();
            _disable();
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

    /* We are on the new thread stack now */
    NewThread = Pcr->PrcbData.CurrentThread;

    /* Now we are the new thread. Check if it's in a new process */
    OldProcess = OldThread->ApcState.Process;
    NewProcess = NewThread->ApcState.Process;
    if (OldProcess != NewProcess)
    {
        /* Check if there is a different LDT */
        if (*(PULONGLONG)&OldProcess->LdtDescriptor != *(PULONGLONG)&NewProcess->LdtDescriptor)
        {
            DPRINT1("LDT switch not implemented\n");
            ASSERT(FALSE);
        }

        /* Switch address space and flush TLB */
        __writecr3(NewProcess->DirectoryTableBase[0]);
    }

    /* Clear GS */
    Ke386SetGs(0);

    /* Set the TEB */
    KiSetTebBase((PKPCR)Pcr, NewThread->Teb);

    /* Set new TSS fields */
    Pcr->TSS->Esp0 = (ULONG_PTR)NewThread->InitialStack;
    if (!((KeGetTrapFrame(NewThread))->EFlags & EFLAGS_V86_MASK))
    {
        Pcr->TSS->Esp0 -= (FIELD_OFFSET(KTRAP_FRAME, V86Gs) - FIELD_OFFSET(KTRAP_FRAME, HardwareSegSs));
    }
    Pcr->TSS->Esp0 -= NPX_FRAME_LENGTH;
    Pcr->TSS->IoMapBase = NewProcess->IopmOffset;

    /* Increase thread context switches */
    NewThread->ContextSwitches++;

    /* Load data from switch frame */
    Pcr->NtTib.ExceptionList = SwitchFrame->ExceptionList;

    /* DPCs shouldn't be active */
    if (Pcr->PrcbData.DpcRoutineActive)
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
            if (SwitchFrame->ApcBypassDisable)
                HalRequestSoftwareInterrupt(APC_LEVEL);
            else
                return TRUE;
        }
    }

    /* Return stating that no kernel APCs are pending*/
    return FALSE;
}

VOID
FASTCALL
KiSwapContextEntry(IN PKSWITCHFRAME SwitchFrame,
                   IN ULONG_PTR OldThreadAndApcFlag)
{
    PKIPCR Pcr = (PKIPCR)KeGetPcr();
    PKTHREAD OldThread, NewThread;
    ULONG Cr0, NewCr0;

    /* Save APC bypass disable */
    SwitchFrame->ApcBypassDisable = OldThreadAndApcFlag & 3;
    SwitchFrame->ExceptionList = Pcr->NtTib.ExceptionList;

    /* Increase context switch count and check if tracing is enabled */
    Pcr->ContextSwitches++;
    if (Pcr->PerfGlobalGroupMask)
    {
        /* We don't support this yet on x86 either */
        DPRINT1("WMI Tracing not supported\n");
        ASSERT(FALSE);
    }

    /* Get thread pointers */
    OldThread = (PKTHREAD)(OldThreadAndApcFlag & ~3);
    NewThread = Pcr->PrcbData.CurrentThread;

    /* Get the old thread and set its kernel stack */
    OldThread->KernelStack = SwitchFrame;

    /* ISRs can change FPU state, so disable interrupts while checking */
    _disable();

    /* Get current and new CR0 and check if they've changed */
    Cr0 = __readcr0();
    NewCr0 = NewThread->NpxState |
             (Cr0 & ~(CR0_MP | CR0_EM | CR0_TS)) |
             KiGetThreadNpxArea(NewThread)->Cr0NpxState;
    if (Cr0 != NewCr0)  __writecr0(NewCr0);

    /* Now enable interrupts and do the switch */
    _enable();
    KiSwitchThreads(OldThread, NewThread->KernelStack);
}

VOID
NTAPI
KiDispatchInterrupt(VOID)
{
    PKIPCR Pcr = (PKIPCR)KeGetPcr();
    PKPRCB Prcb = &Pcr->PrcbData;
    PVOID OldHandler;
    PKTHREAD NewThread, OldThread;

    /* Disable interrupts */
    _disable();

    /* Check for pending timers, pending DPCs, or pending ready threads */
    if ((Prcb->DpcData[0].DpcQueueDepth) ||
        (Prcb->TimerRequest) ||
        (Prcb->DeferredReadyListHead.Next))
    {
        /* Switch to safe execution context */
        OldHandler = Pcr->NtTib.ExceptionList;
        Pcr->NtTib.ExceptionList = EXCEPTION_CHAIN_END;

        /* Retire DPCs while under the DPC stack */
        KiRetireDpcListInDpcStack(Prcb, Prcb->DpcStack);

        /* Restore context */
        Pcr->NtTib.ExceptionList = OldHandler;
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
