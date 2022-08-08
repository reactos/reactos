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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#ifdef __REACTOS__
#define NONAMELESSUNION
#endif
#include "ws2tcpip.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winerror.h"
#include "wininet.h"
#include "wine/winternl.h"
#include "winioctl.h"

#include "rpc.h"
#include "rpcndr.h"

#include "wine/debug.h"

#include "rpc_binding.h"
#include "rpc_assoc.h"
#include "rpc_message.h"
#include "rpc_server.h"
#include "epm_towers.h"

#define DEFAULT_NCACN_HTTP_TIMEOUT (60 * 1000)

WINE_DEFAULT_DEBUG_CHANNEL(rpc);

#ifdef __REACTOS__ /* FIXME: Inspect */
BOOL WINAPI CancelIoEx(HANDLE handle, LPOVERLAPPED lpOverlapped)
{
     IO_STATUS_BLOCK    io_status;

    NtCancelIoFile(handle, &io_status);
    if (io_status.u.Status)
    {
        SetLastError( RtlNtStatusToDosError( io_status.u.Status ) );
        return FALSE;
    }
    return TRUE;
}
#endif

static RpcConnection *rpcrt4_spawn_connection(RpcConnection *old_connection);

/**** ncacn_np support ****/

typedef struct _RpcConnection_np
{
    RpcConnection common;
    HANDLE pipe;
    HANDLE listen_event;
    char *listen_pipe;
    IO_STATUS_BLOCK io_status;
    HANDLE event_cache;
    BOOL read_closed;
} RpcConnection_np;

static RpcConnection *rpcrt4_conn_np_alloc(void)
{
  RpcConnection_np *npc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RpcConnection_np));
  return &npc->common;
}

static HANDLE get_np_event(RpcConnection_np *connection)
{
    HANDLE event = InterlockedExchangePointer(&connection->event_cache, NULL);
    return event ? event : CreateEventW(NULL, TRUE, FALSE, NULL);
}

static void release_np_event(RpcConnection_np *connection, HANDLE event)
{
    event = InterlockedExchangePointer(&connection->event_cache, event);
    if (event)
        CloseHandle(event);
}

#ifdef __REACTOS__
/**
 * @brief
 * Creates a security descriptor for RPC4 pipe
 *
 * @param[out] SecDesc
 * A pointer to an allocated security descriptor.
 *
 * @return
 * ERROR_SUCCESS is returned if the function has
 * successfully created the security descriptor,
 * otherwise a Win32 error code is returned.
 *
 * @remarks
 * Everyone (aka World SID) and anonynous users
 * are given a subset of rights to access the pipe,
 * whereas admins are given full power.
 */
static DWORD rpcrt4_create_pipe_security(PSECURITY_DESCRIPTOR *SecDesc)
{
    DWORD ErrCode;
    PACL Dacl;
    ULONG DaclSize, RelSDSize = 0;
    PSID EveryoneSid = NULL, AnonymousSid = NULL, AdminsSid = NULL;
    PSECURITY_DESCRIPTOR AbsSD = NULL, RelSD = NULL;
    static SID_IDENTIFIER_AUTHORITY WorldAuthority = {SECURITY_WORLD_SID_AUTHORITY};
    static SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};

    if (!AllocateAndInitializeSid(&WorldAuthority,
                                  1,
                                  SECURITY_WORLD_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &EveryoneSid))
    {
       ERR("rpcrt4_create_pipe_security(): Failed to allocate Everyone SID (error code %d)\n", GetLastError());
       return GetLastError();
    }

    if (!AllocateAndInitializeSid(&NtAuthority,
                                  1,
                                  SECURITY_ANONYMOUS_LOGON_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &AnonymousSid))
    {
        ERR("rpcrt4_create_pipe_security(): Failed to allocate Anonymous SID (error code %d)\n", GetLastError());
        ErrCode = GetLastError();
        goto Quit;
    }

    if (!AllocateAndInitializeSid(&NtAuthority,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0,
                                  &AdminsSid))
    {
        ERR("rpcrt4_create_pipe_security(): Failed to allocate Admins SID (error code %d)\n", GetLastError());
        ErrCode = GetLastError();
        goto Quit;
    }

    AbsSD = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SECURITY_DESCRIPTOR));
    if (AbsSD == NULL)
    {
        ERR("rpcrt4_create_pipe_security(): Failed to allocate absolute SD!\n");
        ErrCode = ERROR_OUTOFMEMORY;
        goto Quit;
    }

    if (!InitializeSecurityDescriptor(AbsSD, SECURITY_DESCRIPTOR_REVISION))
    {
        ERR("rpcrt4_create_pipe_security(): Failed to create absolute SD (error code %d)\n", GetLastError());
        ErrCode = GetLastError();
        goto Quit;
    }

    DaclSize = sizeof(ACL) +
               sizeof(ACCESS_ALLOWED_ACE) + RtlLengthSid(EveryoneSid) +
               sizeof(ACCESS_ALLOWED_ACE) + RtlLengthSid(AnonymousSid) +
               sizeof(ACCESS_ALLOWED_ACE) + RtlLengthSid(AdminsSid);


    Dacl = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, DaclSize);
    if (Dacl == NULL)
    {
        ERR("rpcrt4_create_pipe_security(): Failed to allocate DACL!\n");
        ErrCode = ERROR_OUTOFMEMORY;
        goto Quit;
    }

    if (!InitializeAcl(Dacl, DaclSize, ACL_REVISION))
    {
        ERR("rpcrt4_create_pipe_security(): Failed to create DACL (error code %d)\n", GetLastError());
        ErrCode = GetLastError();
        goto Quit;
    }

    if (!AddAccessAllowedAce(Dacl,
                             ACL_REVISION,
                             GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE | READ_CONTROL,
                             EveryoneSid))
    {
        ERR("rpcrt4_create_pipe_security(): Failed to set up ACE for Everyone SID (error code %d)\n", GetLastError());
        ErrCode = GetLastError();
        goto Quit;
    }

    if (!AddAccessAllowedAce(Dacl,
                             ACL_REVISION,
                             GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE | READ_CONTROL,
                             AnonymousSid))
    {
        ERR("rpcrt4_create_pipe_security(): Failed to set up ACE for Anonymous SID (error code %d)\n", GetLastError());
        ErrCode = GetLastError();
        goto Quit;
    }

    if (!AddAccessAllowedAce(Dacl,
                             ACL_REVISION,
                             GENERIC_ALL,
                             AdminsSid))
    {
        ERR("rpcrt4_create_pipe_security(): Failed to set up ACE for Admins SID (error code %d)\n", GetLastError());
        ErrCode = GetLastError();
        goto Quit;
    }

    if (!SetSecurityDescriptorDacl(AbsSD, TRUE, Dacl, FALSE))
    {
        ERR("rpcrt4_create_pipe_security(): Failed to set DACL to absolute SD (error code %d)\n", GetLastError());
        ErrCode = GetLastError();
        goto Quit;
    }

    if (!SetSecurityDescriptorOwner(AbsSD, AdminsSid, FALSE))
    {
        ERR("rpcrt4_create_pipe_security(): Failed to set SD owner (error code %d)\n", GetLastError());
        ErrCode = GetLastError();
        goto Quit;
    }

    if (!SetSecurityDescriptorGroup(AbsSD, AdminsSid, FALSE))
    {
        ERR("rpcrt4_create_pipe_security(): Failed to set SD group (error code %d)\n", GetLastError());
        ErrCode = GetLastError();
        goto Quit;
    }

    if (!MakeSelfRelativeSD(AbsSD, NULL, &RelSDSize) && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        ERR("rpcrt4_create_pipe_security(): Unexpected error code (error code %d -- must be ERROR_INSUFFICIENT_BUFFER)\n", GetLastError());
        ErrCode = GetLastError();
        goto Quit;
    }

    RelSD = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, RelSDSize);
    if (RelSD == NULL)
    {
        ERR("rpcrt4_create_pipe_security(): Failed to allocate relative SD!\n");
        ErrCode = ERROR_OUTOFMEMORY;
        goto Quit;
    }

    if (!MakeSelfRelativeSD(AbsSD, RelSD, &RelSDSize) && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        ERR("rpcrt4_create_pipe_security(): Failed to allocate relative SD, buffer too smal (expected size %lu)\n", RelSDSize);
        ErrCode = ERROR_INSUFFICIENT_BUFFER;
        goto Quit;
    }

    TRACE("rpcrt4_create_pipe_security(): Success!\n");
    *SecDesc = RelSD;
    ErrCode = ERROR_SUCCESS;

Quit:
    if (ErrCode != ERROR_SUCCESS)
    {
        if (RelSD != NULL)
        {
            HeapFree(GetProcessHeap(), 0, RelSD);
        }
    }

    if (EveryoneSid != NULL)
    {
        FreeSid(EveryoneSid);
    }

    if (AnonymousSid != NULL)
    {
        FreeSid(AnonymousSid);
    }

    if (AdminsSid != NULL)
    {
        FreeSid(AdminsSid);
    }

    if (Dacl != NULL)
    {
        HeapFree(GetProcessHeap(), 0, Dacl);
    }

    if (AbsSD != NULL)
    {
        HeapFree(GetProcessHeap(), 0, AbsSD);
    }

    return ErrCode;
}
#endif

static RPC_STATUS rpcrt4_conn_create_pipe(RpcConnection *conn)
{
    RpcConnection_np *connection = (RpcConnection_np *) conn;
#ifdef __REACTOS__
    DWORD ErrCode;
    SECURITY_ATTRIBUTES SecurityAttributes;
    PSECURITY_DESCRIPTOR PipeSecDesc;
#endif

    TRACE("listening on %s\n", connection->listen_pipe);

#ifdef __REACTOS__
    ErrCode = rpcrt4_create_pipe_security(&PipeSecDesc);
    if (ErrCode != ERROR_SUCCESS)
    {
        ERR("rpcrt4_conn_create_pipe(): Pipe security descriptor creation failed!\n");
        return RPC_S_CANT_CREATE_ENDPOINT;
    }

    SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    SecurityAttributes.lpSecurityDescriptor = PipeSecDesc;
    SecurityAttributes.bInheritHandle = FALSE;

    connection->pipe = CreateNamedPipeA(connection->listen_pipe, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
                                        PIPE_UNLIMITED_INSTANCES,
                                        RPC_MAX_PACKET_SIZE, RPC_MAX_PACKET_SIZE, 5000, &SecurityAttributes);
    HeapFree(GetProcessHeap(), 0, PipeSecDesc);
#else
    connection->pipe = CreateNamedPipeA(connection->listen_pipe, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
                                        PIPE_UNLIMITED_INSTANCES,
                                        RPC_MAX_PACKET_SIZE, RPC_MAX_PACKET_SIZE, 5000, NULL);
#endif
    if (connection->pipe == INVALID_HANDLE_VALUE)
    {
        WARN("CreateNamedPipe failed with error %d\n", GetLastError());
        if (GetLastError() == ERROR_FILE_EXISTS)
        {
            return RPC_S_DUPLICATE_ENDPOINT;
        }
        else
        {
            return RPC_S_CANT_CREATE_ENDPOINT;
        }
    }

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
        if (Connection->QOS->qos->IdentityTracking == RPC_C_QOS_IDENTITY_DYNAMIC)
            dwFlags |= SECURITY_CONTEXT_TRACKING;
    }
    pipe = CreateFileA(pname, GENERIC_READ|GENERIC_WRITE, 0, NULL,
                       OPEN_EXISTING, dwFlags | FILE_FLAG_OVERLAPPED, 0);
    if (pipe != INVALID_HANDLE_VALUE) break;
    err = GetLastError();
    if (err == ERROR_PIPE_BUSY) {
      if (WaitNamedPipeA(pname, NMPWAIT_USE_DEFAULT_WAIT)) {
        TRACE("retrying busy server\n");
        continue;
      }
      TRACE("connection failed, error=%x\n", err);
      return RPC_S_SERVER_TOO_BUSY;
#ifdef __REACTOS__
    } else if (err == ERROR_BAD_NETPATH) {
      TRACE("connection failed, error=%x\n", err);
      return RPC_S_SERVER_UNAVAILABLE;
#endif
    }
    if (!wait || !WaitNamedPipeA(pname, NMPWAIT_WAIT_FOREVER)) {
      err = GetLastError();
      WARN("connection failed, error=%x\n", err);
      return RPC_S_SERVER_UNAVAILABLE;
    }
  }

  /* success */
  /* pipe is connected; change to message-read mode. */
  dwMode = PIPE_READMODE_MESSAGE;
  SetNamedPipeHandleState(pipe, &dwMode, NULL, NULL);
  npc->pipe = pipe;

  return RPC_S_OK;
}

static char *ncalrpc_pipe_name(const char *endpoint)
{
  static const char prefix[] = "\\\\.\\pipe\\lrpc\\";
  char *pipe_name;

  /* protseq=ncalrpc: supposed to use NT LPC ports,
   * but we'll implement it with named pipes for now */
  pipe_name = I_RpcAllocate(sizeof(prefix) + strlen(endpoint));
  strcat(strcpy(pipe_name, prefix), endpoint);
  return pipe_name;
}

static RPC_STATUS rpcrt4_ncalrpc_open(RpcConnection* Connection)
{
  RpcConnection_np *npc = (RpcConnection_np *) Connection;
  RPC_STATUS r;
  LPSTR pname;

  /* already connected? */
  if (npc->pipe)
    return RPC_S_OK;

  pname = ncalrpc_pipe_name(Connection->Endpoint);
  r = rpcrt4_conn_open_pipe(Connection, pname, TRUE);
  I_RpcFree(pname);

  return r;
}

static RPC_STATUS rpcrt4_protseq_ncalrpc_open_endpoint(RpcServerProtseq* protseq, const char *endpoint)
{
  RPC_STATUS r;
  RpcConnection *Connection;
  char generated_endpoint[22];

  if (!endpoint)
  {
    static LONG lrpc_nameless_id;
    DWORD process_id = GetCurrentProcessId();
    ULONG id = InterlockedIncrement(&lrpc_nameless_id);
    snprintf(generated_endpoint, sizeof(generated_endpoint),
             "LRPC%08x.%08x", process_id, id);
    endpoint = generated_endpoint;
  }

  r = RPCRT4_CreateConnection(&Connection, TRUE, protseq->Protseq, NULL,
                              endpoint, NULL, NULL, NULL, NULL);
  if (r != RPC_S_OK)
      return r;

  ((RpcConnection_np*)Connection)->listen_pipe = ncalrpc_pipe_name(Connection->Endpoint);
  r = rpcrt4_conn_create_pipe(Connection);

  EnterCriticalSection(&protseq->cs);
  list_add_head(&protseq->listeners, &Connection->protseq_entry);
  Connection->protseq = protseq;
  LeaveCriticalSection(&protseq->cs);

  return r;
}

