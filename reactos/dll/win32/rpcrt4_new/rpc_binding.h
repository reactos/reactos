/*
 * RPC binding API
 *
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
 */

#ifndef __WINE_RPC_BINDING_H
#define __WINE_RPC_BINDING_H

#include "wine/rpcss_shared.h"
#include "security.h"
#include "wine/list.h"


typedef struct _RpcAuthInfo
{
  LONG refs;

  ULONG AuthnLevel;
  ULONG AuthnSvc;
  CredHandle cred;
  TimeStamp exp;
  ULONG cbMaxToken;
  /* the auth identity pointer that the application passed us (freed by application) */
  RPC_AUTH_IDENTITY_HANDLE *identity;
  /* our copy of NT auth identity structure, if the authentication service
   * takes an NT auth identity */
  SEC_WINNT_AUTH_IDENTITY_W *nt_identity;
} RpcAuthInfo;

typedef struct _RpcQualityOfService
{
    LONG refs;

    RPC_SECURITY_QOS_V2_W *qos;
} RpcQualityOfService;

typedef struct _RpcAssoc
{
    struct list entry; /* entry in the global list of associations */
    LONG refs;

    LPSTR Protseq;
    LPSTR NetworkAddr;
    LPSTR Endpoint;
    LPWSTR NetworkOptions;

    /* id of this association group */
    ULONG assoc_group_id;

    CRITICAL_SECTION cs;
    struct list connection_pool;
} RpcAssoc;

struct connection_ops;

typedef struct _RpcConnection
{
  struct _RpcConnection* Next;
  BOOL server;
  LPSTR NetworkAddr;
  LPSTR Endpoint;
  LPWSTR NetworkOptions;
  const struct connection_ops *ops;
  USHORT MaxTransmissionSize;
  /* The active interface bound to server. */
  RPC_SYNTAX_IDENTIFIER ActiveInterface;
  USHORT NextCallId;

  /* authentication */
  CtxtHandle ctx;
  TimeStamp exp;
  ULONG attr;
  RpcAuthInfo *AuthInfo;
  ULONG encryption_auth_len;
  ULONG signature_auth_len;
  RpcQualityOfService *QOS;

  /* client-only */
  struct list conn_pool_entry;
  ULONG assoc_group_id; /* association group returned during binding */
} RpcConnection;

struct connection_ops {
  const char *name;
  unsigned char epm_protocols[2]; /* only floors 3 and 4. see http://www.opengroup.org/onlinepubs/9629399/apdxl.htm */
  RpcConnection *(*alloc)(void);
  RPC_STATUS (*open_connection_client)(RpcConnection *conn);
  RPC_STATUS (*handoff)(RpcConnection *old_conn, RpcConnection *new_conn);
  int (*read)(RpcConnection *conn, void *buffer, unsigned int len);
  int (*write)(RpcConnection *conn, const void *buffer, unsigned int len);
  int (*close)(RpcConnection *conn);
  size_t (*get_top_of_tower)(unsigned char *tower_data, const char *networkaddr, const char *endpoint);
  RPC_STATUS (*parse_top_of_tower)(const unsigned char *tower_data, size_t tower_size, char **networkaddr, char **endpoint);
};

/* don't know what MS's structure looks like */
typedef struct _RpcBinding
{
  LONG refs;
  struct _RpcBinding* Next;
  BOOL server;
  UUID ObjectUuid;
  LPSTR Protseq;
  LPSTR NetworkAddr;
  LPSTR Endpoint;
  LPWSTR NetworkOptions;
  RPC_BLOCKING_FN BlockingFn;
  ULONG ServerTid;
  RpcConnection* FromConn;
  RpcAssoc *Assoc;

  /* authentication */
  RpcAuthInfo *AuthInfo;
  RpcQualityOfService *QOS;
} RpcBinding;

LPSTR RPCRT4_strndupA(LPCSTR src, INT len);
LPWSTR RPCRT4_strndupW(LPCWSTR src, INT len);
LPSTR RPCRT4_strdupWtoA(LPCWSTR src);
LPWSTR RPCRT4_strdupAtoW(LPCSTR src);
void RPCRT4_strfree(LPSTR src);

#define RPCRT4_strdupA(x) RPCRT4_strndupA((x),-1)
#define RPCRT4_strdupW(x) RPCRT4_strndupW((x),-1)

