/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware detection routines
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES ******************************************************************/

#include <uefildr.h>
#include "../vidfb.h"

#include <debug.h>
DBG_DEFAULT_CHANNEL(HWDETECT);

/* GLOBALS *******************************************************************/

extern EFI_SYSTEM_TABLE * GlobalSystemTable;
extern EFI_HANDLE GlobalImageHandle;

/* From uefivid.c */
extern ULONG_PTR VramAddress;
extern ULONG VramSize;
extern PCM_FRAMEBUF_DEVICE_DATA FrameBufferData;

BOOLEAN AcpiPresent = FALSE;
static EFI_EVENT IdleTimerEvent = NULL;

#define PCI_MAX_BUSES 256
#define PCI_MAX_DEVICES 32
#define PCI_MAX_FUNCTIONS 8

#define PCI_HEADER_TYPE_MASK 0x7F
#define PCI_HEADER_TYPE_BRIDGE 0x01
#define PCI_HEADER_TYPE_MULTIFUNC 0x80

/* FUNCTIONS *****************************************************************/

VOID
StallExecutionProcessor(ULONG Microseconds)
{
    GlobalSystemTable->BootServices->Stall(Microseconds);
}

VOID
UefiHwIdle(VOID)
{
    UINTN Index;
    EFI_STATUS Status;
    EFI_BOOT_SERVICES *BootServices = GlobalSystemTable->BootServices;

    /* Keep one timer event around and arm it each idle tick */
    if (IdleTimerEvent == NULL)
    {
        Status = BootServices->CreateEvent(EVT_TIMER,
                                           TPL_APPLICATION,
                                           NULL,
                                           NULL,
                                           &IdleTimerEvent);
        if (EFI_ERROR(Status))
        {
            StallExecutionProcessor(10000); /* 10 ms fallback */
            return;
        }
    }

    /* Set a 10ms (100,000 * 100ns) relative timer */
    Status = BootServices->SetTimer(IdleTimerEvent, TimerRelative, 100000);
    if (!EFI_ERROR(Status))
        Status = BootServices->WaitForEvent(1, &IdleTimerEvent, &Index);
    if (EFI_ERROR(Status))
        StallExecutionProcessor(10000); /* 10 ms fallback */
}

BOOLEAN IsAcpiPresent(VOID)
{
    return AcpiPresent;
}

static
PRSDP_DESCRIPTOR
FindAcpiBios(VOID)
{
    UINTN i;
    RSDP_DESCRIPTOR* rsdp = NULL;
    EFI_GUID acpi2_guid = EFI_ACPI_20_TABLE_GUID;

    for (i = 0; i < GlobalSystemTable->NumberOfTableEntries; i++)
    {
        if (!memcmp(&GlobalSystemTable->ConfigurationTable[i].VendorGuid,
                    &acpi2_guid, sizeof(acpi2_guid)))
        {
            rsdp = (RSDP_DESCRIPTOR*)GlobalSystemTable->ConfigurationTable[i].VendorTable;
            break;
        }
    }

    return rsdp;
}

PDESCRIPTION_HEADER
UefiFindAcpiTable(
    _In_ ULONG Signature)
{
    UINTN Index, Count;
    PRSDP_DESCRIPTOR Rsdp;

    Rsdp = FindAcpiBios();
    if (Rsdp == NULL)
        return NULL;

    if ((Rsdp->revision > 0) && (Rsdp->xsdt_physical_address != 0))
    {
        PXSDT Xsdt = (PXSDT)(ULONG_PTR)Rsdp->xsdt_physical_address;

        if ((Xsdt != NULL) && (Xsdt->Header.Length >= sizeof(Xsdt->Header)))
        {
            Count = (Xsdt->Header.Length - sizeof(Xsdt->Header)) / sizeof(Xsdt->Tables[0]);
            for (Index = 0; Index < Count; ++Index)
            {
                PDESCRIPTION_HEADER Header =
                    (PDESCRIPTION_HEADER)(ULONG_PTR)Xsdt->Tables[Index].QuadPart;

                if ((Header != NULL) && (Header->Signature == Signature))
                    return Header;
            }
        }
    }

    if (Rsdp->rsdt_physical_address != 0)
    {
        PRSDT Rsdt = (PRSDT)(ULONG_PTR)Rsdp->rsdt_physical_address;

        if ((Rsdt != NULL) && (Rsdt->Header.Length >= sizeof(Rsdt->Header)))
        {
            Count = (Rsdt->Header.Length - sizeof(Rsdt->Header)) / sizeof(Rsdt->Tables[0]);
            for (Index = 0; Index < Count; ++Index)
            {
                PDESCRIPTION_HEADER Header =
                    (PDESCRIPTION_HEADER)(ULONG_PTR)Rsdt->Tables[Index];

                if ((Header != NULL) && (Header->Signature == Signature))
                    return Header;
            }
        }
    }

    return NULL;
}

