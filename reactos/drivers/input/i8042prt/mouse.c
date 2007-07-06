/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/input/i8042prt/mouse.c
 * PURPOSE:          i8042 (ps/2 keyboard-mouse controller) driver
 *                   mouse specifics
 * PROGRAMMER:       Victor Kirhenshtein (sauros@iname.com)
 *                   Jason Filby (jasonfilby@yahoo.com)
 *                   Tinus
 */

/* INCLUDES ****************************************************************/

#include "i8042prt.h"

#ifndef NDEBUG
#define NDEBUG
#endif
#include <debug.h>

/*
 * These functions are callbacks for filter driver custom interrupt
 * service routines.
 */
static VOID STDCALL I8042IsrWritePortMouse(PVOID Context,
                                           UCHAR Value)
{
	I8042IsrWritePort(Context, Value, 0xD4);
}

#if 0
static NTSTATUS STDCALL I8042SynchWritePortMouse(PVOID Context,
                                                 UCHAR Value,
                                                 BOOLEAN WaitForAck)
{
	return I8042SynchWritePort((PDEVICE_EXTENSION)Context,
	                           0xD4,
	                           Value,
	                           WaitForAck);
}
#endif

/* Test if packets are taking too long to come in. If they do, we
 * might have gotten out of sync and should just drop what we have.
 *
 * If we want to be totally right, we'd also have to keep a count of
 * errors, and totally reset the mouse after too much of them (can
 * happen if the user is using a KVM switch and an OS on another port
 * resets the mouse, or if the user hotplugs the mouse, or if we're just
 * generally unlucky). Also note the input parsing routine where we
 * drop invalid input packets.
 */
static VOID STDCALL I8042MouseInputTestTimeout(PDEVICE_EXTENSION DevExt)
{
	ULARGE_INTEGER Now;

	if (DevExt->MouseState == MouseExpectingACK ||
	    DevExt->MouseState == MouseResetting)
		return;

	Now.QuadPart = KeQueryInterruptTime();

	if (DevExt->MouseState != MouseIdle) {
		/* Check if the last byte came too long ago */
		if (Now.QuadPart - DevExt->MousePacketStartTime.QuadPart >
		                           DevExt->Settings.MouseSynchIn100ns) {
			DPRINT("Mouse input packet timeout\n");
			DevExt->MouseState = MouseIdle;
		}
	}

	if (DevExt->MouseState == MouseIdle)
		DevExt->MousePacketStartTime.QuadPart = Now.QuadPart;
}

/*
 * Call the customization hook. The Ret2 parameter is about wether
 * we should go on with the interrupt. The return value is what
 * we should return (indicating to the system wether someone else
 * should try to handle the interrupt)
 */
static BOOLEAN STDCALL I8042MouseCallIsrHook(PDEVICE_EXTENSION DevExt,
                                             UCHAR Status,
                                             PUCHAR Input,
				                             PBOOLEAN ToReturn)
{
	BOOLEAN HookReturn, HookContinue;

	HookContinue = FALSE;

	if (DevExt->MouseHook.IsrRoutine) {
		HookReturn = DevExt->MouseHook.IsrRoutine(
		                   DevExt->MouseHook.Context,
		                   DevExt->MouseBuffer + DevExt->MouseInBuffer,
		                   &DevExt->Packet,
		                   Status,
		                   Input,
		                   &HookContinue,
		                   &DevExt->MouseState,
		                   &DevExt->MouseResetState);

		if (!HookContinue) {
			*ToReturn = HookReturn;
			return TRUE;
		}
	}
	return FALSE;
}

