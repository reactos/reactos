/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/bus.c
 * PURPOSE:         Bus functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>


/* FUNCTIONS *****************************************************************/

NTSTATUS HalAssignSlotResources(PUNICODE_STRING RegistryPath,
				PUNICODE_STRING DriverClassName,
				PDRIVER_OBJECT DriverObject,
				PDEVICE_OBJECT DeviceObject,
				INTERFACE_TYPE BusType,
				ULONG BusNumber,
				ULONG SlotNumber,
				PCM_RESOURCE_LIST* AllocatedResources)
{
   UNIMPLEMENTED;
}

ULONG HalGetBusData(BUS_DATA_TYPE BusDataType,
		    ULONG BusNumber,
		    ULONG SlotNumber,
		    PVOID Buffer,
		    ULONG Length)
{
   UNIMPLEMENTED;
}

ULONG HalGetBusDataByOffset(BUS_DATA_TYPE BusDataType,
			    ULONG BusNumber,
			    ULONG SlotNumber,
			    PVOID Buffer,
			    ULONG Offset,
			    ULONG Length)
{
   UNIMPLEMENTED;
}
ULONG HalSetBusData(BUS_DATA_TYPE BusDataType,
		    ULONG BusNumber,
		    ULONG SlotNumber,
		    PVOID Buffer,
		    ULONG Length)
{
   UNIMPLEMENTED;
}

ULONG HalSetBusDataByOffset(BUS_DATA_TYPE BusDataType,
			    ULONG BusNumber,
			    ULONG SlotNumber,
			    PVOID Buffer,
			    ULONG Offset,
			    ULONG Length)
{
   UNIMPLEMENTED;
}

BOOLEAN HalTranslateBusAddress(INTERFACE_TYPE InterfaceType,
			       ULONG BusNumber,
			       PHYSICAL_ADDRESS BusAddress,
			       PULONG AddressSpace,
			       PPHYSICAL_ADDRESS TranslatedAddress)
{
   UNIMPLEMENTED;
}

