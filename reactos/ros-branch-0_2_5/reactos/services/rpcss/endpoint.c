/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: endpoint.c,v 1.1 2002/06/25 21:11:11 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/rpcss/endpoint.c
 * PURPOSE:          RPC server
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <windows.h>
//#include <rpc.h>

/* FUNCTIONS ****************************************************************/

VOID
StartEndpointMapper(VOID)
{
#if 0
  RpcServerRegisterProtseqW("ncacn_np", 1, "\pipe\epmapper");

  RpcServerRegisterProtseqW("ncalrpc", 1, "epmapper");

  RpcServerRegisterIf(epmapper_InterfaceHandle, 0, 0);
#endif
}

/* EOF */
