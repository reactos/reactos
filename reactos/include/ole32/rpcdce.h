#ifndef __WINE_RPCDCE_H
#define __WINE_RPCDCE_H

typedef void* RPC_AUTH_IDENTITY_HANDLE;
typedef void* RPC_AUTHZ_HANDLE;
typedef void* RPC_IF_HANDLE;
typedef I_RPC_HANDLE RPC_BINDING_HANDLE;
typedef RPC_BINDING_HANDLE handle_t;
#define rpc_binding_handle_t RPC_BINDING_HANDLE
#define RPC_MGR_EPV void

#include "rpcdcep.h"

#endif /*__WINE_RPCDCE_H */
