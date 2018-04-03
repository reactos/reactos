
#ifndef _SHELL32_WINETEST_PRECOMP_H_
#define _SHELL32_WINETEST_PRECOMP_H_

#include <assert.h>
#include <stdio.h>

#define WIN32_NO_STATUS

#define COBJMACROS
#define CONST_VTABLE

#include <windows.h>

#include <wine/heap.h>
#include <wine/test.h>

#include <shellapi.h>
#include <shlwapi.h>
#include <shlguid.h>
#include <shlobj.h>
#include <commoncontrols.h>
#include <reactos/undocshell.h>

#include "shell32_test.h"

#endif /* !_SHELL32_WINETEST_PRECOMP_H_ */
