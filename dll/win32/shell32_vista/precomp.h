#pragma once

#include <stdarg.h>
#include <assert.h>

#define COBJMACROS
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define NTOS_MODE_USER

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <ddeml.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <tchar.h>
#include <strsafe.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <atlstr.h>

#include <comctl32_undoc.h>
#include <shlguid_undoc.h>
#include <shlobj_undoc.h>
#include <shlwapi_undoc.h>
#include <undocshell.h>
#include <browseui_undoc.h>

#include <shellutils.h>

#include <wine/debug.h>
#include <wine/unicode.h>
