/*
 * Associations
 *
 * Copyright 2007 Robert Shearman (for CodeWeavers)
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

#pragma once

#include "rpc_binding.h"
#include "wine/list.h"

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
    UUID http_uuid;

    CRITICAL_SECTION cs;

    /* client-only */
    /* connections available to be used (protected by cs) */
    struct list free_connection_pool;
    LONG connection_cnt;

    /* server-only */
    struct list context_handle_list; /* protected by cs */
} RpcAssoc;

RPC_STATUS RPCRT4_GetAssociation(LPCSTR Protseq, LPCSTR NetworkAddr, LPCSTR Endpoint, LPCWSTR NetworkOptions, RpcAssoc **assoc) DECLSPEC_HIDDEN;
RPC_STATUS RpcAssoc_GetClientConnection(RpcAssoc *assoc, const RPC_SYNTAX_IDENTIFIER *InterfaceId,
    const RPC_SYNTAX_IDENTIFIER *TransferSyntax, RpcAuthInfo *AuthInfo, RpcQualityOfService *QOS,
    LPCWSTR CookieAuth, RpcConnection **Connection, BOOL *from_cache) DECLSPEC_HIDDEN;
void RpcAssoc_ReleaseIdleConnection(RpcAssoc *assoc, RpcConnection *Connection) DECLSPEC_HIDDEN;
ULONG RpcAssoc_Release(RpcAssoc *assoc) DECLSPEC_HIDDEN;
RPC_STATUS RpcServerAssoc_GetAssociation(LPCSTR Protseq, LPCSTR NetworkAddr, LPCSTR Endpoint, LPCWSTR NetworkOptions, ULONG assoc_gid, RpcAssoc **assoc_out) DECLSPEC_HIDDEN;
RPC_STATUS RpcServerAssoc_AllocateContextHandle(RpcAssoc *assoc, void *CtxGuard, NDR_SCONTEXT *SContext) DECLSPEC_HIDDEN;
RPC_STATUS RpcServerAssoc_FindContextHandle(RpcAssoc *assoc, const UUID *uuid, void *CtxGuard, ULONG Flags, NDR_SCONTEXT *SContext) DECLSPEC_HIDDEN;
RPC_STATUS RpcServerAssoc_UpdateContextHandle(RpcAssoc *assoc, NDR_SCONTEXT SContext, void *CtxGuard, NDR_RUNDOWN rundown_routine) DECLSPEC_HIDDEN;
unsigned int RpcServerAssoc_ReleaseContextHandle(RpcAssoc *assoc, NDR_SCONTEXT SContext, BOOL release_lock) DECLSPEC_HIDDEN;
void RpcContextHandle_GetUuid(NDR_SCONTEXT SContext, UUID *uuid) DECLSPEC_HIDDEN;
BOOL RpcContextHandle_IsGuardCorrect(NDR_SCONTEXT SContext, void *CtxGuard) DECLSPEC_HIDDEN;
void RpcAssoc_ConnectionReleased(RpcAssoc *assoc) DECLSPEC_HIDDEN;
