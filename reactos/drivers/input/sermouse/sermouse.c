/*
 * Serial Mouse driver 0.0.9
 * Written by Jason Filby (jasonfilby@yahoo.com)
 * And heavily rewritten by Filip Navara (xnavara@volny.cz)
 * For ReactOS (www.reactos.com)
 *
 * Technical information about mouse protocols can be found
 * in the file sermouse.txt.
 */

#include <ddk/ntddk.h>
#include <ddk/ntddmou.h>

/*
 * Compile time options
 */

/* Support for the IOCTL_MOUSE_QUERY_ATTRIBUTES I/O control code */
#define SERMOUSE_QUERYATTRIBUTES_SUPPORT
/* Check for mouse on COM1? */
#define SERMOUSE_COM1_SUPPORT
/* Check for mouse on COM2? */
#define SERMOUSE_COM2_SUPPORT
/* Create \??\Mouse* symlink for device? */
#define SERMOUSE_MOUSESYMLINK_SUPPORT

/*
 * Definitions
 */

#define MOUSE_IRQ_COM1			4
#define MOUSE_IRQ_COM2			3
#define MOUSE_PORT_COM1			0x3f8
#define MOUSE_PORT_COM2			0x2f8

/* Maximum value plus one for \Device\PointerClass* device name */
#define POINTER_PORTS_MAXIMUM	8
/* Letter count for POINTER_PORTS_MAXIMUM variable * sizeof(WCHAR) */
#define SUFFIX_MAXIMUM_SIZE		(1 * sizeof(WCHAR))

/* No Mouse */
#define MOUSE_TYPE_NONE			0
/* Microsoft Mouse with 2 buttons */
#define MOUSE_TYPE_MICROSOFT		1
/* Logitech Mouse with 3 buttons */
#define MOUSE_TYPE_LOGITECH		2
/* Microsoft Wheel Mouse (aka Z Mouse) */
#define MOUSE_TYPE_WHEELZ		3
/* Mouse Systems Mouse */
#define MOUSE_TYPE_MOUSESYSTEMS	4

/* Size for packet buffer used in interrupt routine */
#define PACKET_BUFFER_SIZE		4

/* Hardware byte mask for left button */
#define LEFT_BUTTON_MASK		0x20
/* Hardware to Microsoft specific code byte shift for left button */
#define LEFT_BUTTON_SHIFT		5
/* Hardware byte mask for right button */
#define RIGHT_BUTTON_MASK		0x10
/* Hardware to Microsoft specific code byte shift for right button */
#define RIGHT_BUTTON_SHIFT		3
/* Hardware byte mask for middle button */
#define MIDDLE_BUTTON_MASK		0x20
/* Hardware to Microsoft specific code byte shift for middle button */
#define MIDDLE_BUTTON_SHIFT		3

/* Microsoft byte mask for left button */
#define MOUSE_BUTTON_LEFT		0x01
/* Microsoft byte mask for right button */
#define MOUSE_BUTTON_RIGHT		0x02
/* Microsoft byte mask for middle button */
#define MOUSE_BUTTON_MIDDLE		0x04

/*
 * Structures
 */

