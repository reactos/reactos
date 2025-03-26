//#pragma once

#ifndef __REACTOS__

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
#include <dbt.h>
#include <RegStr.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit
#include <tchar.h>
#include <atlbase.h>
#include <atlstr.h>
#include <atlcoll.h>

#include <strsafe.h>

#include <devmgr/devmgr.h>

#define ERR printf
#define FIXME printf
#define UNIMPLEMENTED
#define WINE_DEFAULT_DEBUG_CHANNEL(t)

DWORD WINAPI pSetupGuidFromString(PCWSTR pString, LPGUID lpGUID);

BOOL
WINAPI
InstallDevInst(
IN HWND hWndParent,
IN LPCWSTR InstanceId,
IN BOOL bUpdate,
OUT LPDWORD lpReboot);

#else

#include <string.h>
#include <wchar.h>

#include <tchar.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <winnls.h>
#include <wincon.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <process.h>
#include <windowsx.h>
#include <strsafe.h>
#include <regstr.h>
#include <newdevp.h>
#include <dbt.h>

#include <setupapi.h>
#include <commctrl.h>
#include <cfgmgr32.h>
#include <uxtheme.h>
#include <devguid.h>

#include <atlbase.h>
#include <atlstr.h>
#include <atlcoll.h>

#include <devmgr/devmgr.h>
#include <wine/debug.h>

#define DYNAMIC_FIELD_OFFSET(Type, Field) ((LONG)(LONG_PTR)&(((Type*) 0)->Field))

//WINE_DEFAULT_DEBUG_CHANNEL(devmgr);

#endif
