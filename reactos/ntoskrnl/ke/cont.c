/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/cont.c
 * PURPOSE:         Continues from the specified context
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 02/10/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID KeContinue(PCONTEXT Context)
/*
 * FUNCTION: Continues from the specified processor context
 * NOTE: This function doesn't return 
 */
{
   _
   __asm__("movl %0,%%eax\n\t"
	   "movw $"STR(USER_DS)",%%bx\n\t"
	   "movw %%bx,%%ds\n\t"
	   "movw %%bx,%%es\n\t"
	   "movw %%bx,%%fs\n\t"
	   "movw %%bx,%%gs\n\t"
	   "pushl $"STR(USER_DS)"\n\t"
	   "pushl $0x2000\n\t"                  // ESP
	   "pushl $0x202\n\t"                     // EFLAGS
	   "pushl $"STR(USER_CS)"\n\t"        // CS
	   "pushl %%eax\n\t"                  // EIP
	   "iret\n\t"
          : /* no output */
          : "d" (Eip));          
}