#ifdef __REACTOS__
static char *ncacn_pipe_name(const char *server, const char *endpoint)
#else
static char *ncacn_pipe_name(const char *endpoint)
#endif
{
#ifdef __REACTOS__
  static const char prefix[] = "\\\\";
  static const char local[] = ".";
  char ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
  DWORD bufLen = ARRAY_SIZE(ComputerName);
#else
  static const char prefix[] = "\\\\.";
#endif
  char *pipe_name;

#ifdef __REACTOS__
  if (server != NULL && *server != 0)
  {
    /* Trim any leading UNC server prefix. */
    if (server[0] == '\\' && server[1] == '\\')
      server += 2;

    /* If the server represents the local computer, use instead
     * the local prefix to avoid a round in UNC name resolution. */
    if (GetComputerNameA(ComputerName, &bufLen) &&
        (stricmp(ComputerName, server) == 0))
    {
      server = local;
    }
  }
  else
  {
    server = local;
  }
#endif

  /* protseq=ncacn_np: named pipes */
#ifdef __REACTOS__
  pipe_name = I_RpcAllocate(sizeof(prefix) + strlen(server) + strlen(endpoint));
  strcpy(pipe_name, prefix);
  strcat(pipe_name, server);
  strcat(pipe_name, endpoint);
#else
  pipe_name = I_RpcAllocate(sizeof(prefix) + strlen(endpoint));
  strcat(strcpy(pipe_name, prefix), endpoint);
#endif
  return pipe_name;
}

static RPC_STATUS rpcrt4_ncacn_np_open(RpcConnection* Connection)
{
  RpcConnection_np *npc = (RpcConnection_np *) Connection;
  RPC_STATUS r;
  LPSTR pname;

  /* already connected? */
  if (npc->pipe)
    return RPC_S_OK;

#ifdef __REACTOS__
  pname = ncacn_pipe_name(Connection->NetworkAddr, Connection->Endpoint);
#else
  pname = ncacn_pipe_name(Connection->Endpoint);
#endif
  r = rpcrt4_conn_open_pipe(Connection, pname, FALSE);
  I_RpcFree(pname);

  return r;
}

static RPC_STATUS rpcrt4_protseq_ncacn_np_open_endpoint(RpcServerProtseq *protseq, const char *endpoint)
{
  RPC_STATUS r;
  RpcConnection *Connection;
  char generated_endpoint[26];

  if (!endpoint)
  {
    static LONG np_nameless_id;
    DWORD process_id = GetCurrentProcessId();
    ULONG id = InterlockedExchangeAdd(&np_nameless_id, 1 );
    snprintf(generated_endpoint, sizeof(generated_endpoint),
             "\\\\pipe\\\\%08x.%03x", process_id, id);
    endpoint = generated_endpoint;
  }

  r = RPCRT4_CreateConnection(&Connection, TRUE, protseq->Protseq, NULL,
                              endpoint, NULL, NULL, NULL, NULL);
  if (r != RPC_S_OK)
    return r;

#ifdef __REACTOS__
  ((RpcConnection_np*)Connection)->listen_pipe = ncacn_pipe_name(NULL, Connection->Endpoint);
#else
  ((RpcConnection_np*)Connection)->listen_pipe = ncacn_pipe_name(Connection->Endpoint);
#endif
  r = rpcrt4_conn_create_pipe(Connection);

  EnterCriticalSection(&protseq->cs);
  list_add_head(&protseq->listeners, &Connection->protseq_entry);
  Connection->protseq = protseq;
  LeaveCriticalSection(&protseq->cs);

  return r;
}

static void rpcrt4_conn_np_handoff(RpcConnection_np *old_npc, RpcConnection_np *new_npc)
{    
    /* because of the way named pipes work, we'll transfer the connected pipe
     * to the child, then reopen the server binding to continue listening */

    new_npc->pipe = old_npc->pipe;
    old_npc->pipe = 0;
    assert(!old_npc->listen_event);
}

static RPC_STATUS rpcrt4_ncacn_np_handoff(RpcConnection *old_conn, RpcConnection *new_conn)
{
  DWORD len = MAX_COMPUTERNAME_LENGTH + 1;
  RPC_STATUS status;

  rpcrt4_conn_np_handoff((RpcConnection_np *)old_conn, (RpcConnection_np *)new_conn);
  status = rpcrt4_conn_create_pipe(old_conn);

  /* Store the local computer name as the NetworkAddr for ncacn_np as long as
   * we don't support named pipes over the network. */
  new_conn->NetworkAddr = HeapAlloc(GetProcessHeap(), 0, len);
  if (!GetComputerNameA(new_conn->NetworkAddr, &len))
  {
    ERR("Failed to retrieve the computer name, error %u\n", GetLastError());
    return RPC_S_OUT_OF_RESOURCES;
  }

  return status;
}

static RPC_STATUS is_pipe_listening(const char *pipe_name)
{
  return WaitNamedPipeA(pipe_name, 1) ? RPC_S_OK : RPC_S_NOT_LISTENING;
}

static RPC_STATUS rpcrt4_ncacn_np_is_server_listening(const char *endpoint)
{
  char *pipe_name;
  RPC_STATUS status;

#ifdef __REACTOS__
  pipe_name = ncacn_pipe_name(NULL, endpoint);
#else
  pipe_name = ncacn_pipe_name(endpoint);
#endif
  status = is_pipe_listening(pipe_name);
  I_RpcFree(pipe_name);
  return status;
}

static RPC_STATUS rpcrt4_ncalrpc_np_is_server_listening(const char *endpoint)
{
  char *pipe_name;
  RPC_STATUS status;

  pipe_name = ncalrpc_pipe_name(endpoint);
  status = is_pipe_listening(pipe_name);
  I_RpcFree(pipe_name);
  return status;
}

static RPC_STATUS rpcrt4_ncalrpc_handoff(RpcConnection *old_conn, RpcConnection *new_conn)
{
  DWORD len = MAX_COMPUTERNAME_LENGTH + 1;
  RPC_STATUS status;

  TRACE("%s\n", old_conn->Endpoint);

  rpcrt4_conn_np_handoff((RpcConnection_np *)old_conn, (RpcConnection_np *)new_conn);
  status = rpcrt4_conn_create_pipe(old_conn);

  /* Store the local computer name as the NetworkAddr for ncalrpc. */
  new_conn->NetworkAddr = HeapAlloc(GetProcessHeap(), 0, len);
  if (!GetComputerNameA(new_conn->NetworkAddr, &len))
  {
    ERR("Failed to retrieve the computer name, error %u\n", GetLastError());
    return RPC_S_OUT_OF_RESOURCES;
  }

  return status;
}

static int rpcrt4_conn_np_read(RpcConnection *conn, void *buffer, unsigned int count)
{
    RpcConnection_np *connection = (RpcConnection_np *) conn;
    HANDLE event;
    NTSTATUS status;

    event = get_np_event(connection);
    if (!event)
        return -1;

    if (connection->read_closed)
        status = STATUS_CANCELLED;
    else
        status = NtReadFile(connection->pipe, event, NULL, NULL, &connection->io_status, buffer, count, NULL, NULL);
    if (status == STATUS_PENDING)
    {
        /* check read_closed again before waiting to avoid a race */
        if (connection->read_closed)
        {
            IO_STATUS_BLOCK io_status;
#ifdef __REACTOS__ /* FIXME: We should also cancel I/O for other threads */
            NtCancelIoFile(connection->pipe, &io_status);
#else
            NtCancelIoFileEx(connection->pipe, &connection->io_status, &io_status);
#endif
        }
        WaitForSingleObject(event, INFINITE);
        status = connection->io_status.u.Status;
    }
    release_np_event(connection, event);
    return status && status != STATUS_BUFFER_OVERFLOW ? -1 : connection->io_status.Information;
}

static int rpcrt4_conn_np_write(RpcConnection *conn, const void *buffer, unsigned int count)
{
    RpcConnection_np *connection = (RpcConnection_np *) conn;
    IO_STATUS_BLOCK io_status;
    HANDLE event;
    NTSTATUS status;

    event = get_np_event(connection);
    if (!event)
        return -1;

    status = NtWriteFile(connection->pipe, event, NULL, NULL, &io_status, buffer, count, NULL, NULL);
    if (status == STATUS_PENDING)
    {
        WaitForSingleObject(event, INFINITE);
        status = io_status.u.Status;
    }
    release_np_event(connection, event);
    if (status)
        return -1;

    assert(io_status.Information == count);
    return count;
}

static int rpcrt4_conn_np_close(RpcConnection *conn)
{
    RpcConnection_np *connection = (RpcConnection_np *) conn;
    if (connection->pipe)
    {
        FlushFileBuffers(connection->pipe);
        CloseHandle(connection->pipe);
        connection->pipe = 0;
    }
    if (connection->listen_event)
    {
        CloseHandle(connection->listen_event);
        connection->listen_event = 0;
    }
    if (connection->event_cache)
    {
        CloseHandle(connection->event_cache);
        connection->event_cache = 0;
    }
    return 0;
}

static void rpcrt4_conn_np_close_read(RpcConnection *conn)
{
    RpcConnection_np *connection = (RpcConnection_np*)conn;
    IO_STATUS_BLOCK io_status;

    connection->read_closed = TRUE;
#ifdef __REACTOS__ /* FIXME: We should also cancel I/O for other threads */
    NtCancelIoFile(connection->pipe, &io_status);
#else
    NtCancelIoFileEx(connection->pipe, &connection->io_status, &io_status);
#endif
}

static void rpcrt4_conn_np_cancel_call(RpcConnection *conn)
{
    RpcConnection_np *connection = (RpcConnection_np *)conn;
    CancelIoEx(connection->pipe, NULL);
}

static int rpcrt4_conn_np_wait_for_incoming_data(RpcConnection *conn)
{
    return rpcrt4_conn_np_read(conn, NULL, 0);
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

    networkaddr_size = networkaddr ? strlen(networkaddr) + 1 : 1;
    endpoint_size = endpoint ? strlen(endpoint) + 1 : 1;
    size = sizeof(*smb_floor) + endpoint_size + sizeof(*nb_floor) + networkaddr_size;

    if (!tower_data)
        return size;

    smb_floor = (twr_empty_floor_t *)tower_data;

    tower_data += sizeof(*smb_floor);

    smb_floor->count_lhs = sizeof(smb_floor->protid);
    smb_floor->protid = EPM_PROTOCOL_SMB;
    smb_floor->count_rhs = endpoint_size;

    if (endpoint)
        memcpy(tower_data, endpoint, endpoint_size);
    else
        tower_data[0] = 0;
    tower_data += endpoint_size;

    nb_floor = (twr_empty_floor_t *)tower_data;

    tower_data += sizeof(*nb_floor);

    nb_floor->count_lhs = sizeof(nb_floor->protid);
    nb_floor->protid = EPM_PROTOCOL_NETBIOS;
    nb_floor->count_rhs = networkaddr_size;

    if (networkaddr)
        memcpy(tower_data, networkaddr, networkaddr_size);
    else
        tower_data[0] = 0;

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
        (smb_floor->count_rhs > tower_size) ||
        (tower_data[smb_floor->count_rhs - 1] != '\0'))
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
        (nb_floor->count_rhs > tower_size) ||
        (tower_data[nb_floor->count_rhs - 1] != '\0'))
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

static RPC_STATUS rpcrt4_conn_np_impersonate_client(RpcConnection *conn)
{
    RpcConnection_np *npc = (RpcConnection_np *)conn;
    BOOL ret;

    TRACE("(%p)\n", conn);

    if (conn->AuthInfo && SecIsValidHandle(&conn->ctx))
        return RPCRT4_default_impersonate_client(conn);

    ret = ImpersonateNamedPipeClient(npc->pipe);
    if (!ret)
    {
        DWORD error = GetLastError();
        WARN("ImpersonateNamedPipeClient failed with error %u\n", error);
        switch (error)
        {
        case ERROR_CANNOT_IMPERSONATE:
            return RPC_S_NO_CONTEXT_AVAILABLE;
        }
    }
    return RPC_S_OK;
}

static RPC_STATUS rpcrt4_conn_np_revert_to_self(RpcConnection *conn)
{
    BOOL ret;

    TRACE("(%p)\n", conn);

    if (conn->AuthInfo && SecIsValidHandle(&conn->ctx))
        return RPCRT4_default_revert_to_self(conn);

    ret = RevertToSelf();
    if (!ret)
    {
        WARN("RevertToSelf failed with error %u\n", GetLastError());
        return RPC_S_NO_CONTEXT_AVAILABLE;
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
    RpcServerProtseq_np *ps = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ps));
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
    LIST_FOR_EACH_ENTRY(conn, &protseq->listeners, RpcConnection_np, common.protseq_entry)
    {
        if (!conn->pipe && rpcrt4_conn_create_pipe(&conn->common) != RPC_S_OK)
            continue;
        if (!conn->listen_event)
        {
            NTSTATUS status;
            HANDLE event;

            event = get_np_event(conn);
            if (!event)
                continue;

            status = NtFsControlFile(conn->pipe, event, NULL, NULL, &conn->io_status, FSCTL_PIPE_LISTEN, NULL, 0, NULL, 0);
            switch (status)
            {
            case STATUS_SUCCESS:
            case STATUS_PIPE_CONNECTED:
                conn->io_status.u.Status = status;
                SetEvent(event);
                break;
            case STATUS_PENDING:
                break;
            default:
                ERR("pipe listen error %x\n", status);
                continue;
            }

            conn->listen_event = event;
        }
        (*count)++;
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
    LIST_FOR_EACH_ENTRY(conn, &protseq->listeners, RpcConnection_np, common.protseq_entry)
    {
        if (conn->listen_event)
            objs[(*count)++] = conn->listen_event;
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
    RpcConnection *cconn = NULL;
    RpcConnection_np *conn;
    
    if (!objs)
        return -1;

    do
    {
        /* an alertable wait isn't strictly necessary, but due to our
         * overlapped I/O implementation in Wine we need to free some memory
         * by the file user APC being called, even if no completion routine was
         * specified at the time of starting the async operation */
        res = WaitForMultipleObjectsEx(count, objs, FALSE, INFINITE, TRUE);
    } while (res == WAIT_IO_COMPLETION);

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
        LIST_FOR_EACH_ENTRY(conn, &protseq->listeners, RpcConnection_np, common.protseq_entry)
        {
            if (b_handle == conn->listen_event)
            {
                release_np_event(conn, conn->listen_event);
                conn->listen_event = NULL;
                if (conn->io_status.u.Status == STATUS_SUCCESS || conn->io_status.u.Status == STATUS_PIPE_CONNECTED)
                    cconn = rpcrt4_spawn_connection(&conn->common);
                else
                    ERR("listen failed %x\n", conn->io_status.u.Status);
                break;
            }
        }
        LeaveCriticalSection(&protseq->cs);
        if (!cconn)
        {
            ERR("failed to locate connection for handle %p\n", b_handle);
            return -1;
        }
        RPCRT4_new_client(cconn);
        return 1;
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

    endpoint_size = strlen(endpoint) + 1;
    size = sizeof(*pipe_floor) + endpoint_size;

    if (!tower_data)
        return size;

    pipe_floor = (twr_empty_floor_t *)tower_data;

    tower_data += sizeof(*pipe_floor);

    pipe_floor->count_lhs = sizeof(pipe_floor->protid);
    pipe_floor->protid = EPM_PROTOCOL_PIPE;
    pipe_floor->count_rhs = endpoint_size;

    memcpy(tower_data, endpoint, endpoint_size);

    return size;
}

static RPC_STATUS rpcrt4_ncalrpc_parse_top_of_tower(const unsigned char *tower_data,
                                                    size_t tower_size,
                                                    char **networkaddr,
                                                    char **endpoint)
{
    const twr_empty_floor_t *pipe_floor = (const twr_empty_floor_t *)tower_data;

    TRACE("(%p, %d, %p, %p)\n", tower_data, (int)tower_size, networkaddr, endpoint);

    if (tower_size < sizeof(*pipe_floor))
        return EPT_S_NOT_REGISTERED;

    tower_data += sizeof(*pipe_floor);
    tower_size -= sizeof(*pipe_floor);

    if ((pipe_floor->count_lhs != sizeof(pipe_floor->protid)) ||
        (pipe_floor->protid != EPM_PROTOCOL_PIPE) ||
        (pipe_floor->count_rhs > tower_size) ||
        (tower_data[pipe_floor->count_rhs - 1] != '\0'))
        return EPT_S_NOT_REGISTERED;

    if (networkaddr)
        *networkaddr = NULL;

    if (endpoint)
    {
        *endpoint = I_RpcAllocate(pipe_floor->count_rhs);
        if (!*endpoint)
            return RPC_S_OUT_OF_RESOURCES;
        memcpy(*endpoint, tower_data, pipe_floor->count_rhs);
    }

    return RPC_S_OK;
}

static BOOL rpcrt4_ncalrpc_is_authorized(RpcConnection *conn)
{
    return FALSE;
}

static RPC_STATUS rpcrt4_ncalrpc_authorize(RpcConnection *conn, BOOL first_time,
                                           unsigned char *in_buffer,
                                           unsigned int in_size,
                                           unsigned char *out_buffer,
                                           unsigned int *out_size)
{
    /* since this protocol is local to the machine there is no need to
     * authenticate the caller */
    *out_size = 0;
    return RPC_S_OK;
}

static RPC_STATUS rpcrt4_ncalrpc_secure_packet(RpcConnection *conn,
    enum secure_packet_direction dir,
    RpcPktHdr *hdr, unsigned int hdr_size,
    unsigned char *stub_data, unsigned int stub_data_size,
    RpcAuthVerifier *auth_hdr,
    unsigned char *auth_value, unsigned int auth_value_size)
{
    /* since this protocol is local to the machine there is no need to secure
     * the packet */
    return RPC_S_OK;
}

static RPC_STATUS rpcrt4_ncalrpc_inquire_auth_client(
    RpcConnection *conn, RPC_AUTHZ_HANDLE *privs, RPC_WSTR *server_princ_name,
    ULONG *authn_level, ULONG *authn_svc, ULONG *authz_svc, ULONG flags)
{
    TRACE("(%p, %p, %p, %p, %p, %p, 0x%x)\n", conn, privs,
          server_princ_name, authn_level, authn_svc, authz_svc, flags);

    if (privs)
    {
        FIXME("privs not implemented\n");
        *privs = NULL;
    }
    if (server_princ_name)
    {
        FIXME("server_princ_name not implemented\n");
        *server_princ_name = NULL;
    }
    if (authn_level) *authn_level = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
    if (authn_svc) *authn_svc = RPC_C_AUTHN_WINNT;
    if (authz_svc)
    {
        FIXME("authorization service not implemented\n");
        *authz_svc = RPC_C_AUTHZ_NONE;
    }
    if (flags)
        FIXME("flags 0x%x not implemented\n", flags);

    return RPC_S_OK;
}

/**** ncacn_ip_tcp support ****/

static size_t rpcrt4_ip_tcp_get_top_of_tower(unsigned char *tower_data,
                                             const char *networkaddr,
                                             unsigned char tcp_protid,
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
    tcp_floor->protid = tcp_protid;
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
        freeaddrinfo(ai);
        return 0;
    }

    freeaddrinfo(ai);

    return size;
}

