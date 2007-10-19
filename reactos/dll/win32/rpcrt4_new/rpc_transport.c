/*
 * RPC transport layer
 *
 * Copyright 2001 Ove Kåven, TransGaming Technologies
 * Copyright 2003 Mike Hearn
 * Copyright 2004 Filip Navara
 * Copyright 2006 Mike McCormack
 * Copyright 2006 Damjan Jovanovic
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
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_TCP_H
# include <netinet/tcp.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif

#include <winsock2.h>
#include <ws2tcpip.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winerror.h"
#include "winternl.h"
#include "wine/unicode.h"

#include "rpc.h"
#include "rpcndr.h"

#include "wine/debug.h"

#include "rpc_binding.h"
#include "rpc_message.h"
#include "rpc_server.h"
#include "epm_towers.h"

#include "unix_func.h"

#ifndef SOL_TCP
# define SOL_TCP IPPROTO_TCP
#endif

WINE_DEFAULT_DEBUG_CHANNEL(rpc);

static CRITICAL_SECTION assoc_list_cs;
static CRITICAL_SECTION_DEBUG assoc_list_cs_debug =
{
    0, 0, &assoc_list_cs,
    { &assoc_list_cs_debug.ProcessLocksList, &assoc_list_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": assoc_list_cs") }
};
static CRITICAL_SECTION assoc_list_cs = { &assoc_list_cs_debug, -1, 0, 0, 0, 0 };

static struct list assoc_list = LIST_INIT(assoc_list);

/**** ncacn_np support ****/

typedef struct _RpcConnection_np
{
  RpcConnection common;
  HANDLE pipe;
  OVERLAPPED ovl;
  BOOL listening;
} RpcConnection_np;

static RpcConnection *rpcrt4_conn_np_alloc(void)
{
  RpcConnection_np *npc = HeapAlloc(GetProcessHeap(), 0, sizeof(RpcConnection_np));
  if (npc)
  {
    npc->pipe = NULL;
    memset(&npc->ovl, 0, sizeof(npc->ovl));
    npc->listening = FALSE;
  }
  return &npc->common;
}

static RPC_STATUS rpcrt4_conn_listen_pipe(RpcConnection_np *npc)
{
  if (npc->listening)
    return RPC_S_OK;

  npc->listening = TRUE;
  if (ConnectNamedPipe(npc->pipe, &npc->ovl))
    return RPC_S_OK;

  if (GetLastError() == ERROR_PIPE_CONNECTED) {
    SetEvent(npc->ovl.hEvent);
    return RPC_S_OK;
  }
  if (GetLastError() == ERROR_IO_PENDING) {
    /* will be completed in rpcrt4_protseq_np_wait_for_new_connection */
    return RPC_S_OK;
  }
  npc->listening = FALSE;
  WARN("Couldn't ConnectNamedPipe (error was %d)\n", GetLastError());
  return RPC_S_OUT_OF_RESOURCES;
}

static RPC_STATUS rpcrt4_conn_create_pipe(RpcConnection *Connection, LPCSTR pname)
{
  RpcConnection_np *npc = (RpcConnection_np *) Connection;
  TRACE("listening on %s\n", pname);

  npc->pipe = CreateNamedPipeA(pname, PIPE_ACCESS_DUPLEX,
                               PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
                               PIPE_UNLIMITED_INSTANCES,
                               RPC_MAX_PACKET_SIZE, RPC_MAX_PACKET_SIZE, 5000, NULL);
  if (npc->pipe == INVALID_HANDLE_VALUE) {
    WARN("CreateNamedPipe failed with error %d\n", GetLastError());
    if (GetLastError() == ERROR_FILE_EXISTS)
      return RPC_S_DUPLICATE_ENDPOINT;
    else
      return RPC_S_CANT_CREATE_ENDPOINT;
  }

  memset(&npc->ovl, 0, sizeof(npc->ovl));
  npc->ovl.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

  /* Note: we don't call ConnectNamedPipe here because it must be done in the
   * server thread as the thread must be alertable */
  return RPC_S_OK;
}

static RPC_STATUS rpcrt4_conn_open_pipe(RpcConnection *Connection, LPCSTR pname, BOOL wait)
{
  RpcConnection_np *npc = (RpcConnection_np *) Connection;
  HANDLE pipe;
  DWORD err, dwMode;

  TRACE("connecting to %s\n", pname);

  while (TRUE) {
    DWORD dwFlags = 0;
    if (Connection->QOS)
    {
        dwFlags = SECURITY_SQOS_PRESENT;
        switch (Connection->QOS->qos->ImpersonationType)
        {
            case RPC_C_IMP_LEVEL_DEFAULT:
                /* FIXME: what to do here? */
                break;
            case RPC_C_IMP_LEVEL_ANONYMOUS:
                dwFlags |= SECURITY_ANONYMOUS;
                break;
            case RPC_C_IMP_LEVEL_IDENTIFY:
                dwFlags |= SECURITY_IDENTIFICATION;
                break;
            case RPC_C_IMP_LEVEL_IMPERSONATE:
                dwFlags |= SECURITY_IMPERSONATION;
                break;
            case RPC_C_IMP_LEVEL_DELEGATE:
                dwFlags |= SECURITY_DELEGATION;
                break;
        }
        if (Connection->QOS->qos->IdentityTracking == RPC_C_QOS_IDENTIFY_DYNAMIC)
            dwFlags |= SECURITY_CONTEXT_TRACKING;
    }
    pipe = CreateFileA(pname, GENERIC_READ|GENERIC_WRITE, 0, NULL,
                       OPEN_EXISTING, dwFlags, 0);
    if (pipe != INVALID_HANDLE_VALUE) break;
    err = GetLastError();
    if (err == ERROR_PIPE_BUSY) {
      TRACE("connection failed, error=%x\n", err);
      return RPC_S_SERVER_TOO_BUSY;
    }
    if (!wait)
      return RPC_S_SERVER_UNAVAILABLE;
    if (!WaitNamedPipeA(pname, NMPWAIT_WAIT_FOREVER)) {
      err = GetLastError();
      WARN("connection failed, error=%x\n", err);
      return RPC_S_SERVER_UNAVAILABLE;
    }
  }

  /* success */
  memset(&npc->ovl, 0, sizeof(npc->ovl));
  /* pipe is connected; change to message-read mode. */
  dwMode = PIPE_READMODE_MESSAGE;
  SetNamedPipeHandleState(pipe, &dwMode, NULL, NULL);
  npc->ovl.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
  npc->pipe = pipe;

  return RPC_S_OK;
}

static RPC_STATUS rpcrt4_ncalrpc_open(RpcConnection* Connection)
{
  RpcConnection_np *npc = (RpcConnection_np *) Connection;
  static const char prefix[] = "\\\\.\\pipe\\lrpc\\";
  RPC_STATUS r;
  LPSTR pname;

  /* already connected? */
  if (npc->pipe)
    return RPC_S_OK;

  /* protseq=ncalrpc: supposed to use NT LPC ports,
   * but we'll implement it with named pipes for now */
  pname = I_RpcAllocate(strlen(prefix) + strlen(Connection->Endpoint) + 1);
  strcat(strcpy(pname, prefix), Connection->Endpoint);
  r = rpcrt4_conn_open_pipe(Connection, pname, TRUE);
  I_RpcFree(pname);

  return r;
}

static RPC_STATUS rpcrt4_protseq_ncalrpc_open_endpoint(RpcServerProtseq* protseq, LPSTR endpoint)
{
  static const char prefix[] = "\\\\.\\pipe\\lrpc\\";
  RPC_STATUS r;
  LPSTR pname;
  RpcConnection *Connection;

  r = RPCRT4_CreateConnection(&Connection, TRUE, protseq->Protseq, NULL,
                              endpoint, NULL, NULL, NULL);
  if (r != RPC_S_OK)
      return r;

  /* protseq=ncalrpc: supposed to use NT LPC ports,
   * but we'll implement it with named pipes for now */
  pname = I_RpcAllocate(strlen(prefix) + strlen(Connection->Endpoint) + 1);
  strcat(strcpy(pname, prefix), Connection->Endpoint);
  r = rpcrt4_conn_create_pipe(Connection, pname);
  I_RpcFree(pname);

  EnterCriticalSection(&protseq->cs);
  Connection->Next = protseq->conn;
  protseq->conn = Connection;
  LeaveCriticalSection(&protseq->cs);

  return r;
}

