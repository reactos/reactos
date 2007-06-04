/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/input/i8042prt/i8042prt.c
 * PURPOSE:          i8042 (ps/2 keyboard-mouse controller) driver
 * PROGRAMMER:       Victor Kirhenshtein (sauros@iname.com)
 *                   Jason Filby (jasonfilby@yahoo.com)
 *                   Tinus
 */

/* INCLUDES ****************************************************************/

#define NDEBUG
#include <debug.h>

#include "i8042prt.h"

/* GLOBALS *******************************************************************/

/*
 * Driver data
 */
#define I8042_TIMEOUT 500000

#define I8042_MAX_COMMAND_LENGTH 16
#define I8042_MAX_UPWARDS_STACK 5

UNICODE_STRING I8042RegistryPath;

/* FUNCTIONS *****************************************************************/

/*
 * FUNCTION: Write data to a port, waiting first for it to become ready
 */
BOOLEAN I8042Write(PDEVICE_EXTENSION DevExt, PUCHAR addr, UCHAR data)
{
	ULONG ResendIterations = DevExt->Settings.PollingIterations;

	while ((KBD_IBF & READ_PORT_UCHAR(I8042_CTRL_PORT)) &&
	       (ResendIterations--))
	{
		KeStallExecutionProcessor(50);
	}

	if (ResendIterations) {
		WRITE_PORT_UCHAR(addr,data);
		DPRINT("Sent %x to %x\n", data, addr);
		return TRUE;
	}
	return FALSE;
}

#if 0 /* function is not needed */
/*
 * FUNCTION: Write data to a port, without waiting first
 */
static BOOLEAN I8042WriteNoWait(PDEVICE_EXTENSION DevExt, int addr, UCHAR data)
{
	WRITE_PORT_UCHAR((PUCHAR)addr,data);
	DPRINT("Sent %x to %x\n", data, addr);
	return TRUE;
}
#endif

/*
 * FUNCTION: Read data from port 0x60
 */
NTSTATUS I8042ReadData(UCHAR *Data)
{
	UCHAR Status;
	Status=READ_PORT_UCHAR(I8042_CTRL_PORT);

	// If data is available
	if ((Status & KBD_OBF)) {
		Data[0]=READ_PORT_UCHAR((PUCHAR)I8042_DATA_PORT);
		DPRINT("Read: %x (status: %x)\n", Data[0], Status);

		// If the data is valid (not timeout, not parity error)
		if (0 == (Status & KBD_PERR))
			return STATUS_SUCCESS;
	}
	return STATUS_UNSUCCESSFUL;
}

NTSTATUS I8042ReadStatus(UCHAR *Status)
{
	Status[0]=READ_PORT_UCHAR(I8042_CTRL_PORT);
	return STATUS_SUCCESS;
}

/*
 * FUNCTION: Read data from port 0x60
 */
NTSTATUS I8042ReadDataWait(PDEVICE_EXTENSION DevExt, UCHAR *Data)
{
	ULONG Counter = DevExt->Settings.PollingIterations;
	NTSTATUS Status;

	while (Counter--) {
		Status = I8042ReadData(Data);

		if (STATUS_SUCCESS == Status)
			return Status;

		KeStallExecutionProcessor(50);
	}
	// Timed out
	return STATUS_IO_TIMEOUT;
}

VOID I8042Flush()
{
	UCHAR Ignore;

	while (STATUS_SUCCESS == I8042ReadData(&Ignore)) {
		DPRINT("Data flushed\n"); /* drop */
	}
}

VOID STDCALL I8042IsrWritePort(PDEVICE_EXTENSION DevExt,
                               UCHAR Value,
                               UCHAR SelectCmd)
{
	if (SelectCmd)
		if (!I8042Write(DevExt, I8042_CTRL_PORT, SelectCmd))
			return;

	I8042Write(DevExt, I8042_DATA_PORT, Value);
}

/*
 * These functions are callbacks for filter driver custom
 * initialization routines.
 */