static RPC_STATUS rpcrt4_ip_tcp_parse_top_of_tower(const unsigned char *tower_data,
                                                   size_t tower_size,
                                                   char **networkaddr,
                                                   unsigned char tcp_protid,
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
        (tcp_floor->protid != tcp_protid) ||
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
            ERR("inet_ntop: %u\n", WSAGetLastError());
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

typedef struct _RpcConnection_tcp
{
  RpcConnection common;
  int sock;
  HANDLE sock_event;
  HANDLE cancel_event;
} RpcConnection_tcp;

static BOOL rpcrt4_sock_wait_init(RpcConnection_tcp *tcpc)
{
  static BOOL wsa_inited;
  if (!wsa_inited)
  {
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);
    /* Note: WSAStartup can be called more than once so we don't bother with
     * making accesses to wsa_inited thread-safe */
    wsa_inited = TRUE;
  }
  tcpc->sock_event = CreateEventW(NULL, FALSE, FALSE, NULL);
  tcpc->cancel_event = CreateEventW(NULL, FALSE, FALSE, NULL);
  if (!tcpc->sock_event || !tcpc->cancel_event)
  {
    ERR("event creation failed\n");
    if (tcpc->sock_event) CloseHandle(tcpc->sock_event);
    return FALSE;
  }
  return TRUE;
}

static BOOL rpcrt4_sock_wait_for_recv(RpcConnection_tcp *tcpc)
{
  HANDLE wait_handles[2];
  DWORD res;
  if (WSAEventSelect(tcpc->sock, tcpc->sock_event, FD_READ | FD_CLOSE) == SOCKET_ERROR)
  {
    ERR("WSAEventSelect() failed with error %d\n", WSAGetLastError());
    return FALSE;
  }
  wait_handles[0] = tcpc->sock_event;
  wait_handles[1] = tcpc->cancel_event;
  res = WaitForMultipleObjects(2, wait_handles, FALSE, INFINITE);
  switch (res)
  {
  case WAIT_OBJECT_0:
    return TRUE;
  case WAIT_OBJECT_0 + 1:
    return FALSE;
  default:
    ERR("WaitForMultipleObjects() failed with error %d\n", GetLastError());
    return FALSE;
  }
}

static BOOL rpcrt4_sock_wait_for_send(RpcConnection_tcp *tcpc)
{
  DWORD res;
  if (WSAEventSelect(tcpc->sock, tcpc->sock_event, FD_WRITE | FD_CLOSE) == SOCKET_ERROR)
  {
    ERR("WSAEventSelect() failed with error %d\n", WSAGetLastError());
    return FALSE;
  }
  res = WaitForSingleObject(tcpc->sock_event, INFINITE);
  switch (res)
  {
  case WAIT_OBJECT_0:
    return TRUE;
  default:
    ERR("WaitForMultipleObjects() failed with error %d\n", GetLastError());
    return FALSE;
  }
}

static RpcConnection *rpcrt4_conn_tcp_alloc(void)
{
  RpcConnection_tcp *tcpc;
  tcpc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RpcConnection_tcp));
  if (tcpc == NULL)
    return NULL;
  tcpc->sock = -1;
  if (!rpcrt4_sock_wait_init(tcpc))
  {
    HeapFree(GetProcessHeap(), 0, tcpc);
    return NULL;
  }
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
    u_long nonblocking;

    if (ai_cur->ai_family != AF_INET && ai_cur->ai_family != AF_INET6)
    {
      TRACE("skipping non-IP/IPv6 address family\n");
      continue;
    }

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
    if (sock == -1)
    {
      WARN("socket() failed: %u\n", WSAGetLastError());
      continue;
    }

    if (0>connect(sock, ai_cur->ai_addr, ai_cur->ai_addrlen))
    {
      WARN("connect() failed: %u\n", WSAGetLastError());
      closesocket(sock);
      continue;
    }

    /* RPC depends on having minimal latency so disable the Nagle algorithm */
    val = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&val, sizeof(val));
    nonblocking = 1;
    ioctlsocket(sock, FIONBIO, &nonblocking);

    tcpc->sock = sock;

    freeaddrinfo(ai);
    TRACE("connected\n");
    return RPC_S_OK;
  }

  freeaddrinfo(ai);
  ERR("couldn't connect to %s:%s\n", Connection->NetworkAddr, Connection->Endpoint);
  return RPC_S_SERVER_UNAVAILABLE;
}

static RPC_STATUS rpcrt4_protseq_ncacn_ip_tcp_open_endpoint(RpcServerProtseq *protseq, const char *endpoint)
{
    RPC_STATUS status = RPC_S_CANT_CREATE_ENDPOINT;
    int sock;
    int ret;
    struct addrinfo *ai;
    struct addrinfo *ai_cur;
    struct addrinfo hints;

    TRACE("(%p, %s)\n", protseq, endpoint);

    hints.ai_flags          = AI_PASSIVE /* for non-localhost addresses */;
    hints.ai_family         = PF_UNSPEC;
    hints.ai_socktype       = SOCK_STREAM;
    hints.ai_protocol       = IPPROTO_TCP;
    hints.ai_addrlen        = 0;
    hints.ai_addr           = NULL;
    hints.ai_canonname      = NULL;
    hints.ai_next           = NULL;

    ret = getaddrinfo(NULL, endpoint ? endpoint : "0", &hints, &ai);
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
        struct sockaddr_storage sa;
        socklen_t sa_len;
        char service[NI_MAXSERV];
        u_long nonblocking;

        if (ai_cur->ai_family != AF_INET && ai_cur->ai_family != AF_INET6)
        {
            TRACE("skipping non-IP/IPv6 address family\n");
            continue;
        }

        if (TRACE_ON(rpc))
        {
            char host[256];
            getnameinfo(ai_cur->ai_addr, ai_cur->ai_addrlen,
                        host, sizeof(host), service, sizeof(service),
                        NI_NUMERICHOST | NI_NUMERICSERV);
            TRACE("trying %s:%s\n", host, service);
        }

        sock = socket(ai_cur->ai_family, ai_cur->ai_socktype, ai_cur->ai_protocol);
        if (sock == -1)
        {
            WARN("socket() failed: %u\n", WSAGetLastError());
            status = RPC_S_CANT_CREATE_ENDPOINT;
            continue;
        }

        ret = bind(sock, ai_cur->ai_addr, ai_cur->ai_addrlen);
        if (ret < 0)
        {
            WARN("bind failed: %u\n", WSAGetLastError());
            closesocket(sock);
            if (WSAGetLastError() == WSAEADDRINUSE)
              status = RPC_S_DUPLICATE_ENDPOINT;
            else
              status = RPC_S_CANT_CREATE_ENDPOINT;
            continue;
        }

        sa_len = sizeof(sa);
        if (getsockname(sock, (struct sockaddr *)&sa, &sa_len))
        {
            WARN("getsockname() failed: %u\n", WSAGetLastError());
            closesocket(sock);
            status = RPC_S_CANT_CREATE_ENDPOINT;
            continue;
        }

        ret = getnameinfo((struct sockaddr *)&sa, sa_len,
                          NULL, 0, service, sizeof(service),
                          NI_NUMERICSERV);
        if (ret)
        {
            WARN("getnameinfo failed: %s\n", gai_strerror(ret));
            closesocket(sock);
            status = RPC_S_CANT_CREATE_ENDPOINT;
            continue;
        }

        create_status = RPCRT4_CreateConnection((RpcConnection **)&tcpc, TRUE,
                                                protseq->Protseq, NULL,
                                                service, NULL, NULL, NULL, NULL);
        if (create_status != RPC_S_OK)
        {
            closesocket(sock);
            status = create_status;
            continue;
        }

        tcpc->sock = sock;
        ret = listen(sock, protseq->MaxCalls);
        if (ret < 0)
        {
            WARN("listen failed: %u\n", WSAGetLastError());
            RPCRT4_ReleaseConnection(&tcpc->common);
            status = RPC_S_OUT_OF_RESOURCES;
            continue;
        }
        /* need a non-blocking socket, otherwise accept() has a potential
         * race-condition (poll() says it is readable, connection drops,
         * and accept() blocks until the next connection comes...)
         */
        nonblocking = 1;
        ret = ioctlsocket(sock, FIONBIO, &nonblocking);
        if (ret < 0)
        {
            WARN("couldn't make socket non-blocking, error %d\n", ret);
            RPCRT4_ReleaseConnection(&tcpc->common);
            status = RPC_S_OUT_OF_RESOURCES;
            continue;
        }

        EnterCriticalSection(&protseq->cs);
        list_add_tail(&protseq->listeners, &tcpc->common.protseq_entry);
        tcpc->common.protseq = protseq;
        LeaveCriticalSection(&protseq->cs);

        freeaddrinfo(ai);

        /* since IPv4 and IPv6 share the same port space, we only need one
         * successful bind to listen for both */
        TRACE("listening on %s\n", endpoint);
        return RPC_S_OK;
    }

    freeaddrinfo(ai);
    ERR("couldn't listen on port %s\n", endpoint);
    return status;
}

static RPC_STATUS rpcrt4_conn_tcp_handoff(RpcConnection *old_conn, RpcConnection *new_conn)
{
  int ret;
  struct sockaddr_in address;
  socklen_t addrsize;
  RpcConnection_tcp *server = (RpcConnection_tcp*) old_conn;
  RpcConnection_tcp *client = (RpcConnection_tcp*) new_conn;
  u_long nonblocking;

  addrsize = sizeof(address);
  ret = accept(server->sock, (struct sockaddr*) &address, &addrsize);
  if (ret < 0)
  {
    ERR("Failed to accept a TCP connection: error %d\n", ret);
    return RPC_S_OUT_OF_RESOURCES;
  }

  nonblocking = 1;
  ioctlsocket(ret, FIONBIO, &nonblocking);
  client->sock = ret;

  client->common.NetworkAddr = HeapAlloc(GetProcessHeap(), 0, INET6_ADDRSTRLEN);
  ret = getnameinfo((struct sockaddr*)&address, addrsize, client->common.NetworkAddr, INET6_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);
  if (ret != 0)
  {
    ERR("Failed to retrieve the IP address, error %d\n", ret);
    return RPC_S_OUT_OF_RESOURCES;
  }

  TRACE("Accepted a new TCP connection from %s\n", client->common.NetworkAddr);
  return RPC_S_OK;
}

static int rpcrt4_conn_tcp_read(RpcConnection *Connection,
                                void *buffer, unsigned int count)
{
  RpcConnection_tcp *tcpc = (RpcConnection_tcp *) Connection;
  int bytes_read = 0;
  while (bytes_read != count)
  {
    int r = recv(tcpc->sock, (char *)buffer + bytes_read, count - bytes_read, 0);
    if (!r)
      return -1;
    else if (r > 0)
      bytes_read += r;
    else if (WSAGetLastError() == WSAEINTR)
      continue;
    else if (WSAGetLastError() != WSAEWOULDBLOCK)
    {
      WARN("recv() failed: %u\n", WSAGetLastError());
      return -1;
    }
    else
    {
      if (!rpcrt4_sock_wait_for_recv(tcpc))
        return -1;
    }
  }
  TRACE("%d %p %u -> %d\n", tcpc->sock, buffer, count, bytes_read);
  return bytes_read;
}

static int rpcrt4_conn_tcp_write(RpcConnection *Connection,
                                 const void *buffer, unsigned int count)
{
  RpcConnection_tcp *tcpc = (RpcConnection_tcp *) Connection;
  int bytes_written = 0;
  while (bytes_written != count)
  {
    int r = send(tcpc->sock, (const char *)buffer + bytes_written, count - bytes_written, 0);
    if (r >= 0)
      bytes_written += r;
    else if (WSAGetLastError() == WSAEINTR)
      continue;
    else if (WSAGetLastError() != WSAEWOULDBLOCK)
      return -1;
    else
    {
      if (!rpcrt4_sock_wait_for_send(tcpc))
        return -1;
    }
  }
  TRACE("%d %p %u -> %d\n", tcpc->sock, buffer, count, bytes_written);
  return bytes_written;
}

