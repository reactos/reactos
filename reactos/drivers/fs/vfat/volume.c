/* $Id: volume.c,v 1.27 2004/11/06 13:44:57 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/volume.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 *                   Hartmut Birr
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <wchar.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* FUNCTIONS ****************************************************************/

static NTSTATUS
FsdGetFsVolumeInformation(PDEVICE_OBJECT DeviceObject,
			  PFILE_FS_VOLUME_INFORMATION FsVolumeInfo,
			  PULONG BufferLength)
{
  DPRINT("FsdGetFsVolumeInformation()\n");
  DPRINT("FsVolumeInfo = %p\n", FsVolumeInfo);
  DPRINT("BufferLength %lu\n", *BufferLength);

  DPRINT("Required length %lu\n", (sizeof(FILE_FS_VOLUME_INFORMATION) + DeviceObject->Vpb->VolumeLabelLength));
  DPRINT("LabelLength %hu\n", DeviceObject->Vpb->VolumeLabelLength);
  DPRINT("Label %*.S\n", DeviceObject->Vpb->VolumeLabelLength / sizeof(WCHAR), DeviceObject->Vpb->VolumeLabel);

  if (*BufferLength < sizeof(FILE_FS_VOLUME_INFORMATION))
    return STATUS_INFO_LENGTH_MISMATCH;

  if (*BufferLength < (sizeof(FILE_FS_VOLUME_INFORMATION) + DeviceObject->Vpb->VolumeLabelLength))
    return STATUS_BUFFER_OVERFLOW;

  /* valid entries */
  FsVolumeInfo->VolumeSerialNumber = DeviceObject->Vpb->SerialNumber;
  FsVolumeInfo->VolumeLabelLength = DeviceObject->Vpb->VolumeLabelLength;
  memcpy(FsVolumeInfo->VolumeLabel, DeviceObject->Vpb->VolumeLabel, FsVolumeInfo->VolumeLabelLength);

  /* dummy entries */
  FsVolumeInfo->VolumeCreationTime.QuadPart = 0;
  FsVolumeInfo->SupportsObjects = FALSE;

  DPRINT("Finished FsdGetFsVolumeInformation()\n");

  *BufferLength -= (sizeof(FILE_FS_VOLUME_INFORMATION) + DeviceObject->Vpb->VolumeLabelLength);

  DPRINT("BufferLength %lu\n", *BufferLength);

  return(STATUS_SUCCESS);
}


static NTSTATUS
FsdGetFsAttributeInformation(PDEVICE_EXTENSION DeviceExt,
			     PFILE_FS_ATTRIBUTE_INFORMATION FsAttributeInfo,
			     PULONG BufferLength)
{
  WCHAR* pName; ULONG Length;
  DPRINT("FsdGetFsAttributeInformation()\n");
  DPRINT("FsAttributeInfo = %p\n", FsAttributeInfo);
  DPRINT("BufferLength %lu\n", *BufferLength);

  if (*BufferLength < sizeof (FILE_FS_ATTRIBUTE_INFORMATION))
    return STATUS_INFO_LENGTH_MISMATCH;

  if (DeviceExt->FatInfo.FatType == FAT32)
  {
    Length = 10;
    pName = L"FAT32";
  }
  else
  {
    Length = 6;
    pName = L"FAT";
  }

  DPRINT("Required length %lu\n", (sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + Length));

  if (*BufferLength < (sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + Length))
    return STATUS_BUFFER_OVERFLOW;

  FsAttributeInfo->FileSystemAttributes =
    FILE_CASE_PRESERVED_NAMES | FILE_UNICODE_ON_DISK;

  FsAttributeInfo->MaximumComponentNameLength = 255;

  FsAttributeInfo->FileSystemNameLength = Length;

  memcpy(FsAttributeInfo->FileSystemName, pName, Length );

  DPRINT("Finished FsdGetFsAttributeInformation()\n");

  *BufferLength -= (sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + Length);
  DPRINT("BufferLength %lu\n", *BufferLength);

  return(STATUS_SUCCESS);
}


static NTSTATUS
FsdGetFsSizeInformation(PDEVICE_OBJECT DeviceObject,
			PFILE_FS_SIZE_INFORMATION FsSizeInfo,
			PULONG BufferLength)
{
  PDEVICE_EXTENSION DeviceExt;
  NTSTATUS Status;

  DPRINT("FsdGetFsSizeInformation()\n");
  DPRINT("FsSizeInfo = %p\n", FsSizeInfo);

  if (*BufferLength < sizeof(FILE_FS_SIZE_INFORMATION))
    return(STATUS_BUFFER_OVERFLOW);

  DeviceExt = DeviceObject->DeviceExtension;
  Status = CountAvailableClusters(DeviceExt, &FsSizeInfo->AvailableAllocationUnits);

  FsSizeInfo->TotalAllocationUnits.QuadPart = DeviceExt->FatInfo.NumberOfClusters;
  FsSizeInfo->SectorsPerAllocationUnit = DeviceExt->FatInfo.SectorsPerCluster;
  FsSizeInfo->BytesPerSector = DeviceExt->FatInfo.BytesPerSector;

  DPRINT("Finished FsdGetFsSizeInformation()\n");
  if (NT_SUCCESS(Status))
    *BufferLength -= sizeof(FILE_FS_SIZE_INFORMATION);

  return(Status);
}


