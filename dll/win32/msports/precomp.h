#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>
#include <wchar.h>
#include <cmtypes.h>
#include <msports.h>
#include <setupapi.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(msports);

#include "internal.h"
#include "resource.h"
