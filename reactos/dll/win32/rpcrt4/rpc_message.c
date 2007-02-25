/*
 * RPC messages
 *
 * Copyright 2001-2002 Ove Kåven, TransGaming Technologies
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * TODO:
 *  - figure out whether we *really* got this right
 *  - check for errors and throw exceptions
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"

#include "rpc.h"
#include "rpcndr.h"
#include "rpcdcep.h"

#include "wine/debug.h"

#include "rpc_binding.h"
#include "rpc_misc.h"
#include "rpc_defs.h"
#include "rpc_message.h"

WINE_DEFAULT_DEBUG_CHANNEL(rpc);

static DWORD RPCRT4_GetHeaderSize(RpcPktHdr *Header)
{
  static const DWORD header_sizes[] = {
    sizeof(Header->request), 0, sizeof(Header->response),
    sizeof(Header->fault), 0, 0, 0, 0, 0, 0, 0, sizeof(Header->bind),
    sizeof(Header->bind_ack), sizeof(Header->bind_nack),
    0, 0, 0, 0, 0
  };
  ULONG ret = 0;
  
  if (Header->common.ptype < sizeof(header_sizes) / sizeof(header_sizes[0])) {
    ret = header_sizes[Header->common.ptype];
    if (ret == 0)
      FIXME("unhandled packet type\n");
    if (Header->common.flags & RPC_FLG_OBJECT_UUID)
      ret += sizeof(UUID);
  } else {
    TRACE("invalid packet type\n");
  }

  return ret;
}

static VOID RPCRT4_BuildCommonHeader(RpcPktHdr *Header, unsigned char PacketType,
                              unsigned long DataRepresentation)
{
  Header->common.rpc_ver = RPC_VER_MAJOR;
  Header->common.rpc_ver_minor = RPC_VER_MINOR;
  Header->common.ptype = PacketType;
  Header->common.drep[0] = LOBYTE(LOWORD(DataRepresentation));
  Header->common.drep[1] = HIBYTE(LOWORD(DataRepresentation));
  Header->common.drep[2] = LOBYTE(HIWORD(DataRepresentation));
  Header->common.drep[3] = HIBYTE(HIWORD(DataRepresentation));
  Header->common.auth_len = 0;
  Header->common.call_id = 1;
  Header->common.flags = 0;
  /* Flags and fragment length are computed in RPCRT4_Send. */
}                              

static RpcPktHdr *RPCRT4_BuildRequestHeader(unsigned long DataRepresentation,
                                     unsigned long BufferLength,
                                     unsigned short ProcNum,
                                     UUID *ObjectUuid)
{
  RpcPktHdr *header;
  BOOL has_object;
  RPC_STATUS status;

  has_object = (ObjectUuid != NULL && !UuidIsNil(ObjectUuid, &status));
  header = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                     sizeof(header->request) + (has_object ? sizeof(UUID) : 0));
  if (header == NULL) {
    return NULL;
  }

  RPCRT4_BuildCommonHeader(header, PKT_REQUEST, DataRepresentation);
  header->common.frag_len = sizeof(header->request);
  header->request.alloc_hint = BufferLength;
  header->request.context_id = 0;
  header->request.opnum = ProcNum;
  if (has_object) {
    header->common.flags |= RPC_FLG_OBJECT_UUID;
    header->common.frag_len += sizeof(UUID);
    memcpy(&header->request + 1, ObjectUuid, sizeof(UUID));
  }

  return header;
}

static RpcPktHdr *RPCRT4_BuildResponseHeader(unsigned long DataRepresentation,
                                      unsigned long BufferLength)
{
  RpcPktHdr *header;

  header = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(header->response));
  if (header == NULL) {
    return NULL;
  }

  RPCRT4_BuildCommonHeader(header, PKT_RESPONSE, DataRepresentation);
  header->common.frag_len = sizeof(header->response);
  header->response.alloc_hint = BufferLength;

  return header;
}

