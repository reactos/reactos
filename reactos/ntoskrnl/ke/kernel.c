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
#include <internal/mm.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static ULONG HardwareMathSupport;

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
	__asm__("movl %%cr0, %0\n\t" : "=a" (cr0));
	cr0 = cr0 | 0x4;
	__asm__("movl %0, %%cr0\n\t" :
		: "a" (cr0));
	DbgPrint("No FPU detected\n");
	return;
     }
   /* FIXME: Do fsetpm */
   HardwareMathSupport = 1;   
}

VOID KeInit1(VOID)
{
   KiCheckFPU();

   KeInitExceptions ();
   KeInitInterrupts ();
}

VOID KeInit2(VOID)
{
   PVOID PcrPage;
   
   KeInitDpc();
   KeInitializeBugCheck();
   KeInitializeDispatcher();
   KeInitializeTimerImpl();
   
   /*
    * Initialize the PCR region. 
    * FIXME: This should be per-processor.
    */
   PcrPage = MmAllocPage(0);
   if (PcrPage == NULL)
     {
	DPRINT1("No memory for PCR page\n");
	KeBugCheck(0);
     }
   MmCreateVirtualMapping(NULL,
			  (PVOID)KPCR_BASE,
			  PAGE_READWRITE,
			  (ULONG)PcrPage);
   memset((PVOID)KPCR_BASE, 0, 4096);
}
