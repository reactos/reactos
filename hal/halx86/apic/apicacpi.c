/*
 * PROJECT:         ReactOS HAL
 * COPYRIGHT:       GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:            hal/halx86/apic/apicacpi.c
 * PURPOSE:         ACPI part APIC HALs code
 * PROGRAMMERS:     Copyright 2020 Vadim Galyant <vgal@rambler.ru>
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
//#define NDEBUG
#include <debug.h>

#include "apic.h"
#include "apicacpi.h"
#include <ioapic.h>

/* DATA ***********************************************************************/

LOCAL_APIC HalpStaticProcLocalApicTable[MAX_CPUS] = {{0}};
IO_APIC_VERSION_REGISTER HalpIOApicVersion[MAX_IOAPICS];
UCHAR HalpIoApicId[MAX_IOAPICS] = {0};
UCHAR HalpMaxProcs = 0;
BOOLEAN HalpPciLockSettings;
PLOCAL_APIC HalpProcLocalApicTable = NULL;
BOOLEAN HalpUsePmTimer = FALSE;
PVOID * HalpLocalNmiSources = NULL;

/* GLOBALS ********************************************************************/

/* APIC */
extern HALP_MP_INFO_TABLE HalpMpInfoTable;
extern APIC_INTI_INFO HalpIntiInfo[MAX_INTI];
extern APIC_ADDRESS_USAGE HalpApicUsage;
extern ULONG HalpDefaultApicDestinationModeMask;
extern USHORT HalpMaxApicInti[MAX_IOAPICS];
extern UCHAR HalpMaxProcsPerCluster;
extern UCHAR HalpIRQLtoTPR[32];    // table, which sets the correspondence between IRQL levels and TPR (Task Priority Register) values.
extern KIRQL HalpVectorToIRQL[16];
extern BOOLEAN HalpForceApicPhysicalDestinationMode;
extern BOOLEAN HalpForceClusteredApicMode;
extern BOOLEAN HalpHiberInProgress;
extern USHORT HalpVectorToINTI[MAX_CPUS * MAX_INT_VECTORS];
extern ULONGLONG HalpProc0TSCHz;

/* ACPI */
extern ULONG HalpPicVectorRedirect[16];
extern ULONG HalpPicVectorFlags[16];
extern BOOLEAN LessThan16Mb;
extern BOOLEAN HalpPhysicalMemoryMayAppearAbove4GB;

extern UCHAR HalpCmosCenturyOffset;

/* FUNCTIONS ******************************************************************/

CODE_SEG("INIT")
VOID
NTAPI
HalpGetParameters(_In_ PCHAR CommandLine)
{
    /* Check if PCI is locked */
    if (strstr(CommandLine, "PCILOCK")) HalpPciLockSettings = TRUE;

    /* Check for initial breakpoint */
    if (strstr(CommandLine, "BREAK"))
    {
        DPRINT1("HalpGetParameters: FIXME parameters [BREAK]. DbgBreakPoint()\n");
        DbgBreakPoint();
    }

    if (strstr(CommandLine, "ONECPU"))
    {
        DPRINT1("HalpGetParameters: FIXME parameters [ONECPU]. DbgBreakPoint()\n");
        DbgBreakPoint();
        //HalpDontStartProcessors++;
    }

  #ifdef CONFIG_SMP // halmacpi only
    if (strstr(CommandLine, "USEPMTIMER"))
    {
        HalpUsePmTimer = TRUE;
    }
  #endif

    if (strstr(CommandLine, "INTAFFINITY"))
    {
        DPRINT1("HalpGetParameters: FIXME parameters [INTAFFINITY]. DbgBreakPoint()\n");
        DbgBreakPoint();
        //HalpStaticIntAffinity = TRUE;
    }

    if (strstr(CommandLine, "USEPHYSICALAPIC"))
    {
        HalpDefaultApicDestinationModeMask = 0;
        HalpForceApicPhysicalDestinationMode = TRUE;
    }

    if (strstr(CommandLine, "MAXPROCSPERCLUSTER"))
    {
        DPRINT1("HalpGetParameters: FIXME parameters [MAXPROCSPERCLUSTER]. DbgBreakPoint()\n");
        DbgBreakPoint();
    }

    if (strstr(CommandLine, "MAXAPICCLUSTER"))
    {
        DPRINT1("HalpGetParameters: FIXME parameters [MAXAPICCLUSTER]. DbgBreakPoint()\n");
        DbgBreakPoint();
    }
}

