/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Device Functions
 * FILE:              subsys/win32k/eng/device.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 3/7/1999: Created
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

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
