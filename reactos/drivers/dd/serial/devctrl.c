/* $Id:
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/dd/serial/devctrl.c
 * PURPOSE:         Serial IRP_MJ_DEVICE_CONTROL operations
 * 
 * PROGRAMMERS:     Hervé Poussineau (poussine@freesurf.fr)
 */
/* FIXME: call IoAcquireRemoveLock/IoReleaseRemoveLock around each I/O operation */

#define NDEBUG
#include "serial.h"

NTSTATUS STDCALL
SerialSetBaudRate(
	IN PSERIAL_DEVICE_EXTENSION DeviceExtension,
	IN ULONG NewBaudRate)
{
	USHORT divisor;
	PUCHAR ComPortBase = (PUCHAR)DeviceExtension->BaseAddress;
	NTSTATUS Status = STATUS_SUCCESS;
	
	if (NewBaudRate & SERIAL_BAUD_USER)
	{
		divisor = (USHORT)(BAUD_CLOCK / (CLOCKS_PER_BIT * (NewBaudRate & ~SERIAL_BAUD_USER)));
	}
	else
	{
		switch (NewBaudRate)
		{
			case SERIAL_BAUD_075:    divisor = 0x600; break;
			case SERIAL_BAUD_110:    divisor = 0x400; break;
			case SERIAL_BAUD_134_5:  divisor = 0x360; break;
			case SERIAL_BAUD_150:    divisor = 0x300; break;
			case SERIAL_BAUD_300:    divisor = 0x180; break;
			case SERIAL_BAUD_600:    divisor = 0xc0; break;
			case SERIAL_BAUD_1200:   divisor = 0x60; break;
			case SERIAL_BAUD_1800:   divisor = 0x40; break;
			case SERIAL_BAUD_2400:   divisor = 0x30; break;
			case SERIAL_BAUD_4800:   divisor = 0x18; break;
			case SERIAL_BAUD_7200:   divisor = 0x10; break;
			case SERIAL_BAUD_9600:   divisor = 0xc; break;
			case SERIAL_BAUD_14400:  divisor = 0x8; break;
			case SERIAL_BAUD_38400:  divisor = 0x3; break;
			case SERIAL_BAUD_57600:  divisor = 0x2; break;
			case SERIAL_BAUD_115200: divisor = 0x1; break;
			case SERIAL_BAUD_56K:    divisor = 0x2; break;
			case SERIAL_BAUD_128K:   divisor = 0x1; break;
			default: Status = STATUS_INVALID_PARAMETER;
		}
	}
	
	if (NT_SUCCESS(Status))
	{
		Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, (PVOID)DeviceExtension->ComPort);
		if (NT_SUCCESS(Status))
		{
			UCHAR Lcr;
			DPRINT("Serial: SerialSetBaudRate(COM%lu, %lu Bauds)\n", DeviceExtension->ComPort, BAUD_CLOCK / (CLOCKS_PER_BIT * divisor));
			/* FIXME: update DeviceExtension->LowerDevice when modifying LCR? */
			/* Set Bit 7 of LCR to expose baud registers */
			Lcr = READ_PORT_UCHAR(SER_LCR(ComPortBase));
			Lcr |= SR_LCR_DLAB;
			WRITE_PORT_UCHAR(SER_LCR(ComPortBase), Lcr);
			/* Write the baud rate */
			WRITE_PORT_UCHAR(SER_DLL(ComPortBase), divisor & 0xff);
			WRITE_PORT_UCHAR(SER_DLM(ComPortBase), divisor >> 8);
			/* Switch back to normal registers */
			Lcr ^= SR_LCR_DLAB;
			WRITE_PORT_UCHAR(SER_LCR(ComPortBase), Lcr);
			
			IoReleaseRemoveLock(&DeviceExtension->RemoveLock, (PVOID)DeviceExtension->ComPort);
		}
	}
	
	if (NT_SUCCESS(Status))
		DeviceExtension->BaudRate = NewBaudRate;
	return Status;
}