typedef struct _DEVICE_EXTENSION {
	PDEVICE_OBJECT DeviceObject;
	ULONG ActiveQueue;
	ULONG InputDataCount[2];
	MOUSE_INPUT_DATA MouseInputData[2][MOUSE_BUFFER_SIZE];
	CLASS_INFORMATION ClassInformation;
	PKINTERRUPT MouseInterrupt;
	KDPC IsrDpc;
	ULONG MousePort;
	ULONG MouseType;
	UCHAR PacketBuffer[PACKET_BUFFER_SIZE];
	ULONG PacketBufferPosition;
	ULONG PreviousButtons;
#ifdef SERMOUSE_QUERYATTRIBUTES_SUPPORT
	MOUSE_ATTRIBUTES AttributesInformation;
#endif
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

/*
 * Functions
 */

void ClearMouse(ULONG Port)
{
	/* Waits until the mouse calms down but also quits out after a while
	 * in case some destructive user wants to keep moving the mouse
	 * before we're done */
	unsigned int Restarts = 0, i;
	for (i = 0; i < 60000; i++)
	{
    	unsigned Temp = READ_PORT_UCHAR((PUCHAR)Port);
	    if (Temp != 0)
    	{
			Restarts++;
			if (Restarts < 300000)
				i = 0;
			else
				i = 60000;
		}
	}
}

BOOLEAN STDCALL
SerialMouseInterruptService(IN PKINTERRUPT Interrupt, PVOID ServiceContext)
{
	PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)ServiceContext;
	PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
	UCHAR *PacketBuffer = DeviceExtension->PacketBuffer;
	ULONG MousePort = DeviceExtension->MousePort;
	UCHAR InterruptId = READ_PORT_UCHAR((PUCHAR)MousePort + 2);
	UCHAR RecievedByte;
	ULONG Queue;
	PMOUSE_INPUT_DATA Input;
	ULONG ButtonsDifference;

	/* Is the interrupt for us? */
	if ((InterruptId & 0x01) == 0x01)
	{
		return FALSE;
	}

	/* Not a Receive Data Available interrupt? */
	if ((InterruptId & 0x04) == 0)
	{
		return TRUE;
	}

	/* Read all available data and process */
	while ((READ_PORT_UCHAR((PUCHAR)MousePort + 5) & 0x01) != 0)
	{
		RecievedByte = READ_PORT_UCHAR((PUCHAR)MousePort);

		/* Synchronize */
		if ((RecievedByte & 0x40) == 0x40)
			DeviceExtension->PacketBufferPosition = 0;

		PacketBuffer[DeviceExtension->PacketBufferPosition] =
			(RecievedByte & 0x7f);
		++DeviceExtension->PacketBufferPosition;

		/* Process packet if complete */
		if (DeviceExtension->PacketBufferPosition >= 3)
		{
			Queue = DeviceExtension->ActiveQueue % 2;
	
			/* Prevent buffer overflow */
			if (DeviceExtension->InputDataCount[Queue] == MOUSE_BUFFER_SIZE)
				continue;

			Input = &DeviceExtension->MouseInputData[Queue][DeviceExtension->InputDataCount[Queue]];

			if (DeviceExtension->PacketBufferPosition == 3)
			{
				/* Retrieve change in x and y from packet */
    	        Input->LastX = (signed char)(PacketBuffer[1] | ((PacketBuffer[0] & 0x03) << 6));
        	    Input->LastY = (signed char)(PacketBuffer[2] | ((PacketBuffer[0] & 0x0c) << 4));
	
				/* Determine the current state of the buttons */
				Input->RawButtons = (DeviceExtension->PreviousButtons & MOUSE_BUTTON_MIDDLE) |
					((UCHAR)(PacketBuffer[0] & LEFT_BUTTON_MASK) >> LEFT_BUTTON_SHIFT) |
					((UCHAR)(PacketBuffer[0] & RIGHT_BUTTON_MASK) >> RIGHT_BUTTON_SHIFT);
			} else
			if (DeviceExtension->PacketBufferPosition == 4)
			{
				DeviceExtension->PacketBufferPosition = 0;
				/* If middle button state changed than report event */
				if (((UCHAR)(PacketBuffer[3] & MIDDLE_BUTTON_MASK) >> MIDDLE_BUTTON_SHIFT) ^
					(DeviceExtension->PreviousButtons & MOUSE_BUTTON_MIDDLE))
				{
					Input->RawButtons ^= MOUSE_BUTTON_MIDDLE;
					Input->LastX = 0;
					Input->LastY = 0;
				}
				else
				{
					continue;
				}
			}

			/* Determine ButtonFlags */
			Input->ButtonFlags = 0;
			ButtonsDifference = DeviceExtension->PreviousButtons ^ Input->RawButtons;

			if (ButtonsDifference != 0)
			{
				if (ButtonsDifference & MOUSE_BUTTON_LEFT)
				{
					if (Input->RawButtons & MOUSE_BUTTON_LEFT)
						Input->ButtonFlags |= MOUSE_LEFT_BUTTON_DOWN;
					else
						Input->ButtonFlags |= MOUSE_LEFT_BUTTON_UP;
				}
				if (ButtonsDifference & MOUSE_BUTTON_RIGHT)
				{
					if (Input->RawButtons & MOUSE_BUTTON_RIGHT)
						Input->ButtonFlags |= MOUSE_RIGHT_BUTTON_DOWN;
					else
						Input->ButtonFlags |= MOUSE_RIGHT_BUTTON_UP;
				}
				if (ButtonsDifference & MOUSE_BUTTON_MIDDLE)
				{
					if (Input->RawButtons & MOUSE_BUTTON_MIDDLE)
						Input->ButtonFlags |= MOUSE_MIDDLE_BUTTON_DOWN;
					else
						Input->ButtonFlags |= MOUSE_MIDDLE_BUTTON_UP;
				}
			}

			/* Send the Input data to the Mouse Class driver */
			DeviceExtension->InputDataCount[Queue]++;
			KeInsertQueueDpc(&DeviceExtension->IsrDpc, DeviceObject->CurrentIrp, NULL);

			/* Copy RawButtons to Previous Buttons for Input */
			DeviceExtension->PreviousButtons = Input->RawButtons;
		}
	}

	return TRUE;
}

