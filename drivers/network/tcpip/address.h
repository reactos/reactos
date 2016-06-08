
#pragma once

typedef struct
{
    LIST_ENTRY ListEntry;
    LONG RefCount;
    IPPROTO Protocol;
    TDI_ADDRESS_IP Address;
    TCPIP_INSTANCE Instance;
    KSPIN_LOCK RequestLock;
    LIST_ENTRY RequestListHead;
    union
    {
        struct raw_pcb* lwip_raw_pcb;
        struct udp_pcb* lwip_udp_pcb;
		struct tcp_pcb* lwip_tcp_pcb;
    };
} ADDRESS_FILE;

void
TcpIpInitializeAddresses(void);

NTSTATUS
TcpIpCreateAddress(
    _Inout_ PIRP Irp,
    _In_ PTDI_ADDRESS_IP Address,
    _In_ IPPROTO Protocol
);

NTSTATUS
TcpIpCloseAddress(
    _Inout_ ADDRESS_FILE* AddressFile
);

NTSTATUS
TcpIpConnect(
	_Inout_ PIRP Irp
);

NTSTATUS
TcpIpAssociateAddress(
	_Inout_ PIRP Irp
);

NTSTATUS
TcpIpDisassociateAddress(
	_Inout_ PIRP Irp
);

NTSTATUS
TcpIpListen(
	_Inout_ PIRP Irp);

NTSTATUS
TcpIpReceiveDatagram(
    _Inout_ PIRP Irp);

NTSTATUS
TcpIpSendDatagram(
    _Inout_ PIRP Irp);

NTSTATUS
AddressSetIpDontFragment(
    _In_ TDIEntityID ID,
    _In_ PVOID InBuffer,
    _In_ ULONG BufferSize);

NTSTATUS
AddressSetTtl(
    _In_ TDIEntityID ID,
    _In_ PVOID InBuffer,
    _In_ ULONG BufferSize);
