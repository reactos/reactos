/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: fsctl.c,v 1.9 2002/09/08 10:22:09 chorns Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/cdfs/fsctl.c
 * PURPOSE:          CDROM (ISO 9660) filesystem driver
 * PROGRAMMER:       Art Yerkes
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>

#include "cdfs.h"

/* FUNCTIONS ****************************************************************/

static VOID
CdfsGetPVDData(PUCHAR Buffer,
	       PDEVICE_EXTENSION Vcb,
	       PVPB Vpb)
{
  PPVD Pvd;
  ULONG i;
  PCHAR pc;
  PWCHAR pw;

  union
    {
      ULONG Value;
      UCHAR Part[4];
    } Serial;

  Pvd = (PPVD)Buffer;

  /* Calculate the volume serial number */
  Serial.Value = 0;
  for (i = 0; i < 2048; i += 4)
    {
      /* DON'T optimize this to ULONG!!! (breaks overflow) */
      Serial.Part[0] += Buffer[i+3];
      Serial.Part[1] += Buffer[i+2];
      Serial.Part[2] += Buffer[i+1];
      Serial.Part[3] += Buffer[i+0];
    }
  Vpb->SerialNumber = Serial.Value;

  /* Extract the volume label */
  pc = Pvd->VolumeId;
  pw = Vpb->VolumeLabel;
  for (i = 0; i < MAXIMUM_VOLUME_LABEL_LENGTH && *pc != ' '; i++)
    {
      *pw++ = (WCHAR)*pc++;
    }
  *pw = 0;
  Vpb->VolumeLabelLength = i;

  Vcb->CdInfo.VolumeSpaceSize = Pvd->VolumeSpaceSizeL;
  Vcb->CdInfo.RootStart = Pvd->RootDirRecord.ExtentLocationL;
  Vcb->CdInfo.RootSize = Pvd->RootDirRecord.DataLengthL;

  DPRINT("VolumeSerial: %08lx\n", Vpb->SerialNumber);
  DPRINT("VolumeLabel: '%S'\n", Vpb->VolumeLabel);
  DPRINT("VolumeLabelLength: %lu\n", Vpb->VolumeLabelLength);
  DPRINT("VolumeSize: %lu\n", Pvd->VolumeSpaceSizeL);
  DPRINT("RootStart: %lu\n", Pvd->RootDirRecord.ExtentLocationL);
  DPRINT("RootSize: %lu\n", Pvd->RootDirRecord.DataLengthL);
}


static VOID
CdfsGetSVDData(PUCHAR Buffer,
	       PDEVICE_EXTENSION Vcb)
{
  PSVD Svd;
  ULONG JolietLevel = 0;

  Svd = (PSVD)Buffer;

  DPRINT("EscapeSequences: '%.32s'\n", Svd->EscapeSequences);

  if (strncmp(Svd->EscapeSequences, "%/@", 3) == 0)
    {
      DPRINT("Joliet extension found (UCS-2 Level 1)\n");
      JolietLevel = 1;
    }
  else if (strncmp(Svd->EscapeSequences, "%/C", 3) == 0)
    {
      DPRINT("Joliet extension found (UCS-2 Level 2)\n");
      JolietLevel = 2;
    }
  else if (strncmp(Svd->EscapeSequences, "%/E", 3) == 0)
    {
      DPRINT("Joliet extension found (UCS-2 Level 3)\n");
      JolietLevel = 3;
    }

  Vcb->CdInfo.JolietLevel = JolietLevel;

  if (JolietLevel != 0)
    {
      Vcb->CdInfo.RootStart = Svd->RootDirRecord.ExtentLocationL;
      Vcb->CdInfo.RootSize = Svd->RootDirRecord.DataLengthL;

      DPRINT("RootStart: %lu\n", Svd->RootDirRecord.ExtentLocationL);
      DPRINT("RootSize: %lu\n", Svd->RootDirRecord.DataLengthL);
    }
}


