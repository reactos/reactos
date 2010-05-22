/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Device Functions
 * FILE:              subsys/win32k/eng/device.c
 * PROGRAMER:         Jason Filby
 *                    Timo Kreuzer
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

static
NTSTATUS
EngpFileIoRequest(
    PFILE_OBJECT pFileObject,
    ULONG   ulMajorFunction,
    LPVOID  lpBuffer,
    DWORD   nBufferSize,
    ULONGLONG  ullStartOffset,
    OUT LPDWORD lpInformation)
{
    PDEVICE_OBJECT pDeviceObject;
    KEVENT Event;
    PIRP pIrp;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;
    LARGE_INTEGER liStartOffset;

    /* Get corresponding device object */
    pDeviceObject = IoGetRelatedDeviceObject(pFileObject);
    if (!pDeviceObject)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Initialize an event */
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    /* Build IPR */
    liStartOffset.QuadPart = ullStartOffset;
    pIrp = IoBuildSynchronousFsdRequest(ulMajorFunction,
                                        pDeviceObject,
                                        lpBuffer,
                                        nBufferSize,
                                        &liStartOffset,
                                        &Event,
                                        &Iosb);
    if (!pIrp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Call the driver */
    Status = IoCallDriver(pDeviceObject, pIrp);

    /* Wait if neccessary */
    if (STATUS_PENDING == Status)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, TRUE, 0);
        Status = Iosb.Status;
    }

    /* Return information to the caller about the operation. */
    *lpInformation = Iosb.Information;

    /* Return NTSTATUS */
    return Status;
}

VOID
APIENTRY
EngFileWrite(
    IN PFILE_OBJECT pFileObject,
    IN PVOID lpBuffer,
    IN SIZE_T nLength,
    IN PSIZE_T lpBytesWritten)
{
    EngpFileIoRequest(pFileObject,
                      IRP_MJ_WRITE,
                      lpBuffer,
                      nLength,
                      0,
                      lpBytesWritten);
}

NTSTATUS
APIENTRY
EngFileIoControl(
    IN PFILE_OBJECT pFileObject,
    IN DWORD dwIoControlCode,
    IN PVOID lpInBuffer,
    IN SIZE_T nInBufferSize,
    OUT PVOID lpOutBuffer,
    IN SIZE_T nOutBufferSize,
    OUT LPDWORD lpInformation)
{
    PDEVICE_OBJECT pDeviceObject;
    KEVENT Event;
    PIRP pIrp;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    /* Get corresponding device object */
    pDeviceObject = IoGetRelatedDeviceObject(pFileObject);
    if (!pDeviceObject)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Initialize an event */
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    /* Build IO control IPR */
    pIrp = IoBuildDeviceIoControlRequest(dwIoControlCode,
                                         pDeviceObject,
                                         lpInBuffer,
                                         nInBufferSize,
                                         lpOutBuffer,
                                         nOutBufferSize,
                                         FALSE,
                                         &Event,
                                         &Iosb);
    if (!pIrp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Call the driver */
    Status = IoCallDriver(pDeviceObject, pIrp);

    /* Wait if neccessary */
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, TRUE, 0);
        Status = Iosb.Status;
    }

    /* Return information to the caller about the operation. */
    *lpInformation = Iosb.Information;

    /* This function returns NTSTATUS */
    return Status;
}

/*
 * @implemented
 */
DWORD APIENTRY
EngDeviceIoControl(
    HANDLE  hDevice,
    DWORD   dwIoControlCode,
    LPVOID  lpInBuffer,
    DWORD   nInBufferSize,
    LPVOID  lpOutBuffer,
    DWORD   nOutBufferSize,
    DWORD *lpBytesReturned)
{
    PIRP Irp;
    NTSTATUS Status;
    KEVENT Event;
    IO_STATUS_BLOCK Iosb;
    PDEVICE_OBJECT DeviceObject;

    DPRINT("EngDeviceIoControl() called\n");

    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    DeviceObject = (PDEVICE_OBJECT) hDevice;

    Irp = IoBuildDeviceIoControlRequest(dwIoControlCode,
                                        DeviceObject,
                                        lpInBuffer,
                                        nInBufferSize,
                                        lpOutBuffer,
                                        nOutBufferSize, FALSE, &Event, &Iosb);
    if (!Irp) return ERROR_NOT_ENOUGH_MEMORY;

    Status = IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        (VOID)KeWaitForSingleObject(&Event, Executive, KernelMode, TRUE, 0);
        Status = Iosb.Status;
    }

    DPRINT("EngDeviceIoControl(): Returning %X/%X\n", Iosb.Status,
           Iosb.Information);

    /* Return information to the caller about the operation. */
    *lpBytesReturned = Iosb.Information;

    /* Convert NT status values to win32 error codes. */
    switch (Status)
    {
        case STATUS_INSUFFICIENT_RESOURCES:
            return ERROR_NOT_ENOUGH_MEMORY;

        case STATUS_BUFFER_OVERFLOW:
            return ERROR_MORE_DATA;

        case STATUS_NOT_IMPLEMENTED:
            return ERROR_INVALID_FUNCTION;

        case STATUS_INVALID_PARAMETER:
            return ERROR_INVALID_PARAMETER;

        case STATUS_BUFFER_TOO_SMALL:
            return ERROR_INSUFFICIENT_BUFFER;

        case STATUS_DEVICE_DOES_NOT_EXIST:
            return ERROR_DEV_NOT_EXIST;

        case STATUS_PENDING:
            return ERROR_IO_PENDING;
    }

    return Status;
}

/* EOF */