VOID
SerialMouseInitializeDataQueue(PVOID Context)
{
}

BOOLEAN STDCALL
MouseSynchronizeRoutine(PVOID Context)
{
	return TRUE;
}

VOID STDCALL
SerialMouseStartIo(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

	if (KeSynchronizeExecution(DeviceExtension->MouseInterrupt, MouseSynchronizeRoutine, Irp))
	{
        	KIRQL oldIrql;
		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	        oldIrql = KeGetCurrentIrql();
                if (oldIrql < DISPATCH_LEVEL)
                  {
                    KeRaiseIrql (DISPATCH_LEVEL, &oldIrql);
                    IoStartNextPacket (DeviceObject, FALSE);
                    KeLowerIrql(oldIrql);
	          }
                else
                  {
                    IoStartNextPacket (DeviceObject, FALSE);
	          }
	}
}

NTSTATUS STDCALL
SerialMouseInternalDeviceControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
	PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS Status;

	switch (Stack->Parameters.DeviceIoControl.IoControlCode)
	{
		case IOCTL_INTERNAL_MOUSE_CONNECT:
			DeviceExtension->ClassInformation =
				*((PCLASS_INFORMATION)Stack->Parameters.DeviceIoControl.Type3InputBuffer);

			/* Reinitialize the port input data queue synchronously */
			KeSynchronizeExecution(DeviceExtension->MouseInterrupt,
				(PKSYNCHRONIZE_ROUTINE)SerialMouseInitializeDataQueue,
				DeviceExtension);

			Status = STATUS_SUCCESS;
			break;

#ifdef SERMOUSE_QUERYATTRIBUTES_SUPPORT
		case IOCTL_MOUSE_QUERY_ATTRIBUTES:
			if (Stack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(MOUSE_ATTRIBUTES))
			{
				*(PMOUSE_ATTRIBUTES)Irp->AssociatedIrp.SystemBuffer =
					DeviceExtension->AttributesInformation;
				Irp->IoStatus.Information = sizeof(MOUSE_ATTRIBUTES);
                Status = STATUS_SUCCESS;				
            } else {
				Status = STATUS_BUFFER_TOO_SMALL;
			}
			break;
#endif

		default:
			Status = STATUS_INVALID_DEVICE_REQUEST;
			break;
	}

	Irp->IoStatus.Status = Status;
	if (Status == STATUS_PENDING)
	{
		IoMarkIrpPending(Irp);
		IoStartPacket(DeviceObject, Irp, NULL, NULL);
	}
	else
	{
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}

	return Status;
}

