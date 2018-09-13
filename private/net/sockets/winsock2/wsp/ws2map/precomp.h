/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    This is the master include file for the Winsock 2 to Winsock 1.1
    Mapper Service Provider.

Author:

    Keith Moore (keithmo) 29-May-1996

Revision History:

--*/


#ifndef _PRECOMP_H_
#define _PRECOMP_H_


//
// System include files.
//

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif

#define INCL_WINSOCK_API_PROTOTYPES 1
#define INCL_WINSOCK_API_TYPEDEFS 1

#include <windows.h>
#include <winsock2.h>
#include <ws2spi.h>
#include <rpc.h>
#include <svcguid.h>

#include <memory.h>
#include <stdlib.h>
#include <wchar.h>
#include <tchar.h>


//
// Local include files.
//

#include "debug.h"
#include "cons.h"
#include "type.h"
#include "proc.h"
#include "data.h"

#include "nthack.h"


#endif // _PRECOMP_H_

