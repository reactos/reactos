/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/cinfo.c
 * PURPOSE:     Per-socket connection information.
 * PROGRAMMER:  Jérôme Gardou
 */

#include "ntdef.h"
#include "precomp.h"

typedef struct tcp_keepalive {
  ULONG onoff;
  ULONG keepalivetime;
  ULONG keepaliveinterval;
} TCP_KEEPALIVE;

TDI_STATUS SetConnectionInfo(TDIObjectID *ID,
                             PCONNECTION_ENDPOINT Connection,
                             PVOID Buffer,
                             UINT BufferSize)
{
    NTSTATUS Status;
    ASSERT(ID->toi_type == INFO_TYPE_CONNECTION);
    switch (ID->toi_id)
    {
        case TCP_SOCKET_NODELAY:
        {
            BOOLEAN Set;
            if (BufferSize < sizeof(BOOLEAN))
                return TDI_INVALID_PARAMETER;
            Set = *(BOOLEAN*)Buffer;
            return TCPSetNoDelay(Connection, Set);
        }
        case TCP_SOCKET_KEEPALIVE:
        {
            DWORD Set;
            if (BufferSize < sizeof(DWORD))
                return TDI_INVALID_PARAMETER;
            Set = *(DWORD*)Buffer;
            return TCPSetKeepAlive(Connection, Set);
        }
        case TCP_SOCKET_KEEPALIVEVALS:
        {
            TCP_KEEPALIVE Set;
            if (BufferSize < sizeof(TCP_KEEPALIVE))
                return TDI_INVALID_PARAMETER;
            Set = *(TCP_KEEPALIVE*)Buffer;
            Status = TCPSetKeepAlive(Connection, Set.onoff);
            if (!NT_SUCCESS(Status))
                return Status;
            
            return TCPSetKeepAliveValues(Connection, Set.keepalivetime, Set.keepaliveinterval);
        }
        default:
            DbgPrint("TCPIP: Unknown connection info ID: %u.\n", ID->toi_id);
    }

    return TDI_INVALID_PARAMETER;
}
