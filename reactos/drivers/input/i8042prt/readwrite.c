/*
 * PROJECT:     ReactOS i8042 (ps/2 keyboard-mouse controller) driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/input/i8042prt/readwrite.c
 * PURPOSE:     Read/write port functions
 * PROGRAMMERS: Copyright Victor Kirhenshtein (sauros@iname.com)
                Copyright Jason Filby (jasonfilby@yahoo.com)
                Copyright Martijn Vernooij (o112w8r02@sneakemail.com)
                Copyright 2006-2007 Hervé Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "i8042prt.h"

/* FUNCTIONS *****************************************************************/

VOID
i8042Flush(
	IN PPORT_DEVICE_EXTENSION DeviceExtension)
{
	UCHAR Ignore;

	/* Flush output buffer */
	while (NT_SUCCESS(i8042ReadData(DeviceExtension, KBD_OBF /* | MOU_OBF*/, &Ignore))) {
		KeStallExecutionProcessor(50);
		TRACE_(I8042PRT, "Output data flushed\n");
	}

	/* Flush input buffer */
	while (NT_SUCCESS(i8042ReadData(DeviceExtension, KBD_IBF, &Ignore))) {
		KeStallExecutionProcessor(50);
		TRACE_(I8042PRT, "Input data flushed\n");
	}
}

BOOLEAN
i8042IsrWritePort(
	IN PPORT_DEVICE_EXTENSION DeviceExtension,
	IN UCHAR Value,
	IN UCHAR SelectCmd OPTIONAL)
{
	if (SelectCmd)
		if (!i8042Write(DeviceExtension, DeviceExtension->ControlPort, SelectCmd))
			return FALSE;

	return i8042Write(DeviceExtension, DeviceExtension->DataPort, Value);
}

/*
 * FUNCTION: Read data from port 0x60
 */
NTSTATUS
i8042ReadData(
	IN PPORT_DEVICE_EXTENSION DeviceExtension,
	IN UCHAR StatusFlags,
	OUT PUCHAR Data)
{
	UCHAR PortStatus;
	NTSTATUS Status;

	Status = i8042ReadStatus(DeviceExtension, &PortStatus);
	if (!NT_SUCCESS(Status))
		return Status;

	// If data is available
	if (PortStatus & StatusFlags)
	{
		*Data = READ_PORT_UCHAR(DeviceExtension->DataPort);
		INFO_(I8042PRT, "Read: 0x%02x (status: 0x%x)\n", Data[0], PortStatus);

		// If the data is valid (not timeout, not parity error)
		if ((PortStatus & KBD_PERR) == 0)
			return STATUS_SUCCESS;
	}
	return STATUS_UNSUCCESSFUL;
}

NTSTATUS
i8042ReadStatus(
	IN PPORT_DEVICE_EXTENSION DeviceExtension,
	OUT PUCHAR Status)
{
	ASSERT(DeviceExtension->ControlPort != NULL);
	*Status = READ_PORT_UCHAR(DeviceExtension->ControlPort);
	return STATUS_SUCCESS;
}

/*
 * FUNCTION: Read data from data port
 */
NTSTATUS
i8042ReadDataWait(
	IN PPORT_DEVICE_EXTENSION DeviceExtension,
	OUT PUCHAR Data)
{
	ULONG Counter;
	NTSTATUS Status;

	Counter = DeviceExtension->Settings.PollingIterations;

	while (Counter--)
	{
		Status = i8042ReadKeyboardData(DeviceExtension, Data);

		if (NT_SUCCESS(Status))
			return Status;

		KeStallExecutionProcessor(50);
	}

	/* Timed out */
	return STATUS_IO_TIMEOUT;
}

/*
 * This one reads a value from the port; You don't have to specify
 * which one, it'll always be from the one you talked to, so one function
 * is enough this time. Note how MSDN specifies the
 * WaitForAck parameter to be ignored.
 */
NTSTATUS NTAPI
i8042SynchReadPort(
	IN PVOID Context,
	OUT PUCHAR Value,
	IN BOOLEAN WaitForAck)
{
	PPORT_DEVICE_EXTENSION DeviceExtension;

	UNREFERENCED_PARAMETER(WaitForAck);

	DeviceExtension = (PPORT_DEVICE_EXTENSION)Context;

	return i8042ReadDataWait(DeviceExtension, Value);
}

/*
 * These functions are callbacks for filter driver custom
 * initialization routines.
 */
NTSTATUS NTAPI
i8042SynchWritePort(
	IN PPORT_DEVICE_EXTENSION DeviceExtension,
	IN UCHAR Port,
	IN UCHAR Value,
	IN BOOLEAN WaitForAck)
{
	NTSTATUS Status;
	UCHAR Ack;
	ULONG ResendIterations;

	ResendIterations = DeviceExtension->Settings.ResendIterations + 1;

	do
	{
		if (Port)
			if (!i8042Write(DeviceExtension, DeviceExtension->DataPort, Port))
			{
				WARN_(I8042PRT, "Failed to write Port\n");
				return STATUS_IO_TIMEOUT;
			}

		if (!i8042Write(DeviceExtension, DeviceExtension->DataPort, Value))
		{
			WARN_(I8042PRT, "Failed to write Value\n");
			return STATUS_IO_TIMEOUT;
		}

		if (WaitForAck)
		{
			Status = i8042ReadDataWait(DeviceExtension, &Ack);
			if (!NT_SUCCESS(Status))
			{
				WARN_(I8042PRT, "Failed to read Ack\n");
				return Status;
			}
			if (Ack == KBD_ACK)
				return STATUS_SUCCESS;
			else if (Ack == KBD_RESEND)
				INFO_(I8042PRT, "i8042 asks for a data resend\n");
		}
		else
		{
			return STATUS_SUCCESS;
		}
		TRACE_(I8042PRT, "Reiterating\n");
		ResendIterations--;
	} while (ResendIterations);

	return STATUS_IO_TIMEOUT;
}

/*
 * FUNCTION: Write data to a port, waiting first for it to become ready
 */
BOOLEAN
i8042Write(
	IN PPORT_DEVICE_EXTENSION DeviceExtension,
	IN PUCHAR addr,
	IN UCHAR data)
{
	ULONG Counter;

	ASSERT(addr);
	ASSERT(DeviceExtension->ControlPort != NULL);

	Counter = DeviceExtension->Settings.PollingIterations;

	while ((KBD_IBF & READ_PORT_UCHAR(DeviceExtension->ControlPort)) &&
	       (Counter--))
	{
		KeStallExecutionProcessor(50);
	}

	if (Counter)
	{
		WRITE_PORT_UCHAR(addr, data);
		INFO_(I8042PRT, "Sent 0x%x to port %p\n", data, addr);
		return TRUE;
	}
	return FALSE;
}
