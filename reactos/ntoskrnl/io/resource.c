/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: resource.c,v 1.6 2001/08/31 19:30:16 dwelch Exp $
 *
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

PCONFIGURATION_INFORMATION STDCALL
IoGetConfigurationInformation(VOID)
{
   return(&SystemConfigurationInformation);
}

NTSTATUS STDCALL
IoReportResourceUsage(PUNICODE_STRING DriverClassName,
		      PDRIVER_OBJECT DriverObject,
		      PCM_RESOURCE_LIST DriverList,
		      ULONG DriverListSize,
		      PDEVICE_OBJECT DeviceObject,
		      PCM_RESOURCE_LIST DeviceList,
		      ULONG DeviceListSize,
		      BOOLEAN OverrideConflict,
		      PBOOLEAN ConflictDetected)
     /*
      * FUNCTION: Reports hardware resources in the 
      * \Registry\Machine\Hardware\ResourceMap tree, so that a subsequently
      * loaded driver cannot attempt to use the same resources.
      * ARGUMENTS:
      *       DriverClassName - The class of driver under which the resource
      *       information should be stored.
      *       DriverObject - The driver object that was input to the 
      *       DriverEntry.
      *       DriverList - Resources that claimed for the driver rather than
      *       per-device.
      *       DriverListSize - Size in bytes of the DriverList.
      *       DeviceObject - The device object for which resources should be
      *       claimed.
      *       DeviceList - List of resources which should be claimed for the
      *       device.
      *       DeviceListSize - Size of the per-device resource list in bytes.
      *       OverrideConflict - True if the resources should be cliamed
      *       even if a conflict is found.
      *       ConflictDetected - Points to a variable that receives TRUE if
      *       a conflict is detected with another driver.
      */
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL
IoAssignResources(PUNICODE_STRING RegistryPath,
		  PUNICODE_STRING DriverClassName,
		  PDRIVER_OBJECT DriverObject,
		  PDEVICE_OBJECT DeviceObject,
		  PIO_RESOURCE_REQUIREMENTS_LIST RequestedResources,
		  PCM_RESOURCE_LIST* AllocatedResources)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL
IoQueryDeviceDescription(PINTERFACE_TYPE BusType,
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

NTSTATUS STDCALL
IoReportHalResourceUsage (PUNICODE_STRING	HalDescription,
			  ULONG		Unknown1,
			  ULONG		Unknown2,
			  ULONG		Unknown3)
{
  UNIMPLEMENTED;
}

/* EOF */
