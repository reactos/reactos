/*
	vfdpnp.c

	Virtual Floppy Drive for Windows NT platform
	Kernel mode driver: Plug & Play functions

	Copyright (C) 2003-2005 Ken Kato
*/

#ifndef VFD_PNP
/*
	Not in working order for the time being
	so DO NOT define VFD_PNP macro
	unless you know exactly what you are doing...
*/
#if !defined(__REACTOS__) || defined(_MSC_VER)
//	suppress empty compile unit warning
#pragma warning (disable: 4206)
#pragma message ("Plug and play support feature is disabled.")
#endif

#else	// VFD_PNP

#include "imports.h"
#include "vfddrv.h"
#include "vfddbg.h"

static NTSTATUS
VfdReportDevice(
	PDEVICE_EXTENSION	device_extension);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, VfdPlugAndPlay)
#pragma alloc_text(PAGE, VfdPowerControl)
#pragma alloc_text(PAGE, VfdSystemControl)
#pragma alloc_text(PAGE, VfdAddDevice)
#pragma alloc_text(PAGE, VfdReportDevice)
#endif	// ALLOC_PRAGMA

#define REMLOCK_TAG				'LdfV'	//	"VfdL"
#define REMLOCK_MAXIMUM			1		//	Max minutes system allows lock to be held
#define REMLOCK_HIGHWATER		10		//	Max number of irps holding lock at one time

#if DBG
static PCSTR StateTable[] ={
	{ "STOPPED"		},
	{ "WORKING"		},
	{ "PENDINGSTOP"	},
	{ "PENDINGREMOVE"	},
	{ "SURPRISEREMOVED" },
	{ "REMOVED"		},
	{ "UNKNOWN"		}
};
#endif	// DBG

//
//	PnP I/O request dispatch
//
NTSTATUS
VfdPlugAndPlay(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp)
{
	PIO_STACK_LOCATION		io_stack;
	PDEVICE_EXTENSION		device_extension;
	NTSTATUS				status = STATUS_SUCCESS;
	BOOLEAN				lockHeld = TRUE;

	//
	//	setup necessary pointers
	//
	io_stack			= IoGetCurrentIrpStackLocation( Irp );
	device_extension	= (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

	ASSERT(device_extension->DeviceState < VFD_MAX_STATE);

	VFDTRACE(VFDINFO, ("[VFD] VfdPlugAndPlay - IN. %ws %s Device State=%s\n",
		device_extension->device_name.Buffer,
		GetPnpIrpName(io_stack->MinorFunction),
		StateTable[device_extension->DeviceState]));

	//
	//	Acquire remove lock
	//
	status = IoAcquireRemoveLock(&device_extension->RemoveLock, Irp);

	if (!NT_SUCCESS(status)) {
		VFDTRACE(0, ("Acquire RemoveLock failed - %s\n", NtStatusToStr(status)));

		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return status;
	}

	//
	//	Process the PnP I/O request
	//
	switch (io_stack->MinorFunction) {
	case IRP_MN_START_DEVICE:					// 0x00
		//
		//	Start the device
		//
		device_extension->DeviceState = VFD_WORKING;
		status = STATUS_SUCCESS;

		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		break;

	case IRP_MN_QUERY_REMOVE_DEVICE:					// 0x01
		//
		//	Prepare device removal
		//
		device_extension->DeviceState = VFD_PENDINGREMOVE;
		status = STATUS_SUCCESS;

		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		break;

	case IRP_MN_REMOVE_DEVICE:							// 0x02
		//
		//	Remove the device
		//
		status = STATUS_SUCCESS;

		//	complete the current request
		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);

		//
		// Set the device status to REMOVED and wait for other drivers
		// to release the lock, then delete the device object
		//
		device_extension->DeviceState = VFD_REMOVED;
		IoReleaseRemoveLockAndWait(&device_extension->RemoveLock, Irp);
		lockHeld = FALSE;

		VfdRemoveDevice(DeviceObject);
		break;

	case IRP_MN_CANCEL_REMOVE_DEVICE:					// 0x03
		//
		// Before sending the IRP down make sure we have received
		// a IRP_MN_QUERY_REMOVE_DEVICE. We may get Cancel Remove
		// without receiving a Query Remove earlier, if the
		// driver on top fails a Query Remove and passes down the
		// Cancel Remove.
		//

		if (device_extension->DeviceState == VFD_PENDINGREMOVE) {
			device_extension->DeviceState = VFD_WORKING;
		}

		status = STATUS_SUCCESS;
		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);

		break;

	case IRP_MN_STOP_DEVICE:							// 0x04
		device_extension->DeviceState = VFD_STOPPED;

		status = STATUS_SUCCESS;
		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		break;

	case IRP_MN_QUERY_STOP_DEVICE:						// 0x05
		device_extension->DeviceState = VFD_PENDINGSTOP;

		status = STATUS_SUCCESS;
		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		break;

	case IRP_MN_CANCEL_STOP_DEVICE:						// 0x06
		//
		// Before sending the IRP down make sure we have received
		// a IRP_MN_QUERY_STOP_DEVICE. We may get Cancel Stop
		// without receiving a Query Stop earlier, if the
		// driver on top fails a Query Stop and passes down the
		// Cancel Stop.
		//

		if (device_extension->DeviceState == VFD_PENDINGSTOP ) {
			device_extension->DeviceState = VFD_WORKING;
		}

		status = STATUS_SUCCESS;
		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		break;

	case IRP_MN_QUERY_DEVICE_RELATIONS:				// 0x07
		status = STATUS_SUCCESS;
		Irp->IoStatus.Information = 0;

		switch (io_stack->Parameters.QueryDeviceRelations.Type) {
		case BusRelations:
			VFDTRACE(VFDINFO, ("------- BusRelations Query\n"));
			break;

		case EjectionRelations:
			VFDTRACE(VFDINFO, ("------- EjectionRelations Query\n"));
			break;

		case PowerRelations:
			VFDTRACE(VFDINFO, ("------- PowerRelations Query\n"));
			break;

		case RemovalRelations:
			VFDTRACE(VFDINFO, ("------- RemovalRelations Query\n"));
			break;

		case TargetDeviceRelation:
			VFDTRACE(VFDINFO, ("------- TargetDeviceRelation Query\n"));

			Irp->IoStatus.Information = (LONG)ExAllocatePoolWithTag(
				PagedPool, sizeof(DEVICE_RELATIONS), VFD_POOL_TAG);

			if (Irp->IoStatus.Information) {
				PDEVICE_RELATIONS rel = (PDEVICE_RELATIONS)Irp->IoStatus.Information;

				rel->Count = 1;
				rel->Objects[0] = device_extension->device_object;

				status = STATUS_SUCCESS;
			}
			else {
				status = STATUS_INSUFFICIENT_RESOURCES;
			}
			break;

		default:
			VFDTRACE(VFDINFO, ("------- Unknown Query\n"));
			break;
		}
		Irp->IoStatus.Status = status;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);

		break;

