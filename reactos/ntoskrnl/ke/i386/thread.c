/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:              ReactOS kernel
 * FILE:                 ntoskrnl/ke/i386/thread.c
 * PURPOSE:              Architecture multitasking functions
 * PROGRAMMER:           David Welch (welch@cwcom.net)
 * REVISION HISTORY:
 *             27/06/98: Created
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define FLAG_NT (1<<14)
#define FLAG_VM (1<<17)
#define FLAG_IF (1<<9)
#define FLAG_IOPL ((1<<12)+(1<<13))

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
   if ((Context->EFlags & FLAG_IOPL) != 0 ||
       (Context->EFlags & FLAG_NT) ||
       (Context->EFlags & FLAG_VM) ||
       (!(Context->EFlags & FLAG_IF)))
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

  /*
   * Setup a stack frame for exit from the task switching routine
   */
  
  InitSize = 5 * sizeof(DWORD) + 6 * sizeof(DWORD) + 
    sizeof(FLOATING_SAVE_AREA) + sizeof(KTRAP_FRAME);
  KernelStack = (PULONG)(Thread->KernelStack - InitSize);

  /* Set up the initial frame for the return from the dispatcher. */
  KernelStack[0] = 0;      /* EDI */
  KernelStack[1] = 0;      /* ESI */
  KernelStack[2] = 0;      /* EBX */
  KernelStack[3] = 0;      /* EBP */
  KernelStack[4] = (ULONG)PsBeginThreadWithContextInternal;   /* EIP */

  /* Set up the initial values of the debugging registers. */
  KernelStack[5] = Context->Dr0;
  KernelStack[6] = Context->Dr1;
  KernelStack[7] = Context->Dr2;
  KernelStack[8] = Context->Dr3;
  KernelStack[9] = Context->Dr6;
  KernelStack[10] = Context->Dr7;

  /* Set up the initial floating point state. */
  memcpy((PVOID)&KernelStack[11], (PVOID)&Context->FloatSave,
	 sizeof(FLOATING_SAVE_AREA));

  /* Set up a trap frame from the context. */
  TrapFrame = (PKTRAP_FRAME)
    ((PBYTE)KernelStack + 11 * sizeof(DWORD) + sizeof(FLOATING_SAVE_AREA));
  TrapFrame->DebugEbp = (PVOID)Context->Ebp;
  TrapFrame->DebugEip = (PVOID)Context->Eip;
  TrapFrame->DebugArgMark = 0;
  TrapFrame->DebugPointer = 0;
  TrapFrame->TempCs = 0;
  TrapFrame->TempEip = 0;
  TrapFrame->Gs = Context->SegGs;
  TrapFrame->Es = Context->SegEs;
  TrapFrame->Ds = Context->SegDs;
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
  TrapFrame->Esp = Context->Esp;
  TrapFrame->Ss = Context->SegSs;
  /* FIXME: Should check for a v86 mode context here. */

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
  
  KernelStack = (PULONG)(Thread->KernelStack - (8*4));
  /* FIXME: Add initial floating point information */
  /* FIXME: Add initial debugging information */
  KernelStack[0] = 0;      /* EDI */
  KernelStack[1] = 0;      /* ESI */
  KernelStack[2] = 0;      /* EBX */
  KernelStack[3] = 0;      /* EBP */
  KernelStack[4] = (ULONG)PsBeginThread;   /* EIP */
  KernelStack[5] = 0;     /* Return EIP */
  KernelStack[6] = (ULONG)StartRoutine; /* First argument to PsBeginThread */
  KernelStack[7] = (ULONG)StartContext; /* Second argument to PsBeginThread */
  Thread->KernelStack = (VOID*)KernelStack;

  return(STATUS_SUCCESS);
}

/* EOF */