static RPC_STATUS rpcrt4_ncacn_np_open(RpcConnection* Connection)
{
  RpcConnection_np *npc = (RpcConnection_np *) Connection;
  static const char prefix[] = "\\\\.";
  RPC_STATUS r;
  LPSTR pname;

  /* already connected? */
  if (npc->pipe)
    return RPC_S_OK;

  /* protseq=ncacn_np: named pipes */
  pname = I_RpcAllocate(strlen(prefix) + strlen(Connection->Endpoint) + 1);
  strcat(strcpy(pname, prefix), Connection->Endpoint);
  r = rpcrt4_conn_open_pipe(Connection, pname, FALSE);
  I_RpcFree(pname);

  return r;
}

static RPC_STATUS rpcrt4_protseq_ncacn_np_open_endpoint(RpcServerProtseq *protseq, LPSTR endpoint)
{
  static const char prefix[] = "\\\\.";
  RPC_STATUS r;
  LPSTR pname;
  RpcConnection *Connection;

  r = RPCRT4_CreateConnection(&Connection, TRUE, protseq->Protseq, NULL,
                              endpoint, NULL, NULL, NULL);
  if (r != RPC_S_OK)
    return r;

  /* protseq=ncacn_np: named pipes */
  pname = I_RpcAllocate(strlen(prefix) + strlen(Connection->Endpoint) + 1);
  strcat(strcpy(pname, prefix), Connection->Endpoint);
  r = rpcrt4_conn_create_pipe(Connection, pname);
  I_RpcFree(pname);

  EnterCriticalSection(&protseq->cs);
  Connection->Next = protseq->conn;
  protseq->conn = Connection;
  LeaveCriticalSection(&protseq->cs);

  return r;
}

static void rpcrt4_conn_np_handoff(RpcConnection_np *old_npc, RpcConnection_np *new_npc)
{
  /* because of the way named pipes work, we'll transfer the connected pipe
   * to the child, then reopen the server binding to continue listening */

  new_npc->pipe = old_npc->pipe;
  new_npc->ovl = old_npc->ovl;
  old_npc->pipe = 0;
  memset(&old_npc->ovl, 0, sizeof(old_npc->ovl));
  old_npc->listening = FALSE;
}

static RPC_STATUS rpcrt4_ncacn_np_handoff(RpcConnection *old_conn, RpcConnection *new_conn)
{
  RPC_STATUS status;
  LPSTR pname;
  static const char prefix[] = "\\\\.";

  rpcrt4_conn_np_handoff((RpcConnection_np *)old_conn, (RpcConnection_np *)new_conn);

  pname = I_RpcAllocate(strlen(prefix) + strlen(old_conn->Endpoint) + 1);
  strcat(strcpy(pname, prefix), old_conn->Endpoint);
  status = rpcrt4_conn_create_pipe(old_conn, pname);
  I_RpcFree(pname);

  return status;
}

static RPC_STATUS rpcrt4_ncalrpc_handoff(RpcConnection *old_conn, RpcConnection *new_conn)
{
  RPC_STATUS status;
  LPSTR pname;
  static const char prefix[] = "\\\\.\\pipe\\lrpc\\";

  TRACE("%s\n", old_conn->Endpoint);

  rpcrt4_conn_np_handoff((RpcConnection_np *)old_conn, (RpcConnection_np *)new_conn);

  pname = I_RpcAllocate(strlen(prefix) + strlen(old_conn->Endpoint) + 1);
  strcat(strcpy(pname, prefix), old_conn->Endpoint);
  status = rpcrt4_conn_create_pipe(old_conn, pname);
  I_RpcFree(pname);

  return status;
}

static int rpcrt4_conn_np_read(RpcConnection *Connection,
                        void *buffer, unsigned int count)
{
  RpcConnection_np *npc = (RpcConnection_np *) Connection;
  char *buf = buffer;
  BOOL ret = TRUE;
  unsigned int bytes_left = count;

  while (bytes_left)
  {
    DWORD bytes_read;
    ret = ReadFile(npc->pipe, buf, bytes_left, &bytes_read, NULL);
    if (!ret || !bytes_read)
        break;
    bytes_left -= bytes_read;
    buf += bytes_read;
  }
  return ret ? count : -1;
}

static int rpcrt4_conn_np_write(RpcConnection *Connection,
                             const void *buffer, unsigned int count)
{
  RpcConnection_np *npc = (RpcConnection_np *) Connection;
  const char *buf = buffer;
  BOOL ret = TRUE;
  unsigned int bytes_left = count;

  while (bytes_left)
  {
    DWORD bytes_written;
    ret = WriteFile(npc->pipe, buf, count, &bytes_written, NULL);
    if (!ret || !bytes_written)
        break;
    bytes_left -= bytes_written;
    buf += bytes_written;
  }
  return ret ? count : -1;
}

static int rpcrt4_conn_np_close(RpcConnection *Connection)
{
  RpcConnection_np *npc = (RpcConnection_np *) Connection;
  if (npc->pipe) {
    FlushFileBuffers(npc->pipe);
    CloseHandle(npc->pipe);
    npc->pipe = 0;
  }
  if (npc->ovl.hEvent) {
    CloseHandle(npc->ovl.hEvent);
    npc->ovl.hEvent = 0;
  }
  return 0;
}

static size_t rpcrt4_ncacn_np_get_top_of_tower(unsigned char *tower_data,
                                               const char *networkaddr,
                                               const char *endpoint)
{
    twr_empty_floor_t *smb_floor;
    twr_empty_floor_t *nb_floor;
    size_t size;
    size_t networkaddr_size;
    size_t endpoint_size;

    TRACE("(%p, %s, %s)\n", tower_data, networkaddr, endpoint);

    networkaddr_size = strlen(networkaddr) + 1;
    endpoint_size = strlen(endpoint) + 1;
    size = sizeof(*smb_floor) + endpoint_size + sizeof(*nb_floor) + networkaddr_size;

    if (!tower_data)
        return size;

    smb_floor = (twr_empty_floor_t *)tower_data;

    tower_data += sizeof(*smb_floor);

    smb_floor->count_lhs = sizeof(smb_floor->protid);
    smb_floor->protid = EPM_PROTOCOL_SMB;
    smb_floor->count_rhs = endpoint_size;

    memcpy(tower_data, endpoint, endpoint_size);
    tower_data += endpoint_size;

    nb_floor = (twr_empty_floor_t *)tower_data;

    tower_data += sizeof(*nb_floor);

    nb_floor->count_lhs = sizeof(nb_floor->protid);
    nb_floor->protid = EPM_PROTOCOL_NETBIOS;
    nb_floor->count_rhs = networkaddr_size;

    memcpy(tower_data, networkaddr, networkaddr_size);
    tower_data += networkaddr_size;

    return size;
}

static RPC_STATUS rpcrt4_ncacn_np_parse_top_of_tower(const unsigned char *tower_data,
                                                     size_t tower_size,
                                                     char **networkaddr,
                                                     char **endpoint)
{
    const twr_empty_floor_t *smb_floor = (const twr_empty_floor_t *)tower_data;
    const twr_empty_floor_t *nb_floor;

    TRACE("(%p, %d, %p, %p)\n", tower_data, (int)tower_size, networkaddr, endpoint);

    if (tower_size < sizeof(*smb_floor))
        return EPT_S_NOT_REGISTERED;

    tower_data += sizeof(*smb_floor);
    tower_size -= sizeof(*smb_floor);

    if ((smb_floor->count_lhs != sizeof(smb_floor->protid)) ||
        (smb_floor->protid != EPM_PROTOCOL_SMB) ||
        (smb_floor->count_rhs > tower_size))
        return EPT_S_NOT_REGISTERED;

    if (endpoint)
    {
        *endpoint = I_RpcAllocate(smb_floor->count_rhs);
        if (!*endpoint)
            return RPC_S_OUT_OF_RESOURCES;
        memcpy(*endpoint, tower_data, smb_floor->count_rhs);
    }
    tower_data += smb_floor->count_rhs;
    tower_size -= smb_floor->count_rhs;

    if (tower_size < sizeof(*nb_floor))
        return EPT_S_NOT_REGISTERED;

    nb_floor = (const twr_empty_floor_t *)tower_data;

    tower_data += sizeof(*nb_floor);
    tower_size -= sizeof(*nb_floor);

    if ((nb_floor->count_lhs != sizeof(nb_floor->protid)) ||
        (nb_floor->protid != EPM_PROTOCOL_NETBIOS) ||
        (nb_floor->count_rhs > tower_size))
        return EPT_S_NOT_REGISTERED;

    if (networkaddr)
    {
        *networkaddr = I_RpcAllocate(nb_floor->count_rhs);
        if (!*networkaddr)
        {
            if (endpoint)
            {
                I_RpcFree(*endpoint);
                *endpoint = NULL;
            }
            return RPC_S_OUT_OF_RESOURCES;
        }
        memcpy(*networkaddr, tower_data, nb_floor->count_rhs);
    }

    return RPC_S_OK;
}