NTSTATUS STDCALL I8042SynchWritePort(PDEVICE_EXTENSION DevExt,
                                     UCHAR Port,
                                     UCHAR Value,
                                     BOOLEAN WaitForAck)
{
	NTSTATUS Status;
	UCHAR Ack;
	ULONG ResendIterations = DevExt->Settings.ResendIterations + 1;

	do {
		if (Port)
			if (!I8042Write(DevExt, I8042_DATA_PORT, Port))
			{
				DPRINT1("Failed to write Port\n");
				return STATUS_IO_TIMEOUT;
			}

		if (!I8042Write(DevExt, I8042_DATA_PORT, Value))
		{
			DPRINT1("Failed to write Value\n");
			return STATUS_IO_TIMEOUT;
		}

		if (WaitForAck) {
			Status = I8042ReadDataWait(DevExt, &Ack);
			if (!NT_SUCCESS(Status))
			{
				DPRINT1("Failed to read Ack\n");
				return Status;
			}
			if (Ack == KBD_ACK)
				return STATUS_SUCCESS;
			if (Ack != KBD_RESEND)
			{
				DPRINT1("Unexpected Ack 0x%x\n", Ack);
				return STATUS_UNEXPECTED_IO_ERROR;
			}
		} else {
			return STATUS_SUCCESS;
		}
		DPRINT("Reiterating\n");
		ResendIterations--;
	} while (ResendIterations);
	return STATUS_IO_TIMEOUT;
}

/*
 * This one reads a value from the port; You don't have to specify
 * which one, it'll always be from the one you talked to, so one function
 * is enough this time. Note how MSDN specifies the
 * WaitForAck parameter to be ignored.
 */
static NTSTATUS STDCALL I8042SynchReadPort(PVOID Context,
                                           PUCHAR Value,
                                           BOOLEAN WaitForAck)
{
	PDEVICE_EXTENSION DevExt = (PDEVICE_EXTENSION)Context;

	return I8042ReadDataWait(DevExt, Value);
}

/* Write the current byte of the packet. Returns FALSE in case
 * of problems.
 */
static BOOLEAN STDCALL I8042PacketWrite(PDEVICE_EXTENSION DevExt)
{
	UCHAR Port = DevExt->PacketPort;

	if (Port) {
		if (!I8042Write(DevExt,
		                I8042_CTRL_PORT,
		                Port)) {
			/* something is really wrong! */
			DPRINT1("Failed to send packet byte!\n");
			return FALSE;
		}
	}

	return I8042Write(DevExt,
	                  I8042_DATA_PORT,
	                  DevExt->Packet.Bytes[DevExt->Packet.CurrentByte]);
}


/*
 * This function starts a packet. It must be called with the
 * correct DIRQL.
 */
NTSTATUS STDCALL I8042StartPacket(PDEVICE_EXTENSION DevExt,
                                  PDEVICE_OBJECT Device,
                                  PUCHAR Bytes,
                                  ULONG ByteCount,
                                  PIRP Irp)
{
	KIRQL Irql;
	NTSTATUS Status;
	PFDO_DEVICE_EXTENSION FdoDevExt = Device->DeviceExtension;

	Irql = KeAcquireInterruptSpinLock(DevExt->HighestDIRQLInterrupt);

	DevExt->CurrentIrp = Irp;
	DevExt->CurrentIrpDevice = Device;

	if (Idle != DevExt->Packet.State) {
		Status = STATUS_DEVICE_BUSY;
		goto startpacketdone;
	}

	DevExt->Packet.Bytes = Bytes;
	DevExt->Packet.CurrentByte = 0;
	DevExt->Packet.ByteCount = ByteCount;
	DevExt->Packet.State = SendingBytes;
	DevExt->PacketResult = Status = STATUS_PENDING;

	if (Mouse == FdoDevExt->Type)
		DevExt->PacketPort = 0xD4;
	else
		DevExt->PacketPort = 0;

	if (!I8042PacketWrite(DevExt)) {
		Status = STATUS_IO_TIMEOUT;
		DevExt->Packet.State = Idle;
		DevExt->PacketResult = STATUS_ABANDONED;
		goto startpacketdone;
	}

	DevExt->Packet.CurrentByte++;

startpacketdone:
	KeReleaseInterruptSpinLock(DevExt->HighestDIRQLInterrupt, Irql);

	if (STATUS_PENDING != Status) {
		DevExt->CurrentIrp = NULL;
		DevExt->CurrentIrpDevice = NULL;
		Irp->IoStatus.Status = Status;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}
	return Status;
}

