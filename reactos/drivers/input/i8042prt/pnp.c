/*
 * PROJECT:     ReactOS i8042 (ps/2 keyboard-mouse controller) driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/input/i8042prt/pnp.c
 * PURPOSE:     IRP_MJ_PNP operations
 * PROGRAMMERS: Copyright 2006-2007 Hervé Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "i8042prt.h"

/* FUNCTIONS *****************************************************************/

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
BOOLEAN
i8042ChangeMode(
	IN PPORT_DEVICE_EXTENSION DeviceExtension,
	IN UCHAR FlagsToDisable,
	IN UCHAR FlagsToEnable)
{
	UCHAR Value;
	NTSTATUS Status;

	if (!i8042Write(DeviceExtension, DeviceExtension->ControlPort, KBD_READ_MODE))
	{
		WARN_(I8042PRT, "Can't read i8042 mode\n");
		return FALSE;
	}

	Status = i8042ReadDataWait(DeviceExtension, &Value);
	if (!NT_SUCCESS(Status))
	{
		WARN_(I8042PRT, "No response after read i8042 mode\n");
		return FALSE;
	}

	Value &= ~FlagsToDisable;
	Value |= FlagsToEnable;

	if (!i8042Write(DeviceExtension, DeviceExtension->ControlPort, KBD_WRITE_MODE))
	{
		WARN_(I8042PRT, "Can't set i8042 mode\n");
		return FALSE;
	}

	if (!i8042Write(DeviceExtension, DeviceExtension->DataPort, Value))
	{
		WARN_(I8042PRT, "Can't send i8042 mode\n");
		return FALSE;
	}

	return TRUE;
}

static NTSTATUS
i8042BasicDetect(
	IN PPORT_DEVICE_EXTENSION DeviceExtension)
{
	NTSTATUS Status;
	UCHAR Value = 0;

	/* Don't enable keyboard and mouse interrupts, disable keyboard/mouse */
	if (!i8042ChangeMode(DeviceExtension, CCB_KBD_INT_ENAB | CCB_MOUSE_INT_ENAB, CCB_KBD_DISAB | CCB_MOUSE_DISAB))
		return STATUS_IO_DEVICE_ERROR;

	i8042Flush(DeviceExtension);

	if (!i8042Write(DeviceExtension, DeviceExtension->ControlPort, CTRL_SELF_TEST))
	{
		WARN_(I8042PRT, "Writing CTRL_SELF_TEST command failed\n");
		return STATUS_IO_TIMEOUT;
	}

	Status = i8042ReadDataWait(DeviceExtension, &Value);
	if (!NT_SUCCESS(Status))
	{
		WARN_(I8042PRT, "Failed to read CTRL_SELF_TEST response, status 0x%08lx\n", Status);
		return Status;
	}

	if (Value != 0x55)
	{
		WARN_(I8042PRT, "Got 0x%02x instead of 0x55\n", Value);
		return STATUS_IO_DEVICE_ERROR;
	}

	/*
	 * We used to send a KBD_LINE_TEST (0xAB) command here, but on at least HP
	 * Pavilion notebooks the response to that command was incorrect.
	 * So now we just assume that a keyboard is attached.
	 */
	DeviceExtension->Flags |= KEYBOARD_PRESENT;

	if (i8042Write(DeviceExtension, DeviceExtension->ControlPort, MOUSE_LINE_TEST))
	{
		Status = i8042ReadDataWait(DeviceExtension, &Value);
		if (NT_SUCCESS(Status) && Value == 0)
			DeviceExtension->Flags |= MOUSE_PRESENT;
	}

	if (IsFirstStageSetup())
		/* Ignore the mouse */
		DeviceExtension->Flags &= ~MOUSE_PRESENT;

	return STATUS_SUCCESS;
}

