/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 services/dd/sndblst/sndblst.c
 * PURPOSE:              Sound Blaster / SB Pro / SB 16 driver
 * PROGRAMMER:           Andrew Greenwood
 * UPDATE HISTORY:
 *                       Sept 28, 2003: Copied from mpu401.c as a template
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
//#include <ddk/ntddbeep.h>

//#define NDEBUG
#include <debug.h>
#include <rosrtl/string.h>

#include "sndblst.h"


/* INTERNAL VARIABLES ******************************************************/

UINT DeviceCount = 0;


/* FUNCTIONS ***************************************************************/

NTSTATUS InitDevice(
    IN PWSTR RegistryPath,
    IN PVOID Context)
{
//    PDEVICE_INSTANCE Instance = Context;
    PDEVICE_OBJECT DeviceObject; // = Context;
    PDEVICE_EXTENSION Parameters; // = DeviceObject->DeviceExtension;
    UNICODE_STRING DeviceName = ROS_STRING_INITIALIZER(L"\\Device\\WaveOut0");   // CHANGE THESE?
    UNICODE_STRING SymlinkName = ROS_STRING_INITIALIZER(L"\\??\\WaveOut0");

//    CONFIG Config;
    RTL_QUERY_REGISTRY_TABLE Table[2];
    NTSTATUS s;
    WORD DSP_Version = 0;
    UCHAR DSP_Major = 0, DSP_Minor = 0;

    // This is TEMPORARY, to ensure that we don't process more than 1 device.
    // This limitation should be removed in future.
    if (DeviceCount > 0)
    {
        DPRINT("Sorry - only 1 device supported by Sound Blaster driver at present :(\n");
        return STATUS_NOT_IMPLEMENTED;
    }
    
    DPRINT("Creating IO device\n");

    s = IoCreateDevice(Context, // driverobject
			  sizeof(DEVICE_EXTENSION),
			  &DeviceName,
			  FILE_DEVICE_SOUND, // Correct?
			  0,
			  FALSE,
			  &DeviceObject);

    if (!NT_SUCCESS(s))
        return s;

    DPRINT("Device Extension at 0x%x\n", DeviceObject->DeviceExtension);
    Parameters = DeviceObject->DeviceExtension;

    DPRINT("Creating DOS link\n");

    /* Create the dos device link */
    IoCreateSymbolicLink(&SymlinkName,
		       &DeviceName);

    DPRINT("Initializing device\n");

//    DPRINT("Allocating memory for parameters structure\n");
    // Bodged:
//    Parameters = (PDEVICE_EXTENSION)ExAllocatePool(NonPagedPool, sizeof(DEVICE_EXTENSION));
//    DeviceObject->DeviceExtension = Parameters;
//    Parameters = Instance->DriverObject->DriverExtension;

    DPRINT("DeviceObject at 0x%x, DeviceExtension at 0x%x\n", DeviceObject, Parameters);
    
    if (! Parameters)
    {
        DPRINT("NULL POINTER!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

//    Instance->DriverObject->DriverExtension = Parameters;

    DPRINT("Setting reg path\n");
    Parameters->RegistryPath = RegistryPath;
//    Parameters->DriverObject = Instance->DriverObject;

    DPRINT("Zeroing table memory and setting query routine\n");
    RtlZeroMemory(Table, sizeof(Table));
    Table[0].QueryRoutine = LoadSettings;

    DPRINT("Setting port and IRQ defaults\n");
    Parameters->Port = DEFAULT_PORT;
    Parameters->IRQ = DEFAULT_IRQ;
    Parameters->DMA = DEFAULT_DMA;
    Parameters->BufferSize = DEFAULT_BUFSIZE;

// Only to be enabled once we can get support for multiple cards working :)
/*    
    DPRINT("Loading settings from: %S\n", RegistryPath);
    
    s = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, RegistryPath, Table,
                                &Parameters, NULL);
*/

    if (! NT_SUCCESS(s))
        return s;

    DPRINT("Port 0x%x  IRQ %d  DMA %d\n", Parameters->Port, Parameters->IRQ, Parameters->DMA);

//    Instance->P

    // Initialize the card
    DSP_Version = InitSoundCard(Parameters->Port);
    if (! DSP_Version)
    {
        DPRINT("Sound card initialization FAILED!\n");
        // Set state indication somehow
        // Failure - what error code do we give?!
        // return STATUS_????
    }

    DSP_Major = DSP_Version / 256;
    DSP_Minor = DSP_Version % 256;

    // Do stuff related to version here...

    DPRINT("Allocating DMA\n");
    if (! CreateDMA(DeviceObject))
        DPRINT("FAILURE!\n");

    // TEMPORARY TESTING STUFF: should be in BlasterCreate
    EnableSpeaker(Parameters->Port, TRUE);
    SetOutputSampleRate(Parameters->Port, 2205);
    BeginPlayback(Parameters->Port, 16, 2, Parameters->BufferSize);

    DeviceCount ++;

    return STATUS_SUCCESS;
}


static NTSTATUS STDCALL
BlasterCreate(PDEVICE_OBJECT DeviceObject,
	   PIRP Irp)
/*
 * FUNCTION: Handles user mode requests
 * ARGUMENTS:
 *                       DeviceObject = Device for request
 *                       Irp = I/O request packet describing request
 * RETURNS: Success or failure
 */
{
    DPRINT("BlasterCreate() called!\n");
    
    // Initialize the MPU-401
    // ... do stuff ...


    // Play a note to say we're alive:
//    WaitToSend(MPU401_PORT);
//    MPU401_WRITE_DATA(MPU401_PORT, 0x90);
//    WaitToSend(MPU401_PORT);
//    MPU401_WRITE_DATA(MPU401_PORT, 0x50);
//    WaitToSend(MPU401_PORT);
//    MPU401_WRITE_DATA(MPU401_PORT, 0x7f);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    DPRINT("IoCompleteRequest()\n");

    IoCompleteRequest(Irp,
		    IO_NO_INCREMENT);

    DPRINT("BlasterCreate() completed\n");
    
    return(STATUS_SUCCESS);
}


static NTSTATUS STDCALL
BlasterClose(PDEVICE_OBJECT DeviceObject,
	  PIRP Irp)
/*
 * FUNCTION: Handles user mode requests
 * ARGUMENTS:
 *                       DeviceObject = Device for request
 *                       Irp = I/O request packet describing request
 * RETURNS: Success or failure
 */
{
  PDEVICE_EXTENSION DeviceExtension;
  NTSTATUS Status;

  DPRINT("BlasterClose() called!\n");
  
  DeviceExtension = DeviceObject->DeviceExtension;

  Status = STATUS_SUCCESS;

  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp,
		    IO_NO_INCREMENT);

  return(Status);
}


static NTSTATUS STDCALL
BlasterCleanup(PDEVICE_OBJECT DeviceObject,
	    PIRP Irp)
/*
 * FUNCTION: Handles user mode requests
 * ARGUMENTS:
 *                       DeviceObject = Device for request
 *                       Irp = I/O request packet describing request
 * RETURNS: Success or failure
 */
{
  UINT Channel;
  DPRINT("BlasterCleanup() called!\n");

    // Reset the device (should we do this?)
    for (Channel = 0; Channel <= 15; Channel ++)
    {
        // All notes off
//        MPU401_WRITE_MESSAGE(MPU401_PORT, 0xb0 + Channel, 123, 0);
        // All controllers off
//        MPU401_WRITE_MESSAGE(MPU401_PORT, 0xb0 + Channel, 121, 0);
    }


  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp,
		    IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}


static NTSTATUS STDCALL
BlasterWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    PDEVICE_EXTENSION DeviceExtension;
    UINT ByteCount;
    PBYTE Data;

    DPRINT("BlasterWrite() called!\n");

    DeviceExtension = DeviceObject->DeviceExtension;
    Stack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("%d bytes\n", Stack->Parameters.Write.Length);

            Data = (PBYTE) Irp->AssociatedIrp.SystemBuffer;

            for (ByteCount = 0; ByteCount < Stack->Parameters.Write.Length; ByteCount ++)
            {
//                DPRINT("0x%x ", Data[ByteCount]);

//                MPU401_WRITE_BYTE(DeviceExtension->Port, Data[ByteCount]);
            }

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp,
		    IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}


