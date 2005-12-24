/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS Multimedia
 * FILE:                 lib/wdmaud/kernel.c
 * PURPOSE:              WDM Audio Support - Kernel Mode Interface
 * PROGRAMMER:           Andrew Greenwood
 * UPDATE HISTORY:
 *                       Nov 18, 2005: Created
 */

#define INITGUID    /* FIXME */

#include <windows.h>
#include <setupapi.h>
#include "wdmaud.h"

/* HACK ALERT - This goes in ksmedia.h */
DEFINE_GUID(KSCATEGORY_WDMAUD,
    0x3e227e76L, 0x690d, 0x11d2, 0x81, 0x61, 0x00, 0x00, 0xf8, 0x77, 0x5b, 0xf1);

/* This stores the handle of the kernel device */
static HANDLE kernel_device_handle = NULL;

//static WCHAR* 


/*
    TODO: There's a variant of this that uses critical sections...
*/

MMRESULT CallKernelDevice(
    PWDMAUD_DEVICE_INFO device,
    DWORD ioctl_code,
    DWORD param1,
    DWORD param2)
{
    OVERLAPPED overlap;
    MMRESULT result = MMSYSERR_ERROR;
    DWORD name_len = 0;
    DWORD bytes_returned = 0;
    BOOL using_critical_section = FALSE;

    ASSERT(kernel_device_handle);
    ASSERT(device);

    DPRINT("Creating event\n");
    overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if ( ! overlap.hEvent )
    {
        DPRINT1("CreateEvent failed - error %d\n", (int)GetLastError());
        result = MMSYSERR_NOMEM;
        goto cleanup;
    }

    DPRINT("Sizeof wchar == %d\n", (int) sizeof(WCHAR));
    name_len = lstrlen(device->path) * sizeof(WCHAR);   /* ok ? */

    /* These seem to carry optional structures */
    device->ioctl_param1 = param1;
    device->ioctl_param2 = param2;

    /* Enter critical section if wave/midi device, and if required */
    if ( ( ! IsMixerDeviceType(device->type) ) &&
         ( ! IsAuxDeviceType(device->type) ) &&
         ( device->with_critical_section ) )
    {
        ASSERT(device->state);
        using_critical_section = TRUE;
        EnterCriticalSection(device->state->device_queue_guard);
    }

    DPRINT("Calling DeviceIoControl with IOCTL %x\n", (int) ioctl_code);
    
    if ( ! DeviceIoControl(kernel_device_handle,
                           ioctl_code,
                           device,
                           name_len + sizeof(WDMAUD_DEVICE_INFO),
                           device,
                           sizeof(WDMAUD_DEVICE_INFO),
                           &bytes_returned,
                           &overlap) )
    {
        DWORD error = GetLastError();

        if (error != ERROR_IO_PENDING)
        {
            DPRINT1("FAILED in CallKernelDevice (error %d)\n", (int) error);

            DUMP_WDMAUD_DEVICE_INFO(device);

            result = TranslateWinError(error);
            goto cleanup;
        }

        DPRINT("Waiting for overlap I/O event\n");

        /* Wait for the IO to be complete */
        WaitForSingleObject(overlap.hEvent, INFINITE);
    }

    result = MMSYSERR_NOERROR;
    DPRINT("CallKernelDevice succeeded :)\n");

    DUMP_WDMAUD_DEVICE_INFO(device);

    cleanup :
    {
        /* Leave the critical section */
        if ( using_critical_section )
            LeaveCriticalSection(device->state->device_queue_guard);

        if ( overlap.hEvent )
            CloseHandle(overlap.hEvent);

        return result;
    }
}


