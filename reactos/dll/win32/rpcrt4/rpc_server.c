/*
 * RPC server API
 *
 * Copyright 2001 Ove Kåven, TransGaming Technologies
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
 *  - a whole lot
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"

#include "rpc.h"
#include "rpcndr.h"
#include "excpt.h"

#include "wine/debug.h"
#include "wine/exception.h"

#include "rpc_server.h"
#include "rpc_misc.h"
#include "rpc_message.h"
#include "rpc_defs.h"

#define MAX_THREADS 128

WINE_DEFAULT_DEBUG_CHANNEL(rpc);

typedef struct _RpcPacket
{
  struct _RpcPacket* next;
  struct _RpcConnection* conn;
  RpcPktHdr* hdr;
  RPC_MESSAGE* msg;
} RpcPacket;

typedef struct _RpcObjTypeMap
{
  /* FIXME: a hash table would be better. */
  struct _RpcObjTypeMap *next;
  UUID Object;
  UUID Type;
} RpcObjTypeMap;

static RpcObjTypeMap *RpcObjTypeMaps;

static RpcServerProtseq* protseqs;
static RpcServerInterface* ifs;

static CRITICAL_SECTION server_cs;
static CRITICAL_SECTION_DEBUG server_cs_debug =
{
    0, 0, &server_cs,
    { &server_cs_debug.ProcessLocksList, &server_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": server_cs") }
};
static CRITICAL_SECTION server_cs = { &server_cs_debug, -1, 0, 0, 0, 0 };

static CRITICAL_SECTION listen_cs;
static CRITICAL_SECTION_DEBUG listen_cs_debug =
{
    0, 0, &listen_cs,
    { &listen_cs_debug.ProcessLocksList, &listen_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": listen_cs") }
};
static CRITICAL_SECTION listen_cs = { &listen_cs_debug, -1, 0, 0, 0, 0 };

/* whether the server is currently listening */
static BOOL std_listen;
/* number of manual listeners (calls to RpcServerListen) */
static LONG manual_listen_count;
/* total listeners including auto listeners */
static LONG listen_count;
/* set on change of configuration (e.g. listening on new protseq) */
static HANDLE mgr_event;
/* mutex for ensuring only one thread can change state at a time */
static HANDLE mgr_mutex;
/* set when server thread has finished opening connections */
static HANDLE server_ready_event;

static CRITICAL_SECTION spacket_cs;
static CRITICAL_SECTION_DEBUG spacket_cs_debug =
{
    0, 0, &spacket_cs,
    { &spacket_cs_debug.ProcessLocksList, &spacket_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": spacket_cs") }
};
static CRITICAL_SECTION spacket_cs = { &spacket_cs_debug, -1, 0, 0, 0, 0 };

static RpcPacket* spacket_head;
static RpcPacket* spacket_tail;
static HANDLE server_sem;

static LONG worker_count, worker_free, worker_tls;

static UUID uuid_nil;

inline static RpcObjTypeMap *LookupObjTypeMap(UUID *ObjUuid)
{
  RpcObjTypeMap *rslt = RpcObjTypeMaps;
  RPC_STATUS dummy;

  while (rslt) {
    if (! UuidCompare(ObjUuid, &rslt->Object, &dummy)) break;
    rslt = rslt->next;
  }

  return rslt;
}

inline static UUID *LookupObjType(UUID *ObjUuid)
{
  RpcObjTypeMap *map = LookupObjTypeMap(ObjUuid);
  if (map)
    return &map->Type;
  else
    return &uuid_nil;
}

static RpcServerInterface* RPCRT4_find_interface(UUID* object,
                                                 RPC_SYNTAX_IDENTIFIER* if_id,
                                                 BOOL check_object)
{
  UUID* MgrType = NULL;
  RpcServerInterface* cif = NULL;
  RPC_STATUS status;

  if (check_object)
    MgrType = LookupObjType(object);
  EnterCriticalSection(&server_cs);
  cif = ifs;
  while (cif) {
    if (!memcmp(if_id, &cif->If->InterfaceId, sizeof(RPC_SYNTAX_IDENTIFIER)) &&
        (check_object == FALSE || UuidEqual(MgrType, &cif->MgrTypeUuid, &status)) &&
        std_listen) break;
    cif = cif->Next;
  }
  LeaveCriticalSection(&server_cs);
  TRACE("returning %p for %s\n", cif, debugstr_guid(object));
  return cif;
}

static void RPCRT4_push_packet(RpcPacket* packet)
{
  packet->next = NULL;
  EnterCriticalSection(&spacket_cs);
  if (spacket_tail) {
    spacket_tail->next = packet;
    spacket_tail = packet;
  } else {
    spacket_head = packet;
    spacket_tail = packet;
  }
  LeaveCriticalSection(&spacket_cs);
}

