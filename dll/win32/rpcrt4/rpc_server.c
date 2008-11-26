/*
 * RPC server API
 *
 * Copyright 2001 Ove Kåven, TransGaming Technologies
 * Copyright 2004 Filip Navara
 * Copyright 2006-2008 Robert Shearman (for CodeWeavers)
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"

#include "rpc.h"
#include "rpcndr.h"
#include "excpt.h"

#include "wine/debug.h"
#include "wine/exception.h"

#include "rpc_server.h"
#include "rpc_assoc.h"
#include "rpc_message.h"
#include "rpc_defs.h"
#include "ncastatus.h"

WINE_DEFAULT_DEBUG_CHANNEL(rpc);

typedef struct _RpcPacket
{
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

/* list of type RpcServerProtseq */
static struct list protseqs = LIST_INIT(protseqs);
static struct list server_interfaces = LIST_INIT(server_interfaces);

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

static UUID uuid_nil;

static inline RpcObjTypeMap *LookupObjTypeMap(UUID *ObjUuid)
{
  RpcObjTypeMap *rslt = RpcObjTypeMaps;
  RPC_STATUS dummy;

  while (rslt) {
    if (! UuidCompare(ObjUuid, &rslt->Object, &dummy)) break;
    rslt = rslt->next;
  }

  return rslt;
}

static inline UUID *LookupObjType(UUID *ObjUuid)
{
  RpcObjTypeMap *map = LookupObjTypeMap(ObjUuid);
  if (map)
    return &map->Type;
  else
    return &uuid_nil;
}

static RpcServerInterface* RPCRT4_find_interface(UUID* object,
                                                 const RPC_SYNTAX_IDENTIFIER* if_id,
                                                 BOOL check_object)
{
  UUID* MgrType = NULL;
  RpcServerInterface* cif;
  RPC_STATUS status;

  if (check_object)
    MgrType = LookupObjType(object);
  EnterCriticalSection(&server_cs);
  LIST_FOR_EACH_ENTRY(cif, &server_interfaces, RpcServerInterface, entry) {
    if (!memcmp(if_id, &cif->If->InterfaceId, sizeof(RPC_SYNTAX_IDENTIFIER)) &&
        (check_object == FALSE || UuidEqual(MgrType, &cif->MgrTypeUuid, &status)) &&
        std_listen) {
      InterlockedIncrement(&cif->CurrentCalls);
      break;
    }
  }
  LeaveCriticalSection(&server_cs);
  if (&cif->entry == &server_interfaces) cif = NULL;
  TRACE("returning %p for object %s, if_id { %d.%d %s }\n", cif,
    debugstr_guid(object), if_id->SyntaxVersion.MajorVersion,
    if_id->SyntaxVersion.MinorVersion, debugstr_guid(&if_id->SyntaxGUID));
  return cif;
}

static void RPCRT4_release_server_interface(RpcServerInterface *sif)
{
  if (!InterlockedDecrement(&sif->CurrentCalls) &&
      sif->Delete) {
    /* sif must have been removed from server_interfaces before
     * CallsCompletedEvent is set */
    if (sif->CallsCompletedEvent)
      SetEvent(sif->CallsCompletedEvent);
    HeapFree(GetProcessHeap(), 0, sif);
  }
}

