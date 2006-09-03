/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/i386/cpu.c
 * PURPOSE:         Routines for CPU-level support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FIXME: Local EFLAGS defines not used anywhere else */
#define EFLAGS_IOPL     0x3000
#define EFLAGS_NF       0x4000
#define EFLAGS_RF       0x10000
#define EFLAGS_ID       0x200000

/* GLOBALS *******************************************************************/

/* The Boot TSS */
KTSS KiBootTss;

/* The TSS to use for Double Fault Traps (INT 0x9) */
UCHAR KiDoubleFaultTSS[KTSS_IO_MAPS];

/* The Boot GDT (FIXME: should have more entries */
KGDTENTRY KiBootGdt[12] =
{
    {0x0000, 0x0000, {{0x00, 0x00, 0x00, 0x00}}},       /* KGDT_NULL */
    {0xffff, 0x0000, {{0x00, 0x9a, 0xcf, 0x00}}},       /* KGDT_R0_CODE */
    {0xffff, 0x0000, {{0x00, 0x92, 0xcf, 0x00}}},       /* KGDT_R0_DATA */
    {0xffff, 0x0000, {{0x00, 0xfa, 0xcf, 0x00}}},       /* KGDT_R3_CODE */
    {0xffff, 0x0000, {{0x00, 0xf2, 0xcf, 0x00}}},       /* KGDT_R3_DATA*/
    {0x0000, 0x0000, {{0x00, 0x00, 0x00, 0x00}}},       /* KGDT_TSS */
    {0x0fff, 0x0000, {{0x00, 0x92, 0x00, 0xff}}},       /* KGDT_R0_PCR */
    {0x0fff, 0x0000, {{0x00, 0xf2, 0x00, 0x00}}},       /* KGDT_R3_TEB */
    {0x0000, 0x0000, {{0x00, 0x00, 0x00, 0x00}}},       /* KGDT_UNUSED */
    {0x0000, 0x0000, {{0x00, 0x00, 0x00, 0x00}}},       /* KGDT_LDT */
    {0x0000, 0x0000, {{0x00, 0x00, 0x00, 0x00}}},       /* KGDT_DF_TSS */
    {0x0000, 0x0000, {{0x00, 0x00, 0x00, 0x00}}}        /* KGDT_NMI_TSS */
};

/* GDT Descriptor */
KDESCRIPTOR KiGdtDescriptor = {sizeof(KiBootGdt), (ULONG)KiBootGdt};

/* CPU Features and Flags */
ULONG KeI386CpuType;
ULONG KeI386CpuStep;
ULONG KeProcessorArchitecture;
ULONG KeProcessorLevel;
ULONG KeProcessorRevision;
ULONG KeFeatureBits;
ULONG KiFastSystemCallDisable = 1;
ULONG KeI386NpxPresent = 0;
ULONG MxcsrFeatureMask = 0;
ULONG KeI386XMMIPresent = 0;
ULONG KeI386FxsrPresent = 0;
ULONG Ke386Pae = FALSE;
ULONG Ke386GlobalPagesEnabled = FALSE;
ULONG Ke386NoExecute = FALSE;
BOOLEAN KiI386PentiumLockErrataPresent;

/* CPU Signatures */
CHAR CmpIntelID[]       = "GenuineIntel";
CHAR CmpAmdID[]         = "AuthenticAMD";
CHAR CmpCyrixID[]       = "CyrixInstead";
CHAR CmpTransmetaID[]   = "GenuineTMx86";
CHAR CmpCentaurID[]     = "CentaurHauls";
CHAR CmpRiseID[]        = "RiseRiseRise";

/* SUPPORT ROUTINES FOR MSVC COMPATIBILITY ***********************************/

VOID
NTAPI
CPUID(IN ULONG CpuInfo[4],
      IN ULONG InfoType)
{
    Ki386Cpuid(InfoType, &CpuInfo[0], &CpuInfo[1], &CpuInfo[2], &CpuInfo[3]);
}

VOID
WRMSR(IN ULONG Register,
      IN LONGLONG Value)
{
    LARGE_INTEGER LargeVal;
    LargeVal.QuadPart = Value;
    Ke386Wrmsr(Register, LargeVal.HighPart, LargeVal.LowPart);
}

