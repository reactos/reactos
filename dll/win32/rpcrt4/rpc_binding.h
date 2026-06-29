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

#include "rpcndr.h"
#include "security.h"
#include "wine/list.h"
#include "rpc_defs.h"


enum secure_packet_direction
{
  SECURE_PACKET_SEND,
  SECURE_PACKET_RECEIVE
};

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
  LPWSTR server_principal_name;
} RpcAuthInfo;

typedef struct _RpcQualityOfService
{
    LONG refs;

    RPC_SECURITY_QOS_V2_W *qos;
} RpcQualityOfService;

struct connection_ops;

typedef struct _RpcConnection
{
  LONG ref;
  BOOL server;
  HANDLE wait_release;
  LPSTR NetworkAddr;
  LPSTR Endpoint;
  LPWSTR NetworkOptions;
  const struct connection_ops *ops;
  USHORT MaxTransmissionSize;

  /* authentication */
  CtxtHandle ctx;
  TimeStamp exp;
  ULONG attr;
  RpcAuthInfo *AuthInfo;
  ULONG auth_context_id;
  ULONG encryption_auth_len;
  ULONG signature_auth_len;
  RpcQualityOfService *QOS;
  LPWSTR CookieAuth;

  /* client-only */
  struct list conn_pool_entry;
  ULONG assoc_group_id; /* association group returned during binding */
  RPC_ASYNC_STATE *async_state;
  struct _RpcAssoc *assoc; /* association this connection is part of */

  /* server-only */
  /* The active interface bound to server. */
  RPC_SYNTAX_IDENTIFIER ActiveInterface;
  USHORT NextCallId;
  struct list protseq_entry;
  struct _RpcServerProtseq *protseq;
  struct _RpcBinding *server_binding;
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
  void (*close_read)(RpcConnection *conn);
  void (*cancel_call)(RpcConnection *conn);
  RPC_STATUS (*is_server_listening)(const char *endpoint);
  int (*wait_for_incoming_data)(RpcConnection *conn);
  size_t (*get_top_of_tower)(unsigned char *tower_data, const char *networkaddr, const char *endpoint);
  RPC_STATUS (*parse_top_of_tower)(const unsigned char *tower_data, size_t tower_size, char **networkaddr, char **endpoint);
  RPC_STATUS (*receive_fragment)(RpcConnection *conn, RpcPktHdr **Header, void **Payload);
  BOOL (*is_authorized)(RpcConnection *conn);
  RPC_STATUS (*authorize)(RpcConnection *conn, BOOL first_time, unsigned char *in_buffer, unsigned int in_len, unsigned char *out_buffer, unsigned int *out_len);
  RPC_STATUS (*secure_packet)(RpcConnection *Connection, enum secure_packet_direction dir, RpcPktHdr *hdr, unsigned int hdr_size, unsigned char *stub_data, unsigned int stub_data_size, RpcAuthVerifier *auth_hdr, unsigned char *auth_value, unsigned int auth_value_size);
  RPC_STATUS (*impersonate_client)(RpcConnection *conn);
  RPC_STATUS (*revert_to_self)(RpcConnection *conn);
  RPC_STATUS (*inquire_auth_client)(RpcConnection *, RPC_AUTHZ_HANDLE *, RPC_WSTR *, ULONG *, ULONG *, ULONG *, ULONG);
  RPC_STATUS (*inquire_client_pid)(RpcConnection *conn, ULONG *pid);
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
  struct _RpcAssoc *Assoc;

  /* authentication */
  RpcAuthInfo *AuthInfo;
  RpcQualityOfService *QOS;
  LPWSTR CookieAuth;
} RpcBinding;

LPWSTR RPCRT4_strndupW(LPCWSTR src, INT len);
LPSTR RPCRT4_strdupWtoA(LPCWSTR src);
LPWSTR RPCRT4_strdupAtoW(LPCSTR src);

RPC_STATUS RpcAuthInfo_Create(ULONG AuthnLevel, ULONG AuthnSvc, CredHandle cred, TimeStamp exp, ULONG cbMaxToken, RPC_AUTH_IDENTITY_HANDLE identity, RpcAuthInfo **ret);
ULONG RpcAuthInfo_AddRef(RpcAuthInfo *AuthInfo);
ULONG RpcAuthInfo_Release(RpcAuthInfo *AuthInfo);
BOOL RpcAuthInfo_IsEqual(const RpcAuthInfo *AuthInfo1, const RpcAuthInfo *AuthInfo2);
ULONG RpcQualityOfService_AddRef(RpcQualityOfService *qos);
ULONG RpcQualityOfService_Release(RpcQualityOfService *qos);
BOOL RpcQualityOfService_IsEqual(const RpcQualityOfService *qos1, const RpcQualityOfService *qos2);

