#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <stdio.h>
#include <wine/test.h>


#include <winuser.h>
#include <winreg.h>

#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <tchar.h>

#include <initguid.h>

#define test_S_OK(hres, message) ok(hres == S_OK, "%s (0x%lx instead of S_OK)\n",message, hResult);
#define test_HRES(hres, hresExpected, message) ok(hres == hresExpected, "%s (0x%lx instead of 0x%lx)\n",message, hResult,hresExpected);

DEFINE_GUID(CLSID_MenuBandSite, 0xE13EF4E4, 0xD2F2, 0x11D0, 0x98, 0x16, 0x00, 0xC0, 0x4F, 0xD9, 0x19, 0x72);

#include "unknownbase.h"