static RPC_STATUS process_bind_packet(RpcConnection *conn, RpcPktBindHdr *hdr, RPC_MESSAGE *msg)
{
  RPC_STATUS status;
  RpcServerInterface* sif;
  RpcPktHdr *response = NULL;

  /* FIXME: do more checks! */
  if (hdr->max_tsize < RPC_MIN_PACKET_SIZE ||
      !UuidIsNil(&conn->ActiveInterface.SyntaxGUID, &status) ||
      conn->server_binding) {
    TRACE("packet size less than min size, or active interface syntax guid non-null\n");
    sif = NULL;
  } else {
    /* create temporary binding */
    if (RPCRT4_MakeBinding(&conn->server_binding, conn) == RPC_S_OK &&
        RpcServerAssoc_GetAssociation(rpcrt4_conn_get_name(conn),
                                      conn->NetworkAddr, conn->Endpoint,
                                      conn->NetworkOptions,
                                      hdr->assoc_gid,
                                      &conn->server_binding->Assoc) == RPC_S_OK)
      sif = RPCRT4_find_interface(NULL, &hdr->abstract, FALSE);
    else
      sif = NULL;
  }
  if (sif == NULL) {
    TRACE("rejecting bind request on connection %p\n", conn);
    /* Report failure to client. */
    response = RPCRT4_BuildBindNackHeader(NDR_LOCAL_DATA_REPRESENTATION,
                                          RPC_VER_MAJOR, RPC_VER_MINOR);
  } else {
    TRACE("accepting bind request on connection %p for %s\n", conn,
          debugstr_guid(&hdr->abstract.SyntaxGUID));

    /* accept. */
    response = RPCRT4_BuildBindAckHeader(NDR_LOCAL_DATA_REPRESENTATION,
                                         RPC_MAX_PACKET_SIZE,
                                         RPC_MAX_PACKET_SIZE,
                                         conn->server_binding->Assoc->assoc_group_id,
                                         conn->Endpoint,
                                         RESULT_ACCEPT, REASON_NONE,
                                         &sif->If->TransferSyntax);

    /* save the interface for later use */
    conn->ActiveInterface = hdr->abstract;
    conn->MaxTransmissionSize = hdr->max_tsize;

    RPCRT4_release_server_interface(sif);
  }

  if (response)
    status = RPCRT4_Send(conn, response, NULL, 0);
  else
    status = ERROR_OUTOFMEMORY;
  RPCRT4_FreeHeader(response);

  return status;
}

