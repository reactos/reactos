/*
 * usb100.h
 *
 * USB 1.0 support
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __USB100_H
#define __USB100_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "ntddk.h"

#define MAXIMUM_USB_STRING_LENGTH         255

#define USB_DEVICE_CLASS_RESERVED           0x00
#define USB_DEVICE_CLASS_AUDIO              0x01
#define USB_DEVICE_CLASS_COMMUNICATIONS     0x02
#define USB_DEVICE_CLASS_HUMAN_INTERFACE    0x03
#define USB_DEVICE_CLASS_MONITOR            0x04
#define USB_DEVICE_CLASS_PHYSICAL_INTERFACE 0x05
#define USB_DEVICE_CLASS_POWER              0x06
#define USB_DEVICE_CLASS_PRINTER            0x07
#define USB_DEVICE_CLASS_STORAGE            0x08
#define USB_DEVICE_CLASS_HUB                0x09
#define USB_DEVICE_CLASS_VENDOR_SPECIFIC    0xFF

#define USB_RESERVED_DESCRIPTOR_TYPE        0x06
#define USB_CONFIG_POWER_DESCRIPTOR_TYPE    0x07
#define USB_INTERFACE_POWER_DESCRIPTOR_TYPE 0x08

#define USB_REQUEST_GET_STATUS            0x00
#define USB_REQUEST_CLEAR_FEATURE         0x01
#define USB_REQUEST_SET_FEATURE           0x03
#define USB_REQUEST_SET_ADDRESS           0x05
#define USB_REQUEST_GET_DESCRIPTOR        0x06
#define USB_REQUEST_SET_DESCRIPTOR        0x07
#define USB_REQUEST_GET_CONFIGURATION     0x08
#define USB_REQUEST_SET_CONFIGURATION     0x09
#define USB_REQUEST_GET_INTERFACE         0x0A
#define USB_REQUEST_SET_INTERFACE         0x0B
#define USB_REQUEST_SYNC_FRAME            0x0C

#define USB_GETSTATUS_SELF_POWERED            0x01
#define USB_GETSTATUS_REMOTE_WAKEUP_ENABLED   0x02

#define BMREQUEST_HOST_TO_DEVICE          0
#define BMREQUEST_DEVICE_TO_HOST          1

#define BMREQUEST_STANDARD                0
#define BMREQUEST_CLASS                   1
#define BMREQUEST_VENDOR                  2

#define BMREQUEST_TO_DEVICE               0
#define BMREQUEST_TO_INTERFACE            1
#define BMREQUEST_TO_ENDPOINT             2
#define BMREQUEST_TO_OTHER                3

/* USB_COMMON_DESCRIPTOR.bDescriptorType constants */
#define USB_DEVICE_DESCRIPTOR_TYPE        0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE 0x02
#define USB_STRING_DESCRIPTOR_TYPE        0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE     0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE      0x05

typedef struct _USB_COMMON_DESCRIPTOR {
	UCHAR  bLength;
	UCHAR  bDescriptorType;
} USB_COMMON_DESCRIPTOR, *PUSB_COMMON_DESCRIPTOR;

#define USB_DESCRIPTOR_MAKE_TYPE_AND_INDEX(d, i) ((USHORT)((USHORT)d << 8 | i))

/* USB_CONFIGURATION_DESCRIPTOR.bmAttributes constants */
#define USB_CONFIG_POWERED_MASK           0xc0
#define USB_CONFIG_BUS_POWERED            0x80
#define USB_CONFIG_SELF_POWERED           0x40
#define USB_CONFIG_REMOTE_WAKEUP          0x20

#include <pshpack1.h>
typedef struct _USB_CONFIGURATION_DESCRIPTOR {
  UCHAR  bLength;
  UCHAR  bDescriptorType;
  USHORT  wTotalLength;
  UCHAR  bNumInterfaces;
  UCHAR  bConfigurationValue;
  UCHAR  iConfiguration;
  UCHAR  bmAttributes;
  UCHAR  MaxPower;
} USB_CONFIGURATION_DESCRIPTOR, *PUSB_CONFIGURATION_DESCRIPTOR;
#include <poppack.h>

typedef struct _USB_DEVICE_DESCRIPTOR {
  UCHAR  bLength;
  UCHAR  bDescriptorType;
  USHORT  bcdUSB;
  UCHAR  bDeviceClass;
  UCHAR  bDeviceSubClass;
  UCHAR  bDeviceProtocol;
  UCHAR  bMaxPacketSize0;
  USHORT  idVendor;
  USHORT  idProduct;
  USHORT  bcdDevice;
  UCHAR  iManufacturer;
  UCHAR  iProduct;
  UCHAR  iSerialNumber;
  UCHAR  bNumConfigurations;
} USB_DEVICE_DESCRIPTOR, *PUSB_DEVICE_DESCRIPTOR;

#define USB_ENDPOINT_DIRECTION_MASK       0x80