static NTSTATUS
FsdGetFsDeviceInformation(PFILE_FS_DEVICE_INFORMATION FsDeviceInfo,
			  PULONG BufferLength)
{
  DPRINT("FsdGetFsDeviceInformation()\n");
  DPRINT("FsDeviceInfo = %p\n", FsDeviceInfo);
  DPRINT("BufferLength %lu\n", *BufferLength);
  DPRINT("Required length %lu\n", sizeof(FILE_FS_DEVICE_INFORMATION));

  if (*BufferLength < sizeof(FILE_FS_DEVICE_INFORMATION))
    return(STATUS_BUFFER_OVERFLOW);

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

  return(STATUS_NOT_IMPLEMENTED);
}


NTSTATUS VfatQueryVolumeInformation(PVFAT_IRP_CONTEXT IrpContext)
/*
 * FUNCTION: Retrieve the specified volume information
 */
{
  FS_INFORMATION_CLASS FsInformationClass;
  NTSTATUS RC = STATUS_SUCCESS;
  PVOID SystemBuffer;
  ULONG BufferLength;

  /* PRECONDITION */
  ASSERT(IrpContext);

  DPRINT("VfatQueryVolumeInformation(IrpContext %x)\n", IrpContext);

  if (!ExAcquireResourceSharedLite(&((PDEVICE_EXTENSION)IrpContext->DeviceObject->DeviceExtension)->DirResource,
                                   (BOOLEAN)(IrpContext->Flags & IRPCONTEXT_CANWAIT)))
  {
     return VfatQueueRequest (IrpContext);
  }

  /* INITIALIZATION */
  FsInformationClass = IrpContext->Stack->Parameters.QueryVolume.FsInformationClass;
  BufferLength = IrpContext->Stack->Parameters.QueryVolume.Length;
  SystemBuffer = IrpContext->Irp->AssociatedIrp.SystemBuffer;


  DPRINT ("FsInformationClass %d\n", FsInformationClass);
  DPRINT ("SystemBuffer %x\n", SystemBuffer);

  switch (FsInformationClass)
    {
    case FileFsVolumeInformation:
      RC = FsdGetFsVolumeInformation(IrpContext->DeviceObject,
				     SystemBuffer,
				     &BufferLength);
      break;

    case FileFsAttributeInformation:
      RC = FsdGetFsAttributeInformation(IrpContext->DeviceObject->DeviceExtension,
					SystemBuffer,
					&BufferLength);
      break;

    case FileFsSizeInformation:
      RC = FsdGetFsSizeInformation(IrpContext->DeviceObject,
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

  ExReleaseResourceLite(&((PDEVICE_EXTENSION)IrpContext->DeviceObject->DeviceExtension)->DirResource);
  IrpContext->Irp->IoStatus.Status = RC;
  if (NT_SUCCESS(RC))
    IrpContext->Irp->IoStatus.Information =
      IrpContext->Stack->Parameters.QueryVolume.Length - BufferLength;
  else
    IrpContext->Irp->IoStatus.Information = 0;
  IoCompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
  VfatFreeIrpContext(IrpContext);

  return RC;
}


NTSTATUS VfatSetVolumeInformation(PVFAT_IRP_CONTEXT IrpContext)
/*
 * FUNCTION: Set the specified volume information
 */
{
  FS_INFORMATION_CLASS FsInformationClass;
  NTSTATUS Status = STATUS_SUCCESS;
  PVOID SystemBuffer;
  ULONG BufferLength;
  PEXTENDED_IO_STACK_LOCATION Stack = (PEXTENDED_IO_STACK_LOCATION) IrpContext->Stack;

  /* PRECONDITION */
  ASSERT(IrpContext);

  DPRINT1("VfatSetVolumeInformation(IrpContext %x)\n", IrpContext);

  if (!ExAcquireResourceExclusiveLite(&((PDEVICE_EXTENSION)IrpContext->DeviceObject->DeviceExtension)->DirResource,
                                      (BOOLEAN)(IrpContext->Flags & IRPCONTEXT_CANWAIT)))
  {
     return VfatQueueRequest (IrpContext);
  }

  FsInformationClass = Stack->Parameters.SetVolume.FsInformationClass;
  BufferLength = Stack->Parameters.SetVolume.Length;
  SystemBuffer = IrpContext->Irp->AssociatedIrp.SystemBuffer;

  DPRINT1("FsInformationClass %d\n", FsInformationClass);
  DPRINT1("BufferLength %d\n", BufferLength);
  DPRINT1("SystemBuffer %x\n", SystemBuffer);

  switch(FsInformationClass)
    {
    case FileFsLabelInformation:
      Status = FsdSetFsLabelInformation(IrpContext->DeviceObject,
					SystemBuffer);
      break;

    default:
      Status = STATUS_NOT_SUPPORTED;
    }

  ExReleaseResourceLite(&((PDEVICE_EXTENSION)IrpContext->DeviceObject->DeviceExtension)->DirResource);
  IrpContext->Irp->IoStatus.Status = Status;
  IrpContext->Irp->IoStatus.Information = 0;
  IoCompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
  VfatFreeIrpContext(IrpContext);

  return(Status);
}

/* EOF */