static RpcPacket* RPCRT4_pop_packet(void)
{
  RpcPacket* packet;
  EnterCriticalSection(&spacket_cs);
  packet = spacket_head;
  if (packet) {
    spacket_head = packet->next;
    if (!spacket_head) spacket_tail = NULL;
  }
  LeaveCriticalSection(&spacket_cs);
  if (packet) packet->next = NULL;
  return packet;
}

#ifndef __REACTOS__
typedef struct {
  PRPC_MESSAGE msg;
  void* buf;
} packet_state;

static WINE_EXCEPTION_FILTER(rpc_filter)
{
  packet_state* state;
  PRPC_MESSAGE msg;
  state = TlsGetValue(worker_tls);
  msg = state->msg;
  if (msg->Buffer != state->buf) I_RpcFreeBuffer(msg);
  msg->RpcFlags |= WINE_RPCFLAG_EXCEPTION;
  msg->BufferLength = sizeof(DWORD);
  I_RpcGetBuffer(msg);
  *(DWORD*)msg->Buffer = GetExceptionCode();
  WARN("exception caught with code 0x%08lx = %ld\n", *(DWORD*)msg->Buffer, *(DWORD*)msg->Buffer);
  TRACE("returning failure packet\n");
  return EXCEPTION_EXECUTE_HANDLER;
}
#endif

static void RPCRT4_process_packet(RpcConnection* conn, RpcPktHdr* hdr, RPC_MESSAGE* msg)
{
  RpcServerInterface* sif;
  RPC_DISPATCH_FUNCTION func;
#ifndef __REACTOS__
  packet_state state;
#endif
  UUID *object_uuid;
  RpcPktHdr *response;
  void *buf = msg->Buffer;
  RPC_STATUS status;

#ifndef __REACTOS__
  state.msg = msg;
  state.buf = buf;
  TlsSetValue(worker_tls, &state);
#endif

  switch (hdr->common.ptype) {
    case PKT_BIND:
      TRACE("got bind packet\n");

      /* FIXME: do more checks! */
      if (hdr->bind.max_tsize < RPC_MIN_PACKET_SIZE ||
          !UuidIsNil(&conn->ActiveInterface.SyntaxGUID, &status)) {
        TRACE("packet size less than min size, or active interface syntax guid non-null\n");
        sif = NULL;
      } else {
        sif = RPCRT4_find_interface(NULL, &hdr->bind.abstract, FALSE);
      }
      if (sif == NULL) {
        TRACE("rejecting bind request on connection %p\n", conn);
        /* Report failure to client. */
        response = RPCRT4_BuildBindNackHeader(NDR_LOCAL_DATA_REPRESENTATION,
                                              RPC_VER_MAJOR, RPC_VER_MINOR);
      } else {
        TRACE("accepting bind request on connection %p\n", conn);

        /* accept. */
        response = RPCRT4_BuildBindAckHeader(NDR_LOCAL_DATA_REPRESENTATION,
                                             RPC_MAX_PACKET_SIZE,
                                             RPC_MAX_PACKET_SIZE,
                                             conn->Endpoint,
                                             RESULT_ACCEPT, NO_REASON,
                                             &sif->If->TransferSyntax);

        /* save the interface for later use */
        conn->ActiveInterface = hdr->bind.abstract;
        conn->MaxTransmissionSize = hdr->bind.max_tsize;
      }

      if (RPCRT4_Send(conn, response, NULL, 0) != RPC_S_OK)
        goto fail;

      break;

    case PKT_REQUEST:
      TRACE("got request packet\n");

      /* fail if the connection isn't bound with an interface */
      if (UuidIsNil(&conn->ActiveInterface.SyntaxGUID, &status)) {
        response = RPCRT4_BuildFaultHeader(NDR_LOCAL_DATA_REPRESENTATION,
                                           status);

        RPCRT4_Send(conn, response, NULL, 0);
        break;
      }

      if (hdr->common.flags & RPC_FLG_OBJECT_UUID) {
        object_uuid = (UUID*)(&hdr->request + 1);
      } else {
        object_uuid = NULL;
      }

      sif = RPCRT4_find_interface(object_uuid, &conn->ActiveInterface, TRUE);
      msg->RpcInterfaceInformation = sif->If;
      /* copy the endpoint vector from sif to msg so that midl-generated code will use it */
      msg->ManagerEpv = sif->MgrEpv;
      if (object_uuid != NULL) {
        RPCRT4_SetBindingObject(msg->Handle, object_uuid);
      }

      /* find dispatch function */
      msg->ProcNum = hdr->request.opnum;
      if (sif->Flags & RPC_IF_OLE) {
        /* native ole32 always gives us a dispatch table with a single entry
         * (I assume that's a wrapper for IRpcStubBuffer::Invoke) */
        func = *sif->If->DispatchTable->DispatchTable;
      } else {
        if (msg->ProcNum >= sif->If->DispatchTable->DispatchTableCount) {
          ERR("invalid procnum\n");
          func = NULL;
        }
        func = sif->If->DispatchTable->DispatchTable[msg->ProcNum];
      }

      /* put in the drep. FIXME: is this more universally applicable?
         perhaps we should move this outward... */
      msg->DataRepresentation =
        MAKELONG( MAKEWORD(hdr->common.drep[0], hdr->common.drep[1]),
                  MAKEWORD(hdr->common.drep[2], hdr->common.drep[3]));

      /* dispatch */
#ifndef __REACTOS__
      __TRY {
        if (func) func(msg);
      } __EXCEPT(rpc_filter) {
        /* failure packet was created in rpc_filter */
      } __ENDTRY
#else
      if (func) func(msg);
#endif

      /* send response packet */
      I_RpcSend(msg);

      msg->RpcInterfaceInformation = NULL;

      break;

    default:
      FIXME("unhandled packet type\n");
      break;
  }

fail:
  /* clean up */
  if (msg->Buffer == buf) msg->Buffer = NULL;
  TRACE("freeing Buffer=%p\n", buf);
  HeapFree(GetProcessHeap(), 0, buf);
  RPCRT4_DestroyBinding(msg->Handle);
  msg->Handle = 0;
  I_RpcFreeBuffer(msg);
  msg->Buffer = NULL;
  RPCRT4_FreeHeader(hdr);
#ifndef __REACTOS__
  TlsSetValue(worker_tls, NULL);
#endif
}