CODE_SEG("INIT")
VOID
NTAPI
HalpAcpiApplyFadtSettings(_In_ PFADT Fadt)
{
    /* FADT Fixed Feature Flags */
    DPRINT1("HalpAcpiApplyFadtSettings: flags %08X\n", Fadt, Fadt->flags);

    if (Fadt->flags & FADT_FORCE_APIC_CLUSTER_MODEL) // FORCE_APIC_CLUSTER_MODEL
    {
        DPRINT1("HalpAcpiApplyFadtSettings: ACPI_APIC_CLUSTER_MODEL\n");
        HalpForceClusteredApicMode = TRUE;
    }

    if (Fadt->flags & FADT_FORCE_APIC_PHYSICAL_DESTINATION_MODE) // FORCE_APIC_PHYSICAL_DESTINATION_MODE
    {
        DPRINT1("HalpAcpiApplyFadtSettings: ACPI_PHYSICAL_DEST_MODE\n");
        HalpForceApicPhysicalDestinationMode = TRUE;
        HalpDefaultApicDestinationModeMask = 0;
    }
}

CODE_SEG("INIT")
VOID
NTAPI
HalpMarkProcessorStarted(_In_ UCHAR Id,
                         _In_ ULONG PrcNumber)
{
    ULONG ix;

    for (ix = 0; ix < HalpMpInfoTable.ProcessorCount; ix++)
    {
        if (HalpProcLocalApicTable[ix].Id == Id)
        {
            HalpProcLocalApicTable[ix].ProcessorStarted = TRUE;
            HalpProcLocalApicTable[ix].ProcessorNumber = PrcNumber;

            if (PrcNumber == 0)
            {
                HalpProcLocalApicTable[ix].FirstProcessor = TRUE;
            }

            break;
        }
    }
}

