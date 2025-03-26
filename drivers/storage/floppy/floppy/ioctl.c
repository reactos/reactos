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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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

#include "precomp.h"

#include <debug.h>

NTSTATUS NTAPI
DeviceIoctl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
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


VOID NTAPI
DeviceIoctlPassive(PDRIVE_INFO DriveInfo, PIRP Irp)
/*
 * FUNCTION: Handles IOCTL requests at PASSIVE_LEVEL
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
    PMOUNTDEV_NAME Name;
    PMOUNTDEV_UNIQUE_ID UniqueId;

    TRACE_(FLOPPY, "DeviceIoctl called\n");
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    /*
     * First the non-change-sensitive ioctls
     */
    if(Code == IOCTL_DISK_GET_MEDIA_TYPES)
    {
        PDISK_GEOMETRY Geometry = OutputBuffer;
        INFO_(FLOPPY, "IOCTL_DISK_GET_MEDIA_TYPES Called\n");

        if(OutputLength < sizeof(DISK_GEOMETRY))
        {
            INFO_(FLOPPY, "IOCTL_DISK_GET_MEDIA_TYPES: insufficient buffer; returning STATUS_INVALID_PARAMETER\n");
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
        INFO_(FLOPPY, "Ioctl: completing with STATUS_SUCCESS\n");
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
    if(DriveInfo->DeviceObject->Flags & DO_VERIFY_VOLUME && !(Stack->Flags & SL_OVERRIDE_VERIFY_VOLUME))
    {
        INFO_(FLOPPY, "DeviceIoctl(): completing with STATUS_VERIFY_REQUIRED\n");
        Irp->IoStatus.Status = STATUS_VERIFY_REQUIRED;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return;
    }

    /*
     * Start the drive to see if the disk has changed
     */
    StartMotor(DriveInfo);

    /*
     * Check the change line, and if it's set, return
     */
    if(HwDiskChanged(DriveInfo, &DiskChanged) != STATUS_SUCCESS)
    {
        WARN_(FLOPPY, "DeviceIoctl(): unable to sense disk change; completing with STATUS_UNSUCCESSFUL\n");
        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        StopMotor(DriveInfo->ControllerInfo);
        return;
    }

    if(DiskChanged)
    {
        INFO_(FLOPPY, "DeviceIoctl(): detected disk changed; signalling media change and completing\n");

        /* The following call sets IoStatus.Status and IoStatus.Information */
        SignalMediaChanged(DriveInfo->DeviceObject, Irp);
        ResetChangeFlag(DriveInfo);

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        StopMotor(DriveInfo->ControllerInfo);
        return;
    }

    switch(Code)
    {
    case IOCTL_DISK_IS_WRITABLE:
    {
        UCHAR Status;

        INFO_(FLOPPY, "IOCTL_DISK_IS_WRITABLE Called\n");

        /* This IRP always has 0 information */
        Irp->IoStatus.Information = 0;

        if(HwSenseDriveStatus(DriveInfo) != STATUS_SUCCESS)
        {
            WARN_(FLOPPY, "IoctlDiskIsWritable(): unable to sense drive status\n");
            Irp->IoStatus.Status = STATUS_IO_DEVICE_ERROR;
            break;
        }

        /* Now, read the drive's status back */
        if(HwSenseDriveStatusResult(DriveInfo->ControllerInfo, &Status) != STATUS_SUCCESS)
        {
            WARN_(FLOPPY, "IoctlDiskIsWritable(): unable to read drive status result\n");
            Irp->IoStatus.Status = STATUS_IO_DEVICE_ERROR;
            break;
        }

        /* Check to see if the write flag is set. */
        if(Status & SR3_WRITE_PROTECT_STATUS_SIGNAL)
        {
            INFO_(FLOPPY, "IOCTL_DISK_IS_WRITABLE: disk is write protected\n");
            Irp->IoStatus.Status = STATUS_MEDIA_WRITE_PROTECTED;
        }
        else
            Irp->IoStatus.Status = STATUS_SUCCESS;
    }
    break;

    case IOCTL_DISK_CHECK_VERIFY:
        INFO_(FLOPPY, "IOCTL_DISK_CHECK_VERIFY called\n");
        if (OutputLength != 0)
        {
            if (OutputLength < sizeof(ULONG))
            {
                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = 0;
            }
            else
            {
                *((PULONG)OutputBuffer) = DriveInfo->DiskChangeCount;
                Irp->IoStatus.Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = sizeof(ULONG);
            }
        }
        else
        {
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;
        }
        break;

    case IOCTL_DISK_GET_DRIVE_GEOMETRY:
    {
        INFO_(FLOPPY, "IOCTL_DISK_GET_DRIVE_GEOMETRY Called\n");
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
        ERR_(FLOPPY, "Format called; not supported yet\n");
        Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
        Irp->IoStatus.Information = 0;
        break;

    case IOCTL_DISK_GET_PARTITION_INFO:
        INFO_(FLOPPY, "IOCTL_DISK_GET_PARTITION_INFO Called; not supported by a floppy driver\n");
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        break;

    case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:
        if(OutputLength < sizeof(MOUNTDEV_UNIQUE_ID))
        {
            Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = 0;
            break;
        }

        UniqueId = Irp->AssociatedIrp.SystemBuffer;
        UniqueId->UniqueIdLength = (USHORT)wcslen(&DriveInfo->DeviceNameBuffer[0]) * sizeof(WCHAR);

        if(OutputLength < FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId) + UniqueId->UniqueIdLength)
        {
            Irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
            Irp->IoStatus.Information = sizeof(MOUNTDEV_UNIQUE_ID);
            break;
        }

        RtlCopyMemory(UniqueId->UniqueId, &DriveInfo->DeviceNameBuffer[0],
                      UniqueId->UniqueIdLength);

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId) + UniqueId->UniqueIdLength;
        break;

    case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
        if(OutputLength < sizeof(MOUNTDEV_NAME))
        {
            Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = 0;
            break;
        }

        Name = Irp->AssociatedIrp.SystemBuffer;
        Name->NameLength = (USHORT)wcslen(&DriveInfo->DeviceNameBuffer[0]) * sizeof(WCHAR);

        if(OutputLength < FIELD_OFFSET(MOUNTDEV_NAME, Name) + Name->NameLength)
        {
            Irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
            Irp->IoStatus.Information = sizeof(MOUNTDEV_NAME);
            break;
        }

        RtlCopyMemory(Name->Name, &DriveInfo->DeviceNameBuffer[0],
                      Name->NameLength);

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = FIELD_OFFSET(MOUNTDEV_NAME, Name) + Name->NameLength;
        break;

    default:
        ERR_(FLOPPY, "UNKNOWN IOCTL CODE: 0x%x\n", Code);
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        Irp->IoStatus.Information = 0;
        break;
    }

    INFO_(FLOPPY, "ioctl: completing with status 0x%x\n", Irp->IoStatus.Status);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    StopMotor(DriveInfo->ControllerInfo);
    return;
}
