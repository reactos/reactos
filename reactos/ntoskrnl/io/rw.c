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
#include <internal/kernel.h>
#include <internal/iomgr.h>
#include <internal/string.h>
#include <internal/objmgr.h>

#define NDEBUG
#include <internal/debug.h>

#ifndef NDEBUG
#define DPRINT1(x) printk(x)
#else
#define DPRINT1(x)
#endif

/* FUNCTIONS ***************************************************************/

NTSTATUS ZwReadFile(HANDLE FileHandle,
		    HANDLE Event,
		    PIO_APC_ROUTINE ApcRoutine,
		    PVOID ApcContext,
		    PIO_STATUS_BLOCK IoStatusBlock,
		    PVOID Buffer,
		    ULONG Length,
		    PLARGE_INTEGER ByteOffset,
		    PULONG Key)
{
   COMMON_BODY_HEADER* hdr = ObGetObjectByHandle(FileHandle);
   PFILE_OBJECT FileObject = (PFILE_OBJECT)hdr;
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   
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
	Irp->UserBuffer = NULL;
     }
   if (FileObject->DeviceObject->Flags&DO_DIRECT_IO)
     {
	DPRINT1("Doing direct i/o\n");
	
	Irp->MdlAddress = MmCreateMdl(NULL,Buffer,Length);
	MmProbeAndLockPages(Irp->MdlAddress,UserMode,IoWriteAccess);
	Irp->UserBuffer = NULL;
	Irp->AssociatedIrp.SystemBuffer = NULL;
     }

   StackPtr = IoGetNextIrpStackLocation(Irp);
   DPRINT("StackPtr %x\n",StackPtr);
   StackPtr->MajorFunction = IRP_MJ_READ;
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
   IoCallDriver(FileObject->DeviceObject,Irp);
   memcpy(Buffer,Irp->AssociatedIrp.SystemBuffer,Length);
   return(STATUS_SUCCESS);
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
   COMMON_BODY_HEADER* hdr = ObGetObjectByHandle(FileHandle);
   PFILE_OBJECT FileObject = (PFILE_OBJECT)hdr;
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   
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
   IoCallDriver(FileObject->DeviceObject,Irp);
   return(STATUS_SUCCESS);
}

