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

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

ULONG KiPcrInitDone = 0;
static ULONG PcrsAllocated = 0;
ULONG Ke386CpuidFlags, Ke386CpuidFlags2, Ke386CpuidExFlags;
ULONG Ke386Cpuid = 0x300;
ULONG Ke386CacheAlignment;
CHAR Ke386CpuidVendor[13] = {0,};
CHAR Ke386CpuidModel[49] = {0,};
ULONG Ke386L1CacheSize;
ULONG Ke386L2CacheSize;
BOOLEAN Ke386NoExecute = FALSE;
BOOLEAN Ke386Pae = FALSE;
BOOLEAN Ke386PaeEnabled = FALSE;

/* FUNCTIONS *****************************************************************/

VOID INIT_FUNCTION STATIC
Ki386GetCpuId(VOID)
{
   ULONG OrigFlags, Flags, FinalFlags;
   ULONG MaxCpuidLevel;
   ULONG Dummy, Ebx, Ecx, Edx;

   Ke386CpuidFlags = Ke386CpuidFlags2 =  Ke386CpuidExFlags = 0;
   Ke386CacheAlignment = 32;

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

   /* Get the vendor name and the maximum cpuid level supported. */
   Ki386Cpuid(0, &MaxCpuidLevel, (PULONG)&Ke386CpuidVendor[0], (PULONG)&Ke386CpuidVendor[8], (PULONG)&Ke386CpuidVendor[4]);
   if (MaxCpuidLevel > 0)
   { 
      /* Get the feature flags. */
      Ki386Cpuid(1, &Ke386Cpuid, &Ebx, &Ke386CpuidFlags2, &Ke386CpuidFlags);
      /* Get the cache alignment, if it is available */
      if (Ke386CpuidFlags & (1<<19))
      {
         Ke386CacheAlignment = ((Ebx >> 8) & 0xff) * 8;
      }
   }

   /* Get the maximum extended cpuid level supported. */
   Ki386Cpuid(0x80000000, &MaxCpuidLevel, &Dummy, &Dummy, &Dummy);
   if (MaxCpuidLevel > 0)
   {
      /* Get the extended feature flags. */
      Ki386Cpuid(0x80000001, &Dummy, &Dummy, &Dummy, &Ke386CpuidExFlags);
   }

   /* Get the model name. */
   if (MaxCpuidLevel >= 0x80000004)
   {
      PULONG v = (PULONG)&Ke386CpuidModel;
      Ki386Cpuid(0x80000002, v, v + 1, v + 2, v + 3);
      Ki386Cpuid(0x80000003, v + 4, v + 5, v + 6, v + 7);
      Ki386Cpuid(0x80000004, v + 8, v + 9, v + 10, v + 11);
   }

   /* Get the L1 cache size */
   if (MaxCpuidLevel >= 0x80000005)
   {
      Ki386Cpuid(0x80000005, &Dummy, &Dummy, &Ecx, &Edx);
      Ke386L1CacheSize = (Ecx >> 24)+(Edx >> 24);
      if ((Ecx & 0xff) > 0)
      {
         Ke386CacheAlignment = Ecx & 0xff;
      }
   }

   /* Get the L2 cache size */
   if (MaxCpuidLevel >= 0x80000006)
   {
      Ki386Cpuid(0x80000006, &Dummy, &Dummy, &Ecx, &Dummy);
      Ke386L2CacheSize = Ecx >> 16;
   }
}

VOID INIT_FUNCTION
KePrepareForApplicationProcessorInit(ULONG Id)
{
  DPRINT("KePrepareForApplicationProcessorInit(Id %d)\n", Id);
  PFN_TYPE PrcPfn;
  PKPCR Pcr;

  Pcr = (PKPCR)((ULONG_PTR)KPCR_BASE + Id * PAGE_SIZE);

  MmRequestPageMemoryConsumer(MC_NPPOOL, TRUE, &PrcPfn);
  MmCreateVirtualMappingForKernel((PVOID)Pcr,
				  PAGE_READWRITE,
				  &PrcPfn,
				  1);
  /*
   * Create a PCR for this processor
   */
  memset(Pcr, 0, PAGE_SIZE);
  Pcr->ProcessorNumber = Id;
  Pcr->Tib.Self = &Pcr->Tib;
  Pcr->Self = Pcr;
  Pcr->Irql = HIGH_LEVEL;

  /* Mark the end of the exception handler list */
  Pcr->Tib.ExceptionList = (PVOID)-1;

  KeInitDpc(Pcr);


  KiGdtPrepareForApplicationProcessorInit(Id);
}

