#ifndef USBEHCI_H__
#define USBEHCI_H__

#include <ntddk.h>
#define NDEBUG
#include <debug.h>
#include <hubbusif.h>
#include <usbbusif.h>
#include <usbioctl.h>

extern "C"
{
#include <usbdlib.h>
}



//
// FIXME: 
// #include <usbprotocoldefs.h>
//
#include <usb.h>
#include <stdio.h>
#include <wdmguid.h>

//
// FIXME:
// the following includes are required to get kcom to compile
//
#include <portcls.h>
#include <dmusicks.h>
#include <kcom.h>

#include "interfaces.h"

//
// tag for allocations
//
#define TAG_USBEHCI 'ICHE'

//
// assert for c++ - taken from portcls
//
#define PC_ASSERT(exp) \
  (VOID)((!(exp)) ? \
    RtlAssert((PVOID) #exp, (PVOID)__FILE__, __LINE__, NULL ), FALSE : TRUE)

//
// hardware.cpp
//
NTSTATUS NTAPI CreateUSBHardware(PUSBHARDWAREDEVICE *OutHardware);

//
// usb_queue.cpp
//
NTSTATUS NTAPI CreateUSBQueue(PUSBQUEUE *OutUsbQueue);

//
// usb_request.cpp
//
NTSTATUS NTAPI InternalCreateUSBRequest(PUSBREQUEST *OutRequest);

#endif