static BOOLEAN STDCALL I8042MouseResetIsr(PDEVICE_EXTENSION DevExt,
                                          UCHAR Status,
                                          PUCHAR Value)
{
	BOOLEAN ToReturn = FALSE;

	if (I8042MouseCallIsrHook(DevExt, Status, Value, &ToReturn))
		return ToReturn;

	if (MouseResetting != DevExt->MouseState) {
		return FALSE;
	}
	DevExt->MouseTimeoutState = TimeoutStart;

	switch (DevExt->MouseResetState) {
	case 1100: /* the first ack, drop it. */
		DevExt->MouseResetState = ExpectingReset;
		return TRUE;
	/* First, 0xFF is sent. The mouse is supposed to say AA00 if ok,
	 * FC00 if not.
	 */
	case ExpectingReset:
		if (0xAA == *Value) {
			DevExt->MouseResetState++;
		} else {
			DevExt->MouseExists = FALSE;
			DevExt->MouseState = MouseIdle;
			DPRINT("Mouse returned bad reset reply: "
			       "%x (expected aa)\n", *Value);
		}
		return TRUE;
	case ExpectingResetId:
		if (0x00 == *Value) {
			DevExt->MouseResetState++;
			DevExt->MouseType = GenericPS2;
			I8042IsrWritePortMouse(DevExt, 0xF2);
		} else {
			DevExt->MouseExists = FALSE;
			DevExt->MouseState = MouseIdle;
			DPRINT1("Mouse returned bad reset reply part two: "
			        "%x (expected 0)\n", *Value);
		}
		return TRUE;
	case ExpectingGetDeviceIdACK:
		if (MOUSE_ACK == *Value) {
			DevExt->MouseResetState++;
		} else if (MOUSE_NACK == *Value ||
		           MOUSE_ERROR == *Value) {
			DevExt->MouseResetState++;
			/* Act as if 00 (normal mouse) was received */
			DPRINT("Mouse doesn't support 0xd2, "
			       "(returns %x, expected %x), faking.\n",
			       *Value, MOUSE_ACK);
			*Value = 0;
			I8042MouseResetIsr(DevExt, Status, Value);
		}
		return TRUE;
	case ExpectingGetDeviceIdValue:
		switch (*Value) {
		case 0x02:
			DevExt->MouseAttributes.MouseIdentifier =
			                            BALLPOINT_I8042_HARDWARE;
			break;
		case 0x03:
		case 0x04:
			DevExt->MouseAttributes.MouseIdentifier =
			                            WHEELMOUSE_I8042_HARDWARE;
			break;
		default:
			DevExt->MouseAttributes.MouseIdentifier =
			                            MOUSE_I8042_HARDWARE;
		}
		DevExt->MouseResetState++;
		I8042IsrWritePortMouse(DevExt, 0xE8);
		return TRUE;
	case ExpectingSetResolutionDefaultACK:
		DevExt->MouseResetState++;
		I8042IsrWritePortMouse(DevExt, 0x00);
		return TRUE;
	case ExpectingSetResolutionDefaultValueACK:
		DevExt->MouseResetState = ExpectingSetScaling1to1ACK;
		I8042IsrWritePortMouse(DevExt, 0xE6);
		return TRUE;
	case ExpectingSetScaling1to1ACK:
	case ExpectingSetScaling1to1ACK2:
		DevExt->MouseResetState++;
		I8042IsrWritePortMouse(DevExt, 0xE6);
		return TRUE;
	case ExpectingSetScaling1to1ACK3:
		DevExt->MouseResetState++;
		I8042IsrWritePortMouse(DevExt, 0xE9);
		return TRUE;
	case ExpectingReadMouseStatusACK:
		DevExt->MouseResetState++;
		return TRUE;
	case ExpectingReadMouseStatusByte1:
		DevExt->MouseLogiBuffer[0] = *Value;
		DevExt->MouseResetState++;
		return TRUE;
	case ExpectingReadMouseStatusByte2:
		DevExt->MouseLogiBuffer[1] = *Value;
		DevExt->MouseResetState++;
		return TRUE;
	case ExpectingReadMouseStatusByte3:
		DevExt->MouseLogiBuffer[2] = *Value;
		/* Now MouseLogiBuffer is a set of info. If the second
		 * byte is 0, the mouse didn't understand the magic
		 * code. Otherwise, it it a Logitech and the second byte
		 * is the number of buttons, bit 7 of the first byte tells
		 * if it understands special E7 commands, the rest is an ID.
		 */
		if (DevExt->MouseLogiBuffer[1]) {
			DevExt->MouseAttributes.NumberOfButtons =
			                         DevExt->MouseLogiBuffer[1];
			/* For some reason the ID is the wrong way around */
			DevExt->MouseLogitechID =
			          ((DevExt->MouseLogiBuffer[0] >> 4) & 0x07) |
			          ((DevExt->MouseLogiBuffer[0] << 3) & 0x78);
			DevExt->MouseType = Ps2pp;
			I8042IsrWritePortMouse(DevExt, 0xf3);
			DevExt->MouseResetState = ExpectingSetSamplingRateACK;
			return TRUE;
			/* TODO: Go through EnableWheel and Enable5Buttons */
		}
		DevExt->MouseResetState = EnableWheel;
		I8042MouseResetIsr(DevExt, Status, Value);
		return TRUE;
	case EnableWheel:
		I8042IsrWritePortMouse(DevExt, 0xf3);
		DevExt->MouseResetState = 1001;
		return TRUE;
	case 1001:
		I8042IsrWritePortMouse(DevExt, 0xc8);
		DevExt->MouseResetState++;
		return TRUE;
	case 1002:
	case 1004:
		I8042IsrWritePortMouse(DevExt, 0xf3);
		DevExt->MouseResetState++;
		return TRUE;
	case 1003:
		I8042IsrWritePortMouse(DevExt, 0x64);
		DevExt->MouseResetState++;
		return TRUE;
	case 1005:
		I8042IsrWritePortMouse(DevExt, 0x50);
		DevExt->MouseResetState++;
		return TRUE;
	case 1006:
		I8042IsrWritePortMouse(DevExt, 0xf2);
		DevExt->MouseResetState++;
		return TRUE;
	case 1007:
		/* Ignore ACK */
		DevExt->MouseResetState++;
		return TRUE;
	case 1008:
		/* Now if the value == 3, it's either an Intellimouse
		 * or Intellimouse Explorer. */
		if (0x03 == *Value) {
			DevExt->MouseAttributes.NumberOfButtons = 3;
			DevExt->MouseAttributes.MouseIdentifier =
			                           WHEELMOUSE_I8042_HARDWARE;
			DevExt->MouseType = Intellimouse;
			DevExt->MouseResetState = Enable5Buttons;
			I8042MouseResetIsr(DevExt, Status, Value);
			return TRUE;
		} /* Else, just set the default settings and be done */
		I8042IsrWritePortMouse(DevExt, 0xf3);
		DevExt->MouseResetState = ExpectingSetSamplingRateACK;
		return TRUE;
	case Enable5Buttons:
		I8042IsrWritePortMouse(DevExt, 0xf3);
		DevExt->MouseResetState = 1021;
		return TRUE;
	case 1022:
	case 1024:
		I8042IsrWritePortMouse(DevExt, 0xf3);
		DevExt->MouseResetState++;
		return TRUE;
	case 1021:
	case 1023:
		I8042IsrWritePortMouse(DevExt, 0xc8);
		DevExt->MouseResetState++;
		return TRUE;
	case 1025:
		I8042IsrWritePortMouse(DevExt, 0x50);
		DevExt->MouseResetState++;
		return TRUE;
	case 1026:
		I8042IsrWritePortMouse(DevExt, 0xf2);
		DevExt->MouseResetState++;
		return TRUE;
	case 1027:
		if (0x04 == *Value) {
			DevExt->MouseAttributes.NumberOfButtons = 5;
			DevExt->MouseAttributes.MouseIdentifier =
			                           WHEELMOUSE_I8042_HARDWARE;
			DevExt->MouseType = IntellimouseExplorer;
		}
		I8042IsrWritePortMouse(DevExt, 0xf3);
		DevExt->MouseResetState = ExpectingSetSamplingRateACK;
		return TRUE;
	case ExpectingSetSamplingRateACK:
		I8042IsrWritePortMouse(DevExt,
		                       (UCHAR)DevExt->MouseAttributes.SampleRate);
		DevExt->MouseResetState++;
		return TRUE;
	case ExpectingSetSamplingRateValueACK:
		if (MOUSE_NACK == *Value) {
			I8042IsrWritePortMouse(DevExt, 0x3c);
			DevExt->MouseAttributes.SampleRate = 60;
			DevExt->MouseResetState = 1040;
			return TRUE;
		}
	case 1040:  /* Fallthrough */
		I8042IsrWritePortMouse(DevExt, 0xe8);
		DevExt->MouseResetState = ExpectingFinalResolutionACK;
		return TRUE;
	case ExpectingFinalResolutionACK:
		I8042IsrWritePortMouse(DevExt,
		                       (UCHAR)(DevExt->Settings.MouseResolution & 0xff));
		DPRINT("%x\n", DevExt->Settings.MouseResolution);
		DevExt->MouseResetState = ExpectingFinalResolutionValueACK;
		return TRUE;
	case ExpectingFinalResolutionValueACK:
		I8042IsrWritePortMouse(DevExt, 0xf4);
		DevExt->MouseResetState = ExpectingEnableACK;
		return TRUE;
	case ExpectingEnableACK:
		DevExt->MouseState = MouseIdle;
		DevExt->MouseTimeoutState = TimeoutCancel;
		DPRINT("Mouse type = %d\n", DevExt->MouseType);
		return TRUE;
	default:
		if (DevExt->MouseResetState < 100 ||
		    DevExt->MouseResetState > 999)
			DPRINT1("MouseResetState went out of range: %d\n",
			                 DevExt->MouseResetState);
		return FALSE;
	}
}