static NTSTATUS
CdfsGetVolumeData(PDEVICE_OBJECT DeviceObject,
		  PDEVICE_EXTENSION Vcb)
{
  PUCHAR Buffer;
  NTSTATUS Status;
  ULONG Sector;
  PVD_HEADER VdHeader;

  Vcb->CdInfo.JolietLevel = 0;
  Sector = CDFS_PRIMARY_DESCRIPTOR_LOCATION;

  Buffer = ExAllocatePool(NonPagedPool,
			  CDFS_BASIC_SECTOR);
  if (Buffer == NULL)
    return(STATUS_INSUFFICIENT_RESOURCES);

  VdHeader = (PVD_HEADER)Buffer;

  do
    {
      /* Read the Primary Volume Descriptor (PVD) */
      Status = CdfsReadRawSectors(DeviceObject,
				  Sector,
				  1,
				  Buffer);
      if (!NT_SUCCESS(Status))
	return(Status);

      switch (VdHeader->VdType)
	{
	  case 0:
	    DPRINT("BootVolumeDescriptor found!\n");
	    break;

	  case 1:
	    DPRINT("PrimaryVolumeDescriptor found!\n");
	    CdfsGetPVDData(Buffer, Vcb, DeviceObject->Vpb);
	    break;

	  case 2:
	    DPRINT("SupplementaryVolumeDescriptor found!\n");
	    CdfsGetSVDData(Buffer, Vcb);
	    break;

	  case 3:
	    DPRINT("VolumePartitionDescriptor found!\n");
	    break;

	  case 255:
	    DPRINT("VolumeDescriptorSetTerminator found!\n");
	    break;

	  default:
	    DPRINT1("Unknown volume descriptor type %u found!\n", VdHeader->VdType);
	    break;
	}

      Sector++;
    }
  while (VdHeader->VdType != 255);

  ExFreePool(Buffer);

  return(STATUS_SUCCESS);
}


