/*
 * Mouse driver 0.0.6
 * Written by Jason Filby (jasonfilby@yahoo.com)
 * For ReactOS (www.reactos.com)
 *
 * Note: The serial.o driver must be loaded before loading this driver
 *
 * Known Limitations:
 * Only supports Microsoft mice on COM port 1
 *
 * Following information obtained from Tomi Engdahl (then@delta.hut.fi),
 * http://www.hut.fi/~then/mytexts/mouse.html
 *
 * Microsoft serial mouse
 *
 *   Serial data parameters:
 *     1200bps, 7 databits, 1 stop-bit
 *
 *   Data packet format:
 *     Data packet is 3 byte packet. It is send to the computer every time mouse
 *     state changes (mouse moves or keys are pressed/released). 
 *         D7      D6      D5      D4      D3      D2      D1      D0
 *     1.  X       1       LB      RB      Y7      Y6      X7      X6
 *     2.  X       0       X5      X4      X3      X2      X1      X0      
 *     3.  X       0       Y5      Y4      Y3      Y2      Y1      Y0
 *
 *     Note: The bit marked with X is 0 if the mouse received with 7 databits
 *     and 2 stop bits format. It is also possible to use 8 databits and 1 stop
 *     bit format for receiving. In this case X gets value 1. The safest thing
 *     to get everything working is to use 7 databits and 1 stopbit when
 *     receiving mouse information (and if you are making mouse then send out
 *     7 databits and 2 stop bits). 
 *     The byte marked with 1. is send first, then the others. The bit D6 in
 *     the first byte is used for syncronizing the software to mouse packets
 *     if it goes out of sync. 
 *
 *      LB is the state of the left button (1 means pressed down)
 *      RB is the state of the right button (1 means pressed down)
 *      X7-X0 movement in X direction since last packet (signed byte)
 *      Y7-Y0 movement in Y direction since last packet (signed byte)
 *
 *    Mouse identification
 *      When DTR line is toggled, mouse should send one data byte containing
 *      letter 'M' (ascii 77).
 *
 *
 * Logitech serial mouse
 *
 *   Logitech uses the Microsoft serial mouse protocol in their mouses (for
 *   example Logitech Pilot mouse and others). The origianal protocol supports
 *   only two buttons, but logitech as added third button to some of their
 *   mouse models. To make this possible logitech has made one extension to
 *   the protocol. 
 *   I have not seen any documentation about the exact documents, but here is
 *   what I have found out: The information of the third button state is sent
 *   using one extra byte which is send after the normal packet when needed.
 *   Value 32 (dec) is sent every time when the center button is pressed down.
 *   It is also sent every time with the data packet when center button is kept
 *   down and the mouse data packet is sent for other reasons. When center
 *   button is released, the mouse sends the normal data packet followed by
 *   data bythe which has value 0 (dec). As you can see the extra data byte
 *   is sent only when you mess with the center button.
 *
 *
 * Mouse systems mouse
 *
 *   Serial data parameters:
 *     1200bps, 8 databits, 1 stop-bit
 *
 *   Data packet format:
 *          D7      D6      D5      D4      D3      D2      D1      D0
 *     1.   1       0       0       0       0       LB      CB      RB
 *     2.   X7      X6      X5      X4      X3      X2      X1      X0
 *     3.   Y7      Y6      Y5      Y4      Y3      Y4      Y1      Y0
 *     4.   X7'     X6'     X5'     X4'     X3'     X2'     X1'     X0'
 *     5.   Y7'     Y6'     Y5'     Y4'     Y3'     Y4'     Y1'     Y0'
 *
 *     LB is left button state (0 = pressed, 1 = released)
 *     CB is center button state (0 = pressed, 1 = released)
 *     RB is right button state (0 = pressed, 1 = released)
 *     X7-X0 movement in X direction since last packet in signed byte 
 *           format (-128..+127), positive direction right
 *     Y7-Y0 movement in Y direction since last packet in signed byte 
 *           format (-128..+127), positive direction up
 *     X7'-X0' movement in X direction since sending of X7-X0 packet in
 *             signed byte format (-128..+127), positive direction right
 *     Y7'-Y0' movement in Y direction since sending of Y7-Y0 packet in
 *             signed byte format (-128..+127), positive direction up
 *
 *     The last two bytes in the packet (bytes 4 and 5) contains information
 *     about movement data changes which have occured after data bytes 2 and 3
 *     have been sent.
 *
 */

#include <ddk/ntddk.h>
#include <ddk/ntddmou.h>

#define MOUSE_IRQ_COM1  4
#define MOUSE_IRQ_COM2  3