CODE_SEG("INIT")
VOID
NTAPI 
HalpInitMpInfo(_In_ PACPI_TABLE_MADT ApicTable,
               _In_ ULONG Phase,
               _In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PACPI_SUBTABLE_HEADER Header;
    PACPI_MADT_LOCAL_APIC LocalApic;
    PIO_APIC_REGISTERS IoApicRegs;
    ULONG_PTR TableEnd;
    IO_APIC_VERSION_REGISTER IoApicVersion;
    PHYSICAL_ADDRESS PhAddress;
    PFN_COUNT PageCount;
    //ULONG NmiIdx = 0;
    ULONG Size;
    ULONG ix = 0;
    ULONG Idx;
    UCHAR NumberProcs = 0;

    HalpMpInfoTable.LocalApicversion = 0x10;

    if ((ApicTable->Flags & ACPI_MADT_DUAL_PIC) == 0)
    {
        KeBugCheckEx(MISMATCHED_HAL, 6, 0, 0, 0);
    }

    if (Phase == 0 && HalpProcLocalApicTable == NULL)
    {
        /* First initialization */

        Header = (PACPI_SUBTABLE_HEADER)&ApicTable[1];
        TableEnd = (ULONG_PTR)ApicTable + ApicTable->Header.Length;

        HalpProcLocalApicTable = HalpStaticProcLocalApicTable;

        if ((ULONG_PTR)Header < TableEnd)
        {
            do
            {
                LocalApic = (PACPI_MADT_LOCAL_APIC)Header;

                if (LocalApic->Header.Type == ACPI_MADT_TYPE_LOCAL_APIC &&
                    LocalApic->Header.Length == sizeof(ACPI_MADT_LOCAL_APIC) &&
                    LocalApic->LapicFlags & ACPI_MADT_ENABLED)
                {
                    ix++;
                }

                Size = Header->Length;
                Header = (PACPI_SUBTABLE_HEADER)((ULONG_PTR)Header + Header->Length);
            }
            while (Size != 0 && (ULONG_PTR)Header < TableEnd);

            if (ix > MAX_CPUS)
            {
                Size = ix * LOCAL_APIC_SIZE;
                PageCount = BYTES_TO_PAGES(Size);

                PhAddress.QuadPart = HalpAllocPhysicalMemory(LoaderBlock, 0xFFFFFFFF, PageCount, FALSE);
                if (!PhAddress.QuadPart)
                {
                    ASSERT(PhAddress.QuadPart != 0);
                    KeBugCheckEx(HAL_INITIALIZATION_FAILED, 0x105, 1, Size, PageCount);
                }

                HalpProcLocalApicTable = HalpMapPhysicalMemory64(PhAddress, PageCount);
                if (!HalpProcLocalApicTable)
                {
                    ASSERT(HalpProcLocalApicTable != NULL);
                    KeBugCheckEx(HAL_INITIALIZATION_FAILED, 0x105, 2, Size, PageCount);
                }

                RtlZeroMemory(HalpProcLocalApicTable, Size);
            }
        }
    }

    Header = (PACPI_SUBTABLE_HEADER)&ApicTable[1];
    TableEnd = (ULONG_PTR)ApicTable + ApicTable->Header.Length;

    for (ix = 0; ((ULONG_PTR)Header < TableEnd); )
    {
        if (Header->Type == ACPI_MADT_TYPE_LOCAL_APIC &&
            Header->Length == sizeof(ACPI_MADT_LOCAL_APIC))
        {
            LocalApic = (PACPI_MADT_LOCAL_APIC)Header;

            if (Phase == 0 && (LocalApic->LapicFlags & ACPI_MADT_ENABLED))
            {
                Idx = HalpMpInfoTable.ProcessorCount;

                HalpProcLocalApicTable[Idx].Id = LocalApic->Id;
                HalpProcLocalApicTable[Idx].ProcessorId = LocalApic->ProcessorId;

                HalpMpInfoTable.ProcessorCount++;
            }

            ix++;
            NumberProcs = ix;

            HalpMaxProcs = max(NumberProcs, HalpMaxProcs);

            Header = (PACPI_SUBTABLE_HEADER)((ULONG_PTR)Header + Header->Length);
        }
        else if (Header->Type == ACPI_MADT_TYPE_IO_APIC &&
                Header->Length == sizeof(ACPI_MADT_IO_APIC))
        {
            Idx = HalpMpInfoTable.IoApicCount;

            if (Phase == 0 && Idx < MAX_IOAPICS)
            {
                PACPI_MADT_IO_APIC IoApic = (PACPI_MADT_IO_APIC)Header;

                HalpIoApicId[Idx] = IoApic->Id;

                HalpMpInfoTable.IoApicIrqBase[Idx] = IoApic->GlobalIrqBase;
                HalpMpInfoTable.IoApicPA[Idx] = IoApic->Address;

                PhAddress.QuadPart = IoApic->Address;
                IoApicRegs = HalpMapPhysicalMemoryWriteThrough64(PhAddress, 1);

                if (!IoApicRegs)
                {
                    ASSERT(IoApicRegs != NULL);
                    KeBugCheckEx(HAL_INITIALIZATION_FAILED, 0x106, (ULONG_PTR)IoApic->Address, (ULONG_PTR)IoApic->Address, 0);
                }

                HalpMpInfoTable.IoApicVA[Idx] = (ULONG)IoApicRegs;

                IoApicRegs->IoRegisterSelect = IOAPIC_VER;
                IoApicRegs->IoWindow = 0;

                IoApicRegs->IoRegisterSelect = IOAPIC_VER;
                IoApicVersion.AsULONG = IoApicRegs->IoWindow;

                HalpIOApicVersion[Idx] = IoApicVersion;

                HalpMaxApicInti[Idx] = IoApicVersion.MaxRedirectionEntry + 1;
                HalpMpInfoTable.IoApicCount++;

                ASSERT(HalpMpInfoTable.IoApicPA[Idx] == IoApic->Address);
            }

            ix = NumberProcs;
            Header = (PACPI_SUBTABLE_HEADER)((ULONG_PTR)Header + Header->Length);
        }
        else if (Header->Type == ACPI_MADT_TYPE_INTERRUPT_OVERRIDE &&
                Header->Length == sizeof(ACPI_MADT_INTERRUPT_OVERRIDE))
        {
            if (Phase == 0)
            {
                PACPI_MADT_INTERRUPT_OVERRIDE InterruptOverride;
                InterruptOverride = (PACPI_MADT_INTERRUPT_OVERRIDE)Header;

                Idx = InterruptOverride->SourceIrq;
                HalpPicVectorRedirect[Idx] = InterruptOverride->GlobalIrq;
                HalpPicVectorFlags[Idx] = InterruptOverride->IntiFlags;

                ASSERT(HalpPicVectorRedirect[Idx] == InterruptOverride->GlobalIrq);
                ASSERT(HalpPicVectorFlags[Idx] == InterruptOverride->IntiFlags);
            }

            Header = (PACPI_SUBTABLE_HEADER)((ULONG_PTR)Header + Header->Length);
        }
        else if (Header->Type == ACPI_MADT_TYPE_NMI_SOURCE &&
                Header->Length == sizeof(ACPI_MADT_NMI_SOURCE))
        {
            if (Phase == 1)
            {
                // FIXME UNIMPLIMENTED;
                ASSERT(FALSE);
            }

            Header = (PACPI_SUBTABLE_HEADER)((ULONG_PTR)Header + Header->Length);
        }
        else if (Header->Type == ACPI_MADT_TYPE_LOCAL_APIC_NMI &&
                Header->Length == sizeof(ACPI_MADT_LOCAL_APIC_NMI))
        {
            if (Phase == 1)
            {
                // FIXME UNIMPLIMENTED;
                ASSERT(FALSE);
            }

            Header = (PACPI_SUBTABLE_HEADER)((ULONG_PTR)Header + Header->Length);
        }
        else
        {
            Header = (PACPI_SUBTABLE_HEADER)((ULONG_PTR)Header + 1);
        }
    }
}

