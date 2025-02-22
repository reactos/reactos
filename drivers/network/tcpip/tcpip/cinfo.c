/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/cinfo.c
 * PURPOSE:     Per-socket connection information.
 * PROGRAMMER:  Jérôme Gardou
 */

#include "precomp.h"

TDI_STATUS SetConnectionInfo(TDIObjectID *ID,
                             PCONNECTION_ENDPOINT Connection,
                             PVOID Buffer,
                             UINT BufferSize)
{
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
        default:
            DbgPrint("TCPIP: Unknown connection info ID: %u.\n", ID->toi_id);
    }

    return TDI_INVALID_PARAMETER;
}
