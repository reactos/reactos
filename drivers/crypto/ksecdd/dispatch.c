/*
 * PROJECT:         ReactOS Drivers
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         Kernel Security Support Provider Interface Driver
 *
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include "ksecdd.h"
#include <ksecioctl.h>

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ******************************************************************/

static
NTSTATUS
KsecQueryFileInformation(
    PVOID InfoBuffer,
    FILE_INFORMATION_CLASS FileInformationClass,
    PSIZE_T BufferLength)
{
    PFILE_STANDARD_INFORMATION StandardInformation;

    /* Only FileStandardInformation is supported */
    if (FileInformationClass != FileStandardInformation)
    {
        return STATUS_INVALID_INFO_CLASS;
    }

    /* Validate buffer size */
    if (*BufferLength >= sizeof(FILE_STANDARD_INFORMATION))
    {
        *BufferLength = sizeof(FILE_STANDARD_INFORMATION);
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    /* Fill the structure */
    StandardInformation = (PFILE_STANDARD_INFORMATION)InfoBuffer;
    StandardInformation->AllocationSize.QuadPart = 0;
    StandardInformation->EndOfFile.QuadPart = 0;
    StandardInformation->NumberOfLinks = 1;
    StandardInformation->DeletePending = FALSE;
    StandardInformation->Directory = FALSE;
    *BufferLength = sizeof(FILE_STANDARD_INFORMATION);

    return STATUS_SUCCESS;
}

static
NTSTATUS
KsecQueryVolumeInformation(
    PVOID InfoBuffer,
    FS_INFORMATION_CLASS FsInformationClass,
    PSIZE_T BufferLength)
{
    PFILE_FS_DEVICE_INFORMATION DeviceInformation;

    /* Only FileFsDeviceInformation is supported */
    if (FsInformationClass != FileFsDeviceInformation)
    {
        return STATUS_INVALID_INFO_CLASS;
    }

    /* Validate buffer size */
    if (*BufferLength < sizeof(FILE_FS_DEVICE_INFORMATION))
    {
        *BufferLength = sizeof(FILE_FS_DEVICE_INFORMATION);
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    /* Fill the structure */
    DeviceInformation = (PFILE_FS_DEVICE_INFORMATION)InfoBuffer;
    DeviceInformation->DeviceType = FILE_DEVICE_NULL;
    DeviceInformation->Characteristics = 0;
    *BufferLength = sizeof(FILE_FS_DEVICE_INFORMATION);

    return STATUS_SUCCESS;
}

static
NTSTATUS
KsecDeviceControl(
    ULONG IoControlCode,
    PVOID Buffer,
    SIZE_T InputLength,
    PSIZE_T OutputLength)
{
    NTSTATUS Status;

    Status = STATUS_SUCCESS;

    /* Check ioctl code */
    switch (IoControlCode)
    {
        case IOCTL_KSEC_REGISTER_LSA_PROCESS:

            Status = STATUS_SUCCESS;
            break;

        case IOCTL_KSEC_RANDOM_FILL_BUFFER:

            Status = KsecGenRandom(Buffer, *OutputLength);
            break;

        default:
            DPRINT1("Unhandled control code 0x%lx\n", IoControlCode);
            __debugbreak();
            return STATUS_INVALID_PARAMETER;
    }

    return Status;
}

NTSTATUS
NTAPI
KsecDdDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation;
    ULONG_PTR Information;
    NTSTATUS Status;
    PVOID Buffer;
    SIZE_T InputLength, OutputLength;
    FILE_INFORMATION_CLASS FileInfoClass;
    FS_INFORMATION_CLASS FsInfoClass;
    ULONG IoControlCode;

    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    switch (IoStackLocation->MajorFunction)
    {
        case IRP_MJ_CREATE:
        case IRP_MJ_CLOSE:

            /* Just return success */
            Status = STATUS_SUCCESS;
            Information = 0;
            break;

        case IRP_MJ_READ:

            /* There is nothing to read */
            Status = STATUS_END_OF_FILE;
            Information = 0;
            break;

        case IRP_MJ_WRITE:

            /* Pretend to have written everything */
            Status = STATUS_SUCCESS;
            Information = IoStackLocation->Parameters.Write.Length;
            break;

        case IRP_MJ_QUERY_INFORMATION:

            /* Extract the parameters */
            Buffer = Irp->AssociatedIrp.SystemBuffer;
            OutputLength = IoStackLocation->Parameters.QueryFile.Length;
            FileInfoClass = IoStackLocation->Parameters.QueryFile.FileInformationClass;

            /* Call the internal function */
            Status = KsecQueryFileInformation(Buffer,
                                              FileInfoClass,
                                              &OutputLength);
            Information = OutputLength;
            break;

        case IRP_MJ_QUERY_VOLUME_INFORMATION:

            /* Extract the parameters */
            Buffer = Irp->AssociatedIrp.SystemBuffer;
            OutputLength = IoStackLocation->Parameters.QueryVolume.Length;
            FsInfoClass = IoStackLocation->Parameters.QueryVolume.FsInformationClass;

            /* Call the internal function */
            Status = KsecQueryVolumeInformation(Buffer,
                                                FsInfoClass,
                                                &OutputLength);
            Information = OutputLength;
            break;

        case IRP_MJ_DEVICE_CONTROL:

            /* Extract the parameters */
            Buffer = Irp->AssociatedIrp.SystemBuffer;
            InputLength = IoStackLocation->Parameters.DeviceIoControl.InputBufferLength;
            OutputLength = IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
            IoControlCode = IoStackLocation->Parameters.DeviceIoControl.IoControlCode;

            /* Call the internal function */
            Status = KsecDeviceControl(IoControlCode,
                                       Buffer,
                                       InputLength,
                                       &OutputLength);
            Information = OutputLength;
            break;

        default:
            DPRINT1("Unhandled major function %lu!\n",
                    IoStackLocation->MajorFunction);
            ASSERT(FALSE);
    }

    /* Return the information */
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Information;

    /* Complete the request */
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}
