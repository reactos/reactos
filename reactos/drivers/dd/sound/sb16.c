/* $Id: sb16.c,v 1.1 2004/01/20 00:29:48 sedwards Exp $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 services/dd/sound/sb16.c
 * PURPOSE:              SB16 device driver
 * PROGRAMMER:           Steven Edwards
 * UPDATE HISTORY:
 *                       19/01/04 Created
 *                       
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <rosrtl/string.h>

#define NDEBUG
#include <debug.h>

NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject,
	    PUNICODE_STRING RegistryPath)
/*
 * FUNCTION:  Called by the system to initalize the driver
 * ARGUMENTS:
 *            DriverObject = object describing this driver
 *            RegistryPath = path to our configuration entries
 * RETURNS:   Success or failure
 */
{
  PDEVICE_OBJECT DeviceObject;
  UNICODE_STRING DeviceName = ROS_STRING_INITIALIZER(L"\\Device\\SNDBLST");
  UNICODE_STRING SymlinkName = ROS_STRING_INITIALIZER(L"\\??\\SNDBLST");
  NTSTATUS Status;

  DPRINT1("Sound Blaster 16 Driver 0.0.1\n");

  DriverObject->Flags = 0;

  Status = IoCreateDevice(DriverObject,
			  0,
			  &DeviceName,
			  FILE_DEVICE_BEEP,
			  0,
			  FALSE,
			  &DeviceObject);
  if (!NT_SUCCESS(Status))
    return Status;

  /* Create the dos device link */
  IoCreateSymbolicLink(&SymlinkName,
		       &DeviceName);

  return(STATUS_SUCCESS);
}

/* EOF */
