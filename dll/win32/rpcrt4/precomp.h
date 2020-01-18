
#ifndef _RPCRT4_PCH_
#define _RPCRT4_PCH_

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#define _INC_WINDOWS

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winternl.h>
#include <winnls.h>
#include <objbase.h>
#include <rpcproxy.h>
#include <security.h>

#include <wine/debug.h>
#include <wine/exception.h>
#include <wine/list.h>
#include <wine/rpcfc.h>

#include "cpsf.h"
#include "ncastatus.h"
#include "ndr_misc.h"
#include "ndr_stubless.h"
#include "rpc_assoc.h"
#include "rpc_binding.h"
#include "rpc_message.h"
#include "rpc_server.h"

#endif /* !_RPCRT4_PCH_ */