static BOOLEAN
i8042DetectKeyboard(
	IN PPORT_DEVICE_EXTENSION DeviceExtension)
{
	NTSTATUS Status;

	/* Set LEDs (that is not fatal if some error occurs) */
	Status = i8042SynchWritePort(DeviceExtension, 0, KBD_CMD_SET_LEDS, TRUE);
	if (NT_SUCCESS(Status))
	{
		Status = i8042SynchWritePort(DeviceExtension, 0, 0, TRUE);
		if (!NT_SUCCESS(Status))
		{
			WARN_(I8042PRT, "Can't finish SET_LEDS (0x%08lx)\n", Status);
			return FALSE;
		}
	}
	else
	{
		WARN_(I8042PRT, "Warning: can't write SET_LEDS (0x%08lx)\n", Status);
	}

	/* Turn on translation and SF (Some machines don't reboot if SF is not set) */
	if (!i8042ChangeMode(DeviceExtension, 0, CCB_TRANSLATE | CCB_SYSTEM_FLAG))
		return FALSE;

	return TRUE;
}

static BOOLEAN
i8042DetectMouse(
	IN PPORT_DEVICE_EXTENSION DeviceExtension)
{
	BOOLEAN Ok = FALSE;
	NTSTATUS Status;
	UCHAR Value;
	UCHAR ExpectedReply[] = { MOUSE_ACK, 0xAA, 0x00 };
	UCHAR ReplyByte;

	i8042Flush(DeviceExtension);

	if (!i8042Write(DeviceExtension, DeviceExtension->ControlPort, CTRL_WRITE_MOUSE)
	  ||!i8042Write(DeviceExtension, DeviceExtension->DataPort, MOU_CMD_RESET))
	{
		WARN_(I8042PRT, "Failed to write reset command to mouse\n");
		goto cleanup;
	}

	for (ReplyByte = 0;
	     ReplyByte < sizeof(ExpectedReply) / sizeof(ExpectedReply[0]);
	     ReplyByte++)
	{
		ULONG Counter = 500;

		do
		{
			Status = i8042ReadDataWait(DeviceExtension, &Value);
		} while (Status == STATUS_IO_TIMEOUT && Counter--);

		if (!NT_SUCCESS(Status))
		{
			WARN_(I8042PRT, "No ACK after mouse reset, status 0x%08lx\n", Status);
			goto cleanup;
		}
		else if (Value != ExpectedReply[ReplyByte])
		{
			WARN_(I8042PRT, "Unexpected reply: 0x%02x (expected 0x%02x)\n",
			        Value, ExpectedReply[ReplyByte]);
			goto cleanup;
		}
	}

	Ok = TRUE;

cleanup:
	if (!Ok)
	{
		/* There is probably no mouse present. On some systems,
		   the probe locks the entire keyboard controller. Let's
		   try to get access to the keyboard again by sending a
		   reset */
		i8042Flush(DeviceExtension);
		i8042Write(DeviceExtension, DeviceExtension->ControlPort, CTRL_SELF_TEST);
		i8042ReadDataWait(DeviceExtension, &Value);
		i8042Flush(DeviceExtension);
	}

	INFO_(I8042PRT, "Mouse %sdetected\n", Ok ? "" : "not ");

	return Ok;
}

