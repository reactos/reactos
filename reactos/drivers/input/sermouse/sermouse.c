/*
 ** Mouse driver 0.0.5
 ** Written by Jason Filby (jasonfilby@yahoo.com)
 ** For ReactOS (www.reactos.com)

 ** Note: The serial.o driver must be loaded before loading this driver

 ** Known Limitations:
 ** Only supports mice on COM port 1
*/

#include <ddk/ntddk.h>
#include <ddk/ntddmou.h>
#include "sermouse.h"
#include "mouse.h"

#define MOUSE_IRQ_COM1  4
#define MOUSE_IRQ_COM2  3

#define COM1_PORT       0x3f8
#define COM2_PORT       0x2f8

#define max_screen_x    79
#define max_screen_y    24

//static unsigned int MOUSE_IRQ=MOUSE_IRQ_COM1;
static unsigned int MOUSE_COM=COM1_PORT;

static unsigned int     bytepos=0, coordinate;
static unsigned char    mpacket[3];
static signed int       mouse_x=40, mouse_y=12;
static unsigned char    mouse_button1, mouse_button2;
static signed int       horiz_sensitivity, vert_sensitivity;

// Previous button state
static ULONG PreviousButtons = 0;

BOOLEAN microsoft_mouse_handler(PKINTERRUPT Interrupt, PVOID ServiceContext)
{
  PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)ServiceContext;
  PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
  PMOUSE_INPUT_DATA Input;
  ULONG Queue, ButtonsDiff;
  unsigned int mbyte=READ_PORT_UCHAR((PUCHAR)MOUSE_COM);

  // Synchronize
  if((mbyte&64)==64)
    bytepos=0;

  mpacket[bytepos]=mbyte;
  bytepos++;

  // Process packet
  if(bytepos==3) {
    // Set local variables for DeviceObject and DeviceExtension
    DeviceObject = (PDEVICE_OBJECT)ServiceContext;
    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    Queue = DeviceExtension->ActiveQueue % 2;

    // Prevent buffer overflow
    if (DeviceExtension->InputDataCount[Queue] == MOUSE_BUFFER_SIZE)
    {
      return TRUE;
    }

    Input = &DeviceExtension->MouseInputData[Queue]
            [DeviceExtension->InputDataCount[Queue]];

    // Retrieve change in x and y from packet
    int change_x=((mpacket[0] & 3) << 6) + mpacket[1];
    int change_y=((mpacket[0] & 12) << 4) + mpacket[2];

    // Some mice need this
    if(coordinate==1) {
      change_x-=128;
      change_y-=128;
    }

    // Change to signed
    if(change_x>=128) { change_x=change_x-256; }
    if(change_y>=128) { change_y=change_y-256; }

    // Adjust mouse position according to sensitivity
    mouse_x+=change_x/horiz_sensitivity;
    mouse_y+=change_y/vert_sensitivity;

    // Check that mouse is still in screen
    if(mouse_x<0) { mouse_x=0; }
    if(mouse_x>max_screen_x) { mouse_x=max_screen_x; }
    if(mouse_y<0) { mouse_y=0; }
    if(mouse_y>max_screen_y) { mouse_y=max_screen_y; }

    Input->LastX = mouse_x;
    Input->LastY = mouse_y;

    // Retrieve mouse button status from packet
    mouse_button1=mpacket[0] & 32;
    mouse_button2=mpacket[0] & 16;
    
    // Determine the current state of the buttons
    Input->RawButtons = mouse_button1 + mouse_button2;
    
    /* Determine ButtonFlags */
    Input->ButtonFlags = 0;
    ButtonsDiff = PreviousButtons ^ Input->RawButtons;

    if (ButtonsDiff & 32)
    {
      if (Input->RawButtons & 32)
      {
        Input->ButtonFlags |= MOUSE_BUTTON_1_DOWN;
      } else {
        Input->ButtonFlags |= MOUSE_BUTTON_1_UP;
      }
    }

    if (ButtonsDiff & 16)
    {
      if (Input->RawButtons & 16)
      {
        Input->ButtonFlags |= MOUSE_BUTTON_2_DOWN;
      } else {
        Input->ButtonFlags |= MOUSE_BUTTON_2_UP;
      }
    }

    bytepos=0;
    
    /* Send the Input data to the Mouse Class driver */
    DeviceExtension->InputDataCount[Queue]++;
    KeInsertQueueDpc(&DeviceExtension->IsrDpc, DeviceObject->CurrentIrp, NULL);

    /* Copy RawButtons to Previous Buttons for Input */
    PreviousButtons = Input->RawButtons;

    return TRUE;
  }

}

