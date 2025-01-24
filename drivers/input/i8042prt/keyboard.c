/*
 * PROJECT:     ReactOS i8042 (ps/2 keyboard-mouse controller) driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/input/i8042prt/keyboard.c
 * PURPOSE:     Keyboard specific functions
 * PROGRAMMERS: Copyright Victor Kirhenshtein (sauros@iname.com)
                Copyright Jason Filby (jasonfilby@yahoo.com)
                Copyright Martijn Vernooij (o112w8r02@sneakemail.com)
                Copyright 2006-2007 Hervé Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES ****************************************************************/

#include "i8042prt.h"

#include <poclass.h>
#include <ndk/kdfuncs.h>

#include <debug.h>

/* GLOBALS *******************************************************************/

static IO_WORKITEM_ROUTINE i8042PowerWorkItem;
static KDEFERRED_ROUTINE i8042KbdDpcRoutine;

/* This structure starts with the same layout as KEYBOARD_INDICATOR_TRANSLATION */
typedef struct _LOCAL_KEYBOARD_INDICATOR_TRANSLATION {
	USHORT NumberOfIndicatorKeys;
	INDICATOR_LIST IndicatorList[3];
} LOCAL_KEYBOARD_INDICATOR_TRANSLATION, *PLOCAL_KEYBOARD_INDICATOR_TRANSLATION;

static LOCAL_KEYBOARD_INDICATOR_TRANSLATION IndicatorTranslation = { 3, {
	{0x3A, KEYBOARD_CAPS_LOCK_ON},
	{0x45, KEYBOARD_NUM_LOCK_ON},
	{0x46, KEYBOARD_SCROLL_LOCK_ON}}};

/* FUNCTIONS *****************************************************************/

/*
 * These functions are callbacks for filter driver custom interrupt
 * service routines.
 */
/*static VOID NTAPI
i8042KbdIsrWritePort(
	IN PVOID Context,
	IN UCHAR Value)
{
	PI8042_KEYBOARD_EXTENSION DeviceExtension;

	DeviceExtension = (PI8042_KEYBOARD_EXTENSION)Context;

	if (DeviceExtension->KeyboardHook.IsrWritePort)
	{
		DeviceExtension->KeyboardHook.IsrWritePort(
			DeviceExtension->KeyboardHook.CallContext,
			Value);
	}
	else
		i8042IsrWritePort(Context, Value, 0);
}*/

static VOID NTAPI
i8042KbdQueuePacket(
	IN PVOID Context)
{
	PI8042_KEYBOARD_EXTENSION DeviceExtension;

	DeviceExtension = (PI8042_KEYBOARD_EXTENSION)Context;

	DeviceExtension->KeyComplete = TRUE;
	DeviceExtension->KeysInBuffer++;
	if (DeviceExtension->KeysInBuffer > DeviceExtension->Common.PortDeviceExtension->Settings.KeyboardDataQueueSize)
	{
		WARN_(I8042PRT, "Keyboard buffer overflow\n");
		DeviceExtension->KeysInBuffer--;
	}

	TRACE_(I8042PRT, "Irq completes key\n");
	KeInsertQueueDpc(&DeviceExtension->DpcKeyboard, NULL, NULL);
}

/*
 * These functions are callbacks for filter driver custom
 * initialization routines.
 */
NTSTATUS NTAPI
i8042SynchWritePortKbd(
	IN PVOID Context,
	IN UCHAR Value,
	IN BOOLEAN WaitForAck)
{
	return i8042SynchWritePort(
		(PPORT_DEVICE_EXTENSION)Context,
		0,
		Value,
		WaitForAck);
}

/*
 * Process the keyboard internal device requests
 */
