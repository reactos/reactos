/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/input/i8042prt/keyboard.c
 * PURPOSE:          i8042 (ps/2 keyboard-mouse controller) driver
 *                   keyboard specifics
 * PROGRAMMER:       Victor Kirhenshtein (sauros@iname.com)
 *                   Jason Filby (jasonfilby@yahoo.com)
 *                   Tinus
 */

/* INCLUDES ****************************************************************/

#include "i8042prt.h"
#include "kdfuncs.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

static UCHAR TypematicTable[] = {
	0x00, 0x00, 0x00, 0x05, 0x08, 0x0B, 0x0D, 0x0F, 0x10, 0x12, /*  0-9 */
	0x13, 0x14, 0x15, 0x16, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1A, /* 10-19 */
	0x1B, 0x1C, 0x1C, 0x1C, 0x1D, 0x1D, 0x1E };

typedef struct _LOCAL_KEYBOARD_INDICATOR_TRANSLATION {
	USHORT NumberOfIndicatorKeys;
	INDICATOR_LIST IndicatorList[3];
} LOCAL_KEYBOARD_INDICATOR_TRANSLATION, *PLOCAL_KEYBOARD_INDICATOR_TRANSLATION;

static LOCAL_KEYBOARD_INDICATOR_TRANSLATION IndicatorTranslation = { 3, {
	{0x3A, KEYBOARD_CAPS_LOCK_ON},
	{0x45, KEYBOARD_NUM_LOCK_ON},
	{0x46, KEYBOARD_SCROLL_LOCK_ON}}};

static VOID STDCALL I8042DebugWorkItem(PDEVICE_OBJECT DeviceObject,
                                       PVOID Context);

/* FUNCTIONS *****************************************************************/

/*
 * These functions are callbacks for filter driver custom interrupt
 * service routines.
 */
VOID STDCALL I8042IsrWritePortKbd(PVOID Context,
                                         UCHAR Value)
{
	I8042IsrWritePort(Context, Value, 0);
}

static VOID STDCALL I8042QueueKeyboardPacket(PVOID Context)
{
	PDEVICE_OBJECT DeviceObject = Context;
	PFDO_DEVICE_EXTENSION FdoDevExt = DeviceObject->DeviceExtension;
	PDEVICE_EXTENSION DevExt = FdoDevExt->PortDevExt;

	DevExt->KeyComplete = TRUE;
	DevExt->KeysInBuffer++;
	if (DevExt->KeysInBuffer >
	                 DevExt->KeyboardAttributes.InputDataQueueLength) {
		DPRINT1("Keyboard buffer overflow\n");
		DevExt->KeysInBuffer--;
	}

	DPRINT("Irq completes key\n");
	KeInsertQueueDpc(&DevExt->DpcKbd, DevExt, NULL);
}

/*
 * These functions are callbacks for filter driver custom
 * initialization routines.
 */
NTSTATUS STDCALL I8042SynchWritePortKbd(PVOID Context,
                                        UCHAR Value,
                                        BOOLEAN WaitForAck)
{
	return I8042SynchWritePort((PDEVICE_EXTENSION)Context,
	                           0,
	                           Value,
	                           WaitForAck);
}