CODE_SEG("INIT")
BOOLEAN
NTAPI 
HalpVerifyIOUnit(_In_ PIO_APIC_REGISTERS IOUnitRegs)
{
    IO_APIC_VERSION_REGISTER IoApicVersion1;
    IO_APIC_VERSION_REGISTER IoApicVersion2;

    IOUnitRegs->IoRegisterSelect = IOAPIC_VER;
    IOUnitRegs->IoWindow = 0;

    IOUnitRegs->IoRegisterSelect = IOAPIC_VER;
    IoApicVersion1.AsULONG = IOUnitRegs->IoWindow;

    IOUnitRegs->IoRegisterSelect = IOAPIC_VER;
    IOUnitRegs->IoWindow = 0;

    IOUnitRegs->IoRegisterSelect = IOAPIC_VER;
    IoApicVersion2.AsULONG = IOUnitRegs->IoWindow;

    if (IoApicVersion1.ApicVersion != IoApicVersion2.ApicVersion ||
        IoApicVersion1.MaxRedirectionEntry != IoApicVersion2.MaxRedirectionEntry)
    {
        return FALSE;
    }

    return TRUE;
}

CODE_SEG("INIT")
BOOLEAN
NTAPI 
DetectMP(_In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PACPI_TABLE_MADT HalpApicTable;
    PHYSICAL_ADDRESS PhAddress;
    ULONG_PTR LocalApicBaseVa;
    ULONG ix;

    LoaderBlock->Extension->HalpIRQLToTPR = HalpIRQLtoTPR;
    LoaderBlock->Extension->HalpVectorToIRQL = HalpVectorToIRQL;

    RtlZeroMemory(&HalpMpInfoTable, sizeof(HalpMpInfoTable));

    HalpApicTable = HalAcpiGetTable(LoaderBlock, 'CIPA');
    if (!HalpApicTable)
    {
        return FALSE;
    }

    HalpInitMpInfo(HalpApicTable, 0, LoaderBlock);
    if (!HalpMpInfoTable.IoApicCount)
    {
        return FALSE;
    }

    HalpMpInfoTable.LocalApicPA = HalpApicTable->Address;
    PhAddress.QuadPart = HalpMpInfoTable.LocalApicPA;

    LocalApicBaseVa = (ULONG_PTR)HalpMapPhysicalMemoryWriteThrough64(PhAddress, 1);

    HalpRemapVirtualAddress64((PVOID)LOCAL_APIC_BASE, PhAddress, TRUE);

    if (*(volatile PUCHAR)(LocalApicBaseVa + APIC_VER) > LOCAL_APIC_VERSION_MAX)
    {
        return FALSE;
    }

    for (ix = 0; ix < HalpMpInfoTable.IoApicCount; ix++)
    {
        if (!HalpVerifyIOUnit((PIO_APIC_REGISTERS)HalpMpInfoTable.IoApicVA[ix]))
        {
            return FALSE;
        }
    }

    return TRUE;
}

