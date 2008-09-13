/*
 * RPC messages
 *
 * Copyright 2001-2002 Ove Kåven, TransGaming Technologies
 * Copyright 2004 Filip Navara
 * Copyright 2006 CodeWeavers
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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winuser.h"

#include "rpc.h"
#include "rpcndr.h"
#include "rpcdcep.h"

#include "wine/debug.h"

#include "rpc_binding.h"
#include "rpc_defs.h"
#include "rpc_message.h"
#include "ncastatus.h"

WINE_DEFAULT_DEBUG_CHANNEL(rpc);

/* note: the DCE/RPC spec says the alignment amount should be 4, but
 * MS/RPC servers seem to always use 16 */
#define AUTH_ALIGNMENT 16

/* gets the amount needed to round a value up to the specified alignment */
#define ROUND_UP_AMOUNT(value, alignment) \
    (((alignment) - (((value) % (alignment)))) % (alignment))
#define ROUND_UP(value, alignment) (((value) + ((alignment) - 1)) & ~((alignment)-1))

enum secure_packet_direction
{
  SECURE_PACKET_SEND,
  SECURE_PACKET_RECEIVE
};

static RPC_STATUS I_RpcReAllocateBuffer(PRPC_MESSAGE pMsg);

static DWORD RPCRT4_GetHeaderSize(const RpcPktHdr *Header)
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

static int packet_has_body(const RpcPktHdr *Header)
{
    return (Header->common.ptype == PKT_FAULT) ||
           (Header->common.ptype == PKT_REQUEST) ||
           (Header->common.ptype == PKT_RESPONSE);
}

static int packet_has_auth_verifier(const RpcPktHdr *Header)
{
    return !(Header->common.ptype == PKT_BIND_NACK) &&
           !(Header->common.ptype == PKT_SHUTDOWN);
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

RpcPktHdr *RPCRT4_BuildResponseHeader(unsigned long DataRepresentation,
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
                                  unsigned long  AssocGroupId,
                                  const RPC_SYNTAX_IDENTIFIER *AbstractId,
                                  const RPC_SYNTAX_IDENTIFIER *TransferId)
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
  header->bind.assoc_gid = AssocGroupId;
  header->bind.num_elements = 1;
  header->bind.num_syntaxes = 1;
  header->bind.abstract = *AbstractId;
  header->bind.transfer = *TransferId;

  return header;
}

static RpcPktHdr *RPCRT4_BuildAuthHeader(unsigned long DataRepresentation)
{
  RpcPktHdr *header;

  header = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                     sizeof(header->common) + 12);
  if (header == NULL)
    return NULL;

  RPCRT4_BuildCommonHeader(header, PKT_AUTH3, DataRepresentation);
  header->common.frag_len = 0x14;
  header->common.auth_len = 0;

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
  header->bind_nack.reject_reason = REJECT_REASON_NOT_SPECIFIED;
  header->bind_nack.protocols_count = 1;
  header->bind_nack.protocols[0].rpc_ver = RpcVersion;
  header->bind_nack.protocols[0].rpc_ver_minor = RpcVersionMinor;

  return header;
}

RpcPktHdr *RPCRT4_BuildBindAckHeader(unsigned long DataRepresentation,
                                     unsigned short MaxTransmissionSize,
                                     unsigned short MaxReceiveSize,
                                     unsigned long AssocGroupId,
                                     LPCSTR ServerAddress,
                                     unsigned long Result,
                                     unsigned long Reason,
                                     const RPC_SYNTAX_IDENTIFIER *TransferId)
{
  RpcPktHdr *header;
  unsigned long header_size;
  RpcAddressString *server_address;
  RpcResults *results;
  RPC_SYNTAX_IDENTIFIER *transfer_id;

  header_size = sizeof(header->bind_ack) +
                ROUND_UP(FIELD_OFFSET(RpcAddressString, string[strlen(ServerAddress) + 1]), 4) +
                sizeof(RpcResults) +
                sizeof(RPC_SYNTAX_IDENTIFIER);

  header = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, header_size);
  if (header == NULL) {
    return NULL;
  }

  RPCRT4_BuildCommonHeader(header, PKT_BIND_ACK, DataRepresentation);
  header->common.frag_len = header_size;
  header->bind_ack.max_tsize = MaxTransmissionSize;
  header->bind_ack.max_rsize = MaxReceiveSize;
  header->bind_ack.assoc_gid = AssocGroupId;
  server_address = (RpcAddressString*)(&header->bind_ack + 1);
  server_address->length = strlen(ServerAddress) + 1;
  strcpy(server_address->string, ServerAddress);
  /* results is 4-byte aligned */
  results = (RpcResults*)((ULONG_PTR)server_address + ROUND_UP(FIELD_OFFSET(RpcAddressString, string[server_address->length]), 4));
  results->num_results = 1;
  results->results[0].result = Result;
  results->results[0].reason = Reason;
  transfer_id = (RPC_SYNTAX_IDENTIFIER*)(results + 1);
  *transfer_id = *TransferId;

  return header;
}

VOID RPCRT4_FreeHeader(RpcPktHdr *Header)
{
  HeapFree(GetProcessHeap(), 0, Header);
}