LONGLONG
RDMSR(IN ULONG Register)
{
    LARGE_INTEGER LargeVal;
    Ke386Rdmsr(Register, LargeVal.HighPart, LargeVal.LowPart);
    return LargeVal.QuadPart;
}

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KiSetProcessorType(VOID)
{
    ULONG EFlags, NewEFlags;
    ULONG Reg[4];
    ULONG Stepping, Type;

    /* Start by assuming no CPUID data */
    KeGetCurrentPrcb()->CpuID = 0;

    /* Save EFlags */
    Ke386SaveFlags(EFlags);

    /* XOR out the ID bit and update EFlags */
    NewEFlags = EFlags ^ EFLAGS_ID;
    Ke386RestoreFlags(NewEFlags);

    /* Get them back and see if they were modified */
    Ke386SaveFlags(NewEFlags);
    if (NewEFlags != EFlags)
    {
        /* The modification worked, so CPUID exists. Set the ID Bit again. */
        EFlags |= EFLAGS_ID;
        Ke386RestoreFlags(EFlags);

        /* Peform CPUID 0 to see if CPUID 1 is supported */
        CPUID(Reg, 0);
        if (Reg[0] > 0)
        {
            /* Do CPUID 1 now */
            CPUID(Reg, 1);

            /*
             * Get the Stepping and Type. The stepping contains both the
             * Model and the Step, while the Type contains the returned Type.
             * We ignore the family.
             *
             * For the stepping, we convert this: zzzzzzxy into this: x0y
             */
            Stepping = Reg[0] & 0xF0;
            Stepping <<= 4;
            Stepping += (Reg[0] & 0xFF);
            Stepping &= 0xF0F;
            Type = Reg[0] & 0xF00;
            Type >>= 8;

            /* Save them in the PRCB */
            KeGetCurrentPrcb()->CpuID = TRUE;
            KeGetCurrentPrcb()->CpuType = (UCHAR)Type;
            KeGetCurrentPrcb()->CpuStep = (USHORT)Stepping;
        }
        else
        {
            DPRINT1("CPUID Support lacking\n");
        }
    }
    else
    {
        DPRINT1("CPUID Support lacking\n");
    }

    /* Restore EFLAGS */
    Ke386RestoreFlags(EFlags);
}

ULONG
NTAPI
KiGetCpuVendor(VOID)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    ULONG Vendor[5];

    /* Assume no Vendor ID and fail if no CPUID Support. */
    Prcb->VendorString[0] = 0;
    if (!Prcb->CpuID) return 0;

    /* Get the Vendor ID and null-terminate it */
    CPUID(Vendor, 0);
    Vendor[4] = 0;

    /* Re-arrange vendor string */
    Vendor[5] = Vendor[2];
    Vendor[2] = Vendor[3];
    Vendor[3] = Vendor[5];

    /* Copy it to the PRCB and null-terminate it again */
    RtlCopyMemory(Prcb->VendorString,
                  &Vendor[1],
                  sizeof(Prcb->VendorString) - sizeof(CHAR));
    Prcb->VendorString[sizeof(Prcb->VendorString) - sizeof(CHAR)] = ANSI_NULL;

    /* Now check the CPU Type */
    if (!strcmp(Prcb->VendorString, CmpIntelID))
    {
        return CPU_INTEL;
    }
    else if (!strcmp(Prcb->VendorString, CmpAmdID))
    {
        return CPU_AMD;
    }
    else if (!strcmp(Prcb->VendorString, CmpCyrixID))
    {
        DPRINT1("Cyrix CPUs not fully supported\n");
        return 0;
    }
    else if (!strcmp(Prcb->VendorString, CmpTransmetaID))
    {
        DPRINT1("Transmeta CPUs not fully supported\n");
        return 0;
    }
    else if (!strcmp(Prcb->VendorString, CmpCentaurID))
    {
        DPRINT1("VIA CPUs not fully supported\n");
        return 0;
    }
    else if (!strcmp(Prcb->VendorString, CmpRiseID))
    {
        DPRINT1("Rise CPUs not fully supported\n");
        return 0;
    }

    /* Invalid CPU */
    return 0;
}