static BOOL ChangeKernelDeviceState(BOOL enable)
{
    PWDMAUD_DEVICE_INFO device = NULL;
    DWORD ioctl_code;
    MMRESULT call_result;

    ioctl_code = enable ? IOCTL_WDMAUD_HELLO : IOCTL_WDMAUD_GOODBYE;

    device = CreateDeviceData(WDMAUD_AUX, 0, L"", FALSE);

    if ( ! device )
    {
        DPRINT1("Couldn't create a new device instance structure\n");
        return FALSE;
    }

    device->with_critical_section = FALSE;

    DPRINT("Calling kernel device\n");

    call_result = CallKernelDevice(device, ioctl_code, 0, 0);

    DeleteDeviceData(device);

    if ( call_result != MMSYSERR_NOERROR )
    {
        DPRINT1("Kernel device doesn't like us! (error %d)\n", (int) GetLastError());
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}


BOOL EnableKernelInterface()
{
    /* SetupAPI variables/structures for querying device data */
    SP_DEVICE_INTERFACE_DATA interface_data;
    PSP_DEVICE_INTERFACE_DETAIL_DATA detail = NULL;
    DWORD detail_size = 0;
    HANDLE heap = NULL;
    HDEVINFO dev_info;

    /* Set to TRUE right at the end to define cleanup behaviour */
    BOOL success = FALSE;

    /* Don't want to be called more than once */
    ASSERT(kernel_device_handle == NULL);

    dev_info = SetupDiGetClassDevs(&KSCATEGORY_WDMAUD,
                                   NULL,
                                   NULL,
                                   DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    if ( ( ! dev_info ) || ( dev_info == INVALID_HANDLE_VALUE ) )
    {
        DPRINT1("SetupDiGetClassDevs failed\n");
        goto cleanup;
    }

    interface_data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    if ( ! SetupDiEnumDeviceInterfaces(dev_info,
                                       NULL,
                                       &KSCATEGORY_WDMAUD,
                                       0,
                                       &interface_data) )
    {
        DPRINT1("SetupDiEnumDeviceInterfaces failed\n");
        goto cleanup;
    }

    /*
        We need to find out the size of the interface detail, before we can
        actually retrieve the detail. This is a bit backwards, as the function
        will return a status of success if the interface is invalid, but we
        need it to fail with ERROR_INSUFFICIENT_BUFFER so we can be told how
        much memory we need to allocate.
    */

    if ( SetupDiGetDeviceInterfaceDetail(dev_info,
                                         &interface_data,
                                         NULL,
                                         0,
                                         &detail_size,
                                         NULL) )
    {
        DPRINT1("SetupDiGetDeviceInterfaceDetail shouldn't succeed!\n");
        goto cleanup;
    }

    /*
        Now we make sure the error was the one we expected. If not, bail out.
    */

    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
    {
        DPRINT1("SetupDiGetDeviceInterfaceDetail returned wrong error code\n");
        goto cleanup;
    }

    heap = GetProcessHeap();

    if ( ! heap )
    {
        DPRINT1("Unable to get the process heap (error %d)\n",
                (int)GetLastError());
        goto cleanup;
    }

    detail = (PSP_DEVICE_INTERFACE_DETAIL_DATA) HeapAlloc(heap,
                                                          HEAP_ZERO_MEMORY,
                                                          detail_size);

    if ( ! detail )
    {
        DPRINT1("Unable to allocate memory for the detail buffer (error %d)\n",
                (int)GetLastError());
        goto cleanup;
    }

    detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

    if ( ! SetupDiGetDeviceInterfaceDetail(dev_info,
                                           &interface_data,
                                           detail,
                                           detail_size,
                                           0,
                                           NULL) )
    {
        DPRINT1("SetupDiGetDeviceInterfaceDetail failed\n");
        goto cleanup;
    }

    DPRINT("Device path: %S\n", detail->DevicePath);

    /* FIXME - params! */
    kernel_device_handle = CreateFile(detail->DevicePath,
                                      0xC0000000,
                                      0,
                                      0,
                                      3,
                                      0x40000080,
                                      0);

    DPRINT("kernel_device_handle == 0x%x\n", (int) kernel_device_handle);

    if ( ! kernel_device_handle )
    {
        DPRINT1("Unable to open kernel device (error %d)\n",
                (int) GetLastError());
        goto cleanup;
    }

    /* Now we say hello to wdmaud.sys */
    if ( ! ChangeKernelDeviceState(TRUE) )
    {
        DPRINT1("Couldn't enable the kernel device\n");
        goto cleanup;
    }

    success = TRUE;

    cleanup :
    {
        DPRINT("Cleanup - success == %d\n", (int) success);

        if ( ! success )
        {
            DPRINT("Failing\n");

            if ( kernel_device_handle )
                CloseHandle(kernel_device_handle);
        }

        if ( heap )
        {
            if ( detail )
                HeapFree(heap, 0, detail);
        }
    }

    return success;
}

/*
    Nothing here should fail, but if it does, we just give up and ASSERT(). If
    we don't, we could be left in a limbo-state (eg: device open but disabled.)
*/

BOOL DisableKernelInterface()
{
    return ChangeKernelDeviceState(FALSE);
}


/*
    The use of this should be avoided...
*/

HANDLE GetKernelInterface()
{
    return kernel_device_handle;
}
