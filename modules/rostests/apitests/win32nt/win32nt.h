#ifndef _WIN32NT_APITEST_H_
#define _WIN32NT_APITEST_H_

/* Definitions */
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define NTOS_MODE_USER

#include <apitest.h>

/* SDK/DDK/NDK Headers. */
#include <stdio.h>
#include <wingdi.h>
#include <objbase.h>
#include <imm.h>

#include <winddi.h>
#include <prntfont.h>

#include <ndk/rtlfuncs.h>
#include <ndk/mmfuncs.h>

/* Public Win32K Headers */
#include <ntuser.h>
#include <ntgdityp.h>
#include <ntgdi.h>
#include <ntgdihdl.h>

#include <gditools.h>

#define TEST(x) ok(x, "TEST failed: %s\n", #x)
#define RTEST(x) ok(x, "RTEST failed: %s\n", #x)

#define GdiHandleTable GdiQueryTable()

#endif /* !_WIN32NT_APITEST_H_ */
