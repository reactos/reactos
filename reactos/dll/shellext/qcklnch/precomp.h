#pragma once

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <tchar.h>

#define COBJMACROS
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define NTOS_MODE_USER

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>
#include <wincon.h>
#include <ddeml.h>
#include <shlguid_undoc.h>
#include <shlwapi.h>
#include <shlguid.h>
#include <shlobj.h>
#include <shlobj_undoc.h>
#include <shlwapi_undoc.h>
#include <tchar.h>
#include <strsafe.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <atlstr.h>
#include <undocshell.h>
#include <shellutils.h>

#include <shellapi.h>

#include <wine/debug.h>
#include <wine/unicode.h>

#include "resource.h"

#undef DbgPrint

#include "CQuickLaunchBand.h"
#include "CISFBand.h"