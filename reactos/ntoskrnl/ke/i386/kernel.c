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
static ULONG Ke386CpuidFlags2, Ke386CpuidExFlags;
ULONG Ke386CacheAlignment;
CHAR Ke386CpuidModel[49] = {0,};
ULONG Ke386L1CacheSize;
BOOLEAN Ke386NoExecute = FALSE;
BOOLEAN Ke386Pae = FALSE;
BOOLEAN Ke386PaeEnabled = FALSE;
BOOLEAN Ke386GlobalPagesEnabled = FALSE;

/* FUNCTIONS *****************************************************************/

VOID INIT_FUNCTION STATIC
Ki386GetCpuId(VOID)
{
   ULONG OrigFlags, Flags, FinalFlags;
   ULONG MaxCpuidLevel;
   ULONG Dummy, Eax, Ebx, Ecx, Edx;
   PKPCR Pcr = KeGetCurrentKPCR();

   Ke386CpuidFlags2 =  Ke386CpuidExFlags = 0;
   Ke386CacheAlignment = 32;

   /* Try to toggle the id bit in eflags. */
   Ke386SaveFlags(OrigFlags);
   Flags = OrigFlags ^ X86_EFLAGS_ID;
   Ke386RestoreFlags(Flags);
   Ke386SaveFlags(FinalFlags);
   if ((OrigFlags & X86_EFLAGS_ID) == (FinalFlags & X86_EFLAGS_ID))
   {
      /* No cpuid supported. */
      Pcr->PrcbData.CpuID = FALSE;
      Pcr->PrcbData.CpuType = 3;
      return;
   }
   Pcr->PrcbData.CpuID = TRUE;

   /* Get the vendor name and the maximum cpuid level supported. */
   Ki386Cpuid(0, &MaxCpuidLevel, (PULONG)&Pcr->PrcbData.VendorString[0], (PULONG)&Pcr->PrcbData.VendorString[8], (PULONG)&Pcr->PrcbData.VendorString[4]);
   if (MaxCpuidLevel > 0)
   { 
      /* Get the feature flags. */
      Ki386Cpuid(1, &Eax, &Ebx, &Ke386CpuidFlags2, &Pcr->PrcbData.FeatureBits);
      /* Get the cache alignment, if it is available */
      if (Pcr->PrcbData.FeatureBits & (1<<19))
      {
         Ke386CacheAlignment = ((Ebx >> 8) & 0xff) * 8;
      }
      Pcr->PrcbData.CpuType = (Eax >> 8) & 0xf;
      Pcr->PrcbData.CpuStep = (Eax & 0xf) | ((Eax << 4) & 0xf00);
   }
   else
   {
      Pcr->PrcbData.CpuType = 4;
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
      PULONG v = (PULONG)Ke386CpuidModel;
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
      Pcr->L2CacheSize = Ecx >> 16;
   }
}

VOID INIT_FUNCTION
KePrepareForApplicationProcessorInit(ULONG Id)
{
  DPRINT("KePrepareForApplicationProcessorInit(Id %d)\n", Id);
  PFN_TYPE PrcPfn;
  PKPCR Pcr;
  PKPCR BootPcr;

  BootPcr = (PKPCR)KPCR_BASE;
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
  Pcr->Irql = SYNCH_LEVEL;

  Pcr->PrcbData.MHz = BootPcr->PrcbData.MHz;
  Pcr->StallScaleFactor = BootPcr->StallScaleFactor; 

  /* Mark the end of the exception handler list */
  Pcr->Tib.ExceptionList = (PVOID)-1;

  KiGdtPrepareForApplicationProcessorInit(Id);
}

