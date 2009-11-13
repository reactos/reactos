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

RpcPktHdr *RPCRT4_BuildFaultHeader(ULONG DataRepresentation, RPC_STATUS Status);
RpcPktHdr *RPCRT4_BuildResponseHeader(ULONG DataRepresentation, ULONG BufferLength);
RpcPktHdr *RPCRT4_BuildBindHeader(ULONG DataRepresentation, unsigned short MaxTransmissionSize, unsigned short MaxReceiveSize, ULONG AssocGroupId, const RPC_SYNTAX_IDENTIFIER *AbstractId, const RPC_SYNTAX_IDENTIFIER *TransferId);
RpcPktHdr *RPCRT4_BuildBindNackHeader(ULONG DataRepresentation, unsigned char RpcVersion, unsigned char RpcVersionMinor);
RpcPktHdr *RPCRT4_BuildBindAckHeader(ULONG DataRepresentation, unsigned short MaxTransmissionSize, unsigned short MaxReceiveSize, ULONG AssocGroupId, LPCSTR ServerAddress, unsigned short Result, unsigned short Reason, const RPC_SYNTAX_IDENTIFIER *TransferId);
RpcPktHdr *RPCRT4_BuildHttpHeader(ULONG DataRepresentation, unsigned short flags, unsigned short num_data_items, unsigned int payload_size);
RpcPktHdr *RPCRT4_BuildHttpConnectHeader(unsigned short flags, int out_pipe, const UUID *connection_uuid, const UUID *pipe_uuid, const UUID *association_uuid);
RpcPktHdr *RPCRT4_BuildHttpFlowControlHeader(BOOL server, ULONG bytes_transmitted, ULONG flow_control_increment, const UUID *pipe_uuid);
VOID RPCRT4_FreeHeader(RpcPktHdr *Header);
RPC_STATUS RPCRT4_Send(RpcConnection *Connection, RpcPktHdr *Header, void *Buffer, unsigned int BufferLength);
RPC_STATUS RPCRT4_Receive(RpcConnection *Connection, RpcPktHdr **Header, PRPC_MESSAGE pMsg);
RPC_STATUS RPCRT4_ReceiveWithAuth(RpcConnection *Connection, RpcPktHdr **Header, PRPC_MESSAGE pMsg, unsigned char **auth_data_out, ULONG *auth_length_out);
DWORD RPCRT4_GetHeaderSize(const RpcPktHdr *Header);
RPC_STATUS RPCRT4_ValidateCommonHeader(const RpcPktCommonHdr *hdr);

BOOL RPCRT4_IsValidHttpPacket(RpcPktHdr *hdr, unsigned char *data, unsigned short data_len);
RPC_STATUS RPCRT4_ParseHttpPrepareHeader1(RpcPktHdr *header, unsigned char *data, ULONG *field1);
RPC_STATUS RPCRT4_ParseHttpPrepareHeader2(RpcPktHdr *header, unsigned char *data, ULONG *field1, ULONG *bytes_until_next_packet, ULONG *field3);
RPC_STATUS RPCRT4_ParseHttpFlowControlHeader(RpcPktHdr *header, unsigned char *data, BOOL server, ULONG *bytes_transmitted, ULONG *flow_control_increment, UUID *pipe_uuid);
NCA_STATUS RPC2NCA_STATUS(RPC_STATUS status);
RPC_STATUS RPCRT4_AuthorizeConnection(RpcConnection* conn, BYTE *challenge, ULONG count);

#endif
