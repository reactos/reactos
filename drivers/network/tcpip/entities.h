
#pragma once

typedef struct
{
    TDIEntityID InstanceId;
    LIST_ENTRY ListEntry;
} TCPIP_INSTANCE;

VOID
TcpIpInitializeEntities(void);

VOID
InsertEntityInstance(
    _In_ ULONG Entity,
    _Out_ TCPIP_INSTANCE* OutInstance);

void
RemoveEntityInstance(
    _In_ TCPIP_INSTANCE* Instance);

NTSTATUS
GetInstance(
    _In_ TDIEntityID,
    _Out_ TCPIP_INSTANCE** Instance);

/* IOCTL_TCP_QUERY_INFORMATION_EX handler */
NTSTATUS
QueryEntityList(
    _In_ TDIEntityID ID,
    _In_ PVOID Context,
    _Out_opt_ PVOID OutBuffer,
    _Inout_ ULONG* BufferSize);
