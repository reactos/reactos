/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/thread.c
 * PURPOSE:         i386 Thread Context Creation
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

typedef struct _KSHARED_CTXSWITCH_FRAME
{
    PVOID ExceptionList;
    PVOID RetEip;
} KSHARED_CTXSWITCH_FRAME, *PKSHARED_CTXSWITCH_FRAME;

typedef struct _KSTART_FRAME
{
    PKSYSTEM_ROUTINE SystemRoutine;
    PKSTART_ROUTINE StartRoutine;
    PVOID StartContext;
    BOOLEAN UserThread;
} KSTART_FRAME, *PKSTART_FRAME;

/*
 * This is the Initial Thread Stack Frame on i386.
 *
 * It is composed of :
 *
 *     - A shared Thread Switching frame so that we can use
 *       the context-switching code when initializing the thread.
 *
 *     - The Stack Frame for KiThreadStartup, which are the parameters
 *       that it will receive (System/Start Routines & Context)
 *
 *     - A Trap Frame with the Initial Context *IF AND ONLY IF THE THREAD IS USER*
 *
 *     - The FPU Save Area, theoretically part of the Trap Frame's "ExtendedRegisters"
 *
 * This Initial Thread Stack Frame starts at Thread->InitialStack and it spans
 * a total size of 0x2B8 bytes.
 */
typedef struct _KUINIT_FRAME {
    KSHARED_CTXSWITCH_FRAME CtxSwitchFrame;    /* -0x2B8 */
    KSTART_FRAME StartFrame;                   /* -0x2AC */
    KTRAP_FRAME TrapFrame;                     /* -0x29C */
    FX_SAVE_AREA FxSaveArea;                   /* -0x210 */
} KUINIT_FRAME, *PKUINIT_FRAME;

typedef struct _KKINIT_FRAME {
    KSHARED_CTXSWITCH_FRAME CtxSwitchFrame;    /* -0x22C */
    KSTART_FRAME StartFrame;                   /* -0x220 */
    FX_SAVE_AREA FxSaveArea;                   /* -0x210 */
} KKINIT_FRAME, *PKKINIT_FRAME;

/* FUNCTIONS *****************************************************************/

VOID
STDCALL
Ke386InitThreadWithContext(PKTHREAD Thread,
                           PKSYSTEM_ROUTINE SystemRoutine,
                           PKSTART_ROUTINE StartRoutine,
                           PVOID StartContext,
                           PCONTEXT ContextPointer)
{
    PFX_SAVE_AREA FxSaveArea;
    PFXSAVE_FORMAT FxSaveFormat;
    PKSTART_FRAME StartFrame;
    PKSHARED_CTXSWITCH_FRAME CtxSwitchFrame;
    PKTRAP_FRAME TrapFrame;
    CONTEXT LocalContext;
    PCONTEXT Context = NULL;
    ULONG ContextFlags;

    /* Check if this is a With-Context Thread */
    DPRINT("Ke386InitThreadContext\n");
    if (ContextPointer)
    {
        /* Set up the Initial Frame */
        PKUINIT_FRAME InitFrame;
        InitFrame = (PKUINIT_FRAME)((ULONG_PTR)Thread->InitialStack -
                                    sizeof(KUINIT_FRAME));
        DPRINT("Setting up a user-mode thread. InitFrame at: %p\n", InitFrame);

        /* Copy over the context we got */
        RtlMoveMemory(&LocalContext, ContextPointer, sizeof(CONTEXT));
        Context = &LocalContext;
        ContextFlags = CONTEXT_CONTROL;

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
            ContextFlags |= (KeI386XMMIPresent) ? CONTEXT_EXTENDED_REGISTERS :
                                                  CONTEXT_FLOATING_POINT;

            /* Set the Thread's NPX State */
            Thread->NpxState = NPX_STATE_NOT_LOADED;
            Thread->DispatcherHeader.NpxIrql = PASSIVE_LEVEL;
        }
        else
        {
            /* We'll use emulation */
            FxSaveArea->Cr0NpxState = CR0_EM;
            Thread->NpxState = NPX_STATE_NOT_LOADED &~ CR0_MP;
        }

        /* Disable any debug regiseters */
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
        TrapFrame->ExceptionList = (PVOID)0xFFFFFFFF;

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
        DPRINT("Setting up a kernel thread. InitFrame at: %p\n", InitFrame);

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
    CtxSwitchFrame->RetEip = KiThreadStartup;
    CtxSwitchFrame->ExceptionList = (PVOID)0xFFFFFFFF;

    /* Save back the new value of the kernel stack. */
    DPRINT("Final Kernel Stack: %x \n", CtxSwitchFrame);
    Thread->KernelStack = (PVOID)CtxSwitchFrame;
    return;
}

/* EOF */