//	case IRP_MN_QUERY_INTERFACE:						// 0x08
//	case IRP_MN_QUERY_CAPABILITIES:					// 0x09
//	case IRP_MN_QUERY_RESOURCES:						// 0x0A
//	case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:			// 0x0B
//	case IRP_MN_QUERY_DEVICE_TEXT:						// 0x0C
//	case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:			// 0x0D
//	case IRP_MN_READ_CONFIG:							// 0x0F
//	case IRP_MN_WRITE_CONFIG:							// 0x10
//	case IRP_MN_EJECT:									// 0x11
//	case IRP_MN_SET_LOCK:								// 0x12
//	case IRP_MN_QUERY_ID:								// 0x13
//	case IRP_MN_QUERY_PNP_DEVICE_STATE:				// 0x14
//	case IRP_MN_QUERY_BUS_INFORMATION:					// 0x15
//	case IRP_MN_DEVICE_USAGE_NOTIFICATION:				// 0x16

	case IRP_MN_SURPRISE_REMOVAL:						// 0x17
		device_extension->DeviceState = VFD_SURPRISEREMOVED;

		status = STATUS_SUCCESS;
		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		break;

//	case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:			// 0x18

	default:
		//
		//	unknown request -- simply pass it to the lower device
		//
		status = STATUS_INVALID_DEVICE_REQUEST;
		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		break;
	}

	//
	// Device Extenion is gone if the current IRP is IRP_MN_REMOVE_DEVICE
	//
	if (lockHeld == TRUE) {
		IoReleaseRemoveLock(&device_extension->RemoveLock, Irp);
	}

	VFDTRACE(VFDINFO, ("[VFD] VfdPlugAndPlay - %s\n", NtStatusToStr(status)));

	return status;
}