NTSTATUS STDCALL
SerialMouseDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS Status;

	switch (Stack->MajorFunction)
	{
		case IRP_MJ_CREATE:
		case IRP_MJ_CLOSE:
			Status = STATUS_SUCCESS;
			break;

		default:
			DbgPrint("NOT IMPLEMENTED\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;
	}

	if (Status == STATUS_PENDING)
	{
		IoMarkIrpPending(Irp);
	}
	else
	{
		Irp->IoStatus.Status = Status;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

	return Status;
}

VOID SerialMouseIsrDpc(PKDPC Dpc, PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
   PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
   ULONG Queue;

   Queue = DeviceExtension->ActiveQueue % 2;
   InterlockedIncrement(&DeviceExtension->ActiveQueue);
   (*(PSERVICE_CALLBACK_ROUTINE)DeviceExtension->ClassInformation.CallBack)(
			DeviceExtension->ClassInformation.DeviceObject,
			DeviceExtension->MouseInputData[Queue],
			NULL,
			&DeviceExtension->InputDataCount[Queue]);

   DeviceExtension->InputDataCount[Queue] = 0;
}

void InitializeSerialPort(ULONG Port, unsigned int LineControl)
{
	WRITE_PORT_UCHAR((PUCHAR)Port + 3, 0x80);  /* set DLAB on   */
	WRITE_PORT_UCHAR((PUCHAR)Port,     0x60);  /* speed LO byte */
	WRITE_PORT_UCHAR((PUCHAR)Port + 1, 0);     /* speed HI byte */
	WRITE_PORT_UCHAR((PUCHAR)Port + 3, LineControl);
	WRITE_PORT_UCHAR((PUCHAR)Port + 1, 0);     /* set comm and DLAB to 0 */
	WRITE_PORT_UCHAR((PUCHAR)Port + 4, 0x09);  /* DR int enable */
	(void) READ_PORT_UCHAR((PUCHAR)Port + 5);  /* clear error bits */
}

ULONG DetectMicrosoftMouse(ULONG Port)
{
	CHAR Buffer[4];
	ULONG i;
	ULONG TimeOut = 250;
	UCHAR LineControl;
    
	/* Save original control */ 
	LineControl = READ_PORT_UCHAR((PUCHAR)Port + 4);

	/* Inactivate RTS and set DTR */
	WRITE_PORT_UCHAR((PUCHAR)Port + 4, (LineControl & ~0x02) | 0x01);
	KeStallExecutionProcessor(150000);      /* Wait > 100 ms */

	/* Clear buffer */
	while (READ_PORT_UCHAR((PUCHAR)Port + 5) & 0x01)
		(void)READ_PORT_UCHAR((PUCHAR)Port);

	/*
	 * Send modem control with 'Data Terminal Ready' and 'Request To Send'
	 * message. This enables mouse to identify.
	 */
	WRITE_PORT_UCHAR((PUCHAR)Port + 4, 0x03);

	/* Wait 10 milliseconds for the mouse getting ready */
	KeStallExecutionProcessor(10000);

	/* Read first four bytes, which contains Microsoft Mouse signs */
	for (i = 0; i < 4; i++)
	{
		while (((READ_PORT_UCHAR((PUCHAR)Port + 5) & 1) == 0) && (TimeOut > 0))
		{
			KeStallExecutionProcessor(1000);
			--TimeOut;
			if (TimeOut == 0)
				return MOUSE_TYPE_NONE;
		}
		Buffer[i] = READ_PORT_UCHAR((PUCHAR)Port);
	}

        /* Restore control */
        WRITE_PORT_UCHAR((PUCHAR)Port + 4, LineControl);

	/* Check that four bytes for signs */
	for (i = 0; i < 4; ++i)
	{
		/* Sign for Microsoft Ballpoint */
    		if (Buffer[i] == 'B')
		{
			DbgPrint("Microsoft Ballpoint device detected");
			DbgPrint("THIS DEVICE IS NOT SUPPORTED, YET");
			return MOUSE_TYPE_NONE;
		} else
		/* Sign for Microsoft Mouse protocol followed by button specifier */
		if (Buffer[i] == 'M')
		{
			if (i == 3)
			{
				/* Overflow Error */
				return MOUSE_TYPE_NONE;
			}
			switch (Buffer[i + 1])
			{
				case '3':
					DbgPrint("Microsoft Mouse with 3-buttons detected\n");
					return MOUSE_TYPE_LOGITECH;
				case 'Z':
					DbgPrint("Microsoft Wheel Mouse detected\n");
					return MOUSE_TYPE_WHEELZ;
				/* case '2': */
				default:
					DbgPrint("Microsoft Mouse with 2-buttons detected\n");
					return MOUSE_TYPE_MICROSOFT;
			}
		}
	}

	return MOUSE_TYPE_NONE;
}

PDEVICE_OBJECT
AllocatePointerDevice(PDRIVER_OBJECT DriverObject)
{
	PDEVICE_OBJECT DeviceObject;
	UNICODE_STRING DeviceName;
	UNICODE_STRING SuffixString;
	UNICODE_STRING SymlinkName;
	PDEVICE_EXTENSION DeviceExtension;
	ULONG Suffix;
	NTSTATUS Status;

	/* Allocate buffer for full device name */   
	RtlInitUnicodeString(&DeviceName, NULL);
	DeviceName.MaximumLength = sizeof(DD_MOUSE_DEVICE_NAME_U) + SUFFIX_MAXIMUM_SIZE + sizeof(UNICODE_NULL);
	DeviceName.Buffer = ExAllocatePool(PagedPool, DeviceName.MaximumLength);
	RtlAppendUnicodeToString(&DeviceName, DD_MOUSE_DEVICE_NAME_U);

	/* Allocate buffer for device name suffix */
	RtlInitUnicodeString(&SuffixString, NULL);
	SuffixString.MaximumLength = SUFFIX_MAXIMUM_SIZE + sizeof(UNICODE_NULL);
	SuffixString.Buffer = ExAllocatePool(PagedPool, SuffixString.MaximumLength);

	/* Generate full qualified name with suffix */
	for (Suffix = 0; Suffix < POINTER_PORTS_MAXIMUM; ++Suffix)
	{
		RtlIntegerToUnicodeString(Suffix, 10, &SuffixString);
		RtlAppendUnicodeToString(&DeviceName, SuffixString.Buffer);
		Status = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION),
			&DeviceName, FILE_DEVICE_SERIAL_MOUSE_PORT, 0, TRUE, &DeviceObject);
		/* Device successfully created, leave the cyclus */
		if (NT_SUCCESS(Status))
			break;
		DeviceName.Length -= SuffixString.Length;
	}
 
	ExFreePool(DeviceName.Buffer);

	/* Couldn't create device */
	if (!NT_SUCCESS(Status))
	{
		ExFreePool(SuffixString.Buffer);
		return NULL;
	}

	DeviceObject->Flags = DeviceObject->Flags | DO_BUFFERED_IO;

