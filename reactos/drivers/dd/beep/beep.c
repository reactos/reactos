/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                         services/dd/beep/beep.c
 * PURPOSE:              BEEP device driver
 * PROGRAMMER:           Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * UPDATE HISTORY:
 *                       Created 30/01/99
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <ddk/ntddbeep.h>

#define NDEBUG
#include <internal/debug.h>


/* TYEPEDEFS ***************************************************************/

typedef struct _BEEP_DEVICE_EXTENSION
{
	KDPC    Dpc;
	KTIMER  Timer;
	KEVENT  Event;
        LONG    BeepOn;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


/* FUNCTIONS ***************************************************************/


VOID BeepDPC(PKDPC Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
	PDEVICE_EXTENSION pDeviceExtension = DeferredContext;

	DPRINT("BeepDPC() called!\n");
	HalMakeBeep (0);
	InterlockedExchange (&(pDeviceExtension->BeepOn), 0);
	KeSetEvent (&(pDeviceExtension->Event), 0, TRUE);

	DPRINT("BeepDPC() finished!\n");
}


NTSTATUS BeepCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: Handles user mode requests
 * ARGUMENTS:
 *                       DeviceObject = Device for request
 *                       Irp = I/O request packet describing request
 * RETURNS: Success or failure
 */
{
	PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS status;

	if (Stack->MajorFunction == IRP_MJ_CREATE)
	{
		DPRINT ("BeepCreate() called!\n");
		Irp->IoStatus.Information = 0;
		status = STATUS_SUCCESS;
	}
	else
		status = STATUS_NOT_IMPLEMENTED;

	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	return(status);
}


NTSTATUS BeepClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: Handles user mode requests
 * ARGUMENTS:
 *                       DeviceObject = Device for request
 *                       Irp = I/O request packet describing request
 * RETURNS: Success or failure
 */
{
	PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS status;

	switch (Stack->MajorFunction)
	{
		case IRP_MJ_CLOSE:
			DPRINT ("BeepClose() called!\n");
			Irp->IoStatus.Information = 0;
			status = STATUS_SUCCESS;
			break;

		default:
			status = STATUS_NOT_IMPLEMENTED;
	}

	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	return(status);
}


NTSTATUS BeepCleanup(PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: Handles user mode requests
 * ARGUMENTS:
 *                       DeviceObject = Device for request
 *                       Irp = I/O request packet describing request
 * RETURNS: Success or failure
 */
{
	PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS status;

	if (Stack->MajorFunction == IRP_MJ_CLEANUP)
	{
		DPRINT ("BeepCleanup() called!\n");
		Irp->IoStatus.Information = 0;
		status = STATUS_SUCCESS;
	}
	else
		status = STATUS_NOT_IMPLEMENTED;

	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	return(status);
}


NTSTATUS BeepDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: Handles user mode requests
 * ARGUMENTS:
 *                       DeviceObject = Device for request
 *                       Irp = I/O request packet describing request
 * RETURNS: Success or failure
 */
{
	PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
	PDEVICE_EXTENSION pDeviceExtension;
	PBEEP_SET_PARAMETERS pbsp;
	NTSTATUS status;

	pDeviceExtension = DeviceObject->DeviceExtension;

	if (Stack->MajorFunction == IRP_MJ_DEVICE_CONTROL)
	{
		DPRINT("BeepDeviceControl() called!\n");
		if (Stack->Parameters.DeviceIoControl.IoControlCode == IOCTL_BEEP_SET)
		{
			Irp->IoStatus.Information = 0;
			if (Stack->Parameters.DeviceIoControl.InputBufferLength == sizeof(BEEP_SET_PARAMETERS))
			{
				pbsp = (PBEEP_SET_PARAMETERS)Irp->AssociatedIrp.SystemBuffer;

				if (pbsp->Frequency >= BEEP_FREQUENCY_MINIMUM &&
				    pbsp->Frequency <= BEEP_FREQUENCY_MAXIMUM)
				{
					LARGE_INTEGER DueTime = 0;

					/* do the beep!! */
					DPRINT("Beep:\n  Freq: %lu Hz\n  Dur: %lu ms\n",
					       pbsp->Frequency, pbsp->Duration);

					if (pbsp->Duration >= 0)
					{
						DueTime = (LARGE_INTEGER)pbsp->Duration * 10000;

						KeSetTimer (&pDeviceExtension->Timer,
						            -DueTime,
						            &pDeviceExtension->Dpc);

						HalMakeBeep (pbsp->Frequency);
						InterlockedExchange(&(pDeviceExtension->BeepOn), TRUE);
						KeWaitForSingleObject (&(pDeviceExtension->Event),
						                       Executive,
						                       KernelMode,
						                       FALSE,
						                       NULL);
					}
					else if (pbsp->Duration == (DWORD)-1)
					{
						if (pDeviceExtension->BeepOn)
						{
							HalMakeBeep(0);
							InterlockedExchange(&(pDeviceExtension->BeepOn), FALSE);
						}
						else
						{
							HalMakeBeep(pbsp->Frequency);
							InterlockedExchange(&(pDeviceExtension->BeepOn), TRUE);
						}
					}

					DPRINT("Did the beep!\n");

					status = STATUS_SUCCESS;
				}
				else
				{
					status = STATUS_INVALID_PARAMETER;
				}
			}
			else
			{
				status = STATUS_INVALID_PARAMETER;
			}
		}
		else
		{
			status = STATUS_NOT_IMPLEMENTED;
		}
	}
	else
		status = STATUS_NOT_IMPLEMENTED;

	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	return(status);
}


NTSTATUS BeepUnload(PDRIVER_OBJECT DriverObject)
{
	DPRINT("BeepUnload() called!\n");
	return(STATUS_SUCCESS);
}


NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Called by the system to initalize the driver
 * ARGUMENTS:
 *                       DriverObject = object describing this driver
 *                       RegistryPath = path to our configuration entries
 * RETURNS: Success or failure
 */
{
    PDEVICE_OBJECT    pDeviceObject;
    PDEVICE_EXTENSION pDeviceExtension;
    NTSTATUS ret;
    ANSI_STRING ansi_device_name;
    UNICODE_STRING device_name;
    ANSI_STRING asymlink_name;
    UNICODE_STRING symlink_name;

    DbgPrint("Beep Device Driver 0.0.1\n");

    RtlInitAnsiString(&ansi_device_name,"\\Device\\Beep");
    RtlAnsiStringToUnicodeString(&device_name,&ansi_device_name,TRUE);
    ret = IoCreateDevice(DriverObject,
                         sizeof(DEVICE_EXTENSION),
                         &device_name,
                         FILE_DEVICE_BEEP,
                         0,
                         FALSE,
                         &pDeviceObject);
    if (ret!=STATUS_SUCCESS)
    {
        return(ret);
    }

    /* prelininary */
    RtlInitAnsiString(&asymlink_name,"\\??\\Beep");
    RtlAnsiStringToUnicodeString(&symlink_name,&asymlink_name,TRUE);
    IoCreateSymbolicLink(&symlink_name,&device_name);

    DriverObject->MajorFunction[IRP_MJ_CREATE] = BeepCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = BeepClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = BeepCleanup;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = BeepDeviceControl;
    DriverObject->DriverUnload = BeepUnload;

    /* set up device extension */
    pDeviceObject->Flags |= DO_BUFFERED_IO;
    pDeviceExtension = pDeviceObject->DeviceExtension;
    pDeviceExtension->BeepOn = 0; /* FALSE */

    KeInitializeDpc (&(pDeviceExtension->Dpc),
                     BeepDPC,
                     pDeviceExtension);
    KeInitializeTimer (&(pDeviceExtension->Timer));
    KeInitializeEvent (&(pDeviceExtension->Event),
                       SynchronizationEvent,
                       FALSE);

    return(STATUS_SUCCESS);
}