static NTSTATUS STDCALL
BlasterDeviceControl(PDEVICE_OBJECT DeviceObject,
		  PIRP Irp)
/*
 * FUNCTION: Handles user mode requests
 * ARGUMENTS:
 *                       DeviceObject = Device for request
 *                       Irp = I/O request packet describing request
 * RETURNS: Success or failure
 */
{
    PIO_STACK_LOCATION Stack;
    PDEVICE_EXTENSION DeviceExtension;
    UINT ByteCount;
    PBYTE Data;

    DPRINT("BlasterDeviceControl() called!\n");

    DeviceExtension = DeviceObject->DeviceExtension;
    Stack = IoGetCurrentIrpStackLocation(Irp);

    switch(Stack->Parameters.DeviceIoControl.IoControlCode)
    {
/*        case IOCTL_MIDI_PLAY :
        {
            DPRINT("Received IOCTL_MIDI_PLAY\n");
            Data = (PBYTE) Irp->AssociatedIrp.SystemBuffer;

            DPRINT("Sending %d bytes of MIDI data to 0x%d:\n", Stack->Parameters.DeviceIoControl.InputBufferLength, DeviceExtension->Port);
            
            for (ByteCount = 0; ByteCount < Stack->Parameters.DeviceIoControl.InputBufferLength; ByteCount ++)
            {
                DPRINT("0x%x ", Data[ByteCount]);

                MPU401_WRITE_BYTE(DeviceExtension->Port, Data[ByteCount]);
//                if (WaitToSend(MPU401_PORT))
//                    MPU401_WRITE_DATA(MPU401_PORT, Data[ByteCount]);
            }

            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            
            return(STATUS_SUCCESS);
        }
*/
    }
    
    return(STATUS_SUCCESS);

/*
  DeviceExtension = DeviceObject->DeviceExtension;
  Stack = IoGetCurrentIrpStackLocation(Irp);
  BeepParam = (PBEEP_SET_PARAMETERS)Irp->AssociatedIrp.SystemBuffer;

  Irp->IoStatus.Information = 0;

  if (Stack->Parameters.DeviceIoControl.IoControlCode != IOCTL_BEEP_SET)
    {
      Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
      IoCompleteRequest(Irp,
			IO_NO_INCREMENT);
      return(STATUS_NOT_IMPLEMENTED);
    }

  if ((Stack->Parameters.DeviceIoControl.InputBufferLength != sizeof(BEEP_SET_PARAMETERS))
      || (BeepParam->Frequency < BEEP_FREQUENCY_MINIMUM)
      || (BeepParam->Frequency > BEEP_FREQUENCY_MAXIMUM))
    {
      Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
      IoCompleteRequest(Irp,
			IO_NO_INCREMENT);
      return(STATUS_INVALID_PARAMETER);
    }

  DueTime.QuadPart = 0;
*/
  /* do the beep!! */
/*  DPRINT("Beep:\n  Freq: %lu Hz\n  Dur: %lu ms\n",
	 pbsp->Frequency,
	 pbsp->Duration);

  if (BeepParam->Duration >= 0)
    {
      DueTime.QuadPart = (LONGLONG)BeepParam->Duration * -10000;

      KeSetTimer(&DeviceExtension->Timer,
		 DueTime,
		 &DeviceExtension->Dpc);

      HalMakeBeep(BeepParam->Frequency);
      DeviceExtension->BeepOn = TRUE;
      KeWaitForSingleObject(&DeviceExtension->Event,
			    Executive,
			    KernelMode,
			    FALSE,
			    NULL);
    }
  else if (BeepParam->Duration == (DWORD)-1)
    {
      if (DeviceExtension->BeepOn == TRUE)
	{
	  HalMakeBeep(0);
	  DeviceExtension->BeepOn = FALSE;
	}
      else
	{
	  HalMakeBeep(BeepParam->Frequency);
	  DeviceExtension->BeepOn = TRUE;
	}
    }

  DPRINT("Did the beep!\n");

  Irp->IoStatus.Status = STATUS_SUCCESS;
  IoCompleteRequest(Irp,
		    IO_NO_INCREMENT);
  return(STATUS_SUCCESS);
*/
}