RpcPktHdr *RPCRT4_BuildFaultHeader(unsigned long DataRepresentation,
                                   RPC_STATUS Status)
{
  RpcPktHdr *header;

  header = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(header->fault));
  if (header == NULL) {
    return NULL;
  }

  RPCRT4_BuildCommonHeader(header, PKT_FAULT, DataRepresentation);
  header->common.frag_len = sizeof(header->fault);
  header->fault.status = Status;

  return header;
}

RpcPktHdr *RPCRT4_BuildBindHeader(unsigned long DataRepresentation,
                                  unsigned short MaxTransmissionSize,
                                  unsigned short MaxReceiveSize,
                                  RPC_SYNTAX_IDENTIFIER *AbstractId,
                                  RPC_SYNTAX_IDENTIFIER *TransferId)
{
  RpcPktHdr *header;

  header = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(header->bind));
  if (header == NULL) {
    return NULL;
  }

  RPCRT4_BuildCommonHeader(header, PKT_BIND, DataRepresentation);
  header->common.frag_len = sizeof(header->bind);
  header->bind.max_tsize = MaxTransmissionSize;
  header->bind.max_rsize = MaxReceiveSize;
  header->bind.num_elements = 1;
  header->bind.num_syntaxes = 1;
  memcpy(&header->bind.abstract, AbstractId, sizeof(RPC_SYNTAX_IDENTIFIER));
  memcpy(&header->bind.transfer, TransferId, sizeof(RPC_SYNTAX_IDENTIFIER));

  return header;
}

RpcPktHdr *RPCRT4_BuildBindNackHeader(unsigned long DataRepresentation,
                                      unsigned char RpcVersion,
                                      unsigned char RpcVersionMinor)
{
  RpcPktHdr *header;

  header = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(header->bind_nack));
  if (header == NULL) {
    return NULL;
  }

  RPCRT4_BuildCommonHeader(header, PKT_BIND_NACK, DataRepresentation);
  header->common.frag_len = sizeof(header->bind_nack);
  header->bind_nack.protocols_count = 1;
  header->bind_nack.protocols[0].rpc_ver = RpcVersion;
  header->bind_nack.protocols[0].rpc_ver_minor = RpcVersionMinor;

  return header;
}

RpcPktHdr *RPCRT4_BuildBindAckHeader(unsigned long DataRepresentation,
                                     unsigned short MaxTransmissionSize,
                                     unsigned short MaxReceiveSize,
                                     LPSTR ServerAddress,
                                     unsigned long Result,
                                     unsigned long Reason,
                                     RPC_SYNTAX_IDENTIFIER *TransferId)
{
  RpcPktHdr *header;
  unsigned long header_size;
  RpcAddressString *server_address;
  RpcResults *results;
  RPC_SYNTAX_IDENTIFIER *transfer_id;

  header_size = sizeof(header->bind_ack) + sizeof(RpcResults) +
                sizeof(RPC_SYNTAX_IDENTIFIER) + sizeof(RpcAddressString) +
                strlen(ServerAddress);

  header = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, header_size);
  if (header == NULL) {
    return NULL;
  }

  RPCRT4_BuildCommonHeader(header, PKT_BIND_ACK, DataRepresentation);
  header->common.frag_len = header_size;
  header->bind_ack.max_tsize = MaxTransmissionSize;
  header->bind_ack.max_rsize = MaxReceiveSize;
  server_address = (RpcAddressString*)(&header->bind_ack + 1);
  server_address->length = strlen(ServerAddress) + 1;
  strcpy(server_address->string, ServerAddress);
  results = (RpcResults*)((ULONG_PTR)server_address + sizeof(RpcAddressString) + server_address->length - 1);
  results->num_results = 1;
  results->results[0].result = Result;
  results->results[0].reason = Reason;
  transfer_id = (RPC_SYNTAX_IDENTIFIER*)(results + 1);
  memcpy(transfer_id, TransferId, sizeof(RPC_SYNTAX_IDENTIFIER));

  return header;
}