BOOLEAN STDCALL I8042PacketIsr(PDEVICE_EXTENSION DevExt,
                            UCHAR Output)
{
	if (Idle == DevExt->Packet.State)
		return FALSE;

	switch (Output) {
	case KBD_RESEND:
		DevExt->PacketResends++;
		if (DevExt->PacketResends > DevExt->Settings.ResendIterations) {
			DevExt->Packet.State = Idle;
			DevExt->PacketComplete = TRUE;
			DevExt->PacketResult = STATUS_IO_TIMEOUT;
			DevExt->PacketResends = 0;
			return TRUE;
		}
		DevExt->Packet.CurrentByte--;
		break;

	case KBD_NACK:
		DevExt->Packet.State = Idle;
		DevExt->PacketComplete = TRUE;
		DevExt->PacketResult = STATUS_UNEXPECTED_IO_ERROR;
		DevExt->PacketResends = 0;
		return TRUE;

	default:
		DevExt->PacketResends = 0;
	}

	if (DevExt->Packet.CurrentByte >= DevExt->Packet.ByteCount) {
		DevExt->Packet.State = Idle;
		DevExt->PacketComplete = TRUE;
		DevExt->PacketResult = STATUS_SUCCESS;
		return TRUE;
	}

	if (!I8042PacketWrite(DevExt)) {
		DevExt->Packet.State = Idle;
		DevExt->PacketComplete = TRUE;
		DevExt->PacketResult = STATUS_IO_TIMEOUT;
		return TRUE;
	}
	DevExt->Packet.CurrentByte++;

	return TRUE;
}

VOID I8042PacketDpc(PDEVICE_EXTENSION DevExt)
{
	BOOLEAN FinishIrp = FALSE;
	NTSTATUS Result = STATUS_INTERNAL_ERROR; /* Shouldn't happen */
	KIRQL Irql;

	/* If the interrupt happens before this is setup, the key
	 * was already in the buffer. Too bad! */
	if (!DevExt->HighestDIRQLInterrupt)
		return;

	Irql = KeAcquireInterruptSpinLock(DevExt->HighestDIRQLInterrupt);

	if (Idle == DevExt->Packet.State &&
	    DevExt->PacketComplete) {
		FinishIrp = TRUE;
		Result = DevExt->PacketResult;
		DevExt->PacketComplete = FALSE;
	}

	KeReleaseInterruptSpinLock(DevExt->HighestDIRQLInterrupt,
	                           Irql);

	if (!FinishIrp)
		return;

	if (DevExt->CurrentIrp) {
		DevExt->CurrentIrp->IoStatus.Status = Result;
		IoCompleteRequest(DevExt->CurrentIrp, IO_NO_INCREMENT);
		IoStartNextPacket(DevExt->CurrentIrpDevice, FALSE);
		DevExt->CurrentIrp = NULL;
		DevExt->CurrentIrpDevice = NULL;
	}
}