static RPC_STATUS process_request_packet(RpcConnection *conn, RpcPktRequestHdr *hdr, RPC_MESSAGE *msg)
{
  RPC_STATUS status;
  RpcPktHdr *response = NULL;
  RpcServerInterface* sif;
  RPC_DISPATCH_FUNCTION func;
  BOOL exception;
  UUID *object_uuid;
  NDR_SCONTEXT context_handle;
  void *buf = msg->Buffer;

  /* fail if the connection isn't bound with an interface */
  if (UuidIsNil(&conn->ActiveInterface.SyntaxGUID, &status)) {
    /* FIXME: should send BindNack instead */
    response = RPCRT4_BuildFaultHeader(NDR_LOCAL_DATA_REPRESENTATION,
                                       status);

    RPCRT4_Send(conn, response, NULL, 0);
    RPCRT4_FreeHeader(response);
    return RPC_S_OK;
  }

  if (hdr->common.flags & RPC_FLG_OBJECT_UUID) {
    object_uuid = (UUID*)(hdr + 1);
  } else {
    object_uuid = NULL;
  }

  sif = RPCRT4_find_interface(object_uuid, &conn->ActiveInterface, TRUE);
  if (!sif) {
    WARN("interface %s no longer registered, returning fault packet\n", debugstr_guid(&conn->ActiveInterface.SyntaxGUID));
    response = RPCRT4_BuildFaultHeader(NDR_LOCAL_DATA_REPRESENTATION,
                                       NCA_S_UNK_IF);

    RPCRT4_Send(conn, response, NULL, 0);
    RPCRT4_FreeHeader(response);
    return RPC_S_OK;
  }
  msg->RpcInterfaceInformation = sif->If;
  /* copy the endpoint vector from sif to msg so that midl-generated code will use it */
  msg->ManagerEpv = sif->MgrEpv;
  if (object_uuid != NULL) {
    RPCRT4_SetBindingObject(msg->Handle, object_uuid);
  }

  /* find dispatch function */
  msg->ProcNum = hdr->opnum;
  if (sif->Flags & RPC_IF_OLE) {
    /* native ole32 always gives us a dispatch table with a single entry
    * (I assume that's a wrapper for IRpcStubBuffer::Invoke) */
    func = *sif->If->DispatchTable->DispatchTable;
  } else {
    if (msg->ProcNum >= sif->If->DispatchTable->DispatchTableCount) {
      WARN("invalid procnum (%d/%d)\n", msg->ProcNum, sif->If->DispatchTable->DispatchTableCount);
      response = RPCRT4_BuildFaultHeader(NDR_LOCAL_DATA_REPRESENTATION,
                                         NCA_S_OP_RNG_ERROR);

      RPCRT4_Send(conn, response, NULL, 0);
      RPCRT4_FreeHeader(response);
    }
    func = sif->If->DispatchTable->DispatchTable[msg->ProcNum];
  }

  /* put in the drep. FIXME: is this more universally applicable?
    perhaps we should move this outward... */
  msg->DataRepresentation =
    MAKELONG( MAKEWORD(hdr->common.drep[0], hdr->common.drep[1]),
              MAKEWORD(hdr->common.drep[2], hdr->common.drep[3]));

  exception = FALSE;

  /* dispatch */
  RPCRT4_SetThreadCurrentCallHandle(msg->Handle);
  __TRY {
    if (func) func(msg);
  } __EXCEPT_ALL {
    WARN("exception caught with code 0x%08x = %d\n", GetExceptionCode(), GetExceptionCode());
    exception = TRUE;
    if (GetExceptionCode() == STATUS_ACCESS_VIOLATION)
      status = ERROR_NOACCESS;
    else
      status = GetExceptionCode();
    response = RPCRT4_BuildFaultHeader(msg->DataRepresentation,
                                       RPC2NCA_STATUS(status));
  } __ENDTRY
    RPCRT4_SetThreadCurrentCallHandle(NULL);

  /* release any unmarshalled context handles */
  while ((context_handle = RPCRT4_PopThreadContextHandle()) != NULL)
    RpcServerAssoc_ReleaseContextHandle(conn->server_binding->Assoc, context_handle, TRUE);

  if (!exception)
    response = RPCRT4_BuildResponseHeader(msg->DataRepresentation,
                                          msg->BufferLength);

  /* send response packet */
  if (response) {
    status = RPCRT4_Send(conn, response, exception ? NULL : msg->Buffer,
                         exception ? 0 : msg->BufferLength);
    RPCRT4_FreeHeader(response);
  } else
    ERR("out of memory\n");

  msg->RpcInterfaceInformation = NULL;
  RPCRT4_release_server_interface(sif);

  if (msg->Buffer == buf) buf = NULL;
  TRACE("freeing Buffer=%p\n", buf);
  I_RpcFree(buf);

  return status;
}

static void RPCRT4_process_packet(RpcConnection* conn, RpcPktHdr* hdr, RPC_MESSAGE* msg)
{
  RPC_STATUS status;

  msg->Handle = (RPC_BINDING_HANDLE)conn->server_binding;

  switch (hdr->common.ptype) {
    case PKT_BIND:
      TRACE("got bind packet\n");

      status = process_bind_packet(conn, &hdr->bind, msg);
      break;

    case PKT_REQUEST:
      TRACE("got request packet\n");

      status = process_request_packet(conn, &hdr->request, msg);
      break;

    default:
      FIXME("unhandled packet type %u\n", hdr->common.ptype);
      break;
  }

  /* clean up */
  I_RpcFree(msg->Buffer);
  RPCRT4_FreeHeader(hdr);
  HeapFree(GetProcessHeap(), 0, msg);
}

static DWORD CALLBACK RPCRT4_worker_thread(LPVOID the_arg)
{
  RpcPacket *pkt = the_arg;
  RPCRT4_process_packet(pkt->conn, pkt->hdr, pkt->msg);
  HeapFree(GetProcessHeap(), 0, pkt);
  return 0;
}

