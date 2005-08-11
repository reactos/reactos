/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/null/null.c
 * PURPOSE:          NULL device driver
 * PROGRAMMER:       David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *              13/08/1998: Created
 *              29/04/2002: Fixed bugs, added zero-stream device
 *              28/06/2004: Compile against the DDK, use PSEH where necessary
 */

/* INCLUDES */
#include <ntddk.h>

#include <pseh/pseh.h>

#include "null.h"

/* OBJECTS */
static const NULL_EXTENSION nxNull = NullBitBucket;
static const NULL_EXTENSION nxZero = NullZeroStream;

/* FUNCTIONS */
NTSTATUS STDCALL
NullDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION piosStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS nErrCode;

    nErrCode = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    switch(piosStack->MajorFunction)
    {
        /* opening and closing handles to the device */
        case IRP_MJ_CREATE:
        case IRP_MJ_CLOSE:
            switch(NULL_DEVICE_TYPE(DeviceObject))
            {
                case NullBitBucket:
                case NullZeroStream:
                    break;

                default:
                    ASSERT(FALSE);
            }

            break;

        /* write data */
        case IRP_MJ_WRITE:
            {
                switch(NULL_DEVICE_TYPE(DeviceObject))
                {
                    case NullBitBucket:
                        Irp->IoStatus.Information = piosStack->Parameters.Write.Length;
                        break;

                    case NullZeroStream:
                        nErrCode = STATUS_INVALID_DEVICE_REQUEST;
                        break;

                    default:
                        ASSERT(FALSE);
                }

                break;
            }

        /* read data */
        case IRP_MJ_READ:
            {
                switch(NULL_DEVICE_TYPE(DeviceObject))
                {
                    case NullBitBucket:
                        nErrCode = STATUS_END_OF_FILE;
                        break;

                    case NullZeroStream:
                        _SEH_TRY
                        {
                            RtlZeroMemory(Irp->AssociatedIrp.SystemBuffer, piosStack->Parameters.Read.Length);
                            Irp->IoStatus.Information = piosStack->Parameters.Read.Length;
                        }
                        _SEH_HANDLE
                        {
                            nErrCode = _SEH_GetExceptionCode();
                        }
                        _SEH_END;

                        break;

                    default:
                        ASSERT(FALSE);

                }

                break;
            }

        case IRP_MJ_QUERY_VOLUME_INFORMATION:
            switch(piosStack->Parameters.QueryVolume.FsInformationClass)
            {
                case FileFsDeviceInformation:
                    {
                        ULONG BufferLength = piosStack->Parameters.QueryVolume.Length;
                        PFILE_FS_DEVICE_INFORMATION FsDeviceInfo = (PFILE_FS_DEVICE_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

                        if (BufferLength >= sizeof(FILE_FS_DEVICE_INFORMATION))
                        {
                            FsDeviceInfo->DeviceType = FILE_DEVICE_NULL;
                            FsDeviceInfo->Characteristics = 0; /* FIXME: fix this !! */
                            Irp->IoStatus.Information = sizeof(FILE_FS_DEVICE_INFORMATION);
                            nErrCode = STATUS_SUCCESS;
                        }
                        else
                        {   
                            Irp->IoStatus.Information  = 0;
                            nErrCode = STATUS_BUFFER_OVERFLOW;
                        }
                    }
                    break;

                default:
                    Irp->IoStatus.Information = 0;
                    nErrCode = STATUS_NOT_IMPLEMENTED;
            }
            break;

        default:
            Irp->IoStatus.Information = 0;
            nErrCode = STATUS_NOT_IMPLEMENTED;

    }

    Irp->IoStatus.Status = nErrCode;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return (nErrCode);
}

VOID STDCALL
NullUnload(PDRIVER_OBJECT DriverObject)
{
}

/* TODO: \Device\Zero should be memory-mappable */
NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    PDEVICE_OBJECT pdoNullDevice;
    PDEVICE_OBJECT pdoZeroDevice;
    UNICODE_STRING wstrNullDeviceName = RTL_CONSTANT_STRING(L"\\Device\\Null");
    UNICODE_STRING wstrZeroDeviceName = RTL_CONSTANT_STRING(L"\\Device\\Zero");
    NTSTATUS nErrCode;

    /* register driver routines */
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = NullDispatch;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = NullDispatch;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = NullDispatch;
    DriverObject->MajorFunction[IRP_MJ_READ] = NullDispatch;
    DriverObject->DriverUnload = NullUnload;

    /* create null device */
    nErrCode = IoCreateDevice(DriverObject,
                              sizeof(NULL_EXTENSION),
                              &wstrNullDeviceName,
                              FILE_DEVICE_NULL,
                              0,
                              FALSE,
                              &pdoNullDevice);

    /* failure */
    if(!NT_SUCCESS(nErrCode))
    {
        return (nErrCode);
    }

    pdoNullDevice->DeviceExtension = (PVOID)&nxNull;

    /* create zero device */
    nErrCode = IoCreateDevice(DriverObject,
                              sizeof(NULL_EXTENSION),   
                              &wstrZeroDeviceName,
                              FILE_DEVICE_NULL,
                              FILE_READ_ONLY_DEVICE, /* zero device is read-only */
                              FALSE,
                              &pdoZeroDevice);

    /* failure */
    if(!NT_SUCCESS(nErrCode))
    {
        IoDeleteDevice(pdoNullDevice);
        return (nErrCode);
    }

    pdoZeroDevice->DeviceExtension = (PVOID)&nxZero;
    pdoZeroDevice->Flags |= DO_BUFFERED_IO;

    return (nErrCode);
}

/* EOF */
