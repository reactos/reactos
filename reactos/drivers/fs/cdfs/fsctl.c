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
/* $Id: fsctl.c,v 1.14 2003/06/07 11:34:35 chorns Exp $
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
#include <ntos/minmax.h>

#define NDEBUG
#include <debug.h>

#include "cdfs.h"

/* FUNCTIONS ****************************************************************/

static inline
int msf_to_lba (BYTE m, BYTE s, BYTE f)
{
   return (((m * 60) + s) * 75 + f) - 150;
}

static VOID
CdfsGetPVDData(PUCHAR Buffer, 
	       PCDINFO CdInfo)
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
  CdInfo->SerialNumber = Serial.Value;

  /* Extract the volume label */
  pc = Pvd->VolumeId;
  pw = CdInfo->VolumeLabel;
  for (i = 0; i < MAXIMUM_VOLUME_LABEL_LENGTH && *pc != ' '; i++)
    {
      *pw++ = (WCHAR)*pc++;
    }
  *pw = 0;
  CdInfo->VolumeLabelLength = i;

  CdInfo->VolumeSpaceSize = Pvd->VolumeSpaceSizeL;
  CdInfo->RootStart = Pvd->RootDirRecord.ExtentLocationL;
  CdInfo->RootSize = Pvd->RootDirRecord.DataLengthL;

  DPRINT("VolumeSerial: %08lx\n", CdInfo->SerialNumber);
  DPRINT("VolumeLabel: '%S'\n", CdInfo->VolumeLabel);
  DPRINT("VolumeLabelLength: %lu\n", CdInfo->VolumeLabelLength);
  DPRINT("VolumeSize: %lu\n", Pvd->VolumeSpaceSizeL);
  DPRINT("RootStart: %lu\n", Pvd->RootDirRecord.ExtentLocationL);
  DPRINT("RootSize: %lu\n", Pvd->RootDirRecord.DataLengthL);

#if 0
  DbgPrint("******** PVD **********\n");
  DbgPrint("VdType:               %d\n", Pvd->VdType);
  DbgPrint("StandardId:           '%.*s'\n", 5, Pvd->StandardId);
  DbgPrint("VdVersion:            %d\n", Pvd->VdVersion);
  DbgPrint("SystemId:             '%.*s'\n", 32, Pvd->SystemId);
  DbgPrint("VolumeId:             '%.*s'\n", 32, Pvd->VolumeId);
  DbgPrint("VolumeSpaceSizeL:     %d (%x)\n", Pvd->VolumeSpaceSizeL, Pvd->VolumeSpaceSizeL);
  DbgPrint("VolumeSpaceSizeM:     %d (%x)\n", Pvd->VolumeSpaceSizeM, Pvd->VolumeSpaceSizeM);
  DbgPrint("VolumeSetSize:        %d (%x)\n", Pvd->VolumeSequenceNumber, Pvd->VolumeSequenceNumber);
  DbgPrint("VolumeSequenceNumber: %d (%x)\n", Pvd->VolumeSequenceNumber, Pvd->VolumeSequenceNumber);
  DbgPrint("LogicalBlockSize:     %d (%x)\n", Pvd->LogicalBlockSize, Pvd->LogicalBlockSize);
  DbgPrint("PathTableSizeL:       %d (%x)\n", Pvd->PathTableSizeL, Pvd->PathTableSizeL);
  DbgPrint("PathTableSizeM:       %d (%x)\n", Pvd->PathTableSizeM, Pvd->PathTableSizeM);
  DbgPrint("LPathTablePos:        %d (%x)\n", Pvd->LPathTablePos, Pvd->LPathTablePos);
  DbgPrint("LOptPathTablePos:     %d (%x)\n", Pvd->LOptPathTablePos, Pvd->LOptPathTablePos);
  DbgPrint("MPathTablePos:        %d (%x)\n", Pvd->MPathTablePos, Pvd->MPathTablePos);
  DbgPrint("MOptPathTablePos:     %d (%x)\n", Pvd->MOptPathTablePos, Pvd->MOptPathTablePos);
  DbgPrint("VolumeSetIdentifier:  '%.*s'\n", 128, Pvd->VolumeSetIdentifier);
  DbgPrint("PublisherIdentifier:  '%.*s'\n", 128, Pvd->PublisherIdentifier);
  DbgPrint("******** Root *********\n");
  DbgPrint("RecordLength:         %d\n", Pvd->RootDirRecord.RecordLength);
  DbgPrint("ExtAttrRecordLength:  %d\n", Pvd->RootDirRecord.ExtAttrRecordLength);
  DbgPrint("ExtentLocationL:      %d\n", Pvd->RootDirRecord.ExtentLocationL);
  DbgPrint("DataLengthL:          %d\n", Pvd->RootDirRecord.DataLengthL);
  DbgPrint("Year:                 %d\n", Pvd->RootDirRecord.Year);
  DbgPrint("Month:                %d\n", Pvd->RootDirRecord.Month);
  DbgPrint("Day:                  %d\n", Pvd->RootDirRecord.Day);
  DbgPrint("Hour:                 %d\n", Pvd->RootDirRecord.Hour);
  DbgPrint("Minute:               %d\n", Pvd->RootDirRecord.Minute);
  DbgPrint("Second:               %d\n", Pvd->RootDirRecord.Second);
  DbgPrint("TimeZone:             %d\n", Pvd->RootDirRecord.TimeZone);
  DbgPrint("FileFlags:            %d\n", Pvd->RootDirRecord.FileFlags);
  DbgPrint("FileUnitSize:         %d\n", Pvd->RootDirRecord.FileUnitSize);
  DbgPrint("InterleaveGapSize:    %d\n", Pvd->RootDirRecord.InterleaveGapSize);
  DbgPrint("VolumeSequenceNumber: %d\n", Pvd->RootDirRecord.VolumeSequenceNumber);
  DbgPrint("FileIdLength:         %d\n", Pvd->RootDirRecord.FileIdLength);
  DbgPrint("FileId:               '%.*s'\n", Pvd->RootDirRecord.FileId);
  DbgPrint("***********************\n");
