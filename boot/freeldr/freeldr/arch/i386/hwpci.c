/*
 *  FreeLoader
 *
 *  Copyright (C) 2004  Eric Kohl
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <freeldr.h>
#include "../../ntldr/ntldropts.h"

#include <debug.h>
DBG_DEFAULT_CHANNEL(HWDETECT);

FIND_PCI_BIOS FindPciBios = NULL;

static
PPCI_IRQ_ROUTING_TABLE
GetPciIrqRoutingTable(VOID)
{
    PPCI_IRQ_ROUTING_TABLE Table;
    PUCHAR Ptr;
    ULONG Sum;
    ULONG i;

    Table = (PPCI_IRQ_ROUTING_TABLE)0xF0000;
    while ((ULONG_PTR)Table < 0x100000)
    {
        if (Table->Signature == 'RIP$')
        {
            TRACE("Found signature\n");

            if (Table->TableSize < FIELD_OFFSET(PCI_IRQ_ROUTING_TABLE, Slot) ||
                Table->TableSize % 16 != 0)
            {
                ERR("Invalid routing table size (%u) at 0x%p. Continue searching...\n", Table->TableSize, Table);
                Table = (PPCI_IRQ_ROUTING_TABLE)((ULONG_PTR)Table + 0x10);
                continue;
            }

            Ptr = (PUCHAR)Table;
            Sum = 0;
            for (i = 0; i < Table->TableSize; i++)
            {
                Sum += Ptr[i];
            }

            if ((Sum & 0xFF) != 0)
            {
                ERR("Invalid routing table checksum (%#lx) at 0x%p. Continue searching...\n", Sum & 0xFF, Table);
            }
            else
            {
                TRACE("Valid checksum (%#lx): found routing table at 0x%p\n", Sum & 0xFF, Table);
                return Table;
            }
        }

        Table = (PPCI_IRQ_ROUTING_TABLE)((ULONG_PTR)Table + 0x10);
    }

    ERR("No valid routing table found!\n");

    return NULL;
}

BOOLEAN
PcFindPciBios(
    _Out_ PPCI_REGISTRY_INFO BusData)
{
    REGS RegsIn;
    REGS RegsOut;

    RegsIn.b.ah = 0xB1; /* Subfunction B1h */
    RegsIn.b.al = 0x01; /* PCI BIOS present */

    Int386(0x1A, &RegsIn, &RegsOut);

    if (INT386_SUCCESS(RegsOut) &&
        (RegsOut.d.edx == ' ICP') &&
        (RegsOut.b.ah == 0))
    {
        TRACE("Found PCI bios\n");

        TRACE("AL: %x\n", RegsOut.b.al);
        TRACE("BH: %x\n", RegsOut.b.bh);
        TRACE("BL: %x\n", RegsOut.b.bl);
        TRACE("CL: %x\n", RegsOut.b.cl);

        BusData->NoBuses = RegsOut.b.cl + 1;
        BusData->MajorRevision = RegsOut.b.bh;
        BusData->MinorRevision = RegsOut.b.bl;
        BusData->HardwareMechanism = RegsOut.b.al;

        return TRUE;
    }

    TRACE("No PCI bios found\n");

    return FALSE;
}

static
PCONFIGURATION_COMPONENT_DATA
DetectPciIrqRoutingTable(
    _In_ PCONFIGURATION_COMPONENT_DATA BusKey)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PPCI_IRQ_ROUTING_TABLE Table;
    PCONFIGURATION_COMPONENT_DATA TableKey;
    ULONG Size;

    Table = GetPciIrqRoutingTable();
    if (!Table)
        return NULL;

    TRACE("Table size: %u\n", Table->TableSize);

    /* Set 'Configuration Data' value */
    Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors) +
           2 * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) + Table->TableSize;
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate resource descriptor\n");
        return NULL;
    }

    /* Initialize resource descriptor */
    RtlZeroMemory(PartialResourceList, Size);
    PartialResourceList->Version = 1;
    PartialResourceList->Revision = 1;
    PartialResourceList->Count = 2;

    PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
    PartialDescriptor->Type = CmResourceTypeBusNumber;
    PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    PartialDescriptor->u.BusNumber.Start = 0;
    PartialDescriptor->u.BusNumber.Length = 1;

    PartialDescriptor = &PartialResourceList->PartialDescriptors[1];
    PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
    PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
    PartialDescriptor->u.DeviceSpecificData.DataSize = Table->TableSize;

    RtlCopyMemory(&PartialResourceList->PartialDescriptors[2],
                  Table,
                  Table->TableSize);

    FldrCreateComponentKey(BusKey,
                           PeripheralClass,
                           RealModeIrqRoutingTable,
                           0,
                           0,
                           0xFFFFFFFF,
                           "PCI Real-mode IRQ Routing Table",
                           PartialResourceList,
                           Size,
                           &TableKey);

    return TableKey;
}


#include <pshpack1.h>