void InitializeMouseHardware(unsigned int mtype)
{
  char clear_error_bits;

  WRITE_PORT_UCHAR((PUCHAR)MOUSE_COM+3, 0x80); // set DLAB on
  WRITE_PORT_UCHAR((PUCHAR)MOUSE_COM, 0x60); // speed LO byte
  WRITE_PORT_UCHAR((PUCHAR)MOUSE_COM+1, 0); // speed HI byte
  WRITE_PORT_UCHAR((PUCHAR)MOUSE_COM+3, mtype); // 2=MS Mouse; 3=Mouse systems mouse
  WRITE_PORT_UCHAR((PUCHAR)MOUSE_COM+1, 0); // set comm and DLAB to 0
  WRITE_PORT_UCHAR((PUCHAR)MOUSE_COM+4, 1); // DR int enable

  clear_error_bits=READ_PORT_UCHAR((PUCHAR)MOUSE_COM+5); // clear error bits
}

int DetMicrosoft(void)
{
  char tmp, ind;
  int buttons=0, i, timeout=250;

  WRITE_PORT_UCHAR((PUCHAR)MOUSE_COM+4, 0x0b);
  tmp=READ_PORT_UCHAR((PUCHAR)MOUSE_COM);

  // Check the first four bytes for signs that this is an MS mouse
  for(i=0; i<4; i++) {
    while(((READ_PORT_UCHAR((PUCHAR)MOUSE_COM+5) & 1)==0) && (timeout>0))
    {
      KeDelayExecutionThread (KernelMode, FALSE, 1);
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
  // Waits until the mouse calms down but also quits out after a while
  // in case some destructive user wants to keep moving the mouse
  // before we're done

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

  horiz_sensitivity=2;
  vert_sensitivity=3;

  // Check for Microsoft mouse (2 buttons)
  if(CheckMouseType(2)!=0)
  {
    gotmouse=1;
    DbgPrint("Microsoft Mouse Detected\n");
    ClearMouse();
    coordinate=0;
  }

  // Check for Microsoft Systems mouse (3 buttons)
  if(gotmouse==0) {
    if(CheckMouseType(3)!=0)
    {
    gotmouse=1;
    DbgPrint("Microsoft Mouse Detected\n");
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

BOOLEAN MouseSynchronizeRoutine(PVOID Context)
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

   MouseDataRequired=stk->Parameters.Read.Length/sizeof(MOUSE_INPUT_DATA);
   MouseDataRead=NrToRead;
   CurrentIrp=Irp;

   return(FALSE);
}

VOID SerialMouseStartIo(PDEVICE_OBJECT DeviceObject, PIRP Irp)
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

NTSTATUS SerialMouseInternalDeviceControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
   PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   NTSTATUS status;

   switch(Stack->Parameters.DeviceIoControl.IoControlCode)
   {
      case IOCTL_INTERNAL_MOUSE_CONNECT:

         DeviceExtension->ClassInformation =
            *((PCLASS_INFORMATION)Stack->Parameters.DeviceIoControl.Type3InputBuffer);

         // Reinitialize the port input data queue synchronously
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

NTSTATUS SerialMouseDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION stk = IoGetCurrentIrpStackLocation(Irp);
   NTSTATUS Status;

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

   (*(PSERVICE_CALLBACK_ROUTINE)DeviceExtension->ClassInformation.CallBack)(
			DeviceExtension->ClassInformation.DeviceObject,
			DeviceExtension->MouseInputData,
			NULL,
			&DeviceExtension->InputDataCount);

   DeviceExtension->ActiveQueue = 0;
}

NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
  PDEVICE_OBJECT DeviceObject;
  UNICODE_STRING DeviceName;
  UNICODE_STRING SymlinkName;
  PDEVICE_EXTENSION DeviceExtension;

  if(InitializeMouse(DeviceObject) == TRUE)
  {
    DbgPrint("Serial Mouse Driver 0.0.5\n");
  } else {
    DbgPrint("Serial mouse not found.\n");
    return STATUS_UNSUCCESSFUL;
  }

  DriverObject->MajorFunction[IRP_MJ_CREATE] = SerialMouseDispatch;
  DriverObject->MajorFunction[IRP_MJ_CLOSE]  = SerialMouseDispatch;
  DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = SerialMouseInternalDeviceControl;
  DriverObject->DriverStartIo                = SerialMouseStartIo;

  RtlInitUnicodeStringFromLiteral(&DeviceName,
                                  L"\\Device\\Mouse"); // FIXME: find correct device name
  IoCreateDevice(DriverObject,
	  sizeof(DEVICE_EXTENSION),
	  &DeviceName,
	  FILE_DEVICE_SERIAL_MOUSE_PORT,
	  0,
	  TRUE,
	  &DeviceObject);
  DeviceObject->Flags = DeviceObject->Flags | DO_BUFFERED_IO;

  RtlInitUnicodeStringFromLiteral(&SymlinkName,
                                  L"\\??\\Mouse"); // FIXME: find correct device name
  IoCreateSymbolicLink(&SymlinkName, &DeviceName);

  DeviceExtension = DeviceObject->DeviceExtension;
  KeInitializeDpc(&DeviceExtension->IsrDpc, (PKDEFERRED_ROUTINE)SerialMouseIsrDpc, DeviceObject);

  return(STATUS_SUCCESS);
}

