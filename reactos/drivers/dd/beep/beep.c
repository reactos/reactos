/* $Id: beep.c,v 1.14 2002/09/08 10:22:04 chorns Exp $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 services/dd/beep/beep.c
 * PURPOSE:              BEEP device driver
 * PROGRAMMER:           Eric Kohl (ekohl@rz-online.de)
 * UPDATE HISTORY:
 *                       30/01/99 Created
 *                       16/10/99 Minor fixes
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <ddk/ntddbeep.h>

#define NDEBUG
#include <debug.h>


/* TYEPEDEFS ***************************************************************/

typedef struct _BEEP_DEVICE_EXTENSION
{
  KDPC Dpc;
  KTIMER Timer;
  KEVENT Event;
  BOOLEAN BeepOn;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


/* FUNCTIONS ***************************************************************/

static VOID STDCALL
BeepDPC(PKDPC Dpc,
	PVOID DeferredContext,
	PVOID SystemArgument1,
	PVOID SystemArgument2)
{
  PDEVICE_EXTENSION DeviceExtension = DeferredContext;

  DPRINT("BeepDPC() called!\n");

  HalMakeBeep(0);
  DeviceExtension->BeepOn = FALSE;
  KeSetEvent(&DeviceExtension->Event,
	     0,
	     TRUE);

  DPRINT("BeepDPC() finished!\n");
}


static NTSTATUS STDCALL
BeepCreate(PDEVICE_OBJECT DeviceObject,
	   PIRP Irp)
/*
 * FUNCTION: Handles user mode requests
 * ARGUMENTS:
 *                       DeviceObject = Device for request
 *                       Irp = I/O request packet describing request
 * RETURNS: Success or failure
 */
{
  DPRINT("BeepCreate() called!\n");

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp,
		    IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}


static NTSTATUS STDCALL
BeepClose(PDEVICE_OBJECT DeviceObject,
	  PIRP Irp)
/*
 * FUNCTION: Handles user mode requests
 * ARGUMENTS:
 *                       DeviceObject = Device for request
 *                       Irp = I/O request packet describing request
 * RETURNS: Success or failure
 */
{
  PDEVICE_EXTENSION DeviceExtension;
  NTSTATUS Status;

  DPRINT("BeepClose() called!\n");

  DeviceExtension = DeviceObject->DeviceExtension;
  if (DeviceExtension->BeepOn == TRUE)
    {
      HalMakeBeep(0);
      DeviceExtension->BeepOn = FALSE;
      KeCancelTimer(&DeviceExtension->Timer);
    }

  Status = STATUS_SUCCESS;

  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp,
		    IO_NO_INCREMENT);

  return(Status);
}


static NTSTATUS STDCALL
BeepCleanup(PDEVICE_OBJECT DeviceObject,
	    PIRP Irp)
/*
 * FUNCTION: Handles user mode requests
 * ARGUMENTS:
 *                       DeviceObject = Device for request
 *                       Irp = I/O request packet describing request
 * RETURNS: Success or failure
 */
{
  DPRINT("BeepCleanup() called!\n");

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp,
		    IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}


static NTSTATUS STDCALL
BeepDeviceControl(PDEVICE_OBJECT DeviceObject,
		  PIRP Irp)