CODE_SEG("INIT")
VOID
NTAPI 
HalpInitIntiInfo()
{
    PAPIC_INTI_INFO pIntiInfo;
    HAL_PIC_VECTOR_FLAGS PicFlags;
    ULONG Vector;
    ULONG Inti;
    ULONG ApicNo;
    USHORT InterruptDesc;
    USHORT SciVector;

    DPRINT("HalpInitIntiInfo: IoApicIrqBase %X, MAX_INTI %X\n", HalpMpInfoTable.IoApicIrqBase[0], MAX_INTI);

    for (Inti = 0; Inti < MAX_INTI; Inti++)
    {
        HalpIntiInfo[Inti].Type = INTI_INFO_TYPE_INT; // default type of Interrupt Source - INTR
        HalpIntiInfo[Inti].TriggerMode = INTI_INFO_TRIGGER_LEVEL;
        HalpIntiInfo[Inti].Polarity = INTI_INFO_POLARITY_ACTIVE_LOW;
    }

    Vector = HalpPicVectorRedirect[APIC_CLOCK_INDEX];

    if (!HalpGetApicInterruptDesc(Vector, &InterruptDesc))
    {
        DPRINT1("HalpInitIntiInfo: KeBugCheckEx\n");
        KeBugCheckEx(HAL_INITIALIZATION_FAILED, 0x3000, 1, Vector, 0);
    }

    DPRINT("HalpInitIntiInfo: Vector %X, InterruptDesc %X\n", Vector, InterruptDesc);

    PicFlags.AsULONG = HalpPicVectorFlags[APIC_CLOCK_INDEX];
    pIntiInfo = &HalpIntiInfo[InterruptDesc];

    DPRINT("HalpInitIntiInfo: PicFlags %X, IntiInfo %X\n", PicFlags.AsULONG, pIntiInfo->AsULONG);

    if (PicFlags.Polarity == PIC_FLAGS_POLARITY_CONFORMS)
    {
        pIntiInfo->Polarity = INTI_INFO_POLARITY_ACTIVE_HIGH;
    }
    else
    {
        pIntiInfo->Polarity = PicFlags.Polarity;
    }

    if (PicFlags.TriggerMode == PIC_FLAGS_TRIGGER_CONFORMS)
    {
        pIntiInfo->TriggerMode = INTI_INFO_TRIGGER_EDGE;
    }
    else
    {
        pIntiInfo->TriggerMode = INTI_INFO_TRIGGER_LEVEL;
    }

    SciVector = HalpFixedAcpiDescTable.sci_int_vector;
    Vector = HalpPicVectorRedirect[SciVector];

    if (!HalpGetApicInterruptDesc(Vector, &InterruptDesc))
    {
        DPRINT1("HalpInitIntiInfo: KeBugCheckEx\n");
        KeBugCheckEx(HAL_INITIALIZATION_FAILED, 0x3000, Vector, 0, 0);
    }

    DPRINT("SciVector %X, Vector %X, InterruptDesc %X\n", SciVector, Vector, InterruptDesc);

    PicFlags.AsULONG = HalpPicVectorFlags[SciVector];
    pIntiInfo = &HalpIntiInfo[InterruptDesc];

    DPRINT("HalpInitIntiInfo: PicFlags %X, IntiInfo %X\n", PicFlags.AsULONG, pIntiInfo->AsULONG);

    if (PicFlags.Polarity == PIC_FLAGS_POLARITY_CONFORMS)
    {
        pIntiInfo->Polarity = INTI_INFO_POLARITY_ACTIVE_LOW;
    }
    else
    {
        pIntiInfo->Polarity = PicFlags.Polarity;
    }

    if (PicFlags.TriggerMode != PIC_FLAGS_TRIGGER_CONFORMS &&
        PicFlags.TriggerMode != PIC_FLAGS_TRIGGER_LEVEL)
    {
        DPRINT1("HalpInitIntiInfo: KeBugCheckEx\n");
        KeBugCheckEx(ACPI_BIOS_ERROR, 0x10008, SciVector, 0, 0);
    }

    pIntiInfo->TriggerMode = INTI_INFO_TRIGGER_LEVEL;

    Inti = 0;
    for (ApicNo = 0; ApicNo < MAX_IOAPICS; ApicNo++)
    {
        Inti += HalpMaxApicInti[ApicNo];
    }

    ASSERT(Inti < MAX_INTI);
}

