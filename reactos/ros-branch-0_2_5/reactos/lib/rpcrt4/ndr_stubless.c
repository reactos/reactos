/*
 * NDR client stuff
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
 *
 * TODO:
 *  - Exception handling
 *  - Context stuff
 *  - Who knows
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"

#include "rpc.h"
#include "rpcndr.h"

#include "wine/debug.h"

#include "ndr_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

LONG_PTR /* CLIENT_CALL_RETURN */ RPCRT4_NdrClientCall2(PMIDL_STUB_DESC pStubDesc, PFORMAT_STRING pFormat, va_list args)
{

  RPC_CLIENT_INTERFACE *rpc_cli_if = (RPC_CLIENT_INTERFACE *)(pStubDesc->RpcInterfaceInformation);
  LONG_PTR ret = 0;
/*
  RPC_BINDING_HANDLE handle = 0;
  RPC_MESSAGE rpcmsg;
  MIDL_STUB_MESSAGE stubmsg;
*/

  FIXME("(pStubDec == ^%p,pFormat = ^%p,...): stub\n", pStubDesc, pFormat);
  if (rpc_cli_if) /* NULL for objects */ {
    TRACE("  *rpc_cli_if (== ^%p) == (RPC_CLIENT_INTERFACE):\n", pStubDesc);
    TRACE("    Length == %d\n", rpc_cli_if->Length);
    TRACE("    InterfaceID == %s (%d.%d)\n", debugstr_guid(&rpc_cli_if->InterfaceId.SyntaxGUID), 
      rpc_cli_if->InterfaceId.SyntaxVersion.MajorVersion, rpc_cli_if->InterfaceId.SyntaxVersion.MinorVersion);
    TRACE("    TransferSyntax == %s (%d.%d)\n", debugstr_guid(&rpc_cli_if->TransferSyntax.SyntaxGUID),
      rpc_cli_if->TransferSyntax.SyntaxVersion.MajorVersion, rpc_cli_if->TransferSyntax.SyntaxVersion.MinorVersion);
    TRACE("    DispatchTable == ^%p\n", rpc_cli_if->DispatchTable);
    TRACE("    RpcProtseqEndpointCount == ^%d\n", rpc_cli_if->RpcProtseqEndpointCount);
    TRACE("    RpcProtseqEndpoint == ^%p\n", rpc_cli_if->RpcProtseqEndpoint);
    TRACE("    Flags == ^%d\n", rpc_cli_if->Flags);
  }

  /* for now, while these functons are under development, this is too sketchy.  commented out. */
  /*
  NdrClientInitializeNew( &rpcmsg, &stubmsg, pStubDesc, 0 );
        
  handle = (RPC_BINDING_HANDLE)0xdeadbeef; */ /* FIXME */

  /* stubmsg.BufferLength = 0;*/ /* FIXME */
  /*
  NdrGetBuffer( &stubmsg, stubmsg.BufferLength, handle );
  NdrSendReceive( &stubmsg, stubmsg.Buffer  );
  NdrFreeBuffer( &stubmsg );
  */
  return ret;
}

/***********************************************************************
 *           NdrClientCall2 [RPCRT4.@]
 */
LONG_PTR /* CLIENT_CALL_RETURN */ WINAPIV NdrClientCall2(PMIDL_STUB_DESC pStubDesc,
  PFORMAT_STRING pFormat, ...)
{
    LONG_PTR ret;
    va_list args;

    TRACE("(%p,%p,...)\n", pStubDesc, pFormat);

    va_start(args, pFormat);
    ret = RPCRT4_NdrClientCall2(pStubDesc, pFormat, args);
    va_end(args);
    return ret;
}