VOID
DetectAcpiBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber)
{
    PCONFIGURATION_COMPONENT_DATA BiosKey;
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PRSDP_DESCRIPTOR Rsdp;
    PACPI_BIOS_DATA AcpiBiosData;
    ULONG TableSize, Size;

    Rsdp = FindAcpiBios();

    if (Rsdp)
    {
        /* Set up the flag in the loader block */
        AcpiPresent = TRUE;

        /* Calculate the table size */
        TableSize = sizeof(ACPI_BIOS_DATA);

        /* Set 'Configuration Data' value */
        Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors[1]) + TableSize;
        PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
        if (PartialResourceList == NULL)
        {
            ERR("Failed to allocate resource descriptor\n");
            return;
        }

        RtlZeroMemory(PartialResourceList, Size);
        PartialResourceList->Version = 0;
        PartialResourceList->Revision = 0;
        PartialResourceList->Count = 1;

        PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
        PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
        PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
        PartialDescriptor->u.DeviceSpecificData.DataSize = TableSize;

        /* Fill the table */
        AcpiBiosData = (PACPI_BIOS_DATA)(PartialDescriptor + 1);

        if (Rsdp->revision > 0)
        {
            TRACE("ACPI >1.0, using XSDT address\n");
            AcpiBiosData->RSDTAddress.QuadPart = Rsdp->xsdt_physical_address;
        }
        else
        {
            TRACE("ACPI 1.0, using RSDT address\n");
            AcpiBiosData->RSDTAddress.LowPart = Rsdp->rsdt_physical_address;
        }

        AcpiBiosData->Count = 0;

        TRACE("RSDT %p, data size %x\n", Rsdp->rsdt_physical_address, TableSize);

        /* Create new bus key */
        FldrCreateComponentKey(SystemKey,
                               AdapterClass,
                               MultiFunctionAdapter,
                               0x0,
                               0x0,
                               0xFFFFFFFF,
                               "ACPI BIOS",
                               PartialResourceList,
                               Size,
                               &BiosKey);

        /* Increment bus number */
        (*BusNumber)++;
    }
}

static VOID
DetectDisplayController(
    _In_ PCONFIGURATION_COMPONENT_DATA BusKey)
{
    PCONFIGURATION_COMPONENT_DATA ControllerKey;
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PCM_FRAMEBUF_DEVICE_DATA FramebufData;
    ULONG Size;

    if (!VramAddress || (VramSize == 0) || !FrameBufferData)
        return;

    Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors[2]) + sizeof(*FramebufData);
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate resource descriptor\n");
        return;
    }

    /* Initialize resource descriptor */
    RtlZeroMemory(PartialResourceList, Size);
    PartialResourceList->Version  = 1;
    PartialResourceList->Revision = 2;
    PartialResourceList->Count = 2;

    /* Set Memory */
    PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
    PartialDescriptor->Type = CmResourceTypeMemory;
    PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    PartialDescriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
    PartialDescriptor->u.Memory.Start.QuadPart = VramAddress;
    PartialDescriptor->u.Memory.Length = VramSize;

    /* Set framebuffer-specific data */
    PartialDescriptor = &PartialResourceList->PartialDescriptors[1];
    PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
    PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
    PartialDescriptor->Flags = 0;
    PartialDescriptor->u.DeviceSpecificData.DataSize = sizeof(*FramebufData);

    /* Get pointer to framebuffer-specific data */
    FramebufData = (PCM_FRAMEBUF_DEVICE_DATA)(PartialDescriptor + 1);
    RtlCopyMemory(FramebufData, FrameBufferData, sizeof(*FrameBufferData));
    FramebufData->Version  = 1;
    FramebufData->Revision = 3;
    FramebufData->VideoClock = 0; // FIXME: Use EDID

    FldrCreateComponentKey(BusKey,
                           ControllerClass,
                           DisplayController,
                           Output | ConsoleOut,
                           0,
                           0xFFFFFFFF,
                           "UEFI GOP Framebuffer",
                           PartialResourceList,
                           Size,
                           &ControllerKey);

    // NOTE: Don't add a MonitorPeripheral for now.
    // We should use EDID data for it.
}

