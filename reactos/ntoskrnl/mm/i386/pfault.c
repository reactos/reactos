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

/* FUNCTIONS *****************************************************************/

NTSTATUS MmPageFault(ULONG Cs,
		     PULONG Eip,
		     PULONG Eax,
		     ULONG Cr2,
		     ULONG ErrorCode)
{
   KPROCESSOR_MODE Mode;
   
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
	return(MmAccessFault(Mode, Cr2));
     }
   else
     {
	return(MmNotPresentFault(Mode, Cr2));
     }
}
