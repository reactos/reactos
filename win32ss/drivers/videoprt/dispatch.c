/*
 * VideoPort driver
 *
 * Copyright (C) 2002, 2003, 2004 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "videoprt.h"

#include <ndk/inbvfuncs.h>
#include <ndk/psfuncs.h>

#define NDEBUG
#include <debug.h>

/* GLOBAL VARIABLES ***********************************************************/

PVIDEO_PORT_DEVICE_EXTENSION ResetDisplayParametersDeviceExtension = NULL;
PVIDEO_WIN32K_CALLOUT Win32kCallout;

/* PRIVATE FUNCTIONS **********************************************************/

/*
 * Reset display to blue screen
 */
BOOLEAN
NTAPI
IntVideoPortResetDisplayParameters(ULONG Columns, ULONG Rows)
{
    PVIDEO_PORT_DRIVER_EXTENSION DriverExtension;

    if (ResetDisplayParametersDeviceExtension == NULL)
        return FALSE;

    DriverExtension = ResetDisplayParametersDeviceExtension->DriverExtension;

    if (DriverExtension->InitializationData.HwResetHw != NULL)
    {
        if (DriverExtension->InitializationData.HwResetHw(
                    &ResetDisplayParametersDeviceExtension->MiniPortDeviceExtension,
                    Columns, Rows))
        {
            ResetDisplayParametersDeviceExtension = NULL;
            return TRUE;
        }
    }

    ResetDisplayParametersDeviceExtension = NULL;
    return FALSE;
}

NTSTATUS
NTAPI
IntVideoPortAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    PVIDEO_PORT_DRIVER_EXTENSION DriverExtension;
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status;

    /* Get the initialization data we saved in VideoPortInitialize. */
    DriverExtension = IoGetDriverObjectExtension(DriverObject, DriverObject);

    /* Create adapter device object. */
    Status = IntVideoPortCreateAdapterDeviceObject(DriverObject,
                                                   DriverExtension,
                                                   PhysicalDeviceObject,
                                                   &DeviceObject);
    if (NT_SUCCESS(Status))
        VideoPortDeviceNumber++;

    return Status;
}

/*
 * IntVideoPortDispatchOpen
 *
 * Answer requests for Open calls.
 *
 * Run Level
 *    PASSIVE_LEVEL
 */
