// Copyright (c) 2004, Antony C. Roberts

// Use of this file is subject to the terms
// described in the LICENSE.TXT file that
// accompanies this file.
//
// Your use of this file indicates your
// acceptance of the terms described in
// LICENSE.TXT.
//
// http://www.freebt.net

#ifndef _FREEBT_USER_H
#define _FREEBT_USER_H

#include <initguid.h>
//#include <winioctl.h>

// {7591F7C7-E760-434a-92D3-C1869930423C}
DEFINE_GUID(GUID_CLASS_FREEBT_USB,
0x7591f7c7, 0xe760, 0x434a, 0x92, 0xd3, 0xc1, 0x86, 0x99, 0x30, 0x42, 0x3c);

#define FREEBT_IOCTL_INDEX             0x0000


#define IOCTL_FREEBT_GET_CONFIG_DESCRIPTOR	CTL_CODE(FILE_DEVICE_UNKNOWN,     \
                                                     FREEBT_IOCTL_INDEX,     \
                                                     METHOD_BUFFERED,         \
                                                     FILE_ANY_ACCESS)

#define IOCTL_FREEBT_HCI_SEND_CMD			CTL_CODE(FILE_DEVICE_UNKNOWN,     \
														FREEBT_IOCTL_INDEX + 1, \
														METHOD_BUFFERED,         \
														FILE_ANY_ACCESS)

#define IOCTL_FREEBT_HCI_GET_EVENT			CTL_CODE(FILE_DEVICE_UNKNOWN,     \
														FREEBT_IOCTL_INDEX + 2, \
														METHOD_BUFFERED,         \
														FILE_ANY_ACCESS)

#endif

