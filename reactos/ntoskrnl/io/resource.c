/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/resource.c
 * PURPOSE:         Hardware resource managment
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static CONFIGURATION_INFORMATION SystemConfigurationInformation = 
	{0, 0, 0, 0, 0, 0, 0, FALSE, FALSE};

/* FUNCTIONS *****************************************************************/

PCONFIGURATION_INFORMATION IoGetConfigurationInformation(VOID)
{
   return(&SystemConfigurationInformation);
}

NTSTATUS IoReportResourceUsage(PUNICODE_STRING DriverClassName,
			       PDRIVER_OBJECT DriverObject,
			       PCM_RESOURCE_LIST DriverList,
			       ULONG DriverListSize,
			       PDEVICE_OBJECT DeviceObject,
			       PCM_RESOURCE_LIST DeviceList,
			       ULONG DeviceListSize,
			       BOOLEAN OverrideConflict,
			       PBOOLEAN ConflictDetected)
{
   UNIMPLEMENTED;
}

NTSTATUS IoAssignResources(PUNICODE_STRING RegistryPath,
			   PUNICODE_STRING DriverClassName,
			   PDRIVER_OBJECT DriverObject,
			   PDEVICE_OBJECT DeviceObject,
			   PIO_RESOURCE_REQUIREMENTS_LIST RequestedResources,
			   PCM_RESOURCE_LIST* AllocatedResources)
{
   UNIMPLEMENTED;
}

NTSTATUS IoQueryDeviceDescription(PINTERFACE_TYPE BusType,
				  PULONG BusNumber,
				  PCONFIGURATION_TYPE ControllerType,
				  PULONG ControllerNumber,
				  PCONFIGURATION_TYPE PeripheralType,
				  PULONG PeripheralNumber,
				  PIO_QUERY_DEVICE_ROUTINE CalloutRoutine,
				  PVOID Context)
{
   UNIMPLEMENTED;
}