VOID NTAPI
i8042KbdStartIo(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	PI8042_KEYBOARD_EXTENSION DeviceExtension;
	PPORT_DEVICE_EXTENSION PortDeviceExtension;

	Stack = IoGetCurrentIrpStackLocation(Irp);
	DeviceExtension = (PI8042_KEYBOARD_EXTENSION)DeviceObject->DeviceExtension;
	PortDeviceExtension = DeviceExtension->Common.PortDeviceExtension;

	switch (Stack->Parameters.DeviceIoControl.IoControlCode)
	{
		case IOCTL_KEYBOARD_SET_INDICATORS:
		{
			TRACE_(I8042PRT, "IOCTL_KEYBOARD_SET_INDICATORS\n");
			INFO_(I8042PRT, "Leds: {%s%s%s }\n",
				DeviceExtension->KeyboardIndicators.LedFlags & KEYBOARD_CAPS_LOCK_ON ? " CAPSLOCK" : "",
				DeviceExtension->KeyboardIndicators.LedFlags & KEYBOARD_NUM_LOCK_ON ? " NUMLOCK" : "",
				DeviceExtension->KeyboardIndicators.LedFlags & KEYBOARD_SCROLL_LOCK_ON ? " SCROLLLOCK" : "");

			PortDeviceExtension->PacketBuffer[0] = KBD_CMD_SET_LEDS;
			PortDeviceExtension->PacketBuffer[1] = 0;
			if (DeviceExtension->KeyboardIndicators.LedFlags & KEYBOARD_CAPS_LOCK_ON)
				PortDeviceExtension->PacketBuffer[1] |= KBD_LED_CAPS;

			if (DeviceExtension->KeyboardIndicators.LedFlags & KEYBOARD_NUM_LOCK_ON)
				PortDeviceExtension->PacketBuffer[1] |= KBD_LED_NUM;

			if (DeviceExtension->KeyboardIndicators.LedFlags & KEYBOARD_SCROLL_LOCK_ON)
				PortDeviceExtension->PacketBuffer[1] |= KBD_LED_SCROLL;

			i8042StartPacket(
				PortDeviceExtension,
				&DeviceExtension->Common,
				PortDeviceExtension->PacketBuffer,
				2,
				Irp);
			break;
		}
		default:
		{
			ERR_(I8042PRT, "Unknown ioctl code 0x%lx\n",
				Stack->Parameters.DeviceIoControl.IoControlCode);
			ASSERT(FALSE);
		}
	}
}

static VOID
i8042PacketDpc(
	IN PPORT_DEVICE_EXTENSION DeviceExtension)
{
	BOOLEAN FinishIrp = FALSE;
	KIRQL Irql;
	NTSTATUS Result = STATUS_INTERNAL_ERROR; /* Shouldn't happen */

	/* If the interrupt happens before this is setup, the key
	 * was already in the buffer. Too bad! */
	if (!DeviceExtension->HighestDIRQLInterrupt)
		return;

	Irql = KeAcquireInterruptSpinLock(DeviceExtension->HighestDIRQLInterrupt);

	if (DeviceExtension->Packet.State == Idle
	 && DeviceExtension->PacketComplete)
	{
		FinishIrp = TRUE;
		Result = DeviceExtension->PacketResult;
		DeviceExtension->PacketComplete = FALSE;
	}

	KeReleaseInterruptSpinLock(DeviceExtension->HighestDIRQLInterrupt, Irql);

	if (!FinishIrp)
		return;

	if (DeviceExtension->CurrentIrp)
	{
		DeviceExtension->CurrentIrp->IoStatus.Status = Result;
		IoCompleteRequest(DeviceExtension->CurrentIrp, IO_NO_INCREMENT);
		IoStartNextPacket(DeviceExtension->CurrentIrpDevice, FALSE);
		DeviceExtension->CurrentIrp = NULL;
		DeviceExtension->CurrentIrpDevice = NULL;
	}
}

