
#pragma once

void
TcpIpInitializeTcp(void);

BOOLEAN
AllocateTcpPort(
    _Inout_ USHORT* PortNumber,
    _In_ BOOLEAN Shared);