#define MOUSE_PORT_COM1 0x3f8
#define MOUSE_PORT_COM2 0x2f8

typedef struct _DEVICE_EXTENSION {
  PDEVICE_OBJECT DeviceObject;
  ULONG ActiveQueue;
  ULONG InputDataCount[2];
  MOUSE_INPUT_DATA MouseInputData[2][MOUSE_BUFFER_SIZE];
  CLASS_INFORMATION ClassInformation;

  PKINTERRUPT MouseInterrupt;
  KDPC IsrDpc;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

static unsigned int MOUSE_IRQ = MOUSE_IRQ_COM1;
static unsigned int MOUSE_COM = MOUSE_PORT_COM1;

static unsigned int     bytepos=0, coordinate;
static unsigned char    mpacket[3];
static unsigned char    mouse_button1, mouse_button2;

// Previous button state
static ULONG PreviousButtons = 0;

BOOLEAN STDCALL
microsoft_mouse_handler(IN PKINTERRUPT Interrupt, PVOID ServiceContext)
{
  PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)ServiceContext;
  PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
  PMOUSE_INPUT_DATA Input;
  ULONG Queue, ButtonsDiff;
  unsigned int mbyte;
  int change_x;
  int change_y;
  UCHAR InterruptId = READ_PORT_UCHAR((PUCHAR)MOUSE_COM + 2);

  /* Is the interrupt for us? */
  if (0 != (InterruptId & 0x01))
  {
    return FALSE;
  }

  /* Not a Receive Data Available interrupt? */
  if (0 == (InterruptId & 0x04))
  {
    return TRUE;
  }

  /* Read all available data and process */
  while (0 != (READ_PORT_UCHAR((PUCHAR)MOUSE_COM + 5) & 0x01))
    {
    mbyte = READ_PORT_UCHAR((PUCHAR)MOUSE_COM);

    /* Synchronize */
    if (0x40 == (mbyte & 0x40))
      bytepos=0;

    mpacket[bytepos] = (mbyte & 0x7f);
    bytepos++;

    /* Process packet if complete */
    if (3 == bytepos)
    {
      /* Set local variables for DeviceObject and DeviceExtension */
      DeviceObject = (PDEVICE_OBJECT)ServiceContext;
      DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
      Queue = DeviceExtension->ActiveQueue % 2;

      /* Prevent buffer overflow */
      if (DeviceExtension->InputDataCount[Queue] == MOUSE_BUFFER_SIZE)
      {
	continue;
      }

      Input = &DeviceExtension->MouseInputData[Queue]
              [DeviceExtension->InputDataCount[Queue]];

      /* Retrieve change in x and y from packet */
      change_x = (int)(signed char)((mpacket[0] & 0x03) << 6) + mpacket[1];
      change_y = (int)(signed char)((mpacket[0] & 0x0c) << 4) + mpacket[2];

      /* Some mice need this */
      if (1 == coordinate)
      {
        change_x-=128;
        change_y-=128;
      }

#if 0
      /* Change to signed */
      if (128 <= change_x)
      {
	change_x = change_x - 256;
      }
      if (128 <= change_y)
      {
	change_y = change_y - 256;
      }
#endif

      Input->LastX = 2 * change_x;
      Input->LastY = - 3 * change_y;

      /* Retrieve mouse button status from packet */
      mouse_button1 = mpacket[0] & 0x20;
      mouse_button2 = mpacket[0] & 0x10;
    
      /* Determine the current state of the buttons */
      Input->RawButtons = mouse_button1 + mouse_button2;
    
      /* Determine ButtonFlags */
      Input->ButtonFlags = 0;
      ButtonsDiff = PreviousButtons ^ Input->RawButtons;

      if (0 != (ButtonsDiff & 0x20))
      {
	if (0 != (Input->RawButtons & 0x20))
	{
	  Input->ButtonFlags |= MOUSE_BUTTON_1_DOWN;
	}
	else
	{
	  Input->ButtonFlags |= MOUSE_BUTTON_1_UP;
	}
      }

      if (0 != (ButtonsDiff & 0x10))
      {
	if (0 != (Input->RawButtons & 0x10))
	{
	  Input->ButtonFlags |= MOUSE_BUTTON_2_DOWN;
	}
        else
	{
	  Input->ButtonFlags |= MOUSE_BUTTON_2_UP;
	}
      }

      bytepos=0;
    
      /* Send the Input data to the Mouse Class driver */
      DeviceExtension->InputDataCount[Queue]++;
      KeInsertQueueDpc(&DeviceExtension->IsrDpc, DeviceObject->CurrentIrp, NULL);

      /* Copy RawButtons to Previous Buttons for Input */
      PreviousButtons = Input->RawButtons;
    }
  }

  return TRUE;
}