static DWORD CALLBACK RPCRT4_worker_thread(LPVOID the_arg)
{
  DWORD obj;
  RpcPacket* pkt;

  for (;;) {
    /* idle timeout after 5s */
    obj = WaitForSingleObject(server_sem, 5000);
    if (obj == WAIT_TIMEOUT) {
      /* if another idle thread exist, self-destruct */
      if (worker_free > 1) break;
      continue;
    }
    pkt = RPCRT4_pop_packet();
    if (!pkt) continue;
    InterlockedDecrement(&worker_free);
    for (;;) {
      RPCRT4_process_packet(pkt->conn, pkt->hdr, pkt->msg);
      HeapFree(GetProcessHeap(), 0, pkt);
      /* try to grab another packet here without waiting
       * on the semaphore, in case it hits max */
      pkt = RPCRT4_pop_packet();
      if (!pkt) break;
      /* decrement semaphore */
      WaitForSingleObject(server_sem, 0);
    }
    InterlockedIncrement(&worker_free);
  }
  InterlockedDecrement(&worker_free);
  InterlockedDecrement(&worker_count);
  return 0;
}

static void RPCRT4_create_worker_if_needed(void)
{
  if (!worker_free && worker_count < MAX_THREADS) {
    HANDLE thread;
    InterlockedIncrement(&worker_count);
    InterlockedIncrement(&worker_free);
    thread = CreateThread(NULL, 0, RPCRT4_worker_thread, NULL, 0, NULL);
    if (thread) CloseHandle(thread);
    else {
      InterlockedDecrement(&worker_free);
      InterlockedDecrement(&worker_count);
    }
  }
}

static DWORD CALLBACK RPCRT4_io_thread(LPVOID the_arg)
{
  RpcConnection* conn = (RpcConnection*)the_arg;
  RpcPktHdr *hdr;
  RpcBinding *pbind;
  RPC_MESSAGE *msg;
  RPC_STATUS status;
  RpcPacket *packet;

  TRACE("(%p)\n", conn);

  for (;;) {
    msg = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RPC_MESSAGE));

    /* create temporary binding for dispatch, it will be freed in
     * RPCRT4_process_packet */
    RPCRT4_MakeBinding(&pbind, conn);
    msg->Handle = (RPC_BINDING_HANDLE)pbind;

    status = RPCRT4_Receive(conn, &hdr, msg);
    if (status != RPC_S_OK) {
      WARN("receive failed with error %lx\n", status);
      break;
    }

#if 0
    RPCRT4_process_packet(conn, hdr, msg);
#else
    packet = HeapAlloc(GetProcessHeap(), 0, sizeof(RpcPacket));
    packet->conn = conn;
    packet->hdr = hdr;
    packet->msg = msg;
    RPCRT4_create_worker_if_needed();
    RPCRT4_push_packet(packet);
    ReleaseSemaphore(server_sem, 1, NULL);
