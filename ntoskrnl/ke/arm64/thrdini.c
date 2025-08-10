/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 Thread Initialization
 * COPYRIGHT:   Copyright 2024 ReactOS Team
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/**
 * @brief Initialize an ARM64 thread's context
 */
VOID
NTAPI
KiInitializeThread(
    IN OUT PKTHREAD Thread,
    IN PVOID KernelStack,
    IN PKSTART_ROUTINE SystemRoutine,
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext,
    IN PCONTEXT ContextFrame,
    IN PVOID Teb,
    IN PKPROCESS Process
)
{
    PKTRAP_FRAME TrapFrame;
    PKEXCEPTION_FRAME ExceptionFrame;
    PULONG64 InitialStack;
    
    DPRINT("KiInitializeThread: Thread=%p, Stack=%p, StartRoutine=%p\n",
           Thread, KernelStack, StartRoutine);
    
    /* Set up the Initial Stack */
    InitialStack = (PULONG64)KernelStack;
    Thread->InitialStack = KernelStack;
    Thread->StackBase = KernelStack;
    Thread->StackLimit = (PVOID)((ULONG_PTR)KernelStack - KERNEL_STACK_SIZE + PAGE_SIZE);
    Thread->KernelStack = KernelStack;
    
    /* Calculate trap frame and exception frame positions */
    TrapFrame = (PKTRAP_FRAME)((ULONG_PTR)InitialStack - 
                               ALIGN_UP(sizeof(KTRAP_FRAME), STACK_ALIGN));
    ExceptionFrame = (PKEXCEPTION_FRAME)((ULONG_PTR)TrapFrame - 
                                        ALIGN_UP(sizeof(KEXCEPTION_FRAME), STACK_ALIGN));
    
    /* Clear the frames */
    RtlZeroMemory(TrapFrame, sizeof(KTRAP_FRAME));
    RtlZeroMemory(ExceptionFrame, sizeof(KEXCEPTION_FRAME));
    
    /* Set up the Trap Frame */
    TrapFrame->PreviousMode = KernelMode;
    TrapFrame->PreviousIrql = PASSIVE_LEVEL;
    
    /* Check if we have a context frame (user thread) */
    if (ContextFrame)
    {
        /* Copy user context */
        TrapFrame->X0 = ContextFrame->X0;
        TrapFrame->X1 = ContextFrame->X1;
        TrapFrame->X2 = ContextFrame->X2;
        TrapFrame->X3 = ContextFrame->X3;
        TrapFrame->X4 = ContextFrame->X4;
        TrapFrame->X5 = ContextFrame->X5;
        TrapFrame->X6 = ContextFrame->X6;
        TrapFrame->X7 = ContextFrame->X7;
        TrapFrame->X8 = ContextFrame->X8;
        TrapFrame->X9 = ContextFrame->X9;
        TrapFrame->X10 = ContextFrame->X10;
        TrapFrame->X11 = ContextFrame->X11;
        TrapFrame->X12 = ContextFrame->X12;
        TrapFrame->X13 = ContextFrame->X13;
        TrapFrame->X14 = ContextFrame->X14;
        TrapFrame->X15 = ContextFrame->X15;
        TrapFrame->X16 = ContextFrame->X16;
        TrapFrame->X17 = ContextFrame->X17;
        TrapFrame->X18 = ContextFrame->X18;
        TrapFrame->X19 = ContextFrame->X19;
        TrapFrame->X20 = ContextFrame->X20;
        TrapFrame->X21 = ContextFrame->X21;
        TrapFrame->X22 = ContextFrame->X22;
        TrapFrame->X23 = ContextFrame->X23;
        TrapFrame->X24 = ContextFrame->X24;
        TrapFrame->X25 = ContextFrame->X25;
        TrapFrame->X26 = ContextFrame->X26;
        TrapFrame->X27 = ContextFrame->X27;
        TrapFrame->X28 = ContextFrame->X28;
        TrapFrame->Fp = ContextFrame->Fp;    /* X29 */
        TrapFrame->Lr = ContextFrame->Lr;    /* X30 */
        TrapFrame->Sp = ContextFrame->Sp;
        TrapFrame->Pc = ContextFrame->Pc;
        TrapFrame->Pstate = ContextFrame->Pstate;
        
        /* Set up user mode */
        TrapFrame->PreviousMode = UserMode;
        
        /* Set up TEB if provided */
        if (Teb)
        {
            TrapFrame->X18 = (ULONG64)Teb;  /* ARM64 TEB pointer convention */
        }
        
        DPRINT("KiInitializeThread: User thread, PC=0x%llX, SP=0x%llX\n",
               TrapFrame->Pc, TrapFrame->Sp);
    }
    else
    {
        /* Kernel thread - set up to call SystemRoutine then StartRoutine */
        TrapFrame->X0 = (ULONG64)StartRoutine;    /* First parameter */
        TrapFrame->X1 = (ULONG64)StartContext;    /* Second parameter */
        TrapFrame->Pc = (ULONG64)SystemRoutine;   /* Start at SystemRoutine */
        TrapFrame->Sp = (ULONG64)ExceptionFrame;  /* Stack pointer */
        TrapFrame->Pstate = 0;                    /* Enable interrupts, EL1 */
        
        DPRINT("KiInitializeThread: Kernel thread, SystemRoutine=%p, StartRoutine=%p\n",
               SystemRoutine, StartRoutine);
    }
    
    /* Set up the Exception Frame for kernel threads */
    ExceptionFrame->X19 = 0;
    ExceptionFrame->X20 = 0;
    ExceptionFrame->X21 = 0;
    ExceptionFrame->X22 = 0;
    ExceptionFrame->X23 = 0;
    ExceptionFrame->X24 = 0;
    ExceptionFrame->X25 = 0;
    ExceptionFrame->X26 = 0;
    ExceptionFrame->X27 = 0;
    ExceptionFrame->X28 = 0;
    ExceptionFrame->Fp = 0;     /* X29 */
    ExceptionFrame->Lr = 0;     /* X30 - will be set to thread exit */
    
    /* Set up thread's kernel stack pointer to point to trap frame */
    Thread->KernelStack = (PVOID)TrapFrame;
    
    /* Initialize other thread fields */
    Thread->TebMapped = FALSE;
    Thread->CallbackStack = NULL;
    
    /* Set up floating point state if needed */
    Thread->NpxState = NPX_STATE_NOT_LOADED;
    
    /* Initialize debugging fields */
    Thread->DebugActive = FALSE;
    
    DPRINT("KiInitializeThread: Thread initialized successfully\n");
}

