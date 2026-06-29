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
RpcPktHdr *RPCRT4_BuildBindNackHeader(ULONG DataRepresentation, unsigned char RpcVersion, unsigned char RpcVersionMinor, unsigned short RejectReason);
RpcPktHdr *RPCRT4_BuildBindAckHeader(ULONG DataRepresentation, unsigned short MaxTransmissionSize, unsigned short MaxReceiveSize, ULONG AssocGroupId, LPCSTR ServerAddress, unsigned char ResultCount, const RpcResult *Results);
RpcPktHdr *RPCRT4_BuildHttpHeader(ULONG DataRepresentation, unsigned short flags, unsigned short num_data_items, unsigned int payload_size);
RpcPktHdr *RPCRT4_BuildHttpConnectHeader(int out_pipe, const UUID *connection_uuid, const UUID *pipe_uuid, const UUID *association_uuid);
RpcPktHdr *RPCRT4_BuildHttpFlowControlHeader(BOOL server, ULONG bytes_transmitted, ULONG flow_control_increment, const UUID *pipe_uuid);
RPC_STATUS RPCRT4_Send(RpcConnection *Connection, RpcPktHdr *Header, void *Buffer, unsigned int BufferLength);
RPC_STATUS RPCRT4_SendWithAuth(RpcConnection *Connection, RpcPktHdr *Header, void *Buffer, unsigned int BufferLength, const void *Auth, unsigned int AuthLength);
RPC_STATUS RPCRT4_ReceiveWithAuth(RpcConnection *Connection, RpcPktHdr **Header, PRPC_MESSAGE pMsg, unsigned char **auth_data_out, ULONG *auth_length_out);
DWORD RPCRT4_GetHeaderSize(const RpcPktHdr *Header);
RPC_STATUS RPCRT4_ValidateCommonHeader(const RpcPktCommonHdr *hdr);

BOOL RPCRT4_IsValidHttpPacket(RpcPktHdr *hdr, unsigned char *data, unsigned short data_len);
RPC_STATUS RPCRT4_ParseHttpPrepareHeader1(RpcPktHdr *header, unsigned char *data, ULONG *field1);
RPC_STATUS RPCRT4_ParseHttpPrepareHeader2(RpcPktHdr *header, unsigned char *data, ULONG *field1, ULONG *bytes_until_next_packet, ULONG *field3);
RPC_STATUS RPCRT4_ParseHttpFlowControlHeader(RpcPktHdr *header, unsigned char *data, BOOL server, ULONG *bytes_transmitted, ULONG *flow_control_increment, UUID *pipe_uuid);
NCA_STATUS RPC2NCA_STATUS(RPC_STATUS status);
RPC_STATUS RPCRT4_ClientConnectionAuth(RpcConnection* conn, BYTE *challenge, ULONG count);
RPC_STATUS RPCRT4_ServerConnectionAuth(RpcConnection* conn, BOOL start, RpcAuthVerifier *auth_data_in, ULONG auth_length_in, unsigned char **auth_data_out, ULONG *auth_length_out);
RPC_STATUS RPCRT4_AuthorizeConnection(RpcConnection* conn, BYTE *challenge, ULONG count);
RPC_STATUS RPCRT4_ServerGetRegisteredAuthInfo(USHORT auth_type, CredHandle *cred, TimeStamp *exp, ULONG *max_token);
RPC_STATUS RPCRT4_default_authorize(RpcConnection *conn, BOOL first_time, unsigned char *in_buffer, unsigned int in_size, unsigned char *out_buffer, unsigned int *out_size);
BOOL RPCRT4_default_is_authorized(RpcConnection *Connection);
RPC_STATUS RPCRT4_default_secure_packet(RpcConnection *Connection, enum secure_packet_direction dir, RpcPktHdr *hdr, unsigned int hdr_size, unsigned char *stub_data, unsigned int stub_data_size, RpcAuthVerifier *auth_hdr, unsigned char *auth_value, unsigned int auth_value_size);
RPC_STATUS RPCRT4_default_impersonate_client(RpcConnection *conn);
RPC_STATUS RPCRT4_default_revert_to_self(RpcConnection *conn);
RPC_STATUS RPCRT4_default_inquire_auth_client(RpcConnection *conn, RPC_AUTHZ_HANDLE *privs, RPC_WSTR *server_princ_name, ULONG *authn_level, ULONG *authn_svc, ULONG *authz_svc, ULONG flags);

#endif
