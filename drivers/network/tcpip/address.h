
#pragma once

#define TCP_REQUEST_CANCEL_MODE_ABORT 1
#define TCP_REQUEST_CANCEL_MODE_CLOSE 2

#define TCP_STATE_CREATED    1  // created, unbound
#define TCP_STATE_BOUND      2  // bound, not listening or trying to connect
#define TCP_STATE_LISTENING  4  // listening, may or may not have connected clients
#define TCP_STATE_CONNECTING 8  // trying to connect as a client
#define TCP_STATE_CONNECTED  16 // connected as client

typedef struct _ADDRESS_FILE {
    LIST_ENTRY ListEntry;
    LONG RefCount;
	LONG ContextCount;
    IPPROTO Protocol;
    TDI_ADDRESS_IP Address;
    TCPIP_INSTANCE Instance;
    KSPIN_LOCK RequestLock;
    LIST_ENTRY RequestListHead;
	UCHAR TcpState;
    union
    {
        struct raw_pcb* lwip_raw_pcb;
        struct udp_pcb* lwip_udp_pcb;
		struct _TCP_CONTEXT *ConnectionContext;
    };
} ADDRESS_FILE, *PADDRESS_FILE;

typedef struct _TCP_CONTEXT {
	LIST_ENTRY ListEntry;
	PADDRESS_FILE AddressFile;
	IPPROTO Protocol;
	TDI_ADDRESS_IP RequestAddress;
	KSPIN_LOCK RequestListLock; // TODO: implement
	LIST_ENTRY RequestListHead;
	struct tcp_pcb* lwip_tcp_pcb;
} TCP_CONTEXT, *PTCP_CONTEXT;

typedef struct _TCP_REQUEST {
	LIST_ENTRY ListEntry;
	PIRP PendingIrp;
	UCHAR CancelMode;
} TCP_REQUEST, *PTCP_REQUEST;

void
TcpIpInitializeAddresses(void);

NTSTATUS
TcpIpCreateAddress(
    _Inout_ PIRP Irp,
    _In_ PTDI_ADDRESS_IP Address,
    _In_ IPPROTO Protocol
);

NTSTATUS
TcpIpCreateContext(
	_Inout_ PIRP Irp,
	_In_ PTDI_ADDRESS_IP Address,
	_In_ IPPROTO Protocol
);

NTSTATUS
TcpIpCloseAddress(
    _Inout_ ADDRESS_FILE* AddressFile
);

NTSTATUS
TcpIpCloseContext(
	_In_ PTCP_CONTEXT Context
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
	_Inout_ PIRP Irp
);

NTSTATUS
TcpIpReceive(
	_Inout_ PIRP Irp
);

NTSTATUS
TcpIpReceiveDatagram(
    _Inout_ PIRP Irp);

NTSTATUS
TcpIpSend(
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