typedef struct _PCI_REGISTRY_DEVICE
{
    union
    {
        struct
        {
            USHORT FunctionNumber : 3;
            USHORT DeviceNumber   : 5;
            USHORT BusNumber      : 8;
        } bits;
        USHORT AsUSHORT;
    } Address;

    PCI_COMMON_CONFIG PciConfig;
} PCI_REGISTRY_DEVICE, *PPCI_REGISTRY_DEVICE;

#include <poppack.h>

/**
 * @brief
 * Retrieves the next PCI device on the specified bus,
 * and obtain its standard configuration information.
 *
 * @param[in]   BusNumber
 * The PCI bus number where to enumerate PCI devices.
 *
 * @param[in,out]   NextSlotNumber
 * In input, specifies the slot where to restart the enumeration.
 * In output, receives the next slot where enumeration can be restarted.
 *
 * @param[out]  PciConfig
 * Optional pointer to a PCI_COMMON_CONFIG buffer receiving
 * the PCI device configuration data.
 *
 * @param[out]  ConfigSize
 * Optional pointer to a ULONG that receives the actual
 * length of the retrieved configuration data.
 *
 * @return
 * The next PCI device slot number on the given bus, or -1 if none.
 *
 * @note    See SpiGetPciConfigData().
 **/
ULONG
GetNextPciDevice(
    _In_ ULONG BusNumber,
    _Inout_ PPCI_SLOT_NUMBER NextSlotNumber,
    _Out_opt_ PPCI_COMMON_CONFIG PciConfig,
    _Out_opt_ PULONG ConfigSize)
{
    ULONG DeviceNumber;
    ULONG FunctionNumber;
    PCI_COMMON_CONFIG PciData;
    ULONG DataSize;

    /* Loop through all devices */
    for (DeviceNumber = NextSlotNumber->u.bits.DeviceNumber;
         DeviceNumber < PCI_MAX_DEVICES;
         DeviceNumber++)
    {
        /* Loop through all functions */
        for (FunctionNumber = NextSlotNumber->u.bits.FunctionNumber;
             FunctionNumber < PCI_MAX_FUNCTION;
             FunctionNumber++)
        {
            PCI_SLOT_NUMBER SlotNumber = {0};
            SlotNumber.u.bits.DeviceNumber   = DeviceNumber;
            SlotNumber.u.bits.FunctionNumber = FunctionNumber;

            /* Retrieve PCI configuration data */
            RtlZeroMemory(&PciData, sizeof(PciData));
            DataSize = (PciConfig ? sizeof(PciData) : PCI_COMMON_HDR_LENGTH);
            DataSize = HalGetBusDataByOffset(PCIConfiguration,
                                             BusNumber,
                                             SlotNumber.u.AsULONG,
                                             &PciData,
                                             0,
                                             DataSize);

            /* If the returned size is 0, then the bus is wrong */
            if (DataSize == 0)
            {
                ERR("HalGetBusDataByOffset(%02x:%02x.%x) failed\n",
                    BusNumber, DeviceNumber, FunctionNumber);
                return -1;
            }

            /* If the result is PCI_INVALID_VENDORID, then this
             * device has no more "Functions" */
            if (PciData.VendorID == PCI_INVALID_VENDORID)
                break;

#if 0
            /* Print out the data */
            DbgPrint("%02x:%02x.%x [%02x%02x]: [%04x:%04x] (rev %02x)\n",
                     BusNumber, DeviceNumber, FunctionNumber,
                     PciData.BaseClass,
                     PciData.SubClass,
                     PciData.VendorID,
                     PciData.DeviceID,
                     PciData.RevisionID);

            if ((PciData.HeaderType & ~PCI_MULTIFUNCTION) == PCI_DEVICE_TYPE)
            {
                DbgPrint("\tSubsystem: [%04x:%04x]\n",
                         PciData.u.type0.SubVendorID,
                         PciData.u.type0.SubSystemID);
            }
#endif

            /* Setup the device and function numbers for the next run */
            NextSlotNumber->u.bits.DeviceNumber   = DeviceNumber;
            NextSlotNumber->u.bits.FunctionNumber = FunctionNumber + 1;

            /* Return the PCI configuration data to the caller */
            if (PciConfig)
                RtlCopyMemory(PciConfig, &PciData, DataSize);
            if (ConfigSize)
                *ConfigSize = DataSize;

            /* And return the slot number of this valid device/function */
            SlotNumber.u.bits.Reserved = 1; // Set it as valid.
            return SlotNumber.u.AsULONG;
        }

        /* Go to the next device and reset the function number */
        NextSlotNumber->u.bits.FunctionNumber = 0;
    }

    /* We are done enumerating */
    NextSlotNumber->u.bits.DeviceNumber = 0;
    return -1;
}


