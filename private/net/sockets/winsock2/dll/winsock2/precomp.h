/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    precomp.h

Abstract:

    This file includes all the headers required to build winsock2.dll
    to ease the process of building a precompiled header.

Author:

Dirk Brandewie dirk@mink.intel.com  11-JUL-1995

Revision History:


--*/

#ifndef _PRECOMP_
#define _PRECOMP_


//
// Turn off "declspec" decoration of entrypoints defined in WINSOCK2.H.
//

#define WINSOCK_API_LINKAGE


#include "osdef.h"
#include "warnoff.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2spi.h>
#include <mswsock.h>
#include <sporder.h>
#include <windows.h>
#include <wtypes.h>
#include <stdio.h>
#include <tchar.h>
#include "trace.h"
#include "wsassert.h"
#include "scihlpr.h"
#include "nsprovid.h"
#include "nspstate.h"
#include "nscatent.h"
#include "nscatalo.h"
#include "nsquery.h"
#include "llist.h"
#include "ws2help.h"
#include "dprovide.h"
#include "dsocket.h"
#include "dprocess.h"
#include "dthread.h"
#include "wsautil.h"
#include "dcatalog.h"
#include "dcatitem.h"
#include "startup.h"
#include "dt_dll.h"
#include "dthook.h"
#include "trycatch.h"
#include "getxbyy.h"
#include "qshelpr.h"
#ifdef RASAUTODIAL
#include "autodial.h"
#endif // RASAUTODIAL
#include "async.h"

#endif  // _PRECOMP_