VOID RPCRT4_FreeHeader(RpcPktHdr *Header)
{
  HeapFree(GetProcessHeap(), 0, Header);
}

/***********************************************************************
 *           RPCRT4_Send (internal)
 * 
 * Transmit a packet over connection in acceptable fragments.
 */
RPC_STATUS RPCRT4_Send(RpcConnection *Connection, RpcPktHdr *Header,
                       void *Buffer, unsigned int BufferLength)
{
  PUCHAR buffer_pos;
  DWORD hdr_size, count;

  buffer_pos = Buffer;
  /* The packet building functions save the packet header size, so we can use it. */
  hdr_size = Header->common.frag_len;
  Header->common.flags |= RPC_FLG_FIRST;
  Header->common.flags &= ~RPC_FLG_LAST;
  while (!(Header->common.flags & RPC_FLG_LAST)) {
    /* decide if we need to split the packet into fragments */
    if ((BufferLength + hdr_size) <= Connection->MaxTransmissionSize) {
      Header->common.flags |= RPC_FLG_LAST;
      Header->common.frag_len = BufferLength + hdr_size;
    } else {
      Header->common.frag_len = Connection->MaxTransmissionSize;
    }

    /* transmit packet header */
    if (!WriteFile(Connection->conn, Header, hdr_size, &count, &Connection->ovl[1]) &&
	ERROR_IO_PENDING != GetLastError()) {
      WARN("WriteFile failed with error %ld\n", GetLastError());
      return GetLastError();
    }
    if (!GetOverlappedResult(Connection->conn, &Connection->ovl[1], &count, TRUE)) {
      WARN("GetOverlappedResult failed with error %ld\n", GetLastError());
      return GetLastError();
    }

    /* fragment consisted of header only and is the last one */
    if (hdr_size == Header->common.frag_len &&
        Header->common.flags & RPC_FLG_LAST) {
      return RPC_S_OK;
    }

    /* send the fragment data */
    if (!WriteFile(Connection->conn, buffer_pos, Header->common.frag_len - hdr_size, &count, &Connection->ovl[1]) &&
	ERROR_IO_PENDING != GetLastError()) {
      WARN("WriteFile failed with error %ld\n", GetLastError());
      return GetLastError();
    }
    if (!GetOverlappedResult(Connection->conn, &Connection->ovl[1], &count, TRUE)) {
      WARN("GetOverlappedResult failed with error %ld\n", GetLastError());
      return GetLastError();
    }

    buffer_pos += Header->common.frag_len - hdr_size;
    BufferLength -= Header->common.frag_len - hdr_size;
    Header->common.flags &= ~RPC_FLG_FIRST;
  }

  return RPC_S_OK;
}

/***********************************************************************
 *           RPCRT4_Receive (internal)
 * 
 * Receive a packet from connection and merge the fragments.
 */