ULONG
NTAPI
KiGetFeatureBits(VOID)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    ULONG Vendor;
    ULONG FeatureBits = KF_WORKING_PTE;
    ULONG Reg[4];
    BOOLEAN ExtendedCPUID = TRUE;
    ULONG CpuFeatures = 0;

    /* Get the Vendor ID */
    Vendor = KiGetCpuVendor();

    /* Make sure we got a valid vendor ID at least. */
    if (!Vendor) return FeatureBits;

    /* Get the CPUID Info. Features are in Reg[3]. */
    CPUID(Reg, 1);

    /* Check for AMD CPU */
    if (Vendor == CPU_AMD)
    {
        /* Check if this is a K5 or higher. */
        if ((Reg[0] & 0x0F00) >= 0x0500)
        {
            /* Check if this is a K5 specifically. */
            if ((Reg[0] & 0x0F00) == 0x0500)
            {
                /* Get the Model Number */
                switch (Reg[0] & 0x00F0)
                {
                    /* Check if this is the Model 1 */
                    case 0x0010:

                        /* Check if this is Step 0 or 1. They don't support PGE */
                        if ((Reg[0] & 0x000F) > 0x03) break;

                    case 0x0000:

                        /* Model 0 doesn't support PGE at all. */
                        Reg[3] &= ~0x2000;
                        break;

                    case 0x0080:

                        /* K6-2, Step 8 and over have support for MTRR. */
                        if ((Reg[0] & 0x000F) >= 0x8) FeatureBits |= KF_AMDK6MTRR;
                        break;

                    case 0x0090:

                        /* As does the K6-3 */
                        FeatureBits |= KF_AMDK6MTRR;
                        break;

                    default:
                        break;
                }
            }
        }
        else
        {
            /* Familes below 5 don't support PGE, PSE or CMOV at all */
            Reg[3] &= ~(0x08 | 0x2000 | 0x8000);

            /* They also don't support advanced CPUID functions. */
            ExtendedCPUID = FALSE;
        }

        /* Set the current features */
        CpuFeatures = Reg[3];
    }

    /* Now check if this is Intel */
    if (Vendor == CPU_INTEL)
    {
        /* Check if it's a P6 */
        if (Prcb->CpuType == 6)
        {
            /* Perform the special sequence to get the MicroCode Signature */
            WRMSR(0x8B, 0);
            CPUID(Reg, 1);
            Prcb->UpdateSignature.QuadPart = RDMSR(0x8B);
        }
        else if (Prcb->CpuType == 5)
        {
            /* On P5, enable workaround for the LOCK errata. */
            KiI386PentiumLockErrataPresent = TRUE;
        }

        /* Check for broken P6 with bad SMP PTE implementation */
        if (((Reg[0] & 0x0FF0) == 0x0610 && (Reg[0] & 0x000F) <= 0x9) ||
            ((Reg[0] & 0x0FF0) == 0x0630 && (Reg[0] & 0x000F) <= 0x4))
        {
            /* Remove support for correct PTE support. */
            FeatureBits &= ~KF_WORKING_PTE;
        }

        /* Set the current features */
        CpuFeatures = Reg[3];
    }

    /* Convert all CPUID Feature bits into our format */
    if (CpuFeatures & 0x00000002) FeatureBits |= KF_V86_VIS | KF_CR4;
    if (CpuFeatures & 0x00000008) FeatureBits |= KF_LARGE_PAGE | KF_CR4;
    if (CpuFeatures & 0x00000010) FeatureBits |= KF_RDTSC;
    if (CpuFeatures & 0x00000100) FeatureBits |= KF_CMPXCHG8B;
    if (CpuFeatures & 0x00000800) FeatureBits |= KF_FAST_SYSCALL;
    if (CpuFeatures & 0x00001000) FeatureBits |= KF_MTRR;
    if (CpuFeatures & 0x00002000) FeatureBits |= KF_GLOBAL_PAGE | KF_CR4;
    if (CpuFeatures & 0x00008000) FeatureBits |= KF_CMOV;
    if (CpuFeatures & 0x00010000) FeatureBits |= KF_PAT;
    if (CpuFeatures & 0x00800000) FeatureBits |= KF_MMX;
    if (CpuFeatures & 0x01000000) FeatureBits |= KF_FXSR;
    if (CpuFeatures & 0x02000000) FeatureBits |= KF_XMMI;

    /* Check if CPUID 0x80000000 is supported */
    if (ExtendedCPUID)
    {
        /* Do the call */
        CPUID(Reg, 0x80000000);
        if ((Reg[0] & 0xffffff00) == 0x80000000)
        {
            /* Check if CPUID 0x80000001 is supported */
            if (Reg[0] >= 0x80000001)
            {
                /* Check which extended features are available. */
                CPUID(Reg, 0x80000001);

                /* Now handle each features for each CPU Vendor */
                switch (Vendor)
                {
                    case CPU_AMD:
                        if (Reg[3] & 0x80000000) FeatureBits |= KF_3DNOW;
                        break;
                }
            }
        }
    }

    /* Return the Feature Bits */
    return FeatureBits;
}