NCA_STATUS RPC2NCA_STATUS(RPC_STATUS status)
{
    switch (status)
    {
    case ERROR_INVALID_HANDLE:              return NCA_S_FAULT_CONTEXT_MISMATCH;
    case ERROR_OUTOFMEMORY:                 return NCA_S_FAULT_REMOTE_NO_MEMORY;
    case RPC_S_NOT_LISTENING:               return NCA_S_SERVER_TOO_BUSY;
    case RPC_S_UNKNOWN_IF:                  return NCA_S_UNK_IF;
    case RPC_S_SERVER_TOO_BUSY:             return NCA_S_SERVER_TOO_BUSY;
    case RPC_S_CALL_FAILED:                 return NCA_S_FAULT_UNSPEC;
    case RPC_S_CALL_FAILED_DNE:             return NCA_S_MANAGER_NOT_ENTERED;
    case RPC_S_PROTOCOL_ERROR:              return NCA_S_PROTO_ERROR;
    case RPC_S_UNSUPPORTED_TYPE:            return NCA_S_UNSUPPORTED_TYPE;
    case RPC_S_INVALID_TAG:                 return NCA_S_FAULT_INVALID_TAG;
    case RPC_S_INVALID_BOUND:               return NCA_S_FAULT_INVALID_BOUND;
    case RPC_S_PROCNUM_OUT_OF_RANGE:        return NCA_S_OP_RNG_ERROR;
    case RPC_X_SS_HANDLES_MISMATCH:         return NCA_S_FAULT_CONTEXT_MISMATCH;
    case RPC_S_CALL_CANCELLED:              return NCA_S_FAULT_CANCEL;
    case RPC_S_COMM_FAILURE:                return NCA_S_COMM_FAILURE;
    case RPC_X_WRONG_PIPE_ORDER:            return NCA_S_FAULT_PIPE_ORDER;
    case RPC_X_PIPE_CLOSED:                 return NCA_S_FAULT_PIPE_CLOSED;
    case RPC_X_PIPE_DISCIPLINE_ERROR:       return NCA_S_FAULT_PIPE_DISCIPLINE;
    case RPC_X_PIPE_EMPTY:                  return NCA_S_FAULT_PIPE_EMPTY;
    case STATUS_FLOAT_DIVIDE_BY_ZERO:       return NCA_S_FAULT_FP_DIV_ZERO;
    case STATUS_FLOAT_INVALID_OPERATION:    return NCA_S_FAULT_FP_ERROR;
    case STATUS_FLOAT_OVERFLOW:             return NCA_S_FAULT_FP_OVERFLOW;
    case STATUS_FLOAT_UNDERFLOW:            return NCA_S_FAULT_FP_UNDERFLOW;
    case STATUS_INTEGER_DIVIDE_BY_ZERO:     return NCA_S_FAULT_INT_DIV_BY_ZERO;
    case STATUS_INTEGER_OVERFLOW:           return NCA_S_FAULT_INT_OVERFLOW;
    default:                                return status;
    }
}

RPC_STATUS NCA2RPC_STATUS(NCA_STATUS status)
{
    switch (status)
    {
    case NCA_S_COMM_FAILURE:            return RPC_S_COMM_FAILURE;
    case NCA_S_OP_RNG_ERROR:            return RPC_S_PROCNUM_OUT_OF_RANGE;
    case NCA_S_UNK_IF:                  return RPC_S_UNKNOWN_IF;
    case NCA_S_YOU_CRASHED:             return RPC_S_CALL_FAILED;
    case NCA_S_PROTO_ERROR:             return RPC_S_PROTOCOL_ERROR;
    case NCA_S_OUT_ARGS_TOO_BIG:        return ERROR_NOT_ENOUGH_SERVER_MEMORY;
    case NCA_S_SERVER_TOO_BUSY:         return RPC_S_SERVER_TOO_BUSY;
    case NCA_S_UNSUPPORTED_TYPE:        return RPC_S_UNSUPPORTED_TYPE;
    case NCA_S_FAULT_INT_DIV_BY_ZERO:   return RPC_S_ZERO_DIVIDE;
    case NCA_S_FAULT_ADDR_ERROR:        return RPC_S_ADDRESS_ERROR;
    case NCA_S_FAULT_FP_DIV_ZERO:       return RPC_S_FP_DIV_ZERO;
    case NCA_S_FAULT_FP_UNDERFLOW:      return RPC_S_FP_UNDERFLOW;
    case NCA_S_FAULT_FP_OVERFLOW:       return RPC_S_FP_OVERFLOW;
    case NCA_S_FAULT_INVALID_TAG:       return RPC_S_INVALID_TAG;
    case NCA_S_FAULT_INVALID_BOUND:     return RPC_S_INVALID_BOUND;
    case NCA_S_RPC_VERSION_MISMATCH:    return RPC_S_PROTOCOL_ERROR;
    case NCA_S_UNSPEC_REJECT:           return RPC_S_CALL_FAILED_DNE;
    case NCA_S_BAD_ACTID:               return RPC_S_CALL_FAILED_DNE;
    case NCA_S_WHO_ARE_YOU_FAILED:      return RPC_S_CALL_FAILED;
    case NCA_S_MANAGER_NOT_ENTERED:     return RPC_S_CALL_FAILED_DNE;
    case NCA_S_FAULT_CANCEL:            return RPC_S_CALL_CANCELLED;
    case NCA_S_FAULT_ILL_INST:          return RPC_S_ADDRESS_ERROR;
    case NCA_S_FAULT_FP_ERROR:          return RPC_S_FP_OVERFLOW;
    case NCA_S_FAULT_INT_OVERFLOW:      return RPC_S_ADDRESS_ERROR;
    case NCA_S_FAULT_UNSPEC:            return RPC_S_CALL_FAILED;
    case NCA_S_FAULT_PIPE_EMPTY:        return RPC_X_PIPE_EMPTY;
    case NCA_S_FAULT_PIPE_CLOSED:       return RPC_X_PIPE_CLOSED;
    case NCA_S_FAULT_PIPE_ORDER:        return RPC_X_WRONG_PIPE_ORDER;
    case NCA_S_FAULT_PIPE_DISCIPLINE:   return RPC_X_PIPE_DISCIPLINE_ERROR;
    case NCA_S_FAULT_PIPE_COMM_ERROR:   return RPC_S_COMM_FAILURE;
    case NCA_S_FAULT_PIPE_MEMORY:       return ERROR_OUTOFMEMORY;
    case NCA_S_FAULT_CONTEXT_MISMATCH:  return ERROR_INVALID_HANDLE;
    case NCA_S_FAULT_REMOTE_NO_MEMORY:  return ERROR_NOT_ENOUGH_SERVER_MEMORY;
    default:                            return status;
    }
}

