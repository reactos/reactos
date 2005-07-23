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

extern ULONG MmGlobalKernelPageDirectory[1024];

BOOLEAN
Mmi386MakeKernelPageTableGlobal(PVOID Address);

/* FUNCTIONS *****************************************************************/

NTSTATUS MmPageFault(ULONG Cs,
                     PULONG Eip,
                     PULONG Eax,
                     ULONG Cr2,
                     ULONG ErrorCode)
{
   KPROCESSOR_MODE Mode;
   NTSTATUS Status;

   DPRINT("MmPageFault(Eip %x, Cr2 %x, ErrorCode %x)\n",
          *Eip, Cr2, ErrorCode);

   if (ErrorCode & 0x4)
   {
      Mode = UserMode;
   }
   else
   {
      Mode = KernelMode;
   }

   if (Mode == KernelMode && Cr2 >= (ULONG_PTR)MmSystemRangeStart &&
         Mmi386MakeKernelPageTableGlobal((PVOID)Cr2))
   {
      return(STATUS_SUCCESS);
   }

   if (ErrorCode & 0x1)
   {
      Status = MmAccessFault(Mode, Cr2, FALSE);
   }
   else
   {
      Status = MmNotPresentFault(Mode, Cr2, FALSE);
   }

   if (Mode == UserMode && KeGetCurrentThread()->ApcState.UserApcPending)
   {
      KIRQL oldIrql;

      KeRaiseIrql(APC_LEVEL, &oldIrql);
      KiDeliverApc(UserMode, NULL, NULL);
      KeLowerIrql(oldIrql);
   }

   if (!NT_SUCCESS(Status) && (Mode == KernelMode) &&
         ((*Eip) >= (ULONG_PTR)MmSafeReadPtrStart) &&
         ((*Eip) <= (ULONG_PTR)MmSafeReadPtrEnd))
   {
      (*Eip) = (ULONG_PTR)MmSafeReadPtrEnd;
      (*Eax) = 0;
      return(STATUS_SUCCESS);
   }

   return(Status);
}