VOID STDCALL I8042SendHookWorkItem(PDEVICE_OBJECT DeviceObject,
                           PVOID Context)
{
	KEVENT Event;
	IO_STATUS_BLOCK IoStatus;
	NTSTATUS Status;
	PDEVICE_EXTENSION DevExt;
	PFDO_DEVICE_EXTENSION FdoDevExt;
	PIRP NewIrp;
	PI8042_HOOK_WORKITEM WorkItemData = (PI8042_HOOK_WORKITEM)Context;

	ULONG IoControlCode;
	PVOID InputBuffer;
	ULONG InputBufferLength;
	BOOLEAN IsKbd;

	DPRINT("HookWorkItem\n");

	FdoDevExt = (PFDO_DEVICE_EXTENSION)
	                          DeviceObject->DeviceExtension;

	DevExt = FdoDevExt->PortDevExt;

	if (WorkItemData->Target == DevExt->KeyboardData.ClassDeviceObject) {
		IoControlCode = IOCTL_INTERNAL_I8042_HOOK_KEYBOARD;
		InputBuffer = &DevExt->KeyboardHook;
		InputBufferLength = sizeof(INTERNAL_I8042_HOOK_KEYBOARD);
		IsKbd = TRUE;
		DPRINT ("is for keyboard.\n");
	} else if (WorkItemData->Target == DevExt->MouseData.ClassDeviceObject){
		IoControlCode = IOCTL_INTERNAL_I8042_HOOK_MOUSE;
		InputBuffer = &DevExt->MouseHook;
		InputBufferLength = sizeof(INTERNAL_I8042_HOOK_MOUSE);
		IsKbd = FALSE;
		DPRINT ("is for mouse.\n");
	} else {
		DPRINT1("I8042SendHookWorkItem: Can't find DeviceObject\n");
		WorkItemData->Irp->IoStatus.Status = STATUS_INTERNAL_ERROR;
		goto hookworkitemdone;
	}

	KeInitializeEvent(&Event, NotificationEvent, FALSE);

	NewIrp = IoBuildDeviceIoControlRequest(
                      IoControlCode,
                      WorkItemData->Target,
                      InputBuffer,
                      InputBufferLength,
                      NULL,
                      0,
                      TRUE,
                      &Event,
		      &IoStatus);

	if (!NewIrp) {
		DPRINT("IOCTL_INTERNAL_(device)_CONNECT: "
		       "Can't allocate IRP\n");
		WorkItemData->Irp->IoStatus.Status =
		              STATUS_INSUFFICIENT_RESOURCES;
		goto hookworkitemdone;
	}

#if 0
	Status = IoCallDriver(
			WorkItemData->Target,
	                NewIrp);

	if (STATUS_PENDING == Status)
		KeWaitForSingleObject(&Event,
		                      Executive,
		                      KernelMode,
		                      FALSE,
		                      NULL);
#endif

	if (IsKbd) {
		/* Call the hooked initialization if it exists */
		if (DevExt->KeyboardHook.InitializationRoutine) {
			Status = DevExt->KeyboardHook.InitializationRoutine(
			                      DevExt->KeyboardHook.Context,
			                      DevExt,
					      I8042SynchReadPort,
					      I8042SynchWritePortKbd,
					      FALSE);
			if (!NT_SUCCESS(Status)) {
				WorkItemData->Irp->IoStatus.Status = Status;
				goto hookworkitemdone;
			}
		}
		/* TODO: Now would be the right time to enable the interrupt */

		DevExt->KeyboardClaimed = TRUE;
	} else {
		/* Mouse doesn't have this, but we need to send a
		 * reset to start the detection.
		 */
		KIRQL Irql;

		Irql = KeAcquireInterruptSpinLock(
				            DevExt->HighestDIRQLInterrupt);

		I8042Write(DevExt, I8042_CTRL_PORT, 0xD4);
		I8042Write(DevExt, I8042_DATA_PORT, 0xFF);

		KeReleaseInterruptSpinLock(DevExt->HighestDIRQLInterrupt, Irql);
	}

	WorkItemData->Irp->IoStatus.Status = STATUS_SUCCESS;

hookworkitemdone:
	WorkItemData->Irp->IoStatus.Information = 0;
	IoCompleteRequest(WorkItemData->Irp, IO_NO_INCREMENT);

	IoFreeWorkItem(WorkItemData->WorkItem);
	ExFreePool(WorkItemData);
	DPRINT("HookWorkItem done\n");
}

static VOID STDCALL I8042StartIo(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	if (!I8042StartIoKbd(DeviceObject, Irp)) {
		DPRINT1("Unhandled StartIo!\n");
	}
}

static NTSTATUS STDCALL I8042InternalDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;
	PFDO_DEVICE_EXTENSION FdoDevExt = DeviceObject->DeviceExtension;

	DPRINT("InternalDeviceControl\n");

	switch (FdoDevExt->Type) {
	case Keyboard:
		Status = I8042InternalDeviceControlKbd(DeviceObject, Irp);
		break;
	case Mouse:
		Status = I8042InternalDeviceControlMouse(DeviceObject, Irp);
		break;
	}

	if (Status == STATUS_INVALID_DEVICE_REQUEST) {
		DPRINT1("Invalid internal device request!\n");
	}

	if (Status != STATUS_PENDING)
		IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return Status;
}