#endif
    msg = NULL;
  }
  HeapFree(GetProcessHeap(), 0, msg);
  RPCRT4_DestroyConnection(conn);
  return 0;
}

static void RPCRT4_new_client(RpcConnection* conn)
{
  HANDLE thread = CreateThread(NULL, 0, RPCRT4_io_thread, conn, 0, NULL);
  if (!thread) {
    DWORD err = GetLastError();
    ERR("failed to create thread, error=%08lx\n", err);
    RPCRT4_DestroyConnection(conn);
  }
  /* we could set conn->thread, but then we'd have to make the io_thread wait
   * for that, otherwise the thread might finish, destroy the connection, and
   * free the memory we'd write to before we did, causing crashes and stuff -
   * so let's implement that later, when we really need conn->thread */

  CloseHandle( thread );
}

static DWORD CALLBACK RPCRT4_server_thread(LPVOID the_arg)
{
  HANDLE m_event = mgr_event, b_handle;
  HANDLE *objs = NULL;
  DWORD count, res;
  RpcServerProtseq* cps;
  RpcConnection* conn;
  RpcConnection* cconn;
  BOOL set_ready_event = FALSE;

  TRACE("(the_arg == ^%p)\n", the_arg);

  for (;;) {
    EnterCriticalSection(&server_cs);
    /* open and count connections */
    count = 1;
    cps = protseqs;
    while (cps) {
      conn = cps->conn;
      while (conn) {
        RPCRT4_OpenConnection(conn);
        if (conn->ovl[0].hEvent) count++;
        conn = conn->Next;
      }
      cps = cps->Next;
    }
    /* make array of connections */
    if (objs)
	objs = HeapReAlloc(GetProcessHeap(), 0, objs, count*sizeof(HANDLE));
    else
	objs = HeapAlloc(GetProcessHeap(), 0, count*sizeof(HANDLE));

    objs[0] = m_event;
    count = 1;
    cps = protseqs;
    while (cps) {
      conn = cps->conn;
      while (conn) {
        if (conn->ovl[0].hEvent) objs[count++] = conn->ovl[0].hEvent;
        conn = conn->Next;
      }
      cps = cps->Next;
    }
    LeaveCriticalSection(&server_cs);

    if (set_ready_event)
    {
        /* signal to function that changed state that we are now sync'ed */
        SetEvent(server_ready_event);
        set_ready_event = FALSE;
    }

    /* start waiting */
    res = WaitForMultipleObjects(count, objs, FALSE, INFINITE);
    if (res == WAIT_OBJECT_0) {
      if (!std_listen)
      {
        SetEvent(server_ready_event);
        break;
      }
      set_ready_event = TRUE;
    }
    else if (res == WAIT_FAILED) {
      ERR("wait failed\n");
    }
    else {
      b_handle = objs[res - WAIT_OBJECT_0];
      /* find which connection got a RPC */
      EnterCriticalSection(&server_cs);
      conn = NULL;
      cps = protseqs;
      while (cps) {
        conn = cps->conn;
        while (conn) {
          if (conn->ovl[0].hEvent == b_handle) break;
          conn = conn->Next;
        }
        if (conn) break;
        cps = cps->Next;
      }
      cconn = NULL;
      if (conn) RPCRT4_SpawnConnection(&cconn, conn);
      LeaveCriticalSection(&server_cs);
      if (!conn) {
        ERR("failed to locate connection for handle %p\n", b_handle);
      }
      if (cconn) RPCRT4_new_client(cconn);
    }
  }
  HeapFree(GetProcessHeap(), 0, objs);
  EnterCriticalSection(&server_cs);
  /* close connections */
  cps = protseqs;
  while (cps) {
    conn = cps->conn;
    while (conn) {
      RPCRT4_CloseConnection(conn);
      conn = conn->Next;
    }
    cps = cps->Next;
  }
  LeaveCriticalSection(&server_cs);
  return 0;
}

/* tells the server thread that the state has changed and waits for it to
 * make the changes */
static void RPCRT4_sync_with_server_thread(void)
{
  /* make sure we are the only thread sync'ing the server state, otherwise
   * there is a race with the server thread setting an older state and setting
   * the server_ready_event when the new state hasn't yet been applied */
  WaitForSingleObject(mgr_mutex, INFINITE);

  SetEvent(mgr_event);
  /* wait for server thread to make the requested changes before returning */
  WaitForSingleObject(server_ready_event, INFINITE);

  ReleaseMutex(mgr_mutex);
}

