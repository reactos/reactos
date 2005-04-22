/* $Id:
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS VT100 emulator
 * FILE:            drivers/dd/green/screen.c
 * PURPOSE:         Screen part of green management
 * 
 * PROGRAMMERS:     Eric Kohl (ekohl@abo.rhein-zeitung.de)
 *                  Art Yerkes
 *                  Hervé Poussineau (hpoussin@reactos.com)
 */

#define NDEBUG
#include "green.h"
#include <stdarg.h>

#define ESC       ((UCHAR)0x1b)

/* Force a move of the cursor on each printer char.
 * Very useful for debug, but it is very slow...
 */
//#define FORCE_POSITION

/* UCHAR is promoted to int when passed through '...',
 * so we get int with va_arg and cast them back to UCHAR.
 */
static VOID
AddToSendBuffer(
	IN PSCREEN_DEVICE_EXTENSION DeviceExtension,
	IN ULONG NumberOfChars,
	... /* IN int */)
{
	PIRP Irp;
	IO_STATUS_BLOCK ioStatus;
	va_list args;
	PDEVICE_OBJECT SerialDevice;
	ULONG SizeLeft;
	int CurrentInt;
	UCHAR CurrentChar;
	NTSTATUS Status;
	
	SizeLeft = sizeof(DeviceExtension->SendBuffer) - DeviceExtension->SendBufferPosition;
	if (SizeLeft < NumberOfChars * 2 || NumberOfChars == 0)
	{
		SerialDevice = ((PGREEN_DEVICE_EXTENSION)DeviceExtension->Green->DeviceExtension)->Serial;
		Irp = IoBuildSynchronousFsdRequest(
			IRP_MJ_WRITE,
			SerialDevice,
			DeviceExtension->SendBuffer, DeviceExtension->SendBufferPosition,
			NULL, /* StartingOffset */
			NULL, /* Event */
			&ioStatus);
		if (!Irp)
		{
			DPRINT1("Green: IoBuildSynchronousFsdRequest() failed. Unable to flush output buffer\n");
			return;
		}
		Status = IoCallDriver(SerialDevice, Irp);
		if (!NT_SUCCESS(Status) && Status != STATUS_PENDING)
		{
			DPRINT1("Green: IoCallDriver() failed. Status = 0x%08lx\n", Status);
			return;
		}
		DeviceExtension->SendBufferPosition = 0;
		SizeLeft = sizeof(DeviceExtension->SendBuffer);
	}
	
	va_start(args, NumberOfChars);
	while (NumberOfChars-- > 0)
	{
		CurrentInt = va_arg(args, int);
		if (CurrentInt > 0)
		{
			CurrentChar = (UCHAR)CurrentInt;
			
			/* Why 0xff chars are printed on a 'dir' ? */
			if (CurrentChar == 0xff) CurrentChar = ' ';
			
			DeviceExtension->SendBuffer[DeviceExtension->SendBufferPosition++] = CurrentChar;
			SizeLeft--;
		}
		else if (CurrentInt == 0)
		{
			DeviceExtension->SendBuffer[DeviceExtension->SendBufferPosition++] = '0';
			SizeLeft--;
		}
		else
		{
			CurrentInt = -CurrentInt;
			ASSERT(CurrentInt < 100);
			if (CurrentInt >= 10)
			{
				DeviceExtension->SendBuffer[DeviceExtension->SendBufferPosition++] =
					(CurrentInt / 10) % 10 + '0';
				SizeLeft--;
			}
			DeviceExtension->SendBuffer[DeviceExtension->SendBufferPosition++] =
				CurrentInt % 10 + '0';
			SizeLeft--;
		}
	}
	va_end(args);
}

