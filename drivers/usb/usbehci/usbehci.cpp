/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/usbehci.cpp
 * PURPOSE:     USB EHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbehci.h"

#define NDEBUG
#include <debug.h>

extern
"C"
NTSTATUS
NTAPI
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath)
{

    /* initialize driver object */
    DriverObject->DriverExtension->AddDevice = USBLIB_AddDevice;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = USBLIB_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = USBLIB_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = USBLIB_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = USBLIB_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = USBLIB_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = USBLIB_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_POWER] = USBLIB_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_PNP] = USBLIB_Dispatch;
    return STATUS_SUCCESS;
}

#ifndef _UNITY_BUILD_ENABLED_
extern "C" {
  void
	  __cxa_pure_virtual()
  {
    // put error handling here

    DbgBreakPoint();

  }
}
#endif

extern "C" {
   void free(void * ptr)
   {
       ExFreePool(ptr);
   }
}