/*
 * Prepare for reading the next packet and queue the dpc for handling
 * this one.
 *
 * Context is the device object.
 */
VOID STDCALL I8042QueueMousePacket(PVOID Context)
{
	PDEVICE_OBJECT DeviceObject = Context;
	PFDO_DEVICE_EXTENSION FdoDevExt = DeviceObject->DeviceExtension;
	PDEVICE_EXTENSION DevExt = FdoDevExt->PortDevExt;

	DevExt->MouseComplete = TRUE;
	DevExt->MouseInBuffer++;
	if (DevExt->MouseInBuffer >
	                 DevExt->MouseAttributes.InputDataQueueLength) {
		DPRINT1("Mouse buffer overflow\n");
		DevExt->MouseInBuffer--;
	}

	DPRINT("Irq completes mouse packet\n");
	KeInsertQueueDpc(&DevExt->DpcMouse, DevExt, NULL);
}

/*
 * Updates ButtonFlags according to RawButtons and a saved state;
 * Only takes in account the bits that are set in Mask
 */
VOID STDCALL I8042MouseHandleButtons(PDEVICE_EXTENSION DevExt,
                                     USHORT Mask)
{
	PMOUSE_INPUT_DATA MouseInput = DevExt->MouseBuffer +
	                                         DevExt->MouseInBuffer;
	USHORT NewButtonData = (USHORT)(MouseInput->RawButtons & Mask);
	USHORT ButtonDiff = (NewButtonData ^ DevExt->MouseButtonState) & Mask;

	/* Note that the defines are such:
	 * MOUSE_LEFT_BUTTON_DOWN 1
	 * MOUSE_LEFT_BUTTON_UP   2
	 */
	MouseInput->ButtonFlags |= (NewButtonData & ButtonDiff) |
	                           (((~(NewButtonData)) << 1) &
	                                               (ButtonDiff << 1)) |
	                           (MouseInput->RawButtons & 0xfc00);

	DPRINT("Left raw/up/down: %d/%d/%d\n", MouseInput->RawButtons & MOUSE_LEFT_BUTTON_DOWN,
			MouseInput->ButtonFlags & MOUSE_LEFT_BUTTON_DOWN,
			MouseInput->ButtonFlags & MOUSE_LEFT_BUTTON_UP);

	DevExt->MouseButtonState = (DevExt->MouseButtonState & ~Mask) |
	                           (NewButtonData & Mask);
}

