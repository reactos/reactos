/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halx86/generic/legacy/bussupp.c
 * PURPOSE:         HAL Legacy Bus Support Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

extern KSPIN_LOCK HalpPCIConfigLock;
ULONG HalpPciIrqMask;

/* PRIVATE FUNCTIONS **********************************************************/

PBUS_HANDLER
NTAPI
HalpAllocateBusHandler(IN INTERFACE_TYPE InterfaceType,
                       IN BUS_DATA_TYPE BusDataType,
                       IN ULONG BusNumber,
                       IN INTERFACE_TYPE ParentBusInterfaceType,
                       IN ULONG ParentBusNumber,
                       IN ULONG BusSpecificData)
{
    PBUS_HANDLER Bus;

    /* Register the bus handler */
    HalRegisterBusHandler(InterfaceType,
                          BusDataType,
                          BusNumber,
                          ParentBusInterfaceType,
                          ParentBusNumber,
                          BusSpecificData,
                          NULL,
                          &Bus);
    if (!Bus)
    {
        return NULL;
    }

    /* Check for a valid interface */
    if (InterfaceType != InterfaceTypeUndefined)
    {
        /* Allocate address ranges and zero them out */
        Bus->BusAddresses = ExAllocatePoolWithTag(NonPagedPoolMustSucceed,
                                                  sizeof(SUPPORTED_RANGES),
                                                  ' laH');
        RtlZeroMemory(Bus->BusAddresses, sizeof(SUPPORTED_RANGES));

        /* Build the data structure */
        Bus->BusAddresses->Version = HAL_SUPPORTED_RANGE_VERSION;
        Bus->BusAddresses->Dma.Limit = 7;
        Bus->BusAddresses->Memory.Limit = 0xFFFFFFFF;
        Bus->BusAddresses->IO.Limit = 0xFFFF;
        Bus->BusAddresses->IO.SystemAddressSpace = 1;
        Bus->BusAddresses->PrefetchMemory.Base = 1;
    }

    /* Return the bus address */
    return Bus;
}

VOID
NTAPI
INIT_FUNCTION
HalpRegisterInternalBusHandlers(VOID)
{
    PBUS_HANDLER Bus;

    /* Only do processor 1 */
    if (KeGetCurrentPrcb()->Number) return;

    /* Register root support */
    HalpInitBusHandler();

    /* Allocate the system bus */
    Bus = HalpAllocateBusHandler(Internal,
                                 ConfigurationSpaceUndefined,
                                 0,
                                 InterfaceTypeUndefined,
                                 0,
                                 0);
    if (Bus)
    {
        /* Set it up */
        Bus->GetInterruptVector = HalpGetSystemInterruptVector;
        Bus->TranslateBusAddress = HalpTranslateSystemBusAddress;
    }

    /* Allocate the CMOS bus */
    Bus = HalpAllocateBusHandler(InterfaceTypeUndefined,
                                 Cmos,
                                 0,
                                 InterfaceTypeUndefined,
                                 0,
                                 0);
    if (Bus)
    {
        /* Set it up */
        Bus->GetBusData = HalpcGetCmosData;
        Bus->SetBusData = HalpcSetCmosData;
    }

    /* Allocate the CMOS bus */
    Bus = HalpAllocateBusHandler(InterfaceTypeUndefined,
                                 Cmos,
                                 1,
                                 InterfaceTypeUndefined,
                                 0,
                                 0);
    if (Bus)
    {
        /* Set it up */
        Bus->GetBusData = HalpcGetCmosData;
        Bus->SetBusData = HalpcSetCmosData;
    }

    /* Allocate ISA bus */
    Bus = HalpAllocateBusHandler(Isa,
                                 ConfigurationSpaceUndefined,
                                 0,
                                 Internal,
                                 0,
                                 0);
    if (Bus)
    {
        /* Set it up */
        Bus->GetBusData = HalpNoBusData;
        Bus->BusAddresses->Memory.Limit = 0xFFFFFF;
        Bus->TranslateBusAddress = HalpTranslateIsaBusAddress;
    }

    /* No support for EISA or MCA */
    ASSERT(HalpBusType == MACHINE_TYPE_ISA);
}