static NTSTATUS STDCALL I8042CreateDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	NTSTATUS Status;

	DPRINT ("I8042CreateDispatch\n");

	Status = STATUS_SUCCESS;

	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return Status;
}

static NTSTATUS STDCALL I8042BasicDetect(PDEVICE_EXTENSION DevExt)
{
	NTSTATUS Status;
	UCHAR Value = 0;
	ULONG Counter;

	DevExt->MouseExists = FALSE;
	DevExt->KeyboardExists = FALSE;

	I8042Flush();

	if (!I8042Write(DevExt, I8042_CTRL_PORT, KBD_SELF_TEST)) {
		DPRINT1("Writing KBD_SELF_TEST command failed\n");
		return STATUS_IO_TIMEOUT;
	}

	// Wait longer?
	Counter = 10;
	do {
		Status = I8042ReadDataWait(DevExt, &Value);
	} while ((Counter--) && (STATUS_IO_TIMEOUT == Status));

	if (!NT_SUCCESS(Status)) {
		DPRINT1("Failed to read KBD_SELF_TEST response, status 0x%x\n",
		        Status);
		return Status;
	}

	if (Value != 0x55) {
		DPRINT1("Got %x instead of 55\n", Value);
		return STATUS_IO_DEVICE_ERROR;
	}

	if (!I8042Write(DevExt, I8042_CTRL_PORT, KBD_READ_MODE)) {
		DPRINT1("Can't read i8042 mode\n");
		return FALSE;
	}

	Status = I8042ReadDataWait(DevExt, &Value);
	if (!NT_SUCCESS(Status)) {
		DPRINT1("No response after read i8042 mode\n");
		return FALSE;
	}

	Value |= CCB_KBD_DISAB | CCB_MOUSE_DISAB; /* disable keyboard/mouse */
	Value &= ~(CCB_KBD_INT_ENAB | CCB_MOUSE_INT_ENAB);
                 /* don't enable keyboard and mouse interrupts */

	if (!I8042Write(DevExt, I8042_CTRL_PORT, KBD_WRITE_MODE)) {
		DPRINT1("Can't set i8042 mode\n");
		return FALSE;
	}

	if (!I8042Write(DevExt, I8042_DATA_PORT, Value)) {
		DPRINT1("Can't send i8042 mode\n");
		return FALSE;
	}

	/*
	 * We used to send a KBD_LINE_TEST command here, but on at least HP
	 * Pavilion notebooks the response to that command was incorrect.
	 * So now we just assume that a keyboard is attached.
	 */
	DevExt->KeyboardExists = TRUE;

	if (I8042Write(DevExt, I8042_CTRL_PORT, MOUSE_LINE_TEST))
	{
		Status = I8042ReadDataWait(DevExt, &Value);
		if (NT_SUCCESS(Status) && Value == 0)
			DevExt->MouseExists = TRUE;
	}

	return STATUS_SUCCESS;
}

