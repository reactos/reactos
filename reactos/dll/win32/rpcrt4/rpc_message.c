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
  server_address = (RpcAddressString*)(&header->bind_ack + 1);
  server_address->length = strlen(ServerAddress) + 1;
  strcpy(server_address->string, ServerAddress);
  /* results is 4-byte aligned */
  results = (RpcResults*)((ULONG_PTR)server_address + ROUND_UP(FIELD_OFFSET(RpcAddressString, string[server_address->length]), 4));
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
#ifdef FIX_LATER
            sec_status = MakeSignature(&Connection->ctx, 0, &message, 0 /* FIXME */);
            if (sec_status != SEC_E_OK)
            {
                ERR("MakeSignature failed with 0x%08x\n", sec_status);
                return RPC_S_SEC_PKG_ERROR;
            }
#else
            return RPC_S_SEC_PKG_ERROR;
#endif
        }
    }
    else if (dir == SECURE_PACKET_RECEIVE)
    {
        if ((auth_hdr->auth_level == RPC_C_AUTHN_LEVEL_PKT_PRIVACY) && packet_has_body(hdr))
        {
            sec_status = DecryptMessage(&Connection->ctx, &message, 0 /* FIXME */, 0);
            if (sec_status != SEC_E_OK)
            {
                ERR("EncryptMessage failed with 0x%08x\n", sec_status);
                return RPC_S_SEC_PKG_ERROR;
            }
        }
        else if (auth_hdr->auth_level != RPC_C_AUTHN_LEVEL_NONE)
        {
#ifdef FIX_LATER
            sec_status = VerifySignature(&Connection->ctx, &message, 0 /* FIXME */, NULL);
            if (sec_status != SEC_E_OK)
            {
                ERR("VerifySignature failed with 0x%08x\n", sec_status);
                return RPC_S_SEC_PKG_ERROR;
            }
#else
            return RPC_S_SEC_PKG_ERROR;
#endif
        }
    }

    return RPC_S_OK;
}
         
/***********************************************************************
 *           RPCRT4_SendAuth (internal)
 * 
 * Transmit a packet with authorization data over connection in acceptable fragments.
 */
static RPC_STATUS RPCRT4_SendAuth(RpcConnection *Connection, RpcPktHdr *Header,
                                  void *Buffer, unsigned int BufferLength,
                                  void *Auth, unsigned int AuthLength)
{
  PUCHAR buffer_pos;
  DWORD hdr_size;
  LONG count;
  unsigned char *pkt;
  LONG alen;
  RPC_STATUS status;

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
          return status;
        }
      }
    }

write:
    count = rpcrt4_conn_write(Connection, pkt, Header->common.frag_len);
    HeapFree(GetProcessHeap(), 0, pkt);
    if (count<0) {
      WARN("rpcrt4_conn_write failed (auth)\n");
      return RPC_S_PROTOCOL_ERROR;
    }

    buffer_pos += Header->common.frag_len - hdr_size - alen - auth_pad_len;
    BufferLength -= Header->common.frag_len - hdr_size - alen - auth_pad_len;
    Header->common.flags &= ~RPC_FLG_FIRST;
  }

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

  r = InitializeSecurityContextA(&conn->AuthInfo->cred, in ? &conn->ctx : NULL,
        NULL, context_req, 0, SECURITY_NETWORK_DREP,
        in ? &inp_desc : NULL, 0, &conn->ctx, &out_desc, &conn->attr,
        &conn->exp);
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
#ifdef FIX_LATER
      r = QueryContextAttributesA(&conn->ctx, SECPKG_ATTR_SIZES, &secctx_sizes);
      if (FAILED(r))
      {
          WARN("QueryContextAttributes failed with error 0x%08x\n", r);
          goto failed;
      }
      conn->signature_auth_len = secctx_sizes.cbMaxSignature;
      conn->encryption_auth_len = secctx_sizes.cbSecurityTrailer;
#else
      goto failed;
