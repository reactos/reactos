/* $Id: volume.c,v 1.9 2001/06/12 12:35:42 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/vfat/volume.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <wchar.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* FUNCTIONS ****************************************************************/

static NTSTATUS
FsdGetFsVolumeInformation(PFILE_OBJECT FileObject,
			  PVFATFCB FCB,
			  PDEVICE_OBJECT DeviceObject,
			  PFILE_FS_VOLUME_INFORMATION FsVolumeInfo,
			  PULONG BufferLength)
{
  ULONG LabelLength;
  
  DPRINT("FsdGetFsVolumeInformation()\n");
  DPRINT("FsVolumeInfo = %p\n", FsVolumeInfo);
  DPRINT("BufferLength %lu\n", *BufferLength);
  DPRINT("Required length %lu\n", (sizeof(FILE_FS_VOLUME_INFORMATION) + LabelLength));
  
  LabelLength = DeviceObject->Vpb->VolumeLabelLength;
  DPRINT("LabelLength %lu\n", LabelLength);
  
  /* FIXME: This does not work correctly! Why?? */
//  if (*BufferLength < (sizeof(FILE_FS_VOLUME_INFORMATION) + LabelLength));
//    return(STATUS_BUFFER_OVERFLOW);

  /* valid entries */
  FsVolumeInfo->VolumeSerialNumber = DeviceObject->Vpb->SerialNumber;
  FsVolumeInfo->VolumeLabelLength = LabelLength;
  wcscpy(FsVolumeInfo->VolumeLabel, DeviceObject->Vpb->VolumeLabel);
  
  /* dummy entries */
  FsVolumeInfo->VolumeCreationTime.QuadPart = 0;
  FsVolumeInfo->SupportsObjects = FALSE;
  
  DPRINT("Finished FsdGetFsVolumeInformation()\n");
  
  *BufferLength -= (sizeof(FILE_FS_VOLUME_INFORMATION) + LabelLength);
  
  DPRINT("BufferLength %lu\n", *BufferLength);
  
  return(STATUS_SUCCESS);
}


static NTSTATUS
FsdGetFsAttributeInformation(PFILE_FS_ATTRIBUTE_INFORMATION FsAttributeInfo,
			     PULONG BufferLength)
{
  DPRINT("FsdGetFsAttributeInformation()\n");
  DPRINT("FsAttributeInfo = %p\n", FsAttributeInfo);
  DPRINT("BufferLength %lu\n", *BufferLength);
  DPRINT("Required length %lu\n", (sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + 6));
  
  /* FIXME: This does not work correctly! Why?? */
//  if (*BufferLength < (sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + 6));
//    return(STATUS_BUFFER_OVERFLOW);
  
  FsAttributeInfo->FileSystemAttributes =
    FILE_CASE_PRESERVED_NAMES | FILE_UNICODE_ON_DISK;
  FsAttributeInfo->MaximumComponentNameLength = 255;
  FsAttributeInfo->FileSystemNameLength = 6;
  wcscpy(FsAttributeInfo->FileSystemName, L"FAT");
  
  DPRINT("Finished FsdGetFsAttributeInformation()\n");
  
  *BufferLength -= (sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + 6);
  DPRINT("BufferLength %lu\n", *BufferLength);
  
  return(STATUS_SUCCESS);
}


static NTSTATUS
FsdGetFsSizeInformation(PDEVICE_OBJECT DeviceObject,
			PFILE_FS_SIZE_INFORMATION FsSizeInfo,
			PULONG BufferLength)
{
  PDEVICE_EXTENSION DeviceExt;
  
  DPRINT("FsdGetFsSizeInformation()\n");
  DPRINT("FsSizeInfo = %p\n", FsSizeInfo);
  
  /* FIXME: This does not work correctly! Why?? */
//  if (*BufferLength < sizeof(FILE_FS_SIZE_INFORMATION));
//    return(STATUS_BUFFER_OVERFLOW);
  
  DeviceExt = DeviceObject->DeviceExtension;
  
  if (DeviceExt->FatType == FAT32)
    {
      struct _BootSector32 *BootSect =
	(struct _BootSector32 *) DeviceExt->Boot;

      FsSizeInfo->TotalAllocationUnits.QuadPart = ((BootSect->Sectors ? BootSect->Sectors : BootSect->SectorsHuge)-DeviceExt->dataStart)/BootSect->SectorsPerCluster;



      FsSizeInfo->AvailableAllocationUnits.QuadPart = FAT32CountAvailableClusters (DeviceExt);

      FsSizeInfo->SectorsPerAllocationUnit = BootSect->SectorsPerCluster;
      FsSizeInfo->BytesPerSector = BootSect->BytesPerSector;
    }
  else
    {
      struct _BootSector *BootSect = (struct _BootSector *) DeviceExt->Boot;

      FsSizeInfo->TotalAllocationUnits.QuadPart = ((BootSect->Sectors ? BootSect->Sectors : BootSect->SectorsHuge)-DeviceExt->dataStart)/BootSect->SectorsPerCluster;

      if (DeviceExt->FatType == FAT16)
	FsSizeInfo->AvailableAllocationUnits.QuadPart =
#if 0
	  FAT16CountAvailableClusters (DeviceExt);
#else
0;
#endif
      else
	FsSizeInfo->AvailableAllocationUnits.QuadPart =
	  FAT12CountAvailableClusters (DeviceExt);
      FsSizeInfo->SectorsPerAllocationUnit = BootSect->SectorsPerCluster;
      FsSizeInfo->BytesPerSector = BootSect->BytesPerSector;
    }
  
  DPRINT("Finished FsdGetFsSizeInformation()\n");
  
  *BufferLength -= sizeof(FILE_FS_SIZE_INFORMATION);
  
  return(STATUS_SUCCESS);
}