static NTSTATUS STDCALL I8042Initialize(PDEVICE_EXTENSION DevExt)
{
	NTSTATUS Status;
	UCHAR Value = 0;

	Status = I8042BasicDetect(DevExt);
	if (!NT_SUCCESS(Status)) {
		DPRINT1("Basic keyboard detection failed: %x\n", Status);
		return Status;
	}

	if (DevExt->MouseExists) {
		DPRINT("Aux port detected\n");
		DevExt->MouseExists = I8042DetectMouse(DevExt);
	}

	if (!DevExt->KeyboardExists) {
		DPRINT("Keyboard port not detected\n");
		if (DevExt->Settings.Headless)
			/* Act as if it exists regardless */
			DevExt->KeyboardExists = TRUE;
	} else {
		DPRINT("Keyboard port detected\n");
		DevExt->KeyboardExists = I8042DetectKeyboard(DevExt);
	}

	if (DevExt->KeyboardExists) {
		DPRINT("Keyboard detected\n");
		I8042KeyboardEnable(DevExt);
		I8042KeyboardEnableInterrupt(DevExt);
	}

	if (DevExt->MouseExists) {
		DPRINT("Mouse detected\n");
		I8042MouseEnable(DevExt);
	}

	/*
	 * Some machines do not reboot if SF is not set.
	 */
	if (!I8042Write(DevExt, I8042_CTRL_PORT, KBD_READ_MODE)) {
		DPRINT1("Can't read i8042 mode\n");
		return Status;
	}

	Status = I8042ReadDataWait(DevExt, &Value);
	if (!NT_SUCCESS(Status)) {
		DPRINT1("No response after read i8042 mode\n");
		return Status;
	}

	Value |= CCB_SYSTEM_FLAG;

	if (!I8042Write(DevExt, I8042_CTRL_PORT, KBD_WRITE_MODE)) {
		DPRINT1("Can't set i8042 mode\n");
		return Status;
	}

	if (!I8042Write(DevExt, I8042_DATA_PORT, Value)) {
		DPRINT1("Can't send i8042 mode\n");
		return Status;
	}

	return STATUS_SUCCESS;
}

static NTSTATUS
AddRegistryEntry(
	IN PCWSTR PortTypeName,
	IN PUNICODE_STRING DeviceName,
	IN PCWSTR RegistryPath)
{
	UNICODE_STRING PathU = RTL_CONSTANT_STRING(L"\\REGISTRY\\MACHINE\\HARDWARE\\DEVICEMAP");
	OBJECT_ATTRIBUTES ObjectAttributes;
	HANDLE hDeviceMapKey = (HANDLE)-1;
	HANDLE hPortKey = (HANDLE)-1;
	UNICODE_STRING PortTypeNameU;
	NTSTATUS Status;

	InitializeObjectAttributes(&ObjectAttributes, &PathU, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
	Status = ZwOpenKey(&hDeviceMapKey, 0, &ObjectAttributes);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("ZwOpenKey() failed with status 0x%08lx\n", Status);
		goto cleanup;
	}

	RtlInitUnicodeString(&PortTypeNameU, PortTypeName);
	InitializeObjectAttributes(&ObjectAttributes, &PortTypeNameU, OBJ_KERNEL_HANDLE, hDeviceMapKey, NULL);
	Status = ZwCreateKey(&hPortKey, KEY_SET_VALUE, &ObjectAttributes, 0, NULL, REG_OPTION_VOLATILE, NULL);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("ZwCreateKey() failed with status 0x%08lx\n", Status);
		goto cleanup;
	}

	Status = ZwSetValueKey(hPortKey, DeviceName, 0, REG_SZ, (PVOID)RegistryPath, wcslen(RegistryPath) * sizeof(WCHAR) + sizeof(UNICODE_NULL));
	if (!NT_SUCCESS(Status))
	{
		DPRINT("ZwSetValueKey() failed with status 0x%08lx\n", Status);
		goto cleanup;
	}

	Status = STATUS_SUCCESS;

cleanup:
	if (hDeviceMapKey != (HANDLE)-1)
		ZwClose(hDeviceMapKey);
	if (hPortKey != (HANDLE)-1)
		ZwClose(hPortKey);
	return Status;
}

