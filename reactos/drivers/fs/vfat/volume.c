/* $Id: volume.c,v 1.4 2000/09/12 10:12:13 jean Exp $
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
#include <ddk/cctypes.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS FsdGetFsVolumeInformation(PFILE_OBJECT FileObject,
                                   PVFATFCB FCB,
                                   PDEVICE_OBJECT DeviceObject,
                                   PFILE_FS_VOLUME_INFORMATION FsVolumeInfo)
{
    DPRINT("FsdGetFsVolumeInformation()\n");
    DPRINT("FsVolumeInfo = %p\n", FsVolumeInfo);

    if (!FsVolumeInfo)
        return(STATUS_SUCCESS);


    /* valid entries */
    FsVolumeInfo->VolumeSerialNumber = DeviceObject->Vpb->SerialNumber;
    FsVolumeInfo->VolumeLabelLength  = DeviceObject->Vpb->VolumeLabelLength;
    wcscpy (FsVolumeInfo->VolumeLabel, DeviceObject->Vpb->VolumeLabel);

    /* dummy entries */
    FsVolumeInfo->VolumeCreationTime.QuadPart = 0;
    FsVolumeInfo->SupportsObjects = FALSE;

    DPRINT("Finished FsdGetFsVolumeInformation()\n");

    return(STATUS_SUCCESS);
}


NTSTATUS FsdGetFsAttributeInformation(PFILE_FS_ATTRIBUTE_INFORMATION FsAttributeInfo)
{
    DPRINT("FsdGetFsAttributeInformation()\n");
    DPRINT("FsAttributeInfo = %p\n", FsAttributeInfo);

    if (!FsAttributeInfo)
        return(STATUS_SUCCESS);

    FsAttributeInfo->FileSystemAttributes = FS_CASE_IS_PRESERVED;
    FsAttributeInfo->MaximumComponentNameLength = 255;
    FsAttributeInfo->FileSystemNameLength = 3;
    wcscpy (FsAttributeInfo->FileSystemName, L"FAT");

    DPRINT("Finished FsdGetFsAttributeInformation()\n");

    return(STATUS_SUCCESS);
}

NTSTATUS FsdGetFsSizeInformation(PDEVICE_OBJECT DeviceObject,
                                 PFILE_FS_SIZE_INFORMATION FsSizeInfo)
{
    PDEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;

    DPRINT("FsdGetFsSizeInformation()\n");
    DPRINT("FsSizeInfo = %p\n", FsSizeInfo);

    if (!FsSizeInfo)
        return(STATUS_SUCCESS);

    if (DeviceExt->FatType == FAT32)
    {
        struct _BootSector32 *BootSect = (struct _BootSector32 *)DeviceExt->Boot;

        if (BootSect->Sectors)
            FsSizeInfo->TotalAllocationUnits.QuadPart = BootSect->Sectors;
        else
            FsSizeInfo->TotalAllocationUnits.QuadPart = BootSect->SectorsHuge;

        FsSizeInfo->AvailableAllocationUnits.QuadPart =
            FAT32CountAvailableClusters(DeviceExt);

        FsSizeInfo->SectorsPerAllocationUnit = BootSect->SectorsPerCluster;
        FsSizeInfo->BytesPerSector = BootSect->BytesPerSector;
    }
    else
    {
        struct _BootSector *BootSect = (struct _BootSector *)DeviceExt->Boot;

        if (BootSect->Sectors)
            FsSizeInfo->TotalAllocationUnits.QuadPart = BootSect->Sectors;
        else
            FsSizeInfo->TotalAllocationUnits.QuadPart = BootSect->SectorsHuge;

        if (DeviceExt->FatType == FAT16)
            FsSizeInfo->AvailableAllocationUnits.QuadPart =
                FAT16CountAvailableClusters(DeviceExt);
        else
            FsSizeInfo->AvailableAllocationUnits.QuadPart =
                FAT12CountAvailableClusters(DeviceExt);

        FsSizeInfo->SectorsPerAllocationUnit = BootSect->SectorsPerCluster;
        FsSizeInfo->BytesPerSector = BootSect->BytesPerSector;
    }

    DPRINT("Finished FsdGetFsSizeInformation()\n");

    return(STATUS_SUCCESS);
}


NTSTATUS STDCALL VfatQueryVolumeInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: Retrieve the specified file information
 */
{
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   FILE_INFORMATION_CLASS FileInformationClass =
     Stack->Parameters.QueryVolume.FileInformationClass;
   PFILE_OBJECT FileObject = NULL;
   PVFATFCB FCB = NULL;
//   PVfatCCB CCB = NULL;

   NTSTATUS RC = STATUS_SUCCESS;
   void *SystemBuffer;

   /* PRECONDITION */
   assert(DeviceObject != NULL);
   assert(Irp != NULL);

   DPRINT("FsdQueryVolumeInformation(DeviceObject %x, Irp %x)\n",
          DeviceObject,Irp);

   /* INITIALIZATION */
   Stack = IoGetCurrentIrpStackLocation(Irp);
   FileInformationClass = Stack->Parameters.QueryVolume.FileInformationClass;
   FileObject = Stack->FileObject;
//   CCB = (PVfatCCB)(FileObject->FsContext2);
//   FCB = CCB->Buffer; // Should be CCB->FCB???
   FCB = ((PVFATCCB)(FileObject->FsContext2))->pFcb;

  // FIXME : determine Buffer for result :
  if (Irp->MdlAddress) 
    SystemBuffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
  else
    SystemBuffer = Irp->UserBuffer;
//   SystemBuffer = Irp->AssociatedIrp.SystemBuffer;

   DPRINT("FileInformationClass %d\n",FileInformationClass);
   DPRINT("SystemBuffer %x\n",SystemBuffer);

   switch (FileInformationClass)
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
         RC = FsdGetFsSizeInformation(DeviceObject, SystemBuffer);
         break;

      default:
         RC=STATUS_NOT_IMPLEMENTED;
   }

   Irp->IoStatus.Status = RC;
   Irp->IoStatus.Information = 0;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   return RC;
}