BOOLEAN STDCALL I8042InterruptServiceKbd(struct _KINTERRUPT *Interrupt,
                                             VOID * Context)
{
	UCHAR Output;
	UCHAR PortStatus;
	NTSTATUS Status;
	PDEVICE_EXTENSION DevExt = (PDEVICE_EXTENSION) Context;
	BOOLEAN HookContinue = FALSE, HookReturn;
	ULONG Iterations = 0;

	KEYBOARD_INPUT_DATA *InputData =
	                         DevExt->KeyboardBuffer + DevExt->KeysInBuffer;

	do {
		Status = I8042ReadStatus(&PortStatus);
		DPRINT("PortStatus: %x\n", PortStatus);
		Status = I8042ReadData(&Output);
		Iterations++;
		if (STATUS_SUCCESS == Status)
			break;
		KeStallExecutionProcessor(1);
	} while (Iterations < DevExt->Settings.PollStatusIterations);

	if (STATUS_SUCCESS != Status) {
		DPRINT("Spurious I8042 interrupt\n");
		return FALSE;
	}

	DPRINT("Got: %x\n", Output);

	if (DevExt->KeyboardHook.IsrRoutine) {
		HookReturn = DevExt->KeyboardHook.IsrRoutine(
                                             DevExt->KeyboardHook.Context,
		                             InputData,
		                             &DevExt->Packet,
		                             PortStatus,
		                             &Output,
		                             &HookContinue,
		                             &DevExt->KeyboardScanState);

		if (!HookContinue)
			return HookReturn;
	}

	if (I8042PacketIsr(DevExt, Output)) {
		if (DevExt->PacketComplete) {
			DPRINT("Packet complete\n");
			KeInsertQueueDpc(&DevExt->DpcKbd, DevExt, NULL);
		}
		DPRINT("Irq eaten by packet\n");
		return TRUE;
	}

	DPRINT("Irq is keyboard input\n");

	if (Normal == DevExt->KeyboardScanState) {
		switch (Output) {
		case 0xe0:
			DevExt->KeyboardScanState = GotE0;
			return TRUE;
		case 0xe1:
			DevExt->KeyboardScanState = GotE1;
			return TRUE;
		default:
			;/* continue */
		}
	}

	InputData->Flags = 0;

	switch (DevExt->KeyboardScanState) {
	case GotE0:
		InputData->Flags |= KEY_E0;
		break;
	case GotE1:
		InputData->Flags |= KEY_E1;
		break;
	default:
		;
	}
	DevExt->KeyboardScanState = Normal;

	if (Output & 0x80)
		InputData->Flags |= KEY_BREAK;
	else
		InputData->Flags |= KEY_MAKE;

	InputData->MakeCode = Output & 0x7f;

	I8042QueueKeyboardPacket(DevExt->KeyboardObject);

	return TRUE;
}

VOID STDCALL I8042DpcRoutineKbd(PKDPC Dpc,
                                PVOID DeferredContext,
                                PVOID SystemArgument1,
                                PVOID SystemArgument2)
{
	PDEVICE_EXTENSION DevExt = (PDEVICE_EXTENSION)SystemArgument1;
	ULONG KeysTransferred = 0;
	ULONG KeysInBufferCopy;
	KIRQL Irql;

	I8042PacketDpc(DevExt);

	if (!DevExt->KeyComplete)
		return;

	/* We got the interrupt as it was being enabled, too bad */
	if (!DevExt->HighestDIRQLInterrupt)
		return;

	Irql = KeAcquireInterruptSpinLock(DevExt->HighestDIRQLInterrupt);

	DevExt->KeyComplete = FALSE;
	KeysInBufferCopy = DevExt->KeysInBuffer;

	KeReleaseInterruptSpinLock(DevExt->HighestDIRQLInterrupt, Irql);

	/* Test for TAB (debugging) */
	if (DevExt->Settings.CrashSysRq) {
		PKEYBOARD_INPUT_DATA InputData = DevExt->KeyboardBuffer +
		                                          KeysInBufferCopy - 1;
		if (InputData->MakeCode == 0x0F) {
			DPRINT("Tab!\n");
			DevExt->TabPressed = !(InputData->Flags & KEY_BREAK);
		} else if (DevExt->TabPressed) {
			DPRINT ("Queueing work item %x\n", DevExt->DebugWorkItem);
			DevExt->DebugKey = InputData->MakeCode;
			DevExt->TabPressed = FALSE;

			IoQueueWorkItem(DevExt->DebugWorkItem,
			                &(I8042DebugWorkItem),
					DelayedWorkQueue,
					DevExt);
		}
	}

	DPRINT ("Send a key\n");

	if (!DevExt->KeyboardData.ClassService)
		return;

	((PSERVICE_CALLBACK_ROUTINE) DevExt->KeyboardData.ClassService)(
		                 DevExt->KeyboardData.ClassDeviceObject,
		                 DevExt->KeyboardBuffer,
		                 DevExt->KeyboardBuffer + KeysInBufferCopy,
		                 &KeysTransferred);

	Irql = KeAcquireInterruptSpinLock(DevExt->HighestDIRQLInterrupt);
	DevExt->KeysInBuffer -= KeysTransferred;
	KeReleaseInterruptSpinLock(DevExt->HighestDIRQLInterrupt, Irql);
}

