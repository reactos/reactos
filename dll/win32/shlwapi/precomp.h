#ifndef _SHLWAPI_PCH_
#define _SHLWAPI_PCH_

#include <wine/config.h>

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <wincon.h>
#include <winternl.h>
#define NO_SHLWAPI_STREAM
#define NO_SHLWAPI_USER
#include <shlwapi.h>
#include <shlobj.h>

#include <wine/debug.h>
#include <wine/unicode.h>

#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#endif /* _SHLWAPI_PCH_ */