#endif
}


static VOID
CdfsGetSVDData(PUCHAR Buffer,
	       PCDINFO CdInfo)
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

  CdInfo->JolietLevel = JolietLevel;

  if (JolietLevel != 0)
    {
      CdInfo->RootStart = Svd->RootDirRecord.ExtentLocationL;
      CdInfo->RootSize = Svd->RootDirRecord.DataLengthL;

      DPRINT("RootStart: %lu\n", Svd->RootDirRecord.ExtentLocationL);
      DPRINT("RootSize: %lu\n", Svd->RootDirRecord.DataLengthL);
    }
}


static NTSTATUS
CdfsGetVolumeData(PDEVICE_OBJECT DeviceObject,
		  PCDINFO CdInfo)
{
  PUCHAR Buffer;
  NTSTATUS Status;
  ULONG Sector;
  PVD_HEADER VdHeader;
  ULONG Size;
  ULONG Offset;
  ULONG i;
  struct
  {
    UCHAR  Length[2];
    UCHAR  FirstSession;
    UCHAR  LastSession;
    TRACK_DATA  TrackData;
  }
  Toc;

  DPRINT("CdfsGetVolumeData\n");
  
  Buffer = ExAllocatePool(NonPagedPool,
			  CDFS_BASIC_SECTOR);

  if (Buffer == NULL)
    return(STATUS_INSUFFICIENT_RESOURCES);
  
  Size = sizeof(Toc);
  Status = CdfsDeviceIoControl(DeviceObject,
			       IOCTL_CDROM_GET_LAST_SESSION,
			       NULL,
			       0,
			       &Toc,
			       &Size);
  if (!NT_SUCCESS(Status))
  {
     ExFreePool(Buffer);
     return Status;
  }

  DPRINT("FirstSession %d, LastSession %d, FirstTrack %d\n", 
         Toc.FirstSession, Toc.LastSession, Toc.TrackData.TrackNumber);

  Offset = 0;
  for (i = 0; i < 4; i++)
  {
     Offset = (Offset << 8) + Toc.TrackData.Address[i];
  }
  CdInfo->VolumeOffset = Offset;

  DPRINT("Offset of first track in last session %d\n", Offset);
  
  CdInfo->JolietLevel = 0;
  VdHeader = (PVD_HEADER)Buffer;
  Buffer[0] = 0;

  for (Sector = CDFS_PRIMARY_DESCRIPTOR_LOCATION; Sector < 100 && Buffer[0] != 255; Sector++)
     {
        /* Read the Primary Volume Descriptor (PVD) */
        Status = CdfsReadRawSectors(DeviceObject,
				    Sector + Offset,
				    1,
				    Buffer);
        if (!NT_SUCCESS(Status))
	{
	    ExFreePool(Buffer);
	    return(Status);
	}

	if (Sector == CDFS_PRIMARY_DESCRIPTOR_LOCATION)
	{  
            DPRINT("CD-identifier: [%.5s]\n", Buffer + 1);

            if (Buffer[0] != 1 || Buffer[1] != 'C' || Buffer[2] != 'D' ||
	        Buffer[3] != '0' || Buffer[4] != '0' || Buffer[5] != '1')
	    {
		ExFreePool(Buffer);
		return STATUS_UNRECOGNIZED_VOLUME;
	    }
	}

        switch (VdHeader->VdType)
	{
	   case 0:
	      DPRINT("BootVolumeDescriptor found!\n");
	      break;

	   case 1:
	      DPRINT("PrimaryVolumeDescriptor found!\n");
	      CdfsGetPVDData(Buffer, CdInfo);
	      break;

	   case 2:
	      DPRINT("SupplementaryVolumeDescriptor found!\n");
	      CdfsGetSVDData(Buffer, CdInfo);
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

     }

  ExFreePool(Buffer);

  return(STATUS_SUCCESS);
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
  CDINFO CdInfo;

  DPRINT("CdfsMountVolume() called\n");

  if (DeviceObject != CdfsGlobalData->DeviceObject)
    {
      Status = STATUS_INVALID_DEVICE_REQUEST;
      goto ByeBye;
    }

  Stack = IoGetCurrentIrpStackLocation(Irp);
  DeviceToMount = Stack->Parameters.MountVolume.DeviceObject;
  Vpb = Stack->Parameters.MountVolume.Vpb;

  Status = CdfsGetVolumeData(DeviceToMount, &CdInfo);
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

  Vpb->SerialNumber = CdInfo.SerialNumber;
  Vpb->VolumeLabelLength = CdInfo.VolumeLabelLength;
  RtlCopyMemory(Vpb->VolumeLabel, CdInfo.VolumeLabel, CdInfo.VolumeLabelLength * sizeof(WCHAR));
  RtlCopyMemory(&DeviceExt->CdInfo, &CdInfo, sizeof(CDINFO));
  
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
  DeviceExt->StreamFileObject->FsContext = Fcb;
  DeviceExt->StreamFileObject->FsContext2 = Ccb;
  DeviceExt->StreamFileObject->SectionObjectPointer = &Fcb->SectionObjectPointers;
  DeviceExt->StreamFileObject->PrivateCacheMap = NULL;
  DeviceExt->StreamFileObject->Vpb = DeviceExt->Vpb;
  Ccb->PtrFileObject = DeviceExt->StreamFileObject;
  Fcb->FileObject = DeviceExt->StreamFileObject;
  Fcb->DevExt = (PDEVICE_EXTENSION)DeviceExt->StorageDevice;

  Fcb->Flags = FCB_IS_VOLUME_STREAM;

  Fcb->RFCB.FileSize.QuadPart = (DeviceExt->CdInfo.VolumeSpaceSize + DeviceExt->CdInfo.VolumeOffset) * BLOCKSIZE;
  Fcb->RFCB.ValidDataLength = Fcb->RFCB.AllocationSize = Fcb->RFCB.FileSize;

  Fcb->Entry.ExtentLocationL = 0;
  Fcb->Entry.DataLengthL = (DeviceExt->CdInfo.VolumeSpaceSize + DeviceExt->CdInfo.VolumeOffset) * BLOCKSIZE;

  Status = CcRosInitializeFileCache(DeviceExt->StreamFileObject,
				    PAGE_SIZE);
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
  NTSTATUS Status;
  CDINFO CdInfo;

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

  CdInfo.SerialNumber = ~DeviceToVerify->Vpb->SerialNumber;

  Status = CdfsGetVolumeData(DeviceToVerify, &CdInfo);

  if (NT_SUCCESS(Status))
  {
     DPRINT("Current serial number %08lx  Vpb serial number %08lx\n",
	    CdInfo.SerialNumber, DeviceToVerify->Vpb->SerialNumber);

     if (CdInfo.SerialNumber != DeviceToVerify->Vpb->SerialNumber)
	 Status = STATUS_WRONG_VOLUME;
  }

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
