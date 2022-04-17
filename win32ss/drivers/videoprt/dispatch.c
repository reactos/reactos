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
#include <ndk/obfuncs.h>
#include <ndk/psfuncs.h>

#define NDEBUG
#include <debug.h>

/* GLOBAL VARIABLES ***********************************************************/

static PVIDEO_WIN32K_CALLOUT Win32kCallout = NULL;
static HANDLE InbvThreadHandle = NULL;
static BOOLEAN InbvMonitoring = FALSE;

/* PRIVATE FUNCTIONS **********************************************************/

static VOID
VideoPortWin32kCallout(
    _In_ PVIDEO_WIN32K_CALLBACKS_PARAMS CallbackParams)
{
    if (!Win32kCallout)
        return;

    /* Perform the call in the context of CSRSS */
    if (!CsrProcess)
        return;

    KeAttachProcess(CsrProcess);
    Win32kCallout(CallbackParams);
    KeDetachProcess();
}

/*
 * Reinitialize the display to base VGA mode.
 *
 * Returns TRUE if it completely resets the adapter to the given character mode.
 * Returns FALSE otherwise, indicating that the HAL should perform the VGA mode
 * reset itself after HwVidResetHw() returns control.
 *
 * This callback has been registered with InbvNotifyDisplayOwnershipLost()
 * and is called by InbvAcquireDisplayOwnership(), typically when the bugcheck
 * code regains display access. Therefore this routine can be called at any
 * IRQL, and in particular at IRQL = HIGH_LEVEL. This routine must also reside
 * completely in non-paged pool, and cannot perform the following actions:
 * Allocate memory, access pageable memory, use any synchronization mechanisms
 * or call any routine that must execute at IRQL = DISPATCH_LEVEL or below.
 */
