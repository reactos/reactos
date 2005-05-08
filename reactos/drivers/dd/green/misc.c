/* $Id:
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS VT100 emulator
 * FILE:            drivers/dd/green/misc.c
 * PURPOSE:         Misceallenous operations
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.com)
 */

//#define NDEBUG
#include "green.h"

NTSTATUS
GreenDeviceIoControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN ULONG CtlCode,
	IN PVOID InputBuffer OPTIONAL,
	IN ULONG InputBufferSize,
	IN OUT PVOID OutputBuffer OPTIONAL,
	IN OUT PULONG OutputBufferSize)
{
	KEVENT Event;
	PIRP Irp;
	IO_STATUS_BLOCK IoStatus;
	NTSTATUS Status;

	KeInitializeEvent (&Event, NotificationEvent, FALSE);

	Irp = IoBuildDeviceIoControlRequest(CtlCode,
		DeviceObject,
		InputBuffer,
		InputBufferSize,
		OutputBuffer,
		(OutputBufferSize) ? *OutputBufferSize : 0,
		FALSE,
		&Event,
		&IoStatus);
	if (Irp == NULL)
	{
		DPRINT("Green: IoBuildDeviceIoControlRequest() failed\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	Status = IoCallDriver(DeviceObject, Irp);

	if (Status == STATUS_PENDING)
	{
		DPRINT("Green: Operation pending\n");
		KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
		Status = IoStatus.Status;
	}

	if (OutputBufferSize)
	{
		*OutputBufferSize = IoStatus.Information;
	}

	return Status;
}