RPC_STATUS RPCRT4_Receive(RpcConnection *Connection, RpcPktHdr **Header,
                          PRPC_MESSAGE pMsg)
{
  RPC_STATUS status;
  DWORD dwRead, hdr_length;
  unsigned short first_flag;
  unsigned long data_length;
  unsigned long buffer_length;
  unsigned char *buffer_ptr;
  RpcPktCommonHdr common_hdr;

  *Header = NULL;

  TRACE("(%p, %p, %p)\n", Connection, Header, pMsg);

  /* read packet common header */
  if (!ReadFile(Connection->conn, &common_hdr, sizeof(common_hdr), &dwRead, &Connection->ovl[0]) &&
      ERROR_IO_PENDING != GetLastError()) {
    WARN("ReadFile failed with error %ld\n", GetLastError());
    status = RPC_S_PROTOCOL_ERROR;
    goto fail;
  }
  if (!GetOverlappedResult(Connection->conn, &Connection->ovl[0], &dwRead, TRUE)) {
    if (GetLastError() != ERROR_MORE_DATA) {
      WARN("GetOverlappedResult failed with error %ld\n", GetLastError());
      status = RPC_S_PROTOCOL_ERROR;
      goto fail;
    }
  }
  if (dwRead != sizeof(common_hdr)) {
    WARN("Short read of header, %ld/%d bytes\n", dwRead, sizeof(common_hdr));
    status = RPC_S_PROTOCOL_ERROR;
    goto fail;
  }

  /* verify if the header really makes sense */
  if (common_hdr.rpc_ver != RPC_VER_MAJOR ||
      common_hdr.rpc_ver_minor != RPC_VER_MINOR) {
    WARN("unhandled packet version\n");
    status = RPC_S_PROTOCOL_ERROR;
    goto fail;
  }

  hdr_length = RPCRT4_GetHeaderSize((RpcPktHdr*)&common_hdr);
  if (hdr_length == 0) {
    WARN("header length == 0\n");
    status = RPC_S_PROTOCOL_ERROR;
    goto fail;
  }

  *Header = HeapAlloc(GetProcessHeap(), 0, hdr_length);
  memcpy(*Header, &common_hdr, sizeof(common_hdr));

  /* read the rest of packet header */
  if (!ReadFile(Connection->conn, &(*Header)->common + 1,
                hdr_length - sizeof(common_hdr), &dwRead, &Connection->ovl[0]) &&
      ERROR_IO_PENDING != GetLastError()) {
    WARN("ReadFile failed with error %ld\n", GetLastError());
    status = RPC_S_PROTOCOL_ERROR;
    goto fail;
  }
  if (!GetOverlappedResult(Connection->conn, &Connection->ovl[0], &dwRead, TRUE)) {
    if (GetLastError() != ERROR_MORE_DATA) {
      WARN("GetOverlappedResult failed with error %ld\n", GetLastError());
      status = RPC_S_PROTOCOL_ERROR;
      goto fail;
    }
  }
  if (dwRead != hdr_length - sizeof(common_hdr)) {
    WARN("bad header length, %ld/%ld bytes\n", dwRead, hdr_length - sizeof(common_hdr));
    status = RPC_S_PROTOCOL_ERROR;
    goto fail;
  }


  /* read packet body */
  switch (common_hdr.ptype) {
  case PKT_RESPONSE:
    pMsg->BufferLength = (*Header)->response.alloc_hint;
    break;
  case PKT_REQUEST:
    pMsg->BufferLength = (*Header)->request.alloc_hint;
    break;
  default:
    pMsg->BufferLength = common_hdr.frag_len - hdr_length;
  }

  TRACE("buffer length = %u\n", pMsg->BufferLength);

  status = I_RpcGetBuffer(pMsg);
  if (status != RPC_S_OK) goto fail;

  first_flag = RPC_FLG_FIRST;
  buffer_length = 0;
  buffer_ptr = pMsg->Buffer;
  while (buffer_length < pMsg->BufferLength)
  {
    data_length = (*Header)->common.frag_len - hdr_length;
    if (((*Header)->common.flags & RPC_FLG_FIRST) != first_flag ||
        data_length + buffer_length > pMsg->BufferLength) {
      TRACE("invalid packet flags or buffer length\n");
      status = RPC_S_PROTOCOL_ERROR;
      goto fail;
    }

    if (data_length == 0) dwRead = 0; else {
      if (!ReadFile(Connection->conn, buffer_ptr, data_length, &dwRead, &Connection->ovl[0]) &&
	  ERROR_IO_PENDING != GetLastError()) {
        WARN("ReadFile failed with error %ld\n", GetLastError());
        status = RPC_S_PROTOCOL_ERROR;
        goto fail;
      }
      if (!GetOverlappedResult(Connection->conn, &Connection->ovl[0], &dwRead, TRUE)) {
        if (GetLastError() != ERROR_MORE_DATA) {
          WARN("GetOverlappedResult failed with error %ld\n", GetLastError());
          status = RPC_S_PROTOCOL_ERROR;
          goto fail;
        }
      }
    }
    if (dwRead != data_length) {
      WARN("bad data length, %ld/%ld\n", dwRead, data_length);
      status = RPC_S_PROTOCOL_ERROR;
      goto fail;
    }

    /* when there is no more data left, it should be the last packet */
    if (buffer_length == pMsg->BufferLength &&
        ((*Header)->common.flags & RPC_FLG_LAST) == 0) {
      WARN("no more data left, but not last packet\n");
      status = RPC_S_PROTOCOL_ERROR;
      goto fail;
    }

    buffer_length += data_length;
    if (buffer_length < pMsg->BufferLength) {
      TRACE("next header\n");

      /* read the header of next packet */
      if (!ReadFile(Connection->conn, *Header, hdr_length, &dwRead, &Connection->ovl[0]) &&
	  ERROR_IO_PENDING != GetLastError()) {
        WARN("ReadFile failed with error %ld\n", GetLastError());
        status = GetLastError();
        goto fail;
      }
      if (!GetOverlappedResult(Connection->conn, &Connection->ovl[0], &dwRead, TRUE)) {
        if (GetLastError() != ERROR_MORE_DATA) {
          WARN("GetOverlappedResult failed with error %ld\n", GetLastError());
          status = RPC_S_PROTOCOL_ERROR;
          goto fail;
        }
      }
      if (dwRead != hdr_length) {
        WARN("invalid packet header size (%ld)\n", dwRead);
        status = RPC_S_PROTOCOL_ERROR;
        goto fail;
      }

      buffer_ptr += data_length;
      first_flag = 0;
    }
  }

  /* success */
  status = RPC_S_OK;

fail:
  if (status != RPC_S_OK && *Header) {
    RPCRT4_FreeHeader(*Header);
    *Header = NULL;
  }
  return status;
}

