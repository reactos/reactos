/*
 *  ReactOS Floppy Driver
 *  Copyright (C) 2004, Vizzini (vizzini@plasmic.com)
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
 *
 * PROJECT:         ReactOS Floppy Driver
 * FILE:            ioctl.c
 * PURPOSE:         IOCTL Routines
 * PROGRAMMER:      Vizzini (vizzini@plasmic.com)
 * REVISIONS:
 *                  15-Feb-2004 vizzini - Created
 * NOTES:
 *     - All IOCTL documentation taken from the DDK
 *     - This driver tries to support all of the IOCTLs that the DDK floppy
 *       sample does
 *
 * TODO: Add support to GET_MEDIA_TYPES for non-1.44 disks
 * TODO: Implement format
 */

#include <ntddk.h>

#include "floppy.h"
#include "hardware.h"
#include "csqrtns.h"
#include "ioctl.h"


NTSTATUS NTAPI DeviceIoctl(PDEVICE_OBJECT DeviceObject, 
                           PIRP Irp)
/*
 * FUNCTION: Queue IOCTL IRPs
 * ARGUMENTS:
 *     DeviceObject: DeviceObject that is the target of the IRP
 *     Irp: IRP to process
 * RETURNS:
 *     STATUS_SUCCESS in all cases, so far
 * NOTES:
 *     - We can't just service these immediately because, even though some
 *       are able to run at DISPATCH, they'll get out of sync with other 
 *       read/write or ioctl irps.
 */
{
  ASSERT(DeviceObject);
  ASSERT(Irp);

  Irp->Tail.Overlay.DriverContext[0] = DeviceObject;
  IoCsqInsertIrp(&Csq, Irp, NULL);

  return STATUS_PENDING;
}


VOID NTAPI DeviceIoctlPassive(PDRIVE_INFO DriveInfo, 
			      PIRP Irp)
