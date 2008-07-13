/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ke/arm/exp.c
 * PURPOSE:         Implements exception helper routines for ARM machines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/


/* FUNCTIONS ******************************************************************/

VOID
NTAPI
KeContextToTrapFrame(IN PCONTEXT Context,
                     IN OUT PKEXCEPTION_FRAME ExceptionFrame,
                     IN OUT PKTRAP_FRAME TrapFrame,
                     IN ULONG ContextFlags,
                     IN KPROCESSOR_MODE PreviousMode)
{
    while (TRUE);
    return;
}

VOID
NTAPI
KeTrapFrameToContext(IN PKTRAP_FRAME TrapFrame,
                     IN PKEXCEPTION_FRAME ExceptionFrame,
                     IN OUT PCONTEXT Context)
{
    while (TRUE);
    return; 
}

VOID
NTAPI
KiDispatchException(IN PEXCEPTION_RECORD ExceptionRecord,
                    IN PKEXCEPTION_FRAME ExceptionFrame,
                    IN PKTRAP_FRAME TrapFrame,
                    IN KPROCESSOR_MODE PreviousMode,
                    IN BOOLEAN FirstChance)
{
    CONTEXT Context;
    
    //
    // Increase number of Exception Dispatches
    //
    KeGetCurrentPrcb()->KeExceptionDispatchCount++;
    
    //
    // Set the context flags
    //
    Context.ContextFlags = CONTEXT_FULL;
    
    //
    // FIXME: Fuck floating point
    //
    
    //
    // Get a Context
    //
    KeTrapFrameToContext(TrapFrame, ExceptionFrame, &Context);
    
    //
    // Look at our exception code
    //
    switch (ExceptionRecord->ExceptionCode)
    {
        //
        // Breakpoint
        //
        case STATUS_BREAKPOINT:
            
            //
            // Decrement PC by one
            //
            Context.Pc--;
            break;
            
        //
        // Internal exception
        //
        case KI_EXCEPTION_ACCESS_VIOLATION:
            
            //
            // Set correct code
            //
            ExceptionRecord->ExceptionCode = STATUS_ACCESS_VIOLATION;
            break;
    }
       
    //
    // Handle kernel-mode first, it's simpler
    //
    if (PreviousMode == KernelMode)
    {
        //
        // Check if this is a first-chance exception
        //
        if (FirstChance == TRUE)
        {
            //
            // Break into the debugger for the first time
            //
            if (KiDebugRoutine(TrapFrame,
                               ExceptionFrame,
                               ExceptionRecord,
                               &Context,
                               PreviousMode,
                               FALSE))
            {
                //
                // Exception was handled
                //
                goto Handled;
            }
            
            //
            // If the Debugger couldn't handle it, dispatch the exception
            //
            if (RtlDispatchException(ExceptionRecord, &Context)) goto Handled;
        }
        
        //
        // This is a second-chance exception, only for the debugger
        //
        if (KiDebugRoutine(TrapFrame,
                           ExceptionFrame,
                           ExceptionRecord,
                           &Context,
                           PreviousMode,
                           TRUE))
        {
            //
            // Exception was handled
            //
            goto Handled;
        }
        
        //
        // Third strike; you're out
        //
        KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED,
                     ExceptionRecord->ExceptionCode,
                     (ULONG_PTR)ExceptionRecord->ExceptionAddress,
                     (ULONG_PTR)TrapFrame,
                     0);
    }
    else
    {
        //
        // FIXME: User mode
        //
        ASSERT(FALSE);
    }
    
Handled:
    //
    // Convert the context back into Trap/Exception Frames
    //
    KeContextToTrapFrame(&Context,
                         ExceptionFrame,
                         TrapFrame,
                         Context.ContextFlags,
                         PreviousMode);
}