static RPC_STATUS RPCRT4_SecurePacket(RpcConnection *Connection,
    enum secure_packet_direction dir,
    RpcPktHdr *hdr, unsigned int hdr_size,
    unsigned char *stub_data, unsigned int stub_data_size,
    RpcAuthVerifier *auth_hdr,
    unsigned char *auth_value, unsigned int auth_value_size)
{
    SecBufferDesc message;
    SecBuffer buffers[4];
    SECURITY_STATUS sec_status;

    message.ulVersion = SECBUFFER_VERSION;
    message.cBuffers = sizeof(buffers)/sizeof(buffers[0]);
    message.pBuffers = buffers;

    buffers[0].cbBuffer = hdr_size;
    buffers[0].BufferType = SECBUFFER_DATA|SECBUFFER_READONLY_WITH_CHECKSUM;
    buffers[0].pvBuffer = hdr;
    buffers[1].cbBuffer = stub_data_size;
    buffers[1].BufferType = SECBUFFER_DATA;
    buffers[1].pvBuffer = stub_data;
    buffers[2].cbBuffer = sizeof(*auth_hdr);
    buffers[2].BufferType = SECBUFFER_DATA|SECBUFFER_READONLY_WITH_CHECKSUM;
    buffers[2].pvBuffer = auth_hdr;
    buffers[3].cbBuffer = auth_value_size;
    buffers[3].BufferType = SECBUFFER_TOKEN;
    buffers[3].pvBuffer = auth_value;

    if (dir == SECURE_PACKET_SEND)
    {
        if ((auth_hdr->auth_level == RPC_C_AUTHN_LEVEL_PKT_PRIVACY) && packet_has_body(hdr))
        {
            sec_status = EncryptMessage(&Connection->ctx, 0, &message, 0 /* FIXME */);
            if (sec_status != SEC_E_OK)
            {
                ERR("EncryptMessage failed with 0x%08x\n", sec_status);
                return RPC_S_SEC_PKG_ERROR;
            }
        }
        else if (auth_hdr->auth_level != RPC_C_AUTHN_LEVEL_NONE)
        {
            sec_status = MakeSignature(&Connection->ctx, 0, &message, 0 /* FIXME */);
            if (sec_status != SEC_E_OK)
            {
                ERR("MakeSignature failed with 0x%08x\n", sec_status);
                return RPC_S_SEC_PKG_ERROR;
            }
        }
    }
    else if (dir == SECURE_PACKET_RECEIVE)
    {
        if ((auth_hdr->auth_level == RPC_C_AUTHN_LEVEL_PKT_PRIVACY) && packet_has_body(hdr))
        {
            sec_status = DecryptMessage(&Connection->ctx, &message, 0 /* FIXME */, 0);
            if (sec_status != SEC_E_OK)
            {
                ERR("DecryptMessage failed with 0x%08x\n", sec_status);
                return RPC_S_SEC_PKG_ERROR;
            }
        }
        else if (auth_hdr->auth_level != RPC_C_AUTHN_LEVEL_NONE)
        {
            sec_status = VerifySignature(&Connection->ctx, &message, 0 /* FIXME */, NULL);
            if (sec_status != SEC_E_OK)
            {
                ERR("VerifySignature failed with 0x%08x\n", sec_status);
                return RPC_S_SEC_PKG_ERROR;
            }
        }
    }

    return RPC_S_OK;
}
         
/***********************************************************************
 *           RPCRT4_SendWithAuth (internal)
 * 
 * Transmit a packet with authorization data over connection in acceptable fragments.
 */
static RPC_STATUS RPCRT4_SendWithAuth(RpcConnection *Connection, RpcPktHdr *Header,
                                      void *Buffer, unsigned int BufferLength,
                                      const void *Auth, unsigned int AuthLength)
{
  PUCHAR buffer_pos;
  DWORD hdr_size;
  LONG count;
  unsigned char *pkt;
  LONG alen;
  RPC_STATUS status;

  RPCRT4_SetThreadCurrentConnection(Connection);

  buffer_pos = Buffer;
  /* The packet building functions save the packet header size, so we can use it. */
  hdr_size = Header->common.frag_len;
  if (AuthLength)
    Header->common.auth_len = AuthLength;
  else if (Connection->AuthInfo && packet_has_auth_verifier(Header))
  {
    if ((Connection->AuthInfo->AuthnLevel == RPC_C_AUTHN_LEVEL_PKT_PRIVACY) && packet_has_body(Header))
      Header->common.auth_len = Connection->encryption_auth_len;
    else
      Header->common.auth_len = Connection->signature_auth_len;
  }
  else
    Header->common.auth_len = 0;
  Header->common.flags |= RPC_FLG_FIRST;
  Header->common.flags &= ~RPC_FLG_LAST;

  alen = RPC_AUTH_VERIFIER_LEN(&Header->common);

  while (!(Header->common.flags & RPC_FLG_LAST)) {
    unsigned char auth_pad_len = Header->common.auth_len ? ROUND_UP_AMOUNT(BufferLength, AUTH_ALIGNMENT) : 0;
    unsigned int pkt_size = BufferLength + hdr_size + alen + auth_pad_len;

    /* decide if we need to split the packet into fragments */
   if (pkt_size <= Connection->MaxTransmissionSize) {
     Header->common.flags |= RPC_FLG_LAST;
     Header->common.frag_len = pkt_size;
    } else {
      auth_pad_len = 0;
      /* make sure packet payload will be a multiple of 16 */
      Header->common.frag_len =
        ((Connection->MaxTransmissionSize - hdr_size - alen) & ~(AUTH_ALIGNMENT-1)) +
        hdr_size + alen;
    }

    pkt = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Header->common.frag_len);

    memcpy(pkt, Header, hdr_size);

    /* fragment consisted of header only and is the last one */
    if (hdr_size == Header->common.frag_len)
      goto write;

    memcpy(pkt + hdr_size, buffer_pos, Header->common.frag_len - hdr_size - auth_pad_len - alen);

    /* add the authorization info */
    if (Connection->AuthInfo && packet_has_auth_verifier(Header))
    {
      RpcAuthVerifier *auth_hdr = (RpcAuthVerifier *)&pkt[Header->common.frag_len - alen];

      auth_hdr->auth_type = Connection->AuthInfo->AuthnSvc;
      auth_hdr->auth_level = Connection->AuthInfo->AuthnLevel;
      auth_hdr->auth_pad_length = auth_pad_len;
      auth_hdr->auth_reserved = 0;
      /* a unique number... */
      auth_hdr->auth_context_id = (unsigned long)Connection;

      if (AuthLength)
        memcpy(auth_hdr + 1, Auth, AuthLength);
      else
      {
        status = RPCRT4_SecurePacket(Connection, SECURE_PACKET_SEND,
            (RpcPktHdr *)pkt, hdr_size,
            pkt + hdr_size, Header->common.frag_len - hdr_size - alen,
            auth_hdr,
            (unsigned char *)(auth_hdr + 1), Header->common.auth_len);
        if (status != RPC_S_OK)
        {
          HeapFree(GetProcessHeap(), 0, pkt);
          RPCRT4_SetThreadCurrentConnection(NULL);
          return status;
        }
      }
    }

write:
    count = rpcrt4_conn_write(Connection, pkt, Header->common.frag_len);
    HeapFree(GetProcessHeap(), 0, pkt);
    if (count<0) {
      WARN("rpcrt4_conn_write failed (auth)\n");
      RPCRT4_SetThreadCurrentConnection(NULL);
      return RPC_S_CALL_FAILED;
    }

    buffer_pos += Header->common.frag_len - hdr_size - alen - auth_pad_len;
    BufferLength -= Header->common.frag_len - hdr_size - alen - auth_pad_len;
    Header->common.flags &= ~RPC_FLG_FIRST;
  }

  RPCRT4_SetThreadCurrentConnection(NULL);
  return RPC_S_OK;
}