#ifndef _MINIHAL_
NTSTATUS
NTAPI
INIT_FUNCTION
HalpMarkChipsetDecode(BOOLEAN OverrideEnable)
{
    NTSTATUS Status;
    UNICODE_STRING KeyString;
    ULONG Data = OverrideEnable;
    HANDLE KeyHandle, Handle;

    /* Open CCS key */
    RtlInitUnicodeString(&KeyString,
                         L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET");
    Status = HalpOpenRegistryKey(&Handle, 0, &KeyString, KEY_ALL_ACCESS, FALSE);
    if (NT_SUCCESS(Status))
    {
        /* Open PNP Bios key */
        RtlInitUnicodeString(&KeyString, L"Control\\Biosinfo\\PNPBios");
        Status = HalpOpenRegistryKey(&KeyHandle,
                                     Handle,
                                     &KeyString,
                                     KEY_ALL_ACCESS,
                                     TRUE);

        /* Close root key */
        ZwClose(Handle);

        /* Check if PNP BIOS key exists */
        if (NT_SUCCESS(Status))
        {
            /* Set the override value */
            RtlInitUnicodeString(&KeyString, L"FullDecodeChipsetOverride");
            Status = ZwSetValueKey(KeyHandle,
                                   &KeyString,
                                   0,
                                   REG_DWORD,
                                   &Data,
                                   sizeof(Data));

            /* Close subkey */
            ZwClose(KeyHandle);
        }
    }

    /* Return status */
    return Status;
}

PBUS_HANDLER
NTAPI
INIT_FUNCTION
HalpAllocateAndInitPciBusHandler(IN ULONG PciType,
                                 IN ULONG BusNo,
                                 IN BOOLEAN TestAllocation)
{
    PBUS_HANDLER Bus;
    PPCIPBUSDATA BusData;

    /* Allocate the bus handler */
    Bus = HalpAllocateBusHandler(PCIBus,
                                 PCIConfiguration,
                                 BusNo,
                                 Internal,
                                 0,
                                 sizeof(PCIPBUSDATA));

    /* Set it up */
    Bus->GetBusData = HalpGetPCIData;
    Bus->SetBusData = HalpSetPCIData;
    Bus->GetInterruptVector = HalpGetPCIIntOnISABus;
    Bus->AdjustResourceList = HalpAdjustPCIResourceList;
    Bus->AssignSlotResources = HalpAssignPCISlotResources;
    Bus->BusAddresses->Dma.Limit = 0;

    /* Get our custom bus data */
    BusData = (PPCIPBUSDATA)Bus->BusData;

    /* Setup custom bus data */
    BusData->CommonData.Tag = PCI_DATA_TAG;
    BusData->CommonData.Version = PCI_DATA_VERSION;
    BusData->CommonData.ReadConfig = HalpReadPCIConfig;
    BusData->CommonData.WriteConfig = HalpWritePCIConfig;
    BusData->CommonData.Pin2Line = HalpPCIPin2ISALine;
    BusData->CommonData.Line2Pin = HalpPCIISALine2Pin;
    BusData->MaxDevice = PCI_MAX_DEVICES;
    BusData->GetIrqRange = HalpGetISAFixedPCIIrq;

    /* Initialize the bitmap */
    RtlInitializeBitMap(&BusData->DeviceConfigured, BusData->ConfiguredBits, 256);

    /* Check the type of PCI bus */
    switch (PciType)
    {
        /* Type 1 PCI Bus */
        case 1:

            /* Copy the Type 1 handler data */
            RtlCopyMemory(&PCIConfigHandler,
                          &PCIConfigHandlerType1,
                          sizeof(PCIConfigHandler));

            /* Set correct I/O Ports */
            BusData->Config.Type1.Address = PCI_TYPE1_ADDRESS_PORT;
            BusData->Config.Type1.Data = PCI_TYPE1_DATA_PORT;
            break;

        /* Type 2 PCI Bus */
        case 2:

            /* Copy the Type 1 handler data */
            RtlCopyMemory(&PCIConfigHandler,
                          &PCIConfigHandlerType2,
                          sizeof (PCIConfigHandler));

            /* Set correct I/O Ports */
            BusData->Config.Type2.CSE = PCI_TYPE2_CSE_PORT;
            BusData->Config.Type2.Forward = PCI_TYPE2_FORWARD_PORT;
            BusData->Config.Type2.Base = PCI_TYPE2_ADDRESS_BASE;

            /* Only 16 devices supported, not 32 */
            BusData->MaxDevice = 16;
            break;

        default:

            /* Invalid type */
            DbgPrint("HAL: Unnkown PCI type\n");
    }

    /* Return the bus handler */
    return Bus;
}

BOOLEAN
NTAPI
INIT_FUNCTION
HalpIsValidPCIDevice(IN PBUS_HANDLER BusHandler,
                     IN PCI_SLOT_NUMBER Slot)
{
    UCHAR DataBuffer[PCI_COMMON_HDR_LENGTH];
    PPCI_COMMON_CONFIG PciHeader = (PVOID)DataBuffer;
    ULONG i;
    ULONG_PTR Address;

    /* Read the PCI header */
    HalpReadPCIConfig(BusHandler, Slot, PciHeader, 0, PCI_COMMON_HDR_LENGTH);

    /* Make sure it's a valid device */
    if ((PciHeader->VendorID == PCI_INVALID_VENDORID) ||
        (PCI_CONFIGURATION_TYPE(PciHeader) != PCI_DEVICE_TYPE))
    {
        /* Bail out */
        return FALSE;
    }

    /* Make sure interrupt numbers make sense */
    if (((PciHeader->u.type0.InterruptPin) &&
         (PciHeader->u.type0.InterruptPin > 4)) ||
        (PciHeader->u.type0.InterruptLine & 0x70))
    {
        /* Bail out */
        return FALSE;
    }

    /* Now scan PCI BARs */
    for (i = 0; i < PCI_TYPE0_ADDRESSES; i++)
    {
        /* Check what kind of address it is */
        Address = PciHeader->u.type0.BaseAddresses[i];
        if (Address & PCI_ADDRESS_IO_SPACE)
        {
            /* Highest I/O port is 65535 */
            if (Address > 0xFFFF) return FALSE;
        }
        else
        {
            /* MMIO should be higher than 0x80000 */
            if ((Address > 0xF) && (Address < 0x80000)) return FALSE;
        }

        /* Is this a 64-bit address? */
        if (!(Address & PCI_ADDRESS_IO_SPACE) &&
            ((Address & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_64BIT))
        {
            /* Check the next-next entry, since this one 64-bits wide */
            i++;
        }
    }

    /* Header, interrupt and address data all make sense */
    return TRUE;
}

static BOOLEAN WarningsGiven[5];

NTSTATUS
NTAPI
INIT_FUNCTION
HalpGetChipHacks(IN USHORT VendorId,
                 IN USHORT DeviceId,
                 IN UCHAR RevisionId,
                 IN PULONG HackFlags)
{
    UNICODE_STRING KeyName, ValueName;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    WCHAR Buffer[32];
    KEY_VALUE_PARTIAL_INFORMATION PartialInfo;
    ULONG ResultLength;

    /* Setup the object attributes for the key */
    RtlInitUnicodeString(&KeyName,
                         L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\"
                         L"Control\\HAL");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Open the key */
    Status = ZwOpenKey(&KeyHandle, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status)) return Status;

    /* Query value */
    swprintf(Buffer, L"%04X%04X", VendorId, DeviceId);
    RtlInitUnicodeString(&ValueName, Buffer);
    Status = ZwQueryValueKey(KeyHandle,
                             &ValueName,
                             KeyValuePartialInformation,
                             &PartialInfo,
                             sizeof(PartialInfo),
                             &ResultLength);
    if (NT_SUCCESS(Status))
    {
        /* Return the flags */
        DbgPrint("\tFound HackFlags for your chipset\n");
        *HackFlags = *(PULONG)PartialInfo.Data;
        DbgPrint("\t\tHack Flags: %lx (Hack Revision: %lx-Your Revision: %lx)\n",
                 *HackFlags, HALP_REVISION_FROM_HACK_FLAGS(*HackFlags), RevisionId);

        /* Does it apply to this revision? */
        if ((RevisionId) && (RevisionId >= (HALP_REVISION_FROM_HACK_FLAGS(*HackFlags))))
        {
            /* Read the revision flags */
            *HackFlags = HALP_REVISION_HACK_FLAGS(*HackFlags);
        }

        /* Throw out revision data */
        *HackFlags = HALP_HACK_FLAGS(*HackFlags);
        if (!*HackFlags) DbgPrint("\tNo HackFlags for your chipset's revision!\n");
    }

    /* Close the handle and return */
    ZwClose(KeyHandle);
    return Status;
}

BOOLEAN
NTAPI
INIT_FUNCTION
HalpIsRecognizedCard(IN PPCI_REGISTRY_INFO_INTERNAL PciRegistryInfo,
                     IN PPCI_COMMON_CONFIG PciData,
                     IN ULONG Flags)
{
    ULONG ElementCount, i;
    PPCI_CARD_DESCRIPTOR CardDescriptor;

    /* How many PCI Cards that we know about? */
    ElementCount = PciRegistryInfo->ElementCount;
    if (!ElementCount) return FALSE;

    /* Loop all descriptors */
    CardDescriptor = &PciRegistryInfo->CardList[0];
    for (i = 0; i < ElementCount; i++, CardDescriptor++)
    {
        /* Check for flag match */
        if (CardDescriptor->Flags != Flags) continue;

        /* Check for VID-PID match */
        if ((CardDescriptor->VendorID != PciData->VendorID) ||
            (CardDescriptor->DeviceID != PciData->DeviceID))
        {
            /* Skip */
            continue;
        }

        /* Check for revision match, if requested */
        if ((CardDescriptor->Flags & HALP_CHECK_CARD_REVISION_ID) &&
            (CardDescriptor->RevisionID != PciData->RevisionID))
        {
            /* Skip */
            continue;
        }

        /* Check what kind of device this is */
        switch (PCI_CONFIGURATION_TYPE(PciData))
        {
            /* CardBUS Bridge */
            case PCI_CARDBUS_BRIDGE_TYPE:

                /* This means the real device header is in the device-specific data */
                PciData = (PPCI_COMMON_CONFIG)PciData->DeviceSpecific;

            /* Normal PCI device */
            case PCI_DEVICE_TYPE:

                /* Check for subvendor match, if requested */
                if ((CardDescriptor->Flags & HALP_CHECK_CARD_SUBVENDOR_ID) &&
                    (CardDescriptor->SubsystemVendorID != PciData->u.type0.SubVendorID))
                {
                    /* Skip */
                    continue;
                }

                /* Check for subsystem match, if requested */
                if ((CardDescriptor->Flags & HALP_CHECK_CARD_SUBSYSTEM_ID) &&
                    (CardDescriptor->SubsystemID != PciData->u.type0.SubSystemID))
                {
                    /* Skip */
                    continue;
                }

                /* You made it! */
                return TRUE;

            /* PCI Bridge -- don't bother */
            case PCI_BRIDGE_TYPE:
            default:

                /* Recognize it */
                return TRUE;
        }
    }

    /* This means the card isn't recognized */
    return FALSE;
}

BOOLEAN
NTAPI
INIT_FUNCTION
HalpIsIdeDevice(IN PPCI_COMMON_CONFIG PciData)
{
    /* Simple test first */
    if ((PciData->BaseClass == PCI_CLASS_MASS_STORAGE_CTLR) &&
        (PciData->SubClass == PCI_SUBCLASS_MSC_IDE_CTLR))
    {
        /* The device is nice enough to admit it */
        return TRUE;
    }

    /* Symphony 82C101 */
    if (PciData->VendorID == 0x1C1C) return TRUE;

    /* ALi MS4803 or M5219 */
    if ((PciData->VendorID == 0x10B9) &&
        ((PciData->DeviceID == 0x5215) || (PciData->DeviceID == 0x5219)))
    {
        return TRUE;
    }

    /* Appian Technology */
    if ((PciData->VendorID == 0x1097) && (PciData->DeviceID == 0x38)) return TRUE;

    /* Compaq Triflex Dual EIDE Controller */
    if ((PciData->VendorID == 0xE11) && (PciData->DeviceID == 0xAE33)) return TRUE;

    /* Micron PC Tech RZ1000 */
    if ((PciData->VendorID == 0x1042) && (PciData->DeviceID == 0x1000)) return TRUE;

    /* SiS 85C601 or 5513 [IDE] */
    if ((PciData->VendorID == 0x1039) &&
        ((PciData->DeviceID == 0x601) || (PciData->DeviceID == 0x5513)))
    {
        return TRUE;
    }

    /* Symphony Labs W83769F */
    if ((PciData->VendorID == 0x10AD) &&
        ((PciData->DeviceID == 0x1) || (PciData->DeviceID == 0x150)))
    {
        return TRUE;
    }

    /* UMC UM8673F */
    if ((PciData->VendorID == 0x1060) && (PciData->DeviceID == 0x101)) return TRUE;

    /* You've survived */
    return FALSE;
}

BOOLEAN
NTAPI
INIT_FUNCTION
HalpIsBridgeDevice(IN PPCI_COMMON_CONFIG PciData)
{
    /* Either this is a PCI-to-PCI Bridge, or a CardBUS Bridge */
    return (((PCI_CONFIGURATION_TYPE(PciData) == PCI_BRIDGE_TYPE) &&
             (PciData->BaseClass == PCI_CLASS_BRIDGE_DEV) &&
             (PciData->SubClass == PCI_SUBCLASS_BR_PCI_TO_PCI)) ||
            ((PCI_CONFIGURATION_TYPE(PciData) == PCI_CARDBUS_BRIDGE_TYPE) &&
             (PciData->BaseClass == PCI_CLASS_BRIDGE_DEV) &&
             (PciData->SubClass == PCI_SUBCLASS_BR_CARDBUS)));
}

BOOLEAN
NTAPI
INIT_FUNCTION
HalpGetPciBridgeConfig(IN ULONG PciType,
                       IN PUCHAR BusCount)
{
    PCI_SLOT_NUMBER PciSlot;
    ULONG i, j, k;
    UCHAR DataBuffer[PCI_COMMON_HDR_LENGTH];
    PPCI_COMMON_CONFIG PciData = (PPCI_COMMON_CONFIG)DataBuffer;
    PBUS_HANDLER BusHandler;

    /* Loop PCI buses */
    PciSlot.u.bits.Reserved = 0;
    for (i = 0; i < *BusCount; i++)
    {
        /* Get the bus handler */
        BusHandler = HalHandlerForBus(PCIBus, i);

        /* Loop every device */
        for (j = 0; j < PCI_MAX_DEVICES; j++)
        {
            /* Loop every function */
            PciSlot.u.bits.DeviceNumber = j;
            for (k = 0; k < PCI_MAX_FUNCTION; k++)
            {
                /* Build the final slot structure */
                PciSlot.u.bits.FunctionNumber = k;

                /* Read the configuration information */
                HalpReadPCIConfig(BusHandler,
                                  PciSlot,
                                  PciData,
                                  0,
                                  PCI_COMMON_HDR_LENGTH);

                /* Skip if this is an invalid function */
                if (PciData->VendorID == PCI_INVALID_VENDORID) continue;

                /* Make sure that this is a PCI bridge or a cardbus bridge */
                if (!HalpIsBridgeDevice(PciData)) continue;

                /* Not supported */
                if (!WarningsGiven[2]++) DPRINT1("Your machine has a PCI-to-PCI or CardBUS Bridge. PCI devices may fail!\n");
                continue;
            }
        }
    }

    /* If we exited the loop, then there's no bridge to worry about */
    return FALSE;
}

VOID
NTAPI
INIT_FUNCTION
HalpFixupPciSupportedRanges(IN ULONG BusCount)
{
    ULONG i;
    PBUS_HANDLER Bus, ParentBus;

    /* Loop all buses */
    for (i = 0; i < BusCount; i++)
    {
        /* Get PCI bus handler */
        Bus = HalHandlerForBus(PCIBus, i);

        /* Loop all parent buses */
        ParentBus = Bus->ParentHandler;
        while (ParentBus)
        {
            /* Should merge addresses */
            if (!WarningsGiven[0]++) DPRINT1("Found parent bus (indicating PCI Bridge). PCI devices may fail!\n");

            /* Check the next parent */
            ParentBus = ParentBus->ParentHandler;
        }
    }

    /* Loop all buses again */
    for (i = 0; i < BusCount; i++)
    {
        /* Get PCI bus handler */
        Bus = HalHandlerForBus(PCIBus, i);

        /* Check if this is a PCI 2.2 Bus with Subtractive Decode */
        if (!((PPCIPBUSDATA)Bus->BusData)->Subtractive)
        {
            /* Loop all parent buses */
            ParentBus = Bus->ParentHandler;
            while (ParentBus)
            {
                /* But check only PCI parent buses specifically */
                if (ParentBus->InterfaceType == PCIBus)
                {
                    /* Should trim addresses */
                    if (!WarningsGiven[1]++) DPRINT1("Found parent PCI Bus (indicating PCI-to-PCI Bridge). PCI devices may fail!\n");
                }

                /* Check the next parent */
                ParentBus = ParentBus->ParentHandler;
            }
        }
    }

    /* Loop buses one last time */
    for (i = 0; i < BusCount; i++)
    {
        /* Get the PCI bus handler */
        Bus = HalHandlerForBus(PCIBus, i);

        /* Sort and combine (trim) bus address range information */
        DPRINT("Warning: Bus addresses not being optimized!\n");
    }
}

VOID
NTAPI
INIT_FUNCTION
ShowSize(ULONG x)
{
    if (!x) return;
    DbgPrint(" [size=");
    if (x < 1024)
    {
        DbgPrint("%d", (int) x);
    }
    else if (x < 1048576)
    {
        DbgPrint("%dK", (int)(x / 1024));
    }
    else if (x < 0x80000000)
    {
        DbgPrint("%dM", (int)(x / 1048576));
    }
    else
    {
        DbgPrint("%d", x);
    }
    DbgPrint("]\n");
}

/*
 * These includes are required to define
 * the ClassTable and VendorTable arrays.
 */
#include "pci_classes.h"
#include "pci_vendors.h"
VOID
NTAPI
INIT_FUNCTION
HalpDebugPciDumpBus(IN ULONG i,
                    IN ULONG j,
                    IN ULONG k,
                    IN PPCI_COMMON_CONFIG PciData)
{
    PCHAR p, ClassName, SubClassName, VendorName, ProductName, SubVendorName;
    ULONG Length;
    CHAR LookupString[16] = "";
    CHAR bSubClassName[64] = "";
    CHAR bVendorName[64] = "";
    CHAR bProductName[128] = "Unknown device";
    CHAR bSubVendorName[128] = "Unknown";
    ULONG Size, Mem, b;

    /* Isolate the class name */
    sprintf(LookupString, "C %02x  ", PciData->BaseClass);
    ClassName = strstr(ClassTable, LookupString);
    if (ClassName)
    {
        /* Isolate the subclass name */
        ClassName += 6;
        sprintf(LookupString, "\t%02x  ", PciData->SubClass);
        SubClassName = strstr(ClassName, LookupString);
        if (SubClassName)
        {
            /* Copy the subclass into our buffer */
            SubClassName += 5;
            p = strpbrk(SubClassName, "\r\n");
            Length = p - SubClassName;
            if (Length >= sizeof(bSubClassName)) Length = sizeof(bSubClassName) - 1;
            strncpy(bSubClassName, SubClassName, Length);
            bSubClassName[Length] = '\0';
        }
    }

    /* Isolate the vendor name */
    sprintf(LookupString, "\r\n%04x  ", PciData->VendorID);
    VendorName = strstr(VendorTable, LookupString);
    if (VendorName)
    {
        /* Copy the vendor name into our buffer */
        VendorName += 8;
        p = strpbrk(VendorName, "\r\n");
        Length = p - VendorName;
        if (Length >= sizeof(bVendorName)) Length = sizeof(bVendorName) - 1;
        strncpy(bVendorName, VendorName, Length);
        bVendorName[Length] = '\0';

        /* Isolate the product name */
        sprintf(LookupString, "\t%04x  ", PciData->DeviceID);
        ProductName = strstr(VendorName, LookupString);
        if (ProductName)
        {
            /* Copy the product name into our buffer */
            ProductName += 7;
            p = strpbrk(ProductName, "\r\n");
            Length = p - ProductName;
            if (Length >= sizeof(bProductName)) Length = sizeof(bProductName) - 1;
            strncpy(bProductName, ProductName, Length);
            bProductName[Length] = '\0';

            /* Isolate the subvendor and subsystem name */
            sprintf(LookupString,
                    "\t\t%04x %04x  ",
                    PciData->u.type0.SubVendorID,
                    PciData->u.type0.SubSystemID);
            SubVendorName = strstr(ProductName, LookupString);
            if (SubVendorName)
            {
                /* Copy the subvendor name into our buffer */
                SubVendorName += 13;
                p = strpbrk(SubVendorName, "\r\n");
                Length = p - SubVendorName;
                if (Length >= sizeof(bSubVendorName)) Length = sizeof(bSubVendorName) - 1;
                strncpy(bSubVendorName, SubVendorName, Length);
                bSubVendorName[Length] = '\0';
            }
        }
    }

    /* Print out the data */
    DbgPrint("%02x:%02x.%x %s [%02x%02x]: %s %s [%04x:%04x] (rev %02x)\n"
             "\tSubsystem: %s [%04x:%04x]\n",
             i,
             j,
             k,
             bSubClassName,
             PciData->BaseClass,
             PciData->SubClass,
             bVendorName,
             bProductName,
             PciData->VendorID,
             PciData->DeviceID,
             PciData->RevisionID,
             bSubVendorName,
             PciData->u.type0.SubVendorID,
             PciData->u.type0.SubSystemID);

    /* Print out and decode flags */
    DbgPrint("\tFlags:");
    if (PciData->Command & PCI_ENABLE_BUS_MASTER) DbgPrint(" bus master,");
    if (PciData->Status & PCI_STATUS_66MHZ_CAPABLE) DbgPrint(" 66MHz,");
    if ((PciData->Status & PCI_STATUS_DEVSEL) == 0x000) DbgPrint(" fast devsel,");
    if ((PciData->Status & PCI_STATUS_DEVSEL) == 0x200) DbgPrint(" medium devsel,");
    if ((PciData->Status & PCI_STATUS_DEVSEL) == 0x400) DbgPrint(" slow devsel,");
    if ((PciData->Status & PCI_STATUS_DEVSEL) == 0x600) DbgPrint(" unknown devsel,");
    DbgPrint(" latency %d", PciData->LatencyTimer);
    if (PciData->u.type0.InterruptPin != 0 &&
        PciData->u.type0.InterruptLine != 0 &&
        PciData->u.type0.InterruptLine != 0xFF) DbgPrint(", IRQ %02d", PciData->u.type0.InterruptLine);
    else if (PciData->u.type0.InterruptPin != 0) DbgPrint(", IRQ assignment required");
    DbgPrint("\n");

    /* Scan addresses */
    Size = 0;
    for (b = 0; b < PCI_TYPE0_ADDRESSES; b++)
    {
        /* Check for a BAR */
        Mem = PciData->u.type0.BaseAddresses[b];
        if (Mem)
        {
            /* Decode the address type */
            if (Mem & PCI_ADDRESS_IO_SPACE)
            {
                /* Guess the size */
                Size = 1 << 2;
                while (!(Mem & Size) && (Size)) Size <<= 1;

                /* Print it out */
                DbgPrint("\tI/O ports at %04lx", Mem & PCI_ADDRESS_IO_ADDRESS_MASK);
                ShowSize(Size);
            }
            else
            {
                /* Guess the size */
                Size = 1 << 8;
                while (!(Mem & Size) && (Size)) Size <<= 1;

                /* Print it out */
                DbgPrint("\tMemory at %08lx (%d-bit, %sprefetchable)",
                         Mem & PCI_ADDRESS_MEMORY_ADDRESS_MASK,
                         (Mem & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_32BIT ? 32 : 64,
                         (Mem & PCI_ADDRESS_MEMORY_PREFETCHABLE) ? "" : "non-");
                ShowSize(Size);
            }
        }
    }
}
#endif

VOID
NTAPI
INIT_FUNCTION
HalpInitializePciBus(VOID)
{
#ifndef _MINIHAL_
    PPCI_REGISTRY_INFO_INTERNAL PciRegistryInfo;
    UCHAR PciType;
    PCI_SLOT_NUMBER PciSlot;
    ULONG i, j, k;
    UCHAR DataBuffer[PCI_COMMON_HDR_LENGTH];
    PPCI_COMMON_CONFIG PciData = (PPCI_COMMON_CONFIG)DataBuffer;
    PBUS_HANDLER BusHandler;
    ULONG HackFlags;
    BOOLEAN ExtendedAddressDecoding = FALSE;
    NTSTATUS Status;

    /* Query registry information */
    PciRegistryInfo = HalpQueryPciRegistryInfo();
    if (!PciRegistryInfo) return;

    /* Initialize the PCI configuration lock */
    KeInitializeSpinLock(&HalpPCIConfigLock);

    /* Get the type and free the info structure */
    PciType = PciRegistryInfo->HardwareMechanism & 0xF;

    /* Check if this is a type 2 PCI bus with at least one bus */
    if ((PciRegistryInfo->NoBuses) && (PciType == 2))
    {
        /* Setup the PCI slot */
        PciSlot.u.bits.Reserved = 0;
        PciSlot.u.bits.FunctionNumber = 0;

        /* Loop all slots */
        for (i = 0; i < 32; i++)
        {
            /* Try to setup a Type 2 PCI slot */
            PciType = 2;
            BusHandler = HalpAllocateAndInitPciBusHandler(2, 0, TRUE);
            if (!BusHandler) break;

            /* Now check if it's valid */
            if (HalpIsValidPCIDevice(BusHandler, PciSlot)) break;

            /* Heh, the BIOS lied... try Type 1 */
            PciType = 1;
            BusHandler = HalpAllocateAndInitPciBusHandler(1, 0, TRUE);
            if (!BusHandler) break;

            /* Now check if it's valid */
            if (HalpIsValidPCIDevice(BusHandler, PciSlot)) break;

            /* Keep trying */
            PciType = 2;
        }

        /* Now allocate the correct kind of handler */
        HalpAllocateAndInitPciBusHandler(PciType, 0, FALSE);
    }

    /* Okay, now loop all PCI bridges */
    do
    {
        /* Loop all PCI buses */
        for (i = 0; i < PciRegistryInfo->NoBuses; i++)
        {
            /* Check if we have a handler for it */
            if (!HalHandlerForBus(PCIBus, i))
            {
                /* Allocate it */
                HalpAllocateAndInitPciBusHandler(PciType, i, FALSE);
            }
        }
        /* Go to the next bridge */
    } while (HalpGetPciBridgeConfig(PciType, &PciRegistryInfo->NoBuses));

    /* Now build correct address range informaiton */
    HalpFixupPciSupportedRanges(PciRegistryInfo->NoBuses);

    /* Loop every bus */
    DbgPrint("\n====== PCI BUS HARDWARE DETECTION =======\n\n");
    PciSlot.u.bits.Reserved = 0;
    for (i = 0; i < PciRegistryInfo->NoBuses; i++)
    {
        /* Get the bus handler */
        BusHandler = HalHandlerForBus(PCIBus, i);

        /* Loop every device */
        for (j = 0; j < 32; j++)
        {
            /* Loop every function */
            PciSlot.u.bits.DeviceNumber = j;
            for (k = 0; k < 8; k++)
            {
                /* Build the final slot structure */
                PciSlot.u.bits.FunctionNumber = k;

                /* Read the configuration information */
                HalpReadPCIConfig(BusHandler,
                                  PciSlot,
                                  PciData,
                                  0,
                                  PCI_COMMON_HDR_LENGTH);

                /* Skip if this is an invalid function */
                if (PciData->VendorID == PCI_INVALID_VENDORID) continue;

                /* Print out the entry */
                HalpDebugPciDumpBus(i, j, k, PciData);

                /* Check if this is a Cardbus bridge */
                if (PCI_CONFIGURATION_TYPE(PciData) == PCI_CARDBUS_BRIDGE_TYPE)
                {
                    /* Not supported */
                    DbgPrint("\tDevice is a PCI Cardbus Bridge. It will not work!\n");
                    continue;
                }

                /* Check if this is a PCI device */
                if (PCI_CONFIGURATION_TYPE(PciData) != PCI_BRIDGE_TYPE)
                {
                    /* Check if it has an interrupt pin and line registered */
                    if ((PciData->u.type1.InterruptPin) &&
                        (PciData->u.type1.InterruptLine))
                    {
                        /* Check if this interrupt line is connected to the bus */
                        if (PciData->u.type1.InterruptLine < 16)
                        {
                            /* Is this an IDE device? */
                            if (!HalpIsIdeDevice(PciData))
                            {
                                /* We'll mask out this interrupt then */
                                DbgPrint("\tDevice is using IRQ %d! ISA Cards using that IRQ may fail!\n",
                                         PciData->u.type1.InterruptLine);
                                HalpPciIrqMask |= (1 << PciData->u.type1.InterruptLine);
                            }
                        }
                    }
                }

                /* Check for broken Intel chips */
                if (PciData->VendorID == 0x8086)
                {
                    /* Check for broken 82830 PCI controller */
                    if ((PciData->DeviceID == 0x04A3) &&
                        (PciData->RevisionID < 0x11))
                    {
                        /* Skip */
                        DbgPrint("\tDevice is a broken Intel 82430 PCI Controller. It will not work!\n\n");
                        continue;
                    }

                    /* Check for broken 82378 PCI-to-ISA Bridge */
                    if ((PciData->DeviceID == 0x0484) &&
                        (PciData->RevisionID <= 3))
                    {
                        /* Skip */
                        DbgPrint("\tDevice is a broken Intel 82378 PCI-to-ISA Bridge. It will not work!\n\n");
                        continue;
                    }

                    /* Check for broken 82450 PCI Bridge */
                    if ((PciData->DeviceID == 0x84C4) &&
                        (PciData->RevisionID <= 4))
                    {
                        DbgPrint("\tDevice is a Intel Orion 82450 PCI Bridge. It will not work!\n\n");
                        continue;
                    }
                }

                /* Do we know this card? */
                if (!ExtendedAddressDecoding)
                {
                    /* Check for it */
                    if (HalpIsRecognizedCard(PciRegistryInfo,
                                             PciData,
                                             HALP_CARD_FEATURE_FULL_DECODE))
                    {
                        /* We'll do chipset checks later */
                        DbgPrint("\tDevice has Extended Address Decoding. It may fail to work on older BIOSes!\n");
                        ExtendedAddressDecoding = TRUE;
                    }
                }

                /* Check if this is a USB controller */
                if ((PciData->BaseClass == PCI_CLASS_SERIAL_BUS_CTLR) &&
                    (PciData->SubClass == PCI_SUBCLASS_SB_USB))
                {
                    /* Check if this is an OHCI controller */
                    if (PciData->ProgIf == 0x10)
                    {
                        DbgPrint("\tDevice is an OHCI (USB) PCI Expansion Card. Turn off Legacy USB in your BIOS!\n\n");
                        continue;
                    }

                    /* Check for Intel UHCI controller */
                    if (PciData->VendorID == 0x8086)
                    {
                        DbgPrint("\tDevice is an Intel UHCI (USB) Controller. Turn off Legacy USB in your BIOS!\n\n");
                        continue;
                    }

                    /* Check for VIA UHCI controller */
                    if (PciData->VendorID == 0x1106)
                    {
                        DbgPrint("\tDevice is a VIA UHCI (USB) Controller. Turn off Legacy USB in your BIOS!\n\n");
                        continue;
                    }
                }

                /* Now check the registry for chipset hacks */
                Status = HalpGetChipHacks(PciData->VendorID,
                                          PciData->DeviceID,
                                          PciData->RevisionID,
                                          &HackFlags);
                if (NT_SUCCESS(Status))
                {
                    /* Check for broken ACPI routing */
                    if (HackFlags & HAL_PCI_CHIP_HACK_DISABLE_ACPI_IRQ_ROUTING)
                    {
                        DbgPrint("This chipset has broken ACPI IRQ Routing! Be aware!\n\n");
                        continue;
                    }

                    /* Check for broken ACPI timer */
                    if (HackFlags & HAL_PCI_CHIP_HACK_BROKEN_ACPI_TIMER)
                    {
                         DbgPrint("This chipset has a broken ACPI timer! Be aware!\n\n");
                         continue;
                    }

                    /* Check for hibernate-disable */
                    if (HackFlags & HAL_PCI_CHIP_HACK_DISABLE_HIBERNATE)
                    {
                        DbgPrint("This chipset has a broken PCI device which is incompatible with hibernation. Be aware!\n\n");
                        continue;
                    }

                    /* Check for USB controllers that generate SMIs */
                    if (HackFlags & HAL_PCI_CHIP_HACK_USB_SMI_DISABLE)
                    {
                        DbgPrint("This chipset has a USB controller which generates SMIs. ReactOS will likely fail to boot!\n\n");
                        continue;
                    }
                }

                /* Terminate the entry */
                DbgPrint("\n");
            }
        }
    }

    /* Initialize NMI Crash Flag */
    HalpGetNMICrashFlag();

    /* Free the registry data */
    ExFreePoolWithTag(PciRegistryInfo, TAG_HAL);

    /* Tell PnP if this hard supports correct decoding */
    HalpMarkChipsetDecode(ExtendedAddressDecoding);
    DbgPrint("====== PCI BUS DETECTION COMPLETE =======\n\n");
#endif
}

VOID
NTAPI
INIT_FUNCTION
HalpInitBusHandlers(VOID)
{
    /* Register the HAL Bus Handler support */
    HalpRegisterInternalBusHandlers();
}

VOID
NTAPI
INIT_FUNCTION
HalpRegisterKdSupportFunctions(VOID)
{
    /* Register PCI Device Functions */
    KdSetupPciDeviceForDebugging = HalpSetupPciDeviceForDebugging;
    KdReleasePciDeviceforDebugging = HalpReleasePciDeviceForDebugging;

    /* Register memory functions */
#ifndef _MINIHAL_
    KdMapPhysicalMemory64 = HalpMapPhysicalMemory64;
    KdUnmapVirtualAddress = HalpUnmapVirtualAddress;
#endif

    /* Register ACPI stub */
    KdCheckPowerButton = HalpCheckPowerButton;
}

NTSTATUS
NTAPI
HalpAssignSlotResources(IN PUNICODE_STRING RegistryPath,
                        IN PUNICODE_STRING DriverClassName,
                        IN PDRIVER_OBJECT DriverObject,
                        IN PDEVICE_OBJECT DeviceObject,
                        IN INTERFACE_TYPE BusType,
                        IN ULONG BusNumber,
                        IN ULONG SlotNumber,
                        IN OUT PCM_RESOURCE_LIST *AllocatedResources)
{
    PBUS_HANDLER Handler;
    NTSTATUS Status;
    PAGED_CODE();
    DPRINT1("Slot assignment for %d on bus %u\n", BusType, BusNumber);

    /* Find the handler */
    Handler = HalReferenceHandlerForBus(BusType, BusNumber);
    if (!Handler) return STATUS_NOT_FOUND;

    /* Do the assignment */
    Status = Handler->AssignSlotResources(Handler,
                                          Handler,
                                          RegistryPath,
                                          DriverClassName,
                                          DriverObject,
                                          DeviceObject,
                                          SlotNumber,
                                          AllocatedResources);

    /* Dereference the handler and return */
    HalDereferenceBusHandler(Handler);
    return Status;
}

BOOLEAN
NTAPI
HaliFindBusAddressTranslation(IN PHYSICAL_ADDRESS BusAddress,
                              IN OUT PULONG AddressSpace,
                              OUT PPHYSICAL_ADDRESS TranslatedAddress,
                              IN OUT PULONG_PTR Context,
                              IN BOOLEAN NextBus)
{
    PHAL_BUS_HANDLER BusHandler;
    PBUS_HANDLER Handler;
    PLIST_ENTRY NextEntry;
    ULONG ContextValue;

    /* Make sure we have a context */
    if (!Context) return FALSE;
    ASSERT((*Context) || (NextBus == TRUE));

    /* Read the context */
    ContextValue = *Context;

    /* Find the bus handler */
    Handler = HalpContextToBusHandler(ContextValue);
    if (!Handler) return FALSE;

    /* Check if this is an ongoing lookup */
    if (NextBus)
    {
        /* Get the HAL bus handler */
        BusHandler = CONTAINING_RECORD(Handler, HAL_BUS_HANDLER, Handler);
        NextEntry = &BusHandler->AllHandlers;

        /* Get the next one if we were already with one */
        if (ContextValue) NextEntry = NextEntry->Flink;

        /* Start scanning */
        while (TRUE)
        {
            /* Check if this is the last one */
            if (NextEntry == &HalpAllBusHandlers)
            {
                /* Quit */
                *Context = 1;
                return FALSE;
            }

            /* Call this translator */
            BusHandler = CONTAINING_RECORD(NextEntry, HAL_BUS_HANDLER, AllHandlers);
            if (HalTranslateBusAddress(BusHandler->Handler.InterfaceType,
                                       BusHandler->Handler.BusNumber,
                                       BusAddress,
                                       AddressSpace,
                                       TranslatedAddress)) break;

            /* Try the next one */
            NextEntry = NextEntry->Flink;
        }

        /* If we made it, we're done */
        *Context = (ULONG_PTR)Handler;
        return TRUE;
    }

    /* Try the first one through */
    if (!HalTranslateBusAddress(Handler->InterfaceType,
                                Handler->BusNumber,
                                BusAddress,
                                AddressSpace,
                                TranslatedAddress)) return FALSE;

    /* Remember for next time */
    *Context = (ULONG_PTR)Handler;
    return TRUE;
}

BOOLEAN
NTAPI
HaliTranslateBusAddress(IN INTERFACE_TYPE InterfaceType,
                        IN ULONG BusNumber,
                        IN PHYSICAL_ADDRESS BusAddress,
                        IN OUT PULONG AddressSpace,
                        OUT PPHYSICAL_ADDRESS TranslatedAddress)
{
    PBUS_HANDLER Handler;
    BOOLEAN Status;

    /* Find the handler */
    Handler = HalReferenceHandlerForBus(InterfaceType, BusNumber);
    if (!(Handler) || !(Handler->TranslateBusAddress))
    {
        DPRINT1("No translator Interface: %x, Bus: %x, Handler: %p, BusAddress: %x!\n", InterfaceType, BusNumber, Handler, BusAddress);
        return FALSE;
    }

    /* Do the assignment */
    Status = Handler->TranslateBusAddress(Handler,
                                          Handler,
                                          BusAddress,
                                          AddressSpace,
                                          TranslatedAddress);

    /* Dereference the handler and return */
    HalDereferenceBusHandler(Handler);
    return Status;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
HalAdjustResourceList(IN PIO_RESOURCE_REQUIREMENTS_LIST *ResourceList)
{
    PBUS_HANDLER Handler;
    ULONG Status;
    PAGED_CODE();

    /* Find the handler */
    Handler = HalReferenceHandlerForBus((*ResourceList)->InterfaceType,
                                        (*ResourceList)->BusNumber);
    if (!Handler) return STATUS_SUCCESS;

    /* Do the assignment */
    Status = Handler->AdjustResourceList(Handler,
                                         Handler,
                                         ResourceList);

    /* Dereference the handler and return */
    HalDereferenceBusHandler(Handler);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
HalAssignSlotResources(IN PUNICODE_STRING RegistryPath,
                       IN PUNICODE_STRING DriverClassName,
                       IN PDRIVER_OBJECT DriverObject,
                       IN PDEVICE_OBJECT DeviceObject,
                       IN INTERFACE_TYPE BusType,
                       IN ULONG BusNumber,
                       IN ULONG SlotNumber,
                       IN OUT PCM_RESOURCE_LIST *AllocatedResources)
{
    PAGED_CODE();

    /* Check the bus type */
    if (BusType != PCIBus)
    {
        /* Call our internal handler */
        return HalpAssignSlotResources(RegistryPath,
                                       DriverClassName,
                                       DriverObject,
                                       DeviceObject,
                                       BusType,
                                       BusNumber,
                                       SlotNumber,
                                       AllocatedResources);
    }
    else
    {
        /* Call the PCI registered function */
        return HalPciAssignSlotResources(RegistryPath,
                                         DriverClassName,
                                         DriverObject,
                                         DeviceObject,
                                         PCIBus,
                                         BusNumber,
                                         SlotNumber,
                                         AllocatedResources);
    }
}

/*
 * @implemented
 */
ULONG
NTAPI
HalGetBusData(IN BUS_DATA_TYPE BusDataType,
              IN ULONG BusNumber,
              IN ULONG SlotNumber,
              IN PVOID Buffer,
              IN ULONG Length)
{
    /* Call the extended function */
    return HalGetBusDataByOffset(BusDataType,
                                 BusNumber,
                                 SlotNumber,
                                 Buffer,
                                 0,
                                 Length);
}

/*
 * @implemented
 */
ULONG
NTAPI
HalGetBusDataByOffset(IN BUS_DATA_TYPE BusDataType,
                      IN ULONG BusNumber,
                      IN ULONG SlotNumber,
                      IN PVOID Buffer,
                      IN ULONG Offset,
                      IN ULONG Length)
{
    PBUS_HANDLER Handler;
    ULONG Status;

    /* Find the handler */
    Handler = HaliReferenceHandlerForConfigSpace(BusDataType, BusNumber);
    if (!Handler) return 0;

    /* Do the assignment */
    Status = Handler->GetBusData(Handler,
                                 Handler,
                                 SlotNumber,
                                 Buffer,
                                 Offset,
                                 Length);

    /* Dereference the handler and return */
    HalDereferenceBusHandler(Handler);
    return Status;
}

/*
 * @implemented
 */
ULONG
NTAPI
HalGetInterruptVector(IN INTERFACE_TYPE InterfaceType,
                      IN ULONG BusNumber,
                      IN ULONG BusInterruptLevel,
                      IN ULONG BusInterruptVector,
                      OUT PKIRQL Irql,
                      OUT PKAFFINITY Affinity)
{
    PBUS_HANDLER Handler;
    ULONG Vector;
    PAGED_CODE();

    /* Defaults */
    *Irql = 0;
    *Affinity = 0;

    /* Find the handler */
    Handler = HalReferenceHandlerForBus(InterfaceType, BusNumber);
    if (!Handler) return 0;

    /* Do the assignment */
    Vector = Handler->GetInterruptVector(Handler,
                                         Handler,
                                         BusInterruptLevel,
                                         BusInterruptVector,
                                         Irql,
                                         Affinity);
    if ((Vector != IRQ2VECTOR(BusInterruptLevel)) ||
        (*Irql != VECTOR2IRQL(IRQ2VECTOR(BusInterruptLevel))))
    {
        DPRINT1("Returning IRQL %lx, Vector %lx for Level/Vector: %lx/%lx\n",
                *Irql, Vector, BusInterruptLevel, BusInterruptVector);
        DPRINT1("Old HAL would've returned IRQL %lx and Vector %lx\n",
                VECTOR2IRQL(IRQ2VECTOR(BusInterruptLevel)),
                IRQ2VECTOR(BusInterruptLevel));
    }

    /* Dereference the handler and return */
    HalDereferenceBusHandler(Handler);
    return Vector;
}

/*
 * @implemented
 */
ULONG
NTAPI
HalSetBusData(IN BUS_DATA_TYPE BusDataType,
              IN ULONG BusNumber,
              IN ULONG SlotNumber,
              IN PVOID Buffer,
              IN ULONG Length)
{
    /* Call the extended function */
    return HalSetBusDataByOffset(BusDataType,
                                 BusNumber,
                                 SlotNumber,
                                 Buffer,
                                 0,
                                 Length);
}

/*
 * @implemented
 */
ULONG
NTAPI
HalSetBusDataByOffset(IN BUS_DATA_TYPE BusDataType,
                      IN ULONG BusNumber,
                      IN ULONG SlotNumber,
                      IN PVOID Buffer,
                      IN ULONG Offset,
                      IN ULONG Length)
{
    PBUS_HANDLER Handler;
    ULONG Status;

    /* Find the handler */
    Handler = HaliReferenceHandlerForConfigSpace(BusDataType, BusNumber);
    if (!Handler) return 0;

    /* Do the assignment */
    Status = Handler->SetBusData(Handler,
                                 Handler,
                                 SlotNumber,
                                 Buffer,
                                 Offset,
                                 Length);

    /* Dereference the handler and return */
    HalDereferenceBusHandler(Handler);
    return Status;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
HalTranslateBusAddress(IN INTERFACE_TYPE InterfaceType,
                       IN ULONG BusNumber,
                       IN PHYSICAL_ADDRESS BusAddress,
                       IN OUT PULONG AddressSpace,
                       OUT PPHYSICAL_ADDRESS TranslatedAddress)
{
    /* Look as the bus type */
    if (InterfaceType == PCIBus)
    {
        /* Call the PCI registered function */
        return HalPciTranslateBusAddress(PCIBus,
                                         BusNumber,
                                         BusAddress,
                                         AddressSpace,
                                         TranslatedAddress);
    }
    else
    {
        /* Call the bus handler */
        return HaliTranslateBusAddress(InterfaceType,
                                       BusNumber,
                                       BusAddress,
                                       AddressSpace,
                                       TranslatedAddress);
    }
}

/* EOF */
