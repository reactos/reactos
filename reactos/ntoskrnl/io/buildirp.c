/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/buildirp.c
 * PURPOSE:         Building various types of irp
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS IoPrepareIrpBuffer(PIRP Irp,
			    PDEVICE_OBJECT DeviceObject,
			    PVOID Buffer,
			    PVOID Length)
/*
 * FUNCTION: Prepares the buffer to be used for an IRP
 */
{
   Irp->UserBuffer = (LPVOID)Buffer;
   if (DeviceObject->Flags&DO_BUFFERED_IO)
     {
	DPRINT("Doing buffer i/o\n",0);
	Irp->AssociatedIrp.SystemBuffer = (PVOID)
	  ExAllocatePool(NonPagedPool,Length);
	if (Irp->AssociatedIrp.SystemBuffer==NULL)
	  {
	     return(STATUS_UNSUCCESSFUL);
	  }
     }
   if (DeviceObject->Flags&DO_DIRECT_IO)
     {
	DPRINT("Doing direct i/o\n",0);
	
	Irp->MdlAddress = MmCreateMdl(NULL,Buffer,Length);
	MmProbeAndLockPages(Irp->MdlAddress,UserMode,IoWriteAccess);
	Irp->UserBuffer = NULL;
	Irp->AssociatedIrp.SystemBuffer = NULL;
     }   
   return(STATUS_SUCCESS);
}

PIRP IoBuildFilesystemControlRequest(ULONG MinorFunction,
				     PDEVICE_OBJECT DeviceObject,
				     PKEVENT UserEvent,
				     PIO_STATUS_BLOCK IoStatusBlock,
				     PDEVICE_OBJECT DeviceToMount)
/*
 * FUNCTION: Allocates and sets up a filesystem control IRP
 * ARGUMENTS:
 *         MinorFunction = Type of filesystem control
 *         DeviceObject = Device object to send the request to
 *         UserEvent = Event used to notify the caller of completion
 *         IoStatusBlock (OUT) = Used to return the status of the operation
 *         DeviceToMount = Device to mount (for the IRP_MN_MOUNT_DEVICE 
 *					    request)
 */
{
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   
   Irp = IoAllocateIrp(DeviceObject->StackSize,TRUE);
   if (Irp==NULL)
     {
	return(NULL);
     }
   
   Irp->UserIosb = IoStatusBlock;
   Irp->UserEvent = UserEvent;
   
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;
   StackPtr->MinorFunction = MinorFunction;
   StackPtr->Flags = 0;
   StackPtr->Control = 0;
   StackPtr->DeviceObject = DeviceObject;
   StackPtr->FileObject = NULL;
   
   switch(MinorFunction)
     {
      case IRP_MN_USER_FS_REQUEST:
	break;
	
      case IRP_MN_MOUNT_VOLUME:
	StackPtr->Parameters.Mount.Vpb = DeviceObject->Vpb;
	StackPtr->Parameters.Mount.DeviceObject = DeviceToMount;
	break;
	
      case IRP_MN_VERIFY_VOLUME:
	break;
	
      case IRP_MN_LOAD_FILE_SYSTEM:
	break;
     }
   return(Irp);
}

PIRP IoBuildAsynchronousFsdRequest(ULONG MajorFunction,
				   PDEVICE_OBJECT DeviceObject,
				   PVOID Buffer,
				   ULONG Length,
				   PLARGE_INTEGER StartingOffset,
				   PIO_STATUS_BLOCK IoStatusBlock)
/*
 * FUNCTION: Allocates and sets up an IRP to be sent to lower level drivers
 * ARGUMENTS:
 *         MajorFunction = One of IRP_MJ_READ, IRP_MJ_WRITE, 
 *                         IRP_MJ_FLUSH_BUFFERS or IRP_MJ_SHUTDOWN
 *         DeviceObject = Device object to send the irp to
 *         Buffer = Buffer into which data will be read or written
 *         Length = Length in bytes of the irp to be allocated
 *         StartingOffset = Starting offset on the device
 *         IoStatusBlock (OUT) = Storage for the result of the operation
 * RETURNS: The IRP allocated on success, or
 *          NULL on failure
 */
{
      PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   
   DPRINT("IoBuildAsynchronousFsdRequest(MajorFunction %x, DeviceObject %x, "
	  "Buffer %x, Length %x, StartingOffset %x, "
	  "IoStatusBlock %x\n",MajorFunction,DeviceObject,Buffer,Length,
	  StartingOffset,IoStatusBlock);
   
   Irp = IoAllocateIrp(DeviceObject->StackSize,TRUE);
   if (Irp==NULL)
     {
	return(NULL);
     }

   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->MajorFunction = MajorFunction;
   StackPtr->MinorFunction = 0;
   StackPtr->Flags = 0;
   StackPtr->Control = 0;
   StackPtr->DeviceObject = DeviceObject;
   StackPtr->FileObject = NULL;
   StackPtr->Parameters.Write.Length = Length;
   
   if (MajorFunction == IRP_MJ_READ || MajorFunction == IRP_MJ_WRITE)
     {
	Irp->UserBuffer = (LPVOID)Buffer;
	if (DeviceObject->Flags&DO_BUFFERED_IO)
	  {
	     DPRINT("Doing buffer i/o\n",0);
	     Irp->AssociatedIrp.SystemBuffer = (PVOID)
	       ExAllocatePool(NonPagedPool,Length);
	     if (Irp->AssociatedIrp.SystemBuffer==NULL)
	       {
		  return(NULL);
	       }
	  }
	if (DeviceObject->Flags&DO_DIRECT_IO)
	  {
	     DPRINT("Doing direct i/o\n",0);
	     
	     Irp->MdlAddress = MmCreateMdl(NULL,Buffer,Length);
	     MmProbeAndLockPages(Irp->MdlAddress,UserMode,IoWriteAccess);
	     Irp->UserBuffer = NULL;
	     Irp->AssociatedIrp.SystemBuffer = NULL;
	  }
	if (StartingOffset!=NULL)
	  {
	     StackPtr->Parameters.Write.ByteOffset = *StartingOffset;
	  }
	else
	  {
	     SET_LARGE_INTEGER_LOW_PART(StackPtr->Parameters.Write.ByteOffset, 0);
	     SET_LARGE_INTEGER_HIGH_PART(StackPtr->Parameters.Write.ByteOffset, 0);
	  }	
     }
	
   Irp->UserIosb = IoStatusBlock;
      
   return(Irp);
}

