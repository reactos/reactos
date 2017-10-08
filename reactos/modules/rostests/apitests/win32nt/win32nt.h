
#pragma once

/* Definitions */
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define NTOS_MODE_USER

#include <apitest.h>

/* SDK/DDK/NDK Headers. */
#include <stdio.h>
#include <excpt.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <wincon.h>
#include <winnls.h>
#include <winver.h>
#include <winnetwk.h>
#include <winreg.h>
#include <winsvc.h>
#include <objbase.h>
#include <imm.h>

#include <winddi.h>
#include <prntfont.h>
#include <winddiui.h>
#include <winspool.h>
#include <ddrawi.h>
#include <ddrawgdi.h>

#include <ndk/ntndk.h>

/* Public Win32K Headers */
#include <ntusrtyp.h>
#include <ntuser.h>
#include <callback.h>
#include <ntgdityp.h>
#include <ntgdi.h>
#include <ntgdihdl.h>

#include <gditools.h>

#define TEST(x) ok(x, "TEST failed: %s\n", #x)
#define RTEST(x) ok(x, "RTEST failed: %s\n", #x)
#define TESTX ok

#define GdiHandleTable GdiQueryTable()


