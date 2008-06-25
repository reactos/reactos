/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ex/atom.c
 * PURPOSE:         Executive Atom Functions
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Gunnar Dalsnes
 */

/* INCLUDES *****************************************************************/

#include <stdio.h>
#include <ntddk.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/


NTSTATUS
NTAPI
WmiCompleteRequest(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp,
                   IN NTSTATUS Status,
                   IN ULONG BufferUsed,
                   IN CCHAR PriorityBoost)
{
	DPRINT1("WmiLib: WmiCompleteRequest() unimplemented\n");
	return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
WmiFireEvent(IN PDEVICE_OBJECT DeviceObject,
             IN LPGUID Guid,
             IN ULONG InstanceIndex,
             IN ULONG EventDataSize,
             IN PVOID EventData)
{
	DPRINT1("WmiLib: WmiFireEvent() unimplemented\n");
	return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
WmiSystemControl(IN PWMILIB_CONTEXT WmiLibInfo,
                 IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp,
                 OUT PSYSCTL_IRP_DISPOSITION IrpDisposition)
{
	DPRINT1("WmiLib: WmiSystemControl() unimplemented\n");

	/* Return info that Irp is not completed */
	if (IrpDisposition)
		*IrpDisposition = IrpNotCompleted;

	return STATUS_SUCCESS;
}