static NTSTATUS
i8042ConnectKeyboardInterrupt(
	IN PI8042_KEYBOARD_EXTENSION DeviceExtension)
{
	PPORT_DEVICE_EXTENSION PortDeviceExtension;
	KIRQL DirqlMax;
	NTSTATUS Status;

	TRACE_(I8042PRT, "i8042ConnectKeyboardInterrupt()\n");

	PortDeviceExtension = DeviceExtension->Common.PortDeviceExtension;
	DirqlMax = MAX(
		PortDeviceExtension->KeyboardInterrupt.Dirql,
		PortDeviceExtension->MouseInterrupt.Dirql);

	INFO_(I8042PRT, "KeyboardInterrupt.Vector         %lu\n",
		PortDeviceExtension->KeyboardInterrupt.Vector);
	INFO_(I8042PRT, "KeyboardInterrupt.Dirql          %lu\n",
		PortDeviceExtension->KeyboardInterrupt.Dirql);
	INFO_(I8042PRT, "KeyboardInterrupt.DirqlMax       %lu\n",
		DirqlMax);
	INFO_(I8042PRT, "KeyboardInterrupt.InterruptMode  %s\n",
		PortDeviceExtension->KeyboardInterrupt.InterruptMode == LevelSensitive ? "LevelSensitive" : "Latched");
	INFO_(I8042PRT, "KeyboardInterrupt.ShareInterrupt %s\n",
		PortDeviceExtension->KeyboardInterrupt.ShareInterrupt ? "yes" : "no");
	INFO_(I8042PRT, "KeyboardInterrupt.Affinity       0x%lx\n",
		PortDeviceExtension->KeyboardInterrupt.Affinity);
	Status = IoConnectInterrupt(
		&PortDeviceExtension->KeyboardInterrupt.Object,
		i8042KbdInterruptService,
		DeviceExtension, &PortDeviceExtension->SpinLock,
		PortDeviceExtension->KeyboardInterrupt.Vector, PortDeviceExtension->KeyboardInterrupt.Dirql, DirqlMax,
		PortDeviceExtension->KeyboardInterrupt.InterruptMode, PortDeviceExtension->KeyboardInterrupt.ShareInterrupt,
		PortDeviceExtension->KeyboardInterrupt.Affinity, FALSE);
	if (!NT_SUCCESS(Status))
	{
		WARN_(I8042PRT, "IoConnectInterrupt() failed with status 0x%08x\n", Status);
		return Status;
	}

	if (DirqlMax == PortDeviceExtension->KeyboardInterrupt.Dirql)
		PortDeviceExtension->HighestDIRQLInterrupt = PortDeviceExtension->KeyboardInterrupt.Object;
	PortDeviceExtension->Flags |= KEYBOARD_INITIALIZED;
	return STATUS_SUCCESS;
}

static NTSTATUS
i8042ConnectMouseInterrupt(
	IN PI8042_MOUSE_EXTENSION DeviceExtension)
{
	PPORT_DEVICE_EXTENSION PortDeviceExtension;
	KIRQL DirqlMax;
	NTSTATUS Status;

	TRACE_(I8042PRT, "i8042ConnectMouseInterrupt()\n");

	Status = i8042MouInitialize(DeviceExtension);
	if (!NT_SUCCESS(Status))
		return Status;

	PortDeviceExtension = DeviceExtension->Common.PortDeviceExtension;
	DirqlMax = MAX(
		PortDeviceExtension->KeyboardInterrupt.Dirql,
		PortDeviceExtension->MouseInterrupt.Dirql);

	INFO_(I8042PRT, "MouseInterrupt.Vector         %lu\n",
		PortDeviceExtension->MouseInterrupt.Vector);
	INFO_(I8042PRT, "MouseInterrupt.Dirql          %lu\n",
		PortDeviceExtension->MouseInterrupt.Dirql);
	INFO_(I8042PRT, "MouseInterrupt.DirqlMax       %lu\n",
		DirqlMax);
	INFO_(I8042PRT, "MouseInterrupt.InterruptMode  %s\n",
		PortDeviceExtension->MouseInterrupt.InterruptMode == LevelSensitive ? "LevelSensitive" : "Latched");
	INFO_(I8042PRT, "MouseInterrupt.ShareInterrupt %s\n",
		PortDeviceExtension->MouseInterrupt.ShareInterrupt ? "yes" : "no");
	INFO_(I8042PRT, "MouseInterrupt.Affinity       0x%lx\n",
		PortDeviceExtension->MouseInterrupt.Affinity);
	Status = IoConnectInterrupt(
		&PortDeviceExtension->MouseInterrupt.Object,
		i8042MouInterruptService,
		DeviceExtension, &PortDeviceExtension->SpinLock,
		PortDeviceExtension->MouseInterrupt.Vector, PortDeviceExtension->MouseInterrupt.Dirql, DirqlMax,
		PortDeviceExtension->MouseInterrupt.InterruptMode, PortDeviceExtension->MouseInterrupt.ShareInterrupt,
		PortDeviceExtension->MouseInterrupt.Affinity, FALSE);
	if (!NT_SUCCESS(Status))
	{
		WARN_(I8042PRT, "IoConnectInterrupt() failed with status 0x%08x\n", Status);
		goto cleanup;
	}

	if (DirqlMax == PortDeviceExtension->MouseInterrupt.Dirql)
		PortDeviceExtension->HighestDIRQLInterrupt = PortDeviceExtension->MouseInterrupt.Object;

	PortDeviceExtension->Flags |= MOUSE_INITIALIZED;
	Status = STATUS_SUCCESS;

cleanup:
	if (!NT_SUCCESS(Status))
	{
		PortDeviceExtension->Flags &= ~MOUSE_INITIALIZED;
		if (PortDeviceExtension->MouseInterrupt.Object)
		{
			IoDisconnectInterrupt(PortDeviceExtension->MouseInterrupt.Object);
			PortDeviceExtension->HighestDIRQLInterrupt = PortDeviceExtension->KeyboardInterrupt.Object;
		}
	}
	return Status;
}