typedef struct _RpcServerProtseq_np
{
    RpcServerProtseq common;
    HANDLE mgr_event;
} RpcServerProtseq_np;

static RpcServerProtseq *rpcrt4_protseq_np_alloc(void)
{
    RpcServerProtseq_np *ps = HeapAlloc(GetProcessHeap(), 0, sizeof(*ps));
    if (ps)
        ps->mgr_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    return &ps->common;
}

static void rpcrt4_protseq_np_signal_state_changed(RpcServerProtseq *protseq)
{
    RpcServerProtseq_np *npps = CONTAINING_RECORD(protseq, RpcServerProtseq_np, common);
    SetEvent(npps->mgr_event);
}

static void *rpcrt4_protseq_np_get_wait_array(RpcServerProtseq *protseq, void *prev_array, unsigned int *count)
{
    HANDLE *objs = prev_array;
    RpcConnection_np *conn;
    RpcServerProtseq_np *npps = CONTAINING_RECORD(protseq, RpcServerProtseq_np, common);

    EnterCriticalSection(&protseq->cs);

    /* open and count connections */
    *count = 1;
    conn = CONTAINING_RECORD(protseq->conn, RpcConnection_np, common);
    while (conn) {
        rpcrt4_conn_listen_pipe(conn);
        if (conn->ovl.hEvent)
            (*count)++;
        conn = CONTAINING_RECORD(conn->common.Next, RpcConnection_np, common);
    }

    /* make array of connections */
    if (objs)
        objs = HeapReAlloc(GetProcessHeap(), 0, objs, *count*sizeof(HANDLE));
    else
        objs = HeapAlloc(GetProcessHeap(), 0, *count*sizeof(HANDLE));
    if (!objs)
    {
        ERR("couldn't allocate objs\n");
        LeaveCriticalSection(&protseq->cs);
        return NULL;
    }

    objs[0] = npps->mgr_event;
    *count = 1;
    conn = CONTAINING_RECORD(protseq->conn, RpcConnection_np, common);
    while (conn) {
        if ((objs[*count] = conn->ovl.hEvent))
            (*count)++;
        conn = CONTAINING_RECORD(conn->common.Next, RpcConnection_np, common);
    }
    LeaveCriticalSection(&protseq->cs);
    return objs;
}

static void rpcrt4_protseq_np_free_wait_array(RpcServerProtseq *protseq, void *array)
{
    HeapFree(GetProcessHeap(), 0, array);
}

static int rpcrt4_protseq_np_wait_for_new_connection(RpcServerProtseq *protseq, unsigned int count, void *wait_array)
{
    HANDLE b_handle;
    HANDLE *objs = wait_array;
    DWORD res;
    RpcConnection *cconn;
    RpcConnection_np *conn;

    if (!objs)
        return -1;

    res = WaitForMultipleObjects(count, objs, FALSE, INFINITE);
    if (res == WAIT_OBJECT_0)
        return 0;
    else if (res == WAIT_FAILED)
    {
        ERR("wait failed with error %d\n", GetLastError());
        return -1;
    }
    else
    {
        b_handle = objs[res - WAIT_OBJECT_0];
        /* find which connection got a RPC */
        EnterCriticalSection(&protseq->cs);
        conn = CONTAINING_RECORD(protseq->conn, RpcConnection_np, common);
        while (conn) {
            if (b_handle == conn->ovl.hEvent) break;
            conn = CONTAINING_RECORD(conn->common.Next, RpcConnection_np, common);
        }
        cconn = NULL;
        if (conn)
            RPCRT4_SpawnConnection(&cconn, &conn->common);
        else
            ERR("failed to locate connection for handle %p\n", b_handle);
        LeaveCriticalSection(&protseq->cs);
        if (cconn)
        {
            RPCRT4_new_client(cconn);
            return 1;
        }
        else return -1;
    }
}

static size_t rpcrt4_ncalrpc_get_top_of_tower(unsigned char *tower_data,
                                              const char *networkaddr,
                                              const char *endpoint)
{
    twr_empty_floor_t *pipe_floor;
    size_t size;
    size_t endpoint_size;

    TRACE("(%p, %s, %s)\n", tower_data, networkaddr, endpoint);

    endpoint_size = strlen(networkaddr) + 1;
    size = sizeof(*pipe_floor) + endpoint_size;

    if (!tower_data)
        return size;

    pipe_floor = (twr_empty_floor_t *)tower_data;

    tower_data += sizeof(*pipe_floor);

    pipe_floor->count_lhs = sizeof(pipe_floor->protid);
    pipe_floor->protid = EPM_PROTOCOL_SMB;
    pipe_floor->count_rhs = endpoint_size;

    memcpy(tower_data, endpoint, endpoint_size);
    tower_data += endpoint_size;

    return size;
}

static RPC_STATUS rpcrt4_ncalrpc_parse_top_of_tower(const unsigned char *tower_data,
                                                    size_t tower_size,
                                                    char **networkaddr,
                                                    char **endpoint)
{
    const twr_empty_floor_t *pipe_floor = (const twr_empty_floor_t *)tower_data;

    TRACE("(%p, %d, %p, %p)\n", tower_data, (int)tower_size, networkaddr, endpoint);

    *networkaddr = NULL;
    *endpoint = NULL;

    if (tower_size < sizeof(*pipe_floor))
        return EPT_S_NOT_REGISTERED;

    tower_data += sizeof(*pipe_floor);
    tower_size -= sizeof(*pipe_floor);

    if ((pipe_floor->count_lhs != sizeof(pipe_floor->protid)) ||
        (pipe_floor->protid != EPM_PROTOCOL_SMB) ||
        (pipe_floor->count_rhs > tower_size))
        return EPT_S_NOT_REGISTERED;

    if (endpoint)
    {
        *endpoint = I_RpcAllocate(pipe_floor->count_rhs);
        if (!*endpoint)
            return RPC_S_OUT_OF_RESOURCES;
        memcpy(*endpoint, tower_data, pipe_floor->count_rhs);
    }

    return RPC_S_OK;
}

/**** ncacn_ip_tcp support ****/

typedef struct _RpcConnection_tcp
{
  RpcConnection common;
  int sock;
} RpcConnection_tcp;

static RpcConnection *rpcrt4_conn_tcp_alloc(void)
{
  RpcConnection_tcp *tcpc;
  tcpc = HeapAlloc(GetProcessHeap(), 0, sizeof(RpcConnection_tcp));
  if (tcpc == NULL)
    return NULL;
  tcpc->sock = -1;
  return &tcpc->common;
}