/**
 * @brief Initialize ARM64-specific thread state
 */
VOID
NTAPI
KiInitializeThreadContext(
    IN PKTHREAD Thread,
    IN PKSYSTEM_ROUTINE SystemRoutine,
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext,
    IN PCONTEXT Context
)
{
    PKTRAP_FRAME TrapFrame;
    
    /* Get the trap frame */
    TrapFrame = (PKTRAP_FRAME)((ULONG_PTR)Thread->InitialStack - 
                               ALIGN_UP(sizeof(KTRAP_FRAME), STACK_ALIGN));
    
    if (Context)
    {
        /* User mode thread */
        DPRINT("KiInitializeThreadContext: User thread\n");
        
        /* The context was already set up in KiInitializeThread */
        TrapFrame->PreviousMode = UserMode;
        
        /* Ensure user mode processor state */
        TrapFrame->Pstate &= ~0xF;  /* Clear exception level to EL0 (user mode) */
    }
    else
    {
        /* Kernel mode thread */
        DPRINT("KiInitializeThreadContext: Kernel thread, SystemRoutine=%p\n", SystemRoutine);
        
        /* Set up to call the system routine */
        TrapFrame->X0 = (ULONG64)StartRoutine;
        TrapFrame->X1 = (ULONG64)StartContext;
        TrapFrame->Pc = (ULONG64)SystemRoutine;
        TrapFrame->PreviousMode = KernelMode;
        
        /* Ensure kernel mode processor state (EL1) */
        TrapFrame->Pstate = (TrapFrame->Pstate & ~0xF) | 0x4;  /* EL1h */
    }
}

