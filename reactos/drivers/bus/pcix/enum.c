/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/enum.c
 * PURPOSE:         PCI Bus/Device Enumeration
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
PciScanBus(IN PPCI_FDO_EXTENSION DeviceExtension)
{
    ULONG MaxDevice = PCI_MAX_DEVICES;
    ULONG i, j, k;
    UCHAR Buffer[PCI_COMMON_HDR_LENGTH];
    PPCI_COMMON_HEADER PciData = (PVOID)Buffer;
    PCI_SLOT_NUMBER PciSlot;
    PWCHAR DescriptionText;
    DPRINT1("PCI Scan Bus: FDO Extension @ 0x%x, Base Bus = 0x%x\n",
            DeviceExtension, DeviceExtension->BaseBus);

    /* Is this the root FDO? */
    if (!PCI_IS_ROOT_FDO(DeviceExtension))
    {
        /* Other FDOs are not currently supported */
        UNIMPLEMENTED;
        while (TRUE);
    }

    /* Loop every device on the bus */
    PciSlot.u.bits.Reserved = 0;
    i = DeviceExtension->BaseBus;
    for (j = 0; j < MaxDevice; j++)
    {
        /* Loop every function of each device */
        PciSlot.u.bits.DeviceNumber = j;
        for (k = 0; k < PCI_MAX_FUNCTION; k++)
        {
            /* Build the final slot structure */
            PciSlot.u.bits.FunctionNumber = k;

            /* Read the vendor for this slot */
            PciReadSlotConfig(DeviceExtension,
                              PciSlot,
                              PciData,
                              0,
                              sizeof(USHORT));

            /* Skip invalid device */
            if (PciData->VendorID == PCI_INVALID_VENDORID) continue;

            /* Now read the whole header */
            PciReadSlotConfig(DeviceExtension,
                              PciSlot,
                              &PciData->DeviceID,
                              sizeof(USHORT),
                              PCI_COMMON_HDR_LENGTH - sizeof(USHORT));

            /* Dump device that was found */
            DPRINT1("Scan Found Device 0x%x (b=0x%x, d=0x%x, f=0x%x)\n",
                    PciSlot.u.AsULONG,
                    i,
                    j,
                    k);

            /* Dump the device's header */
            PciDebugDumpCommonConfig(PciData);

            /* Find description for this device for the debugger's sake */
            DescriptionText = PciGetDeviceDescriptionMessage(PciData->BaseClass,
                                                             PciData->SubClass);
            DPRINT1("Device Description \"%S\".\n", DescriptionText ? DescriptionText : L"(NULL)");
            if (DescriptionText) ExFreePoolWithTag(DescriptionText, 0);

            /* Check if there is an ACPI Watchdog Table */
            if (WdTable)
            {
                /* Check if this PCI device is the ACPI Watchdog Device... */
                UNIMPLEMENTED;
                while (TRUE);
            }
        }
    }

    /* Enumeration is completed */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PciQueryDeviceRelations(IN PPCI_FDO_EXTENSION DeviceExtension,
                        IN OUT PDEVICE_RELATIONS *pDeviceRelations)
{
    NTSTATUS Status;
    PPCI_PDO_EXTENSION PdoExtension;
    ULONG PdoCount = 0;
    PDEVICE_RELATIONS DeviceRelations, NewRelations;
    SIZE_T Size;
    PDEVICE_OBJECT DeviceObject, *ObjectArray;
    PAGED_CODE();

    /* Make sure the FDO is started */
    ASSERT(DeviceExtension->DeviceState == PciStarted);

    /* Synchronize while we enumerate the bus */
    Status = PciBeginStateTransition(DeviceExtension, PciSynchronizedOperation);
    if (!NT_SUCCESS(Status)) return Status;

    /* Scan all children PDO */
    for (PdoExtension = DeviceExtension->ChildPdoList;
         PdoExtension;
         PdoExtension = PdoExtension->Next)
    {
        /* Invalidate them */
        PdoExtension->NotPresent = TRUE;
    }

    /* Scan the PCI Bus */
    Status = PciScanBus(DeviceExtension);
    ASSERT(NT_SUCCESS(Status));

    /* Enumerate all children PDO again */
    for (PdoExtension = DeviceExtension->ChildPdoList;
         PdoExtension;
         PdoExtension = PdoExtension->Next)
    {
        /* Check for PDOs that are still invalidated */
        if (PdoExtension->NotPresent)
        {
            /* This means this PDO existed before, but not anymore */
            PdoExtension->ReportedMissing = TRUE;
            DPRINT1("PCI - Old device (pdox) %08x not found on rescan.\n",
                    PdoExtension);
        }
        else
        {
            /* Increase count of detected PDOs */
            PdoCount++;
        }
    }

    /* Read the current relations and add the newly discovered relations */
    DeviceRelations = *pDeviceRelations;
    Size = FIELD_OFFSET(DEVICE_RELATIONS, Objects) +
           PdoCount * sizeof(PDEVICE_OBJECT);
    if (DeviceRelations) Size += sizeof(PDEVICE_OBJECT) * DeviceRelations->Count;

    /* Allocate the device relations */
    NewRelations = (PDEVICE_RELATIONS)ExAllocatePoolWithTag(0, Size, 'BicP');
    if (!NewRelations)
    {
        /* Out of space, cancel the operation */
        PciCancelStateTransition(DeviceExtension, PciSynchronizedOperation);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Check if there were any older relations */
    NewRelations->Count = 0;
    if (DeviceRelations)
    {
        /* Copy the old relations into the new buffer, then free the old one */
        RtlCopyMemory(NewRelations,
                      DeviceRelations,
                      FIELD_OFFSET(DEVICE_RELATIONS, Objects) +
                      DeviceRelations->Count * sizeof(PDEVICE_OBJECT));
        ExFreePoolWithTag(DeviceRelations, 0);
    }

    /* Print out that we're ready to dump relations */
    DPRINT1("PCI QueryDeviceRelations/BusRelations FDOx %08x (bus 0x%02x)\n",
            DeviceExtension,
            DeviceExtension->BaseBus);

    /* Loop the current PDO children and the device relation object array */
    PdoExtension = DeviceExtension->ChildPdoList;
    ObjectArray = &NewRelations->Objects[NewRelations->Count];
    while (PdoExtension)
    {
        /* Dump this relation */
        DPRINT1("  QDR PDO %08x (x %08x)%s\n",
                PdoExtension->PhysicalDeviceObject,
                PdoExtension,
                PdoExtension->NotPresent ?
                "<Omitted, device flaged not present>" : "");

        /* Is this PDO present? */
        if (!PdoExtension->NotPresent)
        {
            /* Reference it and add it to the array */
            DeviceObject = PdoExtension->PhysicalDeviceObject;
            ObfReferenceObject(DeviceObject);
            *ObjectArray++ = DeviceObject;
        }

        /* Go to the next PDO */
        PdoExtension = PdoExtension->Next;
    }

    /* Terminate dumping the relations */
    DPRINT1("  QDR Total PDO count = %d (%d already in list)\n",
            NewRelations->Count + PdoCount,
            NewRelations->Count);

    /* Return the final count and the new buffer */
    NewRelations->Count += PdoCount;
    *pDeviceRelations = NewRelations;
    return STATUS_SUCCESS;
}

/* EOF */
