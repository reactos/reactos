/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/input/kbdclass/kbdclass.c
 * PURPOSE:          Keyboard class driver
 * PROGRAMMER:       Victor Kirhenshtein (sauros@iname.com)
 *                   Jason Filby (jasonfilby@yahoo.com)
 *                   Tinus_
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>

#include "kbdclass.h"

/* GLOBALS *******************************************************************/

/*
 * Driver data
 */

static BOOLEAN AlreadyOpened = FALSE;

static VOID STDCALL KbdCopyKeys(PDEVICE_OBJECT DeviceObject,
                           PIRP Irp)
{
	PDEVICE_EXTENSION DevExt = DeviceObject->DeviceExtension;
	PIO_STACK_LOCATION stk = IoGetCurrentIrpStackLocation(Irp);
	ULONG NrToRead = stk->Parameters.Read.Length /
		                                 sizeof(KEYBOARD_INPUT_DATA);
	ULONG NrRead = Irp->IoStatus.Information/sizeof(KEYBOARD_INPUT_DATA);
	KEYBOARD_INPUT_DATA *Rec =
	               (KEYBOARD_INPUT_DATA *)Irp->AssociatedIrp.SystemBuffer;

	while (DevExt->KeysInBuffer &&
	       NrRead < NrToRead) {
		memcpy(&Rec[NrRead],
		       &DevExt->KbdBuffer[DevExt->BufHead],
		       sizeof(KEYBOARD_INPUT_DATA));

		if (++DevExt->BufHead >= KBD_BUFFER_SIZE)
			DevExt->BufHead = 0;

		DevExt->KeysInBuffer--;
		NrRead++;
	}
	Irp->IoStatus.Information = NrRead * sizeof(KEYBOARD_INPUT_DATA);

	if (NrRead < NrToRead) {
		Irp->IoStatus.Status = STATUS_PENDING;
		DPRINT("Pending... (NrRead %d, NrToRead %d\n", NrRead, NrToRead);
	} else {
		DPRINT("Send scancode: %x\n", ((KEYBOARD_INPUT_DATA*)Irp->AssociatedIrp.SystemBuffer)->MakeCode);
		Irp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		IoStartNextPacket (DeviceObject, FALSE);
		DPRINT("Success!\n");
	}
}

static VOID STDCALL KbdClassServiceCallback (
                                      PDEVICE_OBJECT DeviceObject,
                                      PKEYBOARD_INPUT_DATA InputDataStart,
                                      PKEYBOARD_INPUT_DATA InputDataEnd,
                                      PULONG InputDataConsumed)
{
	PDEVICE_EXTENSION DevExt = DeviceObject->DeviceExtension;
	PKEYBOARD_INPUT_DATA CurrentInput = InputDataStart;

	DPRINT("ServiceCallback called\n");

	while (DevExt->KeysInBuffer < KBD_BUFFER_SIZE &&
	       CurrentInput < InputDataEnd) {
		memcpy(&DevExt->KbdBuffer[DevExt->BufTail],
		       CurrentInput,
		       sizeof(KEYBOARD_INPUT_DATA));

		if (++DevExt->BufTail >= KBD_BUFFER_SIZE)
			DevExt->BufTail = 0;
		DevExt->KeysInBuffer++;

		CurrentInput++;
		InputDataConsumed[0]++;
	}

	if (CurrentInput < InputDataStart)
		/* Copy the rest to the beginning, perhaps the keyboard
		 * can buffer it for us */
		memmove(InputDataStart,
		        CurrentInput,
		        ((char *)InputDataEnd - (char *)CurrentInput));

	if (DeviceObject->CurrentIrp) {
		PIRP Irp = DeviceObject->CurrentIrp;
   		PIO_STACK_LOCATION stk = IoGetCurrentIrpStackLocation(Irp);
		if (stk->MajorFunction == IRP_MJ_READ)
			KbdCopyKeys(DeviceObject, Irp);
	}
}

VOID STDCALL KbdStartIo(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	/* We only do this for read irps */
	DPRINT("KeyboardStartIo(DeviceObject %x Irp %x)\n",DeviceObject,Irp);
	KbdCopyKeys(DeviceObject, Irp);
}


/*
 * These are just passed down the stack but we must change the IOCTL to be
 * INTERNAL. MSDN says there might be more...
 */
NTSTATUS STDCALL KbdDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PDEVICE_EXTENSION DevExt = DeviceObject->DeviceExtension;
	PIO_STACK_LOCATION Stk = IoGetCurrentIrpStackLocation(Irp);
	PIO_STACK_LOCATION NextStk = IoGetNextIrpStackLocation(Irp);

	DPRINT ("KbdDeviceControl %x\n", Stk->Parameters.DeviceIoControl.IoControlCode);

	switch (Stk->Parameters.DeviceIoControl.IoControlCode) {
	case IOCTL_KEYBOARD_QUERY_ATTRIBUTES:
	case IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION:
	case IOCTL_KEYBOARD_QUERY_INDICATORS:
	case IOCTL_KEYBOARD_QUERY_TYPEMATIC:
	case IOCTL_KEYBOARD_SET_INDICATORS:
	case IOCTL_KEYBOARD_SET_TYPEMATIC: /* not in MSDN, would seem logical */
		IoCopyCurrentIrpStackLocationToNext(Irp);
		NextStk->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

		return IoCallDriver(DevExt->I8042Device, Irp);
	default:
		return STATUS_INVALID_DEVICE_REQUEST;
	}
}