PIRP IoBuildDeviceIoControlRequest(ULONG IoControlCode,
				   PDEVICE_OBJECT DeviceObject,
				   PVOID InputBuffer,
				   ULONG InputBufferLength,
				   PVOID OutputBuffer,
				   ULONG OutputBufferLength,
				   BOOLEAN InternalDeviceIoControl,
				   PKEVENT Event,
				   PIO_STATUS_BLOCK IoStatusBlock)
{
   UNIMPLEMENTED;
}

PIRP IoBuildSynchronousFsdRequest(ULONG MajorFunction,
				  PDEVICE_OBJECT DeviceObject,
				  PVOID Buffer,
				  ULONG Length,
				  PLARGE_INTEGER StartingOffset,
				  PKEVENT Event,
				  PIO_STATUS_BLOCK IoStatusBlock)
/*
 * FUNCTION: Allocates and builds an IRP to be sent synchronously to lower
 * level driver(s)
 * ARGUMENTS:
 *        MajorFunction = Major function code, one of IRP_MJ_READ,
 *                        IRP_MJ_WRITE, IRP_MJ_FLUSH_BUFFERS, IRP_MJ_SHUTDOWN
 *        DeviceObject = Target device object
 *        Buffer = Buffer containing data for a read or write
 *        Length = Length in bytes of the information to be transferred
 *        StartingOffset = Offset to begin the read/write from
 *        Event (OUT) = Will be set when the operation is complete
 *        IoStatusBlock (OUT) = Set to the status of the operation
 * RETURNS: The IRP allocated on success, or
 *          NULL on failure
 */
{
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   
   DPRINT("IoBuildSynchronousFsdRequest(MajorFunction %x, DeviceObject %x, "
	  "Buffer %x, Length %x, StartingOffset %x, Event %x, "
	  "IoStatusBlock %x\n",MajorFunction,DeviceObject,Buffer,Length,
	  StartingOffset,Event,IoStatusBlock);
   
   Irp = IoAllocateIrp(DeviceObject->StackSize,TRUE);
   if (Irp==NULL)
     {
	return(NULL);
     }
   
   Irp->UserBuffer = (LPVOID)Buffer;
   Irp->UserEvent = Event;
   Irp->UserIosb = IoStatusBlock;
   if (DeviceObject->Flags&DO_BUFFERED_IO)
     {
	DPRINT("Doing buffer i/o\n");
	Irp->AssociatedIrp.SystemBuffer = (PVOID)
	                   ExAllocatePool(NonPagedPool,Length);
	if (Irp->AssociatedIrp.SystemBuffer==NULL)
	  {
	     IoFreeIrp(Irp);
	     return(NULL);
	  }
        if (MajorFunction == IRP_MJ_WRITE)
          {
             RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, Buffer, Length);
          }
     }
   if (DeviceObject->Flags&DO_DIRECT_IO)
     {
	DPRINT("Doing direct i/o\n");
	
	Irp->MdlAddress = MmCreateMdl(NULL,Buffer,Length);
        if (MajorFunction == IRP_MJ_READ)
          {
             MmProbeAndLockPages(Irp->MdlAddress,UserMode,IoWriteAccess);
          }
        else
          {
             MmProbeAndLockPages(Irp->MdlAddress,UserMode,IoReadAccess);
          }
	Irp->UserBuffer = NULL;
	Irp->AssociatedIrp.SystemBuffer = NULL;
     }

   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->MajorFunction = MajorFunction;
   StackPtr->MinorFunction = 0;
   StackPtr->Flags = 0;
   StackPtr->Control = 0;
   StackPtr->DeviceObject = DeviceObject;
   StackPtr->FileObject = NULL;
   StackPtr->Parameters.Write.Length = Length;
   if (MajorFunction == IRP_MJ_READ)
     {
       if (StartingOffset != NULL)
         {
            StackPtr->Parameters.Read.ByteOffset = *StartingOffset;
         }
       else
         {
            SET_LARGE_INTEGER_LOW_PART(StackPtr->Parameters.Read.ByteOffset, 0);
            SET_LARGE_INTEGER_HIGH_PART(StackPtr->Parameters.Read.ByteOffset, 0);
         }
     }
   else
     {
       if (StartingOffset!=NULL)
         {
            StackPtr->Parameters.Write.ByteOffset = *StartingOffset;
         }
       else
         {
            SET_LARGE_INTEGER_LOW_PART(StackPtr->Parameters.Write.ByteOffset, 0);
            SET_LARGE_INTEGER_HIGH_PART(StackPtr->Parameters.Write.ByteOffset, 0);
         }
     }

   return(Irp);
}