static RPC_STATUS rpcrt4_ncacn_ip_tcp_open(RpcConnection* Connection)
{
  RpcConnection_tcp *tcpc = (RpcConnection_tcp *) Connection;
  int sock;
  int ret;
  struct addrinfo *ai;
  struct addrinfo *ai_cur;
  struct addrinfo hints;

  TRACE("(%s, %s)\n", Connection->NetworkAddr, Connection->Endpoint);

  if (tcpc->sock != -1)
    return RPC_S_OK;

  hints.ai_flags          = 0;
  hints.ai_family         = PF_UNSPEC;
  hints.ai_socktype       = SOCK_STREAM;
  hints.ai_protocol       = IPPROTO_TCP;
  hints.ai_addrlen        = 0;
  hints.ai_addr           = NULL;
  hints.ai_canonname      = NULL;
  hints.ai_next           = NULL;

  ret = getaddrinfo(Connection->NetworkAddr, Connection->Endpoint, &hints, &ai);
  if (ret)
  {
    ERR("getaddrinfo for %s:%s failed: %s\n", Connection->NetworkAddr,
      Connection->Endpoint, gai_strerror(ret));
    return RPC_S_SERVER_UNAVAILABLE;
  }

  for (ai_cur = ai; ai_cur; ai_cur = ai_cur->ai_next)
  {
    int val;

    if (TRACE_ON(rpc))
    {
      char host[256];
      char service[256];
      getnameinfo(ai_cur->ai_addr, ai_cur->ai_addrlen,
        host, sizeof(host), service, sizeof(service),
        NI_NUMERICHOST | NI_NUMERICSERV);
      TRACE("trying %s:%s\n", host, service);
    }

    sock = socket(ai_cur->ai_family, ai_cur->ai_socktype, ai_cur->ai_protocol);
    if (sock < 0)
    {
      WARN("socket() failed: %s\n", strerror(errno));
      continue;
    }

    if (0>connect(sock, ai_cur->ai_addr, ai_cur->ai_addrlen))
    {
      WARN("connect() failed: %s\n", strerror(errno));
      close(sock);
      continue;
    }

    /* RPC depends on having minimal latency so disable the Nagle algorithm */
    val = 1;
    setsockopt(sock, SOL_TCP, TCP_NODELAY, (char *)&val, sizeof(val));

    tcpc->sock = sock;

    freeaddrinfo(ai);
    TRACE("connected\n");
    return RPC_S_OK;
  }

  freeaddrinfo(ai);
  ERR("couldn't connect to %s:%s\n", Connection->NetworkAddr, Connection->Endpoint);
  return RPC_S_SERVER_UNAVAILABLE;
}

static RPC_STATUS rpcrt4_protseq_ncacn_ip_tcp_open_endpoint(RpcServerProtseq *protseq, LPSTR endpoint)
{
    RPC_STATUS status = RPC_S_CANT_CREATE_ENDPOINT;
    int sock;
    int ret;
    struct addrinfo *ai;
    struct addrinfo *ai_cur;
    struct addrinfo hints;
    RpcConnection *first_connection = NULL;
    u_long blocking;

    TRACE("(%p, %s)\n", protseq, endpoint);

    hints.ai_flags          = AI_PASSIVE /* for non-localhost addresses */;
    hints.ai_family         = PF_UNSPEC;
    hints.ai_socktype       = SOCK_STREAM;
    hints.ai_protocol       = IPPROTO_TCP;
    hints.ai_addrlen        = 0;
    hints.ai_addr           = NULL;
    hints.ai_canonname      = NULL;
    hints.ai_next           = NULL;

    ret = getaddrinfo(NULL, endpoint, &hints, &ai);
    if (ret)
    {
        ERR("getaddrinfo for port %s failed: %s\n", endpoint,
            gai_strerror(ret));
        if ((ret == EAI_SERVICE) || (ret == EAI_NONAME))
            return RPC_S_INVALID_ENDPOINT_FORMAT;
        return RPC_S_CANT_CREATE_ENDPOINT;
    }

    for (ai_cur = ai; ai_cur; ai_cur = ai_cur->ai_next)
    {
        RpcConnection_tcp *tcpc;
        RPC_STATUS create_status;

        if (TRACE_ON(rpc))
        {
            char host[256];
            char service[256];
            getnameinfo(ai_cur->ai_addr, ai_cur->ai_addrlen,
                        host, sizeof(host), service, sizeof(service),
                        NI_NUMERICHOST | NI_NUMERICSERV);
            TRACE("trying %s:%s\n", host, service);
        }

        sock = socket(ai_cur->ai_family, ai_cur->ai_socktype, ai_cur->ai_protocol);
        if (sock < 0)
        {
            WARN("socket() failed: %s\n", strerror(errno));
            status = RPC_S_CANT_CREATE_ENDPOINT;
            continue;
        }

        ret = bind(sock, ai_cur->ai_addr, ai_cur->ai_addrlen);
        if (ret < 0)
        {
            WARN("bind failed: %s\n", strerror(errno));
            close(sock);
            if (errno == WSAEADDRINUSE)
              status = RPC_S_DUPLICATE_ENDPOINT;
            else
              status = RPC_S_CANT_CREATE_ENDPOINT;
            continue;
        }
        create_status = RPCRT4_CreateConnection((RpcConnection **)&tcpc, TRUE,
                                                protseq->Protseq, NULL,
                                                endpoint, NULL, NULL, NULL);
        if (create_status != RPC_S_OK)
        {
            close(sock);
            status = create_status;
            continue;
        }

        tcpc->sock = sock;
        ret = listen(sock, protseq->MaxCalls);
        if (ret < 0)
        {
            WARN("listen failed: %s\n", strerror(errno));
            RPCRT4_DestroyConnection(&tcpc->common);
            status = RPC_S_OUT_OF_RESOURCES;
            continue;
        }
        /* need a non-blocking socket, otherwise accept() has a potential
         * race-condition (poll() says it is readable, connection drops,
         * and accept() blocks until the next connection comes...)
         */
        blocking = 1;
        ret = ioctlsocket(sock, FIONBIO, &blocking);
        if (ret < 0)
        {
            WARN("couldn't make socket non-blocking, error %d\n", ret);
            RPCRT4_DestroyConnection(&tcpc->common);
            status = RPC_S_OUT_OF_RESOURCES;
            continue;
        }

        tcpc->common.Next = first_connection;
        first_connection = &tcpc->common;
    }

    freeaddrinfo(ai);

    /* if at least one connection was created for an endpoint then
     * return success */
    if (first_connection)
    {
        RpcConnection *conn;

        /* find last element in list */
        for (conn = first_connection; conn->Next; conn = conn->Next)
            ;

        EnterCriticalSection(&protseq->cs);
        conn->Next = protseq->conn;
        protseq->conn = first_connection;
        LeaveCriticalSection(&protseq->cs);

        TRACE("listening on %s\n", endpoint);
        return RPC_S_OK;
    }

    ERR("couldn't listen on port %s\n", endpoint);
    return status;
}

static RPC_STATUS rpcrt4_conn_tcp_handoff(RpcConnection *old_conn, RpcConnection *new_conn)
{
  int ret;
  struct sockaddr_in address;
  socklen_t addrsize;
  u_long blocking;
  RpcConnection_tcp *server = (RpcConnection_tcp*) old_conn;
  RpcConnection_tcp *client = (RpcConnection_tcp*) new_conn;

  addrsize = sizeof(address);
  ret = accept(server->sock, (struct sockaddr*) &address, &addrsize);
  if (ret < 0)
  {
    ERR("Failed to accept a TCP connection: error %d\n", ret);
    return RPC_S_OUT_OF_RESOURCES;
  }
  /* reset to blocking behaviour */
  blocking = 0;
  ret = ioctlsocket(ret, FIONBIO, &blocking);
  client->sock = ret;
  TRACE("Accepted a new TCP connection\n");
  return RPC_S_OK;
}

static int rpcrt4_conn_tcp_read(RpcConnection *Connection,
                                void *buffer, unsigned int count)
{
  RpcConnection_tcp *tcpc = (RpcConnection_tcp *) Connection;
  int r = recv(tcpc->sock, buffer, count, MSG_WAITALL);
  TRACE("%d %p %u -> %d\n", tcpc->sock, buffer, count, r);
  return r;
}

static int rpcrt4_conn_tcp_write(RpcConnection *Connection,
                                 const void *buffer, unsigned int count)
{
  RpcConnection_tcp *tcpc = (RpcConnection_tcp *) Connection;
  int r = write(tcpc->sock, buffer, count);
  TRACE("%d %p %u -> %d\n", tcpc->sock, buffer, count, r);
  return r;
}

static int rpcrt4_conn_tcp_close(RpcConnection *Connection)
{
  RpcConnection_tcp *tcpc = (RpcConnection_tcp *) Connection;

  TRACE("%d\n", tcpc->sock);

  if (tcpc->sock != -1)
    close(tcpc->sock);
  tcpc->sock = -1;
  return 0;
}

