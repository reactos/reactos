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

//#define NDEBUG
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
   UNIMPLEMENTED;
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
   UNIMPLEMENTED;
}

static BOOL WriteDevice(PDEVICE_OBJECT dev, LPVOID lpBuffer,
			DWORD nNumberOfBytesToWrite,
			LPDWORD lpNumberOfBytesWritten,
			LPOVERLAPPED lpOverlapped)
{
   PDRIVER_OBJECT drv = dev->DriverObject;
   PIRP irp;
   PIO_STACK_LOCATION StackPtr;
   
   DPRINT("dev %x drv %x\n",dev,drv);
   
   /*
    * Build an irp for the transfer
    */
   irp = IoAllocateIrp(dev->StackSize,TRUE);
   if (irp==NULL)
     {
	printk("Failed to allocate IRP\n");
	return(FALSE);
     }
   
   /*
    * Prepare the user buffer
    */
   DPRINT1("Preparing user buffer\n");
   irp->UserBuffer = (LPVOID)lpBuffer;    // This handles the 'neither' method
   if (dev->Flags&DO_BUFFERED_IO)
     {
	DPRINT1("Doing buffer i/o\n");
	irp->AssociatedIrp.SystemBuffer = (PVOID)
	                   ExAllocatePool(NonPagedPool,nNumberOfBytesToWrite);
	if (irp->AssociatedIrp.SystemBuffer==NULL)
	  {
	     return(FALSE);
	  }
	memcpy(irp->AssociatedIrp.SystemBuffer,lpBuffer,nNumberOfBytesToWrite);
	irp->UserBuffer = NULL;
     }
   if (dev->Flags&DO_DIRECT_IO)
     {
	DPRINT1("Doing direct i/o\n");
	
	irp->MdlAddress = MmCreateMdl(NULL,lpBuffer,nNumberOfBytesToWrite);
	MmProbeAndLockPages(irp->MdlAddress,UserMode,IoWriteAccess);
	irp->UserBuffer = NULL;
	irp->AssociatedIrp.SystemBuffer = NULL;
     }

   /*
    * Set up the stack location 
    */
   StackPtr = IoGetNextIrpStackLocation(irp);
   StackPtr->MajorFunction = IRP_MJ_WRITE;
   StackPtr->MinorFunction = 0;
   StackPtr->Flags = 0;
   StackPtr->Control = 0;
   StackPtr->DeviceObject = dev;
   StackPtr->FileObject = NULL;
   StackPtr->Parameters.Write.Length = nNumberOfBytesToWrite;
   
   DPRINT1("Sending IRP\n");
   IoCallDriver(dev,irp);

   
   /*
    * Free the above buffer
    */
}

WINBOOL STDCALL WriteFile(HANDLE hFile, LPCVOID lpBuffer,
			  DWORD nNumberOfBytesToWrite,
			  LPDWORD lpNumberOfBytesWritten,
			  LPOVERLAPPED lpOverlapped)
{
   COMMON_BODY_HEADER* hdr = ObGetObjectByHandle(hFile);
   
   if (hdr->Type==OBJTYP_DEVICE)
     {
	return(WriteDevice(hdr,lpBuffer,nNumberOfBytesToWrite,
			   lpNumberOfBytesWritten,lpOverlapped));       
     }
   return(FALSE);
}

static BOOL ReadDevice(PDEVICE_OBJECT dev, LPVOID lpBuffer,
			DWORD nNumberOfBytesToWrite,
			LPDWORD lpNumberOfBytesWritten,
			LPOVERLAPPED lpOverlapped)
{
   PDRIVER_OBJECT drv = dev->DriverObject;
   PIRP irp;
   PIO_STACK_LOCATION StackPtr;
   
   DPRINT("dev %x drv %x\n",dev,drv);
   
   /*
    * Build an irp for the transfer
    */
   irp = IoAllocateIrp(dev->StackSize,TRUE);
   if (irp==NULL)
     {
	printk("Failed to allocate IRP\n");
	return(FALSE);
     }
   
   /*
    * Prepare the user buffer
    */
   DPRINT1("Preparing user buffer\n");
   irp->UserBuffer = (LPVOID)lpBuffer;    // This handles the 'neither' method
   if (dev->Flags&DO_BUFFERED_IO)
     {
	DPRINT1("Doing buffer i/o\n");
	irp->AssociatedIrp.SystemBuffer = (PVOID)
	                   ExAllocatePool(NonPagedPool,nNumberOfBytesToWrite);
	if (irp->AssociatedIrp.SystemBuffer==NULL)
	  {
	     return(FALSE);
	  }
	memcpy(irp->AssociatedIrp.SystemBuffer,lpBuffer,nNumberOfBytesToWrite);
	irp->UserBuffer = NULL;
     }
   if (dev->Flags&DO_DIRECT_IO)
     {
	DPRINT1("Doing direct i/o\n");
	
	irp->MdlAddress = MmCreateMdl(NULL,lpBuffer,nNumberOfBytesToWrite);
	MmProbeAndLockPages(irp->MdlAddress,UserMode,IoWriteAccess);
	irp->UserBuffer = NULL;
	irp->AssociatedIrp.SystemBuffer = NULL;
     }

   /*
    * Set up the stack location 
    */
   StackPtr = IoGetNextIrpStackLocation(irp);
   StackPtr->MajorFunction = IRP_MJ_READ;
   StackPtr->MinorFunction = 0;
   StackPtr->Flags = 0;
   StackPtr->Control = 0;
   StackPtr->DeviceObject = dev;
   StackPtr->FileObject = NULL;
   StackPtr->Parameters.Write.Length = nNumberOfBytesToWrite;
   
   DPRINT1("Sending IRP\n");
   IoCallDriver(dev,irp);

   
   /*
    * Free the above buffer
    */
   DPRINT1("Finished ReadDevice\n");
}



WINBOOL STDCALL ReadFile(HANDLE hFile, LPVOID lpBuffer,
			  DWORD nNumberOfBytesToWrite,
			  LPDWORD lpNumberOfBytesWritten,
			  LPOVERLAPPED lpOverlapped)
{
   COMMON_BODY_HEADER* hdr = ObGetObjectByHandle(hFile);
   
   if (hdr->Type==OBJTYP_DEVICE)
     {
	return(ReadDevice((PDEVICE_OBJECT)hdr,lpBuffer,nNumberOfBytesToWrite,
			   lpNumberOfBytesWritten,lpOverlapped));       
     }
   return(FALSE);
}