VOID STDCALL I8042MouseHandle(PDEVICE_EXTENSION DevExt,
                              UCHAR Output)
{
	PMOUSE_INPUT_DATA MouseInput = DevExt->MouseBuffer +
	                                         DevExt->MouseInBuffer;
	CHAR Scroll;

	switch (DevExt->MouseState) {
	case MouseIdle:
		/* This bit should be 1, if not drop the packet, we
		 * might be lucky and get in sync again
		 */
		if (!(Output & 8)) {
			DPRINT1("Bad input, dropping..\n");
			return;
		}

		MouseInput->Buttons = 0;
		MouseInput->RawButtons = 0;
		MouseInput->Flags = MOUSE_MOVE_RELATIVE;

		/* Note how we ignore the overflow bits, like Windows
		 * is said to do. There's no reasonable thing to do
		 * anyway.
		 */

		if (Output & 16)
			MouseInput->LastX = 1;
		else
			MouseInput->LastX = 0;

		if (Output & 32)
			MouseInput->LastY = 1;
		else
			MouseInput->LastY = 0;

		if (Output & 1) {
			MouseInput->RawButtons |= MOUSE_LEFT_BUTTON_DOWN;
		}

		if (Output & 2) {
			MouseInput->RawButtons |= MOUSE_RIGHT_BUTTON_DOWN;
		}

		if (Output & 4) {
			MouseInput->RawButtons |= MOUSE_MIDDLE_BUTTON_DOWN;
		}
		DevExt->MouseState = XMovement;
		break;
	case XMovement:
		if (MouseInput->LastX)
			MouseInput->LastX = (LONG) Output - 256;
		else
			MouseInput->LastX = Output;

		DevExt->MouseState = YMovement;
		break;
	case YMovement:
		if (MouseInput->LastY)
			MouseInput->LastY = (LONG)Output - 256;
		else
			MouseInput->LastY = (LONG)Output;

		/* Windows wants it the other way around */
		MouseInput->LastY = -MouseInput->LastY;

		if (DevExt->MouseType == GenericPS2 ||
		    DevExt->MouseType == Ps2pp) {
			I8042MouseHandleButtons(DevExt,
						MOUSE_LEFT_BUTTON_DOWN |
			                           MOUSE_RIGHT_BUTTON_DOWN |
			                           MOUSE_MIDDLE_BUTTON_DOWN);
			I8042QueueMousePacket(
					DevExt->MouseObject);
			DevExt->MouseState = MouseIdle;
		} else {
			DevExt->MouseState = ZMovement;
		}
		break;
	case ZMovement:
		Scroll = Output & 0x0f;
		if (Scroll & 8)
			Scroll |= 0xf0;

		if (Scroll) {
			MouseInput->RawButtons |= MOUSE_WHEEL;
			MouseInput->ButtonData = (USHORT)(Scroll * -WHEEL_DELTA);
		}

		if (DevExt->MouseType == IntellimouseExplorer) {
			if (Output & 16)
				MouseInput->RawButtons |= MOUSE_BUTTON_4_DOWN;

			if (Output & 32)
				MouseInput->RawButtons |= MOUSE_BUTTON_5_DOWN;
		}
		I8042MouseHandleButtons(DevExt, MOUSE_LEFT_BUTTON_DOWN |
		                                MOUSE_RIGHT_BUTTON_DOWN |
		                                MOUSE_MIDDLE_BUTTON_DOWN |
		                                MOUSE_BUTTON_4_DOWN |
		                                MOUSE_BUTTON_5_DOWN);
		I8042QueueMousePacket(DevExt->MouseObject);
		DevExt->MouseState = MouseIdle;
		break;
	default:
		DPRINT1("Unexpected state!\n");
	}
}

