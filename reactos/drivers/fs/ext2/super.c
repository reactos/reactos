/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/ext2/super.c
 * PURPOSE:          ext2 filesystem
 * PROGRAMMER:       David Welch (welch@mcmail.com)
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/string.h>
#include <wstring.h>

#define NDEBUG
#include <internal/debug.h>

#include "ext2fs.h"

/* GLOBALS *****************************************************************/

static PDRIVER_OBJECT DriverObject;

/* FUNCTIONS ****************************************************************/

NTSTATUS Ext2CloseFile(PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject)
/*
 * FUNCTION: Closes a file
 */
{
}

NTSTATUS Ext2Close(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   PFILE_OBJECT FileObject = Stack->FileObject;
   PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
   NTSTATUS Status;
   
   Status = Ext2CloseFile(DeviceExtension,FileObject);

   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return(Status);
}

NTSTATUS Ext2Write(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   DPRINT("FsdWrite(DeviceObject %x Irp %x)\n",DeviceObject,Irp);
   
   Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
   Irp->IoStatus.Information = 0;
   return(STATUS_UNSUCCESSFUL);
}

NTSTATUS Ext2Read(PDEVICE_OBJECT DeviceObject, PIRP Irp)
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
   Offset = Stack->Parameters.Read.ByteOffset;
   
   Status = Ext2ReadFile(DeviceExt,FileObject,Buffer,Length,Offset);
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = Length;
   IoCompleteRequest(Irp,IO_NO_INCREMENT);
   
   return(Status);
}


NTSTATUS Ext2Mount(PDEVICE_OBJECT DeviceToMount)
{
   PDEVICE_OBJECT DeviceObject;
   PDEVICE_EXTENSION DeviceExt;
   PVOID BlockBuffer;   
   struct ext2_super_block* superblock;
   
   DPRINT("Ext2Mount(DeviceToMount %x)\n",DeviceToMount);
   
   BlockBuffer = ExAllocatePool(NonPagedPool,BLOCKSIZE);
   Ext2ReadSectors(DeviceToMount,
		   1,
		   1,
		   BlockBuffer);
   superblock = BlockBuffer;
   
   if (superblock->s_magic != EXT2_SUPER_MAGIC)
     {
	ExFreePool(BlockBuffer);
	return(STATUS_UNRECOGNIZED_VOLUME);
     }
   DPRINT("Volume recognized\n");
   DPRINT("s_inodes_count %d\n",superblock->s_inodes_count);
   DPRINT("s_blocks_count %d\n",superblock->s_blocks_count);
   
   IoCreateDevice(DriverObject,
		  sizeof(DEVICE_EXTENSION),
		  NULL,
		  FILE_DEVICE_FILE_SYSTEM,
		  0,
		  FALSE,
		  &DeviceObject);
   DeviceObject->Flags = DeviceObject->Flags | DO_DIRECT_IO;
   DeviceExt = (PVOID)DeviceObject->DeviceExtension;
   
   DeviceExt->StorageDevice = IoAttachDeviceToDeviceStack(DeviceObject,
							  DeviceToMount);
   DeviceExt->superblock = superblock;
   
   return(STATUS_SUCCESS);
}

NTSTATUS Ext2FileSystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   PVPB	vpb = Stack->Parameters.Mount.Vpb;
   PDEVICE_OBJECT DeviceToMount = Stack->Parameters.Mount.DeviceObject;
   NTSTATUS Status;
   
   Status = Ext2Mount(DeviceToMount);
   
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
   UNICODE_STRING DeviceNameU;
   ANSI_STRING DeviceNameA;
   
   DbgPrint("Ext2 FSD 0.0.1\n");
          
   DriverObject = _DriverObject;
   
   RtlInitAnsiString(&DeviceNameA,"\\Device\\Ext2Fsd");
   RtlAnsiStringToUnicodeString(&DeviceNameU,&DeviceNameA,TRUE);
   ret = IoCreateDevice(DriverObject,
			0,
			&DeviceNameU,
                        FILE_DEVICE_FILE_SYSTEM,
			0,
			FALSE,
			&DeviceObject);
   if (ret!=STATUS_SUCCESS)
     {
	return(ret);
     }

   DeviceObject->Flags=0;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = Ext2Close;
   DriverObject->MajorFunction[IRP_MJ_CREATE] = Ext2Create;
   DriverObject->MajorFunction[IRP_MJ_READ] = Ext2Read;
   DriverObject->MajorFunction[IRP_MJ_WRITE] = Ext2Write;
   DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] =
                      Ext2FileSystemControl;
   DriverObject->DriverUnload = NULL;
   
   IoRegisterFileSystem(DeviceObject);
   
   return(STATUS_SUCCESS);
}

