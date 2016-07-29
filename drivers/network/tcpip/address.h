
#pragma once

#define TCP_REQUEST_CANCEL_MODE_ABORT    1
#define TCP_REQUEST_CANCEL_MODE_CLOSE    2
#define TCP_REQUEST_CANCEL_MODE_PRESERVE 3

#define TCP_REQUEST_PENDING_GENERAL             0
#define TCP_REQUEST_PENDING_SEND                1
#define TCP_REQUEST_PENDING_RECEIVE             2
#define TCP_REQUEST_PENDING_CONNECT             3
#define TCP_REQUEST_PENDING_LISTEN              4
#define TCP_REQUEST_PENDING_ACCEPTED_CONNECTION 5
#define TCP_REQUEST_PENDING_LISTEN_POLL         6

#define TCP_REQUEST_PAYLOAD_IRP     1
#define TCP_REQUEST_PAYLOAD_CONTEXT 2

// TODO: simplify state machine
#define TCP_STATE_CREATED       0x1 << 0
#define TCP_STATE_BOUND         0x1 << 1
#define TCP_STATE_LISTENING     0x1 << 2
#define TCP_STATE_RECEIVING     0x1 << 3
#define TCP_STATE_ABORTED       0x1 << 4
#define TCP_STATE_CONNECTING    0x1 << 5
#define TCP_STATE_CONNECTED     0x1 << 6
#define TCP_STATE_SENDING       0x1 << 7
#define TCP_STATE_DISASSOCIATED 0x1 << 8
#define TCP_STATE_CLOSED        0x1 << 9

struct _TCP_CONTEXT;
typedef struct _TCP_CONTEXT TCP_CONTEXT, *PTCP_CONTEXT;

typedef struct _ADDRESS_FILE {
    LIST_ENTRY ListEntry;
    LONG RefCount;
    LONG ContextCount;
    IPPROTO Protocol;
    TDI_ADDRESS_IP Address;
    struct netif *NetInterface;
    TCPIP_INSTANCE Instance;
    KSPIN_LOCK RequestLock;
    LIST_ENTRY RequestListHead;
    BOOLEAN HasListener;
    union
    {
        struct raw_pcb* lwip_raw_pcb;
        struct udp_pcb* lwip_udp_pcb;
        PTCP_CONTEXT Listener;
    };
    KSPIN_LOCK AssociatedContextsLock;
} ADDRESS_FILE, *PADDRESS_FILE;

struct _TCP_CONTEXT {
    ULONG TcpState;
    PADDRESS_FILE AddressFile;
    LIST_ENTRY RequestListHead;
    struct tcp_pcb* lwip_tcp_pcb;
    BOOLEAN CreatedWithoutRequest;
    HANDLE MutexOwner;
    volatile long int MutexDepth;
};

typedef struct _TCP_REQUEST {
    LIST_ENTRY ListEntry;
    union
    {
        PIRP PendingIrp;
        PTCP_CONTEXT Context;
    } Payload;
    UCHAR CancelMode;
    UCHAR PendingMode;
    UCHAR PayloadType;
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