BOOLEAN STDCALL I8042InterruptServiceMouse(struct _KINTERRUPT *Interrupt,
                                           VOID *Context)
{
	UCHAR Output, PortStatus;
	NTSTATUS Status;
	PDEVICE_EXTENSION DevExt = (PDEVICE_EXTENSION) Context;
	ULONG Iterations = 0;

	do {
		Status = I8042ReadStatus(&PortStatus);
		Status = I8042ReadData(&Output);
		Iterations++;
		if (STATUS_SUCCESS == Status)
			break;
		KeStallExecutionProcessor(1);
	} while (Iterations < DevExt->Settings.PollStatusIterations);

	if (STATUS_SUCCESS != Status) {
		DPRINT1("Spurious I8042 mouse interrupt\n");
		return FALSE;
	}

	DPRINT("Got: %x\n", Output);

	if (I8042PacketIsr(DevExt, Output)) {
		if (DevExt->PacketComplete) {
			DPRINT("Packet complete\n");
			KeInsertQueueDpc(&DevExt->DpcKbd, DevExt, NULL);
		}
		DPRINT("Irq eaten by packet\n");
		return TRUE;
	}

	I8042MouseInputTestTimeout(DevExt);

	if (I8042MouseResetIsr(DevExt, PortStatus, &Output)) {
		DPRINT("Handled by ResetIsr or hooked Isr\n");
		if (NoChange != DevExt->MouseTimeoutState) {
			KeInsertQueueDpc(&DevExt->DpcMouse, DevExt, NULL);
		}
		return TRUE;
	}

	if (DevExt->MouseType == Ps2pp)
		I8042MouseHandlePs2pp(DevExt, Output);
	else
		I8042MouseHandle(DevExt, Output);

	return TRUE;
}

