/*
 * RPC endpoint mapper
 *
 * Copyright 2002 Greg Turner
 * Copyright 2001 Ove Kåven, TransGaming Technologies
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
 *  - actually do things right
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"

#include "rpc.h"

#include "wine/debug.h"

#include "rpc_binding.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/* The "real" RPC portmapper endpoints that I know of are:
 *
 *  ncadg_ip_udp: 135
 *  ncacn_ip_tcp: 135
 *  ncacn_np: \\pipe\epmapper (?)
 *  ncalrpc: epmapper
 *
 * If the user's machine ran a DCE RPC daemon, it would
 * probably be possible to connect to it, but there are many
 * reasons not to, like:
 *  - the user probably does *not* run one, and probably
 *    shouldn't be forced to run one just for local COM
 *  - very few Unix systems use DCE RPC... if they run a RPC
 *    daemon at all, it's usually Sun RPC
 *  - DCE RPC registrations are persistent and saved on disk,
 *    while MS-RPC registrations are documented as non-persistent
 *    and stored only in RAM, and auto-destroyed when the process
 *    dies (something DCE RPC can't do)
 *
 * Of course, if the user *did* want to run a DCE RPC daemon anyway,
 * there would be interoperability advantages, like the possibility
 * of running a fully functional DCOM server using Wine...
 */

/***********************************************************************
 *             RpcEpRegisterA (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcEpRegisterA( RPC_IF_HANDLE IfSpec, RPC_BINDING_VECTOR *BindingVector,
                                  UUID_VECTOR *UuidVector, unsigned char *Annotation )
{
  RPCSS_NP_MESSAGE msg;
  RPCSS_NP_REPLY reply;
  char *vardata_payload, *vp;
  PRPC_SERVER_INTERFACE If = (PRPC_SERVER_INTERFACE)IfSpec;
  unsigned long c;
  RPC_STATUS rslt = RPC_S_OK;

  TRACE("(%p,%p,%p,%s)\n", IfSpec, BindingVector, UuidVector, debugstr_a(Annotation));
  TRACE(" ifid=%s\n", debugstr_guid(&If->InterfaceId.SyntaxGUID));
  for (c=0; c<BindingVector->Count; c++) {
    RpcBinding* bind = (RpcBinding*)(BindingVector->BindingH[c]);
    TRACE(" protseq[%ld]=%s\n", c, debugstr_a(bind->Protseq));
    TRACE(" endpoint[%ld]=%s\n", c, debugstr_a(bind->Endpoint));
  }
  if (UuidVector) {
    for (c=0; c<UuidVector->Count; c++)
      TRACE(" obj[%ld]=%s\n", c, debugstr_guid(UuidVector->Uuid[c]));
  }

  /* FIXME: Do something with annotation. */

  /* construct the message to rpcss */
  msg.message_type = RPCSS_NP_MESSAGE_TYPEID_REGISTEREPMSG;
  msg.message.registerepmsg.iface = If->InterfaceId;
  msg.message.registerepmsg.no_replace = 0;

  msg.message.registerepmsg.object_count = (UuidVector) ? UuidVector->Count : 0;
  msg.message.registerepmsg.binding_count = BindingVector->Count;

  /* calculate vardata payload size */
  msg.vardata_payload_size = msg.message.registerepmsg.object_count * sizeof(UUID);
  for (c=0; c < msg.message.registerepmsg.binding_count; c++) {
    RpcBinding *bind = (RpcBinding *)(BindingVector->BindingH[c]);
    msg.vardata_payload_size += strlen(bind->Protseq) + 1;
    msg.vardata_payload_size += strlen(bind->Endpoint) + 1;
  }

  /* allocate the payload buffer */
  vp = vardata_payload = LocalAlloc(LPTR, msg.vardata_payload_size);
  if (!vardata_payload)
    return RPC_S_OUT_OF_MEMORY;

  /* populate the payload data */
  for (c=0; c < msg.message.registerepmsg.object_count; c++) {
    CopyMemory(vp, UuidVector->Uuid[c], sizeof(UUID));
    vp += sizeof(UUID);
  }

  for (c=0; c < msg.message.registerepmsg.binding_count; c++) {
    RpcBinding *bind = (RpcBinding*)(BindingVector->BindingH[c]);
    unsigned long pslen = strlen(bind->Protseq) + 1, eplen = strlen(bind->Endpoint) + 1;
    CopyMemory(vp, bind->Protseq, pslen);
    vp += pslen;
    CopyMemory(vp, bind->Endpoint, eplen);
    vp += eplen;
  }

  /* send our request */
  if (!RPCRT4_RPCSSOnDemandCall(&msg, vardata_payload, &reply))
    rslt = RPC_S_OUT_OF_MEMORY;

  /* free the payload buffer */
  LocalFree(vardata_payload);

  return rslt;
}

