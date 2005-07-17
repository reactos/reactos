/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/kernel.c
 * PURPOSE:         Initializes the kernel
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

ULONG KiPcrInitDone = 0;
static ULONG PcrsAllocated = 0;
static ULONG Ke386CpuidFlags2, Ke386CpuidExFlags, Ke386CpuidExMisc;
ULONG Ke386CacheAlignment;
CHAR Ke386CpuidModel[49] = {0,};
ULONG Ke386L1CacheSize;
BOOLEAN Ke386NoExecute = FALSE;
BOOLEAN Ke386Pae = FALSE;
BOOLEAN Ke386GlobalPagesEnabled = FALSE;
ULONG KiFastSystemCallDisable = 1;
extern PVOID Ki386InitialStackArray[MAXIMUM_PROCESSORS];
extern ULONG IdleProcessorMask;

/* FUNCTIONS *****************************************************************/

VOID INIT_FUNCTION STATIC
Ki386GetCpuId(VOID)
{
   ULONG OrigFlags, Flags, FinalFlags;
   ULONG MaxCpuidLevel;
   ULONG Dummy, Eax, Ecx, Edx;
   PKIPCR Pcr = (PKIPCR)KeGetCurrentKPCR();

   Ke386CpuidFlags2 =  Ke386CpuidExFlags = 0;
   Ke386CacheAlignment = 32;

   /* Try to toggle the id bit in eflags. */
   Ke386SaveFlags(OrigFlags);
   Flags = OrigFlags ^ X86_EFLAGS_ID;
   Ke386RestoreFlags(Flags);
   Ke386SaveFlags(FinalFlags);

   Pcr->PrcbData.LogicalProcessorsPerPhysicalProcessor = 1;
   Pcr->PrcbData.InitialApicId = 0xff;

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
      Ki386Cpuid(1, &Eax, &Ke386CpuidExMisc, &Ke386CpuidFlags2, &Pcr->PrcbData.FeatureBits);
      /* Get the cache alignment, if it is available */
      if (Pcr->PrcbData.FeatureBits & (1<<19))
      {
         Ke386CacheAlignment = ((Ke386CpuidExMisc >> 8) & 0xff) * 8;
      }
      Pcr->PrcbData.CpuType = (Eax >> 8) & 0xf;
      Pcr->PrcbData.CpuStep = (Eax & 0xf) | ((Eax << 4) & 0xf00);

      Pcr->PrcbData.InitialApicId = (Ke386CpuidExMisc >> 24) & 0xff;

      /* detect Hyper-Threading on Pentium 4 CPUs or later */
      if ((Pcr->PrcbData.CpuType == 0xf || (Eax & 0x0f00000)) &&
          !strncmp(Pcr->PrcbData.VendorString, "GenuineIntel", 12) &&
          Pcr->PrcbData.FeatureBits & X86_FEATURE_HT)
      {
        Pcr->PrcbData.LogicalProcessorsPerPhysicalProcessor = (Ke386CpuidExMisc >> 16) & 0xff;
      }
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

VOID
KeApplicationProcessorInitDispatcher(VOID)
{
   KIRQL oldIrql;
   oldIrql = KeAcquireDispatcherDatabaseLock();
   IdleProcessorMask |= (1 << KeGetCurrentProcessorNumber());
   KeReleaseDispatcherDatabaseLock(oldIrql);
}

VOID
INIT_FUNCTION
KeCreateApplicationProcessorIdleThread(ULONG Id)
{
  PETHREAD IdleThread;
  PKPRCB Prcb = ((PKPCR)((ULONG_PTR)KPCR_BASE + Id * PAGE_SIZE))->Prcb;

  PsInitializeIdleOrFirstThread(PsIdleProcess,
		     &IdleThread,
		     NULL,
		     KernelMode,
             FALSE);
  IdleThread->Tcb.State = Running;
  IdleThread->Tcb.FreezeCount = 0;
  IdleThread->Tcb.Affinity = 1 << Id;
  IdleThread->Tcb.UserAffinity = 1 << Id;
  IdleThread->Tcb.Priority = LOW_PRIORITY;
  IdleThread->Tcb.BasePriority = LOW_PRIORITY;
  Prcb->IdleThread = &IdleThread->Tcb;
  Prcb->CurrentThread = &IdleThread->Tcb;

  Ki386InitialStackArray[Id] = (PVOID)IdleThread->Tcb.StackLimit;

  DPRINT("IdleThread for Processor %d has PID %d\n",
	   Id, IdleThread->Cid.UniqueThread);
}

VOID INIT_FUNCTION
KePrepareForApplicationProcessorInit(ULONG Id)
{
  DPRINT("KePrepareForApplicationProcessorInit(Id %d)\n", Id);
  PFN_TYPE PrcPfn;
  PKIPCR Pcr;
  PKIPCR BootPcr;

  BootPcr = (PKIPCR)KPCR_BASE;
  Pcr = (PKIPCR)((ULONG_PTR)KPCR_BASE + Id * PAGE_SIZE);

  MmRequestPageMemoryConsumer(MC_NPPOOL, TRUE, &PrcPfn);
  MmCreateVirtualMappingForKernel((PVOID)Pcr,
				  PAGE_READWRITE,
				  &PrcPfn,
				  1);
  /*
   * Create a PCR for this processor
   */
  memset(Pcr, 0, PAGE_SIZE);
  Pcr->Number = Id;
  Pcr->SetMember = 1 << Id;
  Pcr->Tib.Self = &Pcr->Tib;
  Pcr->Self = (PKPCR)Pcr;
  Pcr->Prcb = &Pcr->PrcbData;
  Pcr->Irql = SYNCH_LEVEL;

  Pcr->PrcbData.SetMember = 1 << Id;
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
  PKIPCR Pcr;

  DPRINT("KeApplicationProcessorInit()\n");

  if (Ke386GlobalPagesEnabled)
  {
     /* Enable global pages */
     Ke386SetCr4(Ke386GetCr4() | X86_CR4_PGE);
  }


  Offset = InterlockedIncrementUL(&PcrsAllocated) - 1;
  Pcr = (PKIPCR)((ULONG_PTR)KPCR_BASE + Offset * PAGE_SIZE);

  /*
   * Initialize the GDT
   */
  KiInitializeGdt((PKPCR)Pcr);

  /* Get processor information. */
  Ki386GetCpuId();

  /* Check FPU/MMX/SSE support. */
  KiCheckFPU();

  KeInitDpc(Pcr->Prcb);

  if (Pcr->PrcbData.FeatureBits & X86_FEATURE_SYSCALL)
  {
     extern void KiFastCallEntry(void);

     /* CS Selector of the target segment. */
     Ke386Wrmsr(0x174, KERNEL_CS, 0);
     /* Target ESP. */
     Ke386Wrmsr(0x175, 0, 0);
     /* Target EIP. */
     Ke386Wrmsr(0x176, (ULONG_PTR)KiFastCallEntry, 0);
  }

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
   PKIPCR KPCR;
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

   KPCR = (PKIPCR)KPCR_BASE;
   memset(KPCR, 0, PAGE_SIZE);
   KPCR->Self = (PKPCR)KPCR;
   KPCR->Prcb = &KPCR->PrcbData;
   KPCR->Irql = SYNCH_LEVEL;
   KPCR->Tib.Self = &KPCR->Tib;
   KPCR->GDT = KiBootGdt;
   KPCR->IDT = (PUSHORT)KiIdt;
   KPCR->TSS = &KiBootTss;
   KPCR->Number = 0;
   KPCR->SetMember = 1 << 0;
   KPCR->PrcbData.SetMember = 1 << 0;
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

   KeInitDpc(KPCR->Prcb);

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
#if 0
   /*
    * FIXME:
    *   Make the detection of the noexecute feature more portable.
    */
   if(KPCR->PrcbData.CpuType == 0xf &&
      RtlCompareMemory("AuthenticAMD", KPCR->PrcbData.VendorString, 12) == 12)
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
#endif

   Ke386Pae = Ke386GetCr4() & X86_CR4_PAE ? TRUE : FALSE;
#if 0
   /* Enable PAE mode */
   if ((Pae && (KPCR->PrcbData.FeatureBits & X86_FEATURE_PAE)) || NoExecute)
   {
      MiEnablePAE((PVOID*)LastKernelAddress);
      Ke386PaeEnabled = TRUE;
   }
#endif
   if (KPCR->PrcbData.FeatureBits & X86_FEATURE_SYSCALL)
   {
      extern void KiFastCallEntry(void);

      /* CS Selector of the target segment. */
      Ke386Wrmsr(0x174, KERNEL_CS, 0);
      /* Target ESP. */
      Ke386Wrmsr(0x175, 0, 0);
      /* Target EIP. */
      Ke386Wrmsr(0x176, (ULONG_PTR)KiFastCallEntry, 0);
   }
}

VOID INIT_FUNCTION
KeInit2(VOID)
{
   PKIPCR Pcr = (PKIPCR)KeGetCurrentKPCR();

   KiInitializeBugCheck();
   KeInitializeDispatcher();
   KiInitializeSystemClock();

   if (Pcr->PrcbData.FeatureBits & X86_FEATURE_PAE)
   {
      DPRINT("CPU supports PAE mode\n");
      if (Ke386Pae)
      {
         DPRINT("CPU runs in PAE mode\n");
         if (Ke386NoExecute)
         {
            DPRINT("NoExecute is enabled\n");
         }
      }
      else
      {
         DPRINT("CPU doesn't run in PAE mode\n");
      }
   }
   if ((Pcr->PrcbData.FeatureBits & (X86_FEATURE_FXSR | X86_FEATURE_MMX | X86_FEATURE_SSE | X86_FEATURE_SSE2)) ||
       (Ke386CpuidFlags2 & X86_EXT_FEATURE_SSE3))
      {
         DPRINT("CPU supports" "%s%s%s%s%s" ".\n",
                ((Pcr->PrcbData.FeatureBits & X86_FEATURE_FXSR) ? " FXSR" : ""),
                ((Pcr->PrcbData.FeatureBits & X86_FEATURE_MMX) ? " MMX" : ""),
                ((Pcr->PrcbData.FeatureBits & X86_FEATURE_SSE) ? " SSE" : ""),
                ((Pcr->PrcbData.FeatureBits & X86_FEATURE_SSE2) ? " SSE2" : ""),
                ((Ke386CpuidFlags2 & X86_EXT_FEATURE_SSE3) ? " SSE3" : ""));
      }
   if (Ke386GetCr4() & X86_CR4_OSFXSR)
      {
         DPRINT("SSE enabled.\n");
      }
   if (Ke386GetCr4() & X86_CR4_OSXMMEXCPT)
      {
         DPRINT("Unmasked SIMD exceptions enabled.\n");
      }
   if (Pcr->PrcbData.VendorString[0])
   {
      DPRINT("CPU Vendor: %s\n", Pcr->PrcbData.VendorString);
   }
   if (Ke386CpuidModel[0])
   {
      DPRINT("CPU Model:  %s\n", Ke386CpuidModel);
   }

   DPRINT("Ke386CacheAlignment: %d\n", Ke386CacheAlignment);
   if (Ke386L1CacheSize)
   {
      DPRINT("Ke386L1CacheSize: %dkB\n", Ke386L1CacheSize);
   }
   if (Pcr->L2CacheSize)
   {
      DPRINT("Ke386L2CacheSize: %dkB\n", Pcr->L2CacheSize);
   }
}

VOID INIT_FUNCTION
Ki386SetProcessorFeatures(VOID)
{
   PKIPCR Pcr = (PKIPCR)KeGetCurrentKPCR();
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING KeyName =
   RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager\\Kernel");
   UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"FastSystemCallDisable");
   HANDLE KeyHandle;
   ULONG ResultLength;
   KEY_VALUE_PARTIAL_INFORMATION ValueData;
   NTSTATUS Status;
   ULONG FastSystemCallDisable = 0;

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
   SharedUserData->ProcessorFeatures[PF_PAE_ENABLED] = Ke386Pae;
   SharedUserData->ProcessorFeatures[PF_XMMI64_INSTRUCTIONS_AVAILABLE] =
      (Pcr->PrcbData.FeatureBits & X86_FEATURE_SSE2);

   /* Does the CPU Support Fast System Call? */
   if (Pcr->PrcbData.FeatureBits & X86_FEATURE_SYSCALL) {

        /* FIXME: Check for Family == 6, Model < 3 and Stepping < 3 and disable */

        /* Make sure it's not disabled in registry */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
        Status = NtOpenKey(&KeyHandle, KEY_ALL_ACCESS, &ObjectAttributes);

        if (NT_SUCCESS(Status)) {

            /* Read the Value then Close the Key */
            Status = NtQueryValueKey(KeyHandle,
                                     &ValueName,
                                     KeyValuePartialInformation,
                                     &ValueData,
                                     sizeof(ValueData),
                                     &ResultLength);
            RtlMoveMemory(&FastSystemCallDisable, ValueData.Data, sizeof(ULONG));

            NtClose(KeyHandle);
        }

    } else {

        /* Disable SYSENTER/SYSEXIT, because the CPU doesn't support it */
        FastSystemCallDisable = 1;

    }

    if (FastSystemCallDisable) {
        /* Use INT2E */
        const unsigned char Entry[7] = {0x8D, 0x54, 0x24, 0x08,     /* lea    0x8(%esp),%edx    */
                                        0xCD, 0x2E,                 /* int    0x2e              */
                                        0xC3};                      /* ret                      */
        memcpy(&SharedUserData->SystemCall, Entry, sizeof(Entry));
    } else {
        /* Use SYSENTER */
        const unsigned char Entry[5] = {0x8B, 0xD4,                 /* movl    %esp,%edx        */ 
                                        0x0F, 0x34,                 /* sysenter                 */
                                        0xC3};                      /* ret                      */    
        memcpy(&SharedUserData->SystemCall, Entry, sizeof(Entry));
        /* Enable SYSENTER/SYSEXIT */
        KiFastSystemCallDisable = 0;
    }
}
