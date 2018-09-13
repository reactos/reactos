/*--

Copyright (c) 1992  Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    Header file that is pre-compiled into a .pch file

Author:

    Wesley Witt (wesw) 21-Sep-1993

Environment:

    Win32, User Mode

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#define NOEXTAPI
#include <wdbgexts.h>
#include <ntdbg.h>
#include <windows.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <process.h>
#include <tchar.h>
#include <malloc.h>

#include <cvinfo.h>

#if defined(TARGET_i386) && !defined(WIN32S)
#include <vdmdbg.h>
#endif

#if defined(TARGET_ALPHA) || defined(TARGET_AXP64)
#include <alphaops.h>
#include "ctxptrs.h"
#endif

#if defined(TARGET_IA64)
#include <ia64inst.h>
#include "kxia64.h"
#endif

#if defined(TARGET_MIPS)
#include <mipsinst.h>
#endif

#if defined(TARGET_PPC)
#include <ppcinst.h>
#endif

#include <orpc_dbg.h>

#include <od.h>
#include <odp.h>
#include <emdm.h>
#include <win32dm.h>

#include <sundown.h>

#include "dm.h"
#include "resource.h"
#include "dmole.h"
#include "list.h"
#include "bp.h"
#include "funccall.h"
#include "debug.h"
#include "dbgver.h"