static size_t rpcrt4_ncacn_ip_tcp_get_top_of_tower(unsigned char *tower_data,
                                                   const char *networkaddr,
                                                   const char *endpoint)
{
    twr_tcp_floor_t *tcp_floor;
    twr_ipv4_floor_t *ipv4_floor;
    struct addrinfo *ai;
    struct addrinfo hints;
    int ret;
    size_t size = sizeof(*tcp_floor) + sizeof(*ipv4_floor);

    TRACE("(%p, %s, %s)\n", tower_data, networkaddr, endpoint);

    if (!tower_data)
        return size;

    tcp_floor = (twr_tcp_floor_t *)tower_data;
    tower_data += sizeof(*tcp_floor);

    ipv4_floor = (twr_ipv4_floor_t *)tower_data;

    tcp_floor->count_lhs = sizeof(tcp_floor->protid);
    tcp_floor->protid = EPM_PROTOCOL_TCP;
    tcp_floor->count_rhs = sizeof(tcp_floor->port);

    ipv4_floor->count_lhs = sizeof(ipv4_floor->protid);
    ipv4_floor->protid = EPM_PROTOCOL_IP;
    ipv4_floor->count_rhs = sizeof(ipv4_floor->ipv4addr);

    hints.ai_flags          = AI_NUMERICHOST;
    /* FIXME: only support IPv4 at the moment. how is IPv6 represented by the EPM? */
    hints.ai_family         = PF_INET;
    hints.ai_socktype       = SOCK_STREAM;
    hints.ai_protocol       = IPPROTO_TCP;
    hints.ai_addrlen        = 0;
    hints.ai_addr           = NULL;
    hints.ai_canonname      = NULL;
    hints.ai_next           = NULL;

    ret = getaddrinfo(networkaddr, endpoint, &hints, &ai);
    if (ret)
    {
        ret = getaddrinfo("0.0.0.0", endpoint, &hints, &ai);
        if (ret)
        {
            ERR("getaddrinfo failed: %s\n", gai_strerror(ret));
            return 0;
        }
    }

    if (ai->ai_family == PF_INET)
    {
        const struct sockaddr_in *sin = (const struct sockaddr_in *)ai->ai_addr;
        tcp_floor->port = sin->sin_port;
        ipv4_floor->ipv4addr = sin->sin_addr.s_addr;
    }
    else
    {
        ERR("unexpected protocol family %d\n", ai->ai_family);
        return 0;
    }

    freeaddrinfo(ai);

    return size;
}

static RPC_STATUS rpcrt4_ncacn_ip_tcp_parse_top_of_tower(const unsigned char *tower_data,
                                                         size_t tower_size,
                                                         char **networkaddr,
                                                         char **endpoint)
{
    const twr_tcp_floor_t *tcp_floor = (const twr_tcp_floor_t *)tower_data;
    const twr_ipv4_floor_t *ipv4_floor;
    struct in_addr in_addr;

    TRACE("(%p, %d, %p, %p)\n", tower_data, (int)tower_size, networkaddr, endpoint);

    if (tower_size < sizeof(*tcp_floor))
        return EPT_S_NOT_REGISTERED;

    tower_data += sizeof(*tcp_floor);
    tower_size -= sizeof(*tcp_floor);

    if (tower_size < sizeof(*ipv4_floor))
        return EPT_S_NOT_REGISTERED;

    ipv4_floor = (const twr_ipv4_floor_t *)tower_data;

    if ((tcp_floor->count_lhs != sizeof(tcp_floor->protid)) ||
        (tcp_floor->protid != EPM_PROTOCOL_TCP) ||
        (tcp_floor->count_rhs != sizeof(tcp_floor->port)) ||
        (ipv4_floor->count_lhs != sizeof(ipv4_floor->protid)) ||
        (ipv4_floor->protid != EPM_PROTOCOL_IP) ||
        (ipv4_floor->count_rhs != sizeof(ipv4_floor->ipv4addr)))
        return EPT_S_NOT_REGISTERED;

    if (endpoint)
    {
        *endpoint = I_RpcAllocate(6 /* sizeof("65535") + 1 */);
        if (!*endpoint)
            return RPC_S_OUT_OF_RESOURCES;
        sprintf(*endpoint, "%u", ntohs(tcp_floor->port));
    }

    if (networkaddr)
    {
        *networkaddr = I_RpcAllocate(INET_ADDRSTRLEN);
        if (!*networkaddr)
        {
            if (endpoint)
            {
                I_RpcFree(*endpoint);
                *endpoint = NULL;
            }
            return RPC_S_OUT_OF_RESOURCES;
        }
        in_addr.s_addr = ipv4_floor->ipv4addr;
        if (!inet_ntop(AF_INET, &in_addr, *networkaddr, INET_ADDRSTRLEN))
        {
            ERR("inet_ntop: %s\n", strerror(errno));
            I_RpcFree(*networkaddr);
            *networkaddr = NULL;
            if (endpoint)
            {
                I_RpcFree(*endpoint);
                *endpoint = NULL;
            }
            return EPT_S_NOT_REGISTERED;
        }
    }

    return RPC_S_OK;
}

typedef struct _RpcServerProtseq_sock
{
    RpcServerProtseq common;
    int mgr_event_rcv;
    int mgr_event_snd;
} RpcServerProtseq_sock;

static RpcServerProtseq *rpcrt4_protseq_sock_alloc(void)
{
    RpcServerProtseq_sock *ps = HeapAlloc(GetProcessHeap(), 0, sizeof(*ps));
    if (ps)
    {
        int fds[2];
        u_long blocking;
        if (!socketpair(PF_UNIX, SOCK_DGRAM, 0, fds))
        {
            blocking = 1;
            ioctlsocket(fds[0], FIONBIO, &blocking);
            ioctlsocket(fds[1], FIONBIO, &blocking);
            ps->mgr_event_rcv = fds[0];
            ps->mgr_event_snd = fds[1];
        }
        else
        {
            ERR("socketpair failed with error %s\n", strerror(errno));
            HeapFree(GetProcessHeap(), 0, ps);
            return NULL;
        }
    }
    return &ps->common;
}

static void rpcrt4_protseq_sock_signal_state_changed(RpcServerProtseq *protseq)
{
    RpcServerProtseq_sock *sockps = CONTAINING_RECORD(protseq, RpcServerProtseq_sock, common);
    char dummy = 1;
    write(sockps->mgr_event_snd, &dummy, sizeof(dummy));
}

static void *rpcrt4_protseq_sock_get_wait_array(RpcServerProtseq *protseq, void *prev_array, unsigned int *count)
{
    struct pollfd *poll_info = prev_array;
    RpcConnection_tcp *conn;
    RpcServerProtseq_sock *sockps = CONTAINING_RECORD(protseq, RpcServerProtseq_sock, common);

    EnterCriticalSection(&protseq->cs);

    /* open and count connections */
    *count = 1;
    conn = (RpcConnection_tcp *)protseq->conn;
    while (conn) {
        if (conn->sock != -1)
            (*count)++;
        conn = (RpcConnection_tcp *)conn->common.Next;
    }

    /* make array of connections */
    if (poll_info)
        poll_info = HeapReAlloc(GetProcessHeap(), 0, poll_info, *count*sizeof(*poll_info));
    else
        poll_info = HeapAlloc(GetProcessHeap(), 0, *count*sizeof(*poll_info));
    if (!poll_info)
    {
        ERR("couldn't allocate poll_info\n");
        LeaveCriticalSection(&protseq->cs);
        return NULL;
    }

    poll_info[0].fd = sockps->mgr_event_rcv;
    poll_info[0].events = POLLIN;
    *count = 1;
    conn =  CONTAINING_RECORD(protseq->conn, RpcConnection_tcp, common);
    while (conn) {
        if (conn->sock != -1)
        {
            poll_info[*count].fd = conn->sock;
            poll_info[*count].events = POLLIN;
            (*count)++;
        }
        conn = CONTAINING_RECORD(conn->common.Next, RpcConnection_tcp, common);
    }
    LeaveCriticalSection(&protseq->cs);
    return poll_info;
}