VOID
KeApplicationProcessorInit(VOID)
{
  ULONG Offset;
  PKPCR Pcr;

  DPRINT("KeApplicationProcessorInit()\n");

  if (Ke386GlobalPagesEnabled)
  {
     /* Enable global pages */
     Ke386SetCr4(Ke386GetCr4() | X86_CR4_PGE);
  }
  

  Offset = InterlockedIncrement(&PcrsAllocated) - 1;
  Pcr = (PKPCR)((ULONG_PTR)KPCR_BASE + Offset * PAGE_SIZE);

  /*
   * Initialize the GDT
   */
  KiInitializeGdt(Pcr);

  /* Get processor information. */
  Ki386GetCpuId();

  /* Check FPU/MMX/SSE support. */
  KiCheckFPU();

  KeInitDpc(Pcr);

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

   /*
    * Initialize the initial PCR region. We can't allocate a page
    * with MmAllocPage() here because MmInit1() has not yet been
    * called, so we use a predefined page in low memory 
    */

   KPCR = (PKPCR)KPCR_BASE;
   memset(KPCR, 0, PAGE_SIZE);
   KPCR->Self = KPCR;
   KPCR->Irql = SYNCH_LEVEL;
   KPCR->Tib.Self  = &KPCR->Tib;
   KPCR->GDT = KiBootGdt;
   KPCR->IDT = (PUSHORT)KiIdt;
   KPCR->TSS = &KiBootTss;
   KPCR->ProcessorNumber = 0;
   KiPcrInitDone = 1;
   PcrsAllocated++;

   KiInitializeGdt (NULL);
   Ki386BootInitializeTSS();
   Ki386InitializeLdt();

   /* Get processor information. */
   Ki386GetCpuId();

   /* Check FPU/MMX/SSE support. */
   KiCheckFPU();

   /* Mark the end of the exception handler list */
   KPCR->Tib.ExceptionList = (PVOID)-1;

   KeInitDpc(KPCR);

   KeInitExceptions ();
   KeInitInterrupts ();

   if (KPCR->PrcbData.FeatureBits & X86_FEATURE_PGE)
   {
      ULONG Flags;
      /* Enable global pages */
      Ke386GlobalPagesEnabled = TRUE;
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
   if(KPCR->PrcbData.CpuType == 0xf &&
      0 == strcmp("AuthenticAMD", KPCR->PrcbData.VendorString))
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
   if ((Pae && (KPCR->PrcbData.FeatureBits & X86_FEATURE_PAE)) || NoExecute)
   {
      MiEnablePAE((PVOID*)LastKernelAddress);
      Ke386PaeEnabled = TRUE;
   }
}

VOID INIT_FUNCTION
KeInit2(VOID)
{
   PKPCR Pcr = KeGetCurrentKPCR();

   KeInitializeBugCheck();
   KeInitializeDispatcher();
   KeInitializeTimerImpl();

   if (Pcr->PrcbData.FeatureBits & X86_FEATURE_PAE)
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
   if ((Pcr->PrcbData.FeatureBits & (X86_FEATURE_FXSR | X86_FEATURE_MMX | X86_FEATURE_SSE | X86_FEATURE_SSE2)) ||
       (Ke386CpuidFlags2 & X86_EXT_FEATURE_SSE3))
      {
         DPRINT1("CPU supports" "%s%s%s%s%s" ".\n",
                 ((Pcr->PrcbData.FeatureBits & X86_FEATURE_FXSR) ? " FXSR" : ""),
                 ((Pcr->PrcbData.FeatureBits & X86_FEATURE_MMX) ? " MMX" : ""),
                 ((Pcr->PrcbData.FeatureBits & X86_FEATURE_SSE) ? " SSE" : ""),
                 ((Pcr->PrcbData.FeatureBits & X86_FEATURE_SSE2) ? " SSE2" : ""),
                 ((Ke386CpuidFlags2 & X86_EXT_FEATURE_SSE3) ? " SSE3" : ""));
      }
   if (Ke386GetCr4() & X86_CR4_OSFXSR)
      {
         DPRINT1("SSE enabled.\n");
      }
   if (Ke386GetCr4() & X86_CR4_OSXMMEXCPT)
      {
         DPRINT1("Unmasked SIMD exceptions enabled.\n");
      }
   if (Pcr->PrcbData.VendorString[0])
   {
      DPRINT1("CPU Vendor: %s\n", Pcr->PrcbData.VendorString);
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
   if (Pcr->L2CacheSize)
   {
      DPRINT1("Ke386L2CacheSize: %dkB\n", Pcr->L2CacheSize);
   }
}

VOID INIT_FUNCTION
Ki386SetProcessorFeatures(VOID)
{
  PKPCR Pcr = KeGetCurrentKPCR();

  SharedUserData->ProcessorFeatures[PF_FLOATING_POINT_PRECISION_ERRATA] = FALSE;
  SharedUserData->ProcessorFeatures[PF_FLOATING_POINT_EMULATED] = FALSE;
  SharedUserData->ProcessorFeatures[PF_COMPARE_EXCHANGE_DOUBLE] =
    (Pcr->PrcbData.FeatureBits & X86_FEATURE_CX8);
  SharedUserData->ProcessorFeatures[PF_MMX_INSTRUCTIONS_AVAILABLE] =
    (Pcr->PrcbData.FeatureBits & X86_FEATURE_MMX);
  SharedUserData->ProcessorFeatures[PF_PPC_MOVEMEM_64BIT_OK] = FALSE;
  SharedUserData->ProcessorFeatures[PF_ALPHA_BYTE_INSTRUCTIONS] = FALSE;
  SharedUserData->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE] = 
    (Pcr->PrcbData.FeatureBits & X86_FEATURE_SSE);
  SharedUserData->ProcessorFeatures[PF_3DNOW_INSTRUCTIONS_AVAILABLE] =
    (Ke386CpuidExFlags & X86_EXT_FEATURE_3DNOW);
  SharedUserData->ProcessorFeatures[PF_RDTSC_INSTRUCTION_AVAILABLE] =
    (Pcr->PrcbData.FeatureBits & X86_FEATURE_TSC);
  SharedUserData->ProcessorFeatures[PF_PAE_ENABLED] = Ke386PaeEnabled;
  SharedUserData->ProcessorFeatures[PF_XMMI64_INSTRUCTIONS_AVAILABLE] =
    (Pcr->PrcbData.FeatureBits & X86_FEATURE_SSE2);
}