static NTSTATUS STDCALL KbdInternalDeviceControl(PDEVICE_OBJECT DeviceObject,
                                                 PIRP Irp)
{
	PDEVICE_EXTENSION DevExt = DeviceObject->DeviceExtension;

	DPRINT ("KbdInternalDeviceControl\n");

	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(DevExt->I8042Device, Irp);
}

static NTSTATUS STDCALL KbdDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PIO_STACK_LOCATION stk = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS Status;

	DPRINT("DeviceObject %x\n",DeviceObject);
	DPRINT("Irp %x\n",Irp);

	DPRINT("Dispatch: stk->MajorFunction %d\n", stk->MajorFunction);
	DPRINT("AlreadyOpened %d\n",AlreadyOpened);

	switch (stk->MajorFunction) {
	case IRP_MJ_CREATE:
		if (AlreadyOpened == TRUE) {
			CHECKPOINT;
			Status = STATUS_UNSUCCESSFUL;
			DPRINT1("Keyboard is already open\n");
		} else {
			CHECKPOINT;
			Status = STATUS_SUCCESS;
			AlreadyOpened = TRUE;
		}
		break;

	case IRP_MJ_CLOSE:
		Status = STATUS_SUCCESS;
		AlreadyOpened = FALSE;
		break;

	case IRP_MJ_READ:
		DPRINT("Queueing packet\n");
		IoMarkIrpPending(Irp);
		IoStartPacket(DeviceObject,Irp,NULL,NULL);
		return(STATUS_PENDING);

	default:
		Status = STATUS_NOT_IMPLEMENTED;
		break;
	}

	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	DPRINT("Status %d\n",Status);
	return(Status);
}

static VOID STDCALL KbdClassSendConnect(PDEVICE_EXTENSION DevExt)
{
	CONNECT_DATA ConnectData;
	KEVENT Event;
	IO_STATUS_BLOCK IoStatus;
	NTSTATUS Status;
	PIRP Irp;

	KeInitializeEvent(&Event, NotificationEvent, FALSE);

	ConnectData.ClassDeviceObject = DevExt->DeviceObject;
	ConnectData.ClassService = KbdClassServiceCallback;

	Irp = IoBuildDeviceIoControlRequest(
			IOCTL_INTERNAL_KEYBOARD_CONNECT,
	                DevExt->I8042Device,
	                &ConnectData,
	                sizeof(CONNECT_DATA),
	                NULL,
	                0,
	                TRUE,
	                &Event,
	                &IoStatus);

	if (!Irp)
		return;

	Status = IoCallDriver(
	                DevExt->I8042Device,
	                Irp);
	DPRINT("SendConnect status: %x\n", Status);

	if (STATUS_PENDING ==Status)
		KeWaitForSingleObject(&Event,
		                      Executive,
		                      KernelMode,
		                      FALSE,
		                      NULL);
	DPRINT("SendConnect done\n");
}

NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT DriverObject,
			     PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Module entry point
 */
{
	PDEVICE_OBJECT DeviceObject;
	PDEVICE_EXTENSION DevExt;
	PFILE_OBJECT I8042File;
	NTSTATUS Status;
	UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\Keyboard");
	UNICODE_STRING SymlinkName = RTL_CONSTANT_STRING(L"\\??\\Keyboard");
	UNICODE_STRING I8042Name = RTL_CONSTANT_STRING(L"\\Device\\KeyboardClass0");

	DPRINT("Keyboard Class Driver 0.0.1\n");

	DriverObject->MajorFunction[IRP_MJ_CREATE] = KbdDispatch;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = KbdDispatch;
	DriverObject->MajorFunction[IRP_MJ_READ] = KbdDispatch;
	DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
	                                             KbdInternalDeviceControl;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = KbdDeviceControl;

	DriverObject->DriverStartIo = KbdStartIo;

	IoCreateDevice(DriverObject,
	               sizeof(DEVICE_EXTENSION),
	               &DeviceName,
	               FILE_DEVICE_KEYBOARD,
	               0,
	               TRUE,
	               &DeviceObject);

	RtlZeroMemory(DeviceObject->DeviceExtension, sizeof(DEVICE_EXTENSION));
	DevExt = DeviceObject->DeviceExtension;
	DevExt->DeviceObject = DeviceObject;

	Status = IoGetDeviceObjectPointer(&I8042Name,
	                                  FILE_READ_DATA,
	                                  &I8042File,
	                                  &DevExt->I8042Device);

	if (STATUS_SUCCESS != Status) {
		DPRINT("Failed to open device: %x\n", Status);
		return Status;
	}

	ObReferenceObject(DevExt->I8042Device);
	ObDereferenceObject(I8042File);

	DeviceObject->Flags = DeviceObject->Flags | DO_BUFFERED_IO;

	DeviceObject->StackSize = 1 + DevExt->I8042Device->StackSize;

	IoCreateSymbolicLink(&SymlinkName, &DeviceName);

	KbdClassSendConnect(DevExt);

	return(STATUS_SUCCESS);
}