/***********************************************************************
 *           RPCRT4_ClientAuthorize (internal)
 *
 * Authorize a client connection. A NULL in param signifies a new connection.
 */
static RPC_STATUS RPCRT4_ClientAuthorize(RpcConnection *conn, SecBuffer *in,
                                         SecBuffer *out)
{
  SECURITY_STATUS r;
  SecBufferDesc out_desc;
  SecBufferDesc inp_desc;
  SecPkgContext_Sizes secctx_sizes;
  BOOL continue_needed;
  ULONG context_req = ISC_REQ_CONNECTION | ISC_REQ_USE_DCE_STYLE |
                      ISC_REQ_MUTUAL_AUTH | ISC_REQ_DELEGATE;

  if (conn->AuthInfo->AuthnLevel == RPC_C_AUTHN_LEVEL_PKT_INTEGRITY)
    context_req |= ISC_REQ_INTEGRITY;
  else if (conn->AuthInfo->AuthnLevel == RPC_C_AUTHN_LEVEL_PKT_PRIVACY)
    context_req |= ISC_REQ_CONFIDENTIALITY | ISC_REQ_INTEGRITY;

  out->BufferType = SECBUFFER_TOKEN;
  out->cbBuffer = conn->AuthInfo->cbMaxToken;
  out->pvBuffer = HeapAlloc(GetProcessHeap(), 0, out->cbBuffer);
  if (!out->pvBuffer) return ERROR_OUTOFMEMORY;

  out_desc.ulVersion = 0;
  out_desc.cBuffers = 1;
  out_desc.pBuffers = out;

  inp_desc.cBuffers = 1;
  inp_desc.pBuffers = in;
  inp_desc.ulVersion = 0;

  r = InitializeSecurityContextW(&conn->AuthInfo->cred, in ? &conn->ctx : NULL,
        in ? NULL : conn->AuthInfo->server_principal_name, context_req, 0,
        SECURITY_NETWORK_DREP, in ? &inp_desc : NULL, 0, &conn->ctx,
        &out_desc, &conn->attr, &conn->exp);
  if (FAILED(r))
  {
      WARN("InitializeSecurityContext failed with error 0x%08x\n", r);
      goto failed;
  }

  TRACE("r = 0x%08x, attr = 0x%08x\n", r, conn->attr);
  continue_needed = ((r == SEC_I_CONTINUE_NEEDED) ||
                     (r == SEC_I_COMPLETE_AND_CONTINUE));

  if ((r == SEC_I_COMPLETE_NEEDED) || (r == SEC_I_COMPLETE_AND_CONTINUE))
  {
      TRACE("complete needed\n");
      r = CompleteAuthToken(&conn->ctx, &out_desc);
      if (FAILED(r))
      {
          WARN("CompleteAuthToken failed with error 0x%08x\n", r);
          goto failed;
      }
  }

  TRACE("cbBuffer = %ld\n", out->cbBuffer);

  if (!continue_needed)
  {
      r = QueryContextAttributesA(&conn->ctx, SECPKG_ATTR_SIZES, &secctx_sizes);
      if (FAILED(r))
      {
          WARN("QueryContextAttributes failed with error 0x%08x\n", r);
          goto failed;
      }
      conn->signature_auth_len = secctx_sizes.cbMaxSignature;
      conn->encryption_auth_len = secctx_sizes.cbSecurityTrailer;
  }

  return RPC_S_OK;

failed:
  HeapFree(GetProcessHeap(), 0, out->pvBuffer);
  out->pvBuffer = NULL;
  return ERROR_ACCESS_DENIED; /* FIXME: is this correct? */
}

/***********************************************************************
 *           RPCRT4_AuthorizeBinding (internal)
 */
RPC_STATUS RPCRT4_AuthorizeConnection(RpcConnection* conn, BYTE *challenge,
                                      ULONG count)
{
  SecBuffer inp, out;
  RpcPktHdr *resp_hdr;
  RPC_STATUS status;

  TRACE("challenge %s, %d bytes\n", challenge, count);

  inp.BufferType = SECBUFFER_TOKEN;
  inp.pvBuffer = challenge;
  inp.cbBuffer = count;

  status = RPCRT4_ClientAuthorize(conn, &inp, &out);
  if (status) return status;

  resp_hdr = RPCRT4_BuildAuthHeader(NDR_LOCAL_DATA_REPRESENTATION);
  if (!resp_hdr)
    return E_OUTOFMEMORY;

  status = RPCRT4_SendWithAuth(conn, resp_hdr, NULL, 0, out.pvBuffer, out.cbBuffer);

  HeapFree(GetProcessHeap(), 0, out.pvBuffer);
  RPCRT4_FreeHeader(resp_hdr);

  return status;
}

/***********************************************************************
 *           RPCRT4_Send (internal)
 * 
 * Transmit a packet over connection in acceptable fragments.
 */
RPC_STATUS RPCRT4_Send(RpcConnection *Connection, RpcPktHdr *Header,
                       void *Buffer, unsigned int BufferLength)
{
  RPC_STATUS r;
  SecBuffer out;

  if (!Connection->AuthInfo || SecIsValidHandle(&Connection->ctx))
  {
    return RPCRT4_SendWithAuth(Connection, Header, Buffer, BufferLength, NULL, 0);
  }

  /* tack on a negotiate packet */
  r = RPCRT4_ClientAuthorize(Connection, NULL, &out);
  if (r == RPC_S_OK)
  {
    r = RPCRT4_SendWithAuth(Connection, Header, Buffer, BufferLength, out.pvBuffer, out.cbBuffer);
    HeapFree(GetProcessHeap(), 0, out.pvBuffer);
  }

  return r;
}