VOID
DetectPciBios(
    _In_opt_ PCSTR Options,
    _In_ PCONFIGURATION_COMPONENT_DATA SystemKey,
    _Out_ PULONG BusNumber)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PCONFIGURATION_COMPONENT_DATA BiosKey;
    PCONFIGURATION_COMPONENT_DATA BusKey;
    PCI_REGISTRY_INFO BusData;
    ULONG Size;
    ULONG i;
    BOOLEAN PciEnum;

    /* Report the PCI BIOS */
    if (!FindPciBios(&BusData))
        return;

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

    /* Create new bus key */
    FldrCreateComponentKey(SystemKey,
                           AdapterClass,
                           MultiFunctionAdapter,
                           0,
                           0,
                           0xFFFFFFFF,
                           "PCI BIOS",
                           PartialResourceList,
                           Size,
                           &BiosKey);

    /* Increment bus number */
    (*BusNumber)++;

    DetectPciIrqRoutingTable(BiosKey);

    /* Check whether to enumerate PCI devices during buses enumeration */
    PciEnum = !!NtLdrGetOption(Options, "PCIENUM");

    /* Report PCI buses */
    for (i = 0; i < (ULONG)BusData.NoBuses; i++)
    {
        /* Check if this is the first bus */
        if (i == 0)
        {
            /* Set 'Configuration Data' value */
            Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors) +
                   sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) +
                   sizeof(PCI_REGISTRY_INFO);
            PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
            if (!PartialResourceList)
            {
                ERR("Failed to allocate resource descriptor! Ignoring remaining PCI buses. (i = %lu, NoBuses = %lu)\n",
                    i, (ULONG)BusData.NoBuses);
                return;
            }

            /* Initialize resource descriptor */
            RtlZeroMemory(PartialResourceList, Size);
            PartialResourceList->Version = 1;
            PartialResourceList->Revision = 1;
            PartialResourceList->Count = 1;
            PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
            PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
            PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
            PartialDescriptor->u.DeviceSpecificData.DataSize = sizeof(PCI_REGISTRY_INFO);
            RtlCopyMemory(&PartialResourceList->PartialDescriptors[1],
                          &BusData,
                          sizeof(PCI_REGISTRY_INFO));
        }
        else
        {
            /* Set 'Configuration Data' value */
            Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors);
            PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
            if (!PartialResourceList)
            {
                ERR("Failed to allocate resource descriptor! Ignoring remaining PCI buses. (i = %lu, NoBuses = %lu)\n",
                    i, (ULONG)BusData.NoBuses);
                return;
            }

            /* Initialize resource descriptor */
            RtlZeroMemory(PartialResourceList, Size);
        }

        /* Create the bus key */
        FldrCreateComponentKey(SystemKey,
                               AdapterClass,
                               MultiFunctionAdapter,
                               0,
                               0,
                               0xFFFFFFFF,
                               "PCI",
                               PartialResourceList,
                               Size,
                               &BusKey);

        /* Increment bus number */
        (*BusNumber)++;

        if (PciEnum)
        {
            ULONG PciDevicesNumber, Device;
            ULONG DataSize;
            PPCI_REGISTRY_DEVICE PciDevice;
            PCONFIGURATION_COMPONENT_DATA DataKey;

            PCI_SLOT_NUMBER SlotNumber, NextSlotNumber;

            /* Count all the PCI devices on this bus */
            PciDevicesNumber = 0;
            NextSlotNumber.u.AsULONG = 0;
            while (GetNextPciDevice(i, &NextSlotNumber,
                                    NULL, NULL) != -1)
            {
                ++PciDevicesNumber;
            }
            DataSize = (PciDevicesNumber * sizeof(PCI_REGISTRY_DEVICE));

            /* Set 'Configuration Data' value */
            Size = sizeof(CM_PARTIAL_RESOURCE_LIST) + DataSize;
            PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
            if (!PartialResourceList)
            {
                /* Ignore since this is optional */
                continue;
            }

            /* Initialize resource descriptor */
            RtlZeroMemory(PartialResourceList, Size);
            PartialResourceList->Version = 1;
            PartialResourceList->Revision = 1;
            PartialResourceList->Count = 1;

            PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
            PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
            PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
            PartialDescriptor->u.DeviceSpecificData.DataSize = DataSize;

            /* Get pointer to PCI device data */
            // &PartialResourceList->PartialDescriptors[1]
            PciDevice = (PVOID)(((ULONG_PTR)PartialResourceList) + sizeof(CM_PARTIAL_RESOURCE_LIST));

            /* Retrieve all the devices data on this PCI bus */
            Device = 0;
            NextSlotNumber.u.AsULONG = 0;
            while ((Device < PciDevicesNumber) &&
                   (SlotNumber.u.AsULONG = GetNextPciDevice(i, &NextSlotNumber,
                                                            &PciDevice->PciConfig, NULL)) != -1)
            {
                PciDevice->Address.bits.BusNumber = i;
                PciDevice->Address.bits.DeviceNumber   = SlotNumber.u.bits.DeviceNumber;
                PciDevice->Address.bits.FunctionNumber = SlotNumber.u.bits.FunctionNumber;
                ++PciDevice;
                ++Device;
            }

            /* Create the PCI devices configuration enumeration key */
            FldrCreateComponentKey(BusKey,
                                   PeripheralClass,
                                   RealModePCIEnumeration,
                                   0,
                                   0,
                                   0xFFFFFFFF,
                                   "PCI Devices",
                                   PartialResourceList,
                                   Size,
                                   &DataKey);
        }
    }
}

/* EOF */
