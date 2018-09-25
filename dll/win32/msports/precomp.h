#ifndef _MSPORTS_PCH_
#define _MSPORTS_PCH_

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>
#include <windowsx.h>
#include <msports.h>
#include <setupapi.h>

#include <wine/debug.h>

#include "internal.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(msports);

#endif /* _MSPORTS_PCH_ */