/* validates version and frag_len fields */
RPC_STATUS RPCRT4_ValidateCommonHeader(const RpcPktCommonHdr *hdr)
{
  DWORD hdr_length;

  /* verify if the header really makes sense */
  if (hdr->rpc_ver != RPC_VER_MAJOR ||
      hdr->rpc_ver_minor != RPC_VER_MINOR)
  {
    WARN("unhandled packet version\n");
    return RPC_S_PROTOCOL_ERROR;
  }

  hdr_length = RPCRT4_GetHeaderSize((const RpcPktHdr*)hdr);
  if (hdr_length == 0)
  {
    WARN("header length == 0\n");
    return RPC_S_PROTOCOL_ERROR;
  }

  if (hdr->frag_len < hdr_length)
  {
    WARN("bad frag length %d\n", hdr->frag_len);
    return RPC_S_PROTOCOL_ERROR;
  }

  return RPC_S_OK;
}

/***********************************************************************
 *           RPCRT4_receive_fragment (internal)
 * 
 * Receive a fragment from a connection.
 */
RPC_STATUS RPCRT4_receive_fragment(RpcConnection *Connection, RpcPktHdr **Header, void **Payload)
{
  RPC_STATUS status;
  DWORD hdr_length;
  LONG dwRead;
  RpcPktCommonHdr common_hdr;

  *Header = NULL;
  *Payload = NULL;

  TRACE("(%p, %p, %p)\n", Connection, Header, Payload);

  /* read packet common header */
  dwRead = rpcrt4_conn_read(Connection, &common_hdr, sizeof(common_hdr));
  if (dwRead != sizeof(common_hdr)) {
    WARN("Short read of header, %d bytes\n", dwRead);
    status = RPC_S_CALL_FAILED;
    goto fail;
  }

  status = RPCRT4_ValidateCommonHeader(&common_hdr);
  if (status != RPC_S_OK) goto fail;

  hdr_length = RPCRT4_GetHeaderSize((RpcPktHdr*)&common_hdr);
  if (hdr_length == 0) {
    WARN("header length == 0\n");
    status = RPC_S_PROTOCOL_ERROR;
    goto fail;
  }

  *Header = HeapAlloc(GetProcessHeap(), 0, hdr_length);
  memcpy(*Header, &common_hdr, sizeof(common_hdr));

  /* read the rest of packet header */
  dwRead = rpcrt4_conn_read(Connection, &(*Header)->common + 1, hdr_length - sizeof(common_hdr));
  if (dwRead != hdr_length - sizeof(common_hdr)) {
    WARN("bad header length, %d bytes, hdr_length %d\n", dwRead, hdr_length);
    status = RPC_S_CALL_FAILED;
    goto fail;
  }

  if (common_hdr.frag_len - hdr_length)
  {
    *Payload = HeapAlloc(GetProcessHeap(), 0, common_hdr.frag_len - hdr_length);
    if (!*Payload)
    {
      status = RPC_S_OUT_OF_RESOURCES;
      goto fail;
    }

    dwRead = rpcrt4_conn_read(Connection, *Payload, common_hdr.frag_len - hdr_length);
    if (dwRead != common_hdr.frag_len - hdr_length)
    {
      WARN("bad data length, %d/%d\n", dwRead, common_hdr.frag_len - hdr_length);
      status = RPC_S_CALL_FAILED;
      goto fail;
    }
  }
  else
    *Payload = NULL;

  /* success */
  status = RPC_S_OK;

fail:
  if (status != RPC_S_OK) {
    RPCRT4_FreeHeader(*Header);
    *Header = NULL;
    HeapFree(GetProcessHeap(), 0, *Payload);
    *Payload = NULL;
  }
  return status;
}

/***********************************************************************
 *           RPCRT4_ReceiveWithAuth (internal)
 *
 * Receive a packet from connection, merge the fragments and return the auth
 * data.
 */