static void rpcrt4_protseq_sock_free_wait_array(RpcServerProtseq *protseq, void *array)
{
    HeapFree(GetProcessHeap(), 0, array);
}

static int rpcrt4_protseq_sock_wait_for_new_connection(RpcServerProtseq *protseq, unsigned int count, void *wait_array)
{
    struct pollfd *poll_info = wait_array;
    int ret, i;
    RpcConnection *cconn;
    RpcConnection_tcp *conn;

    if (!poll_info)
        return -1;

    ret = poll(poll_info, count, -1);
    if (ret < 0)
    {
        ERR("poll failed with error %d\n", ret);
        return -1;
    }

    for (i = 0; i < count; i++)
        if (poll_info[i].revents & POLLIN)
        {
            /* RPC server event */
            if (i == 0)
            {
                char dummy;
                read(poll_info[0].fd, &dummy, sizeof(dummy));
                return 0;
            }

            /* find which connection got a RPC */
            EnterCriticalSection(&protseq->cs);
            conn = CONTAINING_RECORD(protseq->conn, RpcConnection_tcp, common);
            while (conn) {
                if (poll_info[i].fd == conn->sock) break;
                conn = CONTAINING_RECORD(conn->common.Next, RpcConnection_tcp, common);
            }
            cconn = NULL;
            if (conn)
                RPCRT4_SpawnConnection(&cconn, &conn->common);
            else
                ERR("failed to locate connection for fd %d\n", poll_info[i].fd);
            LeaveCriticalSection(&protseq->cs);
            if (cconn)
                RPCRT4_new_client(cconn);
            else
                return -1;
        }

    return 1;
}

static const struct connection_ops conn_protseq_list[] = {
  { "ncacn_np",
    { EPM_PROTOCOL_NCACN, EPM_PROTOCOL_SMB },
    rpcrt4_conn_np_alloc,
    rpcrt4_ncacn_np_open,
    rpcrt4_ncacn_np_handoff,
    rpcrt4_conn_np_read,
    rpcrt4_conn_np_write,
    rpcrt4_conn_np_close,
    rpcrt4_ncacn_np_get_top_of_tower,
    rpcrt4_ncacn_np_parse_top_of_tower,
  },
  { "ncalrpc",
    { EPM_PROTOCOL_NCALRPC, EPM_PROTOCOL_PIPE },
    rpcrt4_conn_np_alloc,
    rpcrt4_ncalrpc_open,
    rpcrt4_ncalrpc_handoff,
    rpcrt4_conn_np_read,
    rpcrt4_conn_np_write,
    rpcrt4_conn_np_close,
    rpcrt4_ncalrpc_get_top_of_tower,
    rpcrt4_ncalrpc_parse_top_of_tower,
  },
  { "ncacn_ip_tcp",
    { EPM_PROTOCOL_NCACN, EPM_PROTOCOL_TCP },
    rpcrt4_conn_tcp_alloc,
    rpcrt4_ncacn_ip_tcp_open,
    rpcrt4_conn_tcp_handoff,
    rpcrt4_conn_tcp_read,
    rpcrt4_conn_tcp_write,
    rpcrt4_conn_tcp_close,
    rpcrt4_ncacn_ip_tcp_get_top_of_tower,
    rpcrt4_ncacn_ip_tcp_parse_top_of_tower,
  }
};


static const struct protseq_ops protseq_list[] =
{
    {
        "ncacn_np",
        rpcrt4_protseq_np_alloc,
        rpcrt4_protseq_np_signal_state_changed,
        rpcrt4_protseq_np_get_wait_array,
        rpcrt4_protseq_np_free_wait_array,
        rpcrt4_protseq_np_wait_for_new_connection,
        rpcrt4_protseq_ncacn_np_open_endpoint,
    },
    {
        "ncalrpc",
        rpcrt4_protseq_np_alloc,
        rpcrt4_protseq_np_signal_state_changed,
        rpcrt4_protseq_np_get_wait_array,
        rpcrt4_protseq_np_free_wait_array,
        rpcrt4_protseq_np_wait_for_new_connection,
        rpcrt4_protseq_ncalrpc_open_endpoint,
    },
    {
        "ncacn_ip_tcp",
        rpcrt4_protseq_sock_alloc,
        rpcrt4_protseq_sock_signal_state_changed,
        rpcrt4_protseq_sock_get_wait_array,
        rpcrt4_protseq_sock_free_wait_array,
        rpcrt4_protseq_sock_wait_for_new_connection,
        rpcrt4_protseq_ncacn_ip_tcp_open_endpoint,
    },
};

#define ARRAYSIZE(a) (sizeof((a)) / sizeof((a)[0]))

const struct protseq_ops *rpcrt4_get_protseq_ops(const char *protseq)
{
  int i;
  for(i=0; i<ARRAYSIZE(protseq_list); i++)
    if (!strcmp(protseq_list[i].name, protseq))
      return &protseq_list[i];
  return NULL;
}

static const struct connection_ops *rpcrt4_get_conn_protseq_ops(const char *protseq)
{
    int i;
    for(i=0; i<ARRAYSIZE(conn_protseq_list); i++)
        if (!strcmp(conn_protseq_list[i].name, protseq))
            return &conn_protseq_list[i];
    return NULL;
}

/**** interface to rest of code ****/

RPC_STATUS RPCRT4_OpenClientConnection(RpcConnection* Connection)
{
  TRACE("(Connection == ^%p)\n", Connection);

  assert(!Connection->server);
  return Connection->ops->open_connection_client(Connection);
}

RPC_STATUS RPCRT4_CloseConnection(RpcConnection* Connection)
{
  TRACE("(Connection == ^%p)\n", Connection);
  if (SecIsValidHandle(&Connection->ctx))
  {
    DeleteSecurityContext(&Connection->ctx);
    SecInvalidateHandle(&Connection->ctx);
  }
  rpcrt4_conn_close(Connection);
  return RPC_S_OK;
}

RPC_STATUS RPCRT4_CreateConnection(RpcConnection** Connection, BOOL server,
    LPCSTR Protseq, LPCSTR NetworkAddr, LPCSTR Endpoint,
    LPCWSTR NetworkOptions, RpcAuthInfo* AuthInfo, RpcQualityOfService *QOS)
{
  const struct connection_ops *ops;
  RpcConnection* NewConnection;

  ops = rpcrt4_get_conn_protseq_ops(Protseq);
  if (!ops)
  {
    FIXME("not supported for protseq %s\n", Protseq);
    return RPC_S_PROTSEQ_NOT_SUPPORTED;
  }

  NewConnection = ops->alloc();
  NewConnection->Next = NULL;
  NewConnection->server = server;
  NewConnection->ops = ops;
  NewConnection->NetworkAddr = RPCRT4_strdupA(NetworkAddr);
  NewConnection->Endpoint = RPCRT4_strdupA(Endpoint);
  NewConnection->NetworkOptions = RPCRT4_strdupW(NetworkOptions);
  NewConnection->MaxTransmissionSize = RPC_MAX_PACKET_SIZE;
  memset(&NewConnection->ActiveInterface, 0, sizeof(NewConnection->ActiveInterface));
  NewConnection->NextCallId = 1;

  SecInvalidateHandle(&NewConnection->ctx);
  memset(&NewConnection->exp, 0, sizeof(NewConnection->exp));
  NewConnection->attr = 0;
  if (AuthInfo) RpcAuthInfo_AddRef(AuthInfo);
  NewConnection->AuthInfo = AuthInfo;
  NewConnection->encryption_auth_len = 0;
  NewConnection->signature_auth_len = 0;
  if (QOS) RpcQualityOfService_AddRef(QOS);
  NewConnection->QOS = QOS;

  list_init(&NewConnection->conn_pool_entry);

  TRACE("connection: %p\n", NewConnection);
  *Connection = NewConnection;

  return RPC_S_OK;
}