#endif
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
static RPC_STATUS RPCRT_AuthorizeConnection(RpcConnection* conn,
                                            BYTE *challenge, ULONG count)
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

  status = RPCRT4_SendAuth(conn, resp_hdr, NULL, 0, out.pvBuffer, out.cbBuffer);

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
    return RPCRT4_SendAuth(Connection, Header, Buffer, BufferLength, NULL, 0);
  }

  /* tack on a negotiate packet */
  RPCRT4_ClientAuthorize(Connection, NULL, &out);
  r = RPCRT4_SendAuth(Connection, Header, Buffer, BufferLength, out.pvBuffer, out.cbBuffer);
  HeapFree(GetProcessHeap(), 0, out.pvBuffer);

  return r;
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
  DWORD hdr_length;
  LONG dwRead;
  unsigned short first_flag;
  unsigned long data_length;
  unsigned long buffer_length;
  unsigned long auth_length;
  unsigned char *auth_data = NULL;
  RpcPktCommonHdr common_hdr;

  *Header = NULL;

  TRACE("(%p, %p, %p)\n", Connection, Header, pMsg);

  /* read packet common header */
  dwRead = rpcrt4_conn_read(Connection, &common_hdr, sizeof(common_hdr));
  if (dwRead != sizeof(common_hdr)) {
    WARN("Short read of header, %d bytes\n", dwRead);
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
  dwRead = rpcrt4_conn_read(Connection, &(*Header)->common + 1, hdr_length - sizeof(common_hdr));
  if (dwRead != hdr_length - sizeof(common_hdr)) {
    WARN("bad header length, %d bytes, hdr_length %d\n", dwRead, hdr_length);
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
    pMsg->BufferLength = common_hdr.frag_len - hdr_length - RPC_AUTH_VERIFIER_LEN(&common_hdr);
  }

  TRACE("buffer length = %u\n", pMsg->BufferLength);

  status = I_RpcGetBuffer(pMsg);
  if (status != RPC_S_OK) goto fail;

  first_flag = RPC_FLG_FIRST;
  auth_length = common_hdr.auth_len;
  if (auth_length) {
    auth_data = HeapAlloc(GetProcessHeap(), 0, RPC_AUTH_VERIFIER_LEN(&common_hdr));
    if (!auth_data) {
      status = RPC_S_PROTOCOL_ERROR;
      goto fail;
    }
  }
  buffer_length = 0;
  while (TRUE)
  {
    unsigned int header_auth_len = RPC_AUTH_VERIFIER_LEN(&(*Header)->common);

    /* verify header fields */

    if (((*Header)->common.frag_len < hdr_length) ||
        ((*Header)->common.frag_len - hdr_length < header_auth_len)) {
      WARN("frag_len %d too small for hdr_length %d and auth_len %d\n",
        (*Header)->common.frag_len, hdr_length, header_auth_len);
      status = RPC_S_PROTOCOL_ERROR;
      goto fail;
    }

    if ((*Header)->common.auth_len != auth_length) {
      WARN("auth_len header field changed from %ld to %d\n",
        auth_length, (*Header)->common.auth_len);
      status = RPC_S_PROTOCOL_ERROR;
      goto fail;
    }

    if (((*Header)->common.flags & RPC_FLG_FIRST) != first_flag) {
      TRACE("invalid packet flags\n");
      status = RPC_S_PROTOCOL_ERROR;
      goto fail;
    }

    data_length = (*Header)->common.frag_len - hdr_length - header_auth_len;
    if (data_length + buffer_length > pMsg->BufferLength) {
      TRACE("allocation hint exceeded, new buffer length = %ld\n",
        data_length + buffer_length);
      pMsg->BufferLength = data_length + buffer_length;
      status = I_RpcReAllocateBuffer(pMsg);
      if (status != RPC_S_OK) goto fail;
    }

    if (data_length == 0) dwRead = 0; else
    dwRead = rpcrt4_conn_read(Connection,
        (unsigned char *)pMsg->Buffer + buffer_length, data_length);
    if (dwRead != data_length) {
      WARN("bad data length, %d/%ld\n", dwRead, data_length);
      status = RPC_S_PROTOCOL_ERROR;
      goto fail;
    }

    if (header_auth_len) {
      if (header_auth_len < sizeof(RpcAuthVerifier)) {
        WARN("bad auth verifier length %d\n", header_auth_len);
        status = RPC_S_PROTOCOL_ERROR;
        goto fail;
      }

      /* FIXME: we should accumulate authentication data for the bind,
       * bind_ack, alter_context and alter_context_response if necessary.
       * however, the details of how this is done is very sketchy in the
       * DCE/RPC spec. for all other packet types that have authentication
       * verifier data then it is just duplicated in all the fragments */
      dwRead = rpcrt4_conn_read(Connection, auth_data, header_auth_len);
      if (dwRead != header_auth_len) {
        WARN("bad authentication data length, %d/%d\n", dwRead,
          header_auth_len);
        status = RPC_S_PROTOCOL_ERROR;
        goto fail;
      }

      /* these packets are handled specially, not by the generic SecurePacket
       * function */
      if ((common_hdr.ptype != PKT_BIND) &&
          (common_hdr.ptype != PKT_BIND_ACK) &&
          (common_hdr.ptype != PKT_AUTH3))
        status = RPCRT4_SecurePacket(Connection, SECURE_PACKET_RECEIVE,
            *Header, hdr_length,
            (unsigned char *)pMsg->Buffer + buffer_length, data_length,
            (RpcAuthVerifier *)auth_data,
            (unsigned char *)auth_data + sizeof(RpcAuthVerifier),
            header_auth_len - sizeof(RpcAuthVerifier));
    }

    buffer_length += data_length;
    if (!((*Header)->common.flags & RPC_FLG_LAST)) {
      TRACE("next header\n");

      /* read the header of next packet */
      dwRead = rpcrt4_conn_read(Connection, *Header, hdr_length);
      if (dwRead != hdr_length) {
        WARN("invalid packet header size (%d)\n", dwRead);
        status = RPC_S_PROTOCOL_ERROR;
        goto fail;
      }

      first_flag = 0;
    } else {
      break;
    }
  }
  pMsg->BufferLength = buffer_length;

  /* respond to authorization request */
  if (common_hdr.ptype == PKT_BIND_ACK && auth_length > sizeof(RpcAuthVerifier))
  {
    status = RPCRT_AuthorizeConnection(Connection,
                                       auth_data + sizeof(RpcAuthVerifier),
                                       auth_length);
    if (status)
        goto fail;
  }

  /* success */
  status = RPC_S_OK;

fail:
  if (status != RPC_S_OK) {
    RPCRT4_FreeHeader(*Header);
    *Header = NULL;
  }
  HeapFree(GetProcessHeap(), 0, auth_data);
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
  TRACE("(%p): BufferLength=%d\n", pMsg, pMsg->BufferLength);
  /* FIXME: pfnAllocate? */
  pMsg->Buffer = HeapAlloc(GetProcessHeap(), 0, pMsg->BufferLength);

  TRACE("Buffer=%p\n", pMsg->Buffer);
  /* FIXME: which errors to return? */
  return pMsg->Buffer ? S_OK : E_OUTOFMEMORY;
}

