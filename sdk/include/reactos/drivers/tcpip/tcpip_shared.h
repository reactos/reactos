#pragma once

#define FSCTL_TCP_BASE      FILE_DEVICE_NETWORK
#define _TCP_CTL_CODE(Function, Method, Access) \
    CTL_CODE(FSCTL_TCP_BASE, Function, Method, Access)


/* IP IOCTL definitions */
#define IOCTL_ECHO_REQUEST \
    _TCP_CTL_CODE(0, METHOD_BUFFERED, FILE_ANY_ACCESS)



typedef struct _ICMP_ECHO_DATA_V4
{
    ULONG DestinationAddress;
    PVOID RequestData;
    ULONG RequestSize;
    ULONG Timeout;

    UCHAR Ttl;
    UCHAR Tos;
    UCHAR Flags;
    UCHAR OptionsSize;
    PUCHAR OptionsData;

} ICMP_ECHO_DATA_V4, *PICMP_ECHO_DATA_V4;