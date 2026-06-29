/*
 * RPC server API
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

#ifndef __WINE_RPC_SERVER_H
#define __WINE_RPC_SERVER_H

#include "rpc_binding.h"
#include "wine/list.h"

struct protseq_ops;

typedef struct _RpcServerProtseq
{
  const struct protseq_ops *ops; /* RO */
  struct list entry; /* CS ::server_cs */
  LPSTR Protseq; /* RO */
  UINT MaxCalls; /* RO */
  /* list of listening connections */
  struct list listeners; /* CS cs */
  struct list connections; /* CS cs */
  CRITICAL_SECTION cs;

  /* handle to listening thread */
  HANDLE server_thread; /* CS ::listen_cs */
  /* mutex for ensuring only one thread can change state at a time */
  HANDLE mgr_mutex;
  /* set when server thread has finished opening connections */
  HANDLE server_ready_event;
} RpcServerProtseq;

struct protseq_ops
{
    const char *name;
    RpcServerProtseq *(*alloc)(void);
    void (*signal_state_changed)(RpcServerProtseq *protseq);
    /* previous array is passed in to allow reuse of memory */
    void *(*get_wait_array)(RpcServerProtseq *protseq, void *prev_array, unsigned int *count);
    void (*free_wait_array)(RpcServerProtseq *protseq, void *array);
    /* returns -1 for failure, 0 for server state changed and 1 to indicate a
     * new connection was established */
    int (*wait_for_new_connection)(RpcServerProtseq *protseq, unsigned int count, void *wait_array);
    /* opens the endpoint and optionally begins listening */
    RPC_STATUS (*open_endpoint)(RpcServerProtseq *protseq, const char *endpoint);
};

typedef struct _RpcServerInterface
{
  struct list entry;
  RPC_SERVER_INTERFACE* If;
  UUID MgrTypeUuid;
  RPC_MGR_EPV* MgrEpv;
  UINT Flags;
  UINT MaxCalls;
  UINT MaxRpcSize;
  RPC_IF_CALLBACK_FN* IfCallbackFn;
  LONG CurrentCalls; /* number of calls currently executing */
  /* set when unregistering interface to let the caller of
   * RpcServerUnregisterIf* know that all calls have finished */
  HANDLE CallsCompletedEvent;
  BOOL Delete; /* delete when the last call finishes */
} RpcServerInterface;

void RPCRT4_new_client(RpcConnection* conn);
const struct protseq_ops *rpcrt4_get_protseq_ops(const char *protseq);

void RPCRT4_destroy_all_protseqs(void);
void RPCRT4_ServerFreeAllRegisteredAuthInfo(void);

#endif  /* __WINE_RPC_SERVER_H */
