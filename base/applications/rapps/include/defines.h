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

/* Name of the registry sub-key where RAPPS settings are stored, and
 * of the local AppData sub-directory where the RAPPS files are stored. */
#define RAPPS_NAME                  L"RApps"

/* Name of the RAPPS sub-directory where the offline RAPPS database is stored */
#define RAPPS_DATABASE_SUBDIR       L"appdb"

/* URL and filename of the online RAPPS database */
#define APPLICATION_DATABASE_URL    L"https://rapps.reactos.org/rappmgr2.cab"
#define APPLICATION_DATABASE_NAME   L"rappmgr2.cab"

#define MAX_STR_LEN 256
