/*++

     Copyright c 1996 Intel Corporation
     All Rights Reserved
     
     Permission is granted to use, copy and distribute this software and 
     its documentation for any purpose and without fee, provided, that 
     the above copyright notice and this statement appear in all copies. 
     Intel makes no representations about the suitability of this 
     software for any purpose.  This software is provided "AS IS."  
     
     Intel specifically disclaims all warranties, express or implied, 
     and all liability, including consequential and other indirect 
     damages, for the use of this software, including liability for 
     infringement of any proprietary rights, and including the 
     warranties of merchantability and fitness for a particular purpose. 
     Intel does not assume any responsibility for any errors which may 
     appear in this software nor any responsibility to update it.

Module Name:

    precomp.h

Abstract:

    This file includes all the headers required to build msrlsp.dll
    to ease the process of building a precompiled header.

Author:

    bugs@brandy.jf.intel.com

Revision History:


--*/

#ifndef _PRECOMP_
#define _PRECOMP_

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

//
// Turn off "declspec" decoration of entrypoints defined in WINSOCK2.H.
//

#define WINSOCK_API_LINKAGE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
// #include <windows.h>
#include <ws2spi.h>
#include <wtypes.h> 
#include <assert.h>
// #include <winnt.h>
#include "trace.h"
#include "llist.h"
#include "ws2_if.h"
#include "rprovide.h"
#include "rsocket.h"
#include "dcatalog.h"
#include "dcatitem.h"
#include "rworker.h"
#include "doverlap.h"
#include "dbuffmgr.h"
#include "dt_dll.h"
#include "dthook.h"
#include "cmap.h"

#endif  // _PRECOMP_