static int rpcrt4_conn_tcp_close(RpcConnection *conn)
{
    RpcConnection_tcp *connection = (RpcConnection_tcp *) conn;

    TRACE("%d\n", connection->sock);

    if (connection->sock != -1)
        closesocket(connection->sock);
    connection->sock = -1;
    CloseHandle(connection->sock_event);
    CloseHandle(connection->cancel_event);
    return 0;
}

static void rpcrt4_conn_tcp_close_read(RpcConnection *conn)
{
    RpcConnection_tcp *connection = (RpcConnection_tcp *) conn;
    shutdown(connection->sock, SD_RECEIVE);
}

static void rpcrt4_conn_tcp_cancel_call(RpcConnection *conn)
{
    RpcConnection_tcp *connection = (RpcConnection_tcp *) conn;

    TRACE("%p\n", connection);

    SetEvent(connection->cancel_event);
}

static RPC_STATUS rpcrt4_conn_tcp_is_server_listening(const char *endpoint)
{
    FIXME("\n");
    return RPC_S_ACCESS_DENIED;
}

static int rpcrt4_conn_tcp_wait_for_incoming_data(RpcConnection *Connection)
{
    RpcConnection_tcp *tcpc = (RpcConnection_tcp *) Connection;

    TRACE("%p\n", Connection);

    if (!rpcrt4_sock_wait_for_recv(tcpc))
        return -1;
    return 0;
}

static size_t rpcrt4_ncacn_ip_tcp_get_top_of_tower(unsigned char *tower_data,
                                                   const char *networkaddr,
                                                   const char *endpoint)
{
    return rpcrt4_ip_tcp_get_top_of_tower(tower_data, networkaddr,
                                          EPM_PROTOCOL_TCP, endpoint);
}

typedef struct _RpcServerProtseq_sock
{
    RpcServerProtseq common;
    HANDLE mgr_event;
} RpcServerProtseq_sock;

static RpcServerProtseq *rpcrt4_protseq_sock_alloc(void)
{
    RpcServerProtseq_sock *ps = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ps));
    if (ps)
    {
        static BOOL wsa_inited;
        if (!wsa_inited)
        {
            WSADATA wsadata;
            WSAStartup(MAKEWORD(2, 2), &wsadata);
            /* Note: WSAStartup can be called more than once so we don't bother with
             * making accesses to wsa_inited thread-safe */
            wsa_inited = TRUE;
        }
        ps->mgr_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    }
    return &ps->common;
}

static void rpcrt4_protseq_sock_signal_state_changed(RpcServerProtseq *protseq)
{
    RpcServerProtseq_sock *sockps = CONTAINING_RECORD(protseq, RpcServerProtseq_sock, common);
    SetEvent(sockps->mgr_event);
}

static void *rpcrt4_protseq_sock_get_wait_array(RpcServerProtseq *protseq, void *prev_array, unsigned int *count)
{
    HANDLE *objs = prev_array;
    RpcConnection_tcp *conn;
    RpcServerProtseq_sock *sockps = CONTAINING_RECORD(protseq, RpcServerProtseq_sock, common);

    EnterCriticalSection(&protseq->cs);

    /* open and count connections */
    *count = 1;
    LIST_FOR_EACH_ENTRY(conn, &protseq->listeners, RpcConnection_tcp, common.protseq_entry)
    {
        if (conn->sock != -1)
            (*count)++;
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

    objs[0] = sockps->mgr_event;
    *count = 1;
    LIST_FOR_EACH_ENTRY(conn, &protseq->listeners, RpcConnection_tcp, common.protseq_entry)
    {
        if (conn->sock != -1)
        {
            int res = WSAEventSelect(conn->sock, conn->sock_event, FD_ACCEPT);
            if (res == SOCKET_ERROR)
                ERR("WSAEventSelect() failed with error %d\n", WSAGetLastError());
            else
            {
                objs[*count] = conn->sock_event;
                (*count)++;
            }
        }
    }
    LeaveCriticalSection(&protseq->cs);
    return objs;
}

static void rpcrt4_protseq_sock_free_wait_array(RpcServerProtseq *protseq, void *array)
{
    HeapFree(GetProcessHeap(), 0, array);
}

static int rpcrt4_protseq_sock_wait_for_new_connection(RpcServerProtseq *protseq, unsigned int count, void *wait_array)
{
    HANDLE b_handle;
    HANDLE *objs = wait_array;
    DWORD res;
    RpcConnection *cconn = NULL;
    RpcConnection_tcp *conn;

    if (!objs)
        return -1;

    do
    {
        /* an alertable wait isn't strictly necessary, but due to our
         * overlapped I/O implementation in Wine we need to free some memory
         * by the file user APC being called, even if no completion routine was
         * specified at the time of starting the async operation */
        res = WaitForMultipleObjectsEx(count, objs, FALSE, INFINITE, TRUE);
    } while (res == WAIT_IO_COMPLETION);

    if (res == WAIT_OBJECT_0)
        return 0;
    if (res == WAIT_FAILED)
    {
        ERR("wait failed with error %d\n", GetLastError());
        return -1;
    }

    b_handle = objs[res - WAIT_OBJECT_0];

    /* find which connection got a RPC */
    EnterCriticalSection(&protseq->cs);
    LIST_FOR_EACH_ENTRY(conn, &protseq->listeners, RpcConnection_tcp, common.protseq_entry)
    {
        if (b_handle == conn->sock_event)
        {
            cconn = rpcrt4_spawn_connection(&conn->common);
            break;
        }
    }
    LeaveCriticalSection(&protseq->cs);
    if (!cconn)
    {
        ERR("failed to locate connection for handle %p\n", b_handle);
        return -1;
    }

    RPCRT4_new_client(cconn);
    return 1;
}

static RPC_STATUS rpcrt4_ncacn_ip_tcp_parse_top_of_tower(const unsigned char *tower_data,
                                                         size_t tower_size,
                                                         char **networkaddr,
                                                         char **endpoint)
{
    return rpcrt4_ip_tcp_parse_top_of_tower(tower_data, tower_size,
                                            networkaddr, EPM_PROTOCOL_TCP,
                                            endpoint);
}

/**** ncacn_http support ****/

/* 60 seconds is the period native uses */
#define HTTP_IDLE_TIME 60000

/* reference counted to avoid a race between a cancelled call's connection
 * being destroyed and the asynchronous InternetReadFileEx call being
 * completed */
typedef struct _RpcHttpAsyncData
{
    LONG refs;
    HANDLE completion_event;
    WORD async_result;
    INTERNET_BUFFERSW inet_buffers;
    CRITICAL_SECTION cs;
} RpcHttpAsyncData;

static ULONG RpcHttpAsyncData_AddRef(RpcHttpAsyncData *data)
{
    return InterlockedIncrement(&data->refs);
}

static ULONG RpcHttpAsyncData_Release(RpcHttpAsyncData *data)
{
    ULONG refs = InterlockedDecrement(&data->refs);
    if (!refs)
    {
        TRACE("destroying async data %p\n", data);
        CloseHandle(data->completion_event);
        HeapFree(GetProcessHeap(), 0, data->inet_buffers.lpvBuffer);
        data->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&data->cs);
        HeapFree(GetProcessHeap(), 0, data);
    }
    return refs;
}

static void prepare_async_request(RpcHttpAsyncData *async_data)
{
    ResetEvent(async_data->completion_event);
    RpcHttpAsyncData_AddRef(async_data);
}

static RPC_STATUS wait_async_request(RpcHttpAsyncData *async_data, BOOL call_ret, HANDLE cancel_event)
{
    HANDLE handles[2] = { async_data->completion_event, cancel_event };
    DWORD res;

    if(call_ret) {
        RpcHttpAsyncData_Release(async_data);
        return RPC_S_OK;
    }

    if(GetLastError() != ERROR_IO_PENDING) {
        RpcHttpAsyncData_Release(async_data);
        ERR("Request failed with error %d\n", GetLastError());
        return RPC_S_SERVER_UNAVAILABLE;
    }

    res = WaitForMultipleObjects(2, handles, FALSE, DEFAULT_NCACN_HTTP_TIMEOUT);
    if(res != WAIT_OBJECT_0) {
        TRACE("Cancelled\n");
        return RPC_S_CALL_CANCELLED;
    }

    if(async_data->async_result) {
        ERR("Async request failed with error %d\n", async_data->async_result);
        return RPC_S_SERVER_UNAVAILABLE;
    }

    return RPC_S_OK;
}

struct authinfo
{
    DWORD        scheme;
    CredHandle   cred;
    CtxtHandle   ctx;
    TimeStamp    exp;
    ULONG        attr;
    ULONG        max_token;
    char        *data;
    unsigned int data_len;
    BOOL         finished; /* finished authenticating */
};

typedef struct _RpcConnection_http
{
    RpcConnection common;
    HINTERNET app_info;
    HINTERNET session;
    HINTERNET in_request;
    HINTERNET out_request;
    WCHAR *servername;
    HANDLE timer_cancelled;
    HANDLE cancel_event;
    DWORD last_sent_time;
    ULONG bytes_received;
    ULONG flow_control_mark; /* send a control packet to the server when this many bytes received */
    ULONG flow_control_increment; /* number of bytes to increment flow_control_mark by */
    UUID connection_uuid;
    UUID in_pipe_uuid;
    UUID out_pipe_uuid;
    RpcHttpAsyncData *async_data;
} RpcConnection_http;

static RpcConnection *rpcrt4_ncacn_http_alloc(void)
{
    RpcConnection_http *httpc;
    httpc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*httpc));
    if (!httpc) return NULL;
    httpc->async_data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RpcHttpAsyncData));
    if (!httpc->async_data)
    {
        HeapFree(GetProcessHeap(), 0, httpc);
        return NULL;
    }
    TRACE("async data = %p\n", httpc->async_data);
    httpc->cancel_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    httpc->async_data->refs = 1;
    httpc->async_data->inet_buffers.dwStructSize = sizeof(INTERNET_BUFFERSW);
    InitializeCriticalSection(&httpc->async_data->cs);
    httpc->async_data->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": RpcHttpAsyncData.cs");
    return &httpc->common;
}

typedef struct _HttpTimerThreadData
{
    PVOID timer_param;
    DWORD *last_sent_time;
    HANDLE timer_cancelled;
} HttpTimerThreadData;

static VOID rpcrt4_http_keep_connection_active_timer_proc(PVOID param, BOOLEAN dummy)
{
    HINTERNET in_request = param;
    RpcPktHdr *idle_pkt;

    idle_pkt = RPCRT4_BuildHttpHeader(NDR_LOCAL_DATA_REPRESENTATION, 0x0001,
                                      0, 0);
    if (idle_pkt)
    {
        DWORD bytes_written;
        InternetWriteFile(in_request, idle_pkt, idle_pkt->common.frag_len, &bytes_written);
        RPCRT4_FreeHeader(idle_pkt);
    }
}

static inline DWORD rpcrt4_http_timer_calc_timeout(DWORD *last_sent_time)
{
    DWORD cur_time = GetTickCount();
    DWORD cached_last_sent_time = *last_sent_time;
    return HTTP_IDLE_TIME - (cur_time - cached_last_sent_time > HTTP_IDLE_TIME ? 0 : cur_time - cached_last_sent_time);
}

static DWORD CALLBACK rpcrt4_http_timer_thread(PVOID param)
{
    HttpTimerThreadData *data_in = param;
    HttpTimerThreadData data;
    DWORD timeout;

    data = *data_in;
    HeapFree(GetProcessHeap(), 0, data_in);

    for (timeout = HTTP_IDLE_TIME;
         WaitForSingleObject(data.timer_cancelled, timeout) == WAIT_TIMEOUT;
         timeout = rpcrt4_http_timer_calc_timeout(data.last_sent_time))
    {
        /* are we too soon after last send? */
        if (GetTickCount() - *data.last_sent_time < HTTP_IDLE_TIME)
            continue;
        rpcrt4_http_keep_connection_active_timer_proc(data.timer_param, TRUE);
    }

    CloseHandle(data.timer_cancelled);
    return 0;
}

static VOID WINAPI rpcrt4_http_internet_callback(
     HINTERNET hInternet,
     DWORD_PTR dwContext,
     DWORD dwInternetStatus,
     LPVOID lpvStatusInformation,
     DWORD dwStatusInformationLength)
{
    RpcHttpAsyncData *async_data = (RpcHttpAsyncData *)dwContext;

    switch (dwInternetStatus)
    {
    case INTERNET_STATUS_REQUEST_COMPLETE:
        TRACE("INTERNET_STATUS_REQUEST_COMPLETED\n");
        if (async_data)
        {
            INTERNET_ASYNC_RESULT *async_result = lpvStatusInformation;

            async_data->async_result = async_result->dwResult ? ERROR_SUCCESS : async_result->dwError;
            SetEvent(async_data->completion_event);
            RpcHttpAsyncData_Release(async_data);
        }
        break;
    }
}

static RPC_STATUS rpcrt4_http_check_response(HINTERNET hor)
{
    BOOL ret;
    DWORD status_code;
    DWORD size;
    DWORD index;
    WCHAR buf[32];
    WCHAR *status_text = buf;
    TRACE("\n");

    index = 0;
    size = sizeof(status_code);
    ret = HttpQueryInfoW(hor, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER, &status_code, &size, &index);
    if (!ret)
        return GetLastError();
    if (status_code == HTTP_STATUS_OK)
        return RPC_S_OK;
    index = 0;
    size = sizeof(buf);
    ret = HttpQueryInfoW(hor, HTTP_QUERY_STATUS_TEXT, status_text, &size, &index);
    if (!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        status_text = HeapAlloc(GetProcessHeap(), 0, size);
        ret = HttpQueryInfoW(hor, HTTP_QUERY_STATUS_TEXT, status_text, &size, &index);
    }

    ERR("server returned: %d %s\n", status_code, ret ? debugstr_w(status_text) : "<status text unavailable>");
    if(status_text != buf) HeapFree(GetProcessHeap(), 0, status_text);

    if (status_code == HTTP_STATUS_DENIED)
        return ERROR_ACCESS_DENIED;
    return RPC_S_SERVER_UNAVAILABLE;
}

