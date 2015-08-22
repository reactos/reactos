/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Hackssign application & driver
 * FILE:            cmdutils/hackssign/driver.c
 * PURPOSE:         Driver: Assign drive letter to shared folders for VMware/VBox VMs
 * PROGRAMMERS:     Pierre Schweitzer <pierre@reactos.org>
 */

#include <wdm.h>
#include <ntifs.h>
#include <wchar.h>
#define NDEBUG
#include <debug.h>

#include "ioctl.h"

typedef struct _HS_VCB
{
    SHARE_ACCESS shareAccess;
} HS_VCB, *PHS_VCB;

ERESOURCE globalLock;
PDEVICE_OBJECT gDevObj;

NTSTATUS
NTAPI
hsDispatch(PDEVICE_OBJECT DeviceObject,
           PIRP Irp)
{
    PHS_VCB vcb;
    NTSTATUS status;
    WCHAR dosBuffer[7];
    PFILE_OBJECT fileObject;
    PIO_STACK_LOCATION stack;
    PASSIGN_INPUT inputBuffer;
    UNICODE_STRING target, source;

    FsRtlEnterFileSystem();
    ExAcquireResourceExclusiveLite(&globalLock, TRUE);

    stack = IoGetCurrentIrpStackLocation(Irp);
    fileObject = stack->FileObject;
    switch (stack->MajorFunction)
    {
        case IRP_MJ_CREATE:
            if (fileObject->FileName.Length != 0)
            {
                status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }

            status = IoCheckShareAccess(stack->Parameters.Create.SecurityContext->DesiredAccess,
                                        stack->Parameters.Create.ShareAccess,
                                        fileObject, &((PHS_VCB)DeviceObject->DeviceExtension)->shareAccess, TRUE);
            if (NT_SUCCESS(status))
            {
                DPRINT1("Device opened\n");

                Irp->IoStatus.Information = FILE_OPENED;
                fileObject->FsContext = DeviceObject->DeviceExtension;
                status = STATUS_SUCCESS;
            }
            break;

        case IRP_MJ_FILE_SYSTEM_CONTROL:
            if (stack->Parameters.FileSystemControl.FsControlCode == FSCTL_HACKSSIGN_ASSIGN)
            {
                if (stack->Parameters.FileSystemControl.InputBufferLength <= sizeof(ASSIGN_INPUT) ||
                    Irp->AssociatedIrp.SystemBuffer == NULL)
                {
                    status = STATUS_INVALID_DEVICE_REQUEST;
                    break;
                }

                inputBuffer = Irp->AssociatedIrp.SystemBuffer;

                swprintf(dosBuffer, L"\\??\\%c:", inputBuffer->letter);
                RtlInitUnicodeString(&source, dosBuffer);
                target.Buffer = (PWSTR)((ULONG_PTR)inputBuffer + inputBuffer->offset);
                target.Length = inputBuffer->len;
                target.MaximumLength = target.Length;

                DPRINT1("Will link %wZ to %wZ\n", &source, &target);

                status = IoCreateSymbolicLink(&source, &target);
                break;
            }
            else if (stack->Parameters.FileSystemControl.FsControlCode == FSCTL_HACKSSIGN_DELETE)
            {
                if (stack->Parameters.FileSystemControl.InputBufferLength < sizeof(WCHAR) ||
                    Irp->AssociatedIrp.SystemBuffer == NULL)
                {
                    status = STATUS_INVALID_DEVICE_REQUEST;
                    break;
                }

                inputBuffer = Irp->AssociatedIrp.SystemBuffer;

                swprintf(dosBuffer, L"\\??\\%c:", inputBuffer->letter);
                RtlInitUnicodeString(&source, dosBuffer);

                DPRINT1("Will unlink %wZ\n", &source);

                status = IoDeleteSymbolicLink(&source);
                break;
            }

            status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case IRP_MJ_CLEANUP:
            vcb = fileObject->FsContext;
            if (vcb == NULL)
            {
                status = STATUS_INVALID_HANDLE;
                break;
            }

            DPRINT1("Device cleaned up\n");
            IoRemoveShareAccess(fileObject, &vcb->shareAccess);
            status = STATUS_SUCCESS;
            break;

        case IRP_MJ_CLOSE:
            vcb = fileObject->FsContext;
            if (vcb == NULL)
            {
                status = STATUS_INVALID_HANDLE;
                break;
            }

            DPRINT1("Device closed\n");
            fileObject->FsContext = NULL;
            status = STATUS_SUCCESS;
            break;

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    ExReleaseResourceLite(&globalLock);
    FsRtlExitFileSystem();

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

VOID
NTAPI
hsUnload(PDRIVER_OBJECT DriverObject)
{
    IoDeleteDevice(gDevObj);
    ExDeleteResourceLite(&globalLock);
}

NTSTATUS
NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath)
{
    NTSTATUS status;
    PDEVICE_OBJECT devObj;
    UNICODE_STRING devName, uDevName;

    DPRINT1("Starting hackssign driver\n");

    RtlInitUnicodeString(&devName, L"\\Device\\hackssign");
    status = IoCreateDevice(DriverObject, sizeof(HS_VCB), &devName, FILE_DEVICE_FILE_SYSTEM, 0, FALSE, &devObj);
    if (!NT_SUCCESS(status))
    {
        DPRINT1("IoCreateDevice failed\n");
        return status;
    }

    RtlInitUnicodeString(&uDevName,  L"\\??\\hackssign");
    status = IoCreateSymbolicLink(&uDevName, &devName);
    if (!NT_SUCCESS(status))
    {
        DPRINT1("IoCreateSymbolicLink failed\n");
        IoDeleteDevice(devObj);
        return status;
    }

    gDevObj = devObj;
    ExInitializeResourceLite(&globalLock);
    RtlZeroMemory(devObj->DeviceExtension, sizeof(HS_VCB));

    DriverObject->DriverUnload = hsUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = hsDispatch;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = hsDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = hsDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = hsDispatch;

    return STATUS_SUCCESS;
}