VOID
NTAPI
KiGetCacheInformation(VOID)
{
    PKIPCR Pcr = (PKIPCR)KeGetPcr();
    ULONG Vendor;
    ULONG Data[4];
    ULONG CacheRequests = 0, i;
    ULONG CurrentRegister;
    UCHAR RegisterByte;
    BOOLEAN FirstPass = TRUE;

    /* Set default L2 size */
    Pcr->SecondLevelCacheSize = 0;

    /* Get the Vendor ID and make sure we support CPUID */
    Vendor = KiGetCpuVendor();
    if (!Vendor) return;

    /* Check the Vendor ID */
    switch (Vendor)
    {
        /* Handle Intel case */
        case CPU_INTEL:

            /*Check if we support CPUID 2 */
            CPUID(Data, 0);
            if (Data[0] >= 2)
            {
                /* We need to loop for the number of times CPUID will tell us to */
                do
                {
                    /* Do the CPUID call */
                    CPUID(Data, 2);

                    /* Check if it was the first call */
                    if (FirstPass)
                    {
                        /*
                         * The number of times to loop is the first byte. Read
                         * it and then destroy it so we don't get confused.
                         */
                        CacheRequests = Data[0] & 0xFF;
                        Data[0] &= 0xFFFFFF00;

                        /* Don't go over this again */
                        FirstPass = FALSE;
                    }

                    /* Loop all 4 registers */
                    for (i = 0; i < 4; i++)
                    {
                        /* Get the current register */
                        CurrentRegister = Data[i];

                        /*
                         * If the upper bit is set, then this register should
                         * be skipped.
                         */
                        if (CurrentRegister & 0x80000000) continue;

                        /* Keep looping for every byte inside this register */
                        while (CurrentRegister)
                        {
                            /* Read a byte, skip a byte. */
                            RegisterByte = (UCHAR)(CurrentRegister & 0xFF);
                            CurrentRegister >>= 8;
                            if (!RegisterByte) continue;

                            /*
                             * Valid values are from 0x40 (0 bytes) to 0x49
                             * (32MB), or from 0x80 to 0x89 (same size but
                             * 8-way associative.
                             */
                            if (((RegisterByte > 0x40) &&
                                 (RegisterByte <= 0x49)) ||
                                ((RegisterByte > 0x80) &&
                                (RegisterByte <= 0x89)))
                            {
                                /* Mask out only the first nibble */
                                RegisterByte &= 0x0F;

                                /* Set the L2 Cache Size */
                                Pcr->SecondLevelCacheSize = 0x10000 <<
                                                            RegisterByte;
                            }
                        }
                    }
                } while (--CacheRequests);
            }
        break;

    case CPU_AMD:

        /* FIXME */
        DPRINT1("Not handling AMD caches yet\n");
        break;
    }
}

VOID
NTAPI
KiSetCR0Bits(VOID)
{
    ULONG Cr0;

    /* Save current CR0 */
    Cr0 = Ke386GetCr0();

    /* If this is a 486, enable Write-Protection */
    if (KeGetCurrentPrcb()->CpuType > 3) Cr0 |= CR0_WP;

    /* Set new Cr0 */
    Ke386SetCr0(Cr0);
}