static NTSTATUS
FsdGetFsDeviceInformation(PFILE_FS_DEVICE_INFORMATION FsDeviceInfo,
			  PULONG BufferLength)
{
  DPRINT("FsdGetFsDeviceInformation()\n");
  DPRINT("FsDeviceInfo = %p\n", FsDeviceInfo);
  DPRINT("BufferLength %lu\n", *BufferLength);
  DPRINT("Required length %lu\n", sizeof(FILE_FS_DEVICE_INFORMATION));
  
  /* FIXME: This does not work correctly! Why?? */
//  if (*BufferLength < sizeof(FILE_FS_DEVICE_INFORMATION));
//    return(STATUS_BUFFER_OVERFLOW);
  
  FsDeviceInfo->DeviceType = FILE_DEVICE_DISK;
  FsDeviceInfo->Characteristics = 0; /* FIXME: fix this !! */
  
  DPRINT("FsdGetFsDeviceInformation() finished.\n");
  
  *BufferLength -= sizeof(FILE_FS_DEVICE_INFORMATION);
  DPRINT("BufferLength %lu\n", *BufferLength);
  
  return(STATUS_SUCCESS);
}


static NTSTATUS
FsdSetFsLabelInformation(PDEVICE_OBJECT DeviceObject,
			 PFILE_FS_LABEL_INFORMATION FsLabelInfo)
{
   DPRINT("FsdSetFsLabelInformation()\n");
   
   return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS STDCALL
VfatQueryVolumeInformation(PDEVICE_OBJECT DeviceObject,
			   PIRP Irp)
/*
 * FUNCTION: Retrieve the specified volume information
 */
{
  PIO_STACK_LOCATION Stack;
  FS_INFORMATION_CLASS FsInformationClass;
  PFILE_OBJECT FileObject = NULL;
  PVFATFCB FCB = NULL;
  NTSTATUS RC = STATUS_SUCCESS;
  PVOID SystemBuffer;
  ULONG BufferLength;

  /* PRECONDITION */
  assert(DeviceObject != NULL);
  assert(Irp != NULL);

  DPRINT("FsdQueryVolumeInformation(DeviceObject %x, Irp %x)\n",
	 DeviceObject, Irp);

  /* INITIALIZATION */
  Stack = IoGetCurrentIrpStackLocation (Irp);
  FsInformationClass = Stack->Parameters.QueryVolume.FsInformationClass;
  BufferLength = Stack->Parameters.QueryVolume.Length;
  SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
  FileObject = Stack->FileObject;
//   CCB = (PVfatCCB)(FileObject->FsContext2);
//   FCB = CCB->Buffer; // Should be CCB->FCB???
  FCB = ((PVFATCCB) (FileObject->FsContext2))->pFcb;

  DPRINT ("FsInformationClass %d\n", FsInformationClass);
  DPRINT ("SystemBuffer %x\n", SystemBuffer);

  switch (FsInformationClass)
    {
    case FileFsVolumeInformation:
      RC = FsdGetFsVolumeInformation(FileObject,
				     FCB,
				     DeviceObject,
				     SystemBuffer,
				     &BufferLength);
      break;

    case FileFsAttributeInformation:
      RC = FsdGetFsAttributeInformation(SystemBuffer,
					&BufferLength);
      break;

    case FileFsSizeInformation:
      RC = FsdGetFsSizeInformation(DeviceObject,
				   SystemBuffer,
				   &BufferLength);
      break;

    case FileFsDeviceInformation:
      RC = FsdGetFsDeviceInformation(SystemBuffer,
				     &BufferLength);
      break;

    default:
      RC = STATUS_NOT_SUPPORTED;
    }

  Irp->IoStatus.Status = RC;
  if (NT_SUCCESS(RC))
    Irp->IoStatus.Information =
      Stack->Parameters.QueryVolume.Length - BufferLength;
  else
    Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp,
		    IO_NO_INCREMENT);

  return RC;
}


NTSTATUS STDCALL
VfatSetVolumeInformation(PDEVICE_OBJECT DeviceObject,
			 PIRP Irp)
/*
 * FUNCTION: Set the specified volume information
 */
{
   PIO_STACK_LOCATION Stack;
   FS_INFORMATION_CLASS FsInformationClass;
//   PFILE_OBJECT FileObject = NULL;
//   PVFATFCB FCB = NULL;
   NTSTATUS Status = STATUS_SUCCESS;
   PVOID SystemBuffer;
   ULONG BufferLength;

   /* PRECONDITION */
   assert(DeviceObject != NULL);
   assert(Irp != NULL);

   DPRINT("FsdSetVolumeInformation(DeviceObject %x, Irp %x)\n",
	  DeviceObject,
	  Irp);

   Stack = IoGetCurrentIrpStackLocation (Irp);
   FsInformationClass = Stack->Parameters.SetVolume.FsInformationClass;
   BufferLength = Stack->Parameters.SetVolume.Length;
   SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
//   FileObject = Stack->FileObject;
//   FCB = ((PVFATCCB) (FileObject->FsContext2))->pFcb;

   DPRINT("FsInformationClass %d\n", FsInformationClass);
   DPRINT("BufferLength %d\n", BufferLength);
   DPRINT("SystemBuffer %x\n", SystemBuffer);

   switch (FsInformationClass)
     {
     case FileFsLabelInformation:
       Status = FsdSetFsLabelInformation(DeviceObject,
					 SystemBuffer);
       break;

     default:
       Status = STATUS_NOT_SUPPORTED;
     }

   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   IoCompleteRequest(Irp,
		     IO_NO_INCREMENT);

   return(Status);
}

/* EOF */