static BOOLEAN
NTAPI
IntVideoPortResetDisplayParametersEx(
    _In_ ULONG Columns,
    _In_ ULONG Rows,
    _In_ BOOLEAN CalledByInbv)
{
    BOOLEAN Success = TRUE; // Suppose we don't need to perform a full reset.
    KIRQL OldIrql;
    PLIST_ENTRY PrevEntry, Entry;
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
    PVIDEO_PORT_DRIVER_EXTENSION DriverExtension;

    /* Check if we are at dispatch level or lower, and acquire the lock */
    OldIrql = KeGetCurrentIrql();
    if (OldIrql <= DISPATCH_LEVEL)
    {
        /* Loop until the lock is free, then raise IRQL to dispatch level */
        while (!KeTestSpinLock(&HwResetAdaptersLock));
        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    }
    KeAcquireSpinLockAtDpcLevel(&HwResetAdaptersLock);

    /* Bail out early if we don't have any resettable adapter */
    if (IsListEmpty(&HwResetAdaptersList))
    {
        Success = FALSE; // No adapter found: request HAL to perform a full reset.
        goto Quit;
    }

    /*
     * If we have been unexpectedly called via a callback from
     * InbvAcquireDisplayOwnership(), start monitoring INBV.
     */
    if (CalledByInbv)
        InbvMonitoring = TRUE;

    for (PrevEntry = &HwResetAdaptersList, Entry = PrevEntry->Flink;
         Entry != &HwResetAdaptersList;
         PrevEntry = Entry, Entry = Entry->Flink)
    {
        /*
         * Check whether the entry address is properly aligned,
         * the device and driver extensions must be readable and
         * the device extension properly back-linked to the last entry.
         */
// #define IS_ALIGNED(addr, align) (((ULONG64)(addr) & (align - 1)) == 0)
        if (((ULONG_PTR)Entry & (sizeof(ULONG_PTR) - 1)) != 0)
        {
            Success = FALSE; // We failed: request HAL to perform a full reset.
            goto Quit;
        }

        DeviceExtension = CONTAINING_RECORD(Entry,
                                            VIDEO_PORT_DEVICE_EXTENSION,
                                            HwResetListEntry);
        /*
         * As this function can be called as part of the INBV initialization
         * by the bugcheck code, avoid any problems and protect all accesses
         * within SEH.
         */
        _SEH2_TRY
        {
            DriverExtension = DeviceExtension->DriverExtension;
            ASSERT(DriverExtension);

            if (DeviceExtension->HwResetListEntry.Blink != PrevEntry)
            {
                Success = FALSE; // We failed: request HAL to perform a full reset.
                _SEH2_YIELD(goto Quit);
            }

            if ((DeviceExtension->DeviceOpened >= 1) &&
                (DriverExtension->InitializationData.HwResetHw != NULL))
            {
                Success &= DriverExtension->InitializationData.HwResetHw(
                                &DeviceExtension->MiniPortDeviceExtension,
                                Columns, Rows);
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
        }
        _SEH2_END;
    }

Quit:
    /* Release the lock and restore the old IRQL if we were at dispatch level or lower */
    KeReleaseSpinLockFromDpcLevel(&HwResetAdaptersLock);
    if (OldIrql <= DISPATCH_LEVEL)
        KeLowerIrql(OldIrql);

    return Success;
}

/* This callback is registered with InbvNotifyDisplayOwnershipLost() */
static BOOLEAN
NTAPI
IntVideoPortResetDisplayParameters(ULONG Columns, ULONG Rows)
{
    /* Call the extended function, specifying we were called by INBV */
    return IntVideoPortResetDisplayParametersEx(Columns, Rows, TRUE);
}

/*
 * (Adapted for ReactOS/Win2k3 from an original comment
 *  by Gé van Geldorp, June 2003, r4937)
 *
 * DISPLAY OWNERSHIP
 *
 * So, who owns the physical display and is allowed to write to it?
 *
 * In NT 5.x (Win2k/Win2k3), upon boot INBV/BootVid owns the display, unless
 * /NOGUIBOOT has been specified in the boot command line. Later in the boot
 * sequence, WIN32K.SYS opens the DISPLAY device. This open call ends up in
 * VIDEOPRT.SYS. This component takes ownership of the display by calling
 * InbvNotifyDisplayOwnershipLost() -- effectively telling INBV to release
 * ownership of the display it previously had. From that moment on, the display
 * is owned by that component and can be switched to graphics mode. The display
 * is not supposed to return to text mode, except in case of a bugcheck.
 * The bugcheck code calls InbvAcquireDisplayOwnership() so as to make INBV
 * re-take display ownership, and calls back the function previously registered
 * by VIDEOPRT.SYS with InbvNotifyDisplayOwnershipLost(). After the bugcheck,
 * execution is halted. So, under NT, the only possible sequence of display
 * modes is text mode -> graphics mode -> text mode (the latter hopefully
 * happening very infrequently).
 *
 * In ReactOS things are a little bit different. We want to have a functional
 * interactive text mode. We should be able to switch back and forth from
 * text mode to graphics mode when a GUI app is started and then finished.
 * Also, when the system bugchecks in graphics mode we want to switch back to
 * text mode and show the bugcheck information. Last but not least, when using
 * KDBG in /DEBUGPORT=SCREEN mode, breaking into the debugger would trigger a
 * switch to text mode, and the user would expect that by continuing execution
 * a switch back to graphics mode is done.
 */
static VOID
NTAPI
InbvMonitorThread(
    _In_ PVOID Context)
{
    VIDEO_WIN32K_CALLBACKS_PARAMS CallbackParams;
    LARGE_INTEGER Delay;
    USHORT i;

    KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);

    while (TRUE)
    {
        /*
         * During one second, check the INBV status each 100 milliseconds,
         * then revert to 1 second delay.
         */
        i = 10;
        Delay.QuadPart = (LONGLONG)-100*1000*10; // 100 millisecond delay
        while (!InbvMonitoring)
        {
            KeDelayExecutionThread(KernelMode, FALSE, &Delay);

            if ((i > 0) && (--i == 0))
                Delay.QuadPart = (LONGLONG)-1*1000*1000*10; // 1 second delay
        }

        /*
         * Loop while the display is owned by INBV. We cannot do anything else
         * than polling since INBV does not offer a proper notification system.
         *
         * During one second, check the INBV status each 100 milliseconds,
         * then revert to 1 second delay.
         */
        i = 10;
        Delay.QuadPart = (LONGLONG)-100*1000*10; // 100 millisecond delay
        while (InbvCheckDisplayOwnership())
        {
            KeDelayExecutionThread(KernelMode, FALSE, &Delay);

            if ((i > 0) && (--i == 0))
                Delay.QuadPart = (LONGLONG)-1*1000*1000*10; // 1 second delay
        }

        /* Reset the monitoring */
        InbvMonitoring = FALSE;

        /*
         * Somebody released INBV display ownership, usually by invoking
         * InbvNotifyDisplayOwnershipLost(). However the caller of this
         * function certainly specified a different callback than ours.
         * As we are going to be the only owner of the active display,
         * we need to re-register our own display reset callback.
         */
        InbvNotifyDisplayOwnershipLost(IntVideoPortResetDisplayParameters);

        /* Tell Win32k to reset the display */
        CallbackParams.CalloutType = VideoFindAdapterCallout;
        // CallbackParams.PhysDisp = NULL;
        CallbackParams.Param = (ULONG_PTR)TRUE; // TRUE: Re-enable display; FALSE: Disable display.
        VideoPortWin32kCallout(&CallbackParams);
    }

    // FIXME: See IntVideoPortInbvCleanup().
    // PsTerminateSystemThread(STATUS_SUCCESS);
}

