#ifndef _NFSD_PRECOMP_H_
#define _NFSD_PRECOMP_H_

#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <tchar.h>
#include <time.h>
#include <windows.h>
#include <strsafe.h>
#include <devioctl.h>
#include <iphlpapi.h>
#include <wincrypt.h>
#include <winsock2.h>

#include "daemon_debug.h"
#include "delegation.h"
#include "from_kernel.h"
#include "idmap.h"
#include "nfs41.h"
#include "nfs41_callback.h"
#include "nfs41_compound.h"
#include "nfs41_ops.h"
#include "name_cache.h"
#include "nfs41_xdr.h"
#include "recovery.h"
#include "tree.h"
#include "upcall.h"
#include "util.h"

#include <nfs41_driver.h>
#include <rpc/rpc.h>
#include <rpc/auth_sspi.h>

#endif /* _NFSD_PRECOMP_H_ */