static VOID NTAPI
i8042PowerWorkItem(
	IN PDEVICE_OBJECT DeviceObject,
	IN PVOID Context)
{
	PI8042_KEYBOARD_EXTENSION DeviceExtension;
	PIRP WaitingIrp;
	NTSTATUS Status;

	UNREFERENCED_PARAMETER(DeviceObject);

	__analysis_assume(Context != NULL);
	DeviceExtension = Context;

	/* See https://learn.microsoft.com/en-us/archive/blogs/doronh/how-ps2-and-hid-keyboads-report-power-button-events */

	/* Register GUID_DEVICE_SYS_BUTTON interface and report capability */
	if (DeviceExtension->NewCaps != DeviceExtension->ReportedCaps)
	{
		WaitingIrp = InterlockedExchangePointer((PVOID)&DeviceExtension->PowerIrp, NULL);
		if (WaitingIrp)
		{
			/* Cancel the current power irp, as capability changed */
			WaitingIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
			WaitingIrp->IoStatus.Information = sizeof(ULONG);
			IoCompleteRequest(WaitingIrp, IO_NO_INCREMENT);
		}

		if (DeviceExtension->PowerInterfaceName.MaximumLength == 0)
		{
			/* We have never registered this interface ; do it */
			Status = IoRegisterDeviceInterface(
				DeviceExtension->Common.Pdo,
				&GUID_DEVICE_SYS_BUTTON,
				NULL,
				&DeviceExtension->PowerInterfaceName);
			if (!NT_SUCCESS(Status))
			{
				/* We can't do more yet, ignore the keypress... */
				WARN_(I8042PRT, "IoRegisterDeviceInterface(GUID_DEVICE_SYS_BUTTON) failed with status 0x%08lx\n",
					Status);
				DeviceExtension->PowerInterfaceName.MaximumLength = 0;
				return;
			}
		}
		else
		{
			/* Disable the interface. Once activated again, capabilities would be asked again */
			Status = IoSetDeviceInterfaceState(
				&DeviceExtension->PowerInterfaceName,
				FALSE);
			if (!NT_SUCCESS(Status))
			{
				/* Ignore the key press... */
				WARN_(I8042PRT, "Disabling interface %wZ failed with status 0x%08lx\n",
					&DeviceExtension->PowerInterfaceName, Status);
				return;
			}
		}
		/* Enable the interface. This leads to receiving a IOCTL_GET_SYS_BUTTON_CAPS,
		 * so we can report new capability */
		Status = IoSetDeviceInterfaceState(
				&DeviceExtension->PowerInterfaceName,
				TRUE);
		if (!NT_SUCCESS(Status))
		{
			/* Ignore the key press... */
			WARN_(I8042PRT, "Enabling interface %wZ failed with status 0x%08lx\n",
					&DeviceExtension->PowerInterfaceName, Status);
			return;
		}
	}

	/* Directly complete the IOCTL_GET_SYS_BUTTON_EVENT Irp (if any) */
	WaitingIrp = InterlockedExchangePointer((PVOID)&DeviceExtension->PowerIrp, NULL);
	if (WaitingIrp)
	{
		PULONG pEvent = (PULONG)WaitingIrp->AssociatedIrp.SystemBuffer;

		WaitingIrp->IoStatus.Status = STATUS_SUCCESS;
		WaitingIrp->IoStatus.Information = sizeof(ULONG);
		*pEvent = InterlockedExchange((PLONG)&DeviceExtension->LastPowerKey, 0);
		IoCompleteRequest(WaitingIrp, IO_NO_INCREMENT);
	}
}

/* Return TRUE if it was a power key */
static BOOLEAN
HandlePowerKeys(
	IN PI8042_KEYBOARD_EXTENSION DeviceExtension)
{
	PKEYBOARD_INPUT_DATA InputData;
	ULONG KeyPress;

	InputData = DeviceExtension->KeyboardBuffer + DeviceExtension->KeysInBuffer - 1;
	if (!(InputData->Flags & KEY_E0))
		return FALSE;

	switch (InputData->MakeCode)
	{
		case KEYBOARD_POWER_CODE:
			KeyPress = SYS_BUTTON_POWER;
			break;
		case KEYBOARD_SLEEP_CODE:
			KeyPress = SYS_BUTTON_SLEEP;
			break;
		case KEYBOARD_WAKE_CODE:
			KeyPress = SYS_BUTTON_WAKE;
			break;
		default:
			return FALSE;
	}

    if (InputData->Flags & KEY_BREAK)
		/* We already took care of the key press */
		return TRUE;

	/* Our work can only be done at passive level, so use a workitem */
	DeviceExtension->NewCaps |= KeyPress;
	InterlockedExchange((PLONG)&DeviceExtension->LastPowerKey, KeyPress);
	IoQueueWorkItem(
		DeviceExtension->PowerWorkItem,
		&i8042PowerWorkItem,
		DelayedWorkQueue,
		DeviceExtension);
	return TRUE;
}

static VOID NTAPI
i8042KbdDpcRoutine(
	IN PKDPC Dpc,
	IN PVOID DeferredContext,
	IN PVOID SystemArgument1,
	IN PVOID SystemArgument2)
{
	PI8042_KEYBOARD_EXTENSION DeviceExtension;
	PPORT_DEVICE_EXTENSION PortDeviceExtension;
	ULONG KeysTransferred = 0;
	ULONG KeysInBufferCopy;
	KIRQL Irql;

	UNREFERENCED_PARAMETER(Dpc);
	UNREFERENCED_PARAMETER(SystemArgument1);
	UNREFERENCED_PARAMETER(SystemArgument2);

	__analysis_assume(DeferredContext != NULL);
	DeviceExtension = DeferredContext;
	PortDeviceExtension = DeviceExtension->Common.PortDeviceExtension;

	if (HandlePowerKeys(DeviceExtension))
	{
		DeviceExtension->KeyComplete = FALSE;
		return;
	}

	i8042PacketDpc(PortDeviceExtension);
	if (!DeviceExtension->KeyComplete)
		return;
	/* We got the interrupt as it was being enabled, too bad */
	if (!PortDeviceExtension->HighestDIRQLInterrupt)
		return;

	Irql = KeAcquireInterruptSpinLock(PortDeviceExtension->HighestDIRQLInterrupt);

	DeviceExtension->KeyComplete = FALSE;
	KeysInBufferCopy = DeviceExtension->KeysInBuffer;

	KeReleaseInterruptSpinLock(PortDeviceExtension->HighestDIRQLInterrupt, Irql);

	TRACE_(I8042PRT, "Send a key\n");

	if (!DeviceExtension->KeyboardData.ClassService)
		return;

	INFO_(I8042PRT, "Sending %lu key(s)\n", KeysInBufferCopy);
	(*(PSERVICE_CALLBACK_ROUTINE)DeviceExtension->KeyboardData.ClassService)(
		DeviceExtension->KeyboardData.ClassDeviceObject,
		DeviceExtension->KeyboardBuffer,
		DeviceExtension->KeyboardBuffer + KeysInBufferCopy,
		&KeysTransferred);

	/* Validate that the callback didn't change the Irql. */
	ASSERT(KeGetCurrentIrql() == Irql);

	Irql = KeAcquireInterruptSpinLock(PortDeviceExtension->HighestDIRQLInterrupt);
	DeviceExtension->KeysInBuffer -= KeysTransferred;
	KeReleaseInterruptSpinLock(PortDeviceExtension->HighestDIRQLInterrupt, Irql);
}