static DWORD CALLBACK RPCRT4_io_thread(LPVOID the_arg)
{
  RpcConnection* conn = (RpcConnection*)the_arg;
  RpcPktHdr *hdr;
  RPC_MESSAGE *msg;
  RPC_STATUS status;
  RpcPacket *packet;

  TRACE("(%p)\n", conn);

  for (;;) {
    msg = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RPC_MESSAGE));

    status = RPCRT4_Receive(conn, &hdr, msg);
    if (status != RPC_S_OK) {
      WARN("receive failed with error %lx\n", status);
      HeapFree(GetProcessHeap(), 0, msg);
      break;
    }

    packet = HeapAlloc(GetProcessHeap(), 0, sizeof(RpcPacket));
    if (!packet) {
      I_RpcFree(msg->Buffer);
      RPCRT4_FreeHeader(hdr);
      HeapFree(GetProcessHeap(), 0, msg);
      break;
    }
    packet->conn = conn;
    packet->hdr = hdr;
    packet->msg = msg;
    if (!QueueUserWorkItem(RPCRT4_worker_thread, packet, WT_EXECUTELONGFUNCTION)) {
      ERR("couldn't queue work item for worker thread, error was %d\n", GetLastError());
      I_RpcFree(msg->Buffer);
      RPCRT4_FreeHeader(hdr);
      HeapFree(GetProcessHeap(), 0, msg);
      HeapFree(GetProcessHeap(), 0, packet);
      break;
    }

    msg = NULL;
  }
  RPCRT4_DestroyConnection(conn);
  return 0;
}

