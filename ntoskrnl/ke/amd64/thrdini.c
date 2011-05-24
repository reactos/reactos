/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/thread.c
 * PURPOSE:         i386 Thread Context Creation
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

typedef struct _KUINIT_FRAME
{
    KSWITCH_FRAME CtxSwitchFrame;
    KSTART_FRAME StartFrame;
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
                           IN PCONTEXT ContextPointer)
{
    //PFX_SAVE_AREA FxSaveArea;
    //PFXSAVE_FORMAT FxSaveFormat;
    PKSTART_FRAME StartFrame;
    PKSWITCH_FRAME CtxSwitchFrame;
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
                      KTRAP_FRAME_LENGTH);

        /* Setup the Fx Area */
        //FxSaveArea = &InitFrame->FxSaveArea;

//            /* Get the FX Save Format Area */
//            FxSaveFormat = (PFXSAVE_FORMAT)Context->ExtendedRegisters;
//
//            /* Set an initial state */
//            FxSaveFormat->ControlWord = 0x27F;
//            FxSaveFormat->StatusWord = 0;
//            FxSaveFormat->TagWord = 0;
//            FxSaveFormat->ErrorOffset = 0;
//            FxSaveFormat->ErrorSelector = 0;
//            FxSaveFormat->DataOffset = 0;
//            FxSaveFormat->DataSelector = 0;
//            FxSaveFormat->MXCsr = 0x1F80;

        /* Set an intial NPX State */
        //Context->FloatSave.Cr0NpxState = 0;
        //FxSaveArea->Cr0NpxState = 0;
        //FxSaveArea->NpxSavedCpu = 0;

        /* Now set the context flags depending on XMM support */
        //ContextFlags |= (KeI386FxsrPresent) ? CONTEXT_EXTENDED_REGISTERS :
        //                                      CONTEXT_FLOATING_POINT;

        /* Set the Thread's NPX State */
        Thread->NpxState = 0xA;
        Thread->DispatcherHeader.NpxIrql = PASSIVE_LEVEL;

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
        TrapFrame->SegSs |= RPL_MASK;
        TrapFrame->SegDs |= RPL_MASK;
        TrapFrame->SegEs |= RPL_MASK;
        TrapFrame->Dr7 = 0;

        /* Set the previous mode as user */
        TrapFrame->PreviousMode = UserMode;

        /* Terminate the Exception Handler List */
        TrapFrame->ExceptionFrame = 0;

        /* Setup the Stack for KiThreadStartup and Context Switching */
        StartFrame = &InitFrame->StartFrame;
        CtxSwitchFrame = &InitFrame->CtxSwitchFrame;

        /* Tell the thread it will run in User Mode */
        Thread->PreviousMode = UserMode;

        /* Tell KiThreadStartup of that too */
//        StartFrame->UserThread = TRUE;
    }
    else
    {
        /* Set up the Initial Frame for the system thread */
        PKKINIT_FRAME InitFrame;
        InitFrame = (PKKINIT_FRAME)((ULONG_PTR)Thread->InitialStack -
                                    sizeof(KKINIT_FRAME));

        /* Setup the Fx Area */
        //FxSaveArea = &InitFrame->FxSaveArea;
        //RtlZeroMemory(FxSaveArea, sizeof(FX_SAVE_AREA));

        /* Check if we have Fxsr support */
        DPRINT1("FxsrPresent but did nothing\n");
//        /* Set the stub FX area */
//        FxSaveArea->U.FxArea.ControlWord = 0x27F;
//        FxSaveArea->U.FxArea.MXCsr = 0x1F80;

        /* No NPX State */
        Thread->NpxState = 0xA;

        /* Setup the Stack for KiThreadStartup and Context Switching */
        StartFrame = &InitFrame->StartFrame;
        CtxSwitchFrame = &InitFrame->CtxSwitchFrame;

        /* Tell the thread it will run in Kernel Mode */
        Thread->PreviousMode = KernelMode;

        /* Tell KiThreadStartup of that too */
//        StartFrame->UserThread = FALSE;
    }

    /* Now setup the remaining data for KiThreadStartup */
//    StartFrame->StartContext = StartContext;
//    StartFrame->StartRoutine = StartRoutine;
//    StartFrame->SystemRoutine = SystemRoutine;

    /* And set up the Context Switch Frame */
//    CtxSwitchFrame->RetAddr = KiThreadStartup;
//    CtxSwitchFrame->ApcBypassDisable = TRUE;
//    CtxSwitchFrame->ExceptionList = EXCEPTION_CHAIN_END;;

    /* Save back the new value of the kernel stack. */
    Thread->KernelStack = (PVOID)CtxSwitchFrame;

}

/* EOF */


