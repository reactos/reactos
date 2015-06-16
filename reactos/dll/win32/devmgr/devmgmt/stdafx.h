#pragma once

#ifndef __REACTOS___

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <windowsx.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <commctrl.h>
#include <Uxtheme.h>
#include <Cfgmgr32.h>
#include <devguid.h>
#include <process.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit
#include <tchar.h>
#include <atlbase.h>
#include <atlstr.h>
#include <atlcoll.h>

#else

#include <tchar.h>

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <winnls.h>
#include <wincon.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <process.h>

#include <string.h>
#include <wchar.h>

#include <setupapi.h>
#include <commctrl.h>
#include <cfgmgr32.h>
#include <Uxtheme.h>

#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <atlstr.h>

#endif