/*
 * Runs the keyboard IOCTL dispatch.
 */
NTSTATUS NTAPI
i8042KbdDeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	PI8042_KEYBOARD_EXTENSION DeviceExtension;
	NTSTATUS Status;

	Stack = IoGetCurrentIrpStackLocation(Irp);
	Irp->IoStatus.Information = 0;
	DeviceExtension = (PI8042_KEYBOARD_EXTENSION)DeviceObject->DeviceExtension;

	switch (Stack->Parameters.DeviceIoControl.IoControlCode)
	{
		case IOCTL_GET_SYS_BUTTON_CAPS:
		{
			/* Part of GUID_DEVICE_SYS_BUTTON interface */
			PULONG pCaps;
			TRACE_(I8042PRT, "IOCTL_GET_SYS_BUTTON_CAPS\n");

			if (Stack->Parameters.DeviceIoControl.OutputBufferLength != sizeof(ULONG))
				Status = STATUS_INVALID_PARAMETER;
			else
			{
				pCaps = (PULONG)Irp->AssociatedIrp.SystemBuffer;
				*pCaps = DeviceExtension->NewCaps;
				DeviceExtension->ReportedCaps = DeviceExtension->NewCaps;
				Irp->IoStatus.Information = sizeof(ULONG);
				Status = STATUS_SUCCESS;
			}
			break;
		}
		case IOCTL_GET_SYS_BUTTON_EVENT:
		{
			/* Part of GUID_DEVICE_SYS_BUTTON interface */
			PIRP WaitingIrp;
			TRACE_(I8042PRT, "IOCTL_GET_SYS_BUTTON_EVENT\n");

			if (Stack->Parameters.DeviceIoControl.OutputBufferLength != sizeof(ULONG))
				Status = STATUS_INVALID_PARAMETER;
			else
			{
				WaitingIrp = InterlockedCompareExchangePointer(
					(PVOID)&DeviceExtension->PowerIrp,
					Irp,
					NULL);
				/* Check if an Irp is already pending */
				if (WaitingIrp)
				{
					/* Unable to have a 2nd pending IRP for this IOCTL */
					WARN_(I8042PRT, "Unable to pend a second IRP for IOCTL_GET_SYS_BUTTON_EVENT\n");
					Status = STATUS_INVALID_PARAMETER;
					Irp->IoStatus.Status = Status;
					IoCompleteRequest(Irp, IO_NO_INCREMENT);
				}
				else
				{
					ULONG PowerKey;
					PowerKey = InterlockedExchange((PLONG)&DeviceExtension->LastPowerKey, 0);
					if (PowerKey != 0)
					{
						(VOID)InterlockedCompareExchangePointer((PVOID)&DeviceExtension->PowerIrp, NULL, Irp);
						*(PULONG)Irp->AssociatedIrp.SystemBuffer = PowerKey;
						Status = STATUS_SUCCESS;
						Irp->IoStatus.Status = Status;
						Irp->IoStatus.Information = sizeof(ULONG);
						IoCompleteRequest(Irp, IO_NO_INCREMENT);
					}
					else
					{
						TRACE_(I8042PRT, "Pending IOCTL_GET_SYS_BUTTON_EVENT\n");
						Status = STATUS_PENDING;
						Irp->IoStatus.Status = Status;
						IoMarkIrpPending(Irp);
					}
				}
				return Status;
			}
			break;
		}
		default:
		{
			ERR_(I8042PRT, "IRP_MJ_DEVICE_CONTROL / unknown ioctl code 0x%lx\n",
				Stack->Parameters.DeviceIoControl.IoControlCode);
			return ForwardIrpAndForget(DeviceObject, Irp);
		}
	}

	if (Status != STATUS_PENDING)
	{
		Irp->IoStatus.Status = Status;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}

	return Status;
}

