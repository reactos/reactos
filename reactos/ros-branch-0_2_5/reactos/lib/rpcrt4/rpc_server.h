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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_RPC_SERVER_H
#define __WINE_RPC_SERVER_H

#include "rpc_binding.h"

typedef struct _RpcServerProtseq
{
  struct _RpcServerProtseq* Next;
  LPSTR Protseq;
  LPSTR Endpoint;
  UINT MaxCalls;
  RpcConnection* conn;
} RpcServerProtseq;

typedef struct _RpcServerInterface
{
  struct _RpcServerInterface* Next;
  RPC_SERVER_INTERFACE* If;
  UUID MgrTypeUuid;
  RPC_MGR_EPV* MgrEpv;
  UINT Flags;
  UINT MaxCalls;
  UINT MaxRpcSize;
  RPC_IF_CALLBACK_FN* IfCallbackFn;
} RpcServerInterface;

#endif  /* __WINE_RPC_SERVER_H */