/*
 * FUNCTION: Handles user mode requests
 * ARGUMENTS:
 *                       DeviceObject = Device for request
 *                       Irp = I/O request packet describing request
 * RETURNS: Success or failure
 */
{
  PIO_STACK_LOCATION Stack;
  PDEVICE_EXTENSION DeviceExtension;
  PBEEP_SET_PARAMETERS BeepParam;
  LARGE_INTEGER DueTime;

  DPRINT("BeepDeviceControl() called!\n");

  DeviceExtension = DeviceObject->DeviceExtension;
  Stack = IoGetCurrentIrpStackLocation(Irp);
  BeepParam = (PBEEP_SET_PARAMETERS)Irp->AssociatedIrp.SystemBuffer;

  Irp->IoStatus.Information = 0;

  if (Stack->Parameters.DeviceIoControl.IoControlCode != IOCTL_BEEP_SET)
    {
      Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
      IoCompleteRequest(Irp,
			IO_NO_INCREMENT);
      return(STATUS_NOT_IMPLEMENTED);
    }

  if ((Stack->Parameters.DeviceIoControl.InputBufferLength != sizeof(BEEP_SET_PARAMETERS))
      || (BeepParam->Frequency < BEEP_FREQUENCY_MINIMUM)
      || (BeepParam->Frequency > BEEP_FREQUENCY_MAXIMUM))
    {
      Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
      IoCompleteRequest(Irp,
			IO_NO_INCREMENT);
      return(STATUS_INVALID_PARAMETER);
    }

  DueTime.QuadPart = 0;

  /* do the beep!! */
  DPRINT("Beep:\n  Freq: %lu Hz\n  Dur: %lu ms\n",
	 pbsp->Frequency,
	 pbsp->Duration);

  if (BeepParam->Duration >= 0)
    {
      DueTime.QuadPart = (LONGLONG)BeepParam->Duration * -10000;

      KeSetTimer(&DeviceExtension->Timer,
		 DueTime,
		 &DeviceExtension->Dpc);

      HalMakeBeep(BeepParam->Frequency);
      DeviceExtension->BeepOn = TRUE;
      KeWaitForSingleObject(&DeviceExtension->Event,
			    Executive,
			    KernelMode,
			    FALSE,
			    NULL);
    }
  else if (BeepParam->Duration == (DWORD)-1)
    {
      if (DeviceExtension->BeepOn == TRUE)
	{
	  HalMakeBeep(0);
	  DeviceExtension->BeepOn = FALSE;
	}
      else
	{
	  HalMakeBeep(BeepParam->Frequency);
	  DeviceExtension->BeepOn = TRUE;
	}
    }

  DPRINT("Did the beep!\n");

  Irp->IoStatus.Status = STATUS_SUCCESS;
  IoCompleteRequest(Irp,
		    IO_NO_INCREMENT);
  return(STATUS_SUCCESS);
}


static NTSTATUS STDCALL
BeepUnload(PDRIVER_OBJECT DriverObject)
{
  DPRINT("BeepUnload() called!\n");
  return(STATUS_SUCCESS);
}


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
  PDEVICE_EXTENSION DeviceExtension;
  PDEVICE_OBJECT DeviceObject;
  UNICODE_STRING DeviceName = UNICODE_STRING_INITIALIZER(L"\\Device\\Beep");
  UNICODE_STRING SymlinkName = UNICODE_STRING_INITIALIZER(L"\\??\\Beep");
  NTSTATUS Status;

  DPRINT("Beep Device Driver 0.0.3\n");

  DriverObject->Flags = 0;
  DriverObject->MajorFunction[IRP_MJ_CREATE] = BeepCreate;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = BeepClose;
  DriverObject->MajorFunction[IRP_MJ_CLEANUP] = BeepCleanup;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = BeepDeviceControl;
  DriverObject->DriverUnload = BeepUnload;

  Status = IoCreateDevice(DriverObject,
			  sizeof(DEVICE_EXTENSION),
			  &DeviceName,
			  FILE_DEVICE_BEEP,
			  0,
			  FALSE,
			  &DeviceObject);
  if (!NT_SUCCESS(Status))
    return Status;

  /* set up device extension */
  DeviceExtension = DeviceObject->DeviceExtension;
  DeviceExtension->BeepOn = FALSE;

  KeInitializeDpc(&DeviceExtension->Dpc,
		  BeepDPC,
		  DeviceExtension);
  KeInitializeTimer(&DeviceExtension->Timer);
  KeInitializeEvent(&DeviceExtension->Event,
		    SynchronizationEvent,
		    FALSE);

  /* Create the dos device link */
  IoCreateSymbolicLink(&SymlinkName,
		       &DeviceName);

  return(STATUS_SUCCESS);
}

/* EOF */
