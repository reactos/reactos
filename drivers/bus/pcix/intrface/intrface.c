/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/intrface/intrface.c
 * PURPOSE:         Common Interface Support Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PPCI_INTERFACE PciInterfaces[] =
{
    &ArbiterInterfaceBusNumber,
    &ArbiterInterfaceMemory,
    &ArbiterInterfaceIo,
    &BusHandlerInterface,
    &PciRoutingInterface,
    &PciCardbusPrivateInterface,
    &PciLegacyDeviceDetectionInterface,
    &PciPmeInterface,
    &PciDevicePresentInterface,
//  &PciNativeIdeInterface,
    &PciLocationInterface,
    &AgpTargetInterface,
    NULL
};

PPCI_INTERFACE PciInterfacesLastResort[] =
{
    &TranslatorInterfaceInterrupt,
    NULL
};

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
PciQueryInterface(IN PPCI_FDO_EXTENSION DeviceExtension,
                  IN CONST GUID* InterfaceType,
                  IN ULONG Size,
                  IN ULONG Version,
                  IN PVOID InterfaceData,
                  IN PINTERFACE Interface,
                  IN BOOLEAN LastChance)
{
    UNICODE_STRING GuidString;
    NTSTATUS Status;
    PPCI_INTERFACE *InterfaceList;
    PPCI_INTERFACE PciInterface;
    RtlStringFromGUID(InterfaceType, &GuidString);
    DPRINT1("PCI - PciQueryInterface TYPE = %wZ\n", &GuidString);
    RtlFreeUnicodeString(&GuidString);
    DPRINT1("      Size = %u, Version = %u, InterfaceData = %p, LastChance = %s\n",
            Size,
            Version,
            InterfaceData,
            LastChance ? "TRUE" : "FALSE");

    /* Loop all the available interfaces */
    for (InterfaceList = LastChance ? PciInterfacesLastResort : PciInterfaces;
         *InterfaceList;
         InterfaceList++)
    {
        /* Get the current interface */
        PciInterface = *InterfaceList;

        /* For debugging, construct the GUID string */
        RtlStringFromGUID(PciInterface->InterfaceType, &GuidString);

        /* Check if this is an FDO or PDO */
        if (DeviceExtension->ExtensionType == PciFdoExtensionType)
        {
            /* Check if the interface is for FDOs */
            if (!(PciInterface->Flags & PCI_INTERFACE_FDO))
            {
                /* This interface is not for FDOs, skip it */
                DPRINT1("PCI - PciQueryInterface: guid = %wZ only for FDOs\n",
                        &GuidString);
                RtlFreeUnicodeString(&GuidString);
                continue;
            }

            /* Check if the interface is for root FDO only */
            if ((PciInterface->Flags & PCI_INTERFACE_ROOT) &&
                (!PCI_IS_ROOT_FDO(DeviceExtension)))
            {
                /* This FDO isn't the root, skip the interface */
                DPRINT1("PCI - PciQueryInterface: guid = %wZ only for ROOT\n",
                        &GuidString);
                RtlFreeUnicodeString(&GuidString);
                continue;
            }
        }
        else
        {
            /* This is a PDO, check if the interface is for PDOs too */
            if (!(PciInterface->Flags & PCI_INTERFACE_PDO))
            {
                /* It isn't, skip it */
                DPRINT1("PCI - PciQueryInterface: guid = %wZ only for PDOs\n",
                        &GuidString);
                RtlFreeUnicodeString(&GuidString);
                continue;
            }
        }

        /* Print the GUID for debugging, and then free the string */
        DPRINT1("PCI - PciQueryInterface looking at guid = %wZ\n", &GuidString);
        RtlFreeUnicodeString(&GuidString);

        /* Check if the GUID, version, and size all match */
        if ((IsEqualGUIDAligned(PciInterface->InterfaceType, InterfaceType)) &&
            (Version >= PciInterface->MinVersion) &&
            (Version <= PciInterface->MaxVersion) &&
            (Size >= PciInterface->MinSize))
        {
            /* Call the interface's constructor */
            Status = PciInterface->Constructor(DeviceExtension,
                                               PciInterface,
                                               InterfaceData,
                                               Version,
                                               Size,
                                               Interface);
            if (!NT_SUCCESS(Status))
            {
                /* This interface was not initialized correctly, skip it */
                DPRINT1("PCI - PciQueryInterface - Constructor %p = %08lx\n",
                        PciInterface->Constructor, Status);
                continue;
            }

            /* Reference the interface and return success, all is good */
            Interface->InterfaceReference(Interface->Context);
            DPRINT1("PCI - PciQueryInterface returning SUCCESS\n");
            return Status;
        }
    }

    /* An interface of this type, and for this device, could not be found */
    DPRINT1("PCI - PciQueryInterface FAILED TO FIND INTERFACE\n");
    return STATUS_NOT_SUPPORTED;
}

/* EOF */
