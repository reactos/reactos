#ifndef _MSCONFIG_PCH_
#define _MSCONFIG_PCH_

#include <assert.h>

#include <stdarg.h>

#include <stdio.h>
#include <string.h> // FIXME: Should be normally useless in a proper CRT...
#include <tchar.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define NTOS_MODE_USER

#define _FORCENAMELESSUNION
// #define _WIN32_DCOM // For CoInitializeEx on Win2k and perhaps XP / 2003 ??

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winreg.h>
#include <winuser.h>
#include <winver.h>

#include <initguid.h>
// #include <shlobj.h> // If used, initguid.h must be included before!
#include <shlwapi.h>
#include <shellapi.h>
#include <commctrl.h>
#include <prsht.h>
#include <strsafe.h>

#include "msconfig.h"
#include "resource.h"

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

#endif /* _MSCONFIG_PCH_ */
