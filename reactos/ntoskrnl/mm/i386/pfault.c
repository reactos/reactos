/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/i386/pfault.c
 * PURPOSE:         Paging file functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>

#define NDEBUG
#include <internal/debug.h>

/* EXTERNS *******************************************************************/

extern VOID MmSafeCopyFromUserUnsafeStart(VOID);
extern VOID MmSafeCopyFromUserRestart(VOID);
extern VOID MmSafeCopyToUserUnsafeStart(VOID);
extern VOID MmSafeCopyToUserRestart(VOID);

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
	  Eip, Cr2, ErrorCode);
   
   if (ErrorCode & 0x4)
     {
	Mode = UserMode;
     }
   else
     {
	Mode = KernelMode;
     }
   
   if (ErrorCode & 0x1)
     {
	Status = MmAccessFault(Mode, Cr2);
     }
   else
     {
	Status = MmNotPresentFault(Mode, Cr2);
     }

   if (!NT_SUCCESS(Status) && (Mode == KernelMode) &&
       ((*Eip) >= (ULONG)MmSafeCopyFromUserUnsafeStart) &&
       ((*Eip) <= (ULONG)MmSafeCopyFromUserRestart))
     {
	(*Eip) = (ULONG)MmSafeCopyFromUserRestart;
	(*Eax) = STATUS_ACCESS_VIOLATION;
	return(STATUS_SUCCESS);
     }
   if (!NT_SUCCESS(Status) && (Mode == KernelMode) &&
       ((*Eip) >= (ULONG)MmSafeCopyToUserUnsafeStart) &&
       ((*Eip) <= (ULONG)MmSafeCopyToUserRestart))
     {
	(*Eip) = (ULONG)MmSafeCopyToUserRestart;
	(*Eax) = STATUS_ACCESS_VIOLATION;
	return(STATUS_SUCCESS);
     }
   return(Status);
}
