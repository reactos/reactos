/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/debug.c
 * PURPOSE:         Debug Helpers
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PCHAR PnpCodes[] =
{
    "START_DEVICE",
    "QUERY_REMOVE_DEVICE",
    "REMOVE_DEVICE",
    "CANCEL_REMOVE_DEVICE",
    "STOP_DEVICE",
    "QUERY_STOP_DEVICE",
    "CANCEL_STOP_DEVICE",
    "QUERY_DEVICE_RELATIONS",
    "QUERY_INTERFACE",
    "QUERY_CAPABILITIES",
    "QUERY_RESOURCES",
    "QUERY_RESOURCE_REQUIREMENTS",
    "QUERY_DEVICE_TEXT",
    "FILTER_RESOURCE_REQUIREMENTS",
    "** UNKNOWN PNP IRP Minor Code **",
    "READ_CONFIG",
    "WRITE_CONFIG",
    "EJECT",
    "SET_LOCK",
    "QUERY_ID",
    "QUERY_PNP_DEVICE_STATE",
    "QUERY_BUS_INFORMATION",
    "DEVICE_USAGE_NOTIFICATION"
};

PCHAR PoCodes[] =
{
    "WAIT_WAKE",
    "POWER_SEQUENCE",
    "SET_POWER",
    "QUERY_POWER",
};

PCHAR SystemPowerStates[] =
{
    "Unspecified",
    "Working",
    "Sleeping1",
    "Sleeping2",
    "Sleeping3",
    "Hibernate",
    "Shutdown"
};

PCHAR DevicePowerStates[] =
{
    "Unspecified",
    "D0",
    "D1",
    "D2",
    "D3"
};

ULONG PciBreakOnPdoPowerIrp, PciBreakOnFdoPowerIrp;
ULONG PciBreakOnPdoPnpIrp, PciBreakOnFdoPnpIrp;

/* FUNCTIONS ******************************************************************/

PCHAR
NTAPI
PciDebugPnpIrpTypeToText(IN USHORT MinorFunction)
{
    PCHAR Text;

    /* Catch invalid code */
    if (MinorFunction >= IRP_MN_SURPRISE_REMOVAL)
    {
        /* New version of Windows? Or driver bug */
        Text = "** UNKNOWN PNP IRP Minor Code **";
    }
    else
    {
        /* Get the right text for it */
        Text = PnpCodes[MinorFunction];
    }

    /* Return the symbolic name for the IRP */
    return Text;
}

PCHAR
NTAPI
PciDebugPoIrpTypeToText(IN USHORT MinorFunction)
{
    PCHAR Text;

    /* Catch invalid code */
    if (MinorFunction >= IRP_MN_QUERY_POWER)
    {
        /* New version of Windows? Or driver bug */
        Text = "** UNKNOWN PO IRP Minor Code **";
    }
    else
    {
        /* Get the right text for it */
        Text = PoCodes[MinorFunction];
    }

    /* Return the symbolic name for the IRP */
    return Text;
}

BOOLEAN
NTAPI
PciDebugIrpDispatchDisplay(IN PIO_STACK_LOCATION IoStackLocation,
                           IN PPCI_FDO_EXTENSION DeviceExtension,
                           IN USHORT MaxMinor)
{
    PPCI_PDO_EXTENSION PdoDeviceExtension;
    ULONG BreakMask;
    //ULONG DebugLevel = 0;
    PCHAR IrpString;

    /* Only two functions are recognized */
    switch (IoStackLocation->MajorFunction)
    {
        case IRP_MJ_POWER:

            /* Get the string and the correct break mask for the extension */
            BreakMask = (DeviceExtension->ExtensionType == PciPdoExtensionType) ?
                         PciBreakOnPdoPowerIrp : PciBreakOnFdoPowerIrp;
            IrpString = PciDebugPoIrpTypeToText(IoStackLocation->MinorFunction);
            break;

        case IRP_MJ_PNP:

            /* Get the string and the correct break mask for the extension */
            BreakMask = (DeviceExtension->ExtensionType == PciFdoExtensionType) ?
                         PciBreakOnPdoPnpIrp : PciBreakOnFdoPnpIrp;
            IrpString = PciDebugPnpIrpTypeToText(IoStackLocation->MinorFunction);
            break;

        default:

            /* Other functions are not decoded */
            BreakMask = FALSE;
            IrpString = "";
            break;
    }

    /* Check if this is a PDO */
    if (DeviceExtension->ExtensionType == PciPdoExtensionType)
    {
        /* Choose the correct debug level based on which function this is */
        if (IoStackLocation->MajorFunction == IRP_MJ_POWER)
        {
            //DebugLevel = 0x500;
        }
        else if (IoStackLocation->MajorFunction == IRP_MJ_PNP)
        {
            //DebugLevel = 0x200;
        }

        /* For a PDO, print out the bus, device, and function number */
        PdoDeviceExtension = (PVOID)DeviceExtension;
        DPRINT1("PDO(b=0x%x, d=0x%x, f=0x%x)<-%s\n",
                PdoDeviceExtension->ParentFdoExtension->BaseBus,
                PdoDeviceExtension->Slot.u.bits.DeviceNumber,
                PdoDeviceExtension->Slot.u.bits.FunctionNumber,
                IrpString);
    }
    else if (DeviceExtension->ExtensionType == PciFdoExtensionType)
    {
        /* Choose the correct debug level based on which function this is */
        if (IoStackLocation->MajorFunction == IRP_MJ_POWER)
        {
            //DebugLevel = 0x400;
        }
        else if (IoStackLocation->MajorFunction == IRP_MJ_PNP)
        {
            //DebugLevel = 0x100;
        }

        /* For an FDO, just dump the extension pointer and IRP string */
        DPRINT1("FDO(%p)<-%s\n", DeviceExtension, IrpString);
    }

    /* If the function is illegal for this extension, complain */
    if (IoStackLocation->MinorFunction > MaxMinor)
        DPRINT1("Unknown IRP, minor = 0x%x\n", IoStackLocation->MinorFunction);

    /* Return whether or not the debugger should be broken into for this IRP */
    return ((1 << IoStackLocation->MinorFunction) & BreakMask);
}