static NTSTATUS
EnableInterrupts(
	IN PPORT_DEVICE_EXTENSION DeviceExtension)
{
	UCHAR FlagsToDisable = 0;
	UCHAR FlagsToEnable = 0;

	i8042Flush(DeviceExtension);

	/* Select the devices we have */
	if (DeviceExtension->Flags & KEYBOARD_PRESENT)
	{
		FlagsToDisable |= CCB_KBD_DISAB;
		FlagsToEnable |= CCB_KBD_INT_ENAB;
	}
	if (DeviceExtension->Flags & MOUSE_PRESENT)
	{
		FlagsToDisable |= CCB_MOUSE_DISAB;
		FlagsToEnable |= CCB_MOUSE_INT_ENAB;
	}
	if (!i8042ChangeMode(DeviceExtension, FlagsToDisable, FlagsToEnable))
		return STATUS_UNSUCCESSFUL;

	/* Reset the mouse (if any) to start the detection */
	if (DeviceExtension->Flags & MOUSE_PRESENT)
	{
		KIRQL Irql;

		Irql = KeAcquireInterruptSpinLock(DeviceExtension->HighestDIRQLInterrupt);

		i8042Write(DeviceExtension, DeviceExtension->ControlPort, CTRL_WRITE_MOUSE);
		i8042Write(DeviceExtension, DeviceExtension->DataPort, MOU_CMD_RESET);

		KeReleaseInterruptSpinLock(DeviceExtension->HighestDIRQLInterrupt, Irql);
	}

	return STATUS_SUCCESS;
}

