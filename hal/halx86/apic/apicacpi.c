/*
 * PROJECT:         ReactOS HAL
 * COPYRIGHT:       GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:            hal/halx86/apic/apicacpi.c
 * PURPOSE:         ACPI part APIC HALs code
 * PROGRAMMERS:     Copyright 2020 Vadim Galyant <vgal@rambler.ru>
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

#include "apic.h"
#include "apicacpi.h"

/* GLOBALS ********************************************************************/

APIC_INTI_INFO HalpIntiInfo[MAX_INTI];
LOCAL_APIC HalpStaticProcLocalApicTable[MAX_CPUS] = {{0}};
IO_APIC_VERSION_REGISTER HalpIOApicVersion[MAX_IOAPICS];
UCHAR HalpIoApicId[MAX_IOAPICS] = {0};
UCHAR HalpMaxProcs = 0;
BOOLEAN HalpPciLockSettings;

extern HALP_MP_INFO_TABLE HalpMpInfoTable;
extern APIC_ADDRESS_USAGE HalpApicUsage;
extern PLOCAL_APIC HalpProcLocalApicTable;
extern ULONG HalpDefaultApicDestinationModeMask;
extern ULONG HalpPicVectorRedirect[16];
extern ULONG HalpPicVectorFlags[16];
extern USHORT HalpMaxApicInti[MAX_IOAPICS];
extern UCHAR HalpMaxProcsPerCluster;
extern UCHAR HalpIRQLtoTPR[32];    // table, which sets the correspondence between IRQL levels and TPR (Task Priority Register) values.
extern KIRQL HalpVectorToIRQL[16];
extern BOOLEAN HalpForceApicPhysicalDestinationMode;

/* FUNCTIONS ******************************************************************/

CODE_SEG("INIT")
VOID
NTAPI
HalpGetParameters(IN PCHAR CommandLine)
{
    /* Check if PCI is locked */
    if (strstr(CommandLine, "PCILOCK")) HalpPciLockSettings = TRUE;

    /* Check for initial breakpoint */
    if (strstr(CommandLine, "BREAK")) DbgBreakPoint();

    if (strstr(CommandLine, "ONECPU"))
    {
        DPRINT1("HalpGetParameters: FIXME parameters [ONECPU]\n");
        DbgBreakPoint();
        //HalpDontStartProcessors++;
    }

  #ifdef CONFIG_SMP // halmacpi only
    if (strstr(CommandLine, "USEPMTIMER"))
    {
        DPRINT1("HalpGetParameters: FIXME parameters [USEPMTIMER]\n");
        DbgBreakPoint();
        //HalpUsePmTimer = TRUE;
    }
  #endif

    if (strstr(CommandLine, "INTAFFINITY"))
    {
        DPRINT1("HalpGetParameters: FIXME parameters [INTAFFINITY]\n");
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
        DPRINT1("HalpGetParameters: FIXME parameters [MAXPROCSPERCLUSTER]\n");
        DbgBreakPoint();
    }

    if (strstr(CommandLine, "MAXAPICCLUSTER"))
    {
        DPRINT1("HalpGetParameters: FIXME parameters [MAXAPICCLUSTER]\n");
        DbgBreakPoint();
    }
}

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

                IoApicRegs->IoRegisterSelect = 1;
                IoApicRegs->IoWindow = 0;

                IoApicRegs->IoRegisterSelect = 1;
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

BOOLEAN
NTAPI 
HalpVerifyIOUnit(
    _In_ PIO_APIC_REGISTERS IOUnitRegs)
{
    IO_APIC_VERSION_REGISTER IoApicVersion1;
    IO_APIC_VERSION_REGISTER IoApicVersion2;

    IOUnitRegs->IoRegisterSelect = 1;
    IOUnitRegs->IoWindow = 0;

    IOUnitRegs->IoRegisterSelect = 1;
    IoApicVersion1.AsULONG = IOUnitRegs->IoWindow;

    IOUnitRegs->IoRegisterSelect = 1;
    IOUnitRegs->IoWindow = 0;

    IOUnitRegs->IoRegisterSelect = 1;
    IoApicVersion2.AsULONG = IOUnitRegs->IoWindow;

    if (IoApicVersion1.ApicVersion != IoApicVersion2.ApicVersion ||
        IoApicVersion1.MaxRedirectionEntry != IoApicVersion2.MaxRedirectionEntry)
    {
        return FALSE;
    }

    return TRUE;
}

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

VOID
HalpInitPhase0a(_In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* Initialize ACPI */
    HalpSetupAcpiPhase0(LoaderBlock);

    if (HalDispatchTableVersion >= HAL_DISPATCH_VERSION)
    {
        /* Fill out HalDispatchTable */
        HalInitPowerManagement = HaliInitPowerManagement;

        /* Fill out HalPrivateDispatchTable */
        HalHaltSystem = HalAcpiHaltSystem;
    }

    /* Initialize the PICs */
    HalpInitializePICs(TRUE);

    /* Initialize CMOS lock */
    KeInitializeSpinLock(&HalpSystemHardwareLock);

    /* Initialize CMOS */
    HalpInitializeCmos();

    /* Setup busy waiting */
    HalpCalibrateStallExecution();

    /* Initialize the clock */
    HalpInitializeClock();

    /*
     * We could be rebooting with a pending profile interrupt,
     * so clear it here before interrupts are enabled
     */
    HalStopProfileInterrupt(ProfileTime);

    /* Enable clock interrupt handler */
    HalpEnableInterruptHandler(IDT_INTERNAL,
                               0,
                               APIC_CLOCK_VECTOR,
                               CLOCK2_LEVEL,
                               HalpClockInterrupt,
                               Latched);
}

/* EOF */