/***********************************************************************
 *           I_RpcGetBuffer [RPCRT4.@]
 */
RPC_STATUS WINAPI I_RpcGetBuffer(PRPC_MESSAGE pMsg)
{
  TRACE("(%p): BufferLength=%d\n", pMsg, pMsg->BufferLength);
  /* FIXME: pfnAllocate? */
  pMsg->Buffer = HeapAlloc(GetProcessHeap(), 0, pMsg->BufferLength);

  TRACE("Buffer=%p\n", pMsg->Buffer);
  /* FIXME: which errors to return? */
  return pMsg->Buffer ? S_OK : E_OUTOFMEMORY;
}

/***********************************************************************
 *           I_RpcFreeBuffer [RPCRT4.@]
 */
RPC_STATUS WINAPI I_RpcFreeBuffer(PRPC_MESSAGE pMsg)
{
  TRACE("(%p) Buffer=%p\n", pMsg, pMsg->Buffer);
  /* FIXME: pfnFree? */
  HeapFree(GetProcessHeap(), 0, pMsg->Buffer);
  pMsg->Buffer = NULL;
  return S_OK;
}

/***********************************************************************
 *           I_RpcSend [RPCRT4.@]
 */
RPC_STATUS WINAPI I_RpcSend(PRPC_MESSAGE pMsg)
{
  RpcBinding* bind = (RpcBinding*)pMsg->Handle;
  RpcConnection* conn;
  RPC_CLIENT_INTERFACE* cif = NULL;
  RPC_SERVER_INTERFACE* sif = NULL;
  RPC_STATUS status;
  RpcPktHdr *hdr;

  TRACE("(%p)\n", pMsg);
  if (!bind) return RPC_S_INVALID_BINDING;

  if (bind->server) {
    sif = pMsg->RpcInterfaceInformation;
    if (!sif) return RPC_S_INTERFACE_NOT_FOUND; /* ? */
    status = RPCRT4_OpenBinding(bind, &conn, &sif->TransferSyntax,
                                &sif->InterfaceId);
  } else {
    cif = pMsg->RpcInterfaceInformation;
    if (!cif) return RPC_S_INTERFACE_NOT_FOUND; /* ? */
    status = RPCRT4_OpenBinding(bind, &conn, &cif->TransferSyntax,
                                &cif->InterfaceId);
  }

  if (status != RPC_S_OK) return status;

  if (bind->server) {
    if (pMsg->RpcFlags & WINE_RPCFLAG_EXCEPTION) {
      hdr = RPCRT4_BuildFaultHeader(pMsg->DataRepresentation,
                                    RPC_S_CALL_FAILED);
    } else {
      hdr = RPCRT4_BuildResponseHeader(pMsg->DataRepresentation,
                                       pMsg->BufferLength);
    }
  } else {
    hdr = RPCRT4_BuildRequestHeader(pMsg->DataRepresentation,
                                    pMsg->BufferLength, pMsg->ProcNum,
                                    &bind->ObjectUuid);
  }

  status = RPCRT4_Send(conn, hdr, pMsg->Buffer, pMsg->BufferLength);

  RPCRT4_FreeHeader(hdr);

  /* success */
  if (!bind->server) {
    /* save the connection, so the response can be read from it */
    pMsg->ReservedForRuntime = conn;
    return RPC_S_OK;
  }
  RPCRT4_CloseBinding(bind, conn);
  status = RPC_S_OK;

  return status;
}