static VOID STDCALL
BlasterUnload(PDRIVER_OBJECT DriverObject)
{
  DPRINT("BlasterUnload() called!\n");
}


NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject,
	    PUNICODE_STRING RegistryPath)
/*
 * FUNCTION:  Called by the system to initalize the driver
 * ARGUMENTS:
 *            DriverObject = object describing this driver
 *            RegistryPath = path to our configuration entries
 * RETURNS:   Success or failure
 */
{
//  PDEVICE_EXTENSION DeviceExtension;
//  PDEVICE_OBJECT DeviceObject;
//  DEVICE_INSTANCE Instance;
  // Doesn't support multiple instances (yet ...)
  NTSTATUS Status;

  DPRINT("Sound Blaster Device Driver 0.0.2\n");

//    Instance.DriverObject = DriverObject;
    // previous instance = NULL...

//    DeviceExtension->RegistryPath = RegistryPath;

  DriverObject->Flags = 0;
  DriverObject->MajorFunction[IRP_MJ_CREATE] = BlasterCreate;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = BlasterClose;
  DriverObject->MajorFunction[IRP_MJ_CLEANUP] = BlasterCleanup;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = BlasterDeviceControl;
  DriverObject->MajorFunction[IRP_MJ_WRITE] = BlasterWrite;
  DriverObject->DriverUnload = BlasterUnload;

    // Major hack to just get this damn thing working:
    Status = InitDevice(RegistryPath->Buffer, DriverObject);    // ????

//    DPRINT("Enumerating devices at %wZ\n", RegistryPath);

//    Status = EnumDeviceKeys(RegistryPath, PARMS_SUBKEY, InitDevice, (PVOID)&DeviceObject); // &Instance;

    // check error

  /* set up device extension */
//  DeviceExtension = DeviceObject->DeviceExtension;
//  DeviceExtension->BeepOn = FALSE;

  return(STATUS_SUCCESS);
}

/* EOF */