VOID
NTAPI
PciDebugDumpCommonConfig(IN PPCI_COMMON_HEADER PciData)
{
    USHORT i;

    /* Loop the PCI header */
    for (i = 0; i < PCI_COMMON_HDR_LENGTH; i += 4)
    {
        /* Dump each DWORD and its offset */
        DPRINT1("  %02x - %08x\n", i, *(PULONG)((ULONG_PTR)PciData + i));
    }
}

VOID
NTAPI
PciDebugDumpQueryCapabilities(IN PDEVICE_CAPABILITIES DeviceCaps)
{
    ULONG i;

    /* Dump the capabilities */
    DPRINT1("Capabilities\n  Lock:%u, Eject:%u, Remove:%u, Dock:%u, UniqueId:%u\n",
            DeviceCaps->LockSupported,
            DeviceCaps->EjectSupported,
            DeviceCaps->Removable,
            DeviceCaps->DockDevice,
            DeviceCaps->UniqueID);
    DbgPrint("  SilentInstall:%u, RawOk:%u, SurpriseOk:%u\n",
             DeviceCaps->SilentInstall,
             DeviceCaps->RawDeviceOK,
             DeviceCaps->SurpriseRemovalOK);
    DbgPrint("  Address %08x, UINumber %08x, Latencies D1 %u, D2 %u, D3 %u\n",
             DeviceCaps->Address,
             DeviceCaps->UINumber,
             DeviceCaps->D1Latency,
             DeviceCaps->D2Latency,
             DeviceCaps->D3Latency);

    /* Dump and convert the wake levels */
    DbgPrint("  System Wake: %s, Device Wake: %s\n  DeviceState[PowerState] [",
             SystemPowerStates[min(DeviceCaps->SystemWake, PowerSystemMaximum)],
             DevicePowerStates[min(DeviceCaps->DeviceWake, PowerDeviceMaximum)]);

    /* Dump and convert the power state mappings */
    for (i = PowerSystemWorking; i < PowerSystemMaximum; i++)
        DbgPrint(" %s", DevicePowerStates[DeviceCaps->DeviceState[i]]);

    /* Finish the dump */
    DbgPrint(" ]\n");
}

PCHAR
NTAPI
PciDebugCmResourceTypeToText(IN UCHAR Type)
{
    /* What kind of resource it this? */
    switch (Type)
    {
        /* Pick the correct identifier string based on the type */
        case CmResourceTypeDeviceSpecific: return "CmResourceTypeDeviceSpecific";
        case CmResourceTypePort: return "CmResourceTypePort";
        case CmResourceTypeInterrupt: return "CmResourceTypeInterrupt";
        case CmResourceTypeMemory: return "CmResourceTypeMemory";
        case CmResourceTypeDma: return "CmResourceTypeDma";
        case CmResourceTypeBusNumber: return "CmResourceTypeBusNumber";
        case CmResourceTypeConfigData: return "CmResourceTypeConfigData";
        case CmResourceTypeDevicePrivate: return "CmResourceTypeDevicePrivate";
        case CmResourceTypePcCardConfig: return "CmResourceTypePcCardConfig";
        default: return "*** INVALID RESOURCE TYPE ***";
    }
}

