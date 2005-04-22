/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/thread.c
 * PURPOSE:         i386 Thread Context Creation
 * 
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>
                                    
typedef struct _KSHARED_CTXSWITCH_FRAME {
    ULONG Esp0;
    PVOID ExceptionList;
    PVOID RetEip;
} KSHARED_CTXSWITCH_FRAME, *PKSHARED_CTXSWITCH_FRAME;

typedef struct _KSTART_FRAME {
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
                           PCONTEXT Context)
{
    PFX_SAVE_AREA FxSaveArea;
    PKSTART_FRAME StartFrame;
    PKSHARED_CTXSWITCH_FRAME CtxSwitchFrame;
    PKTRAP_FRAME TrapFrame = NULL;
        
    /* Check if this is a With-Context Thread */
    DPRINT("Ke386InitThreadContext\n");
    if (Context) {
    
        /* Set up the Initial Frame */
        PKUINIT_FRAME InitFrame;
        InitFrame = (PKUINIT_FRAME)((ULONG_PTR)Thread->InitialStack - sizeof(KUINIT_FRAME));
        DPRINT("Setting up a user-mode thread with the Frame at: %x\n", InitFrame);
                
        /* Setup the Fx Area */
        FxSaveArea = &InitFrame->FxSaveArea;
        DPRINT("Fx Save Area: %x\n", FxSaveArea);
    
        /* Setup the Initial Fx State */
        if (KiContextToFxSaveArea(FxSaveArea, Context)) {

            Thread->NpxState = NPX_STATE_VALID;
        
        } else {
            
            Thread->NpxState = NPX_STATE_INVALID;
        }
        
        /* Setup the Trap Frame */
        TrapFrame = &InitFrame->TrapFrame;
        DPRINT("TrapFrame: %x\n", TrapFrame);
               
        /* Set up a trap frame from the context. */
        TrapFrame->DebugEbp = (PVOID)Context->Ebp;
        TrapFrame->DebugEip = (PVOID)Context->Eip;
        TrapFrame->DebugArgMark = 0;
        TrapFrame->DebugPointer = 0;
        TrapFrame->TempCs = 0;
        TrapFrame->TempEip = 0;
        TrapFrame->Gs = (USHORT)Context->SegGs;
        TrapFrame->Es = (USHORT)Context->SegEs;
        TrapFrame->Ds = (USHORT)Context->SegDs;
        TrapFrame->Edx = Context->Edx;
        TrapFrame->Ecx = Context->Ecx;
        TrapFrame->Eax = Context->Eax;
        TrapFrame->PreviousMode = UserMode;
        TrapFrame->ExceptionList = (PVOID)0xFFFFFFFF;
        TrapFrame->Fs = TEB_SELECTOR;
        TrapFrame->Edi = Context->Edi;
        TrapFrame->Esi = Context->Esi;
        TrapFrame->Ebx = Context->Ebx;
        TrapFrame->Ebp = Context->Ebp;
        TrapFrame->ErrorCode = 0;
        TrapFrame->Cs = Context->SegCs;
        TrapFrame->Eip = Context->Eip;
        TrapFrame->Eflags = Context->EFlags | X86_EFLAGS_IF;
        TrapFrame->Eflags &= ~(X86_EFLAGS_VM | X86_EFLAGS_NT | X86_EFLAGS_IOPL);
        TrapFrame->Esp = Context->Esp;
        TrapFrame->Ss = (USHORT)Context->SegSs;
        
        /* Setup the Stack for KiThreadStartup and Context Switching */
        StartFrame = &InitFrame->StartFrame;
        CtxSwitchFrame = &InitFrame->CtxSwitchFrame;
        DPRINT("StartFrame: %x\n", StartFrame);
        DPRINT("CtxSwitchFrame: %x\n", CtxSwitchFrame);
       
        /* Tell the thread it will run in User Mode */
        Thread->PreviousMode = UserMode;
        
        /* Tell KiThreadStartup of that too */
        StartFrame->UserThread = TRUE;
    
    } else {
        
        /* No context Thread, meaning System Thread */
       
        /* Set up the Initial Frame */
        PKKINIT_FRAME InitFrame;
        InitFrame = (PKKINIT_FRAME)((ULONG_PTR)Thread->InitialStack - sizeof(KKINIT_FRAME));
        DPRINT("Setting up a kernel thread with the Frame at: %x\n", InitFrame);
                
        /* Setup the Fx Area */
        FxSaveArea = &InitFrame->FxSaveArea;
        DPRINT("Fx Save Area: %x\n", FxSaveArea);
        RtlZeroMemory(FxSaveArea, sizeof(FX_SAVE_AREA));
        Thread->NpxState = NPX_STATE_INVALID;
        
        /* Setup the Stack for KiThreadStartup and Context Switching */
        StartFrame = &InitFrame->StartFrame;
        CtxSwitchFrame = &InitFrame->CtxSwitchFrame;
        DPRINT("StartFrame: %x\n", StartFrame);
        DPRINT("CtxSwitchFrame: %x\n", CtxSwitchFrame);
        
        /* Tell the thread it will run in Kernel Mode */
        Thread->PreviousMode = KernelMode;
        
        /* Tell KiThreadStartup of that too */
        StartFrame->UserThread = FALSE;
    }
    
    /* Now setup the remaining data for KiThreadStartup */
    DPRINT("Settingup the Start and Context Frames\n");
    StartFrame->StartContext = StartContext;
    StartFrame->StartRoutine = StartRoutine;
    StartFrame->SystemRoutine = SystemRoutine;
    
    /* And set up the Context Switch Frame */
    CtxSwitchFrame->RetEip = KiThreadStartup;
    CtxSwitchFrame->Esp0 = (ULONG)Thread->InitialStack - sizeof(FX_SAVE_AREA);
    CtxSwitchFrame->ExceptionList = (PVOID)0xFFFFFFFF;
        
    /* Save back the new value of the kernel stack. */
    Thread->KernelStack = (PVOID)CtxSwitchFrame;
    DPRINT("Final Kernel Stack: %x \n", CtxSwitchFrame);
    return;
}

/* EOF */

