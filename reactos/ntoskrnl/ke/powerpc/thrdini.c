/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/powerpc/thread.c
 * PURPOSE:         i386 Thread Context Creation
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 *                  arty (ppc adaptation)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
//#define NDEBUG
#include <debug.h>
#include <ndk/powerpc/ketypes.h>
#include <ppcmmu/mmu.h>

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
    KTRAP_FRAME TrapFrame;
    FX_SAVE_AREA FxSaveArea;
} KKINIT_FRAME, *PKKINIT_FRAME;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KePPCInitThreadWithContext(IN PKTHREAD Thread,
                           IN PKSYSTEM_ROUTINE SystemRoutine,
                           IN PKSTART_ROUTINE StartRoutine,
                           IN PVOID StartContext,
                           IN PCONTEXT ContextPointer)
{
    PFX_SAVE_AREA FxSaveArea;
    PKSTART_FRAME StartFrame;
    PKSWITCHFRAME CtxSwitchFrame;
    PKTRAP_FRAME TrapFrame;
    CONTEXT LocalContext;
    PCONTEXT Context = NULL;
    ppc_map_info_t pagemap[16];
    PETHREAD EThread = (PETHREAD)Thread;
    PEPROCESS Process = EThread->ThreadsProcess;
    ULONG ContextFlags, i, pmsize = sizeof(pagemap) / sizeof(pagemap[0]);
    
    DPRINT("Thread: %08x ContextPointer: %08x SystemRoutine: %08x StartRoutine: %08x StartContext: %08x\n",
           Thread,
           ContextPointer,
           SystemRoutine,
           StartRoutine,
           StartContext);

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

        /* Set the previous mode as user */
        TrapFrame->PreviousMode = UserMode;

        /* Terminate the Exception Handler List */
        RtlZeroMemory(TrapFrame->ExceptionRecord, sizeof(TrapFrame->ExceptionRecord));

        /* Setup the Stack for KiThreadStartup and Context Switching */
        StartFrame = &InitFrame->StartFrame;
        CtxSwitchFrame = &InitFrame->CtxSwitchFrame;

        /* Tell the thread it will run in User Mode */
        Thread->PreviousMode = UserMode;

        /* Tell KiThreadStartup of that too */
        StartFrame->UserThread = TRUE;

        Thread->TrapFrame = TrapFrame;

        DPRINT("Thread %08x Iar %08x Msr %08x Gpr1 %08x Gpr3 %08x\n",
               Thread,
               TrapFrame->Iar,
               TrapFrame->Msr,
               TrapFrame->Gpr1,
               TrapFrame->Gpr3);
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

        /* Setup the Stack for KiThreadStartup and Context Switching */
        StartFrame = &InitFrame->StartFrame;
        CtxSwitchFrame = &InitFrame->CtxSwitchFrame;

        /* Tell the thread it will run in Kernel Mode */
        Thread->PreviousMode = KernelMode;

        /* Tell KiThreadStartup of that too */
        StartFrame->UserThread = FALSE;

        /* Setup the Trap Frame */
        TrapFrame = &InitFrame->TrapFrame;
        Thread->TrapFrame = TrapFrame;

        TrapFrame->OldIrql = PASSIVE_LEVEL;
        TrapFrame->Iar = (ULONG)SystemRoutine;
        TrapFrame->Msr = 0xb030;
        TrapFrame->Gpr1 = ((ULONG)&InitFrame->StartFrame) - 0x200;
        TrapFrame->Gpr3 = (ULONG)StartRoutine;
        TrapFrame->Gpr4 = (ULONG)StartContext;
        __asm__("mr %0,13" : "=r" (((PULONG)&TrapFrame->Gpr0)[13]));

        DPRINT("Thread %08x Iar %08x Msr %08x Gpr1 %08x Gpr3 %08x\n",
               Thread,
               TrapFrame->Iar,
               TrapFrame->Msr,
               TrapFrame->Gpr1,
               TrapFrame->Gpr3);
    }

    /* Now setup the remaining data for KiThreadStartup */
    StartFrame->StartContext = StartContext;
    StartFrame->StartRoutine = StartRoutine;
    StartFrame->SystemRoutine = SystemRoutine;

    /* And set up the Context Switch Frame */
    CtxSwitchFrame->RetAddr = KiThreadStartup;
    CtxSwitchFrame->ApcBypassDisable = TRUE;
    CtxSwitchFrame->ExceptionList = EXCEPTION_CHAIN_END;;

    /* Save back the new value of the kernel stack. */
    Thread->KernelStack = (PVOID)CtxSwitchFrame;

    /* If we're the first thread of the new process, copy the top 16 pages
     * from process 0 */
    if (Process && IsListEmpty(&Process->ThreadListHead))
    {
        DPRINT("First Thread in Process %x\n", Process);
        MmuAllocVsid((ULONG)Process->UniqueProcessId, 0xff);
        
        for (i = 0; i < pmsize; i++)
        {
            pagemap[i].proc = 0;
            pagemap[i].addr = 0x7fff0000 + (i * PAGE_SIZE);
        }
        
        MmuInqPage(pagemap, pmsize);
        
        for (i = 0; i < pmsize; i++)
        {
            if (pagemap[i].phys)
            {
                pagemap[i].proc = (ULONG)Process->UniqueProcessId;
                pagemap[i].phys = 0;
                MmuMapPage(&pagemap[i], 1);
                DPRINT("Added map to the new process: P %08x A %08x\n",
                       pagemap[i].proc, pagemap[i].addr);
            }
        }
        
        DPRINT("Did additional aspace setup in the new process\n");
    }
}

/* EOF */


