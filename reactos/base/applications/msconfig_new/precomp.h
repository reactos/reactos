#ifndef _MSCONFIG_PCH_
#define _MSCONFIG_PCH_

// NOTE: It is completely idiotic to need those defines defined
// for having the *_s string functions. In the MS CRT they are
// directly available without further tricks.
#define MINGW_HAS_SECURE_API    1

#include <assert.h>

#include <stdarg.h>

#include <stdio.h>
#include <string.h> // FIXME: Should be normally useless in a proper CRT...
#include <tchar.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define NTOS_MODE_USER

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winreg.h>
#include <winuser.h>
#include <winver.h>

#include <shlobj.h>
#include <strsafe.h>

#include "msconfig.h"
#include "resource.h"

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

#endif /* _MSCONFIG_PCH_ */
