/* $Id:$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/thread.c
 * PURPOSE:         Architecture multitasking functions
 * 
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS 
Ki386ValidateUserContext(PCONTEXT Context)
/*
 * FUNCTION: Validates a processor context
 * ARGUMENTS:
 *        Context = Context to validate
 * RETURNS: Status
 * NOTE: This only validates the context as not violating system security, it
 * doesn't guararantee the thread won't crash at some point
 * NOTE2: This relies on there only being two selectors which can access 
 * system space
 */
{
   if (Context->Eip >= KERNEL_BASE)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (Context->SegCs == KERNEL_CS)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (Context->SegDs == KERNEL_DS)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (Context->SegEs == KERNEL_DS)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (Context->SegFs == KERNEL_DS)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (Context->SegGs == KERNEL_DS)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if ((Context->EFlags & X86_EFLAGS_IOPL) != 0 ||
       (Context->EFlags & X86_EFLAGS_NT) ||
       (Context->EFlags & X86_EFLAGS_VM) ||
       (!(Context->EFlags & X86_EFLAGS_IF)))
     {
        return(STATUS_UNSUCCESSFUL);
     }
   return(STATUS_SUCCESS);
}

NTSTATUS
Ke386InitThreadWithContext(PKTHREAD Thread, PCONTEXT Context)
{
  PULONG KernelStack;
  ULONG InitSize;
  PKTRAP_FRAME TrapFrame;
  PFX_SAVE_AREA FxSaveArea;

  /*
   * Setup a stack frame for exit from the task switching routine
   */
  
  InitSize = 6 * sizeof(DWORD) + sizeof(DWORD) + 6 * sizeof(DWORD) +
           + sizeof(KTRAP_FRAME) + sizeof (FX_SAVE_AREA);
  KernelStack = (PULONG)((char*)Thread->KernelStack - InitSize);

  /* Set up the initial frame for the return from the dispatcher. */
  KernelStack[0] = (ULONG)Thread->InitialStack - sizeof(FX_SAVE_AREA);  /* TSS->Esp0 */
  KernelStack[1] = 0;      /* EDI */
  KernelStack[2] = 0;      /* ESI */
  KernelStack[3] = 0;      /* EBX */
  KernelStack[4] = 0;      /* EBP */
  KernelStack[5] = (ULONG)&PsBeginThreadWithContextInternal;   /* EIP */

  /* Save the context flags. */
  KernelStack[6] = Context->ContextFlags;

  /* Set up the initial values of the debugging registers. */
  KernelStack[7] = Context->Dr0;
  KernelStack[8] = Context->Dr1;
  KernelStack[9] = Context->Dr2;
  KernelStack[10] = Context->Dr3;
  KernelStack[11] = Context->Dr6;
  KernelStack[12] = Context->Dr7;

  /* Set up a trap frame from the context. */
  TrapFrame = (PKTRAP_FRAME)(&KernelStack[13]);
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
  /* FIXME: Should check for a v86 mode context here. */

  /* Set up the initial floating point state. */
  /* FIXME: Do we have to zero the FxSaveArea or is it already? */
  FxSaveArea = (PFX_SAVE_AREA)((ULONG_PTR)KernelStack + InitSize - sizeof(FX_SAVE_AREA));
  if (KiContextToFxSaveArea(FxSaveArea, Context))
    {
      Thread->NpxState = NPX_STATE_VALID;
    }
  else
    {
      Thread->NpxState = NPX_STATE_INVALID;
    }

  /* Save back the new value of the kernel stack. */
  Thread->KernelStack = (PVOID)KernelStack;

  return(STATUS_SUCCESS);
}

NTSTATUS
Ke386InitThread(PKTHREAD Thread, 
		PKSTART_ROUTINE StartRoutine, 
		PVOID StartContext)
     /*
      * Initialize a thread
      */
{
  PULONG KernelStack;

  /*
   * Setup a stack frame for exit from the task switching routine
   */

  KernelStack = (PULONG)((char*)Thread->KernelStack - (9 * sizeof(DWORD)) - sizeof(FX_SAVE_AREA));
  KernelStack[0] = (ULONG)Thread->InitialStack - sizeof(FX_SAVE_AREA);  /* TSS->Esp0 */
  KernelStack[1] = 0;      /* EDI */
  KernelStack[2] = 0;      /* ESI */
  KernelStack[3] = 0;      /* EBX */
  KernelStack[4] = 0;      /* EBP */
  KernelStack[5] = (ULONG)&PsBeginThread;   /* EIP */
  KernelStack[6] = 0;     /* Return EIP */
  KernelStack[7] = (ULONG)StartRoutine; /* First argument to PsBeginThread */
  KernelStack[8] = (ULONG)StartContext; /* Second argument to PsBeginThread */
  Thread->KernelStack = (VOID*)KernelStack;

  /*
   * Setup FPU state
   */
  Thread->NpxState = NPX_STATE_INVALID;

  return(STATUS_SUCCESS);
}

/* EOF */