VOID STDCALL I8042DpcRoutineMouse(PKDPC Dpc,
                                  PVOID DeferredContext,
                                  PVOID SystemArgument1,
                                  PVOID SystemArgument2)
{
	PDEVICE_EXTENSION DevExt = (PDEVICE_EXTENSION)SystemArgument1;
	ULONG MouseTransferred = 0;
	ULONG MouseInBufferCopy;
	KIRQL Irql;
	LARGE_INTEGER Timeout;

	switch (DevExt->MouseTimeoutState) {
	case TimeoutStart:
		DevExt->MouseTimeoutState = NoChange;
		if (DevExt->MouseTimeoutActive &&
		    !KeCancelTimer(&DevExt->TimerMouseTimeout)) {
			/* The timer fired already, give up */
			DevExt->MouseTimeoutActive = FALSE;
			return;
		}

		Timeout.QuadPart = -15000000;
		                    /* 1.5 seconds, should be enough */

		KeSetTimer(&DevExt->TimerMouseTimeout,
		           Timeout,
		           &DevExt->DpcMouseTimeout);
		DevExt->MouseTimeoutActive = TRUE;
		return;
	case TimeoutCancel:
		DevExt->MouseTimeoutState = NoChange;
		KeCancelTimer(&DevExt->TimerMouseTimeout);
		DevExt->MouseTimeoutActive = FALSE;
	default:
		/* nothing, don't want a warning */ ;
	}

	/* Should be unlikely */
	if (!DevExt->MouseComplete)
		return;

	Irql = KeAcquireInterruptSpinLock(DevExt->HighestDIRQLInterrupt);

	DevExt->MouseComplete = FALSE;
	MouseInBufferCopy = DevExt->MouseInBuffer;

	KeReleaseInterruptSpinLock(DevExt->HighestDIRQLInterrupt, Irql);

	DPRINT ("Send a mouse packet\n");

	if (!DevExt->MouseData.ClassService)
		return;

	((PSERVICE_CALLBACK_ROUTINE) DevExt->MouseData.ClassService)(
	                         DevExt->MouseData.ClassDeviceObject,
	                         DevExt->MouseBuffer,
	                         DevExt->MouseBuffer + MouseInBufferCopy,
	                         &MouseTransferred);

	Irql = KeAcquireInterruptSpinLock(DevExt->HighestDIRQLInterrupt);

	DevExt->MouseInBuffer -= MouseTransferred;
	if (DevExt->MouseInBuffer)
		RtlMoveMemory(DevExt->MouseBuffer,
		              DevExt->MouseBuffer+MouseTransferred,
		              DevExt->MouseInBuffer * sizeof(MOUSE_INPUT_DATA));

	KeReleaseInterruptSpinLock(DevExt->HighestDIRQLInterrupt, Irql);
}

/* This timer DPC will be called when the mouse reset times out.
 * I'll just send the 'disable mouse port' command to the controller
 * and say the mouse doesn't exist.
 */
VOID STDCALL I8042DpcRoutineMouseTimeout(PKDPC Dpc,
                                         PVOID DeferredContext,
                                         PVOID SystemArgument1,
                                         PVOID SystemArgument2)
{
	PDEVICE_EXTENSION DevExt = (PDEVICE_EXTENSION)DeferredContext;
	KIRQL Irql;

	Irql = KeAcquireInterruptSpinLock(DevExt->HighestDIRQLInterrupt);

	DPRINT1("Mouse initialization timeout! (substate %x) "
	                                        "Disabling mouse.\n",
		DevExt->MouseResetState);

	if (!I8042MouseDisable(DevExt)) {
		DPRINT1("Failed to disable mouse.\n");
	}

	DevExt->MouseExists = FALSE;

	KeReleaseInterruptSpinLock(DevExt->HighestDIRQLInterrupt, Irql);
}

/*
 * Process the mouse internal device requests
 * returns FALSE if it doesn't understand the
 * call so someone else can handle it.
 */
#if 0
static BOOLEAN STDCALL I8042StartIoMouse(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PIO_STACK_LOCATION Stk;
	PFDO_DEVICE_EXTENSION FdoDevExt = DeviceObject->DeviceExtension;
	PDEVICE_EXTENSION DevExt = FdoDevExt->PortDevExt;

	Stk = IoGetCurrentIrpStackLocation(Irp);

	switch (Stk->Parameters.DeviceIoControl.IoControlCode) {
	case IOCTL_INTERNAL_I8042_MOUSE_WRITE_BUFFER:
		I8042StartPacket(
		             DevExt,
			     DeviceObject,
		             Stk->Parameters.DeviceIoControl.Type3InputBuffer,
		             Stk->Parameters.DeviceIoControl.InputBufferLength,
			     Irp);
		break;

	default:
		return FALSE;
	}

	return TRUE;
}
#endif