/***********************************************************************
 *           I_RpcReceive [RPCRT4.@]
 */
RPC_STATUS WINAPI I_RpcReceive(PRPC_MESSAGE pMsg)
{
  RpcBinding* bind = (RpcBinding*)pMsg->Handle;
  RpcConnection* conn;
  RPC_CLIENT_INTERFACE* cif = NULL;
  RPC_SERVER_INTERFACE* sif = NULL;
  RPC_STATUS status;
  RpcPktHdr *hdr = NULL;

  TRACE("(%p)\n", pMsg);
  if (!bind) return RPC_S_INVALID_BINDING;

  if (pMsg->ReservedForRuntime) {
    conn = pMsg->ReservedForRuntime;
    pMsg->ReservedForRuntime = NULL;
  } else {
    if (bind->server) {
      sif = pMsg->RpcInterfaceInformation;
      if (!sif) return RPC_S_INTERFACE_NOT_FOUND; /* ? */
      status = RPCRT4_OpenBinding(bind, &conn, &sif->TransferSyntax,
                                  &sif->InterfaceId);
    } else {
      cif = pMsg->RpcInterfaceInformation;
      if (!cif) return RPC_S_INTERFACE_NOT_FOUND; /* ? */
      status = RPCRT4_OpenBinding(bind, &conn, &cif->TransferSyntax,
                                  &cif->InterfaceId);
    }
    if (status != RPC_S_OK) return status;
  }

  status = RPCRT4_Receive(conn, &hdr, pMsg);
  if (status != RPC_S_OK) {
    WARN("receive failed with error %lx\n", status);
    goto fail;
  }

  status = RPC_S_PROTOCOL_ERROR;

  switch (hdr->common.ptype) {
  case PKT_RESPONSE:
    if (bind->server) goto fail;
    break;
  case PKT_REQUEST:
    if (!bind->server) goto fail;
    break;
  case PKT_FAULT:
    pMsg->RpcFlags |= WINE_RPCFLAG_EXCEPTION;
    ERR ("we got fault packet with status %lx\n", hdr->fault.status);
    status = RPC_S_CALL_FAILED; /* ? */
    goto fail;
  default:
    WARN("bad packet type %d\n", hdr->common.ptype);
    goto fail;
  }

  /* success */
  status = RPC_S_OK;

fail:
  if (hdr) {
    RPCRT4_FreeHeader(hdr);
  }
  RPCRT4_CloseBinding(bind, conn);
  return status;
}

/***********************************************************************
 *           I_RpcSendReceive [RPCRT4.@]
 */
RPC_STATUS WINAPI I_RpcSendReceive(PRPC_MESSAGE pMsg)
{
  RPC_STATUS status;

  TRACE("(%p)\n", pMsg);
  status = I_RpcSend(pMsg);
  if (status == RPC_S_OK)
    status = I_RpcReceive(pMsg);
  return status;
}