static RPC_STATUS RPCRT4_start_listen(BOOL auto_listen)
{
  RPC_STATUS status = RPC_S_ALREADY_LISTENING;

  TRACE("\n");

  EnterCriticalSection(&listen_cs);
  if (auto_listen || (manual_listen_count++ == 0))
  {
    status = RPC_S_OK;
    if (++listen_count == 1) {
      HANDLE server_thread;
      /* first listener creates server thread */
      if (!mgr_mutex) mgr_mutex = CreateMutexW(NULL, FALSE, NULL);
      if (!mgr_event) mgr_event = CreateEventW(NULL, FALSE, FALSE, NULL);
      if (!server_ready_event) server_ready_event = CreateEventW(NULL, FALSE, FALSE, NULL);
      if (!server_sem) server_sem = CreateSemaphoreW(NULL, 0, MAX_THREADS, NULL);
      if (!worker_tls) worker_tls = TlsAlloc();
      std_listen = TRUE;
      server_thread = CreateThread(NULL, 0, RPCRT4_server_thread, NULL, 0, NULL);
      CloseHandle(server_thread);
    }
  }
  LeaveCriticalSection(&listen_cs);

  return status;
}

static void RPCRT4_stop_listen(BOOL auto_listen)
{
  EnterCriticalSection(&listen_cs);
  if (auto_listen || (--manual_listen_count == 0))
  {
    if (listen_count != 0 && --listen_count == 0) {
      std_listen = FALSE;
      LeaveCriticalSection(&listen_cs);
      RPCRT4_sync_with_server_thread();
      return;
    }
    assert(listen_count >= 0);
  }
  LeaveCriticalSection(&listen_cs);
}

static RPC_STATUS RPCRT4_use_protseq(RpcServerProtseq* ps)
{
  RPCRT4_CreateConnection(&ps->conn, TRUE, ps->Protseq, NULL, ps->Endpoint, NULL, NULL);

  EnterCriticalSection(&server_cs);
  ps->Next = protseqs;
  protseqs = ps;
  LeaveCriticalSection(&server_cs);

  if (std_listen) RPCRT4_sync_with_server_thread();

  return RPC_S_OK;
}

/***********************************************************************
 *             RpcServerInqBindings (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerInqBindings( RPC_BINDING_VECTOR** BindingVector )
{
  RPC_STATUS status;
  DWORD count;
  RpcServerProtseq* ps;
  RpcConnection* conn;

  if (BindingVector)
    TRACE("(*BindingVector == ^%p)\n", *BindingVector);
  else
    ERR("(BindingVector == NULL!!?)\n");

  EnterCriticalSection(&server_cs);
  /* count connections */
  count = 0;
  ps = protseqs;
  while (ps) {
    conn = ps->conn;
    while (conn) {
      count++;
      conn = conn->Next;
    }
    ps = ps->Next;
  }
  if (count) {
    /* export bindings */
    *BindingVector = HeapAlloc(GetProcessHeap(), 0,
                              sizeof(RPC_BINDING_VECTOR) +
                              sizeof(RPC_BINDING_HANDLE)*(count-1));
    (*BindingVector)->Count = count;
    count = 0;
    ps = protseqs;
    while (ps) {
      conn = ps->conn;
      while (conn) {
       RPCRT4_MakeBinding((RpcBinding**)&(*BindingVector)->BindingH[count],
                          conn);
       count++;
       conn = conn->Next;
      }
      ps = ps->Next;
    }
    status = RPC_S_OK;
  } else {
    *BindingVector = NULL;
    status = RPC_S_NO_BINDINGS;
  }
  LeaveCriticalSection(&server_cs);
  return status;
}

/***********************************************************************
 *             RpcServerUseProtseqEpA (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerUseProtseqEpA( unsigned char *Protseq, UINT MaxCalls, unsigned char *Endpoint, LPVOID SecurityDescriptor )
{
  RPC_POLICY policy;

  TRACE( "(%s,%u,%s,%p)\n", Protseq, MaxCalls, Endpoint, SecurityDescriptor );

  /* This should provide the default behaviour */
  policy.Length        = sizeof( policy );
  policy.EndpointFlags = 0;
  policy.NICFlags      = 0;

  return RpcServerUseProtseqEpExA( Protseq, MaxCalls, Endpoint, SecurityDescriptor, &policy );
}

/***********************************************************************
 *             RpcServerUseProtseqEpW (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerUseProtseqEpW( LPWSTR Protseq, UINT MaxCalls, LPWSTR Endpoint, LPVOID SecurityDescriptor )
{
  RPC_POLICY policy;

  TRACE( "(%s,%u,%s,%p)\n", debugstr_w( Protseq ), MaxCalls, debugstr_w( Endpoint ), SecurityDescriptor );

  /* This should provide the default behaviour */
  policy.Length        = sizeof( policy );
  policy.EndpointFlags = 0;
  policy.NICFlags      = 0;

  return RpcServerUseProtseqEpExW( Protseq, MaxCalls, Endpoint, SecurityDescriptor, &policy );
}