#define USB_ENDPOINT_DIRECTION_OUT(x) (!((x) & USB_ENDPOINT_DIRECTION_MASK))
#define USB_ENDPOINT_DIRECTION_IN(x) ((x) & USB_ENDPOINT_DIRECTION_MASK)

/* USB_ENDPOINT_DESCRIPTOR.bmAttributes constants */
#define USB_ENDPOINT_TYPE_MASK            0x03
#define USB_ENDPOINT_TYPE_CONTROL         0x00
#define USB_ENDPOINT_TYPE_ISOCHRONOUS     0x01
#define USB_ENDPOINT_TYPE_BULK            0x02
#define USB_ENDPOINT_TYPE_INTERRUPT       0x03

#include <pshpack1.h>
typedef struct _USB_ENDPOINT_DESCRIPTOR {
  UCHAR  bLength;
  UCHAR  bDescriptorType;
  UCHAR  bEndpointAddress;
  UCHAR  bmAttributes;
  USHORT  wMaxPacketSize;
  UCHAR  bInterval;
} USB_ENDPOINT_DESCRIPTOR, *PUSB_ENDPOINT_DESCRIPTOR;
#include <poppack.h>

#define USB_FEATURE_ENDPOINT_STALL        0x0000
#define USB_FEATURE_REMOTE_WAKEUP         0x0001

typedef struct _USB_INTERFACE_DESCRIPTOR {
  UCHAR  bLength;
  UCHAR  bDescriptorType;
  UCHAR  bInterfaceNumber;
  UCHAR  bAlternateSetting;
  UCHAR  bNumEndpoints;
  UCHAR  bInterfaceClass;
  UCHAR  bInterfaceSubClass;
  UCHAR  bInterfaceProtocol;
  UCHAR  iInterface;
} USB_INTERFACE_DESCRIPTOR, *PUSB_INTERFACE_DESCRIPTOR;

typedef struct _USB_STRING_DESCRIPTOR {
  UCHAR  bLength;
  UCHAR  bDescriptorType;
  WCHAR  bString[1];
} USB_STRING_DESCRIPTOR, *PUSB_STRING_DESCRIPTOR;

#include <pshpack1.h>
typedef struct _USB_HUB_DESCRIPTOR {
	UCHAR  bDescriptorLength;
	UCHAR  bDescriptorType;
	UCHAR  bNumberOfPorts;
	USHORT  wHubCharacteristics;
	UCHAR  bPowerOnToPowerGood;
	UCHAR  bHubControlCurrent;
	UCHAR  bRemoveAndPowerMask[64];
} USB_HUB_DESCRIPTOR, *PUSB_HUB_DESCRIPTOR;
#include <poppack.h>

#define USB_SUPPORT_D0_COMMAND            0x01
#define USB_SUPPORT_D1_COMMAND            0x02
#define USB_SUPPORT_D2_COMMAND            0x04
#define USB_SUPPORT_D3_COMMAND            0x08

#define USB_SUPPORT_D1_WAKEUP             0x10
#define USB_SUPPORT_D2_WAKEUP             0x20

typedef struct _USB_CONFIGURATION_POWER_DESCRIPTOR {
	UCHAR  bLength;
	UCHAR  bDescriptorType;
	UCHAR  SelfPowerConsumedD0[3];
	UCHAR  bPowerSummaryId;
	UCHAR  bBusPowerSavingD1;
	UCHAR  bSelfPowerSavingD1;
	UCHAR  bBusPowerSavingD2;
	UCHAR  bSelfPowerSavingD2;
	UCHAR  bBusPowerSavingD3;
	UCHAR  bSelfPowerSavingD3;
	USHORT  TransitionTimeFromD1;
	USHORT  TransitionTimeFromD2;
	USHORT  TransitionTimeFromD3;
} USB_CONFIGURATION_POWER_DESCRIPTOR, *PUSB_CONFIGURATION_POWER_DESCRIPTOR;

#define USB_FEATURE_INTERFACE_POWER_D0    0x0002
#define USB_FEATURE_INTERFACE_POWER_D1    0x0003
#define USB_FEATURE_INTERFACE_POWER_D2    0x0004
#define USB_FEATURE_INTERFACE_POWER_D3    0x0005

#include <pshpack1.h>
typedef struct _USB_INTERFACE_POWER_DESCRIPTOR {
	UCHAR  bLength;
	UCHAR  bDescriptorType;
	UCHAR  bmCapabilitiesFlags;
	UCHAR  bBusPowerSavingD1;
	UCHAR  bSelfPowerSavingD1;
	UCHAR  bBusPowerSavingD2;
	UCHAR  bSelfPowerSavingD2;
	UCHAR  bBusPowerSavingD3;
	UCHAR  bSelfPowerSavingD3;
	USHORT  TransitionTimeFromD1;
	USHORT  TransitionTimeFromD2;
	USHORT  TransitionTimeFromD3;
} USB_INTERFACE_POWER_DESCRIPTOR, *PUSB_INTERFACE_POWER_DESCRIPTOR;
#include <poppack.h>

#ifdef __cplusplus
}
#endif

#endif /* __USB100_H */
