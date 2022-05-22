/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Precompiled Header.
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 */

#ifndef __PRECOMP_H
#define __PRECOMP_H

#ifndef UNICODE
#error Task-Manager uses NDK functions, so it can only be compiled with Unicode support enabled!
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef _DEBUG
    #define _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>
#endif

#include <assert.h>
#define ASSERT assert

#define WIN32_NO_STATUS

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winnls.h>
#include <winuser.h>
#include <winreg.h>
#include <commctrl.h>
#include <shellapi.h>
#include <tlhelp32.h>

#include <strsafe.h>

#include "column.h"
#include "taskmgr.h"
#include "perfdata.h"
#include "applpage.h"
#include "procpage.h"
#include "perfpage.h"
#include "endproc.h"
#include "graph.h"
#include "graphctl.h"
#include "optnmenu.h"
#include "run.h"
#include "trayicon.h"
#include "shutdown.h"

#endif /* __PRECOMP_H */
