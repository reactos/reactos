#pragma once

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define COBJMACROS
#include <tchar.h>
#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <winnls.h>
#include <winuser.h>
#include <wincon.h>
#include <richedit.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <stdio.h>
#include <strsafe.h>
#include <ndk/rtlfuncs.h>
#include <atlcoll.h>
#include <atlsimpcoll.h>
#include <atlstr.h> 
#include <rappsmsg.h>

#include "resource.h"
#include "winmain.h"

#define APPLICATION_DATABASE_URL L"https://svn.reactos.org/packages/rappmgr.cab"
#define MAX_STR_LEN              256
#define ENUM_ALL_COMPONENTS      30
