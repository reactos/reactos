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
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/kernel.c
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
#include <internal/i386/fpu.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

ULONG KiPcrInitDone = 0;
static ULONG PcrsAllocated = 0;
static PVOID PcrPages[MAXIMUM_PROCESSORS];

/* FUNCTIONS *****************************************************************/

VOID
KePrepareForApplicationProcessorInit(ULONG Id)
{
  PcrPages[Id] = MmAllocPage(0);
  KiGdtPrepareForApplicationProcessorInit(Id);
}

VOID
KeApplicationProcessorInit(VOID)
{
  PKPCR KPCR;
  ULONG Offset;

  /*
   * Create a PCR for this processor
   */
  Offset = InterlockedIncrement(&PcrsAllocated) - 1;
  KPCR = (PKPCR)(KPCR_BASE + (Offset * PAGESIZE));
  MmCreateVirtualMappingForKernel((PVOID)KPCR,
				  PAGE_READWRITE,
				  (ULONG)PcrPages[Offset]);
  memset(KPCR, 0, PAGESIZE);
  KPCR->ProcessorNumber = Offset;
  KPCR->Self = KPCR;
  KPCR->Irql = HIGH_LEVEL;

  /*
   * Initialize the GDT
   */
  KiInitializeGdt(KPCR);
  
  /*
   * It is now safe to process interrupts
   */
  KeLowerIrql(DISPATCH_LEVEL);

  /*
   * Initialize the TSS
   */
  Ki386ApplicationProcessorInitializeTSS();

  /*
   * Initialize a default LDT
   */
  Ki386InitializeLdt();

  __asm__ __volatile__ ("sti\n\t");
}

VOID 
KeInit1(VOID)
{
   PKPCR KPCR;
   extern USHORT KiBootGdt[];
   extern KTSS KiBootTss;

   KiCheckFPU();
   
   KiInitializeGdt (NULL);
   Ki386BootInitializeTSS();
   KeInitExceptions ();
   KeInitInterrupts ();

   /* 
    * Initialize the initial PCR region. We can't allocate a page
    * with MmAllocPage() here because MmInit1() has not yet been
    * called, so we use a predefined page in low memory 
    */
   KPCR = (PKPCR)KPCR_BASE;
   memset(KPCR, 0, PAGESIZE);
   KPCR->Self = (PKPCR)KPCR_BASE;
   KPCR->Irql = HIGH_LEVEL;
   KPCR->GDT = (PUSHORT)&KiBootGdt;
   KPCR->IDT = (PUSHORT)&KiIdt;
   KPCR->TSS = &KiBootTss;
   KPCR->ProcessorNumber = 0;
   KiPcrInitDone = 1;
   PcrsAllocated++;

   Ki386InitializeLdt();

   __asm__ __volatile__ ("sti\n\t");
}

VOID 
KeInit2(VOID)
{
   KeInitDpc();
   KeInitializeBugCheck();
   KeInitializeDispatcher();
   KeInitializeTimerImpl();
}
