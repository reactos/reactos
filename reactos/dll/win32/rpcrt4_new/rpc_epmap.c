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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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

#include "rpc.h"

#include "wine/debug.h"

#include "rpc_binding.h"
#include "epm_towers.h"

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
                                  UUID_VECTOR *UuidVector, RPC_CSTR Annotation )
{
  RPCSS_NP_MESSAGE msg;
  RPCSS_NP_REPLY reply;
  char *vardata_payload, *vp;
  PRPC_SERVER_INTERFACE If = (PRPC_SERVER_INTERFACE)IfSpec;
  unsigned long c;
  RPC_STATUS rslt = RPC_S_OK;

  TRACE("(%p,%p,%p,%s)\n", IfSpec, BindingVector, UuidVector, debugstr_a((char*)Annotation));
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

typedef unsigned int unsigned32;
typedef struct twr_t
    {
    unsigned32 tower_length;
    /* [size_is] */ BYTE tower_octet_string[ 1 ];
    } 	twr_t;

/***********************************************************************
 *             TowerExplode (RPCRT4.@)
 */
RPC_STATUS WINAPI TowerExplode(
    const twr_t *tower, PRPC_SYNTAX_IDENTIFIER object, PRPC_SYNTAX_IDENTIFIER syntax,
    char **protseq, char **endpoint, char **address)
{
    size_t tower_size;
    RPC_STATUS status;
    const unsigned char *p;
    u_int16 floor_count;
    const twr_uuid_floor_t *object_floor;
    const twr_uuid_floor_t *syntax_floor;

    if (protseq)
        *protseq = NULL;
    if (endpoint)
        *endpoint = NULL;
    if (address)
        *address = NULL;

    tower_size = tower->tower_length;

    if (tower_size < sizeof(u_int16))
        return EPT_S_NOT_REGISTERED;

    p = &tower->tower_octet_string[0];

    floor_count = *(const u_int16 *)p;
    p += sizeof(u_int16);
    tower_size -= sizeof(u_int16);
    TRACE("floor_count: %d\n", floor_count);
    /* FIXME: should we do something with the floor count? at the moment we don't */

    if (tower_size < sizeof(*object_floor) + sizeof(*syntax_floor))
        return EPT_S_NOT_REGISTERED;

    object_floor = (const twr_uuid_floor_t *)p;
    p += sizeof(*object_floor);
    tower_size -= sizeof(*object_floor);
    syntax_floor = (const twr_uuid_floor_t *)p;
    p += sizeof(*syntax_floor);
    tower_size -= sizeof(*syntax_floor);

    if ((object_floor->count_lhs != sizeof(object_floor->protid) +
        sizeof(object_floor->uuid) + sizeof(object_floor->major_version)) ||
        (object_floor->protid != EPM_PROTOCOL_UUID) ||
        (object_floor->count_rhs != sizeof(object_floor->minor_version)))
        return EPT_S_NOT_REGISTERED;

    if ((syntax_floor->count_lhs != sizeof(syntax_floor->protid) +
        sizeof(syntax_floor->uuid) + sizeof(syntax_floor->major_version)) ||
        (syntax_floor->protid != EPM_PROTOCOL_UUID) ||
        (syntax_floor->count_rhs != sizeof(syntax_floor->minor_version)))
        return EPT_S_NOT_REGISTERED;

    status = RpcTransport_ParseTopOfTower(p, tower_size, protseq, address, endpoint);
    if ((status == RPC_S_OK) && syntax && object)
    {
        syntax->SyntaxGUID = syntax_floor->uuid;
        syntax->SyntaxVersion.MajorVersion = syntax_floor->major_version;
        syntax->SyntaxVersion.MinorVersion = syntax_floor->minor_version;
        object->SyntaxGUID = object_floor->uuid;
        object->SyntaxVersion.MajorVersion = object_floor->major_version;
        object->SyntaxVersion.MinorVersion = object_floor->minor_version;
    }
    return status;
}

/***********************************************************************
 *             TowerConstruct (RPCRT4.@)
 */
RPC_STATUS WINAPI TowerConstruct(
    const RPC_SYNTAX_IDENTIFIER *object, const RPC_SYNTAX_IDENTIFIER *syntax,
    const char *protseq, const char *endpoint, const char *address,
    twr_t **tower)
{
    size_t tower_size;
    RPC_STATUS status;
    unsigned char *p;
    twr_uuid_floor_t *object_floor;
    twr_uuid_floor_t *syntax_floor;

    *tower = NULL;

    status = RpcTransport_GetTopOfTower(NULL, &tower_size, protseq, address, endpoint);

    if (status != RPC_S_OK)
        return status;

    tower_size += sizeof(u_int16) + sizeof(*object_floor) + sizeof(*syntax_floor);
    *tower = I_RpcAllocate(FIELD_OFFSET(twr_t, tower_octet_string[tower_size]));
    if (!*tower)
        return RPC_S_OUT_OF_RESOURCES;

    (*tower)->tower_length = tower_size;
    p = &(*tower)->tower_octet_string[0];
    *(u_int16 *)p = 5; /* number of floors */
    p += sizeof(u_int16);
    object_floor = (twr_uuid_floor_t *)p;
    p += sizeof(*object_floor);
    syntax_floor = (twr_uuid_floor_t *)p;
    p += sizeof(*syntax_floor);

    object_floor->count_lhs = sizeof(object_floor->protid) + sizeof(object_floor->uuid) +
                              sizeof(object_floor->major_version);
    object_floor->protid = EPM_PROTOCOL_UUID;
    object_floor->count_rhs = sizeof(object_floor->minor_version);
    object_floor->uuid = object->SyntaxGUID;
    object_floor->major_version = object->SyntaxVersion.MajorVersion;
    object_floor->minor_version = object->SyntaxVersion.MinorVersion;

    syntax_floor->count_lhs = sizeof(syntax_floor->protid) + sizeof(syntax_floor->uuid) +
                              sizeof(syntax_floor->major_version);
    syntax_floor->protid = EPM_PROTOCOL_UUID;
    syntax_floor->count_rhs = sizeof(syntax_floor->minor_version);
    syntax_floor->uuid = syntax->SyntaxGUID;
    syntax_floor->major_version = syntax->SyntaxVersion.MajorVersion;
    syntax_floor->minor_version = syntax->SyntaxVersion.MinorVersion;

    status = RpcTransport_GetTopOfTower(p, &tower_size, protseq, address, endpoint);
    if (status != RPC_S_OK)
    {
        I_RpcFree(*tower);
        *tower = NULL;
        return status;
    }
    return RPC_S_OK;
}