/***********************************************************************
 *             RpcServerUseProtseqEpExA (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerUseProtseqEpExA( unsigned char *Protseq, UINT MaxCalls, unsigned char *Endpoint, LPVOID SecurityDescriptor,
                                            PRPC_POLICY lpPolicy )
{
  RpcServerProtseq* ps;

  TRACE("(%s,%u,%s,%p,{%u,%lu,%lu})\n", debugstr_a( (char*)Protseq ), MaxCalls,
       debugstr_a( (char*)Endpoint ), SecurityDescriptor,
       lpPolicy->Length, lpPolicy->EndpointFlags, lpPolicy->NICFlags );

  ps = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RpcServerProtseq));
  ps->MaxCalls = MaxCalls;
  ps->Protseq = RPCRT4_strdupA((char*)Protseq);
  ps->Endpoint = RPCRT4_strdupA((char*)Endpoint);

  return RPCRT4_use_protseq(ps);
}

/***********************************************************************
 *             RpcServerUseProtseqEpExW (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerUseProtseqEpExW( LPWSTR Protseq, UINT MaxCalls, LPWSTR Endpoint, LPVOID SecurityDescriptor,
                                            PRPC_POLICY lpPolicy )
{
  RpcServerProtseq* ps;

  TRACE("(%s,%u,%s,%p,{%u,%lu,%lu})\n", debugstr_w( Protseq ), MaxCalls,
       debugstr_w( Endpoint ), SecurityDescriptor,
       lpPolicy->Length, lpPolicy->EndpointFlags, lpPolicy->NICFlags );

  ps = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RpcServerProtseq));
  ps->MaxCalls = MaxCalls;
  ps->Protseq = RPCRT4_strdupWtoA(Protseq);
  ps->Endpoint = RPCRT4_strdupWtoA(Endpoint);

  return RPCRT4_use_protseq(ps);
}

/***********************************************************************
 *             RpcServerUseProtseqA (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerUseProtseqA(unsigned char *Protseq, unsigned int MaxCalls, void *SecurityDescriptor)
{
  TRACE("(Protseq == %s, MaxCalls == %d, SecurityDescriptor == ^%p)\n", debugstr_a((char*)Protseq), MaxCalls, SecurityDescriptor);
  return RpcServerUseProtseqEpA(Protseq, MaxCalls, NULL, SecurityDescriptor);
}

/***********************************************************************
 *             RpcServerUseProtseqW (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerUseProtseqW(LPWSTR Protseq, unsigned int MaxCalls, void *SecurityDescriptor)
{
  TRACE("Protseq == %s, MaxCalls == %d, SecurityDescriptor == ^%p)\n", debugstr_w(Protseq), MaxCalls, SecurityDescriptor);
  return RpcServerUseProtseqEpW(Protseq, MaxCalls, NULL, SecurityDescriptor);
}

/***********************************************************************
 *             RpcServerRegisterIf (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerRegisterIf( RPC_IF_HANDLE IfSpec, UUID* MgrTypeUuid, RPC_MGR_EPV* MgrEpv )
{
  TRACE("(%p,%s,%p)\n", IfSpec, debugstr_guid(MgrTypeUuid), MgrEpv);
  return RpcServerRegisterIf2( IfSpec, MgrTypeUuid, MgrEpv, 0, RPC_C_LISTEN_MAX_CALLS_DEFAULT, (UINT)-1, NULL );
}

/***********************************************************************
 *             RpcServerRegisterIfEx (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerRegisterIfEx( RPC_IF_HANDLE IfSpec, UUID* MgrTypeUuid, RPC_MGR_EPV* MgrEpv,
                       UINT Flags, UINT MaxCalls, RPC_IF_CALLBACK_FN* IfCallbackFn )
{
  TRACE("(%p,%s,%p,%u,%u,%p)\n", IfSpec, debugstr_guid(MgrTypeUuid), MgrEpv, Flags, MaxCalls, IfCallbackFn);
  return RpcServerRegisterIf2( IfSpec, MgrTypeUuid, MgrEpv, Flags, MaxCalls, (UINT)-1, IfCallbackFn );
}

/***********************************************************************
 *             RpcServerRegisterIf2 (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerRegisterIf2( RPC_IF_HANDLE IfSpec, UUID* MgrTypeUuid, RPC_MGR_EPV* MgrEpv,
                      UINT Flags, UINT MaxCalls, UINT MaxRpcSize, RPC_IF_CALLBACK_FN* IfCallbackFn )
{
  PRPC_SERVER_INTERFACE If = (PRPC_SERVER_INTERFACE)IfSpec;
  RpcServerInterface* sif;
  unsigned int i;

  TRACE("(%p,%s,%p,%u,%u,%u,%p)\n", IfSpec, debugstr_guid(MgrTypeUuid), MgrEpv, Flags, MaxCalls,
         MaxRpcSize, IfCallbackFn);
  TRACE(" interface id: %s %d.%d\n", debugstr_guid(&If->InterfaceId.SyntaxGUID),
                                     If->InterfaceId.SyntaxVersion.MajorVersion,
                                     If->InterfaceId.SyntaxVersion.MinorVersion);
  TRACE(" transfer syntax: %s %d.%d\n", debugstr_guid(&If->TransferSyntax.SyntaxGUID),
                                        If->TransferSyntax.SyntaxVersion.MajorVersion,
                                        If->TransferSyntax.SyntaxVersion.MinorVersion);
  TRACE(" dispatch table: %p\n", If->DispatchTable);
  if (If->DispatchTable) {
    TRACE("  dispatch table count: %d\n", If->DispatchTable->DispatchTableCount);
    for (i=0; i<If->DispatchTable->DispatchTableCount; i++) {
      TRACE("   entry %d: %p\n", i, If->DispatchTable->DispatchTable[i]);
    }
    TRACE("  reserved: %ld\n", If->DispatchTable->Reserved);
  }
  TRACE(" protseq endpoint count: %d\n", If->RpcProtseqEndpointCount);
  TRACE(" default manager epv: %p\n", If->DefaultManagerEpv);
  TRACE(" interpreter info: %p\n", If->InterpreterInfo);

  sif = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RpcServerInterface));
  sif->If           = If;
  if (MgrTypeUuid) {
    memcpy(&sif->MgrTypeUuid, MgrTypeUuid, sizeof(UUID));
    sif->MgrEpv       = MgrEpv;
  } else {
    memset(&sif->MgrTypeUuid, 0, sizeof(UUID));
    sif->MgrEpv       = If->DefaultManagerEpv;
  }
  sif->Flags        = Flags;
  sif->MaxCalls     = MaxCalls;
  sif->MaxRpcSize   = MaxRpcSize;
  sif->IfCallbackFn = IfCallbackFn;

  EnterCriticalSection(&server_cs);
  sif->Next = ifs;
  ifs = sif;
  LeaveCriticalSection(&server_cs);

  if (sif->Flags & RPC_IF_AUTOLISTEN) {
    RPCRT4_start_listen(TRUE);

    /* make sure server is actually listening on the interface before
     * returning */
    RPCRT4_sync_with_server_thread();
  }

  return RPC_S_OK;
}

