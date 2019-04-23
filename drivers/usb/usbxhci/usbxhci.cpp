/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Extensible Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbxhci/usbxhci.cpp
 * PURPOSE:     USB XHCI device driver(based on Haiku XHCI driver and ReactOS EHCI)
 * PROGRAMMERS: lesanilie@gmail
 *
 */

#include "usbxhci.h"

#define NDEBUG
#include <debug.h>

//
// driver entry point
//
extern
"C"
NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath)
{
    //
    // initialize AddDevice callback
    //
    DriverObject->DriverExtension->AddDevice = USBLIB_AddDevice;

    //
    // initialize driver object dispatch table
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE]                  = USBLIB_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                   = USBLIB_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]                 = USBLIB_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]          = USBLIB_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = USBLIB_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]          = USBLIB_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_POWER]                   = USBLIB_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_PNP]                     = USBLIB_Dispatch;
    return STATUS_SUCCESS;
}

extern "C"
{
    void __cxa_pure_virtual()
    {
        DbgBreakPoint();
    }
}

extern "C"
{
    void free(void *ptr)
    {
        ExFreePool(ptr);
    }
}
