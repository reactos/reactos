/* WINE ifdef.h
 * Copyright 2010 Juan Lang
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#ifndef WINE_IFDEF_H
#define WINE_IFDEF_H

#include <ipifcons.h>

typedef ULONG32 NET_IF_OBJECT_ID, *PNET_IF_OBJECT_ID;
typedef UINT32 NET_IF_COMPARTMENT_ID, *PNET_IF_COMPARTMENT_ID;
typedef GUID NET_IF_NETWORK_GUID, *PNET_IF_NETWORK_GUID;
typedef ULONG NET_IFINDEX, *PNET_IFINDEX;
typedef NET_IFINDEX IF_INDEX, *PIF_INDEX;
typedef UINT16 NET_IFTYPE, *PNET_IFTYPE;

#define NET_IF_OPER_STATUS_DOWN_NOT_AUTHENTICATED   0x00000001
#define NET_IF_OPER_STATUS_DOWN_NOT_MEDIA_CONNECTED 0x00000002
#define NET_IF_OPER_STATUS_DORMANT_PAUSED           0x00000004
#define NET_IF_OPER_STATUS_DORMANT_LOW_POWER        0x00000008

#define NET_IF_COMPARTMENT_ID_UNSPECIFIED 0
#define NET_IF_COMPARTMENT_ID_PRIMARY     1

#define NET_IF_OID_IF_ALIAS       0x00000001
#define NET_IF_OID_COMPARTMENT_ID 0x00000002
#define NET_IF_OID_NETWORK_GUID   0x00000003
#define NET_IF_OID_IF_ENTRY       0x00000004

#define NET_SET_UNSPECIFIED_NETWORK_GUID(x)
#define NET_IS_UNSPECIFIED_NETWORK_GUID(x)

#define NET_SITEID_UNSPECIFIED 0
#define NET_SITEID_MAXUSER     0x07ffffff
#define NET_SITEID_MAXSYSTEM   0x0fffffff

#define NET_IFINDEX_UNSPECIFIED 0
#define IFI_UNSPECIFIED         NET_IFINDEX_UNSPECIFIED

#define NET_IFLUID_UNSPECIFIED 0

#define NIIF_HARDWARE_INTERFACE      0x00000001
#define NIIF_FILTER_INTERFACE        0x00000002
#define NIIF_NDIS_WDM_INTERFACE      0x00000020
#define NIIF_NDIS_ENDPOINT_INTERFACE 0x00000040
#define NIIF_NDIS_ISCSI_INTERFACE    0x00000080
#define NIIF_WAN_TUNNEL_TYPE_UNKNOWN 0xffffffff

#define NET_BUS_NUMBER_UNKNOWN      0xffffffff
#define NET_SLOT_NUMBER_UNKNOWN     0xffffffff
#define NET_FUNCTION_NUMBER_UNKNOWN 0xffffffff

#define IF_MAX_STRING_SIZE 256
#define IF_MAX_PHYS_ADDRESS_LENGTH 32

typedef enum _NET_IF_ADMIN_STATUS {
    NET_IF_ADMIN_STATUS_UP      = 1,
    NET_IF_ADMIN_STATUS_DOWN    = 2,
    NET_IF_ADMIN_STATUS_TESTING = 3
} NET_IF_ADMIN_STATUS, *PNET_IF_ADMIN_STATUS;

typedef enum _NET_IF_OPER_STATUS {
    NET_IF_OPER_STATUS_UP               = 1,
    NET_IF_OPER_STATUS_DOWN             = 2,
    NET_IF_OPER_STATUS_TESTING          = 3,
    NET_IF_OPER_STATUS_UNKNOWN          = 4,
    NET_IF_OPER_STATUS_DORMANT          = 5,
    NET_IF_OPER_STATUS_NOT_PRESENT      = 6,
    NET_IF_OPER_STATUS_LOWER_LAYER_DOWN = 7
} NET_IF_OPER_STATUS, *PNET_IF_OPER_STATUS;

typedef enum _NET_IF_RCV_ADDRESS_TYPE {
    NET_IF_RCV_ADDRESS_TYPE_OTHER        = 1,
    NET_IF_RCV_ADDRESS_TYPE_VOLATILE     = 2,
    NET_IF_RCV_ADDRESS_TYPE_NON_VOLATILE = 3
} NET_IF_RCV_ADDRESS_TYPE, *PNET_IF_RCV_ADDRESS_TYPE;

typedef struct _NET_IF_RCV_ADDRESS_LH {
    NET_IF_RCV_ADDRESS_TYPE ifRcvAddressType;
    USHORT                  ifRcvAddressLength;
    USHORT                  ifRcvAddressOffset;
} NET_IF_RCV_ADDRESS_LH, *PNET_IF_RCV_ADDRESS_LH;
typedef NET_IF_RCV_ADDRESS_LH NET_IF_RCV_ADDRESS;
typedef NET_IF_RCV_ADDRESS_LH  *PNET_IF_RCV_ADDRESS;

typedef union _NET_LUID_LH {
    ULONG64 Value;
    struct {
        ULONG64 Reserved : 24;
        ULONG64 NetLuidIndex : 24;
        ULONG64 IfType : 16;
    } Info;
} NET_LUID_LH, *PNET_LUID_LH;
typedef NET_LUID_LH NET_LUID;
typedef NET_LUID *PNET_LUID;
typedef NET_LUID IF_LUID;
typedef NET_LUID *PIF_LUID;

typedef enum _NET_IF_ACCESS_TYPE {
    NET_IF_ACCESS_LOOPBACK             = 1,
    NET_IF_ACCESS_BROADCAST            = 2,
    NET_IF_ACCESS_POINT_TO_POINT       = 2,
    NET_IF_ACCESS_POINT_TO_MULTI_POINT = 4,
    NET_IF_ACCESS_MAXIMUM              = 5
} NET_IF_ACCESS_TYPE, *PNET_IF_ACCESS_TYPE;

typedef enum _NET_IF_DIRECTION_TYPE {
    NET_IF_DIRECTION_SENDRECEIVE = 0,
    NET_IF_DIRECTION_SENDONLY    = 1,
    NET_IF_DIRECTION_RECEIVEONLY = 2,
    NET_IF_DIRECTION_MAXIMUM     = 3
} NET_IF_DIRECTION_TYPE, *PNET_IF_DIRECTION_TYPE;

typedef enum _NET_IF_CONNECTION_TYPE {
    NET_IF_CONNECTION_DEDICATED = 1,
    NET_IF_CONNECTION_PASSIVE   = 2,
    NET_IF_CONNECTION_DEMAND    = 3,
    NET_IF_CONNECTION_MAXIMUM   = 4,
} NET_IF_CONNECTION_TYPE, *PNET_IF_CONNECTION_TYPE;

typedef enum _NET_IF_MEDIA_CONNECT_STATE {
    MediaConnectStateUnknown      = 0,
    MediaConnectStateConnected    = 1,
    MediaConnectStateDisconnected = 2
} NET_IF_MEDIA_CONNECT_STATE, *PNET_IF_MEDIA_CONNECT_STATE;

typedef enum _NET_IF_MEDIA_DUPLEX_STATE {
    MediaDuplexStateUnknown = 0,
    MediaDuplexStateHalf    = 1,
    MediaDuplexStateFull    = 2
} NET_IF_MEDIA_DUPLEX_STATE, *PNET_IF_MEDIA_DUPLEX_STATE;

typedef struct _NET_PHYSICAL_LOCATION_LH {
    ULONG BusNumber;
    ULONG SlotNumber;
    ULONG FunctionNumber;
} NET_PHYSICAL_LOCATION_LH, *PNET_PHYSICAL_LOCATION_LH;
typedef NET_PHYSICAL_LOCATION_LH NET_PHYSICAL_LOCATION;
typedef NET_PHYSICAL_LOCATION *PNET_PHYSICAL_LOCATION;

typedef struct _IF_COUNTED_STRING_LH {
    USHORT Length;
    WCHAR  String[IF_MAX_STRING_SIZE + 1];
} IF_COUNTED_STRING_LH, *PIF_COUNTED_STRING_LH;
typedef IF_COUNTED_STRING_LH IF_COUNTED_STRING;
typedef IF_COUNTED_STRING *PIF_COUNTED_STRING;

typedef struct _IF_PHYSICAL_ADDRESS_LH {
    USHORT Length;
    UCHAR  Address[IF_MAX_PHYS_ADDRESS_LENGTH];
} IF_PHYSICAL_ADDRESS_LH, *PIF_PHYSICAL_ADDRESS_LH;
typedef IF_PHYSICAL_ADDRESS_LH IF_PHYSICAL_ADDRESS;
typedef IF_PHYSICAL_ADDRESS *PIF_PHYSICAL_ADDRESS;

typedef enum {
    TUNNEL_TYPE_NONE    = 0,
    TUNNEL_TYPE_OTHER   = 1,
    TUNNEL_TYPE_DIRECT  = 2,
    TUNNEL_TYPE_6TO4    = 11,
    TUNNEL_TYPE_ISATAP  = 13,
    TUNNEL_TYPE_TEREDO  = 14,
    TUNNEL_TYPE_IPHTTPS = 15,
} TUNNEL_TYPE;

typedef enum _IF_ADMINISTRATIVE_STATE {
    IF_ADMINISTRATIVE_STATE_DISABLED   = 0,
    IF_ADMINISTRATIVE_STATE_ENABLED    = 1,
    IF_ADMINISTRATIVE_STATE_DEMANDDIAL = 2
} IF_ADMINISTRATIVE_STATE, *PIF_ADMINISTRATIVE_STATE;

typedef enum {
    IfOperStatusUp = 1,
    IfOperStatusDown,
    IfOperStatusTesting,
    IfOperStatusUnknown,
    IfOperStatusDormant,
    IfOperStatusNotPresent,
    IfOperStatusLowerLayerDown
} IF_OPER_STATUS;

typedef struct _NDIS_INTERFACE_INFORMATION {
    NET_IF_OPER_STATUS         ifOperStatus;
    ULONG                      ifOperStatusFlags;
    NET_IF_MEDIA_CONNECT_STATE MediaConnectState;
    NET_IF_MEDIA_DUPLEX_STATE  MediaDuplexState;
    ULONG                      ifMtu;
    BOOLEAN                    ifPromiscuousMode;
    BOOLEAN                    ifDeviceWakeUpEnable;
    ULONG64                    XmitLinkSpeed;
    ULONG64                    RcvLinkSpeed;
    ULONG64                    ifLastChange;
    ULONG64                    ifCounterDiscontinuityTime;
    ULONG64                    ifInUnknownProtos;
    ULONG64                    ifInDiscards;
    ULONG64                    ifInErrors;
    ULONG64                    ifHCInOctets;
    ULONG64                    ifHCInUcastPkts;
    ULONG64                    ifHCInMulticastPkts;
    ULONG64                    ifHCInBroadcastPkts;
    ULONG64                    ifHCOutOctets;
    ULONG64                    ifHCOutUcastPkts;
    ULONG64                    ifHCOutMulticastPkts;
    ULONG64                    ifHCOutBroadcastPkts;
    ULONG64                    ifOutErrors;
    ULONG64                    ifOutDiscards;
    ULONG64                    ifHCInUcastOctets;
    ULONG64                    ifHCInMulticastOctets;
    ULONG64                    ifHCInBroadcastOctets;
    ULONG64                    ifHCOutUcastOctets;
    ULONG64                    ifHCOutMulticastOctets;
    ULONG64                    ifHCOutBroadcastOctets;
    NET_IF_COMPARTMENT_ID      CompartmentId;
    ULONG                      SupportedStatistics;
} NDIS_INTERFACE_INFORMATION, *PNDIS_INTERFACE_INFORMATION;

#endif /* WINE_IFDEF_H*/