VOID
NTAPI
KiInitializeTSS2(IN PKTSS Tss,
                 IN PKGDTENTRY TssEntry OPTIONAL)
{
    PUCHAR p;

    /* Make sure the GDT Entry is valid */
    if (TssEntry)
    {
        /* Set the Limit */
        TssEntry->LimitLow = sizeof(KTSS) - 1;
        TssEntry->HighWord.Bits.LimitHi &= 0xF0;
    }

    /* Now clear the I/O Map */
    RtlFillMemory(Tss->IoMaps[0].IoMap, 8096, -1);

    /* Initialize Interrupt Direction Maps */
    p = (PUCHAR)(Tss->IoMaps[0].DirectionMap);
    RtlZeroMemory(p, 32);

    /* Add DPMI support for interrupts */
    p[0] = 4;
    p[3] = 0x18;
    p[4] = 0x18;

    /* Initialize the default Interrupt Direction Map */
    p = Tss->IntDirectionMap;
    RtlZeroMemory(Tss->IntDirectionMap, 32);

    /* Add DPMI support */
    p[0] = 4;
    p[3] = 0x18;
    p[4] = 0x18;
}

VOID
NTAPI
KiInitializeTSS(IN PKTSS Tss)
{
    /* Set an invalid map base */
    Tss->IoMapBase = KiComputeIopmOffset(IO_ACCESS_MAP_NONE);

    /* Disable traps during Task Switches */
    Tss->Flags = 0;

    /* Set LDT and Ring 0 SS */
    Tss->LDT = 0;
    Tss->Ss0 = KGDT_R0_DATA;
}

VOID
NTAPI
Ki386InitializeTss(VOID)
{
    PKTSS Tss;
    PKGDTENTRY TssEntry;
    PKIDTENTRY TaskGateEntry;
    PKIDT_ACCESS TaskGateAccess;

    /* Initialize the boot TSS. */
    Tss = &KiBootTss;
    TssEntry = &KiBootGdt[KGDT_TSS / sizeof(KGDTENTRY)];
    KiInitializeTSS2(Tss, TssEntry);
    KiInitializeTSS(Tss);

    /* Initialize a descriptor for the TSS */
    TssEntry->HighWord.Bits.Type = I386_TSS;
    TssEntry->HighWord.Bits.Pres = 1;
    TssEntry->HighWord.Bits.Dpl = 0;
    TssEntry->BaseLow = (USHORT)((ULONG_PTR)Tss & 0xFFFF);
    TssEntry->HighWord.Bytes.BaseMid = (UCHAR)((ULONG_PTR)Tss >> 16);
    TssEntry->HighWord.Bytes.BaseHi = (UCHAR)((ULONG_PTR)Tss >> 24);

    /* Load the task register */
    __asm__("ltr %%ax":: "a" (KGDT_TSS));

    /* Setup the Task Gate for Double Fault Traps */
    TaskGateEntry = &KiIdt[8];
    TaskGateAccess = (PKIDT_ACCESS)&TaskGateEntry->Access;
#if 0
    TaskGateAccess->SegmentType = I386_TASK_GATE;
    TaskGateAccess->Present = 1;
    TaskGateAccess->Dpl = 0;
    TaskGateEntry->Selector = KGDT_DF_TSS;
#endif

    /* Initialize the TSS used for handling double faults. */
    Tss = (PKTSS)KiDoubleFaultTSS;
    KiInitializeTSS(Tss);
    Tss->CR3 = _Ke386GetCr(3);
    Tss->Esp0 = trap_stack_top;
    Tss->Eip = PtrToUlong(KiTrap8);
    Tss->Cs = KGDT_R0_CODE;
    Tss->Fs = KGDT_R0_PCR;
    Tss->Ss = Ke386GetSs();
    Tss->Es = KGDT_R3_DATA | RPL_MASK;
    Tss->Ds = KGDT_R3_DATA | RPL_MASK;

    /* Setup the Double Trap TSS entry in the GDT */
    TssEntry = &KiBootGdt[KGDT_DF_TSS / sizeof(KGDTENTRY)];
    TssEntry->HighWord.Bits.Type = I386_TSS;
    TssEntry->HighWord.Bits.Pres = 1;
    TssEntry->HighWord.Bits.Dpl = 0;
    TssEntry->BaseLow = (USHORT)((ULONG_PTR)Tss & 0xFFFF);
    TssEntry->HighWord.Bytes.BaseMid = (UCHAR)((ULONG_PTR)Tss >> 16);
    TssEntry->HighWord.Bytes.BaseHi = (UCHAR)((ULONG_PTR)Tss >> 24);
    TssEntry->LimitLow = KTSS_IO_MAPS;
}

