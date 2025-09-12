/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/hookhal.c
 * PURPOSE:         HAL Bus Handler Dispatch Routine Support
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

pHalTranslateBusAddress PcipSavedTranslateBusAddress;
pHalAssignSlotResources PcipSavedAssignSlotResources;

/* FUNCTIONS ******************************************************************/

BOOLEAN
NTAPI
PciTranslateBusAddress(IN INTERFACE_TYPE InterfaceType,
                       IN ULONG BusNumber,
                       IN PHYSICAL_ADDRESS BusAddress,
                       OUT PULONG AddressSpace,
                       OUT PPHYSICAL_ADDRESS TranslatedAddress)
{
    UNREFERENCED_PARAMETER(InterfaceType);
    UNREFERENCED_PARAMETER(BusNumber);
    UNREFERENCED_PARAMETER(AddressSpace);

    /* FIXME: Broken translation */
    UNIMPLEMENTED;
    TranslatedAddress->QuadPart = BusAddress.QuadPart;
    return TRUE;
}

PPCI_PDO_EXTENSION
NTAPI
PciFindPdoByLocation(IN ULONG BusNumber,
                     IN ULONG SlotNumber)
{
    PPCI_FDO_EXTENSION DeviceExtension;
    PPCI_PDO_EXTENSION PdoExtension;
    PCI_SLOT_NUMBER PciSlot;
    PciSlot.u.AsULONG = SlotNumber;

    /* Acquire the global lock */
    KeEnterCriticalRegion();
    KeWaitForSingleObject(&PciGlobalLock, Executive, KernelMode, FALSE, NULL);

    /* Now search for the extension */
    DeviceExtension = (PPCI_FDO_EXTENSION)PciFdoExtensionListHead.Next;
    while (DeviceExtension)
    {
        /* If we found it, break out */
        if (DeviceExtension->BaseBus == BusNumber) break;

        /* Move to the next device */
        DeviceExtension = (PPCI_FDO_EXTENSION)DeviceExtension->List.Next;
    }

    /* Release the global lock */
    KeSetEvent(&PciGlobalLock, IO_NO_INCREMENT, FALSE);
    KeLeaveCriticalRegion();

    /* Check if the device extension for the bus was found */
    if (!DeviceExtension)
    {
        /* It wasn't, bail out */
        DPRINT1("Pci: Could not find PCI bus FDO. Bus Number = 0x%x\n", BusNumber);
        return NULL;
    }

    /* Acquire this device's lock */
    KeEnterCriticalRegion();
    KeWaitForSingleObject(&DeviceExtension->ChildListLock,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    /* Loop every child PDO */
    for (PdoExtension = DeviceExtension->ChildPdoList;
         PdoExtension;
         PdoExtension = PdoExtension->Next)
    {
        /* Check if the function number and header data matches */
        if ((PdoExtension->Slot.u.bits.FunctionNumber == PciSlot.u.bits.FunctionNumber) &&
            (PdoExtension->Slot.u.bits.DeviceNumber == PciSlot.u.bits.DeviceNumber))
        {
            /* This is considered to be the same PDO */
            ASSERT(PdoExtension->Slot.u.AsULONG == PciSlot.u.AsULONG);
            break;
        }
    }

    /* Release this device's lock */
    KeSetEvent(&DeviceExtension->ChildListLock, IO_NO_INCREMENT, FALSE);
    KeLeaveCriticalRegion();

    /* Check if we found something */
    if (!PdoExtension)
    {
        /* Let the debugger know */
        DPRINT1("Pci: Could not find PDO for device @ %x.%x.%x\n",
                BusNumber,
                PciSlot.u.bits.DeviceNumber,
                PciSlot.u.bits.FunctionNumber);
    }

    /* If the search found something, this is non-NULL, otherwise it's NULL */
    return PdoExtension;
}

NTSTATUS
NTAPI
PciAssignSlotResources(IN PUNICODE_STRING RegistryPath,
                       IN PUNICODE_STRING DriverClassName OPTIONAL,
                       IN PDRIVER_OBJECT DriverObject,
                       IN PDEVICE_OBJECT DeviceObject,
                       IN INTERFACE_TYPE BusType,
                       IN ULONG BusNumber,
                       IN ULONG SlotNumber,
                       IN OUT PCM_RESOURCE_LIST *AllocatedResources)
{
    PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList = NULL;
    PCM_RESOURCE_LIST Resources = NULL;
    PCI_COMMON_HEADER PciData;
    PPCI_PDO_EXTENSION PdoExtension;
    NTSTATUS Status;
    PDEVICE_OBJECT ExistingDeviceObject;
    PAGED_CODE();
    ASSERT(PcipSavedAssignSlotResources);
    ASSERT(BusType == PCIBus);

    /* Assume no resources */
    *AllocatedResources = NULL;

    /* Find the PDO for this slot and make sure it exists and is started */
    PdoExtension = PciFindPdoByLocation(BusNumber, SlotNumber);
    if (!PdoExtension) return STATUS_DEVICE_DOES_NOT_EXIST;
    if (PdoExtension->DeviceState == PciNotStarted) return STATUS_INVALID_OWNER;

    /* Acquire the global lock while we attempt to assign resources */
    KeEnterCriticalRegion();
    KeWaitForSingleObject(&PciGlobalLock, Executive, KernelMode, FALSE, NULL);
    do
    {
        /* Make sure we're not on the PDO for some reason */
        ASSERT(DeviceObject != PdoExtension->PhysicalDeviceObject);

        /* Read the PCI header and cache the routing information */
        PciReadDeviceConfig(PdoExtension, &PciData, 0, PCI_COMMON_HDR_LENGTH);
        Status = PciCacheLegacyDeviceRouting(DeviceObject,
                                             BusNumber,
                                             SlotNumber,
                                             PciData.u.type0.InterruptLine,
                                             PciData.u.type0.InterruptPin,
                                             PciData.BaseClass,
                                             PciData.SubClass,
                                             PdoExtension->ParentFdoExtension->
                                             PhysicalDeviceObject,
                                             PdoExtension,
                                             &ExistingDeviceObject);
        if (NT_SUCCESS(Status))
        {
            /* Manually build the requirements for this device, and mark it legacy */
            Status = PciBuildRequirementsList(PdoExtension,
                                              &PciData,
                                              &RequirementsList);
            PdoExtension->LegacyDriver = TRUE;
            if (NT_SUCCESS(Status))
            {
                /* Now call the legacy Pnp function to actually assign resources */
                Status = IoAssignResources(RegistryPath,
                                           DriverClassName,
                                           DriverObject,
                                           DeviceObject,
                                           RequirementsList,
                                           &Resources);
                if (NT_SUCCESS(Status))
                {
                    /* Resources are ready, so enable all decodes */
                    PdoExtension->CommandEnables |= (PCI_ENABLE_IO_SPACE |
                                                     PCI_ENABLE_MEMORY_SPACE |
                                                     PCI_ENABLE_BUS_MASTER);

                    /* Compute new resource settings based on what PnP assigned */
                    PciComputeNewCurrentSettings(PdoExtension, Resources);

                    /* Set these new resources on the device */
                    Status = PciSetResources(PdoExtension, TRUE, TRUE);
                    if (NT_SUCCESS(Status))
                    {
                        /* Some work needs to happen here to handle this */
                        ASSERT(Resources->Count == 1);
                        //ASSERT(PartialList->Count > 0);

                        UNIMPLEMENTED;

                        /* Return the allocated resources, and success */
                        *AllocatedResources = Resources;
                        Resources = NULL;
                        Status = STATUS_SUCCESS;
                    }
                }
                else
                {
                    /* If assignment failed, no resources should exist */
                    ASSERT(Resources == NULL);
                }

                /* If assignment succeed, then we are done */
                if (NT_SUCCESS(Status)) break;
            }

            /* Otherwise, cache the new routing */
            PciCacheLegacyDeviceRouting(ExistingDeviceObject,
                                        BusNumber,
                                        SlotNumber,
                                        PciData.u.type0.InterruptLine,
                                        PciData.u.type0.InterruptPin,
                                        PciData.BaseClass,
                                        PciData.SubClass,
                                        PdoExtension->ParentFdoExtension->
                                        PhysicalDeviceObject,
                                        PdoExtension,
                                        NULL);
        }
    } while (0);

    /* Release the lock */
    KeSetEvent(&PciGlobalLock, 0, 0);
    KeLeaveCriticalRegion();

    /* Free any temporary resource data and return the status */
    if (RequirementsList) ExFreePoolWithTag(RequirementsList, 0);
    if (Resources) ExFreePoolWithTag(Resources, 0);
    return Status;
}

VOID
NTAPI
PciHookHal(VOID)
{
    /* Save the old HAL routines */
    ASSERT(PcipSavedAssignSlotResources == NULL);
    ASSERT(PcipSavedTranslateBusAddress == NULL);
    PcipSavedAssignSlotResources = HalPciAssignSlotResources;
    PcipSavedTranslateBusAddress = HalPciTranslateBusAddress;

    /* Take over the HAL's Bus Handler functions */
//    HalPciAssignSlotResources = PciAssignSlotResources;
    HalPciTranslateBusAddress = PciTranslateBusAddress;
}

/* EOF */