static RPC_STATUS rpcrt4_http_internet_connect(RpcConnection_http *httpc)
{
    static const WCHAR wszUserAgent[] = {'M','S','R','P','C',0};
    LPWSTR proxy = NULL;
    LPWSTR user = NULL;
    LPWSTR password = NULL;
    LPWSTR servername = NULL;
    const WCHAR *option;
    INTERNET_PORT port;

    if (httpc->common.QOS &&
        (httpc->common.QOS->qos->AdditionalSecurityInfoType == RPC_C_AUTHN_INFO_TYPE_HTTP))
    {
        const RPC_HTTP_TRANSPORT_CREDENTIALS_W *http_cred = httpc->common.QOS->qos->u.HttpCredentials;
        if (http_cred->TransportCredentials)
        {
            WCHAR *p;
            const SEC_WINNT_AUTH_IDENTITY_W *cred = http_cred->TransportCredentials;
            ULONG len = cred->DomainLength + 1 + cred->UserLength;
            user = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
            if (!user)
                return RPC_S_OUT_OF_RESOURCES;
            p = user;
            if (cred->DomainLength)
            {
                memcpy(p, cred->Domain, cred->DomainLength * sizeof(WCHAR));
                p += cred->DomainLength;
                *p = '\\';
                p++;
            }
            memcpy(p, cred->User, cred->UserLength * sizeof(WCHAR));
            p[cred->UserLength] = 0;

            password = RPCRT4_strndupW(cred->Password, cred->PasswordLength);
        }
    }

    for (option = httpc->common.NetworkOptions; option;
         option = (wcschr(option, ',') ? wcschr(option, ',')+1 : NULL))
    {
        static const WCHAR wszRpcProxy[] = {'R','p','c','P','r','o','x','y','=',0};
        static const WCHAR wszHttpProxy[] = {'H','t','t','p','P','r','o','x','y','=',0};

        if (!_wcsnicmp(option, wszRpcProxy, ARRAY_SIZE(wszRpcProxy)-1))
        {
            const WCHAR *value_start = option + ARRAY_SIZE(wszRpcProxy)-1;
            const WCHAR *value_end;
            const WCHAR *p;

            value_end = wcschr(option, ',');
            if (!value_end)
                value_end = value_start + lstrlenW(value_start);
            for (p = value_start; p < value_end; p++)
                if (*p == ':')
                {
                    port = wcstol(p+1, NULL, 10);
                    value_end = p;
                    break;
                }
            TRACE("RpcProxy value is %s\n", debugstr_wn(value_start, value_end-value_start));
            servername = RPCRT4_strndupW(value_start, value_end-value_start);
        }
        else if (!_wcsnicmp(option, wszHttpProxy, ARRAY_SIZE(wszHttpProxy)-1))
        {
            const WCHAR *value_start = option + ARRAY_SIZE(wszHttpProxy)-1;
            const WCHAR *value_end;

            value_end = wcschr(option, ',');
            if (!value_end)
                value_end = value_start + lstrlenW(value_start);
            TRACE("HttpProxy value is %s\n", debugstr_wn(value_start, value_end-value_start));
            proxy = RPCRT4_strndupW(value_start, value_end-value_start);
        }
        else
            FIXME("unhandled option %s\n", debugstr_w(option));
    }

    httpc->app_info = InternetOpenW(wszUserAgent, proxy ? INTERNET_OPEN_TYPE_PROXY : INTERNET_OPEN_TYPE_PRECONFIG,
                                    NULL, NULL, INTERNET_FLAG_ASYNC);
    if (!httpc->app_info)
    {
        HeapFree(GetProcessHeap(), 0, password);
        HeapFree(GetProcessHeap(), 0, user);
        HeapFree(GetProcessHeap(), 0, proxy);
        HeapFree(GetProcessHeap(), 0, servername);
        ERR("InternetOpenW failed with error %d\n", GetLastError());
        return RPC_S_SERVER_UNAVAILABLE;
    }
    InternetSetStatusCallbackW(httpc->app_info, rpcrt4_http_internet_callback);

    /* if no RpcProxy option specified, set the HTTP server address to the
     * RPC server address */
    if (!servername)
    {
        servername = HeapAlloc(GetProcessHeap(), 0, (strlen(httpc->common.NetworkAddr) + 1)*sizeof(WCHAR));
        if (!servername)
        {
            HeapFree(GetProcessHeap(), 0, password);
            HeapFree(GetProcessHeap(), 0, user);
            HeapFree(GetProcessHeap(), 0, proxy);
            return RPC_S_OUT_OF_RESOURCES;
        }
        MultiByteToWideChar(CP_ACP, 0, httpc->common.NetworkAddr, -1, servername, strlen(httpc->common.NetworkAddr) + 1);
    }

    port = (httpc->common.QOS &&
            (httpc->common.QOS->qos->AdditionalSecurityInfoType == RPC_C_AUTHN_INFO_TYPE_HTTP) &&
            (httpc->common.QOS->qos->u.HttpCredentials->Flags & RPC_C_HTTP_FLAG_USE_SSL)) ?
            INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;

    httpc->session = InternetConnectW(httpc->app_info, servername, port, user, password,
                                      INTERNET_SERVICE_HTTP, 0, 0);

    HeapFree(GetProcessHeap(), 0, password);
    HeapFree(GetProcessHeap(), 0, user);
    HeapFree(GetProcessHeap(), 0, proxy);

    if (!httpc->session)
    {
        ERR("InternetConnectW failed with error %d\n", GetLastError());
        HeapFree(GetProcessHeap(), 0, servername);
        return RPC_S_SERVER_UNAVAILABLE;
    }
    httpc->servername = servername;
    return RPC_S_OK;
}

static int rpcrt4_http_async_read(HINTERNET req, RpcHttpAsyncData *async_data, HANDLE cancel_event,
                                  void *buffer, unsigned int count)
{
    char *buf = buffer;
    BOOL ret;
    unsigned int bytes_left = count;
    RPC_STATUS status = RPC_S_OK;

    async_data->inet_buffers.lpvBuffer = HeapAlloc(GetProcessHeap(), 0, count);

    while (bytes_left)
    {
        async_data->inet_buffers.dwBufferLength = bytes_left;
        prepare_async_request(async_data);
        ret = InternetReadFileExW(req, &async_data->inet_buffers, IRF_ASYNC, 0);
        status = wait_async_request(async_data, ret, cancel_event);
        if (status != RPC_S_OK)
        {
            if (status == RPC_S_CALL_CANCELLED)
                TRACE("call cancelled\n");
            break;
        }

        if (!async_data->inet_buffers.dwBufferLength)
            break;
        memcpy(buf, async_data->inet_buffers.lpvBuffer,
               async_data->inet_buffers.dwBufferLength);

        bytes_left -= async_data->inet_buffers.dwBufferLength;
        buf += async_data->inet_buffers.dwBufferLength;
    }

    HeapFree(GetProcessHeap(), 0, async_data->inet_buffers.lpvBuffer);
    async_data->inet_buffers.lpvBuffer = NULL;

    TRACE("%p %p %u -> %u\n", req, buffer, count, status);
    return status == RPC_S_OK ? count : -1;
}

static RPC_STATUS send_echo_request(HINTERNET req, RpcHttpAsyncData *async_data, HANDLE cancel_event)
{
    BYTE buf[20];
    BOOL ret;
    RPC_STATUS status;

    TRACE("sending echo request to server\n");

    prepare_async_request(async_data);
    ret = HttpSendRequestW(req, NULL, 0, NULL, 0);
    status = wait_async_request(async_data, ret, cancel_event);
    if (status != RPC_S_OK) return status;

    status = rpcrt4_http_check_response(req);
    if (status != RPC_S_OK) return status;

    rpcrt4_http_async_read(req, async_data, cancel_event, buf, sizeof(buf));
    /* FIXME: do something with retrieved data */

    return RPC_S_OK;
}

static RPC_STATUS insert_content_length_header(HINTERNET request, DWORD len)
{
    static const WCHAR fmtW[] =
        {'C','o','n','t','e','n','t','-','L','e','n','g','t','h',':',' ','%','u','\r','\n',0};
    WCHAR header[ARRAY_SIZE(fmtW) + 10];

    swprintf(header, fmtW, len);
    if ((HttpAddRequestHeadersW(request, header, -1, HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD))) return RPC_S_OK;
    return RPC_S_SERVER_UNAVAILABLE;
}

/* prepare the in pipe for use by RPC packets */
static RPC_STATUS rpcrt4_http_prepare_in_pipe(HINTERNET in_request, RpcHttpAsyncData *async_data, HANDLE cancel_event,
                                              const UUID *connection_uuid, const UUID *in_pipe_uuid,
                                              const UUID *association_uuid, BOOL authorized)
{
    BOOL ret;
    RPC_STATUS status;
    RpcPktHdr *hdr;
    INTERNET_BUFFERSW buffers_in;
    DWORD bytes_written;

    if (!authorized)
    {
        /* ask wininet to authorize, if necessary */
        status = send_echo_request(in_request, async_data, cancel_event);
        if (status != RPC_S_OK) return status;
    }
    memset(&buffers_in, 0, sizeof(buffers_in));
    buffers_in.dwStructSize = sizeof(buffers_in);
    /* FIXME: get this from the registry */
    buffers_in.dwBufferTotal = 1024 * 1024 * 1024; /* 1Gb */
    status = insert_content_length_header(in_request, buffers_in.dwBufferTotal);
    if (status != RPC_S_OK) return status;

    prepare_async_request(async_data);
    ret = HttpSendRequestExW(in_request, &buffers_in, NULL, 0, 0);
    status = wait_async_request(async_data, ret, cancel_event);
    if (status != RPC_S_OK) return status;

    TRACE("sending HTTP connect header to server\n");
    hdr = RPCRT4_BuildHttpConnectHeader(FALSE, connection_uuid, in_pipe_uuid, association_uuid);
    if (!hdr) return RPC_S_OUT_OF_RESOURCES;
    ret = InternetWriteFile(in_request, hdr, hdr->common.frag_len, &bytes_written);
    RPCRT4_FreeHeader(hdr);
    if (!ret)
    {
        ERR("InternetWriteFile failed with error %d\n", GetLastError());
        return RPC_S_SERVER_UNAVAILABLE;
    }

    return RPC_S_OK;
}

static RPC_STATUS rpcrt4_http_read_http_packet(HINTERNET request, RpcHttpAsyncData *async_data,
                                               HANDLE cancel_event, RpcPktHdr *hdr, BYTE **data)
{
    unsigned short data_len;
    unsigned int size;

    if (rpcrt4_http_async_read(request, async_data, cancel_event, hdr, sizeof(hdr->common)) < 0)
        return RPC_S_SERVER_UNAVAILABLE;
    if (hdr->common.ptype != PKT_HTTP || hdr->common.frag_len < sizeof(hdr->http))
    {
        ERR("wrong packet type received %d or wrong frag_len %d\n",
            hdr->common.ptype, hdr->common.frag_len);
        return RPC_S_PROTOCOL_ERROR;
    }

    size = sizeof(hdr->http) - sizeof(hdr->common);
    if (rpcrt4_http_async_read(request, async_data, cancel_event, &hdr->common + 1, size) < 0)
        return RPC_S_SERVER_UNAVAILABLE;

    data_len = hdr->common.frag_len - sizeof(hdr->http);
    if (data_len)
    {
        *data = HeapAlloc(GetProcessHeap(), 0, data_len);
        if (!*data)
            return RPC_S_OUT_OF_RESOURCES;
        if (rpcrt4_http_async_read(request, async_data, cancel_event, *data, data_len) < 0)
        {
            HeapFree(GetProcessHeap(), 0, *data);
            return RPC_S_SERVER_UNAVAILABLE;
        }
    }
    else
        *data = NULL;

    if (!RPCRT4_IsValidHttpPacket(hdr, *data, data_len))
    {
        ERR("invalid http packet\n");
        HeapFree(GetProcessHeap(), 0, *data);
        return RPC_S_PROTOCOL_ERROR;
    }

    return RPC_S_OK;
}

/* prepare the out pipe for use by RPC packets */
static RPC_STATUS rpcrt4_http_prepare_out_pipe(HINTERNET out_request, RpcHttpAsyncData *async_data,
                                               HANDLE cancel_event, const UUID *connection_uuid,
                                               const UUID *out_pipe_uuid, ULONG *flow_control_increment,
                                               BOOL authorized)
{
    BOOL ret;
    RPC_STATUS status;
    RpcPktHdr *hdr;
    BYTE *data_from_server;
    RpcPktHdr pkt_from_server;
    ULONG field1, field3;
    BYTE buf[20];

    if (!authorized)
    {
        /* ask wininet to authorize, if necessary */
        status = send_echo_request(out_request, async_data, cancel_event);
        if (status != RPC_S_OK) return status;
    }
    else
        rpcrt4_http_async_read(out_request, async_data, cancel_event, buf, sizeof(buf));

    hdr = RPCRT4_BuildHttpConnectHeader(TRUE, connection_uuid, out_pipe_uuid, NULL);
    if (!hdr) return RPC_S_OUT_OF_RESOURCES;

    status = insert_content_length_header(out_request, hdr->common.frag_len);
    if (status != RPC_S_OK)
    {
        RPCRT4_FreeHeader(hdr);
        return status;
    }

    TRACE("sending HTTP connect header to server\n");
    prepare_async_request(async_data);
    ret = HttpSendRequestW(out_request, NULL, 0, hdr, hdr->common.frag_len);
    status = wait_async_request(async_data, ret, cancel_event);
    RPCRT4_FreeHeader(hdr);
    if (status != RPC_S_OK) return status;

    status = rpcrt4_http_check_response(out_request);
    if (status != RPC_S_OK) return status;

    status = rpcrt4_http_read_http_packet(out_request, async_data, cancel_event,
                                          &pkt_from_server, &data_from_server);
    if (status != RPC_S_OK) return status;
    status = RPCRT4_ParseHttpPrepareHeader1(&pkt_from_server, data_from_server,
                                            &field1);
    HeapFree(GetProcessHeap(), 0, data_from_server);
    if (status != RPC_S_OK) return status;
    TRACE("received (%d) from first prepare header\n", field1);

    for (;;)
    {
        status = rpcrt4_http_read_http_packet(out_request, async_data, cancel_event,
                                              &pkt_from_server, &data_from_server);
        if (status != RPC_S_OK) return status;
        if (pkt_from_server.http.flags != 0x0001) break;

        TRACE("http idle packet, waiting for real packet\n");
        HeapFree(GetProcessHeap(), 0, data_from_server);
        if (pkt_from_server.http.num_data_items != 0)
        {
            ERR("HTTP idle packet should have no data items instead of %d\n",
                pkt_from_server.http.num_data_items);
            return RPC_S_PROTOCOL_ERROR;
        }
    }
    status = RPCRT4_ParseHttpPrepareHeader2(&pkt_from_server, data_from_server,
                                            &field1, flow_control_increment,
                                            &field3);
    HeapFree(GetProcessHeap(), 0, data_from_server);
    if (status != RPC_S_OK) return status;
    TRACE("received (0x%08x 0x%08x %d) from second prepare header\n", field1, *flow_control_increment, field3);

    return RPC_S_OK;
}

static UINT encode_base64(const char *bin, unsigned int len, WCHAR *base64)
{
    static const char enc[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    UINT i = 0, x;

    while (len > 0)
    {
        /* first 6 bits, all from bin[0] */
        base64[i++] = enc[(bin[0] & 0xfc) >> 2];
        x = (bin[0] & 3) << 4;

        /* next 6 bits, 2 from bin[0] and 4 from bin[1] */
        if (len == 1)
        {
            base64[i++] = enc[x];
            base64[i++] = '=';
            base64[i++] = '=';
            break;
        }
        base64[i++] = enc[x | ((bin[1] & 0xf0) >> 4)];
        x = (bin[1] & 0x0f) << 2;

        /* next 6 bits 4 from bin[1] and 2 from bin[2] */
        if (len == 2)
        {
            base64[i++] = enc[x];
            base64[i++] = '=';
            break;
        }
        base64[i++] = enc[x | ((bin[2] & 0xc0) >> 6)];

        /* last 6 bits, all from bin [2] */
        base64[i++] = enc[bin[2] & 0x3f];
        bin += 3;
        len -= 3;
    }
    base64[i] = 0;
    return i;
}

static inline char decode_char( WCHAR c )
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return 64;
}