/***********************************************************************
 *           I_RpcReAllocateBuffer (internal)
 */
static RPC_STATUS I_RpcReAllocateBuffer(PRPC_MESSAGE pMsg)
{
  TRACE("(%p): BufferLength=%d\n", pMsg, pMsg->BufferLength);
  pMsg->Buffer = HeapReAlloc(GetProcessHeap(), 0, pMsg->Buffer, pMsg->BufferLength);

  TRACE("Buffer=%p\n", pMsg->Buffer);
  return pMsg->Buffer ? RPC_S_OK : RPC_S_OUT_OF_RESOURCES;
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
  TRACE("(%p) Buffer=%p\n", pMsg, pMsg->Buffer);
  /* FIXME: pfnFree? */
  HeapFree(GetProcessHeap(), 0, pMsg->Buffer);
  pMsg->Buffer = NULL;
  return S_OK;
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

    if (!bind->Endpoint || !bind->Endpoint[0])
    {
      TRACE("automatically resolving partially bound binding\n");
      status = RpcEpResolveBinding(bind, cif);
      if (status != RPC_S_OK) return status;
    }

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
    hdr->common.call_id = conn->NextCallId++;
  }

  status = RPCRT4_Send(conn, hdr, pMsg->Buffer, pMsg->BufferLength);

  RPCRT4_FreeHeader(hdr);

  /* success */
  if (!bind->server) {
    /* save the connection, so the response can be read from it */
    pMsg->ReservedForRuntime = conn;
    return status;
  }
  RPCRT4_CloseBinding(bind, conn);

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

      if (!bind->Endpoint || !bind->Endpoint[0])
      {
        TRACE("automatically resolving partially bound binding\n");
        status = RpcEpResolveBinding(bind, cif);
        if (status != RPC_S_OK) return status;
      }

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
    ERR ("we got fault packet with status 0x%lx\n", hdr->fault.status);
    status = hdr->fault.status; /* FIXME: do translation from nca error codes */
    goto fail;
  default:
    WARN("bad packet type %d\n", hdr->common.ptype);
    goto fail;
  }

  /* success */
  status = RPC_S_OK;

fail:
  RPCRT4_FreeHeader(hdr);
  RPCRT4_CloseBinding(bind, conn);
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
  RPC_MESSAGE original_message;

  TRACE("(%p)\n", pMsg);

  original_message = *pMsg;
  status = I_RpcSend(pMsg);
  if (status == RPC_S_OK)
    status = I_RpcReceive(pMsg);
  /* free the buffer replaced by a new buffer in I_RpcReceive */
  if (status == RPC_S_OK)
    I_RpcFreeBuffer(&original_message);
  return status;
}