CODE_SEG("INIT")
VOID
NTAPI 
HalpInitializeIOUnits()
{
    PIO_APIC_REGISTERS IoApicRegs;
    ULONG ApicNo;
    ULONG IoApicId;
    ULONG MaxRedirectRegs;
    ULONG Idx;

    DPRINT("HalpInitializeIOUnits: IoApicCount %X\n", HalpMpInfoTable.IoApicCount);

    for (ApicNo = 0; ApicNo < HalpMpInfoTable.IoApicCount; ApicNo++)
    {
        IoApicRegs = (PIO_APIC_REGISTERS)HalpMpInfoTable.IoApicVA[ApicNo];
        IoApicId = HalpIoApicId[ApicNo];

        IoApicRegs->IoRegisterSelect = IOAPIC_ID;
        IoApicRegs->IoWindow = ((IoApicRegs->IoWindow & 0x00FFFFFF) | SET_IOAPIC_ID(IoApicId));

        IoApicRegs->IoRegisterSelect = IOAPIC_VER;
        MaxRedirectRegs = GET_IOAPIC_MRE(IoApicRegs->IoWindow);

        for (Idx = 0; Idx <= (MaxRedirectRegs * 2); Idx += 2)
        {
            IoApicRegs->IoRegisterSelect = IOAPIC_REDTBL + Idx;

            if ((IoApicRegs->IoWindow & IOAPIC_TBL_DELMOD) != IOAPIC_DM_SMI)
            {
                IoApicRegs->IoRegisterSelect = IOAPIC_REDTBL + Idx;
                IoApicRegs->IoWindow |= (IOAPIC_TBL_IM | IOAPIC_TBL_VECTOR);
            }
        }
    }

    if (HalpHiberInProgress)
    {
        return;
    }

    HalpApicUsage.Next = NULL;
    HalpApicUsage.Type = CmResourceTypeMemory;
    HalpApicUsage.Flags = IDT_DEVICE;

    HalpApicUsage.Element[0].Start = HalpMpInfoTable.LocalApicPA;
    HalpApicUsage.Element[0].Length = IOAPIC_SIZE;

    ASSERT(HalpMpInfoTable.IoApicCount <= MAX_IOAPICS);

    for (ApicNo = 0; ApicNo < HalpMpInfoTable.IoApicCount; ApicNo++)
    {
        HalpApicUsage.Element[ApicNo + 1].Start = HalpMpInfoTable.IoApicPA[ApicNo];
        HalpApicUsage.Element[ApicNo + 1].Length = IOAPIC_SIZE;
    }

    HalpApicUsage.Element[ApicNo + 1].Start = 0;
    HalpApicUsage.Element[ApicNo + 1].Length = 0;

    HalpApicUsage.Next = HalpAddressUsageList;
    HalpAddressUsageList = (PADDRESS_USAGE)&HalpApicUsage;
}

CODE_SEG("INIT")
BOOLEAN
NTAPI
HalpPmTimerScaleTimers()
{
    DPRINT1("HalpPmTimerScaleTimers(). DbgBreakPoint()\n");
    DbgBreakPoint();
    return FALSE;
}

CODE_SEG("INIT")
VOID
NTAPI
HaliInitializeCmos(VOID)
{
    /* Set default century offset byte */
    if (HalpFixedAcpiDescTable.century_alarm_index)
    {
        HalpCmosCenturyOffset = HalpFixedAcpiDescTable.century_alarm_index;
    }
    else
    {
        HalpCmosCenturyOffset = 50;
    }
}