static
VOID
DetectInternal(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCONFIGURATION_COMPONENT_DATA BusKey;
    ULONG Size;

    /* Set 'Configuration Data' value */
    Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors);
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate resource descriptor\n");
        return;
    }

    /* Initialize resource descriptor */
    RtlZeroMemory(PartialResourceList, Size);
    PartialResourceList->Version  = 1;
    PartialResourceList->Revision = 1;
    PartialResourceList->Count = 0;

    /* Create new bus key */
    FldrCreateComponentKey(SystemKey,
                           AdapterClass,
                           MultiFunctionAdapter,
                           0,
                           0,
                           0xFFFFFFFF,
                           "UEFI Internal",
                           PartialResourceList,
                           Size,
                           &BusKey);

    /* Increment bus number */
    (*BusNumber)++;

    /* Detect devices that do not belong to "standard" buses */
    DetectDisplayController(BusKey);

    /* FIXME: Detect more devices */
}

static
ULONG
UefiPciReadConfigDword(
    _In_ UCHAR Bus,
    _In_ UCHAR Device,
    _In_ UCHAR Function,
    _In_ UCHAR Register)
{
    PCI_TYPE1_CFG_BITS Cfg;

    Cfg.u.AsULONG = 0;
    Cfg.u.bits.Enable = 1;
    Cfg.u.bits.BusNumber = Bus;
    Cfg.u.bits.DeviceNumber = Device;
    Cfg.u.bits.FunctionNumber = Function;
    Cfg.u.bits.RegisterNumber = Register & ~3;

    WRITE_PORT_ULONG(PCI_TYPE1_ADDRESS_PORT, Cfg.u.AsULONG);
    return READ_PORT_ULONG((PULONG)PCI_TYPE1_DATA_PORT);
}

static
BOOLEAN
UefiScanPciFunction(
    _In_ UCHAR Bus,
    _In_ UCHAR Device,
    _In_ UCHAR Function,
    _Inout_updates_(PCI_MAX_BUSES) BOOLEAN *ScannedBuses,
    _Inout_updates_(PCI_MAX_BUSES) UCHAR *PendingBuses,
    _Inout_ ULONG *PendingCount)
{
    ULONG VendorDevice, HeaderTypeDword, BridgeBusNumbers;
    UCHAR HeaderType, SecondaryBus, SubordinateBus, b;

    VendorDevice = UefiPciReadConfigDword(Bus, Device, Function, 0x00);
    if ((VendorDevice & 0xFFFF) == 0xFFFF)
        return FALSE;

    HeaderTypeDword = UefiPciReadConfigDword(Bus, Device, Function, 0x0C);
    HeaderType = (UCHAR)((HeaderTypeDword >> 16) & 0xFF);

    if ((HeaderType & PCI_HEADER_TYPE_MASK) == PCI_HEADER_TYPE_BRIDGE)
    {
        BridgeBusNumbers = UefiPciReadConfigDword(Bus, Device, Function, 0x18);
        SecondaryBus = (UCHAR)((BridgeBusNumbers >> 8) & 0xFF);
        SubordinateBus = (UCHAR)((BridgeBusNumbers >> 16) & 0xFF);

        if (SecondaryBus != 0 && SecondaryBus != Bus && SubordinateBus >= SecondaryBus)
        {
            for (b = SecondaryBus; b <= SubordinateBus; b++)
            {
                if (!ScannedBuses[b] && (*PendingCount < PCI_MAX_BUSES))
                {
                    ScannedBuses[b] = TRUE;
                    PendingBuses[(*PendingCount)++] = b;
                }

                if (b == 255)
                    break; /* avoid UCHAR wraparound */
            }
        }
    }

    return TRUE;
}

