/*
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/io/rw.c
 * PURPOSE:        Implements read/write APIs
 * PROGRAMMER:     David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                 30/05/98: Created
 */

/* INCLUDES ****************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/io.h>
#include <internal/string.h>
#include <internal/ob.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

static VOID IoSecondStageCompletion(PIRP Irp, 
				    BOOLEAN FromDevice,
				    PDEVICE_OBJECT DeviceObject,
				    ULONG Length, 
				    PVOID Buffer)
/*
 * FUNCTION: Performs the second stage of irp completion for read/write irps
 * ARGUMENTS:
 *          Irp = Irp to completion
 *          FromDevice = True if the operation transfered data from the device
 */
{
   if (Irp->UserIosb!=NULL)
     {
	*Irp->UserIosb=Irp->IoStatus;
     }
   
   if (DeviceObject->Flags & DO_BUFFERED_IO && FromDevice)
     {		
	memcpy(Buffer,Irp->AssociatedIrp.SystemBuffer,Length);
     }
   if (DeviceObject->Flags & DO_DIRECT_IO)
     {
	if (Irp->MdlAddress->MappedSystemVa!=NULL)
	  {	     
	     MmUnmapLockedPages(Irp->MdlAddress->MappedSystemVa,
				Irp->MdlAddress);
	  }
	MmUnlockPages(Irp->MdlAddress);
	ExFreePool(Irp->MdlAddress);
     }
   
   IoFreeIrp(Irp);
}

NTSTATUS ZwReadFile(HANDLE FileHandle,
                    HANDLE EventHandle,
		    PIO_APC_ROUTINE ApcRoutine,
		    PVOID ApcContext,
		    PIO_STATUS_BLOCK IoStatusBlock,
		    PVOID Buffer,
		    ULONG Length,
		    PLARGE_INTEGER ByteOffset,
		    PULONG Key)
{
   NTSTATUS Status;
   COMMON_BODY_HEADER* hdr = ObGetObjectByHandle(FileHandle);
   PFILE_OBJECT FileObject = (PFILE_OBJECT)hdr;
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   KEVENT Event;
   NTSTATUS Status;
   
   DPRINT("ZwReadFile(FileHandle %x Buffer %x Length %x ByteOffset %x, "
	  "IoStatusBlock %x)\n",
	  FileHandle,Buffer,Length,ByteOffset,IoStatusBlock);
   
   if (hdr==NULL)
     {
	DPRINT("%s() = STATUS_INVALID_HANDLE\n",__FUNCTION__);
	return(STATUS_INVALID_HANDLE);
     }
   
   if (ByteOffset==NULL)
     {
	ByteOffset = &(FileObject->CurrentByteOffset);
     }
   
   KeInitializeEvent(&Event,NotificationEvent,FALSE);
   Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
				      FileObject->DeviceObject,
				      Buffer,
				      Length,
				      ByteOffset,
				      &Event,
				      IoStatusBlock);

   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->FileObject = FileObject;
   StackPtr->Parameters.Read.Length = Length;
   if (ByteOffset!=NULL)
   {
        StackPtr->Parameters.Read.ByteOffset.LowPart = ByteOffset->LowPart;
        StackPtr->Parameters.Read.ByteOffset.HighPart = ByteOffset->HighPart;
   }
   else
   {
        StackPtr->Parameters.Read.ByteOffset.LowPart = 0;
        StackPtr->Parameters.Read.ByteOffset.HighPart = 0;
   }
   if (Key!=NULL)
   {
         StackPtr->Parameters.Read.Key = *Key;
   }
   else
   {
        StackPtr->Parameters.Read.Key = 0;
   }
   
   DPRINT("FileObject->DeviceObject %x\n",FileObject->DeviceObject);
   Status = IoCallDriver(FileObject->DeviceObject,Irp);
   if (NT_SUCCESS(Status))
     {
       KeWaitForSingleObject(&Event,Executive,KernelMode,FALSE,NULL);
       Status = Irp->IoStatus.Status;
       if (NT_SUCCESS(Status))
         {
           if (FileObject->DeviceObject->Flags&DO_BUFFERED_IO)
             {
               memcpy(Buffer,Irp->AssociatedIrp.SystemBuffer,Length);
             }
         }
     }
   return(Status);
}

NTSTATUS ZwWriteFile(HANDLE FileHandle,
		     HANDLE Event,
		     PIO_APC_ROUTINE ApcRoutine,
		     PVOID ApcContext,
		     PIO_STATUS_BLOCK IoStatusBlock,
		     PVOID Buffer,
		     ULONG Length,
		     PLARGE_INTEGER ByteOffset,
		     PULONG Key)
{
   NTSTATUS Status;
   COMMON_BODY_HEADER* hdr = ObGetObjectByHandle(FileHandle);
   PFILE_OBJECT FileObject = (PFILE_OBJECT)hdr;
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   NTSTATUS Status;
   
   if (hdr==NULL)
     {
	return(STATUS_INVALID_HANDLE);
     }
   
   Irp = IoAllocateIrp(FileObject->DeviceObject->StackSize,TRUE);
   if (Irp==NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   Irp->UserBuffer = (LPVOID)Buffer;
   if (FileObject->DeviceObject->Flags&DO_BUFFERED_IO)
     {
	DPRINT1("Doing buffer i/o\n");
	Irp->AssociatedIrp.SystemBuffer = (PVOID)
	                   ExAllocatePool(NonPagedPool,Length);
	if (Irp->AssociatedIrp.SystemBuffer==NULL)
	  {
	     return(STATUS_UNSUCCESSFUL);
	  }
	memcpy(Irp->AssociatedIrp.SystemBuffer,Buffer,Length);
	Irp->UserBuffer = NULL;
     }
   if (FileObject->DeviceObject->Flags&DO_DIRECT_IO)
     {
	DPRINT1("Doing direct i/o\n");
	
	Irp->MdlAddress = MmCreateMdl(NULL,Buffer,Length);
	MmProbeAndLockPages(Irp->MdlAddress,UserMode,IoReadAccess);
	Irp->UserBuffer = NULL;
	Irp->AssociatedIrp.SystemBuffer = NULL;
     }

   StackPtr = IoGetNextIrpStackLocation(Irp);
   DPRINT("StackPtr %x\n",StackPtr);
   StackPtr->MajorFunction = IRP_MJ_WRITE;
   StackPtr->MinorFunction = 0;
   StackPtr->Flags = 0;
   StackPtr->Control = 0;
   StackPtr->DeviceObject = FileObject->DeviceObject;
   StackPtr->FileObject = FileObject;
   StackPtr->Parameters.Write.Length = Length;
   if (ByteOffset!=NULL)
   {
        StackPtr->Parameters.Write.ByteOffset.LowPart = ByteOffset->LowPart;
        StackPtr->Parameters.Write.ByteOffset.HighPart = ByteOffset->HighPart;
   }
   else
   {
        StackPtr->Parameters.Write.ByteOffset.LowPart = 0;
        StackPtr->Parameters.Write.ByteOffset.HighPart = 0;
   }
   if (Key!=NULL)
   {
         StackPtr->Parameters.Write.Key = *Key;
   }
   else
   {
        StackPtr->Parameters.Write.Key = 0;
   }
   
   DPRINT("FileObject->DeviceObject %x\n",FileObject->DeviceObject);
   Status = IoCallDriver(FileObject->DeviceObject,Irp);
   if (Status==STATUS_PENDING)
     {
	KeWaitForSingleObject(&Event,Executive,KernelMode,FALSE,NULL);
	Status = IoStatusBlock->Status;
     }
   return(Status);
}

