/* $Id: volume.c,v 1.8 2001/06/11 19:52:22 ekohl Exp $
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
			  PFILE_FS_VOLUME_INFORMATION FsVolumeInfo)
{
  DPRINT("FsdGetFsVolumeInformation()\n");
  DPRINT("FsVolumeInfo = %p\n", FsVolumeInfo);

  if (!FsVolumeInfo)
    return (STATUS_SUCCESS);


  /* valid entries */
  FsVolumeInfo->VolumeSerialNumber = DeviceObject->Vpb->SerialNumber;
  FsVolumeInfo->VolumeLabelLength = DeviceObject->Vpb->VolumeLabelLength;
  wcscpy(FsVolumeInfo->VolumeLabel, DeviceObject->Vpb->VolumeLabel);

  /* dummy entries */
  FsVolumeInfo->VolumeCreationTime.QuadPart = 0;
  FsVolumeInfo->SupportsObjects = FALSE;

  DPRINT("Finished FsdGetFsVolumeInformation()\n");

  return(STATUS_SUCCESS);
}


static NTSTATUS
FsdGetFsAttributeInformation(PFILE_FS_ATTRIBUTE_INFORMATION FsAttributeInfo)
{
  DPRINT("FsdGetFsAttributeInformation()\n");
  DPRINT("FsAttributeInfo = %p\n", FsAttributeInfo);

  if (!FsAttributeInfo)
    return (STATUS_SUCCESS);

  FsAttributeInfo->FileSystemAttributes =
    FILE_CASE_PRESERVED_NAMES || FILE_UNICODE_ON_DISK;
  FsAttributeInfo->MaximumComponentNameLength = 255;
  FsAttributeInfo->FileSystemNameLength = 6;
  wcscpy(FsAttributeInfo->FileSystemName, L"FAT");

  DPRINT("Finished FsdGetFsAttributeInformation()\n");

  return(STATUS_SUCCESS);
}

static NTSTATUS
FsdGetFsSizeInformation(PDEVICE_OBJECT DeviceObject,
			PFILE_FS_SIZE_INFORMATION FsSizeInfo)
{
  PDEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;

  DPRINT("FsdGetFsSizeInformation()\n");
  DPRINT("FsSizeInfo = %p\n", FsSizeInfo);

  if (!FsSizeInfo)
    return(STATUS_SUCCESS);

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

  return(STATUS_SUCCESS);
}


static NTSTATUS
FsdSetFsLabelInformation(PDEVICE_OBJECT DeviceObject,
			 PFILE_FS_LABEL_INFORMATION FsLabelInfo)
{
   DPRINT1("FsdSetFsLabelInformation()\n");
   
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
				     SystemBuffer);
      break;

    case FileFsAttributeInformation:
      RC = FsdGetFsAttributeInformation(SystemBuffer);
      break;

    case FileFsSizeInformation:
      RC = FsdGetFsSizeInformation (DeviceObject, SystemBuffer);
      break;

    default:
      RC = STATUS_NOT_IMPLEMENTED;
    }

  Irp->IoStatus.Status = RC;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest (Irp, IO_NO_INCREMENT);

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