/* You have to send the rate/delay in a somewhat awkward format */
static UCHAR I8042GetTypematicByte(USHORT Rate, USHORT Delay)
{
	UCHAR ret;

	if (Rate < 3) {
		ret = 0x0;
	} else if (Rate > 26) {
		ret = 0x1F;
	} else {
		ret = TypematicTable[Rate];
	}

	if (Delay < 375) {
		;
	} else if (Delay < 625) {
		ret |= 0x20;
	} else if (Delay < 875) {
		ret |= 0x40;
	} else {
		ret |= 0x60;
	}
	return ret;
}
/*
 * Process the keyboard internal device requests
 * returns FALSE if it doesn't understand the
 * call so someone else can handle it.
 */
BOOLEAN STDCALL I8042StartIoKbd(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PIO_STACK_LOCATION Stk;
	PFDO_DEVICE_EXTENSION FdoDevExt = DeviceObject->DeviceExtension;
	PDEVICE_EXTENSION DevExt = FdoDevExt->PortDevExt;

	Stk = IoGetCurrentIrpStackLocation(Irp);

	switch (Stk->Parameters.DeviceIoControl.IoControlCode) {
	case IOCTL_INTERNAL_I8042_KEYBOARD_WRITE_BUFFER:
		I8042StartPacket(
		             DevExt,
			     DeviceObject,
		             Stk->Parameters.DeviceIoControl.Type3InputBuffer,
		             Stk->Parameters.DeviceIoControl.InputBufferLength,
			     Irp);
		break;

	case IOCTL_KEYBOARD_SET_INDICATORS:
		DevExt->PacketBuffer[0] = 0xED;
		DevExt->PacketBuffer[1] = 0;
		if (DevExt->KeyboardIndicators.LedFlags & KEYBOARD_CAPS_LOCK_ON)
			DevExt->PacketBuffer[1] |= 0x04;

		if (DevExt->KeyboardIndicators.LedFlags & KEYBOARD_NUM_LOCK_ON)
			DevExt->PacketBuffer[1] |= 0x02;

		if (DevExt->KeyboardIndicators.LedFlags & KEYBOARD_SCROLL_LOCK_ON)
			DevExt->PacketBuffer[1] |= 0x01;

		I8042StartPacket(DevExt,
		                 DeviceObject,
		                 DevExt->PacketBuffer,
		                 2,
		                 Irp);
		break;
	case IOCTL_KEYBOARD_SET_TYPEMATIC:
		DevExt->PacketBuffer[0] = 0xF3;
		DevExt->PacketBuffer[1] = I8042GetTypematicByte(
				         DevExt->KeyboardTypematic.Rate,
				         DevExt->KeyboardTypematic.Delay);

		I8042StartPacket(DevExt,
		                 DeviceObject,
		                 DevExt->PacketBuffer,
		                 2,
		                 Irp);
		break;
	default:
		return FALSE;
	}

	return TRUE;
}

/*
 * Runs the keyboard IOCTL_INTERNAL dispatch.
 * Returns NTSTATUS_INVALID_DEVICE_REQUEST if it doesn't handle this request
 * so someone else can have a try at it.
 */
