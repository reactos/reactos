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
/* $Id: resource.c,v 1.9 2002/08/20 20:37:12 hyperion Exp $
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

static CONFIGURATION_INFORMATION
SystemConfigurationInformation = {0, 0, 0, 0, 0, 0, 0, FALSE, FALSE};

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
#if 0
  OBJECT_ATTRIBUTES ObjectAttributes;
  NTSTATUS Status;
  PWCH BaseKeyName[] = 
    L"\\Registry\\Machine\\Hardware\\MultifunctionAdapter\\0";
  HANDLE BaseKeyHandle;
  ULONG i;
  struct
  {
    KEY_BASIC_INFORMATION BasicInfo;
    WCH Name[255];
  } BasicInfo;

  BaseKeyName = L"\\Registry\\Machine\\Hardware\\MultifunctionAdapter";
  InitializeObjectAttributes(&ObjectAttributes,
			     BaseKeyName,
			     0,
			     NULL,
			     NULL);
  Status = ZwOpenKey(&BaseKeyHandle,
		     KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEY,
		     &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  i = 0;
  for (;;)
    {
      Status = ZwEnumerateKey(BaseKeyHandle,
			      i,
			      KeyBasicInformation,
			      &BasicInfo,
			      sizeof(BasicInfo),
			      &ResultLength);
      if (!NT_SUCCESS(Status))
	{
	  break;
	}

      
    }
#endif			  
  return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS STDCALL
IoReportHalResourceUsage(PUNICODE_STRING HalDescription,
			 PCM_RESOURCE_LIST RawList,
			 PCM_RESOURCE_LIST TranslatedList,
			 ULONG ListSize)
/*
 * FUNCTION:
 *      Reports hardware resources of the HAL in the
 *      \Registry\Machine\Hardware\ResourceMap tree.
 * ARGUMENTS:
 *      HalDescription: Descriptive name of the HAL.
 *      RawList: List of raw (bus specific) resources which should be
 *               claimed for the HAL.
 *      TranslatedList: List of translated (system wide) resources which
 *                      should be claimed for the HAL.
 *      ListSize: Size in bytes of the raw and translated resource lists.
 *                Both lists have the same size.
 * RETURNS:
 *      Status.
 */
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING Name;
  ULONG Disposition;
  NTSTATUS Status;
  HANDLE ResourcemapKey;
  HANDLE HalKey;
  HANDLE DescriptionKey;

  /* Open/Create 'RESOURCEMAP' key. */
  RtlInitUnicodeStringFromLiteral(&Name,
		       L"\\Registry\\Machine\\HARDWARE\\RESOURCEMAP");
  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
			     0,
			     NULL);
  Status = NtCreateKey(&ResourcemapKey,
		       KEY_ALL_ACCESS,
		       &ObjectAttributes,
		       0,
		       NULL,
		       REG_OPTION_VOLATILE,
		       &Disposition);
  if (!NT_SUCCESS(Status))
    return(Status);

  /* Open/Create 'Hardware Abstraction Layer' key */
  RtlInitUnicodeStringFromLiteral(&Name,
		       L"Hardware Abstraction Layer");
  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
			     ResourcemapKey,
			     NULL);
  Status = NtCreateKey(&HalKey,
		       KEY_ALL_ACCESS,
		       &ObjectAttributes,
		       0,
		       NULL,
		       REG_OPTION_VOLATILE,
		       &Disposition);
  NtClose(ResourcemapKey);
  if (!NT_SUCCESS(Status))
      return(Status);

  /* Create 'HalDescription' key */
  InitializeObjectAttributes(&ObjectAttributes,
			     HalDescription,
			     OBJ_CASE_INSENSITIVE,
			     HalKey,
			     NULL);
  Status = NtCreateKey(&DescriptionKey,
		       KEY_ALL_ACCESS,
		       &ObjectAttributes,
		       0,
		       NULL,
		       REG_OPTION_VOLATILE,
		       &Disposition);
  NtClose(HalKey);
  if (!NT_SUCCESS(Status))
    return(Status);

  /* Add '.Raw' value. */
  RtlInitUnicodeStringFromLiteral(&Name,
		       L".Raw");
  Status = NtSetValueKey(DescriptionKey,
			 &Name,
			 0,
			 REG_RESOURCE_LIST,
			 RawList,
			 ListSize);
  if (!NT_SUCCESS(Status))
    {
      NtClose(DescriptionKey);
      return(Status);
    }

  /* Add '.Translated' value. */
  RtlInitUnicodeStringFromLiteral(&Name,
		       L".Translated");
  Status = NtSetValueKey(DescriptionKey,
			 &Name,
			 0,
			 REG_RESOURCE_LIST,
			 TranslatedList,
			 ListSize);
  NtClose(DescriptionKey);

  return(Status);
}

/* EOF */
