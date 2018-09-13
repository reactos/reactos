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

// an astonishingly stupid warning
#pragma warning(disable:4124)

#include "biavst.h"

#include <windef.h>
#include <stdio.h>
#include <memory.h>
#include <malloc.h>
#include <process.h>
#include <stdlib.h>
#include <tchar.h>
#include <crt\io.h>
#include <fcntl.h>
#include <setjmp.h>
#include <dbghelp.h>

#define NOEXTAPI
#include <wdbgexts.h>

#include <cvinfo.h>
#include <od.h>
#include <odp.h>
#include <emdm.h>
#include <win32dm.h>

#include <sundown.h>

#include "dm.h"
#include "resource.h"
#include "list.h"
#include "bp.h"
#include "dmkd.h"
#include "funccall.h"
#include "debug.h"
#include "dbgver.h"

