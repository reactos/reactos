/*
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
#include <string.h>
#include <internal/string.h>
#include <internal/ob.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS IoAttachVpb(PDEVICE_OBJECT DeviceObject)
{
   PVPB Vpb;
   
   Vpb = ExAllocatePool(NonPagedPool,sizeof(VPB));
   if (Vpb==NULL)
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
   RtlZeroMemory(Vpb->VolumeLabel,sizeof(WCHAR)*MAXIMUM_VOLUME_LABEL_LENGTH);
   
   DeviceObject->Vpb = Vpb;
}

PIRP IoBuildVolumeInformationIrp(ULONG MajorFunction,
				 PFILE_OBJECT FileObject,
				 PVOID FSInformation,
				 ULONG Length,
				 CINT FSInformationClass,
				 PIO_STATUS_BLOCK IoStatusBlock,
				 PKEVENT Event)
{
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   PDEVICE_OBJECT DeviceObject;
   
   DeviceObject = FileObject->DeviceObject;
   
   Irp = IoAllocateIrp(DeviceObject->StackSize,TRUE);
   if (Irp==NULL)
     {
	return(NULL);
     }
   
   Irp->AssociatedIrp.SystemBuffer = FSInformation;
   
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->MajorFunction = MajorFunction;
   StackPtr->MinorFunction = 0;
   StackPtr->Flags = 0;
   StackPtr->Control = 0;
   StackPtr->DeviceObject = DeviceObject;
   StackPtr->FileObject = FileObject;
   Irp->UserEvent = Event;
   Irp->UserIosb = IoStatusBlock;
   
   if (MajorFunction == IRP_MJ_QUERY_VOLUME_INFORMATION)
     {
	StackPtr->Parameters.SetVolume.Length = Length;
	StackPtr->Parameters.SetVolume.FileInformationClass = 
	             FSInformationClass;	
     }
   else
     {	
	StackPtr->Parameters.QueryVolume.Length = Length;
	StackPtr->Parameters.QueryVolume.FileInformationClass = 
	             FSInformationClass;	
     }
   return(Irp);
}

NTSTATUS STDCALL NtQueryVolumeInformationFile(
					    IN HANDLE FileHandle,
					    OUT PIO_STATUS_BLOCK IoStatusBlock,
					    OUT PVOID FSInformation,
					    IN ULONG Length,
					    IN FS_INFORMATION_CLASS FSInformationClass)

/*
 * FUNCTION: Queries the volume information
 * ARGUMENTS: 
 *        FileHandle  = Handle to a file object on the target volume
 *	  ReturnLength = DataWritten
 *	  FSInformation = Caller should supply storage for the information 
 *                        structure.
 *	  Length = Size of the information structure
 *	  FSInformationClass = Index to a information structure
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
   return(ZwQueryVolumeInformationFile(FileHandle,IoStatusBlock,FSInformation,
				       Length,FSInformationClass));
}

NTSTATUS
STDCALL
ZwQueryVolumeInformationFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID FSInformation,
	IN ULONG Length,
	IN FS_INFORMATION_CLASS FSInformationClass)
{
   PFILE_OBJECT FileObject;
   PDEVICE_OBJECT DeviceObject;
   PIRP Irp;
   KEVENT Event;
   NTSTATUS Status;
   
   Status = ObReferenceObjectByHandle(FileHandle,
				      FILE_READ_ATTRIBUTES,
				      NULL,
				      UserMode,
				      (PVOID*)&FileObject,
				      NULL);   
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   DeviceObject = FileObject->DeviceObject;
   
   KeInitializeEvent(&Event,NotificationEvent,FALSE);
   
   Irp = IoBuildVolumeInformationIrp(IRP_MJ_QUERY_VOLUME_INFORMATION,
				     FileObject,
				     FSInformation,
				     Length,
				     FSInformationClass,
				     IoStatusBlock,
				     &Event);
   Status = IoCallDriver(DeviceObject,Irp);
   if (Status == STATUS_PENDING)
     {
	KeWaitForSingleObject(&Event,UserRequest,KernelMode,FALSE,NULL);
	Status = IoStatusBlock->Status;
     }
   return(Status);
}

NTSTATUS
STDCALL
NtSetVolumeInformationFile(
	IN HANDLE FileHandle,
	IN CINT VolumeInformationClass,
	PVOID VolumeInformation,
	ULONG Length
	)
{
   return(ZwSetVolumeInformationFile(FileHandle,VolumeInformationClass,
				     VolumeInformation,Length));
}

NTSTATUS
STDCALL
ZwSetVolumeInformationFile(
	IN HANDLE FileHandle,
	IN CINT VolumeInformationClass,
	PVOID VolumeInformation,
	ULONG Length
	)
{
   
   PFILE_OBJECT FileObject;
   PDEVICE_OBJECT DeviceObject;
   PIRP Irp;
   KEVENT Event;
   NTSTATUS Status;

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
   
   KeInitializeEvent(&Event,NotificationEvent,FALSE);

   Irp = IoBuildVolumeInformationIrp(IRP_MJ_SET_VOLUME_INFORMATION,
				     FileObject,
				     VolumeInformation,
				     Length,
				     VolumeInformationClass,
				     NULL,
				     &Event);
   Status = IoCallDriver(DeviceObject,Irp);
   if (Status == STATUS_PENDING)
     {
	KeWaitForSingleObject(&Event,UserRequest,KernelMode,FALSE,NULL);
     }
   return(Status);
}