static unsigned int decode_base64( const WCHAR *base64, unsigned int len, char *buf )
{
    unsigned int i = 0;
    char c0, c1, c2, c3;
    const WCHAR *p = base64;

    while (len > 4)
    {
        if ((c0 = decode_char( p[0] )) > 63) return 0;
        if ((c1 = decode_char( p[1] )) > 63) return 0;
        if ((c2 = decode_char( p[2] )) > 63) return 0;
        if ((c3 = decode_char( p[3] )) > 63) return 0;

        if (buf)
        {
            buf[i + 0] = (c0 << 2) | (c1 >> 4);
            buf[i + 1] = (c1 << 4) | (c2 >> 2);
            buf[i + 2] = (c2 << 6) |  c3;
        }
        len -= 4;
        i += 3;
        p += 4;
    }
    if (p[2] == '=')
    {
        if ((c0 = decode_char( p[0] )) > 63) return 0;
        if ((c1 = decode_char( p[1] )) > 63) return 0;

        if (buf) buf[i] = (c0 << 2) | (c1 >> 4);
        i++;
    }
    else if (p[3] == '=')
    {
        if ((c0 = decode_char( p[0] )) > 63) return 0;
        if ((c1 = decode_char( p[1] )) > 63) return 0;
        if ((c2 = decode_char( p[2] )) > 63) return 0;

        if (buf)
        {
            buf[i + 0] = (c0 << 2) | (c1 >> 4);
            buf[i + 1] = (c1 << 4) | (c2 >> 2);
        }
        i += 2;
    }
    else
    {
        if ((c0 = decode_char( p[0] )) > 63) return 0;
        if ((c1 = decode_char( p[1] )) > 63) return 0;
        if ((c2 = decode_char( p[2] )) > 63) return 0;
        if ((c3 = decode_char( p[3] )) > 63) return 0;

        if (buf)
        {
            buf[i + 0] = (c0 << 2) | (c1 >> 4);
            buf[i + 1] = (c1 << 4) | (c2 >> 2);
            buf[i + 2] = (c2 << 6) |  c3;
        }
        i += 3;
    }
    return i;
}

static struct authinfo *alloc_authinfo(void)
{
    struct authinfo *ret;

    if (!(ret = HeapAlloc(GetProcessHeap(), 0, sizeof(*ret) ))) return NULL;

    SecInvalidateHandle(&ret->cred);
    SecInvalidateHandle(&ret->ctx);
    memset(&ret->exp, 0, sizeof(ret->exp));
    ret->scheme    = 0;
    ret->attr      = 0;
    ret->max_token = 0;
    ret->data      = NULL;
    ret->data_len  = 0;
    ret->finished  = FALSE;
    return ret;
}

static void destroy_authinfo(struct authinfo *info)
{
    if (!info) return;

    if (SecIsValidHandle(&info->ctx))
        DeleteSecurityContext(&info->ctx);
    if (SecIsValidHandle(&info->cred))
        FreeCredentialsHandle(&info->cred);

    HeapFree(GetProcessHeap(), 0, info->data);
    HeapFree(GetProcessHeap(), 0, info);
}

static const WCHAR basicW[]     = {'B','a','s','i','c',0};
static const WCHAR ntlmW[]      = {'N','T','L','M',0};
static const WCHAR passportW[]  = {'P','a','s','s','p','o','r','t',0};
static const WCHAR digestW[]    = {'D','i','g','e','s','t',0};
static const WCHAR negotiateW[] = {'N','e','g','o','t','i','a','t','e',0};

static const struct
{
    const WCHAR *str;
    unsigned int len;
    DWORD        scheme;
}
auth_schemes[] =
{
    { basicW,     ARRAY_SIZE(basicW) - 1,     RPC_C_HTTP_AUTHN_SCHEME_BASIC },
    { ntlmW,      ARRAY_SIZE(ntlmW) - 1,      RPC_C_HTTP_AUTHN_SCHEME_NTLM },
    { passportW,  ARRAY_SIZE(passportW) - 1,  RPC_C_HTTP_AUTHN_SCHEME_PASSPORT },
    { digestW,    ARRAY_SIZE(digestW) - 1,    RPC_C_HTTP_AUTHN_SCHEME_DIGEST },
    { negotiateW, ARRAY_SIZE(negotiateW) - 1, RPC_C_HTTP_AUTHN_SCHEME_NEGOTIATE }
};

static DWORD auth_scheme_from_header( const WCHAR *header )
{
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(auth_schemes); i++)
    {
        if (!_wcsnicmp( header, auth_schemes[i].str, auth_schemes[i].len ) &&
            (header[auth_schemes[i].len] == ' ' || !header[auth_schemes[i].len])) return auth_schemes[i].scheme;
    }
    return 0;
}

static BOOL get_authvalue(HINTERNET request, DWORD scheme, WCHAR *buffer, DWORD buflen)
{
    DWORD len, index = 0;
    for (;;)
    {
        len = buflen;
        if (!HttpQueryInfoW(request, HTTP_QUERY_WWW_AUTHENTICATE, buffer, &len, &index)) return FALSE;
        if (auth_scheme_from_header(buffer) == scheme) break;
    }
    return TRUE;
}

static RPC_STATUS do_authorization(HINTERNET request, SEC_WCHAR *servername,
                                   const RPC_HTTP_TRANSPORT_CREDENTIALS_W *creds, struct authinfo **auth_ptr)
{
    struct authinfo *info = *auth_ptr;
    SEC_WINNT_AUTH_IDENTITY_W *id = creds->TransportCredentials;
    RPC_STATUS status = RPC_S_SERVER_UNAVAILABLE;

    if ((!info && !(info = alloc_authinfo()))) return RPC_S_SERVER_UNAVAILABLE;

    switch (creds->AuthnSchemes[0])
    {
    case RPC_C_HTTP_AUTHN_SCHEME_BASIC:
    {
        int userlen = WideCharToMultiByte(CP_UTF8, 0, id->User, id->UserLength, NULL, 0, NULL, NULL);
        int passlen = WideCharToMultiByte(CP_UTF8, 0, id->Password, id->PasswordLength, NULL, 0, NULL, NULL);

        info->data_len = userlen + passlen + 1;
        if (!(info->data = HeapAlloc(GetProcessHeap(), 0, info->data_len)))
        {
            status = RPC_S_OUT_OF_MEMORY;
            break;
        }
        WideCharToMultiByte(CP_UTF8, 0, id->User, id->UserLength, info->data, userlen, NULL, NULL);
        info->data[userlen] = ':';
        WideCharToMultiByte(CP_UTF8, 0, id->Password, id->PasswordLength, info->data + userlen + 1, passlen, NULL, NULL);

        info->scheme   = RPC_C_HTTP_AUTHN_SCHEME_BASIC;
        info->finished = TRUE;
        status = RPC_S_OK;
        break;
    }
    case RPC_C_HTTP_AUTHN_SCHEME_NTLM:
    case RPC_C_HTTP_AUTHN_SCHEME_NEGOTIATE:
    {

        static SEC_WCHAR ntlmW[] = {'N','T','L','M',0}, negotiateW[] = {'N','e','g','o','t','i','a','t','e',0};
        SECURITY_STATUS ret;
        SecBufferDesc out_desc, in_desc;
        SecBuffer out, in;
        ULONG flags = ISC_REQ_CONNECTION|ISC_REQ_USE_DCE_STYLE|ISC_REQ_MUTUAL_AUTH|ISC_REQ_DELEGATE;
        SEC_WCHAR *scheme;
        int scheme_len;
        const WCHAR *p;
        WCHAR auth_value[2048];
        DWORD size = sizeof(auth_value);
        BOOL first = FALSE;

        if (creds->AuthnSchemes[0] == RPC_C_HTTP_AUTHN_SCHEME_NTLM) scheme = ntlmW;
        else scheme = negotiateW;
        scheme_len = lstrlenW( scheme );

        if (!*auth_ptr)
        {
            TimeStamp exp;
            SecPkgInfoW *pkg_info;

            ret = AcquireCredentialsHandleW(NULL, scheme, SECPKG_CRED_OUTBOUND, NULL, id, NULL, NULL, &info->cred, &exp);
            if (ret != SEC_E_OK) break;

            ret = QuerySecurityPackageInfoW(scheme, &pkg_info);
            if (ret != SEC_E_OK) break;

            info->max_token = pkg_info->cbMaxToken;
            FreeContextBuffer(pkg_info);
            first = TRUE;
        }
        else
        {
            if (info->finished || !get_authvalue(request, creds->AuthnSchemes[0], auth_value, size)) break;
            if (auth_scheme_from_header(auth_value) != info->scheme)
            {
                ERR("authentication scheme changed\n");
                break;
            }
        }
        in.BufferType = SECBUFFER_TOKEN;
        in.cbBuffer   = 0;
        in.pvBuffer   = NULL;

        in_desc.ulVersion = 0;
        in_desc.cBuffers  = 1;
        in_desc.pBuffers  = &in;

        p = auth_value + scheme_len;
        if (!first && *p == ' ')
        {
            int len = lstrlenW(++p);
            in.cbBuffer = decode_base64(p, len, NULL);
            if (!(in.pvBuffer = HeapAlloc(GetProcessHeap(), 0, in.cbBuffer))) break;
            decode_base64(p, len, in.pvBuffer);
        }
        out.BufferType = SECBUFFER_TOKEN;
        out.cbBuffer   = info->max_token;
        if (!(out.pvBuffer = HeapAlloc(GetProcessHeap(), 0, out.cbBuffer)))
        {
            HeapFree(GetProcessHeap(), 0, in.pvBuffer);
            break;
        }
        out_desc.ulVersion = 0;
        out_desc.cBuffers  = 1;
        out_desc.pBuffers  = &out;

        ret = InitializeSecurityContextW(first ? &info->cred : NULL, first ? NULL : &info->ctx,
                                         first ? servername : NULL, flags, 0, SECURITY_NETWORK_DREP,
                                         in.pvBuffer ? &in_desc : NULL, 0, &info->ctx, &out_desc,
                                         &info->attr, &info->exp);
        HeapFree(GetProcessHeap(), 0, in.pvBuffer);
        if (ret == SEC_E_OK)
        {
            HeapFree(GetProcessHeap(), 0, info->data);
            info->data     = out.pvBuffer;
            info->data_len = out.cbBuffer;
            info->finished = TRUE;
            TRACE("sending last auth packet\n");
            status = RPC_S_OK;
        }
        else if (ret == SEC_I_CONTINUE_NEEDED)
        {
            HeapFree(GetProcessHeap(), 0, info->data);
            info->data     = out.pvBuffer;
            info->data_len = out.cbBuffer;
            TRACE("sending next auth packet\n");
            status = RPC_S_OK;
        }
        else
        {
            ERR("InitializeSecurityContextW failed with error 0x%08x\n", ret);
            HeapFree(GetProcessHeap(), 0, out.pvBuffer);
            break;
        }
        info->scheme = creds->AuthnSchemes[0];
        break;
    }
    default:
        FIXME("scheme %u not supported\n", creds->AuthnSchemes[0]);
        break;
    }

    if (status != RPC_S_OK)
    {
        destroy_authinfo(info);
        *auth_ptr = NULL;
        return status;
    }
    *auth_ptr = info;
    return RPC_S_OK;
}

static RPC_STATUS insert_authorization_header(HINTERNET request, ULONG scheme, char *data, int data_len)
{
    static const WCHAR authW[] = {'A','u','t','h','o','r','i','z','a','t','i','o','n',':',' '};
    static const WCHAR basicW[] = {'B','a','s','i','c',' '};
    static const WCHAR negotiateW[] = {'N','e','g','o','t','i','a','t','e',' '};
    static const WCHAR ntlmW[] = {'N','T','L','M',' '};
    int scheme_len, auth_len = ARRAY_SIZE(authW), len = ((data_len + 2) * 4) / 3;
    const WCHAR *scheme_str;
    WCHAR *header, *ptr;
    RPC_STATUS status = RPC_S_SERVER_UNAVAILABLE;

    switch (scheme)
    {
    case RPC_C_HTTP_AUTHN_SCHEME_BASIC:
        scheme_str = basicW;
        scheme_len = ARRAY_SIZE(basicW);
        break;
    case RPC_C_HTTP_AUTHN_SCHEME_NEGOTIATE:
        scheme_str = negotiateW;
        scheme_len = ARRAY_SIZE(negotiateW);
        break;
    case RPC_C_HTTP_AUTHN_SCHEME_NTLM:
        scheme_str = ntlmW;
        scheme_len = ARRAY_SIZE(ntlmW);
        break;
    default:
        ERR("unknown scheme %u\n", scheme);
        return RPC_S_SERVER_UNAVAILABLE;
    }
    if ((header = HeapAlloc(GetProcessHeap(), 0, (auth_len + scheme_len + len + 2) * sizeof(WCHAR))))
    {
        memcpy(header, authW, auth_len * sizeof(WCHAR));
        ptr = header + auth_len;
        memcpy(ptr, scheme_str, scheme_len * sizeof(WCHAR));
        ptr += scheme_len;
        len = encode_base64(data, data_len, ptr);
        ptr[len++] = '\r';
        ptr[len++] = '\n';
        ptr[len] = 0;
        if (HttpAddRequestHeadersW(request, header, -1, HTTP_ADDREQ_FLAG_ADD|HTTP_ADDREQ_FLAG_REPLACE))
            status = RPC_S_OK;
        HeapFree(GetProcessHeap(), 0, header);
    }
    return status;
}

static void drain_content(HINTERNET request, RpcHttpAsyncData *async_data, HANDLE cancel_event)
{
    DWORD count, len = 0, size = sizeof(len);
    char buf[2048];

    HttpQueryInfoW(request, HTTP_QUERY_FLAG_NUMBER|HTTP_QUERY_CONTENT_LENGTH, &len, &size, NULL);
    if (!len) return;
    for (;;)
    {
        count = min(sizeof(buf), len);
        if (rpcrt4_http_async_read(request, async_data, cancel_event, buf, count) <= 0) return;
        len -= count;
    }
}

static RPC_STATUS authorize_request(RpcConnection_http *httpc, HINTERNET request)
{
    static const WCHAR authW[] = {'A','u','t','h','o','r','i','z','a','t','i','o','n',':','\r','\n',0};
    struct authinfo *info = NULL;
    RPC_STATUS status;
    BOOL ret;

    for (;;)
    {
        status = do_authorization(request, httpc->servername, httpc->common.QOS->qos->u.HttpCredentials, &info);
        if (status != RPC_S_OK) break;

        status = insert_authorization_header(request, info->scheme, info->data, info->data_len);
        if (status != RPC_S_OK) break;

        prepare_async_request(httpc->async_data);
        ret = HttpSendRequestW(request, NULL, 0, NULL, 0);
        status = wait_async_request(httpc->async_data, ret, httpc->cancel_event);
        if (status != RPC_S_OK || info->finished) break;

        status = rpcrt4_http_check_response(request);
        if (status != RPC_S_OK && status != ERROR_ACCESS_DENIED) break;
        drain_content(request, httpc->async_data, httpc->cancel_event);
    }

    if (info->scheme != RPC_C_HTTP_AUTHN_SCHEME_BASIC)
        HttpAddRequestHeadersW(request, authW, -1, HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD);

    destroy_authinfo(info);
    return status;
}

