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
/* $Id: fsctl.c,v 1.1 2002/04/15 20:39:49 ekohl Exp $
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

//#define NDEBUG
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

  DPRINT1("VolumeSerial: %08lx\n", Vpb->SerialNumber);
  DPRINT1("VolumeLabel: '%S'\n", Vpb->VolumeLabel);
  DPRINT1("VolumeLabelLength: %lu\n", Vpb->VolumeLabelLength);
  DPRINT1("VolumeSize: %lu\n", Pvd->VolumeSpaceSizeL);
  DPRINT1("RootStart: %lu\n", Pvd->RootDirRecord.ExtentLocationL);
  DPRINT1("RootSize: %lu\n", Pvd->RootDirRecord.DataLengthL);
}


static VOID
CdfsGetSVDData(PUCHAR Buffer,
	       PDEVICE_EXTENSION Vcb)
{
  PSVD Svd;
  ULONG JolietLevel = 0;

  Svd = (PSVD)Buffer;

  DPRINT1("EscapeSequences: '%.32s'\n", Svd->EscapeSequences);

  if (strncmp(Svd->EscapeSequences, "%/@", 3) == 0)
    {
      DPRINT1("Joliet extension found (UCS-2 Level 1)\n");
      JolietLevel = 1;
    }
  else if (strncmp(Svd->EscapeSequences, "%/C", 3) == 0)
    {
      DPRINT1("Joliet extension found (UCS-2 Level 2)\n");
      JolietLevel = 2;
    }
  else if (strncmp(Svd->EscapeSequences, "%/E", 3) == 0)
    {
      DPRINT1("Joliet extension found (UCS-2 Level 3)\n");
      JolietLevel = 3;
    }

  Vcb->CdInfo.JolietLevel = JolietLevel;

  if (JolietLevel != 0)
    {
      Vcb->CdInfo.RootStart = Svd->RootDirRecord.ExtentLocationL;
      Vcb->CdInfo.RootSize = Svd->RootDirRecord.DataLengthL;

      DPRINT1("RootStart: %lu\n", Svd->RootDirRecord.ExtentLocationL);
      DPRINT1("RootSize: %lu\n", Svd->RootDirRecord.DataLengthL);
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


  Sector = CDFS_PRIMARY_DESCRIPTOR_LOCATION;

  Buffer = ExAllocatePool(NonPagedPool,
			  CDFS_BASIC_SECTOR);
  if (Buffer == NULL)
    return(STATUS_INSUFFICIENT_RESOURCES);

  VdHeader = (PVD_HEADER)Buffer;

  do
    {
      /* Read the Primary Volume Descriptor (PVD) */
      Status = CdfsReadSectors(DeviceObject,
			       Sector,
			       1,
			       Buffer);
      if (!NT_SUCCESS(Status))
	return(Status);

      switch (VdHeader->VdType)
	{
	  case 0:
	    DPRINT1("BootVolumeDescriptor found!\n");
	    break;

	  case 1:
	    DPRINT1("PrimaryVolumeDescriptor found!\n");
	    CdfsGetPVDData(Buffer, Vcb, DeviceObject->Vpb);
	    break;

	  case 2:
	    DPRINT1("SupplementaryVolumeDescriptor found!\n");
	    CdfsGetSVDData(Buffer, Vcb);
	    break;

	  case 3:
	    DPRINT1("VolumePartitionDescriptor found!\n");
	    break;

	  case 255:
	    DPRINT1("VolumeDescriptorSetTerminator found!\n");
	    break;

	  default:
	    DPRINT1("VolumeDescriptor type %u found!\n", VdHeader->VdType);
	    break;
	}

      Sector++;
    }
  while (VdHeader->VdType != 255);

  ExFreePool(Buffer);

  return(STATUS_SUCCESS);
}


static BOOLEAN
CdfsHasFileSystem(PDEVICE_OBJECT DeviceToMount)
/*
 * FUNCTION: Tests if the device contains a filesystem that can be mounted 
 * by this fsd
 */
{
  PUCHAR bytebuf; // [CDFS_BASIC_SECTOR];
  NTSTATUS Status;
  int ret;

  bytebuf = ExAllocatePool( NonPagedPool, CDFS_BASIC_SECTOR );
  if( !bytebuf ) return FALSE;

  DPRINT1("CDFS: Checking on mount of device %08x\n", DeviceToMount);

  Status = CdfsReadSectors(DeviceToMount,
			   CDFS_PRIMARY_DESCRIPTOR_LOCATION,
			   1,
			   bytebuf);
  bytebuf[6] = 0;
  DPRINT1( "CD-identifier: [%.5s]\n", bytebuf + 1 );

  ret = 
    Status == STATUS_SUCCESS &&
    bytebuf[0] == 1 &&
    bytebuf[1] == 'C' &&
    bytebuf[2] == 'D' &&
    bytebuf[3] == '0' &&
    bytebuf[4] == '0' &&
    bytebuf[5] == '1';

  ExFreePool( bytebuf );

  return ret;
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
  NTSTATUS Status;

  DPRINT1("CdfsMountVolume() called\n");

  if (DeviceObject != CdfsGlobalData->DeviceObject)
    {
      Status = STATUS_INVALID_DEVICE_REQUEST;
      goto ByeBye;
    }

  Stack = IoGetCurrentIrpStackLocation(Irp);
  DeviceToMount = Stack->Parameters.MountVolume.DeviceObject;

  if (CdfsHasFileSystem(DeviceToMount) == FALSE)
    {
      Status = STATUS_UNRECOGNIZED_VOLUME;
      goto ByeBye;
    }

  Status = IoCreateDevice(CdfsGlobalData->DriverObject,
			  sizeof(DEVICE_EXTENSION),
			  NULL,
			  FILE_DEVICE_FILE_SYSTEM,
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
  NewDeviceObject->Vpb->Flags |= VPB_MOUNTED;
  DeviceExt->StorageDevice = IoAttachDeviceToDeviceStack(NewDeviceObject,
							 DeviceToMount);
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
  DeviceExt->StreamFileObject->Vpb = NewDeviceObject->Vpb;
  Ccb->Fcb = Fcb;
  Ccb->PtrFileObject = DeviceExt->StreamFileObject;
  Fcb->FileObject = DeviceExt->StreamFileObject;
  Fcb->DevExt = (PDEVICE_EXTENSION)DeviceExt->StorageDevice;

  Fcb->RFCB.FileSize.QuadPart = DeviceExt->CdInfo.VolumeSpaceSize * BLOCKSIZE;
  Fcb->RFCB.ValidDataLength.QuadPart = DeviceExt->CdInfo.VolumeSpaceSize * BLOCKSIZE;
  Fcb->RFCB.AllocationSize.QuadPart = ROUND_UP(DeviceExt->CdInfo.VolumeSpaceSize * BLOCKSIZE, PAGESIZE);

  Fcb->Entry.ExtentLocationL = 0;
  Fcb->Entry.DataLengthL = DeviceExt->CdInfo.VolumeSpaceSize * BLOCKSIZE;

  Status = CcRosInitializeFileCache(DeviceExt->StreamFileObject, &Fcb->RFCB.Bcb, PAGESIZE);
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

  return(Status);
}


NTSTATUS STDCALL
CdfsFileSystemControl(PDEVICE_OBJECT DeviceObject,
		      PIRP Irp)
{
  PIO_STACK_LOCATION Stack;
  NTSTATUS Status;

  DPRINT1("CdfsFileSystemControl() called\n");

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
	DPRINT("CDFS: IRP_MN_VERIFY_VOLUME\n");
	Status = STATUS_INVALID_DEVICE_REQUEST;
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