/*
 * Runs the mouse IOCTL_INTERNAL dispatch.
 * Returns NTSTATUS_INVALID_DEVICE_REQUEST if it doesn't handle this request
 * so someone else can have a try at it.
 */
NTSTATUS STDCALL I8042InternalDeviceControlMouse(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PIO_STACK_LOCATION Stk;
	PFDO_DEVICE_EXTENSION FdoDevExt = DeviceObject->DeviceExtension;
	PDEVICE_EXTENSION DevExt = FdoDevExt->PortDevExt;

	DPRINT("InternalDeviceControl\n");

	Irp->IoStatus.Information = 0;
	Stk = IoGetCurrentIrpStackLocation(Irp);

	switch (Stk->Parameters.DeviceIoControl.IoControlCode) {

	case IOCTL_INTERNAL_MOUSE_CONNECT:
		DPRINT("IOCTL_INTERNAL_MOUSE_CONNECT\n");
		if (Stk->Parameters.DeviceIoControl.InputBufferLength <
		                                      sizeof(CONNECT_DATA)) {
			DPRINT1("Mouse IOCTL_INTERNAL_MOUSE_CONNECT "
			       "invalid buffer size\n");
			Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
			goto intcontfailure;
		}

		if (!DevExt->MouseExists) {
			Irp->IoStatus.Status = STATUS_DEVICE_NOT_CONNECTED;
			goto intcontfailure;
		}

		if (DevExt->MouseClaimed) {
			DPRINT1("IOCTL_INTERNAL_MOUSE_CONNECT: "
			       "Mouse is already claimed\n");
			Irp->IoStatus.Status = STATUS_SHARING_VIOLATION;
			goto intcontfailure;
		}

		memcpy(&DevExt->MouseData,
		       Stk->Parameters.DeviceIoControl.Type3InputBuffer,
		       sizeof(CONNECT_DATA));
		DevExt->MouseHook.IsrWritePort = I8042IsrWritePortMouse;
		DevExt->MouseHook.QueueMousePacket =
		                                    I8042QueueMousePacket;
		DevExt->MouseHook.CallContext = DevExt;

		{
			PIO_WORKITEM WorkItem;
			PI8042_HOOK_WORKITEM WorkItemData;

			WorkItem = IoAllocateWorkItem(DeviceObject);
			if (!WorkItem) {
				DPRINT ("IOCTL_INTERNAL_MOUSE_CONNECT: "
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
				DPRINT ("IOCTL_INTERNAL_MOUSE_CONNECT: "
				        "Can't allocate work item data\n");
				Irp->IoStatus.Status =
				              STATUS_INSUFFICIENT_RESOURCES;
				IoFreeWorkItem(WorkItem);
				goto intcontfailure;
			}
			WorkItemData->WorkItem = WorkItem;
			WorkItemData->Target =
				        DevExt->MouseData.ClassDeviceObject;
			WorkItemData->Irp = Irp;

			IoMarkIrpPending(Irp);
			DevExt->MouseState = MouseResetting;
			DevExt->MouseResetState = 1100;
			IoQueueWorkItem(WorkItem,
			                I8042SendHookWorkItem,
					DelayedWorkQueue,
					WorkItemData);

			Irp->IoStatus.Status = STATUS_PENDING;
		}

		break;
	case IOCTL_INTERNAL_I8042_MOUSE_WRITE_BUFFER:
		DPRINT("IOCTL_INTERNAL_I8042_MOUSE_WRITE_BUFFER\n");
		if (Stk->Parameters.DeviceIoControl.InputBufferLength < 1) {
			Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
			goto intcontfailure;
		}
		if (!DevExt->MouseInterruptObject) {
			Irp->IoStatus.Status = STATUS_DEVICE_NOT_READY;
			goto intcontfailure;
		}

		IoMarkIrpPending(Irp);
		IoStartPacket(DeviceObject, Irp, NULL, NULL);
		Irp->IoStatus.Status = STATUS_PENDING;

		break;
	case IOCTL_MOUSE_QUERY_ATTRIBUTES:
		DPRINT("IOCTL_MOUSE_QUERY_ATTRIBUTES\n");
		if (Stk->Parameters.DeviceIoControl.InputBufferLength <
		                                 sizeof(MOUSE_ATTRIBUTES)) {
			DPRINT("Mouse IOCTL_MOUSE_QUERY_ATTRIBUTES "
			       "invalid buffer size\n");
			Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
			goto intcontfailure;
		}
		memcpy(Irp->AssociatedIrp.SystemBuffer,
		       &DevExt->MouseAttributes,
		       sizeof(MOUSE_ATTRIBUTES));

		Irp->IoStatus.Status = STATUS_SUCCESS;
		break;
	default:
		Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

intcontfailure:
	return Irp->IoStatus.Status;
}

BOOLEAN STDCALL I8042MouseEnable(PDEVICE_EXTENSION DevExt)
{
	UCHAR Value;
	NTSTATUS Status;

	DPRINT("Enabling mouse\n");

	if (!I8042Write(DevExt, I8042_CTRL_PORT, KBD_READ_MODE)) {
		DPRINT1("Can't read i8042 mode\n");
		return FALSE;
	}

	Status = I8042ReadDataWait(DevExt, &Value);
	if (!NT_SUCCESS(Status)) {
		DPRINT1("No response after read i8042 mode\n");
		return FALSE;
	}

	Value &= ~(0x20); // don't disable mouse
	Value |= 0x02;    // enable mouse interrupts

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

BOOLEAN STDCALL I8042MouseDisable(PDEVICE_EXTENSION DevExt)
{
	UCHAR Value;
	NTSTATUS Status;

	DPRINT("Disabling mouse\n");

	I8042Flush(); /* Just to be (kind of) sure */

	if (!I8042Write(DevExt, I8042_CTRL_PORT, KBD_READ_MODE)) {
		DPRINT1("Can't read i8042 mode\n");
		return FALSE;
	}

	Status = I8042ReadDataWait(DevExt, &Value);
	if (!NT_SUCCESS(Status)) {
		DPRINT1("No response after read i8042 mode\n");
		return FALSE;
	}

	Value |= 0x20; // don't disable mouse
	Value &= ~(0x02);    // enable mouse interrupts

	if (!I8042Write(DevExt, I8042_CTRL_PORT, KBD_WRITE_MODE)) {
		DPRINT1("Can't set i8042 mode\n");
		return FALSE;
	}

	if (!I8042Write(DevExt, I8042_DATA_PORT, Value)) {
		DPRINT1("Can't send i8042 mode\n");
		return FALSE;
	}

	I8042Flush();
	/* Just to be (kind of) sure; if the mouse would
	 * say something while we are disabling it, these bytes would
	 * block the keyboard.
	 */

	return TRUE;
}

BOOLEAN STDCALL I8042DetectMouse(PDEVICE_EXTENSION DevExt)
{
	BOOLEAN Ok = TRUE;
	NTSTATUS Status;
	UCHAR Value;
	UCHAR ExpectedReply[] = { 0xFA, 0xAA, 0x00 };
	unsigned ReplyByte;
	ULONG Counter;

	I8042Flush();

	if (! I8042Write(DevExt, I8042_CTRL_PORT, 0xD4) ||
	    ! I8042Write(DevExt, I8042_DATA_PORT, 0xFF))
	{
		DPRINT1("Failed to write reset command to mouse\n");
		Ok = FALSE;
	}

	for (ReplyByte = 0;
	     ReplyByte < sizeof(ExpectedReply) / sizeof(ExpectedReply[0]) && Ok;
	     ReplyByte++)
	{
		Counter = 500;

		do {
			Status = I8042ReadDataWait(DevExt, &Value);
		} while (Status == STATUS_IO_TIMEOUT && Counter--);

		if (! NT_SUCCESS(Status))
		{
			DPRINT1("No ACK after mouse reset, status 0x%08x\n",
			        Status);
			Ok = FALSE;
		}
		else if (Value != ExpectedReply[ReplyByte])
		{
			DPRINT1("Unexpected reply: 0x%02x (expected 0x%02x)\n",
			        Value, ExpectedReply[ReplyByte]);
			Ok = FALSE;
		}
	}

	if (! Ok)
	{
		/* There is probably no mouse present. On some systems,
		   the probe locks the entire keyboard controller. Let's
		   try to get access to the keyboard again by sending a
		   reset */
		I8042Flush();
		I8042Write(DevExt, I8042_CTRL_PORT, KBD_SELF_TEST);
		I8042ReadDataWait(DevExt, &Value);
		I8042Flush();
	}
		
	DPRINT("Mouse %sdetected\n", Ok ? "" : "not ");

	return Ok;
}