#ifdef SERMOUSE_MOUSESYMLINK_SUPPORT
	/* Create symlink */
	/* FIXME: Why? FiN 20/08/2003 */
	RtlInitUnicodeString(&SymlinkName, NULL);
	SymlinkName.MaximumLength = sizeof(L"\\??\\Mouse") + SUFFIX_MAXIMUM_SIZE + sizeof(UNICODE_NULL);
	SymlinkName.Buffer = ExAllocatePool(PagedPool, SymlinkName.MaximumLength);
	RtlAppendUnicodeToString(&SymlinkName, L"\\??\\Mouse");
	RtlAppendUnicodeToString(&DeviceName, SuffixString.Buffer);
	IoCreateSymbolicLink(&SymlinkName, &DeviceName);
#endif
	ExFreePool(SuffixString.Buffer);

	DeviceExtension = DeviceObject->DeviceExtension;
	KeInitializeDpc(&DeviceExtension->IsrDpc, (PKDEFERRED_ROUTINE)SerialMouseIsrDpc, DeviceObject);

	return DeviceObject;
}

BOOLEAN
InitializeMouse(ULONG Port, ULONG Irq, PDRIVER_OBJECT DriverObject)
{
	PDEVICE_EXTENSION DeviceExtension;
	PDEVICE_OBJECT DeviceObject;
	ULONG MappedIrq;
	KIRQL Dirql;
	KAFFINITY Affinity;
	ULONG MouseType;

	/* Try to detect mouse on specified port */
	InitializeSerialPort(Port, 2);
	MouseType = DetectMicrosoftMouse(Port);

	/* Enable interrupts */
	WRITE_PORT_UCHAR((PUCHAR)(Port) + 1, 1);
	ClearMouse(Port);

	/* No mouse, no need to continue */
	if (MouseType == MOUSE_TYPE_NONE)
		return FALSE;

	/* Allocate new device */
	DeviceObject = AllocatePointerDevice(DriverObject);
	if (!DeviceObject)
	{
		DbgPrint("Oops, couldn't creat device object.\n");
		return FALSE;
	}
	DeviceExtension = DeviceObject->DeviceExtension;

	/* Setup device extension structure */
	DeviceExtension->ActiveQueue = 0;
	DeviceExtension->MouseType = MouseType;
	DeviceExtension->MousePort = Port;
	DeviceExtension->PacketBufferPosition = 0;
	DeviceExtension->PreviousButtons = 0;
#ifdef SERMOUSE_QUERYATTRIBUTES_SUPPORT
	switch (MouseType)
	{
		case MOUSE_TYPE_MICROSOFT:
			DeviceExtension->AttributesInformation.MouseIdentifier = MOUSE_SERIAL_HARDWARE;
			DeviceExtension->AttributesInformation.NumberOfButtons = 2;
			break;
		case MOUSE_TYPE_LOGITECH:
			DeviceExtension->AttributesInformation.MouseIdentifier = MOUSE_SERIAL_HARDWARE;
			DeviceExtension->AttributesInformation.NumberOfButtons = 3;
			break;
		case MOUSE_TYPE_WHEELZ:
			DeviceExtension->AttributesInformation.MouseIdentifier = WHEELMOUSE_SERIAL_HARDWARE;
			DeviceExtension->AttributesInformation.NumberOfButtons = 3;
			break;
	};
	DeviceExtension->AttributesInformation.SampleRate = 40;
	DeviceExtension->AttributesInformation.InputDataQueueLength = MOUSE_BUFFER_SIZE;
#endif

	MappedIrq = HalGetInterruptVector(Internal, 0, 0, Irq, &Dirql, &Affinity);

	IoConnectInterrupt(
		&DeviceExtension->MouseInterrupt, SerialMouseInterruptService,
		DeviceObject, NULL, MappedIrq, Dirql, Dirql, 0, FALSE,
		Affinity, FALSE);		

	return TRUE;
}

NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	BOOL MouseFound = FALSE;

	DbgPrint("Serial Mouse Driver 0.0.9\n");
#ifdef SERMOUSE_COM1_SUPPORT
	DbgPrint("Trying to find mouse on COM1\n");
	MouseFound |= InitializeMouse(MOUSE_PORT_COM1, MOUSE_IRQ_COM1, DriverObject);
#endif
#ifdef SERMOUSE_COM2_SUPPORT
	DbgPrint("Trying to find mouse on COM2\n");
	MouseFound |= InitializeMouse(MOUSE_PORT_COM2, MOUSE_IRQ_COM2, DriverObject);
#endif

	if (!MouseFound)
	{
		DbgPrint("No serial mouse found.\n");
		return STATUS_UNSUCCESSFUL;
	}

	DriverObject->MajorFunction[IRP_MJ_CREATE] = (PDRIVER_DISPATCH)SerialMouseDispatch;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = (PDRIVER_DISPATCH)SerialMouseDispatch;
	DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = (PDRIVER_DISPATCH)SerialMouseInternalDeviceControl;
	DriverObject->DriverStartIo = SerialMouseStartIo;

	return STATUS_SUCCESS;
}