NTSTATUS STDCALL
SerialSetLineControl(
	IN PSERIAL_DEVICE_EXTENSION DeviceExtension,
	IN PSERIAL_LINE_CONTROL NewSettings)
{
	UCHAR Lcr = 0;
	NTSTATUS Status;
	
	DPRINT("Serial: SerialSetLineControl(COM%lu, Settings { %lu %lu %lu })\n",
		DeviceExtension->ComPort, NewSettings->StopBits, NewSettings->Parity, NewSettings->WordLength);
	
	/* Verify parameters */
	switch (NewSettings->WordLength)
	{
		case 5: Lcr |= SR_LCR_CS5; break;
		case 6: Lcr |= SR_LCR_CS6; break;
		case 7: Lcr |= SR_LCR_CS7; break;
		case 8: Lcr |= SR_LCR_CS8; break;
		default: return STATUS_INVALID_PARAMETER;
	}
	
	if (NewSettings->WordLength < 5 || NewSettings->WordLength > 8)
		return STATUS_INVALID_PARAMETER;
	
	switch (NewSettings->Parity)
	{
		case NO_PARITY:    Lcr |= SR_LCR_PNO; break;
		case ODD_PARITY:   Lcr |= SR_LCR_POD; break;
		case EVEN_PARITY:  Lcr |= SR_LCR_PEV; break;
		case MARK_PARITY:  Lcr |= SR_LCR_PMK; break;
		case SPACE_PARITY: Lcr |= SR_LCR_PSP; break;
		default: return STATUS_INVALID_PARAMETER;
	}
	
	switch (NewSettings->StopBits)
	{
		case STOP_BIT_1:
			Lcr |= SR_LCR_ST1;
			break;
		case STOP_BITS_1_5:
			if (NewSettings->WordLength != 5)
				return STATUS_INVALID_PARAMETER;
			Lcr |= SR_LCR_ST2;
			break;
		case STOP_BITS_2:
			if (NewSettings->WordLength < 6 || NewSettings->WordLength > 8)
				return STATUS_INVALID_PARAMETER;
			Lcr |= SR_LCR_ST2;
			break;
		default:
			return STATUS_INVALID_PARAMETER;
	}
	
	/* Update current parameters */
	Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, (PVOID)DeviceExtension->ComPort);
	if (!NT_SUCCESS(Status))
		return Status;
	WRITE_PORT_UCHAR(SER_LCR((PUCHAR)DeviceExtension->BaseAddress), Lcr);
	IoReleaseRemoveLock(&DeviceExtension->RemoveLock, (PVOID)DeviceExtension->ComPort);
	
	if (NT_SUCCESS(Status))
		DeviceExtension->SerialLineControl = *NewSettings;
	
	return Status;
}