static NTSTATUS
StartProcedure(
	IN PPORT_DEVICE_EXTENSION DeviceExtension)
{
	NTSTATUS Status;

	if (DeviceExtension->DataPort == 0)
	{
		/* Unable to do something at the moment */
		return STATUS_SUCCESS;
	}

	if (!(DeviceExtension->Flags & (KEYBOARD_PRESENT | MOUSE_PRESENT)))
	{
		/* Try to detect them */
		TRACE_(I8042PRT, "Check if the controller is really a i8042\n");
		Status = i8042BasicDetect(DeviceExtension);
		if (!NT_SUCCESS(Status))
		{
			WARN_(I8042PRT, "i8042BasicDetect() failed with status 0x%08lx\n", Status);
			return STATUS_UNSUCCESSFUL;
		}
		TRACE_(I8042PRT, "Detecting keyboard\n");
		if (!i8042DetectKeyboard(DeviceExtension))
			return STATUS_UNSUCCESSFUL;
		TRACE_(I8042PRT, "Detecting mouse\n");
		if (!i8042DetectMouse(DeviceExtension))
			return STATUS_UNSUCCESSFUL;
		INFO_(I8042PRT, "Keyboard present: %s\n", DeviceExtension->Flags & KEYBOARD_PRESENT ? "YES" : "NO");
		INFO_(I8042PRT, "Mouse present   : %s\n", DeviceExtension->Flags & MOUSE_PRESENT ? "YES" : "NO");
	}

	/* Connect interrupts */
	if (DeviceExtension->Flags & KEYBOARD_PRESENT &&
	    DeviceExtension->Flags & KEYBOARD_CONNECTED &&
	    DeviceExtension->Flags & KEYBOARD_STARTED &&
	    !(DeviceExtension->Flags & (MOUSE_PRESENT | KEYBOARD_INITIALIZED)))
	{
		/* No mouse, and the keyboard is ready */
		Status = i8042ConnectKeyboardInterrupt(DeviceExtension->KeyboardExtension);
		if (NT_SUCCESS(Status))
		{
			DeviceExtension->Flags |= KEYBOARD_INITIALIZED;
			Status = EnableInterrupts(DeviceExtension);
		}
	}
	else if (DeviceExtension->Flags & MOUSE_PRESENT &&
	         DeviceExtension->Flags & MOUSE_CONNECTED &&
	         DeviceExtension->Flags & MOUSE_STARTED &&
	         !(DeviceExtension->Flags & (KEYBOARD_PRESENT | MOUSE_INITIALIZED)))
	{
		/* No keyboard, and the mouse is ready */
		Status = i8042ConnectMouseInterrupt(DeviceExtension->MouseExtension);
		if (NT_SUCCESS(Status))
		{
			DeviceExtension->Flags |= MOUSE_INITIALIZED;
			Status = EnableInterrupts(DeviceExtension);
		}
	}
	else if (DeviceExtension->Flags & KEYBOARD_PRESENT &&
	         DeviceExtension->Flags & KEYBOARD_CONNECTED &&
	         DeviceExtension->Flags & KEYBOARD_STARTED &&
	         DeviceExtension->Flags & MOUSE_PRESENT &&
	         DeviceExtension->Flags & MOUSE_CONNECTED &&
	         DeviceExtension->Flags & MOUSE_STARTED &&
	         !(DeviceExtension->Flags & (KEYBOARD_INITIALIZED | MOUSE_INITIALIZED)))
	{
		/* The keyboard and mouse are ready */
		Status = i8042ConnectKeyboardInterrupt(DeviceExtension->KeyboardExtension);
		if (NT_SUCCESS(Status))
		{
			DeviceExtension->Flags |= KEYBOARD_INITIALIZED;
			Status = i8042ConnectMouseInterrupt(DeviceExtension->MouseExtension);
			if (NT_SUCCESS(Status))
			{
				DeviceExtension->Flags |= MOUSE_INITIALIZED;
				Status = EnableInterrupts(DeviceExtension);
			}
		}
	}
	else
	{
		/* Nothing to do */
		Status = STATUS_SUCCESS;
	}

	return Status;
}