/***********************************************************************
 *             RpcServerUnregisterIf (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerUnregisterIf( RPC_IF_HANDLE IfSpec, UUID* MgrTypeUuid, UINT WaitForCallsToComplete )
{
  FIXME("(IfSpec == (RPC_IF_HANDLE)^%p, MgrTypeUuid == %s, WaitForCallsToComplete == %u): stub\n",
    IfSpec, debugstr_guid(MgrTypeUuid), WaitForCallsToComplete);

  return RPC_S_OK;
}

/***********************************************************************
 *             RpcServerUnregisterIfEx (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerUnregisterIfEx( RPC_IF_HANDLE IfSpec, UUID* MgrTypeUuid, int RundownContextHandles )
{
  FIXME("(IfSpec == (RPC_IF_HANDLE)^%p, MgrTypeUuid == %s, RundownContextHandles == %d): stub\n",
    IfSpec, debugstr_guid(MgrTypeUuid), RundownContextHandles);

  return RPC_S_OK;
}

/***********************************************************************
 *             RpcObjectSetType (RPCRT4.@)
 *
 * PARAMS
 *   ObjUuid  [I] "Object" UUID
 *   TypeUuid [I] "Type" UUID
 *
 * RETURNS
 *   RPC_S_OK                 The call succeeded
 *   RPC_S_INVALID_OBJECT     The provided object (nil) is not valid
 *   RPC_S_ALREADY_REGISTERED The provided object is already registered
 *
 * Maps "Object" UUIDs to "Type" UUID's.  Passing the nil UUID as the type
 * resets the mapping for the specified object UUID to nil (the default).
 * The nil object is always associated with the nil type and cannot be
 * reassigned.  Servers can support multiple implementations on the same
 * interface by registering different end-point vectors for the different
 * types.  There's no need to call this if a server only supports the nil
 * type, as is typical.
 */