RPC_STATUS RPCRT4_ReceiveWithAuth(RpcConnection *Connection, RpcPktHdr **Header,
                                  PRPC_MESSAGE pMsg,
                                  unsigned char **auth_data_out,
                                  unsigned long *auth_length_out)
{
  RPC_STATUS status;
  DWORD hdr_length;
  unsigned short first_flag;
  unsigned long data_length;
  unsigned long buffer_length;
  unsigned long auth_length = 0;
  unsigned char *auth_data = NULL;
  RpcPktHdr *CurrentHeader = NULL;
  void *payload = NULL;

  *Header = NULL;
  pMsg->Buffer = NULL;

  TRACE("(%p, %p, %p, %p)\n", Connection, Header, pMsg, auth_data_out);

  RPCRT4_SetThreadCurrentConnection(Connection);

  status = RPCRT4_receive_fragment(Connection, Header, &payload);
  if (status != RPC_S_OK) goto fail;

  hdr_length = RPCRT4_GetHeaderSize(*Header);

  /* read packet body */
  switch ((*Header)->common.ptype) {
  case PKT_RESPONSE:
    pMsg->BufferLength = (*Header)->response.alloc_hint;
    break;
  case PKT_REQUEST:
    pMsg->BufferLength = (*Header)->request.alloc_hint;
    break;
  default:
    pMsg->BufferLength = (*Header)->common.frag_len - hdr_length - RPC_AUTH_VERIFIER_LEN(&(*Header)->common);
  }

  TRACE("buffer length = %u\n", pMsg->BufferLength);

  pMsg->Buffer = I_RpcAllocate(pMsg->BufferLength);
  if (!pMsg->Buffer)
  {
    status = ERROR_OUTOFMEMORY;
    goto fail;
  }

  first_flag = RPC_FLG_FIRST;
  auth_length = (*Header)->common.auth_len;
  if (auth_length) {
    auth_data = HeapAlloc(GetProcessHeap(), 0, RPC_AUTH_VERIFIER_LEN(&(*Header)->common));
    if (!auth_data) {
      status = RPC_S_OUT_OF_RESOURCES;
      goto fail;
    }
  }
  CurrentHeader = *Header;
  buffer_length = 0;
  while (TRUE)
  {
    unsigned int header_auth_len = RPC_AUTH_VERIFIER_LEN(&CurrentHeader->common);

    /* verify header fields */

    if ((CurrentHeader->common.frag_len < hdr_length) ||
        (CurrentHeader->common.frag_len - hdr_length < header_auth_len)) {
      WARN("frag_len %d too small for hdr_length %d and auth_len %d\n",
        CurrentHeader->common.frag_len, hdr_length, CurrentHeader->common.auth_len);
      status = RPC_S_PROTOCOL_ERROR;
      goto fail;
    }

    if (CurrentHeader->common.auth_len != auth_length) {
      WARN("auth_len header field changed from %ld to %d\n",
        auth_length, CurrentHeader->common.auth_len);
      status = RPC_S_PROTOCOL_ERROR;
      goto fail;
    }

    if ((CurrentHeader->common.flags & RPC_FLG_FIRST) != first_flag) {
      TRACE("invalid packet flags\n");
      status = RPC_S_PROTOCOL_ERROR;
      goto fail;
    }

    data_length = CurrentHeader->common.frag_len - hdr_length - header_auth_len;
    if (data_length + buffer_length > pMsg->BufferLength) {
      TRACE("allocation hint exceeded, new buffer length = %ld\n",
        data_length + buffer_length);
      pMsg->BufferLength = data_length + buffer_length;
      status = I_RpcReAllocateBuffer(pMsg);
      if (status != RPC_S_OK) goto fail;
    }

    memcpy((unsigned char *)pMsg->Buffer + buffer_length, payload, data_length);

    if (header_auth_len) {
      if (header_auth_len < sizeof(RpcAuthVerifier) ||
          header_auth_len > RPC_AUTH_VERIFIER_LEN(&(*Header)->common)) {
        WARN("bad auth verifier length %d\n", header_auth_len);
        status = RPC_S_PROTOCOL_ERROR;
        goto fail;
      }

      /* FIXME: we should accumulate authentication data for the bind,
       * bind_ack, alter_context and alter_context_response if necessary.
       * however, the details of how this is done is very sketchy in the
       * DCE/RPC spec. for all other packet types that have authentication
       * verifier data then it is just duplicated in all the fragments */
      memcpy(auth_data, (unsigned char *)payload + data_length, header_auth_len);

      /* these packets are handled specially, not by the generic SecurePacket
       * function */
      if (!auth_data_out && SecIsValidHandle(&Connection->ctx))
      {
        status = RPCRT4_SecurePacket(Connection, SECURE_PACKET_RECEIVE,
            CurrentHeader, hdr_length,
            (unsigned char *)pMsg->Buffer + buffer_length, data_length,
            (RpcAuthVerifier *)auth_data,
            auth_data + sizeof(RpcAuthVerifier),
            header_auth_len - sizeof(RpcAuthVerifier));
        if (status != RPC_S_OK) goto fail;
      }
    }

    buffer_length += data_length;
    if (!(CurrentHeader->common.flags & RPC_FLG_LAST)) {
      TRACE("next header\n");

      if (*Header != CurrentHeader)
      {
          RPCRT4_FreeHeader(CurrentHeader);
          CurrentHeader = NULL;
      }
      HeapFree(GetProcessHeap(), 0, payload);
      payload = NULL;

      status = RPCRT4_receive_fragment(Connection, &CurrentHeader, &payload);
      if (status != RPC_S_OK) goto fail;

      first_flag = 0;
    } else {
      break;
    }
  }
  pMsg->BufferLength = buffer_length;

  /* success */
  status = RPC_S_OK;

fail:
  RPCRT4_SetThreadCurrentConnection(NULL);
  if (CurrentHeader != *Header)
    RPCRT4_FreeHeader(CurrentHeader);
  if (status != RPC_S_OK) {
    I_RpcFree(pMsg->Buffer);
    pMsg->Buffer = NULL;
    RPCRT4_FreeHeader(*Header);
    *Header = NULL;
  }
  if (auth_data_out && status == RPC_S_OK) {
    *auth_length_out = auth_length;
    *auth_data_out = auth_data;
  }
  else
    HeapFree(GetProcessHeap(), 0, auth_data);
  HeapFree(GetProcessHeap(), 0, payload);
  return status;
}

/***********************************************************************
 *           RPCRT4_Receive (internal)
 *
 * Receive a packet from connection and merge the fragments.
 */
RPC_STATUS RPCRT4_Receive(RpcConnection *Connection, RpcPktHdr **Header,
                          PRPC_MESSAGE pMsg)
{
    return RPCRT4_ReceiveWithAuth(Connection, Header, pMsg, NULL, NULL);
}

/***********************************************************************
 *           I_RpcNegotiateTransferSyntax [RPCRT4.@]
 *
 * Negotiates the transfer syntax used by a client connection by connecting
 * to the server.
 *
 * PARAMS
 *  pMsg   [I] RPC Message structure.
 *  pAsync [I] Asynchronous state to set.
 *
 * RETURNS
 *  Success: RPC_S_OK.
 *  Failure: Any error code.
 */
RPC_STATUS WINAPI I_RpcNegotiateTransferSyntax(PRPC_MESSAGE pMsg)
{
  RpcBinding* bind = (RpcBinding*)pMsg->Handle;
  RpcConnection* conn;
  RPC_STATUS status = RPC_S_OK;

  TRACE("(%p)\n", pMsg);

  if (!bind || bind->server)
    return RPC_S_INVALID_BINDING;

  /* if we already have a connection, we don't need to negotiate again */
  if (!pMsg->ReservedForRuntime)
  {
    RPC_CLIENT_INTERFACE *cif = pMsg->RpcInterfaceInformation;
    if (!cif) return RPC_S_INTERFACE_NOT_FOUND;

    if (!bind->Endpoint || !bind->Endpoint[0])
    {
      TRACE("automatically resolving partially bound binding\n");
      status = RpcEpResolveBinding(bind, cif);
      if (status != RPC_S_OK) return status;
    }

    status = RPCRT4_OpenBinding(bind, &conn, &cif->TransferSyntax,
                                &cif->InterfaceId);

    if (status == RPC_S_OK)
    {
      pMsg->ReservedForRuntime = conn;
      RPCRT4_AddRefBinding(bind);
    }
  }

  return status;
}