static NTSTATUS STDCALL I8042AddDevice(PDRIVER_OBJECT DriverObject,
                                       PDEVICE_OBJECT Pdo)
{
	UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\KeyboardPort8042");
	UNICODE_STRING MouseName = RTL_CONSTANT_STRING(L"\\Device\\PointerPort8042");
	ULONG MappedIrqKeyboard = 0, MappedIrqMouse = 0;
	KIRQL DirqlKeyboard = 0;
	KIRQL DirqlMouse = 0;
	KIRQL DirqlMax;
	KAFFINITY Affinity;
	NTSTATUS Status;
	PDEVICE_EXTENSION DevExt;
	PFDO_DEVICE_EXTENSION FdoDevExt;
	PDEVICE_OBJECT Fdo;
	static BOOLEAN AlreadyInitialized = FALSE;

	DPRINT("I8042AddDevice\n");

	if (Pdo != NULL)
	{
		/* Device detected by pnpmgr. Ignore it, as we already have
		 * detected the keyboard and mouse at first call */
		return STATUS_UNSUCCESSFUL;
	}

	if (AlreadyInitialized)
		return STATUS_SUCCESS;
	AlreadyInitialized = TRUE;

	Status = IoCreateDevice(DriverObject,
	               sizeof(DEVICE_EXTENSION),
	               NULL,
	               FILE_DEVICE_8042_PORT,
	               FILE_DEVICE_SECURE_OPEN,
	               TRUE,
	               &Fdo);

	if (!NT_SUCCESS(Status))
		return Status;

	DevExt = Fdo->DeviceExtension;

	RtlZeroMemory(DevExt, sizeof(DEVICE_EXTENSION));

	I8042ReadRegistry(DriverObject, DevExt);

	KeInitializeSpinLock(&DevExt->SpinLock);
	InitializeListHead(&DevExt->BusDevices);

	KeInitializeDpc(&DevExt->DpcKbd,
	                I8042DpcRoutineKbd,
	                DevExt);

	KeInitializeDpc(&DevExt->DpcMouse,
	                I8042DpcRoutineMouse,
	                DevExt);

	KeInitializeDpc(&DevExt->DpcMouseTimeout,
	                I8042DpcRoutineMouseTimeout,
	                DevExt);

	KeInitializeTimer(&DevExt->TimerMouseTimeout);

	Status = I8042Initialize(DevExt);
    Fdo->Flags &= ~DO_DEVICE_INITIALIZING;
	if (!NT_SUCCESS(STATUS_SUCCESS)) {
		DPRINT1("Initialization failure: %x\n", Status);
		return Status;
	}

	if (DevExt->KeyboardExists) {
		MappedIrqKeyboard = HalGetInterruptVector(Internal,
		                                          0,
		                                          KEYBOARD_IRQ,
		                                          0,
		                                          &DirqlKeyboard,
		                                          &Affinity);

		Status = IoCreateDevice(DriverObject,
		                        sizeof(FDO_DEVICE_EXTENSION),
		                        &DeviceName,
		                        FILE_DEVICE_8042_PORT,
		                        FILE_DEVICE_SECURE_OPEN,
		                        TRUE,
		                        &Fdo);

		if (NT_SUCCESS(Status))
		{
			AddRegistryEntry(L"KeyboardPort", &DeviceName, L"REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\i8042prt");
			FdoDevExt = Fdo->DeviceExtension;

			RtlZeroMemory(FdoDevExt, sizeof(FDO_DEVICE_EXTENSION));

			FdoDevExt->PortDevExt = DevExt;
			FdoDevExt->Type = Keyboard;
			FdoDevExt->DeviceObject = Fdo;

			Fdo->Flags |= DO_BUFFERED_IO;

			DevExt->DebugWorkItem = IoAllocateWorkItem(Fdo);
			DevExt->KeyboardObject = Fdo;

			DevExt->KeyboardBuffer = ExAllocatePoolWithTag(
			               NonPagedPool,
			               DevExt->KeyboardAttributes.InputDataQueueLength *
			                          sizeof(KEYBOARD_INPUT_DATA),
			               TAG_I8042);

			if (!DevExt->KeyboardBuffer) {
				DPRINT1("No memory for keyboardbuffer\n");
				return STATUS_INSUFFICIENT_RESOURCES;
			}

			InsertTailList(&DevExt->BusDevices, &FdoDevExt->BusDevices);
            Fdo->Flags &= ~DO_DEVICE_INITIALIZING;
		}
		else
			DevExt->KeyboardExists = FALSE;
	}

	if (DevExt->MouseExists) {
		MappedIrqMouse = HalGetInterruptVector(Internal,
		                                          0,
		                                          MOUSE_IRQ,
		                                          0,
		                                          &DirqlMouse,
		                                          &Affinity);

		Status = IoCreateDevice(DriverObject,
		                        sizeof(FDO_DEVICE_EXTENSION),
		                        &MouseName,
		                        FILE_DEVICE_8042_PORT,
		                        FILE_DEVICE_SECURE_OPEN,
		                        TRUE,
		                        &Fdo);

		if (NT_SUCCESS(Status))
		{
			AddRegistryEntry(L"PointerPort", &MouseName, L"REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\i8042prt");
			FdoDevExt = Fdo->DeviceExtension;

			RtlZeroMemory(FdoDevExt, sizeof(FDO_DEVICE_EXTENSION));

			FdoDevExt->PortDevExt = DevExt;
			FdoDevExt->Type = Mouse;
			FdoDevExt->DeviceObject = Fdo;

			Fdo->Flags |= DO_BUFFERED_IO;
			DevExt->MouseObject = Fdo;

			DevExt->MouseBuffer = ExAllocatePoolWithTag(
			               NonPagedPool,
			               DevExt->MouseAttributes.InputDataQueueLength *
			                             sizeof(MOUSE_INPUT_DATA),
			               TAG_I8042);

			if (!DevExt->MouseBuffer) {
				ExFreePoolWithTag(DevExt->KeyboardBuffer, TAG_I8042);
				DPRINT1("No memory for mouse buffer\n");
				return STATUS_INSUFFICIENT_RESOURCES;
			}

			InsertTailList(&DevExt->BusDevices, &FdoDevExt->BusDevices);
            Fdo->Flags &= ~DO_DEVICE_INITIALIZING;
		}
		else
			DevExt->MouseExists = FALSE;
	}

	if (DirqlKeyboard > DirqlMouse)
		DirqlMax = DirqlKeyboard;
	else
		DirqlMax = DirqlMouse;

	if (DevExt->KeyboardExists) {
		Status = IoConnectInterrupt(&DevExt->KeyboardInterruptObject,
		                            I8042InterruptServiceKbd,
		                            (PVOID)DevExt,
		                            &DevExt->SpinLock,
		                            MappedIrqKeyboard,
		                            DirqlKeyboard,
		                            DirqlMax,
		                            Latched,
		                            FALSE,
		                            Affinity,
		                            FALSE);

		DPRINT("Keyboard Irq Status: %x\n", Status);
	}

	if (DevExt->MouseExists) {
		Status = IoConnectInterrupt(&DevExt->MouseInterruptObject,
		                            I8042InterruptServiceMouse,
		                            (PVOID)DevExt,
		                            &DevExt->SpinLock,
		                            MappedIrqMouse,
		                            DirqlMouse,
		                            DirqlMax,
		                            Latched,
		                            FALSE,
		                            Affinity,
					    FALSE);

		DPRINT("Mouse Irq Status: %x\n", Status);
	}

	if (DirqlKeyboard > DirqlMouse)
		DevExt->HighestDIRQLInterrupt = DevExt->KeyboardInterruptObject;
	else
		DevExt->HighestDIRQLInterrupt = DevExt->MouseInterruptObject;

	DPRINT("I8042AddDevice done\n");

	return(STATUS_SUCCESS);
}

NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT DriverObject,
			     PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Module entry point
 */
{
	DPRINT("I8042 Driver 0.0.1\n");

        I8042RegistryPath.MaximumLength = RegistryPath->Length + sizeof(L"\\Parameters");
        I8042RegistryPath.Buffer = ExAllocatePoolWithTag(PagedPool, 
                                                         I8042RegistryPath.MaximumLength,
                                                         TAG_I8042);
        if (I8042RegistryPath.Buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyUnicodeString(&I8042RegistryPath, RegistryPath);
        RtlAppendUnicodeToString(&I8042RegistryPath, L"\\Parameters");
        I8042RegistryPath.Buffer[I8042RegistryPath.Length / sizeof(WCHAR)] = 0;



	DriverObject->MajorFunction[IRP_MJ_CREATE] = I8042CreateDispatch;
	DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
	                                       I8042InternalDeviceControl;

	DriverObject->DriverStartIo = I8042StartIo;
	DriverObject->DriverExtension->AddDevice = I8042AddDevice;
	I8042AddDevice(DriverObject, NULL);

	return(STATUS_SUCCESS);
}