static BOOL has_credentials(RpcConnection_http *httpc)
{
    RPC_HTTP_TRANSPORT_CREDENTIALS_W *creds;
    SEC_WINNT_AUTH_IDENTITY_W *id;

    if (!httpc->common.QOS || httpc->common.QOS->qos->AdditionalSecurityInfoType != RPC_C_AUTHN_INFO_TYPE_HTTP)
        return FALSE;

    creds = httpc->common.QOS->qos->u.HttpCredentials;
    if (creds->AuthenticationTarget != RPC_C_HTTP_AUTHN_TARGET_SERVER || !creds->NumberOfAuthnSchemes)
        return FALSE;

    id = creds->TransportCredentials;
    if (!id || !id->User || !id->Password) return FALSE;

    return TRUE;
}

static BOOL is_secure(RpcConnection_http *httpc)
{
    return httpc->common.QOS &&
           (httpc->common.QOS->qos->AdditionalSecurityInfoType == RPC_C_AUTHN_INFO_TYPE_HTTP) &&
           (httpc->common.QOS->qos->u.HttpCredentials->Flags & RPC_C_HTTP_FLAG_USE_SSL);
}

static RPC_STATUS set_auth_cookie(RpcConnection_http *httpc, const WCHAR *value)
{
    static WCHAR httpW[] = {'h','t','t','p',0};
    static WCHAR httpsW[] = {'h','t','t','p','s',0};
    URL_COMPONENTSW uc;
    DWORD len;
    WCHAR *url;
    BOOL ret;

    if (!value) return RPC_S_OK;

    uc.dwStructSize     = sizeof(uc);
    uc.lpszScheme       = is_secure(httpc) ? httpsW : httpW;
    uc.dwSchemeLength   = 0;
    uc.lpszHostName     = httpc->servername;
    uc.dwHostNameLength = 0;
    uc.nPort            = 0;
    uc.lpszUserName     = NULL;
    uc.dwUserNameLength = 0;
    uc.lpszPassword     = NULL;
    uc.dwPasswordLength = 0;
    uc.lpszUrlPath      = NULL;
    uc.dwUrlPathLength  = 0;
    uc.lpszExtraInfo    = NULL;
    uc.dwExtraInfoLength = 0;

    if (!InternetCreateUrlW(&uc, 0, NULL, &len) && (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
        return RPC_S_SERVER_UNAVAILABLE;

    if (!(url = HeapAlloc(GetProcessHeap(), 0, len))) return RPC_S_OUT_OF_MEMORY;

    len = len / sizeof(WCHAR) - 1;
    if (!InternetCreateUrlW(&uc, 0, url, &len))
    {
        HeapFree(GetProcessHeap(), 0, url);
        return RPC_S_SERVER_UNAVAILABLE;
    }

    ret = InternetSetCookieW(url, NULL, value);
    HeapFree(GetProcessHeap(), 0, url);
    if (!ret) return RPC_S_SERVER_UNAVAILABLE;

    return RPC_S_OK;
}

static RPC_STATUS rpcrt4_ncacn_http_open(RpcConnection* Connection)
{
    RpcConnection_http *httpc = (RpcConnection_http *)Connection;
    static const WCHAR wszVerbIn[] = {'R','P','C','_','I','N','_','D','A','T','A',0};
    static const WCHAR wszVerbOut[] = {'R','P','C','_','O','U','T','_','D','A','T','A',0};
    static const WCHAR wszRpcProxyPrefix[] = {'/','r','p','c','/','r','p','c','p','r','o','x','y','.','d','l','l','?',0};
    static const WCHAR wszColon[] = {':',0};
    static const WCHAR wszAcceptType[] = {'a','p','p','l','i','c','a','t','i','o','n','/','r','p','c',0};
    LPCWSTR wszAcceptTypes[] = { wszAcceptType, NULL };
    DWORD flags;
    WCHAR *url;
    RPC_STATUS status;
    BOOL secure, credentials;
    HttpTimerThreadData *timer_data;
    HANDLE thread;

    TRACE("(%s, %s)\n", Connection->NetworkAddr, Connection->Endpoint);

    if (Connection->server)
    {
        ERR("ncacn_http servers not supported yet\n");
        return RPC_S_SERVER_UNAVAILABLE;
    }

    if (httpc->in_request)
        return RPC_S_OK;

    httpc->async_data->completion_event = CreateEventW(NULL, FALSE, FALSE, NULL);

    UuidCreate(&httpc->connection_uuid);
    UuidCreate(&httpc->in_pipe_uuid);
    UuidCreate(&httpc->out_pipe_uuid);

    status = rpcrt4_http_internet_connect(httpc);
    if (status != RPC_S_OK)
        return status;

    url = HeapAlloc(GetProcessHeap(), 0, sizeof(wszRpcProxyPrefix) + (strlen(Connection->NetworkAddr) + 1 + strlen(Connection->Endpoint))*sizeof(WCHAR));
    if (!url)
        return RPC_S_OUT_OF_MEMORY;
    memcpy(url, wszRpcProxyPrefix, sizeof(wszRpcProxyPrefix));
    MultiByteToWideChar(CP_ACP, 0, Connection->NetworkAddr, -1, url+ARRAY_SIZE(wszRpcProxyPrefix)-1,
                        strlen(Connection->NetworkAddr)+1);
    lstrcatW(url, wszColon);
    MultiByteToWideChar(CP_ACP, 0, Connection->Endpoint, -1, url+lstrlenW(url), strlen(Connection->Endpoint)+1);

    secure = is_secure(httpc);
    credentials = has_credentials(httpc);

    flags = INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_NO_CACHE_WRITE |
            INTERNET_FLAG_NO_AUTO_REDIRECT;
    if (secure) flags |= INTERNET_FLAG_SECURE;
    if (credentials) flags |= INTERNET_FLAG_NO_AUTH;

    status = set_auth_cookie(httpc, Connection->CookieAuth);
    if (status != RPC_S_OK)
    {
        HeapFree(GetProcessHeap(), 0, url);
        return status;
    }
    httpc->in_request = HttpOpenRequestW(httpc->session, wszVerbIn, url, NULL, NULL, wszAcceptTypes,
                                         flags, (DWORD_PTR)httpc->async_data);
    if (!httpc->in_request)
    {
        ERR("HttpOpenRequestW failed with error %d\n", GetLastError());
        HeapFree(GetProcessHeap(), 0, url);
        return RPC_S_SERVER_UNAVAILABLE;
    }

    if (credentials)
    {
        status = authorize_request(httpc, httpc->in_request);
        if (status != RPC_S_OK)
        {
            HeapFree(GetProcessHeap(), 0, url);
            return status;
        }
        status = rpcrt4_http_check_response(httpc->in_request);
        if (status != RPC_S_OK)
        {
            HeapFree(GetProcessHeap(), 0, url);
            return status;
        }
        drain_content(httpc->in_request, httpc->async_data, httpc->cancel_event);
    }

    httpc->out_request = HttpOpenRequestW(httpc->session, wszVerbOut, url, NULL, NULL, wszAcceptTypes,
                                          flags, (DWORD_PTR)httpc->async_data);
    HeapFree(GetProcessHeap(), 0, url);
    if (!httpc->out_request)
    {
        ERR("HttpOpenRequestW failed with error %d\n", GetLastError());
        return RPC_S_SERVER_UNAVAILABLE;
    }

    if (credentials)
    {
        status = authorize_request(httpc, httpc->out_request);
        if (status != RPC_S_OK)
            return status;
    }

    status = rpcrt4_http_prepare_in_pipe(httpc->in_request, httpc->async_data, httpc->cancel_event,
                                         &httpc->connection_uuid, &httpc->in_pipe_uuid,
                                         &Connection->assoc->http_uuid, credentials);
    if (status != RPC_S_OK)
        return status;

    status = rpcrt4_http_prepare_out_pipe(httpc->out_request, httpc->async_data, httpc->cancel_event,
                                          &httpc->connection_uuid, &httpc->out_pipe_uuid,
                                          &httpc->flow_control_increment, credentials);
    if (status != RPC_S_OK)
        return status;

    httpc->flow_control_mark = httpc->flow_control_increment / 2;
    httpc->last_sent_time = GetTickCount();
    httpc->timer_cancelled = CreateEventW(NULL, FALSE, FALSE, NULL);

    timer_data = HeapAlloc(GetProcessHeap(), 0, sizeof(*timer_data));
    if (!timer_data)
        return ERROR_OUTOFMEMORY;
    timer_data->timer_param = httpc->in_request;
    timer_data->last_sent_time = &httpc->last_sent_time;
    timer_data->timer_cancelled = httpc->timer_cancelled;
    /* FIXME: should use CreateTimerQueueTimer when implemented */
    thread = CreateThread(NULL, 0, rpcrt4_http_timer_thread, timer_data, 0, NULL);
    if (!thread)
    {
        HeapFree(GetProcessHeap(), 0, timer_data);
        return GetLastError();
    }
    CloseHandle(thread);

    return RPC_S_OK;
}

static RPC_STATUS rpcrt4_ncacn_http_handoff(RpcConnection *old_conn, RpcConnection *new_conn)
{
    assert(0);
    return RPC_S_SERVER_UNAVAILABLE;
}

static int rpcrt4_ncacn_http_read(RpcConnection *Connection,
                                void *buffer, unsigned int count)
{
  RpcConnection_http *httpc = (RpcConnection_http *) Connection;
  return rpcrt4_http_async_read(httpc->out_request, httpc->async_data, httpc->cancel_event, buffer, count);
}

static RPC_STATUS rpcrt4_ncacn_http_receive_fragment(RpcConnection *Connection, RpcPktHdr **Header, void **Payload)
{
  RpcConnection_http *httpc = (RpcConnection_http *) Connection;
  RPC_STATUS status;
  DWORD hdr_length;
  LONG dwRead;
  RpcPktCommonHdr common_hdr;

  *Header = NULL;

  TRACE("(%p, %p, %p)\n", Connection, Header, Payload);

again:
  /* read packet common header */
  dwRead = rpcrt4_ncacn_http_read(Connection, &common_hdr, sizeof(common_hdr));
  if (dwRead != sizeof(common_hdr)) {
    WARN("Short read of header, %d bytes\n", dwRead);
    status = RPC_S_PROTOCOL_ERROR;
    goto fail;
  }
  if (!memcmp(&common_hdr, "HTTP/1.1", sizeof("HTTP/1.1")) ||
      !memcmp(&common_hdr, "HTTP/1.0", sizeof("HTTP/1.0")))
  {
    FIXME("server returned %s\n", debugstr_a((const char *)&common_hdr));
    status = RPC_S_PROTOCOL_ERROR;
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
  if (!*Header)
  {
    status = RPC_S_OUT_OF_RESOURCES;
    goto fail;
  }
  memcpy(*Header, &common_hdr, sizeof(common_hdr));

  /* read the rest of packet header */
  dwRead = rpcrt4_ncacn_http_read(Connection, &(*Header)->common + 1, hdr_length - sizeof(common_hdr));
  if (dwRead != hdr_length - sizeof(common_hdr)) {
    WARN("bad header length, %d bytes, hdr_length %d\n", dwRead, hdr_length);
    status = RPC_S_PROTOCOL_ERROR;
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

    dwRead = rpcrt4_ncacn_http_read(Connection, *Payload, common_hdr.frag_len - hdr_length);
    if (dwRead != common_hdr.frag_len - hdr_length)
    {
      WARN("bad data length, %d/%d\n", dwRead, common_hdr.frag_len - hdr_length);
      status = RPC_S_PROTOCOL_ERROR;
      goto fail;
    }
  }
  else
    *Payload = NULL;

  if ((*Header)->common.ptype == PKT_HTTP)
  {
    if (!RPCRT4_IsValidHttpPacket(*Header, *Payload, common_hdr.frag_len - hdr_length))
    {
      ERR("invalid http packet of length %d bytes\n", (*Header)->common.frag_len);
      status = RPC_S_PROTOCOL_ERROR;
      goto fail;
    }
    if ((*Header)->http.flags == 0x0001)
    {
      TRACE("http idle packet, waiting for real packet\n");
      if ((*Header)->http.num_data_items != 0)
      {
        ERR("HTTP idle packet should have no data items instead of %d\n", (*Header)->http.num_data_items);
        status = RPC_S_PROTOCOL_ERROR;
        goto fail;
      }
    }
    else if ((*Header)->http.flags == 0x0002)
    {
      ULONG bytes_transmitted;
      ULONG flow_control_increment;
      UUID pipe_uuid;
      status = RPCRT4_ParseHttpFlowControlHeader(*Header, *Payload,
                                                 Connection->server,
                                                 &bytes_transmitted,
                                                 &flow_control_increment,
                                                 &pipe_uuid);
      if (status != RPC_S_OK)
        goto fail;
      TRACE("received http flow control header (0x%x, 0x%x, %s)\n",
            bytes_transmitted, flow_control_increment, debugstr_guid(&pipe_uuid));
      /* FIXME: do something with parsed data */
    }
    else
    {
      FIXME("unrecognised http packet with flags 0x%04x\n", (*Header)->http.flags);
      status = RPC_S_PROTOCOL_ERROR;
      goto fail;
    }
    RPCRT4_FreeHeader(*Header);
    *Header = NULL;
    HeapFree(GetProcessHeap(), 0, *Payload);
    *Payload = NULL;
    goto again;
  }

  /* success */
  status = RPC_S_OK;

  httpc->bytes_received += common_hdr.frag_len;

  TRACE("httpc->bytes_received = 0x%x\n", httpc->bytes_received);

  if (httpc->bytes_received > httpc->flow_control_mark)
  {
    RpcPktHdr *hdr = RPCRT4_BuildHttpFlowControlHeader(httpc->common.server,
                                                       httpc->bytes_received,
                                                       httpc->flow_control_increment,
                                                       &httpc->out_pipe_uuid);
    if (hdr)
    {
      DWORD bytes_written;
      BOOL ret2;
      TRACE("sending flow control packet at 0x%x\n", httpc->bytes_received);
      ret2 = InternetWriteFile(httpc->in_request, hdr, hdr->common.frag_len, &bytes_written);
      RPCRT4_FreeHeader(hdr);
      if (ret2)
        httpc->flow_control_mark = httpc->bytes_received + httpc->flow_control_increment / 2;
    }
  }

fail:
  if (status != RPC_S_OK) {
    RPCRT4_FreeHeader(*Header);
    *Header = NULL;
    HeapFree(GetProcessHeap(), 0, *Payload);
    *Payload = NULL;
  }
  return status;
}

static int rpcrt4_ncacn_http_write(RpcConnection *Connection,
                                 const void *buffer, unsigned int count)
{
  RpcConnection_http *httpc = (RpcConnection_http *) Connection;
  DWORD bytes_written;
  BOOL ret;

  httpc->last_sent_time = ~0U; /* disable idle packet sending */
  ret = InternetWriteFile(httpc->in_request, buffer, count, &bytes_written);
  httpc->last_sent_time = GetTickCount();
  TRACE("%p %p %u -> %s\n", httpc->in_request, buffer, count, ret ? "TRUE" : "FALSE");
  return ret ? bytes_written : -1;
}

static int rpcrt4_ncacn_http_close(RpcConnection *Connection)
{
  RpcConnection_http *httpc = (RpcConnection_http *) Connection;

  TRACE("\n");

  SetEvent(httpc->timer_cancelled);
  if (httpc->in_request)
    InternetCloseHandle(httpc->in_request);
  httpc->in_request = NULL;
  if (httpc->out_request)
    InternetCloseHandle(httpc->out_request);
  httpc->out_request = NULL;
  if (httpc->app_info)
    InternetCloseHandle(httpc->app_info);
  httpc->app_info = NULL;
  if (httpc->session)
    InternetCloseHandle(httpc->session);
  httpc->session = NULL;
  RpcHttpAsyncData_Release(httpc->async_data);
  if (httpc->cancel_event)
    CloseHandle(httpc->cancel_event);
  HeapFree(GetProcessHeap(), 0, httpc->servername);
  httpc->servername = NULL;

  return 0;
}

static void rpcrt4_ncacn_http_close_read(RpcConnection *conn)
{
    rpcrt4_ncacn_http_close(conn); /* FIXME */
}

static void rpcrt4_ncacn_http_cancel_call(RpcConnection *Connection)
{
  RpcConnection_http *httpc = (RpcConnection_http *) Connection;

  SetEvent(httpc->cancel_event);
}

static RPC_STATUS rpcrt4_ncacn_http_is_server_listening(const char *endpoint)
{
    FIXME("\n");
    return RPC_S_ACCESS_DENIED;
}

static int rpcrt4_ncacn_http_wait_for_incoming_data(RpcConnection *Connection)
{
  RpcConnection_http *httpc = (RpcConnection_http *) Connection;
  BOOL ret;
  RPC_STATUS status;

  prepare_async_request(httpc->async_data);
  ret = InternetQueryDataAvailable(httpc->out_request,
    &httpc->async_data->inet_buffers.dwBufferLength, IRF_ASYNC, 0);
  status = wait_async_request(httpc->async_data, ret, httpc->cancel_event);
  return status == RPC_S_OK ? 0 : -1;
}

static size_t rpcrt4_ncacn_http_get_top_of_tower(unsigned char *tower_data,
                                                 const char *networkaddr,
                                                 const char *endpoint)
{
    return rpcrt4_ip_tcp_get_top_of_tower(tower_data, networkaddr,
                                          EPM_PROTOCOL_HTTP, endpoint);
}

static RPC_STATUS rpcrt4_ncacn_http_parse_top_of_tower(const unsigned char *tower_data,
                                                       size_t tower_size,
                                                       char **networkaddr,
                                                       char **endpoint)
{
    return rpcrt4_ip_tcp_parse_top_of_tower(tower_data, tower_size,
                                            networkaddr, EPM_PROTOCOL_HTTP,
                                            endpoint);
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
    rpcrt4_conn_np_close_read,
    rpcrt4_conn_np_cancel_call,
    rpcrt4_ncacn_np_is_server_listening,
    rpcrt4_conn_np_wait_for_incoming_data,
    rpcrt4_ncacn_np_get_top_of_tower,
    rpcrt4_ncacn_np_parse_top_of_tower,
    NULL,
    RPCRT4_default_is_authorized,
    RPCRT4_default_authorize,
    RPCRT4_default_secure_packet,
    rpcrt4_conn_np_impersonate_client,
    rpcrt4_conn_np_revert_to_self,
    RPCRT4_default_inquire_auth_client,
  },
  { "ncalrpc",
    { EPM_PROTOCOL_NCALRPC, EPM_PROTOCOL_PIPE },
    rpcrt4_conn_np_alloc,
    rpcrt4_ncalrpc_open,
    rpcrt4_ncalrpc_handoff,
    rpcrt4_conn_np_read,
    rpcrt4_conn_np_write,
    rpcrt4_conn_np_close,
    rpcrt4_conn_np_close_read,
    rpcrt4_conn_np_cancel_call,
    rpcrt4_ncalrpc_np_is_server_listening,
    rpcrt4_conn_np_wait_for_incoming_data,
    rpcrt4_ncalrpc_get_top_of_tower,
    rpcrt4_ncalrpc_parse_top_of_tower,
    NULL,
    rpcrt4_ncalrpc_is_authorized,
    rpcrt4_ncalrpc_authorize,
    rpcrt4_ncalrpc_secure_packet,
    rpcrt4_conn_np_impersonate_client,
    rpcrt4_conn_np_revert_to_self,
    rpcrt4_ncalrpc_inquire_auth_client,
  },
  { "ncacn_ip_tcp",
    { EPM_PROTOCOL_NCACN, EPM_PROTOCOL_TCP },
    rpcrt4_conn_tcp_alloc,
    rpcrt4_ncacn_ip_tcp_open,
    rpcrt4_conn_tcp_handoff,
    rpcrt4_conn_tcp_read,
    rpcrt4_conn_tcp_write,
    rpcrt4_conn_tcp_close,
    rpcrt4_conn_tcp_close_read,
    rpcrt4_conn_tcp_cancel_call,
    rpcrt4_conn_tcp_is_server_listening,
    rpcrt4_conn_tcp_wait_for_incoming_data,
    rpcrt4_ncacn_ip_tcp_get_top_of_tower,
    rpcrt4_ncacn_ip_tcp_parse_top_of_tower,
    NULL,
    RPCRT4_default_is_authorized,
    RPCRT4_default_authorize,
    RPCRT4_default_secure_packet,
    RPCRT4_default_impersonate_client,
    RPCRT4_default_revert_to_self,
    RPCRT4_default_inquire_auth_client,
  },
  { "ncacn_http",
    { EPM_PROTOCOL_NCACN, EPM_PROTOCOL_HTTP },
    rpcrt4_ncacn_http_alloc,
    rpcrt4_ncacn_http_open,
    rpcrt4_ncacn_http_handoff,
    rpcrt4_ncacn_http_read,
    rpcrt4_ncacn_http_write,
    rpcrt4_ncacn_http_close,
    rpcrt4_ncacn_http_close_read,
    rpcrt4_ncacn_http_cancel_call,
    rpcrt4_ncacn_http_is_server_listening,
    rpcrt4_ncacn_http_wait_for_incoming_data,
    rpcrt4_ncacn_http_get_top_of_tower,
    rpcrt4_ncacn_http_parse_top_of_tower,
    rpcrt4_ncacn_http_receive_fragment,
    RPCRT4_default_is_authorized,
    RPCRT4_default_authorize,
    RPCRT4_default_secure_packet,
    RPCRT4_default_impersonate_client,
    RPCRT4_default_revert_to_self,
    RPCRT4_default_inquire_auth_client,
  },
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

const struct protseq_ops *rpcrt4_get_protseq_ops(const char *protseq)
{
  unsigned int i;
  for(i = 0; i < ARRAY_SIZE(protseq_list); i++)
    if (!strcmp(protseq_list[i].name, protseq))
      return &protseq_list[i];
  return NULL;
}

static const struct connection_ops *rpcrt4_get_conn_protseq_ops(const char *protseq)
{
    unsigned int i;
    for(i = 0; i < ARRAY_SIZE(conn_protseq_list); i++)
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
    LPCWSTR NetworkOptions, RpcAuthInfo* AuthInfo, RpcQualityOfService *QOS, LPCWSTR CookieAuth)
{
  static LONG next_id;
  const struct connection_ops *ops;
  RpcConnection* NewConnection;

  ops = rpcrt4_get_conn_protseq_ops(Protseq);
  if (!ops)
  {
    FIXME("not supported for protseq %s\n", Protseq);
    return RPC_S_PROTSEQ_NOT_SUPPORTED;
  }

  NewConnection = ops->alloc();
  NewConnection->ref = 1;
  NewConnection->server = server;
  NewConnection->ops = ops;
  NewConnection->NetworkAddr = RPCRT4_strdupA(NetworkAddr);
  NewConnection->Endpoint = RPCRT4_strdupA(Endpoint);
  NewConnection->NetworkOptions = RPCRT4_strdupW(NetworkOptions);
  NewConnection->CookieAuth = RPCRT4_strdupW(CookieAuth);
  NewConnection->MaxTransmissionSize = RPC_MAX_PACKET_SIZE;
  NewConnection->NextCallId = 1;

  SecInvalidateHandle(&NewConnection->ctx);
  if (AuthInfo) RpcAuthInfo_AddRef(AuthInfo);
  NewConnection->AuthInfo = AuthInfo;
  NewConnection->auth_context_id = InterlockedIncrement( &next_id );
  if (QOS) RpcQualityOfService_AddRef(QOS);
  NewConnection->QOS = QOS;

  list_init(&NewConnection->conn_pool_entry);
  list_init(&NewConnection->protseq_entry);

  TRACE("connection: %p\n", NewConnection);
  *Connection = NewConnection;

  return RPC_S_OK;
}

static RpcConnection *rpcrt4_spawn_connection(RpcConnection *old_connection)
{
    RpcConnection *connection;
    RPC_STATUS err;

    err = RPCRT4_CreateConnection(&connection, old_connection->server, rpcrt4_conn_get_name(old_connection),
                                  old_connection->NetworkAddr, old_connection->Endpoint, NULL,
                                  old_connection->AuthInfo, old_connection->QOS, old_connection->CookieAuth);
    if (err != RPC_S_OK)
        return NULL;

    rpcrt4_conn_handoff(old_connection, connection);
    if (old_connection->protseq)
    {
        EnterCriticalSection(&old_connection->protseq->cs);
        connection->protseq = old_connection->protseq;
        list_add_tail(&old_connection->protseq->connections, &connection->protseq_entry);
        LeaveCriticalSection(&old_connection->protseq->cs);
    }
    return connection;
}

void rpcrt4_conn_release_and_wait(RpcConnection *connection)
{
    HANDLE event = NULL;

    if (connection->ref > 1)
        event = connection->wait_release = CreateEventW(NULL, TRUE, FALSE, NULL);

    RPCRT4_ReleaseConnection(connection);

    if(event)
    {
        WaitForSingleObject(event, INFINITE);
        CloseHandle(event);
    }
}

RpcConnection *RPCRT4_GrabConnection(RpcConnection *connection)
{
    LONG ref = InterlockedIncrement(&connection->ref);
    TRACE("%p ref=%u\n", connection, ref);
    return connection;
}

void RPCRT4_ReleaseConnection(RpcConnection *connection)
{
    LONG ref;

    /* protseq stores a list of active connections, but does not own references to them.
     * It may need to grab a connection from the list, which could lead to a race if
     * connection is being released, but not yet removed from the list. We handle that
     * by synchronizing on CS here. */
    if (connection->protseq)
    {
        EnterCriticalSection(&connection->protseq->cs);
        ref = InterlockedDecrement(&connection->ref);
        if (!ref)
            list_remove(&connection->protseq_entry);
        LeaveCriticalSection(&connection->protseq->cs);
    }
    else
    {
        ref = InterlockedDecrement(&connection->ref);
    }

    TRACE("%p ref=%u\n", connection, ref);

    if (!ref)
    {
        RPCRT4_CloseConnection(connection);
        RPCRT4_strfree(connection->Endpoint);
        RPCRT4_strfree(connection->NetworkAddr);
        HeapFree(GetProcessHeap(), 0, connection->NetworkOptions);
        HeapFree(GetProcessHeap(), 0, connection->CookieAuth);
        if (connection->AuthInfo) RpcAuthInfo_Release(connection->AuthInfo);
        if (connection->QOS) RpcQualityOfService_Release(connection->QOS);

        /* server-only */
        if (connection->server_binding) RPCRT4_ReleaseBinding(connection->server_binding);
        else if (connection->assoc) RpcAssoc_ConnectionReleased(connection->assoc);

        if (connection->wait_release) SetEvent(connection->wait_release);

        HeapFree(GetProcessHeap(), 0, connection);
    }
}

RPC_STATUS RPCRT4_IsServerListening(const char *protseq, const char *endpoint)
{
  const struct connection_ops *ops;

  ops = rpcrt4_get_conn_protseq_ops(protseq);
  if (!ops)
  {
    FIXME("not supported for protseq %s\n", protseq);
    return RPC_S_INVALID_BINDING;
  }

  return ops->is_server_listening(endpoint);
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
    unsigned int i;

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

    for(i = 0; i < ARRAY_SIZE(conn_protseq_list); i++)
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

/***********************************************************************
 *             RpcProtseqVectorFreeA (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcProtseqVectorFreeA(RPC_PROTSEQ_VECTORA **protseqs)
{
  TRACE("(%p)\n", protseqs);

  if (*protseqs)
  {
    unsigned int i;
    for (i = 0; i < (*protseqs)->Count; i++)
      HeapFree(GetProcessHeap(), 0, (*protseqs)->Protseq[i]);
    HeapFree(GetProcessHeap(), 0, *protseqs);
    *protseqs = NULL;
  }
  return RPC_S_OK;
}

/***********************************************************************
 *             RpcProtseqVectorFreeW (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcProtseqVectorFreeW(RPC_PROTSEQ_VECTORW **protseqs)
{
  TRACE("(%p)\n", protseqs);

  if (*protseqs)
  {
    unsigned int i;
    for (i = 0; i < (*protseqs)->Count; i++)
      HeapFree(GetProcessHeap(), 0, (*protseqs)->Protseq[i]);
    HeapFree(GetProcessHeap(), 0, *protseqs);
    *protseqs = NULL;
  }
  return RPC_S_OK;
}

/***********************************************************************
 *             RpcNetworkInqProtseqsW (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcNetworkInqProtseqsW( RPC_PROTSEQ_VECTORW** protseqs )
{
  RPC_PROTSEQ_VECTORW *pvector;
  unsigned int i;
  RPC_STATUS status = RPC_S_OUT_OF_MEMORY;

  TRACE("(%p)\n", protseqs);

  *protseqs = HeapAlloc(GetProcessHeap(), 0, sizeof(RPC_PROTSEQ_VECTORW)+(sizeof(unsigned short*)*ARRAY_SIZE(protseq_list)));
  if (!*protseqs)
    goto end;
  pvector = *protseqs;
  pvector->Count = 0;
  for (i = 0; i < ARRAY_SIZE(protseq_list); i++)
  {
    pvector->Protseq[i] = HeapAlloc(GetProcessHeap(), 0, (strlen(protseq_list[i].name)+1)*sizeof(unsigned short));
    if (pvector->Protseq[i] == NULL)
      goto end;
    MultiByteToWideChar(CP_ACP, 0, (CHAR*)protseq_list[i].name, -1,
      (WCHAR*)pvector->Protseq[i], strlen(protseq_list[i].name) + 1);
    pvector->Count++;
  }
  status = RPC_S_OK;

end:
  if (status != RPC_S_OK)
    RpcProtseqVectorFreeW(protseqs);
  return status;
}

/***********************************************************************
 *             RpcNetworkInqProtseqsA (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcNetworkInqProtseqsA(RPC_PROTSEQ_VECTORA** protseqs)
{
  RPC_PROTSEQ_VECTORA *pvector;
  unsigned int i;
  RPC_STATUS status = RPC_S_OUT_OF_MEMORY;

  TRACE("(%p)\n", protseqs);

  *protseqs = HeapAlloc(GetProcessHeap(), 0, sizeof(RPC_PROTSEQ_VECTORW)+(sizeof(unsigned char*)*ARRAY_SIZE(protseq_list)));
  if (!*protseqs)
    goto end;
  pvector = *protseqs;
  pvector->Count = 0;
  for (i = 0; i < ARRAY_SIZE(protseq_list); i++)
  {
    pvector->Protseq[i] = HeapAlloc(GetProcessHeap(), 0, strlen(protseq_list[i].name)+1);
    if (pvector->Protseq[i] == NULL)
      goto end;
    strcpy((char*)pvector->Protseq[i], protseq_list[i].name);
    pvector->Count++;
  }
  status = RPC_S_OK;

end:
  if (status != RPC_S_OK)
    RpcProtseqVectorFreeA(protseqs);
  return status;
}