NTSTATUS
ScreenInitialize(
	IN PDRIVER_OBJECT DriverObject,
	OUT PDEVICE_OBJECT* ScreenFdo)
{
	PDEVICE_OBJECT Fdo;
	PSCREEN_DEVICE_EXTENSION DeviceExtension;
	UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\BlueScreen");
	NTSTATUS Status;
	
	DPRINT("Green: ScreenInitialize() called\n");
	
	Status = IoCreateDevice(DriverObject,
		sizeof(SCREEN_DEVICE_EXTENSION),
		&DeviceName, /* FIXME: don't hardcode string */
		FILE_DEVICE_SCREEN,
		FILE_DEVICE_SECURE_OPEN,
		TRUE,
		&Fdo);
	if (!NT_SUCCESS(Status))
		return Status;
	
	DeviceExtension = (PSCREEN_DEVICE_EXTENSION)Fdo->DeviceExtension;
	RtlZeroMemory(DeviceExtension, sizeof(SCREEN_DEVICE_EXTENSION));
	DeviceExtension->Common.Type = Screen;
	/* initialize screen */
	DeviceExtension->Columns = 80;
	DeviceExtension->Rows = 25;
	DeviceExtension->ScanLines = 16;
	DeviceExtension->VideoMemory = (PUCHAR)ExAllocatePool(
		PagedPool,
		2 * DeviceExtension->Columns * DeviceExtension->Rows * sizeof(UCHAR));
	if (!DeviceExtension->VideoMemory)
	{
		IoDeleteDevice(Fdo);
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	DeviceExtension->TabWidth = 8;
	
	/* more initialization */
	DeviceExtension->Mode = ENABLE_PROCESSED_OUTPUT |
		ENABLE_WRAP_AT_EOL_OUTPUT;
	
	/* initialize screen at next write */
	AddToSendBuffer(DeviceExtension, 2, ESC, 'c'); /* reset device */
	AddToSendBuffer(DeviceExtension, 4, ESC, '[', '7', 'l'); /* disable line wrap */
	AddToSendBuffer(DeviceExtension, 4, ESC, '[', '3', 'g'); /* clear all tabs */
	
	Fdo->Flags |= DO_POWER_PAGABLE | DO_BUFFERED_IO;
	Fdo->Flags &= ~DO_DEVICE_INITIALIZING;
	
	*ScreenFdo = Fdo;
	
	return STATUS_SUCCESS;
}

NTSTATUS
ScreenWrite(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	PUCHAR Buffer;
	PSCREEN_DEVICE_EXTENSION DeviceExtension;
	PUCHAR VideoMemory; /* FIXME: is it useful? */
	ULONG VideoMemorySize; /* FIXME: is it useful? */
	
	ULONG Columns, Rows;
	ULONG CursorX, CursorY;
	ULONG i, j;
	NTSTATUS Status;
	
	DPRINT("Green: IRP_MJ_WRITE\n");
	
	Stack = IoGetCurrentIrpStackLocation (Irp);
	Buffer = Irp->UserBuffer;
	DeviceExtension = (PSCREEN_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	VideoMemory = DeviceExtension->VideoMemory;
	
	Columns = DeviceExtension->Columns;
	Rows = DeviceExtension->Rows;
	CursorX = (DeviceExtension->LogicalOffset / 2) % Columns + 1;
	CursorY = (DeviceExtension->LogicalOffset / 2) / Columns + 1;
	VideoMemorySize = Columns * Rows * 2 * sizeof(UCHAR);
	DPRINT1("Y: %lu\n", CursorY);
	DPRINT1("Buffer =");
	for (i = 0; i < Stack->Parameters.Write.Length; i++)
		DbgPrint(" 0x%02x", Buffer[i]);
	DbgPrint("\n");
	
	if (!(DeviceExtension->Mode & ENABLE_PROCESSED_OUTPUT))
	{
		/* raw output mode */
		CHECKPOINT;
		Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
		IoCompleteRequest (Irp, IO_NO_INCREMENT);
		
		return STATUS_NOT_SUPPORTED;
	}
	else
	{
		for (i = 0; i < Stack->Parameters.Write.Length; i++, Buffer++)
		{
			switch (*Buffer)
			{
				case '\b':
				{
					if (CursorX > 1)
					{
						CursorX--;
						AddToSendBuffer(DeviceExtension, 6, ESC, '[', -(int)CursorY, ';', -(int)CursorX, 'H');
						AddToSendBuffer(DeviceExtension, 1, ' ');
						AddToSendBuffer(DeviceExtension, 6, ESC, '[', -(int)CursorY, ';', -(int)CursorX, 'H');
					}
					else if (CursorY > 1)
					{
						CursorX = Columns;
						CursorY--;
						AddToSendBuffer(DeviceExtension, 6, ESC, '[', -(int)CursorY, ';', -(int)CursorX, 'H');
					}
					break;
				}
				case '\n':
				{
					CursorY++;
					CursorX = 1;
					AddToSendBuffer(DeviceExtension, 6, ESC, '[', -(int)CursorY, ';', '1', 'H');
					break;
				}
				case '\r':
				{
					if (CursorX > 1)
					{
						AddToSendBuffer(DeviceExtension, 4, ESC, '[', -(int)(CursorX-1), 'D');
						CursorX = 1;
					}
					break;
				}
				case '\t':
				{
					ULONG Offset = DeviceExtension->TabWidth - (CursorX % DeviceExtension->TabWidth);
					for (j = 0; j < Offset; j++)
					{
#ifdef FORCE_POSITION
						AddToSendBuffer(DeviceExtension, 6, ESC, '[', -(int)CursorY, ';', -(int)CursorX, 'H');
#endif
						AddToSendBuffer(DeviceExtension, 1, ' ');
						CursorX++;
						if (CursorX > Columns)
						{
							CursorX = 1;
							CursorY++;
						}
					}
					break;
				}
				default:
				{
#ifdef FORCE_POSITION
					AddToSendBuffer(DeviceExtension, 6, ESC, '[', -(int)CursorY, ';', -(int)CursorX, 'H');
#endif
					AddToSendBuffer(DeviceExtension, 1, *Buffer);
					CursorX++;
					if (CursorX > Columns)
					{
						CursorX = 1;
						DPRINT1("Y: %lu -> %lu\n", CursorY, CursorY + 1);
						CursorY++;
						AddToSendBuffer(DeviceExtension, 6, ESC, '[', -(int)CursorY, ';', '1', 'H');
						
					}
				}
			}
			if (CursorY > Rows)
			{
				DPRINT1("Y: %lu -> %lu\n", CursorY, CursorY - 1);
				CursorY--;
				AddToSendBuffer(DeviceExtension, 2, ESC, 'D');
				AddToSendBuffer(DeviceExtension, 6, ESC, '[', -(int)CursorY, ';', -(int)CursorX, 'H');
			}
		}
	}
	
	DeviceExtension->LogicalOffset = ((CursorX-1) + (CursorY-1) * Columns) * 2;
	
	/* flush output buffer */
	AddToSendBuffer(DeviceExtension, 0);
	
	Status = STATUS_SUCCESS;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest (Irp, IO_NO_INCREMENT);
	
	return Status;
}

NTSTATUS
ScreenDeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	PSCREEN_DEVICE_EXTENSION DeviceExtension;
	NTSTATUS Status;
	
	Stack = IoGetCurrentIrpStackLocation(Irp);
	DeviceExtension = (PSCREEN_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	
	switch (Stack->Parameters.DeviceIoControl.IoControlCode)
	{
		case IOCTL_CONSOLE_GET_SCREEN_BUFFER_INFO:
		{
			PCONSOLE_SCREEN_BUFFER_INFO pcsbi;
			DPRINT("Green: IRP_MJ_DEVICE_CONTROL / IOCTL_CONSOLE_GET_SCREEN_BUFFER_INFO\n");
			
			pcsbi = (PCONSOLE_SCREEN_BUFFER_INFO)Irp->AssociatedIrp.SystemBuffer;
			
			pcsbi->dwSize.X = DeviceExtension->Columns;
			pcsbi->dwSize.Y = DeviceExtension->Rows;
			
			pcsbi->dwCursorPosition.X = (SHORT)(DeviceExtension->LogicalOffset % DeviceExtension->Columns);
			pcsbi->dwCursorPosition.Y = (SHORT)(DeviceExtension->LogicalOffset / DeviceExtension->Columns);
			
			pcsbi->wAttributes = DeviceExtension->CharAttribute;
			
			pcsbi->srWindow.Left   = 1;
			pcsbi->srWindow.Right  = DeviceExtension->Columns;
			pcsbi->srWindow.Top    = 1;
			pcsbi->srWindow.Bottom = DeviceExtension->Rows;
			
			pcsbi->dwMaximumWindowSize.X = DeviceExtension->Columns;
			pcsbi->dwMaximumWindowSize.Y = DeviceExtension->Rows;
			
			Irp->IoStatus.Information = sizeof(CONSOLE_SCREEN_BUFFER_INFO);
			Status = STATUS_SUCCESS;
			break;
		}
		case IOCTL_CONSOLE_SET_SCREEN_BUFFER_INFO:
		{
			PCONSOLE_SCREEN_BUFFER_INFO pcsbi;
			DPRINT("Green: IRP_MJ_DEVICE_CONTROL / IOCTL_CONSOLE_SET_SCREEN_BUFFER_INFO\n");
			
			pcsbi = (PCONSOLE_SCREEN_BUFFER_INFO)Irp->AssociatedIrp.SystemBuffer;
			/* FIXME: remove */ { pcsbi->dwCursorPosition.X++; }
			/* FIXME: remove */ { pcsbi->dwCursorPosition.Y++; }
			ASSERT(pcsbi->dwCursorPosition.X >= 1);
			ASSERT(pcsbi->dwCursorPosition.Y >= 1);
			ASSERT(pcsbi->dwCursorPosition.X <= DeviceExtension->Columns);
			ASSERT(pcsbi->dwCursorPosition.Y <= DeviceExtension->Rows);
			
			DeviceExtension->LogicalOffset = (
				(pcsbi->dwCursorPosition.Y-1) * DeviceExtension->Columns +
				(pcsbi->dwCursorPosition.X-1)) * 2;
			AddToSendBuffer(DeviceExtension, 6, ESC, '[',
				-(int)pcsbi->dwCursorPosition.Y, ';',
				-(int)pcsbi->dwCursorPosition.X, 'H');
			
			/* flush buffer */
			AddToSendBuffer(DeviceExtension, 0);
			
			DeviceExtension->CharAttribute = pcsbi->wAttributes;
			
			Irp->IoStatus.Information = 0;
			Status = STATUS_SUCCESS;
			break;
		}
		case IOCTL_CONSOLE_GET_CURSOR_INFO:
		{
			PCONSOLE_CURSOR_INFO pcci = (PCONSOLE_CURSOR_INFO)Irp->AssociatedIrp.SystemBuffer;
			DPRINT("Green: IRP_MJ_DEVICE_CONTROL / IOCTL_CONSOLE_GET_CURSOR_INFO\n");
			
			pcci->dwSize = 1;
			pcci->bVisible = TRUE;
			
			Irp->IoStatus.Information = sizeof (CONSOLE_CURSOR_INFO);
			Status = STATUS_SUCCESS;
			break;
		}
		case IOCTL_CONSOLE_GET_MODE:
		{
			PCONSOLE_MODE pcm = (PCONSOLE_MODE)Irp->AssociatedIrp.SystemBuffer;
			DPRINT("Green: IRP_MJ_DEVICE_CONTROL / IOCTL_CONSOLE_GET_MODE\n");
			
			pcm->dwMode = DeviceExtension->Mode;
			
			Irp->IoStatus.Information = sizeof(CONSOLE_MODE);
			Status = STATUS_SUCCESS;
			break;
		}
		case IOCTL_CONSOLE_SET_MODE:
		{
			PCONSOLE_MODE pcm = (PCONSOLE_MODE)Irp->AssociatedIrp.SystemBuffer;
			DPRINT("Green: IRP_MJ_DEVICE_CONTROL / IOCTL_CONSOLE_SET_MODE\n");
			
			DeviceExtension->Mode = pcm->dwMode;
			
			Irp->IoStatus.Information = 0;
			Status = STATUS_SUCCESS;
			break;
		}
		case IOCTL_CONSOLE_FILL_OUTPUT_ATTRIBUTE:
		{
			DPRINT1("Green: IRP_MJ_DEVICE_CONTROL / IOCTL_CONSOLE_FILL_OUTPUT_ATTRIBUTE\n");
			Status = STATUS_NOT_IMPLEMENTED; /* FIXME: IOCTL_CONSOLE_FILL_OUTPUT_ATTRIBUTE */
			break;
		}
		case IOCTL_CONSOLE_READ_OUTPUT_ATTRIBUTE:
		{
			DPRINT1("Green: IRP_MJ_DEVICE_CONTROL / IOCTL_CONSOLE_READ_OUTPUT_ATTRIBUTE\n");
			Status = STATUS_NOT_IMPLEMENTED; /* FIXME: IOCTL_CONSOLE_READ_OUTPUT_ATTRIBUTE */
			break;
		}
		case IOCTL_CONSOLE_WRITE_OUTPUT_ATTRIBUTE:
		{
			DPRINT1("Green: IRP_MJ_DEVICE_CONTROL / IOCTL_CONSOLE_WRITE_OUTPUT_ATTRIBUTE\n");
			Status = STATUS_NOT_IMPLEMENTED; /* FIXME: IOCTL_CONSOLE_WRITE_OUTPUT_ATTRIBUTE */
			break;
		}
		case IOCTL_CONSOLE_SET_TEXT_ATTRIBUTE:
		{
			DPRINT("Green: IRP_MJ_DEVICE_CONTROL / IOCTL_CONSOLE_SET_TEXT_ATTRIBUTE\n");
			
			DeviceExtension->CharAttribute = (WORD)*(PWORD)Irp->AssociatedIrp.SystemBuffer;
			Irp->IoStatus.Information = 0;
			Status = STATUS_SUCCESS;
			break;
		}
		case IOCTL_CONSOLE_FILL_OUTPUT_CHARACTER:
		{
			DPRINT1("Green: IRP_MJ_DEVICE_CONTROL / IOCTL_CONSOLE_FILL_OUTPUT_CHARACTER\n");
			Status = STATUS_NOT_IMPLEMENTED; /* FIXME:IOCTL_CONSOLE_FILL_OUTPUT_CHARACTER */
			break;
		}
		case IOCTL_CONSOLE_READ_OUTPUT_CHARACTER:
		{
			DPRINT1("Green: IRP_MJ_DEVICE_CONTROL / IOCTL_CONSOLE_READ_OUTPUT_CHARACTER\n");
			Status = STATUS_NOT_IMPLEMENTED; /* FIXME: IOCTL_CONSOLE_READ_OUTPUT_CHARACTER */
			break;
		}
		case IOCTL_CONSOLE_WRITE_OUTPUT_CHARACTER:
		{
			DPRINT1("Green: IRP_MJ_DEVICE_CONTROL / IOCTL_CONSOLE_WRITE_OUTPUT_CHARACTER\n");
			Status = STATUS_NOT_IMPLEMENTED; /* FIXME: IOCTL_CONSOLE_WRITE_OUTPUT_CHARACTER */
			break;
		}
		case IOCTL_CONSOLE_DRAW:
		{
			PCONSOLE_DRAW ConsoleDraw;
			PUCHAR Video;
			ULONG x, y;
			BOOLEAN DoOptimization = FALSE;
			DPRINT("Green: IRP_MJ_DEVICE_CONTROL / IOCTL_CONSOLE_DRAW\n");
			
			ConsoleDraw = (PCONSOLE_DRAW)MmGetSystemAddressForMdl(Irp->MdlAddress);
			/* FIXME: remove */ { ConsoleDraw->X++; ConsoleDraw->CursorX++; }
			/* FIXME: remove */ { ConsoleDraw->Y++; ConsoleDraw->CursorY++; }
			DPRINT1("%lu %lu %lu %lu\n",
				ConsoleDraw->X, ConsoleDraw->Y,
				ConsoleDraw->SizeX, ConsoleDraw->SizeY);
			ASSERT(ConsoleDraw->X >= 1);
			ASSERT(ConsoleDraw->Y >= 1);
			ASSERT(ConsoleDraw->X <= DeviceExtension->Columns);
			ASSERT(ConsoleDraw->Y <= DeviceExtension->Rows);
			ASSERT(ConsoleDraw->X + ConsoleDraw->SizeX >= 1);
			ASSERT(ConsoleDraw->Y + ConsoleDraw->SizeY >= 1);
			ASSERT(ConsoleDraw->X + ConsoleDraw->SizeX - 1 <= DeviceExtension->Columns);
			ASSERT(ConsoleDraw->Y + ConsoleDraw->SizeY - 1 <= DeviceExtension->Rows);
			ASSERT(ConsoleDraw->CursorX >= 1);
			ASSERT(ConsoleDraw->CursorY >= 1);
			ASSERT(ConsoleDraw->CursorX <= DeviceExtension->Columns);
			ASSERT(ConsoleDraw->CursorY <= DeviceExtension->Rows);
			
#if 0
			if (ConsoleDraw->X == 1
				&& ConsoleDraw->Y == 1
				&& ConsoleDraw->SizeX == DeviceExtension->Columns
				&& ConsoleDraw->SizeY == DeviceExtension->Rows)
			{
				CHECKPOINT1;
				/* search if we need to clear all screen */
				DoOptimization = TRUE;
				Video = (PUCHAR)(ConsoleDraw + 1);
				x = 0;
				while (DoOptimization && x < DeviceExtension->Columns * DeviceExtension->Rows)
				{
					if (Video[x++] != ' ')
					{
						CHECKPOINT1;
						DoOptimization = FALSE;
					}
					/*if (Video[x++] != DeviceExtension->CharAttribute) DoOptimization = FALSE; */
				}
				if (DoOptimization)
				{
					CHECKPOINT1;
					AddToSendBuffer(DeviceExtension, 4, ESC, '[', '2', 'J');
				}
			}
#endif
			/* add here more optimizations if needed */
			
			if (!DoOptimization)
			{
				for (y = 0; y < ConsoleDraw->SizeY; y++)
				{
					AddToSendBuffer(DeviceExtension, 6, ESC, '[',
						-(int)(ConsoleDraw->Y + y), ';',
						-(int)(ConsoleDraw->X), 'H');
					Video = (PUCHAR)(ConsoleDraw + 1);
					Video = &Video[((ConsoleDraw->Y + y) * DeviceExtension->Columns + ConsoleDraw->X) * 2];
					for (x = 0; x < ConsoleDraw->SizeX; x++)
					{
						AddToSendBuffer(DeviceExtension, 1, Video[x * 2]);
					}
				}
			}
			
			DeviceExtension->LogicalOffset = (
				(ConsoleDraw->CursorY-1) * DeviceExtension->Columns +
				(ConsoleDraw->CursorX-1)) * 2;
			AddToSendBuffer(DeviceExtension, 6, ESC, '[',
				-(int)(ConsoleDraw->CursorY), ';',
				-(int)(ConsoleDraw->CursorX), 'H');
			
			/* flush buffer */
			AddToSendBuffer(DeviceExtension, 0);
			
			Irp->IoStatus.Information = 0;
			Status = STATUS_SUCCESS;
			break;
		}
		default:
			DPRINT1("Green: IRP_MJ_DEVICE_CONTROL / unknown ioctl code 0x%lx\n",
				Stack->Parameters.DeviceIoControl.IoControlCode);
			Status = STATUS_NOT_IMPLEMENTED;
	}
	
	Irp->IoStatus.Status = Status;
	IoCompleteRequest (Irp, IO_NO_INCREMENT);
	return Status;
}