RPC_STATUS RPCRT4_GetAssociation(LPCSTR Protseq, LPCSTR NetworkAddr,
                                 LPCSTR Endpoint, LPCWSTR NetworkOptions,
                                 RpcAssoc **assoc_out)
{
  RpcAssoc *assoc;

  EnterCriticalSection(&assoc_list_cs);
  LIST_FOR_EACH_ENTRY(assoc, &assoc_list, RpcAssoc, entry)
  {
    if (!strcmp(Protseq, assoc->Protseq) &&
        !strcmp(NetworkAddr, assoc->NetworkAddr) &&
        !strcmp(Endpoint, assoc->Endpoint) &&
        ((!assoc->NetworkOptions && !NetworkOptions) || !strcmpW(NetworkOptions, assoc->NetworkOptions)))
    {
      assoc->refs++;
      *assoc_out = assoc;
      LeaveCriticalSection(&assoc_list_cs);
      TRACE("using existing assoc %p\n", assoc);
      return RPC_S_OK;
    }
  }

  assoc = HeapAlloc(GetProcessHeap(), 0, sizeof(*assoc));
  if (!assoc)
  {
    LeaveCriticalSection(&assoc_list_cs);
    return RPC_S_OUT_OF_RESOURCES;
  }
  assoc->refs = 1;
  list_init(&assoc->connection_pool);
  InitializeCriticalSection(&assoc->cs);
  assoc->Protseq = RPCRT4_strdupA(Protseq);
  assoc->NetworkAddr = RPCRT4_strdupA(NetworkAddr);
  assoc->Endpoint = RPCRT4_strdupA(Endpoint);
  assoc->NetworkOptions = NetworkOptions ? RPCRT4_strdupW(NetworkOptions) : NULL;
  assoc->assoc_group_id = 0;
  list_add_head(&assoc_list, &assoc->entry);
  *assoc_out = assoc;

  LeaveCriticalSection(&assoc_list_cs);

  TRACE("new assoc %p\n", assoc);

  return RPC_S_OK;
}

ULONG RpcAssoc_Release(RpcAssoc *assoc)
{
  ULONG refs;

  EnterCriticalSection(&assoc_list_cs);
  refs = --assoc->refs;
  if (!refs)
    list_remove(&assoc->entry);
  LeaveCriticalSection(&assoc_list_cs);

  if (!refs)
  {
    RpcConnection *Connection, *cursor2;

    TRACE("destroying assoc %p\n", assoc);

    LIST_FOR_EACH_ENTRY_SAFE(Connection, cursor2, &assoc->connection_pool, RpcConnection, conn_pool_entry)
    {
      list_remove(&Connection->conn_pool_entry);
      RPCRT4_DestroyConnection(Connection);
    }

    HeapFree(GetProcessHeap(), 0, assoc->NetworkOptions);
    HeapFree(GetProcessHeap(), 0, assoc->Endpoint);
    HeapFree(GetProcessHeap(), 0, assoc->NetworkAddr);
    HeapFree(GetProcessHeap(), 0, assoc->Protseq);

    HeapFree(GetProcessHeap(), 0, assoc);
  }

  return refs;
}

#define ROUND_UP(value, alignment) (((value) + ((alignment) - 1)) & ~((alignment)-1))

static RPC_STATUS RpcAssoc_BindConnection(const RpcAssoc *assoc, RpcConnection *conn,
                                          const RPC_SYNTAX_IDENTIFIER *InterfaceId,
                                          const RPC_SYNTAX_IDENTIFIER *TransferSyntax)
{
  RpcPktHdr *hdr;
  RpcPktHdr *response_hdr;
  RPC_MESSAGE msg;
  RPC_STATUS status;

  TRACE("sending bind request to server\n");

  hdr = RPCRT4_BuildBindHeader(NDR_LOCAL_DATA_REPRESENTATION,
                               RPC_MAX_PACKET_SIZE, RPC_MAX_PACKET_SIZE,
                               assoc->assoc_group_id,
                               InterfaceId, TransferSyntax);

  status = RPCRT4_Send(conn, hdr, NULL, 0);
  RPCRT4_FreeHeader(hdr);
  if (status != RPC_S_OK)
    return status;

  status = RPCRT4_Receive(conn, &response_hdr, &msg);
  if (status != RPC_S_OK)
  {
    ERR("receive failed\n");
    return status;
  }

  switch (response_hdr->common.ptype)
  {
  case PKT_BIND_ACK:
  {
    RpcAddressString *server_address = msg.Buffer;
    if ((msg.BufferLength >= FIELD_OFFSET(RpcAddressString, string[0])) ||
        (msg.BufferLength >= ROUND_UP(FIELD_OFFSET(RpcAddressString, string[server_address->length]), 4)))
    {
        unsigned short remaining = msg.BufferLength -
            ROUND_UP(FIELD_OFFSET(RpcAddressString, string[server_address->length]), 4);
        RpcResults *results = (RpcResults*)((ULONG_PTR)server_address +
            ROUND_UP(FIELD_OFFSET(RpcAddressString, string[server_address->length]), 4));
        if ((results->num_results == 1) && (remaining >= sizeof(*results)))
        {
            switch (results->results[0].result)
            {
            case RESULT_ACCEPT:
                conn->assoc_group_id = response_hdr->bind_ack.assoc_gid;
                conn->MaxTransmissionSize = response_hdr->bind_ack.max_tsize;
                conn->ActiveInterface = *InterfaceId;
                break;
            case RESULT_PROVIDER_REJECTION:
                switch (results->results[0].reason)
                {
                case REASON_ABSTRACT_SYNTAX_NOT_SUPPORTED:
                    ERR("syntax %s, %d.%d not supported\n",
                        debugstr_guid(&InterfaceId->SyntaxGUID),
                        InterfaceId->SyntaxVersion.MajorVersion,
                        InterfaceId->SyntaxVersion.MinorVersion);
                    status = RPC_S_UNKNOWN_IF;
                    break;
                case REASON_TRANSFER_SYNTAXES_NOT_SUPPORTED:
                    ERR("transfer syntax not supported\n");
                    status = RPC_S_SERVER_UNAVAILABLE;
                    break;
                case REASON_NONE:
                default:
                    status = RPC_S_CALL_FAILED_DNE;
                }
                break;
            case RESULT_USER_REJECTION:
            default:
                ERR("rejection result %d\n", results->results[0].result);
                status = RPC_S_CALL_FAILED_DNE;
            }
        }
        else
        {
            ERR("incorrect results size\n");
            status = RPC_S_CALL_FAILED_DNE;
        }
    }
    else
    {
        ERR("bind ack packet too small (%d)\n", msg.BufferLength);
        status = RPC_S_PROTOCOL_ERROR;
    }
    break;
  }
  case PKT_BIND_NACK:
    switch (response_hdr->bind_nack.reject_reason)
    {
    case REJECT_LOCAL_LIMIT_EXCEEDED:
    case REJECT_TEMPORARY_CONGESTION:
        ERR("server too busy\n");
        status = RPC_S_SERVER_TOO_BUSY;
        break;
    case REJECT_PROTOCOL_VERSION_NOT_SUPPORTED:
        ERR("protocol version not supported\n");
        status = RPC_S_PROTOCOL_ERROR;
        break;
    case REJECT_UNKNOWN_AUTHN_SERVICE:
        ERR("unknown authentication service\n");
        status = RPC_S_UNKNOWN_AUTHN_SERVICE;
        break;
    case REJECT_INVALID_CHECKSUM:
        ERR("invalid checksum\n");
        status = ERROR_ACCESS_DENIED;
        break;
    default:
        ERR("rejected bind for reason %d\n", response_hdr->bind_nack.reject_reason);
        status = RPC_S_CALL_FAILED_DNE;
    }
    break;
  default:
    ERR("wrong packet type received %d\n", response_hdr->common.ptype);
    status = RPC_S_PROTOCOL_ERROR;
    break;
  }

  RPCRT4_FreeHeader(response_hdr);
  return status;
}

static RpcConnection *RpcAssoc_GetIdleConnection(RpcAssoc *assoc,
    const RPC_SYNTAX_IDENTIFIER *InterfaceId,
    const RPC_SYNTAX_IDENTIFIER *TransferSyntax, const RpcAuthInfo *AuthInfo,
    const RpcQualityOfService *QOS)
{
  RpcConnection *Connection;
  EnterCriticalSection(&assoc->cs);
  /* try to find a compatible connection from the connection pool */
  LIST_FOR_EACH_ENTRY(Connection, &assoc->connection_pool, RpcConnection, conn_pool_entry)
  {
    if (!memcmp(&Connection->ActiveInterface, InterfaceId,
                sizeof(RPC_SYNTAX_IDENTIFIER)) &&
        RpcAuthInfo_IsEqual(Connection->AuthInfo, AuthInfo) &&
        RpcQualityOfService_IsEqual(Connection->QOS, QOS))
    {
      list_remove(&Connection->conn_pool_entry);
      LeaveCriticalSection(&assoc->cs);
      TRACE("got connection from pool %p\n", Connection);
      return Connection;
    }
  }

  LeaveCriticalSection(&assoc->cs);
  return NULL;
}

