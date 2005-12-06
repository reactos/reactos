#ifndef _NET_LAN_H

#include "net_wh.h"

#define FSCTL_LAN_BASE FILE_DEVICE_NETWORK
#define _LAN_CONTROL_CODE(Function, Method) \
    CTL_CODE(FSCTL_LAN_BASE, Function, Method, FILE_WRITE_ACCESS)

#define LAN_ENUM_ADAPTERS         0
#define LAN_BUFFERED_MODE         1
#define LAN_ADAPTER_INFO          2

#define IOCTL_IF_ENUM_ADAPTERS    \
    _LAN_CONTROL_CODE(LAN_ENUM_ADAPTERS,METHOD_BUFFERED)
#define IOCTL_IF_BUFFERED_MODE    \
    _LAN_CONTROL_CODE(LAN_BUFFERED_MODE,METHOD_BUFFERED)
#define IOCTL_IF_ADAPTER_INFO     \
    _LAN_CONTROL_CODE(LAN_ADAPTER_INFO,METHOD_BUFFERED)

typedef struct _LAN_PACKET_HEADER_T {
    UINT  Adapter;
    UINT  AddressType;
    UINT  AddressLen;
    UINT  PacketType;
    PVOID Mdl;
} LAN_PACKET_HEADER_T, *PLAN_PACKET_HEADER_T;

typedef struct _LAN_PACKET_HEADER {
    LAN_PACKET_HEADER_T Fixed;
    CHAR Address[1];
} LAN_PACKET_HEADER, *PLAN_PACKET_HEADER;

typedef struct _LAN_ADDRESS {
    UINT Adapter;
    UINT Flags;
    USHORT AddressType;
    USHORT AddressLen;
    USHORT HWAddressType;
    USHORT HWAddressLen;
    CHAR Address[1];
} LAN_ADDRESS, *PLAN_ADDRESS;

typedef struct _LAN_ADAPTER_INFO_S {
    UINT Index;
    UINT Media;
    UINT Speed;
    USHORT AddressLen;
    USHORT Overhead;
    USHORT MTU;
    USHORT RegKeySize;
} LAN_ADAPTER_INFO_S, *PLAN_ADAPTER_INFO_S;

#define LAN_DATA_PTR(PH) \
  ((PH)->Address + (PH)->Fixed.AddressLen)
#define LAN_ALLOC_SIZE(AddrLen,PayloadLen) \
  (sizeof(LAN_PACKET_HEADER_T) + (AddrLen) + (PayloadLen))
#define LAN_PAYLOAD_SIZE(PH,Size) \
  (Size - (PH)->Fixed.AddressLen - sizeof(LAN_PACKET_HEADER_T))

#define LAN_ADDR_SIZE(AddrLen,HWAddrLen) \
  (sizeof(LAN_ADDRESS) - 1 + (AddrLen) + (HWAddrLen))
#define LAN_ADDR_PTR(LA) \
  ((LA)->Address)
#define LAN_HWADDR_PTR(LA) \
  ((LA)->Address + (LA)->AddressLen)

#define LAN_EA_INFO_SIZE(NumTypes) \
  sizeof(FILE_FULL_EA_INFORMATION) + (6 + sizeof(USHORT) * NumTypes)
#define LAN_FILL_EA_INFO(Ea,NumTypes,Types) \
  { \
    RtlCopyMemory( (Ea)->EaName, "TYPES", 6 ); \
    (Ea)->EaNameLength = 6; \
    (Ea)->EaValueLength = sizeof(USHORT) * (NumTypes); \
    RtlCopyMemory( (Ea)->EaName + (Ea)->EaNameLength, \
		   (Types), \
		   sizeof(USHORT) * (NumTypes) ); \
  }

#endif/*_NET_LAN_H*/