static NTSTATUS
IntVideoPortInbvInitialize(VOID)
{
    /* Create the INBV monitoring thread if needed */
    if (!InbvThreadHandle)
    {
        NTSTATUS Status;
        OBJECT_ATTRIBUTES ObjectAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(NULL, OBJ_KERNEL_HANDLE);

        Status = PsCreateSystemThread(&InbvThreadHandle,
                                      0,
                                      &ObjectAttributes,
                                      NULL,
                                      NULL,
                                      InbvMonitorThread,
                                      NULL);
        if (!NT_SUCCESS(Status))
            InbvThreadHandle = NULL;
    }

    /* Re-register the display reset callback with INBV */
    InbvNotifyDisplayOwnershipLost(IntVideoPortResetDisplayParameters);

    return STATUS_SUCCESS;
}

static NTSTATUS
IntVideoPortInbvCleanup(
    IN PDEVICE_OBJECT DeviceObject)
{
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
    // HANDLE ThreadHandle;

    DeviceExtension = (PVIDEO_PORT_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    if ((DeviceExtension->DeviceOpened >= 1) &&
        (InterlockedDecrement((PLONG)&DeviceExtension->DeviceOpened) == 0))
    {
        // RemoveEntryList(&DeviceExtension->HwResetListEntry);
        InbvNotifyDisplayOwnershipLost(NULL);
        IntVideoPortResetDisplayParametersEx(80, 50, FALSE);
        // or InbvAcquireDisplayOwnership(); ?
    }

#if 0
    // TODO: Find the best way to communicate the request.
    /* Signal the INBV monitoring thread and wait for it to terminate */
    ThreadHandle = InterlockedExchangePointer((PVOID*)&InbvThreadHandle, NULL);
    if (ThreadHandle)
    {
        KeWaitForSingleObject(&ThreadHandle, Executive, KernelMode, FALSE, NULL);
        /* Close its handle */
        ObCloseHandle(ThreadHandle, KernelMode);
    }
#endif

    return STATUS_SUCCESS;
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
                                                   DriverExtension->InitializationData.StartingDeviceNumber,
                                                   0,
                                                   &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        ERR_(VIDEOPRT, "IntVideoPortCreateAdapterDeviceObject() failed with status 0x%lx\n", Status);
    }
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
    NTSTATUS Status;
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
    PVIDEO_PORT_DRIVER_EXTENSION DriverExtension;

    TRACE_(VIDEOPRT, "IntVideoPortDispatchOpen\n");

    if (!CsrProcess)
    {
        /*
         * We know the first open call will be from the CSRSS process
         * to let us know its handle.
         */
        INFO_(VIDEOPRT, "Referencing CSRSS\n");
        CsrProcess = (PKPROCESS)PsGetCurrentProcess();
        ObReferenceObject(CsrProcess);
        INFO_(VIDEOPRT, "CsrProcess 0x%p\n", CsrProcess);

        Status = IntInitializeVideoAddressSpace();
        if (!NT_SUCCESS(Status))
        {
            ERR_(VIDEOPRT, "IntInitializeVideoAddressSpace() failed: 0x%lx\n", Status);
            ObDereferenceObject(CsrProcess);
            CsrProcess = NULL;
            return Status;
        }
    }

    DeviceExtension = (PVIDEO_PORT_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    DriverExtension = DeviceExtension->DriverExtension;

    // FIXME: (Re-)initialize INBV only if DeviceObject doesn't belong to a mirror driver.
    IntVideoPortInbvInitialize();

    if (DriverExtension->InitializationData.HwInitialize(&DeviceExtension->MiniPortDeviceExtension))
    {
        Status = STATUS_SUCCESS;
        InterlockedIncrement((PLONG)&DeviceExtension->DeviceOpened);

        /* Query children, now that device is opened */
        VideoPortEnumerateChildren(DeviceExtension->MiniPortDeviceExtension, NULL);
    }
    else
    {
        Status = STATUS_UNSUCCESSFUL;
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = FILE_OPENED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
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
    TRACE_(VIDEOPRT, "IntVideoPortDispatchClose\n");

    IntVideoPortInbvCleanup(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

PSTR
IoctlName(ULONG Ioctl)
{
    switch (Ioctl)
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

    return "<unknown ioctl code>";
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

    /* Return reasonable values to Win32k */
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

    switch (IrpStack->MajorFunction)
    {
        case IRP_MJ_DEVICE_CONTROL:
            /* This is the main part of this function and is handled below */
            break;

        case IRP_MJ_SHUTDOWN:
        {
            /* Dereference CSRSS */
            PKPROCESS OldCsrProcess;
            OldCsrProcess = InterlockedExchangePointer((PVOID*)&CsrProcess, NULL);
            if (OldCsrProcess)
                ObDereferenceObject(OldCsrProcess);

            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
        }

        default:
            ERR_(VIDEOPRT, "- Unknown MajorFunction 0x%x\n", IrpStack->MajorFunction);
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
    }

    IoControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;

    INFO_(VIDEOPRT, "- IoControlCode: 0x%x: %s\n", IoControlCode, IoctlName(IoControlCode));

    switch (IoControlCode)
    {
        case IOCTL_VIDEO_ENABLE_VDM:
        case IOCTL_VIDEO_DISABLE_VDM:
        case IOCTL_VIDEO_REGISTER_VDM:
            WARN_(VIDEOPRT, "- IOCTL_VIDEO_*_VDM are UNIMPLEMENTED!\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case IOCTL_VIDEO_SET_OUTPUT_DEVICE_POWER_STATE:
        case IOCTL_VIDEO_GET_OUTPUT_DEVICE_POWER_STATE:
            WARN_(VIDEOPRT, "- IOCTL_VIDEO_GET/SET_OUTPUT_DEVICE_POWER_STATE are UNIMPLEMENTED!\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case IOCTL_VIDEO_SET_POWER_MANAGEMENT:
        case IOCTL_VIDEO_GET_POWER_MANAGEMENT:
            WARN_(VIDEOPRT, "- IOCTL_VIDEO_GET/SET_POWER_MANAGEMENT are UNIMPLEMENTED!\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case IOCTL_VIDEO_QUERY_SUPPORTED_BRIGHTNESS:
        case IOCTL_VIDEO_QUERY_DISPLAY_BRIGHTNESS:
        case IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS:
            WARN_(VIDEOPRT, "- IOCTL_VIDEO_*_BRIGHTNESS are UNIMPLEMENTED!\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case IOCTL_VIDEO_INIT_WIN32K_CALLBACKS:
            INFO_(VIDEOPRT, "- IOCTL_VIDEO_INIT_WIN32K_CALLBACKS\n");
            Status = VideoPortInitWin32kCallbacks(DeviceObject,
                                                  Irp->AssociatedIrp.SystemBuffer,
                                                  IrpStack->Parameters.DeviceIoControl.InputBufferLength,
                                                  &Irp->IoStatus.Information);
            break;

        case IOCTL_VIDEO_IS_VGA_DEVICE:
            WARN_(VIDEOPRT, "- IOCTL_VIDEO_IS_VGA_DEVICE is UNIMPLEMENTED!\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case IOCTL_VIDEO_USE_DEVICE_IN_SESSION:
            INFO_(VIDEOPRT, "- IOCTL_VIDEO_USE_DEVICE_IN_SESSION\n");
            Status = VideoPortUseDeviceInSession(DeviceObject,
                                                 Irp->AssociatedIrp.SystemBuffer,
                                                 IrpStack->Parameters.DeviceIoControl.InputBufferLength,
                                                 &Irp->IoStatus.Information);
            break;

        case IOCTL_VIDEO_PREPARE_FOR_EARECOVERY:
            INFO_(VIDEOPRT, "- IOCTL_VIDEO_PREPARE_FOR_EARECOVERY\n");
            /*
             * The Win32k Watchdog Timer detected that a thread spent more time
             * in a display driver than the allotted time its threshold specified,
             * and thus is going to attempt to recover by switching to VGA mode.
             * If this attempt fails, the watchdog generates bugcheck 0xEA
             * "THREAD_STUCK_IN_DEVICE_DRIVER".
             *
             * Prepare the recovery by resetting the display adapters to
             * standard VGA 80x25 text mode.
             */
            IntVideoPortResetDisplayParametersEx(80, 25, FALSE);
            Status = STATUS_SUCCESS;
            break;

        default:
            /* Forward to the Miniport Driver */
            Status = VideoPortForwardDeviceControl(DeviceObject, Irp);
            break;
    }

    INFO_(VIDEOPRT, "- Returned status: 0x%x\n", Status);

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_VIDEO_INCREMENT);
    return Status;
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
        FullList = AllocatedResources->List;
        ASSERT(AllocatedResources->Count == 1);
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
    NTSTATUS Status;

    if (InterlockedCompareExchange((PLONG)&DeviceExtension->DeviceOpened, 0, 0) == 0)
    {
        /* Device not opened. Don't enumerate children yet */
        WARN_(VIDEOPRT, "Skipping child enumeration because device is not opened");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    /* Query children of the device. */
    Status = IntVideoPortEnumerateChildren(DeviceObject, Irp);
    if (!NT_SUCCESS(Status))
        return Status;

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
            /* Call lower drivers, and ignore result (that's probably STATUS_NOT_SUPPORTED) */
            (VOID)IntVideoPortForwardIrpAndWait(DeviceObject, Irp);
            /* Now, fill resource requirements list */
            Status = IntVideoPortFilterResourceRequirements(DeviceObject, IrpSp, Irp);
            Irp->IoStatus.Status = Status;
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
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status = Irp->IoStatus.Status;
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    if (DeviceExtension->Common.Fdo)
    {
        PoStartNextPowerIrp(Irp);
        IoSkipCurrentIrpStackLocation(Irp);
        return PoCallDriver(DeviceExtension->NextDeviceObject, Irp);
    }
    else
    {
        switch (IrpSp->MinorFunction)
        {
            case IRP_MN_QUERY_POWER:
            case IRP_MN_SET_POWER:
                Status = STATUS_SUCCESS;
                break;
        }
        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }
}

NTSTATUS
NTAPI
IntVideoPortDispatchSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status;
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

    if (DeviceExtension->Common.Fdo)
    {
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
    }
    else
    {
        Status = Irp->IoStatus.Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }
}

VOID
NTAPI
IntVideoPortUnload(PDRIVER_OBJECT DriverObject)
{
}