RPC_STATUS RpcAssoc_GetClientConnection(RpcAssoc *assoc,
    const RPC_SYNTAX_IDENTIFIER *InterfaceId,
    const RPC_SYNTAX_IDENTIFIER *TransferSyntax, RpcAuthInfo *AuthInfo,
    RpcQualityOfService *QOS, RpcConnection **Connection)
{
  RpcConnection *NewConnection;
  RPC_STATUS status;

  *Connection = RpcAssoc_GetIdleConnection(assoc, InterfaceId, TransferSyntax, AuthInfo, QOS);
  if (*Connection)
    return RPC_S_OK;

  /* create a new connection */
  status = RPCRT4_CreateConnection(&NewConnection, FALSE /* is this a server connection? */,
                                   assoc->Protseq, assoc->NetworkAddr,
                                   assoc->Endpoint, assoc->NetworkOptions,
                                   AuthInfo, QOS);
  if (status != RPC_S_OK)
    return status;

  status = RPCRT4_OpenClientConnection(NewConnection);
  if (status != RPC_S_OK)
  {
    RPCRT4_DestroyConnection(NewConnection);
    return status;
  }

  status = RpcAssoc_BindConnection(assoc, NewConnection, InterfaceId, TransferSyntax);
  if (status != RPC_S_OK)
  {
    RPCRT4_DestroyConnection(NewConnection);
    return status;
  }

  *Connection = NewConnection;

  return RPC_S_OK;
}

void RpcAssoc_ReleaseIdleConnection(RpcAssoc *assoc, RpcConnection *Connection)
{
  assert(!Connection->server);
  EnterCriticalSection(&assoc->cs);
  if (!assoc->assoc_group_id) assoc->assoc_group_id = Connection->assoc_group_id;
  list_add_head(&assoc->connection_pool, &Connection->conn_pool_entry);
  LeaveCriticalSection(&assoc->cs);
}


RPC_STATUS RPCRT4_SpawnConnection(RpcConnection** Connection, RpcConnection* OldConnection)
{
  RPC_STATUS err;

  err = RPCRT4_CreateConnection(Connection, OldConnection->server,
                                rpcrt4_conn_get_name(OldConnection),
                                OldConnection->NetworkAddr,
                                OldConnection->Endpoint, NULL,
                                OldConnection->AuthInfo, OldConnection->QOS);
  if (err == RPC_S_OK)
    rpcrt4_conn_handoff(OldConnection, *Connection);
  return err;
}

RPC_STATUS RPCRT4_DestroyConnection(RpcConnection* Connection)
{
  TRACE("connection: %p\n", Connection);

  RPCRT4_CloseConnection(Connection);
  RPCRT4_strfree(Connection->Endpoint);
  RPCRT4_strfree(Connection->NetworkAddr);
  HeapFree(GetProcessHeap(), 0, Connection->NetworkOptions);
  if (Connection->AuthInfo) RpcAuthInfo_Release(Connection->AuthInfo);
  if (Connection->QOS) RpcQualityOfService_Release(Connection->QOS);
  HeapFree(GetProcessHeap(), 0, Connection);
  return RPC_S_OK;
}

RPC_STATUS RpcTransport_GetTopOfTower(unsigned char *tower_data,
                                      size_t *tower_size,
                                      const char *protseq,
                                      const char *networkaddr,
                                      const char *endpoint)
{
    twr_empty_floor_t *protocol_floor;
    const struct connection_ops *protseq_ops = rpcrt4_get_conn_protseq_ops(protseq);

    *tower_size = 0;

    if (!protseq_ops)
        return RPC_S_INVALID_RPC_PROTSEQ;

    if (!tower_data)
    {
        *tower_size = sizeof(*protocol_floor);
        *tower_size += protseq_ops->get_top_of_tower(NULL, networkaddr, endpoint);
        return RPC_S_OK;
    }

    protocol_floor = (twr_empty_floor_t *)tower_data;
    protocol_floor->count_lhs = sizeof(protocol_floor->protid);
    protocol_floor->protid = protseq_ops->epm_protocols[0];
    protocol_floor->count_rhs = 0;

    tower_data += sizeof(*protocol_floor);

    *tower_size = protseq_ops->get_top_of_tower(tower_data, networkaddr, endpoint);
    if (!*tower_size)
        return EPT_S_NOT_REGISTERED;

    *tower_size += sizeof(*protocol_floor);

    return RPC_S_OK;
}

RPC_STATUS RpcTransport_ParseTopOfTower(const unsigned char *tower_data,
                                        size_t tower_size,
                                        char **protseq,
                                        char **networkaddr,
                                        char **endpoint)
{
    const twr_empty_floor_t *protocol_floor;
    const twr_empty_floor_t *floor4;
    const struct connection_ops *protseq_ops = NULL;
    RPC_STATUS status;
    int i;

    if (tower_size < sizeof(*protocol_floor))
        return EPT_S_NOT_REGISTERED;

    protocol_floor = (const twr_empty_floor_t *)tower_data;
    tower_data += sizeof(*protocol_floor);
    tower_size -= sizeof(*protocol_floor);
    if ((protocol_floor->count_lhs != sizeof(protocol_floor->protid)) ||
        (protocol_floor->count_rhs > tower_size))
        return EPT_S_NOT_REGISTERED;
    tower_data += protocol_floor->count_rhs;
    tower_size -= protocol_floor->count_rhs;

    floor4 = (const twr_empty_floor_t *)tower_data;
    if ((tower_size < sizeof(*floor4)) ||
        (floor4->count_lhs != sizeof(floor4->protid)))
        return EPT_S_NOT_REGISTERED;

    for(i = 0; i < ARRAYSIZE(conn_protseq_list); i++)
        if ((protocol_floor->protid == conn_protseq_list[i].epm_protocols[0]) &&
            (floor4->protid == conn_protseq_list[i].epm_protocols[1]))
        {
            protseq_ops = &conn_protseq_list[i];
            break;
        }

    if (!protseq_ops)
        return EPT_S_NOT_REGISTERED;

    status = protseq_ops->parse_top_of_tower(tower_data, tower_size, networkaddr, endpoint);

    if ((status == RPC_S_OK) && protseq)
    {
        *protseq = I_RpcAllocate(strlen(protseq_ops->name) + 1);
        strcpy(*protseq, protseq_ops->name);
    }

    return status;
}

/***********************************************************************
 *             RpcNetworkIsProtseqValidW (RPCRT4.@)
 *
 * Checks if the given protocol sequence is known by the RPC system.
 * If it is, returns RPC_S_OK, otherwise RPC_S_PROTSEQ_NOT_SUPPORTED.
 *
 */
RPC_STATUS WINAPI RpcNetworkIsProtseqValidW(RPC_WSTR protseq)
{
  char ps[0x10];

  WideCharToMultiByte(CP_ACP, 0, protseq, -1,
                      ps, sizeof ps, NULL, NULL);
  if (rpcrt4_get_conn_protseq_ops(ps))
    return RPC_S_OK;

  FIXME("Unknown protseq %s\n", debugstr_w(protseq));

  return RPC_S_INVALID_RPC_PROTSEQ;
}

/***********************************************************************
 *             RpcNetworkIsProtseqValidA (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcNetworkIsProtseqValidA(RPC_CSTR protseq)
{
  UNICODE_STRING protseqW;

  if (RtlCreateUnicodeStringFromAsciiz(&protseqW, (char*)protseq))
  {
    RPC_STATUS ret = RpcNetworkIsProtseqValidW(protseqW.Buffer);
    RtlFreeUnicodeString(&protseqW);
    return ret;
  }
  return RPC_S_OUT_OF_MEMORY;
}