RPC_STATUS RPCRT4_CreateConnection(RpcConnection** Connection, BOOL server, LPCSTR Protseq,
    LPCSTR NetworkAddr, LPCSTR Endpoint, LPCWSTR NetworkOptions, RpcAuthInfo* AuthInfo,
    RpcQualityOfService *QOS, LPCWSTR CookieAuth);
RpcConnection *RPCRT4_GrabConnection( RpcConnection *conn );
void RPCRT4_ReleaseConnection(RpcConnection* Connection);
RPC_STATUS RPCRT4_OpenClientConnection(RpcConnection* Connection);
RPC_STATUS RPCRT4_CloseConnection(RpcConnection* Connection);
RPC_STATUS RPCRT4_IsServerListening(const char *protseq, const char *endpoint);

RPC_STATUS RPCRT4_ResolveBinding(RpcBinding* Binding, LPCSTR Endpoint);
RPC_STATUS RPCRT4_SetBindingObject(RpcBinding* Binding, const UUID* ObjectUuid);
RPC_STATUS RPCRT4_MakeBinding(RpcBinding** Binding, RpcConnection* Connection);
void       RPCRT4_AddRefBinding(RpcBinding* Binding);
RPC_STATUS RPCRT4_ReleaseBinding(RpcBinding* Binding);
RPC_STATUS RPCRT4_OpenBinding(RpcBinding* Binding, RpcConnection** Connection,
                              const RPC_SYNTAX_IDENTIFIER *TransferSyntax, const RPC_SYNTAX_IDENTIFIER *InterfaceId,
                              BOOL *from_cache);
RPC_STATUS RPCRT4_CloseBinding(RpcBinding* Binding, RpcConnection* Connection);

void rpcrt4_conn_release_and_wait(RpcConnection *connection);

static inline const char *rpcrt4_conn_get_name(const RpcConnection *Connection)
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

static inline void rpcrt4_conn_close_read(RpcConnection *connection)
{
  connection->ops->close_read(connection);
}

static inline void rpcrt4_conn_cancel_call(RpcConnection *Connection)
{
  Connection->ops->cancel_call(Connection);
}

static inline RPC_STATUS rpcrt4_conn_handoff(RpcConnection *old_conn, RpcConnection *new_conn)
{
  return old_conn->ops->handoff(old_conn, new_conn);
}

static inline BOOL rpcrt4_conn_is_authorized(RpcConnection *Connection)
{
    return Connection->ops->is_authorized(Connection);
}

static inline RPC_STATUS rpcrt4_conn_authorize(
    RpcConnection *conn, BOOL first_time, unsigned char *in_buffer,
    unsigned int in_len, unsigned char *out_buffer, unsigned int *out_len)
{
    return conn->ops->authorize(conn, first_time, in_buffer, in_len, out_buffer, out_len);
}

static inline RPC_STATUS rpcrt4_conn_secure_packet(
    RpcConnection *conn, enum secure_packet_direction dir,
    RpcPktHdr *hdr, unsigned int hdr_size, unsigned char *stub_data,
    unsigned int stub_data_size, RpcAuthVerifier *auth_hdr,
    unsigned char *auth_value, unsigned int auth_value_size)
{
    return conn->ops->secure_packet(conn, dir, hdr, hdr_size, stub_data, stub_data_size, auth_hdr, auth_value, auth_value_size);
}

static inline RPC_STATUS rpcrt4_conn_impersonate_client(
    RpcConnection *conn)
{
    return conn->ops->impersonate_client(conn);
}

static inline RPC_STATUS rpcrt4_conn_revert_to_self(
    RpcConnection *conn)
{
    return conn->ops->revert_to_self(conn);
}

static inline RPC_STATUS rpcrt4_conn_inquire_auth_client(
    RpcConnection *conn, RPC_AUTHZ_HANDLE *privs, RPC_WSTR *server_princ_name,
    ULONG *authn_level, ULONG *authn_svc, ULONG *authz_svc, ULONG flags)
{
    return conn->ops->inquire_auth_client(conn, privs, server_princ_name, authn_level, authn_svc, authz_svc, flags);
}

/* floors 3 and up */
RPC_STATUS RpcTransport_GetTopOfTower(unsigned char *tower_data, size_t *tower_size, const char *protseq, const char *networkaddr, const char *endpoint);
RPC_STATUS RpcTransport_ParseTopOfTower(const unsigned char *tower_data, size_t tower_size, char **protseq, char **networkaddr, char **endpoint);

void RPCRT4_SetThreadCurrentConnection(RpcConnection *Connection);
void RPCRT4_SetThreadCurrentCallHandle(RpcBinding *Binding);
RpcBinding *RPCRT4_GetThreadCurrentCallHandle(void);
void RPCRT4_PushThreadContextHandle(NDR_SCONTEXT SContext);
void RPCRT4_RemoveThreadContextHandle(NDR_SCONTEXT SContext);
NDR_SCONTEXT RPCRT4_PopThreadContextHandle(void);

#endif