VOID
HalpInitPhase0a(_In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PMEMORY_ALLOCATION_DESCRIPTOR MemDescriptor;
    PHALP_PCR_HAL_RESERVED HalReserved;
    PLIST_ENTRY Entry;
    USHORT IntI;

    /* Initialize ACPI */
    HalpSetupAcpiPhase0(LoaderBlock);

    /* Initialize CMOS lock */
    KeInitializeSpinLock(&HalpSystemHardwareLock);

    if (HalpUsePmTimer)
    {
        DPRINT1("HalpInitPhase0a: HalpUsePmTimer is TRUE. DbgBreakPoint()\n");
        DbgBreakPoint();
        //HalpSetPmTimerFunction();
    }

    if (HalpForceClusteredApicMode)
    {
        HalpMaxProcsPerCluster = APIC_MAX_CPU_PER_CLUSTER;
    }

    HalpInitializeApicAddressing();

    if (HalDispatchTableVersion >= HAL_DISPATCH_VERSION)
    {
        /* Fill out HalDispatchTable */
        HalInitPowerManagement = HaliInitPowerManagement;

        /* Fill out HalPrivateDispatchTable */
        HalHaltSystem = HalAcpiHaltSystem;
    }

    HalpBuildIpiDestinationMap(0);

    RtlZeroMemory(&HalpApicUsage, APIC_ADDRESS_USAGE_SIZE);

    HalpInitIntiInfo();
    HalpInitializeIOUnits();

    /* Initialize the PICs */
    HalpInitializePICs(TRUE);

    /* Initialize CMOS */
    HaliInitializeCmos();

    if (!HalpGetApicInterruptDesc(HalpPicVectorRedirect[APIC_CLOCK_INDEX], &IntI))
    {
        DPRINT1("HalpInitPhase0a: No RTC device interrupt. DbgBreakPoint()\n");
        DbgBreakPoint();
        return;
    }

    DPRINT("HalpInitPhase0a: IntI - %X\n", IntI);

    if (!HalpPmTimerScaleTimers())
    {
        DPRINT1("HalpInitPhase0a: FIXME HalpScaleTimers(). DbgBreakPoint()\n");
        DbgBreakPoint();
        //HalpScaleTimers();
    }

    HalReserved = (PHALP_PCR_HAL_RESERVED)KeGetPcr()->HalReserved;
    HalpProc0TSCHz = HalReserved->TscHz;

    KeRegisterInterruptHandler(APIC_CLOCK_VECTOR, HalpClockInterruptStub);

    /* Enable clock interrupt handler */
    HalpVectorToINTI[APIC_CLOCK_VECTOR] = IntI;

    HalEnableSystemInterrupt(APIC_CLOCK_VECTOR, CLOCK_LEVEL, Latched);

    /* Initialize the clock */
    HalpInitializeClock();

    HalpRegisterVector(IDT_INTERNAL, APIC_NMI_VECTOR, APIC_NMI_VECTOR, HIGH_LEVEL);
    HalpRegisterVector(IDT_INTERNAL, APIC_SPURIOUS_VECTOR, APIC_SPURIOUS_VECTOR, HIGH_LEVEL);

    KeSetProfileIrql(HIGH_LEVEL);

    /*
     * We could be rebooting with a pending profile interrupt,
     * so clear it here before interrupts are enabled
     */
    HalStopProfileInterrupt(ProfileTime);

    /* Fill out HalPrivateDispatchTable */
    HalFindBusAddressTranslation = HalpFindBusAddressTranslation;

    for (Entry = LoaderBlock->MemoryDescriptorListHead.Flink;
         Entry != &LoaderBlock->MemoryDescriptorListHead;
         Entry = Entry->Flink)
    {
        MemDescriptor = CONTAINING_RECORD(Entry, MEMORY_ALLOCATION_DESCRIPTOR, ListEntry);

        if (MemDescriptor->MemoryType != LoaderFirmwarePermanent &&
            MemDescriptor->MemoryType != LoaderSpecialMemory)
        {
            if ((MemDescriptor->BasePage + MemDescriptor->PageCount) > 0x1000) // 16 Mb
            {
                LessThan16Mb = FALSE;
            }

            if ((MemDescriptor->BasePage + MemDescriptor->PageCount) > 0x100000) // 4 Gb
            {
                HalpPhysicalMemoryMayAppearAbove4GB = TRUE;
                break;
            }
        }
    }

    if (LessThan16Mb)
    {
        DPRINT1("HalpInitPhase0a: LessThan16Mb is TRUE\n");
    }

    if (HalpPhysicalMemoryMayAppearAbove4GB)
    {
        DPRINT1("HalpInitPhase0a: HalpPhysicalMemoryMayAppearAbove4GB is TRUE\n");
    }

    DPRINT("HalpInitPhase0a: FIXME MasterAdapter24:32\n");
}

VOID
NTAPI
HaliAcpiSetUsePmClock(VOID)
{
    if (HalpFixedAcpiDescTable.flags & ACPI_USE_PLATFORM_CLOCK)
    {
        DPRINT1("HaliAcpiSetUsePmClock: ACPI_USE_PLATFORM_CLOCK. DbgBreakPoint()\n");
        DbgBreakPoint();
        //HalpSetPmTimerFunction();
        //HalpUsePmTimer = 1;
    }
}

#ifndef _MINIHAL_
VOID
FASTCALL
HalpClockInterruptHandler(_In_ PKTRAP_FRAME TrapFrame)
{
    HaliClockInterrupt(TrapFrame, TRUE);
}
#endif

/* EOF */