/* This is a rather naive implementation of Ke(Save/Restore)FloatingPointState
   which will not work for WDM drivers. Please feel free to improve */
NTSTATUS
NTAPI
KeSaveFloatingPointState(OUT PKFLOATING_SAVE Save)
{
    PFNSAVE_FORMAT FpState;
    ASSERT_IRQL(DISPATCH_LEVEL);

    /* check if we are doing software emulation */
    if (!KeI386NpxPresent) return STATUS_ILLEGAL_FLOAT_CONTEXT;

    FpState = ExAllocatePool(NonPagedPool, sizeof (FNSAVE_FORMAT));
    if (!FpState) return STATUS_INSUFFICIENT_RESOURCES;

    *((PVOID *) Save) = FpState;
    asm volatile("fnsave %0\n\t" : "=m" (*FpState));

    KeGetCurrentThread()->DispatcherHeader.NpxIrql = KeGetCurrentIrql();
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KeRestoreFloatingPointState(IN PKFLOATING_SAVE Save)
{
    PFNSAVE_FORMAT FpState = *((PVOID *) Save);

    ASSERT(KeGetCurrentThread()->DispatcherHeader.NpxIrql == KeGetCurrentIrql());

    asm volatile("fnclex\n\t");
    asm volatile("frstor %0\n\t" : "=m" (*FpState));

    ExFreePool(FpState);
    return STATUS_SUCCESS;
}

VOID
INIT_FUNCTION
Ki386SetProcessorFeatures(VOID)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING KeyName =
   RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager\\Kernel");
   UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"FastSystemCallDisable");
   HANDLE KeyHandle;
   ULONG ResultLength;
   struct
   {
       KEY_VALUE_PARTIAL_INFORMATION Info;
       UCHAR Buffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION,
                                 Data[0]) + sizeof(ULONG)];
   } ValueData;
   NTSTATUS Status;
   ULONG FastSystemCallDisable = 0;
   
   SharedUserData->ProcessorFeatures[PF_FLOATING_POINT_PRECISION_ERRATA] = FALSE;
   SharedUserData->ProcessorFeatures[PF_FLOATING_POINT_EMULATED] = FALSE;
   SharedUserData->ProcessorFeatures[PF_COMPARE_EXCHANGE_DOUBLE] =
       (KeFeatureBits & KF_CMPXCHG8B) ? TRUE : FALSE;
   SharedUserData->ProcessorFeatures[PF_MMX_INSTRUCTIONS_AVAILABLE] =
       (KeFeatureBits & KF_MMX) ? TRUE : FALSE;
   SharedUserData->ProcessorFeatures[PF_PPC_MOVEMEM_64BIT_OK] = FALSE;
   SharedUserData->ProcessorFeatures[PF_ALPHA_BYTE_INSTRUCTIONS] = FALSE;
   SharedUserData->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE] =
       (KeFeatureBits & KF_XMMI) ? TRUE : FALSE;
   SharedUserData->ProcessorFeatures[PF_RDTSC_INSTRUCTION_AVAILABLE] =
       (KeFeatureBits & KF_RDTSC) ? TRUE : FALSE;

   /* Does the CPU Support Fast System Call? */   
   if (KeFeatureBits & KF_FAST_SYSCALL) {

        /* FIXME: Check for Family == 6, Model < 3 and Stepping < 3 and disable */

        /* Make sure it's not disabled in registry */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
        Status = ZwOpenKey(&KeyHandle,
                           KEY_QUERY_VALUE,
                           &ObjectAttributes);

        if (NT_SUCCESS(Status)) {

            /* Read the Value then Close the Key */
            Status = ZwQueryValueKey(KeyHandle,
                                     &ValueName,
                                     KeyValuePartialInformation,
                                     &ValueData,
                                     sizeof(ValueData),
                                     &ResultLength);
            if (NT_SUCCESS(Status))
            {
                if (ResultLength == sizeof(ValueData) &&
                    ValueData.Info.Type == REG_DWORD)
                {
                    FastSystemCallDisable = *(PULONG)ValueData.Info.Data != 0;
                }

                ZwClose(KeyHandle);
            }
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

VOID
NTAPI
KeFlushCurrentTb(VOID)
{
    /* Flush the TLB by resetting CR3 */
    _Ke386SetCr(3, _Ke386GetCr(3));
}