RPC_STATUS WINAPI RpcObjectSetType( UUID* ObjUuid, UUID* TypeUuid )
{
  RpcObjTypeMap *map = RpcObjTypeMaps, *prev = NULL;
  RPC_STATUS dummy;

  TRACE("(ObjUUID == %s, TypeUuid == %s).\n", debugstr_guid(ObjUuid), debugstr_guid(TypeUuid));
  if ((! ObjUuid) || UuidIsNil(ObjUuid, &dummy)) {
    /* nil uuid cannot be remapped */
    return RPC_S_INVALID_OBJECT;
  }

  /* find the mapping for this object if there is one ... */
  while (map) {
    if (! UuidCompare(ObjUuid, &map->Object, &dummy)) break;
    prev = map;
    map = map->next;
  }
  if ((! TypeUuid) || UuidIsNil(TypeUuid, &dummy)) {
    /* ... and drop it from the list */
    if (map) {
      if (prev)
        prev->next = map->next;
      else
        RpcObjTypeMaps = map->next;
      HeapFree(GetProcessHeap(), 0, map);
    }
  } else {
    /* ... , fail if we found it ... */
    if (map)
      return RPC_S_ALREADY_REGISTERED;
    /* ... otherwise create a new one and add it in. */
    map = HeapAlloc(GetProcessHeap(), 0, sizeof(RpcObjTypeMap));
    memcpy(&map->Object, ObjUuid, sizeof(UUID));
    memcpy(&map->Type, TypeUuid, sizeof(UUID));
    map->next = NULL;
    if (prev)
      prev->next = map; /* prev is the last map in the linklist */
    else
      RpcObjTypeMaps = map;
  }

  return RPC_S_OK;
}

/***********************************************************************
 *             RpcServerRegisterAuthInfoA (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerRegisterAuthInfoA( unsigned char *ServerPrincName, unsigned long AuthnSvc, RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFn,
                            LPVOID Arg )
{
  FIXME( "(%s,%lu,%p,%p): stub\n", ServerPrincName, AuthnSvc, GetKeyFn, Arg );

  return RPC_S_UNKNOWN_AUTHN_SERVICE; /* We don't know any authentication services */
}

/***********************************************************************
 *             RpcServerRegisterAuthInfoW (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerRegisterAuthInfoW( LPWSTR ServerPrincName, unsigned long AuthnSvc, RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFn,
                            LPVOID Arg )
{
  FIXME( "(%s,%lu,%p,%p): stub\n", debugstr_w( ServerPrincName ), AuthnSvc, GetKeyFn, Arg );

  return RPC_S_UNKNOWN_AUTHN_SERVICE; /* We don't know any authentication services */
}

/***********************************************************************
 *             RpcServerListen (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerListen( UINT MinimumCallThreads, UINT MaxCalls, UINT DontWait )
{
  RPC_STATUS status;

  TRACE("(%u,%u,%u)\n", MinimumCallThreads, MaxCalls, DontWait);

  if (!protseqs)
    return RPC_S_NO_PROTSEQS_REGISTERED;

  status = RPCRT4_start_listen(FALSE);

  if (status == RPC_S_OK)
    RPCRT4_sync_with_server_thread();

  if (DontWait || (status != RPC_S_OK)) return status;

  return RpcMgmtWaitServerListen();
}

/***********************************************************************
 *             RpcMgmtServerWaitListen (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcMgmtWaitServerListen( void )
{
  TRACE("()\n");

  EnterCriticalSection(&listen_cs);

  if (!std_listen) {
    LeaveCriticalSection(&listen_cs);
    return RPC_S_NOT_LISTENING;
  }

  LeaveCriticalSection(&listen_cs);

  return RPC_S_OK;
}

/***********************************************************************
 *             RpcMgmtStopServerListening (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcMgmtStopServerListening ( RPC_BINDING_HANDLE Binding )
{
  TRACE("(Binding == (RPC_BINDING_HANDLE)^%p)\n", Binding);

  if (Binding) {
    FIXME("client-side invocation not implemented.\n");
    return RPC_S_WRONG_KIND_OF_BINDING;
  }

  RPCRT4_stop_listen(FALSE);

  return RPC_S_OK;
}

/***********************************************************************
 *             I_RpcServerStartListening (RPCRT4.@)
 */
RPC_STATUS WINAPI I_RpcServerStartListening( HWND hWnd )
{
  FIXME( "(%p): stub\n", hWnd );

  return RPC_S_OK;
}

/***********************************************************************
 *             I_RpcServerStopListening (RPCRT4.@)
 */
RPC_STATUS WINAPI I_RpcServerStopListening( void )
{
  FIXME( "(): stub\n" );

  return RPC_S_OK;
}

/***********************************************************************
 *             I_RpcWindowProc (RPCRT4.@)
 */
UINT WINAPI I_RpcWindowProc( void *hWnd, UINT Message, UINT wParam, ULONG lParam )
{
  FIXME( "(%p,%08x,%08x,%08lx): stub\n", hWnd, Message, wParam, lParam );

  return 0;
}
