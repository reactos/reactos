/* $Id: iface.c,v 1.63 2002/05/15 18:05:00 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/vfat/iface.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 * UPDATE HISTORY:
 *    ??           Created
 *   24-10-1998   Fixed bugs in long filename support
 *                Fixed a bug that prevented unsuccessful file open requests
 *                being reported
 *                Now works with long filenames that span over a sector
 *                boundary
 *   28-10-1998   Reads entire FAT into memory
 *                VFatReadSector modified to read in more than one sector at a
 *                time
 *   7-11-1998    Fixed bug that assumed that directory data could be
 *                fragmented
 *   8-12-1998    Added FAT32 support
 *                Added initial writability functions
 *                WARNING: DO NOT ATTEMPT TO TEST WRITABILITY FUNCTIONS!!!
 *   12-12-1998   Added basic support for FILE_STANDARD_INFORMATION request
 *
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* GLOBALS *****************************************************************/


PVFAT_GLOBAL_DATA VfatGlobalData;

/* FUNCTIONS ****************************************************************/

NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject,
	    PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Called by the system to initalize the driver
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 *           RegistryPath = path to our configuration entries
 * RETURNS: Success or failure
 */
{
   PDEVICE_OBJECT DeviceObject;
   UNICODE_STRING DeviceName;
   NTSTATUS Status;

   DPRINT("VFAT 0.0.6\n");

   RtlInitUnicodeString(&DeviceName,
			L"\\Fat");
   Status = IoCreateDevice(DriverObject,
			   sizeof(VFAT_GLOBAL_DATA),
			   &DeviceName,
			   FILE_DEVICE_DISK_FILE_SYSTEM,
			   0,
			   FALSE,
			   &DeviceObject);
   if (!NT_SUCCESS(Status))
     {
	return (Status);
     }
   VfatGlobalData = DeviceObject->DeviceExtension;
   RtlZeroMemory (VfatGlobalData, sizeof(VFAT_GLOBAL_DATA));
   VfatGlobalData->DriverObject = DriverObject;
   VfatGlobalData->DeviceObject = DeviceObject;

   DeviceObject->Flags = DO_DIRECT_IO;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = VfatBuildRequest;
   DriverObject->MajorFunction[IRP_MJ_CREATE] = VfatBuildRequest;
   DriverObject->MajorFunction[IRP_MJ_READ] = VfatBuildRequest;
   DriverObject->MajorFunction[IRP_MJ_WRITE] = VfatBuildRequest;
   DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = VfatBuildRequest;
   DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = VfatBuildRequest;
   DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = VfatBuildRequest;
   DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] = VfatBuildRequest;
   DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = VfatBuildRequest;
   DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION] = VfatBuildRequest;
   DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = VfatShutdown;
   DriverObject->MajorFunction[IRP_MJ_CLEANUP] = VfatBuildRequest;

   DriverObject->DriverUnload = NULL;

   IoRegisterFileSystem(DeviceObject);
   return STATUS_SUCCESS;
}

/* EOF */