VOID
NTAPI
i8042InitializeKeyboardAttributes(
    PI8042_KEYBOARD_EXTENSION DeviceExtension)
{
    PPORT_DEVICE_EXTENSION PortDeviceExtension;
    PI8042_SETTINGS Settings;
    PKEYBOARD_ATTRIBUTES KeyboardAttributes;

    PortDeviceExtension = DeviceExtension->Common.PortDeviceExtension;
    Settings = &PortDeviceExtension->Settings;

    KeyboardAttributes = &DeviceExtension->KeyboardAttributes;

    KeyboardAttributes->KeyboardIdentifier.Type = (UCHAR)Settings->OverrideKeyboardType;
    KeyboardAttributes->KeyboardIdentifier.Subtype = (UCHAR)Settings->OverrideKeyboardSubtype;
    KeyboardAttributes->NumberOfFunctionKeys = 4;
    KeyboardAttributes->NumberOfIndicators = 3;
    KeyboardAttributes->NumberOfKeysTotal = 101;
    KeyboardAttributes->InputDataQueueLength = Settings->KeyboardDataQueueSize;
    KeyboardAttributes->KeyRepeatMinimum.UnitId = 0;
    KeyboardAttributes->KeyRepeatMinimum.Rate = (USHORT)Settings->SampleRate;
    KeyboardAttributes->KeyRepeatMinimum.Delay = 0;
    KeyboardAttributes->KeyRepeatMinimum.UnitId = 0;
    KeyboardAttributes->KeyRepeatMinimum.Rate = (USHORT)Settings->SampleRate;
    KeyboardAttributes->KeyRepeatMinimum.Delay = 0;
}

/*
 * Runs the keyboard IOCTL_INTERNAL dispatch.
 */
