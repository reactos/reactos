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

//#define NDEBUG
#include <debug.h>

#include "ext2fs.h"

/* GLOBALS *****************************************************************/

static PDRIVER_OBJECT DriverObject;

/* FUNCTIONS ****************************************************************/

NTSTATUS STDCALL
Ext2Close(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION Stack;
   PFILE_OBJECT FileObject;
   PDEVICE_EXTENSION DeviceExtension;
   NTSTATUS Status;
   PEXT2_FCB Fcb;
   
   DbgPrint("Ext2Close(DeviceObject %x, Irp %x)\n",DeviceObject,Irp);
   
   Stack = IoGetCurrentIrpStackLocation(Irp);
   FileObject = Stack->FileObject;
   DeviceExtension = DeviceObject->DeviceExtension;
   
   if (FileObject == DeviceExtension->FileObject)
     {
	Status = STATUS_SUCCESS;
   
	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = 0;
	
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return(Status);
     }

   Fcb = (PEXT2_FCB)FileObject->FsContext;
   if (Fcb != NULL)
     {
	if (Fcb->Bcb != NULL)
	  {
	     CcRosReleaseFileCache(FileObject, Fcb->Bcb);
	  }
	ExFreePool(Fcb);
	FileObject->FsContext = NULL;
     }
   
   Status = STATUS_SUCCESS;
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
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
   DPRINT("DeviceObject %x\n",DeviceObject);
   DeviceObject->Flags = DeviceObject->Flags | DO_DIRECT_IO;
   DeviceExt = (PVOID)DeviceObject->DeviceExtension;
   DPRINT("DeviceExt %x\n",DeviceExt);

  DeviceExt->StorageDevice = DeviceToMount;
  DeviceExt->StorageDevice->Vpb->DeviceObject = DeviceObject;
  DeviceExt->StorageDevice->Vpb->RealDevice = DeviceExt->StorageDevice;
  DeviceExt->StorageDevice->Vpb->Flags |= VPB_MOUNTED;
  DeviceObject->StackSize = DeviceExt->StorageDevice->StackSize + 1;
  DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

   DPRINT("DeviceExt->StorageDevice %x\n", DeviceExt->StorageDevice);
   DeviceExt->FileObject = IoCreateStreamFileObject(NULL, DeviceObject);
   DeviceExt->superblock = superblock;
   CcRosInitializeFileCache(DeviceExt->FileObject,
			    &DeviceExt->Bcb,
			    PAGESIZE * 3);
   
   DPRINT("Ext2Mount() = STATUS_SUCCESS\n");
   
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL
Ext2FileSystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
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

NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT _DriverObject,
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
   UNICODE_STRING DeviceName;
   
   DbgPrint("Ext2 FSD 0.0.1\n");
   
   DriverObject = _DriverObject;
   
   RtlInitUnicodeString(&DeviceName,
			L"\\Device\\Ext2Fsd");
   ret = IoCreateDevice(DriverObject,
			0,
			&DeviceName,
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
   DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] =
                      Ext2DirectoryControl;
   DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = 
                      Ext2QueryInformation;
   DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = Ext2SetInformation;
   DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = Ext2FlushBuffers;
   DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = Ext2Shutdown;
   DriverObject->MajorFunction[IRP_MJ_CLEANUP] = Ext2Cleanup;
   DriverObject->MajorFunction[IRP_MJ_QUERY_SECURITY] = Ext2QuerySecurity;
   DriverObject->MajorFunction[IRP_MJ_SET_SECURITY] = Ext2SetSecurity;
   DriverObject->MajorFunction[IRP_MJ_QUERY_QUOTA] = Ext2QueryQuota;
   DriverObject->MajorFunction[IRP_MJ_SET_QUOTA] = Ext2SetQuota;
   
   DriverObject->DriverUnload = NULL;
   
   IoRegisterFileSystem(DeviceObject);
   
   return(STATUS_SUCCESS);
}

