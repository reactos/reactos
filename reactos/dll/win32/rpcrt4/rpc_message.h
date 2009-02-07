/*
 * RPC message API
 *
 * Copyright 2004 Filip Navara
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_RPC_MESSAGE_H
#define __WINE_RPC_MESSAGE_H

#include "rpc_defs.h"

typedef unsigned int NCA_STATUS;

RpcPktHdr *RPCRT4_BuildFaultHeader(unsigned long DataRepresentation, RPC_STATUS Status);
RpcPktHdr *RPCRT4_BuildResponseHeader(unsigned long DataRepresentation, unsigned long BufferLength);
RpcPktHdr *RPCRT4_BuildBindHeader(unsigned long DataRepresentation, unsigned short MaxTransmissionSize, unsigned short MaxReceiveSize, unsigned long AssocGroupId, const RPC_SYNTAX_IDENTIFIER *AbstractId, const RPC_SYNTAX_IDENTIFIER *TransferId);
RpcPktHdr *RPCRT4_BuildBindNackHeader(unsigned long DataRepresentation, unsigned char RpcVersion, unsigned char RpcVersionMinor);
RpcPktHdr *RPCRT4_BuildBindAckHeader(unsigned long DataRepresentation, unsigned short MaxTransmissionSize, unsigned short MaxReceiveSize, unsigned long AssocGroupId, LPCSTR ServerAddress, unsigned long Result, unsigned long Reason, const RPC_SYNTAX_IDENTIFIER *TransferId);
VOID RPCRT4_FreeHeader(RpcPktHdr *Header);
RPC_STATUS RPCRT4_Send(RpcConnection *Connection, RpcPktHdr *Header, void *Buffer, unsigned int BufferLength);
RPC_STATUS RPCRT4_Receive(RpcConnection *Connection, RpcPktHdr **Header, PRPC_MESSAGE pMsg);
RPC_STATUS RPCRT4_ReceiveWithAuth(RpcConnection *Connection, RpcPktHdr **Header, PRPC_MESSAGE pMsg, unsigned char **auth_data_out, unsigned long *auth_length_out);
NCA_STATUS RPC2NCA_STATUS(RPC_STATUS status);
RPC_STATUS RPCRT4_AuthorizeConnection(RpcConnection* conn, BYTE *challenge, ULONG count);

#endif