/***********************************************************************
 *           I_RpcGetBuffer [RPCRT4.@]
 *
 * Allocates a buffer for use by I_RpcSend or I_RpcSendReceive and binds to the
 * server interface.
 *
 * PARAMS
 *  pMsg [I/O] RPC message information.
 *
 * RETURNS
 *  Success: RPC_S_OK.
 *  Failure: RPC_S_INVALID_BINDING if pMsg->Handle is invalid.
 *           RPC_S_SERVER_UNAVAILABLE if unable to connect to server.
 *           ERROR_OUTOFMEMORY if buffer allocation failed.
 *
 * NOTES
 *  The pMsg->BufferLength field determines the size of the buffer to allocate,
 *  in bytes.
 *
 *  Use I_RpcFreeBuffer() to unbind from the server and free the message buffer.
 *
 * SEE ALSO
 *  I_RpcFreeBuffer(), I_RpcSend(), I_RpcReceive(), I_RpcSendReceive().
 */
RPC_STATUS WINAPI I_RpcGetBuffer(PRPC_MESSAGE pMsg)
{
  RPC_STATUS status;
  RpcBinding* bind = (RpcBinding*)pMsg->Handle;

  TRACE("(%p): BufferLength=%d\n", pMsg, pMsg->BufferLength);

  if (!bind)
    return RPC_S_INVALID_BINDING;

  pMsg->Buffer = I_RpcAllocate(pMsg->BufferLength);
  TRACE("Buffer=%p\n", pMsg->Buffer);

  if (!pMsg->Buffer)
    return ERROR_OUTOFMEMORY;

  if (!bind->server)
  {
    status = I_RpcNegotiateTransferSyntax(pMsg);
    if (status != RPC_S_OK)
      I_RpcFree(pMsg->Buffer);
  }
  else
    status = RPC_S_OK;

  return status;
}

/***********************************************************************
 *           I_RpcReAllocateBuffer (internal)
 */
static RPC_STATUS I_RpcReAllocateBuffer(PRPC_MESSAGE pMsg)
{
  TRACE("(%p): BufferLength=%d\n", pMsg, pMsg->BufferLength);
  pMsg->Buffer = HeapReAlloc(GetProcessHeap(), 0, pMsg->Buffer, pMsg->BufferLength);

  TRACE("Buffer=%p\n", pMsg->Buffer);
  return pMsg->Buffer ? RPC_S_OK : ERROR_OUTOFMEMORY;
}

/***********************************************************************
 *           I_RpcFreeBuffer [RPCRT4.@]
 *
 * Frees a buffer allocated by I_RpcGetBuffer or I_RpcReceive and unbinds from
 * the server interface.
 *
 * PARAMS
 *  pMsg [I/O] RPC message information.
 *
 * RETURNS
 *  RPC_S_OK.
 *
 * SEE ALSO
 *  I_RpcGetBuffer(), I_RpcReceive().
 */
RPC_STATUS WINAPI I_RpcFreeBuffer(PRPC_MESSAGE pMsg)
{
  RpcBinding* bind = (RpcBinding*)pMsg->Handle;

  TRACE("(%p) Buffer=%p\n", pMsg, pMsg->Buffer);

  if (!bind) return RPC_S_INVALID_BINDING;

  if (pMsg->ReservedForRuntime)
  {
    RpcConnection *conn = pMsg->ReservedForRuntime;
    RPCRT4_CloseBinding(bind, conn);
    RPCRT4_ReleaseBinding(bind);
    pMsg->ReservedForRuntime = NULL;
  }
  I_RpcFree(pMsg->Buffer);
  return RPC_S_OK;
}

static void CALLBACK async_apc_notifier_proc(ULONG_PTR ulParam)
{
    RPC_ASYNC_STATE *state = (RPC_ASYNC_STATE *)ulParam;
    state->u.APC.NotificationRoutine(state, NULL, state->Event);
}

static DWORD WINAPI async_notifier_proc(LPVOID p)
{
    RpcConnection *conn = p;
    RPC_ASYNC_STATE *state = conn->async_state;

    if (state && conn->ops->wait_for_incoming_data(conn) != -1)
    {
        state->Event = RpcCallComplete;
        switch (state->NotificationType)
        {
        case RpcNotificationTypeEvent:
            TRACE("RpcNotificationTypeEvent %p\n", state->u.hEvent);
            SetEvent(state->u.hEvent);
            break;
        case RpcNotificationTypeApc:
            TRACE("RpcNotificationTypeApc %p\n", state->u.APC.hThread);
            QueueUserAPC(async_apc_notifier_proc, state->u.APC.hThread, (ULONG_PTR)state);
            break;
        case RpcNotificationTypeIoc:
            TRACE("RpcNotificationTypeIoc %p, 0x%x, 0x%lx, %p\n",
                state->u.IOC.hIOPort, state->u.IOC.dwNumberOfBytesTransferred,
                state->u.IOC.dwCompletionKey, state->u.IOC.lpOverlapped);
            PostQueuedCompletionStatus(state->u.IOC.hIOPort,
                state->u.IOC.dwNumberOfBytesTransferred,
                state->u.IOC.dwCompletionKey,
                state->u.IOC.lpOverlapped);
            break;
        case RpcNotificationTypeHwnd:
            TRACE("RpcNotificationTypeHwnd %p 0x%x\n", state->u.HWND.hWnd,
                state->u.HWND.Msg);
            PostMessageW(state->u.HWND.hWnd, state->u.HWND.Msg, 0, 0);
            break;
        case RpcNotificationTypeCallback:
            TRACE("RpcNotificationTypeCallback %p\n", state->u.NotificationRoutine);
            state->u.NotificationRoutine(state, NULL, state->Event);
            break;
        case RpcNotificationTypeNone:
            TRACE("RpcNotificationTypeNone\n");
            break;
        default:
            FIXME("unknown NotificationType: %d/0x%x\n", state->NotificationType, state->NotificationType);
            break;
        }
    }

    return 0;
}

/***********************************************************************
 *           I_RpcSend [RPCRT4.@]
 *
 * Sends a message to the server.
 *
 * PARAMS
 *  pMsg [I/O] RPC message information.
 *
 * RETURNS
 *  Unknown.
 *
 * NOTES
 *  The buffer must have been allocated with I_RpcGetBuffer().
 *
 * SEE ALSO
 *  I_RpcGetBuffer(), I_RpcReceive(), I_RpcSendReceive().
 */