/**
 * @brief Set up a thread to run in user mode
 */
NTSTATUS
NTAPI
KiSetupUserThreadStartup(
    IN PKTHREAD Thread,
    IN PVOID StartAddress,
    IN PVOID Parameter
)
{
    PKTRAP_FRAME TrapFrame;
    
    DPRINT("KiSetupUserThreadStartup: Thread=%p, StartAddress=%p\n", 
           Thread, StartAddress);
    
    /* Get the trap frame */
    TrapFrame = (PKTRAP_FRAME)((ULONG_PTR)Thread->InitialStack - 
                               ALIGN_UP(sizeof(KTRAP_FRAME), STACK_ALIGN));
    
    /* Set up user mode execution */
    TrapFrame->Pc = (ULONG64)StartAddress;
    TrapFrame->X0 = (ULONG64)Parameter;      /* First parameter */
    TrapFrame->PreviousMode = UserMode;
    TrapFrame->Pstate = 0;                   /* EL0 (user mode), interrupts enabled */
    
    /* Set up user stack if not already done */
    if (TrapFrame->Sp == 0)
    {
        /* Use default user stack location */
        TrapFrame->Sp = USER_STACK_BASE - PAGE_SIZE;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Initialize the ARM64 idle thread
 */
VOID
NTAPI
KiInitializeIdleThread(
    IN PKTHREAD Thread,
    IN PKPROCESS Process,
    IN PVOID IdleStack
)
{
    extern VOID KiIdleLoop(VOID);
    
    DPRINT("KiInitializeIdleThread: Thread=%p, Stack=%p\n", Thread, IdleStack);
    
    /* Initialize as a kernel thread */
    KiInitializeThread(Thread,
                      IdleStack,
                      (PKSTART_ROUTINE)KiIdleLoop,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      Process);
    
    /* Mark as idle thread */
    Thread->State = Running;
    Thread->Priority = LOW_PRIORITY;
    Thread->BasePriority = LOW_PRIORITY;
    
    DPRINT("KiInitializeIdleThread: Idle thread initialized\n");
}

/**
 * @brief Get the current trap frame for a thread
 */
PKTRAP_FRAME
NTAPI
KiGetTrapFrame(
    IN PKTHREAD Thread
)
{
    return (PKTRAP_FRAME)((ULONG_PTR)Thread->InitialStack - 
                          ALIGN_UP(sizeof(KTRAP_FRAME), STACK_ALIGN));
}

/**
 * @brief Get the current exception frame for a thread
 */
PKEXCEPTION_FRAME
NTAPI
KiGetExceptionFrame(
    IN PKTHREAD Thread
)
{
    PKTRAP_FRAME TrapFrame = KiGetTrapFrame(Thread);
    
    return (PKEXCEPTION_FRAME)((ULONG_PTR)TrapFrame - 
                              ALIGN_UP(sizeof(KEXCEPTION_FRAME), STACK_ALIGN));
}

/**
 * @brief Handle ARM64-specific thread startup
 */
VOID
NTAPI
KiThreadStartup(VOID)
{
    PKTHREAD Thread;
    PKSTART_ROUTINE StartRoutine;
    PVOID StartContext;
    PKTRAP_FRAME TrapFrame;
    
    /* Get current thread */
    Thread = KeGetCurrentThread();
    TrapFrame = KiGetTrapFrame(Thread);
    
    DPRINT("KiThreadStartup: Thread=%p\n", Thread);
    
    /* Get start routine and context from trap frame */
    StartRoutine = (PKSTART_ROUTINE)TrapFrame->X0;
    StartContext = (PVOID)TrapFrame->X1;
    
    /* Enable interrupts */
    _enable();
    
    /* Call the thread start routine */
    if (StartRoutine)
    {
        DPRINT("KiThreadStartup: Calling StartRoutine=%p with Context=%p\n",
               StartRoutine, StartContext);
        
        StartRoutine(StartContext);
    }
    
    /* Thread finished - terminate it */
    DPRINT("KiThreadStartup: Thread finished, terminating\n");
    PsTerminateSystemThread(STATUS_SUCCESS);
}