NTSTATUS STDCALL
SerialDeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	PSERIAL_DEVICE_EXTENSION DeviceExtension;
	ULONG LengthIn, LengthOut;
	ULONG Information = 0;
	PUCHAR Buffer;
	PUCHAR ComPortBase;
	NTSTATUS Status;
	
	DPRINT("Serial: IRP_MJ_DEVICE_CONTROL dispatch\n");
	
	/* FIXME: pend operation if possible */
	
	Stack = IoGetCurrentIrpStackLocation(Irp);
	LengthIn = Stack->Parameters.DeviceIoControl.InputBufferLength;
	LengthOut = Stack->Parameters.DeviceIoControl.OutputBufferLength;
	Buffer = Irp->AssociatedIrp.SystemBuffer;
	DeviceExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	ComPortBase = (PUCHAR)DeviceExtension->BaseAddress;
	
	/* FIXME: need to probe buffers */
	/* FIXME: see http://www.osronline.com/ddkx/serial/serref_61bm.htm */
	switch (Stack->Parameters.DeviceIoControl.IoControlCode)
	{
		case IOCTL_SERIAL_CLEAR_STATS:
		{
			/* FIXME */
			DPRINT1("Serial: IOCTL_SERIAL_CLEAR_STATS not implemented.\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		case IOCTL_SERIAL_CLR_DTR:
		{
			DPRINT("Serial: IOCTL_SERIAL_CLR_DTR\n");
			/* FIXME: If the handshake flow control of the device is configured to 
			 * automatically use DTR, return STATUS_INVALID_PARAMETER */
			DeviceExtension->MCR &= ~SR_MCR_DTR;
			WRITE_PORT_UCHAR(SER_MCR(ComPortBase), DeviceExtension->MCR);
			Status = STATUS_SUCCESS;
			break;
		}
		case IOCTL_SERIAL_CLR_RTS:
		{
			DPRINT("Serial: IOCTL_SERIAL_CLR_RTS\n");
			/* FIXME: If the handshake flow control of the device is configured to 
			 * automatically use RTS, return STATUS_INVALID_PARAMETER */
			DeviceExtension->MCR &= ~SR_MCR_RTS;
			WRITE_PORT_UCHAR(SER_MCR(ComPortBase), DeviceExtension->MCR);
			Status = STATUS_SUCCESS;
			break;
		}
		case IOCTL_SERIAL_CONFIG_SIZE:
		{
			/* Obsolete on Microsoft Windows 2000+ */
			PULONG pConfigSize;
			DPRINT("Serial: IOCTL_SERIAL_CONFIG_SIZE\n");
			if (LengthOut != sizeof(ULONG) || Buffer == NULL)
				Status = STATUS_INVALID_PARAMETER;
			else
			{
				pConfigSize = (PULONG)Buffer;
				*pConfigSize = 0;
				Status = STATUS_SUCCESS;
			}
			break;
		}
		case IOCTL_SERIAL_GET_BAUD_RATE:
		{
			DPRINT("Serial: IOCTL_SERIAL_GET_BAUD_RATE\n");
			if (LengthOut < sizeof(SERIAL_BAUD_RATE))
				Status = STATUS_BUFFER_TOO_SMALL;
			else if (Buffer == NULL)
				Status = STATUS_INVALID_PARAMETER;
			else
			{
				((PSERIAL_BAUD_RATE)Buffer)->BaudRate = DeviceExtension->BaudRate;
				Information = sizeof(SERIAL_BAUD_RATE);
				Status = STATUS_SUCCESS;
			}
		}
		case IOCTL_SERIAL_GET_CHARS:
		{
			/* FIXME */
			DPRINT1("Serial: IOCTL_SERIAL_GET_CHARS not implemented.\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		case IOCTL_SERIAL_GET_COMMSTATUS:
		{
			/* FIXME */
			DPRINT1("Serial: IOCTL_SERIAL_GET_COMMSTATUS not implemented.\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		case IOCTL_SERIAL_GET_DTRRTS:
		{
			PULONG pDtrRts;
			DPRINT("Serial: IOCTL_SERIAL_GET_DTRRTS\n");
			if (LengthOut != sizeof(ULONG) || Buffer == NULL)
				Status = STATUS_INVALID_PARAMETER;
			else
			{
				pDtrRts = (PULONG)Buffer;
				*pDtrRts = 0;
				if (DeviceExtension->MCR & SR_MCR_DTR)
					*pDtrRts |= SERIAL_DTR_STATE;
				if (DeviceExtension->MCR & SR_MCR_RTS)
					*pDtrRts |= SERIAL_RTS_STATE;
				Status = STATUS_SUCCESS;
			}
			break;
		}
		case IOCTL_SERIAL_GET_HANDFLOW:
		{
			/* FIXME */
			DPRINT1("Serial: IOCTL_SERIAL_GET_HANDFLOW not implemented.\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		case IOCTL_SERIAL_GET_LINE_CONTROL:
		{
			DPRINT("Serial: IOCTL_SERIAL_GET_LINE_CONTROL\n");
			if (LengthOut < sizeof(SERIAL_LINE_CONTROL))
				Status = STATUS_BUFFER_TOO_SMALL;
			else if (Buffer == NULL)
				Status = STATUS_INVALID_PARAMETER;
			else
			{
				*((PSERIAL_LINE_CONTROL)Buffer) = DeviceExtension->SerialLineControl;
				Information = sizeof(SERIAL_LINE_CONTROL);
				Status = STATUS_SUCCESS;
			}
			break;
		}
		case IOCTL_SERIAL_GET_MODEM_CONTROL:
		{
			PULONG pMCR;
			DPRINT("Serial: IOCTL_SERIAL_GET_MODEM_CONTROL\n");
			if (LengthOut != sizeof(ULONG) || Buffer == NULL)
				Status = STATUS_INVALID_PARAMETER;
			else
			{
				pMCR = (PULONG)Buffer;
				*pMCR = DeviceExtension->MCR;
				Status = STATUS_SUCCESS;
			}
			break;
		}
		case IOCTL_SERIAL_GET_MODEMSTATUS:
		{
			PULONG pMSR;
			DPRINT("Serial: IOCTL_SERIAL_GET_MODEMSTATUS\n");
			if (LengthOut != sizeof(ULONG) || Buffer == NULL)
				Status = STATUS_INVALID_PARAMETER;
			else
			{
				pMSR = (PULONG)Buffer;
				*pMSR = DeviceExtension->MSR;
				Status = STATUS_SUCCESS;
			}
			break;
		}
		case IOCTL_SERIAL_GET_PROPERTIES:
		{
			/* FIXME */
			DPRINT1("Serial: IOCTL_SERIAL_GET_PROPERTIES not implemented.\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		case IOCTL_SERIAL_GET_STATS:
		{
			/* FIXME */
			DPRINT1("Serial: IOCTL_SERIAL_GET_STATS not implemented.\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		case IOCTL_SERIAL_GET_TIMEOUTS:
		{
			/* FIXME */
			DPRINT1("Serial: IOCTL_SERIAL_GET_TIMEOUTS not implemented.\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		case IOCTL_SERIAL_GET_WAIT_MASK:
		{
			PULONG pWaitMask;
			DPRINT("Serial: IOCTL_SERIAL_GET_WAIT_MASK\n");
			if (LengthOut != sizeof(ULONG) || Buffer == NULL)
				Status = STATUS_INVALID_PARAMETER;
			else
			{
				pWaitMask = (PULONG)Buffer;
				*pWaitMask = DeviceExtension->WaitMask;
				Status = STATUS_SUCCESS;
			}
			break;
		}
		case IOCTL_SERIAL_IMMEDIATE_CHAR:
		{
			/* FIXME */
			DPRINT1("Serial: IOCTL_SERIAL_IMMEDIATE_CHAR not implemented.\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		case IOCTL_SERIAL_LSRMST_INSERT:
		{
			/* FIXME */
			DPRINT1("Serial: IOCTL_SERIAL_LSRMST_INSERT not implemented.\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		case IOCTL_SERIAL_PURGE:
		{
			/* FIXME */
			DPRINT1("Serial: IOCTL_SERIAL_PURGE not implemented.\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		case IOCTL_SERIAL_RESET_DEVICE:
		{
			/* FIXME */
			DPRINT1("Serial: IOCTL_SERIAL_RESET_DEVICE not implemented.\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		case IOCTL_SERIAL_SET_BAUD_RATE:
		{
			PULONG pNewBaudRate;
			DPRINT("Serial: IOCTL_SERIAL_SET_BAUD_RATE\n");
			if (LengthIn != sizeof(ULONG) || Buffer == NULL)
				Status = STATUS_INVALID_PARAMETER;
			else
			{
				pNewBaudRate = (PULONG)Buffer;
				Status = SerialSetBaudRate(DeviceExtension, *pNewBaudRate);
			}
			break;
		}
		case IOCTL_SERIAL_SET_BREAK_OFF:
		{
			/* FIXME */
			DPRINT1("Serial: IOCTL_SERIAL_SET_BREAK_OFF not implemented.\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		case IOCTL_SERIAL_SET_BREAK_ON:
		{
			/* FIXME */
			DPRINT1("Serial: IOCTL_SERIAL_SET_BREAK_ON not implemented.\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		case IOCTL_SERIAL_SET_CHARS:
		{
			/* FIXME */
			DPRINT1("Serial: IOCTL_SERIAL_SET_CHARS not implemented.\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		case IOCTL_SERIAL_SET_DTR:
		{
			/* FIXME: If the handshake flow control of the device is configured to 
			 * automatically use DTR, return STATUS_INVALID_PARAMETER */
			DPRINT("Serial: IOCTL_SERIAL_SET_DTR\n");
			if (!(DeviceExtension->MCR & SR_MCR_DTR))
			{
				DeviceExtension->MCR |= SR_MCR_DTR;
				WRITE_PORT_UCHAR(SER_MCR(ComPortBase), DeviceExtension->MCR);
			}
			Status = STATUS_SUCCESS;
			break;
		}
		case IOCTL_SERIAL_SET_FIFO_CONTROL:
		{
			DPRINT("Serial: IOCTL_SERIAL_SET_FIFO_CONTROL\n");
			if (LengthIn != sizeof(ULONG) || Buffer == NULL)
				Status = STATUS_INVALID_PARAMETER;
			else
			{
				WRITE_PORT_UCHAR(SER_FCR(ComPortBase), (UCHAR)((*(PULONG)Buffer) & 0xff));
				Status = STATUS_SUCCESS;
			}
			break;
		}
		case IOCTL_SERIAL_SET_HANDFLOW:
		{
			/* FIXME */
			DPRINT1("Serial: IOCTL_SERIAL_SET_HANDFLOW not implemented.\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		case IOCTL_SERIAL_SET_LINE_CONTROL:
		{
			DPRINT("Serial: IOCTL_SERIAL_SET_LINE_CONTROL\n");
			if (LengthIn < sizeof(SERIAL_LINE_CONTROL))
				Status = STATUS_BUFFER_TOO_SMALL;
			else if (Buffer == NULL)
				Status = STATUS_INVALID_PARAMETER;
			else
				Status = SerialSetLineControl(DeviceExtension, (PSERIAL_LINE_CONTROL)Buffer);
			break;
		}
		case IOCTL_SERIAL_SET_MODEM_CONTROL:
		{
			PULONG pMCR;
			DPRINT("Serial: IOCTL_SERIAL_SET_MODEM_CONTROL\n");
			if (LengthIn != sizeof(ULONG) || Buffer == NULL)
				Status = STATUS_INVALID_PARAMETER;
			else
			{
				pMCR = (PULONG)Buffer;
				DeviceExtension->MCR = (UCHAR)(*pMCR & 0xff);
				WRITE_PORT_UCHAR(SER_MCR(ComPortBase), DeviceExtension->MCR);
				Status = STATUS_SUCCESS;
			}
			break;
		}
		case IOCTL_SERIAL_SET_QUEUE_SIZE:
		{
			/* FIXME */
			DPRINT1("Serial: IOCTL_SERIAL_SET_QUEUE_SIZE not implemented.\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		case IOCTL_SERIAL_SET_RTS:
		{
			/* FIXME: If the handshake flow control of the device is configured to 
			 * automatically use DTR, return STATUS_INVALID_PARAMETER */
			DPRINT("Serial: IOCTL_SERIAL_SET_RTS\n");
			if (!(DeviceExtension->MCR & SR_MCR_RTS))
			{
				DeviceExtension->MCR |= SR_MCR_RTS;
				WRITE_PORT_UCHAR(SER_MCR(ComPortBase), DeviceExtension->MCR);
			}
			Status = STATUS_SUCCESS;
			break;
		}
		case IOCTL_SERIAL_SET_TIMEOUTS:
		{
			/* FIXME */
			DPRINT1("Serial: IOCTL_SERIAL_SET_TIMEOUTS not implemented.\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		case IOCTL_SERIAL_SET_WAIT_MASK:
		{
			PULONG pWaitMask;
			DPRINT("Serial: IOCTL_SERIAL_SET_WAIT_MASK\n");
			if (LengthIn != sizeof(ULONG) || Buffer == NULL)
				Status = STATUS_INVALID_PARAMETER;
			else
			{
				pWaitMask = (PULONG)Buffer;
				DeviceExtension->WaitMask = *pWaitMask;
				Status = STATUS_SUCCESS;
			}
			break;
		}
		case IOCTL_SERIAL_SET_XOFF:
		{
			/* FIXME */
			DPRINT1("Serial: IOCTL_SERIAL_SET_XOFF not implemented.\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		case IOCTL_SERIAL_SET_XON:
		{
			/* FIXME */
			DPRINT1("Serial: IOCTL_SERIAL_SET_XON not implemented.\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		case IOCTL_SERIAL_WAIT_ON_MASK:
		{
			/* FIXME */
			DPRINT1("Serial: IOCTL_SERIAL_WAIT_ON_MASK not implemented.\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		case IOCTL_SERIAL_XOFF_COUNTER:
		{
			/* FIXME */
			DPRINT1("Serial: IOCTL_SERIAL_XOFF_COUNTER not implemented.\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		default:
		{
			/* Pass Irp to lower driver */
			DPRINT("Serial: Unknown IOCTL code 0x%x\n", Stack->Parameters.DeviceIoControl.IoControlCode);
			IoSkipCurrentIrpStackLocation(Irp);
			return IoCallDriver(DeviceExtension->LowerDevice, Irp);
		}
	}
	
	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}