RPC_STATUS WINAPI I_RpcSend(PRPC_MESSAGE pMsg)
{
  RpcBinding* bind = (RpcBinding*)pMsg->Handle;
  RpcConnection* conn;
  RPC_STATUS status;
  RpcPktHdr *hdr;

  TRACE("(%p)\n", pMsg);
  if (!bind || bind->server || !pMsg->ReservedForRuntime) return RPC_S_INVALID_BINDING;

  conn = pMsg->ReservedForRuntime;

  hdr = RPCRT4_BuildRequestHeader(pMsg->DataRepresentation,
                                  pMsg->BufferLength,
                                  pMsg->ProcNum & ~RPC_FLAGS_VALID_BIT,
                                  &bind->ObjectUuid);
  if (!hdr)
    return ERROR_OUTOFMEMORY;
  hdr->common.call_id = conn->NextCallId++;

  status = RPCRT4_Send(conn, hdr, pMsg->Buffer, pMsg->BufferLength);

  RPCRT4_FreeHeader(hdr);

  if (status == RPC_S_OK && pMsg->RpcFlags & RPC_BUFFER_ASYNC)
  {
    if (!QueueUserWorkItem(async_notifier_proc, conn, WT_EXECUTEDEFAULT | WT_EXECUTELONGFUNCTION))
        status = RPC_S_OUT_OF_RESOURCES;
  }

  return status;
}

/* is this status something that the server can't recover from? */
static inline BOOL is_hard_error(RPC_STATUS status)
{
    switch (status)
    {
    case 0: /* user-defined fault */
    case ERROR_ACCESS_DENIED:
    case ERROR_INVALID_PARAMETER:
    case RPC_S_PROTOCOL_ERROR:
    case RPC_S_CALL_FAILED:
    case RPC_S_CALL_FAILED_DNE:
    case RPC_S_SEC_PKG_ERROR:
        return TRUE;
    default:
        return FALSE;
    }
}

/***********************************************************************
 *           I_RpcReceive [RPCRT4.@]
 */
RPC_STATUS WINAPI I_RpcReceive(PRPC_MESSAGE pMsg)
{
  RpcBinding* bind = (RpcBinding*)pMsg->Handle;
  RPC_STATUS status;
  RpcPktHdr *hdr = NULL;
  RpcConnection *conn;

  TRACE("(%p)\n", pMsg);
  if (!bind || bind->server || !pMsg->ReservedForRuntime) return RPC_S_INVALID_BINDING;

  conn = pMsg->ReservedForRuntime;
  status = RPCRT4_Receive(conn, &hdr, pMsg);
  if (status != RPC_S_OK) {
    WARN("receive failed with error %lx\n", status);
    goto fail;
  }

  switch (hdr->common.ptype) {
  case PKT_RESPONSE:
    break;
  case PKT_FAULT:
    ERR ("we got fault packet with status 0x%lx\n", hdr->fault.status);
    status = NCA2RPC_STATUS(hdr->fault.status);
    if (is_hard_error(status))
        goto fail;
    break;
  default:
    WARN("bad packet type %d\n", hdr->common.ptype);
    status = RPC_S_PROTOCOL_ERROR;
    goto fail;
  }

  /* success */
  RPCRT4_FreeHeader(hdr);
  return status;

fail:
  RPCRT4_FreeHeader(hdr);
  RPCRT4_DestroyConnection(conn);
  pMsg->ReservedForRuntime = NULL;
  return status;
}

/***********************************************************************
 *           I_RpcSendReceive [RPCRT4.@]
 *
 * Sends a message to the server and receives the response.
 *
 * PARAMS
 *  pMsg [I/O] RPC message information.
 *
 * RETURNS
 *  Success: RPC_S_OK.
 *  Failure: Any error code.
 *
 * NOTES
 *  The buffer must have been allocated with I_RpcGetBuffer().
 *
 * SEE ALSO
 *  I_RpcGetBuffer(), I_RpcSend(), I_RpcReceive().
 */
RPC_STATUS WINAPI I_RpcSendReceive(PRPC_MESSAGE pMsg)
{
  RPC_STATUS status;
  void *original_buffer;

  TRACE("(%p)\n", pMsg);

  original_buffer = pMsg->Buffer;
  status = I_RpcSend(pMsg);
  if (status == RPC_S_OK)
    status = I_RpcReceive(pMsg);
  /* free the buffer replaced by a new buffer in I_RpcReceive */
  if (status == RPC_S_OK)
    I_RpcFree(original_buffer);
  return status;
}

/***********************************************************************
 *           I_RpcAsyncSetHandle [RPCRT4.@]
 *
 * Sets the asynchronous state of the handle contained in the RPC message
 * structure.
 *
 * PARAMS
 *  pMsg   [I] RPC Message structure.
 *  pAsync [I] Asynchronous state to set.
 *
 * RETURNS
 *  Success: RPC_S_OK.
 *  Failure: Any error code.
 */
RPC_STATUS WINAPI I_RpcAsyncSetHandle(PRPC_MESSAGE pMsg, PRPC_ASYNC_STATE pAsync)
{
    RpcBinding* bind = (RpcBinding*)pMsg->Handle;
    RpcConnection *conn;

    TRACE("(%p, %p)\n", pMsg, pAsync);

    if (!bind || bind->server || !pMsg->ReservedForRuntime) return RPC_S_INVALID_BINDING;

    conn = pMsg->ReservedForRuntime;
    conn->async_state = pAsync;

    return RPC_S_OK;
}

/***********************************************************************
 *           I_RpcAsyncAbortCall [RPCRT4.@]
 *
 * Aborts an asynchronous call.
 *
 * PARAMS
 *  pAsync        [I] Asynchronous state.
 *  ExceptionCode [I] Exception code.
 *
 * RETURNS
 *  Success: RPC_S_OK.
 *  Failure: Any error code.
 */
RPC_STATUS WINAPI I_RpcAsyncAbortCall(PRPC_ASYNC_STATE pAsync, ULONG ExceptionCode)
{
    FIXME("(%p, %d): stub\n", pAsync, ExceptionCode);
    return RPC_S_INVALID_ASYNC_HANDLE;
}
