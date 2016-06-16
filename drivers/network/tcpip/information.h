
#pragma once

NTSTATUS
TcpIpQueryInformation(
    _In_ PIRP Irp
);

NTSTATUS
TcpIpQueryKernelInformation(
    _In_ PIRP Irp
);

NTSTATUS
TcpIpSetInformation(
    _Inout_ PIRP Irp
);
