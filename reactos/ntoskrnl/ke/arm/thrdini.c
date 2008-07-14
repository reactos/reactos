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
NTAPI
KeArmInitThreadWithContext(IN PKTHREAD Thread,
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
        ExceptionFrame->Lr = 0;
        
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
    CtxSwitchFrame->Lr = (ULONG)KiThreadStartup;
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
