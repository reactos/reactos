/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/kernel.c
 * PURPOSE:         Initializes the kernel
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static ULONG HardwareMathSupport;
static ULONG x;

/* FUNCTIONS *****************************************************************/

VOID KiCheckFPU(VOID)
{
   unsigned short int status;
   int cr0;
   
   HardwareMathSupport = 0;
   
   __asm__("clts\n\t");
   __asm__("fninit\n\t");
   __asm__("fstsw %0\n\t" : "=a" (status));
   if (status != 0)
     {
	__asm__("movl %%cr0, %0\n\t" : "=g" (cr0));
	cr0 = cr0 | 0x4;
	__asm__("movl %0, %%cr0\n\t" :
		: "g" (cr0));
	DbgPrint("No FPU detected\n");
	return;
     }
   /* FIXME: Do fsetpm */
   DbgPrint("Detected FPU\n");
   HardwareMathSupport = 1;
   
   DbgPrint("Testing FPU\n");
   x = x * 6.789456;   
}

VOID KeInit1(VOID)
{
   KiCheckFPU();

   KeInitExceptions ();
   KeInitInterrupts ();
}

VOID KeInit2(VOID)
{
   KeInitDpc();
   KeInitializeBugCheck();
   KeInitializeDispatcher();
   KeInitializeTimerImpl();
}