/***********************************************************************
 *             RpcEpUnregister (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcEpUnregister( RPC_IF_HANDLE IfSpec, RPC_BINDING_VECTOR *BindingVector,
                                   UUID_VECTOR *UuidVector )
{
  RPCSS_NP_MESSAGE msg;
  RPCSS_NP_REPLY reply;
  char *vardata_payload, *vp;
  PRPC_SERVER_INTERFACE If = (PRPC_SERVER_INTERFACE)IfSpec;
  unsigned long c;
  RPC_STATUS rslt = RPC_S_OK;

  TRACE("(%p,%p,%p)\n", IfSpec, BindingVector, UuidVector);
  TRACE(" ifid=%s\n", debugstr_guid(&If->InterfaceId.SyntaxGUID));
  for (c=0; c<BindingVector->Count; c++) {
    RpcBinding* bind = (RpcBinding*)(BindingVector->BindingH[c]);
    TRACE(" protseq[%ld]=%s\n", c, debugstr_a(bind->Protseq));
    TRACE(" endpoint[%ld]=%s\n", c, debugstr_a(bind->Endpoint));
  }
  if (UuidVector) {
    for (c=0; c<UuidVector->Count; c++)
      TRACE(" obj[%ld]=%s\n", c, debugstr_guid(UuidVector->Uuid[c]));
  }

  /* construct the message to rpcss */
  msg.message_type = RPCSS_NP_MESSAGE_TYPEID_UNREGISTEREPMSG;
  msg.message.unregisterepmsg.iface = If->InterfaceId;

  msg.message.unregisterepmsg.object_count = (UuidVector) ? UuidVector->Count : 0;
  msg.message.unregisterepmsg.binding_count = BindingVector->Count;

  /* calculate vardata payload size */
  msg.vardata_payload_size = msg.message.unregisterepmsg.object_count * sizeof(UUID);
  for (c=0; c < msg.message.unregisterepmsg.binding_count; c++) {
    RpcBinding *bind = (RpcBinding *)(BindingVector->BindingH[c]);
    msg.vardata_payload_size += strlen(bind->Protseq) + 1;
    msg.vardata_payload_size += strlen(bind->Endpoint) + 1;
  }

  /* allocate the payload buffer */
  vp = vardata_payload = LocalAlloc(LPTR, msg.vardata_payload_size);
  if (!vardata_payload)
    return RPC_S_OUT_OF_MEMORY;

  /* populate the payload data */
  for (c=0; c < msg.message.unregisterepmsg.object_count; c++) {
    CopyMemory(vp, UuidVector->Uuid[c], sizeof(UUID));
    vp += sizeof(UUID);
  }

  for (c=0; c < msg.message.unregisterepmsg.binding_count; c++) {
    RpcBinding *bind = (RpcBinding*)(BindingVector->BindingH[c]);
    unsigned long pslen = strlen(bind->Protseq) + 1, eplen = strlen(bind->Endpoint) + 1;
    CopyMemory(vp, bind->Protseq, pslen);
    vp += pslen;
    CopyMemory(vp, bind->Endpoint, eplen);
    vp += eplen;
  }

  /* send our request */
  if (!RPCRT4_RPCSSOnDemandCall(&msg, vardata_payload, &reply))
    rslt = RPC_S_OUT_OF_MEMORY;

  /* free the payload buffer */
  LocalFree(vardata_payload);

  return rslt;
}

/***********************************************************************
 *             RpcEpResolveBinding (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcEpResolveBinding( RPC_BINDING_HANDLE Binding, RPC_IF_HANDLE IfSpec )
{
  RPCSS_NP_MESSAGE msg;
  RPCSS_NP_REPLY reply;
  PRPC_CLIENT_INTERFACE If = (PRPC_CLIENT_INTERFACE)IfSpec;
  RpcBinding* bind = (RpcBinding*)Binding;

  TRACE("(%p,%p)\n", Binding, IfSpec);
  TRACE(" protseq=%s\n", debugstr_a(bind->Protseq));
  TRACE(" obj=%s\n", debugstr_guid(&bind->ObjectUuid));
  TRACE(" ifid=%s\n", debugstr_guid(&If->InterfaceId.SyntaxGUID));

  /* FIXME: totally untested */

  /* just return for fully bound handles */
  if (bind->Endpoint && (bind->Endpoint[0] != '\0'))
    return RPC_S_OK;

  /* construct the message to rpcss */
  msg.message_type = RPCSS_NP_MESSAGE_TYPEID_RESOLVEEPMSG;
  msg.message.resolveepmsg.iface = If->InterfaceId;
  msg.message.resolveepmsg.object = bind->ObjectUuid;
 
  msg.vardata_payload_size = strlen(bind->Protseq) + 1;

  /* send the message */
  if (!RPCRT4_RPCSSOnDemandCall(&msg, bind->Protseq, &reply))
    return RPC_S_OUT_OF_MEMORY;

  /* empty-string result means not registered */
  if (reply.as_string[0] == '\0')
    return EPT_S_NOT_REGISTERED;
  
  /* otherwise we fully bind the handle & return RPC_S_OK */
  return RPCRT4_ResolveBinding(Binding, reply.as_string);
}