void InitializeMouseHardware(unsigned int mtype)
{
  WRITE_PORT_UCHAR((PUCHAR)MOUSE_COM + 3, 0x80);  /* set DLAB on   */
  WRITE_PORT_UCHAR((PUCHAR)MOUSE_COM,     0x60);  /* speed LO byte */
  WRITE_PORT_UCHAR((PUCHAR)MOUSE_COM + 1, 0);     /* speed HI byte */
  WRITE_PORT_UCHAR((PUCHAR)MOUSE_COM + 3, mtype); /* 2=MS Mouse; 3=Mouse systems mouse */
  WRITE_PORT_UCHAR((PUCHAR)MOUSE_COM + 1, 0);     /* set comm and DLAB to 0 */
  WRITE_PORT_UCHAR((PUCHAR)MOUSE_COM + 4, 0x09);  /* DR int enable */

  (void) READ_PORT_UCHAR((PUCHAR)MOUSE_COM+5);    /* clear error bits */
}

int DetMicrosoft(void)
{
  char tmp, ind;
  int buttons=0, i, timeout=250;
  LARGE_INTEGER Timeout;

  WRITE_PORT_UCHAR((PUCHAR)MOUSE_COM+4, 0x0b);
  tmp=READ_PORT_UCHAR((PUCHAR)MOUSE_COM);

  /* Check the first four bytes for signs that this is an MS mouse */
  for(i=0; i<4; i++) {
    while(((READ_PORT_UCHAR((PUCHAR)MOUSE_COM+5) & 1)==0) && (timeout>0))
    {
      Timeout.QuadPart = 1;
      KeDelayExecutionThread (KernelMode, FALSE, &Timeout);
      timeout--;
    }
    ind=READ_PORT_UCHAR((PUCHAR)MOUSE_COM);
    if(ind==0x33) buttons=3;
    if(ind==0x4d) buttons=2;
  }

  return buttons;
}

int CheckMouseType(unsigned int mtype)
{
  unsigned int retval=0;

  InitializeMouseHardware(mtype);
  if(mtype==2) retval=DetMicrosoft();
  if(mtype==3) {
    WRITE_PORT_UCHAR((PUCHAR)MOUSE_COM+4, 11);
    retval=3;
  }
  WRITE_PORT_UCHAR((PUCHAR)MOUSE_COM+1, 1);

  return retval;
}

void ClearMouse(void)
{
  /* Waits until the mouse calms down but also quits out after a while
   * in case some destructive user wants to keep moving the mouse
   * before we're done */

  unsigned int restarts=0, i;
  for (i=0; i<60000; i++)
  {
    unsigned temp=READ_PORT_UCHAR((PUCHAR)MOUSE_COM);
    if(temp!=0) {
      restarts++;
      if(restarts<300000) {
        i=0;
      } else
      {
        i=60000;
      }
    }
  }
}

BOOLEAN InitializeMouse(PDEVICE_OBJECT DeviceObject)
{
  int mbuttons=0, gotmouse=0;
  PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
  ULONG MappedIrq;
  KIRQL Dirql;
  KAFFINITY Affinity;

  /* Check for Microsoft mouse (2 buttons) */
  if(CheckMouseType(2)!=0)
  {
    gotmouse=1;
    DbgPrint("Microsoft Mouse Detected\n");
    ClearMouse();
    coordinate=0;
  }

  /* Check for Microsoft Systems mouse (3 buttons) */
  if(gotmouse==0) {
    if(CheckMouseType(3)!=0)
    {
    gotmouse=1;
    DbgPrint("Mouse Systems Mouse Detected\n");
    ClearMouse();
    coordinate=1;
    }
  }

  if(gotmouse==0) return FALSE;

  DeviceExtension->ActiveQueue    = 0;
  MappedIrq = HalGetInterruptVector(Internal, 0, 0, MOUSE_IRQ, &Dirql, &Affinity);

  IoConnectInterrupt(&DeviceExtension->MouseInterrupt, microsoft_mouse_handler,
                     DeviceObject, NULL, MappedIrq, Dirql, Dirql, 0, FALSE,
                     Affinity, FALSE);

  return TRUE;
}

VOID SerialMouseInitializeDataQueue(PVOID Context)
{
}