static
BOOLEAN
UefiFindPciBios(
    _Out_ PPCI_REGISTRY_INFO BusData)
{
    PMCFG_TABLE Mcfg;
    PMCFG_ALLOCATION Alloc;
    ULONG McfgCount, McfgIndex;
    UCHAR HighestMcfgBus;
    BOOLEAN FoundMcfgSegment0 = FALSE;
    BOOLEAN ScannedBuses[PCI_MAX_BUSES];
    UCHAR PendingBuses[PCI_MAX_BUSES];
    ULONG PendingCount = 0;
    ULONG QueueHead = 0;
    UCHAR Bus, HighestBus = 0;
    ULONG Device, Function, HeaderTypeDword;
    BOOLEAN AnyFound = FALSE;

    /* Prefer MCFG since it lists each segment's bus range directly */
    Mcfg = (PMCFG_TABLE)UefiFindAcpiTable(MCFG_SIGNATURE);
    if ((Mcfg != NULL) && (Mcfg->Header.Length >= sizeof(MCFG_TABLE)))
    {
        McfgCount = (Mcfg->Header.Length - sizeof(MCFG_TABLE)) / sizeof(MCFG_ALLOCATION) + 1;
        HighestMcfgBus = 0;

        for (McfgIndex = 0; McfgIndex < McfgCount; McfgIndex++)
        {
            Alloc = &Mcfg->Allocation[McfgIndex];

            if (Alloc->PciSegmentGroup != 0)
                continue;

            if (Alloc->EndBusNumber < Alloc->StartBusNumber)
                continue; /* malformed entry */

            FoundMcfgSegment0 = TRUE;
            if (Alloc->EndBusNumber > HighestMcfgBus)
                HighestMcfgBus = Alloc->EndBusNumber;
        }

        if (FoundMcfgSegment0)
        {
            BusData->MajorRevision = 3;
            BusData->MinorRevision = 0;
            BusData->NoBuses = HighestMcfgBus + 1;
            BusData->HardwareMechanism = 1;

            TRACE("UEFI PCI: %u bus(es) found via MCFG\n", BusData->NoBuses);
            return TRUE;
        }
    }

    RtlZeroMemory(ScannedBuses, sizeof(ScannedBuses));

    ScannedBuses[0] = TRUE;
    PendingBuses[PendingCount++] = 0;

    while (QueueHead < PendingCount)
    {
        Bus = PendingBuses[QueueHead++];

        for (Device = 0; Device < PCI_MAX_DEVICES; Device++)
        {
            if (!UefiScanPciFunction(Bus, (UCHAR)Device, 0,
                                      ScannedBuses, PendingBuses, &PendingCount))
            {
                continue;
            }

            AnyFound = TRUE;
            if (Bus > HighestBus)
                HighestBus = Bus;

            HeaderTypeDword = UefiPciReadConfigDword(Bus, (UCHAR)Device, 0, 0x0C);
            if (!((HeaderTypeDword >> 16) & PCI_HEADER_TYPE_MULTIFUNC))
                continue;

            for (Function = 1; Function < PCI_MAX_FUNCTIONS; Function++)
            {
                if (UefiScanPciFunction(Bus, (UCHAR)Device, (UCHAR)Function,
                                         ScannedBuses, PendingBuses, &PendingCount))
                {
                    AnyFound = TRUE;
                }
            }
        }
    }

    for (Bus = PCI_MAX_BUSES - 1; Bus > HighestBus; Bus--)
    {
        if (ScannedBuses[Bus])
        {
            HighestBus = Bus;
            break;
        }
    }

    if (!AnyFound)
    {
        WARN("No PCI devices found\n");
        return FALSE;
    }

    BusData->MajorRevision = 3;
    BusData->MinorRevision = 0;
    BusData->NoBuses = HighestBus + 1;
    BusData->HardwareMechanism = 1;

    TRACE("UEFI PCI probe: %u bus(es) found\n", BusData->NoBuses);

    return TRUE;
}

PCONFIGURATION_COMPONENT_DATA
UefiHwDetect(
    _In_opt_ PCSTR Options)
{
    PCONFIGURATION_COMPONENT_DATA SystemKey;
    ULONG BusNumber = 0;

    TRACE("DetectHardware()\n");

    /* Create the 'System' key */
#if defined(_M_IX86) || defined(_M_AMD64)
    FldrCreateSystemKey(&SystemKey, "AT/AT COMPATIBLE");
#elif defined(_M_IA64)
    FldrCreateSystemKey(&SystemKey, "Intel Itanium processor family");
#elif defined(_M_ARM) || defined(_M_ARM64)
    FldrCreateSystemKey(&SystemKey, "ARM processor family");
#else
    #error Please define a system key for your architecture
#endif

    /* Detect buses */
    DetectInternal(SystemKey, &BusNumber);
#if defined(_M_IX86) || defined(_M_AMD64)
    DetectPciBios(SystemKey, &BusNumber, UefiFindPciBios);
#endif
    DetectAcpiBios(SystemKey, &BusNumber);

    TRACE("DetectHardware() Done\n");
    return SystemKey;
}