NTSTATUS
NTAPI
IntVideoPortDispatchOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
    PVIDEO_PORT_DRIVER_EXTENSION DriverExtension;

    TRACE_(VIDEOPRT, "IntVideoPortDispatchOpen\n");

    if (CsrssInitialized == FALSE)
    {
        /*
         * We know the first open call will be from the CSRSS process
         * to let us know its handle.
         */

        INFO_(VIDEOPRT, "Referencing CSRSS\n");
        Csrss = (PKPROCESS)PsGetCurrentProcess();
        INFO_(VIDEOPRT, "Csrss %p\n", Csrss);

        IntInitializeVideoAddressSpace();

        CsrssInitialized = TRUE;
    }

    DeviceExtension = (PVIDEO_PORT_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    DriverExtension = DeviceExtension->DriverExtension;

    if (DriverExtension->InitializationData.HwInitialize(&DeviceExtension->MiniPortDeviceExtension))
    {
        Irp->IoStatus.Status = STATUS_SUCCESS;
        InterlockedIncrement((PLONG)&DeviceExtension->DeviceOpened);
    }
    else
    {
        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    }

    Irp->IoStatus.Information = FILE_OPENED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

/*
 * IntVideoPortDispatchClose
 *
 * Answer requests for Close calls.
 *
 * Run Level
 *    PASSIVE_LEVEL
 */
NTSTATUS
NTAPI
IntVideoPortDispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

    TRACE_(VIDEOPRT, "IntVideoPortDispatchClose\n");

    DeviceExtension = (PVIDEO_PORT_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    if ((DeviceExtension->DeviceOpened >= 1) &&
        (InterlockedDecrement((PLONG)&DeviceExtension->DeviceOpened) == 0))
    {
        ResetDisplayParametersDeviceExtension = NULL;
        InbvNotifyDisplayOwnershipLost(NULL);
        ResetDisplayParametersDeviceExtension = DeviceExtension;
        IntVideoPortResetDisplayParameters(80, 50);
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

PSTR
IoctlName(ULONG Ioctl)
{
    switch(Ioctl)
    {
        case IOCTL_VIDEO_ENABLE_VDM:
            return "IOCTL_VIDEO_ENABLE_VDM"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x00, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_DISABLE_VDM:
            return "IOCTL_VIDEO_DISABLE_VDM"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x01, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_REGISTER_VDM:
            return "IOCTL_VIDEO_REGISTER_VDM"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x02, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_SET_OUTPUT_DEVICE_POWER_STATE:
            return "IOCTL_VIDEO_SET_OUTPUT_DEVICE_POWER_STATE"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x03, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_GET_OUTPUT_DEVICE_POWER_STATE:
            return "IOCTL_VIDEO_GET_OUTPUT_DEVICE_POWER_STATE"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x04, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_MONITOR_DEVICE:
            return "IOCTL_VIDEO_MONITOR_DEVICE"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x05, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_ENUM_MONITOR_PDO:
            return "IOCTL_VIDEO_ENUM_MONITOR_PDO"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x06, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_INIT_WIN32K_CALLBACKS:
            return "IOCTL_VIDEO_INIT_WIN32K_CALLBACKS"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x07, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_HANDLE_VIDEOPARAMETERS:
            return "IOCTL_VIDEO_HANDLE_VIDEOPARAMETERS"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x08, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_IS_VGA_DEVICE:
            return "IOCTL_VIDEO_IS_VGA_DEVICE"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x09, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_USE_DEVICE_IN_SESSION:
            return "IOCTL_VIDEO_USE_DEVICE_IN_SESSION"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x0a, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_PREPARE_FOR_EARECOVERY:
            return "IOCTL_VIDEO_PREPARE_FOR_EARECOVERY"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x0b, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_SAVE_HARDWARE_STATE:
            return "IOCTL_VIDEO_SAVE_HARDWARE_STATE"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x80, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_RESTORE_HARDWARE_STATE:
            return "IOCTL_VIDEO_RESTORE_HARDWARE_STATE"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x81, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_QUERY_AVAIL_MODES:
            return "IOCTL_VIDEO_QUERY_AVAIL_MODES"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x100, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:
            return "IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x101, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_QUERY_CURRENT_MODE:
            return "IOCTL_VIDEO_QUERY_CURRENT_MODE"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x102, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_SET_CURRENT_MODE:
            return "IOCTL_VIDEO_SET_CURRENT_MODE"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x103, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_RESET_DEVICE:
            return "IOCTL_VIDEO_RESET_DEVICE"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x104, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_LOAD_AND_SET_FONT:
            return "IOCTL_VIDEO_LOAD_AND_SET_FONT"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x105, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_SET_PALETTE_REGISTERS:
            return "IOCTL_VIDEO_SET_PALETTE_REGISTERS"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x106, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_SET_COLOR_REGISTERS:
            return "IOCTL_VIDEO_SET_COLOR_REGISTERS"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x107, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_ENABLE_CURSOR:
            return "IOCTL_VIDEO_ENABLE_CURSOR"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x108, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_DISABLE_CURSOR:
            return "IOCTL_VIDEO_DISABLE_CURSOR"; // CTL_CODE (FILE_DEVICE_VIDEO, 0x109, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_SET_CURSOR_ATTR:
            return "IOCTL_VIDEO_SET_CURSOR_ATTR"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x10a, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_QUERY_CURSOR_ATTR:
            return "IOCTL_VIDEO_QUERY_CURSOR_ATTR"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x10b, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_SET_CURSOR_POSITION:
            return "IOCTL_VIDEO_SET_CURSOR_POSITION"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x10c, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_QUERY_CURSOR_POSITION:
            return "IOCTL_VIDEO_QUERY_CURSOR_POSITION"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x10d, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_ENABLE_POINTER:
            return "IOCTL_VIDEO_ENABLE_POINTER"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x10e, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_DISABLE_POINTER:
            return "IOCTL_VIDEO_DISABLE_POINTER"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x10f, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_SET_POINTER_ATTR:
            return "IOCTL_VIDEO_SET_POINTER_ATTR"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x110, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_QUERY_POINTER_ATTR:
            return "IOCTL_VIDEO_QUERY_POINTER_ATTR"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x111, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_SET_POINTER_POSITION:
            return "IOCTL_VIDEO_SET_POINTER_POSITION"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x112, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_QUERY_POINTER_POSITION:
            return "IOCTL_VIDEO_QUERY_POINTER_POSITION"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x113, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_QUERY_POINTER_CAPABILITIES:
            return "IOCTL_VIDEO_QUERY_POINTER_CAPABILITIES"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x114, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_GET_BANK_SELECT_CODE:
            return "IOCTL_VIDEO_GET_BANK_SELECT_CODE"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x115, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_MAP_VIDEO_MEMORY:
            return "IOCTL_VIDEO_MAP_VIDEO_MEMORY"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x116, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_UNMAP_VIDEO_MEMORY:
            return "IOCTL_VIDEO_UNMAP_VIDEO_MEMORY"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x117, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES:
            return "IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x118, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES:
            return "IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x119, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_QUERY_COLOR_CAPABILITIES:
            return "IOCTL_VIDEO_QUERY_COLOR_CAPABILITIES"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x11a, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_SET_POWER_MANAGEMENT:
            return "IOCTL_VIDEO_SET_POWER_MANAGEMENT"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x11b, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_GET_POWER_MANAGEMENT:
            return "IOCTL_VIDEO_GET_POWER_MANAGEMENT"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x11c, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_SHARE_VIDEO_MEMORY:
            return "IOCTL_VIDEO_SHARE_VIDEO_MEMORY"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x11d, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY:
            return "IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x11e, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_SET_COLOR_LUT_DATA:
            return "IOCTL_VIDEO_SET_COLOR_LUT_DATA"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x11f, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_GET_CHILD_STATE:
            return "IOCTL_VIDEO_GET_CHILD_STATE"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x120, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_VALIDATE_CHILD_STATE_CONFIGURATION:
            return "IOCTL_VIDEO_VALIDATE_CHILD_STATE_CONFIGURATION"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x121, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_SET_CHILD_STATE_CONFIGURATION:
            return "IOCTL_VIDEO_SET_CHILD_STATE_CONFIGURATION"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x122, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_SWITCH_DUALVIEW:
            return "IOCTL_VIDEO_SWITCH_DUALVIEW"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x123, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_SET_BANK_POSITION:
            return "IOCTL_VIDEO_SET_BANK_POSITION"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x124, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_QUERY_SUPPORTED_BRIGHTNESS:
            return "IOCTL_VIDEO_QUERY_SUPPORTED_BRIGHTNESS"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x125, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_QUERY_DISPLAY_BRIGHTNESS:
            return "IOCTL_VIDEO_QUERY_DISPLAY_BRIGHTNESS"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x126, METHOD_BUFFERED, FILE_ANY_ACCESS)
        case IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS:
            return "IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS"; // CTL_CODE(FILE_DEVICE_VIDEO, 0x127, METHOD_BUFFERED, FILE_ANY_ACCESS)
    }

    return "<unknown ioctl code";
}

static
NTSTATUS
VideoPortUseDeviceInSession(
    _Inout_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PVIDEO_DEVICE_SESSION_STATUS SessionState,
    _In_ ULONG BufferLength,
    _Out_ PULONG_PTR Information)
{
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

    /* Check buffer size */
    *Information = sizeof(VIDEO_DEVICE_SESSION_STATUS);
    if (BufferLength < sizeof(VIDEO_DEVICE_SESSION_STATUS))
    {
        ERR_(VIDEOPRT, "Buffer too small for VIDEO_DEVICE_SESSION_STATUS: %lx\n",
             BufferLength);
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Get the device extension */
    DeviceExtension = DeviceObject->DeviceExtension;

    /* Shall we enable the session? */
    if (SessionState->bEnable)
    {
        /* Check if we have no session yet */
        if (DeviceExtension->SessionId == -1)
        {
            /* Use this session and return success */
            DeviceExtension->SessionId = PsGetCurrentProcessSessionId();
            SessionState->bSuccess = TRUE;
        }
        else
        {
            ERR_(VIDEOPRT, "Requested to set session, but session is already set to: 0x%lx\n",
                 DeviceExtension->SessionId);
            SessionState->bSuccess = FALSE;
        }
    }
    else
    {
        /* Check if we belong to the current session */
        if (DeviceExtension->SessionId == PsGetCurrentProcessSessionId())
        {
            /* Reset the session and return success */
            DeviceExtension->SessionId = -1;
            SessionState->bSuccess = TRUE;
        }
        else
        {
            ERR_(VIDEOPRT, "Requested to reset session, but session is not set\n");
            SessionState->bSuccess = FALSE;
        }
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
VideoPortInitWin32kCallbacks(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PVIDEO_WIN32K_CALLBACKS Win32kCallbacks,
    _In_ ULONG BufferLength,
    _Out_ PULONG_PTR Information)
{
    *Information = sizeof(VIDEO_WIN32K_CALLBACKS);
    if (BufferLength < sizeof(VIDEO_WIN32K_CALLBACKS))
    {
        ERR_(VIDEOPRT, "Buffer too small for VIDEO_WIN32K_CALLBACKS: %lx\n",
             BufferLength);
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Save the callout function globally */
    Win32kCallout = Win32kCallbacks->Callout;

    /* Return reasonable values to win32k */
    Win32kCallbacks->bACPI = FALSE;
    Win32kCallbacks->pPhysDeviceObject = DeviceObject;
    Win32kCallbacks->DualviewFlags = 0;

    return STATUS_SUCCESS;
}

static
NTSTATUS
VideoPortForwardDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IrpStack;
    PVIDEO_PORT_DRIVER_EXTENSION DriverExtension;
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
    VIDEO_REQUEST_PACKET vrp;

    TRACE_(VIDEOPRT, "VideoPortForwardDeviceControl\n");

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = DeviceObject->DeviceExtension;
    DriverExtension = DeviceExtension->DriverExtension;

    /* Translate the IRP to a VRP */
    vrp.StatusBlock = (PSTATUS_BLOCK)&Irp->IoStatus;
    vrp.IoControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;

    INFO_(VIDEOPRT, "- IoControlCode: %x\n", vrp.IoControlCode);

    /* We're assuming METHOD_BUFFERED */
    vrp.InputBuffer = Irp->AssociatedIrp.SystemBuffer;
    vrp.InputBufferLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    vrp.OutputBuffer = Irp->AssociatedIrp.SystemBuffer;
    vrp.OutputBufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    /* Call the Miniport Driver with the VRP */
    DriverExtension->InitializationData.HwStartIO(&DeviceExtension->MiniPortDeviceExtension,
                                                  &vrp);

    INFO_(VIDEOPRT, "- Returned status: %x\n", Irp->IoStatus.Status);

    /* Map from win32 error codes to NT status values. */
    switch (Irp->IoStatus.Status)
    {
        case NO_ERROR:
            return STATUS_SUCCESS;
        case ERROR_NOT_ENOUGH_MEMORY:
            return STATUS_INSUFFICIENT_RESOURCES;
        case ERROR_MORE_DATA:
            return STATUS_BUFFER_OVERFLOW;
        case ERROR_INVALID_FUNCTION:
            return STATUS_NOT_IMPLEMENTED;
        case ERROR_INVALID_PARAMETER:
            return STATUS_INVALID_PARAMETER;
        case ERROR_INSUFFICIENT_BUFFER:
            return STATUS_BUFFER_TOO_SMALL;
        case ERROR_DEV_NOT_EXIST:
            return STATUS_DEVICE_DOES_NOT_EXIST;
        case ERROR_IO_PENDING:
            return STATUS_PENDING;
        default:
            return STATUS_UNSUCCESSFUL;
    }
}

/*
 * IntVideoPortDispatchDeviceControl
 *
 * Answer requests for device control calls.
 *
 * Run Level
 *    PASSIVE_LEVEL
 */
NTSTATUS
NTAPI
IntVideoPortDispatchDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IrpStack;
    NTSTATUS Status;
    ULONG IoControlCode;

    TRACE_(VIDEOPRT, "IntVideoPortDispatchDeviceControl\n");

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    IoControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;

    INFO_(VIDEOPRT, "- IoControlCode: %x: %s\n", IoControlCode, IoctlName(IoControlCode));

    switch(IoControlCode)
    {
        case IOCTL_VIDEO_INIT_WIN32K_CALLBACKS:
            INFO_(VIDEOPRT, "- IOCTL_VIDEO_INIT_WIN32K_CALLBACKS\n");
            Status = VideoPortInitWin32kCallbacks(DeviceObject,
                                                  Irp->AssociatedIrp.SystemBuffer,
                                                  IrpStack->Parameters.DeviceIoControl.InputBufferLength,
                                                  &Irp->IoStatus.Information);
            break;

        case IOCTL_VIDEO_USE_DEVICE_IN_SESSION:
            INFO_(VIDEOPRT, "- IOCTL_VIDEO_USE_DEVICE_IN_SESSION\n");
            Status = VideoPortUseDeviceInSession(DeviceObject,
                                                 Irp->AssociatedIrp.SystemBuffer,
                                                 IrpStack->Parameters.DeviceIoControl.InputBufferLength,
                                                 &Irp->IoStatus.Information);
            break;

        default:
            /* Forward to the Miniport Driver */
            Status = VideoPortForwardDeviceControl(DeviceObject, Irp);
            break;
    }

    INFO_(VIDEOPRT, "- Returned status: %x\n", Irp->IoStatus.Status);

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

/*
 * IntVideoPortWrite
 *
 * This is a bit of a hack. We want to take ownership of the display as late
 * as possible, just before the switch to graphics mode. Win32k knows when
 * this happens, we don't. So we need Win32k to inform us. This could be done
 * using an IOCTL, but there's no way of knowing which IOCTL codes are unused
 * in the communication between GDI driver and miniport driver. So we use
 * IRP_MJ_WRITE as the signal that win32k is ready to switch to graphics mode,
 * since we know for certain that there is no read/write activity going on
 * between GDI and miniport drivers.
 * We don't actually need the data that is passed, we just trigger on the fact
 * that an IRP_MJ_WRITE was sent.
 *
 * Run Level
 *    PASSIVE_LEVEL
 */
NTSTATUS
NTAPI
IntVideoPortDispatchWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION piosStack = IoGetCurrentIrpStackLocation(Irp);
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS nErrCode;

    DeviceExtension = (PVIDEO_PORT_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /*
     * Storing the device extension pointer in a static variable is an
     * ugly hack. Unfortunately, we need it in IntVideoPortResetDisplayParameters
     * and InbvNotifyDisplayOwnershipLost doesn't allow us to pass a userdata
     * parameter. On the bright side, the DISPLAY device is opened
     * exclusively, so there can be only one device extension active at
     * any point in time.
     *
     * FIXME: We should process all opened display devices in
     * IntVideoPortResetDisplayParameters.
     */
    ResetDisplayParametersDeviceExtension = DeviceExtension;
    InbvNotifyDisplayOwnershipLost(IntVideoPortResetDisplayParameters);

    nErrCode = STATUS_SUCCESS;
    Irp->IoStatus.Information = piosStack->Parameters.Write.Length;
    Irp->IoStatus.Status = nErrCode;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return nErrCode;
}

NTSTATUS
NTAPI
IntVideoPortPnPStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    PDRIVER_OBJECT DriverObject;
    PVIDEO_PORT_DRIVER_EXTENSION DriverExtension;
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
    PCM_RESOURCE_LIST AllocatedResources;

    /* Get the initialization data we saved in VideoPortInitialize.*/
    DriverObject = DeviceObject->DriverObject;
    DriverExtension = IoGetDriverObjectExtension(DriverObject, DriverObject);
    DeviceExtension = (PVIDEO_PORT_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* Store some resources in the DeviceExtension. */
    AllocatedResources = Stack->Parameters.StartDevice.AllocatedResources;
    if (AllocatedResources != NULL)
    {
        CM_FULL_RESOURCE_DESCRIPTOR *FullList;
        CM_PARTIAL_RESOURCE_DESCRIPTOR *Descriptor;
        ULONG ResourceCount;
        ULONG ResourceListSize;

        /* Save the resource list */
        ResourceCount = AllocatedResources->List[0].PartialResourceList.Count;
        ResourceListSize =
            FIELD_OFFSET(CM_RESOURCE_LIST, List[0].PartialResourceList.
                         PartialDescriptors[ResourceCount]);
        DeviceExtension->AllocatedResources = ExAllocatePool(PagedPool, ResourceListSize);
        if (DeviceExtension->AllocatedResources == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(DeviceExtension->AllocatedResources,
                      AllocatedResources,
                      ResourceListSize);

        /* Get the interrupt level/vector - needed by HwFindAdapter sometimes */
        for (FullList = AllocatedResources->List;
             FullList < AllocatedResources->List + AllocatedResources->Count;
             FullList++)
        {
            INFO_(VIDEOPRT, "InterfaceType %u BusNumber List %u Device BusNumber %u Version %u Revision %u\n",
                  FullList->InterfaceType, FullList->BusNumber, DeviceExtension->SystemIoBusNumber, FullList->PartialResourceList.Version, FullList->PartialResourceList.Revision);

            /* FIXME: Is this ASSERT ok for resources from the PNP manager? */
            ASSERT(FullList->InterfaceType == PCIBus);
            ASSERT(FullList->BusNumber == DeviceExtension->SystemIoBusNumber);
            ASSERT(1 == FullList->PartialResourceList.Version);
            ASSERT(1 == FullList->PartialResourceList.Revision);
            for (Descriptor = FullList->PartialResourceList.PartialDescriptors;
                 Descriptor < FullList->PartialResourceList.PartialDescriptors + FullList->PartialResourceList.Count;
                 Descriptor++)
            {
                if (Descriptor->Type == CmResourceTypeInterrupt)
                {
                    DeviceExtension->InterruptLevel = Descriptor->u.Interrupt.Level;
                    DeviceExtension->InterruptVector = Descriptor->u.Interrupt.Vector;
                    if (Descriptor->ShareDisposition == CmResourceShareShared)
                        DeviceExtension->InterruptShared = TRUE;
                    else
                        DeviceExtension->InterruptShared = FALSE;
                }
            }
        }
    }

    INFO_(VIDEOPRT, "Interrupt level: 0x%x Interrupt Vector: 0x%x\n",
          DeviceExtension->InterruptLevel,
          DeviceExtension->InterruptVector);

    /* Create adapter device object. */
    return IntVideoPortFindAdapter(DriverObject,
                                   DriverExtension,
                                   DeviceObject);
}


NTSTATUS
NTAPI
IntVideoPortForwardIrpAndWaitCompletionRoutine(
    PDEVICE_OBJECT Fdo,
    PIRP Irp,
    PVOID Context)
{
    PKEVENT Event = Context;

    if (Irp->PendingReturned)
        KeSetEvent(Event, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
IntVideoPortQueryBusRelations(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PDEVICE_RELATIONS DeviceRelations;
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    PVIDEO_PORT_CHILD_EXTENSION ChildExtension;
    ULONG i;
    PLIST_ENTRY CurrentEntry;

    /* Count the children */
    i = 0;
    CurrentEntry = DeviceExtension->ChildDeviceList.Flink;
    while (CurrentEntry != &DeviceExtension->ChildDeviceList)
    {
        i++;
        CurrentEntry = CurrentEntry->Flink;
    }

    if (i == 0)
        return Irp->IoStatus.Status;

    DeviceRelations = ExAllocatePool(PagedPool,
                                     sizeof(DEVICE_RELATIONS) + ((i - 1) * sizeof(PVOID)));
    if (!DeviceRelations) return STATUS_NO_MEMORY;

    DeviceRelations->Count = i;

    /* Add the children */
    i = 0;
    CurrentEntry = DeviceExtension->ChildDeviceList.Flink;
    while (CurrentEntry != &DeviceExtension->ChildDeviceList)
    {
        ChildExtension = CONTAINING_RECORD(CurrentEntry, VIDEO_PORT_CHILD_EXTENSION, ListEntry);

        ObReferenceObject(ChildExtension->PhysicalDeviceObject);
        DeviceRelations->Objects[i] = ChildExtension->PhysicalDeviceObject;

        i++;
        CurrentEntry = CurrentEntry->Flink;
    }

    INFO_(VIDEOPRT, "Reported %d PDOs\n", i);
    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IntVideoPortForwardIrpAndWait(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    KEVENT Event;
    NTSTATUS Status;
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension =
        (PVIDEO_PORT_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp,
                           IntVideoPortForwardIrpAndWaitCompletionRoutine,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    Status = IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = Irp->IoStatus.Status;
    }

    return Status;
}

NTSTATUS
NTAPI
IntVideoPortDispatchFdoPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (IrpSp->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            Status = IntVideoPortForwardIrpAndWait(DeviceObject, Irp);
            if (NT_SUCCESS(Status) && NT_SUCCESS(Irp->IoStatus.Status))
                Status = IntVideoPortPnPStartDevice(DeviceObject, Irp);
            Irp->IoStatus.Status = Status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;

        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            Status = IntVideoPortForwardIrpAndWait(DeviceObject, Irp);
            if (NT_SUCCESS(Status) && NT_SUCCESS(Irp->IoStatus.Status))
                Status = IntVideoPortFilterResourceRequirements(DeviceObject, Irp);
            Irp->IoStatus.Status = Status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            if (IrpSp->Parameters.QueryDeviceRelations.Type != BusRelations)
            {
                IoSkipCurrentIrpStackLocation(Irp);
                Status = IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
            }
            else
            {
                Status = IntVideoPortQueryBusRelations(DeviceObject, Irp);
                Irp->IoStatus.Status = Status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }
            break;

        case IRP_MN_REMOVE_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL:

        case IRP_MN_STOP_DEVICE:
            Status = IntVideoPortForwardIrpAndWait(DeviceObject, Irp);
            if (NT_SUCCESS(Status) && NT_SUCCESS(Irp->IoStatus.Status))
                Status = STATUS_SUCCESS;
            Irp->IoStatus.Status = Status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;

        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
            Status = STATUS_SUCCESS;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;

        default:
            Status = Irp->IoStatus.Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
    }

    return Status;
}

NTSTATUS
NTAPI
IntVideoPortDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PVIDEO_PORT_COMMON_EXTENSION CommonExtension = DeviceObject->DeviceExtension;

    if (CommonExtension->Fdo)
        return IntVideoPortDispatchFdoPnp(DeviceObject, Irp);
    else
        return IntVideoPortDispatchPdoPnp(DeviceObject, Irp);
}

NTSTATUS
NTAPI
IntVideoPortDispatchCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

    DeviceExtension = DeviceObject->DeviceExtension;
    RtlFreeUnicodeString(&DeviceExtension->RegistryPath);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IntVideoPortDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IntVideoPortDispatchSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
IntVideoPortUnload(PDRIVER_OBJECT DriverObject)
{
}