/*
 * FUNCTION: Handlees IOCTL requests at PASSIVE_LEVEL
 * ARGUMENTS:
 *     DriveInfo: Drive that the IOCTL is directed at
 *     Irp: IRP with the request in it
 */
{
  PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
  ULONG OutputLength = Stack->Parameters.DeviceIoControl.OutputBufferLength;
  PVOID OutputBuffer = Irp->AssociatedIrp.SystemBuffer;
  ULONG Code = Stack->Parameters.DeviceIoControl.IoControlCode;
  BOOLEAN DiskChanged;

  KdPrint(("floppy: DeviceIoctl called\n"));
  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;

  /* 
   * First the non-change-sensitive ioctls 
   */ 
  if(Code == IOCTL_DISK_GET_MEDIA_TYPES)
    {
      PDISK_GEOMETRY Geometry = OutputBuffer;
      KdPrint(("floppy: IOCTL_DISK_GET_MEDIA_TYPES Called\n"));

      if(OutputLength < sizeof(DISK_GEOMETRY))
        {
	  KdPrint(("floppy: IOCTL_DISK_GET_MEDIA_TYPES: insufficient buffer; returning STATUS_INVALID_PARAMETER\n"));
          Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
	  IoCompleteRequest(Irp, IO_NO_INCREMENT);
	  return;
        }

      /* 
       * for now, this driver only supports 3.5" HD media 
       */
      Geometry->MediaType = F3_1Pt44_512;
      Geometry->Cylinders.QuadPart = 80;
      Geometry->TracksPerCylinder = 2 * 18;
      Geometry->SectorsPerTrack = 18;
      Geometry->BytesPerSector = 512;

      Irp->IoStatus.Status = STATUS_SUCCESS;
      Irp->IoStatus.Information = sizeof(DISK_GEOMETRY);
      KdPrint(("floppy: Ioctl: completing with STATUS_SUCCESS\n"));
      IoCompleteRequest(Irp, IO_NO_INCREMENT);

      return;
    }

  /* 
   * Now, check to see if the volume needs to be verified.  If so,
   * return STATUS_VERIFY_REQUIRED.  
   *
   * NOTE:  This code, which is outside of the switch and if/else blocks, 
   * will implicity catch and correctly service IOCTL_DISK_CHECK_VERIFY.  
   * Therefore if we see one below in the switch, we can return STATUS_SUCCESS
   * immediately.
   */
  if(DriveInfo->DeviceObject->Flags & DO_VERIFY_VOLUME && !(DriveInfo->DeviceObject->Flags & SL_OVERRIDE_VERIFY_VOLUME)) 
    {
      KdPrint(("floppy: DeviceIoctl(): completing with STATUS_VERIFY_REQUIRED\n"));
      Irp->IoStatus.Status = STATUS_VERIFY_REQUIRED;
      Irp->IoStatus.Information = 0;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return;
    }

  /*
   * Check the change line, and if it's set, return
   */
  if(HwDiskChanged(DriveInfo, &DiskChanged) != STATUS_SUCCESS)
    {
      KdPrint(("floppy: DeviceIoctl(): unable to sense disk change; completing with STATUS_UNSUCCESSFUL\n"));
      Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
      Irp->IoStatus.Information = 0;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return;
    }

  if(DiskChanged)
    {
      KdPrint(("floppy: DeviceIoctl(): detected disk changed; signalling media change and completing\n"));
      SignalMediaChanged(DriveInfo->DeviceObject, Irp);

      /* 
       * Just guessing here - I have a choice of returning NO_MEDIA or VERIFY_REQUIRED.  If there's
       * really no disk in the drive, I'm thinking I can save time by just reporting that fact, rather
       * than forcing windows to ask me twice.  If this doesn't work, we'll need to split this up and
       * handle the CHECK_VERIFY IOCTL separately.
       */
      if(ResetChangeFlag(DriveInfo) == STATUS_NO_MEDIA_IN_DEVICE)
	Irp->IoStatus.Status = STATUS_NO_MEDIA_IN_DEVICE;

      IoCompleteRequest(Irp, IO_NO_INCREMENT);

      return;
    }

  switch(Code)
    {
    case IOCTL_DISK_IS_WRITABLE:
      {
        UCHAR Status;

        KdPrint(("floppy: IOCTL_DISK_IS_WRITABLE Called\n"));

        /* This IRP always has 0 information */
        Irp->IoStatus.Information = 0;

        if(HwSenseDriveStatus(DriveInfo) != STATUS_SUCCESS)
	  {
	    KdPrint(("floppy: IoctlDiskIsWritable(): unable to sense drive status\n"));
	    Irp->IoStatus.Status = STATUS_IO_DEVICE_ERROR;
	    break;
	  }

	/* Now, read the drive's status back */
	if(HwSenseDriveStatusResult(DriveInfo->ControllerInfo, &Status) != STATUS_SUCCESS)
	  {
	    KdPrint(("floppy: IoctlDiskIsWritable(): unable to read drive status result\n"));
	    Irp->IoStatus.Status = STATUS_IO_DEVICE_ERROR;
	    break;
	  }

        /* Check to see if the write flag is set. */
        if(Status & SR3_WRITE_PROTECT_STATUS_SIGNAL)
          {
            KdPrint(("floppy: IOCTL_DISK_IS_WRITABLE: disk is write protected\n"));
            Irp->IoStatus.Status = STATUS_MEDIA_WRITE_PROTECTED;
          }
        else
	    Irp->IoStatus.Status = STATUS_SUCCESS;
      }
      break;

    case IOCTL_DISK_CHECK_VERIFY:
      KdPrint(("floppy: IOCTL_DISK_CHECK_VERIFY called\n"));
      *((PULONG)OutputBuffer) = DriveInfo->DiskChangeCount;
      Irp->IoStatus.Status = STATUS_SUCCESS;
      Irp->IoStatus.Information = sizeof(ULONG);
      break;

    case IOCTL_DISK_GET_DRIVE_GEOMETRY:
      {
        KdPrint(("floppy: IOCTL_DISK_GET_DRIVE_GEOMETRY Called\n"));
        if(OutputLength < sizeof(DISK_GEOMETRY))
          {
            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
            break;
          }

	/* This still works right even if DriveInfo->DiskGeometry->MediaType = Unknown */
	memcpy(OutputBuffer, &DriveInfo->DiskGeometry, sizeof(DISK_GEOMETRY));
        Irp->IoStatus.Information = sizeof(DISK_GEOMETRY);
        break;
      }

    case IOCTL_DISK_FORMAT_TRACKS:
    case IOCTL_DISK_FORMAT_TRACKS_EX:
      KdPrint(("floppy: Format called; not supported yet\n"));
      break;

    case IOCTL_DISK_GET_PARTITION_INFO:
      KdPrint(("floppy: IOCTL_DISK_GET_PARTITION_INFO Called; not supported\n"));
      Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
      Irp->IoStatus.Information = 0;
      break;

    default:
      KdPrint(("floppy: UNKNOWN IOCTL CODE: 0x%x\n", Code));
      Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
      Irp->IoStatus.Information = 0;
      break;
    }

  KdPrint(("floppy: ioctl: completing with status 0x%x\n", Irp->IoStatus.Status));
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return;
}

