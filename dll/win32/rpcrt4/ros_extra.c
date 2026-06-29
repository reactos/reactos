
#include <stdarg.h>

#include "rpc.h"
#include "rpcndr.h"
#include "rpcasync.h"

#include "wine/debug.h"

#include "rpc_binding.h"
#include "rpc_message.h"
#include "ndr_stubless.h"

WINE_DEFAULT_DEBUG_CHANNEL(rpc);

/***********************************************************************
 *           RpcGetAuthorizationContextForClient [RPCRT4.@]
 *
 * Called by RpcFreeAuthorizationContext to return the Authz context.
 *
 * PARAMS
 *  ClientBinding        [I] Binding handle, represents a binding to a client on the server.
 *  ImpersonateOnReturn  [I] Directs this function to be represented the client on return.
 *  Reserved1            [I] Reserved, equal to null.
 *  expiration_time      [I] Points to the exact date and time when the token expires.
 *  Reserved2            [I] Reserved, equal to a LUID structure which has a members,
 *                           each of them is set to zero.
 *  Reserved3            [I] Reserved, equal to zero.
 *  Reserved4            [I] Reserved, equal to null.
 *  authz_client_context [I] Points to an AUTHZ_CLIENT_CONTEXT_HANDLE structure
 *                           that has direct pass to Authz functions.
 *
 * RETURNS
 *  Success: RPC_S_OK.
 *  Failure: Any error code.
 */
RPC_STATUS
WINAPI
RpcGetAuthorizationContextForClient(RPC_BINDING_HANDLE ClientBinding,
                                    BOOL ImpersonateOnReturn,
                                    void * Reserved1,
                                    PLARGE_INTEGER expiration_time,
                                    LUID Reserved2,
                                    DWORD Reserved3,
                                    PVOID Reserved4,
                                    PVOID *authz_client_context)
{
    FIXME("(%p, %d, %p, %p, (%d, %u), %u, %p, %p): stub\n",
          ClientBinding,
          ImpersonateOnReturn,
          Reserved1,
          expiration_time,
          Reserved2.HighPart,
          Reserved2.LowPart,
          Reserved3,
          Reserved4,
          authz_client_context);
    return RPC_S_NO_CONTEXT_AVAILABLE;
}
