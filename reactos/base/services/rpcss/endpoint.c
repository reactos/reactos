/* 
 * PROJECT:          ReactOS RpcSs service
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             services/rpcss/rpcss.c
 * PURPOSE:          Endpoint mapper
 * COPYRIGHT:        Copyright 2002 Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "rpc.h"

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