static NTSTATUS
CdfsHasFileSystem(PDEVICE_OBJECT DeviceToMount)
/*
 * FUNCTION: Tests if the device contains a filesystem that can be mounted 
 * by this fsd
 */
{
  PUCHAR Buffer;
  NTSTATUS Status;

  Buffer = ExAllocatePool(NonPagedPool,
			  CDFS_BASIC_SECTOR);
  if (Buffer == NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  DPRINT("CDFS: Checking on mount of device %08x\n", DeviceToMount);

  Status = CdfsReadRawSectors(DeviceToMount,
			      CDFS_PRIMARY_DESCRIPTOR_LOCATION,
			      1,
			      Buffer);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  Buffer[6] = 0;
  DPRINT("CD-identifier: [%.5s]\n", Buffer + 1);

  Status = (Buffer[0] == 1 &&
	    Buffer[1] == 'C' &&
	    Buffer[2] == 'D' &&
	    Buffer[3] == '0' &&
	    Buffer[4] == '0' &&
	    Buffer[5] == '1') ? STATUS_SUCCESS : STATUS_UNRECOGNIZED_VOLUME;

  ExFreePool(Buffer);

  return(Status);
}


static NTSTATUS
CdfsMountVolume(PDEVICE_OBJECT DeviceObject,
		PIRP Irp)
{
  PDEVICE_EXTENSION DeviceExt = NULL;
  PDEVICE_OBJECT NewDeviceObject = NULL;
  PDEVICE_OBJECT DeviceToMount;
  PIO_STACK_LOCATION Stack;
  PFCB Fcb = NULL;
  PCCB Ccb = NULL;
  PVPB Vpb;
  NTSTATUS Status;

  DPRINT("CdfsMountVolume() called\n");

  if (DeviceObject != CdfsGlobalData->DeviceObject)
    {
      Status = STATUS_INVALID_DEVICE_REQUEST;
      goto ByeBye;
    }

  Stack = IoGetCurrentIrpStackLocation(Irp);
  DeviceToMount = Stack->Parameters.MountVolume.DeviceObject;
  Vpb = Stack->Parameters.MountVolume.Vpb;

  Status = CdfsHasFileSystem(DeviceToMount);
  if (!NT_SUCCESS(Status))
    {
      goto ByeBye;
    }

  Status = IoCreateDevice(CdfsGlobalData->DriverObject,
			  sizeof(DEVICE_EXTENSION),
			  NULL,
			  FILE_DEVICE_FILE_SYSTEM,
//			  FILE_DEVICE_DISK_FILE_SYSTEM,
			  0,
			  FALSE,
			  &NewDeviceObject);
  if (!NT_SUCCESS(Status))
    goto ByeBye;

  NewDeviceObject->Flags = NewDeviceObject->Flags | DO_DIRECT_IO;
  DeviceExt = (PVOID)NewDeviceObject->DeviceExtension;
  RtlZeroMemory(DeviceExt,
		sizeof(DEVICE_EXTENSION));

  Status = CdfsGetVolumeData(DeviceToMount,
			     DeviceExt);
  if (!NT_SUCCESS(Status))
    goto ByeBye;

  NewDeviceObject->Vpb = DeviceToMount->Vpb;

  DeviceExt->StorageDevice = DeviceToMount;
  DeviceExt->StorageDevice->Vpb->DeviceObject = NewDeviceObject;
  DeviceExt->StorageDevice->Vpb->RealDevice = DeviceExt->StorageDevice;
  DeviceExt->StorageDevice->Vpb->Flags |= VPB_MOUNTED;
  DeviceObject->StackSize = DeviceExt->StorageDevice->StackSize + 1;
  DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

  DeviceExt->StreamFileObject = IoCreateStreamFileObject(NULL,
							 DeviceExt->StorageDevice);

  Fcb = CdfsCreateFCB(NULL);
  if (Fcb == NULL)
    {
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto ByeBye;
    }

  Ccb = ExAllocatePoolWithTag(NonPagedPool,
			      sizeof(CCB),
			      TAG_CCB);
  if (Ccb == NULL)
    {
      Status =  STATUS_INSUFFICIENT_RESOURCES;
      goto ByeBye;
    }
  RtlZeroMemory(Ccb,
		sizeof(CCB));

  DeviceExt->StreamFileObject->Flags = DeviceExt->StreamFileObject->Flags | FO_FCB_IS_VALID | FO_DIRECT_CACHE_PAGING_READ;
  DeviceExt->StreamFileObject->FsContext = (PVOID)&Fcb->RFCB;
  DeviceExt->StreamFileObject->FsContext2 = Ccb;
  DeviceExt->StreamFileObject->SectionObjectPointers = &Fcb->SectionObjectPointers;
  DeviceExt->StreamFileObject->PrivateCacheMap = NULL;
  DeviceExt->StreamFileObject->Vpb = DeviceExt->Vpb;
  Ccb->Fcb = Fcb;
  Ccb->PtrFileObject = DeviceExt->StreamFileObject;
  Fcb->FileObject = DeviceExt->StreamFileObject;
  Fcb->DevExt = (PDEVICE_EXTENSION)DeviceExt->StorageDevice;

  Fcb->Flags = FCB_IS_VOLUME_STREAM;

  Fcb->RFCB.FileSize.QuadPart = DeviceExt->CdInfo.VolumeSpaceSize * BLOCKSIZE;
  Fcb->RFCB.ValidDataLength.QuadPart = DeviceExt->CdInfo.VolumeSpaceSize * BLOCKSIZE;
  Fcb->RFCB.AllocationSize.QuadPart = DeviceExt->CdInfo.VolumeSpaceSize * BLOCKSIZE;

  Fcb->Entry.ExtentLocationL = 0;
  Fcb->Entry.DataLengthL = DeviceExt->CdInfo.VolumeSpaceSize * BLOCKSIZE;

  Status = CcRosInitializeFileCache(DeviceExt->StreamFileObject,
				    &Fcb->RFCB.Bcb,
				    PAGESIZE);
  if (!NT_SUCCESS (Status))
    {
      DbgPrint("CcRosInitializeFileCache failed\n");
      goto ByeBye;
    }

  ExInitializeResourceLite(&DeviceExt->DirResource);
//  ExInitializeResourceLite(&DeviceExt->FatResource);

  KeInitializeSpinLock(&DeviceExt->FcbListLock);
  InitializeListHead(&DeviceExt->FcbListHead);

  Status = STATUS_SUCCESS;

ByeBye:
  if (!NT_SUCCESS(Status))
    {
      /* Cleanup */
      if (DeviceExt && DeviceExt->StreamFileObject)
	ObDereferenceObject(DeviceExt->StreamFileObject);
      if (Fcb)
	ExFreePool(Fcb);
      if (Ccb)
	ExFreePool(Ccb);
      if (NewDeviceObject)
	IoDeleteDevice(NewDeviceObject);
    }

  DPRINT("CdfsMountVolume() done (Status: %lx)\n", Status);

  return(Status);
}


static NTSTATUS
CdfsVerifyVolume(PDEVICE_OBJECT DeviceObject,
		 PIRP Irp)
{
  PDEVICE_OBJECT DeviceToVerify;
  PIO_STACK_LOCATION Stack;
  PUCHAR Buffer;
  ULONG Sector;
  ULONG i;
  NTSTATUS Status;

  union
    {
      ULONG Value;
      UCHAR Part[4];
    } Serial;

  DPRINT1("CdfsVerifyVolume() called\n");

#if 0
  if (DeviceObject != CdfsGlobalData->DeviceObject)
    {
      DPRINT1("DeviceObject != CdfsGlobalData->DeviceObject\n");
      return(STATUS_INVALID_DEVICE_REQUEST);
    }
#endif

  Stack = IoGetCurrentIrpStackLocation(Irp);
  DeviceToVerify = Stack->Parameters.VerifyVolume.DeviceObject;

  DPRINT("Device object %p  Device to verify %p\n", DeviceObject, DeviceToVerify);

  Sector = CDFS_PRIMARY_DESCRIPTOR_LOCATION;

  Buffer = ExAllocatePool(NonPagedPool,
			  CDFS_BASIC_SECTOR);
  if (Buffer == NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  do
    {
      /* Read the Primary Volume Descriptor (PVD) */
      Status = CdfsReadRawSectors(DeviceToVerify,
				  Sector,
				  1,
				  Buffer);
      DPRINT("CdfsReadRawSectors() status %lx\n", Status);
      if (!NT_SUCCESS(Status))
	{
	  goto ByeBye;
	}

      if (Buffer[0] == 1 &&
	  Buffer[1] == 'C' &&
	  Buffer[2] == 'D' &&
	  Buffer[3] == '0' &&
	  Buffer[4] == '0' &&
	  Buffer[5] == '1')
	{
	  break;
	}

      Sector++;
    }
  while (Buffer[0] != 255);

  if (Buffer[0] == 255)
    goto ByeBye;

  Status = STATUS_WRONG_VOLUME;

  /* Calculate the volume serial number */
  Serial.Value = 0;
  for (i = 0; i < 2048; i += 4)
    {
      /* DON'T optimize this to ULONG!!! (breaks overflow) */
      Serial.Part[0] += Buffer[i+3];
      Serial.Part[1] += Buffer[i+2];
      Serial.Part[2] += Buffer[i+1];
      Serial.Part[3] += Buffer[i+0];
    }

  DPRINT("Current serial number %08lx  Vpb serial number %08lx\n",
	  Serial.Value, DeviceToVerify->Vpb->SerialNumber);

  if (Serial.Value == DeviceToVerify->Vpb->SerialNumber)
    Status = STATUS_SUCCESS;

ByeBye:
  ExFreePool(Buffer);

  DPRINT("CdfsVerifyVolume() done (Status: %lx)\n", Status);

  return(Status);
}


NTSTATUS STDCALL
CdfsFileSystemControl(PDEVICE_OBJECT DeviceObject,
		      PIRP Irp)
{
  PIO_STACK_LOCATION Stack;
  NTSTATUS Status;

  DPRINT("CdfsFileSystemControl() called\n");

  Stack = IoGetCurrentIrpStackLocation(Irp);

  switch (Stack->MinorFunction)
    {
      case IRP_MN_USER_FS_REQUEST:
	DPRINT("CDFS: IRP_MN_USER_FS_REQUEST\n");
	Status = STATUS_INVALID_DEVICE_REQUEST;
	break;

      case IRP_MN_MOUNT_VOLUME:
	DPRINT("CDFS: IRP_MN_MOUNT_VOLUME\n");
	Status = CdfsMountVolume(DeviceObject, Irp);
	break;

      case IRP_MN_VERIFY_VOLUME:
	DPRINT1("CDFS: IRP_MN_VERIFY_VOLUME\n");
	Status = CdfsVerifyVolume(DeviceObject, Irp);
	break;

      default:
	DPRINT("CDFS FSC: MinorFunction %d\n", Stack->MinorFunction);
	Status = STATUS_INVALID_DEVICE_REQUEST;
	break;
    }

  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = 0;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(Status);
}

/* EOF */
