/*
 * netpnp.h
 *
 * Network Plug and Play event support
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

#ifndef __NETPNP_H
#define __NETPNP_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _NET_PNP_EVENT_CODE {
  NetEventSetPower,
  NetEventQueryPower,
  NetEventQueryRemoveDevice,
  NetEventCancelRemoveDevice,
  NetEventReconfigure,
  NetEventBindList,
  NetEventBindsComplete,
  NetEventPnPCapabilities,
  NetEventMaximum
} NET_PNP_EVENT_CODE, *PNET_PNP_EVENT_CODE;

typedef struct _NET_PNP_EVENT {
  NET_PNP_EVENT_CODE  NetEvent;
  PVOID  Buffer;
  ULONG  BufferLength;
  ULONG_PTR  NdisReserved[4];
  ULONG_PTR  TransportReserved[4];
  ULONG_PTR  TdiReserved[4];
  ULONG_PTR  TdiClientReserved[4];
} NET_PNP_EVENT, *PNET_PNP_EVENT;

typedef enum _NET_DEVICE_POWER_STATE {
  NetDeviceStateUnspecified,
  NetDeviceStateD0,
  NetDeviceStateD1,
  NetDeviceStateD2,
  NetDeviceStateD3,
  NetDeviceStateMaximum
} NET_DEVICE_POWER_STATE, *PNET_DEVICE_POWER_STATE;

#ifdef __cplusplus
}
#endif

#endif /* __NETPNP_H */
