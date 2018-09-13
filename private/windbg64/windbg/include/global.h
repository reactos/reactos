/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Global.h

Abstract:

    This module is the master header file for the Windbg debugger. It is
    included by every source module in the debugger and is expected to
    be precompiled eventually.

    It only includes .h files with global impact

Author:

    Griffith Wm. Kadnier (v-griffk) 12-Dec-1992

Environment:

    Win32, User Mode

--*/

#if DBG

//
// DbgPrint, ASSERT & ASSERTMSG access.
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntdbg.h>
#define DWORDLONG ULONGLONG

#else

typedef unsigned long PHYSICAL_ADDRESS;

#endif // DBG

#include <windows.h>
#include <shellapi.h>

//
// CRT Headers
//

#include <ctype.h>
#include <direct.h>
#include <dos.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <malloc.h>
#include <math.h>
#include <memory.h>
#include <process.h>
#include <search.h>
#include <share.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys\types.h>
#include <sys\stat.h>
#include <sys\timeb.h>


#include "cvinfo.h"
#include "cvtypes.hxx"
#include "lltypes.h"
#include "linklist.h"
#include "shapi.h"
#include "windbg.h"

#include "change.h"
#include "cl.h"
#include "cmdexec.h"
#include "cmgrhigh.h"
#include "cmgrlow.h"
#include "debug.h"
#include "dialogs.h"
#include "eeproto.h"
#include "export.h"
#include "extern.h"
#include "filedll.h"
#include "inifile.h"
#include "mstools.h"
#include "re.h"
#include "resource.h"
#include "res_str.h"
#include "sbconst.h"
#include "settings.h"
#include "shproto.h"
#include "systemw3.h"
#include "vapi.h"
#include "windbg.h"
#include "winpck.h"
