//#pragma once

#ifndef __REACTOS__

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <windowsx.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <commctrl.h>
#include <Uxtheme.h>
#include <richedit.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit
#include <tchar.h>
#include <atlbase.h>
#include <atlstr.h>
#include <atlcoll.h>

#include <strsafe.h>


#define WINE_DEFAULT_DEBUG_CHANNEL(t)

#include "resource.h"


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
#include <shlwapi.h>
#include <strsafe.h>

#include <commctrl.h>
#include <cfgmgr32.h>
#include <uxtheme.h>

#include <atlbase.h>
#include <atlstr.h>
#include <atlcoll.h>

//WINE_DEFAULT_DEBUG_CHANNEL(charmap);

#endif
