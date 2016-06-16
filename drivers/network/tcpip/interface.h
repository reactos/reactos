
#pragma once

#define TAG_INTERFACE 'fIpI'

#define TCPIP_PACKETPOOL_SIZE 200
#define TCPIP_BUFFERPOOL_SIZE 800

typedef struct
{
    struct netif lwip_netif;
    TCPIP_INSTANCE IfInstance;
    TCPIP_INSTANCE AtInstance;
    TCPIP_INSTANCE ClNlInstance;
    UNICODE_STRING DeviceName;
    NDIS_HANDLE NdisContext;
    NDIS_HANDLE PacketPool;
    NDIS_HANDLE BufferPool;
    UINT MediumIndex;
    ULONG Speed;
} TCPIP_INTERFACE;

extern TCPIP_INTERFACE* LoopbackInterface;

NTSTATUS
TcpIpCreateLoopbackInterface(void);

NTSTATUS
QueryInterfaceEntry(
    _In_ TDIEntityID ID,
    _In_ PVOID Context,
    _Out_opt_ PVOID OutBuffer,
    _Inout_ ULONG* BufferSize);

NTSTATUS
QueryInterfaceAddrTable(
    _In_ TDIEntityID ID,
    _In_ PVOID Context,
    _Out_opt_ PVOID OutBuffer,
    _Inout_ ULONG* BufferSize);
