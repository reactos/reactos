/*
 * usbdi.h
 *
 * USBD and USB device driver definitions
 *
 * FIXME : Obsolete header.. Use usb.h instead.
 *
 * This file is part of the ReactOS PSDK package.
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

#pragma once

/* Helper macro to enable gcc's extension.  */
#ifndef __GNU_EXTENSION
#ifdef __GNUC__
#define __GNU_EXTENSION __extension__
#else
#define __GNU_EXTENSION
#endif
#endif

#include <usb.h>
#include <usbioctl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USBD_STATUS_CANCELLING      ((USBD_STATUS)0x00020000L)
#define USBD_STATUS_CANCELING       ((USBD_STATUS)0x00020000L)
#define USBD_STATUS_NO_MEMORY       ((USBD_STATUS)0x80000100L)
#define USBD_STATUS_ERROR           ((USBD_STATUS)0x80000000L)
#define USBD_STATUS_REQUEST_FAILED  ((USBD_STATUS)0x80000500L)
#define USBD_STATUS_HALTED          ((USBD_STATUS)0xC0000000L)


#define USBD_HALTED(Status)  ((ULONG)(Status) >> 30 == 3)
#define USBD_STATUS(Status) ((ULONG)(Status) & 0x0FFFFFFFL)

#define URB_FUNCTION_RESERVED0                      0x0016
#define URB_FUNCTION_RESERVED                       0x001D
#define URB_FUNCTION_LAST                           0x0029

#define USBD_PF_DOUBLE_BUFFER           0x00000002

#ifdef USBD_PF_VALID_MASK
#undef USBD_PF_VALID_MASK
#endif

#define USBD_PF_VALID_MASK    (USBD_PF_CHANGE_MAX_PACKET | USBD_PF_DOUBLE_BUFFER | \
                              USBD_PF_ENABLE_RT_THREAD_ACCESS | USBD_PF_MAP_ADD_TRANSFERS)

#define USBD_TRANSFER_DIRECTION_BIT             0
#define USBD_SHORT_TRANSFER_OK_BIT              1
#define USBD_START_ISO_TRANSFER_ASAP_BIT        2

#ifdef USBD_TRANSFER_DIRECTION
#undef USBD_TRANSFER_DIRECTION
#endif

#define USBD_TRANSFER_DIRECTION(x)      ((x) & USBD_TRANSFER_DIRECTION_IN)

#ifdef __cplusplus
}
#endif