ULONG RpcAuthInfo_AddRef(RpcAuthInfo *AuthInfo);
ULONG RpcAuthInfo_Release(RpcAuthInfo *AuthInfo);
BOOL RpcAuthInfo_IsEqual(const RpcAuthInfo *AuthInfo1, const RpcAuthInfo *AuthInfo2);
ULONG RpcQualityOfService_AddRef(RpcQualityOfService *qos);
ULONG RpcQualityOfService_Release(RpcQualityOfService *qos);
BOOL RpcQualityOfService_IsEqual(const RpcQualityOfService *qos1, const RpcQualityOfService *qos2);

RPC_STATUS RPCRT4_GetAssociation(LPCSTR Protseq, LPCSTR NetworkAddr, LPCSTR Endpoint, LPCWSTR NetworkOptions, RpcAssoc **assoc);
RPC_STATUS RpcAssoc_GetClientConnection(RpcAssoc *assoc, const RPC_SYNTAX_IDENTIFIER *InterfaceId, const RPC_SYNTAX_IDENTIFIER *TransferSyntax, RpcAuthInfo *AuthInfo, RpcQualityOfService *QOS, RpcConnection **Connection);
void RpcAssoc_ReleaseIdleConnection(RpcAssoc *assoc, RpcConnection *Connection);
ULONG RpcAssoc_Release(RpcAssoc *assoc);

RPC_STATUS RPCRT4_CreateConnection(RpcConnection** Connection, BOOL server, LPCSTR Protseq, LPCSTR NetworkAddr, LPCSTR Endpoint, LPCWSTR NetworkOptions, RpcAuthInfo* AuthInfo, RpcQualityOfService *QOS);
RPC_STATUS RPCRT4_DestroyConnection(RpcConnection* Connection);
RPC_STATUS RPCRT4_OpenClientConnection(RpcConnection* Connection);
RPC_STATUS RPCRT4_CloseConnection(RpcConnection* Connection);
RPC_STATUS RPCRT4_SpawnConnection(RpcConnection** Connection, RpcConnection* OldConnection);

RPC_STATUS RPCRT4_ResolveBinding(RpcBinding* Binding, LPCSTR Endpoint);
RPC_STATUS RPCRT4_SetBindingObject(RpcBinding* Binding, const UUID* ObjectUuid);
RPC_STATUS RPCRT4_MakeBinding(RpcBinding** Binding, RpcConnection* Connection);
RPC_STATUS RPCRT4_ExportBinding(RpcBinding** Binding, RpcBinding* OldBinding);
RPC_STATUS RPCRT4_DestroyBinding(RpcBinding* Binding);
RPC_STATUS RPCRT4_OpenBinding(RpcBinding* Binding, RpcConnection** Connection, PRPC_SYNTAX_IDENTIFIER TransferSyntax, PRPC_SYNTAX_IDENTIFIER InterfaceId);
RPC_STATUS RPCRT4_CloseBinding(RpcBinding* Binding, RpcConnection* Connection);
BOOL RPCRT4_RPCSSOnDemandCall(PRPCSS_NP_MESSAGE msg, char *vardata_payload, PRPCSS_NP_REPLY reply);
HANDLE RPCRT4_GetMasterMutex(void);
HANDLE RPCRT4_RpcssNPConnect(void);

static inline const char *rpcrt4_conn_get_name(RpcConnection *Connection)
{
  return Connection->ops->name;
}

static inline int rpcrt4_conn_read(RpcConnection *Connection,
                     void *buffer, unsigned int len)
{
  return Connection->ops->read(Connection, buffer, len);
}

static inline int rpcrt4_conn_write(RpcConnection *Connection,
                     const void *buffer, unsigned int len)
{
  return Connection->ops->write(Connection, buffer, len);
}

static inline int rpcrt4_conn_close(RpcConnection *Connection)
{
  return Connection->ops->close(Connection);
}

static inline RPC_STATUS rpcrt4_conn_handoff(RpcConnection *old_conn, RpcConnection *new_conn)
{
  return old_conn->ops->handoff(old_conn, new_conn);
}

/* floors 3 and up */
RPC_STATUS RpcTransport_GetTopOfTower(unsigned char *tower_data, size_t *tower_size, const char *protseq, const char *networkaddr, const char *endpoint);
RPC_STATUS RpcTransport_ParseTopOfTower(const unsigned char *tower_data, size_t tower_size, char **protseq, char **networkaddr, char **endpoint);

#endif