static NTSTATUS
i8042PnpStartDevice(
	IN PDEVICE_OBJECT DeviceObject,
	IN PCM_RESOURCE_LIST AllocatedResources,
	IN PCM_RESOURCE_LIST AllocatedResourcesTranslated)
{
	PFDO_DEVICE_EXTENSION DeviceExtension;
	PPORT_DEVICE_EXTENSION PortDeviceExtension;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDescriptor, ResourceDescriptorTranslated;
	INTERRUPT_DATA InterruptData;
	BOOLEAN FoundDataPort = FALSE;
	BOOLEAN FoundControlPort = FALSE;
	BOOLEAN FoundIrq = FALSE;
	ULONG i;
	NTSTATUS Status;

	TRACE_(I8042PRT, "i8042PnpStartDevice(%p)\n", DeviceObject);
	DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	PortDeviceExtension = DeviceExtension->PortDeviceExtension;

	ASSERT(DeviceExtension->PnpState == dsStopped);

	if (!AllocatedResources)
	{
		WARN_(I8042PRT, "No allocated resources sent to driver\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	if (AllocatedResources->Count != 1)
	{
		WARN_(I8042PRT, "Wrong number of allocated resources sent to driver\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	if (AllocatedResources->List[0].PartialResourceList.Version != 1
	 || AllocatedResources->List[0].PartialResourceList.Revision != 1
	 || AllocatedResourcesTranslated->List[0].PartialResourceList.Version != 1
	 || AllocatedResourcesTranslated->List[0].PartialResourceList.Revision != 1)
	{
		WARN_(I8042PRT, "Revision mismatch: %u.%u != 1.1 or %u.%u != 1.1\n",
			AllocatedResources->List[0].PartialResourceList.Version,
			AllocatedResources->List[0].PartialResourceList.Revision,
			AllocatedResourcesTranslated->List[0].PartialResourceList.Version,
			AllocatedResourcesTranslated->List[0].PartialResourceList.Revision);
		return STATUS_REVISION_MISMATCH;
	}

	/* Get Irq and optionally control port and data port */
	for (i = 0; i < AllocatedResources->List[0].PartialResourceList.Count; i++)
	{
		ResourceDescriptor = &AllocatedResources->List[0].PartialResourceList.PartialDescriptors[i];
		ResourceDescriptorTranslated = &AllocatedResourcesTranslated->List[0].PartialResourceList.PartialDescriptors[i];
		switch (ResourceDescriptor->Type)
		{
			case CmResourceTypePort:
			{
				if (ResourceDescriptor->u.Port.Length == 1)
				{
					/* We assume that the first ressource will
					 * be the control port and the second one
					 * will be the data port...
					 */
					if (!FoundDataPort)
					{
						PortDeviceExtension->DataPort = ULongToPtr(ResourceDescriptor->u.Port.Start.u.LowPart);
						INFO_(I8042PRT, "Found data port: %p\n", PortDeviceExtension->DataPort);
						FoundDataPort = TRUE;
					}
					else if (!FoundControlPort)
					{
						PortDeviceExtension->ControlPort = ULongToPtr(ResourceDescriptor->u.Port.Start.u.LowPart);
						INFO_(I8042PRT, "Found control port: %p\n", PortDeviceExtension->ControlPort);
						FoundControlPort = TRUE;
					}
					else
					{
						WARN_(I8042PRT, "Too much I/O ranges provided: 0x%lx\n", ResourceDescriptor->u.Port.Length);
						return STATUS_INVALID_PARAMETER;
					}
				}
				else
					WARN_(I8042PRT, "Invalid I/O range length: 0x%lx\n", ResourceDescriptor->u.Port.Length);
				break;
			}
			case CmResourceTypeInterrupt:
			{
				if (FoundIrq)
					return STATUS_INVALID_PARAMETER;
				InterruptData.Dirql = (KIRQL)ResourceDescriptorTranslated->u.Interrupt.Level;
				InterruptData.Vector = ResourceDescriptorTranslated->u.Interrupt.Vector;
				InterruptData.Affinity = ResourceDescriptorTranslated->u.Interrupt.Affinity;
				if (ResourceDescriptorTranslated->Flags & CM_RESOURCE_INTERRUPT_LATCHED)
					InterruptData.InterruptMode = Latched;
				else
					InterruptData.InterruptMode = LevelSensitive;
				InterruptData.ShareInterrupt = (ResourceDescriptorTranslated->ShareDisposition == CmResourceShareShared);
				INFO_(I8042PRT, "Found irq resource: %lu\n", ResourceDescriptor->u.Interrupt.Level);
				FoundIrq = TRUE;
				break;
			}
			default:
				WARN_(I8042PRT, "Unknown resource descriptor type 0x%x\n", ResourceDescriptor->Type);
		}
	}

	if (!FoundIrq)
	{
		WARN_(I8042PRT, "Interrupt resource was not found in allocated resources list\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	else if (DeviceExtension->Type == Keyboard && (!FoundDataPort || !FoundControlPort))
	{
		WARN_(I8042PRT, "Some required resources were not found in allocated resources list\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	else if (DeviceExtension->Type == Mouse && (FoundDataPort || FoundControlPort))
	{
		WARN_(I8042PRT, "Too much resources were provided in allocated resources list\n");
		return STATUS_INVALID_PARAMETER;
	}

	switch (DeviceExtension->Type)
	{
		case Keyboard:
		{
			RtlCopyMemory(
				&PortDeviceExtension->KeyboardInterrupt,
				&InterruptData,
				sizeof(INTERRUPT_DATA));
			PortDeviceExtension->Flags |= KEYBOARD_STARTED;
			Status = StartProcedure(PortDeviceExtension);
			break;
		}
		case Mouse:
		{
			RtlCopyMemory(
				&PortDeviceExtension->MouseInterrupt,
				&InterruptData,
				sizeof(INTERRUPT_DATA));
			PortDeviceExtension->Flags |= MOUSE_STARTED;
			Status = StartProcedure(PortDeviceExtension);
			break;
		}
		default:
		{
			ERR_(I8042PRT, "Unknown FDO type %u\n", DeviceExtension->Type);
			ASSERT(FALSE);
			Status = STATUS_INVALID_DEVICE_REQUEST;
		}
	}

	if (NT_SUCCESS(Status))
		DeviceExtension->PnpState = dsStarted;

	return Status;
}

NTSTATUS NTAPI
i8042Pnp(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	ULONG MinorFunction;
	I8042_DEVICE_TYPE DeviceType;
	ULONG_PTR Information = 0;
	NTSTATUS Status;

	Stack = IoGetCurrentIrpStackLocation(Irp);
	MinorFunction = Stack->MinorFunction;
	DeviceType = ((PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->Type;

	switch (MinorFunction)
	{
		case IRP_MN_START_DEVICE: /* 0x00 */
		{
			TRACE_(I8042PRT, "IRP_MJ_PNP / IRP_MN_START_DEVICE\n");

			/* Call lower driver (if any) */
			if (DeviceType != PhysicalDeviceObject)
			{
				Status = ForwardIrpAndWait(DeviceObject, Irp);
				if (NT_SUCCESS(Status))
					Status = i8042PnpStartDevice(
						DeviceObject,
						Stack->Parameters.StartDevice.AllocatedResources,
						Stack->Parameters.StartDevice.AllocatedResourcesTranslated);
			}
			else
				Status = STATUS_SUCCESS;
			break;
		}
		case IRP_MN_QUERY_DEVICE_RELATIONS: /* (optional) 0x07 */
		{
			switch (Stack->Parameters.QueryDeviceRelations.Type)
			{
				case BusRelations:
				{
					PDEVICE_RELATIONS DeviceRelations;

					TRACE_(I8042PRT, "IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / BusRelations\n");
					DeviceRelations = ExAllocatePoolWithTag(PagedPool, sizeof(DEVICE_RELATIONS), I8042PRT_TAG);
					if (DeviceRelations)
					{
						DeviceRelations->Count = 0;
						Information = (ULONG_PTR)DeviceRelations;
						Status = STATUS_SUCCESS;
					}
					else
						Status = STATUS_INSUFFICIENT_RESOURCES;
					break;
				}
				case RemovalRelations:
				{
					TRACE_(I8042PRT, "IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / RemovalRelations\n");
					return ForwardIrpAndForget(DeviceObject, Irp);
				}
				default:
					ERR_(I8042PRT, "IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / Unknown type 0x%lx\n",
						Stack->Parameters.QueryDeviceRelations.Type);
					ASSERT(FALSE);
					return ForwardIrpAndForget(DeviceObject, Irp);
			}
			break;
		}
		case IRP_MN_FILTER_RESOURCE_REQUIREMENTS: /* (optional) 0x0d */
		{
			TRACE_(I8042PRT, "IRP_MJ_PNP / IRP_MN_FILTER_RESOURCE_REQUIREMENTS\n");
			/* Nothing to do */
			Status = Irp->IoStatus.Status;
			break;
		}
		default:
		{
			ERR_(I8042PRT, "IRP_MJ_PNP / unknown minor function 0x%x\n", MinorFunction);
			ASSERT(FALSE);
			return ForwardIrpAndForget(DeviceObject, Irp);
		}
	}

	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}
