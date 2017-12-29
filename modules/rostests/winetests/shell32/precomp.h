#ifndef _SHELL32_WINETEST_PRECOMP_H_
#define _SHELL32_WINETEST_PRECOMP_H_

#include <assert.h>
#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define CONST_VTABLE

#include <wine/test.h>

#include <winreg.h>
#include <winnls.h>
#include <winuser.h>
#include <wincon.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlguid.h>
#include <shlobj.h>
#include <ddeml.h>
#include <commoncontrols.h>
#include <reactos/undocshell.h>

#include "shell32_test.h"

#endif /* !_SHELL32_WINETEST_PRECOMP_H_ */