NTSTATUS NTAPI
i8042KbdInternalDeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	PI8042_KEYBOARD_EXTENSION DeviceExtension;
	NTSTATUS Status;

	Stack = IoGetCurrentIrpStackLocation(Irp);
	Irp->IoStatus.Information = 0;
	DeviceExtension = (PI8042_KEYBOARD_EXTENSION)DeviceObject->DeviceExtension;

	switch (Stack->Parameters.DeviceIoControl.IoControlCode)
	{
		case IOCTL_INTERNAL_KEYBOARD_CONNECT:
		{
			SIZE_T Size;
			PIO_WORKITEM WorkItem = NULL;
			PI8042_HOOK_WORKITEM WorkItemData = NULL;

			TRACE_(I8042PRT, "IRP_MJ_INTERNAL_DEVICE_CONTROL / IOCTL_INTERNAL_KEYBOARD_CONNECT\n");
			if (Stack->Parameters.DeviceIoControl.InputBufferLength != sizeof(CONNECT_DATA))
			{
				Status = STATUS_INVALID_PARAMETER;
				goto cleanup;
			}

			DeviceExtension->KeyboardData =
				*((PCONNECT_DATA)Stack->Parameters.DeviceIoControl.Type3InputBuffer);

			/* Send IOCTL_INTERNAL_I8042_HOOK_KEYBOARD to device stack */
			WorkItem = IoAllocateWorkItem(DeviceObject);
			if (!WorkItem)
			{
				WARN_(I8042PRT, "IoAllocateWorkItem() failed\n");
				Status = STATUS_INSUFFICIENT_RESOURCES;
				goto cleanup;
			}
			WorkItemData = ExAllocatePoolWithTag(
				NonPagedPool,
				sizeof(I8042_HOOK_WORKITEM),
				I8042PRT_TAG);
			if (!WorkItemData)
			{
				WARN_(I8042PRT, "ExAllocatePoolWithTag() failed\n");
				Status = STATUS_NO_MEMORY;
				goto cleanup;
			}
			WorkItemData->WorkItem = WorkItem;
			WorkItemData->Irp = Irp;

			/* Initialize extension */
			DeviceExtension->Common.Type = Keyboard;
			Size = DeviceExtension->Common.PortDeviceExtension->Settings.KeyboardDataQueueSize * sizeof(KEYBOARD_INPUT_DATA);
			DeviceExtension->KeyboardBuffer = ExAllocatePoolWithTag(
				NonPagedPool,
				Size,
				I8042PRT_TAG);
			if (!DeviceExtension->KeyboardBuffer)
			{
				WARN_(I8042PRT, "ExAllocatePoolWithTag() failed\n");
				Status = STATUS_NO_MEMORY;
				goto cleanup;
			}
			RtlZeroMemory(DeviceExtension->KeyboardBuffer, Size);
			KeInitializeDpc(
				&DeviceExtension->DpcKeyboard,
				i8042KbdDpcRoutine,
				DeviceExtension);
			DeviceExtension->PowerWorkItem = IoAllocateWorkItem(DeviceObject);
			if (!DeviceExtension->PowerWorkItem)
			{
				WARN_(I8042PRT, "IoAllocateWorkItem() failed\n");
				Status = STATUS_INSUFFICIENT_RESOURCES;
				goto cleanup;
			}
			DeviceExtension->DebugWorkItem = IoAllocateWorkItem(DeviceObject);
			if (!DeviceExtension->DebugWorkItem)
			{
				WARN_(I8042PRT, "IoAllocateWorkItem() failed\n");
				Status = STATUS_INSUFFICIENT_RESOURCES;
				goto cleanup;
			}
			DeviceExtension->Common.PortDeviceExtension->KeyboardExtension = DeviceExtension;
			DeviceExtension->Common.PortDeviceExtension->Flags |= KEYBOARD_CONNECTED;

            i8042InitializeKeyboardAttributes(DeviceExtension);

			IoMarkIrpPending(Irp);
			/* FIXME: DeviceExtension->KeyboardHook.IsrWritePort = ; */
			DeviceExtension->KeyboardHook.QueueKeyboardPacket = i8042KbdQueuePacket;
			DeviceExtension->KeyboardHook.CallContext = DeviceExtension;
			IoQueueWorkItem(WorkItem,
				i8042SendHookWorkItem,
				DelayedWorkQueue,
				WorkItemData);
			Status = STATUS_PENDING;
			break;

cleanup:
			if (DeviceExtension->KeyboardBuffer)
				ExFreePoolWithTag(DeviceExtension->KeyboardBuffer, I8042PRT_TAG);
			if (DeviceExtension->PowerWorkItem)
				IoFreeWorkItem(DeviceExtension->PowerWorkItem);
			if (DeviceExtension->DebugWorkItem)
				IoFreeWorkItem(DeviceExtension->DebugWorkItem);
			if (WorkItem)
				IoFreeWorkItem(WorkItem);
			if (WorkItemData)
				ExFreePoolWithTag(WorkItemData, I8042PRT_TAG);
			break;
		}
		case IOCTL_INTERNAL_KEYBOARD_DISCONNECT:
		{
			TRACE_(I8042PRT, "IRP_MJ_INTERNAL_DEVICE_CONTROL / IOCTL_INTERNAL_KEYBOARD_DISCONNECT\n");
			/* MSDN says that operation is to implemented.
			 * To implement it, we just have to do:
			 * DeviceExtension->KeyboardData.ClassService = NULL;
			 */
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		case IOCTL_INTERNAL_I8042_HOOK_KEYBOARD:
		{
			TRACE_(I8042PRT, "IRP_MJ_INTERNAL_DEVICE_CONTROL / IOCTL_INTERNAL_I8042_HOOK_KEYBOARD\n");
			/* Nothing to do here */
			Status = STATUS_SUCCESS;
			break;
		}
		case IOCTL_KEYBOARD_QUERY_ATTRIBUTES:
		{
		    PKEYBOARD_ATTRIBUTES KeyboardAttributes;

            /* FIXME: KeyboardAttributes are not initialized anywhere */
			TRACE_(I8042PRT, "IRP_MJ_INTERNAL_DEVICE_CONTROL / IOCTL_KEYBOARD_QUERY_ATTRIBUTES\n");
			if (Stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(KEYBOARD_ATTRIBUTES))
			{
				Status = STATUS_BUFFER_TOO_SMALL;
				break;
			}

            KeyboardAttributes = Irp->AssociatedIrp.SystemBuffer;
            *KeyboardAttributes = DeviceExtension->KeyboardAttributes;

			Irp->IoStatus.Information = sizeof(KEYBOARD_ATTRIBUTES);
			Status = STATUS_SUCCESS;
			break;
		}
		case IOCTL_KEYBOARD_QUERY_TYPEMATIC:
		{
			DPRINT1("IOCTL_KEYBOARD_QUERY_TYPEMATIC not implemented\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		case IOCTL_KEYBOARD_SET_TYPEMATIC:
		{
			DPRINT1("IOCTL_KEYBOARD_SET_TYPEMATIC not implemented\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		case IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION:
		{
			TRACE_(I8042PRT, "IRP_MJ_INTERNAL_DEVICE_CONTROL / IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION\n");

			/* We should check the UnitID, but it's kind of pointless as
			 * all keyboards are supposed to have the same one
			 */
			if (Stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(LOCAL_KEYBOARD_INDICATOR_TRANSLATION))
			{
				Status = STATUS_BUFFER_TOO_SMALL;
			}
			else
			{
				RtlCopyMemory(
					Irp->AssociatedIrp.SystemBuffer,
					&IndicatorTranslation,
					sizeof(LOCAL_KEYBOARD_INDICATOR_TRANSLATION));
				Irp->IoStatus.Information = sizeof(LOCAL_KEYBOARD_INDICATOR_TRANSLATION);
				Status = STATUS_SUCCESS;
			}
			break;
		}
		case IOCTL_KEYBOARD_QUERY_INDICATORS:
		{
			TRACE_(I8042PRT, "IRP_MJ_INTERNAL_DEVICE_CONTROL / IOCTL_KEYBOARD_QUERY_INDICATORS\n");

			if (Stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(KEYBOARD_INDICATOR_PARAMETERS))
			{
				Status = STATUS_BUFFER_TOO_SMALL;
			}
			else
			{
				RtlCopyMemory(
					Irp->AssociatedIrp.SystemBuffer,
					&DeviceExtension->KeyboardIndicators,
					sizeof(KEYBOARD_INDICATOR_PARAMETERS));
				Irp->IoStatus.Information = sizeof(KEYBOARD_INDICATOR_PARAMETERS);
				Status = STATUS_SUCCESS;
			}
			break;
		}
		case IOCTL_KEYBOARD_SET_INDICATORS:
		{
			TRACE_(I8042PRT, "IRP_MJ_INTERNAL_DEVICE_CONTROL / IOCTL_KEYBOARD_SET_INDICATORS\n");

			if (Stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(KEYBOARD_INDICATOR_PARAMETERS))
			{
				Status = STATUS_BUFFER_TOO_SMALL;
			}
			else
			{
				RtlCopyMemory(
					&DeviceExtension->KeyboardIndicators,
					Irp->AssociatedIrp.SystemBuffer,
					sizeof(KEYBOARD_INDICATOR_PARAMETERS));
				Status = STATUS_PENDING;
				IoMarkIrpPending(Irp);
				IoStartPacket(DeviceObject, Irp, NULL, NULL);
			}
			break;
		}
		default:
		{
			ERR_(I8042PRT, "IRP_MJ_INTERNAL_DEVICE_CONTROL / unknown ioctl code 0x%lx\n",
				Stack->Parameters.DeviceIoControl.IoControlCode);
			ASSERT(FALSE);
			return ForwardIrpAndForget(DeviceObject, Irp);
		}
	}

	if (Status != STATUS_PENDING)
	{
		Irp->IoStatus.Status = Status;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}
	return Status;
}

/*
 * Call the customization hook. The ToReturn parameter is about whether
 * we should go on with the interrupt. The return value is what
 * we should return (indicating to the system whether someone else
 * should try to handle the interrupt)
 */
static BOOLEAN
i8042KbdCallIsrHook(
	IN PI8042_KEYBOARD_EXTENSION DeviceExtension,
	IN UCHAR Status,
	IN UCHAR Input,
	OUT PBOOLEAN ToReturn)
{
	BOOLEAN HookReturn, HookContinue;

	HookContinue = FALSE;

	if (DeviceExtension->KeyboardHook.IsrRoutine)
	{
		HookReturn = DeviceExtension->KeyboardHook.IsrRoutine(
			DeviceExtension->KeyboardHook.Context,
			DeviceExtension->KeyboardBuffer + DeviceExtension->KeysInBuffer,
			&DeviceExtension->Common.PortDeviceExtension->Packet,
			Status,
			&Input,
			&HookContinue,
			&DeviceExtension->KeyboardScanState);

		if (!HookContinue)
		{
			*ToReturn = HookReturn;
			return TRUE;
		}
	}
	return FALSE;
}

BOOLEAN NTAPI
i8042KbdInterruptService(
	IN PKINTERRUPT Interrupt,
	PVOID Context)
{
	PI8042_KEYBOARD_EXTENSION DeviceExtension;
	PPORT_DEVICE_EXTENSION PortDeviceExtension;
	PKEYBOARD_INPUT_DATA InputData;
	ULONG Counter;
	UCHAR PortStatus = 0, Output = 0;
	BOOLEAN ToReturn = FALSE;
	NTSTATUS Status;

	UNREFERENCED_PARAMETER(Interrupt);

	__analysis_assume(Context != NULL);
	DeviceExtension = Context;
	PortDeviceExtension = DeviceExtension->Common.PortDeviceExtension;
	InputData = DeviceExtension->KeyboardBuffer + DeviceExtension->KeysInBuffer;
	Counter = PortDeviceExtension->Settings.PollStatusIterations;

	while (Counter)
	{
		Status = i8042ReadStatus(PortDeviceExtension, &PortStatus);
		if (!NT_SUCCESS(Status))
		{
			WARN_(I8042PRT, "i8042ReadStatus() failed with status 0x%08lx\n", Status);
			return FALSE;
		}
		Status = i8042ReadKeyboardData(PortDeviceExtension, &Output);
		if (NT_SUCCESS(Status))
			break;
		KeStallExecutionProcessor(1);
		Counter--;
	}
	if (Counter == 0)
	{
		WARN_(I8042PRT, "Spurious i8042 keyboard interrupt\n");
		return FALSE;
	}

	INFO_(I8042PRT, "Got: 0x%02x\n", Output);

	if (PortDeviceExtension->Settings.CrashOnCtrlScroll)
	{
		/* Test for CTRL + SCROLL LOCK twice */
		static const UCHAR ScanCodes[] = { 0x1d, 0x46, 0xc6, 0x46, 0 };

		if (Output == ScanCodes[DeviceExtension->ComboPosition])
		{
			DeviceExtension->ComboPosition++;
			if (ScanCodes[DeviceExtension->ComboPosition] == 0)
				KeBugCheck(MANUALLY_INITIATED_CRASH);
		}
		else if (Output == 0xfa)
		{
		    /* Ignore ACK */
		}
		else if (Output == ScanCodes[0])
			DeviceExtension->ComboPosition = 1;
		else
			DeviceExtension->ComboPosition = 0;

		/* Test for TAB + key combination */
		if (InputData->MakeCode == 0x0F)
			DeviceExtension->TabPressed = !(InputData->Flags & KEY_BREAK);
		else if (DeviceExtension->TabPressed)
		{
			DeviceExtension->TabPressed = FALSE;

            /* Check which action to do */
            if (InputData->MakeCode == 0x25)
            {
                /* k - Breakpoint */
                DbgBreakPointWithStatus(DBG_STATUS_SYSRQ);
            }
            else if (InputData->MakeCode == 0x30)
            {
                /* b - Bugcheck */
                KeBugCheck(MANUALLY_INITIATED_CRASH);
            }
            else
            {
			    /* Send request to the kernel debugger.
			     * Unknown requests will be ignored. */
			    KdSystemDebugControl(' soR',
			                         (PVOID)(ULONG_PTR)InputData->MakeCode,
			                         0,
			                         NULL,
			                         0,
			                         NULL,
			                         KernelMode);
            }
		}
	}

	if (i8042KbdCallIsrHook(DeviceExtension, PortStatus, Output, &ToReturn))
		return ToReturn;

	if (i8042PacketIsr(PortDeviceExtension, Output))
	{
		if (PortDeviceExtension->PacketComplete)
		{
			TRACE_(I8042PRT, "Packet complete\n");
			KeInsertQueueDpc(&DeviceExtension->DpcKeyboard, NULL, NULL);
		}
		TRACE_(I8042PRT, "Irq eaten by packet\n");
		return TRUE;
	}

	TRACE_(I8042PRT, "Irq is keyboard input\n");

	if (DeviceExtension->KeyboardScanState == Normal)
	{
		switch (Output)
		{
			case 0xe0:
				DeviceExtension->KeyboardScanState = GotE0;
				return TRUE;
			case 0xe1:
				DeviceExtension->KeyboardScanState = GotE1;
				return TRUE;
			default:
				break;
		}
	}

	/* Update InputData */
	InputData->Flags = 0;
	switch (DeviceExtension->KeyboardScanState)
	{
		case GotE0:
			InputData->Flags |= KEY_E0;
			break;
		case GotE1:
			InputData->Flags |= KEY_E1;
			break;
		default:
			break;
	}
	DeviceExtension->KeyboardScanState = Normal;
	if (Output & 0x80)
		InputData->Flags |= KEY_BREAK;
	else
		InputData->Flags |= KEY_MAKE;
	InputData->MakeCode = Output & 0x7f;
	InputData->Reserved = 0;

	DeviceExtension->KeyboardHook.QueueKeyboardPacket(DeviceExtension->KeyboardHook.CallContext);

	return TRUE;
}
