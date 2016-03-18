#ifndef USBUHCI_H__
#define USBUHCI_H__

#include <libusb.h>

#include "hardware.h"
#include "interfaces.h"

//
// flags for handling USB_REQUEST_SET_FEATURE / USB_REQUEST_GET_FEATURE
//
#define PORT_ENABLE         1
#define PORT_SUSPEND        2
#define PORT_OVER_CURRENT   3
#define PORT_RESET          4
#define PORT_POWER          8
#define C_PORT_CONNECTION   16
#define C_PORT_ENABLE       17
#define C_PORT_SUSPEND      18
#define C_PORT_OVER_CURRENT 19
#define C_PORT_RESET        20

//
// tag for allocations
//
#define TAG_USBUHCI 'ICHU'

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

#endif /* USBUHCI_H__ */
