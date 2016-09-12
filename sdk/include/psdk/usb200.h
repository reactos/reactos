/*
 * usb200.h
 *
 * This file is part of the ReactOS PSDK package.
 *
 * Contributors:
 *   Magnus Olsen.
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

#pragma once

/* Helper macro to enable gcc's extension. */
#ifndef __GNU_EXTENSION
#ifdef __GNUC__
#define __GNU_EXTENSION __extension__
#else
#define __GNU_EXTENSION
#endif
#endif

#include "usb100.h"

#include <pshpack1.h>

typedef enum _USB_DEVICE_TYPE {
  Usb11Device = 0,
  Usb20Device
} USB_DEVICE_TYPE;

typedef enum _USB_DEVICE_SPEED {
  UsbLowSpeed = 0,
  UsbFullSpeed,
  UsbHighSpeed
} USB_DEVICE_SPEED;

#define USB_PORT_STATUS_CONNECT                       0x0001
#define USB_PORT_STATUS_ENABLE                        0x0002
#define USB_PORT_STATUS_SUSPEND                       0x0004
#define USB_PORT_STATUS_OVER_CURRENT                  0x0008
#define USB_PORT_STATUS_RESET                         0x0010
#define USB_PORT_STATUS_POWER                         0x0100
#define USB_PORT_STATUS_LOW_SPEED                     0x0200
#define USB_PORT_STATUS_HIGH_SPEED                    0x0400


typedef union _BM_REQUEST_TYPE {
  struct _BM {
    UCHAR Recipient:2;
    UCHAR Reserved:3;
    UCHAR Type:2;
    UCHAR Dir:1;
  } _BM;
  UCHAR B;
} BM_REQUEST_TYPE, *PBM_REQUEST_TYPE;

typedef struct _USB_DEFAULT_PIPE_SETUP_PACKET {
  BM_REQUEST_TYPE bmRequestType;
  UCHAR bRequest;
  union _wValue {
    __GNU_EXTENSION struct {
      UCHAR LowByte;
      UCHAR HiByte;
    };
    USHORT W;
  } wValue;
  union _wIndex {
    __GNU_EXTENSION struct {
      UCHAR LowByte;
      UCHAR HiByte;
    };
    USHORT W;
  } wIndex;
  USHORT wLength;
} USB_DEFAULT_PIPE_SETUP_PACKET, *PUSB_DEFAULT_PIPE_SETUP_PACKET;

C_ASSERT(sizeof(USB_DEFAULT_PIPE_SETUP_PACKET) == 8);

#define USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE          0x06
#define USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_TYPE 0x07

typedef struct _USB_DEVICE_QUALIFIER_DESCRIPTOR {
  UCHAR bLength;
  UCHAR bDescriptorType;
  USHORT bcdUSB;
  UCHAR bDeviceClass;
  UCHAR bDeviceSubClass;
  UCHAR bDeviceProtocol;
  UCHAR bMaxPacketSize0;
  UCHAR bNumConfigurations;
  UCHAR bReserved;
} USB_DEVICE_QUALIFIER_DESCRIPTOR, *PUSB_DEVICE_QUALIFIER_DESCRIPTOR;

typedef union _USB_HIGH_SPEED_MAXPACKET {
  struct _MP {
    USHORT MaxPacket:11;
    USHORT HSmux:2;
    USHORT Reserved:3;
  } _MP;
  USHORT us;
} USB_HIGH_SPEED_MAXPACKET, *PUSB_HIGH_SPEED_MAXPACKET;

#define USB_INTERFACE_ASSOCIATION_DESCRIPTOR_TYPE     0x0B

typedef struct _USB_INTERFACE_ASSOCIATION_DESCRIPTOR {
  UCHAR bLength;
  UCHAR bDescriptorType;
  UCHAR bFirstInterface;
  UCHAR bInterfaceCount;
  UCHAR bFunctionClass;
  UCHAR bFunctionSubClass;
  UCHAR bFunctionProtocol;
  UCHAR iFunction;
} USB_INTERFACE_ASSOCIATION_DESCRIPTOR, *PUSB_INTERFACE_ASSOCIATION_DESCRIPTOR;

#include <poppack.h>
