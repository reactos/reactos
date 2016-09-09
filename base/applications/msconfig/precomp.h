#ifndef _MSCONFIG_PCH_
#define _MSCONFIG_PCH_

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define NTOS_MODE_USER

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <tchar.h>
#include <stdio.h>
#include <shlobj.h>
#include <strsafe.h>

#include "msconfig.h"
#include "resource.h"

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

#endif /* _MSCONFIG_PCH_ */
