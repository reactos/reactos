/*
 * PROJECT:     ReactOS VT100 emulator
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/base/green/power.c
 * PURPOSE:     IRP_MJ_POWER operations
 * PROGRAMMERS: Copyright 2006 Hervé Poussineau (hpoussin@reactos.org)
 */

#include "green.h"

#define NDEBUG
#include <debug.h>

NTSTATUS
GreenPower(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	GREEN_DEVICE_TYPE Type;
	PIO_STACK_LOCATION Stack;
	ULONG_PTR Information = Irp->IoStatus.Information;
	NTSTATUS Status = Irp->IoStatus.Status;

	Type = ((PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->Type;
	Stack = IoGetCurrentIrpStackLocation(Irp);

	switch (Stack->MinorFunction)
	{
		case IRP_MN_SET_POWER: /* 0x02 */
		{
			DPRINT("IRP_MJ_POWER / IRP_MN_SET_POWER\n");
			if (Type == GreenFDO)
			{
				PoStartNextPowerIrp(Irp);
				Status = STATUS_SUCCESS;
			}
			else
			{
				DPRINT1("IRP_MJ_POWER / IRP_MN_SET_POWER / Unknown type 0x%lx\n",
					Type);
				ASSERT(FALSE);
			}
			break;
		}
		default:
		{
			DPRINT1("IRP_MJ_POWER / unknown minor function 0x%lx\n", Stack->MinorFunction);
			break;
		}
	}

	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = Information;
	if (Status != STATUS_PENDING)
		IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return Status;
}