void RPCRT4_new_client(RpcConnection* conn)
{
  HANDLE thread = CreateThread(NULL, 0, RPCRT4_io_thread, conn, 0, NULL);
  if (!thread) {
    DWORD err = GetLastError();
    ERR("failed to create thread, error=%08x\n", err);
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
  int res;
  unsigned int count;
  void *objs = NULL;
  RpcServerProtseq* cps = the_arg;
  RpcConnection* conn;
  BOOL set_ready_event = FALSE;

  TRACE("(the_arg == ^%p)\n", the_arg);

  for (;;) {
    objs = cps->ops->get_wait_array(cps, objs, &count);

    if (set_ready_event)
    {
        /* signal to function that changed state that we are now sync'ed */
        SetEvent(cps->server_ready_event);
        set_ready_event = FALSE;
    }

    /* start waiting */
    res = cps->ops->wait_for_new_connection(cps, count, objs);
    if (res == -1)
      break;
    else if (res == 0)
    {
      if (!std_listen)
      {
        SetEvent(cps->server_ready_event);
        break;
      }
      set_ready_event = TRUE;
    }
  }
  cps->ops->free_wait_array(cps, objs);
  EnterCriticalSection(&cps->cs);
  /* close connections */
  conn = cps->conn;
  while (conn) {
    RPCRT4_CloseConnection(conn);
    conn = conn->Next;
  }
  LeaveCriticalSection(&cps->cs);
  return 0;
}

/* tells the server thread that the state has changed and waits for it to
 * make the changes */
static void RPCRT4_sync_with_server_thread(RpcServerProtseq *ps)
{
  /* make sure we are the only thread sync'ing the server state, otherwise
   * there is a race with the server thread setting an older state and setting
   * the server_ready_event when the new state hasn't yet been applied */
  WaitForSingleObject(ps->mgr_mutex, INFINITE);

  ps->ops->signal_state_changed(ps);

  /* wait for server thread to make the requested changes before returning */
  WaitForSingleObject(ps->server_ready_event, INFINITE);

  ReleaseMutex(ps->mgr_mutex);
}

static RPC_STATUS RPCRT4_start_listen_protseq(RpcServerProtseq *ps, BOOL auto_listen)
{
  RPC_STATUS status = RPC_S_OK;
  HANDLE server_thread;

  EnterCriticalSection(&listen_cs);
  if (ps->is_listening) goto done;

  if (!ps->mgr_mutex) ps->mgr_mutex = CreateMutexW(NULL, FALSE, NULL);
  if (!ps->server_ready_event) ps->server_ready_event = CreateEventW(NULL, FALSE, FALSE, NULL);
  server_thread = CreateThread(NULL, 0, RPCRT4_server_thread, ps, 0, NULL);
  if (!server_thread)
  {
    status = RPC_S_OUT_OF_RESOURCES;
    goto done;
  }
  ps->is_listening = TRUE;
  CloseHandle(server_thread);

done:
  LeaveCriticalSection(&listen_cs);
  return status;
}

static RPC_STATUS RPCRT4_start_listen(BOOL auto_listen)
{
  RPC_STATUS status = RPC_S_ALREADY_LISTENING;
  RpcServerProtseq *cps;

  TRACE("\n");

  EnterCriticalSection(&listen_cs);
  if (auto_listen || (manual_listen_count++ == 0))
  {
    status = RPC_S_OK;
    if (++listen_count == 1)
      std_listen = TRUE;
  }
  LeaveCriticalSection(&listen_cs);

  if (std_listen)
  {
    EnterCriticalSection(&server_cs);
    LIST_FOR_EACH_ENTRY(cps, &protseqs, RpcServerProtseq, entry)
    {
      status = RPCRT4_start_listen_protseq(cps, TRUE);
      if (status != RPC_S_OK)
        break;
      
      /* make sure server is actually listening on the interface before
       * returning */
      RPCRT4_sync_with_server_thread(cps);
    }
    LeaveCriticalSection(&server_cs);
  }

  return status;
}

static void RPCRT4_stop_listen(BOOL auto_listen)
{
  EnterCriticalSection(&listen_cs);
  if (auto_listen || (--manual_listen_count == 0))
  {
    if (listen_count != 0 && --listen_count == 0) {
      RpcServerProtseq *cps;

      std_listen = FALSE;
      LeaveCriticalSection(&listen_cs);

      LIST_FOR_EACH_ENTRY(cps, &protseqs, RpcServerProtseq, entry)
        RPCRT4_sync_with_server_thread(cps);

      return;
    }
    assert(listen_count >= 0);
  }
  LeaveCriticalSection(&listen_cs);
}

static RPC_STATUS RPCRT4_use_protseq(RpcServerProtseq* ps, LPSTR endpoint)
{
  RPC_STATUS status;

  status = ps->ops->open_endpoint(ps, endpoint);
  if (status != RPC_S_OK)
    return status;

  if (std_listen)
  {
    status = RPCRT4_start_listen_protseq(ps, FALSE);
    if (status == RPC_S_OK)
      RPCRT4_sync_with_server_thread(ps);
  }

  return status;
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
  LIST_FOR_EACH_ENTRY(ps, &protseqs, RpcServerProtseq, entry) {
    EnterCriticalSection(&ps->cs);
    conn = ps->conn;
    while (conn) {
      count++;
      conn = conn->Next;
    }
    LeaveCriticalSection(&ps->cs);
  }
  if (count) {
    /* export bindings */
    *BindingVector = HeapAlloc(GetProcessHeap(), 0,
                              sizeof(RPC_BINDING_VECTOR) +
                              sizeof(RPC_BINDING_HANDLE)*(count-1));
    (*BindingVector)->Count = count;
    count = 0;
    LIST_FOR_EACH_ENTRY(ps, &protseqs, RpcServerProtseq, entry) {
      EnterCriticalSection(&ps->cs);
      conn = ps->conn;
      while (conn) {
       RPCRT4_MakeBinding((RpcBinding**)&(*BindingVector)->BindingH[count],
                          conn);
       count++;
       conn = conn->Next;
      }
      LeaveCriticalSection(&ps->cs);
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
RPC_STATUS WINAPI RpcServerUseProtseqEpA( RPC_CSTR Protseq, UINT MaxCalls, RPC_CSTR Endpoint, LPVOID SecurityDescriptor )
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
RPC_STATUS WINAPI RpcServerUseProtseqEpW( RPC_WSTR Protseq, UINT MaxCalls, RPC_WSTR Endpoint, LPVOID SecurityDescriptor )
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
 *             alloc_serverprotoseq (internal)
 *
 * Must be called with server_cs held.
 */
static RPC_STATUS alloc_serverprotoseq(UINT MaxCalls, char *Protseq, RpcServerProtseq **ps)
{
  const struct protseq_ops *ops = rpcrt4_get_protseq_ops(Protseq);

  if (!ops)
  {
    FIXME("protseq %s not supported\n", debugstr_a(Protseq));
    return RPC_S_PROTSEQ_NOT_SUPPORTED;
  }

  *ps = ops->alloc();
  if (!*ps)
    return RPC_S_OUT_OF_RESOURCES;
  (*ps)->MaxCalls = MaxCalls;
  (*ps)->Protseq = Protseq;
  (*ps)->ops = ops;
  (*ps)->MaxCalls = 0;
  (*ps)->conn = NULL;
  InitializeCriticalSection(&(*ps)->cs);
  (*ps)->is_listening = FALSE;
  (*ps)->mgr_mutex = NULL;
  (*ps)->server_ready_event = NULL;

  list_add_head(&protseqs, &(*ps)->entry);

  TRACE("new protseq %p created for %s\n", *ps, Protseq);

  return RPC_S_OK;
}

/* Finds a given protseq or creates a new one if one doesn't already exist */
static RPC_STATUS RPCRT4_get_or_create_serverprotseq(UINT MaxCalls, char *Protseq, RpcServerProtseq **ps)
{
    RPC_STATUS status;
    RpcServerProtseq *cps;

    EnterCriticalSection(&server_cs);

    LIST_FOR_EACH_ENTRY(cps, &protseqs, RpcServerProtseq, entry)
        if (!strcmp(cps->Protseq, Protseq))
        {
            TRACE("found existing protseq object for %s\n", Protseq);
            *ps = cps;
            LeaveCriticalSection(&server_cs);
            return S_OK;
        }

    status = alloc_serverprotoseq(MaxCalls, Protseq, ps);

    LeaveCriticalSection(&server_cs);

    return status;
}

/***********************************************************************
 *             RpcServerUseProtseqEpExA (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerUseProtseqEpExA( RPC_CSTR Protseq, UINT MaxCalls, RPC_CSTR Endpoint, LPVOID SecurityDescriptor,
                                            PRPC_POLICY lpPolicy )
{
  char *szps = (char*)Protseq, *szep = (char*)Endpoint;
  RpcServerProtseq* ps;
  RPC_STATUS status;

  TRACE("(%s,%u,%s,%p,{%u,%lu,%lu})\n", debugstr_a(szps), MaxCalls,
       debugstr_a(szep), SecurityDescriptor,
       lpPolicy->Length, lpPolicy->EndpointFlags, lpPolicy->NICFlags );

  status = RPCRT4_get_or_create_serverprotseq(MaxCalls, RPCRT4_strdupA(szps), &ps);
  if (status != RPC_S_OK)
    return status;

  return RPCRT4_use_protseq(ps, szep);
}

/***********************************************************************
 *             RpcServerUseProtseqEpExW (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerUseProtseqEpExW( RPC_WSTR Protseq, UINT MaxCalls, RPC_WSTR Endpoint, LPVOID SecurityDescriptor,
                                            PRPC_POLICY lpPolicy )
{
  RpcServerProtseq* ps;
  RPC_STATUS status;
  LPSTR EndpointA;

  TRACE("(%s,%u,%s,%p,{%u,%lu,%lu})\n", debugstr_w( Protseq ), MaxCalls,
       debugstr_w( Endpoint ), SecurityDescriptor,
       lpPolicy->Length, lpPolicy->EndpointFlags, lpPolicy->NICFlags );

  status = RPCRT4_get_or_create_serverprotseq(MaxCalls, RPCRT4_strdupWtoA(Protseq), &ps);
  if (status != RPC_S_OK)
    return status;

  EndpointA = RPCRT4_strdupWtoA(Endpoint);
  status = RPCRT4_use_protseq(ps, EndpointA);
  RPCRT4_strfree(EndpointA);
  return status;
}

/***********************************************************************
 *             RpcServerUseProtseqA (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerUseProtseqA(RPC_CSTR Protseq, unsigned int MaxCalls, void *SecurityDescriptor)
{
  TRACE("(Protseq == %s, MaxCalls == %d, SecurityDescriptor == ^%p)\n", debugstr_a((char*)Protseq), MaxCalls, SecurityDescriptor);
  return RpcServerUseProtseqEpA(Protseq, MaxCalls, NULL, SecurityDescriptor);
}

/***********************************************************************
 *             RpcServerUseProtseqW (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerUseProtseqW(RPC_WSTR Protseq, unsigned int MaxCalls, void *SecurityDescriptor)
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
    sif->MgrTypeUuid = *MgrTypeUuid;
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
  list_add_head(&server_interfaces, &sif->entry);
  LeaveCriticalSection(&server_cs);

  if (sif->Flags & RPC_IF_AUTOLISTEN)
      RPCRT4_start_listen(TRUE);

  return RPC_S_OK;
}

/***********************************************************************
 *             RpcServerUnregisterIf (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerUnregisterIf( RPC_IF_HANDLE IfSpec, UUID* MgrTypeUuid, UINT WaitForCallsToComplete )
{
  PRPC_SERVER_INTERFACE If = (PRPC_SERVER_INTERFACE)IfSpec;
  HANDLE event = NULL;
  BOOL found = FALSE;
  BOOL completed = TRUE;
  RpcServerInterface *cif;
  RPC_STATUS status;

  TRACE("(IfSpec == (RPC_IF_HANDLE)^%p (%s), MgrTypeUuid == %s, WaitForCallsToComplete == %u)\n",
    IfSpec, debugstr_guid(&If->InterfaceId.SyntaxGUID), debugstr_guid(MgrTypeUuid), WaitForCallsToComplete);

  EnterCriticalSection(&server_cs);
  LIST_FOR_EACH_ENTRY(cif, &server_interfaces, RpcServerInterface, entry) {
    if ((!IfSpec || !memcmp(&If->InterfaceId, &cif->If->InterfaceId, sizeof(RPC_SYNTAX_IDENTIFIER))) &&
        UuidEqual(MgrTypeUuid, &cif->MgrTypeUuid, &status)) {
      list_remove(&cif->entry);
      TRACE("unregistering cif %p\n", cif);
      if (cif->CurrentCalls) {
        completed = FALSE;
        cif->Delete = TRUE;
        if (WaitForCallsToComplete)
          cif->CallsCompletedEvent = event = CreateEventW(NULL, FALSE, FALSE, NULL);
      }
      found = TRUE;
      break;
    }
  }
  LeaveCriticalSection(&server_cs);

  if (!found) {
    ERR("not found for object %s\n", debugstr_guid(MgrTypeUuid));
    return RPC_S_UNKNOWN_IF;
  }

  if (completed)
    HeapFree(GetProcessHeap(), 0, cif);
  else if (event) {
    /* sif will be freed when the last call is completed, so be careful not to
     * touch that memory here as that could happen before we get here */
    WaitForSingleObject(event, INFINITE);
    CloseHandle(event);
  }

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
    map->Object = *ObjUuid;
    map->Type = *TypeUuid;
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
RPC_STATUS WINAPI RpcServerRegisterAuthInfoA( RPC_CSTR ServerPrincName, ULONG AuthnSvc, RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFn,
                            LPVOID Arg )
{
  FIXME( "(%s,%u,%p,%p): stub\n", ServerPrincName, AuthnSvc, GetKeyFn, Arg );
  
  return RPC_S_UNKNOWN_AUTHN_SERVICE; /* We don't know any authentication services */
}

/***********************************************************************
 *             RpcServerRegisterAuthInfoW (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerRegisterAuthInfoW( RPC_WSTR ServerPrincName, ULONG AuthnSvc, RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFn,
                            LPVOID Arg )
{
  FIXME( "(%s,%u,%p,%p): stub\n", debugstr_w( ServerPrincName ), AuthnSvc, GetKeyFn, Arg );
  
  return RPC_S_UNKNOWN_AUTHN_SERVICE; /* We don't know any authentication services */
}

/***********************************************************************
 *             RpcServerListen (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerListen( UINT MinimumCallThreads, UINT MaxCalls, UINT DontWait )
{
  RPC_STATUS status = RPC_S_OK;

  TRACE("(%u,%u,%u)\n", MinimumCallThreads, MaxCalls, DontWait);

  if (list_empty(&protseqs))
    return RPC_S_NO_PROTSEQS_REGISTERED;

  status = RPCRT4_start_listen(FALSE);

  if (DontWait || (status != RPC_S_OK)) return status;

  return RpcMgmtWaitServerListen();
}

/***********************************************************************
 *             RpcMgmtServerWaitListen (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcMgmtWaitServerListen( void )
{
  RpcServerProtseq *cps;

  EnterCriticalSection(&listen_cs);

  if (!std_listen) {
    LeaveCriticalSection(&listen_cs);
    return RPC_S_NOT_LISTENING;
  }

  do {
    LeaveCriticalSection(&listen_cs);
    LIST_FOR_EACH_ENTRY(cps, &protseqs, RpcServerProtseq, entry)
      WaitForSingleObject(cps->server_ready_event, INFINITE);

    EnterCriticalSection(&listen_cs);
  } while (!std_listen);

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
 *             RpcMgmtEnableIdleCleanup (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcMgmtEnableIdleCleanup(void)
{
    FIXME("(): stub\n");
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
  FIXME( "(%p,%08x,%08x,%08x): stub\n", hWnd, Message, wParam, lParam );

  return 0;
}

/***********************************************************************
 *             RpcMgmtInqIfIds (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcMgmtInqIfIds(RPC_BINDING_HANDLE Binding, RPC_IF_ID_VECTOR **IfIdVector)
{
  FIXME("(%p,%p): stub\n", Binding, IfIdVector);
  return RPC_S_INVALID_BINDING;
}

/***********************************************************************
 *             RpcMgmtInqStats (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcMgmtInqStats(RPC_BINDING_HANDLE Binding, RPC_STATS_VECTOR **Statistics)
{
  RPC_STATS_VECTOR *stats;

  FIXME("(%p,%p)\n", Binding, Statistics);

  if ((stats = HeapAlloc(GetProcessHeap(), 0, sizeof(RPC_STATS_VECTOR))))
  {
    stats->Count = 1;
    stats->Stats[0] = 0;
    *Statistics = stats;
    return RPC_S_OK;
  }
  return RPC_S_OUT_OF_RESOURCES;
}

/***********************************************************************
 *             RpcMgmtStatsVectorFree (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcMgmtStatsVectorFree(RPC_STATS_VECTOR **StatsVector)
{
  FIXME("(%p)\n", StatsVector);

  if (StatsVector)
  {
    HeapFree(GetProcessHeap(), 0, *StatsVector);
    *StatsVector = NULL;
  }
  return RPC_S_OK;
}

/***********************************************************************
 *             RpcMgmtEpEltInqBegin (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcMgmtEpEltInqBegin(RPC_BINDING_HANDLE Binding, ULONG InquiryType,
    RPC_IF_ID *IfId, ULONG VersOption, UUID *ObjectUuid, RPC_EP_INQ_HANDLE* InquiryContext)
{
  FIXME("(%p,%u,%p,%u,%p,%p): stub\n",
        Binding, InquiryType, IfId, VersOption, ObjectUuid, InquiryContext);
  return RPC_S_INVALID_BINDING;
}

/***********************************************************************
 *             RpcMgmtIsServerListening (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcMgmtIsServerListening(RPC_BINDING_HANDLE Binding)
{
  FIXME("(%p): stub\n", Binding);
  return RPC_S_INVALID_BINDING;
}

/***********************************************************************
 *             RpcMgmtSetServerStackSize (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcMgmtSetServerStackSize(ULONG ThreadStackSize)
{
  FIXME("(0x%x): stub\n", ThreadStackSize);
  return RPC_S_OK;
}

/***********************************************************************
 *             I_RpcGetCurrentCallHandle (RPCRT4.@)
 */
RPC_BINDING_HANDLE WINAPI I_RpcGetCurrentCallHandle(void)
{
    TRACE("\n");
    return RPCRT4_GetThreadCurrentCallHandle();
}
