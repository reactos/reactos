/* $Id: vpb.c,v 1.14 2001/11/02 22:22:33 hbirr Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/vpb.c
 * PURPOSE:         Volume Parameter Block managment
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/io.h>
#include <internal/mm.h>
#include <internal/pool.h>


#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static KSPIN_LOCK IoVpbLock;

#define TAG_VPB    TAG('V', 'P', 'B', ' ')
#define TAG_SYSB   TAG('S', 'Y', 'S', 'B')

/* FUNCTIONS *****************************************************************/

VOID
IoInitVpbImplementation(VOID)
{
   KeInitializeSpinLock(&IoVpbLock);
}

NTSTATUS
IoAttachVpb(PDEVICE_OBJECT DeviceObject)
{
   PVPB Vpb;
   
   Vpb = ExAllocatePoolWithTag(NonPagedPool,
			       sizeof(VPB),
			       TAG_VPB);
   if (Vpb == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   Vpb->Type = 0;
   Vpb->Size = sizeof(VPB) / sizeof(DWORD);
   Vpb->Flags = 0;
   Vpb->VolumeLabelLength = 0;
   Vpb->DeviceObject = NULL;
   Vpb->RealDevice = DeviceObject;
   Vpb->SerialNumber = 0;
   Vpb->ReferenceCount = 0;
   RtlZeroMemory(Vpb->VolumeLabel,
		 sizeof(WCHAR) * MAXIMUM_VOLUME_LABEL_LENGTH);
   
   DeviceObject->Vpb = Vpb;
   
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtQueryVolumeInformationFile(IN HANDLE FileHandle,
			     OUT PIO_STATUS_BLOCK IoStatusBlock,
			     OUT PVOID FsInformation,
			     IN ULONG Length,
			     IN FS_INFORMATION_CLASS FsInformationClass)

/*
 * FUNCTION: Queries the volume information
 * ARGUMENTS: 
 *	  FileHandle  = Handle to a file object on the target volume
 *	  ReturnLength = DataWritten
 *	  FsInformation = Caller should supply storage for the information 
 *	                  structure.
 *	  Length = Size of the information structure
 *	  FsInformationClass = Index to a information structure
 *
 *		FileFsVolumeInformation		FILE_FS_VOLUME_INFORMATION
 *		FileFsLabelInformation		FILE_FS_LABEL_INFORMATION
 *		FileFsSizeInformation		FILE_FS_SIZE_INFORMATION
 *		FileFsDeviceInformation		FILE_FS_DEVICE_INFORMATION
 *		FileFsAttributeInformation	FILE_FS_ATTRIBUTE_INFORMATION
 *		FileFsControlInformation	
 *		FileFsQuotaQueryInformation	--
 *		FileFsQuotaSetInformation	--
 *		FileFsMaximumInformation	
 *
 * RETURNS: Status
 */
{
   PFILE_OBJECT FileObject;
   PDEVICE_OBJECT DeviceObject;
   PIRP Irp;
   KEVENT Event;
   NTSTATUS Status;
   PIO_STACK_LOCATION StackPtr;
   PVOID SystemBuffer;
   IO_STATUS_BLOCK IoSB;
   
   assert(IoStatusBlock != NULL);
   assert(FsInformation != NULL);
   
   DPRINT("FsInformation %p\n", FsInformation);
   
   Status = ObReferenceObjectByHandle(FileHandle,
				      FILE_READ_ATTRIBUTES,
				      NULL,
				      UserMode,
				      (PVOID*)&FileObject,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   DeviceObject = FileObject->DeviceObject;
   
   KeInitializeEvent(&Event,
		     NotificationEvent,
		     FALSE);
   
   Irp = IoAllocateIrp(DeviceObject->StackSize,
		       TRUE);
   if (Irp == NULL)
     {
	ObDereferenceObject(FileObject);
	return(STATUS_INSUFFICIENT_RESOURCES);
     }
   
   SystemBuffer = ExAllocatePoolWithTag(NonPagedPool,
					Length,
					TAG_SYSB);
   if (SystemBuffer == NULL)
     {
	IoFreeIrp(Irp);
	ObDereferenceObject(FileObject);
	return(STATUS_INSUFFICIENT_RESOURCES);
     }
   
   Irp->AssociatedIrp.SystemBuffer = SystemBuffer;
   Irp->UserEvent = &Event;
   Irp->UserIosb = &IoSB;
   Irp->Tail.Overlay.Thread = PsGetCurrentThread();
   
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->MajorFunction = IRP_MJ_QUERY_VOLUME_INFORMATION;
   StackPtr->MinorFunction = 0;
   StackPtr->Flags = 0;
   StackPtr->Control = 0;
   StackPtr->DeviceObject = DeviceObject;
   StackPtr->FileObject = FileObject;
   StackPtr->Parameters.QueryVolume.Length = Length;
   StackPtr->Parameters.QueryVolume.FsInformationClass =
	FsInformationClass;
   
   Status = IoCallDriver(DeviceObject,
			 Irp);
   if (Status == STATUS_PENDING)
     {
	KeWaitForSingleObject(&Event,
			      UserRequest,
			      KernelMode,
			      FALSE,
			      NULL);
	Status = IoSB.Status;
     }
   DPRINT("Status %x\n", Status);
   
   if (NT_SUCCESS(Status))
     {
	DPRINT("Information %lu\n", IoStatusBlock->Information);
	MmSafeCopyToUser(FsInformation,
			 SystemBuffer,
			 IoSB.Information);
     }
   if (IoStatusBlock)
     {
       *IoStatusBlock = IoSB;
     }
   ExFreePool(SystemBuffer);
   ObDereferenceObject(FileObject);
   
   return(Status);
}


NTSTATUS STDCALL
IoQueryVolumeInformation(IN PFILE_OBJECT FileObject,
			 IN FS_INFORMATION_CLASS FsInformationClass,
			 IN ULONG Length,
			 OUT PVOID FsInformation,
			 OUT PULONG ReturnedLength)
{
   IO_STATUS_BLOCK IoStatusBlock;
   PIO_STACK_LOCATION StackPtr;
   PDEVICE_OBJECT DeviceObject;
   PIRP Irp;
   KEVENT Event;
   NTSTATUS Status;
   
   assert(FsInformation != NULL);
   
   DPRINT("FsInformation %p\n", FsInformation);
   
   Status = ObReferenceObjectByPointer(FileObject,
				       FILE_READ_ATTRIBUTES,
				       IoFileObjectType,
				       KernelMode);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   DeviceObject = FileObject->DeviceObject;
   
   KeInitializeEvent(&Event,
		     NotificationEvent,
		     FALSE);
   
   Irp = IoAllocateIrp(DeviceObject->StackSize,
		       TRUE);
   if (Irp == NULL)
     {
	ObDereferenceObject(FileObject);
	return(STATUS_INSUFFICIENT_RESOURCES);
     }
   
   Irp->AssociatedIrp.SystemBuffer = FsInformation;
   Irp->UserEvent = &Event;
   Irp->UserIosb = &IoStatusBlock;
   Irp->Tail.Overlay.Thread = PsGetCurrentThread();
   
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->MajorFunction = IRP_MJ_QUERY_VOLUME_INFORMATION;
   StackPtr->MinorFunction = 0;
   StackPtr->Flags = 0;
   StackPtr->Control = 0;
   StackPtr->DeviceObject = DeviceObject;
   StackPtr->FileObject = FileObject;
   StackPtr->Parameters.QueryVolume.Length = Length;
   StackPtr->Parameters.QueryVolume.FsInformationClass =
	FsInformationClass;
   
   Status = IoCallDriver(DeviceObject,
			 Irp);
   if (Status == STATUS_PENDING)
     {
	KeWaitForSingleObject(&Event,
			      UserRequest,
			      KernelMode,
			      FALSE,
			      NULL);
	Status = IoStatusBlock.Status;
     }
   DPRINT("Status %x\n", Status);
   
   if (ReturnedLength != NULL)
     {
	*ReturnedLength = IoStatusBlock.Information;
     }
   ObDereferenceObject(FileObject);
   
   return(Status);
}


NTSTATUS STDCALL
NtSetVolumeInformationFile(IN HANDLE FileHandle,
			   OUT PIO_STATUS_BLOCK IoStatusBlock,
			   IN PVOID FsInformation,
			   IN ULONG Length,
			   IN FS_INFORMATION_CLASS FsInformationClass)
{
   PFILE_OBJECT FileObject;
   PDEVICE_OBJECT DeviceObject;
   PIRP Irp;
   KEVENT Event;
   NTSTATUS Status;
   PIO_STACK_LOCATION StackPtr;
   PVOID SystemBuffer;
   IO_STATUS_BLOCK IoSB;
   
   Status = ObReferenceObjectByHandle(FileHandle,
				      FILE_WRITE_ATTRIBUTES,
				      NULL,
				      UserMode,
				      (PVOID*)&FileObject,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   DeviceObject = FileObject->DeviceObject;
   
   KeInitializeEvent(&Event,
		     NotificationEvent,
		     FALSE);
   
   Irp = IoAllocateIrp(DeviceObject->StackSize,TRUE);
   if (Irp == NULL)
     {
	ObDereferenceObject(FileObject);
	return(STATUS_INSUFFICIENT_RESOURCES);
     }
   
   SystemBuffer = ExAllocatePoolWithTag(NonPagedPool,
					Length,
					TAG_SYSB);
   if (SystemBuffer == NULL)
     {
	IoFreeIrp(Irp);
	ObDereferenceObject(FileObject);
	return(STATUS_INSUFFICIENT_RESOURCES);
     }
   
   MmSafeCopyFromUser(SystemBuffer,
		      FsInformation,
		      Length);
   
   Irp->AssociatedIrp.SystemBuffer = SystemBuffer;
   Irp->UserEvent = &Event;
   Irp->UserIosb = &IoSB;
   Irp->Tail.Overlay.Thread = PsGetCurrentThread();
   
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->MajorFunction = IRP_MJ_SET_VOLUME_INFORMATION;
   StackPtr->MinorFunction = 0;
   StackPtr->Flags = 0;
   StackPtr->Control = 0;
   StackPtr->DeviceObject = DeviceObject;
   StackPtr->FileObject = FileObject;
   StackPtr->Parameters.SetVolume.Length = Length;
   StackPtr->Parameters.SetVolume.FsInformationClass =
	FsInformationClass;
   
   Status = IoCallDriver(DeviceObject,Irp);
   if (Status == STATUS_PENDING)
     {
	KeWaitForSingleObject(&Event,
			      UserRequest,
			      KernelMode,
			      FALSE,
			      NULL);
        Status = IoSB.Status;
     }
   if (IoStatusBlock)
   {
     *IoStatusBlock = IoSB;
   }
   ExFreePool(SystemBuffer);
   
   return(Status);
}


VOID STDCALL
IoAcquireVpbSpinLock(OUT PKIRQL Irql)
{
   KeAcquireSpinLock(&IoVpbLock,
		     Irql);
}


VOID STDCALL
IoReleaseVpbSpinLock(IN KIRQL Irql)
{
   KeReleaseSpinLock(&IoVpbLock,
		     Irql);
}


NTSTATUS STDCALL
IoVerifyVolume(IN PDEVICE_OBJECT DeviceObject,
	       IN BOOLEAN AllowRawMount)
{
   UNIMPLEMENTED;
   return(STATUS_NOT_IMPLEMENTED);
}


/* EOF */
