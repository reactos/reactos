/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/template/template.c
 * PURPOSE:          Bare filesystem template
 * PROGRAMMER:       David Welch (welch@mcmail.com)
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/string.h>
#include <wstring.h>

#define NDEBUG
#include <internal/debug.h>

typedef struct
{
   PDEVICE_OBJECT StorageDevice;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

/* GLOBALS *****************************************************************/

static PDRIVER_OBJECT DriverObject;

/* FUNCTIONS ****************************************************************/

NTSTATUS FsdCloseFile(PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject)
/*
 * FUNCTION: Closes a file
 */
{
}

NTSTATUS FsdOpenFile(PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject, 
		     PWSTR FileName)
/*
 * FUNCTION: Opens a file
 */
{
}

BOOLEAN FsdHasFileSystem(PDEVICE_OBJECT DeviceToMount)
/*
 * FUNCTION: Tests if the device contains a filesystem that can be mounted 
 * by this fsd
 */
{
}

NTSTATUS FsdMountDevice(PDEVICE_EXTENSION DeviceExt, 
			PDEVICE_OBJECT DeviceToMount)
/*
 * FUNCTION: Mounts the device
 */
{
}


NTSTATUS FsdReadFile(PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject,
		     PVOID Buffer, ULONG Length, ULONG Offset)
/*
 * FUNCTION: Reads data from a file
 */
{
}


NTSTATUS FsdClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   PFILE_OBJECT FileObject = Stack->FileObject;
   PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
   NTSTATUS Status;
   
   Status = FsdCloseFile(DeviceExtension,FileObject);

   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return(Status);
}

NTSTATUS FsdCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   PFILE_OBJECT FileObject = Stack->FileObject;
   NTSTATUS Status;
   PDEVICE_EXTENSION DeviceExt;
   
   DeviceExt = DeviceObject->DeviceExtension;
   Status = FsdOpenFile(DeviceExt,FileObject,FileObject->FileName.Buffer);
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return(Status);
}


NTSTATUS FsdWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   DPRINT("FsdWrite(DeviceObject %x Irp %x)\n",DeviceObject,Irp);
   
   Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
   Irp->IoStatus.Information = 0;
   return(STATUS_UNSUCCESSFUL);
}

NTSTATUS FsdRead(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   ULONG Length;
   PVOID Buffer;
   ULONG Offset;
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   PFILE_OBJECT FileObject = Stack->FileObject;
   PDEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;
   NTSTATUS Status;
   
   DPRINT("FsdRead(DeviceObject %x, Irp %x)\n",DeviceObject,Irp);
   
   Length = Stack->Parameters.Read.Length;
   Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
   Offset = Stack->Parameters.Read.ByteOffset.LowPart;
   
   Status = FsdReadFile(DeviceExt,FileObject,Buffer,Length,Offset);
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = Length;
   IoCompleteRequest(Irp,IO_NO_INCREMENT);
   return(Status);
}


NTSTATUS FsdMount(PDEVICE_OBJECT DeviceToMount)
{
   PDEVICE_OBJECT DeviceObject;
   PDEVICE_EXTENSION DeviceExt;
      
   IoCreateDevice(DriverObject,
		  sizeof(DEVICE_EXTENSION),
		  NULL,
		  FILE_DEVICE_FILE_SYSTEM,
		  0,
		  FALSE,
		  &DeviceObject);
   DeviceObject->Flags = DeviceObject->Flags | DO_DIRECT_IO;
   DeviceExt = (PVOID)DeviceObject->DeviceExtension;
   
   FsdMountDevice(DeviceExt,DeviceToMount);
   
   DeviceExt->StorageDevice = IoAttachDeviceToDeviceStack(DeviceObject,
							  DeviceToMount);
   return(STATUS_SUCCESS);
}

NTSTATUS FsdFileSystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   PVPB	vpb = Stack->Parameters.Mount.Vpb;
   PDEVICE_OBJECT DeviceToMount = Stack->Parameters.Mount.DeviceObject;
   NTSTATUS Status;
   
   if (FsdHasFileSystem(DeviceToMount))
     {
	Status = FsdMount(DeviceToMount);
     }
   else
     {
	Status = STATUS_UNRECOGNIZED_VOLUME;
     }
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return(Status);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT _DriverObject,
		     PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Called by the system to initalize the driver
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 *           RegistryPath = path to our configuration entries
 * RETURNS: Success or failure
 */
{
   PDEVICE_OBJECT DeviceObject;
   NTSTATUS ret;
   UNICODE_STRING ustr;
   ANSI_STRING astr;
   
   DbgPrint("Bare FSD Template 0.0.1\n");
          
   DriverObject = _DriverObject;
   
   RtlInitAnsiString(&astr,"\\Device\\BareFsd");
   RtlAnsiStringToUnicodeString(&ustr,&astr,TRUE);
   ret = IoCreateDevice(DriverObject,0,&ustr,
                        FILE_DEVICE_FILE_SYSTEM,0,FALSE,&DeviceObject);
   if (ret!=STATUS_SUCCESS)
     {
	return(ret);
     }

   DeviceObject->Flags=0;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = FsdClose;
   DriverObject->MajorFunction[IRP_MJ_CREATE] = FsdCreate;
   DriverObject->MajorFunction[IRP_MJ_READ] = FsdRead;
   DriverObject->MajorFunction[IRP_MJ_WRITE] = FsdWrite;
   DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] =
                      FsdFileSystemControl;
   DriverObject->DriverUnload = NULL;
   
   IoRegisterFileSystem(DeviceObject);
   
   return(STATUS_SUCCESS);
}

