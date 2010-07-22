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
    /* This function is not yet implemented */
    UNIMPLEMENTED;
    while (TRUE);
    return FALSE;
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
    /* This function is not yet implemented */
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_NOT_SUPPORTED;
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
    HalPciAssignSlotResources = PciAssignSlotResources;
    HalPciTranslateBusAddress = PciTranslateBusAddress;
}

/* EOF */
