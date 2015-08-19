
#include <stdio.h>
#include <tchar.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <winnls.h>
#include <wincon.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlobj_undoc.h>
#include <shlwapi.h>
#include <shlguid_undoc.h>
#include <strsafe.h>
#include <browseui_undoc.h>

#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <undocshell.h>
#include <undocuser.h>

#include <shellutils.h>
#include "../shresdef.h"

#include <wine/debug.h>

extern "C" HINSTANCE shell32_hInstance;
