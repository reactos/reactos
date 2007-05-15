/*
 * Copyright (C) 2002 Greg Turner
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

#ifndef __WINE_RPCSS_NP_CLIENT_H
#define __WINE_RPCSS_NP_CLIENT_H

/* rpcss_np_client.c */
HANDLE RPC_RpcssNPConnect(void);
BOOL RPCRT4_SendReceiveNPMsg(HANDLE, PRPCSS_NP_MESSAGE, char *,  PRPCSS_NP_REPLY);

#endif /* __RPCSS_NP_CLINET_H */
