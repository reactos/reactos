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
static PHYSICAL_ADDRESS PcrPages[MAXIMUM_PROCESSORS];
ULONG Ke386CpuidFlags, Ke386CpuidFlags2, Ke386CpuidExFlags;
ULONG Ke386Cpuid = 300;

#define X86_EFLAGS_ID       (1 << 21)

/* FUNCTIONS *****************************************************************/

VOID INIT_FUNCTION STATIC
Ki386GetCpuId(VOID)
{
  ULONG OrigFlags, Flags, FinalFlags;
  ULONG MaxCpuidLevel;

  Ke386CpuidFlags = Ke386CpuidFlags2 =  Ke386CpuidExFlags = 0;

  /* Try to toggle the id bit in eflags. */
  __asm__ ("pushfl\n\t"
	   "popl %0\n\t"
	   : "=r" (OrigFlags));
  Flags = OrigFlags ^ X86_EFLAGS_ID;
  __asm__ ("pushl %1\n\t"
	   "popfl\n\t"
	   "pushfl\n\t"
	   "popl %0\n\t"
	   : "=r" (FinalFlags)
	   : "r" (Flags));
   if ((OrigFlags & X86_EFLAGS_ID) == (FinalFlags & X86_EFLAGS_ID))
    {
      /* No cpuid supported. */
      return;
    }
  
  /* Get maximum cpuid level supported. */
  __asm__("cpuid\n\t"
	  : "=a" (MaxCpuidLevel)
	  : "a" (0x00000000)
	  : "ebx", "ecx", "edx");       
  if (MaxCpuidLevel > 0)
    {
      /* Get the feature flags. */
      __asm__("cpuid\n\t"
	  : "=a" (Ke386Cpuid),"=d" (Ke386CpuidFlags), "=c" (Ke386CpuidFlags2)
	  : "a" (0x00000001)
	  : "ebx");       
    }

  /* Get the maximum extended cpuid level supported. */
  __asm__("cpuid\n\t"
	  : "=a" (MaxCpuidLevel)
	  : "a" (0x80000000)
	  : "ebx", "ecx", "edx");       
  if (MaxCpuidLevel > 0)
    {
      /* Get the extended feature flags. */
      __asm__("cpuid\n\t"
	  : "=d" (Ke386CpuidExFlags)
	  : "a" (0x80000001)
	  : "ebx", "ecx");       
    }
}

VOID INIT_FUNCTION
KePrepareForApplicationProcessorInit(ULONG Id)
{
  MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &PcrPages[Id]);
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
  Offset = InterlockedIncrement((LONG *)&PcrsAllocated) - 1;
  KPCR = (PKPCR)(KPCR_BASE + (Offset * PAGE_SIZE));
  MmCreateVirtualMappingForKernel((PVOID)KPCR,
				  PAGE_READWRITE,
				  PcrPages[Offset]);
  memset(KPCR, 0, PAGE_SIZE);
  KPCR->ProcessorNumber = (UCHAR)Offset;
  KPCR->Self = KPCR;
  KPCR->Irql = HIGH_LEVEL;

  /* Mark the end of the exception handler list */
  KPCR->Tib.ExceptionList = (PVOID)-1;

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

  /* Now we can enable interrupts. */
  Ke386EnableInterrupts();
}

VOID INIT_FUNCTION
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
   memset(KPCR, 0, PAGE_SIZE);
   KPCR->Self = (PKPCR)KPCR_BASE;
   KPCR->Irql = HIGH_LEVEL;
   KPCR->GDT = (PUSHORT)&KiBootGdt;
   KPCR->IDT = (PUSHORT)&KiIdt;
   KPCR->TSS = &KiBootTss;
   KPCR->ProcessorNumber = 0;
   KiPcrInitDone = 1;
   PcrsAllocated++;

   /* Mark the end of the exception handler list */
   KPCR->Tib.ExceptionList = (PVOID)-1;

   Ki386InitializeLdt();
   
   /* Get processor information. */
   Ki386GetCpuId();
}

VOID INIT_FUNCTION
KeInit2(VOID)
{
   KeInitDpc();
   KeInitializeBugCheck();
   KeInitializeDispatcher();
   KeInitializeTimerImpl();
}