NTSTATUS STDCALL I8042InternalDeviceControlKbd(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PIO_STACK_LOCATION Stk;
	PFDO_DEVICE_EXTENSION FdoDevExt = DeviceObject->DeviceExtension;
	PDEVICE_EXTENSION DevExt = FdoDevExt->PortDevExt;

	DPRINT("InternalDeviceControl\n");

	Irp->IoStatus.Information = 0;
	Stk = IoGetCurrentIrpStackLocation(Irp);

	switch (Stk->Parameters.DeviceIoControl.IoControlCode) {

	case IOCTL_INTERNAL_KEYBOARD_CONNECT:
		DPRINT("IOCTL_INTERNAL_KEYBOARD_CONNECT\n");
		if (Stk->Parameters.DeviceIoControl.InputBufferLength <
		                                      sizeof(CONNECT_DATA)) {
			DPRINT1("Keyboard IOCTL_INTERNAL_KEYBOARD_CONNECT "
			       "invalid buffer size\n");
			Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
			goto intcontfailure;
		}

		if (!DevExt->KeyboardExists) {
			Irp->IoStatus.Status = STATUS_DEVICE_NOT_CONNECTED;
			goto intcontfailure;
		}

		if (DevExt->KeyboardClaimed) {
			DPRINT1("IOCTL_INTERNAL_KEYBOARD_CONNECT: "
			       "Keyboard is already claimed\n");
			Irp->IoStatus.Status = STATUS_SHARING_VIOLATION;
			goto intcontfailure;
		}

		memcpy(&DevExt->KeyboardData,
		       Stk->Parameters.DeviceIoControl.Type3InputBuffer,
		       sizeof(CONNECT_DATA));
		DevExt->KeyboardHook.IsrWritePort = I8042IsrWritePortKbd;
		DevExt->KeyboardHook.QueueKeyboardPacket =
		                                    I8042QueueKeyboardPacket;
		DevExt->KeyboardHook.CallContext = DevExt;

		{
			PIO_WORKITEM WorkItem;
			PI8042_HOOK_WORKITEM WorkItemData;

			WorkItem = IoAllocateWorkItem(DeviceObject);
			if (!WorkItem) {
				DPRINT ("IOCTL_INTERNAL_KEYBOARD_CONNECT: "
				        "Can't allocate work item\n");
				Irp->IoStatus.Status =
				              STATUS_INSUFFICIENT_RESOURCES;
				goto intcontfailure;
			}

			WorkItemData = ExAllocatePoolWithTag(
			                           NonPagedPool,
						   sizeof(I8042_HOOK_WORKITEM),
			                           TAG_I8042);
			if (!WorkItemData) {
				DPRINT ("IOCTL_INTERNAL_KEYBOARD_CONNECT: "
				        "Can't allocate work item data\n");
				Irp->IoStatus.Status =
				              STATUS_INSUFFICIENT_RESOURCES;
				IoFreeWorkItem(WorkItem);
				goto intcontfailure;
			}
			WorkItemData->WorkItem = WorkItem;
			WorkItemData->Target =
				        DevExt->KeyboardData.ClassDeviceObject;
			WorkItemData->Irp = Irp;

			IoMarkIrpPending(Irp);
			IoQueueWorkItem(WorkItem,
			                I8042SendHookWorkItem,
					DelayedWorkQueue,
					WorkItemData);

			Irp->IoStatus.Status = STATUS_PENDING;
		}

		break;
	case IOCTL_INTERNAL_I8042_KEYBOARD_WRITE_BUFFER:
		DPRINT("IOCTL_INTERNAL_I8042_KEYBOARD_WRITE_BUFFER\n");
		if (Stk->Parameters.DeviceIoControl.InputBufferLength < 1) {
			Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
			goto intcontfailure;
		}
		if (!DevExt->KeyboardInterruptObject) {
			Irp->IoStatus.Status = STATUS_DEVICE_NOT_READY;
			goto intcontfailure;
		}

		IoMarkIrpPending(Irp);
		IoStartPacket(DeviceObject, Irp, NULL, NULL);
		Irp->IoStatus.Status = STATUS_PENDING;

		break;
	case IOCTL_KEYBOARD_QUERY_ATTRIBUTES:
		DPRINT("IOCTL_KEYBOARD_QUERY_ATTRIBUTES\n");
		if (Stk->Parameters.DeviceIoControl.OutputBufferLength <
		                                 sizeof(KEYBOARD_ATTRIBUTES)) {
			DPRINT("Keyboard IOCTL_KEYBOARD_QUERY_ATTRIBUTES "
			       "invalid buffer size\n");
			Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
			goto intcontfailure;
		}
		memcpy(Irp->AssociatedIrp.SystemBuffer,
		       &DevExt->KeyboardAttributes,
		       sizeof(KEYBOARD_ATTRIBUTES));

		Irp->IoStatus.Status = STATUS_SUCCESS;
		break;
	case IOCTL_KEYBOARD_QUERY_INDICATORS:
		DPRINT("IOCTL_KEYBOARD_QUERY_INDICATORS\n");
		if (Stk->Parameters.DeviceIoControl.OutputBufferLength <
		                       sizeof(KEYBOARD_INDICATOR_PARAMETERS)) {
			DPRINT("Keyboard IOCTL_KEYBOARD_QUERY_INDICATORS "
			       "invalid buffer size\n");
			Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
			goto intcontfailure;
		}
		memcpy(Irp->AssociatedIrp.SystemBuffer,
		       &DevExt->KeyboardIndicators,
		       sizeof(KEYBOARD_INDICATOR_PARAMETERS));

		Irp->IoStatus.Status = STATUS_SUCCESS;
		break;
	case IOCTL_KEYBOARD_QUERY_TYPEMATIC:
		DPRINT("IOCTL_KEYBOARD_QUERY_TYPEMATIC\n");
		if (Stk->Parameters.DeviceIoControl.OutputBufferLength <
		                       sizeof(KEYBOARD_TYPEMATIC_PARAMETERS)) {
			DPRINT("Keyboard IOCTL_KEYBOARD_QUERY_TYPEMATIC "
			       "invalid buffer size\n");
			Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
			goto intcontfailure;
		}
		memcpy(Irp->AssociatedIrp.SystemBuffer,
		       &DevExt->KeyboardTypematic,
		       sizeof(KEYBOARD_TYPEMATIC_PARAMETERS));

		Irp->IoStatus.Status = STATUS_SUCCESS;
		break;
	case IOCTL_KEYBOARD_SET_INDICATORS:
		DPRINT("IOCTL_KEYBOARD_SET_INDICATORS\n");
		if (Stk->Parameters.DeviceIoControl.InputBufferLength <
		                       sizeof(KEYBOARD_INDICATOR_PARAMETERS)) {
			DPRINT("Keyboard IOCTL_KEYBOARD_SET_INDICTATORS "
			       "invalid buffer size\n");
			Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
			goto intcontfailure;
		}

		memcpy(&DevExt->KeyboardIndicators,
		       Irp->AssociatedIrp.SystemBuffer,
		       sizeof(KEYBOARD_INDICATOR_PARAMETERS));

		DPRINT("%x\n", DevExt->KeyboardIndicators.LedFlags);

		IoMarkIrpPending(Irp);
		IoStartPacket(DeviceObject, Irp, NULL, NULL);
		Irp->IoStatus.Status = STATUS_PENDING;

		break;
	case IOCTL_KEYBOARD_SET_TYPEMATIC:
		DPRINT("IOCTL_KEYBOARD_SET_TYPEMATIC\n");
		if (Stk->Parameters.DeviceIoControl.InputBufferLength <
		                       sizeof(KEYBOARD_TYPEMATIC_PARAMETERS)) {
			DPRINT("Keyboard IOCTL_KEYBOARD_SET_TYPEMATIC "
			       "invalid buffer size\n");
			Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
			goto intcontfailure;
		}

		memcpy(&DevExt->KeyboardTypematic,
		       Irp->AssociatedIrp.SystemBuffer,
		       sizeof(KEYBOARD_TYPEMATIC_PARAMETERS));

		IoMarkIrpPending(Irp);
		IoStartPacket(DeviceObject, Irp, NULL, NULL);
		Irp->IoStatus.Status = STATUS_PENDING;

		break;
	case IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION:
		/* We should check the UnitID, but it's kind of pointless as
		 * all keyboards are supposed to have the same one
		 */
		if (Stk->Parameters.DeviceIoControl.OutputBufferLength <
		                 sizeof(LOCAL_KEYBOARD_INDICATOR_TRANSLATION)) {
			DPRINT("IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION: "
			       "invalid buffer size (expected)\n");
			/* It's to query the buffer size */
			Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
			goto intcontfailure;
		}
		Irp->IoStatus.Information =
		                 sizeof(LOCAL_KEYBOARD_INDICATOR_TRANSLATION);

		memcpy(Irp->AssociatedIrp.SystemBuffer,
		       &IndicatorTranslation,
		       sizeof(LOCAL_KEYBOARD_INDICATOR_TRANSLATION));

		Irp->IoStatus.Status = STATUS_SUCCESS;
		break;
	case IOCTL_INTERNAL_I8042_HOOK_KEYBOARD:
		/* Nothing to do here */
		Irp->IoStatus.Status = STATUS_SUCCESS;
		break;
	default:
		Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

intcontfailure:
	return Irp->IoStatus.Status;
}

/* This is all pretty confusing. There's more than one way to
 * disable/enable the keyboard. You can send KBD_ENABLE to the
 * keyboard, and it will start scanning keys. Sending KBD_DISABLE
 * will disable the key scanning but also reset the parameters to
 * defaults.
 *
 * You can also send 0xAE to the controller for enabling the
 * keyboard clock line and 0xAD for disabling it. Then it'll
 * automatically get turned on at the next command. The last
 * way is by modifying the bit that drives the clock line in the
 * 'command byte' of the controller. This is almost, but not quite,
 * the same as the AE/AD thing. The difference can be used to detect
 * some really old broken keyboard controllers which I hope won't be
 * necessary.
 *
 * We change the command byte, sending KBD_ENABLE/DISABLE seems to confuse
 * some kvm switches.
 */

BOOLEAN STDCALL I8042KeyboardEnable(PDEVICE_EXTENSION DevExt)
{
	UCHAR Value;
	NTSTATUS Status;

	DPRINT("Enable keyboard\n");

	if (!I8042Write(DevExt, I8042_CTRL_PORT, KBD_READ_MODE)) {
		DPRINT1("Can't read i8042 mode\n");
		return FALSE;
	}

	Status = I8042ReadDataWait(DevExt, &Value);
	if (!NT_SUCCESS(Status)) {
		DPRINT1("No response after read i8042 mode\n");
		return FALSE;
	}

	Value &= ~CCB_KBD_DISAB; // don't disable keyboard

	if (!I8042Write(DevExt, I8042_CTRL_PORT, KBD_WRITE_MODE)) {
		DPRINT1("Can't set i8042 mode\n");
		return FALSE;
	}

	if (!I8042Write(DevExt, I8042_DATA_PORT, Value)) {
		DPRINT1("Can't send i8042 mode\n");
		return FALSE;
	}

	return TRUE;
}

static BOOLEAN STDCALL I8042KeyboardDefaultsAndDisable(PDEVICE_EXTENSION DevExt)
{
	UCHAR Value;
	NTSTATUS Status;

	DPRINT("Disabling keyboard\n");

	if (!I8042Write(DevExt, I8042_CTRL_PORT, KBD_READ_MODE)) {
		DPRINT1("Can't read i8042 mode\n");
		return FALSE;
	}

	Status = I8042ReadDataWait(DevExt, &Value);
	if (!NT_SUCCESS(Status)) {
		DPRINT1("No response after read i8042 mode\n");
		return FALSE;
	}

	Value |= CCB_KBD_DISAB; // disable keyboard

	if (!I8042Write(DevExt, I8042_CTRL_PORT, KBD_WRITE_MODE)) {
		DPRINT1("Can't set i8042 mode\n");
		return FALSE;
	}

	if (!I8042Write(DevExt, I8042_DATA_PORT, Value)) {
		DPRINT1("Can't send i8042 mode\n");
		return FALSE;
	}

	return TRUE;
}

BOOLEAN STDCALL I8042KeyboardEnableInterrupt(PDEVICE_EXTENSION DevExt)
{
	UCHAR Value;
	NTSTATUS Status;

	DPRINT("Enabling keyboard interrupt\n");

	if (!I8042Write(DevExt, I8042_CTRL_PORT, KBD_READ_MODE)) {
		DPRINT1("Can't read i8042 mode\n");
		return FALSE;
	}

	Status = I8042ReadDataWait(DevExt, &Value);
	if (!NT_SUCCESS(Status)) {
		DPRINT1("No response after read i8042 mode\n");
		return FALSE;
	}

	Value &= ~CCB_KBD_DISAB; // don't disable keyboard
	Value |= CCB_KBD_INT_ENAB;    // enable keyboard interrupts

	if (!I8042Write(DevExt, I8042_CTRL_PORT, KBD_WRITE_MODE)) {
		DPRINT1("Can't set i8042 mode\n");
		return FALSE;
	}

	if (!I8042Write(DevExt, I8042_DATA_PORT, Value)) {
		DPRINT1("Can't send i8042 mode\n");
		return FALSE;
	}

	return TRUE;
}

BOOLEAN STDCALL I8042DetectKeyboard(PDEVICE_EXTENSION DevExt)
{
	NTSTATUS Status;
	UCHAR Value;
	UCHAR Value2;
	ULONG RetryCount = 10;

	DPRINT("Detecting keyboard\n");

	I8042KeyboardDefaultsAndDisable(DevExt);

	do {
		I8042Flush();
		Status = I8042SynchWritePort(DevExt, 0, KBD_GET_ID, TRUE);
	} while (STATUS_IO_TIMEOUT == Status && RetryCount--);

	if (!NT_SUCCESS(Status)) {
		DPRINT1("Can't write GET_ID (%x)\n", Status);
		/* Could be an AT keyboard */
		DevExt->KeyboardIsAT = TRUE;
		goto detectsetleds;
	}

	Status = I8042ReadDataWait(DevExt, &Value);
	if (!NT_SUCCESS(Status)) {
		DPRINT1("No response after GET_ID\n");
		/* Could be an AT keyboard */
		DevExt->KeyboardIsAT = TRUE;
		goto detectsetleds;
	}
	DevExt->KeyboardIsAT = FALSE;

	if (Value != 0xAB && Value != 0xAC) {
		DPRINT("Bad ID: %x\n", Value);
		/* This is certainly not a keyboard */
		return FALSE;
	}

	Status = I8042ReadDataWait(DevExt, &Value2);
	if (!NT_SUCCESS(Status)) {
		DPRINT("Partial ID\n");
		return FALSE;
	}

	DPRINT("Keyboard ID: 0x%x 0x%x\n", Value, Value2);

detectsetleds:
	I8042Flush(); /* Flush any bytes left over from GET_ID */

	Status = I8042SynchWritePort(DevExt, 0, KBD_SET_LEDS, TRUE);
	if (!NT_SUCCESS(Status)) {
		DPRINT("Can't write SET_LEDS (%x)\n", Status);
		return FALSE;
	}
	Status = I8042SynchWritePort(DevExt, 0, 0, TRUE);
	if (!NT_SUCCESS(Status)) {
		DPRINT("Can't finish SET_LEDS (%x)\n", Status);
		return FALSE;
	}

	// Turn on translation

	if (!I8042Write(DevExt, I8042_CTRL_PORT, KBD_READ_MODE)) {
		DPRINT1("Can't read i8042 mode\n");
		return FALSE;
	}

	Status = I8042ReadDataWait(DevExt, &Value);
	if (!NT_SUCCESS(Status)) {
		DPRINT1("No response after read i8042 mode\n");
		return FALSE;
	}

	Value |= 0x40;    // enable keyboard translation

	if (!I8042Write(DevExt, I8042_CTRL_PORT, KBD_WRITE_MODE)) {
		DPRINT1("Can't set i8042 mode\n");
		return FALSE;
	}

	if (!I8042Write(DevExt, I8042_DATA_PORT, Value)) {
		DPRINT1("Can't send i8042 mode\n");
		return FALSE;
	}

	return TRUE;
}

/* debug stuff */
static VOID STDCALL I8042DebugWorkItem(PDEVICE_OBJECT DeviceObject,
                                       PVOID Context)
{
	ULONG Key;
	PFDO_DEVICE_EXTENSION FdoDevExt = DeviceObject->DeviceExtension;
	PDEVICE_EXTENSION DevExt = FdoDevExt->PortDevExt;

	Key = InterlockedExchange((PLONG)&DevExt->DebugKey, 0);
	DPRINT("Debug key: %x\n", Key);

	if (!Key)
		return;

#ifdef __REACTOS__
	/* We hope kernel would understand this. If
	 * that's not the case, nothing would happen.
	 */
    KdSystemDebugControl(TAG('R', 'o', 's', ' '), (PVOID)Key, 0, NULL, 0, NULL, KernelMode);
#endif /* __REACTOS__ */
}
