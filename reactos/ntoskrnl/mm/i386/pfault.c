/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/i386/pfault.c
 * PURPOSE:         Paging file functions
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* EXTERNS *******************************************************************/

extern VOID MmSafeReadPtrStart(VOID);
extern VOID MmSafeReadPtrEnd(VOID);


extern BOOLEAN Mmi386MakeKernelPageTableGlobal(PVOID Address);
extern ULONG KiKernelTrapHandler(PKTRAP_FRAME Tf, ULONG ExceptionNr, PVOID Cr2);

extern BOOLEAN Ke386NoExecute;

/* FUNCTIONS *****************************************************************/

ULONG KiPageFaultHandler(PKTRAP_FRAME Tf, ULONG ExceptionNr)
{
   ULONG_PTR cr2;
   NTSTATUS Status;
   KPROCESSOR_MODE Mode;
   
   ASSERT(ExceptionNr == 14);
   
   /* Store the exception number in an unused field in the trap frame. */
   Tf->DebugArgMark = (PVOID)14;

   /* get the faulting address */
   cr2 = Ke386GetCr2();
   Tf->DebugPointer = (PVOID)cr2;

   /* it's safe to enable interrupts after cr2 has been saved */
   if (Tf->Eflags & (X86_EFLAGS_VM|X86_EFLAGS_IF))
   {
      Ke386EnableInterrupts();
   }

   if (cr2 >= (ULONG_PTR)MmSystemRangeStart)
   {
      /* check for an invalid page directory in kernel mode */
      if (!(Tf->ErrorCode & 0x5) && Mmi386MakeKernelPageTableGlobal((PVOID)cr2))
      {
         return 0;
      }

      /* check for non executable memory in kernel mode */
      if (Ke386NoExecute && Tf->ErrorCode & 0x10)
      {
         KEBUGCHECKWITHTF(ATTEMPTED_EXECUTE_OF_NOEXECUTE_MEMORY, 0, 0, 0, 0, Tf);
      }
   }

   Mode = Tf->ErrorCode & 0x4 ? UserMode : KernelMode;

   /* handle the fault */
   if (Tf->ErrorCode & 0x1)
   {
      Status = MmAccessFault(Mode, cr2, FALSE);
   }
   else
   {
      Status = MmNotPresentFault(Mode, cr2, FALSE);
   }
   
   /* handle the return for v86 mode */
   if (Tf->Eflags & X86_EFLAGS_VM)
   {
      if (!NT_SUCCESS(Status))
      {
         /* FIXME: This should use ->VdmObjects */
         if(!KeGetCurrentProcess()->Unused)
         {
            *((PKV86M_TRAP_FRAME)Tf)->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
	 }
         return 1;
      }
      return 0;
   }

   if (Mode == KernelMode)
   {
      if (!NT_SUCCESS(Status))
      {
         if (Tf->Eip >= (ULONG_PTR)MmSafeReadPtrStart &&
                  Tf->Eip < (ULONG_PTR)MmSafeReadPtrEnd)
         {
            Tf->Eip = (ULONG_PTR)MmSafeReadPtrEnd;
            Tf->Eax = 0;
            return 0;
         }
      }
   }
   else
   {
      if (KeGetCurrentThread()->ApcState.UserApcPending)
      {
         KIRQL oldIrql;
      
         KeRaiseIrql(APC_LEVEL, &oldIrql);
         KiDeliverApc(UserMode, NULL, NULL);
         KeLowerIrql(oldIrql);
      }
   }

   if (NT_SUCCESS(Status))
   {
      return 0;
   }

   /*
    * Handle user exceptions differently
    */
   if (Mode == KernelMode)
   {
      return(KiKernelTrapHandler(Tf, 14, (PVOID)cr2));
   }
   else
   {
      return(KiUserTrapHandler(Tf, 14, (PVOID)cr2));
   }
}

