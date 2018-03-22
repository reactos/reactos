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

enum AppsCategories
{
    ENUM_ALL_AVAILABLE,
    ENUM_CAT_AUDIO,
    ENUM_CAT_VIDEO,
    ENUM_CAT_GRAPHICS,
    ENUM_CAT_GAMES,
    ENUM_CAT_INTERNET,
    ENUM_CAT_OFFICE,
    ENUM_CAT_DEVEL,
    ENUM_CAT_EDU,
    ENUM_CAT_ENGINEER,
    ENUM_CAT_FINANCE,
    ENUM_CAT_SCIENCE,
    ENUM_CAT_TOOLS,
    ENUM_CAT_DRIVERS,
    ENUM_CAT_LIBS,
    ENUM_CAT_OTHER,
    ENUM_CAT_SELECTED,
    ENUM_ALL_INSTALLED,
    ENUM_INSTALLED_APPLICATIONS = 31,
    ENUM_UPDATES = 32,
    ENUM_INSTALLED_MIN = ENUM_ALL_INSTALLED,
    ENUM_INSTALLED_MAX = ENUM_UPDATES,
    ENUM_AVAILABLE_MIN = ENUM_ALL_AVAILABLE,
    ENUM_AVAILABLE_MAX = ENUM_CAT_SELECTED,
};

inline BOOL IsAvailableEnum(INT x)
{
    return (x >= ENUM_AVAILABLE_MIN && x <= ENUM_AVAILABLE_MAX);
}

inline BOOL IsInstalledEnum(INT x)
{
    return (x >= ENUM_INSTALLED_MIN && x <= ENUM_INSTALLED_MAX);
}