VOID
NTAPI
PciDebugPrintIoResource(IN PIO_RESOURCE_DESCRIPTOR Descriptor)
{
    ULONG i;
    PULONG Data;

    /* Print out the header */
    DPRINT1("     IoResource Descriptor dump:  Descriptor @0x%p\n", Descriptor);
    DPRINT1("        Option           = 0x%x\n", Descriptor->Option);
    DPRINT1("        Type             = %u (%s)\n", Descriptor->Type, PciDebugCmResourceTypeToText(Descriptor->Type));
    DPRINT1("        ShareDisposition = %u\n", Descriptor->ShareDisposition);
    DPRINT1("        Flags            = 0x%04X\n", Descriptor->Flags);

    /* Loop private data */
    Data = (PULONG)&Descriptor->u.DevicePrivate;
    for (i = 0; i < 6; i += 3)
    {
        /* Dump it in 32-bit triplets */
        DPRINT1("        Data[%u] = %08x  %08x  %08x\n", i, Data[0], Data[1], Data[2]);
    }
}

VOID
NTAPI
PciDebugPrintIoResReqList(IN PIO_RESOURCE_REQUIREMENTS_LIST Requirements)
{
    ULONG AlternativeLists;
    PIO_RESOURCE_LIST List;
    ULONG Count;
    PIO_RESOURCE_DESCRIPTOR Descriptor;

    /* Make sure there's a list */
    if (!Requirements) return;

    /* Grab the main list and the alternates as well */
    AlternativeLists = Requirements->AlternativeLists;
    List = Requirements->List;

    /* Print out the initial header*/
    DPRINT1("  IO_RESOURCE_REQUIREMENTS_LIST (PCI Bus Driver)\n");
    DPRINT1("     InterfaceType        %d\n", Requirements->InterfaceType);
    DPRINT1("     BusNumber            0x%x\n", Requirements->BusNumber);
    DPRINT1("     SlotNumber           %d (0x%x), (d/f = 0x%x/0x%x)\n",
            Requirements->SlotNumber,
            Requirements->SlotNumber,
            ((PCI_SLOT_NUMBER*)&Requirements->SlotNumber)->u.bits.DeviceNumber,
            ((PCI_SLOT_NUMBER*)&Requirements->SlotNumber)->u.bits.FunctionNumber);
    DPRINT1("     AlternativeLists     %u\n", AlternativeLists);

    /* Scan alternative lists */
    while (AlternativeLists--)
    {
        /* Get the descriptor array, and the count of descriptors */
        Descriptor = List->Descriptors;
        Count = List->Count;

        /* Print out each descriptor */
        DPRINT1("\n     List[%u].Count = %u\n", AlternativeLists, Count);
        while (Count--) PciDebugPrintIoResource(Descriptor++);

        /* Should've reached a new list now */
        List = (PIO_RESOURCE_LIST)Descriptor;
    }

    /* Terminate the dump */
    DPRINT1("\n");
}

VOID
NTAPI
PciDebugPrintPartialResource(IN PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialResource)
{
    /* Dump all the data in the partial */
    DPRINT1("     Partial Resource Descriptor @0x%p\n", PartialResource);
    DPRINT1("        Type             = %u (%s)\n", PartialResource->Type, PciDebugCmResourceTypeToText(PartialResource->Type));
    DPRINT1("        ShareDisposition = %u\n", PartialResource->ShareDisposition);
    DPRINT1("        Flags            = 0x%04X\n", PartialResource->Flags);
    DPRINT1("        Data[%d] = %08x  %08x  %08x\n",
            0,
            PartialResource->u.Generic.Start.LowPart,
            PartialResource->u.Generic.Start.HighPart,
            PartialResource->u.Generic.Length);
}

VOID
NTAPI
PciDebugPrintCmResList(IN PCM_RESOURCE_LIST PartialList)
{
    PCM_FULL_RESOURCE_DESCRIPTOR FullDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    ULONG Count, i, ListCount;

    /* Make sure there's something to dump */
    if (!PartialList) return;

    /* Get the full list count */
    ListCount = PartialList->Count;
    FullDescriptor = PartialList->List;
    DPRINT1("  CM_RESOURCE_LIST (PCI Bus Driver) (List Count = %u)\n", PartialList->Count);

    /* Loop full list */
    for (i = 0; i < ListCount; i++)
    {
        /* Loop full descriptor */
        DPRINT1("     InterfaceType        %d\n", FullDescriptor->InterfaceType);
        DPRINT1("     BusNumber            0x%x\n", FullDescriptor->BusNumber);

        /* Get partial count and loop partials */
        Count = FullDescriptor->PartialResourceList.Count;
        for (PartialDescriptor = FullDescriptor->PartialResourceList.PartialDescriptors;
             Count;
             PartialDescriptor = CmiGetNextPartialDescriptor(PartialDescriptor))
        {
            /* Print each partial */
            PciDebugPrintPartialResource(PartialDescriptor);
            Count--;
        }

        /* Go to the next full descriptor */
        FullDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)PartialDescriptor;
    }

    /* Done printing data */
    DPRINT1("\n");
}


/* EOF */
