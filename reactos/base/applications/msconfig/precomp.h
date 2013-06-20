#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define NTOS_MODE_USER
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winsvc.h>
#include <winver.h>
#include <tchar.h>
#include <stdio.h>
#include <shlwapi.h>
#include <shlobj.h>

#include "resource.h"
#include "msconfig.h"
#include "toolspage.h"
#include "srvpage.h"
#include "startuppage.h"
#include "freeldrpage.h"
#include "systempage.h"
#include "generalpage.h"

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383