//
//	Power management I/O request dispatch
//
NTSTATUS
VfdPowerControl(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP				Irp)
{
	PIO_STACK_LOCATION		io_stack;
	PDEVICE_EXTENSION		device_extension;
	NTSTATUS				status;

	io_stack = IoGetCurrentIrpStackLocation( Irp );
	device_extension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

	VFDTRACE(VFDINFO, ("[VFD] VfdPowerControl - IN. %ws %s Device State=%s\n",
		device_extension->device_name.Buffer,
		GetPnpIrpName(io_stack->MinorFunction),
		StateTable[device_extension->DeviceState]));

	PoStartNextPowerIrp(Irp);

	//
	// If the device has been removed, the driver should not pass
	// the IRP down to the next lower driver.
	//

	if (device_extension->DeviceState == VFD_REMOVED) {
		status = STATUS_DELETE_PENDING;
	}
	else {
		status = STATUS_SUCCESS;
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	VFDTRACE(VFDINFO, ("[VFD] VfdPowerControl - %s\n", NtStatusToStr(status)));

	return status;
}

//
//	WMI I/O request dispatch
//
NTSTATUS
VfdSystemControl(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP				Irp)
{
	PIO_STACK_LOCATION		io_stack;
	PDEVICE_EXTENSION		device_extension;
	NTSTATUS				status;

	io_stack = IoGetCurrentIrpStackLocation(Irp);
	device_extension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

	VFDTRACE(VFDINFO, ("[VFD] VfdSystemControl - IN. %ws %s Device State=%s\n",
		device_extension->device_name.Buffer,
		GetPnpIrpName(io_stack->MinorFunction),
		StateTable[device_extension->DeviceState]));

	status = STATUS_SUCCESS;
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	VFDTRACE(VFDINFO, ("[VFD] VfdSystemControl - %s\n", NtStatusToStr(status)));

	return status;
}

//
//	PnP AddDevice function
//
NTSTATUS
VfdAddDevice(
	IN		PDRIVER_OBJECT		DriverObject,
	IN OUT	PDEVICE_OBJECT		PhysicalDevice)
{
	PDEVICE_OBJECT		device_object;
	PDEVICE_EXTENSION	device_extension;
	NTSTATUS status;

	VFDTRACE(VFDINFO, ("[VFD] VfdAddDevice - IN\n"));

	status = VfdCreateDevice(DriverObject, &device_object);

	if (NT_SUCCESS(status)) {

		device_object->Flags |= DO_POWER_PAGABLE;

		device_extension =
			(PDEVICE_EXTENSION)device_object->DeviceExtension;

		// Device starts in Stopped state
		device_extension->DeviceState = VFD_STOPPED;

		VFDTRACE(VFDINFO, ("[VFD] Initializing the remove lock\n"));

		IoInitializeRemoveLock(
			&device_extension->RemoveLock,
			REMLOCK_TAG,
			REMLOCK_MAXIMUM,
			REMLOCK_HIGHWATER);

		if (PhysicalDevice) {
			device_extension->PhysicalDevice = PhysicalDevice;
		}
		else {
			VfdReportDevice(device_extension);
		}
		VfdRegisterInterface(device_extension);
		VfdMountMgrNotifyVolume(device_extension);
	}

	return status;
}

//
//	Report a VFD device to the PnP manager
//
NTSTATUS
VfdReportDevice(
	PDEVICE_EXTENSION	device_extension)
{
	NTSTATUS status	= STATUS_SUCCESS;
	CM_RESOURCE_LIST				list = {0};
	PCM_FULL_RESOURCE_DESCRIPTOR	full = &(list.List[0]);
	PCM_PARTIAL_RESOURCE_LIST		part = &(full->PartialResourceList);
	PCM_PARTIAL_RESOURCE_DESCRIPTOR	desc = &(part->PartialDescriptors[0]);;

	list.Count				= 1;

	full->InterfaceType		= Internal;
	full->BusNumber		= device_extension->device_number;

	part->Version			= 1;
	part->Revision			= 1;
	part->Count			= 1;

	desc->Type				= CmResourceTypeDeviceSpecific;
	desc->ShareDisposition	= CmResourceShareShared;
	desc->Flags			= 0;

	VFDTRACE(VFDINFO,("[VFD] Reporting device %lu to the PnP manager\n",
		device_extension->device_number));

	status = IoReportDetectedDevice(
		device_extension->device_object->DriverObject,	// IN PDRIVER_OBJECT DriverObject,
		Internal,								// IN INTERFACE_TYPE LegacyBusType,
		(ULONG)-1,								// IN ULONG BusNumber,
		(ULONG)-1,								// IN ULONG SlotNumber,
		&list,									// IN PCM_RESOURCE_LIST ResourceList,
		NULL,									// IN PIO_RESOURCE_REQUIREMENTS_LIST OPTIONAL,
		TRUE,									// IN BOOLEAN ResourceAssigned,
		&(device_extension->PhysicalDevice)		// IN OUT PDEVICE_OBJECT *DeviceObject
	);

	if (!NT_SUCCESS(status)) {
		VFDTRACE(0,
			("[VFD] IoReportDetectedDevice - %s\n",
			NtStatusToStr(status)));
	}

	device_extension->TargetDevice = IoAttachDeviceToDeviceStack(
		device_extension->device_object,
		device_extension->PhysicalDevice);

	return status;
}

#endif	// VFD_PNP
