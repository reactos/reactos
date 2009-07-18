/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engdev.c
 * PURPOSE:         Device Support Routines
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

DWORD
APIENTRY
EngDeviceIoControl(
  IN HANDLE  hDevice,
  IN DWORD  dwIoControlCode,
  IN PVOID  lpInBuffer,
  IN DWORD  nInBufferSize,
  IN OUT PVOID  lpOutBuffer,
  IN DWORD  nOutBufferSize,
  OUT PDWORD  lpBytesReturned)
{
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    PDEVICE_OBJECT DeviceObject;

    DPRINT("EngDeviceIoControl() called\n");

    /* Check if handle is valid */
    if (!hDevice) return ERROR_INVALID_HANDLE;

    /* Initialize an event used for waiting */
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    /* Get DeviceObject pointer from handle */
    DeviceObject = (PDEVICE_OBJECT)hDevice;

    /* Build device IO control request */
    Irp = IoBuildDeviceIoControlRequest(dwIoControlCode,
                                        DeviceObject,
                                        lpInBuffer,
                                        nInBufferSize,
                                        lpOutBuffer,
                                        nOutBufferSize, FALSE, &Event, &Iosb);
    if (!Irp) return ERROR_INVALID_FUNCTION;

    /* Call the driver */
    Status = IoCallDriver(DeviceObject, Irp);

    /* Wait till it finishes if the driver asked so */
    if (Status == STATUS_PENDING)
    {
        (VOID)KeWaitForSingleObject(&Event, Executive, KernelMode, TRUE, 0);
        Status = Iosb.Status;
    }

    DPRINT("EngDeviceIoControl(): Returning %X/%X\n", Iosb.Status,
           Iosb.Information);

    /* Return information to the caller about the operation */
    *lpBytesReturned = Iosb.Information;

    /* Convert NT status code to W32 one */
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
    default:
        return Status;
    }
}

ULONG
APIENTRY
EngHangNotification(IN HDEV hDev,
                    IN PVOID Reserved)
{
    UNIMPLEMENTED;
	return EHN_ERROR;
}

BOOL
APIENTRY
EngQueryDeviceAttribute(
  IN HDEV  hdev,
  IN ENG_DEVICE_ATTRIBUTE  devAttr,
  IN VOID  *pvIn,
  IN ULONG  ulInSize,
  OUT VOID  *pvOut,
  OUT ULONG  ulOutSize)
{
    UNIMPLEMENTED;
	return FALSE;
}