BOOLEAN STDCALL
MouseSynchronizeRoutine(PVOID Context)
{
   PIRP Irp = (PIRP)Context;
   PMOUSE_INPUT_DATA rec  = (PMOUSE_INPUT_DATA)Irp->AssociatedIrp.SystemBuffer;
   PIO_STACK_LOCATION stk = IoGetCurrentIrpStackLocation(Irp);
   ULONG NrToRead         = stk->Parameters.Read.Length/sizeof(MOUSE_INPUT_DATA);
   int i;

   if ((stk->Parameters.Read.Length/sizeof(MOUSE_INPUT_DATA))==NrToRead)
   {
      return(TRUE);
   }

   return(FALSE);
}

VOID STDCALL
SerialMouseStartIo(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

   if (KeSynchronizeExecution(DeviceExtension->MouseInterrupt, MouseSynchronizeRoutine, Irp))
     {
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	IoStartNextPacket(DeviceObject, FALSE);
     }
}

NTSTATUS STDCALL
SerialMouseInternalDeviceControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
   PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   NTSTATUS status;

   switch(Stack->Parameters.DeviceIoControl.IoControlCode)
   {
      case IOCTL_INTERNAL_MOUSE_CONNECT:

         DeviceExtension->ClassInformation =
            *((PCLASS_INFORMATION)Stack->Parameters.DeviceIoControl.Type3InputBuffer);

         /* Reinitialize the port input data queue synchronously */
         KeSynchronizeExecution(DeviceExtension->MouseInterrupt,
            (PKSYNCHRONIZE_ROUTINE)SerialMouseInitializeDataQueue, DeviceExtension);

         status = STATUS_SUCCESS;
         break;

      default:
         status = STATUS_INVALID_DEVICE_REQUEST;
         break;
   }

   Irp->IoStatus.Status = status;
   if (status == STATUS_PENDING) {
      IoMarkIrpPending(Irp);
      IoStartPacket(DeviceObject, Irp, NULL, NULL);
   } else {
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
   }

   return status;
}

NTSTATUS STDCALL
SerialMouseDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION stk = IoGetCurrentIrpStackLocation(Irp);
   NTSTATUS Status;
   static BOOLEAN AlreadyOpened = FALSE;

   switch (stk->MajorFunction)
     {
      case IRP_MJ_CREATE:
	if (AlreadyOpened == TRUE)
	  {
	     Status = STATUS_SUCCESS;
	  }
	else
	  {
	     Status = STATUS_SUCCESS;
	     AlreadyOpened = TRUE;
	  }
	break;
	
      case IRP_MJ_CLOSE:
        Status = STATUS_SUCCESS;
	break;

      default:
        DbgPrint("NOT IMPLEMENTED\n");
        Status = STATUS_NOT_IMPLEMENTED;
	break;
     }

   if (Status==STATUS_PENDING)
     {
	IoMarkIrpPending(Irp);
     }
   else
     {
        Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
     }
   return(Status);
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

NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
  PDEVICE_OBJECT DeviceObject;
  UNICODE_STRING DeviceName;
  UNICODE_STRING SymlinkName;
  PDEVICE_EXTENSION DeviceExtension;

  DriverObject->MajorFunction[IRP_MJ_CREATE] = SerialMouseDispatch;
  DriverObject->MajorFunction[IRP_MJ_CLOSE]  = SerialMouseDispatch;
  DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = SerialMouseInternalDeviceControl;
  DriverObject->DriverStartIo                = SerialMouseStartIo;

  RtlInitUnicodeStringFromLiteral(&DeviceName,
                                  L"\\Device\\Mouse"); /* FIXME: find correct device name */
  IoCreateDevice(DriverObject,
	  sizeof(DEVICE_EXTENSION),
	  &DeviceName,
	  FILE_DEVICE_SERIAL_MOUSE_PORT,
	  0,
	  TRUE,
	  &DeviceObject);
  DeviceObject->Flags = DeviceObject->Flags | DO_BUFFERED_IO;

  if(InitializeMouse(DeviceObject) == TRUE)
  {
    DbgPrint("Serial Mouse Driver 0.0.5\n");
  } else {
    IoDeleteDevice(DeviceObject);
    DbgPrint("Serial mouse not found.\n");
    return STATUS_UNSUCCESSFUL;
  }

  RtlInitUnicodeStringFromLiteral(&SymlinkName,
                                  L"\\??\\Mouse"); /* FIXME: find correct device name */
  IoCreateSymbolicLink(&SymlinkName, &DeviceName);

  DeviceExtension = DeviceObject->DeviceExtension;
  KeInitializeDpc(&DeviceExtension->IsrDpc, (PKDEFERRED_ROUTINE)SerialMouseIsrDpc, DeviceObject);

  return(STATUS_SUCCESS);
}
