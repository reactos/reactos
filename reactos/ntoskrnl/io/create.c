/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/create.c
 * PURPOSE:         Handling file create/open apis
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  24/05/98: Created
 */

/* INCLUDES ***************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/kernel.h>
#include <internal/objmgr.h>
#include <internal/iomgr.h>
#include <internal/string.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *************************************************************/

NTSTATUS ZwCreateFile(PHANDLE FileHandle,
		      ACCESS_MASK DesiredAccess,
		      POBJECT_ATTRIBUTES ObjectAttributes,
		      PIO_STATUS_BLOCK IoStatusBlock,
		      PLARGE_INTEGER AllocateSize,
		      ULONG FileAttributes,
		      ULONG ShareAccess,
		      ULONG CreateDisposition,
		      ULONG CreateOptions,
		      PVOID EaBuffer,
		      ULONG EaLength)
{
   UNIMPLEMENTED;
}

NTSTATUS ZwOpenFile(PHANDLE FileHandle,
		    ACCESS_MASK DesiredAccess,
		    POBJECT_ATTRIBUTES ObjectAttributes,
		    PIO_STATUS_BLOCK IoStatusBlock,
		    ULONG ShareAccess,
		    ULONG OpenOptions)
{
   PVOID Object;
   NTSTATUS Status;
   PIRP Irp;
   KEVENT Event;
   PDEVICE_OBJECT DeviceObject;
   PFILE_OBJECT FileObject;   
   PIO_STACK_LOCATION StackLoc;
   
   assert_irql(PASSIVE_LEVEL);
   
   *FileHandle=0;
   
   Status =  ObOpenObjectByName(ObjectAttributes,&Object);
   DPRINT("Object %x Status %x\n",Object,Status);
   if (!NT_SUCCESS(Status))
     {	
	return(Status);
     }
   
   DeviceObject = (PDEVICE_OBJECT)Object;

   FileObject = ObGenericCreateObject(FileHandle,0,NULL,OBJTYP_FILE);
   DPRINT("FileObject %x DeviceObject %x\n",FileObject,DeviceObject);
   memset(FileObject,0,sizeof(FILE_OBJECT));
   FileObject->DeviceObject=DeviceObject;
   FileObject->Flags = FileObject->Flags | FO_DIRECT_DEVICE_OPEN;
   
   KeInitializeEvent(&Event,NotificationEvent,FALSE);
   
   Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
   if (Irp==NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   StackLoc = IoGetNextIrpStackLocation(Irp);
   DPRINT("StackLoc %x\n",StackLoc);
   StackLoc->MajorFunction = IRP_MJ_CREATE;
   StackLoc->MinorFunction = 0;
   StackLoc->Flags = 0;
   StackLoc->Control = 0;
   StackLoc->DeviceObject = DeviceObject;
   StackLoc->FileObject=FileObject;
   DPRINT("DeviceObject %x\n",DeviceObject);
   DPRINT("DeviceObject->DriverObject %x\n",DeviceObject->DriverObject);
   IoCallDriver(DeviceObject,Irp);
   
   return(STATUS_SUCCESS);
}