VOID
KeApplicationProcessorInit(VOID)
{
  ULONG Offset;
  PKPCR Pcr;

  DPRINT("KeApplicationProcessorInit()\n");

  if (Ke386CpuidFlags & X86_FEATURE_PGE)
  {
     /* Enable global pages */
     Ke386SetCr4(Ke386GetCr4() | X86_CR4_PGE);
  }
  
  /* Enable PAE mode */
  if (Ke386CpuidFlags & X86_FEATURE_PAE)
  {
     MiEnablePAE(NULL);
  }

  Offset = InterlockedIncrement(&PcrsAllocated) - 1;
  Pcr = (PKPCR)((ULONG_PTR)KPCR_BASE + Offset * PAGE_SIZE);

  KiCheckFPU();

  /*
   * Initialize the GDT
   */
  KiInitializeGdt(Pcr);


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
KeInit1(PCHAR CommandLine, PULONG LastKernelAddress)
{
   PKPCR KPCR;
   BOOLEAN Pae = FALSE;
   BOOLEAN NoExecute = FALSE;
   PCHAR p1, p2;
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
   KPCR->Self = KPCR;
   KPCR->Irql = HIGH_LEVEL;
   KPCR->Tib.Self  = &KPCR->Tib;
   KPCR->GDT = KiBootGdt;
   KPCR->IDT = (PUSHORT)KiIdt;
   KPCR->TSS = &KiBootTss;
   KPCR->ProcessorNumber = 0;
   KiPcrInitDone = 1;
   PcrsAllocated++;

   KeInitDpc(KPCR);

   /* Mark the end of the exception handler list */
   KPCR->Tib.ExceptionList = (PVOID)-1;

   Ki386InitializeLdt();
   
   /* Get processor information. */
   Ki386GetCpuId();

   if (Ke386CpuidFlags & X86_FEATURE_PGE)
   {
      ULONG Flags;
      /* Enable global pages */
      Ke386SaveFlags(Flags);
      Ke386DisableInterrupts();
      Ke386SetCr4(Ke386GetCr4() | X86_CR4_PGE);
      Ke386RestoreFlags(Flags);
   }

   /* Search for pae and noexecute */
   p1 = (PCHAR)KeLoaderBlock.CommandLine;
   while(*p1 && (p2 = strchr(p1, '/')))
   {
      p2++;
      if (!_strnicmp(p2, "PAE", 3))
      {
	 if (p2[3] == ' ' || p2[3] == 0)
	 {
	    p2 += 3;
	    Pae = TRUE;
	 }
      }
      else if (!_strnicmp(p2, "NOEXECUTE", 9))
      {
         if (p2[9] == ' ' || p2[9] == '=' || p2[9] == 0)
	 {
	    p2 += 9;
	    NoExecute = TRUE;
	 }
      }
      p1 = p2;
   }

   /* 
    * FIXME:
    *   Make the detection of the noexecute feature more portable.
    */
   if(((Ke386Cpuid >> 8) & 0xf) == 0xf &&
      0 == strcmp("AuthenticAMD", Ke386CpuidVendor))
   {
      if (NoExecute)
      {
         ULONG Flags, l, h;
         Ke386SaveFlags(Flags);
         Ke386DisableInterrupts();

	 Ke386Rdmsr(0xc0000080, l, h);
	 l |= (1 << 11);
	 Ke386Wrmsr(0xc0000080, l, h);
	 Ke386NoExecute = TRUE;
         Ke386RestoreFlags(Flags);
      }
   }  
   else
   {
      NoExecute=FALSE;
   }

      
   /* Enable PAE mode */
   if ((Pae && (Ke386CpuidFlags & X86_FEATURE_PAE)) || NoExecute)
   {
      MiEnablePAE((PVOID*)LastKernelAddress);
      Ke386PaeEnabled = TRUE;
   }
}

VOID INIT_FUNCTION
KeInit2(VOID)
{
   KeInitializeBugCheck();
   KeInitializeDispatcher();
   KeInitializeTimerImpl();

   if (Ke386CpuidFlags & X86_FEATURE_PAE)
   {
      DPRINT1("CPU supports PAE mode\n");
      if (Ke386Pae)
      {
         DPRINT1("CPU runs in PAE mode\n");
         if (Ke386NoExecute)
         {
            DPRINT1("NoExecute is enabled\n");
         }
      }
      else
      {
         DPRINT1("CPU doesn't run in PAE mode\n");
      }
   }
   if (Ke386CpuidVendor[0])
   {
      DPRINT1("CPU Vendor: %s\n", Ke386CpuidVendor);
   }
   if (Ke386CpuidModel[0])
   {
      DPRINT1("CPU Model:  %s\n", Ke386CpuidModel);
   }

   DPRINT1("Ke386CacheAlignment: %d\n", Ke386CacheAlignment);
   if (Ke386L1CacheSize)
   {
      DPRINT1("Ke386L1CacheSize: %dkB\n", Ke386L1CacheSize);
   }
   if (Ke386L2CacheSize)
   {
      DPRINT1("Ke386L2CacheSize: %dkB\n", Ke386L2CacheSize);
   }
}

VOID INIT_FUNCTION
Ki386SetProcessorFeatures(VOID)
{
  SharedUserData->ProcessorFeatures[PF_FLOATING_POINT_PRECISION_ERRATA] = FALSE;
  SharedUserData->ProcessorFeatures[PF_FLOATING_POINT_EMULATED] = FALSE;
  SharedUserData->ProcessorFeatures[PF_COMPARE_EXCHANGE_DOUBLE] =
    (Ke386CpuidFlags & X86_FEATURE_CX8);
  SharedUserData->ProcessorFeatures[PF_MMX_INSTRUCTIONS_AVAILABLE] =
    (Ke386CpuidFlags & X86_FEATURE_MMX);
  SharedUserData->ProcessorFeatures[PF_PPC_MOVEMEM_64BIT_OK] = FALSE;
  SharedUserData->ProcessorFeatures[PF_ALPHA_BYTE_INSTRUCTIONS] = FALSE;
  SharedUserData->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE] = 
    (Ke386CpuidFlags & X86_FEATURE_SSE);
  SharedUserData->ProcessorFeatures[PF_3DNOW_INSTRUCTIONS_AVAILABLE] =
    (Ke386CpuidExFlags & X86_EXT_FEATURE_3DNOW);
  SharedUserData->ProcessorFeatures[PF_RDTSC_INSTRUCTION_AVAILABLE] =
    (Ke386CpuidFlags & X86_FEATURE_TSC);
  SharedUserData->ProcessorFeatures[PF_PAE_ENABLED] = Ke386PaeEnabled;
  SharedUserData->ProcessorFeatures[PF_XMMI64_INSTRUCTIONS_AVAILABLE] =
    (Ke386CpuidFlags & X86_FEATURE_SSE2);
}
