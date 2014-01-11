#ifndef USBEHCI_H__
#define USBEHCI_H__

#include <libusb.h>

#include "hardware.h"
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

#endif /* USBEHCI_H__ */
